//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: audioplayer_setup.cpp 24022025 mohousch $
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
#include <gui/audioplayer.h>
#include <gui/audioplayer_setup.h>

#include <system/debug.h>
#include <system/helpers.h>


int CAudioPlayerSettings::exec(CTarget* parent, const std::string& actionKey)
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

		this->setValueString(g_settings.network_nfs_audioplayerdir);
		
		return CTarget::RETURN_REPAINT;
	}
	
	showMenu();
	this->clearValueString();
	
	return CTarget::RETURN_REPAINT;
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
		audioPlayerSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "audioplayersetup";
		
		//
		audioPlayerSettings = new ClistBox(&box);

		audioPlayerSettings->setMode(ClistBox::MODE_SETUP);
		audioPlayerSettings->enableShrinkMenu();
		
		//
		audioPlayerSettings->enablePaintHead();
		audioPlayerSettings->setTitle(_("Audioplayer settings"), NEUTRINO_ICON_AUDIOPLAYER);

		//
		audioPlayerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		audioPlayerSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(audioPlayerSettings);
	}
	
	//
	setLCDMode(_("Audioplayer settings"));
	
	// intros
	audioPlayerSettings->addItem(new CMenuForwarder(_("back")));
	audioPlayerSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	audioPlayerSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	audioPlayerSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// start dir
	CMenuForwarder *m1 = new CMenuForwarder(_("Start dir."), true, g_settings.network_nfs_audioplayerdir, this, "audioplayerdir");
	audioPlayerSettings->addItem(m1);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        resetLCDMode();
}

