/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: fontrenderer.h 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
        Copyright (C) 2003 thegoodguy

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


#ifndef __FONTRENDERER__
#define __FONTRENDERER__

#include <pthread.h>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H

#include <driver/framebuffer.h>


class FBFontRenderClass;
class CFont
{
	CFrameBuffer	* frameBuffer;
	FTC_ImageTypeRec	font;
	FBFontRenderClass * renderer;
	FT_Face		face;
	FT_Size		size;
	FTC_ScalerRec  scaler;

	FT_Error getGlyphBitmap(FT_ULong glyph_index, FTC_SBit *sbit);

	// these are HACKED values, because the font metrics were unusable.
	int height, ascender, descender, upper, lower;
	int fontwidth;
	inline void paintFontPixel(fb_pixel_t *td, uint8_t fg_red, uint8_t fg_green, uint8_t fg_blue, int faktor, uint8_t index);

	public:
		enum fontmodifier
		{
			Regular,
			Embolden
		};
		fontmodifier stylemodifier;

		void RenderString(int x, int y, const int width, const char * text, const uint8_t color, const int boxheight = 0, bool utf8_encoded = true, const bool useBackground = false);
		void RenderString(int x, int y, const int width, const std::string & text, const uint8_t color, const int boxheight = 0, bool utf8_encoded = true, const bool useBackground = false);

		int getRenderWidth(const char* text, bool utf8_encoded = true);
		int getRenderWidth(const std::string &text, bool utf8_encoded = true);
		int getHeight(void);
		int getWidth(void);
		int getSize(){return font.width;}
		int setSize(int isize);

		CFont(FBFontRenderClass *render, FTC_FaceID faceid, const int isize, const fontmodifier _stylemodifier);
		~CFont(){}		
};

class FBFontRenderClass
{
		struct fontListEntry
		{
			char *filename;
			char *style;
			char *family;
			fontListEntry *next;
			~fontListEntry();
		}
		*font;

		FT_Library	library;
		FTC_Manager	cacheManager;	/* the cache manager               */
		FTC_ImageCache	imageCache;	/* the glyph image cache           */
		FTC_SBitCache	sbitsCache;	/* the glyph small bitmaps cache   */

		FTC_FaceID getFaceID(const char * const family, const char * const style);
		FT_Error getGlyphBitmap(FTC_ImageTypeRec * font, FT_ULong glyph_index, FTC_SBit *sbit);
		FT_Error getGlyphBitmap(FTC_ScalerRec *sc, FT_ULong glyph_index, FTC_SBit *sbit);
		
		int xres;	/* the screen resolution in dpi */
		int yres;	/* defaults to 72 dpi */

	public:
		pthread_mutex_t     render_mutex;

		FT_Error FTC_Face_Requester(FTC_FaceID face_id, FT_Face* aface);


		static FT_Error myFTC_Face_Requester(FTC_FaceID  face_id,
		                                     FT_Library  library,
		                                     FT_Pointer  request_data,
		                                     FT_Face*    aface);

		CFont *getFont(const char * const family, const char * const style, int size);

		std::string getFamily(const char * const filename) const;

		const char * AddFont(const char * const filename, bool make_italics = false);

		FBFontRenderClass(const int xres = 72, const int yres = 72);
		~FBFontRenderClass();
		
		friend class CFont;
};

#endif
