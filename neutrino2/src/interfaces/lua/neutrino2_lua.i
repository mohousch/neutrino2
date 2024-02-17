/*
	$Id: neutrino2_lua.i 19.11.2022 mohousch Exp $

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

%module neutrino2
%{
#define SWIG_COMPILE

#ifdef assert(x)
#undef asser(x)
#endif

#include <config.h>

#include <global.h>
#include <neutrino2.h>
#include <neutrinoMessages.h>

// libconfigfile
#include <libconfigfile/configfile.h>

// libdvbapi
#include <libdvbapi/playback_cs.h>
#include <libdvbapi/audio_cs.h>
#include <libdvbapi/video_cs.h>
#include <libdvbapi/dmx_cs.h>

// driver
#include <driver/gfx/framebuffer.h>
#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/color.h>
#include <driver/gfx/icons.h>
#include <driver/rcinput.h>
#include <driver/file.h>
#include <driver/audiofile.h>
#include <driver/encoding.h>
#include <driver/pictureviewer.h>
#include <driver/audioplay.h>
#include <driver/vcrcontrol.h>

// system
#include <system/settings.h>
#include <system/debug.h>
#include <system/localize.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>
//#include <system/tmdbparser.h>
//#include <system/ytparser.h>
#include <system/httptool.h>
#include <system/flashtool.h>

#include <system/setting_helpers.h>

// widget
#include <gui/widget/drawable.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/progresswindow.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/listbox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/infobox.h>
#include <gui/widget/listframe.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/textbox.h>
#include <gui/widget/menue.h>
#include <gui/widget/framebox.h>
#include <gui/widget/widget.h>

// gui
#include <gui/movieinfo.h>
#include <gui/audioplayer.h>
#include <gui/movieplayer.h>
#include <gui/pictureviewer.h>
#include <gui/filebrowser.h>
#include <gui/plugins.h>
#include <gui/eventlist.h>
#include <gui/epgplus.h>
#include <gui/epgview.h>
#include <gui/bouquetlist.h>
#include <gui/channellist.h>
#include <gui/dboxinfo.h>
#include <gui/audio_select.h>
#include <gui/channel_select.h>
#include <gui/sleeptimer.h>
#include <gui/streaminfo.h>
#include <gui/timerlist.h>
#include <gui/imageinfo.h>
#include <gui/pluginlist.h>
#include <gui/bedit/bouqueteditor_bouquets.h>
#include <gui/update.h>
#include <gui/cam_menu.h>
#include <gui/hdd_menu.h>
#include <gui/rc_lock.h>
#include <gui/vfdcontroler.h>

//gui/setup
#include <gui/scan_setup.h>
#include <gui/audio_setup.h>
#include <gui/video_setup.h>
#include <gui/lcd_setup.h>
#include <gui/misc_setup.h>
#include <gui/movieplayer_setup.h>
#include <gui/network_setup.h>
#include <gui/osd_setup.h>
#include <gui/parentallock_setup.h>
#include <gui/pictureviewer_setup.h>
#include <gui/psisetup.h>
#include <gui/rc_setup.h>
#include <gui/recording_setup.h>
#include <gui/screensetup.h>
#include <gui/zapit_setup.h>
#include <gui/audioplayer_setup.h>
//
#include <gui/main_setup.h>
#include <gui/epg_menu.h>
#include <gui/power_menu.h>
#include <gui/dboxinfo.h>
#include <gui/service_menu.h>
#include <gui/mediaplayer.h>

// zapit
#include <zapit/bouquets.h>
#include <zapit/channel.h>
#include <zapit/zapittypes.h>
#include <zapit/zapit.h>

// sectionsd
#include <sectionsd/sectionsdtypes.h>
#include <sectionsd/sectionsd.h>

// timerd
#include <timerd/timerdtypes.h>
#include <timerd/timerd.h>

// deamonc
#include <daemonc/remotecontrol.h>
%}

%include "typemaps.i"
%include "std_string.i"
%include "stdint.i"
%include "std_vector.i"
%include "carrays.i"
%include "cpointer.i"
//%include "std_list.i"
%include "std_map.i"
%include "std_pair.i"
//%include "std_set.i"
%include "exception.i"

%include <config.h>
%include <src/global.h>
%include <src/neutrino2.h>
%include <src/neutrinoMessages.h>

// libconfigfile
%include <lib/libconfigfile/configfile.h>

// libdvbapi
%include <lib/libdvbapi/playback_cs.h>
%include <lib/libdvbapi/audio_cs.h>
//%include <lib/libdvbapi/video_cs.h>
%include <lib/libdvbapi/dmx_cs.h>

// driver
%include <src/driver/gfx/fontrenderer.h>
%include <src/driver/gfx/framebuffer.h>
%include <src/driver/gfx/color.h>
%include <src/driver/rcinput.h>
%include <src/driver/file.h>
%include <src/driver/audiofile.h>
%include <src/driver/encoding.h>
%include <src/driver/pictureviewer.h>
%include <src/driver/audioplay.h>
%include <src/driver/vcrcontrol.h>
%include <src/driver/gfx/icons.h>


// system
%include <src/system/settings.h>
%include <src/system/debug.h>
%include <src/system/localize.h>
%include <src/system/setting_helpers.h>
%include <src/system/helpers.h>
//%include <src/system/tmdbparser.h>
//%include <src/system/ytparser.h>
%include <src/system/httptool.h>
%include <src/system/flashtool.h>

%include <src/system/setting_helpers.h>

// widget
%include <src/gui/widget/widget_helpers.h>
%include <src/gui/widget/progresswindow.h>
%include <src/gui/widget/messagebox.h>
%include <src/gui/widget/helpbox.h>
%include <src/gui/widget/listbox.h>
%include <src/gui/widget/hintbox.h>
%include <src/gui/widget/infobox.h>
%include <src/gui/widget/listframe.h>
%include <src/gui/widget/stringinput.h>
%include <src/gui/widget/textbox.h>
%include <src/gui/widget/menue.h>
%include <src/gui/widget/framebox.h>
%include <src/gui/widget/widget.h>

// gui
%include <src/gui/movieinfo.h>
%include <src/gui/audioplayer.h>
%include <src/gui/movieplayer.h>
%include <src/gui/pictureviewer.h>
%include <src/gui/filebrowser.h>
%include <src/gui/plugins.h>
%include <src/gui/eventlist.h>
%include <src/gui/epgplus.h>
%include <src/gui/epgview.h>
%include <src/gui/bouquetlist.h>
%include <src/gui/channellist.h>
%include <src/gui/dboxinfo.h>
%include <src/gui/audio_select.h>
%include <src/gui/channel_select.h>
%include <src/gui/sleeptimer.h>
%include <src/gui/streaminfo.h>
%include <src/gui/timerlist.h>
%include <src/gui/imageinfo.h>
%include <src/gui/pluginlist.h>
%include <src/gui/bedit/bouqueteditor_bouquets.h>
%include <src/gui/update.h>
//%include <src/gui/cam_menu.h>
%include <src/gui/hdd_menu.h>
%include <src/gui/rc_lock.h>
%include <src/gui/vfdcontroler.h>

//gui/setup
%include <src/gui/scan_setup.h>
%include <src/gui/audio_setup.h>
%include <src/gui/video_setup.h>
%include <src/gui/lcd_setup.h>
%include <src/gui/misc_setup.h>
%include <src/gui/movieplayer_setup.h>
%include <src/gui/network_setup.h>
%include <src/gui/osd_setup.h>
%include <src/gui/parentallock_setup.h>
%include <src/gui/pictureviewer_setup.h>
%include <src/gui/psisetup.h>
%include <src/gui/rc_setup.h>
%include <src/gui/recording_setup.h>
%include <src/gui/screensetup.h>
%include <src/gui/zapit_setup.h>
%include <src/gui/audioplayer_setup.h>
//
%include <src/gui/main_setup.h>
%include <src/gui/epg_menu.h>
%include <src/gui/power_menu.h>
%include <src/gui/dboxinfo.h>
%include <src/gui/mediaplayer.h>
%include <src/gui/service_menu.h>

// zapit
//%include <src/zapit/bouquets.h>
%include <src/zapit/channel.h>
%include <src/zapit/zapittypes.h>
//%include <src/zapit/zapit.h>

// sectionsd
%include <src/sectionsd/sectionsdtypes.h>
%include <src/sectionsd/sectionsd.h>

// timerd
%include <src/timerd/timerdtypes.h>
%include <src/timerd/timerd.h>

// deamonc
%include <src/daemonc/remotecontrol.h>






