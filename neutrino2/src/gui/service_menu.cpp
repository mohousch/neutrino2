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

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>

#include <gui/scan_setup.h>
#include <gui/widget/icons.h>
#include <gui/update.h>
#include <gui/cam_menu.h>
#include <gui/imageinfo.h>

#include <gui/bedit/bouqueteditor_bouquets.h>


#if !defined (PLATFORM_COOLSTREAM)
#if defined (ENABLE_CI)
extern CCAMMenuHandler * g_CamHandler;		// defined neutrino.cpp
#endif
#endif

extern int FrontendCount;			// defined in zapit.cpp

//
int CServiceMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CServiceMenu::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

// showmenu
void CServiceMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CServiceMenu::showMenu\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* service = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("system");
	
	if (widget == NULL)
	{
		service = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
		service->setWidgetMode(MODE_MENU);
		service->setWidgetType(TYPE_CLASSIC);
		service->enableShrinkMenu();
		
		//
		service->enablePaintHead();
		service->setTitle(_("System"), NEUTRINO_ICON_SERVICE);
		service->enablePaintDate();
		
		//
		service->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		service->setFootButtons(&btn);
	
		// tuner/scan setup
		service->addItem(new ClistBoxItem(_("Scan transponder"), true, NULL, new CTunerSetup(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SCANSETTINGS));

		// reload Channels
		//service->addItem(new ClistBoxItem(_("Reload channel lists"), true, NULL, CNeutrinoApp::getInstance(), "reloadchannels", RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_RELOADCHANNELS));
		
		// Bouquets Editor
		service->addItem(new ClistBoxItem(_("Bouquet Editor"), true, NULL, new CBEBouquetWidget(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_BOUQUETSEDITOR));
		
		// CI Cam 	
#if defined (ENABLE_CI)
		service->addItem(new ClistBoxItem(_("CI Cam"), true, NULL, g_CamHandler, NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_CICAM));
#endif
		
		// software update
		service->addItem(new ClistBoxItem(_("Software update"), true, NULL, new CUpdateSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SOFTUPDATE));

		service->integratePlugins(CPlugins::I_TYPE_SERVICE);
		
		//
		widget = new CWidget(service->getWindowsPos().iX, service->getWindowsPos().iY, service->getWindowsPos().iWidth, service->getWindowsPos().iHeight);
		widget->name = "system";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(service);
	}
	
	widget->exec(NULL, "");
}


