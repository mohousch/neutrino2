/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vcrcontrol.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#include <gui/movieinfo.h>

#include <driver/encoding.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <daemonc/remotecontrol.h>

//
#include <zapit/zapit.h>
#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>
#include <zapit/channel.h>

#include <system/helpers.h>
#include <system/debug.h>
#include <system/tmdbparser.h>

#include <driver/vcrcontrol.h>
#include <driver/genpsi.h>

#include <record_cs.h>


extern bool autoshift;
extern bool autoshift_delete;

CMovieInfo * g_cMovieInfo;
MI_MOVIE_INFO * g_movieInfo;

extern CZapitChannel * live_channel;			// defined in zapit.cpp

//
static cRecord * record = NULL;

extern CFrontend * record_fe;

#define FILENAMEBUFFERSIZE 	1024
#define MAXPIDS			64

//static stream2file_status_t exit_flag = STREAM2FILE_STATUS_IDLE;

char rec_filename[FILENAMEBUFFERSIZE];

std::string ext_channel_name;

//
static CVCRControl vcrControl;

CVCRControl * CVCRControl::getInstance()
{
	return &vcrControl;
}

CVCRControl::CVCRControl()
{
	exit_flag = STREAM2FILE_STATUS_IDLE;
	channel_id = 0;
}

CVCRControl::~CVCRControl()
{
	channel_id = 0;
}

bool CVCRControl::Record(const CTimerd::RecordingInfo * const eventinfo)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::Record: channel_id:%llx\n", eventinfo->channel_id);
	
	int mode = CNeutrinoApp::getInstance()->getMode();
	
	channel_id = eventinfo->channel_id;

	return doRecord(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->epgTitle, eventinfo->apids, eventinfo->epg_starttime); 
}

void CVCRControl::getAPIDs(const t_channel_id channel_id, const unsigned char ap, APIDList & apid_list)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::getAPIDs\n");
	
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
		
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
                        }
                }
                
                if (apid_min != UINT_MAX)
                {
                        APIDDesc a = {apid_min, apid_min_idx, false};
                        apid_list.push_back(a);
                }
        }

	// alternate
        if (apids & TIMERD_APIDS_ALT)
        {
                uint32_t apid_min=UINT_MAX;
                uint32_t apid_min_idx = 0;
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
                        }
                }
                
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid != apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                APIDDesc a = {allpids.APIDs[i].pid, i, false};
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
                                APIDDesc a = {allpids.APIDs[i].pid, i, true};
                                apid_list.push_back(a);
                                ac3_found=true;
                        }
                }
                
                // add non ac3 apid if ac3 not found
                if (!(apids & TIMERD_APIDS_STD) && !ac3_found)
                {
                        uint32_t apid_min = UINT_MAX;
                        uint32_t apid_min_idx = 0;
			
                        for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                        {
                                if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                                {
                                        apid_min = allpids.APIDs[i].pid;
                                        apid_min_idx = i;
                                }
                        }

                        if (apid_min != UINT_MAX)
                        {
                                APIDDesc a = {apid_min, apid_min_idx, false};
                                apid_list.push_back(a);
                        }
                }
        }

        // no apid selected use standard
        if (apid_list.empty() && !allpids.APIDs.empty())
        {
                uint32_t apid_min = UINT_MAX;
                uint32_t apid_min_idx = 0;
		
                for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
                {
                        if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
                        {
                                apid_min = allpids.APIDs[i].pid;
                                apid_min_idx = i;
                        }
                }
                
                if (apid_min != UINT_MAX)
                {
                        APIDDesc a = {apid_min, apid_min_idx, false};
                        apid_list.push_back(a);
                }
        }
}

//
void CVCRControl::RestoreNeutrino(void)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::RestoreNeutrino\n");
	
	CZapit::getInstance()->setRecordMode( false );

	if (!IS_WEBTV(channel_id))
	{
		// start playback
		if (!CZapit::getInstance()->isPlayBackActive() && (CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby))
			CZapit::getInstance()->startPlayBack(live_channel);

		// alten mode wieder herstellen (ausser wen zwischenzeitlich auf oder aus sb geschalten wurde)
		if(CNeutrinoApp::getInstance()->getMode() != last_mode && CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby && last_mode != NeutrinoMessages::mode_standby)
		{
			if(!autoshift) 
				g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);
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
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::CutBackNeutrino\n");
	
	last_mode = CNeutrinoApp::getInstance()->getMode();

	if (!IS_WEBTV(channel_id))
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
		
			// zap to record
			CZapit::getInstance()->zapToRecordID(channel_id);
		}

		// stop playback im standby
		if( last_mode == NeutrinoMessages::mode_standby )
			CZapit::getInstance()->stopPlayBack();
	}
	
	// after this zapit send EVT_RECORDMODE_ACTIVATED, so neutrino getting NeutrinoMessages::EVT_RECORDMODE
	CZapit::getInstance()->setRecordMode( true );
}

//
bool CVCRControl::Stop()
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::Stop\n");
	
	std::string extMessage = " ";
	time_t end_time = time(0);
		
	g_movieInfo->length = (int) round((double) (end_time - start_time) / (double) 60);
	g_cMovieInfo->encodeMovieInfoXml(&extMessage, g_movieInfo);	

	bool return_value = (stopRecording(extMessage.c_str()) == STREAM2FILE_OK);

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
	g_movieInfo->audioPids.clear();
	delete g_movieInfo;
	g_movieInfo = NULL;
	delete g_cMovieInfo;
	g_cMovieInfo = NULL;

	return return_value;
}

//
bool CVCRControl::doRecord(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string& epgTitle, unsigned char apids, const time_t epg_time) 
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::doRecord\n");
	
	// leave menu (if in any)
	g_RCInput->postMsg(RC_timeout, 0);
	
	last_mode = CNeutrinoApp::getInstance()->getMode();

	if(mode != last_mode) 
	{
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
	}
	
	// zaptoRecordChannel
	if (!IS_WEBTV(channel_id))
	{
		// zapit
		if(channel_id != 0)	// wenn ein channel angegeben ist
		{
			// zap for record
			CZapit::getInstance()->zapToRecordID(channel_id);			// for recording
		}

		// apids
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

	// getRecordChannelInfo
	CZapit::CCurrentServiceInfo si;
	si = CZapit::getInstance()->getServiceInfo(channel_id);
	
	CGenPsi psi;

	// vpid
	if (si.vpid != 0)
	{
		psi.addPid(si.vpid, si.vtype ? EN_TYPE_AVC : EN_TYPE_VIDEO, 0); //FIXME:
		
		// pcr pid
		//FIXME:
	}
		
		

	// apids
        APIDList apid_list;
        getAPIDs(channel_id, apids, apid_list);

        for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); it++) 
	{
                pids[numpids++] = it->apid;
		psi.addPid(it->apid, EN_TYPE_AUDIO, it->ac3 ? 1 : 0);
        }
        
        CZapitChannel *channel = CZapit::getInstance()->findChannelByChannelID(channel_id);
        
        // subs
        /*
        if (channel)
        {
        	for (int i = 0 ; i < (int)channel->getSubtitleCount() ; ++i) 
		{
			CZapitAbsSub* s = channel->getChannelSub(i);
			
			//dvbsub
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
				dprintf(DEBUG_NORMAL, "CVCRControl::doRecord: adding DVB subtitle %s pid 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);

				pids[numpids++] = sd->pId;
				
				psi.addPid(sd->pId, EN_TYPE_DVBSUB, 0, (const char *)sd->ISO639_language_code.c_str());
			}
			
			// teletext sub
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				dprintf(DEBUG_NORMAL, "CVCRControl::doRecord: adding TTX subtitle %s pid 0x%x mag 0x%X page 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId, sd->teletext_magazine_number, sd->teletext_page_number);
				
				pids[numpids++] = sd->pId;
				
				psi.addPid(sd->pId, EN_TYPE_DVBSUB, 0, (const char *)sd->ISO639_language_code.c_str());
			}
		}
        }
        */

	//record file name format
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

	if (!(ext_channel_name.empty()))
	{
		strcpy(&(filename[pos]), UTF8_TO_FILESYSTEM_ENCODING(ext_channel_name.c_str()));
		char * p_act = &(filename[pos]);
		do {
			p_act += strcspn(p_act, "/ \"%&-\t`'�!,:;");
			if (*p_act) 
			{
				*p_act++ = '_';
			}
		} while (*p_act);

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
		else if (!epgTitle.empty()) 
		{
			strcpy(&(filename[pos]), epgTitle.c_str());
			char * p_act = &(filename[pos]);
			do {
				p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^�$\\=*#@�|");
				if (*p_act) 
				{
					*p_act++ = '_';
				}
			} while (*p_act);
		}
	}

	pos = strlen(filename);
	time_t t = time(NULL);
	strftime(&(filename[pos]), sizeof(filename) - pos - 1, "%Y%m%d_%H%M%S", localtime(&t));

	// startRecording
	start_time = time(0);

	stream2file_error_msg_t error_msg = STREAM2FILE_BUSY;

	//
	if (IS_WEBTV(channel_id))
	{
		error_msg = STREAM2FILE_RECORDING_THREADS_FAILED;
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

		MessageBox(_("Error"), error_msg == STREAM2FILE_BUSY ? _("One or several recording processes are active.\nIf you encounter this message and no recording is active, please restart Neutrino.") : error_msg == STREAM2FILE_INVALID_DIRECTORY ? _("The recording directory is not writable.\nRecording will not work.") : _("The recording was aborted,\nbecause the target file could not be opened."), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);

		return false;
	}
}

bool CVCRControl::Screenshot(const t_channel_id channel_id, char * fname) 
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::Screenshot\n");
	
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
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::getMovieInfoString\n");
	
	std::string extMessage;
	std::string apids10;
	std::string info1, info2;

	if(!g_cMovieInfo)
		g_cMovieInfo = new CMovieInfo();
		
	if(!g_movieInfo)
		g_movieInfo = new MI_MOVIE_INFO();

	g_cMovieInfo->clearMovieInfo(g_movieInfo);

	CZapit::getInstance()->getPIDS(channel_id, pids);

	//
	CZapit::CCurrentServiceInfo si;

	si = CZapit::getInstance()->getServiceInfo(channel_id);

	//
	std::string tmpstring;

	tmpstring = CZapit::getInstance()->getChannelName(channel_id);

	if (tmpstring.empty())
		g_movieInfo->epgChannel = "unknown";
	else
		g_movieInfo->epgChannel = UTF8_to_UTF8XML(tmpstring.c_str());

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
				
			g_movieInfo->length = epgdata.epg_times.dauer	/ 60;
				
			dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::getMovieInfoString: fsk:%d, Genre:%d, Dauer: %d min\r\n",g_movieInfo->parentalLockAge,g_movieInfo->genreMajor,g_movieInfo->length);	
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

	g_vpid = si.vpid;
	g_vtype = si.vtype;
	g_currentapid = si.apid;
	memset(g_apids, 0, sizeof(unsigned short)*10);
	memset(g_ac3flags, 0, sizeof(unsigned short)*10);
	g_numpida = 0;

	// get apids desc
	EPG_AUDIO_PIDS audio_pids;
	
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
	g_movieInfo->epgVTXPID = si.vtxtpid;
	
	// cover
	CTmdb * tmdb = new CTmdb();

	if(tmdb->getMovieInfo(g_movieInfo->epgTitle))
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
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::processAPIDnames\n");
	
	bool has_unresolved_ctags = false;
	//bool has_ac3 = false; //FIXME what this variable suppoused to do ?? seems unused
	int ac3_found = -1;

	for(unsigned int count = 0; count < pids.APIDs.size(); count++)
	{
		dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::processAPIDnames: apid name= %s (%s) pid= 0x%x\n", pids.APIDs[count].desc, getISO639Description( pids.APIDs[count].desc ), pids.APIDs[count].pid);
		
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
			//strncat(pids.APIDs[count].desc, " (Dolby Digital)", 25);
			//has_ac3 = true;
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
								
								//if ( pids.APIDs[j].is_ac3)
								//	strncat( pids.APIDs[j].desc, " (Dolby Digital)", 25);
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
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::startRecording: %s\n", filename);
	
	int fd;
	char buf[FILENAMEBUFFERSIZE];
	struct statfs s;

	// rip rec_filename
	if(autoshift || CNeutrinoApp::getInstance()->timeshiftstatus)
		sprintf(rec_filename, "%s_temp", filename);
	else
		sprintf(rec_filename, "%s", filename);

	// saveXML()
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
	
	// saveCover

	exit_flag = STREAM2FILE_STATUS_RUNNING;

	sprintf(buf, "%s.ts", filename);

	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::startRecording: file %s vpid 0x%x apid 0x%x\n", buf, vpid, pids[0]);

	fd = open(buf, O_CREAT | O_RDWR | O_LARGEFILE | O_TRUNC , S_IRWXO | S_IRWXG | S_IRWXU);
	if(fd < 0) 
	{
		perror(buf);
		return STREAM2FILE_INVALID_DIRECTORY;
	}
	
	//
	CGenPsi psi;
	psi.genpsi(fd);
	
	// init record
	if(!record)
		record = new cRecord();
	
	// open
	record->Open();

	// start_recording
#if defined (PLATFORM_COOLSTREAM)
	if(!record->Start(fd, vpid, pids, numpids, 0))
#else	  
	if(!record->Start(fd, vpid, pids, numpids, record_fe)) 
#endif	  
	{
		record->Stop();
		delete record;
		record = NULL;
		return STREAM2FILE_INVALID_DIRECTORY;
	}

	return STREAM2FILE_OK;
}

stream2file_error_msg_t CVCRControl::stopRecording(const char * const info, bool file_recording)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CVCRControl::stopRecording\n");
	
	char buf[FILENAMEBUFFERSIZE];
	char buf1[FILENAMEBUFFERSIZE];
	int fd;
	stream2file_error_msg_t ret;

	// savexml
	sprintf(buf, "%s.xml", rec_filename);
	
	if ((fd = open(buf, O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0) 
	{
		write(fd, info, strlen(info));
		fdatasync(fd);
		close(fd);
	} 

	//FIXME: not working now
	if (!file_recording)
	{
		if(record) 
		{
			record->Stop();
			delete record;
			record = NULL;
		}
	}

	if (exit_flag == STREAM2FILE_STATUS_RUNNING) 
	{
		exit_flag = STREAM2FILE_STATUS_IDLE;
		ret = STREAM2FILE_OK;
	}
	else
		ret = STREAM2FILE_RECORDING_THREADS_FAILED;

	if( autoshift || CNeutrinoApp::getInstance()->timeshiftstatus) 
	{
		sprintf(buf, "rm -f %s.ts &", rec_filename);
		sprintf(buf1, "%s.xml", rec_filename);

		system(buf);
		unlink(buf1);
	}

	rec_filename[0] = 0;

	return ret;
}

