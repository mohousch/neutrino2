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
		GLCD::cFont font_channel;
		GLCD::cFont font_epg;
		GLCD::cFont font_time;
		GLCD::cFont font_time_standby;
		////
		int fontsize_channel;
		int fontsize_epg;
		int fontsize_time;
		int fontsize_time_standby;
		int percent_channel;
		int percent_time;
		int percent_time_standby;
		int percent_epg;
		int percent_bar;
		int percent_logo;
		int percent_space;
		std::string Channel;
		std::string Epg;
		std::string stagingChannel;
		std::string stagingEpg;
		t_channel_id channel_id;
		int Scale;
		
	public:
		GLCD::cBitmap *bitmap;
		
		nGLCD();
		~nGLCD();
		
		bool init();
		
		//// paint methods
		void draw_fill_rect (int left,int top,int right,int bottom,int state){};
		void draw_point(const int x, const int y, const int state){};
};
#endif

