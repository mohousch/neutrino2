/*
        $Header$

	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
		baseroutines by tmbinc
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

#ifndef __LCDFONTRENDERER__
#define __LCDFONTRENDERER__

#include <config.h>

#include <driver/lcd/lcddisplay.h>
#include <driver/lcd/tftlcd.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H

#include <asm/types.h>


class LcdFontRenderClass;
class LcdFont
{
#ifdef ENABLE_LCD
        CLCDDisplay             * framebuffer;
#elif defined (ENABLE_TFTLCD)
	CTFTLCD			* framebuffer;
#endif
        
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
        FTC_ImageTypeRec        font;
#else
        FTC_Image_Desc  font;
        FT_Face                 face;
#endif
        LcdFontRenderClass * renderer;
        FT_Size                 size;

        FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);

        public:
                void RenderString(int x, int y, int width, const char *text, int color, int selected = 0, const bool utf8_encoded = false);

                int getRenderWidth(const char *text, const bool utf8_encoded = false);

#ifdef ENABLE_LCD
                LcdFont(CLCDDisplay *fb, LcdFontRenderClass *render, FTC_FaceID faceid, int isize);
#elif defined (ENABLE_TFTLCD)
		LcdFont(CTFTLCD *fb, LcdFontRenderClass *render, FTC_FaceID faceid, int isize);
#endif
                ~LcdFont(){}
};

class LcdFontRenderClass
{ 
#ifdef ENABLE_LCD
	CLCDDisplay 	*framebuffer;
#elif defined (ENABLE_TFTLCD)
	CTFTLCD		*framebuffer;
#endif

	struct fontListEntry
	{
		char *filename, *style, *family;
		fontListEntry *next;
		~fontListEntry();
	} *font;

	FT_Library	library;
	FTC_Manager	cacheManager;        /* the cache manager               */
	FTC_ImageCache	imageCache;          /* the glyph image cache           */
	FTC_SBitCache	sbitsCache;          /* the glyph small bitmaps cache   */

	FTC_FaceID getFaceID(const char *family, const char *style);
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
	FT_Error getGlyphBitmap(FTC_ImageType font, FT_ULong glyph_index, FTC_SBit *sbit);
#else
	FT_Error getGlyphBitmap(FTC_Image_Desc *font, FT_ULong glyph_index, FTC_SBit *sbit);
#endif

	public:
		pthread_mutex_t render_mutex;
		const char * AddFont(const char * const filename);
		void InitFontCache();

		FT_Error FTC_Face_Requester(FTC_FaceID face_id, FT_Face * aface);

		static FT_Error myFTC_Face_Requester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface);
		//FT_Face getFace(const char *family, const char *style);
		LcdFont *getFont(const char *family, const char *style, int size);
#ifdef ENABLE_LCD
		LcdFontRenderClass(CLCDDisplay *fb);
#elif defined (ENABLE_TFTLCD)
		LcdFontRenderClass(CTFTLCD *fb);
#endif
		~LcdFontRenderClass();

		friend class LcdFont;
};

#endif /* __LCDFONTRENDERER__ */

