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
#include <system/setting_helpers.h>
#include <system/helpers.h>

#include <video_cs.h>


extern cVideo * videoDecoder;		//libdvbapi (video_cs.cpp)

//Video Settings
//hdmi color space
#if !defined (PLATFORM_COOLSTREAM)
#ifdef __sh__
#define VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT 3
const keyval VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS[VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT] =
{
	 { HDMI_RGB, "HDMI-RGB" },
	 { HDMI_YUV, "HDMI-YUV" } ,
	 { HDMI_422, "HDMI-422" }
};
#else
// giga
//
//Edid(Auto) 
//Hdmi_Rgb 
//Itu_R_BT_709 
//Unknown
#define VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT 4
const keyval VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS[VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT] =
{
	 { HDMI_AUTO, "Edid(Auto)" },
	 { HDMI_RGB, "Hdmi_Rgb" } ,
	 { HDMI_ITU_R_BT_709, "Itu_R_BT_709" },
	 { HDMI_UNKNOW, "Unknow" }
};
#endif
#endif // !coolstream

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
#elif defined (PLATFORM_COOLSTREAM)
#define VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT 4
const keyval VIDEOMENU_ANALOGUE_MODE_OPTIONS[VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT] =
{
	{ ANALOG_SD_RGB_SCART, "RGB"   }, /* composite + RGB */
	{ ANALOG_SD_YPRPB_SCART, "YPbPr" }, /* YPbPr SCART */
	{ ANALOG_HD_RGB_SCART, "RGB Scart"   },
	{ ANALOG_HD_YPRPB_SCART, "YPbPr Scart" },
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
#elif defined (PLATFORM_COOLSTREAM)
#define VIDEOMENU_VIDEORATIO_OPTION_COUNT 2
const keyval VIDEOMENU_VIDEORATIO_OPTIONS[VIDEOMENU_VIDEORATIO_OPTION_COUNT] =
{
	{ DISPLAY_AR_4_3, "4:3" },
	{ DISPLAY_AR_16_9, "16:9" },
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
//
//letterbox 
//panscan 
//non 
//bestfit
//
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, "Letterbox" },
	{ VIDEOFORMAT_PANSCAN, "Panscan" },
	{ VIDEOFORMAT_FULLSCREEN, _("Fullscreen") },
	{ VIDEOFORMAT_PANSCAN2, "Bestfit" }
};
#elif defined (PLATFORM_COOLSTREAM)
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ DISPLAY_AR_MODE_PANSCAN, "Panscan" },
	{ DISPLAY_AR_MODE_PANSCAN2, "Bestfit" },
	{ DISPLAY_AR_MODE_LETTERBOX, "Letterbox" },
	{ DISPLAY_AR_MODE_NONE, _("Fullscreen") }
};
#else
// giga
//
//letterbox 
//panscan 
//bestfit 
//nonlinear
//
#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] = 
{
	{ VIDEOFORMAT_LETTERBOX, "Letterbox" },
	{ VIDEOFORMAT_PANSCAN, "Panscan" },
	{ VIDEOFORMAT_PANSCAN2, "Bestfit" },
	{ VIDEOFORMAT_FULLSCREEN, _("Fullscreen") }
};
#endif

//video mode
#ifdef __sh__
// cuberevo
//
//pal 
//1080i50 
//720p50 
//576p50 
//576i50 
//1080i60 
//720p60 
//1080p24 
//1080p25 
//1080p30 
//1080p50
//PC
//
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
#elif defined (PLATFORM_COOLSTREAM)
#define VIDEOMENU_VIDEOMODE_OPTION_COUNT 12
const keyval VIDEOMENU_VIDEOMODE_OPTIONS[VIDEOMENU_VIDEOMODE_OPTION_COUNT] =
{
	{ VIDEO_STD_SECAM,   "SECAM"	},
	{ VIDEO_STD_PAL,     "PAL"		},
	{ VIDEO_STD_576P,    "576p"		},
	{ VIDEO_STD_720P50,  "720p 50Hz"	},
	{ VIDEO_STD_1080I50, "1080i 50Hz"	},
	{ VIDEO_STD_1080P24, "1080p 24Hz"	},
	{ VIDEO_STD_1080P25, "1080p 25Hz"	},
	{ VIDEO_STD_NTSC,    "NTSC"		},
	{ VIDEO_STD_480P,    "480p"		},
	{ VIDEO_STD_720P60,  "720p 60Hz"	},
	{ VIDEO_STD_1080I60, "1080i 60Hz"	},
	{ VIDEO_STD_AUTO,    "Auto"         }
};
#else
// giga
//
//pal 
//ntsc 
//480i 
//576i 
//480p 
//576p 
//720p50 
//720p 
//1080i50 
//1080i
//
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
//
//off 
//auto 
//auto(4:3_off) 
//4:3_full_format 
//16:9_full_format 
//14:9_letterbox_center 
//14:9_letterbox_top 
//16:9_letterbox_center 
//16:9_letterbox_top 
//>16:9_letterbox_center 
//14:9_full_format
//
#if !defined (PLATFORM_COOLSTREAM)
#ifdef __sh__
#define VIDEOMENU_WSS_OPTION_COUNT 3
const keyval VIDEOMENU_WSS_OPTIONS[VIDEOMENU_WSS_OPTION_COUNT] =
{
	{ WSS_OFF, "Off" },
	{ WSS_AUTO, "Auto" },
	{ WSS_43_OFF, "Auto(4:3_off)" },
};
#else
// giga
//
//off 
//auto 
//auto(4:3_off) 
//4:3_full_format 
//16:9_full_format 
//14:9_letterbox_center 
//14:9_letterbox_top 
//16:9_letterbox_center 
//16:9_letterbox_top 
//>16:9_letterbox_center 
//14:9_full_format
//
#define VIDEOMENU_WSS_OPTION_COUNT 11
const keyval VIDEOMENU_WSS_OPTIONS[VIDEOMENU_WSS_OPTION_COUNT] =
{
	{ WSS_OFF, "Off" },
	{ WSS_AUTO, "Auto" },
	{ WSS_43_OFF, "Auto(4:3_off)" },
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
#endif // !coolstream

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

int CVideoSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CVideoSettings::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
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
		videoSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		videoSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		videoSettings->setWidgetMode(MODE_SETUP);
		videoSettings->enableShrinkMenu();
		
		videoSettings->enablePaintHead();
		videoSettings->setTitle(_("Video settings"), NEUTRINO_ICON_VIDEO);

		videoSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		videoSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "videosetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(videoSettings);
	}
	
	videoSettings->clearItems();
	
	// intros
	videoSettings->addItem(new ClistBoxItem(_("back")));
	videoSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	videoSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	videoSettings->addItem( new CMenuSeparator(LINE) );

	// video aspect ratio 4:3/16:9
	videoSettings->addItem(new CMenuOptionChooser(_("TV-System"), &g_settings.video_Ratio, VIDEOMENU_VIDEORATIO_OPTIONS, VIDEOMENU_VIDEORATIO_OPTION_COUNT, true, videoSetupNotifier));
	
	// video format bestfit/letterbox/panscan/non
	videoSettings->addItem(new CMenuOptionChooser(_("Video Format"), &g_settings.video_Format, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true, videoSetupNotifier));
	
	// video analogue mode
	videoSettings->addItem(new CMenuOptionChooser(_("Analog Output"), &g_settings.analog_mode, VIDEOMENU_ANALOGUE_MODE_OPTIONS, VIDEOMENU_ANALOGUE_MODE_OPTION_COUNT, true, videoSetupNotifier));

#if !defined (PLATFORM_COOLSTREAM)
	// video hdmi space colour	
	videoSettings->addItem(new CMenuOptionChooser(_("HDMI Color Space"), &g_settings.hdmi_color_space, VIDEOMENU_HDMI_COLOR_SPACE_OPTIONS, VIDEOMENU_HDMI_COLOR_SPACE_OPTION_COUNT, true, videoSetupNotifier));	
	
	// wss
	videoSettings->addItem(new CMenuOptionChooser(_("Colour Range"), &g_settings.wss_mode, VIDEOMENU_WSS_OPTIONS, VIDEOMENU_WSS_OPTION_COUNT, true, videoSetupNotifier));
#endif	

	// video mode
	videoSettings->addItem(new CMenuOptionChooser(_("Video Resolution"), &g_settings.video_Mode, VIDEOMENU_VIDEOMODE_OPTIONS, VIDEOMENU_VIDEOMODE_OPTION_COUNT, true, videoSetupNotifier, RC_nokey, "", true));
	
	// psi
#if defined (__sh__)
	videoSettings->addItem( new CMenuSeparator(LINE) );
	
	CPSISetup * chPSISetup = new CPSISetup(_("PSI settings"), &g_settings.contrast, &g_settings.saturation, &g_settings.brightness, &g_settings.tint);
	videoSettings->addItem( new ClistBoxItem(_("PSI settings"), true, NULL, chPSISetup));
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
	
	widget->exec(NULL, "");
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
#if defined (PLATFORM_COOLSTREAM)
			videoDecoder->SetVideoMode((analog_mode_t) g_settings.analog_mode);
#else			
			videoDecoder->SetAnalogMode(g_settings.analog_mode);
#endif			
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
			if(MessageBox(_("Information"), _("Is this video mode working ok ?"), mbrNo, mbYes | mbNo, NEUTRINO_ICON_INFO) != mbrYes) 
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
#if !defined (PLATFORM_COOLSTREAM)	
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
#endif	

	return true;
}



