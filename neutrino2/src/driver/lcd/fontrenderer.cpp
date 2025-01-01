//
//      $Id: fontrenderer.cpp 31052024 mohousch Exp $     
//
//	LCD-Daemon  -   DBoxII-Project
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	Copyright (C) 2003 thegoodguy
//		baseroutines by tmbinc
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

#include <config.h>

#include "fontrenderer.h"

#include <stdio.h>
#include <string.h>

#include <system/debug.h>

#include <ft2build.h>
#include FT_FREETYPE_H

/* tested with freetype 2.3.9, and 2.1.4 */
//#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
//#define FT_NEW_CACHE_API
//#endif

// fribidi
#include <fribidi/fribidi.h>
#include <pthread.h>


extern int UTF8ToUnicode(const char * &text, const bool utf8_encoded);	//defined in src/driver/fontrenderer.cpp
std::string fribidiShapeChar(const char* text, const bool utf8_encoded);

////
FT_Error LcdFontRenderClass::myFTC_Face_Requester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	return ((LcdFontRenderClass*)request_data)->FTC_Face_Requester(face_id, aface);
}

LcdFontRenderClass::LcdFontRenderClass(CLCDDisplay *fb)
{
	framebuffer = fb;
	
	dprintf(DEBUG_NORMAL, "LcdFontRenderClass::LcdFontRenderClass: initializing core...\n");
	
	if (FT_Init_FreeType(&library))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::LcdFontRenderClass: failed.\n");
		return;
	}
	
	font = NULL;
	pthread_mutex_init(&render_mutex, NULL);
}

LcdFontRenderClass::~LcdFontRenderClass()
{
	fontListEntry * g;
	
	for (fontListEntry * f = font; f; f = g)
	{
		g = f->next;
		delete f;
	}
	
	FTC_Manager_Done(cacheManager);
	FT_Done_FreeType(library);
}

void LcdFontRenderClass::InitFontCache()
{
	dprintf(DEBUG_NORMAL, "LcdFontRenderClass::InitFontCache: Intializing font cache...\n");
	
	if (FTC_Manager_New(library, 3, 0, 0, myFTC_Face_Requester, this, &cacheManager))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::InitFontCache: manager failed!\n");
		return;
	}
	
	if (!cacheManager)
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::InitFontCache: error.\n");
		return;
	}
	
	if (FTC_SBitCache_New(cacheManager, &sbitsCache))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::InitFontCache: sbit failed!\n");
		return;
	}
	
	if (FTC_ImageCache_New(cacheManager, &imageCache))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::InitFontCache: imagecache failed!\n");
	}
}

FT_Error LcdFontRenderClass::FTC_Face_Requester(FTC_FaceID face_id, FT_Face *aface)
{
	fontListEntry *font=(fontListEntry *)face_id;
	
	if (!font)
		return -1;
	
	dprintf(DEBUG_NORMAL, "LcdFontRenderClass::FTC_Face_Requester: FTC_Face_Requester (%s/%s)\n", font->family, font->style);

	int error;
	if ((error = FT_New_Face(library, font->filename, 0, aface)))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::FTC_Face_Requester: failed: %i\n", error);
		return error;
	}
	
	return 0;
}                                                                                                                                

FTC_FaceID LcdFontRenderClass::getFaceID(const char *family, const char *style)
{
	for (fontListEntry *f = font; f; f = f->next)
	{
		if ((!strcmp(f->family, family)) && (!strcmp(f->style, style)))
			return (FTC_FaceID)f;
	}
	
	for (fontListEntry *f = font; f; f = f->next)
	{
		if (!strcmp(f->family, family))
			return (FTC_FaceID)f;
	}
	
	return 0;
}

FT_Error LcdFontRenderClass::getGlyphBitmap(FTC_ImageType font, FT_ULong glyph_index, FTC_SBit *sbit)
{
	return FTC_SBitCache_Lookup(sbitsCache, font, glyph_index, sbit, NULL);
}

const char * LcdFontRenderClass::AddFont(const char * const filename)
{
	dprintf(DEBUG_NORMAL, "LcdFontRenderClass::AddFont: adding font %s...\n", filename);
	
	fflush(stdout);
	int error;
	fontListEntry *n;

	FT_Face face;
	if ((error = FT_New_Face(library, filename, 0, &face)))
	{
		dprintf(DEBUG_NORMAL, "LcdFontRenderClass::AddFont: failed: %i\n", error);
		return NULL;
	}
	
	n = new fontListEntry;
	
	n->filename = strdup(filename);
	n->family   = strdup(face->family_name);
	n->style    = strdup(face->style_name);
	FT_Done_Face(face);

	n->next = font;
	dprintf(DEBUG_NORMAL, "LcdFontRenderClass::AddFont: OK (%s/%s)\n", n->family, n->style);
	font = n;
	
	return n->style;
}

LcdFontRenderClass::fontListEntry::~fontListEntry()
{
	free(filename);
	free(family);
	free(style);
}

LcdFont *LcdFontRenderClass::getFont(const char *family, const char *style, int size)
{
	FTC_FaceID id = getFaceID(family, style);
	
	if (!id)
		return 0;
		
	return new LcdFont(framebuffer, this, id, size);
}

////
LcdFont::LcdFont(CLCDDisplay *fb, LcdFontRenderClass *render, FTC_FaceID faceid, int isize)
{
	framebuffer = fb;
	renderer = render;
	
	font.face_id = faceid;
	font.width  = isize;
	font.height = isize;
	font.flags  = FT_LOAD_FORCE_AUTOHINT | FT_LOAD_MONOCHROME;

	setSize(isize);
}

int LcdFont::setSize(int isize)
{
	FT_Error err;

	int temp = font.width;
	font.width = font.height = isize;
	scaler.face_id = font.face_id;
	scaler.width   = font.width;
	scaler.height  = font.height;
	scaler.pixel   = true;

	err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size);

	if (err != 0)
	{
		dprintf(DEBUG_NORMAL, "FTC_Manager_Lookup_Size failed! (0x%x)\n", err);
		return 0;
	}
	// hack begin (this is a hack to get correct font metrics, didn't find any other way which gave correct values)
	FTC_SBit glyph;
	int index;

	index = FT_Get_Char_Index(size->face, 'M');
	getGlyphBitmap(index, &glyph);
	int tM = glyph->top;
	fontwidth = glyph->width;

	index = FT_Get_Char_Index(size->face, 'g');
	getGlyphBitmap(index, &glyph);
	int hg = glyph->height;
	int tg = glyph->top;

	ascender = tM;
	descender = tg - hg; //this is a negative value!
	int halflinegap = -(descender>>1); // |descender/2| - we use descender as linegap, half at top, half at bottom
	upper = halflinegap+ascender+3;   // we add 3 at top
	lower = -descender+halflinegap+1; // we add 1 at bottom
	height = upper + lower;               // this is total height == distance of lines
	// hack end

	return temp;
}

FT_Error LcdFont::getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit)
{
	return renderer->getGlyphBitmap(&font, glyph_index, sbit);
}

void LcdFont::RenderString(int x, int y, const int width, const char * text, const uint32_t color, const int selected, const bool utf8_encoded)
{
	int err;
	pthread_mutex_lock(&renderer->render_mutex);
	
	// fribidi
	std::string Text = fribidiShapeChar(text, utf8_encoded);
	text = Text.c_str();		

	if ((err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size)) != 0)
	{ 
		dprintf(DEBUG_NORMAL, "LcdFont::RenderString: FTC_Manager_Lookup_Size failed! (%d)\n",err);
		pthread_mutex_unlock(&renderer->render_mutex);
		return;
	}
	
	int left = x, step_y = (size->metrics.height >> 6 )*3/4 + 4;
	face = size->face;

	int pos =0;
	for (; *text; text++)
	{
		pos++;
		FTC_SBit glyph;
		
		//if ((x + size->metrics.x_ppem > (left+width)) || (*text=='\n'))
		if (x + size->metrics.x_ppem > (left+width))
		{ 
			//width clip
			break;
		}
		
		if (*text=='\n')
		{
			x  = left;
		  	y += step_y;
		}

		int unicode_value = UTF8ToUnicode(text, utf8_encoded);

		if (unicode_value == -1)
			break;

		int index = FT_Get_Char_Index(face, unicode_value);

		if (!index)
		  continue;
		  
		if (getGlyphBitmap(index, &glyph))
		{
			dprintf(DEBUG_NORMAL, "LcdFont::RenderString: failed to get glyph bitmap.\n");
			continue;
		}
    
		int rx = x + glyph->left;
		int ry = y - glyph->top;
		
		for (int ay = 0; ay < glyph->height; ay++)
		{
			int ax = 0;
			int w = glyph->width;
			int xpos = rx;
			
			for (; ax < w; ax++)
			{
				unsigned char c = glyph->buffer[ay*abs(glyph->pitch) + (ax>>3)];
				
				if((c>>(7 - (ax&7)))&1)
					framebuffer->draw_point(xpos, ry, color);
					
				xpos++;
			}
			ry++;
		}

		x += glyph->xadvance + 1;
	}
	pthread_mutex_unlock(&renderer->render_mutex);
}

int LcdFont::getRenderWidth(const char * text, const bool utf8_encoded)
{
	pthread_mutex_lock(&renderer->render_mutex);
	
// fribidi
	std::string Text = fribidiShapeChar(text, utf8_encoded);
	text = Text.c_str();	
	
	FT_Error err;

	err = FTC_Manager_LookupSize(renderer->cacheManager, &scaler, &size);

	if (err != 0)
	{ 
		dprintf(DEBUG_NORMAL, "LcdFont::getRenderWidth: FTC_Manager_Lookup_Size failed! (0x%x)\n", err);
		pthread_mutex_unlock(&renderer->render_mutex);
		return -1;
	}
	
	face = size->face;
	
	int x = 0;
	
	for (; *text; text++)
	{
		FTC_SBit glyph;

		int unicode_value = UTF8ToUnicode(text, utf8_encoded);

		if (unicode_value == -1)
			break;

		int index = FT_Get_Char_Index(face, unicode_value);

		if (!index)
			continue;
			
		if (getGlyphBitmap(index, &glyph))
		{
			dprintf(DEBUG_NORMAL, "LcdFont::getRenderWidth: failed to get glyph bitmap.\n");
			continue;
		}
    
		x += glyph->xadvance + 1;
	}
	
	pthread_mutex_unlock(&renderer->render_mutex);
	
	return x;
}

