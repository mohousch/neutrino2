/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: tftlcd.h 01062024 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
		baseroutines by Shadow_
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

#ifndef __tftlcd__
#define __tftlcd__

#include <string>
#include <stdint.h>

#include <linux/fb.h>


#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, uint32_t)
#endif

class CTFTLCD
{
	private:
		int fd;
		int m_xRes, m_yRes, m_bpp;
		int m_brightness, m_gamma, m_alpha;
		int m_available;
		struct fb_var_screeninfo m_screeninfo;
		fb_cmap m_cmap;
		unsigned char m_ramp[256], m_rampalpha[256]; // RGB ramp 0..255
		uint16_t m_red[256], m_green[256], m_blue[256], m_trans[256];
		int m_phys_mem;
		int m_manual_blit;
		int locked;
		unsigned char *_buffer;
		int _stride;
		
		void calcRamp();
		int setMode(int xRes, int yRes, int bpp);
		void getMode();
		void enableManualBlit();
		void disableManualBlit();
		// low level gfx stuff
		int putCMAP();
//		void save2png(unsigned char* output, int xRes, int yRes);
//		void save2bmp(unsigned char* output, int xRes, int yRes); 
		
	public:
		CTFTLCD();
		~CTFTLCD();
		
		bool init(const char *fbdevice = "/dev/fb1");
		void update();  // blit
		int waitVSync();
		int lock();
		void unlock();
		int islocked() { return locked; }
		int setLCDBrightness(int brightness);
		
		//// paint methods
		void draw_fill_rect (int left,int top,int right,int bottom,int state){};
		void draw_point(const int x, const int y, const int state){};
};

#endif

