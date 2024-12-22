/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrino_menu.cpp 23.09.2023 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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


// mainmenu
void CNeutrinoApp::mainMenu(void)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::mainMenu:\n");
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Main Menu"));
	
	CWidget* widget = NULL;
	ClistBox* nMenu = NULL;
	CMenuItem* item = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("mainmenu");
	
	if (widget)
	{
		nMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "mainmenu";
		
		//
		nMenu = new ClistBox(&box);
		
		nMenu->setWidgetMode(ClistBox::MODE_MENU);
		nMenu->setWidgetType(ClistBox::TYPE_CLASSIC);
		nMenu->enableShrinkMenu();
		nMenu->paintMainFrame(true);
		
		// head
		nMenu->enablePaintHead();
		nMenu->setTitle(_("Main Menu"), NEUTRINO_ICON_MAINMENU);
		nMenu->enablePaintDate();
		
		// foot
		nMenu->enablePaintFoot();
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		nMenu->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info)
		{
			if (nMenu) nMenu->enablePaintItemInfo();
		}
		
		//
		widget->addCCItem(nMenu);
		
		char *hint_tv = _("Here you can switch into TV mode");
		char *hint_radio = _("Here you can switch into Radio mode");
			  
		// tv / radio
		item = new CMenuForwarder(_("TV / Radio"), true, NULL, this, "tvradioswitch");
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_TV);
		item->setHint(_("Here you can switch between TV / Radio"));
		item->setIconName(NEUTRINO_ICON_BUTTON_RED);
		item->setDirectKey(CRCInput::RC_red);
		item->setState(g_settings.personalize_tvradio);
		if (nMenu) nMenu->addItem(item);

		// epg / sleeptimer
		item = new CMenuForwarder(_("EPG / Timer"), true, NULL, new CEPGMenuHandler(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_TIMERLIST);
		item->setHint(_("Here you can set Timers and show EPG"));
		item->setIconName(NEUTRINO_ICON_BUTTON_GREEN);
		item->setDirectKey(CRCInput::RC_green);
		item->setState(g_settings.personalize_epgtimer);
		if (nMenu) nMenu->addItem(item);
			
#if defined (ENABLE_SCART)
		// scart
		item = new CMenuForwarder(_("Scart Mode"), true, NULL, this, "scart");
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_SCART);
		item->setHint(_("Here you can switch to scart modus"));
		item->setIconName(NEUTRINO_ICON_BUTTON_YELLOW);
		item->setState(g_settings.personalize_scart);
		if (nMenu) nMenu->addItem(item);
#endif

		// features
		item = new CMenuForwarder(_("Features"), true, NULL, this, "features");
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_FEATURES);
		item->setHint(_("Here you can choose plugins"));
		item->setIconName(NEUTRINO_ICON_BUTTON_BLUE);
		item->setDirectKey(CRCInput::RC_blue);
		item->setState(g_settings.personalize_features);
		if (nMenu) nMenu->addItem(item);
		
		// service
		item = new CMenuForwarder(_("Service"), true, NULL, new CServiceMenu(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_SERVICE);
		item->setHint(_("Here you can set channel scan and more"));
		item->setIconName(NEUTRINO_ICON_BUTTON_1);
		item->setDirectKey(CRCInput::RC_1);
		item->setState(g_settings.personalize_service);
		if (nMenu) nMenu->addItem(item);
			
		// main setting
		item = new CMenuForwarder(_("Settings"), true, NULL, new CMainSettingsMenu(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_SETTINGS);
		item->setHint(_("Here you can setup your box"));
		item->setIconName(NEUTRINO_ICON_BUTTON_2);
		item->setDirectKey(CRCInput::RC_2);
		if (nMenu) nMenu->addItem(item);
		
		// osd
		item = new CMenuForwarder(_("OSD"), true, NULL, new COSDSettings(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_OSDSETTINGS);
		item->setHint(_("Here you can setup OSD"));
		item->setIconName(NEUTRINO_ICON_BUTTON_3);
		item->setDirectKey(CRCInput::RC_3);
		if (nMenu) nMenu->addItem(item);
		
		//box info
		item = new CMenuForwarder(_("Information"), true, NULL, new CInfoMenu(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_BOXINFO);
		item->setHint(_("Here you can get device Info"));
		item->setIconName(NEUTRINO_ICON_BUTTON_INFO_SMALL);
		item->setDirectKey(CRCInput::RC_info);
		item->setState(g_settings.personalize_information);
		if (nMenu) nMenu->addItem(item);

		// power menu
		item = new CMenuForwarder(_("Power Menu"), true, NULL, new CPowerMenu(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_POWERMENU);
		item->setHint(_("Here you can power off or standby your device"));
		item->setIconName(NEUTRINO_ICON_BUTTON_POWER);
		item->setDirectKey(CRCInput::RC_standby);
		item->setState(g_settings.personalize_powermenu);
		if (nMenu) nMenu->addItem(item);
		
		// mediaplayer
		item = new CMenuForwarder(_("Media Player"), true, NULL, new CMediaPlayerMenu(), NULL);
		item->setHintIcon(NEUTRINO_ICON_MENUITEM_MEDIAPLAYER);
		item->setHint(_("Here you can play / show  music / movies / pics"));
		item->setIconName(NEUTRINO_ICON_BUTTON_PLAY_SMALL);
		item->setDirectKey(CRCInput::RC_video);
		item->setState(g_settings.personalize_mediaplayer);
		if (nMenu) nMenu->addItem(item);
		
		//
		if (nMenu) nMenu->integratePlugins(CPlugins::I_TYPE_MAIN);
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

//// User menu
// This is just a quick helper for the usermenu only. I already made it a class for future use.
#define BUTTONMAX 4

const neutrino_msg_t key_helper_msg_def[BUTTONMAX] = {
	CRCInput::RC_red,
	CRCInput::RC_green,
	CRCInput::RC_yellow,
	CRCInput::RC_blue
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
                bool get(neutrino_msg_t* msg, const char** icon, neutrino_msg_t prefered_key = CRCInput::RC_nokey)
                {
                        bool result = false;
                        int button = -1;
                        if(prefered_key == CRCInput::RC_red)
                                button = 0;
                        if(prefered_key == CRCInput::RC_green)
                                button = 1;
                        if(prefered_key == CRCInput::RC_yellow)
                                button = 2;
                        if(prefered_key == CRCInput::RC_blue)
                                button = 3;

                        *msg = CRCInput::RC_nokey;
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
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::showUserMenu: button:%d\n", button);
	
	if(button < 0 || button >= SNeutrinoSettings::BUTTON_MAX)
		return false;

	CMenuItem * menu_item = NULL;
	CKeyHelper keyhelper;
	neutrino_msg_t key = CRCInput::RC_nokey;
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
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, txt.c_str());

	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("features");
	
	if (widget)
	{
		menu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "features";
		
		//
		menu = new ClistBox(&box);
		menu->setWidgetMode(ClistBox::MODE_MENU);
		menu->setWidgetType(ClistBox::TYPE_CLASSIC);
		menu->enableShrinkMenu();
		menu->paintMainFrame(true);
		
		// head
		menu->enablePaintHead();
		menu->setTitle(txt.c_str(), NEUTRINO_ICON_FEATURES);
		menu->enablePaintDate();
		
		// foot
		menu->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
	
		menu->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info)
		{
			if (menu) menu->enablePaintItemInfo();
		}
		
		//
		widget->addCCItem(menu);
	}	

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
				keyhelper.get(&key, &icon, CRCInput::RC_yellow);
				menu_item = new CMenuForwarder(_("Timerlist"), true, NULL, new CTimerList, "-1", key, icon, NEUTRINO_ICON_MENUITEM_TIMERLIST);
				menu_item->setHint(_("Here you can set timers"));
				if (menu) menu->addItem(menu_item, false);
				break;

			// rclock
			case SNeutrinoSettings::ITEM_REMOTE:
				keyhelper.get(&key, &icon);
				menu_item = new CMenuForwarder(_("Remote Control lock"), true, NULL, this->rcLock, "-1", key, icon, NEUTRINO_ICON_MENUITEM_PARENTALLOCKSETTINGS);
				if (menu) menu->addItem(menu_item, false);
				break;
				
			// vtxt	
			case SNeutrinoSettings::ITEM_VTXT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Teletext"), true, NULL, new CTuxtxtChangeExec, "-1", key, icon, NEUTRINO_ICON_MENUITEM_VTXT);
					if (menu) menu->addItem(menu_item, false);
				}
				break;	
				
			// plugins
			case SNeutrinoSettings::ITEM_PLUGIN:
				{
					keyhelper.get(&key, &icon, CRCInput::RC_blue);
					menu_item = new CMenuForwarder(_("Plugins"), true, NULL, new CPluginList(), "-1", key, icon, NEUTRINO_ICON_MENUITEM_PLUGIN);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			// delete zapit
			case SNeutrinoSettings::ITEM_DELETE_ZAPIT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Delete channels"), true, NULL, this, "delete_zapit", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			// delete webtv
			case SNeutrinoSettings::ITEM_DELETE_WEBTV:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Delete WebTV channels"), true, NULL, this, "delete_webtv", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_FREEMEMORY:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Free Memory"), true, NULL, this, "free_memory", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_ZAPIT:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Reload channel lists"), true, NULL, this, "reloadchannels", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_XMLTV:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Reload XMLTV EPG"), true, NULL, this, "reloadxmltvepg", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_LOAD_EPG:
				{
					keyhelper.get(&key, &icon);
					menu_item = new CMenuForwarder(_("Reload EPG"), true, NULL, this, "reloadepg", key, icon, NEUTRINO_ICON_MENUITEM_SERVICE);
					if (menu) menu->addItem(menu_item, false);
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
		if (menu) menu->integratePlugins(CPlugins::I_TYPE_USER, key);
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
		if (menu) 
		{
			menu->setSelected(selected[button]);
			selected[button] = menu->getSelected();
		}
	}
	
	//
	widget->addKey(CRCInput::RC_blue, this, "plugins");
	widget->setTimeOut(g_settings.timing_menu);	
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

	return 0;
}

//// nvod
int CNVODChangeExec::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CNVODChangeExec exec: %s\n", actionKey.c_str());
	
	unsigned int sel = atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);

	if(parent)
		parent->hide();
	
	g_InfoViewer->showSubchan();

	return RETURN_EXIT;
}

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

void CNeutrinoApp::selectNVOD()
{
	dprintf(DEBUG_NORMAL, "selectNVOD\n");
	
        if (!(g_RemoteControl->subChannels.empty()))
        {
                //
                CWidget* widget = NULL;
                ClistBox* NVODSelector = NULL;
                
                widget = CNeutrinoApp::getInstance()->getWidget("nvodselect");
                
                if (widget)
                {
			NVODSelector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
			widget->name = "nvodselect";

			//                	
			NVODSelector = new ClistBox(&box);
			NVODSelector->setWidgetMode(ClistBox::MODE_SETUP);
			NVODSelector->enableShrinkMenu();
			NVODSelector->paintMainFrame(true);
							
			NVODSelector->enablePaintHead();
			NVODSelector->setTitle(g_RemoteControl->are_subchannels ? _("Select Subservice") : _("Select starting-time"), NEUTRINO_ICON_VIDEO);

			NVODSelector->enablePaintFoot();
								
			const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
								
			NVODSelector->setFootButtons(&btn);
			
			//
			widget->addCCItem(NVODSelector);
		}
		
		CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_RemoteControl->are_subchannels ? _("Select Subservice") : _("Select starting-time"));
		
		//
		int count = 0;
        	char nvod_id[5];

        	for( CSubServiceListSorted::iterator e = g_RemoteControl->subChannels.begin(); e != g_RemoteControl->subChannels.end(); ++e)
        	{
			sprintf(nvod_id, "%d", count);

			if( !g_RemoteControl->are_subchannels ) 
			{
				char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
				char nvod_s[255];
				struct  tm *tmZeit;

				tmZeit = localtime(&e->starttime);
				sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t endtime = e->starttime + e->duration;
				tmZeit = localtime(&endtime);
				sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t jetzt = time(NULL);
				if(e->starttime > jetzt) 
				{
					int mins = (e->starttime - jetzt)/ 60;
					sprintf(nvod_time_x, _("(starting in %d min)"), mins);
				}
				else if( (e->starttime <= jetzt) && (jetzt < endtime) ) 
				{
					int proz = (jetzt - e->starttime)*100/ e->duration;
					sprintf(nvod_time_x, _("(%d%% over)"), proz);
				}
				else
					nvod_time_x[0] = 0;

				sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
				NVODSelector->addItem(new CMenuForwarder(nvod_s, true, NULL, new CNVODChangeExec(), nvod_id), (count == g_RemoteControl->selected_subchannel));
			} 
			else 
			{
				if (count == 0)
					NVODSelector->addItem(new CMenuForwarder(::Latin1_to_UTF8(e->subservice_name.c_str()).c_str(), true, NULL, new CNVODChangeExec(), nvod_id, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
				else
					NVODSelector->addItem(new CMenuForwarder(::Latin1_to_UTF8(e->subservice_name.c_str()).c_str(), true, NULL, new CNVODChangeExec(), nvod_id, CRCInput::convertDigitToKey(count)), (count == g_RemoteControl->selected_subchannel));
			}

			count++;
		}

		if( g_RemoteControl->are_subchannels ) 
		{
			NVODSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
			CMenuOptionChooser* oj = new CMenuOptionChooser(_("Direct-Mode"), &g_RemoteControl->director_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
			NVODSelector->addItem(oj);
		}

                widget->setTimeOut(g_settings.timing_menu);
		widget->exec(NULL, "");
                        
		if (widget)
		{
                        delete widget;
                        widget = NULL;
		}
        }
}

