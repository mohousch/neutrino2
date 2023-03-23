/*
	Neutrino-GUI  -   DBoxII-Project

	$id: audioplayer_setup.cpp 2016.01.02 20:57:30 mohousch $
	
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
#include <gui/audioplayer.h>
#include <gui/audioplayer_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("No") },
	{ 1, _("Yes") }
};

CAudioPlayerSettings::CAudioPlayerSettings()
{
}

CAudioPlayerSettings::~CAudioPlayerSettings()
{
}

int CAudioPlayerSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayerSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	if(parent)
		parent->hide();
	
	if(actionKey == "audioplayerdir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.network_nfs_audioplayerdir))
			strncpy(g_settings.network_nfs_audioplayerdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_audioplayerdir) - 1);

		getString() = g_settings.network_nfs_audioplayerdir;
		
		return RETURN_REPAINT;
	}
	
	showMenu();
	
	return RETURN_REPAINT;
}

void CAudioPlayerSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CAudioPlayerSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* audioPlayerSettings = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("audioplayersetup");
	
	if (widget)
	{
		audioPlayerSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		audioPlayerSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		audioPlayerSettings->setWidgetMode(MODE_SETUP);
		audioPlayerSettings->enableShrinkMenu();
		
		audioPlayerSettings->enablePaintHead();
		audioPlayerSettings->setTitle(_("Audioplayer settings"), NEUTRINO_ICON_AUDIOPLAYER);

		audioPlayerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		audioPlayerSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "audioplayersetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(audioPlayerSettings);
	}
	
	audioPlayerSettings->clearItems();
	
	// intros
	audioPlayerSettings->addItem(new ClistBoxItem(_("back")));
	audioPlayerSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	audioPlayerSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	audioPlayerSettings->addItem( new CMenuSeparator(LINE) );

	// high prio
	audioPlayerSettings->addItem(new CMenuOptionChooser(_("High decode priority"), &g_settings.audioplayer_highprio, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true ));

	// start dir
	audioPlayerSettings->addItem(new ClistBoxItem(_("Start dir."), true, g_settings.network_nfs_audioplayerdir, this, "audioplayerdir"));
	
	//
	widget->exec(NULL, "");
}


