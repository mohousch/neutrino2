//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: remotecontrol.cpp 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <daemonc/remotecontrol.h>

#include <global.h>
#include <neutrino2.h>
#include <gui/infoviewer.h>
#include <driver/record.h>

#include <driver/encoding.h>
#include <system/debug.h>
#include <system/helpers.h>

//
#include <zapit/bouquets.h>


//// globals
extern bool autoshift;					// defined in neutrino2.cpp
extern uint32_t scrambled_timer;		// defined in neutrino2.cpp
extern uint32_t shift_timer;				// defined in neutrino2.cpp

//// class CZapProtection
bool CZapProtection::check()
{
	int res;
	char cPIN[5];
	std::string hint2 = " ";
	
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(_("Youth protection"), cPIN, 4, hint2.c_str(), fsk);

		res = PINInput->exec(NULL, "");
		delete PINInput;

		hint2 = _("PIN-Code was wrong! Try again.");
	} while ( (strncmp(cPIN,validPIN, 4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == CWidgetTarget::RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
		  
	return ( ( strncmp(cPIN, validPIN, 4) == 0 ) || ( fsk < g_settings.parentallock_lockage ) );
}

//// class CSubService
CSubService::CSubService(const t_original_network_id anoriginal_network_id, const t_service_id aservice_id, const t_transport_stream_id atransport_stream_id, const std::string &asubservice_name)
{
	service.original_network_id = anoriginal_network_id;
	service.service_id          = aservice_id;
	service.transport_stream_id = atransport_stream_id;
	starttime                   = 0;
	duration                       = 0;
	subservice_name             = asubservice_name;
}

CSubService::CSubService(const t_original_network_id anoriginal_network_id, const t_service_id aservice_id, const t_transport_stream_id atransport_stream_id, const time_t astarttime, const unsigned aduration)
{
	service.original_network_id = anoriginal_network_id;
	service.service_id          = aservice_id;
	service.transport_stream_id = atransport_stream_id;
	starttime                  = astarttime;
	duration                       = aduration;
	subservice_name             = "";
}

t_channel_id CSubService::getChannelID(void) const
{
	return create_channel_id(service.service_id, service.original_network_id, service.transport_stream_id);
}

////
CRemoteControl::CRemoteControl()
{
	current_channel_id = 	0;
	current_sub_channel_id = 0;

	zap_completion_timeout = 0;

	current_EPGid =	0;
	next_EPGid = 	0;
	
	memset(&current_PIDs.otherPIDs, 0, sizeof(current_PIDs.otherPIDs) );
	
	has_ac3 = 	false;
	selected_subchannel = -1;
	needs_nvods = 	false;
	director_mode = 0;
	is_video_started = true;
	zapProtection = NULL;
}

int CRemoteControl::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	//
	if(msg == NeutrinoMessages::EVT_ZAP_COMPLETE || msg == NeutrinoMessages::EVT_ZAP_FAILED) 
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg: %s current_channel_id: 0x%llx data:0x%llx\n", (msg == NeutrinoMessages::EVT_ZAP_FAILED)? "EVT_ZAP_FAILED" : "EVT_ZAP_COMPLETE", CZapit::getInstance()->getCurrentChannelID(), data);
		
		// set audio map after channel zap
		CZapit::getInstance()->getAudioMode(&g_settings.audio_AnalogMode);

		if(g_settings.audio_AnalogMode < 0 || g_settings.audio_AnalogMode > 2)
			g_settings.audio_AnalogMode = 0;

		// kill shifttimer
		if(shift_timer) 
		{
			g_RCInput->killTimer(shift_timer);
			shift_timer = 0;
		}	

		// auto timeshift
		if (!CNeutrinoApp::getInstance()->recordingstatus && g_settings.auto_timeshift) 		  
		{
			int delay = g_settings.auto_timeshift;
			
			// add shift timer
			shift_timer = g_RCInput->addTimer( delay*1000*1000, true );
			
			// infoviewer handle msg
			g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, CNeutrinoApp::getInstance()->recordingstatus);
		}	

		// scrambled timer
		if(scrambled_timer) 
		{
			g_RCInput->killTimer(scrambled_timer);
			scrambled_timer = 0;
		}

		scrambled_timer = g_RCInput->addTimer(10*1000*1000, true); // 10 sec
		
		// select subtitle
		CNeutrinoApp::getInstance()->selectSubtitles();
		
		CNeutrinoApp::getInstance()->startSubtitles(!g_InfoViewer->is_visible);
		
		//
		CLCD::getInstance()->setEPGTitle("");
		
		// store channel into lastchannellist
		CNeutrinoApp::getInstance()->getlastChList() .store(CZapit::getInstance()->getCurrentChannelID());
	}
	
	// shift / scrambled timer events
	if ((msg == NeutrinoMessages::EVT_TIMER)) 
	{
		if(data == shift_timer) 
		{
			shift_timer = 0;
			CNeutrinoApp::getInstance()->startAutoRecord(true);
			
			return messages_return::handled;
		} 
		else if(data == scrambled_timer) 
		{
			scrambled_timer = 0;
			
#ifdef USE_SCRAMBLED_TIMER
			if(true && (videoDecoder->getBlank() && videoDecoder->getPlayState())) 
			{
				HintBox(_("Information"), _("Scrambled channel"), g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth (_("Scrambled channel"), true) + 10, 5);
			}
#endif

			return messages_return::handled;	
		}
	}
	
	//
	if ( zap_completion_timeout != 0 ) 
	{
    		if ( (msg == NeutrinoMessages::EVT_ZAP_COMPLETE) || 
		     (msg == NeutrinoMessages::EVT_ZAP_FAILED  ) || 
		     (msg == NeutrinoMessages::EVT_ZAP_ISNVOD  ) ) 
		{
			dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg: zap_completion_timeout: %llx EVT_ZAP current_channel_id: 0x%llx data: 0x%llx\n", zap_completion_timeout, current_channel_id, data);
			
			if (data != current_channel_id)
			{
				g_InfoViewer->chanready = 0;
				CZapit::getInstance()->zapToServiceIDNOWAIT(current_channel_id );
				CSectionsd::getInstance()->setServiceChanged(current_channel_id, false);

				zap_completion_timeout = getcurrenttime() + 2 * (long long) 1000000;

				return messages_return::handled;
			}
			else 
			{
				zap_completion_timeout = 0;
				g_InfoViewer->chanready = 1;
			
				// infoviewer
				g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

				// zapProtection
				if ((!is_video_started) && (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER))
					processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100);

				// lcd
				CLCD::getInstance()->showServicename(CZapit::getInstance()->getChannelName(CZapit::getInstance()->getCurrentChannelID()), true, CZapit::getInstance()->getChannelNumber(CZapit::getInstance()->getCurrentChannelID()));
			}
		} 	
	} 
	else 
	{
		if ( (msg == NeutrinoMessages::EVT_ZAP_COMPLETE) || 
		     (msg == NeutrinoMessages::EVT_ZAP_FAILED) || 
		     (msg == NeutrinoMessages::EVT_ZAP_ISNVOD) )
		{
			dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg: EVT_ZAP current_channel_id: 0x%llx data: 0x%llx\n", current_channel_id, data);
			
			g_InfoViewer->chanready = 1;

			// 
			if (data != current_channel_id)
			{
				current_channel_id = data;
				
				//
				is_video_started= true;

				current_EPGid = 0;
				next_EPGid = 0;

				memset(&current_PIDs.otherPIDs, 0, sizeof(current_PIDs.otherPIDs) );

				current_PIDs.APIDs.clear();
				has_ac3 = false;

				subChannels.clear();
				selected_subchannel = -1;
				director_mode = 0;
				needs_nvods = (msg == NeutrinoMessages:: EVT_ZAP_ISNVOD);
			}
			
			// infoViewer
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR);

			// zapProtection
			if ((!is_video_started) && (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER))
				processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100);

			// lcd
			CLCD::getInstance()->showServicename(CZapit::getInstance()->getChannelName(CZapit::getInstance()->getCurrentChannelID()), true, CZapit::getInstance()->getChannelNumber(CZapit::getInstance()->getCurrentChannelID()));
		}
		else
		{
			if ( (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE) || 
			     (msg == NeutrinoMessages:: EVT_ZAP_SUB_FAILED ) ) 
			{
				dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg EVT_ZAP_SUB current_sub_channel_id: 0x%llx data: 0x%llx\n", current_sub_channel_id, data);
				
				if (data != current_sub_channel_id)
				{
					current_sub_channel_id = data;

					for(unsigned int i = 0; i < subChannels.size(); i++)
					{
						if (subChannels[i].getChannelID() == data)
						{
							selected_subchannel = i;
							break;
						}
					}
				}
			}
		}
	}

	////
	if ( msg == NeutrinoMessages::EVT_CURRENTEPG )
	{
		if ( (data != current_channel_id) && (data != current_sub_channel_id) )
			return messages_return::handled;

		//
		CSectionsd::CurrentNextInfo info_CN;
		CSectionsd::getInstance()->getCurrentNextServiceKey(CZapit::getInstance()->getChannelEPGID(current_channel_id) & 0xFFFFFFFFFFFFULL, info_CN);
		
		dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg got  EVT_CURRENTEPG, uniqueKey: 0x%llx chid: 0x%llx flags: %x\n", info_CN.current_uniqueKey, current_channel_id, info_CN.flags);
		
		if ((info_CN.current_uniqueKey >> 16) == (CZapit::getInstance()->getChannelEPGID(current_channel_id) & 0xFFFFFFFFFFFFULL) || (info_CN.current_uniqueKey >> 16) == (CZapit::getInstance()->getChannelEPGID(current_sub_channel_id) & 0xFFFFFFFFFFFFULL))
		{
			// CURRENT-EPG for current channel arrived!
			CLCD::getInstance()->setEPGTitle(info_CN.current_name.empty()? "" : info_CN.current_name);			
			
			if (info_CN.current_uniqueKey != current_EPGid)
			{
				if ( current_EPGid != 0 )
				{
					// ist nur ein neues Programm, kein neuer Kanal
					// PIDs neu holen
					CZapit::getInstance()->getCurrentPIDS( current_PIDs );
					
					// APID Bearbeitung neu anstossen
					has_unresolved_ctags = true;
				}
				
				//	
				g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR);

				current_EPGid = info_CN.current_uniqueKey;

				//
				if ( has_unresolved_ctags )
					processAPIDnames();

				//
				if (selected_subchannel <= 0 && info_CN.flags & CSectionsd::epgflags::current_has_linkagedescriptors)
				{
					subChannels.clear();
					getSubChannels();
				}

				//
				if ( needs_nvods )
					getNVODs();
			}

			// is_video_started is only false if channel is locked
			if ((!is_video_started) && (info_CN.current_fsk == 0 || g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
				processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100);
			else
				processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, (const neutrino_msg_data_t)info_CN.current_fsk);
		}

		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_NEXTEPG )
	{
		if (data != current_channel_id)
			return messages_return::handled;

		if ( !is_video_started )
			processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100);
			
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_NOEPG_YET)
	{
		if (data == current_channel_id)
		{
			if ( !is_video_started )
				processZapProtection(NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100);
		}
		
		return messages_return::handled;
	}
	else if ( (msg == NeutrinoMessages::EVT_ZAP_COMPLETE) || (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE) ) 
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::handleMsg: EVT_ZAP_COMPLETE current_channel_id: 0x%llx\n", current_channel_id);
		
		if (data == ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ? current_channel_id : current_sub_channel_id))
		{
			// sectionsd
			CSectionsd::getInstance()->setServiceChanged( current_channel_id, false );
			
			// pids
			CZapit::getInstance()->getCurrentPIDS(current_PIDs );
		
			g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOTPIDS, (const neutrino_msg_data_t)current_channel_id, false);

			// apids
			processAPIDnames();
			
			// radiotext			
			if (g_settings.radiotext_enable && g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == CNeutrinoApp::mode_radio))
			{
				g_Radiotext->setPid(current_PIDs.APIDs[current_PIDs.otherPIDs.selected_apid].pid);
			}
			
			// lcd
			CLCD::getInstance()->showServicename(CZapit::getInstance()->getChannelName(CZapit::getInstance()->getCurrentChannelID()), true, CZapit::getInstance()->getChannelNumber(CZapit::getInstance()->getCurrentChannelID()));
		}

	    	return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_PMT_CHANGED) 
	{
		CZapit::getInstance()->getCurrentPIDS(current_PIDs);
		processAPIDnames();
		
		return messages_return::unhandled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_ISNVOD)
	{
		if (data == current_channel_id)
		{
			needs_nvods = true;
			
			// lcd
			CLCD::getInstance()->showServicename(CZapit::getInstance()->getChannelName(CZapit::getInstance()->getCurrentChannelID()), true, CZapit::getInstance()->getChannelNumber(CZapit::getInstance()->getCurrentChannelID()));
				
			if ( current_EPGid != 0)
			{
				getNVODs();
				
				if (subChannels.empty())
					CSectionsd::getInstance()->setServiceChanged( current_channel_id, true );
			}
			else
				// EVENT anfordern!
				CSectionsd::getInstance()->setServiceChanged( current_channel_id, true );
		}
		
		return messages_return::handled;
	}
	else
		return messages_return::unhandled;
}

void CRemoteControl::getSubChannels()
{
	if ( subChannels.size() == 0 )
	{
		CSectionsd::LinkageDescriptorList linkedServices;
		
		if ( CSectionsd::getInstance()->getLinkageDescriptorsUniqueKey( current_EPGid, linkedServices ) )
		{
			if ( linkedServices.size()> 1 )
			{
				are_subchannels = true;
				for (unsigned int i = 0; i < linkedServices.size(); i++)
				{
					subChannels.push_back(CSubService(
								      linkedServices[i].originalNetworkId,
								      linkedServices[i].serviceId,
								      linkedServices[i].transportStreamId,
								      linkedServices[i].name));
					if (subChannels[i].getChannelID() == (current_channel_id & 0xFFFFFFFFFFFFULL))
						selected_subchannel = i;
				}
				copySubChannelsToZapit();
				
				g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, (const neutrino_msg_data_t)current_channel_id, false);
			}
		}
	}
}

void CRemoteControl::getNVODs()
{
	dprintf(DEBUG_NORMAL, "CRemoteControl::getNVODs: getNVODs, current_EPGid: 0x%llx\n", current_EPGid);
	
	if ( subChannels.size() == 0 )
	{
		CSectionsd::NVODTimesList NVODs;
		
		if ( CSectionsd::getInstance()->getNVODTimesServiceKey( current_channel_id & 0xFFFFFFFFFFFFULL, NVODs ) )
		{
			are_subchannels = false;
			
			for (unsigned int i = 0; i < NVODs.size(); i++)
			{
				if ( NVODs[i].zeit.duration> 0 )
				{
					CSubService newService(
						NVODs[i].original_network_id,
						NVODs[i].service_id,
						NVODs[i].transport_stream_id,
						NVODs[i].zeit.starttime, 
						NVODs[i].zeit.duration);

					CSubServiceListSorted::iterator e = subChannels.begin();
					
					for(; e != subChannels.end(); ++e)
					{
						if ( e->starttime > newService.starttime )
							break;
					}
					subChannels.insert( e, newService );
				}

			}

			copySubChannelsToZapit();
			
			g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, (const neutrino_msg_data_t)current_channel_id, false);

			if ( selected_subchannel == -1 )
			{
				// beim ersten Holen letzten NVOD-Kanal setzen!
				setSubChannel( subChannels.size() - 1 );
			}
			else
			{
				// sollte nur passieren, wenn die aktuelle Sendung vorbei ist?!
				selected_subchannel = -1;
			}
		}
	}
}

void CRemoteControl::processZapProtection(const neutrino_msg_t msg, const neutrino_msg_data_t data)
{
	dprintf(DEBUG_NORMAL, "CRemoteControl::processZapProtection: msg:0x%llx data:0x%llx\n", msg, data);
	
	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS) 
	{
		if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = data;
			else 
			{
				if ((data >= (neutrino_msg_data_t)g_settings.parentallock_lockage) &&
					 ((CZapit::getInstance()->findChannelByChannelID(current_channel_id)->last_unlocked_EPGid != g_RemoteControl->current_EPGid) || (g_RemoteControl->current_EPGid == 0)) &&
					 ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED) || (data >= 0x100)))
				{
					g_RemoteControl->stopvideo();
					
					zapProtection = new CZapProtection( g_settings.parentallock_pincode, data );
					
					if ( zapProtection->check() )
					{
						g_RemoteControl->startvideo(current_channel_id);
						
						// remember it for the next time
						CZapit::getInstance()->findChannelByChannelID(current_channel_id)->last_unlocked_EPGid = g_RemoteControl->current_EPGid;
					}
					
					delete zapProtection;
					zapProtection = NULL;
				}
				else
					g_RemoteControl->startvideo(current_channel_id);
			}
		}
		else
			g_RemoteControl->startvideo(current_channel_id);
	}
}

void CRemoteControl::processAPIDnames()
{
	//FIXME:
	has_unresolved_ctags = false;
	has_ac3 = false; //FIXME what this variable suppoused to do ?? seems unused
	int pref_found = -1;
	int pref_ac3_found = -1;
	int pref_idx = -1;
	int pref_ac3_idx = -1;
	int ac3_found = -1;
	const char * desc;
	char lang[4];

	if(g_settings.auto_lang) 
	{
		// first we check prefs to find pid according to pref index
		for(int i = 0; i < 3; i++) 
		{
			for(int j = 0; j < (int) current_PIDs.APIDs.size(); j++) 
			{
				desc = current_PIDs.APIDs[j].desc;
				// In some cases AAC is the only audio system used
				// so accept it here as a 'normal' sound track
				if(strstr(desc, "(AAC)")) 
				{
					strncpy(lang, desc, 3);
					lang[3] = 0;
					desc = lang;
				}
				
				// processAPIDnames called 2 times, TODO find better way to detect second call
				if(strlen( desc ) != 3)
					continue;
				
				if(strlen(g_settings.pref_lang[i]) == 0)
					continue;

				std::string temp(g_settings.pref_lang[i]);
				std::map<std::string, std::string>::const_iterator it;
				
				for(it = iso639.begin(); it != iso639.end(); it++) 
				{
					if(temp == it->second && strcasecmp(desc, it->first.c_str()) == 0) 
					{
						// remember first pref found index and pid
						if(pref_found < 0) 
						{
							pref_found = j;
							pref_idx = i;
						}
						
						if( current_PIDs.APIDs[j].is_ac3 && g_settings.audio_DolbyDigital && (pref_ac3_found < 0)) {
							pref_ac3_found = j;
							pref_ac3_idx = i;
						}
						break;
					}
				}
			}
		}

		// reset pref ac3, if it have lower priority
		if((pref_idx >= 0) && (pref_idx < pref_ac3_idx))
			pref_ac3_found = -1;
	}

	for(unsigned int count = 0; count < current_PIDs.APIDs.size(); count++)
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::processAPIDnames: apid name= %s (%s) pid= 0x%x\n", current_PIDs.APIDs[count].desc, getISO639Description( current_PIDs.APIDs[count].desc ), current_PIDs.APIDs[count].pid);
		
		if ( current_PIDs.APIDs[count].component_tag != 0xFF )
		{
			has_unresolved_ctags = true;
		}
		
		if ( strlen( current_PIDs.APIDs[count].desc ) == 3 )
		{
			// unaufgeloeste Sprache...
			strcpy( current_PIDs.APIDs[count].desc, getISO639Description( current_PIDs.APIDs[count].desc ) );
		}

		if ( current_PIDs.APIDs[count].is_ac3 )
		{
			//strncat(current_PIDs.APIDs[count].desc, " (Dolby Digital)", 25);
			has_ac3 = true;
			if ((strlen( current_PIDs.APIDs[count].desc ) == 3) && g_settings.audio_DolbyDigital && (ac3_found < 0))
				ac3_found = count;
		}
	}

	if ( has_unresolved_ctags )
	{
		if ( current_EPGid != 0 )
		{
			CSectionsd::ComponentTagList tags;

			if ( CSectionsd::getInstance()->getComponentTagsUniqueKey( current_EPGid, tags ) )
			{
				has_unresolved_ctags = false;

				for (unsigned int i = 0; i < tags.size(); i++)
				{
					for (unsigned int j = 0; j < current_PIDs.APIDs.size(); j++)
					{
						if ( current_PIDs.APIDs[j].component_tag == tags[i].componentTag )
						{
							// workaround for buggy ZDF ctags / or buggy sectionsd/drivers , who knows...
							if(!tags[i].component.empty())
							{
								strncpy(current_PIDs.APIDs[j].desc, tags[i].component.c_str(), 25);
							}
							current_PIDs.APIDs[j].component_tag = -1;
							break;
						}
					}
				}
			}
		}
	}
	
	dprintf(DEBUG_NORMAL, "CRemoteControl::processAPIDnames: pref_found %d pref_ac3_found %d ac3_found %d\n", pref_found, pref_ac3_found, ac3_found);
	
	if(pref_ac3_found >= 0) 
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::processAPIDnames: set apid name= %s pid= 0x%x\n", current_PIDs.APIDs[pref_ac3_found].desc, current_PIDs.APIDs[pref_ac3_found].pid);
		setAPID(pref_ac3_found);
	} 
	else if(pref_found >= 0) 
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::processAPIDnames: set apid name= %s pid= 0x%x\n", current_PIDs.APIDs[pref_found].desc, current_PIDs.APIDs[pref_found].pid);
		setAPID(pref_found);
	}
	else if(ac3_found >= 0) 
	{
		dprintf(DEBUG_NORMAL, "CRemoteControl::processAPIDnames: set apid name= %s pid= 0x%x\n", current_PIDs.APIDs[ac3_found].desc, current_PIDs.APIDs[ac3_found].pid);
		setAPID(ac3_found);
	}
	else if ( current_PIDs.otherPIDs.selected_apid >= current_PIDs.APIDs.size() )
	{
		setAPID( 0 );
	}
			
	g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOTAPIDS, (const neutrino_msg_data_t)current_channel_id, false);
}

void CRemoteControl::copySubChannelsToZapit(void)
{
	CZapit::subServiceList zapitList;

	for (CSubServiceListSorted::const_iterator e = subChannels.begin(); e != subChannels.end(); e++)
		zapitList.push_back(e->getAsZapitSubService());

	CZapit::getInstance()->setSubServices(zapitList);
}

void CRemoteControl::setAPID( uint32_t APID )
{
	if ((current_PIDs.otherPIDs.selected_apid == APID ) || (APID >= current_PIDs.APIDs.size()))
		return;

	current_PIDs.otherPIDs.selected_apid = APID;
	CZapit::getInstance()->setAudioChannel( APID );
	
	// needed for auto audio select
	CLCD::getInstance()->setMovieAudio(current_PIDs.APIDs[current_PIDs.otherPIDs.selected_apid].is_ac3);
	CLCD::getInstance()->ShowIcon(VFD_ICON_DOLBY, current_PIDs.APIDs[current_PIDs.otherPIDs.selected_apid].is_ac3? true : false);
}

static const std::string empty_string;

const std::string & CRemoteControl::setSubChannel(const int numSub, const bool force_zap)
{
	if ((numSub < 0) || (numSub >= (int)subChannels.size()))
		return empty_string;

	if ((selected_subchannel == numSub ) && (!force_zap))
		return empty_string;

	selected_subchannel = numSub;
	current_sub_channel_id = subChannels[numSub].getChannelID();
	g_InfoViewer->chanready = 0;

	if(scrambled_timer) 
	{
		g_RCInput->killTimer(scrambled_timer);
		scrambled_timer = 0;
	}

	CZapit::getInstance()->zapToSubServiceIDNOWAIT( current_sub_channel_id );
	
	// Houdini: to restart reading the private EPG when switching to a new option
	CSectionsd::getInstance()->setServiceChanged( current_sub_channel_id , true );

	return subChannels[numSub].subservice_name;
}

const std::string & CRemoteControl::subChannelUp(void)
{
 	// if there are any NVOD/subchannels switch these else switch audio channel (if any)
  	if (subChannels.size() > 0 )
  	{
  		return setSubChannel((subChannels.size() == 0) ? -1 : (int)((selected_subchannel + 1) % subChannels.size()));
  	}
  	else
  	{
  		if (current_PIDs.APIDs.size() > 0)
  		{
  			setAPID((current_PIDs.otherPIDs.selected_apid + 1) % current_PIDs.APIDs.size());
  		}
  		return (empty_string);
  	}
}

const std::string & CRemoteControl::subChannelDown(void)
{
	// if there are any NVOD/subchannels switch these else switch audio channel (if any)
  	if (subChannels.size() > 0 )
  	{
  		return setSubChannel((selected_subchannel <= 0) ? (subChannels.size() - 1) : (selected_subchannel - 1));
  	}
  	else
  	{
  		if (current_PIDs.APIDs.size() > 0)
  		{
  			if (current_PIDs.otherPIDs.selected_apid <= 0)
  				setAPID(current_PIDs.APIDs.size() - 1);
  			else
  				setAPID((current_PIDs.otherPIDs.selected_apid - 1));
  		}
  		return (empty_string);
  	}
}

//
void CRemoteControl::zapToChannelID(const t_channel_id channel_id, const bool start_video) // UTF-8
{
	dprintf(DEBUG_NORMAL, "CRemoteControl::zapToChannelID: 0x%llx\n", channel_id);
	
	if (start_video)
		startvideo(channel_id);
	else
		stopvideo();

	current_sub_channel_id = 0;
	current_EPGid = 0;
	next_EPGid = 0;

	memset(&current_PIDs.otherPIDs, 0, sizeof(current_PIDs.otherPIDs) );

	current_PIDs.APIDs.clear();
	has_ac3 = false;

	subChannels.clear();
	selected_subchannel = -1;
	needs_nvods = false;
	director_mode = 0;

	unsigned long long now = getcurrenttime();
	
	if ( zap_completion_timeout < now )
	{
		g_InfoViewer->chanready = 0;		
		
		if(autoshift) 
		{
			CNeutrinoApp::getInstance()->stopAutoRecord();
		}
		
		if(scrambled_timer) 
		{
			g_RCInput->killTimer(scrambled_timer);
			scrambled_timer = 0;
		}

		// zap
		CZapit::getInstance()->zapToServiceIDNOWAIT(channel_id);

		zap_completion_timeout = now + 2 * (long long) 1000000;
	}
	
	current_channel_id = channel_id;
}

void CRemoteControl::startvideo(t_channel_id channel_id)
{
	CZapitChannel *chan = CZapit::getInstance()->findChannelByChannelID(channel_id);
	
	if ( !is_video_started )
	{
		is_video_started = true;
		CZapit::getInstance()->startPlayBack(chan);
	}
}

void CRemoteControl::stopvideo()
{
	if ( is_video_started )
	{
		is_video_started = false;
		CZapit::getInstance()->stopPlayBack();
	}
}

void CRemoteControl::radioMode()
{
	dprintf(DEBUG_NORMAL, "CRemoteControl::radioMode\n");
	
	CZapit::getInstance()->setMode( CZapit::MODE_RADIO );
	
	CLCD::getInstance()->ShowIcon(VFD_ICON_RADIO, true);
	CLCD::getInstance()->ShowIcon(VFD_ICON_TV, false);
}

void CRemoteControl::tvMode()
{
	dprintf(DEBUG_NORMAL, "CRemoteControl::tvMode\n");
	
	CZapit::getInstance()->setMode( CZapit::MODE_TV );
	
	CLCD::getInstance()->ShowIcon(VFD_ICON_RADIO, false);
	CLCD::getInstance()->ShowIcon(VFD_ICON_TV, true);
}

