/*
	Neutrino-GUI  -   DBoxII-Project

	$id: service_setup.cpp 2015.12.22 17:19:30 mohousch $
	
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

#include <stdio.h> 

#include <global.h>
#include <neutrino2.h>

#include <gui/service_menu.h>

#include <configfile.h>

#include <system/debug.h>
#include <system/settings.h>
#include <system/flashtool.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>

#include <gui/scan_setup.h>
#include <gui/update.h>
#include <gui/cam_menu.h>
#include <gui/imageinfo.h>

#include <gui/bouqueteditor_bouquets.h>


#if defined (ENABLE_CI)
extern CCAMMenuHandler * g_CamHandler;		// defined neutrino.cpp
#endif

extern int FrontendCount;			// defined in zapit.cpp

//
int CServiceMenu::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CServiceMenu::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	ret = showMenu();
	
	return ret;
}

// showmenu
int CServiceMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CServiceMenu::showMenu\n");
	
	int res = CTarget::RETURN_REPAINT;
	
	//
	
	setLCDMode(_("Service"));
	
	//
	CWidget* widget = NULL;
	ClistBox* service = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("service");
	
	if (widget)
	{
		service = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "service";
		
		//
		service = new ClistBox(&box);
		service->setMode(ClistBox::MODE_MENU);
		service->setLayout(ClistBox::LAYOUT_STANDARD);
		service->enableShrinkMenu();
		// head
		service->enablePaintHead();
		service->setTitle(_("Service"), NEUTRINO_ICON_SERVICE);
		service->enablePaintDate();
		// foot
		service->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		if (!g_settings.item_info) service->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) service->enablePaintItemInfo();
		
		//
		widget->addCCItem(service);
	
		// tuner/scan setup
		service->addItem(new CMenuForwarder(_("Scan transponder"), true, NULL, new CTunerSetup(), NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED, NEUTRINO_ICON_MENUITEM_SCANSETTINGS));
			
		// Bouquets Editor
		service->addItem(new CMenuForwarder(_("Bouquet Editor"), true, NULL, new CBEBouquetWidget(), NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, NEUTRINO_ICON_MENUITEM_BOUQUETSEDITOR));
			
		// CI Cam 	
#if defined (ENABLE_CI) 
		service->addItem(new CMenuForwarder(_("CI Cam"), true, NULL, g_CamHandler, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, NEUTRINO_ICON_MENUITEM_CICAM));
#endif
			
		// software update
		service->addItem(new CMenuForwarder(_("Software update"), true, NULL, new CUpdateSettings(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, NEUTRINO_ICON_MENUITEM_SOFTUPDATE));
	}

	service->integratePlugins(CPlugins::I_TYPE_SERVICE);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	resetLCDMode();
	
	return res;
}

