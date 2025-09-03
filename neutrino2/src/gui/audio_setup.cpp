/*
	Neutrino-GUI  -   DBoxII-Project

	$id: audio_setup.cpp 2016.01.02 19:38:30 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <stdio.h> 

#include <gui/audio_setup.h>


#include <system/debug.h>
#include <system/helpers.h>

#include <audio_cs.h>
#include <video_cs.h>


extern cVideo * videoDecoder;		// libdvbapi (video_cs.cpp)
extern cAudio * audioDecoder;		// libdvbapi (audio_cs.cpp)

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, _("stereo") },
	{ 1, _("mono left") },
	{ 2, _("mono right") }
};

#define AUDIOMENU_AVSYNC_OPTION_COUNT 3
const keyval AUDIOMENU_AVSYNC_OPTIONS[AUDIOMENU_AVSYNC_OPTION_COUNT] =
{
        { AVSYNC_OFF, _("off") },
        { AVSYNC_ON, _("on") },
        { AVSYNC_AM, _("audio master") }
};

// ac3
#define AC3_OPTION_COUNT 2
const keyval AC3_OPTIONS[AC3_OPTION_COUNT] =
{
	{ AC3_DOWNMIX, "downmix" },
	{ AC3_PASSTHROUGH, "passthrough" }
};

#define AUDIODELAY_OPTION_COUNT 9
const keyval AUDIODELAY_OPTIONS[AUDIODELAY_OPTION_COUNT] =
{
	{ -1000, "-1000" },
	{ -750, "-750" },
	{ -500, "-500" },
	{ -250, "-250" },
	{ 0, "0" },
	{ 250, "250" },
	{ 500, "500" },
	{ 750, "750" },
	{ 1000, "1000" }
};

CAudioSettings::CAudioSettings()
{
	audioSetupNotifier = new CAudioSetupNotifier;
}

CAudioSettings *CAudioSettings::getInstance()
{
	static CAudioSettings *audioSettings = NULL;

	if(!audioSettings)
	{
		audioSettings = new CAudioSettings();
		dprintf(DEBUG_NORMAL, "CAudioSettings::getInstance: Instance created\n");
	}
	
	return audioSettings;
}

CAudioSettings::~CAudioSettings()
{
	delete audioSetupNotifier;
}

int CAudioSettings::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CAudioSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CWidgetTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CAudioSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CAudioSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* audioSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("audiosetup");
	
	if (widget)
	{
		audioSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "audiosetup";
		
		//
		audioSettings = new ClistBox(&box);

		audioSettings->setWidgetMode(ClistBox::MODE_SETUP);
		audioSettings->enableShrinkMenu();
		
		//
		audioSettings->enablePaintHead();
		audioSettings->setTitle(_("Audio settings"), NEUTRINO_ICON_AUDIO);

		//
		audioSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		audioSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(audioSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Audio settings"));
	
	// intros
	audioSettings->addItem(new CMenuForwarder(_("back")));
	audioSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE, NULL, true) );
	
	// save settings
	audioSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	audioSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE, NULL, true) );

	// analog output
	audioSettings->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, audioSetupNotifier));
	
	// hdmi-dd
	audioSettings->addItem(new CMenuOptionChooser(_("Dolby Digital"), &g_settings.hdmi_dd, AC3_OPTIONS, AC3_OPTION_COUNT, true, audioSetupNotifier));		

	// A/V sync
	audioSettings->addItem(new CMenuOptionChooser(_("A/V sync"), &g_settings.avsync, AUDIOMENU_AVSYNC_OPTIONS, AUDIOMENU_AVSYNC_OPTION_COUNT, true, audioSetupNotifier));
		
	// ac3 delay
	audioSettings->addItem(new CMenuOptionChooser(_("AC3 Delay"), &g_settings.ac3_delay, AUDIODELAY_OPTIONS, AUDIODELAY_OPTION_COUNT, true, audioSetupNotifier));
	
	// pcm delay
	audioSettings->addItem(new CMenuOptionChooser(_("PCM Delay"), &g_settings.pcm_delay, AUDIODELAY_OPTIONS, AUDIODELAY_OPTION_COUNT, true, audioSetupNotifier));	
	
	// pref lang
	audioSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Audio language preferences"), true));
	
	// auto ac3 
	CMenuOptionChooser * a1 = new CMenuOptionChooser(_("Dolby Digital"), &g_settings.audio_DolbyDigital, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, g_settings.auto_lang, audioSetupNotifier );
	
	// audiolang
	CMenuOptionStringChooser * audiolangSelect[3];
	
	for(int i = 0; i < 3; i++) 
	{
		audiolangSelect[i] = new CMenuOptionStringChooser(_("Preferred language"), g_settings.pref_lang[i], g_settings.auto_lang, NULL, CRCInput::RC_nokey, "", true);
		
		audiolangSelect[i]->addOption("");
		std::map<std::string, std::string>::const_iterator it;
		for(it = iso639rev.begin(); it != iso639rev.end(); it++) 
			audiolangSelect[i]->addOption(it->first.c_str());
	}
	
	CAutoAudioNotifier * autoAudioNotifier = new CAutoAudioNotifier(a1, audiolangSelect[0], audiolangSelect[1], audiolangSelect[2]);
	
	// auto lang
	audioSettings->addItem(new CMenuOptionChooser(_("Auto select audio"), &g_settings.auto_lang, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, autoAudioNotifier));
	
	// ac3
	audioSettings->addItem(a1);
	
	// lang
	for(int i = 0; i < 3; i++) 
		audioSettings->addItem(audiolangSelect[i]);
	
	// sublang
	audioSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Subtitle language preferences"), true));
	
	CMenuOptionStringChooser * sublangSelect[3];
	for(int i = 0; i < 3; i++) 
	{
		sublangSelect[i] = new CMenuOptionStringChooser(_("Preferred subtitle language"), g_settings.pref_subs[i], g_settings.auto_subs, NULL, CRCInput::RC_nokey, "", true);
		std::map<std::string, std::string>::const_iterator it;
		
		sublangSelect[i]->addOption("");
		for(it = iso639rev.begin(); it != iso639rev.end(); it++) 
			sublangSelect[i]->addOption(it->first.c_str());
	}
	
	CSubLangSelectNotifier * subLangSelectNotifier = new CSubLangSelectNotifier(sublangSelect[0], sublangSelect[1], sublangSelect[2]);
	
	// auto sublang
	audioSettings->addItem(new CMenuOptionChooser(_("Auto select subtitles"), &g_settings.auto_subs, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, subLangSelectNotifier));
	
	// sublang
	for(int i = 0; i < 3; i++) 
		audioSettings->addItem(sublangSelect[i]);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	delete subLangSelectNotifier;
	delete autoAudioNotifier;
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

bool CAudioSetupNotifier::changeNotify(const std::string& OptionName, void *)
{
	dprintf(DEBUG_NORMAL, "CAudioSetupNotifier::changeNotify\n");

	if (OptionName == _("Analog Output")) 
	{
		if(audioDecoder) 
				audioDecoder->setChannel(g_settings.audio_AnalogMode);
	} 
	else if (OptionName == _("Dolby Digital")) 
	{
		if(audioDecoder)
			audioDecoder->SetHdmiDD(g_settings.hdmi_dd );
	}
	else if (OptionName == _("A/V sync")) 
	{
		if(videoDecoder)
			videoDecoder->SetSyncMode(g_settings.avsync);			
		
		if(audioDecoder)
			audioDecoder->SetSyncMode(g_settings.avsync);			
	}	
	else if(OptionName == _("AC3 Delay"))
	{
		if(audioDecoder)
			audioDecoder->setHwAC3Delay(g_settings.ac3_delay);
	}
	else if(OptionName == _("PCM Delay"))
	{
		if(audioDecoder)
			audioDecoder->setHwPCMDelay(g_settings.pcm_delay);
	}	

	return true;
}

CAutoAudioNotifier::CAutoAudioNotifier(CMenuItem * item1, CMenuItem * item2, CMenuItem * item3, CMenuItem * item4)
{
	toDisable[0] = item1;
	toDisable[1] = item2;
	toDisable[2] = item3;
	toDisable[3] = item4;
}

bool CAutoAudioNotifier::changeNotify(const std::string&, void *)
{
	// only ac3
	toDisable[0]->setActive(g_settings.auto_lang);
	toDisable[1]->setActive(g_settings.auto_lang);
	toDisable[2]->setActive(g_settings.auto_lang);
	toDisable[3]->setActive(g_settings.auto_lang);
	
	return true;
}

// sublang select notifier
CSubLangSelectNotifier::CSubLangSelectNotifier(CMenuItem * item1, CMenuItem * item2, CMenuItem * item3)
{
	toDisable[0] = item1;
	toDisable[1] = item2;
	toDisable[2] = item3;
}

bool CSubLangSelectNotifier::changeNotify(const std::string&, void *)
{
	// only ac3
	toDisable[0]->setActive(g_settings.auto_subs);
	toDisable[1]->setActive(g_settings.auto_subs);
	toDisable[2]->setActive(g_settings.auto_subs);
	
	return true;
}





