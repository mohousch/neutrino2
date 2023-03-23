/*
	Neutrino-GUI  -   DBoxII-Project

	$id: main_setup.cpp 2015.12.22 21:31:30 mohousch $
	
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

#include <gui/main_setup.h>

#include <gui/audio_setup.h>
#include <gui/video_setup.h>
#include <gui/parentallock_setup.h>
#include <gui/network_setup.h>
#include <gui/movieplayer_setup.h>
#include <gui/osd_setup.h>
#include <gui/audioplayer_setup.h>
#include <gui/pictureviewer_setup.h>
#include <gui/lcd_setup.h>
#include <gui/rc_setup.h>
#include <gui/recording_setup.h>
#include <gui/misc_setup.h>
#include <gui/hdd_menu.h>
#include <gui/zapit_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


// main settings
int CMainSettingsMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMainSettingsMenu::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

// showmenu
void CMainSettingsMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CMainSettingsMenu::showMenu:\n");

	//
	CWidget* widget = NULL;
	ClistBox* mainSettings = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("settings");
	
	if (widget == NULL)
	{
		mainSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
		mainSettings->setWidgetMode(MODE_MENU);
		mainSettings->setWidgetType(TYPE_CLASSIC);
		mainSettings->enableShrinkMenu();
		
		//
		mainSettings->enablePaintHead();
		mainSettings->setTitle(_("Settings"), NEUTRINO_ICON_SETTINGS);
		mainSettings->enablePaintDate();
		
		//
		mainSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mainSettings->setFootButtons(&btn); 

		// video settings
		mainSettings->addItem(new ClistBoxItem(_("Video settings"), true, NULL, new CVideoSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_VIDEOSETTINGS));

		// audio settings
		mainSettings->addItem(new ClistBoxItem(_("Audio settings"), true, NULL, new CAudioSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_AUDIOSETTINGS));

		// parentallock
		mainSettings->addItem(new ClistBoxItem(_("Parentallock settings"), true, NULL, new CParentalLockSettings(), NULL, RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_PARENTALLOCKSETTINGS));

		// network settings
		mainSettings->addItem(new ClistBoxItem(_("Network settings"), true, NULL,  CNetworkSettings::getInstance(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_NETWORKSETTINGS));

		// recording settings
		mainSettings->addItem(new ClistBoxItem(_("Recording settings"), true, NULL, new CRecordingSettings(), NULL, RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_RECORDINGSETTINGS));

		// movieplayer settings
		mainSettings->addItem(new ClistBoxItem(_("Movieplayer settings"), true, NULL, new CMoviePlayerSettings(), NULL, RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_MOVIEPLAYERSETTINGS));

		// vfd/lcd settings
		mainSettings->addItem(new ClistBoxItem(_("Display settings"), true, NULL, new CLCDSettings(), NULL, RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_LCDSETTINGS));	

		// remote control settings
		mainSettings->addItem(new ClistBoxItem(_("Remote Control settings"), true, NULL, new CRemoteControlSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_REMOTECONTROLSETTINGS));

		// audioplayer settings
		mainSettings->addItem(new ClistBoxItem(_("Audioplayer settings"), true, NULL, new CAudioPlayerSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_AUDIOPLAYERSETTINGS));
		
		// pictureviewer settings
		mainSettings->addItem(new ClistBoxItem(_("Pictureviewer settings"), true, NULL, new CPictureViewerSettings(), NULL, RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, NEUTRINO_ICON_MENUITEM_PICTUREVIEWERSETTINGS));

		//HDD settings
		mainSettings->addItem(new ClistBoxItem(_("HDD settings"), true, NULL, new CHDDMenuHandler(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_HDDSETTINGS));
		
		//miscSettings general
		mainSettings->addItem(new ClistBoxItem(_("Misc settings"), true, NULL, new CGeneralSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_GENERALSETTINGS));
		
		//channellist settings
		mainSettings->addItem(new ClistBoxItem(_("Channellist settings"), true, NULL, new CChannelListSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_CHANNELLISTSETTINGS));

		// epg settings
		mainSettings->addItem(new ClistBoxItem(_("EPG settings"), true, NULL, new CEPGSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_EPGSETTINGS));

		// filebrowser settings
		mainSettings->addItem(new ClistBoxItem(_("Filebrowser settings"), true, NULL, new CFileBrowserSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FILEBROWSERSETTINGS));
		
		// zapit setup (start channel)
		mainSettings->addItem(new ClistBoxItem(_("Last Channel settings"), true, NULL, new CZapitSetup(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_STARTCHANNELSETTINGS));
		
		// psi setup
		//FIXME:	
		//CPSISetup * chPSISetup = new CPSISetup(_(PSI settings), &g_settings.contrast, &g_settings.saturation, &g_settings.brightness, &g_settings.tint);
		//mainSettings->addItem( new ClistBoxItem(_("PSI settings"), true, NULL, chPSISetup, NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_PSISETTINGS));
	
		//
		widget = new CWidget(mainSettings->getWindowsPos().iX, mainSettings->getWindowsPos().iY, mainSettings->getWindowsPos().iWidth, mainSettings->getWindowsPos().iHeight);
		widget->name = "settings";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(mainSettings);
	}
	
	//
	widget->exec(NULL, "");
}

