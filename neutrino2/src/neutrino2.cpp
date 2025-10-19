//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: neutrino2.cpp 04092025 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NEUTRINO_CPP

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>

#include <linux/fs.h>

#include <string.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include <global.h>
#include <neutrino2.h>

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/rcinput.h>
#include <driver/record.h>
#include <driver/shutdown_count.h>
#include <driver/streamts.h>
#include <driver/hdmi_cec.h>
#include <driver/movieinfo.h>

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/fontrenderer.h>
#include <driver/gdi/color.h>

#include <gui/epgplus.h>
#include <gui/streaminfo.h>

#include <gui/widget/icons.h>
#include <gui/widget/colorchooser.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/keychooser.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <gui/bouqueteditor_bouquets.h>
#include <gui/bouquetlist.h>
#include <gui/eventlist.h>
#include <gui/channellist.h>
#include <gui/screensetup.h>
#include <gui/pluginlist.h>
#include <gui/plugins.h>
#include <gui/infoviewer.h>
#include <gui/epgview.h>
#include <gui/update.h>
#include <gui/scan.h>
#include <gui/sleeptimer.h>
#include <gui/rc_lock.h>
#include <gui/timerlist.h>
#include <gui/alphasetup.h>
#include <gui/audioplayer.h>
#include <gui/imageinfo.h>
#include <gui/movieplayer.h>
#include <gui/nfs.h>
#include <gui/pictureviewer.h>
#include <gui/motorcontrol.h>
#include <gui/filebrowser.h>
#include <gui/cam_menu.h>
#include <gui/hdd_menu.h>
#include <gui/dboxinfo.h>
#include <gui/audio_select.h>
#include <gui/scan_setup.h>
#include <gui/audio_video_select.h>
#include <gui/mediaplayer.h>
#include <gui/service_menu.h>
#include <gui/main_setup.h>
#include <gui/audio_setup.h>
#include <gui/video_setup.h>
#include <gui/parentallock_setup.h>
#include <gui/network_setup.h>
#include <gui/movieplayer_setup.h>
#include <gui/osd_setup.h>
#include <gui/audioplayer_setup.h>
#include <gui/pictureviewer_setup.h>
#include <gui/lcd_setup.h>
#include <gui/rc_setup.h>
#include <gui/recording_setup.h>
#include <gui/misc_setup.h>
#include <gui/dvbsub_select.h>
#include <gui/epg_menu.h>
#include <gui/cec_setup.h>

#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>
#include <system/helpers.h>
#include <system/channellogo.h>
#include <system/weather.h>

#include <timerd/timerd.h>

//
#include <zapit/frontend_c.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>

//
#include <video_cs.h>
#include <audio_cs.h>

#if defined (ENABLE_CI)
#include <libdvbci/dvb-ci.h>
#endif

#if defined ENABLE_GSTREAMER
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#endif

#include <playback_cs.h>

#if ENABLE_LUA
#include <interfaces/lua/neutrino2_lua.h>
#endif

#include <nhttpd/yhttpd.h>

#ifdef USE_OPENGL
#include <ao/ao.h>
#endif


//// globals
int debug = DEBUG_NORMAL;
//
cVideo * videoDecoder = NULL;
cAudio * audioDecoder = NULL;
// zap
int old_b_id = -1;
// record and timeshift
bool autoshift = false;
uint32_t shift_timer = 0;
uint32_t scrambled_timer = 0;
char recDir[255];
char timeshiftDir[255];
// volume bar
static CCProgressBar * g_volscale;
//
int prev_video_Mode;
int current_volume;
int current_muted;
// bouquets lists
CBouquetList* bouquetList; 				//current bqt list
//
CBouquetList* TVbouquetList;
CBouquetList* TVsatList;
CBouquetList* TVfavList;
CBouquetList* TVallList;
//
CBouquetList* RADIObouquetList;
CBouquetList* RADIOsatList;
CBouquetList* RADIOfavList;
CBouquetList* RADIOallList;
//
#if defined (ENABLE_CI)
CCAMMenuHandler* g_CamHandler;
#endif
//
static char **global_argv;
//user menu
const char *usermenu_button_def[SNeutrinoSettings::BUTTON_MAX] = 
{
	"blue",
#if defined (ENABLE_FUNCTIONKEYS)
	"f1",
	"f2",
	"f3",
	"f4"
#endif
};

////
extern cDemux *videoDemux;				// defined in zapit.cpp
extern cDemux *audioDemux;				// defined in zapit.cpp
//
extern char rec_filename[1024];				// defined in record.cpp
//
extern satellite_map_t satellitePositions;		// defined in zapit.cpp
extern tallchans allchans;				// defined in zapit.cpp
// tuxtxt
extern int  tuxtxt_stop();
extern void tuxtxt_close();
extern void tuxtx_pause_subtitle(bool pause, bool isEplayer);
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern int tuxtx_main(int pid, int page, bool isEplayer);
// dvbsub
extern int dvbsub_init();
extern int dvbsub_close();
extern int dvbsub_stop();
extern int dvbsub_start(int pid, bool isEplayer);
extern int dvbsub_pause();
extern int dvbsub_getpid();
extern void dvbsub_setpid(int pid);

// init globals
static void initGlobals(void)
{
	g_fontRenderer  = NULL;
	g_RCInput       = NULL;
	g_RemoteControl = NULL;
	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;
	g_PluginList    = NULL;
#if defined (ENABLE_CI)	
	g_CamHandler 	= NULL;
#endif	

	g_Radiotext     = NULL;
	g_Locale        = new CLocaleManager;
}

// fonts
#define FONT_STYLE_REGULAR 0
#define FONT_STYLE_BOLD    1
#define FONT_STYLE_ITALIC  2

typedef struct font_sizes
{
        const char* const 	name;
        const unsigned int      defaultsize;
        const unsigned int      style;
        const unsigned int      size_offset;
} font_sizes_struct;

// neutrino_font
font_sizes_struct neutrino_font[FONT_TYPE_COUNT] =
{
        {_("Menu Text")               	,  20, FONT_STYLE_BOLD   , 0},
        {_("Menu Title")         	,  30, FONT_STYLE_BOLD   , 0},
        {_("Menu Info")          	,  16, FONT_STYLE_REGULAR, 0},
        {_("EPG Title")          	,  25, FONT_STYLE_REGULAR, 1},
        {_("EPG Info1")          	,  17, FONT_STYLE_ITALIC , 2},
        {_("EPG Info2")          	,  17, FONT_STYLE_REGULAR, 2},
        {_("EPG Date")           	,  15, FONT_STYLE_REGULAR, 2},
        {_("Eventlist Title")    	,  30, FONT_STYLE_REGULAR, 0},
        {_("Eventlist large")		,  20, FONT_STYLE_BOLD   , 1},
        {_("Eventlist small")		,  14, FONT_STYLE_REGULAR, 1},
        {_("Eventlist date / time") 	,  16, FONT_STYLE_REGULAR, 1},
        {_("Pluginlist large") 		,  20, FONT_STYLE_BOLD   , 1},
        {_("Pluginlist small") 		,  16, FONT_STYLE_REGULAR, 1},
        {_("Channellist")        	,  20, FONT_STYLE_BOLD   , 1},
        {_("Channellist Description")  	,  20, FONT_STYLE_REGULAR, 1},
        {_("Channellist Number") 	,  14, FONT_STYLE_BOLD   , 2},
        {_("Channel direct selection")  ,  40, FONT_STYLE_BOLD   , 0},
        {_("Infobar Number")     	,  30, FONT_STYLE_BOLD   , 0},
        {_("Infobar Channel name")   	,  30, FONT_STYLE_BOLD   , 0},
        {_("Infobar info")       	,  20, FONT_STYLE_REGULAR, 1},
        {_("Infobar small")      	,  14, FONT_STYLE_REGULAR, 1},
        {_("Filebrowser item")   	,  16, FONT_STYLE_BOLD   , 1},
        {_("Menu Title large")         	,  40, FONT_STYLE_REGULAR, 0},
};

////
CNeutrinoApp::CNeutrinoApp()
: configfile('\t')
{
	standby_pressed_at.tv_sec = 0;

	frameBuffer = NULL;

	mode = mode_unknown;
	
	channelList = NULL;
	TVchannelList = NULL;
	RADIOchannelList = NULL;
	
	nextRecordingInfo = NULL;
	skipShutdownTimer = false;
	current_muted = 0;
	
	memset(&font, 0, sizeof(neutrino_font_descr_struct));
	
	//
	recordingstatus = 0;
	timeshiftstatus = 0;

	vol_pixbuf = NULL;
	muteIcon = NULL;
	
	//
	epgUpdateTimer = 0;
}

// CNeutrinoApp - Destructor
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelList)
	{
		delete channelList;
		channelList = NULL;
	}
}

// getInstance
CNeutrinoApp * CNeutrinoApp::getInstance()
{
	static CNeutrinoApp * neutrinoApp = NULL;

	if(!neutrinoApp) 
	{
		neutrinoApp = new CNeutrinoApp();
	}

	return neutrinoApp;
}

// loadSetup
int CNeutrinoApp::loadSetup(const char * fname)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::loadSetup\n");
	
	char cfg_key[81];
	int erg = 0;

	configfile.clear();

	// settings laden - und dabei Defaults setzen!
	if(!configfile.loadConfig(fname)) 
	{
		//file existiert nicht
		erg = 1;
	}

	// video
	g_settings.video_Mode = configfile.getInt32("video_Mode", VIDEO_STD_720P50);
	prev_video_Mode = g_settings.video_Mode;
	
	//analog mode
	g_settings.analog_mode = configfile.getInt32("analog_mode", ANALOG_YUV); 	// default yuv
	
	g_settings.hdmi_color_space = configfile.getInt32("hdmi_color_space", HDMI_RGB); //default RGB	

	//aspect ratio
	g_settings.video_Ratio = configfile.getInt32("video_Ratio", ASPECTRATIO_169);		// 16:9	
	 
	// policy
	g_settings.video_Format = configfile.getInt32("video_Format", VIDEOFORMAT_PANSCAN2);	

	//wss	
	g_settings.wss_mode = configfile.getInt32("wss_mode", WSS_AUTO);	
	
	// psi
	g_settings.contrast = configfile.getInt32( "contrast", 130);
	g_settings.saturation = configfile.getInt32( "saturation", 130);
	g_settings.brightness = configfile.getInt32( "brightness", 130);
	g_settings.tint = configfile.getInt32( "tint", 130);
	// end video

	// audio
	g_settings.audio_AnalogMode = configfile.getInt32( "audio_AnalogMode", 0 );		//default=stereo
	g_settings.audio_DolbyDigital    = configfile.getBool("audio_DolbyDigital", false);
	
	// ac3	
	g_settings.hdmi_dd = configfile.getInt32( "hdmi_dd", AC3_DOWNMIX);	// downmix	
	
	// avsync
	g_settings.avsync = configfile.getInt32( "avsync", AVSYNC_ON);
	
	// ac3 delay
	g_settings.ac3_delay = configfile.getInt32( "ac3_delay", 0);
	
	// pcm delay
	g_settings.pcm_delay = configfile.getInt32( "pcm_delay", 0);
	
	g_settings.auto_lang = configfile.getInt32( "auto_lang", 0 );
	g_settings.auto_subs = configfile.getInt32( "auto_subs", 0 );

	for(int i = 0; i < 3; i++) 
	{
		sprintf(cfg_key, "pref_lang_%d", i);
		strncpy(g_settings.pref_lang[i], configfile.getString(cfg_key, "German").c_str(), 30);
		
		sprintf(cfg_key, "pref_subs_%d", i);
		strncpy(g_settings.pref_subs[i], configfile.getString(cfg_key, "German").c_str(), 30);
	}
	// end audio

	// parentallock
	g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
	g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	strcpy( g_settings.parentallock_pincode, configfile.getString( "parentallock_pincode", "0000" ).c_str() );
	// end parentallock

	// network
	g_settings.network_ntpserver = configfile.getString("network_ntpserver", "time.fu-berlin.de");
        g_settings.network_ntprefresh = configfile.getString("network_ntprefresh", "30" );
        g_settings.network_ntpenable = configfile.getBool("network_ntpenable", true);
	
	snprintf(g_settings.ifname, sizeof(g_settings.ifname), "%s", configfile.getString("ifname", "eth0").c_str());

	// nfs entries
	for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		g_settings.network_nfs_ip[i] = configfile.getString(cfg_key, "");
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		strcpy( g_settings.network_nfs_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		strcpy( g_settings.network_nfs_local_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		g_settings.network_nfs_automount[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		g_settings.network_nfs_type[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key,"network_nfs_username_%d", i);
		strcpy( g_settings.network_nfs_username[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		strcpy( g_settings.network_nfs_password[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		strcpy( g_settings.network_nfs_mount_options1[i], configfile.getString( cfg_key, "ro,soft,udp" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		strcpy( g_settings.network_nfs_mount_options2[i], configfile.getString( cfg_key, "nolock,rsize=8192,wsize=8192" ).c_str() );
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		strcpy( g_settings.network_nfs_mac[i], configfile.getString( cfg_key, "11:22:33:44:55:66").c_str() );
	}
	// end network

	// recording
	strcpy( g_settings.network_nfs_recordingdir, configfile.getString( "network_nfs_recordingdir", "/media/hdd/record" ).c_str() );

	// permanent timeshift
	g_settings.auto_timeshift = configfile.getInt32( "auto_timeshift", 0 );	

	// timeshift dir
	sprintf(timeshiftDir, "%s/.timeshift", g_settings.network_nfs_recordingdir);
	safe_mkdir(timeshiftDir);
	
	dprintf(DEBUG_INFO, "CNeutrinoApp::loadSetup: rec dir %s timeshift dir %s\n", g_settings.network_nfs_recordingdir, timeshiftDir);

	g_settings.record_hours = configfile.getInt32( "record_hours", 4 );
	g_settings.recording_audio_pids_default = configfile.getInt32("recording_audio_pids_default", TIMERD_APIDS_STD | TIMERD_APIDS_AC3);
	g_settings.recording_epg_for_filename = configfile.getBool("recording_epg_for_filename", false);
	g_settings.recording_save_in_channeldir = configfile.getBool("recording_save_in_channeldir", false);
	// end recording

	// movieplayer
	strcpy( g_settings.network_nfs_moviedir, configfile.getString( "network_nfs_moviedir", "/media/hdd/movie" ).c_str() );
	// end movieplayer

	// OSD
	g_settings.gtx_alpha = configfile.getInt32( "gtx_alpha", 255);
	strcpy(g_settings.language, configfile.getString("language", "en").c_str());

	// themes
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 255);
	g_settings.menu_Head_red = configfile.getInt32( "menu_Head_red", 0);
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 11);
	g_settings.menu_Head_blue = configfile.getInt32( "menu_Head_blue", 37);

	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0);
	g_settings.menu_Head_Text_red = configfile.getInt32( "menu_Head_Text_red", 255);
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 255);
	g_settings.menu_Head_Text_blue = configfile.getInt32( "menu_Head_Text_blue", 255);
	
	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 255);
	g_settings.menu_Content_red = configfile.getInt32( "menu_Content_red", 0);
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 11);
	g_settings.menu_Content_blue = configfile.getInt32( "menu_Content_blue", 37);
	
	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 255);
	g_settings.menu_Content_Text_red = configfile.getInt32( "menu_Content_Text_red", 255);
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 255);
	g_settings.menu_Content_Text_blue = configfile.getInt32( "menu_Content_Text_blue", 255);
	
	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 255);
	g_settings.menu_Content_Selected_red = configfile.getInt32( "menu_Content_Selected_red", 20);
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 25);
	g_settings.menu_Content_Selected_blue = configfile.getInt32( "menu_Content_Selected_blue", 65);
	
	g_settings.menu_Content_Selected_Text_alpha = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0);
	g_settings.menu_Content_Selected_Text_red = configfile.getInt32( "menu_Content_Selected_Text_red", 125);
	g_settings.menu_Content_Selected_Text_green = configfile.getInt32( "menu_Content_Selected_Text_green", 125);
	g_settings.menu_Content_Selected_Text_blue = configfile.getInt32( "menu_Content_Selected_Text_blue", 125);
	
	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 205);
	g_settings.menu_Content_inactive_red = configfile.getInt32( "menu_Content_inactive_red", 63);
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 63);
	g_settings.menu_Content_inactive_blue = configfile.getInt32( "menu_Content_inactive_blue", 63);
	
	g_settings.menu_Content_inactive_Text_alpha = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0);
	g_settings.menu_Content_inactive_Text_red = configfile.getInt32( "menu_Content_inactive_Text_red", 125);
	g_settings.menu_Content_inactive_Text_green = configfile.getInt32( "menu_Content_inactive_Text_green", 125);
	g_settings.menu_Content_inactive_Text_blue = configfile.getInt32( "menu_Content_inactive_Text_blue", 125);

	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 255);
	g_settings.infobar_red = configfile.getInt32( "infobar_red", 0);
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 11);
	g_settings.infobar_blue = configfile.getInt32( "infobar_blue", 37);
	
	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0);
	g_settings.infobar_Text_red = configfile.getInt32( "infobar_Text_red", 255);
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 255);
	g_settings.infobar_Text_blue = configfile.getInt32( "infobar_Text_blue", 255);
		
	g_settings.infobar_colored_events_alpha = configfile.getInt32( "infobar_colored_events_alpha", 0);
	g_settings.infobar_colored_events_red = configfile.getInt32( "infobar_colored_events_red", 255);
	g_settings.infobar_colored_events_green = configfile.getInt32( "infobar_colored_events_green", 255);
	g_settings.infobar_colored_events_blue = configfile.getInt32( "infobar_colored_events_blue", 0);
	
	g_settings.menu_Foot_alpha = configfile.getInt32( "menu_Foot_alpha", 255);
	g_settings.menu_Foot_red = configfile.getInt32( "menu_Foot_red", 0);
	g_settings.menu_Foot_green = configfile.getInt32( "menu_Foot_green", 11);
	g_settings.menu_Foot_blue = configfile.getInt32( "menu_Foot_blue", 37);
		
	g_settings.menu_Foot_Text_alpha = configfile.getInt32( "menu_Foot_Text_alpha", 0);
	g_settings.menu_Foot_Text_red = configfile.getInt32( "menu_Foot_Text_red", 255);
	g_settings.menu_Foot_Text_green = configfile.getInt32( "menu_Foot_Text_green", 255);
	g_settings.menu_Foot_Text_blue = configfile.getInt32( "menu_Foot_Text_blue", 255);

	g_settings.menu_Hint_alpha = configfile.getInt32( "menu_Hint_alpha", 205);
	g_settings.menu_Hint_red = configfile.getInt32( "menu_Hint_red", 63);
	g_settings.menu_Hint_green = configfile.getInt32( "menu_Hint_green", 63);
	g_settings.menu_Hint_blue = configfile.getInt32( "menu_Hint_blue", 63);
		
	g_settings.menu_Hint_Text_alpha = configfile.getInt32( "menu_Hint_Text_alpha", 0);
	g_settings.menu_Hint_Text_red = configfile.getInt32( "menu_Hint_Text_red", 215);
	g_settings.menu_Hint_Text_green = configfile.getInt32( "menu_Hint_Text_green", 215);
	g_settings.menu_Hint_Text_blue = configfile.getInt32( "menu_Hint_Text_blue", 215);

	//
	strcpy( g_settings.font_file, configfile.getString( "font_file", DATADIR "/fonts/arial.ttf" ).c_str() );
	g_settings.icons_dir = configfile.getString("icons_dir", DATADIR "/icons/");
	g_settings.buttons_dir = configfile.getString("buttons_dir", DATADIR "/buttons/");
	g_settings.hints_dir = configfile.getString("hints_dir", DATADIR "/hints/");
	g_settings.spinner_dir = configfile.getString("spinner_dir", DATADIR "/spinner/");

	// menue timing
	g_settings.timing_menu = configfile.getInt32("timing_menu", DEFAULT_TIMING_MENU);
	g_settings.timing_channellist = configfile.getInt32("timing_channellist", DEFAULT_TIMING_CHANNELLIST);
	g_settings.timing_epg = configfile.getInt32("timing_epg", DEFAULT_TIMING_EPG);
	g_settings.timing_infobar = configfile.getInt32("timing_infobar", DEFAULT_TIMING_INFOBAR);
	g_settings.timing_filebrowser = configfile.getInt32("timing_filebrowser", DEFAULT_TIMING_FILEBROWSER);
	g_settings.timing_numericzap = configfile.getInt32("timing_numericzap", DEFAULT_TIMING_NUMERICZAP);

	// screen setup
	g_settings.screen_StartX = configfile.getInt32( "screen_StartX", 35 );	
	g_settings.screen_StartY = configfile.getInt32( "screen_StartY", 35 );
	g_settings.screen_EndX = configfile.getInt32( "screen_EndX", frameBuffer->getScreenWidth(true) - 35 );	
	g_settings.screen_EndY = configfile.getInt32( "screen_EndY", frameBuffer->getScreenHeight(true) - 35 );
	
	//
	g_settings.screen_width = configfile.getInt32("screen_width", frameBuffer->getScreenWidth(true) );
	g_settings.screen_height = configfile.getInt32("screen_height", frameBuffer->getScreenHeight(true) );
	
	//
	g_settings.screen_xres = configfile.getInt32("screen_xres", 100);
	g_settings.screen_yres = configfile.getInt32("screen_yres", 75);
	
	//
	g_settings.preferred_skin = configfile.getString("preferred_skin", "standard");
	g_settings.item_info = configfile.getBool("item_info", false);
	g_settings.theme = configfile.getString("theme", "default.config");

	// keysbinding
	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", "250").c_str());
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", "25").c_str());

	g_settings.key_tvradio_mode = configfile.getInt32( "key_tvradio_mode", CRCInput::RC_mode );
	
	g_settings.key_channelList_pageup = configfile.getInt32( "key_channelList_pageup", CRCInput::RC_page_up );
	g_settings.key_channelList_pagedown = configfile.getInt32( "key_channelList_pagedown", CRCInput::RC_page_down );
	g_settings.key_channelList_cancel = configfile.getInt32( "key_channelList_cancel", CRCInput::RC_home );
	
	g_settings.key_list_start = configfile.getInt32( "key_list_start", CRCInput::RC_nokey );
	g_settings.key_list_end = configfile.getInt32( "key_list_end", CRCInput::RC_nokey );
	
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up", CRCInput::RC_right);
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down", CRCInput::RC_left);

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up", CRCInput::RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down", CRCInput::RC_down );
	
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up", CRCInput::RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down", CRCInput::RC_left );
	g_settings.key_zaphistory = configfile.getInt32( "key_zaphistory", CRCInput::RC_home );	
	g_settings.key_lastchannel = configfile.getInt32( "key_lastchannel", CRCInput::RC_recall );
	
	// pip keys
	g_settings.key_pip = configfile.getInt32("key_pip", CRCInput::RC_pip); // current TP

	// media keys
	g_settings.key_movieplayer = configfile.getInt32( "key_movieplayer", CRCInput::RC_pvr );
	g_settings.key_audioplayer = configfile.getInt32( "key_audioplayer", CRCInput::RC_music );
	g_settings.key_pictureviewer = configfile.getInt32( "key_pictureviewer", CRCInput::RC_picture );
	g_settings.key_inetradio = configfile.getInt32( "key_inetradio", CRCInput::RC_net );
	g_settings.key_moviebrowser = configfile.getInt32( "key_moviebrowser", CRCInput::RC_media );
	g_settings.key_pvr = configfile.getInt32( "key_pvr", CRCInput::RC_pvr );
	g_settings.key_timerlist = configfile.getInt32( "key_timerlist", CRCInput::RC_timer );
	g_settings.key_screenshot = configfile.getInt32( "key_screenshot", CRCInput::RC_record );
	
        // USERMENU
        const char * usermenu_default[SNeutrinoSettings::BUTTON_MAX] = 
        {
                "1, 2, 3, 8",   		// BLUE
#if defined (ENABLE_FUNCTIONKEYS)
		"0",				// F1
		"0",				// F2
		"0",				// F3
		"0",				// F4
#endif
        };
        char txt1[81];
        std::string txt2;
        const char* txt2ptr;

        for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
        {
                snprintf(txt1, 80, "usermenu_tv_%s", usermenu_button_def[button]);
                txt2 = configfile.getString(txt1, usermenu_default[button]);
                txt2ptr = txt2.c_str();

                for( int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
                {
                        // find next comma or end of string - if it's not the first round
                        if(pos != 0)
                        {
                                while(*txt2ptr != 0 && *txt2ptr != ',')
                                        txt2ptr++;
                                if(*txt2ptr != 0)
                                        txt2ptr++;
                        }

                        if(*txt2ptr != 0)
                        {
                                g_settings.usermenu[button][pos] = atoi(txt2ptr);  // there is still a string
                                if(g_settings.usermenu[button][pos] >= SNeutrinoSettings::ITEM_MAX)
                                        g_settings.usermenu[button][pos] = 0;
                        }
                        else
                                g_settings.usermenu[button][pos] = 0;     // string empty, fill up with 0

                }
        }
	// end keysbinding

	// audioplayer
	strcpy( g_settings.network_nfs_audioplayerdir, configfile.getString( "network_nfs_audioplayerdir", "/media/hdd/music" ).c_str() );

	g_settings.audioplayer_highprio  = configfile.getInt32("audioplayer_highprio", 0);
	// end audioplayer

	// pictureviewer
	strcpy( g_settings.network_nfs_picturedir, configfile.getString( "network_nfs_picturedir", "/media/hdd/picture" ).c_str() );

	g_settings.picviewer_slide_time = configfile.getInt32("picviewer_slide_time", 10);	// in sec
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", (int)SCALE_SIMPLE);
	// end pictureviewer

	// misc opts
	g_settings.channel_mode = configfile.getInt32("channel_mode", CChannelList::LIST_MODE_ALL);

	//misc
	g_settings.power_standby = configfile.getInt32( "power_standby", 0);

	g_settings.shutdown_real = configfile.getBool("shutdown_real", false);
	g_settings.shutdown_real_rcdelay = configfile.getBool("shutdown_real_rcdelay", false);
        strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count", "0").c_str());

	g_settings.infobar_sat_display   = configfile.getBool("infobar_sat_display"  , true );
	g_settings.infobar_subchan_disp_pos = configfile.getInt32("infobar_subchan_disp_pos"  , 0 );

	g_settings.rotor_swap = configfile.getInt32( "rotor_swap", 0);
	g_settings.zap_cycle = configfile.getInt32( "zap_cycle", 1 );

	//timezone
	strcpy(g_settings.timezone, configfile.getString("timezone", "(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Vienna").c_str());
	
	// debug
	g_settings.debug_level = configfile.getInt32("debug_level", DEBUG_NORMAL);
	
	//zapit setup
	g_settings.lastChannelMode = configfile.getInt32("lastChannelMode", 1);		//TV mode
	g_settings.StartChannelTV = configfile.getString("startchanneltv", "");
	g_settings.StartChannelRadio = configfile.getString("startchannelradio", "");
	g_settings.startchanneltv_id =  configfile.getInt64("startchanneltv_id", 0); 
	g_settings.startchannelradio_id = configfile.getInt64("startchannelradio_id", 0);
	g_settings.startchanneltv_nr =  configfile.getInt32("startchanneltv_nr", 0);
	g_settings.startchannelradio_nr = configfile.getInt32("startchannelradio_nr", 0);
	g_settings.uselastchannel = configfile.getInt32("uselastchannel" , 0);

	// epg
        g_settings.epg_cache            = configfile.getString("epg_cache_time", "14");
        g_settings.epg_extendedcache    = configfile.getString("epg_extendedcache_time", "360");
        g_settings.epg_old_events       = configfile.getString("epg_old_events", "1");
        g_settings.epg_max_events       = configfile.getString("epg_max_events", "50000");
        g_settings.epg_dir              = configfile.getString("epg_dir", "/media/hdd/epg");
	g_settings.epg_save 		= configfile.getBool("epg_save", false);
	
	for(int i = 0; i < 3; i++) 
	{
		sprintf(cfg_key, "pref_epgs_%d", i);
		strncpy(g_settings.pref_epgs[i], configfile.getString(cfg_key, "German").c_str(), 30);
	}
	
	// xml
	g_settings.epg_xmltv = configfile.getBool("epg_xmltv", true);
	//
	
	// channellist 
	g_settings.virtual_zap_mode = configfile.getBool("virtual_zap_mode", false);
	g_settings.make_hd_list = configfile.getInt32("make_hd_list", 0);
	g_settings.channellist_ca = configfile.getInt32("channellist_ca", 1);
	g_settings.channellist_timescale = configfile.getInt32("channellist_timescale", 1);
	g_settings.channellist_alt = configfile.getInt32("channellist_alt", 0);
	g_settings.channellist_number = configfile.getInt32("channellist_number", CChannelList::CHANNEL_NUMBER_LIST_ORDER);
	
	// record screenshot
	g_settings.recording_screenshot = configfile.getInt32("recording_screenshot", 1);
	//

	// filebrowser
	g_settings.filesystem_is_utf8 = configfile.getBool("filesystem_is_utf8", true );
	g_settings.filebrowser_showrights = configfile.getInt32("filebrowser_showrights", 0);
	g_settings.filebrowser_sortmethod = configfile.getInt32("filebrowser_sortmethod", 0);
	if ((g_settings.filebrowser_sortmethod < 0) || (g_settings.filebrowser_sortmethod >= FILEBROWSER_NUMBER_OF_SORT_VARIANTS))
		g_settings.filebrowser_sortmethod = 0;
	g_settings.filebrowser_denydirectoryleave = configfile.getBool("filebrowser_denydirectoryleave", false);
	
	// radiotext	
	g_settings.radiotext_enable = configfile.getBool("radiotext_enable", false);
	
	// logos_dir
	g_settings.logos_dir = configfile.getString("logos_dir", "/media/hdd/logos");
	g_settings.logos_show_logo = configfile.getBool("logos_show_logo", false);
	
	// vol
	g_settings.volume_pos = configfile.getInt32( "volume_pos", 1);		//top_left
	g_settings.current_volume = configfile.getInt32("current_volume", 25);
	strcpy( g_settings.audio_step, configfile.getString( "audio_step" , "5" ).c_str() );

	// progressbar color
	g_settings.progressbar_color = configfile.getInt32("progressbar_color", CCProgressBar::PROGRESSBAR_COLORED);

	// head
	g_settings.Head_corner = configfile.getInt32("Head_corner", CORNER_TOP);
	g_settings.Head_radius = configfile.getInt32("Head_radius", NO_RADIUS);
	g_settings.Head_gradient = configfile.getInt32("Head_gradient", NOGRADIENT);
	g_settings.Head_gradient_type = configfile.getInt32("Head_gradient_type", GRADIENT_COLOR2TRANSPARENT);
	g_settings.Head_line = configfile.getBool("Head_line", true);
	g_settings.Head_line_gradient = configfile.getBool("Head_line_gradient", false);
	
	// foot
	g_settings.Foot_corner = configfile.getInt32("Foot_corner", CORNER_BOTTOM);
	g_settings.Foot_radius = configfile.getInt32("Foot_radius", NO_RADIUS);
	g_settings.Foot_gradient = configfile.getInt32("Foot_gradient", NOGRADIENT);
	g_settings.Foot_gradient_type = configfile.getInt32("Foot_gradient_type", GRADIENT_COLOR2TRANSPARENT);
	g_settings.Foot_line = configfile.getBool("Foot_line", true);
	g_settings.Foot_line_gradient = configfile.getBool("Foot_line_gradient", false);
	
	// infobar
	g_settings.infobar_corner = configfile.getInt32("infobar_corner", CORNER_ALL);
	g_settings.infobar_radius = configfile.getInt32("infobar_radius", NO_RADIUS);
	g_settings.infobar_gradient = configfile.getInt32("infobar_gradient", NOGRADIENT);
	g_settings.infobar_gradient_type = configfile.getInt32("infobar_gradient_type", GRADIENT_COLOR2TRANSPARENT);
	g_settings.infobar_buttonbar = configfile.getBool("infobar_buttonbar", false);
	g_settings.infobar_buttonline = configfile.getBool("infobar_buttonline", true);
	g_settings.infobar_buttonline_gradient = configfile.getBool("infobar_buttonline_gradient", false);
	g_settings.infobar_border = configfile.getBool("infobar_border", false);
	
	// hint
	g_settings.Hint_gradient = configfile.getInt32("Hint_gradient", NOGRADIENT);
	g_settings.Hint_gradient_type = configfile.getInt32("Hint_gradient_type", GRADIENT_COLOR2TRANSPARENT);
	g_settings.Hint_border = configfile.getBool("Hint_border", true);
	g_settings.Hint_radius = configfile.getInt32("Hint_radius", NO_RADIUS);
	g_settings.Hint_corner = configfile.getInt32("Hint_corner", CORNER_ALL);
	
	// sep
	g_settings.sep_gradient = configfile.getBool("separator_gradient", false);
	// END MISC OPTS

	// HDD
	g_settings.hdd_sleep = configfile.getInt32( "hdd_sleep", 120);
	g_settings.hdd_noise = configfile.getInt32( "hdd_noise", 254);
	// END HDD

	// software update
	strcpy(g_settings.softupdate_proxyserver, configfile.getString("softupdate_proxyserver", "" ).c_str());
	strcpy(g_settings.softupdate_proxyusername, configfile.getString("softupdate_proxyusername", "" ).c_str());
	strcpy(g_settings.softupdate_proxypassword, configfile.getString("softupdate_proxypassword", "" ).c_str());
	strcpy( g_settings.update_dir, configfile.getString( "update_dir", "/tmp" ).c_str() );
	strcpy(g_settings.softupdate_url_file, configfile.getString("softupdate_url_file", "/var/etc/update.urls").c_str());
	// end software update

	// VFD
	g_settings.lcd_brightness = configfile.getInt32("lcd_brightness", DEFAULT_LCD_BRIGHTNESS);
	g_settings.lcd_standbybrightness = configfile.getInt32("lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS);
	g_settings.lcd_contrast = configfile.getInt32("lcd_contrast", DEFAULT_LCD_CONTRAST);
	g_settings.lcd_power = configfile.getInt32("lcd_power", DEFAULT_LCD_POWER);
	g_settings.lcd_inverse = configfile.getInt32("lcd_inverse", DEFAULT_LCD_INVERSE);
	g_settings.lcd_statusline = configfile.getInt32("lcd_statusline", DEFAULT_LCD_STATUSLINE);
	g_settings.lcd_epgmode = configfile.getInt32("lcd_epgmode", DEFAULT_LCD_EPGMODE);
	g_settings.lcd_mode = configfile.getInt32("lcd_mode", DEFAULT_LCD_MODE);
	g_settings.lcd_epgalign = configfile.getInt32("lcd_epgalign", DEFAULT_LCD_EPGALIGN);
	strcpy(g_settings.lcd_setting_dim_time, configfile.getString("lcd_dim_time", "0").c_str());
	g_settings.lcd_setting_dim_brightness = configfile.getInt32("lcd_dim_brightness", DEFAULT_LCD_DIM_BRIGHTNESS);
	g_settings.lcd_led = configfile.getInt32("lcd_led", CLCD::LEDCOLOR_BLUE);
	g_settings.lcd_minitv = configfile.getInt32("lcd_minitv", CLCD::MINITV_NORMAL);
	g_settings.lcd_minitvfps = configfile.getInt32("lcd_minitvfps", 30);
	g_settings.lcd_picon = configfile.getInt32("lcd_picon", 0);
	g_settings.lcd_weather = configfile.getInt32("lcd_weather", 0);
	g_settings.lcd_standby_clock = configfile.getInt32("lcd_standby_clock", CLCD::STANDBYCLOCK_DIGITAL);
	// end VFD

	// online epg
	g_settings.epg_enable_localtv_epg = configfile.getBool("epg_enable_localtv_epg", false);
	g_settings.epg_serverbox_ip = configfile.getString("epg_serverbox_ip", "192.168.0.12");
	g_settings.epg_serverbox_type = configfile.getInt32("epg_serverbox_type", CFrontend::DVB_C);
	g_settings.epg_serverbox_gui = configfile.getInt32("epg_serverbox_gui", SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2);

	//
	g_settings.ytkey = configfile.getString("ytkey", "");
	g_settings.weather_api_key = configfile.getString("weather_api_key", "");
	g_settings.show_weather = configfile.getInt32("show_weather", 0);

	// tmdb
	g_settings.tmdbkey = configfile.getString("tmdbkey", "af3a53eb387d57fc935e9128468b1899");
	g_settings.enable_tmdb_infos = configfile.getBool("enable_tmdb_infos", false);
	g_settings.enable_tmdb_preview = configfile.getBool("enable_tmdb_preview", false);
	
#if defined (ENABLE_CI)
	// cicam
	g_settings.ci_standby_reset = configfile.getInt32("ci_standby_reset", 0);
	g_settings.ci_check_live = configfile.getInt32("ci_check_live", 0);
	g_settings.ci_tuner = configfile.getInt32("ci_tuner", -1);
	g_settings.ci_delay = configfile.getInt32("ci_delay", 128);
#endif

	// cec
	g_settings.hdmi_cec_mode = configfile.getInt32("hdmi_cec_mode", 0); 		// default off
	g_settings.hdmi_cec_standby = configfile.getInt32("hdmi_cec_standby", 0); 	// default off
	g_settings.hdmi_cec_volume = configfile.getInt32("hdmi_cec_volume", 0);
	
	// personalize
	g_settings.personalize_tvradio = configfile.getInt32("personalize_tvradio", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_epgtimer = configfile.getInt32("personalize_epgtimer", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_scart = configfile.getInt32("personalize_scart", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_features = configfile.getInt32("personalize_features", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_service = configfile.getInt32("personalize_service", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_information = configfile.getInt32("personalize_information", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_powermenu = configfile.getInt32("personalize_powermenu", CMenuItem::ITEM_ACTIVE);
	g_settings.personalize_mediaplayer = configfile.getInt32("personalize_mediaplayer", CMenuItem::ITEM_ACTIVE);
	
	//
#ifdef ENABLE_GRAPHLCD
	g_settings.glcd_enable = configfile.getInt32("glcd_enable", 0);
	g_settings.glcd_selected_config = configfile.getInt32("glcd_selected_config", 0);
	g_settings.glcd_brightness = configfile.getInt32("glcd_brightness", 255);
	g_settings.glcd_brightness_standby = configfile.getInt32("glcd_brightness_standby", 255);
#endif
	
	//set OSD resolution
#define DEFAULT_X_OFF 35
#define DEFAULT_Y_OFF 35

	if((g_settings.screen_width != (int) frameBuffer->getScreenWidth(true)) || (g_settings.screen_height != (int) frameBuffer->getScreenHeight(true))) 
	{
		g_settings.screen_StartX = DEFAULT_X_OFF;
		g_settings.screen_StartY = DEFAULT_Y_OFF;
		g_settings.screen_EndX = frameBuffer->getScreenWidth(true) - DEFAULT_X_OFF;
		g_settings.screen_EndY = frameBuffer->getScreenHeight(true) - DEFAULT_Y_OFF;
		
		g_settings.screen_width = frameBuffer->getScreenWidth(true);
		g_settings.screen_height = frameBuffer->getScreenHeight(true);
	}	

	if(configfile.getUnknownKeyQueryedFlag() && (erg==0)) 
	{
		erg = 2;
	}

	if(erg)
		configfile.setModifiedFlag(true);

	return erg;
}

// saveSetup, save the application-settings
void CNeutrinoApp::saveSetup(const char * fname)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveSetup:%s\n", fname);
	
	char cfg_key[81];

	// VIDEO
	configfile.setInt32( "video_Mode", g_settings.video_Mode );
	configfile.setInt32( "analog_mode", g_settings.analog_mode );
	
	// hdmi space colour
	configfile.setInt32( "hdmi_color_space", g_settings.hdmi_color_space );
	
	//aspect ratio
	configfile.setInt32( "video_Ratio", g_settings.video_Ratio );
	
	//display format
	configfile.setInt32( "video_Format", g_settings.video_Format );

	// wss
	configfile.setInt32("wss_mode", g_settings.wss_mode);
	
	configfile.setInt32( "contrast", g_settings.contrast);
	configfile.setInt32( "saturation", g_settings.saturation);
	configfile.setInt32( "brightness", g_settings.brightness);
	configfile.setInt32( "tint", g_settings.tint);
	// END VIDEO

	// AUDIO
	configfile.setInt32( "audio_AnalogMode", g_settings.audio_AnalogMode );
	configfile.setBool("audio_DolbyDigital", g_settings.audio_DolbyDigital);
	configfile.setInt32( "hdmi_dd", g_settings.hdmi_dd);
	
	configfile.setInt32( "avsync", g_settings.avsync);
	
	// ac3 delay
	configfile.setInt32( "ac3_delay", g_settings.ac3_delay);
	
	// pcm delay
	configfile.setInt32( "pcm_delay", g_settings.pcm_delay);
	
	// auto langs/subs/ac3
	configfile.setInt32( "auto_lang", g_settings.auto_lang );
	configfile.setInt32( "auto_subs", g_settings.auto_subs );
	for(int i = 0; i < 3; i++) {
		sprintf(cfg_key, "pref_lang_%d", i);
		configfile.setString(cfg_key, g_settings.pref_lang[i]);
		sprintf(cfg_key, "pref_subs_%d", i);
		configfile.setString(cfg_key, g_settings.pref_subs[i]);
	}
	// END AUDIO

	// PARENTALLOCK
	configfile.setInt32( "parentallock_prompt", g_settings.parentallock_prompt );
	configfile.setInt32( "parentallock_lockage", g_settings.parentallock_lockage );
	configfile.setString( "parentallock_pincode", g_settings.parentallock_pincode );
	// END PARENTALLOCK

	// NETWORK
        configfile.setString( "network_ntpserver", g_settings.network_ntpserver);
        configfile.setString( "network_ntprefresh", g_settings.network_ntprefresh);
        configfile.setBool( "network_ntpenable", g_settings.network_ntpenable);
	
	configfile.setString("ifname", g_settings.ifname);

	for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_ip[i] );
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_dir[i] );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_local_dir[i] );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_automount[i]);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_type[i]);
		sprintf(cfg_key,"network_nfs_username_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_username[i] );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_password[i] );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options1[i]);
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options2[i]);
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mac[i]);
	}
	// END NETWORK

	// RECORDING
	configfile.setString( "network_nfs_recordingdir", g_settings.network_nfs_recordingdir);
	configfile.setInt32( "auto_timeshift", g_settings.auto_timeshift );
	configfile.setInt32( "record_hours", g_settings.record_hours );
	configfile.setInt32 ("recording_audio_pids_default"       , g_settings.recording_audio_pids_default);
	configfile.setBool  ("recording_epg_for_filename"         , g_settings.recording_epg_for_filename);
	configfile.setBool  ("recording_save_in_channeldir"       , g_settings.recording_save_in_channeldir);
	// END RECORDING

	// MOVIEPLAYER
	configfile.setString( "network_nfs_moviedir", g_settings.network_nfs_moviedir);
	// END MOVIEPLAYER

	// OSD
	configfile.setInt32( "gtx_alpha", g_settings.gtx_alpha);

	configfile.setString("language", g_settings.language);

	// theme
	configfile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	configfile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	configfile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	configfile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );

	configfile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	configfile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	configfile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	configfile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	configfile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	configfile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	configfile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	configfile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );

	configfile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	configfile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	configfile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	configfile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	configfile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	configfile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	configfile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	configfile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );

	configfile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	configfile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	configfile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	configfile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	configfile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	configfile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	configfile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	configfile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );

	configfile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	configfile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	configfile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	configfile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	configfile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	configfile.setInt32( "infobar_red", g_settings.infobar_red );
	configfile.setInt32( "infobar_green", g_settings.infobar_green );
	configfile.setInt32( "infobar_blue", g_settings.infobar_blue );

	configfile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	configfile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	configfile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	configfile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );
	
	configfile.setInt32( "infobar_colored_events_alpha", g_settings.infobar_colored_events_alpha );
	configfile.setInt32( "infobar_colored_events_red", g_settings.infobar_colored_events_red );
	configfile.setInt32( "infobar_colored_events_green", g_settings.infobar_colored_events_green );
	configfile.setInt32( "infobar_colored_events_blue", g_settings.infobar_colored_events_blue );
	
	configfile.setInt32( "menu_Foot_alpha", g_settings.menu_Foot_alpha );
	configfile.setInt32( "menu_Foot_red", g_settings.menu_Foot_red );
	configfile.setInt32( "menu_Foot_green", g_settings.menu_Foot_green );
	configfile.setInt32( "menu_Foot_blue", g_settings.menu_Foot_blue );
	
	configfile.setInt32( "menu_Foot_Text_alpha", g_settings.menu_Foot_Text_alpha );
	configfile.setInt32( "menu_Foot_Text_red", g_settings.menu_Foot_Text_red );
	configfile.setInt32( "menu_Foot_Text_green", g_settings.menu_Foot_Text_green );
	configfile.setInt32( "menu_Foot_Text_blue", g_settings.menu_Foot_Text_blue );

	configfile.setInt32( "menu_Hint_alpha", g_settings.menu_Hint_alpha );
	configfile.setInt32( "menu_Hint_red", g_settings.menu_Hint_red );
	configfile.setInt32( "menu_Hint_green", g_settings.menu_Hint_green );
	configfile.setInt32( "menu_Hint_blue", g_settings.menu_Hint_blue );
	
	configfile.setInt32( "menu_Hint_Text_alpha", g_settings.menu_Hint_Text_alpha );
	configfile.setInt32( "menu_Hint_Text_red", g_settings.menu_Hint_Text_red );
	configfile.setInt32( "menu_Hint_Text_green", g_settings.menu_Hint_Text_green );
	configfile.setInt32( "menu_Hint_Text_blue", g_settings.menu_Hint_Text_blue );

	configfile.setInt32( "screen_StartX", g_settings.screen_StartX );
	configfile.setInt32( "screen_StartY", g_settings.screen_StartY );
	configfile.setInt32( "screen_EndX", g_settings.screen_EndX );
	configfile.setInt32( "screen_EndY", g_settings.screen_EndY );
	configfile.setInt32( "screen_width", g_settings.screen_width);
	configfile.setInt32( "screen_height", g_settings.screen_height);
	configfile.setInt32( "screen_xres", g_settings.screen_xres);
	configfile.setInt32( "screen_yres", g_settings.screen_yres);

	//
	configfile.setString("font_file", g_settings.font_file);
	configfile.setString("icons_dir", g_settings.icons_dir);
	configfile.setString("buttons_dir", g_settings.buttons_dir);
	configfile.setString("hints_dir", g_settings.hints_dir);
	configfile.setString("spinner_dir", g_settings.spinner_dir);

	// menue timing
	configfile.setInt32("timing_menu", g_settings.timing_menu);
	configfile.setInt32("timing_channellist", g_settings.timing_channellist);
	configfile.setInt32("timing_epg", g_settings.timing_epg);
	configfile.setInt32("timing_infobar", g_settings.timing_infobar);
	configfile.setInt32("timing_filebrowser", g_settings.timing_filebrowser);
	configfile.setInt32("timing_numericzap", g_settings.timing_numericzap);
	
	//
	configfile.setString("preferred_skin", g_settings.preferred_skin);
	configfile.setBool("item_info", g_settings.item_info);
	configfile.setString("theme", g_settings.theme);
	// END OSD

	// KEYS
	configfile.setString( "repeat_blocker", g_settings.repeat_blocker );
	configfile.setString( "repeat_genericblocker", g_settings.repeat_genericblocker );

	configfile.setInt32( "key_tvradio_mode", g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );

	configfile.setInt32( "key_quickzap_up", g_settings.key_quickzap_up );
	configfile.setInt32( "key_quickzap_down", g_settings.key_quickzap_down );
	
	configfile.setInt32( "key_bouquet_up", g_settings.key_bouquet_up );
	configfile.setInt32( "key_bouquet_down", g_settings.key_bouquet_down );
	
	configfile.setInt32( "key_subchannel_up", g_settings.key_subchannel_up );
	configfile.setInt32( "key_subchannel_down", g_settings.key_subchannel_down );
	configfile.setInt32( "key_zaphistory", g_settings.key_zaphistory );
	configfile.setInt32( "key_lastchannel", g_settings.key_lastchannel );

	configfile.setInt32( "key_list_start", g_settings.key_list_start );
	configfile.setInt32( "key_list_end", g_settings.key_list_end );
	configfile.setInt32( "key_pip", g_settings.key_pip ); // current TP
	
	// media keys
	configfile.setInt32( "key_movieplayer", g_settings.key_movieplayer );
	configfile.setInt32( "key_audioplayer", g_settings.key_audioplayer );
	configfile.setInt32( "key_pictureviewer", g_settings.key_pictureviewer );
	configfile.setInt32( "key_inetradio", g_settings.key_inetradio );
	configfile.setInt32( "key_moviebrowser", g_settings.key_moviebrowser );
	configfile.setInt32( "key_pvr", g_settings.key_pvr);
	configfile.setInt32( "key_timerlist", g_settings.key_timerlist );
	configfile.setInt32( "key_screenshot", g_settings.key_screenshot );
	
        // USERMENU
        char txt1[81];
        char txt2[81];
        for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++) 
	{
                char* txt2ptr = txt1;
                snprintf(txt1, 80, "usermenu_tv_%s", usermenu_button_def[button]);

                for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++) 
		{
                        if( g_settings.usermenu[button][pos] != 0) 
			{
                                if(pos != 0)
                                        *txt2ptr++ = ',';
                                txt2ptr += snprintf(txt2ptr, 80, "%d", g_settings.usermenu[button][pos]);
                        }
                }
                configfile.setString(txt1, txt2);
        }
	// END KEYS

	// AUDIOPLAYER
	configfile.setString( "network_nfs_audioplayerdir", g_settings.network_nfs_audioplayerdir);
	configfile.setInt32( "audioplayer_highprio", g_settings.audioplayer_highprio );

	// PICVIEWER
	configfile.setString("network_nfs_picturedir", g_settings.network_nfs_picturedir);
	configfile.setInt32("picviewer_slide_time", g_settings.picviewer_slide_time);
	configfile.setInt32("picviewer_scaling", g_settings.picviewer_scaling);

	// MISC OPTS
	configfile.setInt32("power_standby", g_settings.power_standby);
	configfile.setBool("shutdown_real", g_settings.shutdown_real);
	configfile.setBool("shutdown_real_rcdelay", g_settings.shutdown_real_rcdelay);
	configfile.setString("shutdown_count", g_settings.shutdown_count);

	configfile.setBool("infobar_sat_display", g_settings.infobar_sat_display  );
	configfile.setInt32("infobar_subchan_disp_pos", g_settings.infobar_subchan_disp_pos  );
	
	// channellist
	configfile.setInt32("channellist_ca", g_settings.channellist_ca);
	configfile.setInt32("make_hd_list", g_settings.make_hd_list);
	configfile.setInt32("channellist_timescale", g_settings.channellist_timescale);
	configfile.setInt32("channellist_alt", g_settings.channellist_alt);
	configfile.setInt32("channellist_number", g_settings.channellist_number);

	//
	configfile.setString("timezone", g_settings.timezone);
	
	// debug
	configfile.setInt32( "debug_level", g_settings.debug_level );

	// epg
	configfile.setBool("epg_save", g_settings.epg_save);
        configfile.setString("epg_cache_time", g_settings.epg_cache );
        configfile.setString("epg_extendedcache_time", g_settings.epg_extendedcache);
        configfile.setString("epg_old_events", g_settings.epg_old_events );
        configfile.setString("epg_max_events", g_settings.epg_max_events );
        configfile.setString("epg_dir", g_settings.epg_dir);
	
	for(int i = 0; i < 3; i++) 
	{
		sprintf(cfg_key, "pref_epgs_%d", i);
		configfile.setString(cfg_key, g_settings.pref_epgs[i]);
	}
	
	//
	configfile.setBool("epg_xmltv", g_settings.epg_xmltv);

	//filebrowser
	configfile.setBool  ("filesystem_is_utf8", g_settings.filesystem_is_utf8);
	configfile.setInt32("filebrowser_showrights", g_settings.filebrowser_showrights);
	configfile.setInt32("filebrowser_sortmethod", g_settings.filebrowser_sortmethod);
	configfile.setBool("filebrowser_denydirectoryleave", g_settings.filebrowser_denydirectoryleave);

	//
	configfile.setInt32( "channel_mode", g_settings.channel_mode );

	// scan/channellist
	configfile.setInt32( "rotor_swap", g_settings.rotor_swap);
	configfile.setInt32( "zap_cycle", g_settings.zap_cycle );
	configfile.setBool("virtual_zap_mode", g_settings.virtual_zap_mode);
	
	//zapit setup
	configfile.setInt32("lastChannelMode", g_settings.lastChannelMode);
	configfile.setString("startchanneltv", g_settings.StartChannelTV );
	configfile.setString("startchannelradio", g_settings.StartChannelRadio );
	configfile.setInt64("startchanneltv_id", g_settings.startchanneltv_id);
	configfile.setInt64("startchannelradio_id", g_settings.startchannelradio_id);
	configfile.setInt32("startchanneltv_nr", g_settings.startchanneltv_nr);
	configfile.setInt32("startchannelradio_nr", g_settings.startchannelradio_nr);
	configfile.setInt32("uselastchannel", g_settings.uselastchannel);
	
	// radiotext	
	configfile.setBool("radiotext_enable", g_settings.radiotext_enable);	
	
	// logos_dir
	configfile.setString("logos_dir", g_settings.logos_dir);
	configfile.setBool("logos_show_logo", g_settings.logos_show_logo);
	
	// record screenshot
	configfile.setInt32("recording_screenshot", g_settings.recording_screenshot);
	
	// vol
	configfile.setInt32( "volume_pos", g_settings.volume_pos);
	configfile.setInt32( "current_volume", g_settings.current_volume );
	configfile.setString( "audio_step"	, g_settings.audio_step);

	configfile.setInt32("progressbar_color", g_settings.progressbar_color);

	// head
	configfile.setInt32("Head_corner", g_settings.Head_corner);
	configfile.setInt32("Head_radius", g_settings.Head_radius);
	configfile.setInt32("Head_gradient", g_settings.Head_gradient);
	configfile.setInt32("Head_gradient_type", g_settings.Head_gradient_type);
	configfile.setBool("Head_line", g_settings.Head_line);
	configfile.setBool("Head_line_gradient", g_settings.Head_line_gradient);
	
	// foot
	configfile.setInt32("Foot_corner", g_settings.Foot_corner);
	configfile.setInt32("Foot_radius", g_settings.Foot_radius);
	configfile.setInt32("Foot_gradient", g_settings.Foot_gradient);
	configfile.setInt32("Foot_gradient_type", g_settings.Foot_gradient_type);
	configfile.setBool("Foot_line", g_settings.Foot_line);
	configfile.setBool("Foot_line_gradient", g_settings.Foot_line_gradient);
	
	// hint
	configfile.setInt32("Hint_gradient", g_settings.Hint_gradient);
	configfile.setInt32("Hint_gradient_type", g_settings.Hint_gradient_type);
	configfile.setBool("Hint_border", g_settings.Hint_border);
	configfile.setInt32("Hint_radius", g_settings.Hint_radius);
	configfile.setInt32("Hint_corner", g_settings.Hint_corner);
	
	// infobar
	configfile.setInt32("infobar_gradient", g_settings.infobar_gradient);
	configfile.setInt32("infobar_gradient_type", g_settings.infobar_gradient_type);
	configfile.setInt32("infobar_corner", g_settings.infobar_corner);
	configfile.setInt32("infobar_radius", g_settings.infobar_radius);
	configfile.setBool("infobar_buttonbar", g_settings.infobar_buttonbar);
	configfile.setBool("infobar_buttonline", g_settings.infobar_buttonline);
	configfile.setBool("infobar_buttonline_gradient", g_settings.infobar_buttonline_gradient);
	configfile.setBool("infobar_border", g_settings.infobar_border);
	
	// sep
	configfile.setBool("separator_gradient", g_settings.sep_gradient);

	//
	configfile.setString("ytkey", g_settings.ytkey);
	configfile.setString("weather_api_key", g_settings.weather_api_key);
	configfile.setInt32("show_weather", g_settings.show_weather);
	
	// tmdb
	configfile.setString("tmdbkey", g_settings.tmdbkey);
	configfile.setBool("enable_tmdb_infos", g_settings.enable_tmdb_infos);
	configfile.setBool("enable_tmdb_preview", g_settings.enable_tmdb_preview);
	// END MISC OPTS

	// HDD
	configfile.setInt32( "hdd_sleep", g_settings.hdd_sleep);
	configfile.setInt32( "hdd_noise", g_settings.hdd_noise);
	// END HDD

	// SOFT UPDATE
	configfile.setString("update_dir", g_settings.update_dir);
	configfile.setString("softupdate_proxyserver", g_settings.softupdate_proxyserver );
	configfile.setString("softupdate_proxyusername", g_settings.softupdate_proxyusername );
	configfile.setString("softupdate_proxypassword", g_settings.softupdate_proxypassword );
	configfile.setString("softupdate_url_file"      , g_settings.softupdate_url_file      );
	// END UPDATE

	// VFD 
	configfile.setInt32("lcd_brightness", g_settings.lcd_brightness);
	configfile.setInt32("lcd_standbybrightness", g_settings.lcd_standbybrightness);
	configfile.setInt32("lcd_contrast", g_settings.lcd_contrast);
	configfile.setInt32("lcd_power", g_settings.lcd_power);
	configfile.setInt32("lcd_inverse", g_settings.lcd_inverse);
	configfile.setInt32("lcd_statusline", g_settings.lcd_statusline);
	configfile.setInt32("lcd_epgmode", g_settings.lcd_epgmode);
	configfile.setInt32("lcd_mode", g_settings.lcd_mode);
	configfile.setInt32("lcd_epgalign", g_settings.lcd_epgalign);	
	configfile.setString("lcd_dim_time", g_settings.lcd_setting_dim_time);
	configfile.setInt32("lcd_dim_brightness", g_settings.lcd_setting_dim_brightness);
	configfile.setInt32("lcd_led", g_settings.lcd_led);
	configfile.setInt32("lcd_minitv", g_settings.lcd_minitv);
	configfile.setInt32("lcd_minitvfps", g_settings.lcd_minitvfps);
	configfile.setInt32("lcd_picon", g_settings.lcd_picon);
	configfile.setInt32("lcd_weather", g_settings.lcd_weather);
	configfile.setInt32("lcd_standby_clock", g_settings.lcd_standby_clock);
	// END VFD

	// online epg
	configfile.setBool("epg_enable_localtv_epg", g_settings.epg_enable_localtv_epg);
	configfile.setString("epg_serverbox_ip", g_settings.epg_serverbox_ip);
	configfile.setInt32("epg_serverbox_type", g_settings.epg_serverbox_type);
	configfile.setInt32("epg_serverbox_gui", g_settings.epg_serverbox_gui);
	
	// cicam
#if defined (ENABLE_CI)
	configfile.setInt32("ci_standby_reset", g_settings.ci_standby_reset);
	configfile.setInt32("ci_check_live", g_settings.ci_check_live);
	configfile.setInt32("ci_tuner", g_settings.ci_tuner);
	configfile.setInt32("ci_delay", g_settings.ci_delay);
#endif

	// cec
	configfile.setInt32( "hdmi_cec_mode", g_settings.hdmi_cec_mode );
	configfile.setInt32( "hdmi_cec_standby", g_settings.hdmi_cec_standby );
	configfile.setInt32( "hdmi_cec_volume", g_settings.hdmi_cec_volume );
	
	// personalize
	configfile.setInt32("personalize_tvradio", g_settings.personalize_tvradio);
	configfile.setInt32("personalize_epgtimer", g_settings.personalize_epgtimer);
	configfile.setInt32("personalize_scart", g_settings.personalize_scart);
	configfile.setInt32("personalize_features", g_settings.personalize_features);
	configfile.setInt32("personalize_service", g_settings.personalize_service);
	configfile.setInt32("personalize_information", g_settings.personalize_information);
	configfile.setInt32("personalize_powermenu", g_settings.personalize_powermenu);
	configfile.setInt32("personalize_mediaplayer", g_settings.personalize_mediaplayer);
	
	//
#ifdef ENABLE_GRAPHLCD
	configfile.setInt32("glcd_enable", g_settings.glcd_enable);
	configfile.setInt32("glcd_selected_config", g_settings.glcd_selected_config);
	configfile.setInt32("glcd_brightness", g_settings.glcd_brightness);
	configfile.setInt32("glcd_brightness_standby", g_settings.glcd_brightness_standby);
#endif

	if(strcmp(fname, NEUTRINO_SETTINGS_FILE))
		configfile.saveConfig(fname);

	else if (configfile.getModifiedFlag())
	{
		configfile.saveConfig(fname);
		configfile.setModifiedFlag(false);
	}
}

// firstChannel, get the initial channel
void CNeutrinoApp::firstChannel()
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::firstChannel\n");

	CZapit::getInstance()->getLastChannel(firstchannel.id, firstchannel.channelNumber, firstchannel.mode);
}

// CNeutrinoApp -  channelsInit, get the Channellist from zapit
void CNeutrinoApp::channelsInit()
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit\n");

	//
	if(TVallList) 
		delete TVallList;

	if(RADIOallList) 
		delete RADIOallList;

	if(TVbouquetList) 
		delete TVbouquetList;

	if(TVsatList) 
		delete TVsatList;

	if(TVfavList) 
		delete TVfavList;

	if(RADIObouquetList) 
		delete RADIObouquetList;

	if(RADIOsatList) 
		delete RADIOsatList;

	if(RADIOfavList) 
		delete RADIOfavList;

	if(TVchannelList) 
		delete TVchannelList;

	if(RADIOchannelList) 
		delete RADIOchannelList;

	TVchannelList = new CChannelList(_("All Services"));
	RADIOchannelList = new CChannelList(_("All Services"));

	TVbouquetList = new CBouquetList(_("Providers"));

	TVsatList = new CBouquetList(_("Satellites"));

	TVfavList = new CBouquetList(_("Favorites"));

	RADIObouquetList = new CBouquetList(_("Provider"));

	RADIOsatList = new CBouquetList(_("Satellites"));

	RADIOfavList = new CBouquetList(_("Favorites"));

	//
	int tvi = 1, ri = 1, hi = 0, webtvi = 0;
	
	// hd bouquet
	CBouquet * hdBouquet;
	if(g_settings.make_hd_list)
		hdBouquet = new CBouquet(0, (char *) "HD", 0);

	//// all services TV/ Radio/WebTV Channellist
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		if ((it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE)) 
		{
			TVchannelList->addChannel(&(it->second), tvi++);
			//tvi++;
			
			if (it->second.isWEBTV())
				webtvi++;
			
			if(it->second.isHD()) 
			{
				if(g_settings.make_hd_list)
				{
					hdBouquet->channelList->addChannel(&(it->second));
				}
				hi++;
			}
		}
		else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
		{
			RADIOchannelList->addChannel(&(it->second), ri++);
			//ri++;
		}
	}

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d services %d TV and %d RADIO (%d HD %d WEBTV)\n", (int)allchans.size(), tvi - 1, ri - 1, hi, webtvi);

	CBouquet * tmp;

	// tv all list
	TVallList = new CBouquetList(_("All Services"));
	tmp = TVallList->addBouquet(_("All Services"));
	*(tmp->channelList) = *TVchannelList;
//	tmp->channelList->SortAlpha();

	// radio all list
	RADIOallList = new CBouquetList(_("All Services"));
	tmp = RADIOallList->addBouquet(_("All Services"));
	*(tmp->channelList) = *RADIOchannelList;
//	tmp->channelList->SortAlpha();

	//// sat
	int bnum = 0;
	tvi = 1;
	ri = 1;
	sat_iterator_t sit;
	for (sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		CBouquet * tmp1 = TVsatList->addBouquet(sit->second.name.c_str());
		CBouquet * tmp2 = RADIOsatList->addBouquet(sit->second.name.c_str());

		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
		{
			if (it->second.getSatellitePosition() == sit->first && !it->second.isWEBTV()) //FIXME: 
			{
				if (it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE) 
				{
					tmp1->channelList->addChannel(&(it->second), tvi++);
				}
				else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
				{
					tmp2->channelList->addChannel(&(it->second), ri++);
				}
			}
		}

		if (tmp1->channelList->getSize() == 0)
			TVsatList->deleteBouquet(tmp1);

		if(tmp2->channelList->getSize() == 0)
			RADIOsatList->deleteBouquet(tmp2);
	}

	// tv favorites
	bnum = 0;
	tvi = 1;

	for (uint32_t i = 0; i < CZapit::getInstance()->Bouquets.size(); i++) 
	{
		if (!CZapit::getInstance()->Bouquets[i]->bHidden && !CZapit::getInstance()->Bouquets[i]->tvChannels.empty())
		{
			CBouquet * ltmp;
			if (CZapit::getInstance()->Bouquets[i]->bUser || CZapit::getInstance()->Bouquets[i]->bWebTV) 
			{
				ltmp = TVfavList->addBouquet(CZapit::getInstance()->Bouquets[i]);

				ZapitChannelList * channels = &(CZapit::getInstance()->Bouquets[i]->tvChannels);
				if (channels->size()) ltmp->channelList->setSize(channels->size());
				
				for(int j = 0; j < (int) channels->size(); j++) 
				{
					ltmp->channelList->addChannel((*channels)[j], tvi++);
				}
				bnum++;
			}
		}
	}
	
	if(g_settings.make_hd_list)
		TVfavList->Bouquets.push_back(hdBouquet);

	// radio fav
	bnum = 0;
	ri = 1;
	
	for (uint32_t i = 0; i < CZapit::getInstance()->Bouquets.size(); i++) 
	{	
		if (!CZapit::getInstance()->Bouquets[i]->bHidden && !CZapit::getInstance()->Bouquets[i]->bWebTV && !CZapit::getInstance()->Bouquets[i]->radioChannels.empty())
		{
			CBouquet * ltmp;
			if (CZapit::getInstance()->Bouquets[i]->bUser) 
			{
				ltmp = RADIOfavList->addBouquet(CZapit::getInstance()->Bouquets[i]);

				ZapitChannelList *channels = &(CZapit::getInstance()->Bouquets[i]->radioChannels);
				if (channels->size()) ltmp->channelList->setSize(channels->size());
				
				for(int j = 0; j < (int) channels->size(); j++) 
				{
					ltmp->channelList->addChannel((*channels)[j], ri++);
				}
				bnum++;
			}
		}
	}
	
	// tv provider
	bnum = 0;
	tvi = 1;

	for (uint32_t i = 0; i < CZapit::getInstance()->Bouquets.size(); i++) 
	{
		if (!CZapit::getInstance()->Bouquets[i]->bHidden && !CZapit::getInstance()->Bouquets[i]->tvChannels.empty())
		{
			CBouquet * ltmp;
			if(!CZapit::getInstance()->Bouquets[i]->bUser && !CZapit::getInstance()->Bouquets[i]->bWebTV) 
			{
				ltmp = TVbouquetList->addBouquet(CZapit::getInstance()->Bouquets[i]);

				ZapitChannelList * channels = &(CZapit::getInstance()->Bouquets[i]->tvChannels);
				if (channels->size()) ltmp->channelList->setSize(channels->size());
				
				for(int j = 0; j < (int) channels->size(); j++) 
				{
					ltmp->channelList->addChannel((*channels)[j], tvi++);
				}
				bnum++;
			}
		}
	}
	
	// radio provider
	bnum = 0;
	ri = 1;

	for (uint32_t i = 0; i < CZapit::getInstance()->Bouquets.size(); i++) 
	{	
		if (!CZapit::getInstance()->Bouquets[i]->bHidden && !CZapit::getInstance()->Bouquets[i]->radioChannels.empty())
		{
			CBouquet * ltmp;
			if(!CZapit::getInstance()->Bouquets[i]->bUser && !CZapit::getInstance()->Bouquets[i]->bWebTV) 
			{
				ltmp = RADIObouquetList->addBouquet(CZapit::getInstance()->Bouquets[i]);

				ZapitChannelList *channels = &(CZapit::getInstance()->Bouquets[i]->radioChannels);
				if (channels->size()) ltmp->channelList->setSize(channels->size());
				
				for(int j = 0; j < (int) channels->size(); j++) 
				{
					ltmp->channelList->addChannel((*channels)[j], ri++);
				}
				bnum++;
			}
		}
	}
	
	// loadwebtvlogos
	if (g_settings.logos_show_logo)
		CChannellogo::getInstance()->loadWebTVlogos();
}

void CNeutrinoApp::setChannelMode(int newmode, int nMode)
{
	const char *aLISTMODE[] = {
		"LIST_MODE_FAV",
		"LIST_MODE_PROV",
		"LIST_MODE_SAT",
		"LIST_MODE_ALL"
	};
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setChannelMode: ChannelsMode: %s nMode: %d\n", aLISTMODE[newmode], nMode);

	chmode = nMode;

	// bouquetList
	switch(newmode) 
	{
		case CChannelList::LIST_MODE_FAV:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOfavList;
			} 
			else if(nMode == mode_tv) 
			{
				bouquetList = TVfavList;
			}
			break;

		case CChannelList::LIST_MODE_SAT:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOsatList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVsatList;
			}
			break;
			
		case CChannelList::LIST_MODE_PROV:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIObouquetList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVbouquetList;
			}
			break;

		default:
		case CChannelList::LIST_MODE_ALL:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOallList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVallList;
			}
			break;
	}
	
	// channelList // FIXME:
	if(nMode == mode_radio)
		channelList = RADIOchannelList;
	else if(nMode == mode_tv)
		channelList = TVchannelList;

	g_settings.channel_mode = newmode;
}

//
void CNeutrinoApp::setDebugLevel( int level )
{	
	debug = level;
	
	const char *DEBUGMODE[] = {
		"NORMAL",
		"INFO",
		"DEBUG"
	};
	
	printf("CNeutrinoApp::setDebugLevel: %s\n", DEBUGMODE[debug]);
}

// setup the framebuffer
void CNeutrinoApp::setupFrameBuffer()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	frameBuffer->init();
	
	if(frameBuffer->setMode()) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupFrameBuffer: Error while setting framebuffer mode\n");
		exit(-1);
	}	
}

void CNeutrinoApp::setupColor(void)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupColor\n");
	
	// head
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUHEAD, convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue), convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue), 8, g_settings.menu_Head_alpha);

	// menu content
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUCONTENT, convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue), convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), 8, g_settings.menu_Content_alpha);

	// menu content dark
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUCONTENTDARK, convertSetupColor2RGB((g_settings.menu_Content_red*0.6), (g_settings.menu_Content_green*0.6), (g_settings.menu_Content_blue*0.6)), convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), 8, g_settings.menu_Content_alpha);

	// menu content selected
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUCONTENTSELECTED, convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue), convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue), 8, g_settings.menu_Content_Selected_alpha);

	// menu content inactiv
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUCONTENTINACTIVE, convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue), convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue), 8, g_settings.menu_Content_inactive_alpha);

	// foot
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUFOOT, convertSetupColor2RGB(g_settings.menu_Foot_red, g_settings.menu_Foot_green, g_settings.menu_Foot_blue), convertSetupColor2RGB(g_settings.menu_Foot_Text_red, g_settings.menu_Foot_Text_green, g_settings.menu_Foot_Text_blue), 8, g_settings.menu_Foot_alpha);

	// infobar
	CFrameBuffer::getInstance()->paletteGenFade(COL_INFOBAR, convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue), convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), 8, g_settings.infobar_alpha);

	// infobar shadow
	CFrameBuffer::getInstance()->paletteGenFade(COL_INFOBAR_SHADOW, convertSetupColor2RGB((g_settings.infobar_red*0.4), (g_settings.infobar_green*0.4), (g_settings.infobar_blue*0.4)), convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), 8, g_settings.infobar_alpha);

	// hint
	CFrameBuffer::getInstance()->paletteGenFade(COL_MENUHINT, convertSetupColor2RGB(g_settings.menu_Hint_red, g_settings.menu_Hint_green, g_settings.menu_Hint_blue), convertSetupColor2RGB(g_settings.menu_Hint_Text_red, g_settings.menu_Hint_Text_green, g_settings.menu_Hint_Text_blue), 8, g_settings.menu_Hint_alpha);
	
	// menu content inactiv text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUCONTENTINACTIVE_TEXT, convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue), g_settings.menu_Content_inactive_alpha);
	
	// menu content selected text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUCONTENTSELECTED_TEXT, convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue), g_settings.menu_Content_Selected_alpha);
	
	// menu content text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUCONTENT_TEXT, convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue), g_settings.menu_Content_alpha);
	
	// head text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUHEAD_TEXT, convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue), g_settings.menu_Head_alpha);
	
	// foot text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUFOOT_TEXT, convertSetupColor2RGB(g_settings.menu_Foot_Text_red, g_settings.menu_Foot_Text_green, g_settings.menu_Foot_Text_blue), g_settings.menu_Foot_alpha);
	
	// infobar text
	CFrameBuffer::getInstance()->paletteSetColor(COL_INFOBAR_TEXT, convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue), g_settings.infobar_alpha);

	// infobar colored events text
	CFrameBuffer::getInstance()->paletteSetColor(COL_INFOBAR_COLORED_EVENTS_TEXT, convertSetupColor2RGB(g_settings.infobar_colored_events_red, g_settings.infobar_colored_events_green, g_settings.infobar_colored_events_blue), g_settings.infobar_alpha);

	// hint text
	CFrameBuffer::getInstance()->paletteSetColor(COL_MENUHINT_TEXT, convertSetupColor2RGB(g_settings.menu_Hint_Text_red, g_settings.menu_Hint_Text_green, g_settings.menu_Hint_Text_blue), g_settings.menu_Hint_alpha);

	CFrameBuffer::getInstance()->paletteSet();
}


// setup fonts
void CNeutrinoApp::setupFonts(const char* font_file)
{
	const char * style[3];

	if (g_fontRenderer != NULL)
		delete g_fontRenderer;

	g_fontRenderer = new FBFontRenderClass(72 * g_settings.screen_xres / 100, 72 * g_settings.screen_yres / 100);

	if(font.filename != NULL)
		free((void *)font.filename);

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupFonts: font file: %s\n", font_file);

	if(access(font_file, F_OK)) 
	{
		if(!access(DATADIR "/fonts/arial.ttf", F_OK))
		{
			font.filename = strdup(DATADIR "/fonts/arial.ttf");
			strcpy(g_settings.font_file, font.filename);
		}
		else if(!access("/usr/share/fonts/arial.ttf", F_OK))
		{
			font.filename = strdup("/usr/share/fonts/arial.ttf");
			strcpy((char *)font_file, font.filename);
		}
		else
		{
			  fprintf( stderr,"CNeutrinoApp::setupFonts: font file [%s] not found\n neutrino exit\n", DATADIR "/fonts/arial.ttf");
			  _exit(0);
		}
	}
	else
	{
		font.filename = strdup(font_file);
		
		// check??? (use only true type fonts or fallback to arial.ttf
		if( !strstr(font.filename, ".ttf") )
		{
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupFonts: font file %s not ok falling back to arial.ttf\n", font_file);
			
			if(!access(DATADIR "/fonts/arial.ttf", F_OK))
			{
				font.filename = strdup(DATADIR "/fonts/arial.ttf");
				strcpy((char *)font_file, font.filename);
			}
			else if(!access("/usr/share/fonts/arial.ttf", F_OK))
			{
				font.filename = strdup("/usr/share/fonts/arial.ttf");
				strcpy((char *)font_file, font.filename);
			}
			else
			{
				  fprintf( stderr,"CNeutrinoApp::setupFonts: font file [%s] not found\n neutrino exit\n", DATADIR "/fonts/arial.ttf");
				  _exit(0);
			}
		}
	}

	style[0] = g_fontRenderer->AddFont(font.filename);

	if(font.name != NULL)
		free((void *)font.name);

	font.name = strdup(g_fontRenderer->getFamily(font.filename).c_str());

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupFonts: font family %s\n", font.name);

	style[1] = "Bold Regular";

	g_fontRenderer->AddFont(font.filename, true);  // make italics
	style[2] = "Italic";

	// set neutrino font
	for (int i = 0; i < FONT_TYPE_COUNT; i++)
	{
		if(g_Font[i] != NULL) 
			delete g_Font[i];

		g_Font[i] = g_fontRenderer->getFont(font.name, style[neutrino_font[i].style], neutrino_font[i].defaultsize + neutrino_font[i].size_offset * font.size_offset);
	}
}

//// start autoRecord (permanent/temp timeshift)
int CNeutrinoApp::startAutoRecord(bool addTimer)
{
	CTimerd::EventInfo eventinfo;

	if(CNeutrinoApp::getInstance()->recordingstatus)
		return 0;

	eventinfo.channel_id = CZapit::getInstance()->getCurrentChannelID();
	CEPGData epgData;
	
	if (CSectionsd::getInstance()->getActualEPGServiceKey(CZapit::getInstance()->getChannelEPGID(CZapit::getInstance()->getCurrentChannelID()) & 0xFFFFFFFFFFFFULL, &epgData ))
	{
		eventinfo.epgID = epgData.eventID;
		eventinfo.epg_starttime = epgData.epg_times.starttime;
		strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
		eventinfo.epgTitle[EPG_TITLE_MAXLEN - 1] = 0;
	}
	else 
	{
		eventinfo.epgID = 0;
		eventinfo.epg_starttime = 0;
		strcpy(eventinfo.epgTitle, "");
	}

	eventinfo.apids = TIMERD_APIDS_CONF;
	
	dprintf(DEBUG_NORMAL, "startAutoRecord: dir %s\n", timeshiftDir);

	CRecord::getInstance()->Directory = timeshiftDir;

	autoshift = 1;
	CNeutrinoApp::getInstance()->recordingstatus = 1;

	if( CRecord::getInstance()->Record(&eventinfo) == false ) 
	{
		CNeutrinoApp::getInstance()->recordingstatus = 0;
		autoshift = 0;
		
		CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );	
	}
	else if (addTimer) 
	{
		time_t now = time(NULL);
		CNeutrinoApp::getInstance()->recording_id = CTimerd::getInstance()->addImmediateRecordTimerEvent(eventinfo.channel_id, now, now + g_settings.record_hours*60*60, eventinfo.epgID, eventinfo.epg_starttime, eventinfo.apids);
	}	

	CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

	return 0;
}

// stop auto record
void CNeutrinoApp::stopAutoRecord()
{
	dprintf(DEBUG_NORMAL, "stopAutoRecord: recordingstatus %d\n", CNeutrinoApp::getInstance()->recordingstatus);
	
	if(autoshift && recordingstatus) 
	{
		CRecord::getInstance()->Stop();
		autoshift = false;
		
		if(CNeutrinoApp::getInstance()->recording_id) 
		{
			CTimerd::getInstance()->stopTimerEvent(CNeutrinoApp::getInstance()->recording_id);
			CNeutrinoApp::getInstance()->recording_id = 0;
		}
	} 
	else if(shift_timer)  
	{
		g_RCInput->killTimer(shift_timer);
		shift_timer = 0;
	}
	
	recordingstatus = 0;
	
	CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);
}

// do gui-record
void CNeutrinoApp::doGuiRecord(char * preselectedDir, bool addTimer)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::doGuiRecord:");
	
	CTimerd::EventInfo eventinfo;

	// stop autorecord
	if(autoshift) 
		stopAutoRecord();

	//
	if(recordingstatus == 1) 
	{
		//
		time_t now = time(NULL);
		int record_end = now + g_settings.record_hours*60*60;
		int pre = 0, post = 0;

		// get EPG info
		eventinfo.channel_id = CZapit::getInstance()->getCurrentChannelID();

		CEPGData epgData;

		if (CSectionsd::getInstance()->getActualEPGServiceKey(eventinfo.channel_id & 0xFFFFFFFFFFFFULL, &epgData ))
		{
			eventinfo.epgID = epgData.eventID;
			eventinfo.epg_starttime = epgData.epg_times.starttime;
			strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
			eventinfo.epgTitle[EPG_TITLE_MAXLEN - 1] = 0;
				
			// record end time
			CTimerd::getInstance()->getRecordingSafety(pre, post);
				
			if (epgData.epg_times.starttime > 0)
				record_end = epgData.epg_times.starttime + epgData.epg_times.duration + post;
		}
		else 
		{
			eventinfo.epgID = 0;
			eventinfo.epg_starttime = 0;
			strcpy(eventinfo.epgTitle, "");
		}

		eventinfo.apids = TIMERD_APIDS_CONF;

		// rec dir
		strcpy(recDir, (preselectedDir != NULL) ? preselectedDir : g_settings.network_nfs_recordingdir);
				
		CRecord::getInstance()->Directory = recDir;
			
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::doGuiRecord: start record to dir %s\n", recDir);

		// start to record 
		if(CRecord::getInstance()->Record(&eventinfo) == false )
		{
			recordingstatus = 0;
			timeshiftstatus = 0;
		}
		else if (addTimer) // add timer
		{
			recording_id = CTimerd::getInstance()->addImmediateRecordTimerEvent(eventinfo.channel_id, now, record_end, eventinfo.epgID, eventinfo.epg_starttime, eventinfo.apids);
		}
	} 
	else 
	{
		CTimerd::getInstance()->stopTimerEvent(recording_id);
			
		startNextRecording();
	}
		
	if (recordingstatus) 
		CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);
}

// startNextRecording
void CNeutrinoApp::startNextRecording()
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::startNextRecording\n");
	
	if ( nextRecordingInfo != NULL ) 
	{
		bool doRecord = true;
		
		recording_id = nextRecordingInfo->eventID;
			
		if (recDir != NULL)
		{
			char *lrecDir = strlen(nextRecordingInfo->recordingDir) > 0 ? nextRecordingInfo->recordingDir : g_settings.network_nfs_recordingdir;

			if (!CFSMounter::isMounted(lrecDir)) 
			{
				doRecord = false;
					
				for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
				{
					if (strcmp(g_settings.network_nfs_local_dir[i], lrecDir) == 0) 
					{
						CFSMounter::MountRes mres = CFSMounter::mount(g_settings.network_nfs_ip[i].c_str(), g_settings.network_nfs_dir[i], g_settings.network_nfs_local_dir[i], (CFSMounter::FSType) g_settings.network_nfs_type[i], g_settings.network_nfs_username[i], g_settings.network_nfs_password[i], g_settings.network_nfs_mount_options1[i], g_settings.network_nfs_mount_options2[i]);
										  
						if (mres == CFSMounter::MRES_OK) 
						{
							doRecord = true;
						} 
						else 
						{
							const char * merr = mntRes2Str(mres);
							int msglen = strlen(merr) + strlen(nextRecordingInfo->recordingDir) + 7;
							char msg[msglen];
							strcpy(msg, merr);
							strcat(msg, "\nDir: ");
							strcat(msg, nextRecordingInfo->recordingDir);

							HintBox(_("Error"), _(msg)); // UTF-8
							doRecord = false;
						}
						break;
					}
				}

				if (!doRecord) 
				{
					// recording dir does not seem to exist in config anymore
					// or an error occured while mounting
					// -> try default dir
					lrecDir = g_settings.network_nfs_recordingdir;
					
					doRecord = true;
				}
			}

			CRecord::getInstance()->Directory = lrecDir;
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::startNextRecording: start to dir %s\n", lrecDir);

			CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);
		}
			
		if(doRecord && CRecord::getInstance()->Record(nextRecordingInfo))
			recordingstatus = 1;
		else
			recordingstatus = 0;

		nextRecordingInfo = NULL;
	}
}

// send sectionsd config
void CNeutrinoApp::sendSectionsdConfig(void)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::sendSectionsdConfig\n");

        CSectionsd::epg_config config;
	
        config.epg_cache                = atoi(g_settings.epg_cache.c_str());
        config.epg_old_events           = atoi(g_settings.epg_old_events.c_str());
        config.epg_max_events           = atoi(g_settings.epg_max_events.c_str());
        config.epg_extendedcache        = atoi(g_settings.epg_extendedcache.c_str());
        config.epg_dir                  = g_settings.epg_dir;
        config.network_ntpserver        = g_settings.network_ntpserver;
        config.network_ntprefresh       = atoi(g_settings.network_ntprefresh.c_str());
        config.network_ntpenable        = g_settings.network_ntpenable;
	
        CSectionsd::getInstance()->setConfig(config);
}

// init zapper
void CNeutrinoApp::initZapper()
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::initZapper\n");

	// read saved epg
	if (g_settings.epg_save)
	{
		//
		readEPG();
		
		//
		for (unsigned long i = 0; i < g_settings.xmltv.size(); i++)
		{
			CSectionsd::getInstance()->readSIfromXMLTV(g_settings.xmltv[i].c_str());
		}
	}
	
	// init channel / setChannelsMode
	channelsInit();

	// getchannelsMode
	int tvmode = CZapit::getInstance()->getMode();

	if (tvmode == CZapit::MODE_TV)
		tvMode(false);
	else if (tvmode == CZapit::MODE_RADIO)
		radioMode(false);

	lastMode = mode;
	
	//
	setChannelMode(g_settings.channel_mode, mode);
	
	// start epgTimer
	epgUpdateTimer = g_RCInput->addTimer( 60 * 1000 * 1000, false );

	// zap / epg / autorecord / infoviewer
	if (channelList)
	{
		if (channelList->getSize() && CZapit::getInstance()->getCurrentChannelID() != 0)
		{
			// channellist adjust to channeliD
			channelList->adjustToChannelID(CZapit::getInstance()->getCurrentChannelID());
					
			// permenant timeshift
			if(g_settings.auto_timeshift)
				startAutoRecord(true);
			
			//
			selectSubtitles();
			startSubtitles();
		}
	}
}

// quickZap
void CNeutrinoApp::quickZap(int msg)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::quickZap: key:0x%x\n", msg);
	
	stopSubtitles();
	
	if(g_settings.zap_cycle && (bouquetList != NULL) && !(bouquetList->Bouquets.empty()))
		bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->quickZap(msg, true);
	else
		channelList->quickZap(msg);
}

int CNeutrinoApp::numericZap(int msg)
{
	int res = 0;
	int old_b = bouquetList->getActiveBouquetNumber();
				
	if(bouquetList->Bouquets.size() && bouquetList->Bouquets[old_b]->channelList->getSize() > 0)
		res = bouquetList->Bouquets[old_b]->channelList->numericZap( msg );
	else
		res = channelList->numericZap( msg );
		
	return res;
}

// showInfo
void CNeutrinoApp::showInfo(const CZapitChannel *channel)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::showInfo:\n");
	
	stopSubtitles();

	//
	g_InfoViewer->showTitle(channel? channel->getIndex() : 0, channel? channel->getName() : "", channel? channel->getSatellitePosition() : 0, channel? channel->getChannelID() : 0);

	startSubtitles();
}

void CNeutrinoApp::readEPG()
{
	// read saved epg
	struct stat my_stat;
	
	if(stat(g_settings.epg_dir.c_str(), &my_stat) == 0)
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::readEpg: read EPG from: %s....\n", g_settings.epg_dir.c_str());

		CSectionsd::getInstance()->readSIfromXML(g_settings.epg_dir.c_str());
	}
}

// save epg
void CNeutrinoApp::saveEpg()
{
	struct stat my_stat;
   
	if(stat(g_settings.epg_dir.c_str(), &my_stat) == 0)
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveEpg: Saving EPG to %s....\n", g_settings.epg_dir.c_str());
		
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;
		
		CSectionsd::getInstance()->writeSI2XML(g_settings.epg_dir.c_str());

		while( true ) 
		{
			g_RCInput->getMsg(&msg, &data, 30); // 30 secs
			
			if (( msg == CRCInput::RC_timeout ) || (msg == NeutrinoMessages::EVT_SI_FINISHED)) 
			{
				break;
			}
		}
	}
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveEpg: Saving EPG to %s finished\n", g_settings.epg_dir.c_str());
}

// mute
void CNeutrinoApp::audioMute( int newValue, bool isEvent )
{
	muteIcon->enableSaveScreen();

	CLCD::getInstance()->setMuted(newValue);

	current_muted = newValue;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::audioMute: current_muted %d new %d isEvent: %d\n", current_muted, newValue, isEvent);
	
	CZapit::getInstance()->muteAudio(current_muted);

	// paint
	if( isEvent && ( mode != mode_scart ) && ( mode != mode_audio) && ( mode != mode_pic))
	{
		if( current_muted ) 
		{
			muteIcon->paint();
		}
		else
		{
			muteIcon->hide();
		}
		
		frameBuffer->blit();	
	}
}

// set volume
void CNeutrinoApp::setVolume(const neutrino_msg_t key, const bool bDoPaint)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp:setVolume:\n");
	
	neutrino_msg_t msg = key;

	int dx = 296;
	int dy = 32;

	int x = frameBuffer->getScreenX() + 10;
	int y = frameBuffer->getScreenY() + 10;

	current_volume = g_settings.current_volume;
	
	int a_step = atoi(g_settings.audio_step);
	
	switch( g_settings.volume_pos )
	{
		case 0:// upper right
			x = frameBuffer->getScreenWidth() - dx - 6;
			break;
			
		case 1:// upper left
			break;
			
		case 2:// bottom left
			y = frameBuffer->getScreenHeight() - dy;
			break;
			
		case 3:// bottom right
			x = frameBuffer->getScreenWidth() - dx;
			y = frameBuffer->getScreenHeight() - dy;
			break;
			
		case 4:// center default
			x = ((frameBuffer->getScreenWidth() - dx) / 2) + x;
			break;
			
		case 5:// center higher
			x = ((frameBuffer->getScreenWidth() - dx) / 2) + x;
			y = frameBuffer->getScreenHeight() - frameBuffer->getScreenHeight()/15;
			break;
	}
	
	g_volscale = new CCProgressBar(x + dy + dy/4, y + dy/4, 200, 15);

	if(bDoPaint) 
	{
		if (vol_pixbuf == NULL)
		{
			vol_pixbuf = new fb_pixel_t[dx * dy];

			if(vol_pixbuf != NULL)
			{
				frameBuffer->saveScreen(x, y, dx, dy, vol_pixbuf);			
			}
		}
		
		// background box
		frameBuffer->paintBoxRel(x, y, dx, dy, COL_MENUCONTENT_PLUS_0, g_settings.Head_radius | g_settings.Foot_radius, CORNER_ALL);
		
		// vol box aussen
		frameBuffer->paintBoxRel(x + dy + dy/4 - 2, y + dy/4 - 2, dy*25/4 + 4, dy/2 + 4, COL_MENUCONTENT_PLUS_3);
		
		// vol box innen
		frameBuffer->paintBoxRel(x + dy + dy/4, y + dy/4, dy*25/4, dy/2, COL_MENUCONTENT_PLUS_1);
		
		//icon
		frameBuffer->paintIcon(NEUTRINO_ICON_VOLUME, x + dy/2, y + dy/4, 0, dy);

		g_volscale->reset();

		//
		g_volscale->refresh(current_volume);

		char p1[4]; // 3 digits + '\0'
		sprintf(p1, "%3d", current_volume);

		// erase the numbers
		frameBuffer->paintBoxRel(x + dx - 50, y , 40, dy, COL_MENUCONTENT_PLUS_0);

		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p1, COL_MENUCONTENT_TEXT_PLUS_0);

		frameBuffer->blit();
	}

	neutrino_msg_data_t data;

	uint64_t timeoutEnd;

	do {
		if (msg <= CRCInput::RC_MaxRC) 
		{
			if ( msg == CRCInput::RC_plus ) 
			{ 
				if (g_settings.current_volume < 100 - a_step )
					g_settings.current_volume += a_step;
				else
					g_settings.current_volume = 100;
			}
			else if ( msg == CRCInput::RC_minus ) 
			{ 
				if (g_settings.current_volume > a_step)
					g_settings.current_volume -= a_step;
				else
					g_settings.current_volume = 0;
			}
			else 
			{
				g_RCInput->postMsg(msg, data);
				break;
			}

			//
			if(audioDecoder)
				audioDecoder->setVolume(g_settings.current_volume, g_settings.current_volume);	
			
			//FIXME
			if (current_muted && msg == CRCInput::RC_plus)
				audioMute(0, true);

			timeoutEnd = CRCInput::calcTimeoutEnd(10);
		}
		else if (handleMsg(msg, data) & messages_return::unhandled) 
		{
			g_RCInput->postMsg(msg, data);
			break;
		}

		if (bDoPaint) 
		{
			if(current_volume != g_settings.current_volume) 
			{
				current_volume = g_settings.current_volume;
				
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::setVolume: current_volume %d\n", current_volume);

				//
				g_volscale->refresh(current_volume);

				char p[4]; // 3 digits + '\0'
				sprintf(p, "%3d", current_volume);

				// erase the numbers
				frameBuffer->paintBoxRel(x + dx - 50, y, 40, dy, COL_MENUCONTENT_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p, COL_MENUCONTENT_TEXT_PLUS_0);
				
				frameBuffer->blit();

				//FIXME
				if (mode != mode_scart && mode != mode_pic && (g_settings.current_volume == 0) )
					audioMute(true, true);
			}
		}

		CLCD::getInstance()->showVolume(g_settings.current_volume);

		if (msg != CRCInput::RC_timeout) 
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		}

		frameBuffer->blit();	
	} while (msg != CRCInput::RC_timeout);

	if(bDoPaint) 
	{
		if(vol_pixbuf != NULL) 
		{
			frameBuffer->restoreScreen(x, y, dx, dy, vol_pixbuf);
		
			delete [] vol_pixbuf;
			vol_pixbuf = NULL;
			
		}
		else
			frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
			
		frameBuffer->blit();
	}
	
	if (g_volscale)
	{
		delete g_volscale;
		g_volscale = NULL;
	}
}

// tv mode
void CNeutrinoApp::tvMode( bool rezap )
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::tvMode: rezap %s\n", rezap ? "yes" : "no");
	
	//
	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();
	frameBuffer->blit();
	
	if(mode == mode_radio ) 
	{	  
		if (g_settings.radiotext_enable && g_Radiotext) 
		{
			videoDecoder->finishShowSinglePic();
			
			delete g_Radiotext;
			g_Radiotext = NULL;
		}			

		CLCD::getInstance()->ShowIcon(VFD_ICON_RADIO, false);

		startSubtitles(!rezap);
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

        if( mode == mode_tv ) 
	{
                return;
	}
	else if( mode == mode_scart )
	{	  
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);		
	}
	else if( mode == mode_standby ) 
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		
		// set video standby		
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);		
	}

	bool stopauto = (mode != mode_ts);	
	mode = mode_tv;
	
	if(stopauto && autoshift) 
	{
		stopAutoRecord();
	}	

	g_RemoteControl->tvMode();
	setChannelMode(g_settings.channel_mode, mode);

	// rezap
	if( rezap ) 
	{
		// FIXME:
		firstChannel();
		channelList->tuned = 0xfffffff;
		channelList->zapTo(firstchannel.channelNumber);
		//channelList->zapToChannelID(firstchannel.id, rezap);
	}
}

// Radio Mode
void CNeutrinoApp::radioMode( bool rezap)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: rezap %s\n", rezap ? "yes" : "no");

	if(mode == mode_tv ) 
	{	  
		stopSubtitles();
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

	if( mode == mode_radio ) 
	{
		return;
	}
	else if( mode == mode_scart ) 
	{	  
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);	
	}
	else if( mode == mode_standby ) 
	{	  
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

		// set video standby
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);		
	}

	mode = mode_radio;

	if(autoshift) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: standby on: autoshift ! stopping ...\n");
		
		stopAutoRecord();
	}
	
	//
	frameBuffer->useBackground(false);
	frameBuffer->loadBackgroundPic("radiomode.jpg");
	frameBuffer->blit();

	g_RemoteControl->radioMode();
	setChannelMode( g_settings.channel_mode, mode);
	
	////
	if (g_settings.radiotext_enable) 
	{
		//
		if (g_Radiotext == NULL)
			g_Radiotext = new CRadioText();
	} 
	
	if( rezap ) 
	{
		// FIXME:
		firstChannel();
		channelList->tuned = 0xfffffff;
		channelList->zapTo(firstchannel.channelNumber);
		//channelList->zapToChannelID(firstchannel.id, rezap);
	}
}

// scart Mode
void CNeutrinoApp::scartMode( bool bOnOff )
{
	printf( ( bOnOff ) ? "CNeutrinoApp::scartMode: scart on\n" : "CNeutrinoApp::scartMode: mode: scart off\n" );

	if( bOnOff ) 
	{
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();
		frameBuffer->blit();

		CLCD::getInstance()->setMode(CLCD::MODE_SCART);

		lastMode = mode;
		mode = mode_scart;
			  
		if(videoDecoder)
		{
			videoDecoder->SetInput(INPUT_SCART);
			videoDecoder->SetStandby(STANDBY_ON);
		}	
	} 
	else 
	{	  
		if(videoDecoder)
		{
			videoDecoder->SetInput(INPUT_ENCODER);
			videoDecoder->SetStandby(STANDBY_OFF);
		}

		mode = mode_unknown;
		
		//re-set mode
		if( lastMode == mode_radio ) 
		{
			radioMode( false );
		}
		else if( lastMode == mode_tv ) 
		{
			tvMode( false );
		}
		else if( lastMode == mode_standby ) 
		{
			standbyMode( true );
		}
	}
}

// standby mode
void CNeutrinoApp::standbyMode( bool bOnOff )
{
	static bool wasshift = false;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::standbyMode: recordingstatus (%d) timeshiftstatus(%d) bOnOff (%d)\n", recordingstatus, timeshiftstatus, bOnOff);

	if( bOnOff ) 
	{
		if(autoshift) 
		{
			stopAutoRecord();
			wasshift = true;
		}
		
		if( mode == mode_scart ) 
		{
			// do not things
		}

		if (mode == mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL)
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}

		stopSubtitles();

		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();
		frameBuffer->blit();		
		
		// show time in vfd
		CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
			  
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_ON);		
		
		// zapit standby
		if(!recordingstatus && !timeshiftstatus && !CStreamManager::getInstance()->StreamStatus())
		{
			CZapit::getInstance()->setStandby(true);
		} 
		else
		{
			//zapit stop playback
			CZapit::getInstance()->stopPlayBack();
		}

		// stop sectionsd
		CSectionsd::getInstance()->setServiceChanged(0, false);
		CSectionsd::getInstance()->pauseScanning(true);

		//save epg
		if(!recordingstatus && !timeshiftstatus)
		{
			if(g_settings.epg_save) 
			{
				saveEpg();
			}
		}			

		lastMode = mode;
		mode = mode_standby;

		frameBuffer->setActive(false);

		// cec
#if !defined (__sh__)	
		hdmi_cec::getInstance()->setCECAutoStandby(g_settings.hdmi_cec_standby == 1);
		hdmi_cec::getInstance()->getAudioDestination();
		hdmi_cec::getInstance()->setCECMode((VIDEO_HDMI_CEC_MODE)g_settings.hdmi_cec_mode);
#endif	
	} 
	else 
	{
		// cec
#if !defined (__sh__)
		hdmi_cec::getInstance()->setCECAutoStandby(g_settings.hdmi_cec_standby == 1);
		hdmi_cec::getInstance()->getAudioDestination();
		hdmi_cec::getInstance()->setCECMode((VIDEO_HDMI_CEC_MODE)g_settings.hdmi_cec_mode);
#endif		

		// set fb active
		frameBuffer->setActive(true);

		// vfd mode
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
			  
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);		
				
		// setmode?tv/radio
		mode = mode_unknown;

		// zapit startplayback
		CZapit::getInstance()->setStandby(false);

		// this is buggy don't respect parentallock
		if(!recordingstatus && !timeshiftstatus)
			CZapit::getInstance()->startPlayBack(CZapit::getInstance()->getCurrentChannel());


		CSectionsd::getInstance()->pauseScanning(false);
		CSectionsd::getInstance()->setServiceChanged(CZapit::getInstance()->getCurrentChannelID(), true );

		if( lastMode == mode_radio ) 
		{
			radioMode(false);
		} 
		else if(lastMode == mode_tv)
		{
			tvMode(false);
		}

		// set vol (saved)
		audioMute(current_muted, false );					

		// start record if
		if((mode == mode_tv) && wasshift) 
		{
			startAutoRecord(true);
		}

		wasshift = false;

		startSubtitles();
	}
}

int CNeutrinoApp::exec(CWidgetTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: actionKey: %s\n", actionKey.c_str());

	int returnval = RETURN_REPAINT;
	
	if (parent)
		parent->hide();

	if(actionKey == "shutdown") 
	{
		exitRun(SHUTDOWN);
	}
	else if(actionKey == "reboot")
	{
		exitRun(REBOOT);
	}
	else if(actionKey == "restart") 
	{		
		exitRun(RESTART);
	}
	else if(actionKey == "restart_dont_save") 
	{		
		exitRun(RESTART, false);
	}
	else if(actionKey == "standby")
	{
		standbyMode(true);
	}	
	else if(actionKey == "tv") 
	{
		tvMode();
		returnval = RETURN_EXIT_ALL;
	}
	else if(actionKey == "radio") 
	{
		radioMode();
		returnval = RETURN_EXIT_ALL;
	}
	else if(actionKey == "scart") 
	{
		g_RCInput->postMsg( NeutrinoMessages::RECORD_ON);
		returnval = RETURN_EXIT_ALL;
	}
	else if (actionKey == "tvradioswitch")
	{
		if( mode == mode_tv )
			radioMode();
		else if( mode == mode_radio )
			tvMode();
			
		returnval = RETURN_EXIT_ALL;
	}
	else if(actionKey == "savesettings") 
	{
		HintBox(_("Information"), _("Saving settings now, please be patient."));
		
		//
		saveSetup(NEUTRINO_SETTINGS_FILE);
	}
	else if (actionKey == "saveskinsettings")
	{
		// fetch skin config file
		std::string skinConfig = CONFIGDIR "/skins/";
		skinConfig += g_settings.preferred_skin.c_str();
		skinConfig += "/";
		skinConfig += g_settings.preferred_skin.c_str();
		skinConfig += ".config";
				
		saveSkinConfig(skinConfig.c_str());
	}
	else if(actionKey == "reloadchannels")
	{
		HintBox(_("Information"), _("Reloading channel lists, please be patient."));
		
		CZapit::getInstance()->reinitChannels();
	}
	else if (actionKey == "reloadepg")
	{
		HintBox(_("Information"), _("Reloading EPG, please be patient."));
			
		CSectionsd::getInstance()->readSIfromXML(g_settings.epg_dir.c_str());
	}
	else if (actionKey == "reloadxmltvepg")
	{
		HintBox(_("Information"), _("Reloading XMLTV EPG, please be patient."));
			
		for (unsigned long i = 0; i < g_settings.xmltv.size(); i++)
		{
			CSectionsd::getInstance()->readSIfromXMLTV(g_settings.xmltv[i].c_str());
		}
	}
	else if (actionKey == "delete_zapit")
	{
		my_system(3, "/bin/sh", "-c", "rm -f " CONFIGDIR "/zapit/services.xml");
		my_system(3, "/bin/sh", "-c", "rm -f " CONFIGDIR "/zapit/bouquets.xml");
		CZapit::getInstance()->reinitChannels();
	}
	else if (actionKey == "delete_webtv")
	{
		my_system(3, "/bin/sh", "-c", "rm -f " CONFIGDIR "/webtv/*.*");
		CZapit::getInstance()->reinitChannels();
	}
	else if (actionKey == "free_memory")
	{
		CSectionsd::getInstance()->dumpStatus();
		CSectionsd::getInstance()->freeMemory();
	}
	else if (actionKey == "mainmenu")
	{
		mainMenu();
	}
	else if(actionKey == "features")
	{
		stopSubtitles();
		showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
		startSubtitles();
	}
	else if(actionKey == "plugins")
	{
		stopSubtitles();
		
		CPluginList * pluginList = new CPluginList();
		pluginList->exec(NULL, "");
		delete pluginList;
		pluginList = NULL;

		startSubtitles();
	}

	return returnval;
}

// start subtitle
void CNeutrinoApp::startSubtitles(bool show)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::startSubtitles\n");
	
	if(!show || IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
		return;
	
	//start dvbsub
	dvbsub_start(dvbsub_getpid(), false);
	
	// start tuxtxt
	tuxtx_pause_subtitle( false, false);
}

// stop subtitle
void CNeutrinoApp::stopSubtitles()
{
	if (IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
		return;
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::stopSubtitles\n");
	
	int ttx, ttxpid, ttxpage;

	// dvbsub
	int dvbpid = dvbsub_getpid();
	
	if(dvbpid)
	{
		dvbsub_pause();
	}
	
	// tuxtxt
	tuxtx_subtitle_running(&ttxpid, &ttxpage, &ttx);
	
	if(ttx) 
	{
		tuxtx_pause_subtitle(true, false);
		
		// clear framebuffer
		frameBuffer->paintBackground();
		frameBuffer->blit();
	}
}

// select subtitle
void CNeutrinoApp::selectSubtitles()
{
	if(!g_settings.auto_subs)
		return;

	int curnum = channelList->getActiveChannelNumber();
	CZapitChannel * cc = channelList->getChannel(curnum);

	for(int i = 0; i < 3; i++) 
	{
		if(strlen(g_settings.pref_subs[i]) == 0)
			continue;

		std::string temp(g_settings.pref_subs[i]);

		// dvbsub
		for(int j = 0 ; j < (int)cc->getSubtitleCount() ; j++) 
		{
			CZapitAbsSub* s = cc->getChannelSub(j);
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
				std::map<std::string, std::string>::const_iterator it;
				for(it = iso639.begin(); it != iso639.end(); it++) 
				{
					if(temp == it->second && sd->ISO639_language_code == it->first) 
					{
						printf("CNeutrinoApp::selectSubtitles: found DVB lang=%s pid=0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);
						dvbsub_stop();
						dvbsub_setpid(sd->pId);
						return;
					}
				}
			}
		}
		
		// tuxtxtsub
		for(int j = 0 ; j < (int)cc->getSubtitleCount() ; j++) 
		{
			CZapitAbsSub* s = cc->getChannelSub(j);
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				std::map<std::string, std::string>::const_iterator it;
				
				for(it = iso639.begin(); it != iso639.end(); it++) 
				{
					if(temp == it->second && sd->ISO639_language_code == it->first) 
					{
						int page = ((sd->teletext_magazine_number & 0xFF) << 8) | sd->teletext_page_number;
						printf("CNeutrinoApp::selectSubtitles: found TTX lang=%s, pid=0x%x page=0x%03X\n", sd->ISO639_language_code.c_str(), sd->pId, page);

						tuxtx_stop_subtitle();

						tuxtx_set_pid(sd->pId, page, (char *) sd->ISO639_language_code.c_str());
						return;
					}
				}
			}
		}
	}
}

void CNeutrinoApp::lockPlayBack(void)
{
	// stop subtitles
	stopSubtitles();

	// stop/lock live playback	
	CZapit::getInstance()->lockPlayBack();
		
	//pause epg scanning
	CSectionsd::getInstance()->pauseScanning(true);	
}

void CNeutrinoApp::unlockPlayBack(void)
{
	// unlock playback	
	CZapit::getInstance()->unlockPlayBack();	
		
	//start epg scanning
	CSectionsd::getInstance()->pauseScanning(false);

	// start subtitles
	startSubtitles();
}

// exitRun
void CNeutrinoApp::exitRun(int retcode, bool save)
{
	dprintf(DEBUG_NORMAL, "(retcode:%d) (save:%s)\n", retcode, save? "true" :"false");
	
	// break silently autotimeshift
	if(autoshift) 
	{
		stopAutoRecord();
	}
	
	//
	if (!recordingstatus || MessageBox(_("Information"), _("You really want to to stop record ?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == CMessageBox::mbrYes)  
	{
		// stop recording
		if(recordingstatus) 
		{
			CRecord::getInstance()->Stop();
			CTimerd::getInstance()->stopTimerEvent(recording_id);
		}

		// stop playback
		CZapit::getInstance()->stopPlayBack();
		
		if(retcode > RESTART)
		{
			// vfd mode shutdown
			CLCD::getInstance()->setMode(CLCD::MODE_SHUTDOWN);
			
			frameBuffer->loadBackgroundPic("shutdown.jpg");
			frameBuffer->blit();
		}

		// network down
		CNetworkSettings::getInstance()->networkConfig->automatic_start = (CNetworkSettings::getInstance()->network_automatic_start == 1);

		CNetworkSettings::getInstance()->networkConfig->commitConfig();
		
		// save neutrino.conf
		if (save)
		{
			// save neutrino.conf
			saveSetup(NEUTRINO_SETTINGS_FILE);
		}

		// save epg
		if(save && g_settings.epg_save ) 
			saveEpg();

		// stop / close dvbsub
		dvbsub_close();

		// stop / close txt
		tuxtxt_stop();
		tuxtxt_close();
		tuxtx_stop_subtitle();
			
		// stop nhttpd		
		Cyhttpd::getInstance()->Stop();
		
		// stop streamts	
		CStreamManager::getInstance()->Stop();

		// stop timerd	  
		CTimerd::getInstance()->Stop();		

		// stop sectionsd
		CSectionsd::getInstance()->Stop();

		// zapit stop	
		CZapit::getInstance()->Stop();
		
		//
#if !defined (USE_OPENGL)		
		time_t t = time(NULL);
	
		struct tm *lt = localtime(&t);
		
		proc_put("/proc/stb/fp/rtc", t + lt->tm_gmtoff);
#endif
		if (epgUpdateTimer)
		{
			g_RCInput->killTimer(epgUpdateTimer);
			epgUpdateTimer = 0;
		}	
			
		if(playback)
		{
			delete playback;
			playback = NULL;
		}
			
		if(audioDecoder)
		{
			delete audioDecoder;
			audioDecoder = NULL;
		}
	
		if(videoDecoder)
		{
			delete videoDecoder;
			videoDecoder = NULL;
		}
			
		if (g_RCInput != NULL)
		{
			delete g_RCInput;
			g_RCInput = NULL;
		}
			
		if(g_RemoteControl)
		{
			delete g_RemoteControl;
			g_RemoteControl = NULL;
		}
			
		if (g_EpgData)
		{
			delete g_EpgData;
			g_EpgData = NULL;
		}
			
		if (g_EventList)
		{
			delete g_EventList;
			g_EventList = NULL;
		}
			
		if(g_fontRenderer)
		{
			delete g_fontRenderer;
			g_fontRenderer = NULL;
		}
		
		// cec
#if !defined (__sh__)
		hdmi_cec::getInstance()->setCECAutoStandby(g_settings.hdmi_cec_standby == 1);
		hdmi_cec::getInstance()->getAudioDestination();
		hdmi_cec::getInstance()->setCECMode((VIDEO_HDMI_CEC_MODE)g_settings.hdmi_cec_mode);
#endif
		
		//
		if (muteIcon)
		{
			delete muteIcon;
			muteIcon = NULL;
		}
		
#ifdef USE_OPENGL
		ao_shutdown();
#endif

		dprintf(DEBUG_NORMAL, ">>> CNeutrinoApp::exitRun: Good bye (retcode: %d) <<<\n", retcode);
		
		//
		usleep(1500);
				
		if(retcode == RESTART)
		{		  
			execvp(global_argv[0], global_argv); // no return if successful
		}		
		
		_exit(retcode);	
	}
}

// handle msg
int CNeutrinoApp::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int res = 0;

	// zap complete event
	if(msg == NeutrinoMessages::EVT_ZAP_COMPLETE || msg == NeutrinoMessages::EVT_ZAP_FAILED) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: %s current_channel_id: 0x%llx data:0x%llx\n", (msg == NeutrinoMessages::EVT_ZAP_FAILED)? "EVT_ZAP_FAILED" : "EVT_ZAP_COMPLETE", CZapit::getInstance()->getCurrentChannelID(), data);
		
		// set audio map after channel zap
		CZapit::getInstance()->getAudioMode(&g_settings.audio_AnalogMode);

		if(g_settings.audio_AnalogMode < 0 || g_settings.audio_AnalogMode > 2)
			g_settings.audio_AnalogMode = 0;

		// kill shifttimer
		if(shift_timer) 
		{
			g_RCInput->killTimer(shift_timer);
			shift_timer = 0;
		}	

		// auto timeshift
		if (!recordingstatus && g_settings.auto_timeshift) 		  
		{
			int delay = g_settings.auto_timeshift;
			
			// add shift timer
			shift_timer = g_RCInput->addTimer( delay*1000*1000, true );
			
			// infoviewer handle msg
			g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, recordingstatus);
		}	

		// scrambled timer
		if(scrambled_timer) 
		{
			g_RCInput->killTimer(scrambled_timer);
			scrambled_timer = 0;
		}

		scrambled_timer = g_RCInput->addTimer(10*1000*1000, true); // 10 sec
		
		// select subtitle
		selectSubtitles();
		
		startSubtitles(!g_InfoViewer->is_visible);
		
		//
		CLCD::getInstance()->setEPGTitle("");
		
		// store channel into lastchannellist
		_lastChList.store(CZapit::getInstance()->getCurrentChannelID());
	}

	// shift / scrambled timer events
	if ((msg == NeutrinoMessages::EVT_TIMER)) 
	{
		if(data == shift_timer) 
		{
			shift_timer = 0;
			startAutoRecord(true);
			
			return messages_return::handled;
		} 
		else if(data == scrambled_timer) 
		{
			scrambled_timer = 0;
			
#ifdef USE_SCRAMBLED_TIMER
			if(true && (videoDecoder->getBlank() && videoDecoder->getPlayState())) 
			{
				HintBox(_("Information"), _("Scrambled channel"), g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth (_("Scrambled channel"), true) + 10, 5);
			}
#endif

			return messages_return::handled;	
		}
	}

	// handle msg with CRemotecontrol / CInfoviewer / CCAMMenuHandler
	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
#if defined (ENABLE_CI)	
	res = res | g_CamHandler->handleMsg(msg, data);
#endif

	//
	if( res != messages_return::unhandled ) 
	{
		return( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

	// 
	if( msg == CRCInput::RC_ok || msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
	{
		if(g_InfoViewer->is_visible)
			g_InfoViewer->killTitle();

		if( (mode == mode_tv) || (mode == mode_radio))
		{
			stopSubtitles();

			// pre-selected channel-num/bouquet-num/channel-mode
			int nNewChannel = -1;
			int old_num = 0;
			int old_b = bouquetList->getActiveBouquetNumber();
			int old_mode = g_settings.channel_mode;

			if(bouquetList->Bouquets.size()) 
			{
				old_num = bouquetList->Bouquets[old_b]->channelList->getActiveChannelNumber();
			}
			
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: ZAP START: bouquet: %d channel: %d\n", old_b, old_num);

			if( msg == CRCInput::RC_ok ) 
			{
				if(bouquetList->Bouquets.size() && bouquetList->Bouquets[old_b]->channelList->getSize() > 0)
					nNewChannel = bouquetList->Bouquets[old_b]->channelList->exec();
				else
					nNewChannel = bouquetList->exec();
			}
			else if(msg == CRCInput::RC_sat) 
			{
				setChannelMode(CChannelList::LIST_MODE_SAT, mode);
				nNewChannel = bouquetList->exec();
			}
			else if(msg == CRCInput::RC_favorites) 
			{
				setChannelMode(CChannelList::LIST_MODE_FAV, mode);
				nNewChannel = bouquetList->exec();
			}
_repeat:
			dprintf(DEBUG_DEBUG, "CNeutrinoApp::handleMsg: ZAP RES: nNewChannel %d\n", nNewChannel);

			if(nNewChannel == -1) // timeout / cancel
			{
				// restore orig. bouquet and selected channel on cancel
				setChannelMode(old_mode, mode);
				bouquetList->activateBouquet(old_b);
				
				if(bouquetList->Bouquets.size())
					bouquetList->Bouquets[old_b]->channelList->setSelected(old_num - 1);
				
				startSubtitles(mode == mode_tv);
			}
			else if(nNewChannel == -3) // list mode changed
			{ 
				nNewChannel = bouquetList->exec();
				goto _repeat;
			}
			else if(nNewChannel == -4) // list edited
			{
				if(old_b_id < 0) 
					old_b_id = old_b;

				CZapit::getInstance()->saveBouquets();
			}

			return messages_return::handled;
		}
	}
	else if (msg == CRCInput::RC_standby ) 
	{
		if (data == 0) 
		{
			neutrino_msg_t new_msg;

			gettimeofday(&standby_pressed_at, NULL);

			if ((mode != mode_standby) && (g_settings.shutdown_real)) 
			{
				new_msg = NeutrinoMessages::SHUTDOWN;
			}
			else 
			{
				new_msg = (mode == mode_standby) ? NeutrinoMessages::STANDBY_OFF : NeutrinoMessages::STANDBY_ON;
				
				if ((g_settings.shutdown_real_rcdelay)) 
				{
					neutrino_msg_t      lmsg;
					neutrino_msg_data_t ldata;
					struct timeval      endtime;
					time_t              seconds;

					int timeout = 0;
					int timeout1 = 0;

					sscanf(g_settings.repeat_blocker, "%d", &timeout);
					sscanf(g_settings.repeat_genericblocker, "%d", &timeout1);

					if (timeout1 > timeout)
						timeout = timeout1;

					timeout += 500;

					while(true) 
					{
						g_RCInput->getMsg_ms(&lmsg, &ldata, timeout);
						
						if (lmsg == CRCInput::RC_timeout)
							break;

						gettimeofday(&endtime, NULL);
						seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
						
						if (endtime.tv_usec < standby_pressed_at.tv_usec)
							seconds--;
						
						if (seconds >= 1) 
						{
							if ( lmsg == CRCInput::RC_standby )
								new_msg = NeutrinoMessages::SHUTDOWN;
							break;
						}
					}
				}
			}
			
			g_RCInput->postMsg(new_msg);
			return messages_return::cancel_all | messages_return::handled;
		}
		
		return messages_return::handled;
	}
	else if ( (msg == CRCInput::RC_plus) || (msg == CRCInput::RC_minus) )
	{
		setVolume(msg, (mode != mode_scart));
		return messages_return::handled;
	}
	else if( msg == CRCInput::RC_spkr ) 
	{
		if( mode == mode_standby ) 
		{
			//switch lcd off/on
			CLCD::getInstance()->togglePower();
		}
		else 
		{
			//mute
			audioMute( !current_muted, true);
		}
		
		return messages_return::handled;
	}	
	else if( msg == NeutrinoMessages::EVT_SERVICESCHANGED ) //reinitChannels / bqedit
	{
		channelsInit();

		channelList->adjustToChannelID(CZapit::getInstance()->getCurrentChannelID());
		
		if(old_b_id >= 0) 
		{
			bouquetList->activateBouquet(old_b_id);
			old_b_id = -1;
			g_RCInput->postMsg(CRCInput::RC_ok);
		}
	}
	else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED ) // reloadCurrentServices / scan
	{
		channelsInit();

		channelList->adjustToChannelID(CZapit::getInstance()->getCurrentChannelID());

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_RECORDMODE) 
	{
		// sent by rcinput, then got msg from zapit about record activated/deactivated
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: recordmode %s\n", ( data ) ? "on" : "off" );
		
		if(!recordingstatus && (!data)) 
		{
			if( mode == mode_standby )
			{
				// set standby
				CZapit::getInstance()->setStandby(true);				
			}
		}
		
		recordingstatus = data;
		
		if( ( !g_InfoViewer->is_visible ) && data && !autoshift)
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR);

		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::RECORD_START) 
	{
		if(autoshift) 
		{
			stopAutoRecord();
		}

		nextRecordingInfo = (CTimerd::EventInfo *)data;
		
		startNextRecording();

		return messages_return::handled | messages_return::cancel_all;
	}
	else if( msg == NeutrinoMessages::RECORD_STOP) 
	{
		if(((CTimerd::RecordingStopInfo*)data)->eventID == recording_id)
		{ 
			
			if (CRecord::getInstance()->getDeviceState() == CRecord::CMD_RECORD_RECORD)
			{
				CRecord::getInstance()->Stop();
					
				recordingstatus = 0;
				autoshift = 0;
				timeshiftstatus = 0;
			}

			startNextRecording();

			if ( recordingstatus == 0 ) 
			{
				CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);
			}
		}
		else if(nextRecordingInfo != NULL) 
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID) 
			{
				nextRecordingInfo = NULL;
			}
		}
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_STREAM_START) 
	{
		int fd = (int) data;

		//// FIXME: do we need really this ?
		bool alive = recordingstatus || CStreamManager::getInstance()->StreamStatus();
		
		if ((mode == mode_standby) && !alive) 
		{
			CZapit::getInstance()->setStandby(false);
		}
		
		if (g_Radiotext)
			g_Radiotext->setPid(0);

		if (!CStreamManager::getInstance()->AddClient(fd)) 
		{
			close(fd);
			g_RCInput->postMsg(NeutrinoMessages::EVT_STREAM_STOP);
		}

		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_STREAM_STOP) 
	{
		//
		bool alive = recordingstatus || CStreamManager::getInstance()->StreamStatus();
		
		if ((mode == mode_standby) && !alive) 
		{
			CZapit::getInstance()->setStandby(true);
			CSectionsd::getInstance()->pauseScanning(true);

		}
	
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_PMT_CHANGED) 
	{
		// FIXME:
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ZAPTO ) 
	{
		CTimerd::EventInfo * eventinfo;
		eventinfo = (CTimerd::EventInfo *) data;
		
		if(recordingstatus == 0) 
		{
			bool isTVMode = CZapit::getInstance()->isChannelTVChannel(eventinfo->channel_id);
			bool isRadioMode = CZapit::getInstance()->isChannelRadioChannel(eventinfo->channel_id);
			bool isWEBTVMode = CZapit::getInstance()->isChannelWEBTVChannel(eventinfo->channel_id);

			dvbsub_stop();

			if ((isRadioMode) && (mode != mode_radio)) 
			{
				radioMode(false);
			}
			else if ( (isTVMode || isWEBTVMode) && (mode != mode_tv)) 
			{
				tvMode(false);
			}
			
			channelList->zapToChannelID(eventinfo->channel_id);
		}
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_ZAPTO) 
	{
		if( mode == mode_standby ) 
		{
			standbyMode( false );
		}
		
		if( mode != mode_scart ) 
		{
			std::string name = _("Zapto timer in one minute");

			CTimerd::TimerList tmpTimerList;

			tmpTimerList.clear();
			CTimerd::getInstance()->getTimerList( tmpTimerList );

			if(tmpTimerList.size() > 0) 
			{
				sort( tmpTimerList.begin(), tmpTimerList.end() );

				CTimerd::timerEvent &timer = tmpTimerList[0];

				name += "\n";

				std::string zAddData = CZapit::getInstance()->getChannelName( timer.channel_id ); // UTF-8
				
				if( zAddData.empty()) 
				{
					zAddData = _("Program unknown");
				}

				if(timer.epgID != 0) 
				{
					CEPGData epgdata;
					zAddData += " :\n";
					
					if (CSectionsd::getInstance()->getEPGid(timer.epgID, timer.epg_starttime, &epgdata)) 
					{
						zAddData += epgdata.title;
					}
					else if(strlen(timer.epgTitle)!=0) 
					{
						zAddData += timer.epgTitle;
					}
				}
				else if(strlen(timer.epgTitle)!=0) 
				{
					zAddData += timer.epgTitle;
				}

				name += zAddData;
			}
			HintBox(_("Information"), _(name.c_str()), HINTBOX_WIDTH, -1, NEUTRINO_ICON_INFO, CComponent::BORDER_ALL);
		}

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_RECORD) 
	{
		char * lrecDir = ((CTimerd::EventInfo*)data)->recordingDir;

		// ether-wake
		for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++) 
		{
			if (strcmp(g_settings.network_nfs_local_dir[i], lrecDir) == 0) 
			{
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: waking up %s (%s)\n",g_settings.network_nfs_ip[i].c_str(),lrecDir);
					
				std::string command = "etherwake ";
				command += g_settings.network_nfs_mac[i];

				if(system(command.c_str()) != 0)
					perror("etherwake failed");
				break;
			}
		}

		//stop autoshift
		if(autoshift) 
		{
			stopAutoRecord();
		}
		
		if( mode != mode_scart )
			HintBox(_("Information"), _("Recording starts in a few minutes"), HINTBOX_WIDTH, -1, NEUTRINO_ICON_INFO, CComponent::BORDER_ALL);
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER) 
	{
		if( mode != mode_scart )
			HintBox(_("Information"), _("Sleeptimer in 1 min"), HINTBOX_WIDTH, -1, NEUTRINO_ICON_INFO, CComponent::BORDER_ALL);
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::SLEEPTIMER) 
	{
		if(g_settings.shutdown_real)
			exitRun(SHUTDOWN);
		else
			standbyMode( true );
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_ON ) 
	{
		if( mode != mode_standby ) 
		{
			standbyMode( true );
		}
		
		g_RCInput->clearRCMsg();
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_OFF ) 
	{
		if( mode == mode_standby ) 
		{
			standbyMode( false );
		}
		
		g_RCInput->clearRCMsg();
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN ) 
	{
		if( mode != mode_scart )
			skipShutdownTimer = (MessageBox(_("Information"), _("Box will shutdown in 1 min.\nCancel Sutdown ?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, MESSAGEBOX_WIDTH, 5, false, CComponent::BORDER_ALL) == CMessageBox::mbrYes);
	}
	else if( msg == NeutrinoMessages::SHUTDOWN ) 
	{
		if(CStreamManager::getInstance()->StreamStatus())
			skipShutdownTimer = true;
			
		if(!skipShutdownTimer) 
		{
			exitRun(SHUTDOWN);
		}
		else 
		{
			skipShutdownTimer = false;
		}
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REBOOT ) 
	{
		if(!skipShutdownTimer) 
		{
			exitRun(REBOOT);
		}
		else 
		{
			skipShutdownTimer = false;
		}
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::RESTART ) 
	{
		if(!skipShutdownTimer) 
		{
			exitRun(RESTART);
		}
		else 
		{
			skipShutdownTimer = false;
		}
		
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_POPUP ) 
	{
		if (mode != mode_scart)
			HintBox(_("Information"), (char *)data); // UTF-8
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_EXTMSG) 
	{
		if (mode != mode_scart)
			MessageBox(_("Information"), (char *)data, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REMIND) 
	{
		std::string text = (char*)data;
		std::string::size_type pos;
		
		while((pos = text.find('/')) != std::string::npos)
		{
			text[pos] = '\n';
		}
		
		if( mode != mode_scart )
			MessageBox(_("Reminder"), text.c_str(), CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::LOCK_RC) 
	{
		this->rcLock->exec(NULL, CRCLock::NO_USER_INPUT);

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::CHANGEMODE ) 
	{
		if((data & mode_mask) != mode_radio)
		{		  
			if (g_Radiotext)
			{
				delete g_Radiotext;
				g_Radiotext = NULL;
			}
		}

		if((data & mode_mask) == mode_radio) 
		{
			if( mode != mode_radio ) 
			{
				if((data & norezap) == norezap)
					radioMode(false);
				else
					radioMode(true);			
			}
		}
		
		if((data & mode_mask) == mode_tv) 
		{
			if( mode != mode_tv ) 
			{
				if((data & norezap) == norezap)
					tvMode(false);
				else
					tvMode(true);
			}
		}
		
		if((data &mode_mask) == mode_standby) 
		{
			if(mode != mode_standby)
				standbyMode( true );
		}
		
		if((data &mode_mask) == mode_audio) 
		{
			lastMode = mode;
			mode = mode_audio;
		}
		
		if((data &mode_mask) == mode_pic) 
		{
			lastMode = mode;
			mode = mode_pic;
		}
		
		if((data &mode_mask) == mode_ts) 
		{
			lastMode = mode;
			mode = mode_ts;
		}
	}	
	else if( msg == NeutrinoMessages::RECORD_ON ) 
	{
		if( mode != mode_scart ) 
		{
			scartMode( true );
		}
		else
		{
			CLCD::getInstance()->setMode(CLCD::MODE_SCART);
		}
	}	
	else if( msg == NeutrinoMessages::RECORD_OFF ) 
	{
		if( mode == mode_scart ) 
		{
			scartMode( false );
		}
	}	
	else if (msg == NeutrinoMessages::EVT_START_PLUGIN) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: start Plugin: %s\n", (char *)data);
		
		g_PluginList->startPlugin((const char *)data);
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_SERVICES_UPD) // sdtthread
	{
		channelsInit();

		channelList->adjustToChannelID(CZapit::getInstance()->getCurrentChannelID());
		
		return messages_return::handled;
	}	

	return messages_return::unhandled;
}

// real run
void CNeutrinoApp::realRun(void)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::realRun:\n");

	// clear msg 
	g_RCInput->clearRCMsg();

	// if start to standby
	if(g_settings.power_standby)
	{
		standbyMode(true);
	}

	// main run loop
	while( true ) 
	{
		g_RCInput->getMsg(&msg, &data, 1);	// 1 secs..	

		// mode TV/Radio
		if( (mode == mode_tv) || (mode == mode_radio) ) 
		{
			if(msg == NeutrinoMessages::SHOW_EPG) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				g_EpgData->show(CZapit::getInstance()->getCurrentChannelID());

				startSubtitles();
			}
			else if(msg == CRCInput::RC_epg) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				g_EventList->show(CZapit::getInstance()->getCurrentChannelID(), CZapit::getInstance()->getCurrentChannelName());
				
				// restore mute symbol
				audioMute(current_muted, true);

				startSubtitles();
			}
			else if( msg == CRCInput::RC_text ) 
			{
				if (!IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
				{
					//
					if(g_InfoViewer->is_visible)
						g_InfoViewer->killTitle();

					stopSubtitles();
					
					tuxtx_stop_subtitle();

					tuxtx_main(g_RemoteControl->current_PIDs.otherPIDs.vtxtpid, 0, false);
					
					// restore mute symbol
					audioMute(current_muted, true);

					startSubtitles();
				}
			}			
			else if(msg == (neutrino_msg_t)g_settings.key_timerlist) //timerlist
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				CTimerList* Timerlist = new CTimerList;
				Timerlist->exec(NULL, "");
				delete Timerlist;
				Timerlist = NULL;
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}		
			else if( msg == CRCInput::RC_setup ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				//
				mainMenu();

				// restore mute symbol
				audioMute(current_muted, true);

				startSubtitles();
			}
			else if( msg == (neutrino_msg_t) g_settings.key_tvradio_mode)
			{
				if( mode == mode_tv )
					radioMode();
				else if( mode == mode_radio )
					tvMode();
			}
			else if(( msg == (neutrino_msg_t) g_settings.key_quickzap_up ) || ( msg == (neutrino_msg_t) g_settings.key_quickzap_down ))
			{
				quickZap(msg);
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_up ) 
			{
			   	if(g_RemoteControl->subChannels.size() > 0) 
				{
					stopSubtitles();
					g_RemoteControl->subChannelUp();
					g_InfoViewer->showSubchan(); 
			    	} 
			    	else if(g_settings.virtual_zap_mode) 
				{
					showInfo(CZapit::getInstance()->getCurrentChannel());	
				}
				else
				{
					quickZap(msg);
				}
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_down ) 
			{
			   	if(g_RemoteControl->subChannels.size()> 0) 
				{
					stopSubtitles();
					g_RemoteControl->subChannelDown();
					g_InfoViewer->showSubchan();
			    	} 
			    	else if(g_settings.virtual_zap_mode) 
				{
					showInfo(CZapit::getInstance()->getCurrentChannel());	
				}
				else
				{
					quickZap(msg);
				}
			}
			// in case key_subchannel_up/down redefined
			else if((msg == CRCInput::RC_left || msg == CRCInput::RC_right)) 
			{
				showInfo(CZapit::getInstance()->getCurrentChannel());
			}
			else if( msg == (neutrino_msg_t) g_settings.key_zaphistory) 
			{
				stopSubtitles();
				
				// Zap-History "Bouquet"
				int res = numericZap( msg );

				startSubtitles(res < 0);
			}
			else if(msg == (neutrino_msg_t) g_settings.key_lastchannel) 
			{
				stopSubtitles();
				
				// Quick Zap
				int res = numericZap( msg );

				startSubtitles(res < 0);
			}
			else if(msg == (neutrino_msg_t) g_settings.key_pip) // current TP
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				// first step show channels from the same TP
				int res = numericZap( msg );
				
				// restore mute symbol
				audioMute(current_muted, true);
				
				//
				startSubtitles();
			}
			else if ( CRCInput::isNumeric(msg) && g_RemoteControl->director_mode ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				g_RemoteControl->setSubChannel(CRCInput::getNumericValue(msg));
				
				g_InfoViewer->showSubchan();
				
				// restore mute symbol
				audioMute(current_muted, true);
				
				//
				startSubtitles();
			}
			else if (CRCInput::isNumeric(msg)) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				//
				numericZap( msg );
				
				// restore mute symbol
				audioMute(current_muted, true);
				
				//
				startSubtitles();
			}			
			else if (CRCInput::isNumeric(msg) && (mode == mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL && g_Radiotext->Rass_Show) ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				g_Radiotext->RassImage(0, g_RCInput->getNumericValue(msg), true);
			}			
			else if(msg == CRCInput::RC_pause) // start timeshift recording
			{
				if (recDir != NULL)
				{
					if(g_RemoteControl->is_video_started || IS_WEBTV(CZapit::getInstance()->getCurrentChannelID())) 
					{		
						// permanenttimeshift / already recording
						if(recordingstatus) 
						{
							timeshiftstatus = recordingstatus;
						} 
						else
						{
							// timeshift
							recordingstatus = 1;	
							timeshiftstatus = recordingstatus;
							doGuiRecord(timeshiftDir, true);
						}

						// freeze audio/video
						if (timeshiftstatus)
						{
							CZapit::getInstance()->pausePlayBack();
						}
					}
				}
			}
			else if( ((msg == CRCInput::RC_play) && timeshiftstatus)) // play timeshift
			{		
				if(g_RemoteControl->is_video_started || IS_WEBTV(CZapit::getInstance()->getCurrentChannelID())) 
				{
					CMoviePlayerGui tmpMoviePlayerGui;
					CMovieInfo cMovieInfo;
					MI_MOVIE_INFO mfile;

					//
					char fname[1024];
					int cnt = 10*1000000;

					while (!strlen(rec_filename)) 
					{
						usleep(1000);
						cnt -= 1000;

						if (!cnt)
							break;
					}

					if (!strlen(rec_filename))
						return;

					sprintf(fname, "%s.ts", rec_filename);

					//
					if (IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))						
					{
						CZapit::getInstance()->lockPlayBack();
						playback->Close(); // not needed???
						playback->Open();
						playback->Start(fname);
					}
					else if (!playback->playing)
					{
						CZapit::getInstance()->lockPlayBack();
						playback->Close(); // not needed???
						playback->Open();
						playback->Start(fname);
					}
				}
			}
			else if( (msg == CRCInput::RC_record || msg == CRCInput::RC_stop) ) 
			{
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::realRun\n");
				
				if(autoshift) 
				{
					stopAutoRecord();
				}

				// stop record if recording
				if( recordingstatus ) 
				{
					if(MessageBox(_("Information"), _("You really want to to stop record ?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == CMessageBox::mbrYes)
					{
						if (recording_id)
							CTimerd::getInstance()->stopTimerEvent(recording_id);
						else
							CRecord::getInstance()->Stop();
							
						recordingstatus = 0;
						timeshiftstatus = 0;
						CLCD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );
						
						//
						if (playback->playing)
						{
							playback->Close(); // not needed???
						}
						
						CZapit::getInstance()->unlockPlayBack();
					}
				} 
				// start record
				else if(msg != CRCInput::RC_stop )
				{
					recordingstatus = 1;
					timeshiftstatus = 0;
					doGuiRecord( g_settings.network_nfs_recordingdir, true );
				}
			}
			else if( msg == CRCInput::RC_red ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				//
				stopSubtitles();
				
				//	
				CEPGMenuHandler* redMenu = new CEPGMenuHandler();
							
				redMenu->exec(NULL, "");
							
				delete redMenu;
				redMenu = NULL;
				
				// restore mute symbol
				audioMute(current_muted, true);
				
				//
				startSubtitles();
			}
			else if( ( msg == CRCInput::RC_green) || ( msg == CRCInput::RC_audio) )
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();
					
				stopSubtitles();

				if (IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
				{
					CAVPIDSelectWidget * AVSelectHandler = new CAVPIDSelectWidget();
					AVSelectHandler->exec(NULL, "");
							
					delete AVSelectHandler;
					AVSelectHandler = NULL;
				}
				else
				{
					// audio handler
					CAudioSelectMenuHandler* audioSelectMenuHandler = new CAudioSelectMenuHandler();

					audioSelectMenuHandler->exec(NULL, "");
							
					delete audioSelectMenuHandler;
					audioSelectMenuHandler = NULL;
				}
				
				startSubtitles();
				// restore mute symbol
				audioMute(current_muted, true);
			}
			else if( (msg == CRCInput::RC_yellow || msg == CRCInput::RC_multifeed) )
			{ 
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				// select NVODs
				if (!IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
				{
					selectNVOD();
				}

				// restore mute symbol
				audioMute(current_muted, true);
				
				startSubtitles();
			}
			else if( msg == CRCInput::RC_blue ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				// features
				showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
				
				// restore mute symbol
				audioMute(current_muted, true);

				startSubtitles();
			}
#if defined (ENABLE_FUNCTIONKEYS)			
			else if( msg == CRCInput::RC_f1 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F1);
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}
			else if( msg == CRCInput::RC_f2 )
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F2);
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}
			else if( msg == CRCInput::RC_f3 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();
 
				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F3);
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}
			else if( msg == CRCInput::RC_f4 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F4);
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}
#endif			
			else if( msg == CRCInput::RC_dvbsub )
			{
				if (!IS_WEBTV(CZapit::getInstance()->getCurrentChannelID()))
				{
					if(g_InfoViewer->is_visible)
						g_InfoViewer->killTitle();

					stopSubtitles();
					
					// show list only if we have subs
					if(CZapit::getInstance()->getCurrentChannel())
					{
						if(CZapit::getInstance()->getCurrentChannel()->getSubtitleCount() > 0)
						{
							CDVBSubSelectMenuHandler tmpDVBSubSelectMenuHandler;
							tmpDVBSubSelectMenuHandler.exec(NULL, "");
						}
					}
					
					
					// restore mute symbol
					audioMute(current_muted, true);
					startSubtitles();
				}
			}
			else if( msg == (neutrino_msg_t)g_settings.key_audioplayer ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("audioplayer");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}
			else if( msg == (neutrino_msg_t)g_settings.key_inetradio ) 	// internet radio
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();
	  
				stopSubtitles();
				g_PluginList->startPlugin("internetradio");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();	
			}			
			else if( msg == (neutrino_msg_t)g_settings.key_movieplayer )	// recordsbrowser
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("movieplayer");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();			
			}
			else if( msg == (neutrino_msg_t)g_settings.key_moviebrowser)		// mediaportal
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("moviebrowser");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();	
			}
			else if( msg == (neutrino_msg_t)g_settings.key_pvr)		// mediaportal
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("mediaportal");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();	
			}
			else if( msg == (neutrino_msg_t)g_settings.key_pictureviewer ) 	// picture viewer
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("picviewer");
				
				// restore mute symbol
				audioMute(current_muted, true);
				startSubtitles();
			}			
			else if((msg == CRCInput::RC_info) || ( msg == NeutrinoMessages::SHOW_INFOBAR ))
			{
				bool show_info = ((msg != NeutrinoMessages::SHOW_INFOBAR) || (g_InfoViewer->is_visible || g_settings.timing_infobar != 0));
					
				// turn on LCD display
				CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
					
				if(show_info) 
				{
					showInfo(CZapit::getInstance()->getCurrentChannel());
				}
			}
			else 
			{
				if ( msg == CRCInput::RC_home )
				{ 
					if(g_InfoViewer->is_visible)
						g_InfoViewer->killTitle();

  					CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
				}

				//
				handleMsg(msg, data);
			}
		}
		else //other modes
		{
			if( msg == CRCInput::RC_home ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				if( mode == mode_scart ) 
				{
					//wenn VCR Aufnahme dann stoppen
					if (CRecord::getInstance()->getDeviceState() == CRecord::CMD_RECORD_RECORD)
					{
						CRecord::getInstance()->Stop();
						recordingstatus = 0;
						startNextRecording();
					}

					// Scart-Mode verlassen
					scartMode( false );
				}
			}
			else 
			{
				//
				handleMsg(msg, data);
			}
		}
	}
}

//
int CNeutrinoApp::run(int argc, char **argv)
{
	//	
	global_argv = new char *[argc + 1];
	
        for (int i = 0; i < argc; i++)
                global_argv[i] = argv[i];
	
        global_argv[argc] = NULL;
        
        //
        setupFrameBuffer();
	
	// font
	font.name = NULL;
	font.filename = NULL;

	// load settings
	int loadSettingsErg = loadSetup(NEUTRINO_SETTINGS_FILE);
	
	// set debug level
	setDebugLevel(g_settings.debug_level);
	
	// init iso639
	initialize_iso639_map();

	// load locale
	g_Locale->loadLocale(Lang2I18N(g_settings.language).c_str());

	// set icons/buttons/hints path
	frameBuffer->setIconBasePath(g_settings.icons_dir);
	frameBuffer->setButtonBasePath(g_settings.buttons_dir);
	frameBuffer->setHintBasePath(g_settings.hints_dir);
	frameBuffer->setSpinnerBasePath(g_settings.spinner_dir);

	// setup fonts
	setupFonts(g_settings.font_file);
	
	// setup color
	setupColor();
	
	// init remote control defore CLCD otherwise CLCD crashes
	g_RemoteControl = new CRemoteControl();

	// init CLCD display
	CLCD::getInstance()->init(font.filename, font.name);
	
	// CLCD clear	
	CLCD::getInstance()->Clear();
		
	// show startup msg on CLCD
	CLCD::getInstance()->showTextScreen("NeutrinoNG", "", CLCD::EPGMODE_CHANNEL, true, true); // always centered

	// rcinput
	g_RCInput = new CRCInput();
	g_RCInput->setRepeat(atoi(g_settings.repeat_blocker), atoi(g_settings.repeat_genericblocker));

	// playback
	playback = new cPlayback();

	// plugins
	g_PluginList = new CPlugins();
	g_PluginList->setPluginDir(PLUGINDIR);
	g_PluginList->loadPlugins();
	
	// load selected skin
	loadSkin(g_settings.preferred_skin);
	
	current_volume = g_settings.current_volume;

	// init video / audio decoder	
	videoDecoder = new cVideo();
	audioDecoder = new cAudio();
		
	if(videoDecoder)
	{
		// set video system
		videoDecoder->SetVideoSystem(g_settings.video_Mode);	

		// aspect ratio
		videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
	
		// wss
		videoDecoder->SetWideScreen(g_settings.wss_mode);
	
		// avsync
		videoDecoder->SetSyncMode(g_settings.avsync);
	}		
		
	if(audioDecoder)
	{
		// audio volume (default)
		audioDecoder->setVolume(g_settings.current_volume, g_settings.current_volume);
		
		// sync mode
		audioDecoder->SetSyncMode(g_settings.avsync);

		// hdmi_dd
		audioDecoder->SetHdmiDD(g_settings.hdmi_dd);

		// ac3 delay
		audioDecoder->setHwAC3Delay(g_settings.ac3_delay);

		// pcm delay 
		audioDecoder->setHwPCMDelay(g_settings.pcm_delay);
	}
	
	// cec
#if !defined (__sh__)
	hdmi_cec::getInstance()->setCECAutoStandby(g_settings.hdmi_cec_standby == 1);
	hdmi_cec::getInstance()->getAudioDestination();
	hdmi_cec::getInstance()->setCECMode((VIDEO_HDMI_CEC_MODE)g_settings.hdmi_cec_mode);
#endif
	
	// zapit
	CZapit::getInstance()->Start();
	
	// sectionsd
	CSectionsd::getInstance()->Start();

	// timerd
	CTimerd::getInstance()->Start();

	// nhttpd
	Cyhttpd::getInstance()->Start();

	// streamts
	CStreamManager::getInstance()->Start();
	
	// dvbsub thread
	dvbsub_init();	

	//
	CLCD::getInstance()->showVolume(g_settings.current_volume);
	CLCD::getInstance()->setMuted(current_muted);
	
	// epg view
	g_EpgData = new CEpgData();
	
	// channel infoviewer
	g_InfoViewer = new CInfoViewer();
	
	// event list
	g_EventList = new EventList();

	// Ci Cammenu handler
#if defined (ENABLE_CI)	
	g_CamHandler = new CCAMMenuHandler();
	g_CamHandler->init();	
#endif
	
	// init network at startup
	CNetworkSettings::getInstance()->readNetworkSettings(g_settings.ifname);

	if (CNetworkSettings::getInstance()->network_automatic_start == 1)
		CNetworkSettings::getInstance()->setNetwork();	
	
	// mount shares
	CFSMounter::automount();

	// assuming that mdev/fstab has mounted devices
	CHDDDestExec * hdd = new CHDDDestExec();
	hdd->exec(NULL, "");
	delete hdd;
	hdd = NULL;
	
	// init rclock
	rcLock = new CRCLock();

	// LCD
	CLCD::getInstance()->setPower(g_settings.lcd_power);
	CLCD::getInstance()->setlcdparameter();
	
	// start assistant
	if(loadSettingsErg) 
	{
		int tvmode = CZapit::getInstance()->getMode();

		if (tvmode == CZapit::MODE_TV)
			mode = mode_tv;
		else if (tvmode == CZapit::MODE_RADIO)
			mode = mode_radio;

		// startup pic
		frameBuffer->useBackground(false);
		frameBuffer->loadBackgroundPic("start.jpg");	
		frameBuffer->blit();
	
		// setup languages
		CLanguageSettings* languageSettings = new CLanguageSettings(true);
		int ret = languageSettings->exec(NULL, "");
	
		// video setup wizard
		if(ret != RETURN_EXIT_ALL)
		{
			CVideoSettings videoSettings;
			videoSettings.exec(NULL, "");
		}
		
		 // audio setup wizard
		if(ret != RETURN_EXIT_ALL)
		{
			CAudioSettings audioSettings;
			audioSettings.exec(NULL, "");
		}		

		// setup color
		if(ret != RETURN_EXIT_ALL)
		{
			COSDSettings colorSettings;
			colorSettings.exec(NULL, "");
		}

		// setup timezone
		if(ret != RETURN_EXIT_ALL)
		{
			CMenuOptionStringChooser * tzSelect = NULL;
			xmlDocPtr parser;

			parser = parseXmlFile("/etc/timezone.xml");
			if (parser != NULL) 
			{	
				tzSelect = new CMenuOptionStringChooser(_("Time Zone"), g_settings.timezone, true, new CTZChangeNotifier(), CRCInput::RC_nokey, "", true);

				xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
				bool found = false;

				while (search) 
				{
					if (!strcmp(xmlGetName(search), "zone")) 
					{
						std::string name = xmlGetAttribute(search, (char *) "name");
						
						tzSelect->addOption(name.c_str());
						found = true;
					}
					search = search->xmlNextNode;
				}

				if(found)
					tzSelect->exec(this);
				else 
				{
					delete tzSelect;
					tzSelect = NULL;
				}	
				xmlFreeDoc(parser);
			}
		}

		// setup network
		if(ret != RETURN_EXIT_ALL)
		{
			CNetworkSettings networkSettings;
			networkSettings.exec(NULL, "");
		}
		
		// recordingsettings
		if(ret != RETURN_EXIT_ALL)
		{
			CRecordingSettings recordingSettings;
			recordingSettings.exec(NULL, "");
		}
		
		// movieplayer settings
		if(ret != RETURN_EXIT_ALL)
		{
			CMoviePlayerSettings moviePlayerSettings;
			moviePlayerSettings.exec(NULL, "");
		}
		
		// audioplayersettings
		if(ret != RETURN_EXIT_ALL)
		{
			CAudioPlayerSettings audioPlayerSettings;
			audioPlayerSettings.exec(NULL, "");
		}
		
		// picviewersettings
		if(ret != RETURN_EXIT_ALL)
		{
			CPictureViewerSettings picViewerSettings;
			picViewerSettings.exec(NULL, "");
		}
		
		// keysettings
		if(ret != RETURN_EXIT_ALL)
		{
			CRemoteControlSettings bindSettings;
			bindSettings.exec(NULL, "");
		}
		
		// service settings
		if(ret != RETURN_EXIT_ALL)
		{
			CServiceMenu service;
			service.exec(NULL, "");
		}

		dprintf(DEBUG_INFO, "config file or options missing\n");

		HintBox(_("Information"), loadSettingsErg ==  1 ? _("No neutrino-settings found, Using defaults.") : _("The neutrino-settings have been updated.\nNew Options will be set to default."));
		
		configfile.setModifiedFlag(true);

		saveSetup(NEUTRINO_SETTINGS_FILE);
	}
	
	// zapper
	initZapper();
	
	// init muteIcon
	int dx = 32;
	int dy = 32;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_MUTE, &dx, &dy);

	int x = g_settings.screen_EndX - 10 - dx;
	int y = g_settings.screen_StartY + 10;

	muteIcon = new CCIcon(x, y, dx, dy);
	
	muteIcon->setIcon(NEUTRINO_ICON_BUTTON_MUTE);
	
	// audio mute
	audioMute(current_muted, true);		
	
	// init shutdown count
	SHTDCNT::getInstance()->init();
	
	// getMyGeoLocation / weather
	CWeather::getInstance()->getMyGeoLocation();
	
	
	if (g_settings.lcd_weather || g_settings.show_weather)
	{
		CWeather::getInstance()->GetWeatherDetails();
	}

	// realRun loop ;-)
	realRun();

	// exitRun
	exitRun(SHUTDOWN);	

	// never reached
	return 0;
}

// signal handler
void sighandler(int signum)
{
	signal(signum, SIG_IGN);
	
        switch (signum) 
	{
		case SIGTERM:
		case SIGINT:
			// stop nhttpd		
			Cyhttpd::getInstance()->Stop();
			
			// stop streamts	
			CStreamManager::getInstance()->Stop();

			// stop timerd	  
			CTimerd::getInstance()->Stop();		

			// stop sectionsd
			CSectionsd::getInstance()->Stop();

			// zapit stop	
			CZapit::getInstance()->Stop();
			
			_exit(0);
			
		  default:
			break;
        }
}

//// main function
int main(int argc, char *argv[])
{
	// build date
	printf(">>> neutrinoNG v %s (compiled %s %s) <<<\n", PACKAGE_VERSION, __DATE__, __TIME__);	

	// sighandler
        signal(SIGTERM, sighandler);
        signal(SIGINT, sighandler);
        signal(SIGHUP, SIG_IGN);
	signal(SIGHUP, sighandler);
	signal(SIGPIPE, SIG_IGN);
	
	// init locale
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE_NAME, "UTF8");
	textdomain(PACKAGE_NAME);
	
	// set localtime
	tzset();
	
	// init globals
	initGlobals();

#if ENABLE_GSTREAMER
	gst_init(NULL, NULL);
#endif

#ifdef USE_OPENGL
	ao_initialize();
#endif

	// set python path
#if ENABLE_PYTHON
	setenv("PYTHONPATH", DATADIR "/python", 0);
#endif

	return CNeutrinoApp::getInstance()->run(argc, argv);
}

