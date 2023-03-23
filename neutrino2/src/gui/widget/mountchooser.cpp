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

#include <gui/widget/mountchooser.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <system/debug.h>


CMountChooser::CMountChooser(const char* const Name, const std::string& Icon, int* chosenIndex, char* chosenLocalDir, const char* const selectedLocalDir, const int mwidth, const int mheight)
{
	//
	widget = CNeutrinoApp::getInstance()->getWidget("mountchooser");
		
	if (widget)
	{
		menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
			
		// title
		if (menu->hasHead())
			menu->setTitle(Name, Icon.c_str());
	}
	else
	{
		menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		menu->setWidgetMode(MODE_SETUP);
		
		//
		menu->enablePaintHead();
		menu->setTitle(Name, Icon.c_str());
			
		//
		menu->enablePaintFoot();		
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
		menu->setFootButtons(&btn);
			
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "mountchooser";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->enableSaveScreen();
		widget->addWidgetItem(menu);
	}
		
	menu->clear();

	//
	char indexStr[2];
	for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		if (g_settings.network_nfs_local_dir[i] != NULL &&
		    strcmp(g_settings.network_nfs_local_dir[i],"") != 0 &&
		    (strstr(g_settings.network_nfs_mount_options1[i], "rw") != NULL ||
		     strstr(g_settings.network_nfs_mount_options2[i], "rw") != NULL))
		{
			std::string s(g_settings.network_nfs_local_dir[i]);
			s += " (";
			s += g_settings.network_nfs_ip[i];
			s += ":";
			s += g_settings.network_nfs_dir[i];
			s +=")";
			snprintf(indexStr,2,"%d",i);
			
			menu->addItem(new ClistBoxItem(s.c_str(), true, NULL, this, (std::string("MID:") + std::string(indexStr)).c_str()), (strcmp(selectedLocalDir,g_settings.network_nfs_local_dir[i]) == 0));
		}
	}	
}

int CMountChooser::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMountChooser::exec: actionKey:%s\n", actionKey.c_str());

	const char * key = actionKey.c_str();
	
	if (strncmp(key, "MID:", 4) == 0)
	{
		int mount_id = -1;
		sscanf(&key[4],"%d",&mount_id);
		
		if ((mount_id > -1) && (mount_id < NETWORK_NFS_NR_OF_ENTRIES))
		{
			if (index)
				*index = mount_id;
				
			if (localDir) 
				strcpy(localDir, g_settings.network_nfs_local_dir[mount_id]);
		}

		hide();

		return RETURN_EXIT;
	} 
	else 
	{
		return widget->exec(parent, actionKey);
	}
}

void CMountChooser::setSelectedItem(int selection)
{
	//selected = selection;
}

bool CMountChooser::hasItem()
{
	bool ret = false;
	
	if (menu)
	{
		if (menu->hasItem())
			ret = true;
	}
	
	return ret;
}



