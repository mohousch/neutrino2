/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: skin.cpp 23.09.2023 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <string>
#include <stdlib.h>
#include <dirent.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>
#include <gui/widget/framebox.h>
#include <gui/widget/hintbox.h>

#include <gui/plugins.h>
//
#include <gui/main_setup.h>
#include <gui/epg_menu.h>
#include <gui/epgplus.h>
#include <gui/timerlist.h>
#include <gui/streaminfo.h>
#include <gui/service_menu.h>
#include <gui/mediaplayer.h>
#include <gui/dboxinfo.h>
#include <gui/power_menu.h>

#include <gui/audio_setup.h>
#include <gui/video_setup.h>
#include <gui/parentallock_setup.h>
#include <gui/network_setup.h>
#include <gui/proxyserver_setup.h>
#include <gui/nfs.h>
#include <gui/movieplayer_setup.h>
#include <gui/osd_setup.h>
#include <gui/audioplayer_setup.h>
#include <gui/pictureviewer_setup.h>
#include <gui/lcd_setup.h>
#include <gui/rc_setup.h>
#include <gui/recording_setup.h>
#include <gui/misc_setup.h>
#include <gui/hdd_menu.h>
#include <gui/screensetup.h>
#include <gui/alphasetup.h>
#include <gui/zapit_setup.h>
#include <gui/bedit/bouqueteditor_bouquets.h>
#include <gui/cam_menu.h>
#include <gui/update.h>
#include <gui/imageinfo.h>
#include <gui/sleeptimer.h>

#include <system/helpers.h>
#include <system/debug.h>

#if ENABLE_LUA
#include <interfaces/lua/neutrino2_lua.h>
#endif


//
CMenuTarget* CNeutrinoApp::convertTarget(const std::string& name)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertTarget: %s\n", name.c_str());
	
	CMenuTarget* parent = NULL;
	
	if (name == "neutrino") 
	{
		parent = this;
	}
	else if (name == "mainmenu") 
	{
		parent = this;
	}
	else if (name == "features") 
	{
		parent = this;
	}
	else if (name == "nvod") 
	{
		parent = this;
	}
	else if (name == "settings")
	{
		parent = new CMainSettingsMenu();
	}
	else if (name == "epgtimer")
	{
		parent = new CEPGMenuHandler();
	}
	else if (name == "system")
	{
		parent = new CServiceMenu();
	}
	else if (name == "information")
	{
		parent = new CInfoMenu();
	}
	else if (name == "powermenu")
	{
		parent = new CPowerMenu();
	}
	else if (name == "mediaplayer")
	{
		parent = new CMediaPlayerMenu();
	}
	else if (name == "videosetup")
	{
		parent = new CVideoSettings();
	}
	else if (name == "audiosetup")
	{
		parent = new CAudioSettings();
	}
	else if (name == "parentallocksetup")
	{	
		parent = new CParentalLockSettings();
	}
	else if (name == "networksetup")
	{		
		parent = CNetworkSettings::getInstance();
	}
	else if (name == "recordingsetup")
	{	
		parent = new CRecordingSettings();
	}
	else if (name == "movieplayersetup")
	{
		parent = new CMoviePlayerSettings();
	}
	else if (name == "osd")
	{		
		parent = new COSDSettings();
	}
	else if (name == "lcdsetup")
	{
		parent = new CLCDSettings();
	}
	else if (name == "rcsetup")
	{		
		parent = new CRemoteControlSettings();
	}
	else if (name == "audioplayersetup")
	{
		parent = new CAudioPlayerSettings();
	}
	else if (name == "pictureviewersetup")
	{		
		parent = new CPictureViewerSettings();
	}
	else if (name == "hddsetup")
	{		
		parent = new CHDDMenuHandler();
	}
	else if (name == "skinsetup")
	{	
		parent = new CSkinManager();
	}
	else if (name == "menusetup")
	{
		parent = new COSDMenuColorSettings();
	}
	else if (name == "infobarsetup")
	{		
		parent = new COSDInfoBarColorSettings();
	}
	else if (name == "languagesetup")
	{		
		parent = new CLanguageSettings();
	}
	else if (name == "fontsetup")
	{		
		parent = new CFontSettings();
	}
	else if (name == "osdtimingsetup")
	{
		parent = new COSDTimingSettings;
	}
	else if (name == "screensetup")
	{
		parent = new CScreenSetup();
	}
	else if (name == "osdmiscsetup")
	{		
		parent = new COSDDiverses;
	}
	else if (name == "alphasetup")
	{		
		parent = new CAlphaSetup(_("Alpha Setup"), &g_settings.gtx_alpha);;
	}
	else if (name == "skinstyleselectionsetup")
	{		
		parent = new CSkinSettings();
	}
	else if (name == "miscsetup")
	{		
		parent = new CGeneralSettings();
	}
	else if (name == "epgsetup")
	{		
		parent = new CEPGSettings();
	}
	else if (name == "channellistsetup")
	{		
		parent = new CChannelListSettings();
	}
	else if (name == "zapitsetup")
	{		
		parent = new CZapitSetup();
	}
	else if (name == "filebrowsersetup")
	{		
		parent = new CFileBrowserSettings();
	}
	else if (name == "mediaplayer")
	{		
		parent = new CTunerSetup();
	}
	else if (name == "cicamsetup")
	{
#if defined ENABLE_CI			
		parent = new CCAMMenuHandler();
#endif
	}
	else if (name == "updatesetup")
	{				
		parent = new CUpdateSettings();
	}
	else if (name == "bouqueteditor")
	{		
		parent = new CBEBouquetWidget();
	}
	else if (name == "imageinfo")
	{		
		parent = new CImageInfo();
	}
	else if (name == "boxinfo")
	{
		parent = new CDBoxInfoWidget;
	}
	else if (name == "streaminfo")
	{		
		parent = new CStreamInfo();
	}
	else if (name == "sleeptimer")
	{		
		parent = new CSleepTimerWidget();
	}
	else if (name == "eventlist")
	{		
		parent = new CEventListHandler();
	}
	else if (name == "epgview")
	{		
		parent = new CEPGDataHandler();
	}
	else if (name == "epgplus")
	{		
		parent = new CEPGplusHandler();
	}
	else if (name == "pluginlist")
	{		
		parent = new CPluginList();
	}
	else if (name == "timerlist")
	{
		parent = new CTimerList();
	}
	else if (name == "newtimer")
	{
		parent = new CTimerList();
	}
	else if (name == "modifytimer")
	{	
		parent = new CTimerList();	
	}
	else if (name == "proxysetup")
	{
		parent = new CProxySetup();
	}
	else if (name == "nfs")
	{		
		parent = new CNFSMountGui();
	}
	else if (name == "scansetup")
	{
		parent = new CTunerSetup();
	}
	else if (name == "transponder")
	{
		parent = new CTPSelectHandler();
	}
	else if (name == "manualscan")
	{
		parent = new CScanSetup();
	}
	else if (name == "autoscan")
	{
		parent = new CScanSetup();
	}
	else if (name == "unicablesetup")
	{
		parent = new CScanSetup();
	}
	
	return parent;
}


//
uint32_t CNeutrinoApp::convertColor(const char* const color)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertColor: color: %s\n", color);
	
	uint32_t rgba = COL_MENUCONTENT_PLUS_0;
	
	uint8_t a = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
				
	if (color != NULL)
	{
		/*rrggbbaa*/
		if (color[0] == '#')
		{
			uint32_t col = 0;
						
			if (sscanf(color + 1, "%lx", &col) == 1)
			{
				r = (col >> 24)&0xFF; 
				g = (col >> 16)&0xFF;
				b = (col >> 8)&0xFF;
				a = (col)&0xFF;
			}
		}
		else
		{
			if ( strcmp(color, "COL_MAROON_PLUS_0") == 0)
			{
				return COL_MAROON_PLUS_0;
			}
			else if ( strcmp(color, "COL_GREEN_PLUS_0") == 0)
			{
				return COL_GREEN_PLUS_0;
			}
			else if ( strcmp(color, "COL_OLIVE_PLUS_0") == 0)
			{
				return COL_OLIVE_PLUS_0;
			}
			else if ( strcmp(color, "COL_NAVY_PLUS_0") == 0)
			{
				return COL_NAVY_PLUS_0;
			}
			else if ( strcmp(color, "COL_PURPLE_PLUS_0") == 0)
			{
				return COL_PURPLE_PLUS_0;
			}
			else if ( strcmp(color, "COL_TEAL_PLUS_0") == 0)
			{
				return COL_TEAL_PLUS_0;
			}
			else if ( strcmp(color, "COL_NOBEL_PLUS_0") == 0)
			{
				return COL_NOBEL_PLUS_0;
			}
			else if ( strcmp(color, "COL_MATTERHORN_PLUS_0") == 0)
			{
				return COL_MATTERHORN_PLUS_0;
			}
			else if ( strcmp(color, "COL_RED_PLUS_0") == 0)
			{
				return COL_RED_PLUS_0;
			}
			else if ( strcmp(color, "COL_LIME_PLUS_0") == 0)
			{
				return COL_LIME_PLUS_0;
			}
			else if ( strcmp(color, "COL_YELLOW_PLUS_0") == 0)
			{
				return COL_YELLOW_PLUS_0;
			}
			else if ( strcmp(color, "COL_BLUE_PLUS_0") == 0)
			{
				return COL_BLUE_PLUS_0;
			}
			else if ( strcmp(color, "COL_MAGENTA_PLUS_0") == 0)
			{
				return COL_MAGENTA_PLUS_0;
			}
			else if ( strcmp(color, "COL_AQUA_PLUS_0") == 0)
			{
				return COL_AQUA_PLUS_0;
			}
			else if ( strcmp(color, "COL_WHITE_PLUS_0") == 0)
			{
				return COL_WHITE_PLUS_0;
			}
			else if ( strcmp(color, "COL_BLACK_PLUS_0") == 0)
			{
				return COL_BLACK_PLUS_0;
			}
			else if ( strcmp(color, "COL_ORANGE_PLUS_0") == 0)
			{
				return COL_ORANGE_PLUS_0;
			}
			else if ( strcmp(color, "COL_SILVER_PLUS_0") == 0)
			{
				return COL_SILVER_PLUS_0;
			}
			else if ( strcmp(color, "COL_BACKGROUND_PLUS_0") == 0)
			{
				return COL_BACKGROUND_PLUS_0;
			}
			else if ( strcmp(color, "COL_INFOBAR_PLUS_0") == 0)
			{
				return COL_INFOBAR_PLUS_0;
			}
			else if ( strcmp(color, "COL_INFOBAR_SHADOW_PLUS_0") == 0)
			{
				return COL_INFOBAR_SHADOW_PLUS_0;
			}
			else if ( strcmp(color, "COL_INFOBAR_SHADOW_PLUS_1") == 0)
			{
				return COL_INFOBAR_SHADOW_PLUS_1;
			}
			else if ( strcmp(color, "COL_MENUHEAD_PLUS_0") == 0)
			{
				return COL_MENUHEAD_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_0") == 0)
			{
				return COL_MENUCONTENT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_1") == 0)
			{
				return COL_MENUCONTENT_PLUS_1;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_2") == 0)
			{
				return COL_MENUCONTENT_PLUS_2;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_3") == 0)
			{
				return COL_MENUCONTENT_PLUS_3;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_4") == 0)
			{
				return COL_MENUCONTENT_PLUS_4;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_5") == 0)
			{
				return COL_MENUCONTENT_PLUS_5;
			}
			else if ( strcmp(color, "COL_MENUCONTENT_PLUS_6") == 0)
			{
				return COL_MENUCONTENT_PLUS_6;
			}
			else if ( strcmp(color, "COL_MENUCONTENTDARK_PLUS_0") == 0)
			{
				return COL_MENUCONTENTDARK_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENTSELECTED_PLUS_0") == 0)
			{
				return COL_MENUCONTENTSELECTED_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENTSELECTED_PLUS_1") == 0)
			{
				return COL_MENUCONTENTSELECTED_PLUS_1;
			}
			else if ( strcmp(color, "COL_MENUCONTENTINACTIVE_PLUS_0") == 0)
			{
				return COL_MENUCONTENTINACTIVE_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUFOOT_PLUS_0") == 0)
			{
				return COL_MENUFOOT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUHINT_PLUS_0") == 0)
			{
				return COL_MENUHINT_PLUS_0;
			}
		}
	}
				
	rgba = ::rgbaToColor(r, g, b, a);
	
	return rgba;
}

//
uint32_t CNeutrinoApp::convertFontColor(const char* const color)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertFontColor: color: %s\n", color);
	
	uint32_t rgb = COL_MENUCONTENT_TEXT_PLUS_0;
	
	uint8_t a = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
				
	if (color != NULL)
	{
		if (color[0] == '#')
		{
			uint32_t col = 0;
						
			if (sscanf(color + 1, "%lx", &col) == 1)
			{
				r = (col >> 24)&0xFF; 
				g = (col >> 16)&0xFF;
				b = (col >> 8)&0xFF;
				a = (col)&0xFF;
			}
		}
		else
		{
			if ( strcmp(color, "COL_MAROON") == 0)
			{
				return COL_MAROON_PLUS_0;
			}
			else if ( strcmp(color, "COL_GREEN") == 0)
			{
				return COL_GREEN_PLUS_0;
			}
			else if ( strcmp(color, "COL_OLIVE") == 0)
			{
				return COL_OLIVE_PLUS_0;
			}
			else if ( strcmp(color, "COL_NAVY") == 0)
			{
				return COL_NAVY_PLUS_0;
			}
			else if ( strcmp(color, "COL_PURPLE") == 0)
			{
				return COL_PURPLE_PLUS_0;
			}
			else if ( strcmp(color, "COL_TEAL") == 0)
			{
				return COL_TEAL_PLUS_0;
			}
			else if ( strcmp(color, "COL_NOBEL") == 0)
			{
				return COL_NOBEL_PLUS_0;
			}
			else if ( strcmp(color, "COL_MATTERHORN") == 0)
			{
				return COL_MATTERHORN_PLUS_0;
			}
			else if ( strcmp(color, "COL_RED") == 0)
			{
				return COL_RED_PLUS_0;
			}
			else if ( strcmp(color, "COL_LIME") == 0)
			{
				return COL_LIME_PLUS_0;
			}
			else if ( strcmp(color, "COL_YELLOW") == 0)
			{
				return COL_YELLOW_PLUS_0;
			}
			else if ( strcmp(color, "COL_BLUE") == 0)
			{
				return COL_BLUE_PLUS_0;
			}
			else if ( strcmp(color, "COL_MAGENTA") == 0)
			{
				return COL_MAGENTA_PLUS_0;
			}
			else if ( strcmp(color, "COL_AQUA") == 0)
			{
				return COL_AQUA_PLUS_0;
			}
			else if ( strcmp(color, "COL_WHITE") == 0)
			{
				return COL_WHITE_PLUS_0;
			}
			else if ( strcmp(color, "COL_BLACK") == 0)
			{
				return COL_BLACK_PLUS_0;
			}
			else if ( strcmp(color, "COL_ORANGE") == 0)
			{
				return COL_ORANGE_PLUS_0;
			}
			else if ( strcmp(color, "COL_SILVER") == 0)
			{
				return COL_SILVER_PLUS_0;
			}
			else if ( strcmp(color, "COL_BACKGROUND") == 0)
			{
				return COL_BACKGROUND_PLUS_0;
			}
			else if ( strcmp(color, "COL_INFOBAR") == 0)
			{
				return COL_INFOBAR_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUHEAD") == 0)
			{
				return COL_MENUHEAD_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENT") == 0)
			{
				return COL_MENUCONTENT_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENTSELECTED") == 0)
			{
				return COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUCONTENTINACTIVE") == 0)
			{
				return COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUFOOT") == 0)
			{
				return COL_MENUFOOT_TEXT_PLUS_0;
			}
			else if ( strcmp(color, "COL_MENUHINT") == 0)
			{
				return COL_MENUHINT_TEXT_PLUS_0;
			}
		}
	}
				
	rgb = ::rgbaToColor(r, g, b, a);
	
	return rgb;
}

// 
int CNeutrinoApp::convertFontSize(const char * const size)
{
	int fs = SNeutrinoSettings::FONT_TYPE_MENU;
	
	if (size != NULL)
	{
		if ( strcmp(size, "FONT_TYPE_MENU") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_MENU;
		}
		else if ( strcmp(size, "FONT_TYPE_MENU_TITLE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
		}
		else if ( strcmp(size, "FONT_TYPE_MENU_INFO") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_MENU_INFO;
		}
		else if ( strcmp(size, "FONT_TYPE_EPG_TITLE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EPG_TITLE;
		}
		else if ( strcmp(size, "FONT_TYPE_EPG_INFO1") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
		}
		else if ( strcmp(size, "FONT_TYPE_EPG_INFO2") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
		}
		else if ( strcmp(size, "FONT_TYPE_EPG_DATE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EPG_DATE;
		}
		else if ( strcmp(size, "FONT_TYPE_EVENTLIST_TITLE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE;
		}
		else if ( strcmp(size, "FONT_TYPE_EVENTLIST_ITEMLARGE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE;
		}
		else if ( strcmp(size, "FONT_TYPE_EVENTLIST_ITEMSMALL") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL;
		}
		else if ( strcmp(size, "FONT_TYPE_PLUGINLIST_ITEMLARGE") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_PLUGINLIST_ITEMLARGE;
		}
		else if ( strcmp(size, "FONT_TYPE_PLUGINLIST_ITEMSMALL") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_PLUGINLIST_ITEMSMALL;
		}
		else if ( strcmp(size, "FONT_TYPE_CHANNELLIST") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_CHANNELLIST;
		}
		else if ( strcmp(size, "FONT_TYPE_CHANNELLIST_DESCR") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR;
		}
		else if ( strcmp(size, "FONT_TYPE_CHANNELLIST_NUMBER") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER;
		}
		else if ( strcmp(size, "FONT_TYPE_CHANNEL_NUM_ZAP") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP;
		}
		else if ( strcmp(size, "FONT_TYPE_INFOBAR_NUMBER") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER;
		}
		else if ( strcmp(size, "FONT_TYPE_INFOBAR_CHANNAME") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME;
		}
		else if ( strcmp(size, "FONT_TYPE_INFOBAR_INFO") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO;
		}
		else if ( strcmp(size, "FONT_TYPE_INFOBAR_SMALL") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;
		}
		else if ( strcmp(size, "FONT_TYPE_FILEBROWSER_ITEM") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM;
		}
		else if ( strcmp(size, "FONT_TYPE_MENU_TITLE2") == 0)
		{
			fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE2;
		}
	}
	
	return fs;
}

//
int CNeutrinoApp::convertCorner(const char* const corner)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertCorner: corner: %s\n", corner);
	
	int co = CORNER_NONE;
	
	if (corner != NULL)
	{
		if ( strcmp(corner, "CORNER_NONE") == 0)
		{
			co = CORNER_NONE;
		}
		else if ( strcmp(corner, "CORNER_TOP_LEFT") == 0)
		{
			co = CORNER_TOP_LEFT;
		}
		else if ( strcmp(corner, "CORNER_TOP_RIGHT") == 0)
		{
			co = CORNER_TOP_RIGHT;
		}
		else if ( strcmp(corner, "CORNER_TOP") == 0)
		{
			co = CORNER_TOP;
		}
		else if ( strcmp(corner, "CORNER_BOTTOM_LEFT") == 0)
		{
			co = CORNER_BOTTOM_LEFT;
		}
		else if ( strcmp(corner, "CORNER_LEFT") == 0)
		{
			co = CORNER_LEFT;
		}
		else if ( strcmp(corner, "CORNER_BOTTOM_RIGHT") == 0)
		{
			co = CORNER_BOTTOM_RIGHT;
		}
		else if ( strcmp(corner, "CORNER_RIGHT") == 0)
		{
			co = CORNER_RIGHT;
		}
		else if ( strcmp(corner, "CORNER_BOTTOM") == 0)
		{
			co = CORNER_BOTTOM;
		}
		else if ( strcmp(corner, "CORNER_ALL") == 0)
		{
			co = CORNER_ALL;
		}
		else if ( strcmp(corner, "CORNER_BOTH") == 0)
		{
			co = CORNER_BOTH;
		}
	}
	
	return co;
}

//
int CNeutrinoApp::convertRadius(const char* const radius)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertRadius: radius: %s\n", radius);
	
	int ra = NO_RADIUS;
	
	if (radius != NULL)
	{
		if ( strcmp(radius, "NO_RADIUS") == 0)
		{
			ra = NO_RADIUS;
		}
		else if ( strcmp(radius, "RADIUS_SMALL") == 0)
		{
			ra = RADIUS_SMALL;
		}
		else if ( strcmp(radius, "RADIUS_MID") == 0)
		{
			ra = RADIUS_MID;
		}
		else if ( strcmp(radius, "RADIUS_LARGE") == 0)
		{
			ra = RADIUS_LARGE;
		}
		else if ( strcmp(radius, "RADIUS_VERYLARGE") == 0)
		{
			ra = RADIUS_VERYLARGE;
		}
	}
	
	return ra;
}

//
int CNeutrinoApp::convertGradient(const char* const gradient)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertGradient: gradient: %s\n", gradient);
	
	int gr = NOGRADIENT;
	
	if (gradient != NULL)
	{
		if ( strcmp(gradient, "NOGRADIENT") == 0)
		{
			gr = NOGRADIENT;
		}
		else if ( strcmp(gradient, "DARK2LIGHT") == 0)
		{
			gr = DARK2LIGHT;
		}
		else if ( strcmp(gradient, "LIGHT2DARK") == 0)
		{
			gr = LIGHT2DARK;
		}
		else if ( strcmp(gradient, "DARK2LIGHT2DARK") == 0)
		{
			gr = DARK2LIGHT2DARK;
		}
		else if ( strcmp(gradient, "LIGHT2DARK2LIGHT") == 0)
		{
			gr = LIGHT2DARK2LIGHT;
		}
	}
	
	return gr;
}

//
int CNeutrinoApp::convertGradientType(const char* const type)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertGradientType: gradient_type: %s\n", type);
	
	int gr_type = GRADIENT_COLOR2TRANSPARENT;
	
	if (type != NULL)
	{
		if ( strcmp(type, "GRADIENT_COLOR2TRANSPARENT") == 0)
		{
			gr_type = GRADIENT_COLOR2TRANSPARENT;
		}
		else if ( strcmp(type, "GRADIENT_ONECOLOR") == 0)
		{
			gr_type = GRADIENT_ONECOLOR;
		}
		else if ( strcmp(type, "GRADIENT_COLOR2COLOR") == 0)
		{
			gr_type = GRADIENT_COLOR2COLOR;
		}
	}
	
	return gr_type;
}

//
neutrino_msg_t CNeutrinoApp::convertKey(const char * const key)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertKey: key: %s\n", key);
	
	neutrino_msg_t msg = CRCInput::RC_nokey;
	
	if (key != NULL)
	{
		if ( strcmp(key, "RC_nokey") == 0)
		{
			msg = CRCInput::RC_nokey;
		}
		else if ( strcmp(key, "RC_red") == 0)
		{
			msg = CRCInput::RC_red;
		}
		else if ( strcmp(key, "RC_green") == 0)
		{
			msg = CRCInput::RC_green;
		}
		else if ( strcmp(key, "RC_yellow") == 0)
		{
			msg = CRCInput::RC_yellow;
		}
		else if ( strcmp(key, "RC_blue") == 0)
		{
			msg = CRCInput::RC_blue;
		}
		else if ( strcmp(key, "RC_0") == 0)
		{
			msg = CRCInput::RC_0;
		}
		else if ( strcmp(key, "RC_1") == 0)
		{
			msg = CRCInput::RC_1;
		}
		else if ( strcmp(key, "RC_2") == 0)
		{
			msg = CRCInput::RC_2;
		}
		else if ( strcmp(key, "RC_3") == 0)
		{
			msg = CRCInput::RC_3;
		}
		else if ( strcmp(key, "RC_4") == 0)
		{
			msg = CRCInput::RC_4;
		}
		else if ( strcmp(key, "RC_5") == 0)
		{
			msg = CRCInput::RC_5;
		}
		else if ( strcmp(key, "RC_6") == 0)
		{
			msg = CRCInput::RC_6;
		}
		else if ( strcmp(key, "RC_7") == 0)
		{
			msg = CRCInput::RC_7;
		}
		else if ( strcmp(key, "RC_8") == 0)
		{
			msg = CRCInput::RC_8;
		}
		else if ( strcmp(key, "RC_9") == 0)
		{
			msg = CRCInput::RC_9;
		}
		else if ( strcmp(key, "RC_mode") == 0)
		{
			msg = CRCInput::RC_mode;
		}
		else if ( strcmp(key, "RC_standby") == 0)
		{
			msg = CRCInput::RC_standby;
		}
		else if ( strcmp(key, "RC_home") == 0)
		{
			msg = CRCInput::RC_home;
		}
		else if ( strcmp(key, "RC_setup") == 0)
		{
			msg = CRCInput::RC_setup;
		}
		else if ( strcmp(key, "RC_info") == 0)
		{
			msg = CRCInput::RC_info;
		}
		else if ( strcmp(key, "RC_epg") == 0)
		{
			msg = CRCInput::RC_epg;
		}
	}
	
	return msg;
}

//
int CNeutrinoApp::convertButtonMode(const char * const mode)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::convertButtonMode: mode: %s\n", mode);
	
	int bmode = CCButtons::BUTTON_ICON;
	
	if (mode != NULL)
	{
		if ( strcmp(mode, "BUTTON_ICON") == 0)
		{
			bmode = CCButtons::BUTTON_ICON;
		}
		else if ( strcmp(mode, "BUTTON_FRAME") == 0)
		{
			bmode = CCButtons::BUTTON_FRAME;
		}
		else if ( strcmp(mode, "BUTTON_COLOREDFRAME") == 0)
		{
			bmode = CCButtons::BUTTON_COLOREDFRAME;
		}
		else if ( strcmp(mode, "BUTTON_COLOREDLINE") == 0)
		{
			bmode = CCButtons::BUTTON_COLOREDLINE;
		}
	}
	
	return bmode;
}

//
int CNeutrinoApp::convertMenuPosition(const char* const position)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertMenuPosition: position: %s\n", position);
	
	int pos = CWidget::MENU_POSITION_NONE;
	
	if (position != NULL)
	{
		if ( strcmp(position, "MENU_POSITION_NONE") == 0)
		{
			pos = CWidget::MENU_POSITION_NONE;
		}
		else if ( strcmp(position, "MENU_POSITION_LEFT") == 0)
		{
			pos = CWidget::MENU_POSITION_LEFT;
		}
		else if ( strcmp(position, "MENU_POSITION_CENTER") == 0)
		{
			pos = CWidget::MENU_POSITION_CENTER;
		}
		else if ( strcmp(position, "MENU_POSITION_RIGHT") == 0)
		{
			pos = CWidget::MENU_POSITION_RIGHT;
		}
	}
	
	return pos;
}

//
int CNeutrinoApp::convertClistBoxMode(const char * const mode)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertClistBoxMode: mode: %s\n", mode);
	
	int m = ClistBox::MODE_LISTBOX;
	
	if (mode != NULL)
	{
		if ( strcmp(mode, "MODE_LISTBOX") == 0)
		{
			m = ClistBox::MODE_LISTBOX;
		}
		else if ( strcmp(mode, "MODE_MENU") == 0)
		{
			m = ClistBox::MODE_MENU;
		}
		else if ( strcmp(mode, "MODE_SETUP") == 0)
		{
			m = ClistBox::MODE_SETUP;
		}
	}
	
	return m;
}

//
int CNeutrinoApp::convertClistBoxType(const char * const type)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertClistBoxType: type: %s\n", type);
	
	int t = CMenuItem::TYPE_STANDARD;
	
	if (type != NULL)
	{
		if ( strcmp(type, "TYPE_STANDARD") == 0)
		{
			t = CMenuItem::TYPE_STANDARD;
		}
		else if ( strcmp(type, "TYPE_CLASSIC") == 0)
		{
			t = CMenuItem::TYPE_CLASSIC;
		}
		else if ( strcmp(type, "TYPE_EXTENDED") == 0)
		{
			t = CMenuItem::TYPE_EXTENDED;
		}
		else if ( strcmp(type, "TYPE_FRAME") == 0)
		{
			t = CMenuItem::TYPE_FRAME;
		}
	}
	
	return t;
}

//
int CNeutrinoApp::convertItemInfoMode(const char * const mode)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertItemInfoMode: mode: %s\n", mode);
	
	int m = CCItemInfo::ITEMINFO_INFO;
	
	if (mode != NULL)
	{
		if ( strcmp(mode, "ITEMINFO_INFO") == 0)
		{
			m = CCItemInfo::ITEMINFO_INFO;
		}
		else if ( strcmp(mode, "ITEMINFO_HINTITEM") == 0)
		{
			m = CCItemInfo::ITEMINFO_HINTITEM;
		}
		else if ( strcmp(mode, "ITEMINFO_HINTICON") == 0)
		{
			m = CCItemInfo::ITEMINFO_HINTICON;
		}
		else if ( strcmp(mode, "ITEMINFO_ICON") == 0)
		{
			m = CCItemInfo::ITEMINFO_ICON;
		}
		else if ( strcmp(mode, "ITEMINFO_HINT") == 0)
		{
			m = CCItemInfo::ITEMINFO_HINT;
		}
	}
	
	return m;
}

int CNeutrinoApp::convertCMenuItemID(const char * const id)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertCMenuItemID: id: %s\n", id);
	
	int i = CMenuItem::MENUITEM_FORWARDER;
	
	if (id != NULL)
	{
		if ( strcmp(id, "OPTIONCHOOSER") == 0)
		{
			i = CMenuItem::MENUITEM_OPTIONCHOOSER;
		}
		else if ( strcmp(id, "OPTIONNUMBERCHOOSER") == 0)
		{
			i = CMenuItem::MENUITEM_OPTIONNUMBERCHOOSER;
		}
		else if ( strcmp(id, "OPTIONSTRINGCHOOSER") == 0)
		{
			i = CMenuItem::MENUITEM_OPTIONSTRINGCHOOSER;
		}
		else if ( strcmp(id, "SEPARATOR") == 0)
		{
			i = CMenuItem::MENUITEM_SEPARATOR;
		}
		else if ( strcmp(id, "FORWARDER") == 0)
		{
			i = CMenuItem::MENUITEM_FORWARDER;
		}
	}
	
	return i;
}

int CNeutrinoApp::convertCMeuSeparatorType(const char * const type)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertCMenuSeparatorType: id: %s\n", type);
	
	int st = CMenuSeparator::LINE;
	
	if (type != NULL)
	{
		if ( strcmp(type, "EMPTY") == 0)
		{
			st = CMenuSeparator::EMPTY;
		}
		else if ( strcmp(type, "LINE") == 0)
		{
			st = CMenuSeparator::LINE;
		}
		else if ( strcmp(type, "STRING") == 0)
		{
			st = CMenuSeparator::STRING;
		}
		else if ( strcmp(type, "STRING|ALIGN_LEFT") == 0)
		{
			st = CMenuSeparator::STRING | CMenuSeparator::ALIGN_LEFT;
		}
		else if ( strcmp(type, "STRING|ALIGN_RIGHT") == 0)
		{
			st = CMenuSeparator::STRING | CMenuSeparator::ALIGN_RIGHT;
		}
		else if ( strcmp(type, "LINE|STRING") == 0)
		{
			st = CMenuSeparator::LINE | CMenuSeparator::STRING;
		}
		else if ( strcmp(type, "LINE|STRING|ALIGN_LEFT") == 0)
		{
			st = CMenuSeparator::LINE | CMenuSeparator::STRING | CMenuSeparator::ALIGN_LEFT;
		}
		else if ( strcmp(type, "LINE|STRING|ALIGN_RIGHT") == 0)
		{
			st = CMenuSeparator::LINE | CMenuSeparator::STRING | CMenuSeparator::ALIGN_RIGHT;
		}
	}
	
	return st;
}

//
int CNeutrinoApp::convertBorder(const char* const border)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertBorder: value: %s\n", border);
	
	int bor = CComponent::BORDER_NO;
	
	if (border != NULL)
	{
		if ( strcmp(border, "BORDER_NO") == 0)
		{
			bor = CComponent::BORDER_NO;
		}
		else if ( strcmp(border, "BORDER_ALL") == 0)
		{
			bor = CComponent::BORDER_ALL;
		}
		else if ( strcmp(border, "BORDER_LEFTRIGHT") == 0)
		{
			bor = CComponent::BORDER_LEFTRIGHT;
		}
		else if ( strcmp(border, "BORDER_TOPBOTTOM") == 0)
		{
			bor = CComponent::BORDER_TOPBOTTOM;
		}
	}
	
	return bor;
}

//
int CNeutrinoApp::convertBool(const char* const value)
{
	dprintf(DEBUG_DEBUG, "CNeutrinoApp::convertBool: value: %s\n", value);
	
	int val = 0;
	
	if (value != NULL)
	{
		if ( strcmp(value, "false") == 0)
		{
			val = 0;
		}
		else if ( strcmp(value, "true") == 0)
		{
			val = 1;
		}
	}
	
	return val;
}

// ClistBox
void CNeutrinoApp::parseClistBox(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseClistBox:\n");
	
	ClistBox* listBox = NULL;
	CMenuItem* menuItem = NULL;
	
	//
	char* name = NULL;
				
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * gradient = NULL;
	char *corner = NULL;
	char * radius = NULL;
	
	char * listboxtype = NULL;
	char * listboxmode = NULL;
	unsigned int scrollbar = 1;
	
	unsigned int shrink = 0;
				
	// head
	unsigned int painthead = 0;
	unsigned int paintdate = 0;
	const char* title = NULL;
	const char* icon = NULL;
	const char* format = NULL;
	unsigned int halign = 0;
	unsigned int head_line = 0;
	unsigned int head_line_gradient = 0;
				
	// foot
	unsigned int paintfoot = 0;
	unsigned int foot_line = 0;
	unsigned int foot_line_gradient = 0;
				
	// iteminfo
	unsigned int paintiteminfo = 0;
	char * iteminfomode = NULL;		
	unsigned int iteminfoframe = 0;
	unsigned int iteminfo_posx = 0;
	unsigned int iteminfo_posy = 0;
	unsigned int iteminfo_width = 0;
	unsigned int iteminfo_height = 0;
	const char* iteminfo_color = NULL;
	
	// item
	unsigned int itemborder = 0;
	char *itembordercolor = NULL;
	char * itemgradient = NULL;
	unsigned int item2lines = 0;
	
	//
	xmlNodePtr listboxitem_node = NULL;
	xmlNodePtr listboxintegration_node = NULL;
	xmlNodePtr buttonlabel_node = NULL;
				
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//			
	posx = xmlGetSignedNumericAttribute(node, "posx", 0);
	posy = xmlGetSignedNumericAttribute(node, "posy", 0);
	width = xmlGetSignedNumericAttribute(node, "width", 0);
	height = xmlGetSignedNumericAttribute(node, "height", 0);
				
	paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);
	color = xmlGetAttribute(node, (char*)"color");
	gradient = xmlGetAttribute(node, (char *)"gradient");
	corner = xmlGetAttribute(node, (char *)"corner");
	radius = xmlGetAttribute(node, (char *)"radius");
	listboxtype = xmlGetAttribute(node, (char *)"type");
	listboxmode = xmlGetAttribute(node, (char *)"mode");
	scrollbar = xmlGetSignedNumericAttribute(node, "scrollbar", 0);
	shrink = xmlGetSignedNumericAttribute(node, "shrink", 0);
				
	// head
	painthead = xmlGetSignedNumericAttribute(node, "painthead", 0);
	paintdate = xmlGetSignedNumericAttribute(node, "paintdate", 0);
	title = xmlGetAttribute(node, "title");
	icon = xmlGetAttribute(node, "icon");
	//format = xmlGetAttribute(node, (char*)"format"); //FIXME:
	halign = xmlGetSignedNumericAttribute(node, "halign", 0);
	head_line = xmlGetSignedNumericAttribute(node, "headline", 0);
	head_line_gradient = xmlGetSignedNumericAttribute(node, "headlinegradient", 0);
				
	// foot
	paintfoot = xmlGetSignedNumericAttribute(node, "paintfoot", 0);
	foot_line = xmlGetSignedNumericAttribute(node, "footline", 0);
	foot_line_gradient = xmlGetSignedNumericAttribute(node, "footlinegradient", 0);
		
	// item
	itemborder = xmlGetSignedNumericAttribute(node, "itemborder", 0);
	itembordercolor = xmlGetAttribute(node, (char *)"itembordercolor");
	itemgradient = xmlGetAttribute(node, (char *)"itemgradient");
	item2lines = xmlGetSignedNumericAttribute(node, "itemlines", 0);
				
	// iteminfo
	paintiteminfo = xmlGetSignedNumericAttribute(node, "paintiteminfo", 0);
	iteminfomode = xmlGetAttribute(node, (char *)"iteminfomode");
	iteminfoframe = xmlGetSignedNumericAttribute(node, "iteminfoframe", 0);
	iteminfo_posx = xmlGetSignedNumericAttribute(node, "iteminfoposx", 0);
	iteminfo_posy = xmlGetSignedNumericAttribute(node, "iteminfoposy", 0);
	iteminfo_width = xmlGetSignedNumericAttribute(node, "iteminfowidth", 0);
	iteminfo_height = xmlGetSignedNumericAttribute(node, "iteminfoheight", 0);

	iteminfo_color = xmlGetAttribute(node, (char*)"iteminfocolor");
	uint32_t hintColor = COL_MENUCONTENT_PLUS_0;
				
	if (iteminfo_color) hintColor = convertColor(iteminfo_color);
		
	// recalculate posx / posy
	int x = posx;
	int y = posy;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + posx;
		y = widget->getWindowsPos().iY + posy;
				
		if (width > widget->getWindowsPos().iWidth)
			width = widget->getWindowsPos().iWidth;
				
		if (height > widget->getWindowsPos().iHeight)
			height = widget->getWindowsPos().iHeight;
	}
					
	listBox = new ClistBox(x, y, width, height);
		
	listBox->cc_type = CComponent::CC_LISTBOX;
	if (name) listBox->cc_name = name;
		
	//
	int t = CMenuItem::TYPE_STANDARD;
	if (listboxtype) t = convertClistBoxType(listboxtype);
	listBox->setWidgetType(t);
	//
	int m = ClistBox::MODE_LISTBOX;
	if (listboxmode) m = convertClistBoxMode(listboxmode);
	listBox->setWidgetMode(m);
	listBox->paintScrollBar(scrollbar);
	listBox->paintMainFrame(paintframe);
		
	// color
	uint32_t finalColor = COL_MENUCONTENT_PLUS_0;
				
	if (color != NULL) 
	{
		finalColor = convertColor(color);
		listBox->setColor(finalColor);
	}
		
	if (shrink) listBox->enableShrinkMenu();
				
	// head
	if (painthead)
	{
		listBox->enablePaintHead();
		listBox->setTitle(_(title), icon);
		if (paintdate) listBox->enablePaintDate();
		//if (format) listBox->setFormat(format); //FIXME:
		listBox->setTitleHAlign(halign);
		//
		listBox->setHeadLine(head_line, head_line_gradient);
	}
				
	// foot
	if (paintfoot)
	{
		listBox->enablePaintFoot();	
		listBox->setFootLine(foot_line, foot_line_gradient);
	}
				
	// iteminfo
	if (paintiteminfo)
	{
		listBox->enablePaintItemInfo(70);
		int iimode = CCItemInfo::ITEMINFO_INFO;
		if (iteminfomode) iimode = convertItemInfoMode(iteminfomode);
		listBox->setItemInfoMode(iimode);		
		listBox->setItemInfoPos(iteminfo_posx, iteminfo_posy, iteminfo_width, iteminfo_height);
		listBox->paintItemInfoFrame(iteminfoframe);
		if (iteminfo_color) listBox->setItemInfoColor(hintColor);
	}
		
	// item
	// border
	if (itemborder) listBox->setItemBorderMode(itemborder);
	// bordercolor
	uint32_t bColor = COL_MENUCONTENT_PLUS_6;
				
	if (itembordercolor != NULL) 
	{
		bColor = convertColor(itembordercolor);
		listBox->setItemBorderColor(bColor);
	}
	// itemgradient
	int item_gr = NOGRADIENT;
	if (itemgradient) item_gr = convertGradient(itemgradient);
	listBox->setItemGradient(item_gr);
	// item2lines
	if (item2lines) listBox->setItem2Lines();
				
	// ITEM	
	listboxitem_node = node->xmlChildrenNode;
		
	//
	char * item_id = NULL;
						
	char * item_localename = NULL;
	char * item_option = NULL;
	char * item_option_info = NULL;
	char * item_actionkey = NULL;
	char * item_target = NULL;
	char * item_hinticon = NULL;
	char * item_hint = NULL;
	char * item_icon = NULL;
	char * item_directkey = NULL;
	unsigned int item_lines = 0;
	char * item_border = NULL;
	char * item_gradient = NULL;
	char * sep_type = NULL;
					
	while ((listboxitem_node = xmlGetNextOccurence(listboxitem_node, "item")) != NULL) 
	{	
		item_id = xmlGetAttribute(listboxitem_node, (char *)"id");				
		item_localename = xmlGetAttribute(listboxitem_node, (char*)"localename");
		item_option = xmlGetAttribute(listboxitem_node, (char*)"option");
		item_option_info = xmlGetAttribute(listboxitem_node, (char*)"optioninfo");
		item_actionkey = xmlGetAttribute(listboxitem_node, (char*)"actionkey");
		item_target = xmlGetAttribute(listboxitem_node, (char*)"target");
		item_hinticon = xmlGetAttribute(listboxitem_node, (char*)"itemicon");
		item_hint = xmlGetAttribute(listboxitem_node, (char*)"hint");
		item_icon = xmlGetAttribute(listboxitem_node, (char*)"iconname");
		item_directkey = xmlGetAttribute(listboxitem_node, (char*)"directkey");
		item_lines = xmlGetSignedNumericAttribute(listboxitem_node, "lines", 0);
		item_border = xmlGetAttribute(listboxitem_node, (char *)"border");
		item_gradient = xmlGetAttribute(listboxitem_node, (char *)"gradient");
		sep_type = xmlGetAttribute(listboxitem_node, (char *)"type");
						
		CMenuTarget* parent = NULL;
		std::string actionKey = "";
		std::string itemName = "";
		neutrino_msg_t key = CRCInput::RC_nokey;
						
		if (item_localename) itemName = _(item_localename);
						
		//
		int id = CMenuItem::MENUITEM_FORWARDER;
		if (item_id) id = convertCMenuItemID(item_id);
		
		//FIXME: other items	
		if (id == CMenuItem::MENUITEM_FORWARDER)
			menuItem = new CMenuForwarder(itemName.c_str());
		//else if (id == CMenuItem::MENUITEM_OPTIONCHOOSER)
		//	menuItem = new CMenuOptionChooser(itemName.c_str());
		//else if (id == CMenuItem::MENUITEM_OPTIONNUMBERCHOOSER)
		//	menuItem = new CMenuOptionNumberChooser(itemName.c_str());
		//else if (id == CMenuItem::MENUITEM_OPTIONSTRINGCHOOSER)
		//	menuItem = new CMenuOptionStringChooser(itemName.c_str());
		else if (id == CMenuItem::MENUITEM_SEPARATOR)
		{
			int type = CMenuSeparator::LINE;
			
			if (sep_type) type = convertCMeuSeparatorType(sep_type);
			
			menuItem = new CMenuSeparator(type, itemName.c_str());
		}
						
		if (item_actionkey) actionKey = item_actionkey;
			
		if (item_target) parent = convertTarget(item_target);
							
		menuItem->setActionKey(parent, actionKey.c_str());
					
		//	
		if (item_directkey)
		{
			key = convertKey(item_directkey);
			menuItem->setDirectKey(key);
		}
		
		if (item_icon) menuItem->setIconName(item_icon);	
		if (item_hint) menuItem->setHint(item_hint);
		if (item_lines) menuItem->set2lines();
		if (item_option) menuItem->setOption(item_option);
		if (item_option_info) menuItem->setOptionInfo(item_option_info);
		int br = CComponent::BORDER_NO;
		if (item_border) br = convertBorder(item_border);
		menuItem->setBorderMode(br);
		int gr = NOGRADIENT;
		if (item_gradient) gr = convertGradient(item_gradient);
		menuItem->setGradient(gr);
					
		if (item_hinticon)
		{
			std::string filename = CONFIGDIR "/skins/";
			filename += g_settings.preferred_skin;
			filename += "/";
			filename += item_hinticon;
						
			if (file_exists(filename.c_str()))
				menuItem->setHintIcon(filename.c_str());
			else
			{
				menuItem->setHintIcon(item_hinticon);
			}
		}
						
		listBox->addItem(menuItem);
				
		listboxitem_node = listboxitem_node->xmlNextNode;
	}
				
	// INTEGRATION
	listboxintegration_node = node->xmlChildrenNode;
		
	CPlugins::i_type_t integration = CPlugins::I_TYPE_DISABLED;
	unsigned int mode = ClistBox::MODE_MENU;
	unsigned int shortcut = CRCInput::RC_nokey;
	unsigned int type = CMenuItem::TYPE_STANDARD;
	unsigned int i_lines = false;
	unsigned int i_border = false;
					
	while ((listboxintegration_node = xmlGetNextOccurence(listboxintegration_node, "integration")) != NULL) 
	{	
		integration = (CPlugins::i_type_t)xmlGetSignedNumericAttribute(listboxintegration_node, "id", 0);
		mode = xmlGetSignedNumericAttribute(listboxintegration_node, "mode", 0);
		shortcut = xmlGetSignedNumericAttribute(listboxintegration_node, "shortcut", 0);
		type = xmlGetSignedNumericAttribute(listboxintegration_node, "type", 0);
		i_lines = xmlGetSignedNumericAttribute(listboxintegration_node, "lines", 0);
		i_border = xmlGetSignedNumericAttribute(listboxintegration_node, "border", 0);
						
		listBox->integratePlugins(integration, shortcut? shortcut : CRCInput::RC_nokey, true, mode, type, i_lines, i_border);
				
		listboxintegration_node = listboxintegration_node->xmlNextNode;
	}
				
	// BUTTON_LABEL FOOT
	buttonlabel_node = node->xmlChildrenNode;
		
	char* button = NULL;
	char* localename = NULL;
					
	while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "foot_button_label")) != NULL) 
	{	
		button = xmlGetAttribute(buttonlabel_node, (char*)"name");
		localename = xmlGetAttribute(buttonlabel_node, (char*)"localename");
						
		button_label_struct btn;
		btn.button = " ";
		if (button) btn.button = button;
		btn.localename = " ";
		if (localename) btn.localename = localename;
						
		listBox->setFootButtons(&btn);
				
		buttonlabel_node = buttonlabel_node->xmlNextNode;
	}
				
	// BUTTON_LABEL HEAD
	while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "head_button_label")) != NULL) 
	{	
		button = xmlGetAttribute(buttonlabel_node, (char*)"name");
		//localename = xmlGetAttribute(buttonlabel_node, (char*)"localename");
						
		button_label_struct btn;
		btn.button = " ";
		if (button) btn.button = button;
		btn.localename = " ";
		//if (localename) btn.localename = localename;
						
		listBox->setHeadButtons(&btn);
				
		buttonlabel_node = buttonlabel_node->xmlNextNode;
	}
	
	//	
	if (widget) widget->addCCItem(listBox);
}

// CHead
void CNeutrinoApp::parseCHead(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCHead:\n");
	
	CCHeaders* head = NULL;
	
	char* name = NULL;
				
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * gradient = NULL;
	char * gradient_type = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	char* title = NULL;
	unsigned int halign = 0;
	const char* icon = NULL;
	unsigned int head_line = 0;
	unsigned int head_line_gradient = 0;
	
	unsigned int paintdate = 0;
	char* format = NULL;
	
	char *bmode = NULL;
	
	xmlNodePtr buttonlabel_node = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
						
	posx = xmlGetSignedNumericAttribute(node, "posx", 0);
	posy = xmlGetSignedNumericAttribute(node, "posy", 0);
	width = xmlGetSignedNumericAttribute(node, "width", 0);
	height = xmlGetSignedNumericAttribute(node, "height", 0);
				
	paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);		
	color = xmlGetAttribute(node, (char*)"color");
	gradient = xmlGetAttribute(node, (char *)"gradient");
	gradient_type = xmlGetAttribute(node, (char *)"gradienttype");
	corner = xmlGetAttribute(node, (char *)"corner");
	radius = xmlGetAttribute(node, (char *)"radius");
				
	// parse color
	uint32_t finalColor = COL_MENUCONTENT_PLUS_0;		
	if (color) finalColor = convertColor(color);
					
	title = xmlGetAttribute(node, (char*)"title");
	halign = xmlGetSignedNumericAttribute(node, "halign", 0);
	icon = xmlGetAttribute(node, (char*)"icon");
	head_line = xmlGetSignedNumericAttribute(node, "line", 0);
	head_line_gradient = xmlGetSignedNumericAttribute(node, "linegradient", 0);
		
	paintdate = xmlGetSignedNumericAttribute(node, "paintdate", 0);
	format = xmlGetAttribute(node, (char*)"format");
	
	bmode = xmlGetAttribute(node, (char *)"buttonmode");
		
	// recalculate posx / posy
	int x = posx;
	int y = posy;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + posx;
		y = widget->getWindowsPos().iY + posy;
				
		if (width > widget->getWindowsPos().iWidth)
			width = widget->getWindowsPos().iWidth;
				
		if (height > widget->getWindowsPos().iHeight)
			height = widget->getWindowsPos().iHeight;
	}

	head = new CCHeaders(x, y, width, height);
		
	head->cc_type = CComponent::CC_HEAD;
	if (name) head->cc_name = name;
	
	// mainframe	
	head->paintMainFrame(paintframe);
	// title
	if (title != NULL) head->setTitle(_(title));
	head->setHAlign(halign);
	// icon
	if (icon != NULL) head->setIcon(icon);
	// color
	if(color != NULL) head->setColor(finalColor);
	// gradient
	int gr = NOGRADIENT;
	if (gradient) gr = convertGradient(gradient);
	int gt = GRADIENT_COLOR2TRANSPARENT;
	if (gradient_type) gt = convertGradientType(gradient_type);
	head->setGradient(gr, GRADIENT_VERTICAL, INT_LIGHT, gt);
	// corner
	int co = CORNER_NONE;
	int ra = NO_RADIUS;
	if (corner) co = convertCorner(corner);
	if (radius) ra = convertRadius(radius);
	head->setCorner(ra, co);	
	// line
	head->setLine(head_line, head_line_gradient);
	// date	
	if (paintdate) head->enablePaintDate();
	if (format != NULL) head->setFormat(_(format));
	//
	int buttonmode = CCButtons::BUTTON_ICON;
	if (bmode) buttonmode = convertButtonMode(bmode);
	head->setButtonMode(buttonmode);
					
	// BUTTON_LABEL
	buttonlabel_node = node->xmlChildrenNode;
		
	char* button = NULL;
	char* localename = NULL;
					
	while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "button_label")) != NULL) 
	{	
		button = xmlGetAttribute(buttonlabel_node, (char*)"name");
		localename = xmlGetAttribute(buttonlabel_node, (char*)"localename");
						
		button_label_struct btn;
		btn.button = " ";
		if (button) btn.button = button;
		btn.localename = " ";
		if (localename) btn.localename = localename;
						
		head->setButtons(&btn);
				
		buttonlabel_node = buttonlabel_node->xmlNextNode;
	}
	
	//				
	if (widget) widget->addCCItem(head);
}

// CFoot
void CNeutrinoApp::parseCFoot(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCFoot:\n");
	
	CCFooters* foot = NULL;
	
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;
				
	unsigned int paintframe = 1;
	char * color = NULL;
	char * gradient = NULL;
	char * gradient_type = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	unsigned int foot_line = 0;
	unsigned int foot_line_gradient = 0;
	
	char *bmode = NULL;
	
	xmlNodePtr buttonlabel_node = NULL;
	
	//		
	posx = xmlGetSignedNumericAttribute(node, "posx", 0);
	posy = xmlGetSignedNumericAttribute(node, "posy", 0);
	width = xmlGetSignedNumericAttribute(node, "width", 0);
	height = xmlGetSignedNumericAttribute(node, "height", 0);
				
	paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);	
	color = xmlGetAttribute(node, (char*)"color");
	gradient = xmlGetAttribute(node, (char *)"gradient");
	gradient_type = xmlGetAttribute(node, (char *)"gradienttype");
	corner = xmlGetAttribute(node, (char *)"corner");
	radius = xmlGetAttribute(node, (char *)"radius");
	
	bmode = xmlGetAttribute(node, (char *)"buttonmode");
				
	// parse color
	uint32_t finalColor = COL_MENUCONTENT_PLUS_0;		
	if (color) finalColor = convertColor(color);
				
	foot_line = xmlGetSignedNumericAttribute(node, "line", 0);
	foot_line_gradient = xmlGetSignedNumericAttribute(node, "linegradient", 0);
		
	// recalculate posx / posy
	int x = posx;
	int y = posy;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + posx;
		y = widget->getWindowsPos().iY + posy;
				
		if (width > widget->getWindowsPos().iWidth)
			width = widget->getWindowsPos().iWidth;
				
		if (height > widget->getWindowsPos().iHeight)
			height = widget->getWindowsPos().iHeight;
	}
						
	foot = new CCFooters(x, y, width, height);
		
	foot->cc_type = CComponent::CC_FOOT;
		
	foot->paintMainFrame(paintframe);
	// color			
	if (color != NULL) foot->setColor(finalColor);
	// gradient
	int gr = NOGRADIENT;
	if (gradient) gr = convertGradient(gradient);
	int gt = GRADIENT_COLOR2TRANSPARENT;
	if (gradient_type) gt = convertGradientType(gradient_type);
	foot->setGradient(gr, GRADIENT_VERTICAL, INT_LIGHT, gt);
	// corner
	int co = CORNER_NONE;
	int ra = NO_RADIUS;
	if (corner) co = convertCorner(corner);
	if (radius) ra = convertRadius(radius);
	foot->setCorner(ra, co);
	// line
	foot->setLine(foot_line, foot_line_gradient);
	//
	int buttonmode = CCButtons::BUTTON_ICON;
	if (bmode) buttonmode = convertButtonMode(bmode);
	foot->setButtonMode(buttonmode);
					
	// BUTTON_LABEL
	buttonlabel_node = node->xmlChildrenNode;
		
	char *button = NULL;
	char *localename = NULL;
	char *bcolor = NULL;
					
	while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "button_label")) != NULL) 
	{	
		button = xmlGetAttribute(buttonlabel_node, (char*)"name");
		localename = xmlGetAttribute(buttonlabel_node, (char*)"localename");
		bcolor = xmlGetAttribute(buttonlabel_node, (char*)"color");
						
		button_label_struct btn;
		btn.button = " ";
		btn.localename = " ";
		btn.color = 0;
		
		if (button) btn.button = button;
		if (localename) btn.localename = localename;
		//
		fb_pixel_t col = COL_BACKGROUND_PLUS_0;
		if (bcolor) col = convertColor(bcolor);
		btn.color = col;
						
		foot->setButtons(&btn);
				
		buttonlabel_node = buttonlabel_node->xmlNextNode;
	}
	
	//				
	if (widget) widget->addCCItem(foot);
}

// CTextBox
void CNeutrinoApp::parseCTextBox(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCTextBox:\n");
	
	CTextBox* textBox = NULL;
	
	char* name = NULL;
	
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;
				
	unsigned int paintframe = 1;
	char * color = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	char * textColor = NULL;
	char * font = NULL;
	unsigned int mode = CTextBox::SCROLL;
	unsigned int border = CComponent::BORDER_NO;
	
	unsigned int tmode = CTextBox::PIC_RIGHT;
	unsigned int tw = 0;
	unsigned int th = 0;
	unsigned int tframe = 0;
	
	char * text = NULL;
	char * pic = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//		
	posx = xmlGetSignedNumericAttribute(node, "posx", 0);
	posy = xmlGetSignedNumericAttribute(node, "posy", 0);
	width = xmlGetSignedNumericAttribute(node, "width", 0);
	height = xmlGetSignedNumericAttribute(node, "height", 0);
				
	paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);		
	color = xmlGetAttribute(node, (char*)"color");
	corner = xmlGetAttribute(node, (char *)"corner");
	radius = xmlGetAttribute(node, (char *)"radius");
		
	textColor = xmlGetAttribute(node, (char*)"textcolor");
	font = xmlGetAttribute(node, (char *)"font");
		
	mode = xmlGetSignedNumericAttribute(node, "mode", 0);
	border = xmlGetSignedNumericAttribute(node, "border", 0);
		
	tmode = xmlGetSignedNumericAttribute(node, "tmode", 0);
	tw = xmlGetSignedNumericAttribute(node, "twidth", 0);
	th = xmlGetSignedNumericAttribute(node, "theight", 0);
	tframe = xmlGetSignedNumericAttribute(node, "tframe", 0);
		
	text = xmlGetAttribute(node, (char*)"text");
	pic = xmlGetAttribute(node, (char*)"pic");
		
	// recalculate posx / posy
	int x = posx;
	int y = posy;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + posx;
		y = widget->getWindowsPos().iY + posy;
				
		if (width > widget->getWindowsPos().iWidth)
			width = widget->getWindowsPos().iWidth;
				
		if (height > widget->getWindowsPos().iHeight)
			height = widget->getWindowsPos().iHeight;
	}
						
	textBox = new CTextBox(x, y, width, height);
		
	textBox->cc_type = CComponent::CC_TEXTBOX;
	if (name) textBox->cc_name = name;
	// color
	uint32_t finalColor = COL_MENUCONTENT_PLUS_0;			
	if (color != NULL) 
	{
		finalColor = convertColor(color);
		textBox->setBackgroundColor(finalColor);
	}
		
	//
	int co = CORNER_NONE;
	int ra = NO_RADIUS;
	if (corner) co = convertCorner(corner);
	if (radius) ra = convertRadius(radius);
	textBox->setCorner(ra, co);
	textBox->paintMainFrame(paintframe);
					
	textBox->setTextColor(textColor? convertFontColor(textColor) : COL_MENUCONTENT_TEXT_PLUS_0);
	//
	int fs = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	if (font) fs = convertFontSize(font);
	textBox->setFont(fs);
	//
	textBox->setMode(mode);
	
	//FIXME:	
	if (text) textBox->setText(text, pic? pic : NULL, tw, th, tmode, tframe);
	
	//				
	if (widget) widget->addCCItem(textBox);
}

// CWindow
void CNeutrinoApp::parseCCWindow(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCWindow\n");
	
	CCWindow* window = NULL;
	
	char* name = NULL;
				
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * corner = NULL;
	char * radius = NULL;
	unsigned int border = 0;
	char *bordercolor = NULL;
	char *gradient = NULL;
	unsigned int gradient_direction = GRADIENT_VERTICAL;
	unsigned int gradient_intensity = INT_LIGHT;
	char * gradient_type = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//		
	posx = xmlGetSignedNumericAttribute(node, "posx", 0);
	posy = xmlGetSignedNumericAttribute(node, "posy", 0);
	width = xmlGetSignedNumericAttribute(node, "width", 0);
	height = xmlGetSignedNumericAttribute(node, "height", 0);
				
	paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);		
	color = xmlGetAttribute(node, (char*)"color");

	corner = xmlGetAttribute(node, (char *)"corner");
	radius = xmlGetAttribute(node, (char *)"radius");
	border = xmlGetSignedNumericAttribute(node, "border", 0);
	bordercolor = xmlGetAttribute(node, (char *)"bordercolor");
		
	gradient = xmlGetAttribute(node, (char *)"gradient");
	gradient_direction = xmlGetSignedNumericAttribute(node, "gradientdirection", 0);
	gradient_intensity = xmlGetSignedNumericAttribute(node, "gradientintensity", 0);
	gradient_type = xmlGetAttribute(node, (char *)"type");
		
	// recalculate posx / posy
	int x = posx;
	int y = posy;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + posx;
		y = widget->getWindowsPos().iY + posy;
				
		if (width > widget->getWindowsPos().iWidth)
			width = widget->getWindowsPos().iWidth;
				
		if (height > widget->getWindowsPos().iHeight)
			height = widget->getWindowsPos().iHeight;
	}
					
	window = new CCWindow(x, y, width, height);
		
	window->cc_type = CComponent::CC_WINDOW;
	if (name) window->cc_name = name;
					
	window->paintMainFrame(paintframe);
		
	// color
	uint32_t finalColor = COL_MENUCONTENT_PLUS_0;
				
	if (color != NULL) 
	{
		finalColor = convertColor(color);
		window->setColor(finalColor);
	}
		
	// corner/radius
	int co = CORNER_NONE;
	int ra = NO_RADIUS;
	if (corner) co = convertCorner(corner);
	if (radius) ra = convertRadius(radius);
	window->setCorner(ra, co);

	// border
	window->setBorderMode(border);
		
	// bordercolor
	fb_pixel_t bColor = COL_INFOBAR_SHADOW_PLUS_0;	
	if (bordercolor != NULL)
	{ 
		bColor = convertColor(bordercolor);
		window->setBorderColor(bColor);
	}

	// gradient
	int gr = NOGRADIENT;
	if (gradient) gr = convertGradient(gradient);
	int gt = GRADIENT_COLOR2TRANSPARENT;
	if (gradient_type) gt = convertGradientType(gradient_type);
	
	window->setGradient(gr, gradient_direction, gradient_intensity, gt);
	
	//				
	if (widget) widget->addCCItem(window);
}

// CCLabel
void CNeutrinoApp::parseCCLabel(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCLabel:\n");
	
	CCLabel* label = NULL;
	
	//
	char* name = NULL;
				
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
						
	unsigned int cc_refresh = 0;
	unsigned int l_halign = 0;
	
	//int font_size = -1;
	char * font_size = NULL;
	char* font_color = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//						
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
						
	cc_refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
		
	//
	font_size = xmlGetAttribute(node, (char *)"font");
	font_color = xmlGetAttribute(node, (char*)"fontcolor");
		
	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
		
	if (font_color) color = convertFontColor(font_color);
						
	std::string text = "";
							
	text = xmlGetAttribute(node, (char*)"text");
	l_halign = xmlGetSignedNumericAttribute(node, "halign", 0);
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
							
	label = new CCLabel(x, y, cc_dx, cc_dy);
		
	label->cc_type = CComponent::CC_LABEL;
	if (name) label->cc_name = name;
							
	if (!text.empty()) label->setText(_(text.c_str()));
	label->setHAlign(l_halign);
	//if (font_size) label->setFont(font_size);
	int fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	if (font_size) fs = convertFontSize(font_size);
	label->setFont(fs);
	//
	if (font_color) label->setColor(color);
	
	//						
	if (widget) widget->addCCItem(label);
}

// CCImage
void CNeutrinoApp::parseCCImage(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCImage:\n");
	
	CCImage* pic = NULL;
	
	char *name = NULL;
				
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	char* image = NULL;
						
	unsigned int cc_refresh = 0;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//				
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
						
	cc_refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
				
	image = xmlGetAttribute(node, (char*)"image");
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
							
	pic = new CCImage(x, y, cc_dx, cc_dy);
		
	pic->cc_type = CComponent::CC_IMAGE;
	if (name) pic->cc_name = name;
							
	if (image != NULL)
	{
		std::string filename = CONFIGDIR "/skins/";
		filename += g_settings.preferred_skin;
		filename += "/";
		filename += image;
					
		if (file_exists(filename.c_str()))
			pic->setImage(filename.c_str());
		else
			pic->setImage(image);
	}
							
	if (widget) widget->addCCItem(pic);	
}

// CCTime
void CNeutrinoApp::parseCCTime(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCTime:\n");
	
	CCTime* time = NULL;
	
	char *name = NULL;
				
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	//int font_size = -1;
	char * font_size = NULL;
	char* font_color = NULL;
						
	unsigned int cc_refresh = 0;
	
	char* cc_format = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//				
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
	//
	font_size = xmlGetAttribute(node, (char *)"font");
	font_color = xmlGetAttribute(node, (char*)"fontcolor");
		
	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
		
	if (font_color) color = convertFontColor(font_color);
						
	cc_refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
					
	cc_format = xmlGetAttribute(node, (char*)"format");
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
							
	time = new CCTime(x, y, cc_dx, cc_dy);
		
	time->cc_type = CComponent::CC_TIME;
	if (name) time->cc_name = name;
							
	if (cc_format != NULL) time->setFormat(_(cc_format));
	if (cc_refresh) time->enableRepaint();
	//
	int fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	if (font_size) fs = convertFontSize(font_size);
	time->setFont(fs);
	//
	if (font_color) time->setColor(color);
	
	//						
	if (widget) widget->addCCItem(time);
}

// CCButton
void CNeutrinoApp::parseCCButtons(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCButtons:\n");
	
	CCButtons* cButton = NULL;
	
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	unsigned int head = 0;
	char * mode = NULL;
	
	xmlNodePtr buttonlabel_node = NULL;
	
	//
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
	//
	head = xmlGetSignedNumericAttribute(node, "head", 0);
	mode = xmlGetAttribute(node, (char *)"buttonmode");
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
				
	cButton = new CCButtons(x, y, cc_dx, cc_dy);
	
	cButton->cc_type = CComponent::CC_BUTTON;
	
	//
	int bmode = CCButtons::BUTTON_ICON;
	if (mode) bmode = convertButtonMode(mode);	
	cButton->setButtonMode(bmode);
				
	// BUTTON_LABEL
	buttonlabel_node = node->xmlChildrenNode;
		
	char *button = NULL;
	char *localename = NULL;
	char *color = NULL;
				
	while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "button_label")) != NULL) 
	{		
		button = xmlGetAttribute(buttonlabel_node, (char *)"name");
		localename = xmlGetAttribute(buttonlabel_node, (char *)"localename");
		color = xmlGetAttribute(buttonlabel_node, (char *)"color");
							
		button_label_struct btn;
		btn.button = " ";
		btn.localename = " ";
		btn.color = COL_BACKGROUND_PLUS_0;
			
		//
		if (button) btn.button = button;
		if (localename) btn.localename = localename;
		fb_pixel_t col = COL_BACKGROUND_PLUS_0;
		if (color) col = convertColor(color);
		btn.color = col;
							
		cButton->setButtons(&btn, 1, head);
					
		buttonlabel_node = buttonlabel_node->xmlNextNode;
	}
	
	//				
	if (widget) widget->addCCItem(cButton);
}

// CCHline
void CNeutrinoApp::parseCCHline(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCHline:\n");
	
	CCHline* hline = NULL;
	
	char *name = NULL;
	
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	//
	char * gradient = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
	gradient = xmlGetAttribute(node, (char *)"gradient");
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
				
	hline = new CCHline(x, y, cc_dx, cc_dy);
		
	hline->cc_type = CComponent::CC_HLINE;
	if (name) hline->cc_name = name;
		
	int gr = NOGRADIENT;
	if (gradient) gr = convertGradient(gradient);
	hline->setGradient(gr);
	
	//				
	if (widget) widget->addCCItem(hline);
}

// CCVline
void CNeutrinoApp::parseCCVline(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCVline:\n");
	
	CCVline* vline = NULL;
	
	char *name = NULL;
	
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	char *gradient = NULL;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
	gradient = xmlGetAttribute(node, (char *)"gradient");
				
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
		
	vline = new CCVline(x, y, cc_dx, cc_dy);
		
	vline->cc_type = CComponent::CC_VLINE;
	if (name) vline->cc_name = name;
		
	int gr = NOGRADIENT;
	if (gradient) gr = convertGradient(gradient);
	vline->setGradient(gr);
	
	//				
	if (widget) widget->addCCItem(vline);
}

// CCPig
void CNeutrinoApp::parseCCPig(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCPig:\n");
	
	CCPig* pig = NULL;
	
	char *name = NULL;
	
	int cc_x = 0;
	int cc_y = 0;
	int cc_dx = 0;
	int cc_dy = 0;
	
	//
	name = xmlGetAttribute(node, (char*)"name");
		
	//
	cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
	cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
	cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
	cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
	// recalculate posx / posy
	int x = cc_x;
	int y = cc_y;
		
	if (widget)
	{			
		x = widget->getWindowsPos().iX + cc_x;
		y = widget->getWindowsPos().iY + cc_y;
				
		if (cc_dx > widget->getWindowsPos().iWidth)
			cc_dx = widget->getWindowsPos().iWidth;
				
		if (cc_dy > widget->getWindowsPos().iHeight)
			cc_dy = widget->getWindowsPos().iHeight;
	}
				
	pig = new CCPig(x, y, cc_dx, cc_dy);
		
	pig->cc_type = CComponent::CC_PIG;
	if (name) pig->cc_name = name;
	
	//				
	if (widget) widget->addCCItem(pig);
}

// widget key
void CNeutrinoApp::parseKey(xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseKey:\n");
	
	char* key_name = NULL;
	char* key_actionkey = NULL;
	char *key_target = NULL;
	
	key_name = xmlGetAttribute(node, (char*)"name");
	key_actionkey = xmlGetAttribute(node, (char*)"actionkey");
	key_target = xmlGetAttribute(node, (char*)"target");
		
	neutrino_msg_t key = CRCInput::RC_nokey;		
	CMenuTarget* key_parent = NULL;
		
	if (key_name) key = convertKey(key_name);		
	if (key_target) key_parent = convertTarget(key_target);
	
	//			
	if (widget) widget->addKey(key, key_parent, key_actionkey);		
}

/*
void CNeutrinoApp::parseCWidget(xmlNodePtr node)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCWidget:\n");
	
	CWidget *widget = NULL;
	
	//
	char* name = NULL;
			
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int dx = 0;
	unsigned int dy = 0;
			
	char* color = NULL;
	char *gradient = NULL;
	char *gradient_type = NULL;
	char *gradient_direction = NULL;
	char *gradient_intensity = NULL;
	char * corner = NULL;
	char * radius = NULL;
	char * border = NULL;
	char *bordercolor = NULL;
			
	unsigned int paintframe = 0;
	unsigned int savescreen = 0;
	unsigned int timeout = 300;
	char *position = NULL;
	
	//
	xmlNodePtr item = NULL;
}
*/

// getWidget
CWidget *CNeutrinoApp::getWidget(const char * const widgetname, const char *const skinfilename, bool data)
{
	dprintf(DEBUG_NORMAL, ANSI_GREEN"\nCNeutrinoApp::getWidget: <<%s>>\n", widgetname);
	
	//
	CWidget *widget = NULL;
	
	//
	char* name = NULL;
			
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int dx = 0;
	unsigned int dy = 0;
			
	char* color = NULL;
	char *gradient = NULL;
	char *gradient_type = NULL;
	char *gradient_direction = NULL;
	char *gradient_intensity = NULL;
	char * corner = NULL;
	char * radius = NULL;
	char * border = NULL;
	char *bordercolor = NULL;
			
	unsigned int paintframe = 0;
	unsigned int savescreen = 0;
	unsigned int timeout = 300;
	char *position = NULL;
	
	//
	xmlDocPtr parser = NULL;
	xmlNodePtr item = NULL;
	
	//
	if (skinfilename != NULL)
	{
		if (data)
		{
			parser = parseXml(skinfilename);
		}
		else
		{
			parser = parseXmlFile(skinfilename);
		}
	}
	else
	{
		std::string filename = CONFIGDIR "/skins/";
		filename += g_settings.preferred_skin.c_str();
		filename += "/skin.xml";
		
		if (data)
		{
			parser = parseXml(filename.c_str());
		}
		else
		{
			parser = parseXmlFile(filename.c_str());
		}
	}

	//	
	if (parser)
	{
		xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
			
		while ( (search = xmlGetNextOccurence(search, "screen")) != NULL ) 
		{
			//
			name = xmlGetAttribute(search, (char*)"name");
			
			//
			if ( !strcmp(name, widgetname) )
			{	
				x = xmlGetSignedNumericAttribute(search, "posx", 0);
				y = xmlGetSignedNumericAttribute(search, "posy", 0);
				dx = xmlGetSignedNumericAttribute(search, "width", 0);
				dy = xmlGetSignedNumericAttribute(search, "height", 0);
					
				color = xmlGetAttribute(search, (char*)"color");
				gradient = xmlGetAttribute(search, (char *)"gradient");
				gradient_type = xmlGetAttribute(search, (char *)"gradienttype");
				gradient_direction = xmlGetAttribute(search, (char *)"gradientdirection");
				gradient_intensity = xmlGetAttribute(search, (char *)"gradientintensity");
				corner = xmlGetAttribute(search, (char *)"corner");
				radius = xmlGetAttribute(search, (char *)"radius");
				
				border = xmlGetAttribute(search, (char *)"border");
				bordercolor = xmlGetAttribute(search, (char*)"bordercolor");
					
				paintframe = xmlGetSignedNumericAttribute(search, "paintframe", 0);
				savescreen = xmlGetSignedNumericAttribute(search, "savescreen", 0);
				if (xmlGetAttribute(search, (char *)"timeout"))
				{
					timeout = xmlGetSignedNumericAttribute(search, "timeout", 0);
				}
				position = xmlGetAttribute(search, (char *)"position");
					
				//
				widget = new CWidget(x, y, dx, dy);
					
				if (name != NULL) widget->name = name;
				
				// paintmainframe
				widget->paintMainFrame(paintframe);
				
				// color
				fb_pixel_t wColor = COL_MENUCONTENT_PLUS_0;	
				if (color != NULL)
				{ 
					wColor = convertColor(color);
					widget->setColor(wColor);
				}
				
				// gradient
				int gr = NOGRADIENT;
				if (gradient) gr = convertGradient(gradient);
				int gt = GRADIENT_COLOR2TRANSPARENT;
				if (gradient_type) gt = convertGradientType(gradient_type);
				int gd = GRADIENT_VERTICAL;
				//if (gradient_direction) gd = convertGradientDirection(gradient_direction);
				int gi = INT_LIGHT;
				//if (gradient_intensity) gi = convertGradientIntensity(gradient_intensity);
				
				widget->setGradient(gr, gd, gi, gt);

				// corner / radius
				int co = CORNER_NONE;
				int ra = NO_RADIUS;
				if (corner) co = convertCorner(corner);
				if (radius) ra = convertRadius(radius);
				widget->setCorner(ra, co);
				
				// border
				int br = CComponent::BORDER_NO;
				if (border) br = convertBorder(border);
				widget->setBorderMode(br);
				
				// bordercolor
				fb_pixel_t bColor = COL_MENUCONTENT_PLUS_6;	
				if (bordercolor != NULL)
				{ 
					bColor = convertColor(bordercolor);
					widget->setBorderColor(bColor);
				}
				
				// saveScreen
				if (savescreen) widget->enableSaveScreen();
				
				// timeout
				widget->setTimeOut(timeout);
				
				// menuposition
				int pos = CWidget::MENU_POSITION_NONE;
				if (position) pos = convertMenuPosition(position);
				widget->setMenuPosition(pos);
				
				//
				item = search->xmlChildrenNode;
				
				while (item)
				{
					if ( !(strcmp(xmlGetName(item), "head")))	
					{
						parseCHead(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "foot"))) 	
					{
						parseCFoot(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "listbox"))) 
					{
						parseClistBox(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "textbox")))
					{
						parseCTextBox(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "window")))
					{
						parseCCWindow(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "label")))
					{
						parseCCLabel(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "image")))
					{
						parseCCImage(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "time")))
					{
						parseCCTime(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "button")))
					{
						parseCCButtons(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "hline")))
					{
						parseCCHline(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "vline")))
					{
						parseCCVline(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "pig")))
					{
						parseCCPig(item, widget);
					}
					else if ( !(strcmp(xmlGetName(item), "key")))
					{
						parseKey(item, widget);
					}
					
					item = item->xmlNextNode;
				}
			}
							
			//
			search = search->xmlNextNode;		
		}
		
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	return widget;
}

//
void CNeutrinoApp::loadSkin(std::string skinName)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::loadSkin: %s\n", skinName.c_str());
	
	std::string skinPath = CONFIGDIR "/skins/";
	skinPath += skinName.c_str();
	
	// read skin config
	std::string skinConfigFile = skinPath.c_str();
	skinConfigFile += "/";
	skinConfigFile += skinName.c_str();
	skinConfigFile += ".config";
	
	readSkinConfig(skinConfigFile.c_str());
	
	// read skin font/icons/buttons/hints
	std::string fontFileName;
		
	struct dirent **namelist;
	int i = 0;
		
	// setup font
	std::string fontPath = skinPath.c_str();
	fontPath += "/fonts";
			
	i = scandir(fontPath.c_str(), &namelist, 0, 0);

	if (i > 0)
	{
		while(i--)
		{
			if( (strcmp(namelist[i]->d_name, ".") != 0) && (strcmp(namelist[i]->d_name, "..") != 0) )
			{
				std::string filename = fontPath.c_str();
				filename += "/";
				filename += namelist[i]->d_name;
						
				std::string extension = getFileExt(filename);
							
				if ( strcasecmp("ttf", extension.c_str()) == 0)
					fontFileName = filename;
						
				filename.clear();			
			}
			free(namelist[i]);
		}
		free(namelist);
		
		strcpy( g_settings.font_file, fontFileName.c_str() );
			
		CNeutrinoApp::getInstance()->setupFonts(g_settings.font_file);
	}
			
	// setIconPath
	std::string iconsDir = CONFIGDIR "/skins/";
	iconsDir += skinName.c_str();
	iconsDir += "/icons/";
			
	// check if not empty
	i = scandir(iconsDir.c_str(), &namelist, 0, 0);
	if(i < 0)
	{
		frameBuffer->setIconBasePath(DATADIR "/icons/");
	}
	else
	{
		frameBuffer->setIconBasePath(iconsDir);
		free(namelist);
	}
			
	// setButtonPath
	std::string buttonsDir = CONFIGDIR "/skins/";
	buttonsDir += skinName.c_str();
	buttonsDir += "/buttons/";
			
	// check if not empty
	i = scandir(buttonsDir.c_str(), &namelist, 0, 0);
	if(i < 0)
	{
		frameBuffer->setButtonBasePath(DATADIR "/buttons/");
	}
	else
	{
		frameBuffer->setButtonBasePath(buttonsDir);
		free(namelist);
	}
			
	// setHintPath
	std::string hintsDir = CONFIGDIR "/skins/";
	hintsDir += skinName.c_str();
	hintsDir += "/hints/";
			
	// check if not empty
	i = scandir(hintsDir.c_str(), &namelist, 0, 0);
	if(i < 0)
	{
		frameBuffer->setHintBasePath(DATADIR "/hints/");
	}
	else
	{
		frameBuffer->setHintBasePath(hintsDir);
		free(namelist);
	}
			
	// setSpinnerPath
	std::string spinnerDir = CONFIGDIR "/skins/";
	spinnerDir += skinName.c_str();
	spinnerDir += "/spinner/";
			
	// check if not empty
	i = scandir(spinnerDir.c_str(), &namelist, 0, 0);
	if(i < 0)
	{
		frameBuffer->setSpinnerBasePath(DATADIR "/spinner/");
	}
	else
	{
		frameBuffer->setSpinnerBasePath(spinnerDir);
		free(namelist);
	}
}

//
void CNeutrinoApp::readSkinConfig(const char* const filename)
{
	dprintf(DEBUG_NORMAL, "CNeutrinpApp::readSkinConfig %s\n", filename);
	
	CConfigFile* skinConfig = new CConfigFile(',');
	
	if(skinConfig->loadConfig(filename))
	{
		g_settings.menu_Head_alpha = skinConfig->getInt32( "menu_Head_alpha", 85);
		g_settings.menu_Head_red = skinConfig->getInt32( "menu_Head_red", 15);
		g_settings.menu_Head_green = skinConfig->getInt32( "menu_Head_green", 15);
		g_settings.menu_Head_blue = skinConfig->getInt32( "menu_Head_blue", 15);

		g_settings.menu_Head_Text_alpha = skinConfig->getInt32( "menu_Head_Text_alpha", 0);
		g_settings.menu_Head_Text_red = skinConfig->getInt32( "menu_Head_Text_red", 100 );
		g_settings.menu_Head_Text_green = skinConfig->getInt32( "menu_Head_Text_green", 100 );
		g_settings.menu_Head_Text_blue = skinConfig->getInt32( "menu_Head_Text_blue", 100 );
	
		g_settings.menu_Content_alpha = skinConfig->getInt32( "menu_Content_alpha", 80);
		g_settings.menu_Content_red = skinConfig->getInt32( "menu_Content_red", 25);
		g_settings.menu_Content_green = skinConfig->getInt32( "menu_Content_green", 25);
		g_settings.menu_Content_blue = skinConfig->getInt32( "menu_Content_blue", 25);
		
		g_settings.menu_Content_Text_alpha = skinConfig->getInt32( "menu_Content_Text_alpha", 0);
		g_settings.menu_Content_Text_red = skinConfig->getInt32( "menu_Content_Text_red", 85 );
		g_settings.menu_Content_Text_green = skinConfig->getInt32( "menu_Content_Text_green", 85 );
		g_settings.menu_Content_Text_blue = skinConfig->getInt32( "menu_Content_Text_blue", 85 );
	
		g_settings.menu_Content_Selected_alpha = skinConfig->getInt32( "menu_Content_Selected_alpha", 80);
		g_settings.menu_Content_Selected_red = skinConfig->getInt32( "menu_Content_Selected_red", 75);
		g_settings.menu_Content_Selected_green = skinConfig->getInt32( "menu_Content_Selected_green", 75);
		g_settings.menu_Content_Selected_blue = skinConfig->getInt32( "menu_Content_Selected_blue", 75);
		
		g_settings.menu_Content_Selected_Text_alpha = skinConfig->getInt32( "menu_Content_Selected_Text_alpha", 0);
		g_settings.menu_Content_Selected_Text_red = skinConfig->getInt32( "menu_Content_Selected_Text_red", 25 );
		g_settings.menu_Content_Selected_Text_green = skinConfig->getInt32( "menu_Content_Selected_Text_green", 25 );
		g_settings.menu_Content_Selected_Text_blue = skinConfig->getInt32( "menu_Content_Selected_Text_blue", 25 );
	
		g_settings.menu_Content_inactive_alpha = skinConfig->getInt32( "menu_Content_inactive_alpha", 80);
		g_settings.menu_Content_inactive_red = skinConfig->getInt32( "menu_Content_inactive_red", 25);
		g_settings.menu_Content_inactive_green = skinConfig->getInt32( "menu_Content_inactive_green", 25);
		g_settings.menu_Content_inactive_blue = skinConfig->getInt32( "menu_Content_inactive_blue", 25);
		
		g_settings.menu_Content_inactive_Text_alpha = skinConfig->getInt32( "menu_Content_inactive_Text_alpha", 0);
		g_settings.menu_Content_inactive_Text_red = skinConfig->getInt32( "menu_Content_inactive_Text_red", 55);
		g_settings.menu_Content_inactive_Text_green = skinConfig->getInt32( "menu_Content_inactive_Text_green", 55);
		g_settings.menu_Content_inactive_Text_blue = skinConfig->getInt32( "menu_Content_inactive_Text_blue", 55);

		g_settings.infobar_alpha = skinConfig->getInt32( "infobar_alpha", 80);
		g_settings.infobar_red = skinConfig->getInt32( "infobar_red", 25);
		g_settings.infobar_green = skinConfig->getInt32( "infobar_green", 25);
		g_settings.infobar_blue = skinConfig->getInt32( "infobar_blue", 25);
		
		g_settings.infobar_Text_alpha = skinConfig->getInt32( "infobar_Text_alpha", 0);
		g_settings.infobar_Text_red = skinConfig->getInt32( "infobar_Text_red", 100);
		g_settings.infobar_Text_green = skinConfig->getInt32( "infobar_Text_green", 100);
		g_settings.infobar_Text_blue = skinConfig->getInt32( "infobar_Text_blue", 100);
		
		g_settings.infobar_colored_events_alpha = skinConfig->getInt32( "infobar_colored_events_alpha", 100);
		g_settings.infobar_colored_events_red = skinConfig->getInt32( "infobar_colored_events_red", 95);
		g_settings.infobar_colored_events_green = skinConfig->getInt32( "infobar_colored_events_green", 70);
		g_settings.infobar_colored_events_blue = skinConfig->getInt32( "infobar_colored_events_blue", 0);
	
		g_settings.menu_Foot_alpha = skinConfig->getInt32( "menu_Foot_alpha", 85);
		g_settings.menu_Foot_red = skinConfig->getInt32( "menu_Foot_red", 15);
		g_settings.menu_Foot_green = skinConfig->getInt32( "menu_Foot_green", 15);
		g_settings.menu_Foot_blue = skinConfig->getInt32( "menu_Foot_blue", 15);
		
		g_settings.menu_Foot_Text_alpha = skinConfig->getInt32( "menu_Foot_Text_alpha", 0);
		g_settings.menu_Foot_Text_red = skinConfig->getInt32( "menu_Foot_Text_red", 100);
		g_settings.menu_Foot_Text_green = skinConfig->getInt32( "menu_Foot_Text_green", 100);
		g_settings.menu_Foot_Text_blue = skinConfig->getInt32( "menu_Foot_Text_blue", 100);

		g_settings.menu_Hint_alpha = skinConfig->getInt32( "menu_Hint_alpha", 100);
		g_settings.menu_Hint_red = skinConfig->getInt32( "menu_Hint_red", 15);
		g_settings.menu_Hint_green = skinConfig->getInt32( "menu_Hint_green", 15);
		g_settings.menu_Hint_blue = skinConfig->getInt32( "menu_Hint_blue", 15);
		
		g_settings.menu_Hint_Text_alpha = skinConfig->getInt32( "menu_Hint_Text_alpha", 0);
		g_settings.menu_Hint_Text_red = skinConfig->getInt32( "menu_Hint_Text_red", 85);
		g_settings.menu_Hint_Text_green = skinConfig->getInt32( "menu_Hint_Text_green", 85);
		g_settings.menu_Hint_Text_blue = skinConfig->getInt32( "menu_Hint_Text_blue", 85);
		
		// head
		g_settings.Head_gradient = skinConfig->getInt32("Head_gradient", DARK2LIGHT);
		g_settings.Head_gradient_type = skinConfig->getInt32("Head_gradient_type", GRADIENT_COLOR2TRANSPARENT);
		g_settings.Head_corner = skinConfig->getInt32("Head_corner", CORNER_TOP);
		g_settings.Head_radius = skinConfig->getInt32("Head_radius", RADIUS_MID);
		g_settings.Head_line = skinConfig->getBool("Head_line", false);
		g_settings.Head_line_gradient = skinConfig->getBool("Head_line_gradient", false);
		
		// foot
		g_settings.Foot_gradient = skinConfig->getInt32("Foot_gradient", LIGHT2DARK);
		g_settings.Foot_gradient_type = skinConfig->getInt32("Foot_gradient_type", GRADIENT_COLOR2TRANSPARENT);
		g_settings.Foot_corner = skinConfig->getInt32("Foot_corner", CORNER_BOTTOM);
		g_settings.Foot_radius = skinConfig->getInt32("Foot_radius", RADIUS_MID);
		g_settings.Foot_line = skinConfig->getBool("Foot_line", false);
		g_settings.Foot_line_gradient = skinConfig->getBool("Foot_line_gradient", false);
		
		// infobar
		g_settings.infobar_gradient = skinConfig->getInt32("infobar_gradient", DARK2LIGHT);
		g_settings.infobar_gradient_type = skinConfig->getInt32("infobar_gradient_type", GRADIENT_COLOR2TRANSPARENT);
		g_settings.infobar_corner = skinConfig->getInt32("infobar_corner", CORNER_ALL);
		g_settings.infobar_radius = skinConfig->getInt32("infobar_radius", RADIUS_MID);
		g_settings.infobar_border = skinConfig->getBool("infobar_border", false);
		g_settings.infobar_buttonbar = skinConfig->getBool("infobar_buttonbar", true);
		g_settings.infobar_buttonline = skinConfig->getBool("infobar_buttonline", false);
		g_settings.infobar_buttonline_gradient = skinConfig->getBool("infobar_buttonline_gradient", false);
		
		// itemInfo
		g_settings.Hint_border = skinConfig->getBool("Hint_border", true);
		g_settings.Hint_gradient = skinConfig->getInt32("Hint_gradient", DARK2LIGHT);
		g_settings.Hint_gradient_type = skinConfig->getInt32("Hint_gradient_type", GRADIENT_COLOR2TRANSPARENT);
		g_settings.Hint_radius = skinConfig->getInt32("Hint_radius", NO_RADIUS);
		g_settings.Hint_corner = skinConfig->getInt32("Hint_corner", CORNER_ALL);
		
		// progressbar color
		g_settings.progressbar_color = skinConfig->getInt32("progressbar_color", 1);
		//g_settings.progressbar_gradient_type = skinConfig->getInt32("progressbar_gradient_type", GRADIENT_ONECOLOR);
		
		strcpy( g_settings.font_file, skinConfig->getString( "font_file", DATADIR "/fonts/arial.ttf" ).c_str() );

		colorSetupNotifier = new CColorSetupNotifier;
		colorSetupNotifier->changeNotify("", NULL);
		
		delete colorSetupNotifier;
	}
	else
		printf("CNeutrinoApp::readSkinConfig: %s not found\n", filename);
}

void CNeutrinoApp::saveSkinConfig(const char * const filename)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveFile: %s\n", filename);
	
	CConfigFile* skinConfig = new CConfigFile(',');;
	
	skinConfig->setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	skinConfig->setInt32( "menu_Head_red", g_settings.menu_Head_red );
	skinConfig->setInt32( "menu_Head_green", g_settings.menu_Head_green );
	skinConfig->setInt32( "menu_Head_blue", g_settings.menu_Head_blue );
	skinConfig->setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	skinConfig->setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	skinConfig->setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	skinConfig->setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	skinConfig->setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	skinConfig->setInt32( "menu_Content_red", g_settings.menu_Content_red );
	skinConfig->setInt32( "menu_Content_green", g_settings.menu_Content_green );
	skinConfig->setInt32( "menu_Content_blue", g_settings.menu_Content_blue );
	skinConfig->setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	skinConfig->setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	skinConfig->setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	skinConfig->setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	skinConfig->setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	skinConfig->setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	skinConfig->setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	skinConfig->setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );
	skinConfig->setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	skinConfig->setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	skinConfig->setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	skinConfig->setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	skinConfig->setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	skinConfig->setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	skinConfig->setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	skinConfig->setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );
	skinConfig->setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	skinConfig->setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	skinConfig->setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	skinConfig->setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	skinConfig->setInt32( "infobar_alpha", g_settings.infobar_alpha );
	skinConfig->setInt32( "infobar_red", g_settings.infobar_red );
	skinConfig->setInt32( "infobar_green", g_settings.infobar_green );
	skinConfig->setInt32( "infobar_blue", g_settings.infobar_blue );
	skinConfig->setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	skinConfig->setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	skinConfig->setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	skinConfig->setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );
	
	skinConfig->setInt32( "infobar_colored_events_alpha", g_settings.infobar_colored_events_alpha );
	skinConfig->setInt32( "infobar_colored_events_red", g_settings.infobar_colored_events_red );
	skinConfig->setInt32( "infobar_colored_events_green", g_settings.infobar_colored_events_green );
	skinConfig->setInt32( "infobar_colored_events_blue", g_settings.infobar_colored_events_blue );
	
	skinConfig->setInt32( "menu_Foot_alpha", g_settings.menu_Foot_alpha );
	skinConfig->setInt32( "menu_Foot_red", g_settings.menu_Foot_red );
	skinConfig->setInt32( "menu_Foot_green", g_settings.menu_Foot_green );
	skinConfig->setInt32( "menu_Foot_blue", g_settings.menu_Foot_blue );
	skinConfig->setInt32( "menu_Foot_Text_alpha", g_settings.menu_Foot_Text_alpha );
	skinConfig->setInt32( "menu_Foot_Text_red", g_settings.menu_Foot_Text_red );
	skinConfig->setInt32( "menu_Foot_Text_green", g_settings.menu_Foot_Text_green );
	skinConfig->setInt32( "menu_Foot_Text_blue", g_settings.menu_Foot_Text_blue );

	skinConfig->setInt32( "menu_Hint_alpha", g_settings.menu_Hint_alpha );
	skinConfig->setInt32( "menu_Hint_red", g_settings.menu_Hint_red );
	skinConfig->setInt32( "menu_Hint_green", g_settings.menu_Hint_green );
	skinConfig->setInt32( "menu_Hint_blue", g_settings.menu_Hint_blue );
	skinConfig->setInt32( "menu_Hint_Text_alpha", g_settings.menu_Hint_Text_alpha );
	skinConfig->setInt32( "menu_Hint_Text_red", g_settings.menu_Hint_Text_red );
	skinConfig->setInt32( "menu_Hint_Text_green", g_settings.menu_Hint_Text_green );
	skinConfig->setInt32( "menu_Hint_Text_blue", g_settings.menu_Hint_Text_blue );
	
	// head
	skinConfig->setInt32("Head_gradient", g_settings.Head_gradient);
	skinConfig->setInt32("Head_gradient_type", g_settings.Head_gradient_type);
	skinConfig->setInt32("Head_corner", g_settings.Head_corner);
	skinConfig->setInt32("Head_radius", g_settings.Head_radius);
	skinConfig->setBool("Head_line", g_settings.Head_line);
	skinConfig->setBool("Head_line_gradient", g_settings.Head_line_gradient);
	
	// foot
	skinConfig->setInt32("Foot_gradient", g_settings.Foot_gradient);
	skinConfig->setInt32("Foot_gradient_type", g_settings.Foot_gradient_type);
	skinConfig->setInt32("Foot_corner", g_settings.Foot_corner);
	skinConfig->setInt32("Foot_radius", g_settings.Foot_radius);
	skinConfig->setBool("Foot_line", g_settings.Foot_line);
	skinConfig->setBool("Foot_line_gradient", g_settings.Foot_line_gradient);
	
	// infobar
	skinConfig->setInt32("infobar_gradient", g_settings.infobar_gradient);
	skinConfig->setInt32("infobar_gradient_type", g_settings.infobar_gradient_type);
	skinConfig->setInt32("infobar_corner", g_settings.infobar_corner);
	skinConfig->setInt32("infobar_radius", g_settings.infobar_radius);
	skinConfig->setBool("infobar_buttonbar", g_settings.infobar_buttonbar);
	skinConfig->setBool("infobar_buttonline", g_settings.infobar_buttonline);
	skinConfig->setBool("infobar_buttonline_gradient", g_settings.infobar_buttonline_gradient);
	skinConfig->setBool("infobar_border", g_settings.infobar_border);
	
	// itemInfo
	skinConfig->setBool("Hint_border", g_settings.Hint_border);
	skinConfig->setInt32("Hint_gradient", g_settings.Hint_gradient);
	skinConfig->setInt32("Hint_gradient_type", g_settings.Hint_gradient_type);
	skinConfig->setInt32("Hint_radius", g_settings.Hint_radius);
	skinConfig->setInt32("Hint_corner", g_settings.Hint_corner);
	
	// progressbar color
	skinConfig->setInt32("progressbar_color", g_settings.progressbar_color);
	//skinConfig->setInt32("progressbar_gradient_type", g_settings.progressbar_gradient_type);
		
	skinConfig->setString("font_file", g_settings.font_file);

	if (!skinConfig->saveConfig(filename))
		printf("CNeutrinoApp::saveSkinConfig %s write error\n", filename);
}

