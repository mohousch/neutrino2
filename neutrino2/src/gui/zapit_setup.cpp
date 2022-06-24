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
#include <bouquets.h>

#include "gui/zapit_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/channel_select.h>

#include <system/debug.h>


extern CBouquetManager * g_bouquetManager;	// defined in der zapit.cpp

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
	//{ NeutrinoMessages::mode_webtv, "WEBTV" }
};

CZapitSetup::CZapitSetup()
{
}

CZapitSetup::~CZapitSetup()
{
}

int CZapitSetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CZapitSetup::exec: actionKey:%s\n", actionKey.c_str());
	
	int   res = RETURN_REPAINT;
	CSelectChannelWidget*  CSelectChannelWidgetHandler;
	
	if (parent)
		parent->hide();
	
	if(actionKey == "tv")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "tv");
		
		g_settings.startchanneltv_id = CSelectChannelWidgetHandler->getChannelID();
		g_settings.StartChannelTV = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		g_settings.startchanneltv_nr = g_Zapit->getChannelNumber(CSelectChannelWidgetHandler->getChannelID()) - 1;

		this->getString() = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "radio")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "radio");
		
		g_settings.startchannelradio_id = CSelectChannelWidgetHandler->getChannelID();
		g_settings.StartChannelRadio = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		g_settings.startchannelradio_nr = g_Zapit->getChannelNumber(CSelectChannelWidgetHandler->getChannelID()) - 1;

		this->getString() = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		return RETURN_REPAINT;
	}
	/*
	else if(actionKey == "webtv")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "webtv");
		
		g_settings.startchannelwebtv_id = CSelectChannelWidgetHandler->getChannelID();
		g_settings.StartChannelWEBTV = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		g_settings.startchannelwebtv_nr = g_Zapit->getChannelNumber(CSelectChannelWidgetHandler->getChannelID()) - 1;

		this->getString() = g_Zapit->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;

		printf("id:%llx name:%s nr:%d\n", g_settings.startchannelwebtv_id, g_settings.StartChannelWEBTV.c_str(), g_settings.startchannelwebtv_nr);
		
		return RETURN_REPAINT;
	}
	*/

	showMenu();

	return res;
}

void CZapitSetup::showMenu()
{
	dprintf(DEBUG_NORMAL, "CZapitSetup::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* zapit = NULL;
	
	if (CNeutrinoApp::getInstance()->getWidget("zapitsetup"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("zapitsetup");
		zapit = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		zapit = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		zapit->setMenuPosition(MENU_POSITION_CENTER);
		zapit->setWidgetMode(MODE_SETUP);
		zapit->enableShrinkMenu();
		
		zapit->enablePaintHead();
		zapit->setTitle(_("Start Channel settings"), NEUTRINO_ICON_SETTINGS);

		zapit->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		zapit->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addItem(zapit);
	}
	
	zapit->clearItems();
	
	// intros
	zapit->addItem(new CMenuForwarder(_("back")));
	zapit->addItem(new CMenuSeparator(LINE));
	
	// save settings
	zapit->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	zapit->addItem(new CMenuSeparator(LINE));

	bool activTV = false;
	bool activRadio = false;
	//bool activWebTV = false;

	if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_tv) )
		activTV = true;

	if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_radio) )
		activRadio = true;

	//if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_webtv) )
	//	activWebTV = true;

	// last TV channel
	CMenuForwarder * m3 = new CMenuForwarder(_("TV Channel"), activTV, g_settings.StartChannelTV.c_str(), this, "tv");

	// last radio channel
	CMenuForwarder * m4 = new CMenuForwarder(_("Radio Channel"), activRadio, g_settings.StartChannelRadio.c_str(), this, "radio");

	// last webtv channel
	//CMenuForwarder * m5 = new CMenuForwarder(_("WebTV Channel"), activWebTV, g_settings.StartChannelWEBTV.c_str(), this, "webtv");

	// last mode
	CZapitSetupModeNotifier zapitSetupModeNotifier((int *)&g_settings.lastChannelMode, m3, m4/*, m5*/);

	CMenuOptionChooser * m2 = new CMenuOptionChooser(_("Start Mode"), (int *)&g_settings.lastChannelMode, OPTIONS_LASTMODE_OPTIONS, OPTIONS_LASTMODE_OPTION_COUNT, !g_settings.uselastchannel, &zapitSetupModeNotifier);
	
	// use lastchannel
	CZapitSetupNotifier zapitSetupNotifier(m2, m3, m4/*, m5*/);

	CMenuOptionChooser * m1 = new CMenuOptionChooser(_("Start Channel"), &g_settings.uselastchannel, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, &zapitSetupNotifier);
	
	zapit->addItem(m1);
	zapit->addItem(m2);
	zapit->addItem(m3);
	zapit->addItem(m4);
	//zapit->addItem(m5);

	//
	widget->exec(NULL, "");
	
	delete zapit;
}

//
CZapitSetupNotifier::CZapitSetupNotifier(CMenuOptionChooser* m1, CMenuForwarder* m2, CMenuForwarder* m3/*, CMenuForwarder* m4*/)
{
	zapit1 = m1;
	zapit2 = m2;
	zapit3 = m3;
	//zapit4 = m4;
}

bool CZapitSetupNotifier::changeNotify(const std::string& OptionName, void *)
{
	if (OptionName == _("Start Channel"))
	{
		bool activTV = false;
		bool activRadio = false;
		//bool activWebTV = false;

		if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_tv) )
			activTV = true;

		if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_radio) )
			activRadio = true;

		//if( (!g_settings.uselastchannel) && (g_settings.lastChannelMode == NeutrinoMessages::mode_webtv) )
		//	activWebTV = true;

		zapit1->setActive(!g_settings.uselastchannel);
		zapit2->setActive(activTV);
		zapit3->setActive(activRadio);
		//zapit4->setActive(activWebTV);
	}

	return true;
}

//
CZapitSetupModeNotifier::CZapitSetupModeNotifier(int *zMode, CMenuItem *m1, CMenuItem *m2/*, CMenuItem *m3*/)
{
	mode = zMode;
	item1 = m1;
	item2 = m2;
	//item3 = m3;
}

bool CZapitSetupModeNotifier::changeNotify(const std::string&, void *)
{
	int nmode = *mode;
	
	if(nmode == NeutrinoMessages::mode_radio)
	{
		item1->setActive(false);
		item2->setActive(true);
		item3->setActive(false);
	}
	else if(nmode == NeutrinoMessages::mode_tv)
	{
		item1->setActive(true);
		item2->setActive(false);
		item3->setActive(false);
	}
	/*
	else if(nmode == NeutrinoMessages::mode_webtv)
	{
		item1->setActive(false);
		item2->setActive(false);
		item3->setActive(true);
	}
	*/

	return true;
}




