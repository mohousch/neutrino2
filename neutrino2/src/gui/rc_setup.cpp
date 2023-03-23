/*
	Neutrino-GUI  -   DBoxII-Project

	$id: rc_setup.cpp 2016.01.02 21:33:30 mohousch $
	
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

#include <gui/widget/hintbox.h>
#include <gui/widget/keychooser.h>
#include <gui/widget/stringinput.h>

#include <gui/rc_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


// remote control settings
enum keynames {
	// zap
	KEY_TV_RADIO_MODE,
	KEY_PAGE_UP,
	KEY_PAGE_DOWN,
	KEY_LIST_START,
	KEY_LIST_END,
	KEY_CANCEL_ACTION,
	KEY_BOUQUET_UP,
	KEY_BOUQUET_DOWN,
	KEY_CHANNEL_UP,
	KEY_CHANNEL_DOWN,
	KEY_SUBCHANNEL_UP,
	KEY_SUBCHANNEL_DOWN,
	KEY_ZAP_HISTORY,
	KEY_LASTCHANNEL,
	KEY_SAME_TP,
	
	// media
	KEY_EXTRAS_MOVIEPLAYER,
	KEY_EXTRAS_AUDIOPLAYER,
	KEY_EXTRAS_PICTUREVIEWER,
	KEY_EXTRAS_TIMERLIST,
	KEY_EXTRAS_INETRADIO,
	KEY_EXTRAS_MOVIEBROWSER,
	KEY_EXTRAS_FILEBROWSER,
	KEY_EXTRAS_SCREENSHOT
};

#define KEYBINDS_COUNT 23
const char* const  keydescription[KEYBINDS_COUNT] =
{
	// zap
	_("TV/Radio-mode"),
	_("Page Up"),
	_("Page Down"),
	_("List Start"),
	_("List End"),
	_("Cancel"),
	_("Next bouquet"),
	_("Bouquet back"),
	_("Channel up"),
	_("Channel down"),
	_("Subchannel up"),
	_("Subchannel down"),
	_("Zapping History Bouquet"),
	_("Last Channel"),
	_("Same TP"),

	// media
	_("Movieplayer"),
	_("Audioplayer"),
	_("Pictureviewer"),
	_("Timerlist"),
	_("Internet Radio"),
	_("Movies Browser"),
	_("Files Browser"),
	_("Screenshot")
};

CRemoteControlSettings::CRemoteControlSettings()
{
}

CRemoteControlSettings::~CRemoteControlSettings()
{
}

int CRemoteControlSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CRemoteControlSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void CRemoteControlSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CRemoteControlSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* remoteControlSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("rcsetup");
	
	if (widget)
	{
		remoteControlSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		remoteControlSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		remoteControlSettings->setWidgetMode(MODE_SETUP);
		remoteControlSettings->enableShrinkMenu();
		
		remoteControlSettings->enablePaintHead();
		remoteControlSettings->setTitle(_("Keybinding settings"), NEUTRINO_ICON_KEYBINDING);

		remoteControlSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		remoteControlSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "rcsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(remoteControlSettings);
	}
	
	remoteControlSettings->clearItems();
	
	// intros
	remoteControlSettings->addItem(new ClistBoxItem(_("back")));
	remoteControlSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	remoteControlSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));

	keySetupNotifier = new CKeySetupNotifier;
	
	// repeat generic blocker
	CStringInput * remoteControlSettings_repeat_genericblocker = new CStringInput(_("Generic delay"), g_settings.repeat_genericblocker, 3, _("Shortest time (in ms) to recognize 2 keystrokes"), _("Enter 0 to switch of the blocker (red is space)"), "0123456789 ", keySetupNotifier);
	
	// repeat blocker
	CStringInput * remoteControlSettings_repeatBlocker = new CStringInput(_("Repeat delay"), g_settings.repeat_blocker, 3, _("Shortest time (in ms) to recognize 2 keystrokes"), _("Enter 0 to switch of the blocker (red is space)"), "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify("", NULL);

	remoteControlSettings->addItem(new CMenuSeparator(LINE | STRING, _("Key Repeat-blocker")));
	
	// repeat blocker
	remoteControlSettings->addItem(new ClistBoxItem(_("Repeat delay"), true, g_settings.repeat_blocker, remoteControlSettings_repeatBlocker));
	
	// repeat generic blocker
 	remoteControlSettings->addItem(new ClistBoxItem(_("Generic delay"), true, g_settings.repeat_genericblocker, remoteControlSettings_repeat_genericblocker));

	// keybinding menu
	remoteControlSettings->addItem(new CMenuSeparator(LINE | STRING, _("Hot Keys mapping")));
	
	remoteControlSettings->addItem(new ClistBoxItem(_("Hot Keys mapping"), true, NULL, new CKeysBindingSettings()));

        // usermenu 
        remoteControlSettings->addItem(new CMenuSeparator(LINE | STRING, _("User menu")));

	// blue
        remoteControlSettings->addItem(new ClistBoxItem(_("User menu blue"), true, NULL, new CUserMenuMenu(_("User menu blue"), 0)));

#if defined (ENABLE_FUNCTIONKEYS)	
	remoteControlSettings->addItem(new ClistBoxItem(_("User menu F1"), true, NULL, new CUserMenuMenu(_("User menu F1"), 1) ));
        remoteControlSettings->addItem(new ClistBoxItem(_("User menu F2"), true, NULL, new CUserMenuMenu(_("User menu F2"), 2) ));
        remoteControlSettings->addItem(new ClistBoxItem(_("User menu F3"), true, NULL, new CUserMenuMenu(_("User menu F3"), 3) ));
        remoteControlSettings->addItem(new ClistBoxItem(_("User menu F4"), true, NULL, new CUserMenuMenu(_("User menu F4"), 4) ));	
#endif
	
	//
	widget->exec(NULL, "");
}

// keys binding settings
CKeysBindingSettings::CKeysBindingSettings()
{
}

CKeysBindingSettings::~CKeysBindingSettings()
{
}

int CKeysBindingSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CKeysBindingSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "savercconfig")
	{
		CHintBox * hintBox = new CHintBox(_("Information"), _("Saving RC configuration, please wait...")); // UTF-8
		hintBox->paint();
		
		g_RCInput->configfile.setModifiedFlag(true);
		g_RCInput->saveRCConfig(NEUTRINO_RCCONFIG_FILE);
		
		sleep(2);
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;

		return RETURN_REPAINT;	
	}
	
	showMenu();
	
	return ret;
}

void CKeysBindingSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CKeysBindingSettings::showMenu:\n");

	int * keyvalue_p[KEYBINDS_COUNT] =
	{
		// zap
		&g_settings.key_tvradio_mode,
		&g_settings.key_channelList_pageup,
		&g_settings.key_channelList_pagedown,
		&g_settings.key_list_start,
		&g_settings.key_list_end,
		&g_settings.key_channelList_cancel,
		&g_settings.key_bouquet_up,
		&g_settings.key_bouquet_down,
		&g_settings.key_quickzap_up,
		&g_settings.key_quickzap_down,
		&g_settings.key_subchannel_up,
		&g_settings.key_subchannel_down,
		&g_settings.key_zaphistory,
		&g_settings.key_lastchannel,
		&g_settings.key_pip,
		
		// media
		&g_settings.key_movieplayer,
		&g_settings.key_audioplayer,
		&g_settings.key_pictureviewer,
		&g_settings.key_timerlist,
		&g_settings.key_inetradio,
		&g_settings.key_moviebrowser,
		&g_settings.key_filebrowser,
		
		// misc
		&g_settings.key_screenshot
	};
	
	//
	CWidget* widget = NULL;
	ClistBox* bindSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("rcbinding");
	
	if (widget)
	{
		bindSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		bindSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		bindSettings->setWidgetMode(MODE_SETUP);
		bindSettings->enableShrinkMenu();
		
		bindSettings->enablePaintHead();
		bindSettings->setTitle(_("Hot Keys mapping"), NEUTRINO_ICON_KEYBINDING);

		bindSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		bindSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "rcbinding";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(bindSettings);
	}
	
	bindSettings->clearItems();
	
	// intros
	bindSettings->addItem(new ClistBoxItem(_("back")));
	bindSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	bindSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));

	bindSettings->addItem(new CMenuSeparator(LINE | STRING, _("Modechange")));
	
	// tv/radio mode
	bindSettings->addItem(new ClistBoxItem(keydescription[KEY_TV_RADIO_MODE], true, NULL, new CKeyChooser(keyvalue_p[KEY_TV_RADIO_MODE], keydescription[KEY_TV_RADIO_MODE], NEUTRINO_ICON_SETTINGS)));

	// channellist
	bindSettings->addItem(new CMenuSeparator(LINE | STRING, _("Channellist")));

	for (int i = KEY_PAGE_UP; i <= KEY_BOUQUET_DOWN; i++)
		bindSettings->addItem(new ClistBoxItem(keydescription[i], true, NULL, new CKeyChooser(keyvalue_p[i], keydescription[i], NEUTRINO_ICON_SETTINGS)));

	// quick zap
	bindSettings->addItem(new CMenuSeparator(LINE | STRING, _("Quickzap")));

	for (int i = KEY_CHANNEL_UP; i <= KEY_SAME_TP; i++)
		bindSettings->addItem(new ClistBoxItem(keydescription[i], true, NULL, new CKeyChooser(keyvalue_p[i], keydescription[i], NEUTRINO_ICON_SETTINGS)));
	
	// media
	bindSettings->addItem(new CMenuSeparator(LINE | STRING, _("Media")));
	for (int i = KEY_EXTRAS_MOVIEPLAYER; i <= KEY_EXTRAS_FILEBROWSER; i++)
		bindSettings->addItem(new ClistBoxItem(keydescription[i], true, NULL, new CKeyChooser(keyvalue_p[i], keydescription[i], NEUTRINO_ICON_SETTINGS)));

	// misc
	bindSettings->addItem(new CMenuSeparator(LINE | STRING, _("Misc settings")));
	
	// screenshot key
	bindSettings->addItem(new ClistBoxItem(keydescription[KEY_EXTRAS_SCREENSHOT], true, NULL, new CKeyChooser(keyvalue_p[KEY_EXTRAS_SCREENSHOT], keydescription[KEY_EXTRAS_SCREENSHOT], NEUTRINO_ICON_SETTINGS)));
	
	// save rc config
	bindSettings->addItem(new ClistBoxItem(_("Save RC configuration"), true, NULL, this, "savercconfig" ) );
	
	//
	widget->exec(NULL, "");
}

// key setup notifier
bool CKeySetupNotifier::changeNotify(const std::string&, void *)
{
	g_RCInput->setRepeat(atoi(g_settings.repeat_blocker), atoi(g_settings.repeat_genericblocker));

	return false;
}


