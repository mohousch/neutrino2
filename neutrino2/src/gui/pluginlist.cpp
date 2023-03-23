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

//#include <plugin.h>

#include <gui/pluginlist.h>
#include <gui/widget/infobox.h>
#include <gui/widget/icons.h>

#include <sstream>
#include <fstream>
#include <iostream>

#include <dirent.h>
#include <dlfcn.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <plugins.h>
#include <driver/encoding.h>

#include <system/debug.h>


extern CPlugins * g_PluginList;    /* neutrino.cpp */

CPluginList::CPluginList()
{
	selected = 0;

	pWidget = NULL;
	plist = NULL;
	item = NULL;
}

CPluginList::~CPluginList()
{
}

void CPluginList::hide()
{
	plist->hide();	
}

#define NUM_LIST_BUTTONS 4
struct button_label CPluginListButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Start") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, " " },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Reload")}
};

struct button_label CPluginListHeadButtons = {NEUTRINO_ICON_BUTTON_HELP_SMALL, "" };

void CPluginList::showMenu()
{
	dprintf(DEBUG_NORMAL, "CPluginList::showMenu\n");
	
	pWidget = CNeutrinoApp::getInstance()->getWidget("plugins");

	//
	if (pWidget)
	{
		plist = (ClistBox*)pWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		pWidget = new CWidget(0, 0, CFrameBuffer::getInstance()->getScreenWidth() / 20 * 17, CFrameBuffer::getInstance()->getScreenHeight() / 20 * 18);
		
		pWidget->name = "plugins";
		
		pWidget->setMenuPosition(MENU_POSITION_CENTER);
		
		//
		plist = new ClistBox(0, 0, CFrameBuffer::getInstance()->getScreenWidth() / 20 * 17, CFrameBuffer::getInstance()->getScreenHeight() / 20 * 18);
		
		plist->setWidgetType(TYPE_CLASSIC);
		plist->setWidgetMode(MODE_MENU);
		plist->enableShrinkMenu();
		
		// head
		plist->enablePaintHead();
		plist->setTitle(_("Plugins"), NEUTRINO_ICON_FEATURES);
		plist->setTitleHAlign(CC_ALIGN_CENTER);
		plist->enablePaintDate();
		plist->setFormat("%d.%m.%Y %H:%M:%S");
		plist->setHeadButtons(&CPluginListHeadButtons, 1);
		
		// foot
		plist->enablePaintFoot();
		plist->setFootButtons(CPluginListButtons, NUM_LIST_BUTTONS);
		
		pWidget->addWidgetItem(plist);
	}
	
	//
	plist->clearItems();

	//
	for(unsigned int count = 0; count < (unsigned int)g_PluginList->getNumberOfPlugins(); count++)
	{
		std::string IconName = "";

		IconName = PLUGINDIR;
		IconName += "/";
		IconName += g_PluginList->getFileName(count);
		IconName += "/";
		IconName += g_PluginList->getIcon(count);

		bool enabled = g_PluginList->getType(count) != CPlugins::P_TYPE_DISABLED;
		
		// skip hidden plugins
		if (!g_PluginList->isHidden(count))
		{	
			item = new ClistBoxItem(_(g_PluginList->getName(count)), enabled, _(g_PluginList->getDescription(count).c_str()), CPluginsExec::getInstance(), g_PluginList->getFileName(count));
			
			item->setHintIcon(file_exists(IconName.c_str())? IconName.c_str() : NEUTRINO_ICON_MENUITEM_PLUGIN);

			item->set2lines();
			//item->setBorderMode();

			plist->addItem(item);
		}
	}
	
	//
	pWidget->addKey(RC_red, this, CRCInput::getSpecialKeyName(RC_red));
	pWidget->addKey(RC_green, this, CRCInput::getSpecialKeyName(RC_green));
	pWidget->addKey(RC_blue, this, CRCInput::getSpecialKeyName(RC_blue));
	pWidget->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));
	pWidget->addKey(RC_ok, this, CRCInput::getSpecialKeyName(RC_ok));
	
	//
	pWidget->exec(NULL, "");
}

int CPluginList::exec(CMenuTarget * parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPluginList::exec: actionKey:%s\n", actionKey.c_str());

	if (parent)
		parent->hide();

	if(actionKey == "RC_red")
	{
		selected = plist->getSelected();

		// remove selected plugin
		g_PluginList->removePlugin(selected);

		// relaod plugins
		g_PluginList->loadPlugins();

		if(selected > (int)g_PluginList->getNumberOfPlugins() - 1)
			selected = (int)g_PluginList->getNumberOfPlugins() - 1;

		showMenu();
		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		selected = plist->getSelected();
		g_PluginList->startPlugin(plist->getSelected());
	}
	else if(actionKey == "RC_blue")
	{
		g_PluginList->loadPlugins();
		showMenu();
		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_info")
	{
		selected = plist->getSelected();
		std::string buffer;

		buffer = "Name: ";
		buffer += _(g_PluginList->getName(selected));
		buffer += "\n";

		if(!g_PluginList->getDescription(selected).empty())
		{
			buffer += _("Description: ");
			buffer += _(g_PluginList->getDescription(selected).c_str());
			buffer += "\n";
		}

		if(!g_PluginList->getVersion(selected).empty())
		{
			buffer += "Version: ";
			buffer += g_PluginList->getVersion(selected);
			buffer += "\n";
		}
		
		InfoBox(buffer.c_str(), _("Plugins"), NEUTRINO_ICON_SHELL);
		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_ok")
	{
		selected = plist->getSelected();

		if(pluginSelected() == close)
			return RETURN_EXIT_ALL;
		else
			return RETURN_REPAINT;
	}

	showMenu();

	return RETURN_REPAINT;
}

CPluginList::result_ CPluginList::pluginSelected()
{
	g_PluginList->startPlugin(plist->getSelected());
	
	return resume;
}

CPluginChooser::CPluginChooser(char* pluginname)
	: CPluginList(), selected_plugin(pluginname)
{
}

CPluginList::result_ CPluginChooser::pluginSelected()
{
	strcpy(selected_plugin, g_PluginList->getFileName(plist->getSelected()));

	return CPluginList::close;
}

//
CPluginsExec* CPluginsExec::getInstance()
{
	static CPluginsExec* pluginsExec = NULL;

	if (!pluginsExec)
		pluginsExec = new CPluginsExec();

	return pluginsExec;
}

int CPluginsExec::exec(CMenuTarget* parent, const std::string& actionKey)
{
	if (actionKey.empty())
		return RETURN_NONE;

	dprintf(DEBUG_NORMAL, "CPluginsExec exec: %s\n", actionKey.c_str());

	if (parent != NULL)
		parent->hide();

	if (!actionKey.empty())
		g_PluginList->startPlugin(actionKey.c_str());

	return RETURN_REPAINT;
}


