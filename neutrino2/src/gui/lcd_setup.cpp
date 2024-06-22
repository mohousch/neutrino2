/*
	Neutrino-GUI  -   DBoxII-Project

	$id: lcd_setup.cpp 05062024 mohousch $
	
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
#include <gui/widget/hintbox.h>

#include <gui/lcdcontroler.h>
#include <gui/lcd_setup.h>

#include <system/debug.h>
#include <system/helpers.h>


#define OPTIONS_OFF0_ON1_OPTIONS_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTIONS_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define LCDMENU_STATUSLINE_OPTION_COUNT 2
const keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ CLCD::STATUSLINE_PLAYTIME, _("playtime")   },
	{ CLCD::STATUSLINE_VOLUME, _("volume")     }
//	{ CLCD::STATUSLINE_VOLUME_PLAYTIME, _("volume / playtime")       },
//	{ CLCD::STATUSLINE_VOLUME_PLAYTIME_AUDIO, _("volume / playtime / audio") }
};

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
#define LCDMENU_EPG_OPTION_COUNT 3
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ CLCD::EPGMODE_CHANNEL, _("channel") },
	{ CLCD::EPGMODE_TITLE, _("title")	},
	{ CLCD::EPGMODE_CHANNEL_TITLE, _("channel / title") },
//	{ CLCD::EPGMODE_CHANNEL_LINE_TITLE, _("channel / sep.-line / title") },
//	{ CLCD::EPGMODE_CHANNEL_SHORT_TITLE, _("channel (short) / title") },
//	{ CLCD::EPGMODE_CHANNEL_SHORT_LINE_TITLE, _("channel (short) / sep.-line / title") }
};
#elif defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
#define LCDMENU_EPG_OPTION_COUNT 2
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ CLCD::EPGMODE_CHANNELNUMBER, _("channel") },
	{ CLCD::EPGMODE_TIME, _("time") }
};
#endif

#define LCDMENU_EPGALIGN_OPTION_COUNT 2
const keyval LCDMENU_EPGALIGN_OPTIONS[LCDMENU_EPGALIGN_OPTION_COUNT] =
{
	{ CLCD::EPGALIGN_LEFT, _("left")   },
	{ CLCD::EPGALIGN_CENTER, _("center") }
};

#define LCDMENU_LEDCOLOR_OPTION_COUNT 4
const keyval LCDMENU_LEDCOLOR_OPTIONS[LCDMENU_LEDCOLOR_OPTION_COUNT] =
{
	{ CLCD::LEDCOLOR_OFF, _("off") 	},
	{ CLCD::LEDCOLOR_BLUE, _("blue") 	},
	{ CLCD::LEDCOLOR_RED, _("red") 	},
	{ CLCD::LEDCOLOR_PURPLE, _("purple") },
};

#define LCDMENU_MINITV_OPTION_COUNT 4
const keyval LCDMENU_MINITV_OPTIONS[LCDMENU_MINITV_OPTION_COUNT] =
{
	{ CLCD::MINITV_NORMAL, _("Normal") },
	{ CLCD::MINITV_TV, _("Mini TV") },
	{ CLCD::MINITV_OSD, _("OSD") },
	{ CLCD::MINITV_OSD_TV, _("OSD / TV") },
};

CLCDSettings::CLCDSettings()
{
#ifdef ENABLE_GRAPHLCD
//	nglcd = new nGLCD();
	item = NULL;
#endif
}

CLCDSettings::~CLCDSettings()
{
#ifdef ENABLE_GRAPHLCD
/*
	if (nglcd)
	{
		delete nglcd;
		nglcd = NULL;
	}
*/
#endif
}

int CLCDSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CLCDSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
		
	if (actionKey == "set_dimm_timeout")
	{
		CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
		
		dim_time->exec(NULL, "");
		
		m1->addOption(dim_time->getValueString().c_str());
		
		delete dim_time;
		dim_time = NULL;
		
		return ret;
	}
#ifdef ENABLE_GRAPHLCD	
	else if (actionKey == "select_driver")
	{
		int select = -1;
		
		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		CCHeaders *head = NULL;
		CCFooters *foot = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionstringchooser");
		
		if (widget)
		{
			menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
			head = (CCHeaders *)widget->getCCItem(CComponent::CC_HEAD);
			foot = (CCFooters *)widget->getCCItem(CComponent::CC_FOOT);
		}
		else
		{
			//
			widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			widget->name = "optionstringchooser";
			widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY + 50, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight - 100);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			
			//
			head = new CCHeaders(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, 50);
			
			//	
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		

			foot = new CCFooters(widget->getWindowsPos().iX, widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - 50, widget->getWindowsPos().iWidth, 50);
			foot->setButtons(&btn);
			
			//
			widget->addCCItem(menu);
			widget->addCCItem(head);
			widget->addCCItem(foot);
		}
		
		if (head)
			head->setTitle(_("Type"));
		
		for (int i = 0; i < nglcd->GetConfigSize(); i++)
		{
			bool selected = false;
			
			if (g_settings.glcd_selected_config == i)
				selected = true;
			
			menu->addItem(new CMenuForwarder(nglcd->GetConfigName(i).c_str(), true), selected);
		}
		
		widget->exec(NULL, "");
		ret = CMenuTarget::RETURN_REPAINT;

		select = menu->getSelected();
		
		if(select >= 0)
			g_settings.glcd_selected_config = select;
			
		if (widget)
		{
			delete widget;
			widget = NULL;
		}
		
		item->setOption(nglcd->GetConfigName(g_settings.glcd_selected_config).c_str());
		
		return ret;
	}
#endif
	
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
		lcdSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->addCCItem(lcdSettings);
	}
	
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Display settings"));
	
	// intros
	lcdSettings->addItem(new CMenuForwarder(_("back")));
	lcdSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	lcdSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	// lcd_power
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD Power"), &g_settings.lcd_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
	
	// led
#if defined (PLATFORM_GIGABLUE)	
	lcdSettings->addItem(new CMenuOptionChooser(_("LED Color"), &g_settings.lcd_led, LCDMENU_LEDCOLOR_OPTIONS, LCDMENU_LEDCOLOR_OPTION_COUNT, true, this));
#endif

	// minitv
	lcdSettings->addItem(new CMenuOptionChooser(_("Mini TV"), &g_settings.lcd_minitv, LCDMENU_MINITV_OPTIONS, LCDMENU_MINITV_OPTION_COUNT, true, this));
	
	// minitv fps
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Mini TV FPS"), &g_settings.lcd_minitvfps, true, 0, 30, this));

	//option invert
	lcdSettings->addItem(new CMenuOptionChooser(_("Invert"), &g_settings.lcd_inverse, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));

	//status display
	lcdSettings->addItem(new CMenuOptionChooser(_("Status line"), &g_settings.lcd_show_volume, LCDMENU_STATUSLINE_OPTIONS, LCDMENU_STATUSLINE_OPTION_COUNT, true));
	
	//lcd_epg
	lcdSettings->addItem(new CMenuOptionChooser(_("EPG"), &g_settings.lcd_epgmode, LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true));

	//align
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD EPG Align"), &g_settings.lcd_epgalign, LCDMENU_EPGALIGN_OPTIONS, LCDMENU_EPGALIGN_OPTION_COUNT, true));
	
	// dimm-time
	m1 = new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, this, "set_dimm_timeout");
	lcdSettings->addItem(m1);

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, DEFAULT_LCD_DIM_BRIGHTNESS, this, 0, -1, true));

	// brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness"), &g_settings.lcd_brightness, true, 0, DEFAULT_LCD_BRIGHTNESS, this, 0, -1, true));
	
	// standby brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Standby Brightness"), &g_settings.lcd_standbybrightness, true, 0, DEFAULT_LCD_STANDBYBRIGHTNESS, this, 0, -1, true));
	
	// deepstandby brightness
	// contrast
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Contrast"), &g_settings.lcd_contrast, true, 0, DEFAULT_LCD_CONTRAST, this, 0, -1, true));
	
#elif defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	// lcd_power
#if defined (PLATFORM_GIGABLUE)	
	lcdSettings->addItem(new CMenuOptionChooser(_("LED Color"), &g_settings.lcd_led, LCDMENU_LEDCOLOR_OPTIONS, LCDMENU_LEDCOLOR_OPTION_COUNT, true, this));
#else
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD Power"), &g_settings.lcd_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
#endif
	
	// epgmode
	lcdSettings->addItem(new CMenuOptionChooser(_("EPG"), &g_settings.lcd_epgmode, LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true));	

#if !defined (PLATFORM_CUBEREVO_250HD) && !defined (PLATFORM_SPARK) && !defined (PLATFORM_GIGABLUE)	
	// dimm-time
	CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
	lcdSettings->addItem(new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, dim_time));

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, DEFAULT_LCD_DIM_BRIGHTNESS, this, 0, -1, true));

	// brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness"), &g_settings.lcd_brightness, true, 0, DEFAULT_LCD_BRIGHTNESS, this, 0, -1, true));
	
	// standby brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Standby Brightness"), &g_settings.lcd_standbybrightness, true, 0, DEFAULT_LCD_STANDBYBRIGHTNESS, this, 0, -1, true));
	
	// deepstandby brightness
	// contrast
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Contrast"), &g_settings.lcd_contrast, true, 0, DEFAULT_LCD_CONTRAST, this, 0, -1, true));
#endif	
#endif

#ifdef ENABLE_GRAPHLCD	
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("GraphLCD Setup"))));
	
	// enable glcd
	lcdSettings->addItem(new CMenuOptionChooser(_("Enable NGLCD"), &g_settings.glcd_enable, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
	
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// select driver
	item = new CMenuForwarder(_("Type"), (nglcd->GetConfigSize() > 1), nglcd->GetConfigName(g_settings.glcd_selected_config).c_str(), this, "select_driver");
	
	lcdSettings->addItem(item);
#endif	
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

bool CLCDSettings::changeNotify(const std::string &locale, void *Data)
{
	int state = *(int *)Data;

	dprintf(DEBUG_NORMAL, "ClCDNotifier: %s state: %d\n", locale.c_str(), state);
	
	if (locale == _("LCD Power"))	
	{
		g_settings.lcd_power = state;
		CLCD::getInstance()->setPower(state);	
	}
	else if (locale == _("LED Color"))
	{
		g_settings.lcd_led = state;
		CLCD::getInstance()->setLED(state, state);
	}
	else if (locale == _("Mini TV"))
	{
		g_settings.lcd_minitv = state;
		CLCD::getInstance()->setMiniTV(state);
	}
	else if (locale == _("Mini TV FPS"))
	{
		g_settings.lcd_minitvfps = state;
		
		proc_put("/proc/stb/lcd/fps", state);
	}
	else if (locale == _("Brightness"))
	{
		g_settings.lcd_brightness = state;
		CLCD::getInstance()->setBrightness(g_settings.lcd_brightness);
	}
	else if (locale == _("Standby Brightness"))
	{
		g_settings.lcd_standbybrightness = state;
		CLCD::getInstance()->setBrightnessStandby(g_settings.lcd_standbybrightness);
	}
	else if (locale == _("Contrast"))
	{
		g_settings.lcd_contrast = state;
		CLCD::getInstance()->setContrast(g_settings.lcd_contrast);
	}
#ifdef ENABLE_GRAPHLCD
	else if (locale == "Type")
	{
		for (int i = 0; i < nglcd->GetConfigSize(); i++)
		{
			item->addOption(nglcd->GetConfigName(i).c_str(), i);
		}
	}
#endif
	
	return true;
}

