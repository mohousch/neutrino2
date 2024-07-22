/*
	$Id: screenshot.h 21072024 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
	Homepage: http://dbox.cyberphoria.org/

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

#ifndef _screenshot_h_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <sstream>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


class CScreenshot
{
	public:
		typedef enum
		{
			FORMAT_PNG,
			FORMAT_JPG
		} screenshot_format_t;
		
	private:
		screenshot_format_t format;
		std::string filename;
		uint8_t *pixel_data;
		int xres;
		int yres;
		bool extra_osd;
		bool get_osd;
		bool get_video;
		bool scale_to_video;
		FILE *fd;
		
		bool getData();
		bool openFile();
		bool saveFile();

		bool savePng();
		bool saveJpg();
		
	public:
		CScreenshot();
		virtual ~CScreenshot(){};
		
		static CScreenshot *getInstance();
		
		bool dumpFile(const std::string &fname = "", screenshot_format_t fmt = CScreenshot::FORMAT_PNG, bool osd = true, bool video = false, bool scale = false);
};

#endif

