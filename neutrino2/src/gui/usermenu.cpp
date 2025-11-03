/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: usermenu.cpp 23042024 mohousch Exp $
 
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

#include <config.h>

#include <neutrino2.h>

#include <gui/widget/listbox.h>
#include <gui/widget/stringinput.h>

#include <gui/usermenu.h>

#define USERMENU_ITEM_OPTION_COUNT SNeutrinoSettings::ITEM_MAX
const keyval USERMENU_ITEM_OPTIONS[USERMENU_ITEM_OPTION_COUNT] =
{
	{ SNeutrinoSettings::ITEM_NONE, " " },  
	{ SNeutrinoSettings::ITEM_TIMERLIST, _("Timerlist") },
	{ SNeutrinoSettings::ITEM_PLUGIN, _("Plugins") },
	{ SNeutrinoSettings::ITEM_VTXT, _("Videotext") },
	{ SNeutrinoSettings::ITEM_REMOTE, _("Lock RC") },
	{ SNeutrinoSettings::ITEM_DELETE_ZAPIT, _("Delete channels") },
	{ SNeutrinoSettings::ITEM_DELETE_WEBTV, _("Delete WebTV channels") },
	{ SNeutrinoSettings::ITEM_FREEMEMORY, _("Free Memory") },
	{ SNeutrinoSettings::ITEM_LOAD_ZAPIT, _("Reload channel lists") },
	{ SNeutrinoSettings::ITEM_LOAD_XMLTV, _("Reload XMLTV EPG") },
	{ SNeutrinoSettings::ITEM_LOAD_EPG, _("Reload EPG") }
};

int CUserMenu::exec(CTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL , "CUserMenu::exec: %s\n", actionKey.c_str());
	
	int res = CTarget::RETURN_REPAINT;
	
        if(parent)
                parent->hide();
                
        if (actionKey == "savesettings")
        {        	
        	CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
        	return RETURN_REPAINT;
        }
                
        res = doMenu();
        
        return res;
}

int CUserMenu::doMenu(void)
{
	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	CCHeaders *head = NULL;
	CCFooters *foot = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("usermenu");
	
	if (widget)
	{	
		menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
		head = (CCHeaders *)widget->getCCItem(CComponent::CC_HEAD);
		foot = (CCFooters *)widget->getCCItem(CComponent::CC_FOOT);
		
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
		widget->enableSaveScreen();
		
		//
		menu = new ClistBox(box.iX, box.iY + 50, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight - 100);
		
		menu->setWidgetMode(ClistBox::MODE_MENU);
		menu->paintMainFrame(true);
		
		// 	
		head = new CCHeaders(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, 50);
			
		//	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0};		

		foot = new CCFooters(widget->getWindowsPos().iX, widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - 50, widget->getWindowsPos().iWidth, 50);
		foot->setButtons(&btn);
			
		//
		widget->addCCItem(menu);
		widget->addCCItem(head);
		widget->addCCItem(foot);
	}
	
	if (head)
	{
		head->setTitle(local.c_str(), NEUTRINO_ICON_KEYBINDING);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, local.c_str());
	
	// intros
	menu->addItem(new CMenuForwarder(_("back")));
	menu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// savesettings
	menu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	menu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

        char text[10];
        
        for(int item = 0; item < SNeutrinoSettings::ITEM_MAX; item++)
        {
                snprintf(text, 10, "%d:", item);
                text[9] = 0;
                
                menu->addItem( new CMenuOptionChooser(text, &g_settings.usermenu[button][item], USERMENU_ITEM_OPTIONS, USERMENU_ITEM_OPTION_COUNT, true, NULL, CRCInput::RC_nokey, "", true ));
        }

	int res = widget->exec(this, "");
        
        if (widget)
        {
        	delete widget;
        	widget = NULL;
        }
        
        //
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

        return res;
}

