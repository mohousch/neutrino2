//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: plugin.h 04092025 mohousch Exp $
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

#ifndef TPLUGIN_H
#define TPLUGIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <system/set_threadname.h>

//
#include <neutrino2.h>

#include <sectionsd/sectionsd.h>

#include <timerd/timerd.h>

//
#include <driver/gdi/fontrenderer.h>
#include <driver/gdi/framebuffer.h>
#include <driver/gdi/color.h>

#include <driver/rcinput.h>
#include <driver/radiotext.h>
#include <driver/encoding.h>
#include <driver/rcinput.h>
#include <driver/record.h>
#include <driver/shutdown_count.h>
#include <driver/audioplay.h>
#include <driver/lcdd.h>
#include <driver/file.h>

#include <daemonc/remotecontrol.h>

#include <gui/widget/icons.h>
#include <gui/widget/colorchooser.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/keychooser.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/keyboard_input.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/infobox.h>
#include <gui/widget/textbox.h>
#include <gui/widget/listbox.h>
#include <gui/widget/listframe.h>
#include <gui/widget/framebox.h>
#include <gui/widget/component.h>
#include <gui/widget/widget.h>

#include <gui/bouqueteditor_bouquets.h>
#include <gui/bouqueteditor_chanselect.h>
#include <gui/bouqueteditor_channels.h>
#include <gui/bouqueteditor_bouquets.h>

#include <gui/epgview.h>
#include <gui/infoviewer.h>
#include <gui/eventlist.h>
#include <gui/epgplus.h>
#include <gui/streaminfo.h>
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
#include <gui/dvbsub_select.h>
#include <gui/channel_select.h>
#include <gui/audio_video_select.h>
#include <gui/psisetup.h>
#include <gui/rc_lock.h>
#include <gui/mediaplayer.h>
#include <gui/service_menu.h>
#include <gui/main_setup.h>
#include <gui/power_menu.h>
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
#include <gui/hdd_menu.h>
#include <gui/movieinfo.h>
#include <gui/usermenu.h>
//
#include <gui/mediaplayer.h>
#include <gui/epg_menu.h>

#include <system/localize.h>
#include <system/settings.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>
#include <system/helpers.h>
#include <system/tmdbparser.h>
#include <system/ytparser.h>
#include <system/httptool.h>
#include <system/channellogo.h>
#include <system/weather.h>
#include <system/screenshot.h>

//
#include <zapit/frontend_c.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>
#include <zapit/zapit.h>
#include <zapit/zapittypes.h>

//
#include <libdvbapi/playback_cs.h>
#include <libdvbapi/video_cs.h>
#include <libdvbapi/audio_cs.h>

#include <libnet/libnet.h>


//// globals
extern  SNeutrinoSettings g_settings;
extern  FBFontRenderClass *g_fontRenderer;
extern  CFont * g_Font[FONT_TYPE_COUNT];
extern  CFont * g_SignalFont;
extern  CRCInput *g_RCInput;
extern  CEpgData *g_EpgData;
extern  CInfoViewer *g_InfoViewer;
extern  EventList *g_EventList;
extern CLocaleManager *g_Locale;
extern CRadioText *g_Radiotext;
extern bool has_hdd;					// defined in gui/hdd_menu.cpp
//
extern tallchans allchans;				// defined in zapit.cpp
// tuxtxt
extern int  tuxtxt_stop();
extern void tuxtxt_close();
extern void tuxtx_pause_subtitle(bool pause, bool isEplayer);
extern void tuxtx_stop_subtitle();
extern void tuxtxt_start(int tpid);
extern void tuxtx_set_pid(int pid, int page, const char * cc);
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern int tuxtx_main(int pid, int page, bool isEplayer);

// dvbsub
extern int dvbsub_init();
extern int dvbsub_stop();
extern int dvbsub_close();
extern int dvbsub_start(int pid, bool isEplayer);
extern int dvbsub_pause();
extern int dvbsub_getpid();
extern void dvbsub_setpid(int pid);
extern int dvbsub_terminate();
// streamts thread
extern int streamts_stop;				// defined in streamts.cpp
// zapit thread
extern int zapit_ready;					//defined in zapit.cpp
//extern t_channel_id live_channel_id; 			//defined in zapit.cpp
//extern CZapitChannel * live_channel;			// defined in zapit.cpp
//extern CFrontend * live_fe;
extern CScanSettings * scanSettings;
// sectionsd thread
extern int sectionsd_stop;				// defined in sectionsd.cpp
extern bool timeset;
//Audio/Video Decoder
extern cVideo 		* videoDecoder;		// libdvbapi (video_cs.cpp)
extern cAudio 		* audioDecoder;		// libdvbapi (audio_cs.cpp)
/* bouquets lists */
extern CBouquetList   		* bouquetList; 				//current bqt list
extern CBouquetList   		* TVbouquetList;
extern CBouquetList   		* TVsatList;
extern CBouquetList   		* TVfavList;
extern CBouquetList   		* TVallList;
//
extern CBouquetList   		* RADIObouquetList;
extern CBouquetList   		* RADIOsatList;
extern CBouquetList   		* RADIOfavList;
extern CBouquetList   		* RADIOallList;
//
extern CPlugins       		* g_PluginList;
extern CRemoteControl 		* g_RemoteControl;
extern SMSKeyInput 		* c_SMSKeyInput;	//defined in filebrowser and used in ChanneList
extern CPictureViewer 		* g_PicViewer;
//
extern cPlayback *playback;
// record and timeshift
extern bool autoshift;
extern uint32_t shift_timer;
extern uint32_t scrambled_timer;
extern char recDir[255];
extern char timeshiftDir[255];
extern std::string timeshiftMode;

#endif

