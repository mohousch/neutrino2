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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <global.h>

#include <system/debug.h>
#include <system/settings.h>
#include <system/channellogo.h>
#include <system/set_threadname.h>

// zapit includes
#include <zapit/bouquets.h>

#include <wordexp.h>

#include <cstring>
#include <fstream>
#include <string>
#include <iostream>


extern tallchans allchans;	// defined in bouquets.h

CChannellogo* CChannellogo::getInstance()
{
	static CChannellogo* Channellogo = NULL;
	if(!Channellogo)
		Channellogo = new CChannellogo();
	return Channellogo;
}

// check for logo
bool CChannellogo::checkLogo(t_channel_id logo_id)
{	
        std::string logo_name;
	bool logo_ok = false;
	
	// first png, then jpg, then gif
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += toHexString(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			dprintf(DEBUG_INFO, "CChannellogo::checkLogo: found logo: %s\n", logo_name.c_str());
			break;
		}
	}
	
	return logo_ok;
}

void CChannellogo::getLogoSize(t_channel_id logo_id, int * width, int * height, int * bpp)
{
	std::string logo_name;
	bool logo_ok = false;
	
	// check for logo/convert channelid to logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += toHexString(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}
	
	if(logo_ok)
	{
		// get logo real size
		CFrameBuffer::getInstance()->getSize(logo_name.c_str(), width, height, bpp);
		
		dprintf(DEBUG_INFO, "CChannellogo::getLogoSize: logo: %s (%dx%d) %dbpp\n", logo_name.c_str(), *width, *height, *bpp);
	}
}

// display logo
bool CChannellogo::displayLogo(t_channel_id logo_id, int posx, int posy, int width, int height, bool upscale, bool center_x, bool center_y)
{	
        std::string logo_name;
	bool ret = false;
	bool logo_ok = false;
	
	int logo_w = width;
	int logo_h = height;
	int logo_bpp = 0;
	
	
	// check for logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += toHexString(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}
	
	if(logo_ok)
	{
		// get logo real size
		CFrameBuffer::getInstance()->getSize(logo_name, &logo_w, &logo_h, &logo_bpp);
	
		// scale only PNG logos
		if( logo_name.find(".png") == (logo_name.length() - 4) )
		{
			// upscale
			if(upscale)
			{	
				//rescale logo image
				float aspect = (float)(logo_w) / (float)(logo_h);
					
				if (((float)(logo_w) / (float)width) > ((float)(logo_h) / (float)height)) 
				{
					logo_w = width;
					logo_h = (int)(width / aspect);
				}
				else
				{
					logo_h = height;
					logo_w = (int)(height * aspect);
				}
			}
			
			ret = CFrameBuffer::getInstance()->displayImage(logo_name, center_x?posx + (width - logo_w)/2 : posx, center_y?posy + (height - logo_h)/2 : posy, logo_w, logo_h);
		}
		else
		{
			ret = CFrameBuffer::getInstance()->displayImage(logo_name, posx, posy, width, height);
		}
        }

	return ret;
}

std::string CChannellogo::getLogoName(t_channel_id logo_id)
{
	std::string logo_name = "";
	bool logo_ok = false;
	
	// check for logo/convert channelid to logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 2; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += toHexString(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}

	return logo_name;
}

//
bool CChannellogo::loadWebTVlogos()
{
	dprintf(DEBUG_NORMAL, "CChannellogo::loadWebTVlogos:\n");
	
	if (logo_running)
		return false;

	logo_running = true;
	
	return (OpenThreads::Thread::start() == 0);
}

void CChannellogo::run()
{
	set_threadname(__func__);
	
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
	{
		if (IS_WEBTV(it->second.getChannelID()))
		{
			// download logos
			std::string logo_name;
			logo_name = g_settings.logos_dir;
			logo_name += "/";
			logo_name += toHexString(it->second.getLogoID() & 0xFFFFFFFFFFFFULL);
			logo_name += ".png";
								
			if(access(logo_name.c_str(), F_OK))
			{
				downloadUrl(it->second.getLogoUrl(), logo_name, "", 30);
			}
		}
	}
	
	OpenThreads::Thread::join();
	logo_running = false;
}

