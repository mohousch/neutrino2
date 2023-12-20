/*
	$Id: audio_cs.cpp 2016.06.22 11:57:30 mohousch Exp $
	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>

#include <config.h>

#include <linux/dvb/audio.h>

#include "audio_cs.h"

#include <system/debug.h>


#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#endif

#ifdef USE_OPENGL
#include <OpenThreads/Thread>

#include "dmx_cs.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <ao/ao.h>
}
/* ffmpeg buf 2k */
#define INBUF_SIZE 0x0800
/* my own buf 16k */
#define DMX_BUF_SZ 0x4000

static ao_device *adevice = NULL;
static ao_sample_format sformat;
//
static AVCodecContext *c = NULL;
static AVCodecParameters *p = NULL;
//
extern cAudio *audioDecoder;
extern cDemux *audioDemux;
//
static uint8_t *dmxbuf = NULL;
static int bufpos;
#endif

////
cAudio::cAudio(int num)
{  
	dprintf(DEBUG_INFO, "cAudio::cAudio: num:%d\n", num);

	Muted = false;
	
	audio_fd = -1;	
	audio_adapter = 0;
	audio_num = num;
		
	StreamType = AUDIO_STREAMTYPE_MPEG;

	volume = 0;
	
	m_pcm_delay = -1,
	m_ac3_delay = -1;

#if not defined (__sh__) // dont reset volume on start
	setVolume(100, 100);
#endif

#ifdef USE_OPENGL
	thread_started = false;
	dmxbuf = (uint8_t *)malloc(DMX_BUF_SZ);
	bufpos = 0;
	curr_pts = 0;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif
#endif
}

cAudio::~cAudio(void)
{  
	dprintf(DEBUG_INFO, "cAudio::~cAudio\n");	

	Close();
	
#ifdef USE_OPENGL
	free(dmxbuf);
	if (adevice)
		ao_close(adevice);
		
	adevice = NULL;
#endif
}

bool cAudio::Open(CFrontend * fe)
{ 
#if !defined USE_OPENGL 
	if(fe)
		audio_adapter = fe->feadapter;
	
	char devname[32];

	// Open audio device	
	sprintf(devname, "/dev/dvb/adapter%d/audio%d", audio_adapter, audio_num);
	
	if(audio_fd > 0)
	{
		printf("%s %s already opened\n", __FUNCTION__, devname);
		return true;
	}

	audio_fd = ::open(devname, O_RDWR);

	if(audio_fd > 0)
	{
		dprintf(DEBUG_NORMAL, "cAudio::Open %s\n", devname);
		
		return true;
	}
#endif	

	return false;
}

bool cAudio::Close()
{ 
	dprintf(DEBUG_NORMAL, "cAudio::Close\n");
	
#if !defined USE_OPENGL 
	if (audio_fd < 0)
		return false;
	 
	if (audio_fd >= 0)
	{
		::close(audio_fd);
		audio_fd = -1;	
	}
#endif
	
	return true;
}

// shut up
int cAudio::SetMute(int enable)
{ 
	dprintf(DEBUG_NORMAL, "cAudio::SetMute (%d)\n", enable);	
	
	Muted = enable?true:false;
	
	int ret = 0;	
	
#if !defined USE_OPENGL
#if !defined (__sh__)
	if (audio_fd > 0)
	{
		ret = ::ioctl(audio_fd, AUDIO_SET_MUTE, enable);
	
		if(ret < 0)
			perror("AUDIO_SET_MUTE"); 
	}
#endif

	char sMuted[4];
	sprintf(sMuted, "%d", Muted);

	int fd = ::open("/proc/stb/audio/j1_mute", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, sMuted, strlen(sMuted));
		::close(fd);
	}
#endif

	return ret;
}

/* volume, min = 0, max = 100 */
int cAudio::setVolume(unsigned int left, unsigned int right)
{ 
	dprintf(DEBUG_INFO, "cAudio::setVolume volume: %d\n", left);
	
	int ret = -1;
	
#if !defined USE_OPENGL
	volume = (left + right)/2;
	
	// map volume
	if (volume < 0)
		volume = 0;
	else if (volume > 100)
		volume = 100;

	// convert to -1dB steps
	int _left = 63 - volume * 0.63;
	int _right = 63 - volume * 0.63;
	//now range is 63..0, where 0 is loudest
	
#if !defined (__sh__)
	audio_mixer_t mixer;

	mixer.volume_left = _left;
	mixer.volume_right = _right;
	
	if (audio_fd > 0)
	{
		ret = ::ioctl(audio_fd, AUDIO_SET_MIXER, &mixer);
	
		if(ret < 0)
			perror("AUDIO_SET_MIXER");
	}
#endif	

#if !defined (PLATFORM_HYPERCUBE)
	char sVolume[4];
	
#if defined (__sh__)
	sprintf(sVolume, "%d", _left);
#else
	sprintf(sVolume, "%d", volume);
#endif

	int fd = ::open("/proc/stb/avs/0/volume", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, sVolume, strlen(sVolume));
		::close(fd);
	}
#endif
#endif
	
	return ret;
}

/* start audio */
int cAudio::Start(void)
{ 
	dprintf(DEBUG_INFO, "cAudio::Start\n");
	
	int ret = -1;
	
#ifdef USE_OPENGL
	if (!thread_started)
	{
		ret = OpenThreads::Thread::start();
	}
#else
	if (audio_fd < 0)
		return -1;
	
	ret = ::ioctl(audio_fd, AUDIO_PLAY);
	
	if(ret < 0)
		perror("AUDIO_PLAY");	
#endif

	return ret;
}

int cAudio::Stop(void)
{ 
	dprintf(DEBUG_INFO, "cAudio::Stop\n");
	
	int ret = -1;
	
#ifdef USE_OPENGL
	if (thread_started)
	{
		thread_started = false;
		ret = OpenThreads::Thread::join();
	}
#else
	if (audio_fd < 0)
		return -1;
		
	ret = ::ioctl(audio_fd, AUDIO_STOP);
	
	if(ret < 0)
		perror("AUDIO_STOP");	
#endif

	return ret;
}

bool cAudio::Pause()
{
	dprintf(DEBUG_INFO, "cAudio::Pause\n");
	 
#if !defined USE_OPENGL
	if (audio_fd < 0)
		return false;
	
	if (::ioctl(audio_fd, AUDIO_PAUSE) < 0)
	{
		perror("AUDIO_PAUSE");
		return false;
	}
#endif	

	return true;
}

bool cAudio::Resume()
{ 
	dprintf(DEBUG_INFO, "cAudio::Resume\n");
	
#if !defined USE_OPENGL
	if (audio_fd < 0)
		return false;	

	if(::ioctl(audio_fd, AUDIO_CONTINUE) < 0)
	{
		perror("AUDIO_CONTINUE");
		return false;
	}
#endif	
	
	return true;
}

void cAudio::SetStreamType(AUDIO_FORMAT type)
{
	const char* aAUDIOFORMAT[] = {
		"AUDIO_STREAMTYPE_AC3",
		"AUDIO_STREAMTYPE_MPEG",
		"AUDIO_STREAMTYPE_DTS",
		"AUDIO_STREAMTYPE_LPCMDVD",
		"AUDIO_STREAMTYPE_AAC",
		"AUDIO_STREAMTYPE_AACPLUS",
		"AUDIO_STREAMTYPE_LPCMDVD",
		"AUDIO_STREAMTYPE_MP3",
		"AUDIO_STREAMTYPE_AAC_PLUS",
		"AUDIO_STREAMTYPE_DTSHD",
		"AUDIO_STREAMTYPE_WMA",
		"AUDIO_STREAMTYPE_WMA_PRO",
		"AUDIO_STREAMTYPE_EAC3",
		"AUDIO_STREAMTYPE_AMR",
		"AUDIO_STREAMTYPE_RAW"
	};

	dprintf(DEBUG_INFO, "cAudio::SetStreamType - type=%s\n", aAUDIOFORMAT[type]);
	
#if !defined USE_OPENGL
	if (audio_fd < 0)
		return;
	
	if (::ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, type) < 0)
	{
		perror("AUDIO_SET_BYPASS_MODE");
		return;
	}
#endif	

	StreamType = type;
}

void cAudio::SetSyncMode(int Mode)
{
	dprintf(DEBUG_INFO, "cAudio::SetSyncMode\n");	
	
#if !defined USE_OPENGL
	if (audio_fd < 0)
		return;
	
	if (::ioctl(audio_fd, AUDIO_SET_AV_SYNC, Mode) < 0)
	{
		perror("AUDIO_SET_AV_SYNC");
		return;
	}
#endif	
}

int cAudio::Flush(void)
{  
	dprintf(DEBUG_INFO, "cAudio::Flush\n");
	
	int ret = -1;

#if !defined USE_OPENGL	
	if (audio_fd < 0)
		return -1;

#if defined (__sh__)
	ret = (::ioctl(audio_fd, AUDIO_FLUSH) < 0);
#else
	ret = ::ioctl(audio_fd, AUDIO_CLEAR_BUFFER);
#endif

	if(ret < 0)
		perror("AUDIO_FLUSH");	
#endif
	
	return ret;
}

/* select channels */
int cAudio::setChannel(int channel)
{
	const char * aAUDIOCHANNEL[] = {
		"STEREO",
		"MONOLEFT",
		"MONORIGHT",
	};
	 
	dprintf(DEBUG_INFO, "cAudio::setChannel: %s\n", aAUDIOCHANNEL[channel]);
	
	int ret = -1;

#if !defined (USE_OPENGL)
	if (audio_fd < 0)
		return -1;

	ret = ::ioctl(audio_fd, AUDIO_CHANNEL_SELECT, (audio_channel_select_t)channel);
		perror("AUDIO_CHANNEL_SELECT");	
#endif
	
	return ret;
}

void cAudio::SetHdmiDD(int ac3)
{
	const char *aHDMIDD[] = {
		"downmix",
		"passthrough"
	};
	
	dprintf(DEBUG_NORMAL, "cAudio::SetHdmiDD: %s\n", aHDMIDD[ac3]);	

#if !defined (USE_OPENGL)
#if defined (__sh__)
	const char *aHDMIDDSOURCE[] = {
		"pcm",
		"spdif",
		"8ch",
		"none"
	};
	
	int fd = ::open("/proc/stb/hdmi/audio_source", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aHDMIDDSOURCE[ac3], strlen(aHDMIDDSOURCE[ac3]));
		::close(fd);
	}
#endif

	int fd_ac3 = ::open("/proc/stb/audio/ac3", O_RDWR);
	
	if(fd_ac3 > 0)
	{
		write(fd_ac3, aHDMIDD[ac3], strlen(aHDMIDD[ac3]));
		::close(fd_ac3);
	}
#endif	
}

/* set source */
int cAudio::setSource(int source)
{ 
	int ret = -1;
	
	const char *aAUDIOSTREAMSOURCE[] = {
		"AUDIO_SOURCE_DEMUX",
		"AUDIO_SOURCE_MEMORY",
		"AUDIO_SOURCE_HDMI"
	};
		
	dprintf(DEBUG_INFO, "cAudio::setSource: - source=%s\n", aAUDIOSTREAMSOURCE[source]);
	
#if !defined USE_OPENGL	
	if (audio_fd < 0)
		return -1;

	ret = ::ioctl(audio_fd, AUDIO_SELECT_SOURCE, source);
#endif

	return ret;
}

int cAudio::setHwPCMDelay(int delay)
{  
	dprintf(DEBUG_INFO, "cAudio::setHwPCMDelay: delay=%d\n", delay);
	
#if !defined (USE_OPENGL)	
	if (delay != m_pcm_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_pcm", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_pcm_delay = delay;
			return 0;
		}
	}
#endif	
	
	return -1;
}

int cAudio::setHwAC3Delay(int delay)
{
	dprintf(DEBUG_INFO, "cAudio::setHwAC3Delay: delay=%d\n", delay);
	
#if !defined (USE_OPENGL)	
	if ( delay != m_ac3_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_bitstream", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_ac3_delay = delay;
			return 0;
		}
	}
#endif	
	
	return -1;
}

#ifdef USE_OPENGL
static int my_read(void *, uint8_t *buf, int buf_size)
{
	int tmp = 0;
	
	//
	if (audioDecoder && audioDemux && bufpos < DMX_BUF_SZ - 4096)
	{
		while (bufpos < buf_size && ++tmp < 20)   // retry max 20 times
		{
			int ret = audioDemux->Read(dmxbuf + bufpos, DMX_BUF_SZ - bufpos, 10);
			
			if (ret > 0)
				bufpos += ret;
		}
	}
	
	if (bufpos == 0)
		return 0;
	
	if (bufpos > buf_size)
	{
		memcpy(buf, dmxbuf, buf_size);
		memmove(dmxbuf, dmxbuf + buf_size, bufpos - buf_size);
		bufpos -= buf_size;
		
		return buf_size;
	}
	
	memcpy(buf, dmxbuf, bufpos);
	tmp = bufpos;
	bufpos = 0;
	
	return tmp;
}

void cAudio::run()
{
	dprintf(DEBUG_NORMAL, "cAudio::run: START\n");

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
	const AVCodec *codec;
	const AVInputFormat *inp;
#else
	AVCodec *codec;
	AVInputFormat *inp;
#endif	
	AVFormatContext *avfc = NULL;
	AVFrame *frame;
	uint8_t *inbuf = (uint8_t *)av_malloc(INBUF_SIZE);
	AVPacket avpkt;
	int ret, driver;
	int av_ret = 0;
	// libao
	ao_info *ai;
	// resample
	SwrContext *swr = NULL;
	uint8_t *obuf = NULL;
	int obuf_sz = 0; 	// in samples
	int obuf_sz_max = 0;
	int o_ch, o_sr; 	// output channels and sample rate
	uint64_t o_layout; 	// output channels layout
	char tmp[64] = "unknown";

	curr_pts = 0;
	
	//
	av_init_packet(&avpkt);
	
	//
	inp = av_find_input_format("mpegts");
	
	AVIOContext *pIOCtx = avio_alloc_context(inbuf, INBUF_SIZE, // internal Buffer and its size
	        0,      	// bWriteable (1=true,0=false)
	        NULL,       	// user data; will be passed to our callback functions
	        my_read,   	// read callback
	        NULL,       	// write callback
	        NULL);      	// seek callback

	//	        
	avfc = avformat_alloc_context();
	avfc->pb = pIOCtx;
	avfc->iformat = inp;
	avfc->probesize = 188 * 5;
	
	thread_started = true;

	if (avformat_open_input(&avfc, NULL, inp, NULL) < 0)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: avformat_open_input() failed.\n");
		goto out;
	}
	
	ret = avformat_find_stream_info(avfc, NULL);
	dprintf(DEBUG_NORMAL, "cAudio::run: avformat_find_stream_info: %d\n", ret);
	
	if (avfc->nb_streams != 1)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: nb_streams: %d, should be 1!\n", avfc->nb_streams);
		goto out;
	}
	
	p = avfc->streams[0]->codecpar;
	
	if (p->codec_type != AVMEDIA_TYPE_AUDIO)
		printf("cAudio::run: stream 0 no audio codec? 0x%x\n", p->codec_type);

	codec = avcodec_find_decoder(p->codec_id);
	
	if (!codec)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: Codec for %s not found\n", avcodec_get_name(p->codec_id));
		goto out;
	}
	
	dprintf(DEBUG_NORMAL, "cAudio::run: decoding %s\n", avcodec_get_name(p->codec_id));
	
	if (c)
		av_free(c);
		
	c = avcodec_alloc_context3(codec);
	
	if (avcodec_open2(c, codec, NULL) < 0)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: avcodec_open2() failed\n");
		goto out;
	}
	
	if (p->sample_rate == 0 || p->channels == 0)
	{
		av_get_sample_fmt_string(tmp, sizeof(tmp), c->sample_fmt);
		
		dprintf(DEBUG_NORMAL, "cAudio::run: Header missing %s, sample_fmt %d (%s) sample_rate %d channels %d\n", avcodec_get_name(p->codec_id), c->sample_fmt, tmp, p->sample_rate, p->channels);
		
		goto out2;
	}
	
	frame = av_frame_alloc();
	
	if (!frame)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: av_frame_alloc failed\n");
		goto out2;
	}
	
	// output sample rate, channels, layout could be set here if necessary 
	o_ch = p->channels;     	// 2
	o_sr = p->sample_rate;      	// 48000
	o_layout = p->channel_layout;   // AV_CH_LAYOUT_STEREO
	
	if (sformat.channels != o_ch || sformat.rate != o_sr || sformat.byte_format != AO_FMT_NATIVE || sformat.bits != 16 || adevice == NULL)
	{
		driver = ao_default_driver_id();
		sformat.bits = 16;
		sformat.channels = o_ch;
		sformat.rate = o_sr;
		sformat.byte_format = AO_FMT_NATIVE;
		sformat.matrix = 0;
		
		if (adevice)
			ao_close(adevice);
			
		adevice = ao_open_live(driver, &sformat, NULL);
		ai = ao_driver_info(driver);
		
		dprintf(DEBUG_NORMAL, "cAudio::run: changed params ch %d srate %d bits %d adevice %p\n", o_ch, o_sr, 16, adevice);
	}

	av_get_sample_fmt_string(tmp, sizeof(tmp), c->sample_fmt);
	
	dprintf(DEBUG_NORMAL, "cAudio::run: decoding %s, sample_fmt %d (%s) sample_rate %d channels %d\n", avcodec_get_name(p->codec_id), c->sample_fmt, tmp, p->sample_rate, p->channels);
	
	swr = swr_alloc_set_opts(swr,
	        o_layout, AV_SAMPLE_FMT_S16, o_sr,         		// output
	        p->channel_layout, c->sample_fmt, p->sample_rate,  	// input
	        0, NULL);
	        
	if (! swr)
	{
		dprintf(DEBUG_NORMAL, "cAudio::run: could not alloc resample context\n");
		goto out3;
	}
	
	swr_init(swr);
	
	while (thread_started)
	{
		int gotframe = 0;
		
		if (av_read_frame(avfc, &avpkt) < 0)
			break;
			
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		avcodec_decode_audio4(c, frame, &gotframe, &avpkt);
#else
		av_ret = avcodec_send_packet(c, &avpkt);
		
		if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
		{
			//hal_info("%s: avcodec_send_packet %d\n", __func__, av_ret);
		}
		else
		{
			av_ret = avcodec_receive_frame(c, frame);
			
			if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
			{
				//hal_info("%s: avcodec_send_packet %d\n", __func__, av_ret);
			}
			else
			{
				gotframe = 1;
			}
		}
#endif

		if (gotframe && thread_started)
		{
			int out_linesize;
			obuf_sz = av_rescale_rnd(swr_get_delay(swr, p->sample_rate) + frame->nb_samples, o_sr, p->sample_rate, AV_ROUND_UP);

			if (obuf_sz > obuf_sz_max)
			{
				av_free(obuf);
				
				if (av_samples_alloc(&obuf, &out_linesize, o_ch, frame->nb_samples, AV_SAMPLE_FMT_S16, 1) < 0)
				{
					av_packet_unref(&avpkt);
					break; // while (thread_started)
				}
				
				obuf_sz_max = obuf_sz;
			}
			
			obuf_sz = swr_convert(swr, &obuf, obuf_sz, (const uint8_t **)frame->extended_data, frame->nb_samples);
#if (LIBAVUTIL_VERSION_MAJOR < 54)
			curr_pts = av_frame_get_best_effort_timestamp(frame);
#else
			curr_pts = frame->best_effort_timestamp;
#endif
			int o_buf_sz = av_samples_get_buffer_size(&out_linesize, o_ch, obuf_sz, AV_SAMPLE_FMT_S16, 1);
			
			if (o_buf_sz > 0)
				ao_play(adevice, (char *)obuf, o_buf_sz);
		}
		
		av_packet_unref(&avpkt);
	}
	// ao_close(adevice); /* can take long :-(*/
	av_free(obuf);
	swr_free(&swr);
out3:
	av_frame_free(&frame);
out2:
	avcodec_close(c);
	av_free(c);
	c = NULL;
out:
	avformat_close_input(&avfc);
	av_free(pIOCtx->buffer);
	av_free(pIOCtx);
	
	dprintf(DEBUG_NORMAL, "cAudio::run: END\n");
}
#endif

