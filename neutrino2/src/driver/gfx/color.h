/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: color.h 17062024 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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


#ifndef __color__
#define __color__

#include <stdint.h>

#include <driver/gfx/framebuffer.h>


//
#define COL_MAROON	 			2
#define COL_GREEN	 			3
#define COL_OLIVE	 			4
#define COL_NAVY	 			5
#define COL_PURPLE		 		6
#define COL_TEAL		 		7
#define COL_NOBEL		 		8
#define COL_MATTERHORN 				9
#define COL_RED	 				10
#define COL_LIME	 			11
#define COL_YELLOW	 			12
#define COL_BLUE	 			13
#define COL_MAGENTA	 			14
#define COL_AQUA		 		15
#define COL_WHITE	 			16
#define COL_BLACK	 			17
#define COL_ORANGE				18
#define COL_SILVER				19
//
#define RADAR_OFFSET				20
//
#define COL_BACKGROUND 				255				
//
#define COL_MENUCONTENTINACTIVE			254-8*1
#define COL_MENUCONTENTSELECTED			254-8*2
#define COL_MENUCONTENTDARK			254-8*3
#define COL_MENUCONTENT				254-8*4
#define COL_MENUHEAD				254-8*5
#define COL_MENUFOOT				254-8*6
#define COL_INFOBAR				254-8*7
#define COL_INFOBAR_SHADOW			254-8*8
#define COL_INFOBAR_COLORED_EVENTS		254-8*9
#define COL_MENUHINT				254-8*10
// text colors
#define COL_MENUCONTENTINACTIVE_TEXT		254-8*11
#define COL_MENUCONTENTSELECTED_TEXT		254-8*12
#define COL_MENUCONTENT_TEXT			254-8*13
#define COL_MENUHEAD_TEXT			254-8*14
#define COL_MENUFOOT_TEXT			254-8*15
#define COL_INFOBAR_TEXT			254-8*16
#define COL_INFOBAR_COLORED_EVENTS_TEXT		254-8*17
#define COL_MENUHINT_TEXT			254-8*18
//
#define COL_FREE_MAX				COL_MENUHINT_TEXT - 1

////
#define COL_MAROON_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_MAROON]
#define COL_GREEN_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_GREEN]
#define COL_OLIVE_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_OLIVE]
#define COL_NAVY_PLUS_0				CFrameBuffer::getInstance()->realcolor[COL_NAVY]
#define COL_PURPLE_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_PURPLE]
#define COL_TEAL_PLUS_0				CFrameBuffer::getInstance()->realcolor[COL_TEAL]
#define COL_NOBEL_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_NOBEL]
#define COL_MATTERHORN_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_MATTERHORN]
#define COL_RED_PLUS_0				CFrameBuffer::getInstance()->realcolor[COL_RED]
#define COL_LIME_PLUS_0				CFrameBuffer::getInstance()->realcolor[COL_LIME]
#define COL_YELLOW_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_YELLOW]
#define COL_BLUE_PLUS_0                       	CFrameBuffer::getInstance()->realcolor[COL_BLUE]
#define COL_MAGENTA_PLUS_0 			CFrameBuffer::getInstance()->realcolor[COL_MAGENTA]
#define COL_AQUA_PLUS_0 			CFrameBuffer::getInstance()->realcolor[COL_AQUA]
#define COL_WHITE_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_WHITE]
#define COL_BLACK_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_BLACK]
#define COL_ORANGE_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_ORANGE]
#define COL_SILVER_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_SILVER]
//
#define COL_INFOBAR_PLUS_0              	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR + 0]
#define COL_INFOBAR_SHADOW_PLUS_0       	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_SHADOW + 0]
#define COL_INFOBAR_SHADOW_PLUS_1       	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_SHADOW + 1]
#define COL_MENUHEAD_PLUS_0             	CFrameBuffer::getInstance()->realcolor[COL_MENUHEAD + 0]
#define COL_MENUCONTENT_PLUS_0          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 0]
#define COL_MENUCONTENT_PLUS_1          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 1] // scrollbar
#define COL_MENUCONTENT_PLUS_2          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 2] // progressbar
#define COL_MENUCONTENT_PLUS_3          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 3] // slider
#define COL_MENUCONTENT_PLUS_4          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 4] // stringinput
#define COL_MENUCONTENT_PLUS_5          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 5] // epgplus
#define COL_MENUCONTENT_PLUS_6          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 6] // shadow
#define COL_MENUCONTENTDARK_PLUS_0      	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTDARK + 0] //
#define COL_MENUCONTENTSELECTED_PLUS_0  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTSELECTED + 0]
#define COL_MENUCONTENTSELECTED_PLUS_1  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTSELECTED + 1]
#define COL_MENUCONTENTINACTIVE_PLUS_0  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTINACTIVE + 0]
#define COL_MENUFOOT_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_MENUFOOT + 0]
#define COL_MENUHINT_PLUS_0        		CFrameBuffer::getInstance()->realcolor[COL_MENUHINT + 0]
// text
#define COL_MENUCONTENTINACTIVE_TEXT_PLUS_0	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTINACTIVE_TEXT]		
#define COL_MENUCONTENTSELECTED_TEXT_PLUS_0	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTSELECTED_TEXT]		
#define COL_MENUCONTENT_TEXT_PLUS_0		CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT_TEXT]			
#define COL_MENUHEAD_TEXT_PLUS_0		CFrameBuffer::getInstance()->realcolor[COL_MENUHEAD_TEXT]			
#define COL_MENUFOOT_TEXT_PLUS_0		CFrameBuffer::getInstance()->realcolor[COL_MENUFOOT_TEXT]			
#define COL_INFOBAR_TEXT_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_TEXT]			
#define COL_INFOBAR_COLORED_EVENTS_TEXT_PLUS_0	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_COLORED_EVENTS_TEXT]	
#define COL_MENUHINT_TEXT_PLUS_0		CFrameBuffer::getInstance()->realcolor[COL_MENUHINT_TEXT]			
//
#define COL_BACKGROUND_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_BACKGROUND]

////
int convertSetupColor2RGB(uint8_t r, uint8_t g, uint8_t b);
int convertSetupAlpha2Alpha(uint8_t alpha);
uint32_t convertSetupColor2Color(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);

////
inline uint32_t rgbaToColor(unsigned int rgb, uint8_t tr = 0xFF)
{
	uint32_t col = ((tr << 24) & 0xFF000000) | rgb;
	
	return col;
}

inline uint32_t rgbaToColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

inline uint32_t rgbToColor(uint8_t r, uint8_t g, uint8_t b)
{
	return (r << 16) | (g << 8) | b;
}

////
typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} RgbColor;

typedef struct {
	float h;
	float s;
	float v;
} HsvColor;

uint8_t limitChar(int c);
uint32_t Hsv2SysColor(HsvColor *hsv, uint8_t tr=0xFF);
uint8_t SysColor2Hsv(uint32_t color, HsvColor *hsv);
void Hsv2Rgb(HsvColor *hsv, RgbColor *rgb);
void Rgb2Hsv(RgbColor *rgb, HsvColor *hsv);
uint32_t *gradientColorToTransparent(uint32_t col, int bSize, int mode, int intensity = INT_LIGHT);
uint32_t *gradientOneColor(uint32_t col, int bSize, int mode, int intensity = INT_LIGHT);
uint32_t *gradientColorToColor(uint32_t start_col, uint32_t end_col, int bSize, int mode, int intensity = INT_LIGHT);

////
struct gRGB
{
    	union {
#if BYTE_ORDER == LITTLE_ENDIAN
        	struct {
            		unsigned char b, g, r, a;
        	};
#else
        	struct {
            		unsigned char a, r, g, b;
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
#define BLEND(x, y, a) (y + (((x-y) * a)>>8))
        	b = BLEND(other.b, b, other.a);
        	g = BLEND(other.g, g, other.a);
        	r = BLEND(other.r, r, other.a);
        	a = BLEND(0xFF, a, other.a);
#undef BLEND
    	}
};


struct gPalette
{
    	int start, colors;
    	gRGB *data;
};

//
enum
{
        blitAlphaTest 		= 1,
        blitAlphaBlend 		= 2
};

//// for future use 
struct gUnmanagedSurface
{
    	int x, y, bpp, bypp, stride;
    	gPalette clut;
    	void *data;
    	int data_phys;

    	gUnmanagedSurface();
    	gUnmanagedSurface(int width, int height, int bpp);
};

struct gSurface: gUnmanagedSurface
{
    	gSurface(): gUnmanagedSurface() {}
    	gSurface(int width, int height, int bpp);
    	~gSurface();
    	
	private:
    		gSurface(const gSurface&); /* Copying managed gSurface is not allowed */
    		gSurface& operator =(const gSurface&);
};

#endif

