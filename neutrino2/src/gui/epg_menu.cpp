//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: epg_menu.cpp 21122024 mohousch Exp $
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

#include <gui/epg_menu.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/epgplus.h>
#include <gui/streaminfo.h>

#include <system/debug.h>


int CEPGMenuHandler::exec(CTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CEPGMenuHandler::exec:\n");
	
	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	res = doMenu();
	
	return res;
}

int CEPGMenuHandler::doMenu()
{
	int res = RETURN_REPAINT;
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("EPG / Timer"));
	
	//
	CWidget* widget = NULL;
	ClistBox* redMenu = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("epgtimer");
	
	if (widget)
	{
		redMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "epgtimer";
		
		//
		redMenu = new ClistBox(&box);
		redMenu->setWidgetMode(ClistBox::MODE_MENU);
		redMenu->setWidgetType(ClistBox::TYPE_STANDARD);
		redMenu->enableShrinkMenu();
		redMenu->paintMainFrame(true);
		// head
		redMenu->enablePaintHead();
		redMenu->setTitle(_("EPG / Timer"), NEUTRINO_ICON_BUTTON_EPG);
		redMenu->enablePaintDate();
		// foot
		redMenu->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
		redMenu->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) redMenu->enablePaintItemInfo();
		
		//
		widget->addCCItem(redMenu);
		
		// eventlist
		redMenu->addItem(new CMenuForwarder(_("Eventlist current programm"), true, NULL, new CEventListHandler(), "", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED, NEUTRINO_ICON_MENUITEM_SLEEPTIMER));

		// epg view
		redMenu->addItem(new CMenuForwarder(_("Details current program"), true, NULL, new CEPGDataHandler(), "", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, NEUTRINO_ICON_MENUITEM_RESTART));
					
		// epgplus
		redMenu->addItem(new CMenuForwarder(_("Eventlist overview"), true, NULL, new CEPGplusHandler(), "", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, NEUTRINO_ICON_MENUITEM_STANDBY));
			
		// timerlist
		redMenu->addItem(new CMenuForwarder(_("Timerlist"), true, NULL, new CTimerList(), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, NEUTRINO_ICON_MENUITEM_TIMERLIST));
	}
		
	//
	widget->setTimeOut(g_settings.timing_menu);				
	res = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
        	widget = NULL;
        }
        
        //
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

