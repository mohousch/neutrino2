//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: movieplayer_setup.cpp 24022025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <stdio.h> 

#include <gui/filebrowser.h>
#include <gui/movieplayer_setup.h>

#include <system/debug.h>
#include <system/helpers.h>


int CMoviePlayerSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "moviedir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;

		if (b.exec(g_settings.network_nfs_moviedir))
			strncpy(g_settings.network_nfs_moviedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_moviedir)-1);

		this->setValueString(g_settings.network_nfs_moviedir);
		
		return ret;
	}
	
	showMenu();
	this->clearValueString();
	
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
		moviePlayerSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "movieplayersetup";
		
		//
		moviePlayerSettings = new ClistBox(&box);

		moviePlayerSettings->setWidgetMode(ClistBox::MODE_SETUP);
		moviePlayerSettings->enableShrinkMenu();
		
		//
		moviePlayerSettings->enablePaintHead();
		moviePlayerSettings->setTitle(_("Movieplayer settings"), NEUTRINO_ICON_MOVIEPLAYER);

		//
		moviePlayerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		moviePlayerSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(moviePlayerSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Movieplayer settings"));
	
	// intros
	moviePlayerSettings->addItem(new CMenuForwarder(_("back")));
	moviePlayerSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	moviePlayerSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	moviePlayerSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// multiformat Dir
	CMenuForwarder *m1 = new CMenuForwarder(_("Start dir."), true, g_settings.network_nfs_moviedir, this, "moviedir");
	moviePlayerSettings->addItem(m1); 
	
	//
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

