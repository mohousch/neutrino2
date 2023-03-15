/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: skin.cpp.cpp 15.01.2022 mohousch Exp $

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
#include <gui/widget/window.h>
#include <gui/widget/hintbox.h>

#include <gui/plugins.h>
#include <gui/themes.h>
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
_xmlDocPtr parser = NULL;

//
CMenuTarget* CNeutrinoApp::convertTarget(const std::string& name)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertTarget: %s\n", name.c_str());
	
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
	else if (name == "themesetup")
	{		
		parent = new CThemes();
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
		parent = new CAlphaSetup(_("Alpha"), &g_settings.gtx_alpha);;
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
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertColor: color: %s\n", color);
	
	uint32_t rgba = COL_MENUCONTENT_PLUS_0;
	
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	unsigned int a = 0;
				
	if (color != NULL)
	{
		if (color[0] == '#')
		{
			unsigned long col = 0;
						
			if (sscanf(color + 1, "%lx", &col) == 1)
			{
				r = (col>>24)&0xFF; 
				g = (col>>16)&0xFF;
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
				
	rgba = convertSetupColor2Color(r, g, b, a); 
	
	return rgba;
}

//
uint8_t CNeutrinoApp::convertFontColor(const char* const color)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertFontColor: color: %s\n", color);
	
	uint8_t rgb = COL_MENUCONTENT;
	
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
				
	if (color != NULL)
	{
		if (color[0] == '#')
		{
			unsigned long col = 0;
						
			if (sscanf(color + 1, "%lx", &col) == 1)
			{
				r = (col>>16)&0xFF;
				g = (col >> 8)&0xFF;
				b = (col)&0xFF;
			}
		}
		else
		{
			if ( strcmp(color, "COL_MAROON") == 0)
			{
				return COL_MAROON;
			}
			else if ( strcmp(color, "COL_GREEN") == 0)
			{
				return COL_GREEN;
			}
			else if ( strcmp(color, "COL_OLIVE") == 0)
			{
				return COL_OLIVE;
			}
			else if ( strcmp(color, "COL_NAVY") == 0)
			{
				return COL_NAVY;
			}
			else if ( strcmp(color, "COL_PURPLE") == 0)
			{
				return COL_PURPLE;
			}
			else if ( strcmp(color, "COL_TEAL") == 0)
			{
				return COL_TEAL;
			}
			else if ( strcmp(color, "COL_NOBEL") == 0)
			{
				return COL_NOBEL;
			}
			else if ( strcmp(color, "COL_MATTERHORN") == 0)
			{
				return COL_MATTERHORN;
			}
			else if ( strcmp(color, "COL_RED") == 0)
			{
				return COL_RED;
			}
			else if ( strcmp(color, "COL_LIME") == 0)
			{
				return COL_LIME;
			}
			else if ( strcmp(color, "COL_YELLOW") == 0)
			{
				return COL_YELLOW;
			}
			else if ( strcmp(color, "COL_BLUE") == 0)
			{
				return COL_BLUE;
			}
			else if ( strcmp(color, "COL_MAGENTA") == 0)
			{
				return COL_MAGENTA;
			}
			else if ( strcmp(color, "COL_AQUA") == 0)
			{
				return COL_AQUA;
			}
			else if ( strcmp(color, "COL_WHITE") == 0)
			{
				return COL_WHITE;
			}
			else if ( strcmp(color, "COL_BLACK") == 0)
			{
				return COL_BLACK;
			}
			else if ( strcmp(color, "COL_ORANGE") == 0)
			{
				return COL_ORANGE;
			}
			else if ( strcmp(color, "COL_SILVER") == 0)
			{
				return COL_SILVER;
			}
			else if ( strcmp(color, "COL_BACKGROUND") == 0)
			{
				return COL_BACKGROUND;
			}
			else if ( strcmp(color, "COL_INFOBAR") == 0)
			{
				return COL_INFOBAR;
			}
			else if ( strcmp(color, "COL_INFOBAR_SHADOW") == 0)
			{
				return COL_INFOBAR_SHADOW;
			}
			else if ( strcmp(color, "COL_MENUHEAD") == 0)
			{
				return COL_MENUHEAD;
			}
			else if ( strcmp(color, "COL_MENUCONTENT") == 0)
			{
				return COL_MENUCONTENT;
			}
			else if ( strcmp(color, "COL_MENUCONTENTDARK") == 0)
			{
				return COL_MENUCONTENTDARK;
			}
			else if ( strcmp(color, "COL_MENUCONTENTSELECTED") == 0)
			{
				return COL_MENUCONTENTSELECTED;
			}
			else if ( strcmp(color, "COL_MENUCONTENTINACTIVE") == 0)
			{
				return COL_MENUCONTENTINACTIVE;
			}
			else if ( strcmp(color, "COL_MENUFOOT") == 0)
			{
				return COL_MENUFOOT;
			}
			else if ( strcmp(color, "COL_MENUHINT") == 0)
			{
				return COL_MENUHINT;
			}
		}
	}
				
	rgb = convertSetupColor2RGB(r, g, b); 
	
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
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertCorner: corner: %s\n", corner);
	
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
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertRadius: radius: %s\n", radius);
	
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
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertGradient: gradient: %s\n", gradient);
	
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
neutrino_msg_t CNeutrinoApp::convertKey(const char * const key)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertKey: key: %s\n", key);
	
	neutrino_msg_t msg = RC_nokey;
	
	if (key != NULL)
	{
		if ( strcmp(key, "RC_nokey") == 0)
		{
			msg = RC_nokey;
		}
		else if ( strcmp(key, "RC_red") == 0)
		{
			msg = RC_red;
		}
		else if ( strcmp(key, "RC_green") == 0)
		{
			msg = RC_green;
		}
		else if ( strcmp(key, "RC_yellow") == 0)
		{
			msg = RC_yellow;
		}
		else if ( strcmp(key, "RC_blue") == 0)
		{
			msg = RC_blue;
		}
		else if ( strcmp(key, "RC_0") == 0)
		{
			msg = RC_0;
		}
		else if ( strcmp(key, "RC_1") == 0)
		{
			msg = RC_1;
		}
		else if ( strcmp(key, "RC_2") == 0)
		{
			msg = RC_2;
		}
		else if ( strcmp(key, "RC_3") == 0)
		{
			msg = RC_3;
		}
		else if ( strcmp(key, "RC_4") == 0)
		{
			msg = RC_4;
		}
		else if ( strcmp(key, "RC_5") == 0)
		{
			msg = RC_5;
		}
		else if ( strcmp(key, "RC_6") == 0)
		{
			msg = RC_6;
		}
		else if ( strcmp(key, "RC_7") == 0)
		{
			msg = RC_7;
		}
		else if ( strcmp(key, "RC_8") == 0)
		{
			msg = RC_8;
		}
		else if ( strcmp(key, "RC_9") == 0)
		{
			msg = RC_9;
		}
		else if ( strcmp(key, "RC_mode") == 0)
		{
			msg = RC_mode;
		}
		else if ( strcmp(key, "RC_standby") == 0)
		{
			msg = RC_standby;
		}
		else if ( strcmp(key, "RC_home") == 0)
		{
			msg = RC_home;
		}
		else if ( strcmp(key, "RC_setup") == 0)
		{
			msg = RC_setup;
		}
		else if ( strcmp(key, "RC_info") == 0)
		{
			msg = RC_info;
		}
		else if ( strcmp(key, "RC_epg") == 0)
		{
			msg = RC_epg;
		}
	}
	
	return msg;
}

//
int CNeutrinoApp::convertMenuPosition(const char* const position)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertMenuPosition: position: %s\n", position);
	
	int pos = MENU_POSITION_NONE;
	
	if (position != NULL)
	{
		if ( strcmp(position, "MENU_POSITION_NONE") == 0)
		{
			pos = MENU_POSITION_NONE;
		}
		else if ( strcmp(position, "MENU_POSITION_LEFT") == 0)
		{
			pos = MENU_POSITION_LEFT;
		}
		else if ( strcmp(position, "MENU_POSITION_CENTER") == 0)
		{
			pos = MENU_POSITION_CENTER;
		}
		else if ( strcmp(position, "MENU_POSITION_RIGHT") == 0)
		{
			pos = MENU_POSITION_RIGHT;
		}
	}
	
	return pos;
}

//
int CNeutrinoApp::convertClistBoxMode(const char * const mode)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertClistBoxMode: mode: %s\n", mode);
	
	int m = MODE_LISTBOX;
	
	if (mode != NULL)
	{
		if ( strcmp(mode, "MODE_LISTBOX") == 0)
		{
			m = MODE_LISTBOX;
		}
		else if ( strcmp(mode, "MODE_MENU") == 0)
		{
			m = MODE_MENU;
		}
		else if ( strcmp(mode, "MODE_SETUP") == 0)
		{
			m = MODE_SETUP;
		}
	}
	
	return m;
}

//
int CNeutrinoApp::convertClistBoxType(const char * const type)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertClistBoxType: type: %s\n", type);
	
	int t = TYPE_STANDARD;
	
	if (type != NULL)
	{
		if ( strcmp(type, "TYPE_STANDARD") == 0)
		{
			t = TYPE_STANDARD;
		}
		else if ( strcmp(type, "TYPE_CLASSIC") == 0)
		{
			t = TYPE_CLASSIC;
		}
		else if ( strcmp(type, "TYPE_EXTENDED") == 0)
		{
			t = TYPE_EXTENDED;
		}
		else if ( strcmp(type, "TYPE_FRAME") == 0)
		{
			t = TYPE_FRAME;
		}
	}
	
	return t;
}

int CNeutrinoApp::convertCMenuItemID(const char * const id)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertCMenuItemID: id: %s\n", id);
	
	int i = MENUITEM_LISTBOXITEM;
	
	if (id != NULL)
	{
		if ( strcmp(id, "MENUITEM_OPTION_CHOOSER") == 0)
		{
			i = MENUITEM_OPTION_CHOOSER;
		}
		else if ( strcmp(id, "MENUITEM_OPTION_NUMBER_CHOOSER") == 0)
		{
			i = MENUITEM_OPTION_NUMBER_CHOOSER;
		}
		else if ( strcmp(id, "MENUITEM_OPTION_STRING_CHOOSER") == 0)
		{
			i = MENUITEM_OPTION_STRING_CHOOSER;
		}
		else if ( strcmp(id, "MENUITEM_OPTION_LANGUAGE_CHOOSER") == 0)
		{
			i = MENUITEM_OPTION_LANGUAGE_CHOOSER;
		}
		else if ( strcmp(id, "MENUITEM_SEPARATOR") == 0)
		{
			i = MENUITEM_SEPARATOR;
		}
		else if ( strcmp(id, "MENUITEM_FORWARDER") == 0)
		{
			i = MENUITEM_FORWARDER;
		}
		else if ( strcmp(id, "MENUITEM_LOCKED_FORWARDER") == 0)
		{
			i = MENUITEM_LOCKED_FORWARDER;
		}
		else if ( strcmp(id, "MENUITEM_LISTBOXITEM") == 0)
		{
			i = MENUITEM_LISTBOXITEM;
		}
		else if ( strcmp(id, "MENUITEM_PLUGINITEM") == 0)
		{
			i = MENUITEM_PLUGINITEM;
		}	
	}
	
	return i;
}

//
int CNeutrinoApp::convertBorder(const char* const border)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertBorder: value: %s\n", border);
	
	int bor = BORDER_NO;
	
	if (border != NULL)
	{
		if ( strcmp(border, "BORDER_NO") == 0)
		{
			bor = BORDER_NO;
		}
		else if ( strcmp(border, "BORDER_ALL") == 0)
		{
			bor = BORDER_ALL;
		}
		else if ( strcmp(border, "BORDER_LEFTRIGHT") == 0)
		{
			bor = BORDER_LEFTRIGHT;
		}
		else if ( strcmp(border, "BORDER_TOPBOTTOM") == 0)
		{
			bor = BORDER_TOPBOTTOM;
		}
	}
	
	return bor;
}

//
int CNeutrinoApp::convertBool(const char* const value)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::convertBool: value: %s\n", value);
	
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
void CNeutrinoApp::parseClistBox(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseClistBox:\n");
	
	ClistBox* listBox = NULL;
	CMenuItem* menuItem = NULL;
	
	//
	char* name = NULL;
				
	unsigned int posx = 0;
	unsigned int posy = 0;
	unsigned int width = 0;
	unsigned int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * gradient = NULL;
	char *corner = NULL;
	char * radius = NULL;
	
	char * type = NULL;
	char * mode = NULL;
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
	char * head_line_gradient = NULL;
				
	// foot
	unsigned int paintfoot = 0;
	unsigned int foot_line = 0;
	char * foot_line_gradient = NULL;
				
	// iteminfo
	unsigned int paintiteminfo = 0;
	unsigned int iteminfomode = 0;		
	unsigned int iteminfoframe = 0;
	unsigned int iteminfo_posx = 0;
	unsigned int iteminfo_posy = 0;
	unsigned int iteminfo_width = 0;
	unsigned int iteminfo_height = 0;
	const char* iteminfo_color = NULL;
	
	// item
	unsigned int itemborder = 0;
	char * itemgradient = NULL;
	unsigned int item2lines = 0;
	
	//
	_xmlNodePtr listboxitem_node = NULL;
	_xmlNodePtr listboxintegration_node = NULL;
	_xmlNodePtr buttonlabel_node = NULL;
				
	while ((node = xmlGetNextOccurence(node, "LISTBOX")) != NULL) 
	{
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
				
		// parse color
		uint32_t finalColor = COL_MENUCONTENT_PLUS_0;
				
		if (color) finalColor = convertColor(color);
					
		type = xmlGetAttribute(node, (char *)"type");
		mode = xmlGetAttribute(node, (char *)"mode");
		scrollbar = xmlGetSignedNumericAttribute(node, "scrollbar", 0);
		
		shrink = xmlGetSignedNumericAttribute(node, "shrink", 0);
				
		// head
		painthead = xmlGetSignedNumericAttribute(node, "painthead", 0);
		paintdate = xmlGetSignedNumericAttribute(node, "paintdate", 0);
		title = xmlGetAttribute(node, "title");
		icon = xmlGetAttribute(node, "icon");
		//format = xmlGetAttribute(node, (char*)"format"); //FIXME:
		halign = xmlGetSignedNumericAttribute(node, "halign", 0);
		head_line = xmlGetSignedNumericAttribute(node, "head_line", 0);
		head_line_gradient = xmlGetAttribute(node, (char *)"head_line_gradient");
				
		// foot
		paintfoot = xmlGetSignedNumericAttribute(node, "paintfoot", 0);
		foot_line = xmlGetSignedNumericAttribute(node, "foot_line", 0);
		foot_line_gradient = xmlGetAttribute(node, (char *)"foot_line_gradient");
		
		// item
		itemborder = xmlGetSignedNumericAttribute(node, "itemborder", 0);
		itemgradient = xmlGetAttribute(node, (char *)"itemgradient");
		item2lines = xmlGetSignedNumericAttribute(node, "itemlines", 0);
				
		// iteminfo
		paintiteminfo = xmlGetSignedNumericAttribute(node, "paintiteminfo", 0);
		iteminfomode = xmlGetSignedNumericAttribute(node, "iteminfomode", 0);
		iteminfoframe = xmlGetSignedNumericAttribute(node, "iteminfoframe", 0);
		iteminfo_posx = xmlGetSignedNumericAttribute(node, "iteminfoposx", 0);
		iteminfo_posy = xmlGetSignedNumericAttribute(node, "iteminfoposy", 0);
		iteminfo_width = xmlGetSignedNumericAttribute(node, "iteminfowidth", 0);
		iteminfo_height = xmlGetSignedNumericAttribute(node, "iteminfoheight", 0);

		iteminfo_color = xmlGetAttribute(node, (char*)"iteminfocolor");
		uint32_t hintColor = COL_MENUCONTENT_PLUS_0;
				
		if (iteminfo_color) hintColor = convertColor(iteminfo_color);
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (posx < widget->getWindowsPos().iX)
				posx = widget->getWindowsPos().iX;
				
			if (posy < widget->getWindowsPos().iY)
				posy = widget->getWindowsPos().iY;
		}
					
		listBox = new ClistBox(posx, posy, width, height);
		
		listBox->widgetItem_type = WIDGETITEM_LISTBOX;
		if (name) listBox->widgetItem_name = name;
		
		//
		int t = TYPE_STANDARD;
		if (type) t = convertClistBoxType(type);
		listBox->setWidgetType(t);
		//
		int m = MODE_LISTBOX;
		if (mode) m = convertClistBoxMode(mode);
		listBox->setWidgetMode(m);
		listBox->paintScrollBar(scrollbar);
		listBox->paintMainFrame(paintframe);
		if (color != NULL) listBox->setColor(finalColor);
		// gradient
		// corner
		// radius
		
		if (shrink) listBox->enableShrinkMenu();
				
		//
		if (painthead)
		{
			listBox->enablePaintHead();
			listBox->setTitle(_(title), icon);
			if (paintdate) listBox->enablePaintDate();
			//if (format) listBox->setFormat(format); //FIXME:
			listBox->setTitleHAlign(halign);
			//
			int head_line_gr = NOGRADIENT;
			if (head_line_gradient) head_line_gr = convertGradient(head_line_gradient);
			listBox->setHeadLine(head_line, head_line_gr);
		}
				
		//
		if (paintfoot)
		{
			listBox->enablePaintFoot();
			//
			int foot_line_gr = NOGRADIENT;
			if (foot_line_gradient) foot_line_gr = convertGradient(foot_line_gradient);
			listBox->setFootLine(foot_line, foot_line_gr);
		}
				
		// iteminfo
		if (paintiteminfo)
		{
			listBox->enablePaintItemInfo(70);
			listBox->setItemInfoMode(iteminfomode);
					
			listBox->setItemInfoPos(iteminfo_posx, iteminfo_posy, iteminfo_width, iteminfo_height);
			listBox->paintItemInfoFrame(iteminfoframe);
			if (iteminfo_color) listBox->setItemInfoColor(hintColor);
		}
		
		// item
		if (itemborder) listBox->setItemBorderMode(itemborder);
		//
		int item_gr = NOGRADIENT;
		if (itemgradient) item_gr = convertGradient(itemgradient);
		listBox->setItemGradient(item_gr);
		if (item2lines) listBox->setItem2Lines();
				
		// ITEM	
		listboxitem_node = node->xmlChildrenNode;
		
		//
		char * item_id = NULL;
						
		char* item_localename = NULL;
		char* item_option = NULL;
		char* item_actionkey = NULL;
		char* item_target = NULL;
		char* item_hinticon = NULL;
		char* item_hint = NULL;
		char* item_icon = NULL;
		char* item_directkey = NULL;
		unsigned int item_lines = 0;
		unsigned int item_border = 0;
		unsigned int item_gradient = 0;
					
		while ((listboxitem_node = xmlGetNextOccurence(listboxitem_node, "ITEM")) != NULL) 
		{	
			item_id = xmlGetAttribute(listboxitem_node, (char *)"id");
						
			item_localename = xmlGetAttribute(listboxitem_node, (char*)"localename");
			item_option = xmlGetAttribute(listboxitem_node, (char*)"option");
			item_actionkey = xmlGetAttribute(listboxitem_node, (char*)"actionkey");
			item_target = xmlGetAttribute(listboxitem_node, (char*)"target");
			item_hinticon = xmlGetAttribute(listboxitem_node, (char*)"itemicon");
			item_hint = xmlGetAttribute(listboxitem_node, (char*)"hint");
			item_icon = xmlGetAttribute(listboxitem_node, (char*)"iconname");
			item_directkey = xmlGetAttribute(listboxitem_node, (char*)"directkey");
			item_lines = xmlGetSignedNumericAttribute(listboxitem_node, "lines", 0);
			item_border = xmlGetSignedNumericAttribute(listboxitem_node, "border", 0);
			item_gradient = xmlGetSignedNumericAttribute(listboxitem_node, "gradient", 0);
						
			CMenuTarget* parent = NULL;
			std::string actionKey = "";
			std::string itemName = "";
			neutrino_msg_t key = RC_nokey;
						
			if (item_localename) itemName = _(item_localename);
						
			//
			int id = MENUITEM_FORWARDER;
			if (item_id) id = convertCMenuItemID(item_id);
			
			if (id == MENUITEM_FORWARDER)
				menuItem = new CMenuForwarder(itemName.c_str());
			else if (id == MENUITEM_LISTBOXITEM)
				menuItem = new ClistBoxItem(itemName.c_str());
						
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
			if (item_border) menuItem->setBorderMode(item_border);
			if (item_gradient) menuItem->setGradient(item_gradient);
					
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
		unsigned int mode = MODE_MENU;
		unsigned int shortcut = RC_nokey;
		unsigned int type = TYPE_STANDARD;
		unsigned int i_lines = false;
		unsigned int i_border = false;
					
		while ((listboxintegration_node = xmlGetNextOccurence(listboxintegration_node, "INTEGRATION")) != NULL) 
		{	
			integration = (CPlugins::i_type_t)xmlGetSignedNumericAttribute(listboxintegration_node, "id", 0);
			mode = xmlGetSignedNumericAttribute(listboxintegration_node, "mode", 0);
			shortcut = xmlGetSignedNumericAttribute(listboxintegration_node, "shortcut", 0);
			type = xmlGetSignedNumericAttribute(listboxintegration_node, "type", 0);
			i_lines = xmlGetSignedNumericAttribute(listboxintegration_node, "lines", 0);
			i_border = xmlGetSignedNumericAttribute(listboxintegration_node, "border", 0);
						
			listBox->integratePlugins(integration, shortcut? shortcut : RC_nokey, true, mode, type, i_lines, i_border);
				
			listboxintegration_node = listboxintegration_node->xmlNextNode;
		}
				
		// BUTTON_LABEL / FOOT
		buttonlabel_node = node->xmlChildrenNode;
		
		char* button = NULL;
		char* localename = NULL;
					
		while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "FOOT_BUTTON_LABEL")) != NULL) 
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
				
		// BUTTON_LABEL / HEAD
		while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "HEAD_BUTTON_LABEL")) != NULL) 
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
		
		if (widget) widget->addWidgetItem(listBox);
			
		node = node->xmlNextNode;
	}
}

// CWindow
void CNeutrinoApp::parseCWindow(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCWindow\n");
	
	CWindow* window = NULL;
	
	char* name = NULL;
				
	unsigned int posx = 0;
	unsigned int posy = 0;
	unsigned int width = 0;
	unsigned int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * corner = NULL;
	char * radius = NULL;
	unsigned int border = 0;
	char *gradient = NULL;
	unsigned int gradient_direction = 0;
	unsigned int gradient_intensity = 0;
	unsigned int gradient_type = 0;
	
	unsigned int refresh = 0;
	
	while ((node = xmlGetNextOccurence(node, "WINDOW")) != NULL) 
	{
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
		
		gradient = xmlGetAttribute(node, (char *)"gradient");
		gradient_direction = xmlGetSignedNumericAttribute(node, "direction", 0);
		gradient_intensity = xmlGetSignedNumericAttribute(node, "intensity", 0);
		gradient_type = xmlGetSignedNumericAttribute(node, "type", 0);
				
		// parse color
		uint32_t finalColor = COL_MENUCONTENT_PLUS_0;
				
		if (color) finalColor = convertColor(color);
				
		refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (posx < widget->getWindowsPos().iX)
				posx = widget->getWindowsPos().iX;
				
			if (posy < widget->getWindowsPos().iY)
				posy = widget->getWindowsPos().iY;
		}
					
		window = new CWindow(posx, posy, width, height);
		
		window->widgetItem_type = WIDGETITEM_WINDOW;
		if (name) window->widgetItem_name = name;
					
		window->paintMainFrame(paintframe);
		
		if (!paintframe) window->enableSaveScreen();
		if (color) window->setColor(finalColor);
		if (refresh) window->enableRepaint();

		int co = CORNER_NONE;
		int ra = NO_RADIUS;
		if (corner) co = convertCorner(corner);
		if (radius) ra = convertRadius(radius);
		window->setCorner(ra, co);
		window->setBorderMode(border);

		int gr = NOGRADIENT;
		if (gradient) gr = convertGradient(gradient);
		window->setGradient(gr, gradient_direction, gradient_intensity, gradient_type);
		
		// LABEL
		parseCCLabel(node->xmlChildrenNode, NULL, window);
			
		// IMAGE
		parseCCImage(node->xmlChildrenNode, NULL, window);
			
		// TIME
		parseCCTime(node->xmlChildrenNode, NULL, window);
			
		// BUTTONS
		parseCCButtons(node->xmlChildrenNode, NULL, window);
		
		// PIG
		parseCCPig(node->xmlChildrenNode, NULL, window);
					
		if (widget) widget->addWidgetItem(window);
			
		node = node->xmlNextNode;
	}
}

// CHead
void CNeutrinoApp::parseCHead(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCHead:\n");
	
	CHeaders* head = NULL;
	
	char* name = NULL;
				
	unsigned int posx = 0;
	unsigned int posy = 0;
	unsigned int width = 0;
	unsigned int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * gradient = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	char* title = NULL;
	unsigned int halign = 0;
	const char* icon = NULL;
	unsigned int head_line = 0;
	char * head_line_gradient = NULL;
	
	unsigned int paintdate = 0;
	char* format = NULL;
	
	_xmlNodePtr buttonlabel_node = NULL;
	
	while ((node = xmlGetNextOccurence(node, "HEAD")) != NULL) 
	{
		name = xmlGetAttribute(node, (char*)"name");
						
		posx = xmlGetSignedNumericAttribute(node, "posx", 0);
		posy = xmlGetSignedNumericAttribute(node, "posy", 0);
		width = xmlGetSignedNumericAttribute(node, "width", 0);
		height = xmlGetSignedNumericAttribute(node, "height", 0);
				
		paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);		
		color = xmlGetAttribute(node, (char*)"color");
		gradient = xmlGetAttribute(node, (char *)"gradient");
		corner = xmlGetAttribute(node, (char *)"corner");
		radius = xmlGetAttribute(node, (char *)"radius");
				
		// parse color
		uint32_t finalColor = COL_MENUCONTENT_PLUS_0;		
		if (color) finalColor = convertColor(color);
					
		title = xmlGetAttribute(node, (char*)"title");
		halign = xmlGetSignedNumericAttribute(node, "halign", 0);
		icon = xmlGetAttribute(node, (char*)"icon");
		head_line = xmlGetSignedNumericAttribute(node, "line", 0);
		head_line_gradient = xmlGetAttribute(node, (char *)"line_gradient");
		
		paintdate = xmlGetSignedNumericAttribute(node, "paintdate", 0);
		format = xmlGetAttribute(node, (char*)"format");
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (posx < widget->getWindowsPos().iX)
				posx = widget->getWindowsPos().iX;
				
			if (posy < widget->getWindowsPos().iY)
				posy = widget->getWindowsPos().iY;
		}

		head = new CHeaders(posx, posy, width, height);
		
		head->widgetItem_type = WIDGETITEM_HEAD;
		if (name) head->widgetItem_name = name;
		
		head->paintMainFrame(paintframe);
	
		if (title != NULL) head->setTitle(_(title));
		head->setHAlign(halign);
		if (icon != NULL) head->setIcon(icon);
		if(color != NULL) head->setColor(finalColor);
		int gr = NOGRADIENT;
		if (gradient) gr = convertGradient(gradient);
		head->setGradient(gr);
		int co = CORNER_NONE;
		int ra = NO_RADIUS;
		if (corner) co = convertCorner(corner);
		if (radius) ra = convertRadius(radius);
		head->setCorner(ra, co);
		
		int head_line_gr = NOGRADIENT;
		if (head_line_gradient) head_line_gr = convertGradient(head_line_gradient);
		head->setHeadLine(head_line, head_line_gr);
		
		if (paintdate) head->enablePaintDate();
		if (format != NULL) head->setFormat(_(format));
					
		// BUTTON_LABEL
		buttonlabel_node = node->xmlChildrenNode;
		
		char* button = NULL;
		char* localename = NULL;
					
		while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "BUTTON_LABEL")) != NULL) 
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
					
		if (widget) widget->addWidgetItem(head);
			
		node = node->xmlNextNode;
	}
}

// CFoot
void CNeutrinoApp::parseCFoot(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCFoot:\n");
	
	CFooters* foot = NULL;
	
	unsigned int posx = 0;
	unsigned int posy = 0;
	unsigned int width = 0;
	unsigned int height = 0;
				
	unsigned int paintframe = 1;
	char* color = NULL;
	char * gradient = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	unsigned int foot_line = 0;
	char * foot_line_gradient = NULL;
	
	_xmlNodePtr buttonlabel_node = NULL;
	
	while ((node = xmlGetNextOccurence(node, "FOOT")) != NULL) 
	{		
		posx = xmlGetSignedNumericAttribute(node, "posx", 0);
		posy = xmlGetSignedNumericAttribute(node, "posy", 0);
		width = xmlGetSignedNumericAttribute(node, "width", 0);
		height = xmlGetSignedNumericAttribute(node, "height", 0);
				
		paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);	
		color = xmlGetAttribute(node, (char*)"color");
		gradient = xmlGetAttribute(node, (char *)"gradient");
		corner = xmlGetAttribute(node, (char *)"corner");
		radius = xmlGetAttribute(node, (char *)"radius");
				
		// parse color
		uint32_t finalColor = COL_MENUCONTENT_PLUS_0;		
		if (color) finalColor = convertColor(color);
				
		foot_line = xmlGetSignedNumericAttribute(node, "line", 0);
		foot_line_gradient = xmlGetAttribute(node, (char *)"line_gradient");
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (posx < widget->getWindowsPos().iX)
				posx = widget->getWindowsPos().iX;
			
			// FIXME:	
			if (posy < widget->getWindowsPos().iY)
				posy = widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - height;
		}
						
		foot = new CFooters(posx, posy, width, height);
		
		foot->widgetItem_type = WIDGETITEM_FOOT;
		
		foot->paintMainFrame(paintframe);			
		if (color != NULL) foot->setColor(finalColor);
		int gr = NOGRADIENT;
		if (gradient) gr = convertGradient(gradient);
		foot->setGradient(gr);
		int co = CORNER_NONE;
		int ra = NO_RADIUS;
		if (corner) co = convertCorner(corner);
		if (radius) ra = convertRadius(radius);
		foot->setCorner(ra, co);
		int foot_line_gr = NOGRADIENT;
		if (foot_line_gradient) gr = convertGradient(foot_line_gradient);
		foot->setFootLine(foot_line, foot_line_gr);
					
		// BUTTON_LABEL
		buttonlabel_node = node->xmlChildrenNode;
		
		char* button = NULL;
		char* localename = NULL;
					
		while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "BUTTON_LABEL")) != NULL) 
		{	
			button = xmlGetAttribute(buttonlabel_node, (char*)"name");
			localename = xmlGetAttribute(buttonlabel_node, (char*)"localename");
						
			button_label_struct btn;
			btn.button = " ";
			if (button) btn.button = button;
			btn.localename = " ";
			if (localename) btn.localename = localename;
						
			foot->setButtons(&btn);
				
			buttonlabel_node = buttonlabel_node->xmlNextNode;
		}
					
		if (widget) widget->addWidgetItem(foot);
			
		node = node->xmlNextNode;
	}
}

// CTextBox
void CNeutrinoApp::parseCTextBox(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCTextBox:\n");
	
	CTextBox* textBox = NULL;
	
	char* name = NULL;
	
	unsigned int posx = 0;
	unsigned int posy = 0;
	unsigned int width = 0;
	unsigned int height = 0;
				
	unsigned int paintframe = 1;
	unsigned int savescreen = 0;
	char* color = NULL;
	char * corner = NULL;
	char * radius = NULL;
	
	char* textColor = NULL;
	//unsigned int font = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	char *font = NULL;
	unsigned int fontbg = 0;
	unsigned int mode = SCROLL;
	unsigned int border = BORDER_NO;
	
	unsigned int tmode = PIC_RIGHT;
	unsigned int tw = 0;
	unsigned int th = 0;
	unsigned int tframe = 0;
	
	char* text = NULL;
	char* pic = NULL;
	
	while ((node = xmlGetNextOccurence(node, "TEXTBOX")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//		
		posx = xmlGetSignedNumericAttribute(node, "posx", 0);
		posy = xmlGetSignedNumericAttribute(node, "posy", 0);
		width = xmlGetSignedNumericAttribute(node, "width", 0);
		height = xmlGetSignedNumericAttribute(node, "height", 0);
				
		paintframe = xmlGetSignedNumericAttribute(node, "paintframe", 0);
		savescreen = xmlGetSignedNumericAttribute(node, "savescreen", 0);		
		color = xmlGetAttribute(node, (char*)"color");
		corner = xmlGetAttribute(node, (char *)"corner");
		radius = xmlGetAttribute(node, (char *)"radius");
				
		// parse color
		uint32_t finalColor = COL_MENUCONTENT_PLUS_0;
				
		if (color) finalColor = convertColor(color);
		
		textColor = xmlGetAttribute(node, (char*)"textcolor");
		//font = xmlGetSignedNumericAttribute(node, "font", 0);
		font = xmlGetAttribute(node, (char *)"font");
		fontbg = xmlGetSignedNumericAttribute(node, "fontbg", 0);
		
		mode = xmlGetSignedNumericAttribute(node, "mode", 0);
		border = xmlGetSignedNumericAttribute(node, "border", 0);
		
		tmode = xmlGetSignedNumericAttribute(node, "tmode", 0);
		tw = xmlGetSignedNumericAttribute(node, "twidth", 0);
		th = xmlGetSignedNumericAttribute(node, "theight", 0);
		tframe = xmlGetSignedNumericAttribute(node, "tframe", 0);
		
		text = xmlGetAttribute(node, (char*)"text");
		pic = xmlGetAttribute(node, (char*)"pic");
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (posx < widget->getWindowsPos().iX)
				posx = widget->getWindowsPos().iX;
				
			if (posy < widget->getWindowsPos().iY)
				posy = widget->getWindowsPos().iY;
		}
						
		textBox = new CTextBox(posx, posy, width, height);
		
		textBox->widgetItem_type = WIDGETITEM_TEXTBOX;
		if (name) textBox->widgetItem_name = name;
					
		if (color != NULL) textBox->setBackgroundColor(finalColor);
		
		//
		int co = CORNER_NONE;
		int ra = NO_RADIUS;
		if (corner) co = convertCorner(corner);
		if (radius) ra = convertRadius(radius);
		textBox->setCorner(ra, co);
		textBox->paintMainFrame(paintframe);
		if (savescreen || paintframe == 0) textBox->enableSaveScreen();
					
		textBox->setTextColor(textColor? convertFontColor(textColor) : COL_MENUCONTENT);
		//
		int fs = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
		if (font) fs = convertFontSize(font);
		textBox->setFont(fs);
		//
		textBox->setMode(mode);
		
		/*
		std::string filename = "";
		std::string image = pic;
		if (pic != NULL)
		{
			filename = CONFIGDIR "/skins/";
			filename += g_settings.preferred_skin;
			filename += "/";
			filename += pic;
					
			if (file_exists(filename.c_str()))
				image = filename.c_str();
		}

		if (text) textBox->setText(text, image.c_str(), tw, th, tmode, tframe, fontbg);
		*/
					
		if (widget) widget->addWidgetItem(textBox);
			
		node = node->xmlNextNode;
	}
}

// CCLabel
void CNeutrinoApp::parseCCLabel(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCLabel:\n");
	
	CCLabel* label = NULL;
	
	//
	char* name = NULL;
				
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
						
	unsigned int cc_refresh = 0;
	unsigned int l_halign = 0;
	
	//int font_size = -1;
	char * font_size = NULL;
	char* font_color = NULL;
	
	while ((node = xmlGetNextOccurence(node, "LABEL")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//						
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
						
		cc_refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
		
		//font_size = xmlGetSignedNumericAttribute(node, "font", 0);
		font_size = xmlGetAttribute(node, (char *)"font");
		font_color = xmlGetAttribute(node, (char*)"fontcolor");
		
		uint8_t color = COL_MENUCONTENT;
		
		if (font_color) color = convertFontColor(font_color);
						
		std::string text = "";
							
		text = xmlGetAttribute(node, (char*)"text");
		l_halign = xmlGetSignedNumericAttribute(node, "halign", 0);
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
							
		label = new CCLabel(cc_x, cc_y, cc_dx, cc_dy);
		
		label->cc_type = CC_LABEL;
		if (name) label->cc_name = name;
							
		if (!text.empty()) label->setText(_(text.c_str()));
		label->setHAlign(l_halign);
		//if (font_size) label->setFont(font_size);
		int fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
		if (font_size) fs = convertFontSize(font_size);
		label->setFont(fs);
		//
		if (font_color) label->setColor(color);
							
		if (widget) widget->addCCItem(label);
		if (window) window->addCCItem(label);
			
		node = node->xmlNextNode;
	}
}

// CCImage
void CNeutrinoApp::parseCCImage(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCImage:\n");
	
	CCImage* pic = NULL;
	
	char *name = NULL;
				
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	char* image = NULL;
						
	unsigned int cc_refresh = 0;
	
	while ((node = xmlGetNextOccurence(node, "IMAGE")) != NULL) 
	{
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
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
							
		pic = new CCImage(cc_x, cc_y, cc_dx, cc_dy);
		
		pic->cc_type = CC_IMAGE;
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
		if (window) window->addCCItem(pic);	
			
		node = node->xmlNextNode;
	}	
}

// CCTime
void CNeutrinoApp::parseCCTime(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCTime:\n");
	
	CCTime* time = NULL;
	
	char *name = NULL;
				
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	//int font_size = -1;
	char * font_size = NULL;
	char* font_color = NULL;
						
	unsigned int cc_refresh = 0;
	
	char* cc_format = NULL;
	
	while ((node = xmlGetNextOccurence(node, "TIME")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//				
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
		//font_size = xmlGetSignedNumericAttribute(node, "font", 0);
		font_size = xmlGetAttribute(node, (char *)"font");
		font_color = xmlGetAttribute(node, (char*)"fontcolor");
		
		uint8_t color = COL_MENUCONTENT;
		
		if (font_color) color = convertFontColor(font_color);
						
		cc_refresh = xmlGetSignedNumericAttribute(node, "refresh", 0);
					
		cc_format = xmlGetAttribute(node, (char*)"format");
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
							
		time = new CCTime(cc_x, cc_y, cc_dx, cc_dy);
		
		time->cc_type = CC_TIME;
		if (name) time->cc_name = name;
							
		if (cc_format != NULL) time->setFormat(_(cc_format));
		if (cc_refresh) time->enableRepaint();
		//if (font_size) time->setFont(font_size);
		int fs = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
		if (font_size) fs = convertFontSize(font_size);
		time->setFont(fs);
		//
		if (font_color) time->setColor(color);
							
		if (widget) widget->addCCItem(time);
		if (window) window->addCCItem(time);
			
		node = node->xmlNextNode;
	}
}

// CCButton
void CNeutrinoApp::parseCCButtons(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCButtons:\n");
	
	CCButtons* cButton = NULL;
	
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	unsigned int head = 0;
	unsigned int mode = BUTTON_BUTTON;
	
	_xmlNodePtr buttonlabel_node = NULL;
	
	while ((node = xmlGetNextOccurence(node, "BUTTON")) != NULL) 
	{
		//
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
		//
		head = xmlGetSignedNumericAttribute(node, "head", 0);
		mode = xmlGetSignedNumericAttribute(node, "mode", 0);
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
				
		cButton = new CCButtons(cc_x, cc_y, cc_dx, cc_dy);
		
		cButton->setMode(mode);
				
		// BUTTON_LABEL
		buttonlabel_node = node->xmlChildrenNode;
		
		char *button = NULL;
		char *localename = NULL;
		char *color = NULL;
				
		while ((buttonlabel_node = xmlGetNextOccurence(buttonlabel_node, "BUTTON_LABEL")) != NULL) 
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
					
		if (widget) widget->addCCItem(cButton);
		if (window) window->addCCItem(cButton);
				
		node = node->xmlNextNode;
	}
}

// CCHline
void CNeutrinoApp::parseCCHline(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCHline:\n");
	
	CCHline* hline = NULL;
	
	char *name = NULL;
	
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	//unsigned int gradient = 0;
	char * gradient = NULL;
	
	while ((node = xmlGetNextOccurence(node, "HLINE")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
		gradient = xmlGetAttribute(node, (char *)"gradient");
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
				
		hline = new CCHline(cc_x, cc_y, cc_dx, cc_dy);
		
		hline->cc_type = CC_HLINE;
		if (name) hline->cc_name = name;
		
		int gr = NOGRADIENT;
		if (gradient) gr = convertGradient(gradient);
		hline->setGradient(gr);
					
		if (widget) widget->addCCItem(hline);
		if (window) window->addCCItem(hline);
				
		node = node->xmlNextNode;
	}
}

// CCVline
void CNeutrinoApp::parseCCVline(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCVline:\n");
	
	CCVline* vline = NULL;
	
	char *name = NULL;
	
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	char *gradient = NULL;
	
	while ((node = xmlGetNextOccurence(node, "VLINE")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
		gradient = xmlGetAttribute(node, (char *)"gradient");
				
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
		
		vline = new CCVline(cc_x, cc_y, cc_dx, cc_dy);
		
		vline->cc_type = CC_VLINE;
		if (name) vline->cc_name = name;
		
		int gr = NOGRADIENT;
		if (gradient) gr = convertGradient(gradient);
		vline->setGradient(gr);
					
		if (widget) widget->addCCItem(vline);
		if (window) window->addCCItem(vline);
				
		node = node->xmlNextNode;
	}
}

// CCPig
void CNeutrinoApp::parseCCPig(_xmlNodePtr node, CWidget* widget, CWindow* window)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseCCPig:\n");
	
	CCPig* pig = NULL;
	
	char *name = NULL;
	
	unsigned int cc_x = 0;
	unsigned int cc_y = 0;
	unsigned int cc_dx = 0;
	unsigned int cc_dy = 0;
	
	while ((node = xmlGetNextOccurence(node, "PIG")) != NULL) 
	{
		//
		name = xmlGetAttribute(node, (char*)"name");
		
		//
		cc_x = xmlGetSignedNumericAttribute(node, "posx", 0);
		cc_y = xmlGetSignedNumericAttribute(node, "posy", 0);
		cc_dx = xmlGetSignedNumericAttribute(node, "width", 0);
		cc_dy = xmlGetSignedNumericAttribute(node, "height", 0);
		
		// recalculate posx / posy
		if (widget && widget->getMenuPosition())
		{			
			if (cc_x < widget->getWindowsPos().iX)
				cc_x = widget->getWindowsPos().iX;
				
			if (cc_y < widget->getWindowsPos().iY)
				cc_y = widget->getWindowsPos().iY;
		}
				
		pig = new CCPig(cc_x, cc_y, cc_dx, cc_dy);
		
		pig->cc_type = CC_PIG;
		if (name) pig->cc_name = name;
					
		if (widget) widget->addCCItem(pig);
		if (window) window->addCCItem(pig);
				
		node = node->xmlNextNode;
	}
}

// widget key
void CNeutrinoApp::parseKey(_xmlNodePtr node, CWidget* widget)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseKey:\n");
	
	char* key_name = NULL;
	char* key_actionkey = NULL;
	char *key_target = NULL;
	
	while ((node = xmlGetNextOccurence(node, "KEY")) != NULL) 
	{
		key_name = xmlGetAttribute(node, (char*)"name");
		key_actionkey = xmlGetAttribute(node, (char*)"actionkey");
		key_target = xmlGetAttribute(node, (char*)"target");
		
		neutrino_msg_t key = RC_nokey;		
		CMenuTarget* key_parent = NULL;
		
		if (key_name) key = convertKey(key_name);		
		if (key_target) key_parent = convertTarget(key_target);
				
		if (widget) widget->addKey(key, key_parent, key_actionkey);
			
		node = node->xmlNextNode;
	}
}

// parseCWidget
/*
CWidget *CNeutrinoApp::parseCWidget(const char * const filename, const char * const widgetName, bool data)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::parseCWidget: %s\n", widgetName);
	
	CWidget *ret = NULL;
	
	//
	char* name = NULL;
			
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int dx = 0;
	unsigned int dy = 0;
			
	char* color = NULL;
	char *gradient = NULL;
	char * corner = NULL;
	char * radius = NULL;
	char * border = NULL;
			
	unsigned int paintframe = 0;
	unsigned int savescreen = 0;
	unsigned int timeout = 0;
	char *position = NULL;
	
	//
	parseSkinInputXml(filename, data);
	
	if (parser)
	{
		_xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
			
		while ( (search = xmlGetNextOccurence(search, "WIDGET")) != NULL ) 
		{
			CWidget *widget = NULL;
			
			//
			name = xmlGetAttribute(search, (char*)"name");
			
			//
			if (!strcmp(name, widgetName) )
			{	
				x = xmlGetSignedNumericAttribute(search, "posx", 0);
				y = xmlGetSignedNumericAttribute(search, "posy", 0);
				dx = xmlGetSignedNumericAttribute(search, "width", 0);
				dy = xmlGetSignedNumericAttribute(search, "height", 0);
					
				color = xmlGetAttribute(search, (char*)"color");
				gradient = xmlGetAttribute(search, (char *)"gradient");
				corner = xmlGetAttribute(search, (char *)"corner");
				radius = xmlGetAttribute(search, (char *)"radius");
				border = xmlGetSignedNumericAttribute(search, "border", 0);
					
				paintframe = xmlGetSignedNumericAttribute(search, "paintframe", 0);
				savescreen = xmlGetSignedNumericAttribute(search, "savescreen", 0);
				timeout = xmlGetSignedNumericAttribute(search, "timeout", 0);
				position = xmlGetAttribute(search, (char *)"position");
					
				// parse color
				uint32_t wColor = COL_MENUCONTENT_PLUS_0;	
				if (color != NULL) wColor = convertColor(color);
					
				//
				widget = new CWidget(x, y, dx, dy);
					
				if (name != NULL) widget->name = name;
				widget->paintMainFrame(paintframe);
				if (color != NULL) widget->setColor(wColor);
				int gr = NOGRADIENT;
				if (gradient) gr = convertGradient(gradient);
				widget->setGradient(gr);

				int co = CORNER_NONE;
				int ra = NO_RADIUS;
				if (corner) co = convertCorner(corner);
				if (radius) ra = convertRadius(radius);
				widget->setCorner(ra, co);
				widget->setBorderMode(border);
				if (savescreen) widget->enableSaveScreen();
				widget->setTimeOut(timeout);
				int pos = MENU_POSITION_NONE;
				if (position) pos = convertMenuPosition(position);
				widget->setMenuPosition(pos);
					
				// WINDOW
				parseCWindow(search->xmlChildrenNode, widget);
					
				// HEAD
				parseCHead(search->xmlChildrenNode, widget);
					
				// FOOT
				parseCFoot(search->xmlChildrenNode, widget);
					
				// LISTBOX
				parseClistBox(search->xmlChildrenNode, widget);
					
				// TEXTBOX
				parseCTextBox(search->xmlChildrenNode, widget);
					
				// LABEL
				parseCCLabel(search->xmlChildrenNode, widget);
					
				// IMAGE
				parseCCImage(search->xmlChildrenNode, widget);
					
				// TIME
				parseCCTime(search->xmlChildrenNode, widget);
					
				// BUTTONS
				parseCCButtons(search->xmlChildrenNode, widget);
					
				// HLINE
				parseCCHline(search->xmlChildrenNode, widget);
					
				// VLINE
				parseCCVline(search->xmlChildrenNode, widget);
				
				// PIG
				parseCCPig(search->xmlChildrenNode, widget);
					
				// KEY
				parseKey(search->xmlChildrenNode, widget);
				
				ret = widget;
			}
							
			//
			search = search->xmlNextNode;		
		}
		
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	return ret;
}
*/

//
bool CNeutrinoApp::parseSkinInputXml(const char* const filename, bool xml_data)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::parseSkinInputXml: %s\n", filename);
	
	bool ret = false;
	
	//
	if(parser) 
	{
		delete parser;
		parser = NULL;
	}

	if (xml_data)
	{
		parser = parseXml(filename);
		ret = true;
	}
	else
	{
		parser = parseXmlFile(filename);
		ret = true;
	}
	
	return ret;
}

// parseSkin
bool CNeutrinoApp::parseSkin(const char* const filename, bool xml_data)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::parseSkin: %s\n", filename);
	
	parseSkinInputXml(filename, xml_data);
	
	if (parser)
	{			
		_xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode; //WIDGET
		
		if (search) 
		{
			while ((search = xmlGetNextOccurence(search, "WIDGET")) != NULL) 
			{
				//
				CWidget* wdg = NULL;
				
				//
				char* name = NULL;
				
				//		
				unsigned int x = 0;
				unsigned int y = 0;
				unsigned int dx = 0;
				unsigned int dy = 0;
				
				char* color = NULL;
				char * gradient = NULL;
				char * corner = NULL;
				char * radius = NULL;
				char * border = NULL;
				
				unsigned int paintframe = 0;
				unsigned int savescreen = 0;
				char * position = NULL;
				unsigned int timeout = 0;
				
				//
				name = xmlGetAttribute(search, (char*)"name");
				
				//
				x = xmlGetSignedNumericAttribute(search, "posx", 0);
				y = xmlGetSignedNumericAttribute(search, "posy", 0);
				dx = xmlGetSignedNumericAttribute(search, "width", 0);
				dy = xmlGetSignedNumericAttribute(search, "height", 0);
				
				color = xmlGetAttribute(search, (char*)"color");
				gradient = xmlGetAttribute(search, (char *)"gradient");
				corner = xmlGetAttribute(search, (char *)"corner");
				radius = xmlGetAttribute(search, (char *)"radius");
				border = xmlGetAttribute(search, (char *)"border");
				paintframe = xmlGetSignedNumericAttribute(search, "paintframe", 0);
				savescreen = xmlGetSignedNumericAttribute(search, "savescreen", 0);
				timeout = xmlGetSignedNumericAttribute(search, "timeout", 0);
				position = xmlGetAttribute(search, (char *)"position");

				//				
				wdg = new CWidget(x, y, dx, dy);
				
				// name
				if (name != NULL) wdg->name = name;
				
				// paintframe
				wdg->paintMainFrame(paintframe);
				
				// color
				uint32_t wColor = COL_MENUCONTENT_PLUS_0;	
				if (color != NULL) wColor = convertColor(color);
				wdg->setColor(wColor);
				
				// gradient
				int gr = NOGRADIENT;
				if (gradient) gr = convertGradient(gradient);
				wdg->setGradient(gr);
				
				// corner / radius
				int co = CORNER_NONE;
				int ra = NO_RADIUS;
				if (corner) co = convertCorner(corner);
				if (radius) ra = convertRadius(radius);
				wdg->setCorner(ra, co);

				// border
				int br = BORDER_NO;
				if (border) br = convertBorder(border);
				wdg->setBorderMode(br);
				
				// savescreen
				if (savescreen) wdg->enableSaveScreen();
				
				// timeout
				wdg->setTimeOut(timeout);
				
				// position
				int pos = MENU_POSITION_NONE;
				if (position) pos = convertMenuPosition(position);
				wdg->setMenuPosition(pos);
				
				// WINDOW
				parseCWindow(search->xmlChildrenNode, wdg);
				
				// HEAD
				parseCHead(search->xmlChildrenNode, wdg);
				
				// FOOT
				parseCFoot(search->xmlChildrenNode, wdg);
				
				// LISTBOX
				parseClistBox(search->xmlChildrenNode, wdg);
				
				// TEXTBOX
				parseCTextBox(search->xmlChildrenNode, wdg);
				
				// LABEL
				parseCCLabel(search->xmlChildrenNode, wdg);
				
				// IMAGE
				parseCCImage(search->xmlChildrenNode, wdg);
				
				// TIME
				parseCCTime(search->xmlChildrenNode, wdg);
				
				// BUTTONS
				parseCCButtons(search->xmlChildrenNode, wdg);
				
				// HLINE
				parseCCHline(search->xmlChildrenNode, wdg);
				
				// VLINE
				parseCCVline(search->xmlChildrenNode, wdg);
				
				// PIG
				parseCCPig(search->xmlChildrenNode, wdg);
				
				// KEY
				parseKey(search->xmlChildrenNode, wdg);
				
				//
				widgets.push_back(wdg);				
							
				//
				search = search->xmlNextNode;		
			}
		}
		
		xmlFreeDoc(parser);
		parser = NULL;
	}
	
	//
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::parseSkin: widgets count:%d\n", (int)widgets.size());
	
	return true;
}

// getWidget
CWidget* CNeutrinoApp::getWidget(const char* const name)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::getWidget: %s\n", name);
	
	CWidget* ret = NULL;
	
	for (unsigned int i = 0; i < (unsigned int )widgets.size(); i++)
	{
		if ( (widgets[i] != NULL) && (widgets[i]->name == name) )
		{
			ret = widgets[i];
			break;
		}
	}
	
	return ret;
}
/*
bool CNeutrinoApp::widget_exists(const char* const name)
{
	bool ret = false;
	
	for (unsigned int i = 0; i < (unsigned int )widgets.size(); i++)
	{
		if ( (widgets[i] != NULL) && (widgets[i]->name == name) )
		{
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::widget_exists: (%s)\n", name);
			
			ret = true;
			break;
		}
	}
	
	return ret;
}
*/
// eraseWidget
bool CNeutrinoApp::eraseWidget(const char* const name)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::eraseWidget: %s\n", name);
	
	bool ret = false;
	
	for (unsigned int i = 0; i < (unsigned int )widgets.size(); i++)
	{
		if ( (widgets[i] != NULL) && (widgets[i]->name == name) )
		{
			widgets.erase(widgets.begin() + i);
			ret = true;
			break;
		}
	}
	
	return ret;
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
	
	// parse skin
	std::string skinFileName = skinPath.c_str();
	skinFileName += "/skin.xml";
	
	if (parseSkin(skinFileName.c_str()))
	{
		// read skin font/icons/buttons/hints
		std::string fontFileName;
		
		struct dirent **namelist;
		int i = 0;
		
		if (CNeutrinoApp::getInstance()->skin_exists(skinName.c_str()))
		{
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
			}
			 
			strcpy( g_settings.font_file, fontFileName.c_str() );
			
			CNeutrinoApp::getInstance()->setupFonts(g_settings.font_file);
			
			// setIconPath
			std::string iconsDir = CONFIGDIR "/skins/";
			iconsDir += skinName.c_str();
			iconsDir += "/icons/";
			
			// check if not empty
			i = scandir(iconsDir.c_str(), &namelist, 0, 0);
			if(i < 0)
			{
				g_settings.icons_dir = DATADIR "/icons/"; //fallback to default if empty
			}
			else
			{
				g_settings.icons_dir = iconsDir;
				free(namelist);
			}
			
			frameBuffer->setIconBasePath(g_settings.icons_dir);
			
			// setButtonPath
			std::string buttonsDir = CONFIGDIR "/skins/";
			buttonsDir += skinName.c_str();
			buttonsDir += "/buttons/";
			
			// check if not empty
			i = scandir(buttonsDir.c_str(), &namelist, 0, 0);
			if(i < 0)
			{
				g_settings.buttons_dir = DATADIR "/buttons/"; //fallback to default if empty
			}
			else
			{
				g_settings.buttons_dir = buttonsDir;
				free(namelist);
			}
			
			frameBuffer->setButtonBasePath(g_settings.buttons_dir);
			
			// setHintPath
			std::string hintsDir = CONFIGDIR "/skins/";
			hintsDir += skinName.c_str();
			hintsDir += "/hints/";
			
			// check if not empty
			i = scandir(hintsDir.c_str(), &namelist, 0, 0);
			if(i < 0)
			{
				g_settings.hints_dir = DATADIR "/hints/"; //fallback to default if empty
			}
			else
			{
				g_settings.hints_dir = hintsDir;
				free(namelist);
			}
			
			frameBuffer->setHintBasePath(g_settings.hints_dir);
			
			// setSpinnerPath
			std::string spinnerDir = CONFIGDIR "/skins/";
			hintsDir += skinName.c_str();
			hintsDir += "/spinner/";
			
			// check if not empty
			i = scandir(spinnerDir.c_str(), &namelist, 0, 0);
			if(i < 0)
			{
				g_settings.spinner_dir = DATADIR "/spinner/"; //fallback to default if empty
			}
			else
			{
				g_settings.spinner_dir = spinnerDir;
				free(namelist);
			}
			
			frameBuffer->setSpinnerBasePath(g_settings.spinner_dir);
			
			// setup colors / corners / position
			std::string skinConfigFile = CONFIGDIR "/skins/";
			skinConfigFile += skinName.c_str();
			skinConfigFile += "/";
			skinConfigFile += skinName.c_str();
			skinConfigFile += ".config";
			
			readSkinConfig(skinConfigFile.c_str());
		}
		else //fallback to default (neutrino intern)
		{
			strcpy( g_settings.font_file, DATADIR "/fonts/arial.ttf");
			
			CNeutrinoApp::getInstance()->setupFonts(DATADIR "/fonts/arial.ttf");
			
			g_settings.icons_dir = DATADIR "/icons/";
			g_settings.buttons_dir = DATADIR "/buttons/";
			g_settings.hints_dir = DATADIR "/hints/";
			g_settings.spinner_dir = DATADIR "/spinner/";
			
			frameBuffer->setIconBasePath(DATADIR "/icons/");
			frameBuffer->setButtonBasePath(DATADIR "/buttons/");
			frameBuffer->setHintBasePath(DATADIR "/hints/");
			frameBuffer->setSpinnerBasePath(DATADIR "/spinner/");
		}
	}	
}

//
bool CNeutrinoApp::skin_exists(const char* const filename)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::skin_exists: %s\n", filename);
	
	bool ret = false;
	
	std::string skin = CONFIGDIR "/skins/";
	skin += filename;
	skin += "/skin.xml";
	
	if (::file_exists(skin.c_str()))
		ret = true;
	
	return ret;
}

void CNeutrinoApp::unloadSkin()
{
	// clear all skin widgets
	for (unsigned int i = 0; i < (unsigned int) widgets.size(); i++)
	{
		if (widgets[i])
		{
			delete widgets[i];
			widgets[i] = NULL;
		}
	}
	
	widgets.clear();
}

//
void CNeutrinoApp::readSkinConfig(const char* const filename)
{
	dprintf(DEBUG_INFO, "CNeutrinpApp::readSkinConfig %s\n", filename);
	
	CConfigFile* skinConfig = new CConfigFile(',');
	
	if(skinConfig->loadConfig(filename))
	{
		g_settings.menu_Head_alpha = skinConfig->getInt32( "menu_Head_alpha", 15);
		g_settings.menu_Head_red = skinConfig->getInt32( "menu_Head_red", 15);
		g_settings.menu_Head_green = skinConfig->getInt32( "menu_Head_green", 15);
		g_settings.menu_Head_blue = skinConfig->getInt32( "menu_Head_blue", 15);

		g_settings.menu_Head_Text_alpha = skinConfig->getInt32( "menu_Head_Text_alpha", 0);
		g_settings.menu_Head_Text_red = skinConfig->getInt32( "menu_Head_Text_red", 100 );
		g_settings.menu_Head_Text_green = skinConfig->getInt32( "menu_Head_Text_green", 100 );
		g_settings.menu_Head_Text_blue = skinConfig->getInt32( "menu_Head_Text_blue", 100 );
	
		g_settings.menu_Content_alpha = skinConfig->getInt32( "menu_Content_alpha", 20);
		g_settings.menu_Content_red = skinConfig->getInt32( "menu_Content_red", 25);
		g_settings.menu_Content_green = skinConfig->getInt32( "menu_Content_green", 25);
		g_settings.menu_Content_blue = skinConfig->getInt32( "menu_Content_blue", 25);
		
		g_settings.menu_Content_Text_alpha = skinConfig->getInt32( "menu_Content_Text_alpha", 0);
		g_settings.menu_Content_Text_red = skinConfig->getInt32( "menu_Content_Text_red", 85 );
		g_settings.menu_Content_Text_green = skinConfig->getInt32( "menu_Content_Text_green", 85 );
		g_settings.menu_Content_Text_blue = skinConfig->getInt32( "menu_Content_Text_blue", 85 );
	
		g_settings.menu_Content_Selected_alpha = skinConfig->getInt32( "menu_Content_Selected_alpha", 20);
		g_settings.menu_Content_Selected_red = skinConfig->getInt32( "menu_Content_Selected_red", 75);
		g_settings.menu_Content_Selected_green = skinConfig->getInt32( "menu_Content_Selected_green", 75);
		g_settings.menu_Content_Selected_blue = skinConfig->getInt32( "menu_Content_Selected_blue", 75);
		
		g_settings.menu_Content_Selected_Text_alpha = skinConfig->getInt32( "menu_Content_Selected_Text_alpha", 0);
		g_settings.menu_Content_Selected_Text_red = skinConfig->getInt32( "menu_Content_Selected_Text_red", 25 );
		g_settings.menu_Content_Selected_Text_green = skinConfig->getInt32( "menu_Content_Selected_Text_green", 25 );
		g_settings.menu_Content_Selected_Text_blue = skinConfig->getInt32( "menu_Content_Selected_Text_blue", 25 );
	
		g_settings.menu_Content_inactive_alpha = skinConfig->getInt32( "menu_Content_inactive_alpha", 20);
		g_settings.menu_Content_inactive_red = skinConfig->getInt32( "menu_Content_inactive_red", 25);
		g_settings.menu_Content_inactive_green = skinConfig->getInt32( "menu_Content_inactive_green", 25);
		g_settings.menu_Content_inactive_blue = skinConfig->getInt32( "menu_Content_inactive_blue", 25);
		
		g_settings.menu_Content_inactive_Text_alpha = skinConfig->getInt32( "menu_Content_inactive_Text_alpha", 0);
		g_settings.menu_Content_inactive_Text_red = skinConfig->getInt32( "menu_Content_inactive_Text_red", 55);
		g_settings.menu_Content_inactive_Text_green = skinConfig->getInt32( "menu_Content_inactive_Text_green", 55);
		g_settings.menu_Content_inactive_Text_blue = skinConfig->getInt32( "menu_Content_inactive_Text_blue", 55);

		g_settings.infobar_alpha = skinConfig->getInt32( "infobar_alpha", 20);
		g_settings.infobar_red = skinConfig->getInt32( "infobar_red", 25);
		g_settings.infobar_green = skinConfig->getInt32( "infobar_green", 25);
		g_settings.infobar_blue = skinConfig->getInt32( "infobar_blue", 25);
		
		g_settings.infobar_Text_alpha = skinConfig->getInt32( "infobar_Text_alpha", 0);
		g_settings.infobar_Text_red = skinConfig->getInt32( "infobar_Text_red", 100);
		g_settings.infobar_Text_green = skinConfig->getInt32( "infobar_Text_green", 100);
		g_settings.infobar_Text_blue = skinConfig->getInt32( "infobar_Text_blue", 100);
		
		g_settings.infobar_colored_events_alpha = skinConfig->getInt32( "infobar_colored_events_alpha", 0);
		g_settings.infobar_colored_events_red = skinConfig->getInt32( "infobar_colored_events_red", 95);
		g_settings.infobar_colored_events_green = skinConfig->getInt32( "infobar_colored_events_green", 70);
		g_settings.infobar_colored_events_blue = skinConfig->getInt32( "infobar_colored_events_blue", 0);
	
		g_settings.menu_Foot_alpha = skinConfig->getInt32( "menu_Foot_alpha", 15);
		g_settings.menu_Foot_red = skinConfig->getInt32( "menu_Foot_red", 15);
		g_settings.menu_Foot_green = skinConfig->getInt32( "menu_Foot_green", 15);
		g_settings.menu_Foot_blue = skinConfig->getInt32( "menu_Foot_blue", 15);
		
		g_settings.menu_Foot_Text_alpha = skinConfig->getInt32( "menu_Foot_Text_alpha", 0);
		g_settings.menu_Foot_Text_red = skinConfig->getInt32( "menu_Foot_Text_red", 100);
		g_settings.menu_Foot_Text_green = skinConfig->getInt32( "menu_Foot_Text_green", 100);
		g_settings.menu_Foot_Text_blue = skinConfig->getInt32( "menu_Foot_Text_blue", 100);

		g_settings.menu_Hint_alpha = skinConfig->getInt32( "menu_Hint_alpha", 0);
		g_settings.menu_Hint_red = skinConfig->getInt32( "menu_Hint_red", 15);
		g_settings.menu_Hint_green = skinConfig->getInt32( "menu_Hint_green", 15);
		g_settings.menu_Hint_blue = skinConfig->getInt32( "menu_Hint_blue", 15);
		
		g_settings.menu_Hint_Text_alpha = skinConfig->getInt32( "menu_Hint_Text_alpha", 0);
		g_settings.menu_Hint_Text_red = skinConfig->getInt32( "menu_Hint_Text_red", 85);
		g_settings.menu_Hint_Text_green = skinConfig->getInt32( "menu_Hint_Text_green", 85);
		g_settings.menu_Hint_Text_blue = skinConfig->getInt32( "menu_Hint_Text_blue", 85);
		
		// head
		g_settings.Head_gradient = skinConfig->getInt32("Head_gradient", DARK2LIGHT2DARK);
		g_settings.Head_corner = skinConfig->getInt32("Head_corner", CORNER_TOP);
		g_settings.Head_radius = skinConfig->getInt32("Head_radius", RADIUS_MID);
		g_settings.Head_line = skinConfig->getBool("Head_line", false);
		
		// foot
		g_settings.Foot_gradient = skinConfig->getInt32("Foot_gradient", DARK2LIGHT2DARK);
		g_settings.Foot_corner = skinConfig->getInt32("Foot_corner", CORNER_BOTTOM);
		g_settings.Foot_radius = skinConfig->getInt32("Foot_radius", RADIUS_MID);
		g_settings.Foot_line = skinConfig->getBool("Foot_line", false);
		
		// infobar
		g_settings.infobar_gradient = skinConfig->getInt32("infobar_gradient", NOGRADIENT);
		g_settings.infobar_gradient_direction = skinConfig->getInt32("infobar_gradient_direction", GRADIENT_HORIZONTAL);
		g_settings.infobar_corner = skinConfig->getInt32("infobar_corner", CORNER_ALL);
		g_settings.infobar_radius = skinConfig->getInt32("infobar_radius", RADIUS_MID);
		g_settings.infobar_border = skinConfig->getBool("infobar_border", false);
		g_settings.infobar_buttonbar = skinConfig->getBool("infobar_buttonbar", true);
		g_settings.infobar_buttonline = skinConfig->getBool("infobar_buttonline", false);
		
		// itemInfo
		g_settings.Hint_border = skinConfig->getBool("Hint_border", true);
		g_settings.Hint_gradient = skinConfig->getInt32("Hint_gradient", NOGRADIENT);
		g_settings.Hint_radius = skinConfig->getInt32("Hint_radius", NO_RADIUS);
		g_settings.Hint_corner = skinConfig->getInt32("Hint_corner", CORNER_ALL);
		
		// progressbar color
		g_settings.progressbar_color = skinConfig->getInt32("progressbar_color", 1);
		g_settings.progressbar_gradient = skinConfig->getInt32("progressbar_gradient", DARK2LIGHT2DARK);
		
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
	dprintf(DEBUG_INFO, "CNeutrinoApp::saveFile: %s\n", filename);
	
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
	
	//
	skinConfig->setInt32("Head_gradient", g_settings.Head_gradient);
	skinConfig->setInt32("Head_corner", g_settings.Head_corner);
	skinConfig->setInt32("Head_radius", g_settings.Head_radius);
	skinConfig->setBool("Head_line", g_settings.Head_line);
	
	//
	skinConfig->setInt32("Foot_gradient", g_settings.Foot_gradient);
	skinConfig->setInt32("Foot_corner", g_settings.Foot_corner);
	skinConfig->setInt32("Foot_radius", g_settings.Foot_radius);
	skinConfig->setBool("Foot_line", g_settings.Foot_line);
	
	//
	skinConfig->setInt32("infobar_gradient", g_settings.infobar_gradient);
	skinConfig->setInt32("infobar_gradient_direction", g_settings.infobar_gradient_direction);
	skinConfig->setInt32("infobar_corner", g_settings.infobar_corner);
	skinConfig->setInt32("infobar_radius", g_settings.infobar_radius);
	skinConfig->setBool("infobar_buttonbar", g_settings.infobar_buttonbar);
	skinConfig->setBool("infobar_buttonline", g_settings.infobar_buttonline);
	skinConfig->setBool("infobar_border", g_settings.infobar_border);
	
	// itemInfo
	skinConfig->setBool("Hint_border", g_settings.Hint_border);
	skinConfig->setInt32("Hint_gradient", g_settings.Hint_gradient);
	skinConfig->setInt32("Hint_radius", g_settings.Hint_radius);
	skinConfig->setInt32("Hint_corner", g_settings.Hint_corner);
	
	// progressbar color
	skinConfig->setInt32("progressbar_color", g_settings.progressbar_color);
	skinConfig->setInt32("progressbar_gradient", g_settings.progressbar_gradient);
		
	skinConfig->setString("font_file", g_settings.font_file);

	if (!skinConfig->saveConfig(filename))
		printf("CNeutrinoApp::saveSkinConfig %s write error\n", filename);
}

//// helpers methods
//
int CNeutrinoApp::execSkinWidget(const char* const name, CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::execSkinWidget: actionKey: (%s)\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	CWidget * widget = NULL;
	
	widget = getWidget(name);
	
	if (widget)
	{
		ret = widget->exec(parent, actionKey);
	}
	
	return ret;	
}

bool CNeutrinoApp::paintSkinWidget(const char* const name)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::paintSkinWidget:\n");
	
	bool ret = false;
	CWidget * widget = NULL;
	
	widget = getWidget(name);
	
	if (widget)
	{
		widget->paint();
		ret = true;
	}
	
	return ret;	
}

bool CNeutrinoApp::hideSkinWidget(const char* const name)
{
	dprintf(DEBUG_INFO, "CNeutrinoApp::hideSkinWidget:\n");
	
	bool ret = false;
	CWidget * widget = NULL;
	
	widget = getWidget(name);
	
	if (widget)
	{
		widget->hide();
		ret = true;
	}
	
	return ret;	
}


