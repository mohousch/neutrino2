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
#include <gui/widget/messagebox.h>

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
};

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
#define LCDMENU_EPG_OPTION_COUNT 	3
const keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ CLCD::EPGMODE_CHANNEL, _("channel") },
	{ CLCD::EPGMODE_TITLE, _("title")	},
	{ CLCD::EPGMODE_CHANNEL_TITLE, _("channel / title") }
};
#endif

//#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
#define LCDMENU_MODE_OPTION_COUNT 	2
const keyval LCDMENU_MODE_OPTIONS[LCDMENU_MODE_OPTION_COUNT] =
{
	{ CLCD::MODE_CHANNEL_INFO, _("channel") },
	{ CLCD::MODE_TIME, _("time") }
};
//#endif

#define LCDMENU_EPGALIGN_OPTION_COUNT 2
const keyval LCDMENU_EPGALIGN_OPTIONS[LCDMENU_EPGALIGN_OPTION_COUNT] =
{
	{ CLCD::EPGALIGN_LEFT, _("left")   },
	{ CLCD::EPGALIGN_CENTER, _("center") }
};

#define LCDMENU_LEDCOLOR_OPTION_COUNT 	4
const keyval LCDMENU_LEDCOLOR_OPTIONS[LCDMENU_LEDCOLOR_OPTION_COUNT] =
{
	{ CLCD::LEDCOLOR_OFF, _("off") 	},
	{ CLCD::LEDCOLOR_BLUE, _("blue") 	},
	{ CLCD::LEDCOLOR_RED, _("red") 	},
	{ CLCD::LEDCOLOR_PURPLE, _("purple") },
};

#define LCDMENU_MINITV_OPTION_COUNT 	4
const keyval LCDMENU_MINITV_OPTIONS[LCDMENU_MINITV_OPTION_COUNT] =
{
	{ CLCD::MINITV_NORMAL, _("Normal") },
	{ CLCD::MINITV_TV, _("Mini TV") },
	{ CLCD::MINITV_OSD, _("OSD") },
	{ CLCD::MINITV_OSD_TV, _("OSD / TV") },
};

#define LCDMENU_STANDBY_CLOCK_OPTION_COUNT	2
const keyval LCDMENU_STANDBY_CLOCK_OPTIONS[LCDMENU_STANDBY_CLOCK_OPTION_COUNT] =
{
	{ CLCD::STANDBYCLOCK_DIGITAL, _("Digital") },
	{ CLCD::STANDBYCLOCK_ANALOG, _("Analog") }
};

CLCDSettings::CLCDSettings()
{
	widget = NULL;
	lcdSettings = NULL;
	selected = -1;
	
#ifdef ENABLE_GRAPHLCD
	item = NULL;
	item1 = NULL;
	item2 = NULL;
#endif
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
		
	if (actionKey == "set_dimm_timeout")
	{
		CStringInput * dim_time = new CStringInput(_("Dim timeout"), g_settings.lcd_setting_dim_time, 3, NULL, NULL, "0123456789 ");
		
		dim_time->exec(NULL, "");
		
		m1->addOption(dim_time->getValueString().c_str());
		
		delete dim_time;
		dim_time = NULL;
		
		return ret;
	}
	else if (actionKey == "reset")
	{
		g_settings.lcd_brightness = DEFAULT_LCD_BRIGHTNESS;
		g_settings.lcd_standbybrightness = DEFAULT_LCD_STANDBYBRIGHTNESS;
		g_settings.lcd_contrast = DEFAULT_LCD_CONTRAST;
		g_settings.lcd_setting_dim_brightness = DEFAULT_LCD_DIM_BRIGHTNESS;
		
		CLCD::getInstance()->setBrightness(g_settings.lcd_brightness);
		CLCD::getInstance()->setBrightnessStandby(g_settings.lcd_standbybrightness);
		CLCD::getInstance()->setContrast(g_settings.lcd_contrast);
		
		selected = lcdSettings->getSelected();
		
		showMenu();
	
		return RETURN_EXIT;
	}
#ifdef ENABLE_GRAPHLCD	
	else if (actionKey == "select_driver")
	{
		int old_glcd_selected = g_settings.glcd_selected_config;
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
			CBox box;
			box.iWidth = MENU_WIDTH;
			box.iHeight = MENU_HEIGHT;
			box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
			box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
			widget = new CWidget(&box);
			widget->name = "optionstringchooser";
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(box.iX, box.iY + 50, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight - 100);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			
			//
			head = new CCHeaders(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, 50);
			
			//	
			const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };		

			foot = new CCFooters(widget->getWindowsPos().iX, widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - 50, widget->getWindowsPos().iWidth, 50);
			foot->setButtons(&btn);
			
			//
			widget->addCCItem(menu);
			widget->addCCItem(head);
			widget->addCCItem(foot);
		}
		
		if (head)
			head->setTitle(_("Type"));
		
		for (int i = 0; i < CLCD::getInstance()->GetConfigSize(); i++)
		{
			bool selected = false;
			
			if (g_settings.glcd_selected_config == i)
				selected = true;
			
			menu->addItem(new CMenuForwarder(CLCD::getInstance()->GetConfigName(i).c_str(), true), selected);
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
		
		if (old_glcd_selected != g_settings.glcd_selected_config)
		{
			if (MessageBox(_("Information"), _("this need GUI restart\ndo you really want to restart?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == CMessageBox::mbrYes) 
			{
				CNeutrinoApp::getInstance()->exec(NULL, "restart");
			}
			else
			{
				g_settings.glcd_selected_config = old_glcd_selected;
			}
		}
		
		item->setOption(CLCD::getInstance()->GetConfigName(g_settings.glcd_selected_config).c_str());
		
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
	widget = CNeutrinoApp::getInstance()->getWidget("lcdsetup");
	
	if (widget)
	{
		lcdSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "lcdsetup";
		
		//
		lcdSettings = new ClistBox(&box);

		lcdSettings->setWidgetMode(ClistBox::MODE_SETUP);
		lcdSettings->enableShrinkMenu();
		
		lcdSettings->enablePaintHead();
		lcdSettings->setTitle(_("Display settings"), NEUTRINO_ICON_LCD);

		lcdSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		lcdSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(lcdSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Display settings"));
	
	// intros
	lcdSettings->addItem(new CMenuForwarder(_("back")));
	lcdSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	lcdSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// lcd_power
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD Power"), &g_settings.lcd_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
#endif

	// mode
	lcdSettings->addItem(new CMenuOptionChooser(_("Mode"), &g_settings.lcd_mode, LCDMENU_MODE_OPTIONS, LCDMENU_MODE_OPTION_COUNT, true));
	
	// led_color
#if defined (PLATFORM_GIGABLUE)	
	lcdSettings->addItem(new CMenuOptionChooser(_("LED Color"), &g_settings.lcd_led, LCDMENU_LEDCOLOR_OPTIONS, LCDMENU_LEDCOLOR_OPTION_COUNT, true, this));
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	// minitv
//	lcdSettings->addItem(new CMenuOptionChooser(_("Mini TV"), &g_settings.lcd_minitv, LCDMENU_MINITV_OPTIONS, LCDMENU_MINITV_OPTION_COUNT, true, this, CRCInput::RC_nokey, NULL, true));
	
	// minitv fps
//	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Mini TV FPS"), &g_settings.lcd_minitvfps, true, 0, 30, this));

	// invert
//	lcdSettings->addItem(new CMenuOptionChooser(_("Invert"), &g_settings.lcd_inverse, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	// statusline
	lcdSettings->addItem(new CMenuOptionChooser(_("Status line"), &g_settings.lcd_statusline, LCDMENU_STATUSLINE_OPTIONS, LCDMENU_STATUSLINE_OPTION_COUNT, true));
	
	// lcd_epg
	lcdSettings->addItem(new CMenuOptionChooser(_("EPG"), &g_settings.lcd_epgmode, LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true));

	// align
	lcdSettings->addItem(new CMenuOptionChooser(_("LCD EPG Align"), &g_settings.lcd_epgalign, LCDMENU_EPGALIGN_OPTIONS, LCDMENU_EPGALIGN_OPTION_COUNT, true));
	
	// logo
	lcdSettings->addItem(new CMenuOptionChooser(_("Show picon"), &g_settings.lcd_picon, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
	
	// weather
	lcdSettings->addItem(new CMenuOptionChooser(_("Show weather"), &g_settings.lcd_weather, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));

	// standby_clock
	lcdSettings->addItem(new CMenuOptionChooser(_("Standby Clock"), &g_settings.lcd_standby_clock, LCDMENU_STANDBY_CLOCK_OPTIONS, LCDMENU_STANDBY_CLOCK_OPTION_COUNT, true, this, CRCInput::RC_nokey, NULL, false, false));
#endif

#ifdef ENABLE_4DIGITS
	// dimm-time
	m1 = new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, this, "set_dimm_timeout");
	lcdSettings->addItem(m1);

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, MAXBRIGHTNESS, this));

	// brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness"), &g_settings.lcd_brightness, true, 0, MAXBRIGHTNESS, this));
	
	// standby brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Standby Brightness"), &g_settings.lcd_standbybrightness, true, 0, MAXBRIGHTNESS, this));
	
	// reset brightness / contrast to default
	lcdSettings->addItem(new CMenuForwarder(_("Reset to defaults"), true, NULL, this, "reset"));
#elif defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_VFD)
	// dimm-time
	m1 = new CMenuForwarder(_("Dim timeout"), true, g_settings.lcd_setting_dim_time, this, "set_dimm_timeout");
	lcdSettings->addItem(m1);

	// dimm brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness after dim timeout"), &g_settings.lcd_setting_dim_brightness, true, 0, MAXBRIGHTNESS, this));

	// brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Brightness"), &g_settings.lcd_brightness, true, 0, MAXBRIGHTNESS, this));
	
	// standby brightness
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Standby Brightness"), &g_settings.lcd_standbybrightness, true, 0, MAXBRIGHTNESS, this));
	
	// contrast
	lcdSettings->addItem(new CMenuOptionNumberChooser(_("Contrast"), &g_settings.lcd_contrast, true, 0, MAXCONTRAST, this));
	
	// reset brightness / contrast to default
	lcdSettings->addItem(new CMenuForwarder(_("Reset to defaults"), true, NULL, this, "reset"));
#endif	

#ifdef ENABLE_GRAPHLCD	
	lcdSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// enable glcd
	lcdSettings->addItem(new CMenuOptionChooser(_("Enable NGLCD"), &g_settings.glcd_enable, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTIONS_COUNT, true, this, CRCInput::RC_nokey, NULL, false, true));
	
	// select driver
	item = new CMenuForwarder(_("Type"), (CLCD::getInstance()->GetConfigSize() > 1), CLCD::getInstance()->GetConfigName(g_settings.glcd_selected_config).c_str(), this, "select_driver");
	
	if (g_settings.glcd_enable)
	{
		item->setState(CMenuItem::ITEM_ACTIVE);
	}
	else
	{
		item->setState(CMenuItem::ITEM_INACTIVE);
	}
	
	lcdSettings->addItem(item);
	
	// glcd_brightness
	item1 = new CMenuOptionNumberChooser(_("GLCD Brightness"), &g_settings.glcd_brightness, true, 0, 255, this);
	if (g_settings.glcd_enable)
	{
		item1->setState(CMenuItem::ITEM_ACTIVE);
	}
	else
	{
		item1->setState(CMenuItem::ITEM_INACTIVE);
	}
	lcdSettings->addItem(item1);
	
	// glcd_standby brightness
	item2 = new CMenuOptionNumberChooser(_("GLCD Standby Brightness"), &g_settings.glcd_brightness_standby, true, 0, 255, this);
	if (g_settings.glcd_enable)
	{
		item2->setState(CMenuItem::ITEM_ACTIVE);
	}
	else
	{
		item2->setState(CMenuItem::ITEM_INACTIVE);
	}
	lcdSettings->addItem(item2);
#endif

	lcdSettings->setSelected(selected);	
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

bool CLCDSettings::changeNotify(const std::string &locale, void *Data)
{
	int state = *(int *)Data;

	dprintf(DEBUG_NORMAL, "%s state: %d\n", locale.c_str(), state);
	
	if (locale == _("LCD Power"))	
	{
		g_settings.lcd_power = state;
		CLCD::getInstance()->setPower(state);	
	}
	else if (locale == _("Mode"))
	{
		g_settings.lcd_mode = state;
		CLCD::getInstance()->Clear();
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
		for (int i = 0; i < CLCD::getInstance()->GetConfigSize(); i++)
		{
			item->addOption(CLCD::getInstance()->GetConfigName(i).c_str(), i);
		}
	}
	else if (locale == _("GLCD Brightness"))
	{
		g_settings.glcd_brightness = state;
		CLCD::getInstance()->setGLCDBrightness(g_settings.glcd_brightness);
	}
	else if (locale == _("GLCD Standby Brightness"))
	{
		g_settings.glcd_brightness_standby = state;
		CLCD::getInstance()->setGLCDBrightness(g_settings.glcd_brightness_standby);
	}
	else if (locale == _("Enable NGLCD"))
	{
		g_settings.glcd_enable = state;
		
		if (MessageBox(_("Information"), _("changes are activ after restart\ndo you want to restart?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == CMessageBox::mbrYes) 
		{
			CNeutrinoApp::getInstance()->exec(NULL, "restart");
		}
		
		if (g_settings.glcd_enable)
		{
			item->setState(CMenuItem::ITEM_ACTIVE);
			item1->setState(CMenuItem::ITEM_ACTIVE);
			item2->setState(CMenuItem::ITEM_ACTIVE);
		}
		else
		{
			item->setState(CMenuItem::ITEM_INACTIVE);
			item1->setState(CMenuItem::ITEM_INACTIVE);
			item2->setState(CMenuItem::ITEM_INACTIVE);
		}
	}
#endif
	
	return true;
}

