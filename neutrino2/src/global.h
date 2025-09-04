//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: global.h 04092025 mohousch Exp $
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

#ifndef __neutrino_global_h__
#define __neutrino_global_h__

#include <stdio.h>

//
#include <zapit/zapit.h>
#include <zapit/bouquets.h>

//
#include <sectionsd/sectionsd.h>
#include <timerd/timerd.h>

#include <driver/gdi/fontrenderer.h>
#include <driver/rcinput.h>

#include <driver/lcdd.h>

#include <system/localize.h>
#include <system/settings.h>

#include <gui/epgview.h>
#include <gui/infoviewer.h>
#include <gui/eventlist.h>

#include <driver/radiotext.h>

#include <src/daemonc/remotecontrol.h>

#include <lib/libdvbapi/playback_cs.h>


#ifndef NEUTRINO_CPP
#define NEUTRINO_CPP 			extern
#endif

#define NEUTRINO_SETTINGS_FILE          CONFIGDIR "/neutrino2.conf"
#define NEUTRINO_SCAN_SETTINGS_FILE     CONFIGDIR "/scan.conf"

// globals
NEUTRINO_CPP SNeutrinoSettings		g_settings;
NEUTRINO_CPP FBFontRenderClass		*g_fontRenderer;
NEUTRINO_CPP CFont 			*g_Font[FONT_TYPE_COUNT];
NEUTRINO_CPP CRCInput			*g_RCInput;
NEUTRINO_CPP CEpgData			*g_EpgData;
NEUTRINO_CPP CInfoViewer		*g_InfoViewer;
NEUTRINO_CPP EventList			*g_EventList;
NEUTRINO_CPP CLocaleManager		*g_Locale;
NEUTRINO_CPP CRadioText			*g_Radiotext;
NEUTRINO_CPP CPlugins			*g_PluginList;
NEUTRINO_CPP CRemoteControl		*g_RemoteControl;
NEUTRINO_CPP cPlayback			*playback;

#endif /* __neutrino_global_h__ */

