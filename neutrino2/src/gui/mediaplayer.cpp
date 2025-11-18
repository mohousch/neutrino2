/*
	Neutrino-GUI  -   DBoxII-Project

	$id: mediaplayer.cpp 2015.12.22 12:07:30 mohousch $
	
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

#include <system/debug.h>

#include <gui/mediaplayer.h>
#include <gui/audioplayer.h>
#include <gui/movieplayer.h>
#include <gui/pictureviewer.h>


int CMediaPlayerMenu::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMediaplayerMenu::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = CTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return res;
}

// showmenu
void CMediaPlayerMenu::showMenu()
{
	dprintf(DEBUG_NORMAL, "CMediaPlayerMenu::showMenu:\n");
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Media Player"));

	//
	CWidget* widget = NULL;
	ClistBox* mediaPlayer = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("mediaplayer");
	
	if (widget)
	{
		mediaPlayer = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "mediaplayer";
		
		//
		mediaPlayer = new ClistBox(&box);
		mediaPlayer->setWidgetMode(ClistBox::MODE_MENU);
		mediaPlayer->setWidgetType(ClistBox::TYPE_STANDARD);
		mediaPlayer->enableShrinkMenu();
		//
		mediaPlayer->enablePaintHead();
		mediaPlayer->setTitle(_("Media Player"), NEUTRINO_ICON_MULTIMEDIA);
		mediaPlayer->enablePaintDate();
		mediaPlayer->paintMainFrame(true);
		
		//
		mediaPlayer->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		mediaPlayer->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) mediaPlayer->enablePaintItemInfo();
		
		//
		widget->addCCItem(mediaPlayer);
	}

	//
	mediaPlayer->integratePlugins(CPlugins::I_TYPE_MULTIMEDIA);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

