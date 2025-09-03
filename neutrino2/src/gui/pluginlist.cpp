/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#include <gui/pluginlist.h>
#include <gui/widget/infobox.h>

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
	if (plist) plist->hide();	
}

#define NUM_LIST_BUTTONS 4
struct button_label CPluginListButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete"), 0 },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Start"), 0 },
	{ NEUTRINO_ICON_BUTTON_YELLOW, " ", 0 },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Reload"), 0}
};

struct button_label CPluginListHeadButtons = {NEUTRINO_ICON_BUTTON_HELP_SMALL, " ", 0 };

int CPluginList::showMenu()
{
	dprintf(DEBUG_NORMAL, "CPluginList::showMenu\n");
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	pWidget = CNeutrinoApp::getInstance()->getWidget("plugins");

	//
	if (pWidget)
	{
		plist = (ClistBox*)pWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		pWidget = new CWidget(&box);
		pWidget->name = "plugins";
		
		//
		plist = new ClistBox(&box);
		plist->setWidgetType(ClistBox::TYPE_CLASSIC);
		plist->setWidgetMode(ClistBox::MODE_MENU);
		plist->enableShrinkMenu();
		plist->paintMainFrame(true);
		
		// head
		plist->enablePaintHead();
		plist->setTitle(_("Plugins"), NEUTRINO_ICON_FEATURES);
		plist->enablePaintDate();
		plist->setFormat("%d.%m.%Y %H:%M");
		plist->setHeadButtons(&CPluginListHeadButtons, 1);
		// foot
		plist->enablePaintFoot();
		plist->setFootButtons(CPluginListButtons, NUM_LIST_BUTTONS);
		
		// recalc widget  pos
		pWidget->setPosition(plist->getWindowsPos().iX, plist->getWindowsPos().iY, plist->getWindowsPos().iWidth, plist->getWindowsPos().iHeight);
		pWidget->addCCItem(plist);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Plugins"));

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
			item = new CMenuForwarder(_(g_PluginList->getName(count)), enabled, _(g_PluginList->getDescription(count).c_str()), CPluginsExec::getInstance(), g_PluginList->getFileName(count));
			
			item->setHintIcon(file_exists(IconName.c_str())? IconName.c_str() : NEUTRINO_ICON_MENUITEM_PLUGIN);
			
			item->set2lines(true);

			if (plist) plist->addItem(item);
		}
	}
	
	//
	pWidget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	pWidget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	pWidget->addKey(CRCInput::RC_blue, this, CRCInput::getSpecialKeyName(CRCInput::RC_blue));
	pWidget->addKey(CRCInput::RC_info, this, CRCInput::getSpecialKeyName(CRCInput::RC_info));
	pWidget->addKey(CRCInput::RC_ok, this, CRCInput::getSpecialKeyName(CRCInput::RC_ok));
	
	//
	pWidget->setTimeOut(g_settings.timing_filebrowser);
	res = pWidget->exec(NULL, "");
	
	if (pWidget)
	{
		delete pWidget;
		pWidget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

int CPluginList::exec(CWidgetTarget * parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPluginList::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = CWidgetTarget::RETURN_REPAINT;

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
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		selected = plist? plist->getSelected() : 0;
		g_PluginList->startPlugin(selected);
	}
	else if(actionKey == "RC_blue")
	{
		g_PluginList->loadPlugins();
		showMenu();
		return CWidgetTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_info")
	{
		selected = plist? plist->getSelected() : 0;
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
		
		InfoBox(_("Plugins"), buffer.c_str(), NEUTRINO_ICON_SHELL);
		return CWidgetTarget::RETURN_REPAINT;
	}
	else if(actionKey == "RC_ok")
	{
		if(pluginSelected() == close)
			return CWidgetTarget::RETURN_EXIT_ALL;
		else
			return CWidgetTarget::RETURN_REPAINT;
	}

	res = showMenu();

	return res;
}

CPluginList::result_ CPluginList::pluginSelected()
{
	selected = plist? plist->getSelected() : 0;
	
	g_PluginList->startPlugin(selected);
	
	return resume;
}

CPluginChooser::CPluginChooser(char* pluginname)
	: CPluginList(), selected_plugin(pluginname)
{
}

CPluginList::result_ CPluginChooser::pluginSelected()
{
	int selected = plist? plist->getSelected() : 0;
	
	strcpy(selected_plugin, g_PluginList->getFileName(selected));

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

int CPluginsExec::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	if (actionKey.empty())
		return CWidgetTarget::RETURN_NONE;

	dprintf(DEBUG_NORMAL, "CPluginsExec exec: %s\n", actionKey.c_str());

	if (parent != NULL)
		parent->hide();

	if (!actionKey.empty())
		g_PluginList->startPlugin(actionKey.c_str());

	return CWidgetTarget::RETURN_REPAINT;
}

