//
//	$Id: proxyserver_setup.cpp 25022025 mohousch Exp $
//
//	proxyserver_setup menue - Neutrino-GUI
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2010 T. Graf 'dbt'
//	Homepage: http://www.dbox2-tuning.net/
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

#include <gui/proxyserver_setup.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/keyboard_input.h>

#include <system/debug.h>


int CProxySetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "CProxySetup::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = CMenuTarget::RETURN_REPAINT;

	if (parent)
		parent->hide();
		
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return res;
	}

	res = showProxySetup();
	
	return res;
}

int CProxySetup::showProxySetup()
{
	dprintf(DEBUG_DEBUG, "CProxySetup::showProxySetup\n");

	//
	CWidget* widget = NULL;
	ClistBox* mn = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("proxysetup");
	
	if (widget)
	{
		mn = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "proxysetup";
		
		//
		mn = new ClistBox(&box);

		mn->setWidgetMode(ClistBox::MODE_SETUP);
		mn->enableShrinkMenu();
		
		mn->enablePaintHead();
		mn->setTitle(_("Proxy server"), NEUTRINO_ICON_NETWORK);

		mn->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		mn->setFootButtons(&btn);
		
		//
		widget->addCCItem(mn);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Proxy server"));
	
	//
	mn->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	mn->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// hostname
	CKeyboardInput * softUpdate_proxy = new CKeyboardInput(_("Hostname"), g_settings.softupdate_proxyserver, MAX_INPUT_CHARS, _("enter proxyserver name or ip, use host:port"), _("a empty entry means no proxy"));
	mn->addItem(new CMenuForwarder(_("Hostname"), true, g_settings.softupdate_proxyserver, softUpdate_proxy));

	// username
	CKeyboardInput * softUpdate_proxyuser = new CKeyboardInput(_("Username"), g_settings.softupdate_proxyusername, MAX_INPUT_CHARS, _("enter the proxyserver username"), _("a empty entry means no proxy-auth"));
	mn->addItem(new CMenuForwarder(_("Username"), true, g_settings.softupdate_proxyusername, softUpdate_proxyuser));

	// password
	CKeyboardInput * softUpdate_proxypass = new CKeyboardInput(_("Password"), g_settings.softupdate_proxypassword, MAX_INPUT_CHARS, _("enter the proxyserver password"), _("a empty entry means no proxy-auth"));
	mn->addItem(new CMenuForwarder(_("Password"), true, g_settings.softupdate_proxypassword, softUpdate_proxypass));

	//
	int res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	if (softUpdate_proxy)
	{
		delete softUpdate_proxy;
		softUpdate_proxy = NULL;
	}
	
	if (softUpdate_proxyuser)
	{
		delete softUpdate_proxyuser;
		softUpdate_proxyuser = NULL;
	}
	
	if (softUpdate_proxypass)
	{
		delete softUpdate_proxypass;
		softUpdate_proxypass = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

