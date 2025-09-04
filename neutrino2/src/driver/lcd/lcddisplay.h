//
//	LCD-Daemon  -   DBoxII-Project
//	
//	$Id: lcddisplay.h 04092025 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//		baseroutines by Shadow_
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
 //
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

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

// aabbggrr
#define LCD_PIXEL_BACKGROUND				0xFF000000
#define LCD_PIXEL_WHITE						0xFFFFFFFF
#define LCD_PIXEL_RED							0xFF0000FF
#define LCD_PIXEL_GREEN						0xFF00FF00
#define LCD_PIXEL_BLUE							0xFFFF0000
#define LCD_PIXEL_YELLOW						0xFF00FFFF
#define LCD_PIXEL_PERCENT						0xFF02C6FF
#define LCD_PIXEL_BLACK						0xFF000000

#define LCD_IOCTL_CLEAR						(26)

////
#define LCDSET 									0x1000
#define LCD_IOCTL_ASC_MODE 					(21 | LCDSET)
#define LCD_MODE_ASC 							0
#define LCD_MODE_BIN 							1

#define FP_IOCTL_LCD_DIMM       					3

#define LCD_IOCTL_ON            					(2 |LCDSET)
#define LCD_IOCTL_REVERSE       					(4 |LCDSET)
#define LCD_IOCTL_SRV           					(10|LCDSET)

#define LED_IOCTL_BRIGHTNESS_NORMAL 		0X10
#define LED_IOCTL_BRIGHTNESS_DEEPSTANDBY 	0X11
#define LED_IOCTL_BLINKING_TIME 				0X12
#define LED_IOCTL_SET_DEFAULT 				0x13

struct gRGB
{
    	union {
#if BYTE_ORDER == LITTLE_ENDIAN
        	struct {
            		uint8_t b, g, r, a;
        	};
#else
        	struct {
            		uint8_t a, r, g, b;
        	};
#endif
        	uint32_t value;
    	};
    
    	gRGB(int r, int g, int b, int a = 0): b(b), g(g), r(r), a(a)
    	{
    	}
    
    	gRGB(uint32_t val): value(val)
    	{
    	}
    
    	gRGB(const gRGB& other): value(other.value)
    	{
    	}
    
    	gRGB(const char *colorstring)
    	{
        	uint32_t val = 0;

        	if (colorstring)
        	{
            		for (int i = 0; i < 8; i++)
            		{
                		char c = colorstring[i];
                		if (!c) break;
                		val <<= 4;
                		if (c >= '0' && c <= '9')
                    			val |= c - '0';
                		else if(c >= 'a' && c <= 'f')
                    			val |= c - 'a' + 10;
                		else if(c >= 'A' && c <= 'F')
                    			val |= c - 'A' + 10;
                		else if(c >= ':' && c <= '?') // Backwards compatibility for old style color strings
                    			val |= c & 0x0f;
            		}
        	}
        	value = val;
    	}
    
    	gRGB(): value(0)
    	{
    	}

    	uint32_t argb() const
    	{
        	return value;
    	}

    	void set(uint32_t val)
    	{
        	value = val;
    	}

    	void operator=(uint32_t val)
    	{
        	value = val;
    	}
    
    	bool operator < (const gRGB &c) const
    	{
        	if (b < c.b)
            		return true;
        	if (b == c.b)
        	{
            		if (g < c.g)
                		return true;
            		if (g == c.g)
            		{
                		if (r < c.r)
                    			return true;
                		if (r == c.r)
                    			return a < c.a;
            		}
        	}
        	return false;
    	}
    
    	bool operator==(const gRGB &c) const
    	{
        return c.value == value;
    	}
    
    	bool operator != (const gRGB &c) const
    	{
        return c.value != value;
    	}
    
    	operator const std::string () const
    	{
        	uint32_t val = value;
        	std::string escapecolor = "\\c";
        	escapecolor.resize(10);
        	
        	for (int i = 9; i >= 2; i--)
        	{
            		int hexbits = val & 0xf;
            		escapecolor[i] = hexbits < 10? '0' + hexbits : 'a' - 10 + hexbits;
            		val >>= 4;
        	}
        	
        	return escapecolor;
    	}
    
    	void alpha_blend(const gRGB other)
    	{
#define BLEND(x, y, a) (y + (((x - y) * a)>>8))
        	b = BLEND(other.b, b, other.a);
        	g = BLEND(other.g, g, other.a);
        	r = BLEND(other.r, r, other.a);
        	a = BLEND(0xFF, a, other.a);
#undef BLEND
    	}
};

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
	uint32_t *buffer;
};

class CLCDDisplay
{
	private:
		int fd;
		int paused;
		int locked;
		uint32_t inverted;
		bool flipped;
		int lcd_type;
		int last_brightness;
		
#ifdef ENABLE_LCD
		uint8_t * lcd_buffer;
		int lcd_xres, lcd_yres;
		int lcd_stride;
		int lcd_bpp, lcd_bypp;
		int lcd_buffer_size;
		////
		int lcd_real_offset;
		int lcd_real_yres;
		void setSize(int w, int h, int b);
#endif
	
#ifdef ENABLE_TFTLCD
		uint32_t * tftbuffer;
		int tftxres, tftyres;
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
		
		enum
                {
	                blitAlphaNone		= 0,
                        blitAlphaTest 		= 1,
                        blitAlphaBlend 		= 2
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
		void load_screen(uint32_t ** const screen);
		void dump_screen(uint32_t **screen);
		bool dump_png(const char * const filename);
		int showPNGImage(const char* filename, int posx, int posy, int width = 0, int height = 0, int transp = 0xFF);
		void load_png_element(raw_lcd_element_t * element, int posx, int posy, int width = 0, int height = 0);
		void show_png_element(raw_lcd_element_t *element, int posx, int posy, int width = 0, int height = 0);
		void show_analog_clock(int hour, int min, int sec, int posx, int posy, int hour_size, int min_size, int sec_size);
		////
		int setLCDContrast(int contrast);
		int setLCDBrightness(int brightness);
		int setLED(int value, int option);
		void setInverted(uint32_t inv);
		void setFlipped(bool);
		int islocked() { return locked; }
		
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

		//// raw buffer
		uint32_t *raw_buffer;
		int raw_stride;
		int raw_buffer_size;
		int raw_bpp, raw_bypp;
		int xres, yres;
};

#endif

