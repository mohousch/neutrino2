
#ifndef __channellogo__
#define __channellogo__

/*
	neutrino2

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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <sstream>

#include <curl/curl.h>
#include <curl/easy.h>

#include <vector>
#include <string>
#include <map>

#include <driver/file.h>
#include <driver/rcinput.h>
#include <driver/framebuffer.h>

#include <gui/widget/icons.h>

#include <system/settings.h>
#include <system/helpers.h>

// zapit types
#include <zapit/zapittypes.h>
#include <zapit/getservices.h>

#include <OpenThreads/Thread>


class CChannellogo : public OpenThreads::Thread
{
	public:
		CChannellogo(){logo_running = false;};
		~CChannellogo(){};
		
		static CChannellogo* getInstance();
		
		bool displayLogo(t_channel_id logo_id, int posx, int posy, int width, int height, bool upscale = false, bool center_x = true, bool center_y = true);
		bool checkLogo(t_channel_id logo_id);
		void getLogoSize(t_channel_id logo_id, int * width, int * height, int * bpp);
		std::string getLogoName(t_channel_id logo_id);
		
		// webtv
		void run();
		bool loadWebTVlogos();
		bool logo_running;
};

#endif

