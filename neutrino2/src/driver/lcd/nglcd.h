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


////
static GLCD::cFont font_channel;
static GLCD::cFont font_epg;
static GLCD::cFont font_time;
static GLCD::cFont font_time_standby;
		
class nGLCD
{	
	private:
		GLCD::cDriver *lcd;
	
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
		void initFonts();
		
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
		void showAnalogClock(int posx, int posy, int dia);
		
		void drawText(int x, int y, int width, const std::string &text, GLCD::cFont *font, uint32_t color = GLCD::cColor::White, uint32_t bgcolor = GLCD::cColor::Black, bool proportional = true, int skipPixels = 0, int align = ALIGN_CENTER);
		
		void update();
		void clear();
		
		void SetBrightness(unsigned int b);
		////
		int getWidth();
		int getHeight();
		int getFontHeight(GLCD::cFont *font);
		int getFontRenderWidth(GLCD::cFont *font, const std::string &text);
		/*
		void DrawPixel(int x, int y, uint32_t color);
	    	void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
    		void DrawHLine(int x1, int y, int x2, uint32_t color);
    		void DrawVLine(int x, int y1, int y2, uint32_t color);
    		void DrawRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled);
    		void DrawRoundRectangle(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int size);
    		void DrawEllipse(int x1, int y1, int x2, int y2, uint32_t color, bool filled, int quadrants);
    		void DrawSlope(int x1, int y1, int x2, int y2, uint32_t color, int type);
    		void DrawBitmap(int x, int y, const cBitmap & bitmap, uint32_t color = cColor::White, uint32_t bgcolor = cColor::Black, int opacity = 255);
    		int DrawText(int x, int y, int xmax, const std::string &text, const GLCD::cFont *font,
                 uint32_t color = GLCD::cColor::White, uint32_t bgcolor = GLCD::cColor::Black, bool proportional = true, int skipPixels = 0);
		*/
};
#endif

