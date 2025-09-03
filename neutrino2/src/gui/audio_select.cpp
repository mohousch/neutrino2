//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: audio_select.cpp 21122024 mohousch Exp $
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

#include <global.h>
#include <neutrino2.h>

#include <gui/audio_select.h>
#include <gui/audio_setup.h>

#include <system/debug.h>

#include <audio_cs.h>


//// globals
// dvbsub
extern int dvbsub_getpid();				// defined in libdvbsub
extern int dvbsub_stop();
extern int dvbsub_start(int pid, bool isEplayer);
extern int dvbsub_pause();
// tuxtxt
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_main(int pid, int page, bool isEplayer);
extern void tuxtx_pause_subtitle(bool pause, bool isEplayer);
extern int tuxtx_subtitle_running(int * pid, int * page, int * running);
//
extern int current_muted;

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
}; 

#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, _("stereo")   },
	{ 1, _("mono left") },
	{ 2, _("mono right") }
};

// ac3
#define AC3_OPTION_COUNT 2
const keyval AC3_OPTIONS[AC3_OPTION_COUNT] =
{
	{ AC3_PASSTHROUGH, _("passthrough") },
	{ AC3_DOWNMIX, _("downmix") }
};

int CAudioSelectMenuHandler::exec(CWidgetTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CAudioSelectMenuHandler::exec:\n");

	int res = CWidgetTarget::RETURN_REPAINT;

	if (parent) 
		parent->hide();

	res = doMenu();

	return res;
}

//
int CAudioSelectMenuHandler::doMenu()
{
	dprintf(DEBUG_NORMAL, "CAudioSelectMenuHandler::doMenu\n");
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	//
	CWidget* widget = NULL;
	ClistBox* AudioSelector = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("audioselect");
	
	if (widget)
	{
		AudioSelector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "audioselect";
		
		//			
		AudioSelector = new ClistBox(&box);

		AudioSelector->setWidgetMode(ClistBox::MODE_SETUP);
		AudioSelector->enableShrinkMenu();
						
		AudioSelector->enablePaintHead();
		AudioSelector->setTitle(_("Select language"), NEUTRINO_ICON_AUDIO);

		AudioSelector->enablePaintFoot();
							
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
							
		AudioSelector->setFootButtons(&btn);
						
		//
		widget->addCCItem(AudioSelector);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Select language"));
	
	//
	unsigned int count;
	
	CAPIDChangeExec APIDChanger;
	CSubtitleChangeExec SubtitleChanger;
	
	// audio pids
	for(count = 0; count < g_RemoteControl->current_PIDs.APIDs.size(); count++ ) 
	{
		char apid[5];
		sprintf(apid, "%d", count);

		AudioSelector->addItem(new CMenuForwarder(g_RemoteControl->current_PIDs.APIDs[count].desc, true, NULL, &APIDChanger, apid, CRCInput::convertDigitToKey(count + 1)), (count == g_RemoteControl->current_PIDs.otherPIDs.selected_apid));
	}

	if(g_RemoteControl->current_PIDs.APIDs.size())
		AudioSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// analogue output
	AudioSelector->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	// ac3	
	AudioSelector->addItem(new CMenuOptionChooser(_("Dolby Digital"), &g_settings.hdmi_dd, AC3_OPTIONS, AC3_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN ));

	//dvb/tuxtxt subs
	CZapitChannel * cc = CZapit::getInstance()->getCurrentChannel();

	bool sep_added = false;
	if(cc) 
	{
		for (int i = 0 ; i < (int)cc->getSubtitleCount() ; ++i) 
		{
			CZapitAbsSub* s = cc->getChannelSub(i);
			
			//dvbsub
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
				
				printf("CAudioSelectMenuHandler::doMenu: adding DVB subtitle lang=%s pid=0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);
				
				if(!sep_added) 
				{
					sep_added = true;
					AudioSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Subtitles")));
				}
				
				char spid[10];
				snprintf(spid,sizeof(spid), "DVB:%d", sd->pId);
				char item[64];
				
				snprintf(item, sizeof(item), "DVB: %s", sd->ISO639_language_code.c_str());
				
				AudioSelector->addItem(new CMenuForwarder(item, sd->pId != dvbsub_getpid(), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
			
			//txtsub
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				
				printf("CAudioSelectMenuHandler::doMenu: adding TTX subtitle lang=%s pid=0x%x page=0x%1X%02X\n", sd->ISO639_language_code.c_str(), sd->pId, sd->teletext_magazine_number, sd->teletext_page_number);
				
				if(!sep_added) 
				{
					sep_added = true;
					AudioSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Subtitles")));
				}
				
				char spid[64];
				int page = ((sd->teletext_magazine_number & 0xFF) << 8) | sd->teletext_page_number;
				int pid = sd->pId;
				
				snprintf(spid, sizeof(spid), "TTX:%d:%03X:%s", sd->pId, page, sd->ISO639_language_code.c_str()); 
				char item[64];
				snprintf(item, sizeof(item), "TTX: %s", sd->ISO639_language_code.c_str());
				AudioSelector->addItem(new CMenuForwarder(item,  !tuxtx_subtitle_running(&pid, &page, NULL), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
		}
		
		if(sep_added) 
		{
//			AudioSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
			AudioSelector->addItem(new CMenuForwarder(_("Stop subtitles"), true, NULL, &SubtitleChanger, "off", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW ));
		}

	}
	
	// volume percent
	sep_added = false;
	int percent[g_RemoteControl->current_PIDs.APIDs.size()];
	
	for(count = 0; count < g_RemoteControl->current_PIDs.APIDs.size(); count++) 
	{
		CZapit::getInstance()->getVolumePercent((unsigned int *) &percent[count], 0, g_RemoteControl->current_PIDs.APIDs[count].pid);
		int is_active = count == g_RemoteControl->current_PIDs.otherPIDs.selected_apid;
		
		if(!sep_added) 
		{
			sep_added = true;
			AudioSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Volume adjustment (in %)")));
		}
		
		AudioSelector->addItem(new CMenuOptionNumberChooser(g_RemoteControl->current_PIDs.APIDs[count].desc, &percent[count], is_active, 0, 100, this));
	}

	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
        	widget = NULL;
        }
        
        //
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

bool CAudioSelectMenuHandler::changeNotify(const std::string& OptionName __attribute__((unused)), void *data)
{
	int per= *(int *) data;
	
	CZapit::getInstance()->setVolumePercent(per, CZapit::getInstance()->getCurrentChannelID(), g_RemoteControl->current_PIDs.otherPIDs.selected_apid);
	
	return false;
}

//// apid change notifier
int CAPIDChangeExec::exec(CWidgetTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CAPIDChangeExec exec: %s\n", actionKey.c_str());

	unsigned int sel = atoi(actionKey.c_str());
	
	if (g_RemoteControl->current_PIDs.otherPIDs.selected_apid != sel )
	{
		g_RemoteControl->setAPID(sel);
	}

	return CWidgetTarget::RETURN_EXIT;
}

//// txt/dvb subtitle
int CSubtitleChangeExec::exec(CWidgetTarget *, const std::string & actionKey)
{
	dprintf(DEBUG_INFO, "CSubtitleChangeExec::exec: action %s\n", actionKey.c_str());
	
	if(actionKey == "off") 
	{
		// tuxtxt stop
		tuxtx_stop_subtitle(); //this kill subthread
		
		// dvbsub stop
		dvbsub_stop();
		
		return RETURN_EXIT;
	}
	
	if(!strncmp(actionKey.c_str(), "DVB", 3)) 
	{
		char const * pidptr = strchr(actionKey.c_str(), ':');
		int pid = atoi(pidptr + 1);
		
		// tuxtxt stop
		tuxtx_stop_subtitle();
		
		dvbsub_pause();
		dvbsub_start(pid, false);
	} 
	else if (!strncmp(actionKey.c_str(), "TTX", 3))
	{
		char const * ptr = strchr(actionKey.c_str(), ':');
		ptr++;
		int pid = atoi(ptr);
		ptr = strchr(ptr, ':');
		ptr++;
		int page = strtol(ptr, NULL, 16);
		ptr = strchr(ptr, ':');
		ptr++;
		
		dvbsub_stop();
		
		tuxtx_stop_subtitle();
		tuxtx_set_pid(pid, page, ptr);
		tuxtx_main(pid, page, false);
	}
	
        return RETURN_EXIT;
}

//// tuxtxt
int CTuxtxtChangeExec::exec(CWidgetTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CTuxtxtChangeExec exec: %s\n", actionKey.c_str());

	if(parent)
		parent->hide();
	
	if (!IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
	{

		CNeutrinoApp::getInstance()->stopSubtitles();
				
		tuxtx_stop_subtitle();
		tuxtx_main(g_RemoteControl->current_PIDs.otherPIDs.vtxtpid, 0, false);
				
		CNeutrinoApp::getInstance()->audioMute(current_muted, true);

		CNeutrinoApp::getInstance()->startSubtitles();
	}

	return RETURN_REPAINT;
}

