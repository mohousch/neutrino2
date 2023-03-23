/*
	Neutrino-GUI  -   DBoxII-Project

	$id: pictureviewer_setup.cpp 2016.01.02 21:09:30 mohousch $
	
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

#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <gui/filebrowser.h>
#include <gui/pictureviewer_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>

#include <video_cs.h>
#include <audio_cs.h>


#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("No") },
	{ 1, _("Yes") }
};

#define PICTUREVIEWER_SCALING_OPTION_COUNT 3
const keyval PICTUREVIEWER_SCALING_OPTIONS[PICTUREVIEWER_SCALING_OPTION_COUNT] =
{
	{ SIMPLE, _("simple") },
	{ COLOR , _("advanced") },
	{ NONE  , _("none") }
};

CPictureViewerSettings::CPictureViewerSettings()
{
}

CPictureViewerSettings::~CPictureViewerSettings()
{
}

int CPictureViewerSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPicTureViewerSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "picturedir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.network_nfs_picturedir))
			strncpy(g_settings.network_nfs_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_picturedir)-1);

		getString() = g_settings.network_nfs_picturedir;
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void CPictureViewerSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CPicTureViewerSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* PicViewerSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("pictureviewersetup");
	
	if (widget)
	{
		PicViewerSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		PicViewerSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		PicViewerSettings->setWidgetMode(MODE_SETUP);
		PicViewerSettings->enableShrinkMenu();
		
		PicViewerSettings->enablePaintHead();
		PicViewerSettings->setTitle(_("Pictureviewer settings"), NEUTRINO_ICON_PICTUREVIEWER);

		PicViewerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		PicViewerSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "pictureviewersetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(PicViewerSettings);
	}
	
	PicViewerSettings->clearItems();
	
	// intros
	PicViewerSettings->addItem(new ClistBoxItem(_("back")));
	PicViewerSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	PicViewerSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	PicViewerSettings->addItem( new CMenuSeparator(LINE) );

	// Pic Viewer Scaling
	PicViewerSettings->addItem(new CMenuOptionChooser(_("Scaling"), &g_settings.picviewer_scaling, PICTUREVIEWER_SCALING_OPTIONS, PICTUREVIEWER_SCALING_OPTION_COUNT, true));

	// slide Timeout
	PicViewerSettings->addItem(new CMenuOptionNumberChooser(_("Slideshow display time"), &g_settings.picviewer_slide_time, true, 0, 999));

	// Pic Viewer Default Dir
	PicViewerSettings->addItem(new ClistBoxItem(_("Start dir."), true, g_settings.network_nfs_picturedir, this, "picturedir"));
	
	//
	widget->exec(NULL, "");
}


