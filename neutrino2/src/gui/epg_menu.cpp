/*
	Neutrino-GUI  -   DBoxII-Project

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

#include <gui/epg_menu.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/epgplus.h>
#include <gui/streaminfo.h>

#include <system/debug.h>


int CEPGMenuHandler::exec(CMenuTarget* parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CEPGMenuHandler::exec:\n");
	
	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	doMenu();
	
	return res;
}

int CEPGMenuHandler::doMenu()
{
	int res = RETURN_REPAINT;
	
	//
	CWidget* widget = NULL;
	ClistBox* redMenu = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("epgtimer");
	
	if (widget)
	{
		redMenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget();
		redMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		redMenu->setWidgetMode(MODE_MENU);
		redMenu->setWidgetType(TYPE_CLASSIC);
		redMenu->enableShrinkMenu();
		
		redMenu->enablePaintHead();
		redMenu->setTitle(_("EPG / Timer"), NEUTRINO_ICON_BUTTON_EPG);
		redMenu->enablePaintDate();
		
		redMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		redMenu->setFootButtons(&btn);
		
		//
		widget->setPosition(redMenu->getWindowsPos().iX, redMenu->getWindowsPos().iY, redMenu->getWindowsPos().iWidth, redMenu->getWindowsPos().iHeight);
		widget->name = "epgtimer";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(redMenu);
	}
		
	// eventlist
	redMenu->addItem(new CMenuForwarder(_("Eventlist current programm"), true, NULL, new CEventListHandler(), "", RC_red, NULL, NEUTRINO_ICON_MENUITEM_SLEEPTIMER));

	// epg view
	redMenu->addItem(new CMenuForwarder(_("Details current program"), true, NULL, new CEPGDataHandler(), "", RC_green, NULL, NEUTRINO_ICON_MENUITEM_RESTART));
				
	// epgplus
	redMenu->addItem(new CMenuForwarder(_("Eventlist overview"), true, NULL, new CEPGplusHandler(), "", RC_yellow, NULL, NEUTRINO_ICON_MENUITEM_STANDBY));
		
	// timerlist
	redMenu->addItem(new CMenuForwarder(_("Timerlist"), true, NULL, new CTimerList(), "", RC_blue, NULL, NEUTRINO_ICON_MENUITEM_TIMERLIST));
		
	//
	widget->setTimeOut(g_settings.timing_menu);				
	res = widget->exec(NULL, "");
	
	delete redMenu;
	redMenu = NULL;
	delete widget;
        widget = NULL;
	
	return res;
}

