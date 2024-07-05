/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: lcddisplay.h 31052024 mohousch Exp $

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

#ifndef __lcddisplay__
#define __lcddisplay__

#include <string>
#include <stdint.h>

#include <linux/fb.h>

#include <driver/gfx/color.h>


#define LCD_PIXEL_OFF				0x00
#define LCD_PIXEL_ON				0xFF
#define LCD_PIXEL_INV				0x1000000

/*
#define LCD_MODE_ASC				0
#define LCD_MODE_BIN				2
#define LCD_IOCTL_ASC_MODE			(25)
*/
#define LCD_IOCTL_CLEAR				(26)

////
#define LCDSET 					0x1000
#define LCD_IOCTL_ASC_MODE 			(21 | LCDSET)
#define LCD_MODE_ASC 0
#define LCD_MODE_BIN 1

#define FP_IOCTL_LCD_DIMM       		3

#define LCD_IOCTL_ON            		(2 |LCDSET)
#define LCD_IOCTL_REVERSE       		(4 |LCDSET)
#define LCD_IOCTL_SRV           		(10|LCDSET)

#define LED_IOCTL_BRIGHTNESS_NORMAL 		0X10
#define LED_IOCTL_BRIGHTNESS_DEEPSTANDBY 	0X11
#define LED_IOCTL_BLINKING_TIME 		0X12
#define LED_IOCTL_SET_DEFAULT 			0x13

struct raw_lcd_element_t
{
	std::string name;
	int width;
	int height;
	int bpp;
	uint8_t *buffer;
};

class CLCDDisplay
{
	private:
		int           fd;
		int	      paused;
		
		unsigned char inverted;
		bool 	      flipped;
		int 	      lcd_type;
		int 	      last_brightness;
		////
		uint8_t	*_buffer;
		int 	      _stride;
		////
		uint8_t     * surface_data;
		int 	      surface_stride;
		int 	      surface_bpp, surface_bypp;
		int 	      surface_buffer_size;
		////
		int 	      real_offset;
		int 	      real_yres;
		////
		int locked;
#ifdef ENABLE_TFTLCD
		int m_xRes, m_yRes, m_bpp;
		int m_brightness, m_gamma, m_alpha;
		int m_available;
		struct fb_var_screeninfo m_screeninfo;
		fb_cmap m_cmap;
		unsigned char m_ramp[256], m_rampalpha[256]; // RGB ramp 0..255
		uint16_t m_red[256], m_green[256], m_blue[256], m_trans[256];
		int m_phys_mem;
		int m_manual_blit;
		void calcRamp();
		int setMode(int xRes, int yRes, int bpp);
		void getMode();
		void enableManualBlit();
		void disableManualBlit();
		// low level gfx stuff
		int putCMAP();
		int waitVSync();
		int lock();
		void unlock();
#endif
	
	public:
		enum
		{
			PIXEL_ON  = LCD_PIXEL_ON,
			PIXEL_OFF = LCD_PIXEL_OFF,
			PIXEL_INV = LCD_PIXEL_INV
		};
		
		enum
		{
			LED_BRIGHTNESS = 0,
			LED_DEEPSTANDBY,
			LED_BLINKINGTIME
		};
	
		CLCDDisplay();
		~CLCDDisplay();
		
		bool init(const char *fbdevice = "/dev/fb1");

		void pause();
		void resume();

		void update();
		void clear_screen();
		void convert2LCD(int area_left, int area_top, int area_right, int area_bottom, int color);
		////
		void draw_point(const int x, const int y, const int state);
		void draw_line(const int x1, const int y1, const int x2, const int y2, const int state);
		void draw_fill_rect(int left, int top, int right, int bottom, int state);
		void draw_rectangle(int left, int top, int right, int bottom, int linestate, int fillstate);
		void draw_polygon(int num_vertices, int *vertices, int state);
		////
		void load_screen_element(raw_lcd_element_t * element, int left, int top, int w = 0, int h = 0);
		void load_screen(uint8_t ** const screen);
		void dump_screen(uint8_t **screen);
		bool load_png_element(const char * const filename, raw_lcd_element_t * element);
		bool load_png(const char * const filename);
		bool dump_png_element(const char * const filename, raw_lcd_element_t * element);
		bool dump_png(const char * const filename);
		////
		int setLCDContrast(int contrast);
		int setLCDBrightness(int brightness);
		int setLED(int value, int option);
		void setInverted( unsigned char );
		void setFlipped(bool);
		////
		int raw_buffer_size;
		int xres, yres, bpp;
//		int bypp;
		void setSize(int w, int h, int b);
		////
		int islocked() { return locked; }
		////
		gUnmanagedSurface* loadPNG(const char* filename);
		int showPNGImage(const char* filename, int posX, int posY, int width, int height, int flag = blitAlphaTest);
};

#endif

