/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/settings.h,v 1.9 2013/08/18 11:23:30 mohousch Exp $
 *
 * zapit's settings - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __zapit__settings_h__
#define __zapit__settings_h__

#include <config.h>


// settings
#define SERVICES_XML    		CONFIGDIR "/zapit/services.xml"
#define BOUQUETS_XML    		CONFIGDIR "/zapit/bouquets.xml"
#define UBOUQUETS_XML    		CONFIGDIR "/zapit/ubouquets.xml"
#define MYSERVICES_XML			CONFIGDIR "/zapit/myservices.xml"

// getservices
#define SERVICES_TMP    		"/tmp/services.tmp"

// sdt update
#define CURRENTSERVICES_XML     	"/tmp/currentservices.xml"
#define CURRENTSERVICES_TMP     	"/tmp/currentservices.tmp"

// providers
#define CABLES_XML      		CONFIGDIR "/cables.xml"
#define SATELLITES_XML  		CONFIGDIR "/satellites.xml"
#define TERRESTRIALS_XML 		CONFIGDIR "/terrestrial.xml"
#define ATSC_XML                	CONFIGDIR "/atsc.xml"

// zapit/frontend/audio
#define ZAPIT_CONFIGFILE      		CONFIGDIR "/zapit/zapit.conf"
#define SATCONFIG			CONFIGDIR "/zapit/sat.conf"
#define FRONTEND_CONFIGFILE 		CONFIGDIR "/zapit/frontend.conf"
#define VOLUME_CONFIG_FILE 		CONFIGDIR "/zapit/audiovolume.conf"
#define AUDIO_CONFIG_FILE 		CONFIGDIR "/zapit/audio.conf"

#define DVBADAPTER_MAX			2	// needed for twin on generic HW
#define FRONTEND_MAX			4

#define CAMD_UDS_NAME  			"/tmp/camd.socket"

#endif /* __zapit__settings_h__ */
