/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: color.h 25.09.2023 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#include <driver/framebuffer.h>


// font common colors
#define COL_MAROON	 			0x02
#define COL_GREEN	 			0x03
#define COL_OLIVE	 			0x04
#define COL_NAVY	 			0x05
#define COL_PURPLE		 		0x06
#define COL_TEAL		 		0x07
#define COL_NOBEL		 		0x08
#define COL_MATTERHORN 				0x09
#define COL_RED	 				0x0A
#define COL_LIME	 			0x0B
#define COL_YELLOW	 			0x0C
#define COL_BLUE	 			0x0D
#define COL_MAGENTA	 			0x0E
#define COL_AQUA		 		0x0F
#define COL_WHITE	 			0x10
#define COL_BLACK	 			0x11
#define COL_ORANGE				0x12
#define COL_SILVER				0x13
#define COL_BACKGROUND 				0xFF

// neutrino font colors
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

// gui common colors
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

#define COL_BACKGROUND_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_BACKGROUND]

// neutrino gui colors
// infoBar color
#define COL_INFOBAR_PLUS_0              	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR + 0]
#define COL_INFOBAR_SHADOW_PLUS_0       	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_SHADOW + 0] // CMessageBox
#define COL_INFOBAR_SHADOW_PLUS_1       	CFrameBuffer::getInstance()->realcolor[COL_INFOBAR_SHADOW + 1] // infobar buttons

// head color
#define COL_MENUHEAD_PLUS_0             	CFrameBuffer::getInstance()->realcolor[COL_MENUHEAD + 0]

// menucontent
#define COL_MENUCONTENT_PLUS_0          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 0]
#define COL_MENUCONTENT_PLUS_1          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 1] // scrollbar
#define COL_MENUCONTENT_PLUS_2          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 2] // progressbar
#define COL_MENUCONTENT_PLUS_3          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 3] // scrollbarslider
#define COL_MENUCONTENT_PLUS_4          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 4] // stringinput
#define COL_MENUCONTENT_PLUS_5          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 5] // epgplus
#define COL_MENUCONTENT_PLUS_6          	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENT + 6] // shadow

// menucontentdark
#define COL_MENUCONTENTDARK_PLUS_0      	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTDARK + 0] //

// menuselected
#define COL_MENUCONTENTSELECTED_PLUS_0  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTSELECTED + 0]
#define COL_MENUCONTENTSELECTED_PLUS_1  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTSELECTED + 1] // marked

// menuinactive
#define COL_MENUCONTENTINACTIVE_PLUS_0  	CFrameBuffer::getInstance()->realcolor[COL_MENUCONTENTINACTIVE + 0]

// foot
#define COL_MENUFOOT_PLUS_0			CFrameBuffer::getInstance()->realcolor[COL_MENUFOOT + 0]

// itemInfo
#define COL_MENUHINT_PLUS_0        		CFrameBuffer::getInstance()->realcolor[COL_MENUHINT + 0]

////
int convertSetupColor2RGB(unsigned char r, unsigned char g, unsigned char b);
int convertSetupAlpha2Alpha(unsigned char alpha);
//
inline uint32_t make16color(__u32 rgb)
{
        return 0xFF000000 | rgb;
}
// 
inline uint32_t make16Color(unsigned int rgb)
{
	uint32_t col = 0xFF000000 | rgb;
	
        return col;
}
//
inline uint32_t convertSetupColor2Color(unsigned char r, unsigned char g, unsigned char b, unsigned char alpha)
{
	int color = convertSetupColor2RGB(r, g, b);
	int tAlpha = (alpha ? (convertSetupAlpha2Alpha(alpha)) : 0);

	if(!alpha) 
		tAlpha = 0xFF;

	fb_pixel_t col = ((tAlpha << 24) & 0xFF000000) | color;
	
	return col;
}

//
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

//
uint8_t limitChar(int c);
uint8_t getBrightnessRGB(fb_pixel_t color);
fb_pixel_t changeBrightnessRGBRel(fb_pixel_t color, int br, bool transp = true);
fb_pixel_t changeBrightnessRGB(fb_pixel_t color, uint8_t br, bool transp = true);
fb_pixel_t Hsv2SysColor(HsvColor *hsv, uint8_t tr=0xFF);
uint8_t SysColor2Hsv(fb_pixel_t color, HsvColor *hsv);
void Hsv2Rgb(HsvColor *hsv, RgbColor *rgb);
void Rgb2Hsv(RgbColor *rgb, HsvColor *hsv);
fb_pixel_t* gradientColorToTransparent(fb_pixel_t col, fb_pixel_t *gradientBuf, int bSize, int mode, int intensity = INT_LIGHT);
fb_pixel_t* gradientOneColor(fb_pixel_t col, fb_pixel_t *gradientBuf, int bSize, int mode, int intensity = INT_LIGHT, uint8_t v_min = 0x40, uint8_t v_max = 0xE0, uint8_t s = 0xC0);
fb_pixel_t* gradientColorToColor(fb_pixel_t start_col, fb_pixel_t end_col, fb_pixel_t *gradientBuf, int bSize, int mode, int intensity = INT_LIGHT);

#endif

