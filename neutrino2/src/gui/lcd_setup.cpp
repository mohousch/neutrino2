/*
	Neutrino-GUI  -   DBoxII-Project

	$id: lcd_setup.cpp 2016.01.02 21:24:30 mohousch $
	
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

#include <gui/vfdcontroler.h>
#include <gui/lcd_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define LCDMENU_STATUSLINE_OPTION_COUNT 4
const keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ 0, _("playtime")   },
	{ 1, _("volume")     },
	{ 2, _("volume / playtime")       },
	{ 3, _("volume / playtime / audio") }
};

#if defined (ENABLE_LCD)
#define LCDMENU_EPG_OPTION_COUNT 6
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ 1, _("channel")		},
	{ 2, _("title")		},
	{ 3, _("channel / title")	},
	{ 7, _("channel / sep.-line / title") },
	{ 11, _("channel (short) / title") },
	{ 15, _("channel (short) / sep.-line / title") }
};
#else
#define LCDMENU_EPG_OPTION_COUNT 2
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ 0, _("channel")	},
	{ 1, _("time")			}
};
#endif

#define LCDMENU_EPGALIGN_OPTION_COUNT 2
const keyval LCDMENU_EPGALIGN_OPTIONS[LCDMENU_EPGALIGN_OPTION_COUNT] =
{
	{ 0, _("left")   },
	{ 1, _("center") }
};

#define LCDMENU_LEDCOLOR_OPTION_COUNT 4
const keyval LCDMENU_LEDCOLOR_OPTIONS[LCDMENU_LEDCOLOR_OPTION_COUNT] =
{
	{ 0, _("off") 	},
	{ 1, _("blue") 	},
	{ 2, _("red") 	},
	{ 3, _("purple") },
};

//
CLCDSettings::CLCDSettings()
{
}

CLCDSettings::~CLCDSettings()
{
}

int CLCDSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CLCDSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CLCDSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CLCDSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* lcdSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("lcdsetup");
	
	if (widget)
	{
		lcdSettings = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "lcdsetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		lcdSettings = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		lcdSettings->setWidgetMode(ClistBox::MODE_SETUP);
		lcdSettings->enableShrinkMenu();
		
		lcdSettings->enablePaintHead();
		lcdSettings->setTitle(_("Display settings"), NEUTRINO_ICON_LCD);

		lcdSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		lcdSettings->setFootButtons(&btn);
		
		//
		widget->addWidgetItem(lcdSettings);
	}
	
	// intros
	lcdSettings->addItem(new CMenuForwarder(_("back")));
	lcdSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	lcdSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	CLcdNotifier * lcdnotifier = new CLcdNotifier();
	CVfdControler * lcdsliders = new CVfdControler(_("Display settings"), NULL);
	
	// LCD
#if defined (ENABLE_LCD)
	// led_power
	lcdSettings->addItem(new CMenuOptionChooser(_("LED-Power"), &g_settings.lcd_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier));
	
	//option invert
	lcdSettings->addItem(new CMenuOptionChooser(_("Invert"), &g_settings.lcd_inverse, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier));

	//status display
	lcdSettings->addItem(new CMenuOptionChooser(_("Status line"), &g_settings.lcd_show_volume, LCDMENU_STATUSLINE_OPTIONS, LCDMENU_STATUSLINE_OPTION_COUNT, true));
	
	//lcd_epg
	lcdSettings->addItem(new CMenuOptionChooser(_("EPG"), &g_settings.lcd_epgmode, LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true));

	//align
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD EPG Align"), &g_settings.lcd_epgalign, LCDMENU_EPGALIGN_OPTIONS, LCDMENU_EPGALIGN_OPTION_COUNT, true));

	//dump to png
	//lcdSettings->addItem(new CMenuOptionChooser(_("output to PNG"), &g_settings.lcd_dump_png, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// dimm-time
	CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
	lcdSettings->addItem(new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, dim_time));

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, 15));

	// vfd controller
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// lcd controller
	lcdSettings->addItem(new CMenuForwarder(_("Contrast / Brightness"), true, NULL, lcdsliders));
#else
	// lcd_power
#if defined (PLATFORM_GIGABLUE)	
	lcdSettings->addItem(new CMenuOptionChooser(_("LED_Power"), &g_settings.lcd_power, LCDMENU_LEDCOLOR_OPTIONS, LCDMENU_LEDCOLOR_OPTION_COUNT, true, lcdnotifier));
#else
	lcdSettings->addItem(new CMenuOptionChooser(_("LED-Power"), &g_settings.lcd_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier));
#endif
	
	// epgmode
	lcdSettings->addItem(new CMenuOptionChooser(_("EPG"), &g_settings.lcd_epgmode, LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true));	

#if !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_SPARK) && !defined (PLATFORM_GIGABLUE)	
	// dimm-time
	CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
	lcdSettings->addItem(new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, dim_time));

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, 15));

	// vfd controller
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	lcdSettings->addItem(new CMenuForwarder(_("Contrast / Brightness"), true, NULL, lcdsliders));	
#endif	
#endif	
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete lcdSettings;
	lcdSettings = NULL;
	delete widget;
	widget = NULL;
}

// lcd notifier
bool CLcdNotifier::changeNotify(const std::string&, void * Data)
{
	int state = *(int *)Data;

	dprintf(DEBUG_NORMAL, "ClcdNotifier: state: %d\n", state);
		
	CVFD::getInstance()->setPower(state);	

	return true;
}


