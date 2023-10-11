/*
	$port: proxyserver_setup.cpp,v 1.2 2010/03/02 20:11:42 tuxbox-cvs Exp $

	proxyserver_setup menue - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2010 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#include "gui/proxyserver_setup.h"

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/stringinput.h>

#include <system/debug.h>


int CProxySetup::exec(CMenuTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "CProxySetup::exec\n");
	
	int res = CMenuTarget::RETURN_REPAINT;

	if (parent)
		parent->hide();

	res = showProxySetup();
	
	return res;
}

/* shows entries for proxy settings */
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
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "proxysetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		mn = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		mn->setWidgetMode(ClistBox::MODE_SETUP);
		mn->enableShrinkMenu();
		
		mn->enablePaintHead();
		mn->setTitle(_("Proxy server"), NEUTRINO_ICON_NETWORK);

		mn->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mn->setFootButtons(&btn);
		
		//
		widget->addCCItem(mn);
	}

	CStringInputSMS * softUpdate_proxy = new CStringInputSMS(_("Hostname"), g_settings.softupdate_proxyserver, MAX_INPUT_CHARS, _("enter proxyserver name or ip, use host:port"), _("a empty entry means no proxy"), "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
	mn->addItem(new CMenuForwarder(_("Hostname"), true, g_settings.softupdate_proxyserver, softUpdate_proxy));

	CStringInputSMS * softUpdate_proxyuser = new CStringInputSMS(_("Username"), g_settings.softupdate_proxyusername, MAX_INPUT_CHARS, _("enter the proxyserver username"), _("a empty entry means no proxy-auth"), "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
	mn->addItem(new CMenuForwarder(_("Username"), true, g_settings.softupdate_proxyusername, softUpdate_proxyuser));

	CStringInputSMS * softUpdate_proxypass = new CStringInputSMS(_("Password"), g_settings.softupdate_proxypassword, MAX_INPUT_CHARS, _("enter the proxyserver password"), _("a empty entry means no proxy-auth"), "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
	mn->addItem(new CMenuForwarder(_("Password"), true, g_settings.softupdate_proxypassword, softUpdate_proxypass));

	//
	int res = widget->exec(NULL, "");
	
	if (mn)
	{
		delete mn;
		mn = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	return res;
}

