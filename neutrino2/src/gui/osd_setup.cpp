//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: osd_setup.cpp 24022025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
//	Homepage: http://dbox.cyberphoria.org/
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

#include <global.h>
#include <neutrino2.h>

#include <stdio.h>
 
#include <sys/stat.h>
#include <dirent.h>

#include <fstream>
#include <iostream>

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>

#include <gui/widget/colorchooser.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/keyboard_input.h>

#include <gui/filebrowser.h>
#include <gui/osd_setup.h>
#include <gui/screensetup.h>
#include <gui/alphasetup.h>

#include <system/debug.h>
#include <system/helpers.h>


////
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

//// osd settings
int COSDSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();

	ret = showMenu();
	
	return ret;
}

int COSDSettings::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "COSDSettings::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("OSD"));
	
	//
	CWidget* widget = NULL;
	ClistBox* osdSettings = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("osd");
	
	if (widget)
	{
		osdSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "osd";
		
		//
		osdSettings = new ClistBox(&box);
		
		osdSettings->setWidgetMode(ClistBox::MODE_MENU);
		osdSettings->setWidgetType(ClistBox::TYPE_CLASSIC);
		osdSettings->enableShrinkMenu();
		osdSettings->paintMainFrame(true);
		//
		osdSettings->enablePaintHead();
		osdSettings->setTitle(_("OSD"), NEUTRINO_ICON_COLORS);
		osdSettings->enablePaintDate();
		//
		osdSettings->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		osdSettings->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) osdSettings->enablePaintItemInfo();
		
		//
		widget->addCCItem(osdSettings);
	
		// skin manager
		osdSettings->addItem( new CMenuForwarder(_("Skin select"), true, NULL, new CSkinManager(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SKIN));
			
		// skin themes
		osdSettings->addItem(new CMenuForwarder(_("Skin Style"), true, NULL, new CSkinSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_THEMES));

		// menu colors
		osdSettings->addItem( new CMenuForwarder(_("Menu"), true, NULL, new COSDMenuColorSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_MENUCOLORS));

		// infobar
		osdSettings->addItem( new CMenuForwarder(_("Infobar"), true, NULL, new COSDInfoBarColorSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_INFOBARCOLORS));

		// language
		osdSettings->addItem(new CMenuForwarder(_("Language"), true, NULL, new CLanguageSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_LANGUAGE));
			
		// font
		osdSettings->addItem(new CMenuForwarder(_("Font"), true, NULL, new CFontSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FONT));
			
		// osd timing
		osdSettings->addItem(new CMenuForwarder(_("Timing"), true, NULL, new COSDTimingSettings(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSDTIMING));

		// sceensetup
		osdSettings->addItem(new CMenuForwarder(_("Screen"), true, NULL, new CScreenSetup(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_SCREENSETUP));
		
		// alpha setup
#ifdef __sh__
		CAlphaSetup * chAlphaSetup = new CAlphaSetup(_("Alpha"), &g_settings.gtx_alpha);
		osdSettings->addItem( new CMenuForwarder(_("Alpha Setup"), true, NULL, chAlphaSetup, NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_ALPHASETUP));	
#endif
		
		// personalize
		if (g_settings.preferred_skin == "standard")
		{
			osdSettings->addItem(new CMenuForwarder(_("Personalisation"), true, NULL, new CPersonalisation(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSD));
		}
		
		// diverses
		osdSettings->addItem(new CMenuForwarder(_("Misc settings"), true, NULL, new COSDDiverses(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_OSD));

		//
		osdSettings->integratePlugins(CPlugins::I_TYPE_OSD);
	}
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

//// osd menucolor settings
#define COLOR_GRADIENT_OPTION_COUNT	5
const keyval COLOR_GRADIENT_OPTIONS[COLOR_GRADIENT_OPTION_COUNT] =
{
	{ NOGRADIENT, _("no gradient") },
	{ DARK2LIGHT, _("dark to light") },
	{ LIGHT2DARK, _("light to dark") },
	{ DARK2LIGHT2DARK, _("dark to light to dark") },
	{ LIGHT2DARK2LIGHT, _("light to dark to light") }
};

#define GRADIENT_TYPE_OPTION_COUNT	3
const keyval GRADIENT_TYPE_OPTIONS[GRADIENT_TYPE_OPTION_COUNT] =
{
	{ GRADIENT_COLOR2TRANSPARENT, _("Color to transparent") },
	{ GRADIENT_ONECOLOR, _("One color") },
	{ GRADIENT_COLOR2COLOR, _("Color to color") }
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

int COSDMenuColorSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDMenuColorSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
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
		OSDmenuColorsSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "menusetup";
		
		//
		OSDmenuColorsSettings = new ClistBox(&box);

		OSDmenuColorsSettings->setWidgetMode(ClistBox::MODE_SETUP);
		OSDmenuColorsSettings->enableShrinkMenu();
		
		//
		OSDmenuColorsSettings->enablePaintHead();
		OSDmenuColorsSettings->setTitle(_("Menu"), NEUTRINO_ICON_COLORS);

		//
		OSDmenuColorsSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		OSDmenuColorsSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(OSDmenuColorsSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Menu"));
	
	// intros
	OSDmenuColorsSettings->addItem(new CMenuForwarder(_("back")));

	// save settings
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	OSDmenuColorsSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	// head
	CColorChooser* chHeadcolor = new CColorChooser(_("Background"), &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue, &g_settings.menu_Head_alpha);

	CColorChooser* chHeadTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue, NULL);

	// window content
	CColorChooser* chContentcolor = new CColorChooser(_("Background"), &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,&g_settings.menu_Content_alpha);

	CColorChooser* chContentTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue, NULL);

	// window content inactive
	CColorChooser* chContentInactivecolor = new CColorChooser(_("Background"), &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue, &g_settings.menu_Content_inactive_alpha);

	CColorChooser* chContentInactiveTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue, NULL);
	
	// window content selected
	CColorChooser* chContentSelectedcolor = new CColorChooser(_("Background"), &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,&g_settings.menu_Content_Selected_alpha);

	CColorChooser* chContentSelectedTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue, NULL);
	
	// foot
	CColorChooser* chFootcolor = new CColorChooser(_("Background"), &g_settings.menu_Foot_red, &g_settings.menu_Foot_green, &g_settings.menu_Foot_blue, &g_settings.menu_Foot_alpha);

	CColorChooser * chFootTextcolor = new CColorChooser(_("Textcolor"), &g_settings.menu_Foot_Text_red, &g_settings.menu_Foot_Text_green, &g_settings.menu_Foot_Text_blue, NULL);

	// itemInfo
	CColorChooser* chHintColor = new CColorChooser(_("Background"), &g_settings.menu_Hint_red, &g_settings.menu_Hint_green, &g_settings.menu_Hint_blue, &g_settings.menu_Hint_alpha);

	CColorChooser * chHintTextColor = new CColorChooser(_("Textcolor"), &g_settings.menu_Hint_Text_red, &g_settings.menu_Hint_Text_green, &g_settings.menu_Hint_Text_blue, NULL);

	// head
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Head")));
	
	// head colr
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chHeadcolor ));
	
	// head text
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chHeadTextcolor ));

	// head gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Head_gradient, COLOR_GRADIENT_OPTIONS, COLOR_GRADIENT_OPTION_COUNT, true));
	
	// head gradienttype
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient type"), &g_settings.Head_gradient_type, GRADIENT_TYPE_OPTIONS, GRADIENT_TYPE_OPTION_COUNT, true));
	
	// head corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Head_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// head radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Head_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// head line
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line"), &g_settings.Head_line, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// head line gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line Gradient"), &g_settings.Head_line_gradient, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// window content
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Window-Content")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentcolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentTextcolor ));

	// window content inactiv
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Window-Content inactive")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentInactivecolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentInactiveTextcolor));

	// window content selected
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Window-Content selected")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chContentSelectedcolor ));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chContentSelectedTextcolor ));
	
	// foot
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Foot")));
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chFootcolor ));

	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chFootTextcolor ));

	// foot gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Foot_gradient, COLOR_GRADIENT_OPTIONS, COLOR_GRADIENT_OPTION_COUNT, true));
	
	// foot gradienttype
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient type"), &g_settings.Foot_gradient_type, GRADIENT_TYPE_OPTIONS, GRADIENT_TYPE_OPTION_COUNT, true));
	
	// foot corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Foot_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// foot radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Foot_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// foot line
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line"), &g_settings.Foot_line, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// foot line gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Line Gradient"), &g_settings.Foot_line_gradient, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// itemInfo
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Item Info")));
	
	// itemInfocolor
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Background"), true, NULL, chHintColor ));

	// fontinfo text color
	OSDmenuColorsSettings->addItem( new CMenuForwarder(_("Textcolor"), true, NULL, chHintTextColor ));

	// itemInfo gradient
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.Hint_gradient, COLOR_GRADIENT_OPTIONS, COLOR_GRADIENT_OPTION_COUNT, true));
	
	// iteminfo gradienttype
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Gradient type"), &g_settings.Hint_gradient_type, GRADIENT_TYPE_OPTIONS, GRADIENT_TYPE_OPTION_COUNT, true));
	
	// iteminfo border
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Border"), &g_settings.Hint_border, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// itemInfo Radius
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.Hint_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	// itemInfo corner
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.Hint_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// sep gradient
	OSDmenuColorsSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Separator")));
	
	OSDmenuColorsSettings->addItem(new CMenuOptionChooser(_("Separator Gradient"), &g_settings.sep_gradient, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
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

//// osd infobarcolor settings
int COSDInfoBarColorSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDInfoBarColorSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
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
		OSDinfobarColorSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "infobarsetup";
		
		//
		OSDinfobarColorSettings = new ClistBox(&box);

		OSDinfobarColorSettings->setWidgetMode(ClistBox::MODE_SETUP);
		OSDinfobarColorSettings->enableShrinkMenu();
		
		//
		OSDinfobarColorSettings->enablePaintHead();
		OSDinfobarColorSettings->setTitle(_("Infobar"), NEUTRINO_ICON_COLORS);

		//
		OSDinfobarColorSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		OSDinfobarColorSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(OSDinfobarColorSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Infobar"));
	
	// intros
	OSDinfobarColorSettings->addItem(new CMenuForwarder(_("back")));

	OSDinfobarColorSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	OSDinfobarColorSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));	

	// bg
	CColorChooser * chInfobarcolor = new CColorChooser(_("Background"), &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,&g_settings.infobar_alpha);
	
	// text
	CColorChooser * chInfobarTextcolor = new CColorChooser(_("Textcolor"), &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue, NULL);
	
	// colored events
	CColorChooser * chColored_Events = new CColorChooser(_("Events Textcolor"), &g_settings.infobar_colored_events_red, &g_settings.infobar_colored_events_green, &g_settings.infobar_colored_events_blue, NULL);

	OSDinfobarColorSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE));

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
	
	// infobar buttonline gradient
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Buttons Line Gradient"), &g_settings.infobar_buttonline_gradient, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));

	// gradient
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Gradient"), &g_settings.infobar_gradient, COLOR_GRADIENT_OPTIONS, COLOR_GRADIENT_OPTION_COUNT, true));
	
	// gradient type
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Gradient type"), &g_settings.infobar_gradient_type, GRADIENT_TYPE_OPTIONS, GRADIENT_TYPE_OPTION_COUNT, true));
	
	// corner
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Corner"), &g_settings.infobar_corner, CORNER_TYPE_OPTIONS, CORNER_TYPE_OPTION_COUNT, true));
	
	// radius
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Radius"), &g_settings.infobar_radius, RADIUS_TYPE_OPTIONS, RADIUS_TYPE_OPTION_COUNT, true));
	
	//
	OSDinfobarColorSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE));
	
	// sig/snr
	OSDinfobarColorSettings->addItem(new CMenuOptionChooser(_("Satellite display on infobar"), &g_settings.infobar_sat_display, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
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

//// osd language settings
CLanguageSettings::CLanguageSettings(bool wizzard)
{
	fromStartWizzard = wizzard;
}

int CLanguageSettings::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CLanguageSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();

	if (!actionKey.empty() && actionKey != g_settings.language)
	{
		strcpy(g_settings.language, actionKey.c_str());
		g_Locale->loadLocale(Lang2I18N(g_settings.language).c_str());
		
		//
		if (!fromStartWizzard)
		{
			CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
			hide();
			showMenu();
			return RETURN_EXIT_ALL;
		}
		
		return CMenuTarget::RETURN_EXIT;
	}
	
	ret = showMenu();
	
	return ret;
}

int CLanguageSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CLanguageSettings::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	ClistBox* languageSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("languagesetup");
	
	if (widget)
	{
		languageSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "languagesetup";
		
		//
		languageSettings = new ClistBox(&box);

		languageSettings->setWidgetMode(ClistBox::MODE_SETUP);
		languageSettings->enableShrinkMenu();
		
		//
		languageSettings->enablePaintHead();
		languageSettings->setTitle(_("Language"), NEUTRINO_ICON_LANGUAGE);

		//
		languageSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		languageSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(languageSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Language"));
	
	// intros
	languageSettings->addItem(new CMenuForwarder(_("back")));
	languageSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	languageSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	languageSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	item = new CMenuForwarder(_("English"), true, NULL, this, "en");
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
				item = new CMenuForwarder(_(locale2lang(namelist[n]->d_name).c_str()), true, NULL, this, namelist[n]->d_name);
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
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

//// CFontSettings
int CFontSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CFontSettings::exec: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->setupFonts(g_settings.font_file);
		
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
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
			CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
			CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		}
		
		this->setValueString(g_settings.font_file);
		
		return ret;
	}
		
	showMenu();
	this->clearValueString();
	
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
		fontSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = 700;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "fontsetup";
		
		//
		fontSettings = new ClistBox(&box);

		fontSettings->setWidgetMode(ClistBox::MODE_SETUP);
		fontSettings->enableShrinkMenu();
		
		//
		fontSettings->enablePaintHead();
		fontSettings->setTitle(_("Font"), NEUTRINO_ICON_COLORS);

		//
		fontSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		fontSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(fontSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Font"));
	
	// intros
	fontSettings->addItem(new CMenuForwarder(_("back")));
	fontSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	fontSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	fontSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// font name
	CMenuForwarder *m1 = new CMenuForwarder(_("Font name"), true, g_settings.font_file, this, "select_font");
	fontSettings->addItem(m1);
	
	// font scaling
	fontSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Font")));
	fontSettings->addItem(new CMenuOptionNumberChooser(_("Horizontal (in %)"), &g_settings.screen_xres, true, 50, 200, NULL) );
	fontSettings->addItem(new CMenuOptionNumberChooser(_("Vertikal (in %)"), &g_settings.screen_yres, true, 50, 200, NULL) );
		
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

//// osd timing settings
int COSDTimingSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDTimingSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
		
	if(actionKey == "osd.def") 
	{
		g_settings.timing_menu = DEFAULT_TIMING_MENU;
		g_settings.timing_channellist = DEFAULT_TIMING_CHANNELLIST;
		g_settings.timing_epg = DEFAULT_TIMING_EPG;
		g_settings.timing_infobar = DEFAULT_TIMING_INFOBAR;
		g_settings.timing_filebrowser = DEFAULT_TIMING_FILEBROWSER;
		g_settings.timing_numericzap = DEFAULT_TIMING_NUMERICZAP;
		
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
		osdTimingSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "osdtimingsetup";
		
		//
		osdTimingSettings = new ClistBox(&box);

		osdTimingSettings->setWidgetMode(ClistBox::MODE_SETUP);
		osdTimingSettings->enableShrinkMenu();
		
		//
		osdTimingSettings->enablePaintHead();
		osdTimingSettings->setTitle(_("Timing"), NEUTRINO_ICON_TIMER);

		//
		osdTimingSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		osdTimingSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(osdTimingSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Timing"));
	
	// intros
	osdTimingSettings->addItem(new CMenuForwarder(_("back")));
	osdTimingSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	osdTimingSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	osdTimingSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("Menu"), &g_settings.timing_menu, true, 0, 600));
	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("Channellist"), &g_settings.timing_channellist, true, 0, 600));
	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("EPG"), &g_settings.timing_epg, true, 0, 600));
	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("Infobar"), &g_settings.timing_infobar, true, 0, 600));
	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("Filebrowser"), &g_settings.timing_filebrowser, true, 0, 600));
	osdTimingSettings->addItem(new CMenuOptionNumberChooser(_("Numericzap"), &g_settings.timing_numericzap, true, 0, 600));
	

	osdTimingSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	osdTimingSettings->addItem(new CMenuForwarder(_("Default"), true, NULL, this, "osd.def"));
	
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

//// diverses
int COSDDiverses::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "COSDDiverses::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
		
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "logos_dir") 
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.logos_dir.c_str())) 
		{
			g_settings.logos_dir = b.getSelectedFile()->Name;

			dprintf(DEBUG_NORMAL, "COSDDiverses::exec: new logos dir %s\n", b.getSelectedFile()->Name.c_str());
		}

		this->setValueString(g_settings.logos_dir.c_str());
		
		return ret;
	}
	
	showMenu();
	this->clearValueString();
	
	return ret;
}

// progressbar color
#define PROGRESSBAR_COLOR_OPTION_COUNT 6
const keyval PROGRESSBAR_COLOR_OPTIONS[PROGRESSBAR_COLOR_OPTION_COUNT] =
{
	{ CCProgressBar::PROGRESSBAR_MONOCHROM, _("monochrom") },
	{ CCProgressBar::PROGRESSBAR_COLORED, _("colored") },
	{ CCProgressBar::PROGRESSBAR_RED, _("red") },
	{ CCProgressBar::PROGRESSBAR_GREEN, _("green") },
	{ CCProgressBar::PROGRESSBAR_YELLOW, _("yellow") },
	{ CCProgressBar::PROGRESSBAR_BLUE, _("blue") }
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
		osdDiverseSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "osdmiscsetup";
		
		//
		osdDiverseSettings = new ClistBox(&box);

		osdDiverseSettings->setWidgetMode(ClistBox::MODE_SETUP);
		osdDiverseSettings->enableShrinkMenu();
		
		//
		osdDiverseSettings->enablePaintHead();
		osdDiverseSettings->setTitle(_("Misc settings"), NEUTRINO_ICON_COLORS);

		//
		osdDiverseSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		osdDiverseSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(osdDiverseSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Misc settings"));
	
	// intros
	osdDiverseSettings->addItem(new CMenuForwarder(_("back")));
	osdDiverseSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	osdDiverseSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	osdDiverseSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// progressbar color
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Progressbar Color"), &g_settings.progressbar_color, PROGRESSBAR_COLOR_OPTIONS, PROGRESSBAR_COLOR_OPTION_COUNT, true));
	
	// subchan pos
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Subchannel display"), &g_settings.infobar_subchan_disp_pos, INFOBAR_SUBCHAN_DISP_POS_OPTIONS, INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT, true));
	
	// volumebar position
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Volumebar"), &g_settings.volume_pos, VOLUMEBAR_DISP_POS_OPTIONS, VOLUMEBAR_DISP_POS_OPTIONS_COUNT, true));

	// volume bar steps
	CStringInput * audio_step = new CStringInput(_("Volume Step Size"), g_settings.audio_step, 2, NULL, NULL, "0123456789 " );
	CMenuForwarder *as = new CMenuForwarder(_("Volume Step Size"), true, g_settings.audio_step, audio_step );
	osdDiverseSettings->addItem(as);
	
	//
	osdDiverseSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE, NULL, true));
	
	// logos
	osdDiverseSettings->addItem(new CMenuOptionChooser(_("Channel Logo"), &g_settings.logos_show_logo, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	// logos dir
	CMenuForwarder *m1 = new CMenuForwarder(_("logos Dir"), true, g_settings.logos_dir.c_str(), this, "logos_dir" );
	osdDiverseSettings->addItem(m1);
	
	// weather
	osdDiverseSettings->addItem(new CMenuOptionChooser("Show weather", &g_settings.show_weather, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	// item info
	if (g_settings.preferred_skin == "standard")
	{
		//
		osdDiverseSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
		osdDiverseSettings->addItem(new CMenuOptionChooser(_("Show Hints"), &g_settings.item_info, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	}

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

//// skinManager
int CSkinManager::showMenu()
{
	dprintf(DEBUG_NORMAL, "CSkinManager::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	ClistBox* skinMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("skinsetup");
	
	if (widget)
	{
		skinMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
		CBox box;
		
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "skinsetup";
		
		//
		skinMenu = new ClistBox(&box);

		skinMenu->setWidgetMode(ClistBox::MODE_LISTBOX);
		skinMenu->setWidgetType(ClistBox::TYPE_CLASSIC);
		skinMenu->enableShrinkMenu();
		skinMenu->enablePaintHead();
		skinMenu->setTitle(_("Skin Select"), NEUTRINO_ICON_COLORS);
		skinMenu->enablePaintFoot();	
		skinMenu->setFootButtons(&btn);
		
		if (g_settings.item_info)
		{
			skinMenu->enablePaintItemInfo();
		}
		
		//
		widget->addCCItem(skinMenu);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Skin Select"));
	
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
				item = new CMenuForwarder(namelist[i]->d_name);
				
				item->setActionKey(this, namelist[i]->d_name);
				
				// itemIcom
				std::string hintIcon = skinPath;
				hintIcon += "/";
				hintIcon += namelist[i]->d_name;
				hintIcon += "/prev.png";
				item->setHintIcon(hintIcon.c_str());
				
				// itemHint
				if (strstr(namelist[i]->d_name, "standard"))
				{
					item->setOption("Author: mohousch");
					item->setHint(_("default Skin for NeutrinoNG"));
				}
				
				bool select = false;
				
				if (g_settings.preferred_skin == namelist[i]->d_name)
				{
					item->setIcon1(NEUTRINO_ICON_MARK);
					select = true;
				}
				
				item->set2lines(true);
				
				skinMenu->addItem(item, select);	
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

int CSkinManager::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CSkinManager::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	if (!actionKey.empty() && actionKey != g_settings.preferred_skin)
	{
		// save current skin settings
		CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
		
		// load selected skin
		std::string buffer;
		buffer = _("Loading skin ");
		buffer += actionKey.c_str();
		buffer += _(", please be patient...");
		
		HintBox(_("Information"), _(buffer.c_str()));
			
		//
		CNeutrinoApp::getInstance()->loadSkin(actionKey.c_str());
		
		g_settings.preferred_skin = actionKey;
		
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return CMenuTarget::RETURN_EXIT_ALL;
	}
		
	ret = showMenu();
	
	return ret;
}

//// skinSettings
int CSkinSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CSkinSettings::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	CMenuItem* item = NULL;
	CWidget* widget = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("skinstyleselectionsetup");
	
	if (widget)
	{
		skinSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "skinstyleselectionsetup";
		
		//
		skinSettings = new ClistBox(&box);

		skinSettings->setWidgetMode(ClistBox::MODE_SETUP);
		skinSettings->enableShrinkMenu();
		
		//
		skinSettings->enablePaintHead();
		skinSettings->setTitle(_("Skin Style"), NEUTRINO_ICON_COLORS);

		//
		skinSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		skinSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(skinSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Skin Style"));
	
	// intros
	skinSettings->addItem(new CMenuForwarder(_("back")));
	skinSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save current skin style
	skinSettings->addItem(new CMenuForwarder(_("Save current skin style"), true, NULL, this, "savecurrentstyle", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	skinSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
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
						item = new CMenuForwarder(removeExtension(filename).c_str());
				
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
	
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

int CSkinSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CSkinSettings::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	if (!actionKey.empty())
	{
		if (actionKey == "savecurrentstyle")
		{
			std::string file_name = "";
			CKeyboardInput * nameInput = new CKeyboardInput(_("Skin Style name"), file_name.c_str());

			nameInput->exec(NULL, "");
				
			//
			if (!nameInput->getExitPressed())
			{
				HintBox(_("Save current style"), _("Saving current style!"));
					
				std::string skinConfig = CONFIGDIR "/skins/";
				skinConfig += g_settings.preferred_skin.c_str();
				skinConfig += "/";
				skinConfig += nameInput->getValueString().c_str();
				skinConfig += ".config";
				
				//
				std::ifstream input(skinConfig.c_str());
				bool overwrite = true;

				// 
				if (input.is_open()) 
				{
					overwrite = CFileHelpers::getInstance()->askToOverwriteFile(skinConfig);
				}
				input.close();
				
				if (overwrite) CNeutrinoApp::getInstance()->saveSkinConfig(skinConfig.c_str());
			}
			
			//
			CMenuItem *item = new CMenuForwarder(nameInput->getValueString().c_str());
			skinSettings->addItem(item);

			file_name.clear();
			delete nameInput;
			nameInput = NULL;

			return RETURN_REPAINT;
		}
		else
		{
			g_settings.theme = actionKey.c_str();
			//
			std::string buffer;
			buffer = _("Loading skin style ");
			buffer += actionKey.c_str();
			buffer += _(", please be patient...");
			
			HintBox(_("Information"), _(buffer.c_str()));
		
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
				
			// 
			CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
			
			return CMenuTarget::RETURN_EXIT_ALL;
		}
	}
		
	ret = showMenu();
	
	return ret;
}

//// personalize
int CPersonalisation::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CPersonalisation::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	ret = showMenu();
	
	return ret;
}

int CPersonalisation::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CSkinSettings::showMenu:\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	CMenuItem *item = NULL;
	CWidget* widget = NULL;
	ClistBox* personalizeSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("personalisation");
	
	if (widget)
	{
		personalizeSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "personalisation";
		
		//
		personalizeSettings = new ClistBox(&box);

		personalizeSettings->setWidgetMode(ClistBox::MODE_SETUP);
		personalizeSettings->enableShrinkMenu();
		
		//
		personalizeSettings->enablePaintHead();
		personalizeSettings->setTitle(_("Personalisation"), NEUTRINO_ICON_COLORS);

		//
		personalizeSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		personalizeSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(personalizeSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Personalisation"));
	
	// intros
	personalizeSettings->addItem(new CMenuForwarder(_("back")));
	personalizeSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	personalizeSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	personalizeSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// mainmenu
	// tv / radio
	item = new CMenuOptionChooser(_("TV / Radio"), &g_settings.personalize_tvradio);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	item->enablePullDown();
	personalizeSettings->addItem(item);
	
	// epg / timer
	item = new CMenuOptionChooser(_("EPG / Timer"), &g_settings.personalize_epgtimer);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	// scart
#ifdef ENABLE_SCART
	item = new CMenuOptionChooser(_("Scart"), &g_settings.personalize_scart);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
#endif
	
	// features
	item = new CMenuOptionChooser(_("Features"), &g_settings.personalize_features);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	// service
	item = new CMenuOptionChooser(_("Service"), &g_settings.personalize_service);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	// settings
	
	// information
	item = new CMenuOptionChooser(_("Information"), &g_settings.personalize_information);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	// powermenu
	item = new CMenuOptionChooser(_("Power Menu"), &g_settings.personalize_powermenu);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	// mediaplayer
	item = new CMenuOptionChooser(_("Media Player"), &g_settings.personalize_mediaplayer);
	item->setActive(true);
	item->addOption(_("active"), CMenuItem::ITEM_ACTIVE);
	item->addOption(_("locked"), CMenuItem::ITEM_LOCKED);
	item->addOption(_("hidden"), CMenuItem::ITEM_HIDDEN);
	item->addOption(_("inactive"), CMenuItem::ITEM_INACTIVE);
	personalizeSettings->addItem(item);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

