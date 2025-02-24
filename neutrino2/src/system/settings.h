/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2001 Steffen Hehn 'McClean'
  Homepage: http://dbox.cyberphoria.org/
  
  $Id: settings.h 23.09.2023 mohousch Exp $

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

#ifndef __settings__
#define __settings__

#include <string>

#include <zapit/zapit.h>

#include <configfile.h>

#include <system/localize.h>

#include <gui/widget/icons.h>

#include <string>
#include <list>


#define MAX_INPUT_CHARS		34

struct SNeutrinoSettings
{
	// VIDEO
	int video_Mode;
	int analog_mode;	//scart
	int hdmi_color_space;
	int video_Ratio;
	int video_Format;
	int wss_mode;
	unsigned char contrast;
	unsigned char saturation;
	unsigned char brightness;
	unsigned char tint;
	// END VIDEO

	// AUDIO
	int audio_AnalogMode;
	int audio_DolbyDigital;
	int hdmi_dd;
	int avsync;
	int ac3_delay;
	int pcm_delay;
	//
	
	// subs
	int auto_lang;
	int auto_subs;
	char pref_lang[3][30];
	char pref_subs[3][30];
	char pref_epgs[3][30];
	// END AUDIO

	// PARENTALLOCK
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[5];
	// END PARENTALLOCK

	// NETWORK
#define NETWORK_NFS_NR_OF_ENTRIES 8
	std::string network_nfs_ip[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mac[NETWORK_NFS_NR_OF_ENTRIES][MAX_INPUT_CHARS + 1];
	char network_nfs_local_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	char network_nfs_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	int  network_nfs_automount[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mount_options1[NETWORK_NFS_NR_OF_ENTRIES][MAX_INPUT_CHARS + 1];
	char network_nfs_mount_options2[NETWORK_NFS_NR_OF_ENTRIES][MAX_INPUT_CHARS +  1];
	int  network_nfs_type[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_username[NETWORK_NFS_NR_OF_ENTRIES][MAX_INPUT_CHARS + 1];
	char network_nfs_password[NETWORK_NFS_NR_OF_ENTRIES][MAX_INPUT_CHARS + 1];

	std::string network_ntpserver;
	std::string network_ntprefresh;
	int network_ntpenable;
	char ifname[10];
	// END NETWORK

	// RECORDING
	char record_safety_time_before[3];
	char record_safety_time_after[3];
	char network_nfs_recordingdir[100];
	int auto_timeshift; 	// permanent timeshift
	int record_hours;
	unsigned char recording_audio_pids_default;
	int recording_audio_pids_std;
	int recording_audio_pids_alt;
	int recording_audio_pids_ac3;
	int recording_epg_for_filename;
	int recording_save_in_channeldir;
	int recording_screenshot;
	// END RECORDING

	// MOVIEPLAYER
	char network_nfs_moviedir[100];
	// END MOVIEPLAYER

	// OSD
	unsigned char gtx_alpha;
	//
	char language[25];
	//
	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;
	
	unsigned char menu_Foot_alpha;
	unsigned char menu_Foot_red;
	unsigned char menu_Foot_green;
	unsigned char menu_Foot_blue;
	
	unsigned char menu_Foot_Text_alpha;
	unsigned char menu_Foot_Text_red;
	unsigned char menu_Foot_Text_green;
	unsigned char menu_Foot_Text_blue;

	unsigned char menu_Hint_alpha;
	unsigned char menu_Hint_red;
	unsigned char menu_Hint_green;
	unsigned char menu_Hint_blue;
	
	unsigned char menu_Hint_Text_alpha;
	unsigned char menu_Hint_Text_red;
	unsigned char menu_Hint_Text_green;
	unsigned char menu_Hint_Text_blue;
	
	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;
	
	unsigned char infobar_colored_events_alpha;
	unsigned char infobar_colored_events_red;
	unsigned char infobar_colored_events_green;
	unsigned char infobar_colored_events_blue;

	//
	char font_file[100];
	std::string icons_dir;
	std::string buttons_dir;
	std::string hints_dir;
	std::string spinner_dir;
	
	int timing_menu;
	int timing_channellist;
	int timing_epg;
	int timing_infobar;
	int timing_filebrowser;
	int timing_numericzap;

#define FONT_TYPE_COUNT 23
	enum FONT_TYPES {
		FONT_TYPE_MENU                	=  0,
		FONT_TYPE_MENU_TITLE          	=  1,
		FONT_TYPE_MENU_INFO           	=  2,
		FONT_TYPE_EPG_TITLE           	=  3,
		FONT_TYPE_EPG_INFO1           	=  4,
		FONT_TYPE_EPG_INFO2           	=  5,
		FONT_TYPE_EPG_DATE            	=  6,
		FONT_TYPE_EVENTLIST_TITLE     	=  7,
		FONT_TYPE_EVENTLIST_ITEMLARGE 	=  8,
		FONT_TYPE_EVENTLIST_ITEMSMALL 	=  9,
		FONT_TYPE_EVENTLIST_DATETIME  	= 10,
		FONT_TYPE_PLUGINLIST_ITEMLARGE  = 11,
		FONT_TYPE_PLUGINLIST_ITEMSMALL  = 12,
		FONT_TYPE_CHANNELLIST         	= 13,
		FONT_TYPE_CHANNELLIST_DESCR   	= 14,
		FONT_TYPE_CHANNELLIST_NUMBER  	= 15,
		FONT_TYPE_CHANNEL_NUM_ZAP     	= 16,
		FONT_TYPE_INFOBAR_NUMBER      	= 17,
		FONT_TYPE_INFOBAR_CHANNAME    	= 18,
		FONT_TYPE_INFOBAR_INFO        	= 19,
		FONT_TYPE_INFOBAR_SMALL       	= 20,
		FONT_TYPE_FILEBROWSER_ITEM    	= 21,
		FONT_TYPE_MENU_TITLELARGE	= 22
	};

	int screen_StartX;
	int screen_StartY;
	int screen_EndX;
	int screen_EndY;
	int screen_width;
	int screen_height;
	
	// font scaling
	int screen_xres;
	int screen_yres;
	
	int volume_pos;
	std::string preferred_skin;
	int item_info;
	std::string theme;
	// END OSD

	// KEYS
	char repeat_blocker[4];
	char repeat_genericblocker[4];

	long key_tvradio_mode;
	
	long key_channelList_pageup;
	long key_channelList_pagedown;
	long key_channelList_cancel;

	long key_quickzap_up;
	long key_quickzap_down;

	long key_bouquet_up;
	long key_bouquet_down;

	long key_subchannel_up;
	long key_subchannel_down;

	long key_zaphistory;
	long key_lastchannel;
	long key_list_start;
	long key_list_end;
	
	//pip
	long key_pip;

	// media	
	long key_movieplayer;
	long key_audioplayer;
	long key_pictureviewer;
	long key_inetradio;
	long key_moviebrowser;
	long key_pvr;
	
	// divers
	long key_timerlist;
	long key_screenshot;

	// USERMENU
        typedef enum
        {
                BUTTON_BLUE = 0,
#if defined (ENABLE_FUNCTIONKEYS)  
		BUTTON_F1 = 1,
		BUTTON_F2 = 2,
		BUTTON_F3 = 3,
		BUTTON_F4 = 4,
#endif
                BUTTON_MAX   // MUST be always the last in the list
        }USER_BUTTON;

        typedef enum
        {
		ITEM_NONE = 0,
		ITEM_TIMERLIST = 1,
		ITEM_PLUGIN = 2,
		ITEM_VTXT = 3,
		ITEM_REMOTE = 4,
		ITEM_DELETE_ZAPIT = 5,
		ITEM_DELETE_WEBTV = 6,
		ITEM_FREEMEMORY = 7,
		ITEM_LOAD_ZAPIT = 8,
		ITEM_LOAD_XMLTV = 9,
		ITEM_LOAD_EPG = 10,          
		ITEM_MAX   // MUST be always the last in the list
        }USER_ITEM;
	
        int usermenu[BUTTON_MAX][ITEM_MAX];
	// END KEYS

	// AUDIOPLAYER
	char network_nfs_audioplayerdir[100];
	int   audioplayer_highprio;
	// END AUDIOPLAYER

	// PICVIEWER
	char network_nfs_picturedir[100];
	int picviewer_slide_time;
	int picviewer_scaling;
	// END PICVIEWER

	// MISC OPTS
	int shutdown_real;
	int shutdown_real_rcdelay;
        char shutdown_count[4];
	int power_standby;
	int infobar_sat_display;
	int infobar_subchan_disp_pos;
	int rotor_swap;
	char timezone[150];

	int radiotext_enable;
	
	std::string logos_dir;
	int logos_show_logo;
	
	int debug_level;

	// channellist
	int zap_cycle;
	int virtual_zap_mode;
	int channellist_ca;
	int make_hd_list;
	int channellist_timescale;
	int channellist_alt;
	int channellist_number;

	// epg
	int epg_save;
	std::string epg_cache;
	std::string epg_old_events;
	std::string epg_max_events;
	std::string epg_extendedcache;
	std::string epg_dir;
	// xmltv
	std::vector<std::string> xmltv;
	int epg_xmltv;
	// localtv
	enum {
		SATIP_SERVERBOX_GUI_NHD2,
		SATIP_SERVERBOX_GUI_NMP,
		SATIP_SERVERBOX_GUI_ENIGMA2
	};
	
	int epg_enable_localtv_epg;
	std::string epg_serverbox_ip;
	int epg_serverbox_type;
	int epg_serverbox_gui;

	//filebrowser
	int filesystem_is_utf8;
	int filebrowser_showrights;
	int filebrowser_sortmethod;
	int filebrowser_denydirectoryleave;

	//zapit setup
	int lastChannelMode;
	std::string StartChannelTV;
	std::string StartChannelRadio;
	t_channel_id startchanneltv_id;
	t_channel_id startchannelradio_id;
	int startchanneltv_nr;
	int startchannelradio_nr;
	int uselastchannel;
	
	// inter without GUI setup
	int channel_mode;
	
	// vol
	char current_volume;
	char audio_step[3];
	// MISC OPTS

	// HDD
	int	hdd_sleep;
	int	hdd_noise;
	// END HDD

	// UPDATE
	char update_dir[100];
	char softupdate_proxyserver[MAX_INPUT_CHARS + 1];
	char softupdate_proxyusername[MAX_INPUT_CHARS + 1];
	char softupdate_proxypassword[MAX_INPUT_CHARS + 1];
	char softupdate_url_file[MAX_INPUT_CHARS + 1];
	// END UPDATE

	// VFD
	int lcd_brightness;
	int lcd_standbybrightness;
	int lcd_contrast;
	int lcd_power;
	int lcd_inverse;
	int lcd_statusline;
	int lcd_epgmode;
	int lcd_mode;
	int lcd_epgalign;
	char lcd_setting_dim_time[4];
	int lcd_setting_dim_brightness;
	int lcd_led;
	int lcd_minitv;
	int lcd_minitvfps;
	int lcd_picon;
	int lcd_weather;
	int lcd_standby_clock;
	// END VFD

	// tmdb
	std::string tmdbkey;
	int enable_tmdb_infos;
	int enable_tmdb_preview;
	// youtube
	std::string ytkey;
	//weather
	std::string weather_api_key;
	int show_weather;

	// progressbar color
	int progressbar_color;

	//head
	int Head_gradient;
	int Head_gradient_type;
	int Head_corner;
	int Head_radius;
	int Head_line;
	int Head_line_gradient;
	
	// foot
	int Foot_gradient;
	int Foot_gradient_type;
	int Foot_corner;
	int Foot_radius;
	int Foot_line;
	int Foot_line_gradient;
	
	// hint
	int Hint_gradient;
	int Hint_gradient_type;
	int Hint_border;
	int Hint_corner;
	int Hint_radius;
	
	// infobar
	int infobar_gradient;
	int infobar_gradient_type;
	int infobar_corner;
	int infobar_radius;
	int infobar_buttonbar;
	int infobar_buttonline;
	int infobar_buttonline_gradient;
	int infobar_border;
	
	// separator
	int sep_gradient;
	
	// cicam
	int ci_standby_reset;
	int ci_clock[4];
	int ci_delay;
	int ci_rpr[4];
	int ci_ignore_messages[4];
	int ci_save_pincode[4];
	std::string ci_pincode[4];
	int ci_check_live;
	int ci_tuner;
	
	// cec
	int hdmi_cec_mode;
	int hdmi_cec_standby;
	int hdmi_cec_volume;
	
	// personalisation
	int  personalize_tvradio;
	int  personalize_epgtimer;
	int  personalize_scart;
	int  personalize_features;
	int  personalize_service;
	int  personalize_information;
	int  personalize_powermenu;
	int  personalize_mediaplayer;
	std::string personalize_pincode;
	
	//// graphlcd
#ifdef ENABLE_GRAPHLCD
	int glcd_enable;
	int glcd_selected_config;
	int glcd_brightness;
	int glcd_brightness_standby;
#endif

#define FILESYSTEM_ENCODING_TO_UTF8(a) (g_settings.filesystem_is_utf8 ? (a) : Latin1_to_UTF8(a).c_str())
#define UTF8_TO_FILESYSTEM_ENCODING(a) (g_settings.filesystem_is_utf8 ? (a) : UTF8_to_Latin1(a).c_str())	
};

// default menu timing
#define DEFAULT_TIMING_MENU			600
#define DEFAULT_TIMING_CHANNELLIST		60
#define DEFAULT_TIMING_EPG			240
#define DEFAULT_TIMING_INFOBAR			6
#define DEFAULT_TIMING_FILEBROWSER		600
#define DEFAULT_TIMING_NUMERICZAP		3	

// lcdd
#ifdef ENABLE_4DIGITS
#define DEFAULT_LCD_BRIGHTNESS			0xF
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0x7
#define DEFAULT_LCD_DIM_BRIGHTNESS		0xA
#else
#define DEFAULT_LCD_BRIGHTNESS			0xFF
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0xAA
#define DEFAULT_LCD_DIM_BRIGHTNESS		0x7D
#endif
#define DEFAULT_LCD_CONTRAST			0x3F
#define DEFAULT_LCD_POWER			0x01
#define DEFAULT_LCD_INVERSE			0x00
#define DEFAULT_LCD_STATUSLINE			0x00	/* playtime */
#define DEFAULT_LCD_EPGMODE			0x03	/* channel / title */
#ifdef ENABLE_4DIGITS
#define DEFAULT_LCD_MODE			0x2	/* time */
#else
#define DEFAULT_LCD_MODE			0x01	/* channel */
#endif
#define DEFAULT_LCD_EPGALIGN			0x01    /* centered */
#ifdef ENABLE_4DIGITS
#define MAXBRIGHTNESS				0xF
#else
#define MAXBRIGHTNESS				0xFF
#endif
#define MAXCONTRAST				0x3F

// corners (osd)
#define NO_RADIUS				0
#define RADIUS_SMALL    			5
#define RADIUS_MID      			10
#define RADIUS_LARGE    			15
#define RADIUS_VERYLARGE			20

//
#define BORDER_LEFT				10
#define BORDER_RIGHT				10
#define SCROLLBAR_WIDTH				10 // 15
#define ICON_OFFSET				2 // offset from left border
#define ICON_TO_ICON_OFFSET			2

// CMenuWidget
#define ITEM_ICON_W				128
#define ITEM_ICON_H				128
#define ITEM_ICON_W_MINI			100
#define ITEM_ICON_H_MINI			40 // 80

// CHintBox
#define HINTBOX_MAX_HEIGHT 			0.4*DEFAULT_XRES
#define HINTBOX_MAX_WIDTH  			(g_settings.screen_EndX - g_settings.screen_StartX )
#define HINTBOX_WIDTH				650

// CMessageBox
#define MESSAGEBOX_MAX_HEIGHT 			0.4*DEFAULT_XRES
#define MESSAGEBOX_MAX_WIDTH  			(g_settings.screen_EndX - g_settings.screen_StartX )
#define MESSAGEBOX_WIDTH			650

// helpBox
#define HELPBOX_MAX_HEIGHT 			0.4*DEFAULT_XRES
#define HELPBOX_MAX_WIDTH  			(g_settings.screen_EndX - g_settings.screen_StartX )
#define HELPBOX_WIDTH				650
#define HELPBOX_HEIGHT				450

// 
#define PIC_W 					0.25*DEFAULT_XRES  //320
#define PIC_H					0.355*DEFAULT_YRES //256

// CInfoBox
#define MAX_WINDOW_WIDTH  			(g_settings.screen_EndX - g_settings.screen_StartX )
#define MAX_WINDOW_HEIGHT 			(g_settings.screen_EndY - g_settings.screen_StartY - 40)	

#define MIN_WINDOW_WIDTH  			(MAX_WINDOW_WIDTH>>1)
#define MIN_WINDOW_HEIGHT 			40

// CTextBox
#define THUMBNAIL_OFFSET			1
#define BIG_FONT_FAKTOR 			1.5

// multi widget
#define INTER_FRAME_SPACE 			5 

// widget
#define MENU_WIDTH				0.507*DEFAULT_XRES 	//650
#define MENU_HEIGHT				0.833*DEFAULT_YRES	//600

// parentallock
const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

//
#define widest_number 				"2"
#define ANNOUNCETIME 				(1 * 60)

#endif

