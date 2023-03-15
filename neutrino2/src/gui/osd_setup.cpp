/*
	Neutrino-GUI  -   DBoxII-Project

	$id: osd_setup.cpp 21.09.21 mohousch $
	
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
 
#include <sys/stat.h>
#include <dirent.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>

#include <gui/widget/colorchooser.h>
#include <gui/widget/stringinput.h>

#include <gui/filebrowser.h>
#include <gui/osd_setup.h>
#include <gui/themes.h>
#include <gui/screensetup.h>
#include <gui/alphasetup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>


#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

// osd settings
COSDSettings::COSDSettings()
{
}

COSDSettings::~COSDSettings()
{
}

int COSDSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();

	showMenu();
	
	return ret;
}

// showmenu
void COSDSettings::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "COSDSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* osdSettings = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("osd");
	
	if (widget == NULL)
	{
		osdSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
		osdSettings->setWidgetMode(MODE_MENU);
		osdSettings->setWidgetType(TYPE_CLASSIC);
		osdSettings->enableShrinkMenu();
		
		//
		osdSettings->enablePaintHead();
		osdSettings->setTitle(_("OSD"), NEUTRINO_ICON_COLORS);
		osdSettings->enablePaintDate();
		
		//
		osdSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		osdSettings->setFootButtons(&btn); 
	
		// skin manager
		osdSettings->addItem( new CMenuForwarder(_("Skin select"), true, NULL, new CSkinManager(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_THEMES));
		
		// skin style
		osdSettings->addItem(new CMenuForwarder(_("Skin Style"), true, NULL, new CSkinSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDSETTINGS));

		// Themes
		osdSettings->addItem( new CMenuForwarder(_("Themes"), true, NULL, new CThemes(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_THEMES));

		// menu colors
		osdSettings->addItem( new CMenuForwarder(_("Menu"), true, NULL, new COSDMenuColorSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_MENUCOLORS));

		// infobar
		osdSettings->addItem( new CMenuForwarder(_("Infobar"), true, NULL, new COSDInfoBarColorSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_INFOBARCOLORS));

		// language
		osdSettings->addItem(new CMenuForwarder(_("Language"), true, NULL, new CLanguageSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_LANGUAGE));
		
		// font
		osdSettings->addItem(new CMenuForwarder(_("Font"), true, NULL, new CFontSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FONT));
		
		// osd timing
		osdSettings->addItem(new CMenuForwarder(_("Timing"), true, NULL, new COSDTimingSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDTIMING));

		// sceensetup
		osdSettings->addItem(new CMenuForwarder(_("Screen"), true, NULL, new CScreenSetup(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SCREENSETUP));
		
		// alpha setup
		//FIXME:
		//CAlphaSetup * chAlphaSetup = new CAlphaSetup(_("Alpha"), &g_settings.gtx_alpha);
		//osdSettings->addItem( new CMenuForwarder(_("Alpha Setup"), true, NULL, chAlphaSetup, NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_ALPHASETUP));
		
		// diverses
		osdSettings->addItem(new CMenuForwarder(_("Misc settings"), true, NULL, new COSDDiverses(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDSETTINGS));
		
		// skin style
		//osdSettings->addItem(new CMenuForwarder(_("Skin Style"), true, NULL, new CSkinSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDSETTINGS));
		
		//
		widget = new CWidget(osdSettings->getWindowsPos().iX, osdSettings->getWindowsPos().iY, osdSettings->getWindowsPos().iWidth, osdSettings->getWindowsPos().iHeight);
		widget->name = "osd";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(osdSettings);
	}
	
	//
	widget->exec(NULL, "");
}

// osd menucolor settings
#define COLOR_GRADIENT_TYPE_OPTION_COUNT	5
const keyval COLOR_GRADIENT_TYPE_OPTIONS[COLOR_GRADIENT_TYPE_OPTION_COUNT] =
{
	{ NOGRADIENT, _("no gradient") },
	{ DARK2LIGHT, _("Dark to Light") },
	{ LIGHT2DARK, _("Light to Dark") },
	{ DARK2LIGHT2DARK, _("Dark to Light to Dark") },
	{ LIGHT2DARK2LIGHT, _("Light to Dark to Light") }
};

#define GRADIENT_DIRECTION_TYPE_OPTION_COUNT	2
const keyval GRADIENT_DIRECTION_TYPE_OPTIONS[GRADIENT_DIRECTION_TYPE_OPTION_COUNT] =
{
	{ GRADIENT_HORIZONTAL, _("Horizontal") },
	{ GRADIENT_VERTICAL, _("Vertical") }
};

#define CORNER_TYPE_OPTION_COUNT	10
const keyval CORNER_TYPE_OPTIONS[CORNER_TYPE_OPTION_COUNT] =
{
	{ CORNER_NONE, _("CORNER NONE") },
	{ CORNER_TOP_LEFT, _("TOP LEFT") },
	{ CORNER_TOP_RIGHT, _("TOP RIGHT") },
	{ CORNER_TOP, _("TOP") },
	{ CORNER_BOTTOM_RIGHT, _("BOTTOM RIGHT") },
	{ CORNER_RIGHT, _("RIGHT") },
	{ CORNER_BOTTOM_LEFT, _("BOTTOM LEFT") },
	{ CORNER_LEFT, _("LEFT") },
	{ CORNER_BOTTOM, _("BOTTOM") },
	{ CORNER_ALL, _("CORNER ALL") },
};

#define RADIUS_TYPE_OPTION_COUNT	5
const keyval RADIUS_TYPE_OPTIONS[RADIUS_TYPE_OPTION_COUNT] =
{
	{ NO_RADIUS, _("angular") },
	{ RADIUS_SMALL, _("small") },
	{ RADIUS_MID, _("middle") },
	{ RADIUS_LARGE, _("large") },
	{ RADIUS_VERYLARGE, _("very large") }
};

COSDMenuColorSettings::COSDMenuColorSettings()
{
}

COSDMenuColorSettings::~COSDMenuColorSettings()
{
}

int COSDMenuColorSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDMenuColorSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
		//HintBox(_("Information"), _("Saving settings now, please be patient."));
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void COSDMenuColorSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "COSDMenuColorSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* OSDmenuColorsSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("menusetup");
	
	if (widget)
	{
		OSDmenuColorsSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		OSDmenuColorsSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		OSDmenuColorsSettings->setWidgetMode(MODE_SETUP);
		OSDmenuColorsSettings->enableShrinkMenu();
		
		//
		OSDmenuColorsSettings->enablePaintHead();
		OSDmenuColorsSettings->setTitle(_("Menu"), NEUTRINO_ICON_COLORS);

		//
		OSDmenuColorsSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		OSDmenuColorsSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "menusetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(OSDmenuColorsSettings);
	}
	
	OSDmenuColorsSettings->clearItems();
	
	// intros
	OSDmenuColorsSettings->addItem(new CMenuForwarder(_("back")));

	// save settings
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE) );
	OSDmenuColorsSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));

	// head
	CColorChooser* chHeadcolor = new CColorChooser(_("Background"), &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue, &g_settings.menu_Head_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser* chHeadTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);

	// window content
	CColorChooser* chContentcolor = new CColorChooser(_("Background"), &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,&g_settings.menu_Content_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser* chContentTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);

	// window content inactive
	CColorChooser* chContentInactivecolor = new CColorChooser(_("Background"), &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue,&g_settings.menu_Content_inactive_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser* chContentInactiveTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);
	
	// window content selected
	CColorChooser* chContentSelectedcolor = new CColorChooser(_("Background"), &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,&g_settings.menu_Content_Selected_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser* chContentSelectedTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);
	
	// foot
	CColorChooser* chFootcolor = new CColorChooser(_("Background"), &g_settings.menu_Foot_red, &g_settings.menu_Foot_green, &g_settings.menu_Foot_blue, &g_settings.menu_Foot_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser * chFootTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Foot_Text_red, &g_settings.menu_Foot_Text_green, &g_settings.menu_Foot_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);

	// itemInfo
	CColorChooser* chHintColor = new CColorChooser(_("Background"), &g_settings.menu_Hint_red, &g_settings.menu_Hint_green, &g_settings.menu_Hint_blue, &g_settings.menu_Hint_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	CColorChooser * chHintTextColor = new CColorChooser(_("Textcolor"), &g_settings.menu_Hint_Text_red, &g_settings.menu_Hint_Text_green, &g_settings.menu_Hint_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);

	// head
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Head")));
	
	// head colr
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chHeadcolor ));
	
	// head text
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chHeadTextcolor ));

	// head gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Head_gradient, COLOR_GRADIENT_TYPE_OPTIONS, COLOR_GRADIENT_TYPE_OPTION_COUNT, true));
	
	// head corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Head_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// head radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Head_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// head line
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line"), &g_settings.Head_line, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// window content
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Window-Content")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentcolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentTextcolor ));

	// window content inactiv
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Window-Content inactive")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentInactivecolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentInactiveTextcolor));

	// window content selected
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Window-Content selected")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentSelectedcolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentSelectedTextcolor ));
	
	// foot
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Foot")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chFootcolor ));

	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chFootTextcolor ));

	// foot gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Foot_gradient, COLOR_GRADIENT_TYPE_OPTIONS, COLOR_GRADIENT_TYPE_OPTION_COUNT, true));
	
	// foot corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Foot_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// foot radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Foot_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// foot line
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line"), &g_settings.Foot_line, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// itemInfo
	OSDmenuColorsSettings->addItem( new CMenuSeparator(LINE | STRING, _("Item Info")));
	
	// itemInfocolor
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chHintColor ));

	// fontinfo text color
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chHintTextColor ));

	// itemInfo gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Hint_gradient, COLOR_GRADIENT_TYPE_OPTIONS, COLOR_GRADIENT_TYPE_OPTION_COUNT, true));
	
	// iteminfo border
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Border"), &g_settings.Hint_border, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// itemInfo Radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Hint_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// itemInfo corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Hint_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	//
	widget->exec(NULL, "");
}

// osd infobarcolor settings
COSDInfoBarColorSettings::COSDInfoBarColorSettings()
{
}

COSDInfoBarColorSettings::~COSDInfoBarColorSettings()
{
}

int COSDInfoBarColorSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDInfoBarColorSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
		//HintBox(_("Information"), _("Saving settings now, please be patient."));
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void COSDInfoBarColorSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "COSDInfoBarColorSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* OSDinfobarColorSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("infobarsetup");
	
	if (widget)
	{
		OSDinfobarColorSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		OSDinfobarColorSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		OSDinfobarColorSettings->setWidgetMode(MODE_SETUP);
		OSDinfobarColorSettings->enableShrinkMenu();
		
		OSDinfobarColorSettings->enablePaintHead();
		OSDinfobarColorSettings->setTitle(_("Infobar"), NEUTRINO_ICON_COLORS);

		OSDinfobarColorSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		OSDinfobarColorSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "infobarsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(OSDinfobarColorSettings);
	}
	
	OSDinfobarColorSettings->clearItems();
	
	// intros
	OSDinfobarColorSettings->addItem(new CMenuForwarder(_("back")));

	OSDinfobarColorSettings->addItem(new CMenuSeparator(LINE));
	OSDinfobarColorSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));

	// bg
	CColorChooser * chInfobarcolor = new CColorChooser(_("Background"), &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,&g_settings.infobar_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);
	
	// text
	CColorChooser * chInfobarTextcolor = new CColorChooser(_("Textcolor"), &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);
	
	// colored events
	CColorChooser * chColored_Events = new CColorChooser(_("Events Textcolor"), &g_settings.infobar_colored_events_red, &g_settings.infobar_colored_events_green, &g_settings.infobar_colored_events_blue, NULL, CNeutrinoApp::getInstance()->colorSetupNotifier);

	OSDinfobarColorSettings->addItem( new CMenuSeparator(LINE));

	// bg color
	OSDinfobarColorSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chInfobarcolor ));

	// text color
	OSDinfobarColorSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chInfobarTextcolor ));

	// events text color
	OSDinfobarColorSettings->addItem( new CMenuForwarder(_("Events Textcolor"), true, NULL, chColored_Events ));
	
	// infobar border
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Border"), &g_settings.infobar_border, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	// infobar buttonbar
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Buttons Bar"), &g_settings.infobar_buttonbar, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	// infobar buttonline
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Buttons Line"), &g_settings.infobar_buttonline, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));

	// gradient
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.infobar_gradient, COLOR_GRADIENT_TYPE_OPTIONS, COLOR_GRADIENT_TYPE_OPTION_COUNT, true));
	
	// gradient direction
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Gradient direction"), &g_settings.infobar_gradient_direction, GRADIENT_DIRECTION_TYPE_OPTIONS, GRADIENT_DIRECTION_TYPE_OPTION_COUNT, true));
	
	// corner
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.infobar_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// radius
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.infobar_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	//
	OSDinfobarColorSettings->addItem( new CMenuSeparator(LINE));
	
	// sig/snr
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Satellite display on infobar"), &g_settings.infobar_sat_display, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	//
	widget->exec(NULL, "");
}

// osd language settings
CLanguageSettings::CLanguageSettings(bool wizzard)
{
	fromStartWizzard = wizzard;
}

CLanguageSettings::~CLanguageSettings()
{
}

int CLanguageSettings::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CLanguageSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();

	if (!actionKey.empty() && actionKey != g_settings.language)
	{
		strcpy(g_settings.language, actionKey.c_str());
		g_Locale->loadLocale(Lang2I18N(g_settings.language).c_str());
		
		if (!fromStartWizzard)
		{
			if (MessageBox(_("Information"), _("this need Neutrino restart\ndo you really want to restart?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
			{
				CNeutrinoApp::getInstance()->exec(NULL, "restart");
			}
		}
		
		return RETURN_EXIT;
	}
	
	showMenu();
	
	return ret;
}

void CLanguageSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CLanguageSettings::showMenu:\n");
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	ClistBox* languageSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("languagesetup");
	
	if (widget)
	{
		languageSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		languageSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		languageSettings->setWidgetMode(MODE_SETUP);
		languageSettings->enableShrinkMenu();
		
		languageSettings->enablePaintHead();
		languageSettings->setTitle(_("Language"), NEUTRINO_ICON_LANGUAGE);

		languageSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		languageSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "languagesetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(languageSettings);
	}
	
	languageSettings->clearItems();
	
	// intros
	languageSettings->addItem(new CMenuForwarder(_("back")));
	languageSettings->addItem(new CMenuSeparator(LINE));

	languageSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	languageSettings->addItem(new CMenuSeparator(LINE));
	
	item = new ClistBoxItem(_("English"), true, NULL, this, "en");
	item->setIconName("en");
	
	if (strcmp(g_settings.language, "en") == 0)
	{
		item->setIcon1(NEUTRINO_ICON_MARK);
		//item->setMarked(true);
	}
					
	languageSettings->addItem(item);

	struct dirent **namelist;
	int n;

	n = scandir(LOCALEDIR, &namelist, 0, alphasort);
		
	if(n > 0)
	{
		while(n--)
		{
			if(namelist[n]->d_type == DT_DIR && !strstr(namelist[n]->d_name, ".") && !strstr(namelist[n]->d_name, ".."))
			{	
				item = new ClistBoxItem(_(locale2lang(namelist[n]->d_name).c_str()), true, NULL, this, namelist[n]->d_name);
				item->setIconName(namelist[n]->d_name);
					
				if (strcmp(g_settings.language, namelist[n]->d_name) == 0)
				{
					item->setIcon1(NEUTRINO_ICON_MARK);
					//item->setMarked(true);
				}
					
				languageSettings->addItem(item);	
			}
			free(namelist[n]);
		}
		free(namelist);
	}
	
	//
	widget->exec(NULL, "");
}

bool CLanguageSettings::changeNotify(const std::string& OptionName, void */*data*/)
{
	if (OptionName == _("Select language"))
	{
		dprintf(DEBUG_NORMAL, "CLanguageSettings::changeNotify: %s\n", g_settings.language);
		
		g_Locale->loadLocale(Lang2I18N(g_settings.language).c_str());

		return true;
	}
	
	return false;
}


// CFontSettings
CFontSettings::CFontSettings()
{
}

CFontSettings::~CFontSettings()
{
}

int CFontSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CFontSettings::exec: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->setupFonts(g_settings.font_file);
		
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
		//HintBox(_("Information"), _("Saving settings now, please be patient."));
		
		return ret;
	}
	else if(actionKey == "select_font")
	{
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("ttf");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec(DATADIR "/fonts") == true)
		{
			strcpy(g_settings.font_file, fileBrowser.getSelectedFile()->Name.c_str());
			dprintf(DEBUG_NORMAL, "COSDSettings::exec: new font file %s\n", fileBrowser.getSelectedFile()->Name.c_str());
			
			CNeutrinoApp::getInstance()->setupFonts(g_settings.font_file);
			CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		}
		
		getString() = g_settings.font_file;
		
		return ret;
	}
		
	showMenu();
	
	return ret;
}

void CFontSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CFontSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* fontSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("fontsetup");
	
	if (widget)
	{
		fontSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		fontSettings = new ClistBox(0, 0, 700, MENU_HEIGHT);

		fontSettings->setWidgetMode(MODE_SETUP);
		fontSettings->enableShrinkMenu();
		
		fontSettings->enablePaintHead();
		fontSettings->setTitle(_("Font"), NEUTRINO_ICON_COLORS);

		fontSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		fontSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, 700, MENU_HEIGHT);
		widget->name = "fontsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(fontSettings);
	}
	
	fontSettings->clearItems();
	
	// intros
	fontSettings->addItem(new CMenuForwarder(_("back")));
	fontSettings->addItem(new CMenuSeparator(LINE));

	fontSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	fontSettings->addItem(new CMenuSeparator(LINE));
	
	// font name
	fontSettings->addItem(new CMenuForwarder(_("Font name"), true, g_settings.font_file, this, "select_font"));
	
	// font scaling
	fontSettings->addItem(new CMenuSeparator(LINE|STRING, _("Font")));
	fontSettings->addItem(new CMenuOptionNumberChooser(_("Horizontal (in %)"), &g_settings.screen_xres, true, 50, 200, NULL) );
	fontSettings->addItem(new CMenuOptionNumberChooser(_("Vertikal (in %)"), &g_settings.screen_yres, true, 50, 200, NULL) );
		
	//
	widget->exec(NULL, "");
}

// osd timing settings
static CTimingSettingsNotifier timingsettingsnotifier;

int COSDTimingSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDTimingSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "osd.def") 
	{
		for (int i = 0; i < TIMING_SETTING_COUNT; i++)
			g_settings.timing[i] = default_timing[i];

		CNeutrinoApp::getInstance()->setupTiming();
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void COSDTimingSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "COSDTimingSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* osdTimingSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("osdtimingsetup");
	
	if (widget)
	{
		osdTimingSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		osdTimingSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		osdTimingSettings->setWidgetMode(MODE_SETUP);
		osdTimingSettings->enableShrinkMenu();
		
		osdTimingSettings->enablePaintHead();
		osdTimingSettings->setTitle(_("Timing"), NEUTRINO_ICON_TIMER);

		osdTimingSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		osdTimingSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "osdtimingsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(osdTimingSettings);
	}
	
	osdTimingSettings->clearItems();
	
	// intros
	osdTimingSettings->addItem(new CMenuForwarder(_("back")));
	osdTimingSettings->addItem(new CMenuSeparator(LINE));

	osdTimingSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	osdTimingSettings->addItem(new CMenuSeparator(LINE));

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		CStringInput * colorSettings_timing_item = new CStringInput(timing_setting_name[i], g_settings.timing_string[i], 3, _("Time in sec. After this time the"), _("infobar will be faded out."), "0123456789 ", &timingsettingsnotifier);

		osdTimingSettings->addItem(new CMenuForwarder(timing_setting_name[i], true, g_settings.timing_string[i], colorSettings_timing_item));
	}

	osdTimingSettings->addItem(new CMenuSeparator(LINE));
	osdTimingSettings->addItem(new CMenuForwarder(_("Default"), true, NULL, this, "osd.def"));
	
	//
	widget->exec(NULL, "");
}

// timing settings notifier
bool CTimingSettingsNotifier::changeNotify(const std::string& OptionName, void *)
{
	dprintf(DEBUG_NORMAL, "CTimingSettingsNotifier::changeNotify:\n");
		
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		if (OptionName == timing_setting_name[i])
		{
			g_settings.timing[i] = atoi(g_settings.timing_string[i]);
			return true;
		}
	}

	return false;
}

// diverses
int COSDDiverses::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDDiverses::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
		
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		//HintBox(_("Information"), _("Saving settings now, please be patient."));
		
		return ret;
	}
	else if(actionKey == "select_icons_dir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.icons_dir.c_str())) 
		{
			g_settings.icons_dir = b.getSelectedFile()->Name + "/";

			dprintf(DEBUG_NORMAL, "COSDMiscSettings::exec: new icons dir %s\n", g_settings.icons_dir.c_str());

			CFrameBuffer::getInstance()->setIconBasePath(g_settings.icons_dir);
			//CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		}
		
		getString() = g_settings.icons_dir;
		
		return ret;
	}
	else if(actionKey == "select_buttons_dir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.buttons_dir.c_str())) 
		{
			g_settings.buttons_dir = b.getSelectedFile()->Name + "/";

			dprintf(DEBUG_NORMAL, "COSDMiscSettings::exec: new buttons dir %s\n", g_settings.buttons_dir.c_str());

			CFrameBuffer::getInstance()->setButtonBasePath(g_settings.buttons_dir);
			//CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		}
		
		getString() = g_settings.buttons_dir;
		
		return ret;
	}
	else if(actionKey == "select_hints_dir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.hints_dir.c_str())) 
		{
			g_settings.hints_dir = b.getSelectedFile()->Name + "/";

			dprintf(DEBUG_NORMAL, "COSDMiscSettings::exec: new hints dir %s\n", g_settings.hints_dir.c_str());

			CFrameBuffer::getInstance()->setHintBasePath(g_settings.hints_dir);
			//CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		}
		
		getString() = g_settings.hints_dir;
		
		return ret;
	}
	else if(actionKey == "select_spinner_dir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.spinner_dir.c_str())) 
		{
			g_settings.spinner_dir = b.getSelectedFile()->Name + "/";

			dprintf(DEBUG_NORMAL, "COSDMiscSettings::exec: new spinner dir %s\n", g_settings.spinner_dir.c_str());

			CFrameBuffer::getInstance()->setSpinnerBasePath(g_settings.spinner_dir);
			//CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		}
		
		getString() = g_settings.spinner_dir;
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

// progressbar color
#define PROGRESSBAR_COLOR_OPTION_COUNT 2
const keyval PROGRESSBAR_COLOR_OPTIONS[PROGRESSBAR_COLOR_OPTION_COUNT] =
{
	{ 0, _("monochrom") },
	{ 1, _("colored") }
};

// volumebar position
#define VOLUMEBAR_DISP_POS_OPTIONS_COUNT 6
const keyval  VOLUMEBAR_DISP_POS_OPTIONS[VOLUMEBAR_DISP_POS_OPTIONS_COUNT]=
{
	{ 0 , _("top right") },
	{ 1 , _("top left") },
	{ 2 , _("bottom left") },
	{ 3 , _("bottom right") },
	{ 4 , _("default center") },
	{ 5 , _("higher center") }
};

#define INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT 4
const keyval  INFOBAR_SUBCHAN_DISP_POS_OPTIONS[INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT]=
{
	{ 0 , _("top right") },
	{ 1 , _("top left") },
	{ 2 , _("bottom left") },
	{ 3 , _("bottom right") }
};

void COSDDiverses::showMenu()
{
	dprintf(DEBUG_NORMAL, "COSDTimingSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* osdDiverseSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("osdmiscsetup");
	
	if (widget)
	{
		osdDiverseSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		osdDiverseSettings = new ClistBox(0, 0, 800, MENU_HEIGHT);

		osdDiverseSettings->setWidgetMode(MODE_SETUP);
		osdDiverseSettings->enableShrinkMenu();
		
		osdDiverseSettings->enablePaintHead();
		osdDiverseSettings->setTitle(_("Misc settings"), NEUTRINO_ICON_COLORS);

		osdDiverseSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		osdDiverseSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, 800, MENU_HEIGHT);
		widget->name = "osdmiscsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(osdDiverseSettings);
	}
	
	osdDiverseSettings->clearItems();
	
	// intros
	osdDiverseSettings->addItem(new CMenuForwarder(_("back")));
	osdDiverseSettings->addItem(new CMenuSeparator(LINE));

	osdDiverseSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	osdDiverseSettings->addItem(new CMenuSeparator(LINE));

	// progressbar color
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Progressbar Color"), &g_settings.progressbar_color, PROGRESSBAR_COLOR_OPTIONS, PROGRESSBAR_COLOR_OPTION_COUNT, true));
	
	// progressbar_gradient
	osdDiverseSettings->addItem(new CMenuOptionChooser("ProgressBar Gradient", &g_settings.progressbar_gradient, COLOR_GRADIENT_TYPE_OPTIONS, COLOR_GRADIENT_TYPE_OPTION_COUNT, true));
	
	// subchan pos
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Subchannel display"), &g_settings.infobar_subchan_disp_pos, INFOBAR_SUBCHAN_DISP_POS_OPTIONS, INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT, true));
	
	// volumebar position
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Volumebar"), &g_settings.volume_pos, VOLUMEBAR_DISP_POS_OPTIONS, VOLUMEBAR_DISP_POS_OPTIONS_COUNT, true));

	// volume bar steps
	CStringInput * audio_step = new CStringInput(_("Volume Step Size"), g_settings.audio_step, 2, NULL, NULL, "0123456789 " );
	CMenuForwarder *as = new CMenuForwarder(_("Volume Step Size"), true, g_settings.audio_step, audio_step );
	osdDiverseSettings->addItem(as);
	
	//
	osdDiverseSettings->addItem(new CMenuSeparator(LINE));
	
	// icons dir
	osdDiverseSettings->addItem(new CMenuForwarder(_("Icons Dir"), true, g_settings.icons_dir.c_str(), this, "select_icons_dir"));
	
	// buttons dir
	osdDiverseSettings->addItem(new CMenuForwarder(_("Buttons Dir"), true, g_settings.buttons_dir.c_str(), this, "select_buttons_dir"));
	
	// hints dir
	osdDiverseSettings->addItem(new CMenuForwarder(_("Hints Dir"), true, g_settings.hints_dir.c_str(), this, "select_hints_dir"));
	
	// spinner dir
	osdDiverseSettings->addItem(new CMenuForwarder(_("Spinner Dir"), true, g_settings.spinner_dir.c_str(), this, "select_spinner_dir"));

	//
	widget->exec(NULL, "");
}

//// skinManager
void CSkinManager::showMenu()
{
	dprintf(DEBUG_NORMAL, "CSkinManager::showMenu:\n");
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	ClistBox* skinMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("skinsetup");
	
	if (widget)
	{
		skinMenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		skinMenu = new ClistBox(0, 0, 800, MENU_HEIGHT);

		skinMenu->setWidgetMode(MODE_LISTBOX);
		skinMenu->setWidgetType(TYPE_CLASSIC);
		skinMenu->enableShrinkMenu();
		
		skinMenu->enablePaintHead();
		skinMenu->setTitle(_("Skin Select"), NEUTRINO_ICON_COLORS);

		skinMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		skinMenu->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, 800, MENU_HEIGHT);
		widget->name = "skinsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(skinMenu);
	}
	
	skinMenu->clearItems();
	
	//
	std::string skinPath = CONFIGDIR "/skins";
	
	struct dirent **namelist;
	int i = 0;

	i = scandir(skinPath.c_str(), &namelist, 0, 0);

	if (i > 0)
	{
		while(i--)
		{
			if(namelist[i]->d_type == DT_DIR && !strstr(namelist[i]->d_name, ".") && !strstr(namelist[i]->d_name, ".."))
			{	
				item = new ClistBoxItem((strcmp(namelist[i]->d_name, "neutrino2"))? namelist[i]->d_name : "default");
				
				item->setActionKey(this, namelist[i]->d_name);
				
				std::string hint = skinPath;
				hint += "/";
				hint += namelist[i]->d_name;
				hint += "/prev.png";
				item->setHintIcon(hint.c_str());
				item->setHint(_("Here you can select a skin from the following list."));
				
				bool select = false;
				
				if (g_settings.preferred_skin == namelist[i]->d_name)
				{
					item->setIcon1(NEUTRINO_ICON_MARK);
					select = true;
				}
				
				item->set2lines();
				
				skinMenu->addItem(item, select);	
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	
	//
	widget->exec(NULL, "");
}

int CSkinManager::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CSkinManager::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
		
	if (actionKey == "neutrino2")
	{
		if (MessageBox(_("Skin Select"), _("this need Neutrino restart\ndo you really want to restart?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			g_settings.preferred_skin = "neutrino2";
			
			CNeutrinoApp::getInstance()->unloadSkin();
			CNeutrinoApp::getInstance()->exec(NULL, "restart");
		}
		
		return ret;
	}
	else if (!actionKey.empty())
	{
		if (MessageBox(_("Skin Select"), _("this need Neutrino restart\ndo you really want to restart?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			CNeutrinoApp::getInstance()->unloadSkin();
			g_settings.preferred_skin = actionKey;
			
			usleep(1000);
			CNeutrinoApp::getInstance()->exec(NULL, "restart");
		}
		
		return ret;
	}
		
	showMenu();
	
	return ret;
}

//// skinSettings
void CSkinSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CSkinSettings::showMenu:\n");
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	ClistBox* skinSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("skinstyleselectionsetup");
	
	if (widget)
	{
		skinSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		skinSettings = new ClistBox(0, 0, 800, 600);

		skinSettings->setWidgetMode(MODE_SETUP);
		skinSettings->enableShrinkMenu();
		
		skinSettings->enablePaintHead();
		skinSettings->setTitle(_("Skin Style"), NEUTRINO_ICON_COLORS);

		skinSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		skinSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, 800, 600);
		widget->name = "skinstyleselectionsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(skinSettings);
	}
	
	skinSettings->clearItems();
	
	// intros
	skinSettings->addItem(new CMenuForwarder(_("back")));
	skinSettings->addItem( new CMenuSeparator(LINE) );
	
	// save current skin style
	skinSettings->addItem(new CMenuForwarder(_("Save current skin style"), true, NULL, this, "savecurrentstyle", RC_red, NEUTRINO_ICON_BUTTON_RED));

	skinSettings->addItem( new CMenuSeparator(LINE) );
	
	// load config files
	std::string skinPath = CONFIGDIR "/skins/";
	skinPath += g_settings.preferred_skin.c_str();
	
	struct dirent **namelist;
	int i = 0;

	i = scandir(skinPath.c_str(), &namelist, 0, 0);

	if (i > 0)
	{
		while(i--)
		{
			if( (strcmp(namelist[i]->d_name, ".") != 0) && (strcmp(namelist[i]->d_name, "..") != 0) )
			{
				std::string filename = namelist[i]->d_name;
				
				std::string extension = getFileExt(filename);
				
				// file to skip
				std::string skipFile = g_settings.preferred_skin;
				skipFile += ".config";
				
				if ( strcasecmp("config", extension.c_str()) == 0)
				{
					if (!filename.empty() && filename != skipFile.c_str())
					{
						item = new ClistBoxItem(removeExtension(filename).c_str());
				
						item->setActionKey(this, namelist[i]->d_name);
						item->setHint(_("choose Skin Style."));
						
						skinSettings->addItem(item);
					}
				}
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	
	
	widget->exec(NULL, "");
}

int CSkinSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CSkinSettings::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	if (!actionKey.empty())
	{
		if (actionKey == "savecurrentstyle")
		{
			if (MessageBox(_("Information"), _("Save current style"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
			{
				std::string file_name = "";
				CStringInputSMS * nameInput = new CStringInputSMS(_("Skin Style name"), file_name.c_str());

				nameInput->exec(NULL, "");
				
				//
				if (!nameInput->getExitPressed())
				{
					HintBox(_("Save current style"), _("Saving current style!"));
					
					std::string skinConfig = CONFIGDIR "/skins/";
					skinConfig += g_settings.preferred_skin.c_str();
					skinConfig += "/";
					skinConfig += nameInput->getString().c_str();
					skinConfig += ".config";
				
					CNeutrinoApp::getInstance()->saveSkinConfig(skinConfig.c_str());
				}

				file_name.clear();

				delete nameInput;
				nameInput = NULL;
			}

			return ret;
		}
		else
		{
			if (MessageBox(_("Skin Style"), _("this need Neutrino restart"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
			{
				// read skin config
				std::string skinConfigFile = CONFIGDIR "/skins/";
				skinConfigFile += g_settings.preferred_skin.c_str();
				skinConfigFile += "/";
				skinConfigFile += actionKey.c_str();
				
				CNeutrinoApp::getInstance()->readSkinConfig(skinConfigFile.c_str());
				
				// save skin config
				std::string skinConfig = CONFIGDIR "/skins/";
				skinConfig += g_settings.preferred_skin.c_str();
				skinConfig += "/";
				skinConfig += g_settings.preferred_skin.c_str();
				skinConfig += ".config";
				
				CNeutrinoApp::getInstance()->saveSkinConfig(skinConfig.c_str());
				
				// retsrat
				usleep(1000);
				CNeutrinoApp::getInstance()->exec(NULL, "restart");
			}
			
			return ret;
		}
	}
		
	showMenu();
	
	return ret;
}

