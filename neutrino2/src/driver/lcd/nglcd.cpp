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

#include <config.h>

#include <global.h>
#include <neutrino2.h>

#include <system/settings.h>

#include <driver/lcd/nglcd.h>


static const char *kDefaultConfigFile = "/etc/graphlcd.conf";

nGLCD::nGLCD()
{
	lcd = NULL;
	bitmap = NULL;
	
	Channel = "NeutrinoNG2";
	fontsize_channel = 0;
	fontsize_epg = 0;
	fontsize_time = 0;
	fontsize_time_standby = 0;
//	fonts_initialized = false;
//	doScrollChannel = false;
//	doScrollEpg = false;
	percent_channel = 0;
	percent_time = 0;
	percent_time_standby = 0;
	percent_epg = 0;
	percent_bar = 0;
	percent_space = 0;
	percent_logo = 0;
	Scale = 0;
}

nGLCD::~nGLCD()
{
	if (lcd)
	{
		lcd->DeInit();
		delete lcd;
		lcd = NULL;
	}
}

bool nGLCD::init()
{
	bool ret = false;
	
	// configfile
	if (GLCD::Config.Load(kDefaultConfigFile) == false)
	{
		fprintf(stderr, "Error loading config file!\n");
		return false;
	}
	
	// driver config
	if ((GLCD::Config.driverConfigs.size() < 1))
	{
		fprintf(stderr, "nGLCD::init: No driver config found!\n");
		return false;
	}
	
	// create driver
	lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[g_settings.glcd_selected_config].id, &GLCD::Config.driverConfigs[g_settings.glcd_selected_config]);
	
	if (!lcd)
	{
		fprintf(stderr, "nGLCD::init: CreateDriver failed.\n");
		return false;
	}
	
	fprintf(stderr, "nGLCD::init: CreateDriver succeeded.\n");
	
	//
	if (lcd->Init())
	{
		delete lcd;
		lcd = NULL;

		fprintf(stderr, "nGLCD::init: init failed.\n");
		
		return false;
	}

	fprintf(stderr, "nGLCD::init: init succeeded.\n");

	//
	lcd->SetBrightness(0);
	
	// create bitmap
	if (!bitmap)
		bitmap = new GLCD::cBitmap(lcd->Width(), lcd->Height(), g_settings.glcd_color_bg);
	
	if (bitmap)	
		ret = true;
	
	return ret;
}

