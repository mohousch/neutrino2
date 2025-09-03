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
int CPowerMenu::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPowerMenu::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CWidgetTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	ret = showMenu();
	
	return ret;
}

// showmenu
int CPowerMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CPowerMenu::showMenu:\n");
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Power Menu"));

	//
	CWidget* widget = NULL;
	ClistBox* powerMenu = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("powermenu");
	
	if (widget)
	{
		powerMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "powermenu";
		
		//
		powerMenu = new ClistBox(&box);
		
		powerMenu->setWidgetMode(ClistBox::MODE_MENU);
		powerMenu->setWidgetType(ClistBox::TYPE_CLASSIC);
		powerMenu->enableShrinkMenu();
		powerMenu->paintMainFrame(true);
		
		//
		powerMenu->enablePaintHead();
		powerMenu->setTitle(_("Power Menu"), NEUTRINO_ICON_BUTTON_POWER);
		powerMenu->enablePaintDate();
		
		//
		powerMenu->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		powerMenu->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) powerMenu->enablePaintItemInfo(60);
		
		//
		widget->addCCItem(powerMenu);
	
		// sleep timer
		powerMenu->addItem(new CMenuForwarder(_("Sleeptimer"), true, NULL, new CSleepTimerWidget(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SLEEPTIMER));

		// restart neutrino
		powerMenu->addItem(new CMenuForwarder(_("GUI restart"), true, NULL, CNeutrinoApp::getInstance(), "restart", CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_RESTART));

		// standby
		powerMenu->addItem(new CMenuForwarder(_("Standby"), true, NULL, CNeutrinoApp::getInstance(), "standby", CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_STANDBY));

		// reboot
		powerMenu->addItem(new CMenuForwarder(_("Reboot"), true, NULL, CNeutrinoApp::getInstance(), "reboot", CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_REBOOT));

		// shutdown
		powerMenu->addItem(new CMenuForwarder(_("Shutdown"), true, NULL, CNeutrinoApp::getInstance(), "shutdown", CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SHUTDOWN));

		//
		powerMenu->integratePlugins(CPlugins::I_TYPE_POWER);
	}
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

