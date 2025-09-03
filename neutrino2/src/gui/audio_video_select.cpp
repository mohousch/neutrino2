//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: audio_video_select.cpp 21122024 mohousch Exp $
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

#include <stdio.h>

#include <algorithm>    // std::sort
#include <fstream>
#include <iostream>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>
#include <system/helpers.h>

#include <playback_cs.h>
#include <video_cs.h>
#include <audio_cs.h>

#include <gui/audio_video_select.h>
#include <gui/video_setup.h>
#include <gui/audio_setup.h>
#include <gui/movieplayer.h>
#include <gui/filebrowser.h>

#include <system/helpers.h>


//// globals
unsigned short apids[10];
bool ac3flags[10] = {false};
unsigned short numpida = 0;
unsigned short vpid = 0;
unsigned short vtype = 0;
std::string language[100];
unsigned int currentapid = 0;
unsigned int currentac3 = 0;
unsigned int ac3state = CInfoViewer::NO_AC3;
//
unsigned short spids[10];
unsigned short numpids = 0;
int currentspid = -1;
////
bool isEXtSub = false;
unsigned short extspids[10];
unsigned short extnumpids = 0;
int currentextspid = -1;
std::string subtitle_file;
//
extern cPlayback *playback;

#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, _("stereo")   },
	{ 1, _("mono left") },
	{ 2, _("mono right") }
};

#define AC3_OPTION_COUNT 2
const keyval AC3_OPTIONS[AC3_OPTION_COUNT] =
{
	{ AC3_PASSTHROUGH, "passthrough" },
	{ AC3_DOWNMIX, "downmix" }
};

////
int CAVPIDChangeExec::exec(CWidgetTarget*, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CAVPIDSelectWidget::exec: %s (currentapid:%d)\n", actionKey.c_str(), currentapid);
	
	unsigned int sel = atoi(actionKey.c_str());

	if (currentapid != apids[sel]) 
	{
		currentapid = apids[sel];
		currentac3 = ac3flags[sel];
		
		if(playback)
			playback->SetAPid(currentapid);
		
		ac3state = currentac3? CInfoViewer::AC3_ACTIVE : CInfoViewer::NO_AC3;
	}
	
	return CWidgetTarget::RETURN_EXIT_ALL;
}

////
int CAVSubPIDChangeExec::exec(CWidgetTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CAVSubPIDSelectWidget::exec: %s (currentspid:%d)\n", actionKey.c_str(), currentspid);
	
#ifdef ENABLE_GSTREAMER
	unsigned int sel = atoi(actionKey.c_str());
	
	if (currentspid != spids[sel]) 
	{
		currentspid = spids[sel];
		
		if(playback)
			playback->SetSubPid(currentspid);
			
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey == "off") 
	{
		currentspid = -1;
		
		if(playback)
			playback->SetSubPid(-1);
			
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
#else
	if (strstr(actionKey.c_str(), "DVB") || strstr(actionKey.c_str(), "PGS") || strstr(actionKey.c_str(), "SUBRIP") || strstr(actionKey.c_str(), "ASS") || strstr(actionKey.c_str(), "SSA") || strstr(actionKey.c_str(), "SRT") || strstr(actionKey.c_str(), "UTF-8") || strstr(actionKey.c_str(), "XSUB"))
	{
		isEXtSub = (strstr(actionKey.c_str(), "(EXT)"));
		
		char const * pidptr = strchr(actionKey.c_str(), ':');
		
		if (isEXtSub)
		{
			currentextspid = atoi(pidptr + 1);
			currentspid = -1;
			
			if(playback)
				playback->SetExtSubPid(currentextspid);
		}
		else
		{
			currentextspid = -1;
			currentspid = atoi(pidptr + 1);
			
			if(playback)
				playback->SetSubPid(currentspid);
		}
			
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
	else if (strstr(actionKey.c_str(), "TELETEXT"))
	{
		char const * pidptr = strchr(actionKey.c_str(), ':');
		
		currentspid = atoi(pidptr + 1);
		currentextspid = -1;
		
		if(playback)
			playback->SetSubPid(currentspid);
			
		return CWidgetTarget::RETURN_EXIT_ALL;		
	}
	else if(actionKey == "off") 
	{
		currentspid = -1;
		currentextspid = -1;
		
		if(playback)
		{
			playback->SetSubPid(-1);
			playback->SetExtSubPid(-1);
		}
			
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
#endif

	return CWidgetTarget::RETURN_EXIT_ALL;
}

////
int CAVPIDSelectWidget::exec(CWidgetTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CAVPIDSelectWidget::exec: %s\n", actionKey.c_str());
	
	int res = CWidgetTarget::RETURN_REPAINT;

	if (parent) 
		parent->hide();
		
#ifndef ENABLE_GSTREAMER
	if(actionKey == "add_subtitle")
	{
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("srt");
		fileFilter.addFilter("ass");
		fileFilter.addFilter("ass");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec(g_settings.network_nfs_recordingdir) == true)
		{
			strcpy(g_settings.font_file, fileBrowser.getSelectedFile()->Name.c_str());
			
			dprintf(DEBUG_NORMAL, "CAVPIDSelectWidget::exec: Add Subtitle File %s\n", fileBrowser.getSelectedFile()->Name.c_str());
			
			if (playback)
				playback->AddSubtitleFile(fileBrowser.getSelectedFile()->Name.c_str());
		}
		
		currentextspid = -1;
		
		hide();
		showAudioDialog();
		
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
#endif

	res = showAudioDialog();

	return res;
}

int CAVPIDSelectWidget::showAudioDialog(void)
{
	dprintf(DEBUG_NORMAL, "CAVPIDSelectWidget::showAudioDialog (currentapid:%d)\n", currentapid);
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	//
	CWidget* widget = NULL;
	ClistBox* AVPIDSelector = NULL;
	CMenuItem* item = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("avselect");
	
	if (widget)
	{
		AVPIDSelector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "avselect";
		
		//			
		AVPIDSelector = new ClistBox(&box);

		AVPIDSelector->setWidgetMode(ClistBox::MODE_SETUP);
		AVPIDSelector->enableShrinkMenu();
						
		AVPIDSelector->enablePaintHead();
		AVPIDSelector->setTitle(_("AV Select"), NEUTRINO_ICON_AUDIO);

		AVPIDSelector->enablePaintFoot();
							
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
							
		AVPIDSelector->setFootButtons(&btn);
						
		//
		widget->addCCItem(AVPIDSelector);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("AV Select"));
	
	// audio pids
	CAVPIDChangeExec AVPIDChanger;
	numpida = 0;
	
	if(playback)
		playback->FindAllPids(apids, ac3flags, &numpida, language);
			
	if (numpida > 0) 
	{
		for (unsigned int count = 0; count < numpida; count++) 
		{
			char apidnumber[64];
			sprintf(apidnumber, "%d", count);
			
			std::string apidtitle = "Stream ";

			// language
			if (!language[count].empty())
			{
				apidtitle = language[count];
			}
			
			item = new CMenuForwarder(apidtitle.c_str(), true, NULL, &AVPIDChanger, apidnumber, CRCInput::convertDigitToKey(count + 1)), (count == currentapid);
			
			if (currentapid == count)
				item->setIcon1(NEUTRINO_ICON_MARK);

			AVPIDSelector->addItem(item, (count == currentapid));
		}
		
		AVPIDSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	}
	
	// analogue output
	AVPIDSelector->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	//ac3				
	AVPIDSelector->addItem(new CMenuOptionChooser(_("Dolby Digital"), &g_settings.hdmi_dd, AC3_OPTIONS, AC3_OPTION_COUNT, true, CAudioSettings::getInstance()->audioSetupNotifier, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN ));				
	
	// subs
	CAVSubPIDChangeExec AVSubPIDChanger;
	numpids = 0;
	
	if(playback)
		playback->FindAllSubPids(spids, &numpids, language);
	
	if (numpids > 0) 
	{
		AVPIDSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));

		for (int count = 0; count < numpids; count++) 
		{
			char spidnumber[64];
			
			std::string spidtitle = "Sub ";

			// language
			if (!language[count].empty())
			{
				spidtitle = language[count];
			}
			
#ifdef ENABLE_GSTREAMER
			sprintf(spidnumber, "%d", count);
#else
			sprintf(spidnumber, "%s:%d", spidtitle.c_str(), count); // dont change this
#endif

			AVPIDSelector->addItem(new CMenuForwarder(spidtitle.c_str(), currentspid == count? false : true, NULL, &AVSubPIDChanger, spidnumber, CRCInput::convertDigitToKey(count + 1)));
		}
	}
	
	// add subtitle file
	extnumpids = 0;
	
#ifndef ENABLE_GSTREAMER
	if (CNeutrinoApp::getInstance()->getMode() == CNeutrinoApp::mode_ts)
	{	
		AVPIDSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
		AVPIDSelector->addItem(new CMenuForwarder(_("Add Subtitle File"), true, subtitle_file.c_str(), this, "add_subtitle"));
		
		if(playback)
			playback->FindAllExtSubPids(extspids, &extnumpids, language);
		
		if (extnumpids > 0) 
		{

			for (int count = 0; count < extnumpids; count++) 
			{
				char spidnumber[64];
				
				std::string spidtitle = "Sub ";

				// language
				if (!language[count].empty())
				{
					spidtitle = language[count];
				}
				
				sprintf(spidnumber, "%s:%d(EXT)", spidtitle.c_str(), count); // dont change this

				AVPIDSelector->addItem(new CMenuForwarder(spidtitle.c_str(), currentextspid == count? false : true, NULL, &AVSubPIDChanger, spidnumber, CRCInput::convertDigitToKey(count + 1)));
			}
		}
	}
	
	if (extnumpids || numpids)
	{
		AVPIDSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		AVPIDSelector->addItem(new CMenuForwarder(_("Stop subtitles"), true, NULL, &AVSubPIDChanger, "off", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	}
#endif

	//	
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

