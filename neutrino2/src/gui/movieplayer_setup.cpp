/*
	Neutrino-GUI  -   DBoxII-Project

	$id: movieplayer_setup.cpp 2016.01.02 20:33:30 mohousch $
	
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

#include <gui/filebrowser.h>
#include <gui/movieplayer_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


CMoviePlayerSettings::CMoviePlayerSettings()
{
}

CMoviePlayerSettings::~CMoviePlayerSettings()
{
}

int CMoviePlayerSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "moviedir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;

		if (b.exec(g_settings.network_nfs_moviedir))
			strncpy(g_settings.network_nfs_moviedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_moviedir)-1);

		getString() = g_settings.network_nfs_moviedir;
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void CMoviePlayerSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* moviePlayerSettings = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("movieplayersetup");
	
	if (widget)
	{
		moviePlayerSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		moviePlayerSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		moviePlayerSettings->setWidgetMode(MODE_SETUP);
		moviePlayerSettings->enableShrinkMenu();
		
		moviePlayerSettings->enablePaintHead();
		moviePlayerSettings->setTitle(_("Movieplayer settings"), NEUTRINO_ICON_MOVIEPLAYER);

		moviePlayerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		moviePlayerSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "movieplayersetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(moviePlayerSettings);
	}
	
	moviePlayerSettings->clearItems();
	
	// intros
	moviePlayerSettings->addItem(new ClistBoxItem(_("back")));
	moviePlayerSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	moviePlayerSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	moviePlayerSettings->addItem( new CMenuSeparator(LINE) );

	// multiformat Dir
	moviePlayerSettings->addItem(new ClistBoxItem(_("Start dir."), true, g_settings.network_nfs_moviedir, this, "moviedir") ); 
	
	//
	widget->exec(NULL, "");
}


