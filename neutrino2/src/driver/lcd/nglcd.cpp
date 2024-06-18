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

#include <algorithm>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>

#include <system/settings.h>
#include <system/channellogo.h>

#include <driver/lcd/nglcd.h>

#include <libngpng/libngpng.h>


static const char *kDefaultConfigFile = "/etc/graphlcd.conf";

nGLCD::nGLCD()
{
	lcd = NULL;
	bitmap = NULL;
	
//	Channel = "NeutrinoNG2";
/*
	fontsize_channel = 0;
	fontsize_epg = 0;
	fontsize_time = 0;
	fontsize_time_standby = 0;
	*/
	/*
	percent_channel = 0;
	percent_time = 0;
	percent_time_standby = 0;
	percent_epg = 0;
	percent_bar = 0;
	percent_space = 0;
	percent_logo = 0;
	*/
//	Scale = 0;
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
	lcd->SetBrightness(g_settings.glcd_brightness);
	
	// create bitmap
	if (!bitmap)
		bitmap = new GLCD::cBitmap(lcd->Width(), lcd->Height(), g_settings.glcd_color_bg);
	
	if (bitmap)	
		ret = true;
	
	return ret;
}

int nGLCD::GetConfigSize()
{
	return (int) GLCD::Config.driverConfigs.size();
}

std::string nGLCD::GetConfigName(int driver)
{
	if ((driver < 0) || (driver > GetConfigSize() - 1))
		driver = 0;
		
	return GLCD::Config.driverConfigs[driver].name;
}

bool nGLCD::getBoundingBox(uint32_t *buffer, int width, int height, int &bb_x, int &bb_y, int &bb_w, int &bb_h)
{
	if (!width || !height)
	{
		bb_x = bb_y = bb_w = bb_h = 0;
		return false;
	}

	int y_min = height;
	uint32_t *b = buffer;
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++, b++)
			if (*b)
			{
				y_min = y;
				goto out1;
			}
out1:

	int y_max = y_min;
	b = buffer + height * width - 1;
	for (int y = height - 1; y_min < y; y--)
		for (int x = 0; x < width; x++, b--)
			if (*b)
			{
				y_max = y;
				goto out2;
			}
out2:

	int x_min = width;
	for (int x = 0; x < width; x++)
	{
		b = buffer + x + y_min * width;
		for (int y = y_min; y < y_max; y++, b += width)
			if (*b)
			{
				x_min = x;
				goto out3;
			}
	}
out3:

	int x_max = x_min;
	for (int x = width - 1; x_min < x; x--)
	{
		b = buffer + x + y_min * width;
		for (int y = y_min; y < y_max; y++, b += width)
			if (*b)
			{
				x_max = x;
				goto out4;
			}
	}
out4:

	bb_x = x_min;
	bb_y = y_min;
	bb_w = 1 + x_max - x_min;
	bb_h = 1 + y_max - y_min;

	if (bb_x < 0)
		bb_x = 0;
	if (bb_y < 0)
		bb_y = 0;

	return true;
}


bool nGLCD::showImage(uint32_t *s, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, bool transp, bool maximize)
{
	int bb_x, bb_y, bb_w, bb_h;

	if (getBoundingBox(s, sw, sh, bb_x, bb_y, bb_w, bb_h) && bb_w && bb_h)
	{
		if (!maximize)
		{
			if (bb_h * dw > bb_w * dh)
			{
				uint32_t dw_new = dh * bb_w / bb_h;
				dx += (dw - dw_new) >> 1;
				dw = dw_new;
			}
			else
			{
				uint32_t dh_new = dw * bb_h / bb_w;
				dy += (dh - dh_new) >> 1;
				dh = dh_new;
			}
		}
		for (u_int y = 0; y < dh; y++)
		{
			for (u_int x = 0; x < dw; x++)
			{
				uint32_t pix = *(s + (y * bb_h / dh + bb_y) * sw + x * bb_w / dw + bb_x);
				if (!transp || pix)
					bitmap->DrawPixel(x + dx, y + dy, pix);
			}
		}
		return true;
	}
	return false;
}

bool nGLCD::showImage(const std::string &filename, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, bool transp, bool maximize)
{
	bool res = false;
	if (!dw || !dh)
		return res;
	fb_pixel_t *s = getImage(filename, sw, sh);
	if (s && sw && sh)
		res = showImage(s, sw, sh, dx, dy, dw, dh, transp, maximize);
	if (s)
		free(s);
	return res;
}

bool nGLCD::showImage(uint64_t cid, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, bool transp, bool maximize)
{
	std::string logo;
	int sw, sh, sbpp;
	
	CChannellogo::getInstance()->getLogoSize(cid, &sw, &sh, &sbpp);

	if (cid != 1)
	{
		logo = CChannellogo::getInstance()->getLogoName(cid); //, cname, logo, &sw, &sh))

		std::string logo_tmp = DATADIR "/neutrino/icons/picon_default.png";
		
		if (logo != logo_tmp)
			return showImage(logo, (uint32_t) sw, (uint32_t) sh, dx, dy, dw, dh, transp, maximize);
	}
	
	return false;
}

void nGLCD::LcdAnalogClock(int posx, int posy, int dia)
{
	int tm_, th_, mx_, my_, hx_, hy_;
	double pi_ = 3.1415926535897932384626433832795, mAngleInRad, mAngleSave, hAngleInRad;
	
	struct timeval tm;
	struct tm * t;

	gettimeofday(&tm, NULL);
	t = localtime(&tm.tv_sec);

	tm_ = t->tm_min;
	th_ = t->tm_hour;

	mAngleInRad = ((6 * tm_) * (2 * pi_ / 360));
	mAngleSave = mAngleInRad;
	mAngleInRad -= pi_ / 2;
#if BOXMODEL_VUULTIMO
	mx_ = int((dia * 0.13 * cos(mAngleInRad)));
	my_ = int((dia * 0.13 * sin(mAngleInRad)));
#elif BOXMODEL_E4HDULTRA
	mx_ = int((dia * 0.30 * cos(mAngleInRad)));
	my_ = int((dia * 0.30 * sin(mAngleInRad)));
#elif BOXMODEL_VUUNO4KSE || BOXMODEL_DM900 || BOXMODEL_DM920
	mx_ = int((dia * 0.55 * cos(mAngleInRad)));
	my_ = int((dia * 0.55 * sin(mAngleInRad)));
#else
	mx_ = int((dia * 0.7 * cos(mAngleInRad)));
	my_ = int((dia * 0.7 * sin(mAngleInRad)));
#endif

	hAngleInRad = ((30 * th_) * (2 * pi_ / 360));
	hAngleInRad += mAngleSave / 12;
	hAngleInRad -= pi_ / 2;
#if BOXMODEL_VUULTIMO
	hx_ = int((dia * 0.08 * cos(hAngleInRad)));
	hy_ = int((dia * 0.08 * sin(hAngleInRad)));
#elif BOXMODEL_E4HDULTRA
	hx_ = int((dia * 0.20 * cos(hAngleInRad)));
	hy_ = int((dia * 0.20 * sin(hAngleInRad)));
#elif BOXMODEL_VUUNO4KSE || BOXMODEL_DM900 || BOXMODEL_DM920
	hx_ = int((dia * 0.25 * cos(hAngleInRad)));
	hy_ = int((dia * 0.25 * sin(hAngleInRad)));
#else
	hx_ = int((dia * 0.4 * cos(hAngleInRad)));
	hy_ = int((dia * 0.4 * sin(hAngleInRad)));
#endif

	std::string a_clock = "";

	a_clock = DATADIR "/icons/a_clock.png";
	if (access(a_clock.c_str(), F_OK) != 0)
		a_clock = DATADIR "/icons/a_clock.png";

	int lcd_a_clock_width = 0, lcd_a_clock_height = 0, lcd_a_clock_bpp = 0;
	getSize(a_clock.c_str(), &lcd_a_clock_width, &lcd_a_clock_height, &lcd_a_clock_bpp);
	if (lcd_a_clock_width && lcd_a_clock_height)
	{
		showImage(a_clock, (uint32_t) lcd_a_clock_width, (uint32_t) lcd_a_clock_height,
			0, 0, (uint32_t) bitmap->Width(), (uint32_t) bitmap->Height(), false, false);

		lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height());
		lcd->Refresh(true);
	}

	// hour
	bitmap->DrawLine(posx, posy - 8, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 7, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 6, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 5, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 4, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 3, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 2, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 1, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx + 1, posy, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 1, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 2, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 3, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 4, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 5, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 6, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 7, posx + hx_, posy + hy_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 8, posx + hx_, posy + hy_, g_settings.glcd_color_fg);

	// minute
	bitmap->DrawLine(posx, posy - 6, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 5, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 4, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 3, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 2, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy - 1, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx + 1, posy, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 1, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 2, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 3, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 4, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 5, posx + mx_, posy + my_, g_settings.glcd_color_fg);
	bitmap->DrawLine(posx, posy + 6, posx + mx_, posy + my_, g_settings.glcd_color_fg);
}

void nGLCD::update()
{
	lcd->Refresh(true);
}

void nGLCD::clear()
{
	lcd->Refresh(false);
}
