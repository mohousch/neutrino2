/*
	$Id: neutrino2_python.i 19.11.2022 mohousch Exp $

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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

%module neutrino2
%{
#define SWIG_COMPILE

#include <config.h>

#include <global.h>
#include <neutrino2.h>
#include <neutrinoMessages.h>
#include <swig_helpers.h>

// libconfigfile
#include <libconfigfile/configfile.h>

// libdvbapi
//#include <libdvbapi/playback_cs.h>
//#include <libdvbapi/audio_cs.h>
//#include <libdvbapi/video_cs.h>
//#include <libdvbapi/dmx_cs.h>

// driver
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/color.h>
#include <driver/rcinput.h>
#include <driver/file.h>
#include <driver/audiofile.h>
#include <driver/audiometadata.h>
#include <driver/encoding.h>
#include <driver/pictureviewer/pictureviewer.h>
#include <driver/audioplay.h>
#include <driver/audiodec/basedec.h>
//#include <driver/stream2file.h>
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

// widget
#include <gui/widget/icons.h>
#include <gui/widget/drawable.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/window.h>
#include <gui/widget/progresswindow.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/listbox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/listframe.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/textbox.h>
#include <gui/widget/infobox.h>
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
%include "carrays.i"
%include "cpointer.i"
/*
%include "std_vector.i"
%include "std_list.i"
%include "std_map.i"
%include "std_pair.i"
%include "std_set.i"
*/
%include "exception.i"

%include <config.h>
%include <src/global.h>
%include <src/neutrino2.h>
%include <src/neutrinoMessages.h>
%include <src/swig_helpers.h>

// libconfigfile
%include <lib/libconfigfile/configfile.h>

// libdvbapi
//%include <lib/libdvbapi/playback_cs.h>
//%include <lib/libdvbapi/audio_cs.h>
//%include <lib/libdvbapi/video_cs.h>
//%include <lib/libdvbapi/dmx_cs.h>

// driver
%include <src/driver/fontrenderer.h>
%include <src/driver/framebuffer.h>
%include <src/driver/color.h>
%include <src/driver/rcinput.h>
%include <src/driver/file.h>
%include <src/driver/audiofile.h>
%include <src/driver/audiometadata.h>
%include <src/driver/encoding.h>
%include <src/driver/pictureviewer/pictureviewer.h>
%include <src/driver/audioplay.h>
%include <src/driver/audiodec/basedec.h>
//%include <src/driver/stream2file.h>
%include <src/driver/vcrcontrol.h>

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

// widget
%include <src/gui/widget/icons.h>
//%include <src/gui/widget/drawable.h>
%include <src/gui/widget/widget_helpers.h>
%include <src/gui/widget/window.h>
%include <src/gui/widget/progresswindow.h>
%include <src/gui/widget/messagebox.h>
%include <src/gui/widget/helpbox.h>
%include <src/gui/widget/listbox.h>
%include <src/gui/widget/hintbox.h>
%include <src/gui/widget/textbox.h>
%include <src/gui/widget/infobox.h>
%include <src/gui/widget/listframe.h>
%include <src/gui/widget/stringinput.h>
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

// zapit
//%include <src/zapit/bouquets.h>
%include <src/zapit/channel.h>
%include <src/zapit/zapittypes.h>
%include <src/zapit/zapit.h>

// sectionsd
%include <src/sectionsd/sectionsdtypes.h>
%include <src/sectionsd/sectionsd.h>

// timerd
%include <src/timerd/timerdtypes.h>
%include <src/timerd/timerd.h>

// deamonc
%include <src/daemonc/remotecontrol.h>







