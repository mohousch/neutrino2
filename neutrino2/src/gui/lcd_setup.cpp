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

#if defined (ENABLE_LCD)
#define LCDMENU_STATUSLINE_OPTION_COUNT 4
const keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ 0, _("Playtime")   },
	{ 1, _("Volume")     },
	{ 2, _("Volume / Playtime")       },
	{ 3, _("Volume / Playtime / Audio") }
};

#define LCDMENU_EPG_OPTION_COUNT 6
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ 1, _("Default (Channel)")		},
	{ 2, _("Title")		},
	{ 3, _("Channel / Title")	},
	{ 7, _("Channel / Sep.-Line / Title") },
	{ 11, _("Channel (short) / Title") },
	{ 15, _("Channel (short) / Sep.-Line / Title") }
};

#define LCDMENU_EPGALIGN_OPTION_COUNT 2
const keyval LCDMENU_EPGALIGN_OPTIONS[LCDMENU_EPGALIGN_OPTION_COUNT] =
{
	{ 0, _("left")   },
	{ 1, _("center") }
};
#endif

#if defined (PLATFORM_GIGABLUE) 
#if !defined (ENABLE_LCD)
#define LCDMENU_LEDCOLOR_OPTION_COUNT 4
const keyval LCDMENU_LEDCOLOR_OPTIONS[LCDMENU_LEDCOLOR_OPTION_COUNT] =
{
	{ CVFD::LED_OFF, _("off") },
	{ CVFD::LED_BLUE, _("blue") },
	{ CVFD::LED_RED, _("red") },
	{ CVFD::LED_PURPLE, _("purple") },
};
#endif
#endif

CLCDSettings::CLCDSettings()
{
}

CLCDSettings::~CLCDSettings()
{
}

int CLCDSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CLCDSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	
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
		lcdSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		lcdSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		lcdSettings->setWidgetMode(MODE_SETUP);
		lcdSettings->enableShrinkMenu();
		
		lcdSettings->enablePaintHead();
		lcdSettings->setTitle(_("Display settings"), NEUTRINO_ICON_LCD);

		lcdSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		lcdSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "lcdsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(lcdSettings);
	}
	
	lcdSettings->clearItems();
	
	// intros
	lcdSettings->addItem(new ClistBoxItem(_("back")));
	lcdSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	lcdSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	lcdSettings->addItem(new CMenuSeparator(LINE));
	
	CLcdNotifier * lcdnotifier = new CLcdNotifier();
	
	CVfdControler * lcdsliders = new CVfdControler(_("Display settings"), NULL);
	
	// LCD
#if defined (ENABLE_LCD)
	//option invert
	CMenuOptionChooser* oj_inverse = new CMenuOptionChooser(_("Invert"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
	lcdSettings->addItem(oj_inverse);

	//status display
	CMenuOptionChooser* oj_status = new CMenuOptionChooser(_("Status line"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME], LCDMENU_STATUSLINE_OPTIONS, LCDMENU_STATUSLINE_OPTION_COUNT, true);
	lcdSettings->addItem(oj_status);
	
	//lcd_epg
	CMenuOptionChooser* oj_epg = new CMenuOptionChooser(_("EPG"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE], LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true);
	lcdSettings->addItem(oj_epg);

	//align
	CMenuOptionChooser* oj_align = new CMenuOptionChooser(_("LCD EPG Align"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGALIGN], LCDMENU_EPGALIGN_OPTIONS, LCDMENU_EPGALIGN_OPTION_COUNT, true);
	lcdSettings->addItem(oj_align);

	//dump to png
	CMenuOptionChooser* oj_dumppng = new CMenuOptionChooser(_("output to PNG"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_DUMP_PNG], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	lcdSettings->addItem(oj_dumppng);
	
	// lcd controller
	lcdSettings->addItem(new ClistBoxItem(_("Contrast / Brightness"), true, NULL, lcdsliders));
#else	
#if defined (PLATFORM_GIGABLUE)	
	// led color
	lcdSettings->addItem(new CMenuOptionChooser(_("Led Color"), &g_settings.lcd_ledcolor, LCDMENU_LEDCOLOR_OPTIONS, LCDMENU_LEDCOLOR_OPTION_COUNT, true, lcdnotifier));	
#elif !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_SPARK)
	// vfd power
	CMenuOptionChooser * oj2 = new CMenuOptionChooser(_("LED-Power"), &g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
	lcdSettings->addItem(oj2);
	
	// dimm-time
	CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
	lcdSettings->addItem(new ClistBoxItem(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, dim_time));

	// dimm brightness
	//CStringInput * dim_brightness = new CStringInput(_("brightness after dim timeout"), g_settings.lcd_setting_dim_brightness, 3, NULL, NULL, "0123456789 ");
	//lcdSettings->addItem(new ClistBoxItem(_("Brightness after dim timeout"), true, g_settings.lcd_setting_dim_brightness, dim_brightness));

	// vfd controller
	lcdSettings->addItem(new CMenuSeparator(LINE));
	lcdSettings->addItem(new ClistBoxItem(_("Contrast / Brightness"), true, NULL, lcdsliders));	
#endif	
#endif	
	
	//
	widget->exec(NULL, "");
}

// lcd notifier
bool CLcdNotifier::changeNotify(const std::string&, void * Data)
{
	int state = *(int *)Data;

	dprintf(DEBUG_NORMAL, "ClcdNotifier: state: %d\n", state);
	
#if defined (PLATFORM_GIGABLUE) && !defined (ENABLE_LCD)
	CVFD::getInstance()->vfd_led(state);
#else	
	CVFD::getInstance()->setPower(state);
	CVFD::getInstance()->setlcdparameter();
#endif	

	return true;
}


