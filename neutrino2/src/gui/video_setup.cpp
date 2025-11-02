/*
	Neutrino-GUI  -   DBoxII-Project

	$id: main_setup.cpp 2015.12.22 21:31:30 mohousch $
	
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

#include <gui/widget/messagebox.h>

#include <gui/video_setup.h>
#include <gui/psisetup.h>

#include <system/debug.h>
#include <system/helpers.h>

#include <video_cs.h>


extern cVideo * videoDecoder;		//libdvbapi (video_cs.cpp)

//hdmi color space
#ifdef __sh__
#define VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT 3
const keyval VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS[VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT] =
{
	 { HDMI_RGB, "HDMI-RGB" },
	 { HDMI_YUV, "HDMI-YUV" } ,
	 { HDMI_422, "HDMI-422" }
};
#else
#define VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT 4
const keyval VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS[VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT] =
{
	 { HDMI_AUTO, "edid(Auto)" },
	 { HDMI_RGB, "hdmi_Rgb" } ,
	 { HDMI_ITU_R_BT_709, "itu_R_BT_709" },
	 { HDMI_UNKNOW, "unknow" }
};
#endif

//Analog Output 
#ifdef __sh__
//rgb/cvbs/svideo/yuv
#define VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT 4
const keyval VIDEOMENU_ANALOGUE_MODE_OPTIONS[VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT] =
{
	 { ANALOG_RGB, "RGB" },
	 { ANALOG_CVBS, "CVBS" },
	 { ANALOG_SVIDEO, "SVIDEO" }, //not used
	 { ANALOG_YUV, "YUV" }
};
#else
//rgb/cvbs/yuv
#define VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT 3
const keyval VIDEOMENU_ANALOGUE_MODE_OPTIONS[VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT] =
{
	 { ANALOG_RGB, "RGB" },
	 { ANALOG_CVBS, "CVBS" },
	 { ANALOG_YUV, "YUV" }
};
#endif

// aspect ratio
#ifdef __sh__
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 2
const keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ ASPECTRATIO_43, "4:3" },
	{ ASPECTRATIO_169, "16:9" }
};
#else
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 3
const keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ ASPECTRATIO_43, "4:3" },
	{ ASPECTRATIO_169, "16:9" },
	{ ASPECTRATIO_AUTO, "Auto" }
};
#endif

// policy
#ifdef __sh__
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, "letterbox" },
	{ VIDEOFORMAT_PANSCAN, "panscan" },
	{ VIDEOFORMAT_FULLSCREEN, _("fullscreen") },
	{ VIDEOFORMAT_PANSCAN2, "bestfit" }
};
#else
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, "letterbox" },
	{ VIDEOFORMAT_PANSCAN, "panscan" },
	{ VIDEOFORMAT_PANSCAN2, "bestfit" },
	{ VIDEOFORMAT_FULLSCREEN, _("fullscreen") }
};
#endif

//video mode
#ifdef __sh__
#define VIDEOMENU_VIDEOMODE_OPTION_COUNT 12
const keyval VIDEOMENU_VIDEOMODE_OPTIONS[VIDEOMENU_VIDEOMODE_OPTION_COUNT] =
{
	{ VIDEO_STD_PAL, "PAL"		},
	{ VIDEO_STD_1080I50, "1080i 50Hz"	},
	{ VIDEO_STD_720P50, "720p 50Hz"	},
	{ VIDEO_STD_576P50, "576p 50Hz"	},
	{ VIDEO_STD_576I50, "576i 50Hz"	},
	{ VIDEO_STD_1080I60, "1080i 60Hz"	},
	{ VIDEO_STD_720P60, "720p 60Hz"	},
	{ VIDEO_STD_1080P24, "1080p 24Hz"	},
	{ VIDEO_STD_1080P25, "1080p 25Hz"	},
	{ VIDEO_STD_1080P30, "1080p 30Hz"	},
	{ VIDEO_STD_1080P50, "1080p 50Hz" 	},
	{ VIDEO_STD_PC, "PC"		}
};
#else
#define VIDEOMENU_VIDEOMODE_OPTION_COUNT 10
const keyval VIDEOMENU_VIDEOMODE_OPTIONS[VIDEOMENU_VIDEOMODE_OPTION_COUNT] =
{
	{ VIDEO_STD_PAL, "PAL"		},
	{ VIDEO_STD_NTSC, "NTSC"		},
	{ VIDEO_STD_480I60, "480i 60Hz"	},
	{ VIDEO_STD_576I50, "576i 50Hz"	},
	{ VIDEO_STD_480P60, "480p 60Hz"	},
	{ VIDEO_STD_576P50, "576p 50Hz"	},
	{ VIDEO_STD_720P50, "720p 50Hz"	},
	{ VIDEO_STD_720P60, "720p 60Hz"	},
	{ VIDEO_STD_1080I50, "1080i 50Hz"	},
	{ VIDEO_STD_1080I60, "1080i 60Hz"	}
};
#endif

// wss
#ifdef __sh__
#define VIDEOMENU_WSS_OPTION_COUNT 3
const keyval VIDEOMENU_WSS_OPTIONS[VIDEOMENU_WSS_OPTION_COUNT] =
{
	{ WSS_OFF, "off" },
	{ WSS_AUTO, "auto" },
	{ WSS_43_OFF, "auto(4:3_off)" },
};
#else
#define VIDEOMENU_WSS_OPTION_COUNT 11
const keyval VIDEOMENU_WSS_OPTIONS[VIDEOMENU_WSS_OPTION_COUNT] =
{
	{ WSS_OFF, "off" },
	{ WSS_AUTO, "auto" },
	{ WSS_43_OFF, "auto(4:3_off)" },
	{ WSS_43_FULL, "4:3(full_format)" },
	{ WSS_169_FULL, "16:9(full_format)" },
	{ WSS_149_LETTERBOX_CENTER, "14:9(letterbox-center)" },
	{ WSS_149_LETTERBOX_TOP, "14:9(letterbox-top)" },
	{ WSS_169_LETTERBOX_CENTER, "16:9(letterbox-center)" },
	{ WSS_169_LETTERBOX_TOP, "16:9(letterbox-top)" },
	{ WSS_169_LETTERBOX_CENTER_RIGHT, ">16:9(letterbox-center)" },
	{ WSS_149_FULL, "14:9(full)"}
	
};
#endif

CVideoSettings::CVideoSettings()
{
	videoSetupNotifier = new CVideoSetupNotifier;
}

CVideoSettings *CVideoSettings::getInstance()
{
	static CVideoSettings *videoSettings = NULL;

	if(!videoSettings)
	{
		videoSettings = new CVideoSettings();
		dprintf(DEBUG_NORMAL, "CVideoSettings::getInstance: Instance created\n");
	}
	
	return videoSettings;
}

CVideoSettings::~CVideoSettings()
{
	delete videoSetupNotifier;
}

int CVideoSettings::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CVideoSettings::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CVideoSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CVideoSettings::showMenu\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* videoSettings = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("videosetup");
	
	if (widget)
	{
		videoSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "videosetup";
		
		//
		videoSettings = new ClistBox(&box);

		videoSettings->setWidgetMode(ClistBox::MODE_SETUP);
		videoSettings->enableShrinkMenu();
		
		//
		videoSettings->enablePaintHead();
		videoSettings->setTitle(_("Video settings"), NEUTRINO_ICON_VIDEO);

		//
		videoSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		videoSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(videoSettings);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Video settings"));
	
	// intros
	videoSettings->addItem(new CMenuForwarder(_("back")));
	videoSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE, NULL, true) );
	
	// save settings
	videoSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	videoSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE, NULL, true) );

	// video aspect ratio 4:3/16:9
	videoSettings->addItem(new CMenuOptionChooser(_("TV-System"), &g_settings.video_Ratio, VIDEOMENU_VIDEORATIO_OPTIONS, VIDEOMENU_VIDEORATIO_OPTION_COUNT, true, videoSetupNotifier));
	
	// video format bestfit/letterbox/panscan/non
	videoSettings->addItem(new CMenuOptionChooser(_("Video Format"), &g_settings.video_Format, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true, videoSetupNotifier));
	
	// video analogue mode
	videoSettings->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.analog_mode, VIDEOMENU_ANALOGUE_MODE_OPTIONS, VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT, true, videoSetupNotifier));

	// video hdmi space colour	
	//videoSettings->addItem(new CMenuOptionChooser(_("HDMI Color Space"), &g_settings.hdmi_color_space, VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS, VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT, true, videoSetupNotifier));	
	
	// wss
	videoSettings->addItem(new CMenuOptionChooser(_("Colour Range"), &g_settings.wss_mode, VIDEOMENU_WSS_OPTIONS, VIDEOMENU_WSS_OPTION_COUNT, true, videoSetupNotifier));	

	// video mode
	videoSettings->addItem(new CMenuOptionChooser(_("Video Resolution"), &g_settings.video_Mode, VIDEOMENU_VIDEOMODE_OPTIONS, VIDEOMENU_VIDEOMODE_OPTION_COUNT, true, videoSetupNotifier, CRCInput::RC_nokey, "", true));
	
	// psi
#if defined (__sh__)
	videoSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE, NULL, true) );
	
	CPSISetup * chPSISetup = new CPSISetup(_("PSI settings"), &g_settings.contrast, &g_settings.saturation, &g_settings.brightness, &g_settings.tint);
	videoSettings->addItem( new CMenuForwarder(_("PSI settings"), true, NULL, chPSISetup));
#endif

	/*
	// contrast
	videoSettings->addItem(new CMenuOptionNumberChooser(_("Contrast"), (int *)&g_settings.contrast, true, 0, 255));

	// saturation
	videoSettings->addItem(new CMenuOptionNumberChooser(_("Saturation"), (int *)&g_settings.saturation, true, 0, 255));

	// brightness
	videoSettings->addItem(new CMenuOptionNumberChooser(_("Brightness"), (int *)&g_settings.brightness, true, 0, 255));

	// tint
	videoSettings->addItem(new CMenuOptionNumberChooser(_("Tint"), (int *)&g_settings.tint, true, 0, 255));
	*/
	
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

// video setup notifier
extern int prev_video_Mode;

bool CVideoSetupNotifier::changeNotify(const std::string& OptionName, void *)
{
	dprintf(DEBUG_NORMAL, "CVideoSetupNotifier::changeNotify\n");
	
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();

	if (OptionName == _("Analog Output"))	/* video analoue mode */
	{
		if(videoDecoder)			
			videoDecoder->SetAnalogMode(g_settings.analog_mode);		
	}
	else if ( (OptionName == _("TV-System") ) || (OptionName == _("Video Format")) )	// format aspect-ratio
	{
		if(videoDecoder)
			videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
	}
	else if (OptionName == _("Video Resolution"))	// mode
	{
		if(videoDecoder)
			videoDecoder->SetVideoSystem(g_settings.video_Mode);
		
		// clear screen
		frameBuffer->paintBackground();
		frameBuffer->blit();		

		if(prev_video_Mode != g_settings.video_Mode) 
		{
			if(MessageBox(_("Information"), _("Is this video mode working ok ?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO) != CMessageBox::mbrYes) 
			{
				g_settings.video_Mode = prev_video_Mode;
				if(videoDecoder)
					videoDecoder->SetVideoSystem(g_settings.video_Mode);	//no-> return to prev mode
			} 
			else
			{
				prev_video_Mode = g_settings.video_Mode;
			}
		}
	}	
	else if (OptionName == _("HDMI Color Space")) 
	{
		if(videoDecoder)
			videoDecoder->SetSpaceColour(g_settings.hdmi_color_space);
	}
	else if (OptionName == _("Wide Screen Auto")) 
	{
		if(videoDecoder)
			videoDecoder->SetWideScreen(g_settings.wss_mode);
	}

	return true;
}



