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

#include <config.h>

#include <driver/gdi/color.h>

#ifdef ENABLE_GRAPHLCD
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>

#define kDefaultConfigFile 			"/etc/graphlcd.conf"
#endif

// argb
#define LCD_PIXEL_OFF				0xFF000000  // bg
#define LCD_PIXEL_ON				0xFFFFFFFF
#define LCD_PIXEL_INV				0x1000000

#define LCD_PIXEL_BACKGROUND			COL_BACKGROUND_PLUS_0 	//0xFF000000
#define LCD_PIXEL_WHITE				COL_WHITE_PLUS_0
#define LCD_PIXEL_RED				COL_RED_PLUS_0
#define LCD_PIXEL_GREEN				COL_GREEN_PLUS_0
#define LCD_PIXEL_BLUE				COL_BLUE_PLUS_0
#define LCD_PIXEL_YELLOW			COL_YELLOW_PLUS_0	//0xFF00FFFF
#define LCD_PIXEL_MAGENTA			COL_MAGENTA_PLUS_0
#define LCD_PIXEL_PERCENT			0xFFFFC602 		// COL_AQUA_PLUS_0
#define LCD_PIXEL_BLACK				COL_BLACK_PLUS_0

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

#if defined BOXMODEL_DM8000 || defined BOXMODEL_DM7080
#define lcd_pixel_t 				uint8_t

#define DEFAULT_LCD_XRES			132
#define DEFAULT_LCD_YRES			64
#else
#define lcd_pixel_t 				uint32_t

#define DEFAULT_LCD_XRES			220
#define DEFAULT_LCD_YRES			176
#endif

struct raw_lcd_element_t
{
	std::string name;
	int x;
	int y;
	int width;
	int height;
	int bpp;
	int bypp;
	int stride;
	lcd_pixel_t *buffer;
	gPalette clut;
};

class CLCDDisplay
{
	private:
		int           	fd;
		int	      	paused;
		int 		locked;
		uint32_t	inverted;
		bool 	      	flipped;
		int 	      	lcd_type;
		int 	      	last_brightness;
		
#ifdef ENABLE_LCD
		uint8_t     	* surface_data;
		int 	      	surface_stride;
		int 	      	surface_bpp, surface_bypp;
		int 	      	surface_buffer_size;
		////
		int 	      	real_offset;
		int 	      	real_yres;
#endif
	
#ifdef ENABLE_TFTLCD
		uint32_t * tftbuffer;
		int tftstride;
		int tftbpp;
		int tftbypp;
		int tft_buffer_size;
		int m_brightness, m_gamma, m_alpha;
		int m_available;
		struct fb_var_screeninfo m_screeninfo;
		fb_cmap m_cmap;
		unsigned char m_ramp[256], m_rampalpha[256]; // RGB ramp 0..255
		uint16_t m_red[256], m_green[256], m_blue[256], m_trans[256];
		int m_phys_mem;
		int m_manual_blit;
		void calcRamp();
		int setMode(int nxRes, int nyRes, int nbpp);
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
			LED_BRIGHTNESS = 0,
			LED_DEEPSTANDBY,
			LED_BLINKINGTIME
		};
	
		CLCDDisplay();
		~CLCDDisplay();
		
		bool init(const char *fbdevice = "/dev/fb1");
		bool initGLCD();
		void initBuffer();

		void pause();
		void resume();

		void update();
		void blitBox2LCD(int flag = blitAlphaBlend);
		void blit(void);
		void clear_screen();
		////
		void draw_point(const int x, const int y, uint32_t color);
		void draw_line(const int x1, const int y1, const int x2, const int y2, const uint32_t color);
		void draw_fill_rect(int left, int top, int right, int bottom, uint32_t color);
		void draw_rectangle(int left, int top, int right, int bottom, uint32_t linecolor, uint32_t fillcolor);
		void draw_polygon(int num_vertices, int *vertices, uint32_t color);
		////
		void load_screen_element(raw_lcd_element_t * element, int left, int top);
		void load_screen(lcd_pixel_t ** const screen);
		void dump_screen(lcd_pixel_t **screen);
		bool dump_png(const char * const filename);
		////
		int setLCDContrast(int contrast);
		int setLCDBrightness(int brightness);
		int setLED(int value, int option);
		void setInverted(uint32_t inv);
		void setFlipped(bool);
		//// raw buffer
		lcd_pixel_t *raw_buffer;
		int raw_stride;
		int raw_buffer_size;
		int raw_bpp, raw_bypp;
		gPalette raw_clut;
		////
		int xres, yres;
		void setSize(int w, int h, int b);
		////
		int islocked() { return locked; }
		////
		int showPNGImage(const char* filename, int posx, int posy, int width = 0, int height = 0, int flag = blitAlphaBlend);
		void load_png_element(raw_lcd_element_t * element, int posx, int posy, int width = 0, int height = 0);
		void show_png_element(raw_lcd_element_t *element, int posx, int posy, int width = 0, int height = 0);
		////
		void show_analog_clock(int hour, int min, int sec, int posx, int posy, int hour_size, int min_size, int sec_size);
		
#ifdef ENABLE_GRAPHLCD
		GLCD::cDriver *lcd;
		uint32_t * ngbuffer;
		int ngstride;
		int ngbpp;
		int ngbypp;
		int ng_buffer_size;
		int ngxres, ngyres;

		int GetConfigSize();
		std::string GetConfigName(int);
		int setGLCDBrightness(int brightness);
#endif
};

#endif

