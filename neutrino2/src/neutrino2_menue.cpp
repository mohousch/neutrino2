/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrino_menu.cpp 2013/10/12 11:23:30 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <string>

#include <global.h>
#include <neutrino2.h>

#include <daemonc/remotecontrol.h>

#include <gui/widget/textbox.h>
#include <gui/widget/icons.h>
#include <gui/mediaplayer.h>
#include <gui/service_menu.h>
#include <gui/main_setup.h>
#include <gui/timerlist.h>
#include <gui/sleeptimer.h>
#include <gui/power_menu.h>
#include <gui/dboxinfo.h>
#include <gui/osd_setup.h>
#include <gui/epg_menu.h>

#include <gui/audio_select.h>
#include <gui/epgplus.h>
#include <gui/streaminfo.h>
#include <gui/movieplayer.h>
#include <gui/pluginlist.h>

#include <system/debug.h>

#include <driver/encoding.h>


extern CPlugins * g_PluginList;    /* neutrino.cpp */

// mainmenu
void CNeutrinoApp::mainMenu(void)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::mainMenu:\n");
	
	CWidget* widget = NULL;
	ClistBox* nMenu = NULL;
	CMenuItem* item = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("mainmenu");
	
	if (widget == NULL)
	{
		nMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
		nMenu->setWidgetMode(MODE_MENU);
		nMenu->setWidgetType(TYPE_CLASSIC);
		nMenu->enableShrinkMenu();
		
		// head
		nMenu->enablePaintHead();
		nMenu->setTitle(_("Main Menu"), NEUTRINO_ICON_MAINMENU);
		nMenu->enablePaintDate();
		
		// foot
		nMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		nMenu->setFootButtons(&btn);
			  
		// tv modus
		item = new ClistBoxItem(_("TV / Radio"), true, NULL, this, "tvradioswitch", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_TV);
		nMenu->addItem(item, true);

		// epg / sleeptimer
		item = new ClistBoxItem(_("Timer / EPG"), true, NULL, new CEPGMenuHandler(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SLEEPTIMER);
		nMenu->addItem(item);
			
#if defined (ENABLE_SCART)
		// scart
		item = new ClistBoxItem(_("Scart Mode"), true, NULL, this, "scart", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SCART);
		nMenu->addItem(item);
#endif

		// features
		item = new ClistBoxItem(_("Features"), true, NULL, this, "features", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FEATURES);
		nMenu->addItem(item);
		
		// service
		item = new ClistBoxItem(_("System"), true, NULL, new CServiceMenu(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SERVICE);
		nMenu->addItem(item);
			
		// main setting
		nMenu->addItem(new ClistBoxItem(_("Settings"), true, NULL, new CMainSettingsMenu(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SETTINGS));
		
		// osd
		item = new ClistBoxItem(_("OSD"), true, NULL, new COSDSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDSETTINGS);
		nMenu->addItem(item);
		
		//box info
		item = new ClistBoxItem(_("Information"), true, NULL, new CInfoMenu(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_BOXINFO);
		nMenu->addItem(item);

		// power menu
		item = new ClistBoxItem(_("Power Menu"), true, NULL, new CPowerMenu(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_POWERMENU);
		nMenu->addItem(item);
		
		// mediaplayer
		item = new ClistBoxItem(_("Media Player"), true, NULL, new CMediaPlayerMenu(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_MEDIAPLAYER);
		nMenu->addItem(item);
		
		//
		widget = new CWidget(nMenu->getWindowsPos().iX, nMenu->getWindowsPos().iY, nMenu->getWindowsPos().iWidth, nMenu->getWindowsPos().iHeight);
		widget->name = "mainmenu";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(nMenu);
	}

	widget->exec(NULL, "");
}

// User menu
// This is just a quick helper for the usermenu only. I already made it a class for future use.
#define BUTTONMAX 4

const neutrino_msg_t key_helper_msg_def[BUTTONMAX] = {
	RC_red,
	RC_green,
	RC_yellow,
	RC_blue
};

const char * key_helper_icon_def[BUTTONMAX]={
	NEUTRINO_ICON_BUTTON_RED, 
	NEUTRINO_ICON_BUTTON_GREEN, 
	NEUTRINO_ICON_BUTTON_YELLOW, 
	NEUTRINO_ICON_BUTTON_BLUE
};

class CKeyHelper
{
        private:
                int number_key;
                bool color_key_used[BUTTONMAX];
        public:
                CKeyHelper(){reset();};
                void reset(void)
                {
                        number_key = 1;
                        for(int i= 0; i < BUTTONMAX; i++ )
                                color_key_used[i] = false;
                };

                /* Returns the next available button, to be used in menu as 'direct' keys. Appropriate
                 * definitions are returnd in msp and icon
                 * A color button could be requested as prefered button (other buttons are not supported yet).
                 * If the appropriate button is already in used, the next number_key button is returned instead
                 * (first 1-9 and than 0). */
                bool get(neutrino_msg_t* msg, const char** icon, neutrino_msg_t prefered_key = RC_nokey)
                {
                        bool result = false;
                        int button = -1;
                        if(prefered_key == RC_red)
                                button = 0;
                        if(prefered_key == RC_green)
                                button = 1;
                        if(prefered_key == RC_yellow)
                                button = 2;
                        if(prefered_key == RC_blue)
                                button = 3;

                        *msg = RC_nokey;
                        *icon = "";
                        if(button >= 0 && button < BUTTONMAX)
                        {
				// try to get color button
                                if( color_key_used[button] == false)
                                {
                                        color_key_used[button] = true;
                                        *msg = key_helper_msg_def[button];
                                        *icon = key_helper_icon_def[button];
                                        result = true;
                                }
                        }

                        if( result == false && number_key < 10) // no key defined yet, at least try to get a numbered key
                        {
                                // there is still a available number_key
                                *msg = CRCInput::convertDigitToKey(number_key);
                                *icon = "";
                                if(number_key == 9)
                                        number_key = 0;
                                else if(number_key == 0)
                                        number_key = 10;
                                else
                                        number_key++;

                                result = true;
                        }

                        return (result);
                };
};

bool CNeutrinoApp::showUserMenu(int button)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::showUserMenu\n");
	
	if(button < 0 || button >= SNeutrinoSettings::BUTTON_MAX)
		return false;

	CMenuItem * menu_item = NULL;
	CKeyHelper keyhelper;
	neutrino_msg_t key = RC_nokey;
	const char * icon = NULL;

	//
	static int selected[SNeutrinoSettings::BUTTON_MAX] = {
		-1,
#if defined (ENABLE_FUNCTIONKEYS) //FIXME:???
		-1,
		-1,
		-1,
		-1,
#endif		
	};

	std::string txt = g_settings.usermenu_text[button];

	if( button == SNeutrinoSettings::BUTTON_BLUE) 
	{
		if( txt.empty() )		                
			txt = _("Features");
	}

	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("features");
	
	if (widget)
	{
		menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		menu->setWidgetMode(MODE_MENU);
		menu->setWidgetType(TYPE_CLASSIC);
		menu->enableShrinkMenu();
		
		//
		menu->enablePaintHead();
		menu->setTitle(txt.c_str(), NEUTRINO_ICON_FEATURES);
		menu->enablePaintDate();
		
		//
		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		menu->setFootButtons(&btn);
		
		//
		widget = new CWidget(menu->getWindowsPos().iX, menu->getWindowsPos().iY, menu->getWindowsPos().iWidth, menu->getWindowsPos().iHeight);
		widget->name = "features";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(menu);
	}	

	widget->addKey(RC_blue, this, "plugins");
	
	menu->clearItems();	

	// go through any postition number
	for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX ; pos++) 
	{
		// now compare pos with the position of any item. Add this item if position is the same
		switch(g_settings.usermenu[button][pos]) 
		{
			case SNeutrinoSettings::ITEM_NONE:
			{
				continue;
			}

			// timerlist
			case SNeutrinoSettings::ITEM_TIMERLIST:
				keyhelper.get(&key, &icon, RC_yellow);
				menu_item = new ClistBoxItem(_("Timerlist"), true, NULL, new CTimerList, "-1", key, icon, NEUTRINO_ICON_MENUITEM_TIMERLIST);
				menu->addItem(menu_item, false);
				break;

			// rclock
			case SNeutrinoSettings::ITEM_REMOTE:
				keyhelper.get(&key, &icon);
				menu_item = new ClistBoxItem(_("Remote Control lock"), true, NULL, this->rcLock, "-1", key, icon, NEUTRINO_ICON_MENUITEM_PARENTALLOCKSETTINGS);
				menu->addItem(menu_item, false);
				break;
				
			// vtxt	
			case SNeutrinoSettings::ITEM_VTXT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Teletext"), true, NULL, new CTuxtxtChangeExec, "-1", key, icon, NEUTRINO_ICON_MENUITEM_VTXT);
					menu->addItem(menu_item, false);
				}
				break;	
				
			// plugins
			case SNeutrinoSettings::ITEM_PLUGIN:
				{
					keyhelper.get(&key, &icon, RC_blue);
					menu_item = new ClistBoxItem(_("Plugins"), true, NULL, new CPluginList(), "-1", key, icon, NEUTRINO_ICON_MENUITEM_PLUGIN);
					menu->addItem(menu_item, false);
				}
				break;
				
			// delete zapit
			case SNeutrinoSettings::ITEM_DELETE_ZAPIT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Delete channels"), true, NULL, this, "delete_zapit", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;
				
			// delete webtv
			case SNeutrinoSettings::ITEM_DELETE_WEBTV:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Delete WebTV channels"), true, NULL, this, "delete_webtv", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_FREEMEMORY:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Free Memory"), true, NULL, this, "free_memory", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_ZAPIT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Reload channel lists"), true, NULL, this, "reloadchannels", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_XMLTV:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_(""), true, NULL, this, "reloadxmltvepg", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_EPG:
				{
					keyhelper.get(&key, &icon);
					menu_item = new ClistBoxItem(_("Reload EPG"), true, NULL, this, "reloadepg", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					menu->addItem(menu_item, false);
				}
				break;

			default:
				dprintf(DEBUG_NORMAL, "[neutrino] WARNING! menu wrong item!!\n");
				break;
		}
	}

	// integragte blue button plugins
	if( button == SNeutrinoSettings::BUTTON_BLUE) 
	{
		keyhelper.get(&key, &icon);
		menu->integratePlugins(CPlugins::I_TYPE_USER, key);
	}
		
	// start directly plugins if there is no items / start features
	if(menu && menu->getItemsCount() == 0)
	{
		CPluginList * pluginList = new CPluginList();
		pluginList->exec(NULL, "");
		delete pluginList;
		pluginList = NULL;
	}
	else
	{
		menu->setSelected(selected[button]);
		selected[button] = menu->getSelected();
	}
		
	widget->exec(NULL, "");

	return 0;
}

