//
//      Neutrino-GUI  -   DBoxII-Project
//
//      Copyright (C) 2011-2012 CoolStream International Ltd
//
//	based on code which is
//	Copyright (C) 2002 Andreas Oberritter <obi@tuxbox.org>
//	Copyright (C) 2001 TripleDES
//	Copyright (C) 2000, 2001 Marcus Metzler <marcus@convergence.de>
//
//      License: GPLv2
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation;
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <poll.h>
#include <syscall.h>

#include <global.h>
#include <neutrino2.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>

#include <string.h>

#include <zapit/pmt.h>
#include <zapit/zapit.h>
#include <zapit/pat.h>
#include <driver/streamts.h>
#include <driver/record.h>
#include <driver/genpsi.h>
#include <system/set_threadname.h>
#include <gui/movieplayer.h>


#if (LIBAVCODEC_VERSION_MAJOR > 55)
#define	av_free_packet av_packet_unref
#else
#define av_packet_unref	av_free_packet
#endif

/* experimental mode:
 * stream not possible, if record running
 * pids in url ignored, and added from channel, with fake PAT/PMT
 * different channels supported,
 * with url like http://coolstream:31339/id=c32400030070283e (channel id)
 */
#define ENABLE_MULTI_CHANNEL

#define TS_SIZE 188
#define DMX_BUFFER_SIZE (5*2048*TS_SIZE)
#define IN_SIZE (250*TS_SIZE)

CStreamInstance::CStreamInstance(int clientfd, t_channel_id chid, stream_pids_t &_pids)
{
	printf("CStreamInstance:: new channel %" PRIx64 " fd %d\n", chid, clientfd);
	
	fds.insert(clientfd);
	pids = _pids;
	channel_id = chid;
	running = false;
	dmx = NULL;
	buf = NULL;
	frontend = NULL;
}

CStreamInstance::~CStreamInstance()
{
	Stop();
	Close();
}

bool CStreamInstance::Start()
{
	if (running)
		return false;

	buf =  new unsigned char [IN_SIZE];
	if (buf == NULL)
	{
		perror("CStreamInstance::Start: buf");
		return false;
	}
	running = true;
	
	printf("CStreamInstance::Start: %" PRIx64 "\n", channel_id);
	
	return (OpenThreads::Thread::start() == 0);
}

bool CStreamInstance::Stop()
{
	if (!running)
		return false;

	printf("CStreamInstance::Stop: %" PRIx64 "\n", channel_id);
	
	running = false;
	
	return (OpenThreads::Thread::join() == 0);
}

bool CStreamInstance::Send(ssize_t r, unsigned char *_buf)
{
	stream_fds_t cfds;
	mutex.lock();
	cfds = fds;
	mutex.unlock();
	int flags = 0;
	if (cfds.size() > 1)
		flags = MSG_DONTWAIT;
		
	for (stream_fds_t::iterator it = cfds.begin(); it != cfds.end(); ++it)
	{
		int i = 10;
		unsigned char *b = _buf ? _buf : buf;
		ssize_t count = r;
		do
		{
			int ret = send(*it, b, count, flags);
			if (ret > 0)
			{
				b += ret;
				count -= ret;
			}
		}
		while ((count > 0) && (i-- > 0));
		
		if (count)
			printf("send err, fd %d: (%zd from %zd)\n", *it, r - count, r);
	}
	
	return true;
}

void CStreamInstance::Close()
{
	for (stream_fds_t::iterator fit = fds.begin(); fit != fds.end(); ++fit)
		close(*fit);
	fds.clear();
}

void CStreamInstance::AddClient(int clientfd)
{
	fds.insert(clientfd);
	printf("CStreamInstance::AddClient: %d (count %d)\n", clientfd, (int)fds.size());
}

void CStreamInstance::RemoveClient(int clientfd)
{
	fds.erase(clientfd);
	close(clientfd);
	printf("CStreamInstance::RemoveClient: %d (count %d)\n", clientfd, (int)fds.size());
}

bool CStreamInstance::Open()
{
	CZapitChannel *tmpchan = CZapit::getInstance()->findChannelByChannelID(channel_id);
	if (!tmpchan)
		return false;

	dmx = new cDemux();
	if(!dmx)
		return false;
		
	return dmx->Open(DMX_TP_CHANNEL, DMX_BUFFER_SIZE, frontend);
}

void CStreamInstance::run()
{
	set_threadname("CStreamInstance::run");
	printf("CStreamInstance::run: %" PRIx64 "\n", channel_id);
	set_threadname("n:streaminstance");

	// pids here cannot be empty
	stream_pids_t::iterator it = pids.begin();
	
	printf("CStreamInstance::run: add pid %x\n", *it);
	
	dmx->pesFilter(*it);
	++it;
	
	for (; it != pids.end(); ++it)
	{
		printf("CStreamInstance::run: add pid %x\n", *it);
		dmx->addPid(*it);
	}

	dmx->Start();

	while (running)
	{
		ssize_t r = dmx->Read(buf, IN_SIZE, 100);
		if (r > 0)
			Send(r);
	}

	printf("CStreamInstance::run: exiting %" PRIx64 " (%d fds)\n", channel_id, (int)fds.size());

	Close();
	delete dmx;
	delete []buf;
}

bool CStreamInstance::HasFd(int fd)
{
	if (fds.find(fd) != fds.end())
		return true;
	return false;
}

////
CStreamManager *CStreamManager::sm = NULL;
CStreamManager::CStreamManager()
{
	enabled = true;
	running = false;
	listenfd = -1;
	port = 31339;
}

CStreamManager::~CStreamManager()
{
	Stop();
}

CStreamManager *CStreamManager::getInstance()
{
	if (sm == NULL)
		sm = new CStreamManager();
	return sm;
}

bool CStreamManager::Start(int _port)
{
	if (running)
		return false;

	if (_port)
		port = _port;
	if (!Listen())
		return false;

	running = true;
	return (OpenThreads::Thread::start() == 0);
}

bool CStreamManager::Stop()
{
	if (!running)
		return false;
	running = false;
	bool ret = false;
	
	if (OpenThreads::Thread::CurrentThread() == this)
	{
		cancel();
		ret = (OpenThreads::Thread::join() == 0);
	}
	
	StopAll();
	
	return ret;
}

bool CStreamManager::SetPort(int newport)
{
	bool ret = false;
	
	if (port != newport)
	{
		port = newport;
		mutex.lock();
		if (listenfd >= 0)
			close(listenfd);
		ret = Listen();
		mutex.unlock();
	}
	
	return ret;
}

// FIXME: revise
CFrontend *CStreamManager::FindFrontend(CZapitChannel *channel)
{
	dprintf(DEBUG_NORMAL, "CFrontend *CStreamManager::FindFrontend\n");
	
	std::set<CFrontend *> frontends;
	CFrontend *frontend = NULL;

	t_channel_id chid = channel->getChannelID();
	
	//
	if (chid == CZapit::getInstance()->getCurrentChannelID())
		return CZapit::getInstance()->getCurrentFrontend();

	// lock live frontend
	CZapit::getInstance()->lockFrontend(CZapit::getInstance()->getCurrentFrontend());

	// lock running instance frontend
	for (streammap_iterator_t it = streams.begin(); it != streams.end(); ++it)
		frontends.insert(it->second->frontend);

	for (std::set<CFrontend *>::iterator ft = frontends.begin(); ft != frontends.end(); ++ft)
		CZapit::getInstance()->lockFrontend(*ft);

	frontend = CZapit::getInstance()->getFrontend(channel);

	if (frontend) 
	{

		bool ret = false;

		ret = (CZapit::getInstance()->zapToRecordID(chid) == 0);

		if (!ret)
		{
			frontend = NULL;
		}
	}

	// unlock running instance frontend
	for (std::set<CFrontend *>::iterator ft = frontends.begin(); ft != frontends.end(); ++ft)
		CZapit::getInstance()->unlockFrontend(*ft);

	// unlock live frontend
	CZapit::getInstance()->unlockFrontend(CZapit::getInstance()->getCurrentFrontend());

	return frontend;
}

bool CStreamManager::Parse(int fd, stream_pids_t &pids, t_channel_id &chid, CFrontend *&frontend)
{
	char cbuf[512];
	char *bp;

	FILE *fp = fdopen(fd, "r+");
	if (fp == NULL)
	{
		perror("fdopen");
		return false;
	}
	cbuf[0] = 0;
	bp = &cbuf[0];

	// read one line
	while (bp - &cbuf[0] < (int) sizeof(cbuf) - 1)
	{
		unsigned char c;
		int res = read(fd, &c, 1);
		if (res == 0)
			break;
		if (res < 0)
		{
			perror("read");
			return false;
		}
		if ((*bp++ = c) == '\n')
			break;
	}
	*bp = 0;

	printf("CStreamManager::Parse: got %d bytes '%s'", (int)(bp - &cbuf[0]), cbuf);
	bp = &cbuf[0];

	// send response to http client
	if (!strncmp(cbuf, "GET /", 5))
	{
		fprintf(fp, "HTTP/1.1 200 OK\r\nServer: streamts (%s)\r\n\r\n", "ts");
		fflush(fp);
		bp += 5;
	}
	else
	{
		printf("Received garbage\n");
		return false;
	}

	chid = CZapit::getInstance()->getCurrentChannelID();
	CZapitChannel *channel = CZapit::getInstance()->getCurrentChannel();

#ifndef ENABLE_MULTI_CHANNEL
	// parse stdin / url path, start dmx filters
	do
	{
		int pid;
		int res = sscanf(bp, "%x", &pid);
		if (res == 1)
		{
			printf("CStreamManager::Parse: pid: 0x%x\n", pid);
			pids.insert(pid);
		}
	}
	while ((bp = strchr(bp, ',')) && (bp++));
#else
	t_channel_id tmpid;
	bp = &cbuf[5];
	
	if (sscanf(bp, "id=%" SCNx64, &tmpid) == 1)
	{
		channel = CZapit::getInstance()->findChannelByChannelID(tmpid);
		chid = tmpid;
		pids.clear(); // to catch and stream all pids later !
	}
#endif

	if (!channel)
	{
		printf("CStreamManager::Parse: no channel\n");
		return false;
	}

	printf("CStreamManager::Parse: channel_id %" PRIx64 " [%s]\n", chid, channel->getName().c_str());
	
	if (IS_WEBTV(chid))
		return true;

	// allocate frontend / zap
	frontend = FindFrontend(channel);
	
	if (!frontend)
	{
		printf("CStreamManager::Parse: no free frontend\n");
		return false;
	}

	// add pids
	AddPids(fd, channel, pids);

	return !pids.empty();
}

void CStreamManager::AddPids(int fd, CZapitChannel *channel, stream_pids_t &pids)
{
	dprintf(DEBUG_NORMAL, "CStreamManager::AddPids\n");
	
	if (pids.empty())
	{
		printf("CStreamManager::AddPids: no pids in url, using channel %" PRIx64 " pids\n", channel->getChannelID());
		
		if (channel->getVideoPid())
			pids.insert(channel->getVideoPid());
			
		for (int i = 0; i <  channel->getAudioChannelCount(); i++)
			pids.insert(channel->getAudioChannel(i)->pid);

	}

	CGenPsi psi;
	for (stream_pids_t::iterator it = pids.begin(); it != pids.end(); ++it)
	{
		if (*it == channel->getVideoPid())
		{
			printf("CStreamManager::AddPids: genpsi vpid %x (%d)\n", *it, channel->videoType);
			psi.addPid(*it, channel->videoType == CZapitChannel::CHANNEL_VIDEO_MPEG4 ? EN_TYPE_AVC : channel->videoType == CZapitChannel::CHANNEL_VIDEO_HEVC ? EN_TYPE_HEVC : EN_TYPE_VIDEO, 0);
		}
		else
		{
			for (int i = 0; i <  channel->getAudioChannelCount(); i++)
			{
				if (*it == channel->getAudioChannel(i)->pid)
				{
					CZapitAudioChannel::ZapitAudioChannelType atype = channel->getAudioChannel(i)->audioChannelType;
					printf("CStreamManager::AddPids: genpsi apid %x (%d)\n", *it, atype);
					if (channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::EAC3)
					{
						psi.addPid(*it, EN_TYPE_AUDIO_EAC3, 0, channel->getAudioChannel(i)->description.c_str());
					}
					else if (channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AAC)
					{
						psi.addPid(*it, EN_TYPE_AUDIO_AAC, 0, channel->getAudioChannel(i)->description.c_str());
					}
					else if (channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACHE)
					{
						psi.addPid(*it, EN_TYPE_AUDIO_AACP, 0, channel->getAudioChannel(i)->description.c_str());
					}
					else
					{
						psi.addPid(*it, EN_TYPE_AUDIO, (atype == CZapitAudioChannel::AC3), channel->getAudioChannel(i)->description.c_str());
					}
				}
			}
		}
	}
	
	//add pcr pid
	if (channel->getPcrPid() && (channel->getPcrPid() != channel->getVideoPid()))
	{
		pids.insert(channel->getPcrPid());
		psi.addPid(channel->getPcrPid(), EN_TYPE_PCR, 0);
	}
	
	//add teletext pid
	if (channel->getTeletextPid() != 0)
	{
		pids.insert(channel->getTeletextPid());
		psi.addPid(channel->getTeletextPid(), EN_TYPE_TELTEX, 0, channel->getTeletextLang());
	}
	
	//add dvb sub pid
	if (channel->getSubtitleCount())
	{
		for (int i = 0 ; i < (int)channel->getSubtitleCount() ; ++i)
		{
			CZapitAbsSub *s = channel->getChannelSub(i);
			if (s->thisSubType == CZapitAbsSub::DVB)
			{
				if (i > 9) //max sub pids
					break;

				CZapitDVBSub *sd = reinterpret_cast<CZapitDVBSub *>(s);
				pids.insert(sd->pId);
				psi.addPid(sd->pId, EN_TYPE_DVBSUB, 0, sd->ISO639_language_code.c_str());
			}
		}
	}

	psi.genpsi(fd);
}

bool CStreamManager::AddClient(int connfd)
{
	stream_pids_t pids;
	t_channel_id channel_id;
	CFrontend *frontend;

	if (Parse(connfd, pids, channel_id, frontend))
	{
		streammap_iterator_t it = streams.find(channel_id);
		
		if (it != streams.end())
		{
			it->second->AddClient(connfd);
		}
		else
		{
			CStreamInstance *stream;
			
			if (IS_WEBTV(channel_id))
			{
				stream = new CStreamStream(connfd, channel_id, pids);
			}
			else
			{
				stream = new CStreamInstance(connfd, channel_id, pids);
				stream->frontend = frontend;
			}

			int sendsize = 10 * IN_SIZE;
			unsigned int m = sizeof(sendsize);
			setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (void *)&sendsize, m);
			
			if (stream->Open() && stream->Start())
				streams.insert(streammap_pair_t(channel_id, stream));
			else
				delete stream;
		}
		
		return true;
	}
	
	return false;
}

void CStreamManager::RemoveClient(int fd)
{
	for (streammap_iterator_t it = streams.begin(); it != streams.end(); ++it)
	{
		if (it->second->HasFd(fd))
		{
			CStreamInstance *stream = it->second;
			stream->RemoveClient(fd);
			if (stream->GetFds().empty())
			{
				streams.erase(stream->GetChannelId());
				delete stream;
			}
			break;
		}
	}
}

void CStreamManager::run()
{
	set_threadname("CStreamManager::run");
	struct sockaddr_in servaddr;
	int clilen = sizeof(servaddr);

	struct pollfd pfd[128];
	int poll_cnt;
	int poll_timeout = -1;

	printf("CStreamManager::run: Starting STREAM thread keeper, tid %ld\n", syscall(__NR_gettid));
	set_threadname("n:streammanager");

	while (running)
	{
		mutex.lock();
		pfd[0].fd = listenfd;
		pfd[0].events = (POLLIN | POLLPRI);
		pfd[0].revents = 0;
		poll_cnt = 1;
		
		for (streammap_iterator_t it = streams.begin(); it != streams.end(); ++it)
		{
			stream_fds_t fds = it->second->GetFds();
			
			for (stream_fds_t::iterator fit = fds.begin(); fit != fds.end(); ++fit)
			{
				pfd[poll_cnt].fd = *fit;
				pfd[poll_cnt].events = POLLRDHUP | POLLHUP;
				pfd[poll_cnt].revents = 0;
				poll_cnt++;
			}
		}
		
		mutex.unlock();
		
		int pollres = poll(pfd, poll_cnt, poll_timeout);
		if (pollres <= 0)
		{
			if (pollres < 0)
				perror("CStreamManager::run(): poll");
			continue;
		}
		for (int i = poll_cnt - 1; i >= 0; i--)
		{
			if (pfd[i].revents & (POLLIN | POLLPRI | POLLHUP | POLLRDHUP))
			{
				printf("fd %d has events %x\n", pfd[i].fd, pfd[i].revents);
				if (pfd[i].fd == listenfd)
				{
					int connfd = accept(listenfd, (struct sockaddr *) &servaddr, (socklen_t *) & clilen);
					printf("CStreamManager::run(): connection, fd %d\n", connfd);
					if (connfd < 0)
					{
						perror("CStreamManager::run(): accept");
						continue;
					}
					g_RCInput->postMsg(NeutrinoMessages::EVT_STREAM_START, (const neutrino_msg_data_t)connfd);
					poll_timeout = 1000;
				}
				else
				{
					if (pfd[i].revents & (POLLHUP | POLLRDHUP))
					{
						printf("CStreamManager::run(): POLLHUP, fd %d\n", pfd[i].fd);
						RemoveClient(pfd[i].fd);
						if (streams.empty())
						{
							poll_timeout = 2000;
							g_RCInput->postMsg(NeutrinoMessages::EVT_STREAM_STOP);
						}
					}
				}
			}
		}
	}
	
	printf("CStreamManager::run: stopping...\n");
	
	close(listenfd);
	listenfd = -1;
	StopAll();
}

bool CStreamManager::StopAll()
{
	bool ret = !streams.empty();
	for (streammap_iterator_t it = streams.begin(); it != streams.end(); ++it)
	{
		it->second->Stop();
		delete it->second;
	}
	streams.clear();
	return ret;
}

bool CStreamManager::StopStream(t_channel_id channel_id)
{
	bool ret = false;
	
	mutex.lock();
	if (channel_id)
	{
		streammap_iterator_t it = streams.find(channel_id);
		if (it != streams.end())
		{
			delete it->second;
			streams.erase(channel_id);
			ret = true;
		}
	}
	else
	{
		ret = StopAll();
	}
	mutex.unlock();
	
	return ret;
}

bool CStreamManager::StopStream(CFrontend *fe)
{
	bool ret = false;
	for (streammap_iterator_t it = streams.begin(); it != streams.end();)
	{
		if (it->second->frontend == fe)
		{
			delete it->second;
			streams.erase(it++);
			ret = true;
		}
		else
		{
			++it;
		}
	}
	
	return ret;
}

bool CStreamManager::StreamStatus(t_channel_id channel_id)
{
	bool ret;
	mutex.lock();
	if (channel_id)
		ret = (streams.find(channel_id) != streams.end());
	else
		ret = !streams.empty();
	mutex.unlock();
	
	return ret;
}

bool CStreamManager::Listen()
{
	struct sockaddr_in socketAddr;
	int socketOptActive = 1;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "network port %u open: ", port);
		perror("socket");
		return false;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&socketOptActive, sizeof(int)) < 0)
	{
		fprintf(stderr, "network port %u open: error setsockopt\n", port);
		perror("setsockopt");
		goto _error;
	}

	socketAddr.sin_family = AF_INET;
	socketAddr.sin_port = htons(port);
	socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *) &socketAddr, sizeof(socketAddr)) < 0)
	{
		fprintf(stderr, "network port %u open: ", port);
		perror("bind");
		goto _error;
	}

	if (listen(listenfd, 5) < 0)
	{
		fprintf(stderr, "network port %u open: ", port);
		perror("listen");
		goto _error;
	}

	printf("CStreamManager::Listen: on %d, fd %d\n", port, listenfd);
	return true;
_error:
	close(listenfd);
	return false;
}

////
CStreamStream::CStreamStream(int clientfd, t_channel_id chid, stream_pids_t &_pids)
	: CStreamInstance(clientfd, chid, _pids)
{
	ifcx = NULL;
	ofcx = NULL;
	bsfc = NULL;
	avio_ctx = NULL;
	stopped = true;
	interrupt = false;
}

CStreamStream::~CStreamStream()
{
	Stop();
	Close();
}

int CStreamStream::write_packet(void *opaque, uint8_t *buffer, int buf_size)
{
	CStreamStream *st = (CStreamStream *) opaque;
	st->Send(buf_size, buffer);
	return buf_size;
}

int CStreamStream::Interrupt(void *data)
{
	CStreamStream *sr = (CStreamStream *) data;
	if (sr->interrupt)
		return 1;
	return 0;
}

void CStreamStream::Close()
{
	if (ifcx)
		avformat_close_input(&ifcx);

	if (ofcx)
		avformat_free_context(ofcx);

	if (buf)
		av_freep(&buf);

	if (avio_ctx)
		av_free(avio_ctx);

	if (bsfc)
	{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,100)
		av_bitstream_filter_close(bsfc);
#else
		av_bsf_free(&bsfc);
#endif
	}

	ifcx = NULL;
	ofcx = NULL;
	bsfc = NULL;
	avio_ctx = NULL;
}

bool CStreamStream::Open()
{
	CZapitChannel *channel = CZapit::getInstance()->findChannelByChannelID(channel_id);
	
	if (!channel)
		return false;

	std::string url = channel->getUrl();

	if (url.empty())
		return false;

	std::string pretty_name, livestreamInfo1, livestreamInfo2, headers, dumb;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
	avcodec_register_all();
#endif
	avformat_network_init();

	dprintf(DEBUG_INFO, "Open input [%s]....\n", url.c_str());

	AVDictionary *options = NULL;
	av_dict_set(&options, "auth_type", "basic", 0);
	
	if (!headers.empty())
	{
		headers += "\r\n";
		av_dict_set(&options, "headers", headers.c_str(), 0);
	}

	if (avformat_open_input(&ifcx, url.c_str(), NULL, &options) != 0)
	{
		dprintf(DEBUG_INFO, "Cannot open input [%s]!\n", channel->getUrl().c_str());
		av_dict_free(&options);
		
		return false;
	}

	av_dict_free(&options);

	if (avformat_find_stream_info(ifcx, NULL) < 0)
	{
		dprintf(DEBUG_INFO, "Cannot find stream info [%s]!\n", channel->getUrl().c_str());
		return false;
	}
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58,27,102)
	const char *hls = "applehttp";
#else
	const char *hls = "hls";
#endif
	if (!strstr(ifcx->iformat->name, hls) &&
		!strstr(ifcx->iformat->name, "mpegts") &&
		!strstr(ifcx->iformat->name, "matroska") &&
		!strstr(ifcx->iformat->name, "avi") &&
		!strstr(ifcx->iformat->name, "mp4"))
	{
		dprintf(DEBUG_INFO, "not supported format [%s]!\n", ifcx->iformat->name);
		return false;
	}

	AVIOInterruptCB int_cb = { Interrupt, this };
	ifcx->interrupt_callback = int_cb;

#if (LIBAVFORMAT_VERSION_MAJOR < 58)
	snprintf(ifcx->filename, sizeof(ifcx->filename), "%s", channel->getUrl().c_str());
	av_dump_format(ifcx, 0, ifcx->filename, 0);
#else
	av_dump_format(ifcx, 0, ifcx->url, 0);
#endif

	buf = (unsigned char *) av_malloc(IN_SIZE);
	if (buf == NULL)
	{
		perror("CStreamStream::Open: buf");
		return false;
	}
	
	avio_ctx = avio_alloc_context(buf, IN_SIZE, 1, this, NULL, &write_packet, NULL);
	if (!avio_ctx)
	{
		printf("%s: avio_alloc_context failed\n", __FUNCTION__);
		return false;
	}

	if (avformat_alloc_output_context2(&ofcx, NULL, "mpegts", NULL) < 0)
	{
		printf("%s: avformat_alloc_output_context2 failed\n", __FUNCTION__);
		return false;
	}
	ofcx->pb = avio_ctx;

	av_dict_copy(&ofcx->metadata, ifcx->metadata, 0);
	int stid = 0x200;
	for (unsigned i = 0; i < ifcx->nb_streams; i++)
	{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
		AVCodecContext *iccx = ifcx->streams[i]->codec;
		AVStream *ost = avformat_new_stream(ofcx, iccx->codec);
		avcodec_copy_context(ost->codec, iccx);
#else
		AVCodecParameters *iccx = ifcx->streams[i]->codecpar;
		AVStream *ost = avformat_new_stream(ofcx, NULL);
		avcodec_parameters_copy(ost->codecpar, iccx);
#endif
		av_dict_copy(&ost->metadata, ifcx->streams[i]->metadata, 0);
		ost->time_base = ifcx->streams[i]->time_base;
		ost->id = stid++;
	}

#if (LIBAVFORMAT_VERSION_MAJOR < 58)
	av_dump_format(ofcx, 0, ofcx->filename, 1);
#else
	av_dump_format(ofcx, 0, ofcx->url, 1);
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,100)
	bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	if (!bsfc)
		ng_err("av_bitstream_filter_init h264_mp4toannexb failed!\n");
#else
	const AVBitStreamFilter *bsf = av_bsf_get_by_name("h264_mp4toannexb");
	if (!bsf)
	{
		return false;
	}
	
	if ((av_bsf_alloc(bsf, &bsfc)))
	{
		return false;
	}
#endif
	return true;
}

bool CStreamStream::Start()
{
	if (!stopped)
		return false;

	printf("%s: Starting...\n", __FUNCTION__);
	stopped = false;
	
	int ret = start();
	
	return (ret == 0);
}

bool CStreamStream::Stop()
{
	if (stopped)
		return false;

	printf("%s: Stopping...\n", __FUNCTION__);
	interrupt = true;
	stopped = true;
	
	int ret = join();
	
	interrupt = false;
	
	return (ret == 0);
}

void CStreamStream::run()
{
	AVPacket pkt;

	printf("%s: Started.\n", __FUNCTION__);
	
	if (avformat_write_header(ofcx, NULL) < 0)
	{
		dprintf(DEBUG_INFO, "avformat_write_header failed\n");
		return;
	}

	while (!stopped)
	{
		av_init_packet(&pkt);
		
		if (av_read_frame(ifcx, &pkt) < 0)
			break;
			
		if (pkt.stream_index < 0)
			continue;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
		AVCodecContext *codec = ifcx->streams[pkt.stream_index]->codec;
#else
		AVCodecParameters *codec = ifcx->streams[pkt.stream_index]->codecpar;
#endif
		if (bsfc && codec->codec_id == AV_CODEC_ID_H264)
		{
			AVPacket newpkt = pkt;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,100)
			if (av_bitstream_filter_filter(bsfc, codec, NULL, &newpkt.data, &newpkt.size, pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY) >= 0)
			{
#if (LIBAVFORMAT_VERSION_MAJOR == 57 && LIBAVFORMAT_VERSION_MINOR == 25)
				av_packet_unref(&pkt);
#else
				av_free_packet(&pkt);
#endif
				newpkt.buf = av_buffer_create(newpkt.data, newpkt.size, av_buffer_default_free, NULL, 0);
				pkt = newpkt;
			}
#else
			int ret = av_bsf_send_packet(bsfc, &pkt);
			if (ret < 0)
			{
				break;
			}
			
			ret = av_bsf_receive_packet(bsfc, &newpkt);
			
			if (ret == AVERROR(EAGAIN))
			{
				break;
			}
			
			if (ret != AVERROR_EOF)
			{
				av_packet_unref(&pkt);
				pkt = newpkt;
			}
#endif
		}
		pkt.pts = av_rescale_q(pkt.pts, ifcx->streams[pkt.stream_index]->time_base, ofcx->streams[pkt.stream_index]->time_base);
		pkt.dts = av_rescale_q(pkt.dts, ifcx->streams[pkt.stream_index]->time_base, ofcx->streams[pkt.stream_index]->time_base);

		av_write_frame(ofcx, &pkt);
#if (LIBAVFORMAT_VERSION_MAJOR == 57 && LIBAVFORMAT_VERSION_MINOR == 25)
		av_packet_unref(&pkt);
#else
		av_free_packet(&pkt);
#endif
	}

	av_read_pause(ifcx);
	av_write_trailer(ofcx);
	
	printf("%s: Stopped.\n", __FUNCTION__);
}

