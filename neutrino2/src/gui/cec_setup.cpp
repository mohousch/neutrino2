/*
	cec settings menu - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2011 T. Graf 'dbt'
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gui/cec_setup.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/hdmi_cec.h>

#include <system/debug.h>


CCECSetup::CCECSetup()
{
	cec2 = NULL;
	cec3 = NULL;
}

int CCECSetup::exec(CTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CCECSetup::exec:\n");
	
	int   res = CTarget::RETURN_REPAINT;

	if (parent)
		parent->hide();

	res = showMenu();

	return res;
}

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define VIDEOMENU_HDMI_CEC_MODE_OPTION_COUNT 3
const keyval VIDEOMENU_HDMI_CEC_MODE_OPTIONS[VIDEOMENU_HDMI_CEC_MODE_OPTION_COUNT] =
{
	{ VIDEO_HDMI_CEC_MODE_OFF	, _("CEC mode off")      },
	{ VIDEO_HDMI_CEC_MODE_TUNER	, _("CEC mode tuner")   },
	{ VIDEO_HDMI_CEC_MODE_RECORDER	, _("CEC mode recorder") }
};

#define VIDEOMENU_HDMI_CEC_VOL_OPTION_COUNT 3
const keyval VIDEOMENU_HDMI_CEC_VOL_OPTIONS[VIDEOMENU_HDMI_CEC_VOL_OPTION_COUNT] =
{
	{ VIDEO_HDMI_CEC_VOL_OFF		, _("CEC volume off") },
	{ VIDEO_HDMI_CEC_VOL_AUDIOSYSTEM, _("CEC volume audiosystem") },
	{ VIDEO_HDMI_CEC_VOL_TV			, _("CEC volume TV") }
};

int CCECSetup::showMenu()
{
	//menue init
	CWidget *widget = NULL;
	ClistBox *cec = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("cecsetup");
	
	if (widget)
	{
		cec = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "cecsetup";
		
		//
		cec = new ClistBox(&box);

		cec->setWidgetMode(ClistBox::MODE_SETUP);
		cec->enableShrinkMenu();
		
		//
		cec->enablePaintHead();
		cec->setTitle(_("CEC Setup"), NEUTRINO_ICON_SETTINGS);

		//
		cec->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		cec->setFootButtons(&btn);
		
		//
		widget->addCCItem(cec);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("CEC Setup"));
	
	// intros
	cec->addItem(new CMenuForwarder(_("back")));
	cec->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	cec->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	cec->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// cec mode
	CMenuOptionChooser *cec1 = new CMenuOptionChooser(_("CEC mode"), &g_settings.hdmi_cec_mode, VIDEOMENU_HDMI_CEC_MODE_OPTIONS, VIDEOMENU_HDMI_CEC_MODE_OPTION_COUNT, true, this);
	
	// cec standby
	cec2 = new CMenuOptionChooser(_("CEC standby"), &g_settings.hdmi_cec_standby, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, g_settings.hdmi_cec_mode != VIDEO_HDMI_CEC_MODE_OFF, this);
	
	// cec volume
	cec3 = new CMenuOptionChooser(_("CEC volume"), &g_settings.hdmi_cec_volume, VIDEOMENU_HDMI_CEC_VOL_OPTIONS, VIDEOMENU_HDMI_CEC_VOL_OPTION_COUNT, g_settings.hdmi_cec_mode != VIDEO_HDMI_CEC_MODE_OFF, this);

	cec->addItem(cec1);
	cec->addItem(cec2);
	cec->addItem(cec3);

	widget->setTimeOut(g_settings.timing_menu);
	
	int res = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

	return res;
}

bool CCECSetup::changeNotify(const std::string& OptionName, void *)
{
	dprintf(DEBUG_NORMAL, "CCECSetup::changeNotify\n");

	if (OptionName == _("CEC mode"))
	{
		cec2->setActive(g_settings.hdmi_cec_mode != VIDEO_HDMI_CEC_MODE_OFF);
		cec3->setActive(g_settings.hdmi_cec_mode != VIDEO_HDMI_CEC_MODE_OFF);
		
		hdmi_cec::getInstance()->setCECMode((VIDEO_HDMI_CEC_MODE)g_settings.hdmi_cec_mode);
	}
	else if (OptionName == _("CEC standby"))
	{
		hdmi_cec::getInstance()->setCECAutoStandby(g_settings.hdmi_cec_standby == 1);
	}
	else if (OptionName == _("CEC volume"))
	{
		if (g_settings.hdmi_cec_mode != VIDEO_HDMI_CEC_MODE_OFF)
		{
			g_settings.current_volume = 100;
			
			hdmi_cec::getInstance()->getAudioDestination();
		}
	}

	return false;
}

