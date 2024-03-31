/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vcrcontrol.cpp 31.03.2024 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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
#include <config.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <errno.h>
#include <math.h>

#include <libgen.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/encoding.h>

#include <gui/widget/messagebox.h>

#include <daemonc/remotecontrol.h>

//
#include <zapit/zapit.h>
#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>
#include <zapit/channel.h>

#include <sectionsd/abstime.h>

#include <system/helpers.h>
#include <system/debug.h>
#include <system/tmdbparser.h>

#include <driver/vcrcontrol.h>
#include <driver/genpsi.h>

#include <record_cs.h>

//// defines
#define FILENAMEBUFFERSIZE 	1024
#define MAXPIDS			64

//// globals
char rec_filename[FILENAMEBUFFERSIZE];
static CVCRControl vcrControl;
static cRecord * record = NULL;
//
extern bool autoshift;					// defined in neutrino2.cpp
extern CZapitChannel * live_channel;			// defined in zapit.cpp
extern CFrontend * record_fe;				// defined in zapit.cpp

////
CVCRControl * CVCRControl::getInstance()
{
	return &vcrControl;
}

CVCRControl::CVCRControl()
{
	channel_id = 0;
	
	//
	SwitchToScart = false;
	
	//
	g_cMovieInfo = NULL;
	g_movieInfo = NULL;
	
	//
	ifcx = NULL;
	ofcx = NULL;
	stopped = true;
	bsfc = NULL;
}

CVCRControl::~CVCRControl()
{
	channel_id = 0;
	
	if (g_movieInfo)
	{
		delete g_movieInfo;
		g_movieInfo = NULL;
	}
	
	if (g_cMovieInfo)
	{
		delete g_cMovieInfo;
		g_cMovieInfo = NULL;
	}
	
	//
	Stop();
	Close();
}

bool CVCRControl::Record(const CTimerd::RecordingInfo * const eventinfo)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::Record: channel_id:%llx\n", eventinfo->channel_id);
	
	int mode = CNeutrinoApp::getInstance()->getMode();
	
	channel_id = eventinfo->channel_id;

	return doRecord(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->epgTitle, eventinfo->apids, eventinfo->epg_starttime); 
}

void CVCRControl::getAPIDs(const t_channel_id channel_id, const unsigned char ap, APIDList& apid_list)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::getAPIDs\n");
	
        unsigned char apids = ap;

        if (apids == TIMERD_APIDS_CONF)
                apids = g_settings.recording_audio_pids_default;

        apid_list.clear();
        CZapit::responseGetPIDs allpids;
	
	CZapit::getInstance()->getPIDS(channel_id, allpids);

        // assume smallest apid ist std apid
        if (apids & TIMERD_APIDS_STD)
        {
                uint32_t apid_min = UINT_MAX;
                uint32_t apid_min_idx = 0;
                std::string language = "Stream";
		
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
                                language = allpids.APIDs[i].desc;
                        }
                }
                
                if (apid_min != UINT_MAX)
                {
                        APIDDesc a = {apid_min, apid_min_idx, false, language};
                        apid_list.push_back(a);
                }
        }

	// alternate
        if (apids & TIMERD_APIDS_ALT)
        {
                uint32_t apid_min=UINT_MAX;
                uint32_t apid_min_idx = 0;
//                std::string language = "Stream";
                
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
//                                language = allpids.APIDs[i].desc;
                        }
                }
                
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid != apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                APIDDesc a = {allpids.APIDs[i].pid, i, false, allpids.APIDs[i].desc};
                                apid_list.push_back(a);
                        }
                }
        }

	// ac3
        if (apids & TIMERD_APIDS_AC3)
        {
                bool ac3_found = false;
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].is_ac3)
                        {
                                APIDDesc a = {allpids.APIDs[i].pid, i, true, allpids.APIDs[i].desc};
                                apid_list.push_back(a);
                                ac3_found=true;
                        }
                }
                
                // add non ac3 apid if ac3 not found
                if (!(apids & TIMERD_APIDS_STD) && !ac3_found)
                {
                        uint32_t apid_min = UINT_MAX;
                        uint32_t apid_min_idx = 0;
                        std::string language = "Stream";
			
                        for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                        {
                                if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                                {
                                        apid_min = allpids.APIDs[i].pid;
                                        apid_min_idx = i;
                                        language = allpids.APIDs[i].desc;
                                }
                        }

                        if (apid_min != UINT_MAX)
                        {
                                APIDDesc a = {apid_min, apid_min_idx, false, language};
                                apid_list.push_back(a);
                        }
                }
        }

        // no apid selected use standard
        if (apid_list.empty() && !allpids.APIDs.empty())
        {
                uint32_t apid_min = UINT_MAX;
                uint32_t apid_min_idx = 0;
                std::string language = "Stream";
		
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
                                language = allpids.APIDs[i].desc;
                        }
                }
                
                if (apid_min != UINT_MAX)
                {
                        APIDDesc a = {apid_min, apid_min_idx, false, language};
                        apid_list.push_back(a);
                }
        }
}

//
void CVCRControl::RestoreNeutrino(void)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::RestoreNeutrino\n");
	
	CZapit::getInstance()->setRecordMode( false );

//	if (!IS_WEBTV(channel_id))
	{
		// start playback
		if (!CZapit::getInstance()->isPlayBackActive() && (CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby))
			CZapit::getInstance()->startPlayBack(live_channel);

		// alten mode wieder herstellen (ausser wen zwischenzeitlich auf oder aus sb geschalten wurde)
		if(CNeutrinoApp::getInstance()->getMode() != last_mode && CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby && last_mode != NeutrinoMessages::mode_standby)
		{
			if(!autoshift) 
				g_RCInput->postMsg(NeutrinoMessages::CHANGEMODE, last_mode);
		}

		if(last_mode == NeutrinoMessages::mode_standby && CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_standby )
		{
			//Wenn vorher und jetzt standby, dann die zapit wieder auf sb schalten
			CZapit::getInstance()->setStandby(true);
		}
	}	
}

//
void CVCRControl::CutBackNeutrino(const t_channel_id channel_id, const int mode)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::CutBackNeutrino\n");
	
	last_mode = CNeutrinoApp::getInstance()->getMode();

//	if (!IS_WEBTV(channel_id))
	{
		if(last_mode == NeutrinoMessages::mode_standby)
		{
			CZapit::getInstance()->setStandby(false);
		}
	
		if (channel_id != 0) 
		{
			if (mode != last_mode && (last_mode != NeutrinoMessages::mode_standby || mode != CNeutrinoApp::getInstance()->getLastMode())) 
			{
				CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
				// Wenn wir im Standby waren, dann brauchen wir f�rs streamen nicht aufwachen...
				if(last_mode == NeutrinoMessages::mode_standby)
					CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_standby);
			}
		}

		// stop playback im standby
		if( last_mode == NeutrinoMessages::mode_standby )
			CZapit::getInstance()->stopPlayBack();
	}
	
	// after this zapit send EVT_RECORDMODE_ACTIVATED, so neutrino getting NeutrinoMessages::EVT_RECORDMODE
	CZapit::getInstance()->setRecordMode( true );
}

//
void CVCRControl::Stop()
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::Stop\n");
	
	std::string extMessage = " ";
	time_t end_time = time(0);
		
	g_movieInfo->length = (int) round((double) (end_time - start_time) / (double) 60);
	g_cMovieInfo->encodeMovieInfoXml(&extMessage, g_movieInfo);	
	
	if (IS_WEBTV(channel_id))
		stopWebTVRecording();
	else
		stopRecording();

	//
	RestoreNeutrino();

	deviceState = CMD_VCR_STOP;
	
	// set lastmode
	if(last_mode != NeutrinoMessages::mode_scart)
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);
	}
	
	// cleanup
	if (g_movieInfo->audioPids.size())
		g_movieInfo->audioPids.clear();
		
	if (g_movieInfo->vtxtPids.size())
		g_movieInfo->vtxtPids.clear();
	
	if (g_movieInfo)
	{
		delete g_movieInfo;
		g_movieInfo = NULL;
	}
	
	if (g_cMovieInfo)
	{
		delete g_cMovieInfo;
		g_cMovieInfo = NULL;
	}
}

//
bool CVCRControl::doRecord(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string& epgTitle, unsigned char apids, const time_t epg_time) 
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::doRecord\n");
	
	// leave menu (if in any)
	g_RCInput->postMsg(CRCInput::RC_timeout, 0);
	
	last_mode = CNeutrinoApp::getInstance()->getMode();

	if(mode != last_mode) 
	{
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
	}
	
	// zaptoRecordChannel
	if(channel_id != 0)	// wenn ein channel angegeben ist
	{
		// zap for record
		CZapit::getInstance()->zapToRecordID(channel_id);			// for recording
	}

	// set apids
	if (!IS_WEBTV(channel_id))
	{
		if(! (apids & TIMERD_APIDS_STD)) // nicht std apid
		{
			APIDList apid_list;
			getAPIDs(channel_id, apids, apid_list);

			if(!apid_list.empty())
			{
				if(!apid_list.begin()->ac3)
					CZapit::getInstance()->setAudioChannel(apid_list.begin()->index);
				else
					CZapit::getInstance()->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
			}
			else
				CZapit::getInstance()->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
		}
		else
			CZapit::getInstance()->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
	}

	// switch to scart
	if(SwitchToScart)
	{
		// Auf Scart schalten
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::VCR_ON, 0 );
		// Das ganze nochmal in die queue, da obiges RC_timeout erst in der naechsten ev. loop ausgef�hrt wird
		// und dann das menu widget das display falsch r�cksetzt
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
	}

	deviceState = CMD_VCR_RECORD;
	
	//
	unsigned short pids[MAXPIDS];
	unsigned int numpids = 0;
	unsigned int pos = 0;

	// cut neutrino
	CutBackNeutrino(channel_id, mode);

	// genpsi / add pids
	 APIDList apid_list;
	 CZapit::CServiceInfo si;
	 
	if (!IS_WEBTV(channel_id))
	{
		si = CZapit::getInstance()->getServiceInfo(channel_id);
		
		CZapitChannel *channel = CZapit::getInstance()->findChannelByChannelID(channel_id);
		
		// vpid / pcrpid
		if (si.vpid != 0)
		{
			psi.addPid(si.vpid, si.vtype == CHANNEL_VIDEO_MPEG4 ? EN_TYPE_AVC : si.vtype == CHANNEL_VIDEO_HEVC ? EN_TYPE_HEVC : EN_TYPE_VIDEO, 0);
			
			if (si.pcrpid && (si.pcrpid != si.vpid))
			{
				psi.addPid(si.pcrpid, EN_TYPE_PCR, 0);
				pids[numpids++] = si.pcrpid;
			}
		}
			
		// apids
		getAPIDs(channel_id, apids, apid_list);

		for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); it++) 
		{
		        pids[numpids++] = it->apid;

			psi.addPid(it->apid, EN_TYPE_AUDIO, it->ac3 ? 1 : 0, it->language.c_str());
		}
		
		// subs
		if (channel)
		{
			for (int i = 0 ; i < (int)channel->getSubtitleCount() ; ++i) 
			{
				CZapitAbsSub* s = channel->getChannelSub(i);
				
				// teletext
				if (s->thisSubType == CZapitAbsSub::TTX) 
				{
					CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
					
					dprintf(DEBUG_NORMAL, "CVCRControl::doRecord: adding TTX subtitle %s pid 0x%x mag 0x%X page 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId, sd->teletext_magazine_number, sd->teletext_page_number);
					
					pids[numpids++] = sd->pId;
					
					psi.addPid(sd->pId, EN_TYPE_TELTEX, 0, sd->ISO639_language_code.c_str());
				}
				
				//dvbsub
				if (s->thisSubType == CZapitAbsSub::DVB) 
				{
					CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
					
					dprintf(DEBUG_NORMAL, "CVCRControl::doRecord: adding DVB subtitle %s pid 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);

					pids[numpids++] = sd->pId;
					
					psi.addPid(sd->pId, EN_TYPE_DVBSUB, 0, sd->ISO639_language_code.c_str());
				}
			}
		}
        }

	// generate record file name format
	char filename[512]; // UTF-8

	// Create filename for recording
	pos = Directory.size();
	strcpy(filename, Directory.c_str());
	
	if ((pos == 0) || (filename[pos - 1] != '/')) 
	{
		filename[pos] = '/';
		pos++;
		filename[pos] = '\0';
	}

	pos = strlen(filename);

	ext_channel_name = CZapit::getInstance()->getChannelName(channel_id);

	if (!ext_channel_name.empty())
	{
		strcpy(&(filename[pos]), UTF8_TO_FILESYSTEM_ENCODING(ext_channel_name.c_str()));
		char * p_act = &(filename[pos]);
		
		if (!IS_WEBTV(channel_id)) //FIXME:
		{
			do {
				p_act += strcspn(p_act, "/ \"%&-\t`'�!,:;");
				if (*p_act) 
				{
					*p_act++ = '_';
				}
			} while (*p_act);
		}

		// save channel name dir
		if (!autoshift && g_settings.recording_save_in_channeldir)
		{
			struct stat statInfo;
			int res = stat(filename, &statInfo);

			if (res == -1) 
			{
				if (errno == ENOENT) 
				{
					res = safe_mkdir(filename);

					if (res == 0) 
					{
						strcat(filename, "/");
					} 
					else 
					{
						perror("[vcrcontrol] mkdir");
					}
						
				} 
				else 
				{
					perror("[vcrcontrol] stat");
				}
			} 
			else 
			{
				// directory exists
				strcat(filename, "/");
			}	
				
		} 
		else
		{
			strcat(filename, "_");
		}
	}

	pos = strlen(filename);

	// epg for filename
	if (g_settings.recording_epg_for_filename) 
	{ 
		if(epgid != 0) 
		{
			CShortEPGData epgdata;

			if(CSectionsd::getInstance()->getEPGidShort(epgid, &epgdata))
			{
				if (!(epgdata.title.empty()))
				{
					strcpy(&(filename[pos]), epgdata.title.c_str());
					char * p_act = &(filename[pos]);
					
					if (!IS_WEBTV(channel_id)) //FIXME:
					{
						do {
							p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^�$\\=*#@�|");

							if (*p_act) 
							{
								*p_act++ = '_';
							}
						} while (*p_act);
					}
				}
			}
		} 
		else if (!epgTitle.empty()) 
		{
			strcpy(&(filename[pos]), epgTitle.c_str());
			char * p_act = &(filename[pos]);
			
			if (!IS_WEBTV(channel_id))
			{
				do {
					p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^�$\\=*#@�|");
					if (*p_act) 
					{
						*p_act++ = '_';
					}
				} while (*p_act);
			}
		}
	}

	pos = strlen(filename);
	time_t t = time(NULL);
	strftime(&(filename[pos]), sizeof(filename) - pos - 1, "%Y%m%d_%H%M%S", localtime(&t));

	// startRecording
	start_time = time(0);

	stream2file_error_msg_t error_msg = STREAM2FILE_BUSY;
	
	if (IS_WEBTV(channel_id))
	{
		error_msg = startWebTVRecording(filename, epgid, epgTitle, epg_time);
	}
	else
	{
		error_msg = startRecording(filename, getMovieInfoString(channel_id, epgid, epgTitle, apid_list, epg_time).c_str(), si.vpid, pids, numpids);
	}

	if (error_msg == STREAM2FILE_OK) 
	{
		deviceState = CMD_VCR_RECORD;
		
		return true;
	}
	else 
	{
		RestoreNeutrino();

		MessageBox(_("Error"), error_msg == STREAM2FILE_BUSY ? _("One or several recording processes are active.\nIf you encounter this message and no recording is active, please restart Neutrino.") : error_msg == STREAM2FILE_INVALID_DIRECTORY ? _("The recording directory is not writable.\nRecording will not work.") : _("The recording was aborted,\nbecause the target file could not be opened."), CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);

		return false;
	}
}

bool CVCRControl::Screenshot(const t_channel_id channel_id, char * fname) 
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::Screenshot\n");
	
	//FIXME:
	char filename[512]; // UTF-8
	char cmd[512];
	std::string channel_name;
	CEPGData epgData;
	event_id_t epgid = 0;
	unsigned int pos = 0;

	if(!fname) // live stream
	{
		std::string str = "sda1";
		
		struct statfs s;
		
		if (::statfs(g_settings.network_nfs_recordingdir, &s) == 0) 
		{
			//std::string str1 = g_settings.network_nfs_recordingdir;
			//str = str1.substr(7, 4);
			//str = str1.substr(str1.length() - 4, str1.length());
		}
		char str2[100];	
		sprintf(str2, "/media/%s/screenshots/", str.c_str() );
		
		if(safe_mkdir(str2))
			return false;

		strcpy(filename, str2);
	
		pos = strlen(filename);

		channel_name = CZapit::getInstance()->getChannelName(channel_id);

		if (!(channel_name.empty())) 
		{
			strcpy(&(filename[pos]), UTF8_TO_FILESYSTEM_ENCODING(channel_name.c_str()));
			char * p_act = &(filename[pos]);

			do {
				p_act += strcspn(p_act, "/ \"%&-\t`'�!,:;");
				if (*p_act) {
					*p_act++ = '_';
				}
			} while (*p_act);
			strcat(filename, "_");
		}

		pos = strlen(filename);

		if(CSectionsd::getInstance()->getActualEPGServiceKey(channel_id&0xFFFFFFFFFFFFULL, &epgData));
			epgid = epgData.eventID;

		if(epgid != 0) 
		{
			CShortEPGData epgdata;

			if(CSectionsd::getInstance()->getEPGidShort(epgid, &epgdata)) 
			{
				if (!(epgdata.title.empty())) 
				{
					strcpy(&(filename[pos]), epgdata.title.c_str());
					char * p_act = &(filename[pos]);
					do {
						p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^�$\\=*#@�|");
						if (*p_act) {
							*p_act++ = '_';
						}
					} while (*p_act);
				}
			}
		}
		
		pos = strlen(filename);
		time_t t = time(NULL);
		strftime(&(filename[pos]), sizeof(filename) - pos - 1, "%Y%m%d_%H%M%S", localtime(&t));
		
		strcat(filename, ".jpg");
		
		sprintf(cmd, "grab -v -r320 %s", filename);
	} 
	else
	{
		//from tsbrowser
		strcpy(filename, fname);

		std::string file_name = fname; // UTF-8
		
		changeFileNameExt(file_name, ".jpg");
		
		sprintf(cmd, "grab -v -r320 %s", (char *)file_name.c_str());
	}
	
	printf("Executing %s\n", cmd);
	
	if(system(cmd))
	{
		return false;
	}
	
	return true;
}

//
std::string CVCRControl::getMovieInfoString(const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, APIDList apid_list, const time_t epg_time)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::getMovieInfoString\n");
	
	std::string extMessage;
	std::string apids10;
	std::string info1, info2;

	if(!g_cMovieInfo)
		g_cMovieInfo = new CMovieInfo();
		
	if(!g_movieInfo)
		g_movieInfo = new MI_MOVIE_INFO();

	g_cMovieInfo->clearMovieInfo(g_movieInfo);
	
	CZapitChannel * channel = CZapit::getInstance()->findChannelByChannelID(channel_id);

	CZapit::getInstance()->getPIDS(channel_id, pids);

	//
	CZapit::CServiceInfo si;

	si = CZapit::getInstance()->getServiceInfo(channel_id);

	//
	std::string tmpstring;

	// channel name
	tmpstring = CZapit::getInstance()->getChannelName(channel_id);

	if (tmpstring.empty())
		g_movieInfo->epgChannel = "unknown";
	else
		g_movieInfo->epgChannel = UTF8_to_UTF8XML(tmpstring.c_str());

	// epg
	tmpstring = "not available";
	
	if (epgid != 0) 
	{
		CEPGData epgdata;
		
		if (CSectionsd::getInstance()->getEPGid(epgid, epg_time, &epgdata)) 
		{
			tmpstring = epgdata.title;
			
			info1 = epgdata.info1;
			info2 = epgdata.info2;
			
			g_movieInfo->parentalLockAge = epgdata.fsk;
			
			if(epgdata.contentClassification.size() > 0 )
				g_movieInfo->genreMajor = epgdata.contentClassification[0];
				
			g_movieInfo->length = epgdata.epg_times.duration	/ 60;
				
			dprintf(DEBUG_INFO, ANSI_YELLOW "CVCRControl::getMovieInfoString: fsk:%d, Genre:%d, Duration: %d min\r\n",g_movieInfo->parentalLockAge,g_movieInfo->genreMajor,g_movieInfo->length);	
		}
	} 
	else if (!epgTitle.empty()) 
	{
		tmpstring = epgTitle;
	}
	
	g_movieInfo->epgTitle		= UTF8_to_UTF8XML(tmpstring.c_str());
	g_movieInfo->epgId		= channel_id;
	g_movieInfo->epgInfo1		= UTF8_to_UTF8XML(info1.c_str());
	g_movieInfo->epgInfo2		= UTF8_to_UTF8XML(info2.c_str());
	g_movieInfo->epgEpgId		= epgid ;

	g_movieInfo->epgMode		= CZapit::getInstance()->getMode();
	g_movieInfo->epgVideoPid	= si.vpid;
	g_movieInfo->VideoType		= si.vtype;

	// ???
	g_vpid = si.vpid;
	g_vtype = si.vtype;
	g_currentapid = si.apid;
	memset(g_apids, 0, sizeof(unsigned short)*10);
	memset(g_ac3flags, 0, sizeof(unsigned short)*10);
	g_numpida = 0;

	// get apids desc
	EPG_AUDIO_PIDS audio_pids;
	
	if (!IS_WEBTV(channel_id))
		processAPIDnames();

	APIDList::iterator it;
	for(unsigned int i = 0; i < pids.APIDs.size(); i++) 
	{
		for(it = apid_list.begin(); it != apid_list.end(); it++) 
		{
			if(pids.APIDs[i].pid == it->apid) 
			{
				audio_pids.epgAudioPid = pids.APIDs[i].pid;
				audio_pids.epgAudioPidName = UTF8_to_UTF8XML(pids.APIDs[i].desc);
				audio_pids.atype = pids.APIDs[i].is_ac3;
				audio_pids.selected = (audio_pids.epgAudioPid == (int) g_currentapid) ? 1 : 0;

				g_movieInfo->audioPids.push_back(audio_pids);

				if(pids.APIDs[i].is_ac3)
					g_ac3flags[i] = 1;

				g_apids[i] = pids.APIDs[i].pid;
				if(g_apids[i] == g_currentapid)
					g_currentac3 = pids.APIDs[i].is_ac3;
				g_numpida++;
			}
		}
	}

	//FIXME sometimes no apid in xml ??
	if(g_movieInfo->audioPids.empty() && pids.APIDs.size()) 
	{
		int i = 0;
		audio_pids.epgAudioPid = pids.APIDs[i].pid;
		audio_pids.epgAudioPidName = UTF8_to_UTF8XML(pids.APIDs[i].desc);
		audio_pids.atype = pids.APIDs[i].is_ac3;
		audio_pids.selected = 1;

		g_movieInfo->audioPids.push_back(audio_pids);
	}
	
	// vtxt
	EPG_VTXT_PIDS vtxt_pids;
	
	for (int i = 0 ; i < (int)channel->getSubtitleCount() ; ++i) 
	{
		CZapitAbsSub* s = channel->getChannelSub(i);
		CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(s);	
			
		if (s->thisSubType == CZapitAbsSub::TTX) 
		{
			vtxt_pids.language = st->ISO639_language_code.c_str();
			vtxt_pids.pid = st->pId;
			vtxt_pids.page = ((st->teletext_magazine_number & 0xFF) << 8) | st->teletext_page_number;

			g_movieInfo->vtxtPids.push_back(vtxt_pids);
		}
	}
	
	// set / save cover
	CTmdb * tmdb = new CTmdb();

	if(tmdb->getMovieInfo(g_movieInfo->epgTitle) && (!CNeutrinoApp::getInstance()->timeshiftstatus || !autoshift))
	{
		if ((!tmdb->getDescription().empty())) 
		{
			std::string tname = Directory.c_str();
			tname += "/";
			tname += g_movieInfo->epgTitle;
			tname += ".jpg";

			tmdb->getSmallCover(tmdb->getPosterPath(), tname);

			if(!tname.empty())
				g_movieInfo->tfile = tname;
		}
	}

	delete tmdb;
	tmdb = NULL;

	g_cMovieInfo->encodeMovieInfoXml(&extMessage, g_movieInfo);

	return extMessage;
}

void CVCRControl::processAPIDnames()
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::processAPIDnames\n");
	
	bool has_unresolved_ctags = false;
	int ac3_found = -1;

	for(unsigned int count = 0; count < pids.APIDs.size(); count++)
	{
		dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::processAPIDnames: apid name= %s (%s) pid= 0x%x\n", pids.APIDs[count].desc, getISO639Description( pids.APIDs[count].desc ), pids.APIDs[count].pid);
		
		if ( pids.APIDs[count].component_tag != 0xFF )
		{
			has_unresolved_ctags= true;
		}
		
		if ( strlen( pids.APIDs[count].desc ) == 3 )
		{
			// unaufgeloeste Sprache...
			strcpy( pids.APIDs[count].desc, getISO639Description( pids.APIDs[count].desc ) );
		}

		if ( pids.APIDs[count].is_ac3 )
		{
			if((strlen( pids.APIDs[count].desc ) == 3) && (ac3_found < 0))
				ac3_found = count;
		}
	}

	if ( has_unresolved_ctags )
	{
		if ( record_EPGid != 0 )
		{
			CSectionsd::ComponentTagList tags;

			if ( CSectionsd::getInstance()->getComponentTagsUniqueKey( record_EPGid, tags ) )
			{
				has_unresolved_ctags = false;

				for (unsigned int i = 0; i < tags.size(); i++)
				{
					for (unsigned int j=0; j< pids.APIDs.size(); j++)
					{
						if ( pids.APIDs[j].component_tag == tags[i].componentTag )
						{
							// workaround for buggy ZDF ctags / or buggy sectionsd/drivers , who knows...
							if(!tags[i].component.empty())
							{
								strncpy(pids.APIDs[j].desc, tags[i].component.c_str(), 25);
							}
							pids.APIDs[j].component_tag = -1;
							break;
						}
					}
				}
			}
		}
	}
}

//
stream2file_error_msg_t CVCRControl::startRecording(const char * const filename, const char * const info, unsigned short vpid, unsigned short * pids, int numpids)
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::startRecording: %s\n", filename);
	
	int fd;
	char buf[FILENAMEBUFFERSIZE];
	struct statfs s;

	// rip rec_filename
	sprintf(rec_filename, "%s", filename);

	// saveXML/cover
	sprintf(buf, "%s.xml", filename);

	char * dir = strdup(buf);
	int ret = statfs(dirname(dir), &s);
	free(dir);

	if((ret != 0) || (s.f_type == 0x72b6) || (s.f_type == 0x24051905)) 
	{
		return STREAM2FILE_INVALID_DIRECTORY;
	}

	if ((fd = open(buf, O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0) 
	{
		write(fd, info, strlen(info));
		fdatasync(fd);
		close(fd);
	} 
	else 
	{
		return STREAM2FILE_INVALID_DIRECTORY;
	}

	// record
	sprintf(buf, "%s.ts", filename);

	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::startRecording: file %s vpid 0x%x apid 0x%x\n", buf, vpid, pids[0]);

	fd = open(buf, O_CREAT | O_RDWR | O_LARGEFILE | O_TRUNC , S_IRWXO | S_IRWXG | S_IRWXU);
	if(fd < 0) 
	{
		perror(buf);
		return STREAM2FILE_INVALID_DIRECTORY;
	}
	
	//
	psi.genpsi(fd);
	
	// init record
	if(!record)
		record = new cRecord();
	
	// open
	record->Open();

	// start_recording	  
	if(!record->Start(fd, vpid, pids, numpids, record_fe)) 	  
	{
		record->Stop();
		delete record;
		record = NULL;
		return STREAM2FILE_INVALID_DIRECTORY;
	}

	return STREAM2FILE_OK;
}

void CVCRControl::stopRecording()
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::stopRecording autoshift:%d timeshift:%d\n", autoshift, CNeutrinoApp::getInstance()->timeshiftstatus);
	
	char buf[FILENAMEBUFFERSIZE];
	char buf1[FILENAMEBUFFERSIZE];
	
	if(record) 
	{
		record->Stop();
		delete record;
		record = NULL;
	}

	if( autoshift || CNeutrinoApp::getInstance()->timeshiftstatus) 
	{
		sprintf(buf, "rm -f %s.ts &", rec_filename);
		sprintf(buf1, "%s.xml", rec_filename);

//		system(buf);
		unlink(buf);
		unlink(buf1);
		
		if (g_movieInfo->tfile != DATADIR "/icons/no_coverArt.png" && g_movieInfo->tfile != DATADIR "/icons/nopreview.jpg") 
			unlink(g_movieInfo->tfile.c_str());
	}

	rec_filename[0] = 0;
}

////
void CVCRControl::Close()
{
	if (ifcx)
	{
		avformat_close_input(&ifcx);
	}
	
	if (ofcx)
	{
		if (ofcx->pb)
		{
			avio_close(ofcx->pb);
			ofcx->pb = NULL;
		}
		
		avformat_free_context(ofcx);
	}
	
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
}

void CVCRControl::FillMovieInfo(CZapitChannel * channel, APIDList& apid_list)
{
	g_movieInfo->VideoType = 0;

	for (unsigned i = 0; i < ofcx->nb_streams; i++)
	{
		AVStream *st = ofcx->streams[i];
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
		AVCodecContext *codec = st->codec;
#else
		AVCodecParameters *codec = st->codecpar;
#endif

		if (codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			EPG_AUDIO_PIDS audio_pids;
			AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);
			AVDictionaryEntry *title = av_dict_get(st->metadata, "title", NULL, 0);

			std::string desc;
			if (lang)
				desc += lang->value;

			if (title)
			{
				if (desc.length() != 0)
					desc += " ";
				desc += title->value;
			}
			
			switch (codec->codec_id)
			{
				case AV_CODEC_ID_AC3:
					audio_pids.atype = CZapitAudioChannel::AC3;
					break;
					
				case AV_CODEC_ID_AAC:
					audio_pids.atype = CZapitAudioChannel::AAC;
					break;
					
				case AV_CODEC_ID_EAC3:
					audio_pids.atype = CZapitAudioChannel::EAC3;
					break;
					
				case AV_CODEC_ID_MP2:
				default:
					audio_pids.atype = CZapitAudioChannel::MPEG;
					break;
			}

			audio_pids.selected = 0;
			audio_pids.epgAudioPidName = desc;
			audio_pids.epgAudioPid = st->id;
			g_movieInfo->audioPids.push_back(audio_pids);

		}
		else if (codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			g_movieInfo->epgVideoPid = st->id;
			
			if (codec->codec_id == AV_CODEC_ID_H264)
				g_movieInfo->VideoType = 1;
		}
	}
}

bool CVCRControl::saveXML(const char* const filename, const char* const info)
{
	int fd;
	char buf[FILENAMEBUFFERSIZE];
	struct statfs s;

	sprintf(buf, "%s.xml", filename);

	char * dir = strdup(buf);
	int ret = statfs(dirname(dir), &s);
	free(dir);

	if((ret != 0) || (s.f_type == 0x72b6) || (s.f_type == 0x24051905)) 
	{
		return false;
	}

	if ((fd = open(buf, O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0) 
	{
		write(fd, info, strlen(info));
		fdatasync(fd);
		close(fd);
		
		return true;
	} 
	else 
	{
		return false;
	}
}

bool CVCRControl::Start()
{
	if (!stopped)
		return false;
		
	stopped = false;
	int ret = start();
	
	return (ret == 0);
}

void CVCRControl::stopWebTVRecording()
{
	dprintf(DEBUG_NORMAL, ANSI_YELLOW "CVCRControl::stopWebTVRecording\n");
	
	if (stopped)
		return;

	time_t end_time = time_monotonic();

	stopped = true;
	
	int ret = join();

	struct stat test;
	std::string xmlfile = std::string(rec_filename) + ".xml";
	std::string tsfile = std::string(rec_filename) + ".ts";
	
	if (stat(xmlfile.c_str(), &test) == 0)
	{
		if(!g_movieInfo)
			g_movieInfo = new MI_MOVIE_INFO();

		g_cMovieInfo->clearMovieInfo(g_movieInfo);
		
		g_movieInfo->file.Name = tsfile;
		
		g_cMovieInfo->loadMovieInfo(g_movieInfo);//restore user bookmark
	}

	// rewrite length (recorded time not the length from epg)
	g_movieInfo->length = (int) round((double)(end_time - time_started) / (double) 60);

//	SaveXml();

	// close
	Close();

	//
	if( autoshift || CNeutrinoApp::getInstance()->timeshiftstatus) 
	{
		unlink(tsfile.c_str());
		unlink(xmlfile.c_str());
		
		if (g_movieInfo->tfile != DATADIR "/icons/no_coverArt.png" && g_movieInfo->tfile != DATADIR "/icons/nopreview.jpg") 
			unlink(g_movieInfo->tfile.c_str());
	}

	rec_filename[0] = 0;
}

stream2file_error_msg_t CVCRControl::startWebTVRecording(const char* const filename, const event_id_t epgid, const std::string& epgTitle, const time_t epg_time)
{
	APIDList apid_list;

	CZapitChannel *channel = CZapit::getInstance()->findChannelByChannelID(channel_id);
	
	if (channel == NULL)
	{
		return STREAM2FILE_INVALID_PID;
	}
	
	//
	sprintf(rec_filename, "%s", filename);
	
	// open / start
	if (!Open(channel, filename) || !Start())
	{
		Close();

		return STREAM2FILE_RECORDING_THREADS_FAILED;
	}
	
	// save xml
//	FillMovieInfo(channel, apid_list); //FIXME:

	saveXML(filename, getMovieInfoString(channel_id, epgid, epgTitle, apid_list, epg_time).c_str());

	return STREAM2FILE_OK;
}

bool CVCRControl::Open(CZapitChannel *channel, const char * const filename)
{
	std::string url = channel->getUrl();

	if (url.empty())
		return false;

	// fill headers
	std::string pretty_name, headers, dumb;

	// init av
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
	avcodec_register_all();
#endif
	avformat_network_init();

//	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	AVDictionary *options = NULL;
	av_dict_set(&options, "auth_type", "basic", 0);
	
	if (!headers.empty())//add cookies
	{
		headers += "\r\n";
		av_dict_set(&options, "headers", headers.c_str(), 0);
	}

//	av_log_set_level(AV_LOG_DEBUG);
	
	if (avformat_open_input(&ifcx, url.c_str(), NULL, &options) != 0)
	{
//		printf("%s: Cannot open input [%s]!\n", __FUNCTION__, url.c_str());
//		av_log_set_level(AV_LOG_INFO);
		av_dict_free(&options);
		
		return false;
	}

//	av_log_set_level(AV_LOG_INFO);
	av_dict_free(&options);

	if (avformat_find_stream_info(ifcx, NULL) < 0)
	{
//		printf("%s: Cannot find stream info [%s]!\n", __FUNCTION__, channel->getUrl().c_str());
		
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
//		printf("%s: not supported format [%s]!\n", __FUNCTION__, ifcx->iformat->name);
		return false;
	}

#if (LIBAVFORMAT_VERSION_MAJOR < 58)
	snprintf(ifcx->filename, sizeof(ifcx->filename), "%s", channel->getUrl().c_str());
	av_dump_format(ifcx, 0, ifcx->filename, 0);
#else
	av_dump_format(ifcx, 0, ifcx->url, 0);
#endif

	//
	std::string tsfile = std::string(filename) + ".ts";
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,0,100)
	AVOutputFormat *ofmt = av_guess_format(NULL, tsfile.c_str(), NULL);
#else
	const AVOutputFormat *ofmt = av_guess_format(NULL, tsfile.c_str(), NULL);
#endif
	if (ofmt == NULL)
	{
//		printf("%s: av_guess_format for [%s] failed!\n", __FUNCTION__, tsfile.c_str());
		return false;
	}

	ofcx = avformat_alloc_context();
	ofcx->oformat = ofmt;

	if (avio_open2(&ofcx->pb, tsfile.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0)
	{
//		printf("%s: avio_open2 for [%s] failed!\n", __FUNCTION__, tsfile.c_str());
		return false;
	}

	av_dict_copy(&ofcx->metadata, ifcx->metadata, 0);
	
#if (LIBAVFORMAT_VERSION_MAJOR < 58)
	snprintf(ofcx->filename, sizeof(ofcx->filename), "%s", tsfile.c_str());
#else
	ofcx->url = av_strdup(!tsfile.empty() ? tsfile.c_str() : "");
#endif

	stream_index = -1;
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
		
		if (iccx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			stream_index = i;
		}
		else if (stream_index < 0)
			stream_index = i;
	}
	
//	av_log_set_level(AV_LOG_VERBOSE);
#if (LIBAVFORMAT_VERSION_MAJOR < 58)
	av_dump_format(ofcx, 0, ofcx->filename, 1);
#else
	av_dump_format(ofcx, 0, ofcx->url, 1);
#endif

//	av_log_set_level(AV_LOG_WARNING);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,100)
	bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	
//	if (!bsfc)
//		printf("%s: av_bitstream_filter_init h264_mp4toannexb failed!\n", __FUNCTION__);
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

void CVCRControl::run()
{
	AVPacket pkt;

	time_t now = 0;
	time_t tstart = time_monotonic();
	time_started = tstart;
	start_time = time(0);
	
	if (avformat_write_header(ofcx, NULL) < 0)
	{
//		printf("%s: avformat_write_header failed\n", __FUNCTION__);
		return;
	}

	double total = 0;
	
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

		if (pkt.stream_index == stream_index)
		{
			total += (double) 1000 * pkt.duration * av_q2d(ifcx->streams[stream_index]->time_base);
			//printf("PKT: duration %d (%f) total %f (ifcx->duration %016llx\n", pkt.duration, duration, total, ifcx->duration);
		}

		if (now == 0)
			WriteHeader(1000);
			
		now = time_monotonic();
		
		if (now - tstart > 1)
		{
			tstart = now;
			WriteHeader(total);
		}
	}

	av_read_pause(ifcx);
	av_write_trailer(ofcx);
	WriteHeader(total);
	
	printf("%s: Stopped.\n", __FUNCTION__);
}

void CVCRControl::WriteHeader(uint32_t duration)
{
	std::string tsfile = std::string(rec_filename) + ".ts";

	int srcfd = open(tsfile.c_str(), O_WRONLY | O_LARGEFILE);
	
	if (srcfd >= 0)
	{
		if (lseek64(srcfd, 188 - sizeof(PVR_FILE_INFO), SEEK_SET) >= 0)
		{
			PVR_FILE_INFO pinfo;
			pinfo.uDuration = duration;
			pinfo.uTSPacketSize = 188;
			
			write(srcfd, (uint8_t *)&pinfo, sizeof(PVR_FILE_INFO));
		}
		close(srcfd);
	}
	else
		perror(tsfile.c_str());
}

