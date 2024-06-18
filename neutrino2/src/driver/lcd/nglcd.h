/*
	nglcd.h -- Neutrino GraphLCD driver

	Copyright (C) 2012-2014 martii

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __glcd_h__
#define __glcd_h__

#include <string>
#include <vector>
#include <time.h>
#include <string>
#include <sys/types.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>
#pragma GCC diagnostic warning "-Wunused-parameter"

#include <zapit/zapittypes.h>


class nGLCD
{
	private:
		GLCD::cDriver *lcd;
		
		////
		GLCD::cFont font_channel;
		GLCD::cFont font_epg;
		GLCD::cFont font_time;
		GLCD::cFont font_time_standby;
		////
		int fontsize_channel;
		int fontsize_epg;
		int fontsize_time;
		int fontsize_time_standby;
		////
		int percent_channel;
		int percent_time;
		int percent_time_standby;
		int percent_epg;
		int percent_bar;
		int percent_logo;
		int percent_space;
		
		bool fonts_initialized;
		/*
		std::string Channel;
		std::string Epg;
		std::string stagingChannel;
		std::string stagingEpg;
		t_channel_id channel_id;
		int Scale;
		*/
		
	public:
		GLCD::cBitmap *bitmap;
		
		nGLCD();
		~nGLCD();
		
		enum {
			ALIGN_NONE = 0,
			ALIGN_LEFT = 1,
			ALIGN_CENTER = 2,
			ALIGN_RIGHT = 3,
		};
		
		enum {
			REC = 0,
			MUTE = 1,
			TS = 2,
			ECM = 3,
			TIMER = 4,
			DD = 5,
			TXT = 6,
			SUB = 7,
			CAM = 8,
		};
		
		enum {
			CLOCK_OFF = 0,
			CLOCK_SIMPLE = 1,
			CLOCK_LED = 2,
			CLOCK_LCD = 3,
			CLOCK_DIGITAL = 4,
			CLOCK_ANALOG = 5
		};
		
		bool init();
		int GetConfigSize();
		std::string GetConfigName(int);
		
		////
		void updateFonts();
		
		bool showImage(uint32_t *s,
			uint32_t sw, uint32_t sh,
			uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh,
			bool transp = false, bool maximize = false);
		bool showImage(const std::string &filename,
			uint32_t sw, uint32_t sh,
			uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh,
			bool transp = false, bool maximize = false);
		bool showImage(uint64_t channel_id,
			uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh,
			bool transp = false, bool maximize = false);
		bool getBoundingBox(uint32_t *buffer,
			int width, int height,
			int &bb_x, int &bb_y, int &bb_width, int &bb_height);
		void LcdAnalogClock(int posx, int posy, int dia);
		
		bool drawText(int x, int y, int xmax, int text_width, std::string & text, /*GLCD::cFont font,*/ uint32_t color1 = GLCD::cColor::White, uint32_t color2 = GLCD::cColor::Transparent, bool proportional = true, int skipPixels = 0, int align = ALIGN_CENTER);
		
		void update();
		void clear();
		
		void SetBrightness(unsigned int b);
};
#endif

