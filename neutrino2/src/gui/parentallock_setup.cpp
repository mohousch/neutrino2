/*
	Neutrino-GUI  -   DBoxII-Project

	$id: parentallock_setup.cpp 2016.01.02 20:10:30 mohousch $
	
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
#include <gui/parentallock_setup.h>

#include <system/debug.h>
#include <system/helpers.h>


//extern bool parentallocked;			// defined neutrino.cpp

#define PARENTALLOCK_PROMPT_OPTION_COUNT 3
const keyval PARENTALLOCK_PROMPT_OPTIONS[PARENTALLOCK_PROMPT_OPTION_COUNT] =
{
	{ PARENTALLOCK_PROMPT_NEVER         , _("never")         },
	{ PARENTALLOCK_PROMPT_CHANGETOLOCKED, _("on locked bouquets") },
	{ PARENTALLOCK_PROMPT_ONSIGNAL      , _("on broadcasted lock")      }
};

#define PARENTALLOCK_LOCKAGE_OPTION_COUNT 3
const keyval PARENTALLOCK_LOCKAGE_OPTIONS[PARENTALLOCK_LOCKAGE_OPTION_COUNT] =
{
	{ 12, _("from 12 years up") },
	{ 16, _("from 16 years up") },
	{ 18, _("from 18 years up") }
};

CParentalLockSettings::CParentalLockSettings()
{
}

CParentalLockSettings::~CParentalLockSettings()
{
}

int CParentalLockSettings::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CParentalLockSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CWidgetTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CParentalLockSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CParentalLockSettings::showMenu:\n");
	
	CWidget *widget = NULL;
	ClistBox *listBox = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("parentallocksetup");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "parentallocksetup";
		
		//
		listBox = new ClistBox(&box);

		listBox->setWidgetMode(ClistBox::MODE_SETUP);
		listBox->enableShrinkMenu();
		
		//
		listBox->enablePaintHead();
		listBox->setTitle(_("Parentallock settings"), NEUTRINO_ICON_LOCK);

		//
		listBox->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		listBox->setFootButtons(&btn);
		
		//
		widget->addCCItem(listBox);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Parentallock settings"));
	
	// intro
	listBox->addItem(new CMenuForwarder(_("back")));
	listBox->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	listBox->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	listBox->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// prompt
	listBox->addItem(new CMenuOptionChooser(_("Prompt for PIN"), &g_settings.parentallock_prompt, PARENTALLOCK_PROMPT_OPTIONS, PARENTALLOCK_PROMPT_OPTION_COUNT, /*!parentallocked*/true));

	// lockage
	listBox->addItem(new CMenuOptionChooser(_("Lock program"), &g_settings.parentallock_lockage, PARENTALLOCK_LOCKAGE_OPTIONS, PARENTALLOCK_LOCKAGE_OPTION_COUNT, /*!parentallocked*/true));

	// Pin
	CPINChangeWidget * pinChangeWidget = new CPINChangeWidget(_("Change PIN code"), g_settings.parentallock_pincode, 4, _("Enter your new youth protection pin code here!"));
	listBox->addItem( new CMenuForwarder(_("Change PIN code"), true, g_settings.parentallock_pincode, pinChangeWidget, NULL));
	
	//
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

