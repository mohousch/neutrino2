/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrino2.cpp 19.11.2022 mohousch Exp $

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
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/stream2file.h>
#include <driver/vcrcontrol.h>
#include <driver/shutdown_count.h>
#include <driver/color.h>
#include <driver/streamts.h>

#include <gui/epgplus.h>
#include <gui/streaminfo.h>

#include <gui/widget/colorchooser.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/keychooser.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/mountchooser.h>

#include <gui/bedit/bouqueteditor_bouquets.h>
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
#if !defined (PLATFORM_COOLSTREAM)
#include <gui/cam_menu.h>
#endif
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
#include <gui/vfdcontroler.h>
#include <gui/dvbsub_select.h>
#include <gui/movieinfo.h>
#include <gui/themes.h>
#include <gui/epg_menu.h>

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>
#include <system/helpers.h>

#include <timerd/timerd.h>

// zapit includes
#include <zapit/frontend_c.h>
#include <zapit/getservices.h>
#include <zapit/satconfig.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>

// libdvbapi
#include <video_cs.h>
#include <audio_cs.h>

// libcoolstream
#if defined (PLATFORM_COOLSTREAM)
#include <cs_api.h>
#endif

#include <libdvbci/dvb-ci.h>

#if defined ENABLE_GSTREAMER
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#endif

#include <playback_cs.h>

#if ENABLE_LUA
#include <interfaces/lua/neutrino2_lua.h>
#endif

#include <nhttpd/yhttpd.h>


//
cPlayback* playback = NULL;

extern char rec_filename[1024];				// defined in stream2file.cpp

extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern tallchans allchans;				// defined in zapit.cpp
extern CBouquetManager * g_bouquetManager;		// defined in zapit.cpp

int old_b_id = -1;

// record and timeshift
bool autoshift = false;
uint32_t shift_timer;
uint32_t scrambled_timer;
char recDir[255];
char timeshiftDir[255];

// tuxtxt
//extern int  tuxtxt_init();
//extern void tuxtxt_start(int tpid, int source );
extern int  tuxtxt_stop();
extern void tuxtxt_close();
extern void tuxtx_pause_subtitle(bool pause, int source);
extern void tuxtx_stop_subtitle();
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern int tuxtx_main(int pid, int page, int source );

// dvbsub
//extern int dvbsub_initialise();
extern int dvbsub_init( /*int source*/);
extern int dvbsub_stop();
extern int dvbsub_close();
extern int dvbsub_start(int pid);
extern int dvbsub_pause();
extern int dvbsub_getpid();
extern void dvbsub_setpid(int pid);
extern int dvbsub_terminate();

// volume bar
static CProgressBar * g_volscale;

// zapit
extern int zapit_ready;					//defined in zapit.cpp
extern t_channel_id live_channel_id; 			//defined in zapit.cpp
Zapit_config zapitCfg;
extern CZapitChannel * live_channel;			// defined in zapit.cpp
extern CFrontend * live_fe;

// Audio/Video Decoder
extern cVideo* videoDecoder;		//libcoolstream (video_cs.cpp)
extern cAudio* audioDecoder;		//libcoolstream (audio_cs.cpp)

//
int prev_video_Mode;
int current_volume;
int current_muted;

// bouquets lists
CBouquetList* bouquetList; 				//current bqt list

CBouquetList* TVbouquetList;
CBouquetList* TVsatList;
CBouquetList* TVfavList;
CBouquetList* TVallList;

CBouquetList* RADIObouquetList;
CBouquetList* RADIOsatList;
CBouquetList* RADIOfavList;
CBouquetList* RADIOallList;

//
CEventServer *eventServer = NULL;
CPlugins* g_PluginList;
CRemoteControl* g_RemoteControl;

#if defined (ENABLE_CI)
CCAMMenuHandler* g_CamHandler;
#endif


// timezone for wizard
extern CMenuOptionStringChooser* tzSelect;

bool parentallocked = false;
static char **global_argv;

//user menu
const char *usermenu_button_def[SNeutrinoSettings::BUTTON_MAX] = {
	"blue",
#if defined (ENABLE_FUNCTIONKEYS)
	"f1",
	"f2",
	"f3",
	"f4"
#endif
};

CVCRControl::CDevice * recordingdevice = NULL;

// init globals
static void initGlobals(void)
{
	g_fontRenderer  = NULL;
	g_RCInput       = NULL;
	g_RemoteControl = NULL;
	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;
	g_Locale        = new CLocaleManager;
	eventServer 	= new CEventServer;
	g_PluginList    = NULL;
#if defined (ENABLE_CI)	
	g_CamHandler 	= NULL;
#endif	

	g_Radiotext     = NULL;
}

// CNeutrinoApp - Constructor, initialize g_fontRenderer
CNeutrinoApp::CNeutrinoApp()
: configfile('\t')
{
	standby_pressed_at.tv_sec = 0;

	frameBuffer = CFrameBuffer::getInstance();
	setupFrameBuffer();

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
#if defined (USE_OPENGL) // opengl playback
	playbackstatus = 0;
#endif

	mute_pixbuf = NULL;
	vol_pixbuf = NULL;
}

// CNeutrinoApp - Destructor
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelList)
		delete channelList;
}

// getInstance
CNeutrinoApp * CNeutrinoApp::getInstance()
{
	static CNeutrinoApp * neutrinoApp = NULL;

	if(!neutrinoApp) 
	{
		neutrinoApp = new CNeutrinoApp();
		dprintf(DEBUG_NORMAL, "NeutrinoApp Instance created\n");
	}

	return neutrinoApp;
}

// fonts
#define FONT_STYLE_REGULAR 0
#define FONT_STYLE_BOLD    1
#define FONT_STYLE_ITALIC  2

typedef struct font_sizes
{
        const char* const name;
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
        {_("Menu Title2")         	,  40, FONT_STYLE_REGULAR   , 0},
};

// signal font
const font_sizes_struct signal_font = {_("Signal small"),  14, FONT_STYLE_REGULAR, 1};

// LCD settings
typedef struct lcd_setting_t
{
	const char * const name;
	const unsigned int default_value;
} lcd_setting_struct_t;

const lcd_setting_struct_t lcd_setting[LCD_SETTING_COUNT] =
{
	{"lcd_brightness"       , DEFAULT_LCD_BRIGHTNESS       },
	{"lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS},
	{"lcd_contrast"         , DEFAULT_LCD_CONTRAST         },
	{"lcd_power"            , DEFAULT_LCD_POWER            },
	{"lcd_inverse"          , DEFAULT_LCD_INVERSE          },
	{"lcd_show_volume"      , DEFAULT_LCD_SHOW_VOLUME      },
	{"lcd_autodimm"         , DEFAULT_LCD_AUTODIMM         },
	{"lcd_scroll_text"      , DEFAULT_LCD_SCROLL_TEXT      },
#if ENABLE_LCD
	{"lcd_epgmode"          , DEFAULT_LCD_EPGMODE          },
	{"lcd_epgalign"         , DEFAULT_LCD_EPGALIGN         },
	{"lcd_dump_png"         , DEFAULT_LCD_DUMP_PNG         },
#endif	
};

// loadSetup, load the application-settings
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

	// parentallock check
        std::ifstream checkParentallocked(NEUTRINO_PARENTALLOCKED_FILE);
	if(checkParentallocked) 
	{
	        parentallocked = true;
	        checkParentallocked.close();
	}

	// video
	g_settings.video_Mode = configfile.getInt32("video_Mode", VIDEO_STD_720P50);
	prev_video_Mode = g_settings.video_Mode;
	
	//analog mode
#if defined (PLATFORM_COOLSTREAM)
	g_settings.analog_mode = configfile.getInt32("analog_mode", (int)ANALOG_SD_RGB_SCART); // default RGB
#else
	g_settings.analog_mode = configfile.getInt32("analog_mode", ANALOG_YUV); 	// default yuv
	
	g_settings.hdmi_color_space = configfile.getInt32("hdmi_color_space", HDMI_RGB); //default RGB
#endif	

	//aspect ratio
#if defined (PLATFORM_COOLSTREAM)
	g_settings.video_Ratio = configfile.getInt32("video_Ratio", DISPLAY_AR_16_9);		// 16:9
#else
	g_settings.video_Ratio = configfile.getInt32("video_Ratio", ASPECTRATIO_169);		// 16:9
#endif	
	 
	// policy
#if defined (PLATFORM_COOLSTREAM)
	g_settings.video_Format = configfile.getInt32("video_Format", DISPLAY_AR_MODE_LETTERBOX);
#else
	g_settings.video_Format = configfile.getInt32("video_Format", VIDEOFORMAT_PANSCAN2);
#endif	

	//wss
#if !defined (PLATFORM_COOLSTREAM)	
	g_settings.wss_mode = configfile.getInt32("wss_mode", WSS_AUTO);
#endif	
	
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
#if !defined (PLATFORM_COOLSTREAM)	
	g_settings.hdmi_dd = configfile.getInt32( "hdmi_dd", AC3_DOWNMIX);	// downmix
#endif	
	
	// avsync
#if defined (PLATFORM_COOLSTREAM)
	g_settings.avsync = configfile.getInt32( "avsync", 1);
#else
	g_settings.avsync = configfile.getInt32( "avsync", AVSYNC_ON);
#endif	
	
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
	if (!parentallocked) 
	{
	  	g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
		g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	} 
	else 
	{
	        g_settings.parentallock_prompt = 3;
	        g_settings.parentallock_lockage = 18;
	}
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
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 15);
	g_settings.menu_Head_red = configfile.getInt32( "menu_Head_red", 15);
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 15);
	g_settings.menu_Head_blue = configfile.getInt32( "menu_Head_blue", 15);

	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0);
	g_settings.menu_Head_Text_red = configfile.getInt32( "menu_Head_Text_red", 100);
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 100);
	g_settings.menu_Head_Text_blue = configfile.getInt32( "menu_Head_Text_blue", 100);
	
	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 0);
	g_settings.menu_Content_red = configfile.getInt32( "menu_Content_red", 15);
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 15);
	g_settings.menu_Content_blue = configfile.getInt32( "menu_Content_blue", 15);
	
	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 0);
	g_settings.menu_Content_Text_red = configfile.getInt32( "menu_Content_Text_red", 85);
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 85);
	g_settings.menu_Content_Text_blue = configfile.getInt32( "menu_Content_Text_blue", 85);
	
	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 20);
	g_settings.menu_Content_Selected_red = configfile.getInt32( "menu_Content_Selected_red", 75);
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 75);
	g_settings.menu_Content_Selected_blue = configfile.getInt32( "menu_Content_Selected_blue", 75);
	
	g_settings.menu_Content_Selected_Text_alpha = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0);
	g_settings.menu_Content_Selected_Text_red = configfile.getInt32( "menu_Content_Selected_Text_red", 25);
	g_settings.menu_Content_Selected_Text_green = configfile.getInt32( "menu_Content_Selected_Text_green", 25);
	g_settings.menu_Content_Selected_Text_blue = configfile.getInt32( "menu_Content_Selected_Text_blue", 25);
	
	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 20);
	g_settings.menu_Content_inactive_red = configfile.getInt32( "menu_Content_inactive_red", 25);
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 25);
	g_settings.menu_Content_inactive_blue = configfile.getInt32( "menu_Content_inactive_blue", 25);
	
	g_settings.menu_Content_inactive_Text_alpha = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0);
	g_settings.menu_Content_inactive_Text_red = configfile.getInt32( "menu_Content_inactive_Text_red", 55);
	g_settings.menu_Content_inactive_Text_green = configfile.getInt32( "menu_Content_inactive_Text_green", 55);
	g_settings.menu_Content_inactive_Text_blue = configfile.getInt32( "menu_Content_inactive_Text_blue", 55);

	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 20);
	g_settings.infobar_red = configfile.getInt32( "infobar_red", 25);
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 25);
	g_settings.infobar_blue = configfile.getInt32( "infobar_blue", 25);
	
	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0);
	g_settings.infobar_Text_red = configfile.getInt32( "infobar_Text_red", 100);
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 100);
	g_settings.infobar_Text_blue = configfile.getInt32( "infobar_Text_blue", 100);
		
	g_settings.infobar_colored_events_alpha = configfile.getInt32( "infobar_colored_events_alpha", 0);
	g_settings.infobar_colored_events_red = configfile.getInt32( "infobar_colored_events_red", 95);
	g_settings.infobar_colored_events_green = configfile.getInt32( "infobar_colored_events_green", 70);
	g_settings.infobar_colored_events_blue = configfile.getInt32( "infobar_colored_events_blue", 0);
	
	g_settings.menu_Foot_alpha = configfile.getInt32( "menu_Foot_alpha", 15);
	g_settings.menu_Foot_red = configfile.getInt32( "menu_Foot_red", 15);
	g_settings.menu_Foot_green = configfile.getInt32( "menu_Foot_green", 15);
	g_settings.menu_Foot_blue = configfile.getInt32( "menu_Foot_blue", 15);
		
	g_settings.menu_Foot_Text_alpha = configfile.getInt32( "menu_Foot_Text_alpha", 0);
	g_settings.menu_Foot_Text_red = configfile.getInt32( "menu_Foot_Text_red", 100);
	g_settings.menu_Foot_Text_green = configfile.getInt32( "menu_Foot_Text_green", 100);
	g_settings.menu_Foot_Text_blue = configfile.getInt32( "menu_Foot_Text_blue", 100);

	g_settings.menu_Hint_alpha = configfile.getInt32( "menu_Hint_alpha", 20);
	g_settings.menu_Hint_red = configfile.getInt32( "menu_Hint_red", 25);
	g_settings.menu_Hint_green = configfile.getInt32( "menu_Hint_green", 25);
	g_settings.menu_Hint_blue = configfile.getInt32( "menu_Hint_blue", 25);
		
	g_settings.menu_Hint_Text_alpha = configfile.getInt32( "menu_Hint_Text_alpha", 0);
	g_settings.menu_Hint_Text_red = configfile.getInt32( "menu_Hint_Text_red", 85);
	g_settings.menu_Hint_Text_green = configfile.getInt32( "menu_Hint_Text_green", 85);
	g_settings.menu_Hint_Text_blue = configfile.getInt32( "menu_Hint_Text_blue", 85);

	strcpy( g_settings.font_file, configfile.getString( "font_file", DATADIR "/fonts/arial.ttf" ).c_str() );

	// menue timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		g_settings.timing[i] = configfile.getInt32(timing_setting_name[i], default_timing[i]);

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
	g_settings.screen_yres = configfile.getInt32("screen_yres", 100);
	
	//
	g_settings.preferred_skin = configfile.getString("preferred_skin", "neutrino2");

	// keysbinding
	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", "250").c_str());
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", "25").c_str());

	g_settings.key_tvradio_mode = configfile.getInt32( "key_tvradio_mode", RC_mode );
	
	g_settings.key_channelList_pageup = configfile.getInt32( "key_channelList_pageup", RC_page_up );
	g_settings.key_channelList_pagedown = configfile.getInt32( "key_channelList_pagedown", RC_page_down );
	g_settings.key_channelList_cancel = configfile.getInt32( "key_channelList_cancel", RC_home );
	
	g_settings.key_list_start = configfile.getInt32( "key_list_start", RC_nokey );
	g_settings.key_list_end = configfile.getInt32( "key_list_end", RC_nokey );
	
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up", RC_right);
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down", RC_left);

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up", RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down", RC_down );
	
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up", RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down", RC_left );
	g_settings.key_zaphistory = configfile.getInt32( "key_zaphistory", RC_home );	
	g_settings.key_lastchannel = configfile.getInt32( "key_lastchannel", RC_recall );
	
	// pip keys
	g_settings.key_pip = configfile.getInt32("key_pip", RC_pip);

	// media keys
	g_settings.key_movieplayer = configfile.getInt32( "key_movieplayer", RC_nokey );
	g_settings.key_audioplayer = configfile.getInt32( "key_audioplayer", RC_nokey );
	g_settings.key_pictureviewer = configfile.getInt32( "key_pictureviewer", RC_nokey );
	g_settings.key_timerlist = configfile.getInt32( "key_timerlist", RC_nokey );
	g_settings.key_inetradio = configfile.getInt32( "key_inetradio", RC_nokey );
	g_settings.key_moviebrowser = configfile.getInt32( "key_moviebrowser", RC_nokey );
	g_settings.key_filebrowser = configfile.getInt32( "key_filebrowser", RC_nokey );
	g_settings.key_screenshot = configfile.getInt32( "key_screenshot", RC_record );
	
        // USERMENU -> in system/settings.h
        //-------------------------------------------
        // this is as the current neutrino usermen
        const char * usermenu_default[SNeutrinoSettings::BUTTON_MAX] = {
                "1, 2, 3",   			// BLUE
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
                snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
                txt1[80] = 0; // terminate for sure
                g_settings.usermenu_text[button] = configfile.getString(txt1, "" );

                snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
                txt2 = configfile.getString(txt1,usermenu_default[button]);
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

	g_settings.picviewer_slide_time = configfile.getInt32("picviewer_slide_time", 10);
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", (int)SIMPLE);
	// end pictureviewer

	// misc opts
	g_settings.channel_mode = configfile.getInt32("channel_mode", LIST_MODE_ALL);

	//misc
	g_settings.power_standby = configfile.getInt32( "power_standby", 0);

	g_settings.shutdown_real = configfile.getBool("shutdown_real", true);
	g_settings.shutdown_real_rcdelay = configfile.getBool("shutdown_real_rcdelay", false);
        strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count", "0").c_str());

	g_settings.infobar_sat_display   = configfile.getBool("infobar_sat_display"  , true );
	g_settings.infobar_subchan_disp_pos = configfile.getInt32("infobar_subchan_disp_pos"  , 0 );

	g_settings.rotor_swap = configfile.getInt32( "rotor_swap", 0);
	g_settings.zap_cycle = configfile.getInt32( "zap_cycle", 1 );

	//timezone
	strcpy(g_settings.timezone, configfile.getString("timezone", "(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Vienna").c_str());
	
	//zapit setup
	g_settings.lastChannelMode = configfile.getInt32("lastChannelMode", 1);		//TV mode
	g_settings.StartChannelTV = configfile.getString("startchanneltv", "");
	g_settings.StartChannelRadio = configfile.getString("startchannelradio", "");
	g_settings.startchanneltv_id =  configfile.getInt64("startchanneltv_id", 0); 
	g_settings.startchannelradio_id = configfile.getInt64("startchannelradio_id", 0);
	g_settings.startchanneltv_nr =  configfile.getInt32("startchanneltv_nr", 0);
	g_settings.startchannelradio_nr = configfile.getInt32("startchannelradio_nr", 0);
	g_settings.uselastchannel = configfile.getInt32("uselastchannel" , 1);

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
	//g_settings.xmltv.clear();
	g_settings.epg_xmltv		= configfile.getBool("epg_xmltv", true);
	//
	
	// channellist 
	g_settings.virtual_zap_mode = configfile.getBool("virtual_zap_mode", false);
	g_settings.make_hd_list = configfile.getInt32("make_hd_list", 0);
	g_settings.channellist_ca = configfile.getInt32("channellist_ca", 1);
	g_settings.channellist_timescale = configfile.getInt32("channellist_timescale", 1);
	
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
	g_settings.logos_dir = configfile.getString("logos_dir", "/var/tuxbox/logos");
	
	// epgplus logos
	g_settings.epgplus_show_logo = configfile.getBool("epgplus_show_logo", false);
	
	// vol
	g_settings.volume_pos = configfile.getInt32( "volume_pos", 1);		//top_left
	g_settings.current_volume = configfile.getInt32("current_volume", 25);
	strcpy( g_settings.audio_step, configfile.getString( "audio_step" , "5" ).c_str() );

	// progressbar color
	g_settings.progressbar_color = configfile.getInt32("progressbar_color", 1);
	g_settings.progressbar_gradient = configfile.getInt32("progressbar_gradient", DARK2LIGHT2DARK);

	// head
	g_settings.Head_corner = configfile.getInt32("Head_corner", CORNER_TOP);
	g_settings.Head_radius = configfile.getInt32("Head_radius", RADIUS_MID);
	g_settings.Head_gradient = configfile.getInt32("Head_gradient", DARK2LIGHT2DARK);
	g_settings.Head_line = configfile.getInt32("Head_line", false);
	
	//
	g_settings.Foot_corner = configfile.getInt32("Foot_corner", CORNER_BOTTOM);
	g_settings.Foot_radius = configfile.getInt32("Foot_radius", RADIUS_MID);
	g_settings.Foot_gradient = configfile.getInt32("Foot_gradient", DARK2LIGHT2DARK);
	g_settings.Foot_line = configfile.getInt32("Foot_line", false);
	
	//
	g_settings.infobar_corner = configfile.getInt32("infobar_corner", CORNER_ALL);
	g_settings.infobar_radius = configfile.getInt32("infobar_radius", NO_RADIUS);
	g_settings.infobar_gradient = configfile.getInt32("infobar_gradient", NOGRADIENT);
	g_settings.infobar_gradient_direction = configfile.getInt32("infobar_gradient_direction", GRADIENT_HORIZONTAL);
	g_settings.infobar_buttonbar = configfile.getBool("infobar_buttonbar", true);
	g_settings.infobar_buttonline = configfile.getBool("infobar_buttonline", false);
	g_settings.infobar_border = configfile.getBool("infobar_border", false);
	
	//
	g_settings.Hint_gradient = configfile.getInt32("Hint_gradient", NOGRADIENT);
	g_settings.Hint_border = configfile.getBool("Hint_border", true);
	g_settings.Hint_radius = configfile.getInt32("Hint_radius", NO_RADIUS);
	g_settings.Hint_corner = configfile.getInt32("Hint_corner", CORNER_ALL);
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
	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		g_settings.lcd_setting[i] = configfile.getInt32(lcd_setting[i].name, lcd_setting[i].default_value);
	
	strcpy(g_settings.lcd_setting_dim_time, configfile.getString("lcd_dim_time", "0").c_str());
	g_settings.lcd_setting_dim_brightness = configfile.getInt32("lcd_dim_brightness", 0);
	
	g_settings.lcd_ledcolor = configfile.getInt32("lcd_ledcolor", 1);
	// end VFD

	// online epg
	g_settings.epg_enable_localtv_epg = configfile.getBool("epg_enable_localtv_epg", false);
	g_settings.epg_serverbox_ip = configfile.getString("epg_serverbox_ip", "192.168.0.12");
	g_settings.epg_serverbox_type = configfile.getInt32("epg_serverbox_type", DVB_C);
	g_settings.epg_serverbox_gui = configfile.getInt32("epg_serverbox_gui", SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2);

	// mode
	//g_settings.mode = configfile.getInt32("mode", mode_tv);

	//
	g_settings.ytkey = configfile.getString("ytkey", "");
	g_settings.weather_api_key = configfile.getString("weather_api_key", "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

	// tmdb
	g_settings.tmdbkey = configfile.getString("tmdbkey", "507930c8d6d400c85eae3a7e7b3f6c78");
	g_settings.enable_tmdb_infos = configfile.getBool("enable_tmdb_infos", false);
	g_settings.enable_tmdb_preview = configfile.getBool("enable_tmdb_preview", false);

	// icons dir
	g_settings.icons_dir = configfile.getString("icons_dir", DATADIR "/icons/");
	g_settings.hints_dir = configfile.getString("hints_dir", DATADIR "/hints/");
	g_settings.buttons_dir = configfile.getString("buttons_dir", DATADIR "/buttons/");
	g_settings.spinner_dir = configfile.getString("spinner_dir", DATADIR "/spinner/");
	
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

	configfile.setString("font_file", g_settings.font_file);

	// menue timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		configfile.setInt32(timing_setting_name[i], g_settings.timing[i]);
	
	//
	configfile.setString("preferred_skin", g_settings.preferred_skin);
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
	configfile.setInt32( "key_pip", g_settings.key_pip );
	
	// media keys
	configfile.setInt32( "key_movieplayer", g_settings.key_movieplayer );
	configfile.setInt32( "key_audioplayer", g_settings.key_audioplayer );
	configfile.setInt32( "key_pictureviewer", g_settings.key_pictureviewer );
	configfile.setInt32( "key_timerlist", g_settings.key_timerlist );
	configfile.setInt32( "key_inetradio", g_settings.key_inetradio );
	configfile.setInt32( "key_moviebrowser", g_settings.key_moviebrowser );
	configfile.setInt32( "key_filebrowser", g_settings.key_filebrowser );
	
	configfile.setInt32( "key_screenshot", g_settings.key_screenshot );
	
        // USERMENU
        char txt1[81];
        char txt2[81];
        for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++) 
	{
                snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
                txt1[80] = 0; // terminate for sure
                configfile.setString(txt1,g_settings.usermenu_text[button]);

                char* txt2ptr = txt2;
                snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);

                for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++) 
		{
                        if( g_settings.usermenu[button][pos] != 0) 
			{
                                if(pos != 0)
                                        *txt2ptr++ = ',';
                                txt2ptr += snprintf(txt2ptr,80,"%d",g_settings.usermenu[button][pos]);
                        }
                }
                configfile.setString(txt1,txt2);
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
	
	//crypticon channellist
	configfile.setInt32("channellist_ca", g_settings.channellist_ca);
	configfile.setInt32("make_hd_list", g_settings.make_hd_list);
	configfile.setInt32("channellist_timescale", g_settings.channellist_timescale);

	//
	configfile.setString("timezone", g_settings.timezone);

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
	//g_settings.xmltv.clear();
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
	//configfile.setInt32( "sms_channel", g_settings.sms_channel );
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
	
	// epgplus logos
	configfile.setBool("epgplus_show_logo", g_settings.epgplus_show_logo);
	
	// record screenshot
	configfile.setInt32("recording_screenshot", g_settings.recording_screenshot);
	
	// vol
	configfile.setInt32( "volume_pos", g_settings.volume_pos);
	configfile.setInt32( "current_volume", g_settings.current_volume );
	configfile.setString( "audio_step"	, g_settings.audio_step);

	configfile.setInt32("progressbar_color", g_settings.progressbar_color);
	configfile.setInt32("progressbar_gradient", g_settings.progressbar_gradient);

	// head
	configfile.setInt32("Head_corner", g_settings.Head_corner);
	configfile.setInt32("Head_radius", g_settings.Head_radius);
	configfile.setInt32("Head_gradient", g_settings.Head_gradient);
	configfile.setBool("Head_line", g_settings.Head_line);
	
	// foot
	configfile.setInt32("Foot_corner", g_settings.Foot_corner);
	configfile.setInt32("Foot_radius", g_settings.Foot_radius);
	configfile.setInt32("Foot_gradient", g_settings.Foot_gradient);
	configfile.setBool("Foot_line", g_settings.Foot_line);
	
	//
	configfile.setInt32("Hint_gradient", g_settings.Hint_gradient);
	configfile.setBool("Hint_border", g_settings.Hint_border);
	configfile.setInt32("Hint_radius", g_settings.Hint_radius);
	configfile.setInt32("Hint_corner", g_settings.Hint_corner);
	
	//
	configfile.setInt32("infobar_gradient", g_settings.infobar_gradient);
	configfile.setInt32("infobar_gradient_direction", g_settings.infobar_gradient_direction);
	configfile.setInt32("infobar_corner", g_settings.infobar_corner);
	configfile.setInt32("infobar_radius", g_settings.infobar_radius);
	configfile.setBool("infobar_buttonbar", g_settings.infobar_buttonbar);
	configfile.setBool("infobar_buttonline", g_settings.infobar_buttonline);
	configfile.setBool("infobar_border", g_settings.infobar_border);

	//
	configfile.setString("ytkey", g_settings.ytkey);
	configfile.setString("weather_api_key", g_settings.weather_api_key);
	
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
	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		configfile.setInt32(lcd_setting[i].name, g_settings.lcd_setting[i]);
	
	configfile.setString("lcd_dim_time", g_settings.lcd_setting_dim_time);
	configfile.setInt32("lcd_dim_brightness", g_settings.lcd_setting_dim_brightness);
	
	configfile.setInt32("lcd_ledcolor", g_settings.lcd_ledcolor);
	// END VFD

	// online epg
	configfile.setBool("epg_enable_localtv_epg", g_settings.epg_enable_localtv_epg);
	configfile.setString("epg_serverbox_ip", g_settings.epg_serverbox_ip);
	configfile.setInt32("epg_serverbox_type", g_settings.epg_serverbox_type);
	configfile.setInt32("epg_serverbox_gui", g_settings.epg_serverbox_gui);

	// icons dir
	configfile.setString("icons_dir", g_settings.icons_dir);
	configfile.setString("hints_dir", g_settings.hints_dir);	
	configfile.setString("buttons_dir", g_settings.buttons_dir);
	configfile.setString("spinner_dir", g_settings.spinner_dir);

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

	CZapit::getInstance()->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
}

// CNeutrinoApp -  channelsInit, get the Channellist from zapit
void CNeutrinoApp::channelsInit(bool /*bOnly*/)
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
	TVbouquetList->orgChannelList = TVchannelList;

	TVsatList = new CBouquetList(_("Satellites"));
	TVsatList->orgChannelList = TVchannelList;

	TVfavList = new CBouquetList(_("Favorites"));
	TVfavList->orgChannelList = TVchannelList;

	RADIObouquetList = new CBouquetList(_("Provider"));
	RADIObouquetList->orgChannelList = RADIOchannelList;

	RADIOsatList = new CBouquetList(_("Satellites"));
	RADIOsatList->orgChannelList = RADIOchannelList;

	RADIOfavList = new CBouquetList(_("Favorites"));
	RADIOfavList->orgChannelList = RADIOchannelList;

	//
	uint32_t i;
	i = 1;

	int tvi = 1, ri = 1, hi = 0, webtvi = 0;
	
	// hd bouquet
	CBouquet * hdBouquet;
	if(g_settings.make_hd_list)
		hdBouquet = new CBouquet(0, (char *) "HD", 0);

	// all services TV/ Radio/WebTV Channellist
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		if ((it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE)) 
		{
			TVchannelList->addChannel(&(it->second), tvi++);

			if(it->second.isHD()) 
			{
				if(g_settings.make_hd_list)
					hdBouquet->channelList->addChannel(&(it->second));
				hi++;
			}
			
			if (it->second.isWEBTV())
				webtvi++;
		}
		else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
		{
			RADIOchannelList->addChannel(&(it->second), ri++);
		}
	}
	
	if(g_settings.make_hd_list)
		hdBouquet->channelList->SortSat();

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d TV (%d is HD) and %d RADIO and %d WEBTV channels\n", tvi - 1, hi, ri - 1, webtvi);

	CBouquet * tmp;

	// tv all list
	TVallList = new CBouquetList(_("All Services"));
	tmp = TVallList->addBouquet(_("All Services"));
	*(tmp->channelList) = *TVchannelList;
	//tmp->channelList->SortAlpha();
	TVallList->orgChannelList = TVchannelList;

	// radio all list
	RADIOallList = new CBouquetList(_("All Services"));
	tmp = RADIOallList->addBouquet(_("All Services"));
	*(tmp->channelList) = *RADIOchannelList;
	//tmp->channelList->SortAlpha();
	RADIOallList->orgChannelList = RADIOchannelList;

	// sat
	int bnum;
	sat_iterator_t sit;
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		tvi = 0, ri = 0;
		CBouquet * tmp1 = TVsatList->addBouquet(sit->second.name.c_str());
		CBouquet * tmp2 = RADIOsatList->addBouquet(sit->second.name.c_str());

		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
		{
			if(it->second.getSatellitePosition() == sit->first && !it->second.isWEBTV()) //FIXME: 
			{
				if (it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE) 
				{
					tmp1->channelList->addChannel(&(it->second));
					tvi++;
				}
				else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
				{
					tmp2->channelList->addChannel(&(it->second));
					ri++;
				}
			}
		}

		if(tvi)
			tmp1->channelList->SortAlpha();
		else
			TVsatList->deleteBouquet(tmp1);

		if(ri)
			tmp2->channelList->SortAlpha();
		else
			RADIOsatList->deleteBouquet(tmp2);

		if(tvi || ri)
			printf("CNeutrinoApp::channelsInit: created %s with %d TV and %d RADIO channels\n", sit->second.name.c_str(), tvi, ri);
	}

	// tv/webtv fav / provider list
	bnum = 0;
	for (i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{
		// tv
		if (!g_bouquetManager->Bouquets[i]->bHidden && !g_bouquetManager->Bouquets[i]->tvChannels.empty())
		{
			CBouquet * ltmp;
			if(g_bouquetManager->Bouquets[i]->bUser || g_bouquetManager->Bouquets[i]->bWebTV) 
				ltmp = TVfavList->addBouquet(g_bouquetManager->Bouquets[i]);
			else
				ltmp = TVbouquetList->addBouquet(g_bouquetManager->Bouquets[i]);

			ZapitChannelList * channels = &(g_bouquetManager->Bouquets[i]->tvChannels);
			ltmp->channelList->setSize(channels->size());
			
			for(int j = 0; j < (int) channels->size(); j++) 
			{
				ltmp->channelList->addChannel((*channels)[j]);
			}
			bnum++;
		}
	}
	
	if(g_settings.make_hd_list)
		TVfavList->Bouquets.push_back(hdBouquet);
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d TV bouquets\n", bnum);

	// radio fav / provider list
	bnum = 0;
	for (i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{	
		if (!g_bouquetManager->Bouquets[i]->bHidden && !g_bouquetManager->Bouquets[i]->radioChannels.empty() )
		{
			CBouquet * ltmp;
			if(g_bouquetManager->Bouquets[i]->bUser) 
				ltmp = RADIOfavList->addBouquet(g_bouquetManager->Bouquets[i]);
			else
				ltmp = RADIObouquetList->addBouquet(g_bouquetManager->Bouquets[i]);

			ZapitChannelList *channels = &(g_bouquetManager->Bouquets[i]->radioChannels);
			ltmp->channelList->setSize(channels->size());
			for(int j = 0; j < (int) channels->size(); j++) 
			{
				ltmp->channelList->addChannel((*channels)[j]);
			}
			bnum++;
		}
	}

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::channelsInit: got %d RADIO bouquets\n", bnum);

	//
	setChannelMode( g_settings.channel_mode, mode);
	
	// load webtv logos
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

	// channelList
	if(nMode == mode_radio)
		channelList = RADIOchannelList;
	else if(nMode == mode_tv)
		channelList = TVchannelList;

	// bouquetList
	switch(newmode) 
	{
		case LIST_MODE_FAV:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOfavList;
			} 
			else if(nMode == mode_tv) 
			{
				bouquetList = TVfavList;
			}
			break;

		case LIST_MODE_SAT:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOsatList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVsatList;
			}
			break;

		case LIST_MODE_ALL:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIOallList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVallList;
			}
			break;

		default:
		case LIST_MODE_PROV:
			if(nMode == mode_radio) 
			{
				bouquetList = RADIObouquetList;
			} 
			else if(nMode == mode_tv)
			{
				bouquetList = TVbouquetList;
			}
			break;
	}

	g_settings.channel_mode = newmode;
}

// CNeutrinoApp -  run, the main runloop
void CNeutrinoApp::cmdParser(int argc, char **argv)
{
        global_argv = new char *[argc + 1];
	
        for (int i = 0; i < argc; i++)
                global_argv[i] = argv[i];
	
        global_argv[argc] = NULL;

	for(int x = 1; x < argc; x++) 
	{
		if (((!strcmp(argv[x], "-v")) || (!strcmp(argv[x], "--verbose"))) && (x + 1 < argc)) 
		{
			int dl = atoi(argv[x + 1]);
			dprintf(DEBUG_NORMAL, "set debuglevel: %d\n", dl);
			setDebugLevel(dl);
			x++;
		}
		else 
		{
			dprintf(DEBUG_NORMAL, "Usage: neutrino [-v | --verbose 0..2]\n");
		}
	}
}

// setup the framebuffer
void CNeutrinoApp::setupFrameBuffer()
{
	frameBuffer->init();
	
	if(frameBuffer->setMode() ) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupFrameBuffer: Error while setting framebuffer mode\n");
		exit(-1);
	}	
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

	// set signal font
	g_SignalFont = g_fontRenderer->getFont(font.name, style[signal_font.style], signal_font.defaultsize + signal_font.size_offset * font.size_offset);

	// recalculate infobar position
	if (g_InfoViewer)
		g_InfoViewer->start();
}

// setup the menu timeouts
void CNeutrinoApp::setupTiming()
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupTiming\n");

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		sprintf(g_settings.timing_string[i], "%d", g_settings.timing[i]);
};

// setup recording device
void CNeutrinoApp::setupRecordingDevice(void)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::setupRecordingDevice\n");

	if (recDir != NULL)
	{
		recordingdevice = new CVCRControl::CFileDevice(g_settings.network_nfs_recordingdir );

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else
	{
		if (CVCRControl::getInstance()->isDeviceRegistered())
			CVCRControl::getInstance()->unregisterDevice();
	}
}

#if defined (USE_OPENGL) // opengl playback
int startOpenGLplayback()
{
	CTimerd::RecordingInfo eventinfo;

	if( !CVCRControl::getInstance()->isDeviceRegistered() )
		return 0;

	eventinfo.channel_id = live_channel_id;
	CEPGData epgData;
	
	if (CSectionsd::getInstance()->getActualEPGServiceKey(live_channel_id & 0xFFFFFFFFFFFFULL, &epgData ))
	{
		eventinfo.epgID = epgData.eventID;
		eventinfo.epg_starttime = epgData.epg_times.startzeit;
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

	(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = timeshiftDir;

	if( CVCRControl::getInstance()->Record(&eventinfo))
		CNeutrinoApp::getInstance()->playbackstatus = 1;
	else
		CNeutrinoApp::getInstance()->playbackstatus = 0;
	
	// start playback	
	char fname[255];
	int cnt = 10 * 1000000;

	while (!strlen(rec_filename)) 
	{
			usleep(1000);
			cnt -= 1000;
			if (!cnt)
				break;
	}

	if(strlen(rec_filename))
	{
		sprintf(fname, "%s.ts", rec_filename);
		
		usleep(10000000);
		playback->Open();
		playback->Start(fname);
	} 
	
	return 0;
}

void stopOpenGLplayback()
{
	// stop playback
	playback->Close();
	
	// stop recording
	if(CNeutrinoApp::getInstance()->playbackstatus) 
	{
		CNeutrinoApp::getInstance()->playbackstatus = 0;
		
		CVCRControl::getInstance()->Stop();

		char buf[1024];
		char buf1[1024];

		sprintf(buf, "rm -f %s.ts &", rec_filename);
		sprintf(buf1, "%s.xml", rec_filename);

		system(buf);
		unlink(buf1);
	}
}
#endif

// start auto record (permanent/temp timeshift)
int startAutoRecord(bool addTimer)
{
	CTimerd::RecordingInfo eventinfo;

	if(CNeutrinoApp::getInstance()->recordingstatus || !CVCRControl::getInstance()->isDeviceRegistered() )
		return 0;

	eventinfo.channel_id = live_channel_id;
	CEPGData epgData;
	
	if (CSectionsd::getInstance()->getActualEPGServiceKey(live_channel_id & 0xFFFFFFFFFFFFULL, &epgData ))
	{
		eventinfo.epgID = epgData.eventID;
		eventinfo.epg_starttime = epgData.epg_times.startzeit;
		strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
		eventinfo.epgTitle[EPG_TITLE_MAXLEN-1]=0;
	}
	else 
	{
		eventinfo.epgID = 0;
		eventinfo.epg_starttime = 0;
		strcpy(eventinfo.epgTitle, "");
	}

	eventinfo.apids = TIMERD_APIDS_CONF;
	
	dprintf(DEBUG_NORMAL, "startAutoRecord: dir %s\n", timeshiftDir);

	(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = timeshiftDir;

	autoshift = 1;
	CNeutrinoApp::getInstance()->recordingstatus = 1;
	CNeutrinoApp::getInstance()->timeshiftstatus = 1;

	if( CVCRControl::getInstance()->Record(&eventinfo) == false ) 
	{
		CNeutrinoApp::getInstance()->recordingstatus = 0;
		CNeutrinoApp::getInstance()->timeshiftstatus = 0;
		autoshift = 0;
		
		CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );	
	}
	else if (addTimer) 
	{
		time_t now = time(NULL);
		CNeutrinoApp::getInstance()->recording_id = CTimerd::getInstance()->addImmediateRecordTimerEvent(eventinfo.channel_id, now, now + g_settings.record_hours*60*60, eventinfo.epgID, eventinfo.epg_starttime, eventinfo.apids);
	}	

	CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

	return 0;
}

// stop auto record
void stopAutoRecord()
{
	if(autoshift && CNeutrinoApp::getInstance()->recordingstatus) 
	{
		dprintf(DEBUG_NORMAL, "stopAutoRecord: autoshift, recordingstatus %d, stopping ...\n", CNeutrinoApp::getInstance()->recordingstatus);

		CVCRControl::getInstance()->Stop();
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
	
	CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);
}

// do gui-record
bool CNeutrinoApp::doGuiRecord(char * preselectedDir, bool addTimer)
{
	CTimerd::RecordingInfo eventinfo;
	bool refreshGui = false;

	if(CVCRControl::getInstance()->isDeviceRegistered()) 
	{
		// stop auto record
		if(autoshift) 
			stopAutoRecord();

		//
		if(recordingstatus == 1) 
		{
			puts("CNeutrinoApp::doGuiRecord: executing " NEUTRINO_RECORDING_START_SCRIPT ".");

			if (system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
				perror(NEUTRINO_RECORDING_START_SCRIPT " failed");
			
			//
			time_t now = time(NULL);
			int record_end = now + g_settings.record_hours*60*60;
			int pre = 0, post = 0;

			// get EPG info
			eventinfo.channel_id = live_channel_id;

			CEPGData epgData;

			if (CSectionsd::getInstance()->getActualEPGServiceKey(eventinfo.channel_id & 0xFFFFFFFFFFFFULL, &epgData ))
			{
				eventinfo.epgID = epgData.eventID;
				eventinfo.epg_starttime = epgData.epg_times.startzeit;
				strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
				eventinfo.epgTitle[EPG_TITLE_MAXLEN - 1] = 0;
				
				// record end time
				CTimerd::getInstance()->getRecordingSafety(pre, post);
				
				if (epgData.epg_times.startzeit > 0)
					record_end = epgData.epg_times.startzeit + epgData.epg_times.dauer + post;
			}
			else 
			{
				eventinfo.epgID = 0;
				eventinfo.epg_starttime = 0;
				strcpy(eventinfo.epgTitle, "");
			}

			eventinfo.apids = TIMERD_APIDS_CONF;

			bool doRecord = true;

			// rec dir
			strcpy(recDir, (preselectedDir != NULL) ? preselectedDir : g_settings.network_nfs_recordingdir);
				
			(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = recDir;
			
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::doGuiRecord: start record to dir %s\n", recDir);

			// start to record 
			if( !doRecord || (CVCRControl::getInstance()->Record(&eventinfo) == false ) ) 
			{
				recordingstatus = 0;

				if(doRecord)
					return true;// try to refresh gui if record was not ok ?

				return refreshGui;
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
		
		CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);

		return refreshGui;
	}
	else
		puts("CNeutrinoApp::doGuiRecord: no recording devices");


	return false;
}

// start next recording
void CNeutrinoApp::startNextRecording()
{
	if ( nextRecordingInfo != NULL ) 
	{
		bool doRecord = true;
		
		if (CVCRControl::getInstance()->isDeviceRegistered()) 
		{
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
							CFSMounter::MountRes mres =
								CFSMounter::mount(g_settings.network_nfs_ip[i].c_str(), g_settings.network_nfs_dir[i],
										  g_settings.network_nfs_local_dir[i], (CFSMounter::FSType) g_settings.network_nfs_type[i],
										  g_settings.network_nfs_username[i], g_settings.network_nfs_password[i],
										  g_settings.network_nfs_mount_options1[i], g_settings.network_nfs_mount_options2[i]);
										  
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

				(static_cast<CVCRControl::CFileDevice*>(recordingdevice))->Directory = std::string(lrecDir);
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::startNextRecording: start to dir %s\n", lrecDir);

				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, true);
			}
			
			if(doRecord && CVCRControl::getInstance()->Record(nextRecordingInfo))
				recordingstatus = 1;
			else
				recordingstatus = 0;
		}
		else
			puts("CNeutrinoApp::startNextRecording: no recording devices");

		/* Note: CTimerd::RecordingInfo is a class!
		 * What a brilliant idea to send classes via the eventserver!
		 * => typecast to avoid destructor call
		 */
		delete [](unsigned char *)nextRecordingInfo;

		nextRecordingInfo = NULL;
	}
}

#define LCD_UPDATE_TIME_RADIO_MODE (6 * 1000 * 1000)
#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

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

	// start infobar
	g_InfoViewer->start();
	
	// send sectionsd config
	sendSectionsdConfig();

	// read saved epg
	if (g_settings.epg_save)
		readEPG();

	// first channel
	firstChannel();

	int tvmode = CZapit::getInstance()->getMode();

	if (tvmode == CZapit::MODE_TV)
		mode = NeutrinoMessages::mode_tv;
	else if (tvmode == CZapit::MODE_RADIO)
		mode = NeutrinoMessages::mode_radio;

	lastMode = mode;
	
	// init channel
	channelsInit();

	// set mode
	if(firstchannel.mode == 't') 
	{
		tvMode(false);
	} 
	else if(firstchannel.mode == 'r') 
	{
#if defined (ENABLE_LCD)	  
			g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
			g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );
#endif		
		
			radioMode(false);
	}

	// zap / epg / autorecord / infoviewer
	if(channelList->getSize() && live_channel_id)
	{
		// channellist adjust to channeliD
		channelList->adjustToChannelID(live_channel_id);

		// show service name in vfd
		CVFD::getInstance()->showServicename(channelList->getActiveChannelName(), true, channelList->getActiveChannelNumber());

		// start epg scanning
		CSectionsd::getInstance()->pauseScanning(false);
		CSectionsd::getInstance()->setServiceChanged(live_channel_id, true );
		
		// process apids
		CZapit::getInstance()->getPIDS(g_RemoteControl->current_PIDs);
		g_RemoteControl->processAPIDnames();
				
		// permenant timeshift
		if(g_settings.auto_timeshift)
			startAutoRecord(true);
		
		// show infobar
		g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR, 0);
		
		selectSubtitles();
		
		startSubtitles();
	}
}

#if defined (PLATFORM_COOLSTREAM)
static void CSSendMessage(uint32_t msg, uint32_t data)
{
	if (g_RCInput)
		g_RCInput->postMsg(msg, data);
}
#endif

void CISendMessage(uint32_t msg, uint32_t data)
{
	if (g_RCInput)
	      g_RCInput->postMsg(msg, data);
}

int CNeutrinoApp::run(int argc, char **argv)
{
	dprintf( DEBUG_NORMAL, "CNeutrinoApp::run:\n");

	cmdParser(argc, argv);
	
#if defined (PLATFORM_COOLSTREAM)
	cs_api_init();
	cs_register_messenger(CSSendMessage);
#endif
	
	// font
	font.name = NULL;
	font.filename = NULL;

	// load settings
	int loadSettingsErg = loadSetup(NEUTRINO_SETTINGS_FILE);

	// check locale language
	g_Locale->loadLocale(Lang2I18N(g_settings.language).c_str());

	// icons/buttons/hints path
	frameBuffer->setIconBasePath(g_settings.icons_dir);
	frameBuffer->setButtonBasePath(g_settings.buttons_dir);
	frameBuffer->setHintBasePath(g_settings.hints_dir);
	frameBuffer->setSpinnerBasePath(g_settings.spinner_dir);

	// setup fonts
	setupFonts(g_settings.font_file);
	
	// setup menue timing
	setupTiming();
	
	// setup color
	colorSetupNotifier = new CColorSetupNotifier;
	colorSetupNotifier->changeNotify("", NULL);

	// init vfd/lcd display
#if ENABLE_LCD
	CVFD::getInstance()->init(font.filename, font.name);
#else	
	CVFD::getInstance()->init();
#endif	

	// VFD clear	
	CVFD::getInstance()->Clear();	

	// show startup msg in vfd
	CVFD::getInstance()->ShowText( (char *)"N2");
	
	// register events
	// httpd
	eventServer->registerEvent2(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::STANDBY_TOGGLE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_POPUP, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_EXTMSG, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::CHANGEMODE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_START_PLUGIN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::LOCK_RC, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::UNLOCK_RC, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::REBOOT, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::RESTART, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	// sectionsd
	eventServer->registerEvent2(NeutrinoMessages::EVT_TIMESET, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_CURRENTNEXT_EPG, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SI_FINISHED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	// zapit
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_COMPLETE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_ISNVOD, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_FAILED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_SUB_FAILED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_MOTOR, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_CA_CLEAR, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_CA_LOCK, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_CA_FTA, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_ZAP_CA_ID, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_RECORDMODE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	//eventServer->registerEvent2(NeutrinoMessages::EVT_RECORDMODE_DEACTIVATED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_COMPLETE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_FAILED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_SATELLITE, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_NUM_CHANNELS, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_PROVIDER, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_BOUQUETSCHANGED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SERVICESCHANGED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_SERVICENAME, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_SERVICES_UPD, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_PMT_CHANGED, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	// timerd
	eventServer->registerEvent2(NeutrinoMessages::ANNOUNCE_SHUTDOWN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::SHUTDOWN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_NEXTPROGRAM, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	//eventServer->registerEvent2(NeutrinoMessages::NEXTPROGRAM, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::STANDBY_ON, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::STANDBY_OFF, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::ANNOUNCE_RECORD, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::RECORD_START, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::RECORD_STOP, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::ANNOUNCE_ZAPTO, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::ZAPTO, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::SLEEPTIMER, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::ANNOUNCE_SLEEPTIMER, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::REMIND, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);
	eventServer->registerEvent2(NeutrinoMessages::EVT_START_PLUGIN, CEventServer::INITID_NEUTRINO, NEUTRINO_UDS_NAME);

	// rc 
	g_RCInput = new CRCInput;
	g_RCInput->setRepeat(atoi(g_settings.repeat_blocker), atoi(g_settings.repeat_genericblocker));

	// playback
	playback = new cPlayback();

	// plugins
	g_PluginList = new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);
	g_PluginList->loadPlugins();
	
	// load selected skin
	loadSkin(g_settings.preferred_skin);
	
	// zapit	
	Z_start_arg ZapStart_arg;
	
	ZapStart_arg.lastchannelmode = g_settings.lastChannelMode;
	ZapStart_arg.startchanneltv_id = g_settings.startchanneltv_id;
	ZapStart_arg.startchannelradio_id = g_settings.startchannelradio_id;
	ZapStart_arg.startchanneltv_nr = g_settings.startchanneltv_nr;
	ZapStart_arg.startchannelradio_nr = g_settings.startchannelradio_nr;
	ZapStart_arg.uselastchannel = g_settings.uselastchannel;
	ZapStart_arg.video_mode = g_settings.video_Mode;
	
	current_volume = g_settings.current_volume;

	// zapit
	CZapit::getInstance()->Start(&ZapStart_arg);

	// wait until zapit is ready
	while(!zapit_ready)
		usleep(0);
	
	// dvbsub thread
	dvbsub_init();

	// audio volume (default)
	if(audioDecoder)
		audioDecoder->setVolume(g_settings.current_volume, g_settings.current_volume);

	// video format
	if(videoDecoder)
		videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
	
	// wss
	if(videoDecoder)
		videoDecoder->SetWideScreen(g_settings.wss_mode);
	
	// avsync
#if defined (PLATFORM_COOLSTREAM)
	videoDecoder->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
	audioDecoder->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
	videoDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
	audioDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
	pcrDemux->SetSyncMode((AVSYNC_TYPE)g_settings.avsync);
#else
	if(videoDecoder)
		videoDecoder->SetSyncMode(g_settings.avsync);			
		
	if(audioDecoder)
		audioDecoder->SetSyncMode(g_settings.avsync);
#endif

	if(audioDecoder)
	{
		// hdmi_dd
		audioDecoder->SetHdmiDD(g_settings.hdmi_dd );

		// ac3 delay
		audioDecoder->setHwAC3Delay(g_settings.ac3_delay);

		// pcm delay 
		audioDecoder->setHwPCMDelay(g_settings.pcm_delay);
	}

	// timerd
	CTimerd::getInstance()->Start();

	// nhttpd
	Cyhttpd::getInstance()->Start();

	// streamts
	CStreamTS::getInstance()->Start();	

	// sectionsd
	CSectionsd::getInstance()->Start();

	// for boxes with lcd :-)
#if ENABLE_LCD	
	CVFD::getInstance()->showVolume(g_settings.current_volume);
	CVFD::getInstance()->setMuted(current_muted);
#endif
	
	// remote control
	g_RemoteControl = new CRemoteControl;
	
	// epg view
	g_EpgData = new CEpgData;
	
	// channel infoviewer
	g_InfoViewer = new CInfoViewer;
	
	// event list
	g_EventList = new EventList;

	// Ci Cam handler
#if defined (ENABLE_CI)	
	g_CamHandler = new CCAMMenuHandler();
	g_CamHandler->init();	
#endif
	
	// init network at startup
	CNetworkSettings::getInstance()->readNetworkSettings(g_settings.ifname);

	if (CNetworkSettings::getInstance()->network_automatic_start == 1)
		CNetworkSettings::getInstance()->setNetwork();	
	
	// mount shares before scanning for plugins
	CFSMounter::automount();

	// assuming that mdev/fstab has mounted devices
	CHDDDestExec * hdd = new CHDDDestExec();
	hdd->exec(NULL, "");
	delete hdd;
	hdd = NULL;
	
	// init nvod changer
	NVODChanger = new CNVODChangeExec;
	
	// init rclock
	rcLock = new CRCLock();

	// setup recording device
	setupRecordingDevice();

	//
	CVFD::getInstance()->setPower(g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER]);
	CVFD::getInstance()->setlcdparameter();
	
	// start assistant
	if(loadSettingsErg) 
	{
		int tvmode = CZapit::getInstance()->getMode();

		if (tvmode == CZapit::MODE_TV)
			mode = NeutrinoMessages::mode_tv;
		else if (tvmode == CZapit::MODE_RADIO)
			mode = NeutrinoMessages::mode_radio;

		// startup pic : FIXME
		//frameBuffer->loadBackgroundPic("start.jpg");	
		//frameBuffer->blit();
	
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
			if(tzSelect)
				tzSelect->exec(NULL);
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
	
	// audio mute
	audioMute(current_muted, true);	

// Cam-Ci
#if defined (ENABLE_CI)	
	cDvbCi::getInstance()->SetHook(CISendMessage);	
#endif	
	
	// init shutdown count
	SHTDCNT::getInstance()->init();

	// realRun loop ;-)
	realRun();

	// exitRun
	exitRun(SHUTDOWN);

	// never reached
	return 0;
}

// quickZap
void CNeutrinoApp::quickZap(int msg)
{
	stopSubtitles();
	
	if(g_settings.zap_cycle && (bouquetList != NULL) && !(bouquetList->Bouquets.empty()))
		bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->quickZap(msg, true);
	else
		channelList->quickZap(msg);
}

// showInfo
void CNeutrinoApp::showInfo()
{
	stopSubtitles();

	g_InfoViewer->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID());

	startSubtitles();
}

// real run
void CNeutrinoApp::realRun(void)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::realRun: initialized everything\n");

	// clear msg 
	g_RCInput->clearRCMsg();

	// if start to standby
	if(g_settings.power_standby)
	{
		standbyMode(true);
	}

	// neutrino run loop
	while( true ) 
	{
		g_RCInput->getMsg(&msg, &data, 100);	// 10 secs..
		
		dprintf(DEBUG_DEBUG, "CNeutrinoApp::realRun: msg:%s\n", CRCInput::getSpecialKeyName(msg));		

		// mode TV/Radio/IPTV
		if( (mode == mode_tv) || (mode == mode_radio) ) 
		{
			if(msg == NeutrinoMessages::SHOW_EPG) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				g_EpgData->show(live_channel_id);

				startSubtitles();
			}
			else if(msg == RC_epg) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				g_EventList->exec(live_channel_id, channelList->getActiveChannelName());

				startSubtitles();
			}
			else if( msg == RC_text ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				g_RCInput->clearRCMsg();

				stopSubtitles();
				
				tuxtx_stop_subtitle();

				tuxtx_main(g_RemoteControl->current_PIDs.PIDs.vtxtpid, 0, live_fe?live_fe->fenumber : 0 );

				frameBuffer->paintBackground();

				frameBuffer->blit();
				
				g_RCInput->clearRCMsg();
				
				audioMute(current_muted, true);

				startSubtitles();
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
				
				startSubtitles();
			}		
			else if( msg == RC_setup ) 
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
					if(channelList->getSize()) 
						showInfo();	
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
					if(channelList->getSize()) 
						showInfo();	
				}
				else
				{
					quickZap(msg);
				}
			}
			// in case key_subchannel_up/down redefined
			else if((msg == RC_left || msg == RC_right)) 
			{
				if(channelList->getSize()) 
				{
					showInfo();
				}
			}
			else if( msg == (neutrino_msg_t) g_settings.key_zaphistory) 
			{
				stopSubtitles();
				
				// Zap-History "Bouquet"
				int res = channelList->numericZap( msg );

				startSubtitles(res < 0);
			}
			else if(msg == (neutrino_msg_t) g_settings.key_lastchannel) 
			{
				stopSubtitles();
				
				// Quick Zap
				int res = channelList->numericZap( msg );

				startSubtitles(res < 0);
			}
			else if(msg == RC_pause) // start timeshift recording
			{
				if (IS_WEBTV(live_channel_id))
				{
					CZapit::getInstance()->pausePlayBack();
					timeshiftstatus = 1;
				}
				else
				{
					if (recDir != NULL)
					{
						if(g_RemoteControl->is_video_started) 
						{		
							// ptimeshift
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
							audioDecoder->Stop();
							videoDecoder->Stop(false); // dont blank
						}
					}
				}
			}
			else if( ((msg == RC_play) && timeshiftstatus)) // play timeshift
			{
				
				if (IS_WEBTV(live_channel_id))
				{
					CZapit::getInstance()->continuePlayBack();
					timeshiftstatus = 0;
				}
				else
				{		
					if(g_RemoteControl->is_video_started) 
					{
						CMoviePlayerGui tmpMoviePlayerGui;
						CMovieInfo cMovieInfo;
						MI_MOVIE_INFO mfile;

						//
						char fname[255];
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
						cMovieInfo.clearMovieInfo(&mfile);

						mfile.file.Name = fname;
			
						// extract channel epg infos
						CEPGData epgData;
						event_id_t epgid = 0;
			
						if(CSectionsd::getInstance()->getActualEPGServiceKey(live_channel_id & 0xFFFFFFFFFFFFULL, &epgData))
							epgid = epgData.eventID;

						if(epgid != 0) 
						{
							CShortEPGData epgdata;
				
							if(CSectionsd::getInstance()->getEPGidShort(epgid, &epgdata)) 
							{
								if (!(epgdata.title.empty())) 
									mfile.epgTitle = epgdata.title;
					
								if(!(epgdata.info1.empty()))
									mfile.epgInfo1 = epgdata.info1;
					
								if(!(epgdata.info2.empty()))
									mfile.epgInfo2 = epgdata.info2;
							}
						}

						// epgTitle
						if(mfile.epgTitle.empty())
						{
							std::string Title = mfile.file.getFileName();
							removeExtension(Title);
							mfile.epgTitle = Title;
						}

						mfile.ytid = "timeshift";

						tmpMoviePlayerGui.addToPlaylist(mfile);
						tmpMoviePlayerGui.exec(NULL, "urlplayback");
					}
				}
			}
			else if( (msg == RC_record || msg == RC_stop) ) 
			{
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::realRun\n");
				
				if(autoshift) 
				{
					stopAutoRecord();
					recordingstatus = 0;
					timeshiftstatus = 0;
				}

				// stop record if recording
				if( recordingstatus ) 
				{
					if(MessageBox(_("Information"), _("You really want to to stop record ?"), mbrYes, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes)
					{
						CTimerd::getInstance()->stopTimerEvent(recording_id);
						recordingstatus = 0; //FIXME???
						CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );
					}
				} 
				// start record
				else if(msg != RC_stop )
				{
					recordingstatus = 1;
					doGuiRecord( g_settings.network_nfs_recordingdir, true );
				}
			}
			else if( msg == RC_red ) 
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
				
				//
				startSubtitles();
			}
			else if( ( msg == RC_green) || ( msg == RC_audio) )
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				if (IS_WEBTV(live_channel_id))
				{
					CAVPIDSelectWidget * AVSelectHandler = new CAVPIDSelectWidget();
					AVSelectHandler->exec(NULL, "");
							
					delete AVSelectHandler;
					AVSelectHandler = NULL;
				}
				else
				{
					stopSubtitles();

					// audio handler
					CAudioSelectMenuHandler* audioSelectMenuHandler = new CAudioSelectMenuHandler();

					audioSelectMenuHandler->exec(NULL, "");
							
					delete audioSelectMenuHandler;
					audioSelectMenuHandler = NULL;

					startSubtitles();
				}
			}
			else if( (msg == RC_yellow || msg == RC_multifeed) )
			{ 
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				// select NVODs
				if (!IS_WEBTV(live_channel_id))
				{
					selectNVOD();
				}

				startSubtitles();
			}
			else if( msg == RC_blue ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();

				// features
				showUserMenu(SNeutrinoSettings::BUTTON_BLUE);

				startSubtitles();
			}
#if defined (ENABLE_FUNCTIONKEYS)			
			else if( msg == RC_f1 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F1);
				startSubtitles();
			}
			else if( msg == RC_f2 )
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F2);
				startSubtitles();
			}
			else if( msg == RC_f3 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();
 
				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F3);
				startSubtitles();
			}
			else if( msg == RC_f4 ) 
			{
				//
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				showUserMenu(SNeutrinoSettings::BUTTON_F4);
				startSubtitles();
			}
#endif			
			else if( msg == RC_dvbsub )
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				// show list only if we have subs
				if(live_channel)
				{
					if(live_channel->getSubtitleCount() > 0)
					{
						CDVBSubSelectMenuHandler tmpDVBSubSelectMenuHandler;
						tmpDVBSubSelectMenuHandler.exec(NULL, "");
					}
				}
				
				startSubtitles();
			}
			else if( msg == (neutrino_msg_t)g_settings.key_audioplayer ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("audioplayer");
				startSubtitles();
			}
			else if( msg == (neutrino_msg_t)g_settings.key_inetradio ) 	// internet radio
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();
	  
				stopSubtitles();
				g_PluginList->startPlugin("internetradio");
				startSubtitles();	
			}			
			else if( msg == (neutrino_msg_t)g_settings.key_movieplayer )	// recordsbrowser
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("movieplayer");
				startSubtitles();			
			}
			else if( msg == (neutrino_msg_t)g_settings.key_moviebrowser )	// moviebrowser
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("moviebrowser");
				startSubtitles();	
			}
			else if( msg == (neutrino_msg_t)g_settings.key_filebrowser )	// filebrowser player
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("mediaplayer");
				startSubtitles();	
			}
			else if( msg == (neutrino_msg_t)g_settings.key_pictureviewer ) 	// picture viewer
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				g_PluginList->startPlugin("picviewer");
				startSubtitles();
			}			
			else if ( CRCInput::isNumeric(msg) && g_RemoteControl->director_mode ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				g_RemoteControl->setSubChannel(CRCInput::getNumericValue(msg));
				
				g_InfoViewer->showSubchan();
				
				startSubtitles();
			}
			else if (CRCInput::isNumeric(msg)) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				channelList->numericZap( msg );
				
				startSubtitles();
			}			
			else if (CRCInput::isNumeric(msg) && (mode == mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL && g_Radiotext->Rass_Show) ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				// convert msg to int
				int QKey = 0;
				if(msg == RC_0)
					QKey = 0;
				else if(msg == RC_1)
					QKey = 1;
				else if(msg == RC_2)
					QKey = 2;
				else if(msg == RC_3)
					QKey = 3;
				else if(msg == RC_4)
					QKey = 4;
				else if(msg == RC_5)
					QKey = 5;
				else if(msg == RC_6)
					QKey = 6;
				else if(msg == RC_7)
					QKey = 7;
				else if(msg == RC_8)
					QKey = 8;
				else if(msg == RC_9)
					QKey = 9;

				//FIXME: think about gallery i.e QKey > 9

				g_Radiotext->RassImage(0, QKey, true);
			}			
			else if((msg == RC_info) || ( msg == NeutrinoMessages::SHOW_INFOBAR ))
			{
				bool show_info = ((msg != NeutrinoMessages::SHOW_INFOBAR) || (g_InfoViewer->is_visible || g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] != 0));
					
				// turn on LCD display
				CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
					
				if(show_info && channelList->getSize()) 
				{
					showInfo();
				}
			}
			else if(msg == (neutrino_msg_t) g_settings.key_pip)
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				stopSubtitles();
				
				// first step show channels from the same TP
				channelList->numericZap( msg );
				
				startSubtitles();
			}
			else 
			{
				if ( msg == RC_home )
				{ 
					if(g_InfoViewer->is_visible)
						g_InfoViewer->killTitle();

  					CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
				}

				handleMsg(msg, data);
			}
		}
		else //other modes
		{
			if( msg == RC_home ) 
			{
				if(g_InfoViewer->is_visible)
					g_InfoViewer->killTitle();

				if( mode == mode_scart ) 
				{
					//wenn VCR Aufnahme dann stoppen
					if(CVCRControl::getInstance()->isDeviceRegistered()) 
					{
						if ((CVCRControl::getInstance()->Device->getDeviceType() == CVCRControl::DEVICE_VCR) &&
						    (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD ||
						     CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE))
						{
							CVCRControl::getInstance()->Stop();
							recordingstatus = 0;
							startNextRecording();
						}
					}
					// Scart-Mode verlassen
					scartMode( false );
				}
			}
			else 
			{
				// hier geht es weiter
				handleMsg(msg, data);
			}
		}
	}
}

// handle msg
int CNeutrinoApp::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int res = 0;

	// zap complete event
	if(msg == NeutrinoMessages::EVT_ZAP_COMPLETE) 
	{
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
			g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, 1);
		}	

		// scrambled timer
		if(scrambled_timer) 
		{
			g_RCInput->killTimer(scrambled_timer);
			scrambled_timer = 0;
		}

		scrambled_timer = g_RCInput->addTimer(10*1000*1000, true);
		
		// select subtitle
		selectSubtitles();
		
		startSubtitles(!g_InfoViewer->is_visible);
	}

	// timer event
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
			
/*
			if(true && (videoDecoder->getBlank() && videoDecoder->getPlayState())) 
			{
				const char * text = _("Scrambled channel");
				HintBox(_("Information"), _(text), g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth (text, true) + 10, 5);
			}
*/

			return messages_return::handled;	
		}
	}

	// handle msg with remotecontrol/Infoviewer/Channellist
	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);
	
	if( res != messages_return::unhandled ) 
	{
		if( ( msg >= RC_WithData ) && ( msg < RC_WithData + 0x10000000 ) )
			delete [] (unsigned char*) data;

		return( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

	// we assume g_CamHandler free/delete data if needed
#if defined (ENABLE_CI)	
	res = g_CamHandler->handleMsg(msg, data);
	if( res != messages_return::unhandled ) 
	{
		return(res & (0xFFFFFFFF - messages_return::unhandled));
	}
#endif	

	// handle Keys
	if( msg == RC_ok || msg == RC_sat || msg == RC_favorites)
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

			dprintf(DEBUG_NORMAL, "\nCNeutrinoApp::handleMsg: ZAP START:\n");

			if(bouquetList->Bouquets.size()) 
			{
				old_num = bouquetList->Bouquets[old_b]->channelList->getActiveChannelNumber();
			}

			if( msg == RC_ok ) 
			{
				if(bouquetList->Bouquets.size() && bouquetList->Bouquets[old_b]->channelList->getSize() > 0)
					nNewChannel = bouquetList->Bouquets[old_b]->channelList->exec();	//with ZAP!
				else
					nNewChannel = bouquetList->exec(true, true);	//with zap
			}
			else if(msg == RC_sat) 
			{
				setChannelMode(LIST_MODE_SAT, mode);
				nNewChannel = bouquetList->exec(true, true);
			}
			else if(msg == RC_favorites) 
			{
				setChannelMode(LIST_MODE_FAV, mode);
				nNewChannel = bouquetList->exec(true, true);
			}
_repeat:
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::handleMsg: ZAP RES: nNewChannel %d\n", nNewChannel);

			if(nNewChannel == -1) // on cancel
			{
				// restore orig. bouquet and selected channel on cancel
				setChannelMode(old_mode, mode);
				bouquetList->activateBouquet(old_b, false);
				
				if(bouquetList->Bouquets.size())
					bouquetList->Bouquets[old_b]->channelList->setSelected(old_num - 1);
				
				startSubtitles(mode == mode_tv);
			}
			else if(nNewChannel == -3) // list mode changed
			{ 
				nNewChannel = bouquetList->exec(true, true);
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
	else if (msg == RC_standby ) 
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
						
						if (lmsg == RC_timeout)
							break;

						gettimeofday(&endtime, NULL);
						seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
						if (endtime.tv_usec < standby_pressed_at.tv_usec)
							seconds--;
						
						if (seconds >= 1) 
						{
							if ( lmsg == RC_standby )
								new_msg = NeutrinoMessages::SHUTDOWN;
							break;
						}
					}
				}
			}
			
			g_RCInput->postMsg(new_msg, 0);
			return messages_return::cancel_all | messages_return::handled;
		}
		else if (standby_pressed_at.tv_sec != 0)
		{
			struct timeval endtime;
			gettimeofday(&endtime, NULL);
			time_t seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
			if (endtime.tv_usec < standby_pressed_at.tv_usec)
				seconds--;
				
			if (seconds >= 1) 
			{
				g_RCInput->postMsg(NeutrinoMessages::SHUTDOWN, 0);
				return messages_return::cancel_all | messages_return::handled;
			}
		}
	}
	else if ( (msg == RC_plus) || (msg == RC_minus) )
	{
		setVolume(msg, (mode != mode_scart));
		return messages_return::handled;
	}
	else if( msg == RC_spkr ) 
	{
		if( mode == mode_standby ) 
		{
			//switch lcd off/on
			CVFD::getInstance()->togglePower();
		}
		else 
		{
			//mute
			audioMute( !current_muted, true);
		}
		
		return messages_return::handled;
	}	
	// event messages
	else if (msg == NeutrinoMessages::EVT_VOLCHANGED) 
	{
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_MUTECHANGED ) 
	{
		delete[] (unsigned char*) data;

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_SERVICESCHANGED ) 
	{
		channelsInit();

		channelList->adjustToChannelID(live_channel_id);
		
		if(old_b_id >= 0) 
		{
			bouquetList->activateBouquet(old_b_id, false);
			old_b_id = -1;
			g_RCInput->postMsg(RC_ok, 0);
		}
	}
	else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED ) 
	{
		channelsInit();

		channelList->adjustToChannelID(live_channel_id);

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_RECORDMODE ) 
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
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::RECORD_START) 
	{
		if(autoshift) 
		{
			stopAutoRecord();
			recordingstatus = 0;
			timeshiftstatus = 0;
		}

		puts("CNeutrinoApp::handleMsg: executing " NEUTRINO_RECORDING_START_SCRIPT ".");
		if (system(NEUTRINO_RECORDING_START_SCRIPT) != 0)
			perror(NEUTRINO_RECORDING_START_SCRIPT " failed");

		if (nextRecordingInfo != NULL)
			delete[] (unsigned char *) nextRecordingInfo;

		nextRecordingInfo = (CTimerd::RecordingInfo *) data;
		startNextRecording();

		return messages_return::handled | messages_return::cancel_all;
	}
	else if( msg == NeutrinoMessages::RECORD_STOP) 
	{
		if(((CTimerd::RecordingStopInfo*)data)->eventID == recording_id)
		{ 
			if (CVCRControl::getInstance()->isDeviceRegistered()) 
			{
				if ((CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD) || (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE ))
				{
					CVCRControl::getInstance()->Stop();
					
					recordingstatus = 0;
					autoshift = 0;
					
					if(timeshiftstatus)
					{
						// set timeshift status to false
						timeshiftstatus = 0;
					}
				}
				else
					printf("CNeutrinoApp::handleMsg: wrong state\n");
			}
			else
				puts("CNeutrinoApp::handleMsg: no recording devices");

			startNextRecording();

			if ( recordingstatus == 0 ) 
			{
				puts("CNeutrinoApp::handleMsg: executing " NEUTRINO_RECORDING_ENDED_SCRIPT ".");
				if (system(NEUTRINO_RECORDING_ENDED_SCRIPT) != 0)
					perror(NEUTRINO_RECORDING_ENDED_SCRIPT " failed");

				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false);
			}
		}
		else if(nextRecordingInfo != NULL) 
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID) 
			{
				delete[] (unsigned char *) nextRecordingInfo;
				nextRecordingInfo = NULL;
			}
		}
		
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_PMT_CHANGED) 
	{
		res = messages_return::handled;

		return res;
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
			
			channelList->zapTo_ChannelID(eventinfo->channel_id);
		}
		
		delete[] (unsigned char*) data;
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

				CTimerd::responseGetTimer &timer = tmpTimerList[0];

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
			HintBox(_("Information"), _(name.c_str()) );
		}

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_RECORD) 
	{
		system(NEUTRINO_RECORDING_TIMER_SCRIPT);

		char * lrecDir = ((CTimerd::RecordingInfo*)data)->recordingDir;

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
			recordingstatus = 0;
			timeshiftstatus = 0;
		}

		delete[] (unsigned char*) data;
		
		if( mode != mode_scart )
			HintBox(_("Information"), _("Recording starts in a few minutes"));
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER) 
	{
		if( mode != mode_scart )
			HintBox(_("Information"), _("Sleeptimer in 1 min") );
		
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
	else if( msg == NeutrinoMessages::STANDBY_TOGGLE ) 
	{
		standbyMode( !(mode & mode_standby) );
		g_RCInput->clearRCMsg();
		
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
	else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN) 
	{
		if( mode != mode_scart )
			skipShutdownTimer = (MessageBox(_("Information"), _("Box will shutdown in 1 min.\nCancel Sutdown ?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 5) == mbrYes);
	}
	else if( msg == NeutrinoMessages::SHUTDOWN ) 
	{
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
			HintBox(_("Information"), (const char *) data); // UTF-8
		
		delete[] (unsigned char*) data;
		
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_EXTMSG) 
	{
		if (mode != mode_scart)
			MessageBox(_("Information"), (const char *) data, mbrBack, mbBack, NEUTRINO_ICON_INFO); // UTF-8
			
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_RECORDING_ENDED) 
	{
		if (mode != mode_scart) 
		{
			std::string msgbody;
			if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_BUFFER_OVERFLOW)
				msgbody = "The recording was aborted,\nsince the data could not be written fast enough.";
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_OPEN_FAILURE)
				msgbody = "The recording was aborted,\nbecause the target file could not be opened.";
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_FAILURE)
				msgbody = "The recording was aborted,\nsince an error occured during the writing process.";
			else
				goto skip_message;

			MessageBox(_("Information"), _(msgbody.c_str()), mbrBack, mbBack, NEUTRINO_ICON_INFO, MESSAGEBOX_WIDTH, 5);

skip_message:
			;
		}
		
		if ((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_IDLE) 
		{
			// restart recording
			//FIXME doGuiRecord((*(stream2file_status2_t *) data).dir);
			//changeNotify(_("start"), data);
		}

		delete[] (unsigned char*) data;
		
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REMIND) 
	{
		std::string text = (char*)data;
		std::string::size_type pos;
		while((pos=text.find('/'))!= std::string::npos)
		{
			text[pos] = '\n';
		}
		
		if( mode != mode_scart )
			MessageBox(_("Reminder"), text.c_str(), mbrBack, mbBack, NEUTRINO_ICON_INFO); // UTF-8
			
		delete[] (unsigned char*) data;
		
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
	
				//FIXME: this sucks when no DVB device is present
				if (g_settings.radiotext_enable && g_Radiotext)
					g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);				
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
	else if( msg == NeutrinoMessages::VCR_ON ) 
	{
		if( mode != mode_scart ) 
		{
			scartMode( true );
		}
		else
		{
			CVFD::getInstance()->setMode(CVFD::MODE_SCART);
		}
	}	
	else if( msg == NeutrinoMessages::VCR_OFF ) 
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
		
		delete[] (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_SERVICES_UPD) 
	{
		channelsInit();

		channelList->adjustToChannelID(live_channel_id);
	}

	if ((msg >= RC_WithData) && (msg < RC_WithData + 0x10000000))
		delete[] (unsigned char*) data;

	//dprintf(DEBUG_DEBUG, "CNeutrinoApp::handleMsg: messages_return::unhandled\n");

	return messages_return::unhandled;
}

// exit run
void CNeutrinoApp::exitRun(int retcode, bool save)
{ 
	// break silently autotimeshift
	if(autoshift) 
	{
		stopAutoRecord();
		recordingstatus = 0;
		timeshiftstatus = 0;
	}
	
	//
	if (!recordingstatus || MessageBox(_("Information"), _("You really want to to stop record ?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes)  
	{
		// stop recording
		if(recordingstatus) 
		{
			CVCRControl::getInstance()->Stop();
			CTimerd::getInstance()->stopTimerEvent(recording_id);
		}

		if(retcode > RESTART)
		{
			// vfd mode shutdown
			CVFD::getInstance()->setMode(CVFD::MODE_SHUTDOWN);
		}

		// stop playback
		CZapit::getInstance()->stopPlayBack();

		if(retcode > RESTART)
		{
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

		// stop dvbsub
		dvbsub_close();

		// stop txt
		tuxtxt_stop();
		tuxtxt_close();

		tuxtx_stop_subtitle();
		
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::exitRun: entering off state (retcode:%d)\n", retcode);
			
		// stop nhttpd		
		Cyhttpd::getInstance()->Stop();
		
		// stop streamts
		CStreamTS::getInstance()->Stop();	

		// stop timerd	  
		CTimerd::getInstance()->Stop();		

		// stop sectionsd
		CSectionsd::getInstance()->Stop();

		// zapit stop	
		CZapit::getInstance()->Stop();
		
		//
#if !defined (USE_OPENGL)		
		time_t t = time(NULL);
		
		proc_put("/proc/stb/fp/rtc", t);

		struct tm *lt = localtime(&t);
		struct tm *gt = gmtime(&t);
		int offset = (lt->tm_hour - gt->tm_hour) * 3600;

		proc_put("/proc/stb/fp/rtc_offset", offset);
#endif		

#if defined (PLATFORM_COOLSTREAM)
		CVFD::getInstance()->Clear();
		cs_deregister_messenger();
		cs_api_exit();
#endif
			
		//
		if(playback)
			delete playback;
			
		if (g_RCInput != NULL)
			delete g_RCInput;
			
		if(g_RemoteControl)
			delete g_RemoteControl;
			
		if (g_EpgData)
			delete g_EpgData;
			
		if (g_EventList)
			delete g_EventList;
			
		if(g_fontRenderer)
			delete g_fontRenderer;
			
		if (frameBuffer != NULL)
			delete frameBuffer;

		dprintf(DEBUG_NORMAL, ">>> CNeutrinoApp::exitRun: Good bye (retcode: %d) <<<\n", retcode);
		
#if defined (USE_OPENGL)		
		if(retcode == RESTART)
		{		  
			execvp(global_argv[0], global_argv); // no return if successful
		}
#endif		
		
		_exit(retcode);	
	}
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
			g_RCInput->getMsg(&msg, &data, 300); // 30 secs
			
			if (( msg == RC_timeout ) || (msg == NeutrinoMessages::EVT_SI_FINISHED)) 
			{
				delete [] (unsigned char*) data;
				break;
			}
		}
	}
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::saveEpg: Saving EPG to %s finished\n", g_settings.epg_dir.c_str());
}

// mute
void CNeutrinoApp::audioMute( int newValue, bool isEvent )
{
	int dx = 32;
	int dy = 32;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_MUTE, &dx, &dy);

	int x = g_settings.screen_EndX - 10 - dx;
	int y = g_settings.screen_StartY + 10;

	//FIXME:
	if(mute_pixbuf == NULL)
	{
		mute_pixbuf = new fb_pixel_t[dx*dy];
		
		frameBuffer->saveScreen(x, y, dx, dy, mute_pixbuf);	
	}

#if ENABLE_LCD
	CVFD::getInstance()->setMuted(newValue);
#endif

	current_muted = newValue;

	dprintf(DEBUG_NORMAL, "CNeutrinoApp::audioMute: current_muted %d new %d isEvent: %d\n", current_muted, newValue, isEvent);
	
	CZapit::getInstance()->muteAudio(current_muted);

	if( isEvent && ( mode != mode_scart ) && ( mode != mode_audio) && ( mode != mode_pic))
	{
		if( current_muted ) 
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, x, y);
		}
		else
		{
			if( mute_pixbuf) 
			{
				frameBuffer->restoreScreen(x, y, dx, dy, mute_pixbuf);
	
				delete [] mute_pixbuf;
				mute_pixbuf = NULL;
			}
			else
				frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
		}
		
		frameBuffer->blit();	
	}
}

// set vol
void CNeutrinoApp::setvol(int vol)
{
	if(audioDecoder)
		audioDecoder->setVolume(vol, vol);
}

// set volume
void CNeutrinoApp::setVolume(const neutrino_msg_t key, const bool bDoPaint, bool nowait)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp:setVolume:\n");
	
	neutrino_msg_t msg = key;

	int dx = 296;	//256 (16*16) for vulme bar + 40 for volume digits
	int dy = 32;

	int x = frameBuffer->getScreenX() + 10;
	int y = frameBuffer->getScreenY() + 10;

	current_volume = g_settings.current_volume;
	
	int sw = frameBuffer->getScreenWidth();
	int sh = frameBuffer->getScreenHeight();
	
	int a_step = atoi(g_settings.audio_step);
	
	switch( g_settings.volume_pos )
	{
		case 0:// upper right
			x = sw - dx - 6;
			break;
			
		case 1:// upper left
			break;
			
		case 2:// bottom left
			y = sh - dy;
			break;
			
		case 3:// bottom right
			x = sw - dx;
			y = sh - dy;
			break;
			
		case 4:// center default
			x = ((sw - dx) / 2) + x;
			break;
			
		case 5:// center higher
			x = ((sw - dx) / 2) + x;
			y = sh - sh/15;
			break;
	}
	
	g_volscale = new CProgressBar(x + dy+ (dy/4), y +(dy/4), 200, 15);

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
		frameBuffer->paintBoxRel(x, y , dx, dy, COL_MENUCONTENT_PLUS_0, g_settings.Head_radius | g_settings.Foot_radius, CORNER_ALL);
		
		// vol box aussen
		frameBuffer->paintBoxRel(x + dy + dy/4 - 2, y + dy/4 - 2, dy*25/4 + 4, dy/2 + 4, COL_MENUCONTENT_PLUS_3);
		
		// vol box innen
		frameBuffer->paintBoxRel(x + dy + dy/4, y + dy/4, dy*25/4, dy/2,   COL_MENUCONTENT_PLUS_1);
		
		//icon
		frameBuffer->paintIcon(NEUTRINO_ICON_VOLUME, x+dy/2,y + (dy/4), 0, COL_MENUCONTENT_PLUS_0);

		g_volscale->reset();

		//
		g_volscale->paint(current_volume);

		char p1[4]; // 3 digits + '\0'
		sprintf(p1, "%3d", current_volume);

		// erase the numbers
		frameBuffer->paintBoxRel(x + dx - 50, y , 40, dy, COL_MENUCONTENT_PLUS_0);

		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p1, COL_MENUCONTENT );

		frameBuffer->blit();
	}

	neutrino_msg_data_t data;

	uint64_t timeoutEnd;

	do {
		if (msg <= RC_MaxRC) 
		{
			if ( msg == RC_plus ) 
			{ 
				if (g_settings.current_volume < 100 - a_step )
					g_settings.current_volume += a_step;
				else
					g_settings.current_volume = 100;
			}
			else if ( msg == RC_minus ) 
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

			setvol(g_settings.current_volume);	
			
			//FIXME
			if (current_muted && msg == RC_plus)
				audioMute(0, true);

			timeoutEnd = CRCInput::calcTimeoutEnd(nowait ? 5 : 10);
		}
		else if (msg == NeutrinoMessages::EVT_VOLCHANGED) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(5);
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
				g_volscale->paint(current_volume);

				char p[4]; // 3 digits + '\0'
				sprintf(p, "%3d", current_volume);

				// erase the numbers
				frameBuffer->paintBoxRel(x + dx - 50, y , 40, dy, COL_MENUCONTENT_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(x + dx - 45, y + dy/2 + 14, 36, p, COL_MENUCONTENT);
				
				frameBuffer->blit();

				//FIXME
				if (mode != mode_scart && mode != mode_pic && (g_settings.current_volume == 0) )
					audioMute(true, true);
			}
		}

#if ENABLE_LCD
		CVFD::getInstance()->showVolume(g_settings.current_volume);
#endif

		if (msg != RC_timeout) 
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		}

		frameBuffer->blit();	
	} while (msg != RC_timeout);

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
	
	delete g_volscale;
	g_volscale = NULL;
}

// tv mode
void CNeutrinoApp::tvMode( bool rezap )
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::tvMode: rezap %s\n", rezap ? "yes" : "no");
	
	if(mode == mode_radio ) 
	{	  
		if (g_settings.radiotext_enable && g_Radiotext) 
		{
			videoDecoder->finishShowSinglePic();
			
			delete g_Radiotext;
			g_Radiotext = NULL;
		}		

#if defined (ENABLE_LCD)
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false );
#endif		

		CVFD::getInstance()->ShowIcon(VFD_ICON_RADIO, false);

		startSubtitles(!rezap);
	}

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

        if( mode == mode_tv ) 
	{
                return;
	}
	else if( mode == mode_scart )
	{
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);
#endif		
	}
	else if( mode == mode_standby ) 
	{
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
		
		// set video standby
#if !defined (PLATFORM_COOLSTREAM)		
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);
#endif		
	}

	bool stopauto = (mode != mode_ts);	
	mode = mode_tv;
	
	if(stopauto && autoshift) 
	{
		stopAutoRecord();
		recordingstatus = 0;
		timeshiftstatus = 0;
	}	

	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();
	frameBuffer->blit();

	g_RemoteControl->tvMode();
	setChannelMode(g_settings.channel_mode, mode);

	// rezap
	if( rezap ) 
	{
		firstChannel();
		channelList->tuned = 0xfffffff;
		channelList->zapTo( firstchannel.channelNumber - 1 );
	}
}

// Radio Mode
void CNeutrinoApp::radioMode( bool rezap)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: rezap %s\n", rezap ? "yes" : "no");

	if(mode == mode_tv ) 
	{
#if defined (ENABLE_LCD)	  
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );
#endif		

		stopSubtitles();
	}

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	if( mode == mode_radio ) 
	{
		return;
	}
	else if( mode == mode_scart ) 
	{
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
			videoDecoder->SetInput(INPUT_ENCODER);
#endif		
	}
	else if( mode == mode_standby ) 
	{	  
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

		// set video standby
#if !defined (PLATFORM_COOLSTREAM)
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);
#endif		
	}

	mode = mode_radio;

	if(autoshift) 
	{
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::radioMode: standby on: autoshift ! stopping ...\n");
		stopAutoRecord();
		recordingstatus = 0;
		timeshiftstatus = 0;
	}
	
	//
	frameBuffer->useBackground(false);
	frameBuffer->loadBackgroundPic("radiomode.jpg");
	frameBuffer->blit();

	g_RemoteControl->radioMode();
	setChannelMode( g_settings.channel_mode, mode);
	
	if (g_settings.radiotext_enable) 
	{
		if(g_Radiotext == NULL)
			g_Radiotext = new CRadioText;
	}
	
	if( rezap ) 
	{
		firstChannel();
		channelList->tuned = 0xfffffff;
		channelList->zapTo( firstchannel.channelNumber - 1 );
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

		CVFD::getInstance()->setMode(CVFD::MODE_SCART);

		lastMode = mode;
		mode = mode_scart;
		
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
		{
			videoDecoder->SetInput(INPUT_SCART);
			videoDecoder->SetStandby(STANDBY_ON);
		}
#endif		
	} 
	else 
	{
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
		{
			videoDecoder->SetInput(INPUT_ENCODER);
			videoDecoder->SetStandby(STANDBY_OFF);
		}
#endif
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
			recordingstatus = 0;
			timeshiftstatus = 0;
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
	
#if defined (PLATFORM_COOLSTREAM)
		CVFD::getInstance()->Clear();
#endif		
		
		// show time in vfd
		CVFD::getInstance()->setMode(CVFD::MODE_STANDBY);
		
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_ON);
#endif		
		
		// zapit standby
		if(!recordingstatus && !timeshiftstatus)
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

		//run script
		puts("CNeutrinoApp::standbyMode: executing " NEUTRINO_ENTER_STANDBY_SCRIPT ".");
		
		if (system(NEUTRINO_ENTER_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_ENTER_STANDBY_SCRIPT " failed");

		lastMode = mode;
		mode = mode_standby;

		frameBuffer->setActive(false);
			
		// set fan off
#if !ENABLE_LCD		
		CVFD::getInstance()->setFan(false);
#endif		
	} 
	else 
	{	
		// set fan on
#if !ENABLE_LCD		
		CVFD::getInstance()->setFan(true);
#endif		

		// set fb active
		frameBuffer->setActive(true);

		puts("CNeutrinoApp::standbyMode: executing " NEUTRINO_LEAVE_STANDBY_SCRIPT ".");
		if (system(NEUTRINO_LEAVE_STANDBY_SCRIPT) != 0)
			perror(NEUTRINO_LEAVE_STANDBY_SCRIPT " failed");

		// vfd mode
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
		
#if !defined (PLATFORM_COOLSTREAM)	  
		if(videoDecoder)
			videoDecoder->SetInput(STANDBY_OFF);
#endif		
				
		// setmode?tv/radio
		mode = mode_unknown;

		// zapit startplayback
		CZapit::getInstance()->setStandby(false);

		// this is buggy don't respect parentallock
		if(!recordingstatus && !timeshiftstatus)
			CZapit::getInstance()->startPlayBack(live_channel);


		CSectionsd::getInstance()->pauseScanning(false);
		CSectionsd::getInstance()->setServiceChanged(live_channel_id, true );

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
		
		g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	}
}

// exec, menuitem callback (shutdown)
int CNeutrinoApp::exec(CMenuTarget * parent, const std::string & actionKey)
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
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
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
		if (MessageBox(_("Information"), _("Save settings now?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			saveSetup(NEUTRINO_SETTINGS_FILE);

			tuxtxt_close();
			
			zapitCfg.saveLastChannel = g_settings.uselastchannel;
			CZapit::getInstance()->setZapitConfig(&zapitCfg);

			//
			HintBox(_("Information"), _("Saving settings now, please be patient."));
		}
	}
	else if (actionKey == "saveskinsettings")
	{
		if (MessageBox(_("Information"), _("Save Skin settings now?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			// fetch skin config file
			std::string skinConfig = CONFIGDIR "/skins/";
			skinConfig += g_settings.preferred_skin.c_str();
			skinConfig += "/";
			skinConfig += g_settings.preferred_skin.c_str();
			skinConfig += ".config";
				
			saveSkinConfig(skinConfig.c_str());
				
			//tuxtxt_close();
				
			HintBox(_("Information"), _("Saving Skin settings now, please be patient.\n this needs Neutrino restart."));
		}
	}
	else if (actionKey == "defaultskinsettings")
	{
		if (MessageBox(_("Information"), _("load default skin configuration?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			std::string skinDefaultConfigFile = CONFIGDIR "/skins/";
			skinDefaultConfigFile += g_settings.preferred_skin.c_str();
			skinDefaultConfigFile += "/";
			skinDefaultConfigFile += "default.config";
			
			readSkinConfig(skinDefaultConfigFile.c_str());
		}
	}
	else if(actionKey == "reloadchannels")
	{
		if (MessageBox(_("Information"), _("do you want to reload channel lists?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			HintBox(_("Information"), _("Reloading channel lists, please be patient."));
			CZapit::getInstance()->reinitChannels();
		}
	}
	else if (actionKey == "reloadepg")
	{
		if (MessageBox(_("Information"), _("do you want to reload EPG?"), mbrNo, mbYes | mbNo, NULL, MESSAGEBOX_WIDTH, 30, true) == mbrYes) 
		{
			HintBox(_("Information"), _("Reloading EPG, please be patient."));
			readEPG();
		}
	}
	else if (actionKey == "delete_zapit")
	{
		my_system(3, "/bin/sh", "-c", "rm -f " CONFIGDIR "/zapit/*.xml");
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
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "features")
	{
		if(parent)
			parent->hide();
		
		stopSubtitles();
		showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
		startSubtitles();
				
		return RETURN_REPAINT;	
	}
	if(actionKey == "plugins")
	{
		if(parent)
			parent->hide();
		
		stopSubtitles();
		
		CPluginList * pluginList = new CPluginList();
		pluginList->exec(NULL, "");
		delete pluginList;
		pluginList = NULL;

		startSubtitles();
				
		return RETURN_REPAINT;
	}

	return returnval;
}

// changeNotify - features menu recording start / stop
bool CNeutrinoApp::changeNotify(const std::string& OptionName, void */*data*/)
{
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::changeNotify\n");

	if ( (OptionName == _("Start"))  || (OptionName == _("Recording")) )
	{
		if(g_RemoteControl->is_video_started) 
		{
			bool res = doGuiRecord(NULL, (OptionName == _("Recording")));
			return res;
		}
		else 
		{
			recordingstatus = 0;
			return false;
		}
	}

	return false;
}

// stop subtitle
void CNeutrinoApp::stopSubtitles()
{
	if (IS_WEBTV(live_channel_id))
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
		tuxtx_pause_subtitle(true, live_fe?live_fe->fenumber:0 );
		
		frameBuffer->paintBackground();

		frameBuffer->blit();
	}
}

// start subtitle
void CNeutrinoApp::startSubtitles(bool show)
{
	if (IS_WEBTV(live_channel_id))
		return;
	
	dprintf(DEBUG_NORMAL, "CNeutrinoApp::startSubtitles\n");
	
	if(!show)
		return;
	
	//dvbsub
	dvbsub_start(dvbsub_getpid());
	
	// tuxtxt
	tuxtx_pause_subtitle( false, live_fe?live_fe->fenumber:0 );
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
						printf("CNeutrinoApp::selectSubtitles: found DVB %s, pid %x\n", sd->ISO639_language_code.c_str(), sd->pId);
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
						printf("CNeutrinoApp::selectSubtitles: found TTX %s, pid %x page %03X\n", sd->ISO639_language_code.c_str(), sd->pId, page);

						tuxtx_stop_subtitle();

						tuxtx_set_pid(sd->pId, page, (char *) sd->ISO639_language_code.c_str());
						return;
					}
				}
			}
		}
	}
}

// select NVOD
void CNeutrinoApp::selectNVOD()
{
	dprintf(DEBUG_NORMAL, "selectNVOD\n");
	
        if (!(g_RemoteControl->subChannels.empty()))
        {
                //
                CWidget* widget = NULL;
                ClistBox* NVODSelector = NULL;
                
                widget = CNeutrinoApp::getInstance()->getWidget("nvodselect");
                
                if (widget)
                {
			NVODSelector = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
		}
		else
                {
			NVODSelector = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
			NVODSelector->setWidgetMode(MODE_SETUP);
			NVODSelector->enableShrinkMenu();
							
			NVODSelector->enablePaintHead();
			NVODSelector->setTitle(g_RemoteControl->are_subchannels ? _("Select Subservice") : _("Select starting-time"), NEUTRINO_ICON_VIDEO);

			NVODSelector->enablePaintFoot();
								
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
								
			NVODSelector->setFootButtons(&btn);
			
			//
			widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			widget->name = "nvodselect";
			widget->setMenuPosition(MENU_POSITION_CENTER);
			widget->addWidgetItem(NVODSelector);
		}
		
		NVODSelector->clearItems();

		//
                if(getNVODMenu(NVODSelector))
                        widget->exec(NULL, "");		
        }
}

// option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

bool CNeutrinoApp::getNVODMenu(ClistBox* menu)
{	
	//
        if (g_RemoteControl->subChannels.empty())
                return false;
                
	menu->clearItems();

        int count = 0;
        char nvod_id[5];

        for( CSubServiceListSorted::iterator e = g_RemoteControl->subChannels.begin(); e != g_RemoteControl->subChannels.end(); ++e)
        {
		sprintf(nvod_id, "%d", count);

		if( !g_RemoteControl->are_subchannels ) 
		{
			char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
			char nvod_s[100];
			struct  tm *tmZeit;

			tmZeit = localtime(&e->startzeit);
			sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

			time_t endtime = e->startzeit + e->dauer;
			tmZeit = localtime(&endtime);
			sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

			time_t jetzt = time(NULL);
			if(e->startzeit > jetzt) 
			{
				int mins = (e->startzeit - jetzt)/ 60;
				sprintf(nvod_time_x, _("(starting in %d min)"), mins);
			}
			else if( (e->startzeit <= jetzt) && (jetzt < endtime) ) 
			{
				int proz = (jetzt - e->startzeit)*100/ e->dauer;
				sprintf(nvod_time_x, _("(%d%% over)"), proz);
			}
			else
				nvod_time_x[0] = 0;

			sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
			menu->addItem(new CMenuForwarder(nvod_s, true, NULL, NVODChanger, nvod_id), (count == g_RemoteControl->selected_subchannel));
		} 
		else 
		{
			if (count == 0)
				menu->addItem(new CMenuForwarder(Latin1_to_UTF8(e->subservice_name.c_str()).c_str(), true, NULL, NVODChanger, nvod_id, RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
			else
				menu->addItem(new CMenuForwarder(Latin1_to_UTF8(e->subservice_name.c_str()).c_str(), true, NULL, NVODChanger, nvod_id, CRCInput::convertDigitToKey(count)), (count == g_RemoteControl->selected_subchannel));
		}

		count++;
	}

	if( g_RemoteControl->are_subchannels ) 
	{
		menu->addItem(new CMenuSeparator(LINE));
		CMenuOptionChooser* oj = new CMenuOptionChooser(_("Direct-Mode"), &g_RemoteControl->director_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
		menu->addItem(oj);
	}

	return true;
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
			CStreamTS::getInstance()->Stop();	

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

// main function
int main(int argc, char *argv[])
{
	// build date
	printf(">>> neutrino2 (compiled %s %s) <<<\n", __DATE__, __TIME__);

	// sighandler
        signal(SIGTERM, sighandler);
        signal(SIGINT, sighandler);
        signal(SIGHUP, SIG_IGN);
	signal(SIGHUP, sighandler);
	signal(SIGPIPE, SIG_IGN);
	
	// initlocale
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
	
	dprintf(DEBUG_NORMAL, "main: gst initialized\n");
#endif

	// set python path
#if ENABLE_PYTHON
	setenv("PYTHONPATH", DATADIR "/python", 0);
#endif

	return CNeutrinoApp::getInstance()->run(argc, argv);
}

