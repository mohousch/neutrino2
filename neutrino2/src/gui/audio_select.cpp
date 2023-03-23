/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: audio_select.cpp 2013/10/12 mohousch Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>

#include <gui/audio_select.h>
#include <gui/audio_setup.h>

#include <system/debug.h>

#include <audio_cs.h>


extern t_channel_id live_channel_id;
extern CRemoteControl * g_RemoteControl; 		/* defined neutrino.cpp */

// dvbsub
extern int dvbsub_getpid();				// defined in libdvbsub
//extern int dvbsub_getpid(int * pid, int * running);	// defined in libdvbsub

// tuxtxt
extern int tuxtx_subtitle_running(int * pid, int * page, int * running);


// -- this is a copy from neutrino.cpp!!
/* option off0_on1 */
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("Off") },
        { 1, _("On") }
}; 

#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, _("stereo")   },
	{ 1, _("mono left") },
	{ 2, _("mono right") }
};

// ac3
#if !defined (PLATFORM_COOLSTREAM)
#define AC3_OPTION_COUNT 2
const keyval AC3_OPTIONS[AC3_OPTION_COUNT] =
{
	{ AC3_PASSTHROUGH, _("passthrough") },
	{ AC3_DOWNMIX, _("downmix") }
};
#endif

int CAudioSelectMenuHandler::exec(CMenuTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CAudioSelectMenuHandler::exec:\n");

	int res = RETURN_REPAINT;

	if (parent) 
		parent->hide();

	doMenu();

	return res;
}

//
void CAudioSelectMenuHandler::doMenu()
{
	dprintf(DEBUG_NORMAL, "CAudioSelectMenuHandler::doMenu\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* AudioSelector = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("audioselect");
	
	if (widget)
	{
		AudioSelector = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{			
		AudioSelector = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		AudioSelector->setWidgetMode(MODE_SETUP);
		AudioSelector->enableShrinkMenu();
						
		AudioSelector->enablePaintHead();
		AudioSelector->setTitle(_("Select language"), NEUTRINO_ICON_AUDIO);

		AudioSelector->enablePaintFoot();
							
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
							
		AudioSelector->setFootButtons(&btn);
						
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "audioselect";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(AudioSelector);
	}
	
	AudioSelector->clearItems();
	
	//
	unsigned int count;
	
	CAPIDChangeExec APIDChanger;
	CSubtitleChangeExec SubtitleChanger;
	
	// audio pids
	for(count = 0; count < g_RemoteControl->current_PIDs.APIDs.size(); count++ ) 
	{
		char apid[5];
		sprintf(apid, "%d", count);

		AudioSelector->addItem(new ClistBoxItem(g_RemoteControl->current_PIDs.APIDs[count].desc, true, NULL, &APIDChanger, apid, CRCInput::convertDigitToKey(count + 1)), (count == g_RemoteControl->current_PIDs.PIDs.selected_apid));
	}

	if(g_RemoteControl->current_PIDs.APIDs.size())
		AudioSelector->addItem(new CMenuSeparator(LINE));

	// analogue output
	AudioSelector->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	// ac3
#if !defined (PLATFORM_COOLSTREAM)	
	AudioSelector->addItem(new CMenuOptionChooser(_("Dolby Digital"), &g_settings.hdmi_dd, AC3_OPTIONS, AC3_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, RC_green, NEUTRINO_ICON_BUTTON_GREEN ));
#endif

	//dvb/tuxtxt subs
	CChannelList * channelList = CNeutrinoApp::getInstance()->channelList;
	int curnum = channelList->getActiveChannelNumber();
	CZapitChannel * cc = channelList->getChannel(curnum);

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
				printf("[CAudioSelectMenuHandler] adding DVB subtitle %s pid 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);
				if(!sep_added) 
				{
					sep_added = true;
					AudioSelector->addItem(new CMenuSeparator(LINE | STRING, _("Subtitles")));
				}
				char spid[10];
				//int pid = sd->pId;
				snprintf(spid,sizeof(spid), "DVB:%d", sd->pId);
				char item[64];
				//snprintf(item,sizeof(item), "DVB: %s (pid %x)", sd->ISO639_language_code.c_str(), sd->pId);
				snprintf(item, sizeof(item), "DVB: %s", sd->ISO639_language_code.c_str());
				AudioSelector->addItem(new ClistBoxItem(item, sd->pId != dvbsub_getpid() /* !dvbsub_getpid(&pid, NULL)*/, NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
			
			//txtsub
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				printf("[CAudioSelectMenuHandler] adding TTX subtitle %s pid 0x%x mag 0x%X page 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId, sd->teletext_magazine_number, sd->teletext_page_number);
				if(!sep_added) 
				{
					sep_added = true;
					AudioSelector->addItem(new CMenuSeparator(LINE | STRING, _("Subtitles")));
				}
				char spid[64];
				int page = ((sd->teletext_magazine_number & 0xFF) << 8) | sd->teletext_page_number;
				int pid = sd->pId;
				snprintf(spid,sizeof(spid), "TTX:%d:%03X:%s", sd->pId, page, sd->ISO639_language_code.c_str()); 
				char item[64];
				//snprintf(item, sizeof(item), "TTX: %s (pid %x page %03X)", sd->ISO639_language_code.c_str(), sd->pId, page);
				snprintf(item, sizeof(item), "TTX: %s", sd->ISO639_language_code.c_str());
				AudioSelector->addItem(new ClistBoxItem(item,  !tuxtx_subtitle_running(&pid, &page, NULL), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
		}
		
		if(sep_added) 
		{
			AudioSelector->addItem(new CMenuSeparator(LINE));
			AudioSelector->addItem(new ClistBoxItem(_("Stop subtitles"), true, NULL, &SubtitleChanger, "off", RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW ));
		}

	}
	
	// volume percent
	CAudioSetupNotifierVolPercent * audioSetupNotifierVolPercent = new CAudioSetupNotifierVolPercent;
	int percent[g_RemoteControl->current_PIDs.APIDs.size()];
	
	for(count = 0; count < g_RemoteControl->current_PIDs.APIDs.size(); count++ ) 
	{
		CZapit::getInstance()->getVolumePercent((unsigned int *) &percent[count], 0, g_RemoteControl->current_PIDs.APIDs[count].pid);
		int is_active = count == g_RemoteControl->current_PIDs.PIDs.selected_apid;
		
		if(!sep_added) 
		{
			sep_added = true;
			AudioSelector->addItem(new CMenuSeparator(LINE | STRING, _("Volume adjustment (in %)")));
		}
		
		AudioSelector->addItem(new CMenuOptionNumberChooser("", &percent[count],
			is_active,
			0, 100, audioSetupNotifierVolPercent, 0, 0,
			g_RemoteControl->current_PIDs.APIDs[count].desc));
	}

	widget->exec(NULL, "");

	delete audioSetupNotifierVolPercent;
}

// apid change notifier
int CAPIDChangeExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CAPIDChangeExec exec: %s\n", actionKey.c_str());

	unsigned int sel = atoi(actionKey.c_str());
	
	if (g_RemoteControl->current_PIDs.PIDs.selected_apid != sel )
	{
		g_RemoteControl->setAPID(sel);
	}

	return RETURN_EXIT;
}

// volume conf
bool CAudioSetupNotifierVolPercent::changeNotify(const std::string& OptionName __attribute__((unused)), void *data)
{
	int percent = *(int *) data;
	
	CZapit::getInstance()->setVolumePercent(percent, live_channel_id, g_RemoteControl->current_PIDs.PIDs.selected_apid);
	
	return true;
}

