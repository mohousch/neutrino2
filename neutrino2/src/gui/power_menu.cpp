/*
	Neutrino-GUI  -   DBoxII-Project

	$id: power_menu.cpp 2016.01.29 17:22:30 mohousch $
	
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

#include <gui/power_menu.h>
#include <gui/sleeptimer.h>

#include <system/debug.h>


//
int CPowerMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPowerMenu::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	ret = showMenu();
	
	return ret;
}

// showmenu
int CPowerMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CPowerMenu::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;

	//
	CWidget* widget = NULL;
	ClistBox* powerMenu = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("powermenu");
	
	if (widget)
	{
		powerMenu = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "powermenu";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		powerMenu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);
		
		powerMenu->setWidgetMode(ClistBox::MODE_MENU);
		powerMenu->setWidgetType(CMenuItem::TYPE_CLASSIC);
		powerMenu->enableShrinkMenu();
		
		//
		powerMenu->enablePaintHead();
		powerMenu->setTitle(_("Power Menu"), NEUTRINO_ICON_BUTTON_POWER);
		powerMenu->enablePaintDate();
		
		//
		powerMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		powerMenu->setFootButtons(&btn); 
		
		//
		widget->addWidgetItem(powerMenu);
	}
	
	// sleep timer
	powerMenu->addItem(new CMenuForwarder(_("Sleeptimer"), true, NULL, new CSleepTimerWidget(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SLEEPTIMER));

	// restart neutrino
	powerMenu->addItem(new CMenuForwarder(_("GUI restart"), true, NULL, CNeutrinoApp::getInstance(), "restart", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_RESTART));

	// standby
	powerMenu->addItem(new CMenuForwarder(_("Standby"), true, NULL, CNeutrinoApp::getInstance(), "standby", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_STANDBY));

	// reboot
	powerMenu->addItem(new CMenuForwarder(_("Reboot"), true, NULL, CNeutrinoApp::getInstance(), "reboot", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_REBOOT));

	// shutdown
	powerMenu->addItem(new CMenuForwarder(_("Shutdown"), true, NULL, CNeutrinoApp::getInstance(), "shutdown", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SHUTDOWN));

	//
	powerMenu->integratePlugins(CPlugins::I_TYPE_POWER);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	delete powerMenu;
	powerMenu = NULL;
	delete widget;
	widget = NULL;
	
	return res;
}

