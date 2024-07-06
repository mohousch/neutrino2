/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: pixmap.h 05072024 mohousch Exp $

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


#ifndef __pixmap__
#define __pixmap__

#include <stdint.h>

#include <driver/gfx/framebuffer.h>


//
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

//
enum
{
        blitAlphaTest 		= 1,
        blitAlphaBlend 		= 2,
        blitScale 		= 4,
        blitKeepAspectRatio 	= 8
};

int blitBox(gUnmanagedSurface *m_surface, const int src_w, const int src_h, const CBox &_pos, gUnmanagedSurface * surface, int flag);
#endif

