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
#include <gui/cec_setup.h>

#include <system/debug.h>
#include <system/helpers.h>


// main settings
int CMainSettingsMenu::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMainSettingsMenu::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CTarget::RETURN_REPAINT;
	
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
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Settings"));

	//
	CWidget* widget = NULL;
	ClistBox* mainSettings = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("settings");
	
	if (widget)
	{
		mainSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "settings";
		
		//
		mainSettings = new ClistBox(&box);
		
		mainSettings->setWidgetMode(ClistBox::MODE_MENU);
		mainSettings->setWidgetType(ClistBox::TYPE_CLASSIC);
		mainSettings->enableShrinkMenu();
		mainSettings->paintMainFrame(true);
		
		//
		mainSettings->enablePaintHead();
		mainSettings->setTitle(_("Settings"), NEUTRINO_ICON_SETTINGS);
		mainSettings->enablePaintDate();
		//
		mainSettings->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		mainSettings->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) mainSettings->enablePaintItemInfo(60);
		
		//
		widget->addCCItem(mainSettings);

		// video settings
		mainSettings->addItem(new CMenuForwarder(_("Video settings"), true, NULL, new CVideoSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_VIDEOSETTINGS));

		// audio settings
		mainSettings->addItem(new CMenuForwarder(_("Audio settings"), true, NULL, new CAudioSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_AUDIOSETTINGS));

		// parentallock
		mainSettings->addItem(new CMenuForwarder(_("Parentallock settings"), true, NULL, new CParentalLockSettings(), NULL, CRCInput::RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_PARENTALLOCKSETTINGS));

		// network settings
		mainSettings->addItem(new CMenuForwarder(_("Network settings"), true, NULL,  CNetworkSettings::getInstance(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_NETWORKSETTINGS));

		// recording settings
		mainSettings->addItem(new CMenuForwarder(_("Recording settings"), true, NULL, new CRecordingSettings(), NULL, CRCInput::RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_RECORDINGSETTINGS));

		// movieplayer settings
		mainSettings->addItem(new CMenuForwarder(_("Movieplayer settings"), true, NULL, new CMoviePlayerSettings(), NULL, CRCInput::RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_MOVIEPLAYERSETTINGS));

		// vfd/lcd settings
#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD) || defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
		mainSettings->addItem(new CMenuForwarder(_("Display settings"), true, NULL, new CLCDSettings(), NULL, CRCInput::RC_nokey, NULL,  NEUTRINO_ICON_MENUITEM_LCDSETTINGS));
#endif	

		// remote control settings
		mainSettings->addItem(new CMenuForwarder(_("Remote Control settings"), true, NULL, new CRemoteControlSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_REMOTECONTROLSETTINGS));

		// audioplayer settings
		mainSettings->addItem(new CMenuForwarder(_("Audioplayer settings"), true, NULL, new CAudioPlayerSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_AUDIOPLAYERSETTINGS));
			
		// pictureviewer settings
		mainSettings->addItem(new CMenuForwarder(_("Pictureviewer settings"), true, NULL, new CPictureViewerSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_PICTUREVIEWERSETTINGS));

		//HDD settings
		mainSettings->addItem(new CMenuForwarder(_("HDD settings"), true, NULL, new CHDDMenuHandler(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_HDDSETTINGS));
			
		//channellist settings
		mainSettings->addItem(new CMenuForwarder(_("Channellist settings"), true, NULL, new CChannelListSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_CHANNELLISTSETTINGS));

		// epg settings
		mainSettings->addItem(new CMenuForwarder(_("EPG settings"), true, NULL, new CEPGSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_EPGSETTINGS));

		// filebrowser settings
		mainSettings->addItem(new CMenuForwarder(_("Filebrowser settings"), true, NULL, new CFileBrowserSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FILEBROWSERSETTINGS));
			
		// zapit setup (start channel)
		mainSettings->addItem(new CMenuForwarder(_("Start Channel settings"), true, NULL, new CZapitSetup(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_STARTCHANNELSETTINGS));
		
		// psi setup
		//FIXME:	
		//CPSISetup * chPSISetup = new CPSISetup(_(PSI settings), &g_settings.contrast, &g_settings.saturation, &g_settings.brightness, &g_settings.tint);
		//mainSettings->addItem( new CMenuForwarder(_("PSI settings"), true, NULL, chPSISetup, NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_PSISETTINGS));
		
		// cec setup 
#if !defined (__sh__)
		mainSettings->addItem(new CMenuForwarder(_("CEC settings"), true, NULL, new CCECSetup(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SETTINGS));
#endif
	
		//miscSettings general
		mainSettings->addItem(new CMenuForwarder(_("Misc settings"), true, NULL, new CGeneralSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_MISCSETTINGS));
	
		//
		mainSettings->integratePlugins(CPlugins::I_TYPE_SETTING);
	}
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

