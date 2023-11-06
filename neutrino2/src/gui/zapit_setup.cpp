/*
	* $Id: zapit_setup.cpp 16.11.2020 mohousch Exp $
	
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

// zapit includes
#include <zapit/bouquets.h>

#include "gui/zapit_setup.h"

#include <global.h>
#include <neutrino2.h>

#include <gui/channel_select.h>

#include <system/debug.h>


//// globals
extern Zapit_config zapitCfg;

//option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

/* option off1 on0*/
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, _("off") },
        { 0, _("on") }
};

#define OPTIONS_LASTMODE_OPTION_COUNT 2
const keyval OPTIONS_LASTMODE_OPTIONS[OPTIONS_LASTMODE_OPTION_COUNT] =
{
	{ NeutrinoMessages::mode_tv, "TV" },
        { NeutrinoMessages::mode_radio, "Radio" },
};

CZapitSetup::CZapitSetup()
{
	selected = 0;
	widget = NULL;
	zapit = NULL;
	m3 = NULL;
	m4 = NULL;
}

CZapitSetup::~CZapitSetup()
{
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CZapitSetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CZapitSetup::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = CMenuTarget::RETURN_REPAINT;
	CSelectChannelWidget*  CSelectChannelWidgetHandler = NULL;
	
	if (parent)
		parent->hide();
		
	if (zapit) selected = zapit->getSelected();
	
	if(actionKey == "tv")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "tv");
		
		g_settings.startchanneltv_id = CSelectChannelWidgetHandler->getChannelID();
		g_settings.StartChannelTV = CZapit::getInstance()->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		g_settings.startchanneltv_nr = CZapit::getInstance()->getChannelNumber(CSelectChannelWidgetHandler->getChannelID()) - 1;
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		//
		setValueString(g_settings.StartChannelTV.c_str());
		
		return CMenuTarget::RETURN_REPAINT;
	}
	else if(actionKey == "radio")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "radio");
		
		g_settings.startchannelradio_id = CSelectChannelWidgetHandler->getChannelID();
		g_settings.StartChannelRadio = CZapit::getInstance()->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		g_settings.startchannelradio_nr = CZapit::getInstance()->getChannelNumber(CSelectChannelWidgetHandler->getChannelID()) - 1;
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		//
		setValueString(g_settings.StartChannelRadio.c_str());
		
		return CMenuTarget::RETURN_REPAINT;
	}
	else if (actionKey == "savesettings")
	{
		// save zapitconfig	
		zapitCfg.saveLastChannel = !g_settings.uselastchannel;
		zapitCfg.lastchannelmode = g_settings.lastChannelMode;
		zapitCfg.startchanneltv_nr = g_settings.startchanneltv_nr;
		zapitCfg.startchannelradio_nr = g_settings.startchannelradio_nr;
		zapitCfg.startchanneltv_id = g_settings.startchanneltv_id;
		zapitCfg.startchannelradio_id = g_settings.startchannelradio_id;
		
		CZapit::getInstance()->setZapitConfig(&zapitCfg);
		
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return RETURN_REPAINT;
	}

	showMenu();

	return res;
}

void CZapitSetup::showMenu()
{
	dprintf(DEBUG_NORMAL, "CZapitSetup::showMenu:\n");
	
	//
	if (widget == NULL)
		widget = CNeutrinoApp::getInstance()->getWidget("zapitsetup");
	
	if (widget)
	{
		zapit = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "zapitsetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		zapit = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		zapit->setWidgetMode(ClistBox::MODE_SETUP);
		zapit->enableShrinkMenu();
		
		//
		zapit->enablePaintHead();
		zapit->setTitle(_("Start Channel settings"), NEUTRINO_ICON_ZAPIT);
//		zapit->setHeadLine(true, true);

		//
		zapit->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		zapit->setFootButtons(&btn);
//		zapit->setFootLine(true, true);
		
		//
		widget->addCCItem(zapit);
	}
	
	// intros
	zapit->addItem(new CMenuForwarder(_("back")));
	zapit->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// save settings
	zapit->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	zapit->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	bool activTV = false;
	bool activRadio = false;

	if( (g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_tv) )
		activTV = true;

	if( (g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_radio) )
		activRadio = true;

	// last TV channel
	m3 = new CMenuForwarder(_("TV Channel"), activTV, g_settings.StartChannelTV.c_str(), this, "tv");

	// last radio channel
	m4 = new CMenuForwarder(_("Radio Channel"), activRadio, g_settings.StartChannelRadio.c_str(), this, "radio");

	// last mode
	CZapitSetupModeNotifier zapitSetupModeNotifier((int *)&g_settings.lastChannelMode, m3, m4);

	CMenuOptionChooser * m2 = new CMenuOptionChooser(_("Start Mode"), &g_settings.lastChannelMode, OPTIONS_LASTMODE_OPTIONS, OPTIONS_LASTMODE_OPTION_COUNT, g_settings.uselastchannel, &zapitSetupModeNotifier);
	
	// use lastchannel
	CZapitSetupNotifier zapitSetupNotifier(m2, m3, m4);

	CMenuOptionChooser * m1 = NULL;
	m1 = new CMenuOptionChooser(_("Start Channel"), &g_settings.uselastchannel, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &zapitSetupNotifier);
	
	zapit->addItem(m1);
	zapit->addItem(m2);
	zapit->addItem(m3);
	zapit->addItem(m4);
	
	zapit->setSelected(selected);

	//
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

//
CZapitSetupNotifier::CZapitSetupNotifier(CMenuOptionChooser* m1, CMenuForwarder* m2, CMenuForwarder* m3)
{
	zapit1 = m1;
	zapit2 = m2;
	zapit3 = m3;
}

bool CZapitSetupNotifier::changeNotify(const std::string& OptionName, void *)
{
	if (OptionName == _("Start Channel"))
	{
		bool activTV = false;
		bool activRadio = false;

		if( (g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_tv) )
			activTV = true;

		if( (g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_radio) )
			activRadio = true;

		zapit1->setActive(g_settings.uselastchannel);
		zapit2->setActive(activTV);
		zapit3->setActive(activRadio);
	}

	return true;
}

//
CZapitSetupModeNotifier::CZapitSetupModeNotifier(int *zMode, CMenuItem *m1, CMenuItem *m2)
{
	mode = zMode;
	item1 = m1;
	item2 = m2;
}

bool CZapitSetupModeNotifier::changeNotify(const std::string&, void *)
{
	int nmode = *mode;
	
	if(nmode == NeutrinoMessages::mode_radio)
	{
		item1->setActive(false);
		item2->setActive(true);
	}
	else if(nmode == NeutrinoMessages::mode_tv)
	{
		item1->setActive(true);
		item2->setActive(false);
	}

	return true;
}

