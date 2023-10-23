/*
	Neutrino-GUI  -   DBoxII-Project
	
	$id: setting_helpers.cpp 23.09.2023 mohousch $

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

#include <system/setting_helpers.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <fcntl.h>
#include <signal.h>
#include "libnet.h"

#include <config.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <gui/plugins.h>
#include <gui/network_setup.h>

#include <daemonc/remotecontrol.h>
#include <xmlinterface.h>

#include "debug.h"

#include <audio_cs.h>
#include <video_cs.h>
#include <dmx_cs.h>

/*zapit includes*/
#include <zapit/frontend_c.h>

/*gui includes*/
#include <gui/scan_setup.h>
#include <gui/filebrowser.h>

/* configfiles */
#include <timerd/timermanager.h>
#include <nhttpd/yconfig.h>


// globals
extern int dvbsub_stop();
extern int dvbsub_close();
extern int dvbsub_start(int pid);
extern int dvbsub_pause();
// tuxtxt
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_main(int pid, int page, int source);
//
extern CFrontend * live_fe;
extern t_channel_id live_channel_id;

// sectionsd config notifier
bool CSectionsdConfigNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_INFO, "CSectionsdConfigNotifier::changeNotify\n");

        CNeutrinoApp::getInstance()->sendSectionsdConfig();
	
        return true;
}

// color setup notifier
bool CColorSetupNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_INFO, "CColorSetupNotifier::changeNotify:\n");
	
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
	
	// head
	frameBuffer->paletteGenFade(COL_MENUHEAD, convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue), convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue), 8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ));

	// menu content
	frameBuffer->paletteGenFade(COL_MENUCONTENT, convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue), convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	// menu content dark
	frameBuffer->paletteGenFade(COL_MENUCONTENTDARK, convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)), convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	// menu content selected
	frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED, convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue), convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

	// menu content inactiv
	frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE, convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue), convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

	// foot
	frameBuffer->paletteGenFade(COL_MENUFOOT, convertSetupColor2RGB(g_settings.menu_Foot_red, g_settings.menu_Foot_green, g_settings.menu_Foot_blue), convertSetupColor2RGB(g_settings.menu_Foot_Text_red, g_settings.menu_Foot_Text_green, g_settings.menu_Foot_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.menu_Foot_alpha) );

	// infobar
	frameBuffer->paletteGenFade(COL_INFOBAR, convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue), convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// infobar shadow
	frameBuffer->paletteGenFade(COL_INFOBAR_SHADOW, convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)), convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// infobar colored events
	frameBuffer->paletteGenFade(COL_INFOBAR_COLORED_EVENTS, convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue), convertSetupColor2RGB(g_settings.infobar_colored_events_red, g_settings.infobar_colored_events_green, g_settings.infobar_colored_events_blue), 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// hint
	frameBuffer->paletteGenFade(COL_MENUHINT, convertSetupColor2RGB(g_settings.menu_Hint_red, g_settings.menu_Hint_green, g_settings.menu_Hint_blue), convertSetupColor2RGB(g_settings.menu_Hint_Text_red, g_settings.menu_Hint_Text_green, g_settings.menu_Hint_Text_blue), 8, convertSetupAlpha2Alpha( g_settings.menu_Hint_alpha ) );
	
	// menu content inactiv text
	frameBuffer->paletteSetColor(COL_MENUCONTENTINACTIVE_TEXT, convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue), convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );
	
	// menu content selected text
	frameBuffer->paletteSetColor(COL_MENUCONTENTSELECTED_TEXT,	convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue), convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );
	
	// menu content text
	frameBuffer->paletteSetColor(COL_MENUCONTENT_TEXT, convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );
	
	// head text
	frameBuffer->paletteSetColor(COL_MENUHEAD_TEXT, convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue), convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ));
	
	// foot text
	frameBuffer->paletteSetColor(COL_MENUFOOT_TEXT, convertSetupColor2RGB(g_settings.menu_Foot_Text_red, g_settings.menu_Foot_Text_green, g_settings.menu_Foot_Text_blue), convertSetupAlpha2Alpha(g_settings.menu_Foot_alpha) );
	
	// infobar text
	frameBuffer->paletteSetColor(COL_INFOBAR_TEXT, convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// infobar colored events text
	frameBuffer->paletteSetColor(COL_INFOBAR_COLORED_EVENTS_TEXT, convertSetupColor2RGB(g_settings.infobar_colored_events_red, g_settings.infobar_colored_events_green, g_settings.infobar_colored_events_blue), convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

	// hint text
	frameBuffer->paletteSetColor(COL_MENUHINT_TEXT, convertSetupColor2RGB(g_settings.menu_Hint_Text_red, g_settings.menu_Hint_Text_green, g_settings.menu_Hint_Text_blue), convertSetupAlpha2Alpha( g_settings.menu_Hint_alpha ) );

	frameBuffer->paletteSet();

	return false;
}

// txt/dvb sub change exec
int CSubtitleChangeExec::exec(CMenuTarget */*parent*/, const std::string & actionKey)
{
	dprintf(DEBUG_INFO, "CSubtitleChangeExec::exec: action %s\n", actionKey.c_str());
	
	if(actionKey == "off") 
	{
		// tuxtxt stop
		tuxtx_stop_subtitle(); //this kill sub thread
		
		// dvbsub stop
		dvbsub_stop();
		//dvbsub_close();
		
		return RETURN_EXIT;
	}
	
	if(!strncmp(actionKey.c_str(), "DVB", 3)) 
	{
		char const * pidptr = strchr(actionKey.c_str(), ':');
		int pid = atoi(pidptr+1);
		
		// tuxtxt stop
		tuxtx_stop_subtitle();
		
		dvbsub_pause();
		dvbsub_start(pid);
	} 
	else 
	{
		char const * ptr = strchr(actionKey.c_str(), ':');
		ptr++;
		int pid = atoi(ptr);
		ptr = strchr(ptr, ':');
		ptr++;
		int page = strtol(ptr, NULL, 16);
		ptr = strchr(ptr, ':');
		ptr++;
		
		dprintf(DEBUG_NORMAL, "CSubtitleChangeExec::exec: TTX, pid %x page %x lang %s\n", pid, page, ptr);
		
		dvbsub_stop();
		
		tuxtx_stop_subtitle();
		
		tuxtx_set_pid(pid, page, ptr);
		
		// start tuxtxt
		tuxtx_main(pid, page, (live_fe)?live_fe->fenumber : 0 ); //FIXME
	}
	
        return RETURN_EXIT;
}

// nvod change exec
int CNVODChangeExec::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CNVODChangeExec exec: %s\n", actionKey.c_str());
	
	unsigned int sel = atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);

	if(parent)
		parent->hide();
	
	g_InfoViewer->showSubchan();

	return RETURN_EXIT;
}

// tuxtxt changer
extern int current_muted;
int CTuxtxtChangeExec::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CTuxtxtChangeExec exec: %s\n", actionKey.c_str());

	if(parent)
		parent->hide();
	
	g_RCInput->clearRCMsg();

	CNeutrinoApp::getInstance()->stopSubtitles();
				
	tuxtx_stop_subtitle();

	tuxtx_main(g_RemoteControl->current_PIDs.PIDs.vtxtpid, 0, live_fe?live_fe->fenumber:0 );

	CFrameBuffer::getInstance()->paintBackground();

	CFrameBuffer::getInstance()->blit();
				
	g_RCInput->clearRCMsg();
				
	CNeutrinoApp::getInstance()->audioMute(current_muted, true);

	CNeutrinoApp::getInstance()->startSubtitles();

	return RETURN_REPAINT;
}

// USERMENU
#define USERMENU_ITEM_OPTION_COUNT SNeutrinoSettings::ITEM_MAX
const keyval USERMENU_ITEM_OPTIONS[USERMENU_ITEM_OPTION_COUNT] =
{
	{ SNeutrinoSettings::ITEM_NONE, " " },  
	{ SNeutrinoSettings::ITEM_TIMERLIST, _("Timerlist") },
	{ SNeutrinoSettings::ITEM_PLUGIN, _("Plugins") },
	{ SNeutrinoSettings::ITEM_VTXT, _("Videotext") },
	{ SNeutrinoSettings::ITEM_REMOTE, _("Lock RC") },
	{ SNeutrinoSettings::ITEM_DELETE_ZAPIT, _("Delete channels") },
	{ SNeutrinoSettings::ITEM_DELETE_WEBTV, _("Delete WebTV channels") },
	{ SNeutrinoSettings::ITEM_FREEMEMORY, _("Free Memory") },
	{ SNeutrinoSettings::ITEM_LOAD_ZAPIT, _("Reload channel lists") },
	{ SNeutrinoSettings::ITEM_LOAD_XMLTV, _("Reload XMLTV EPG") },
	{ SNeutrinoSettings::ITEM_LOAD_EPG, _("Reload EPG") }
};

int CUserMenuMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL , "CUserMenuMenu::exec: %s\n", actionKey.c_str());
	
	int res = CMenuTarget::RETURN_REPAINT;
	
        if(parent)
                parent->hide();
	
	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	CCHeaders *head = NULL;
	CCFooters *foot = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("usermenu");
	
	if (widget)
	{	
		menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
		head = (CCHeaders *)widget->getCCItem(CComponent::CC_HEAD);
		foot = (CCFooters *)widget->getCCItem(CComponent::CC_FOOT);
		
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		widget->enableSaveScreen();
		
		//
		menu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY + 50, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight - 100);
		
		menu->setWidgetMode(ClistBox::MODE_SETUP);
		
		// 	
		head = new CCHeaders(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, 50);
//		head->setLine(true, true);
			
		//	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		

		foot = new CCFooters(widget->getWindowsPos().iX, widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - 50, widget->getWindowsPos().iWidth, 50);
		foot->setButtons(&btn);
//		foot->setLine(true, true);
			
		//
		widget->addCCItem(menu);
		widget->addCCItem(head);
		widget->addCCItem(foot);
	}
	
	if (head)
	{
		head->setTitle(local.c_str());
		head->setIcon(NEUTRINO_ICON_KEYBINDING);
	}
	
	// intros
	menu->addItem(new CMenuForwarder(_("back")));
	menu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	menu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	menu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	//
        CStringInputSMS name(_("User menu"), g_settings.usermenu_text[button].c_str());
        
        menu->addItem(new CMenuForwarder(_("Name"), true, g_settings.usermenu_text[button].c_str(), &name));
        menu->addItem(new CMenuSeparator(CMenuSeparator::LINE));

        char text[10];
        
        for(int item = 0; item < SNeutrinoSettings::ITEM_MAX; item++)
        {
                snprintf(text, 10, "%d:", item);
                text[9] = 0;// terminate for sure
                
                menu->addItem( new CMenuOptionChooser(text, &g_settings.usermenu[button][item], USERMENU_ITEM_OPTIONS, USERMENU_ITEM_OPTION_COUNT, true, NULL, CRCInput::RC_nokey, "", true ));
        }

        res = widget->exec(NULL, "");
        
        if (menu)
        {
        	delete menu;
        	menu = NULL;
        }
        
        if (widget)
        {
        	delete widget;
        	widget = NULL;
        }

        return res;
}


