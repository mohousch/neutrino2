/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: pixmap.cpp 05072024 mohousch Exp $
 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <stdio.h>
#include <math.h>

#include <driver/gfx/color.h>
#include <system/debug.h>

#include <gui/widget/widget_helpers.h>

#ifndef __DARWIN_LITTLE_ENDIAN
#include <byteswap.h>
#else

#define bswap_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

#endif

////
//#define PIXMAP_DEBUG
#define PIXMAP_SILENT

static short debug_level = 10;

#ifdef PIXMAP_DEBUG
#define pixmap_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define pixmap_printf(level, fmt, x...)
#endif

#ifndef PIXMAP_SILENT
#define pixmap_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define pixmap_err(fmt, x...)
#endif

////
gUnmanagedSurface::gUnmanagedSurface():
    	x(0), y(0), bpp(0), bypp(0), stride(0),
    	data(0),
    	data_phys(0)
{
}

gUnmanagedSurface::gUnmanagedSurface(int width, int height, int _bpp):
    	x(width),
    	y(height),
    	bpp(_bpp),
    	data(0),
    	data_phys(0)
{
    	switch (_bpp)
    	{
    		case 8:
        		bypp = 1;
        		break;
        		
    		case 15:
    		case 16:
        		bypp = 2;
        		break;
        		
    		case 24:        // never use 24bit mode
    		case 32:
        		bypp = 4;
        		break;
        		
    		default:
        		bypp = (bpp+7)/8;
    	}
    	
    	stride = x*bypp;
}


gSurface::gSurface(int width, int height, int _bpp):
    	gUnmanagedSurface(width, height, _bpp)
{
    	if (!data)
    	{
        	data = new unsigned char [y * stride];
    	}
}

gSurface::~gSurface()
{
    	if (data)
    	{
        	delete [] (unsigned char*)data;
    	}
    	
    	if (clut.data)
    	{
        	delete [] clut.data;
    	}
}

static inline void blit_8i_to_32(uint32_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
        	*dst++=pal[*src++];
}

static inline void blit_8i_to_32_at(uint32_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
    	{
        	if (!(pal[*src]&0x80000000))
        	{
            		src++;
            		dst++;
        	} 
        	else
            		*dst++=pal[*src++];
   	 }
}

static inline void blit_8i_to_16(uint16_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
        	*dst++=pal[*src++] & 0xFFFF;
}

static inline void blit_8i_to_16_at(uint16_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
    	{
        	if (!(pal[*src]&0x80000000))
        	{
            		src++;
            		dst++;
        	} 
        	else
            		*dst++=pal[*src++] & 0xFFFF;
    	}
}

static void blit_8i_to_32_ab(gRGB *dst, const uint8_t *src, const gRGB *pal, int width)
{
    	while (width--)
    	{
        	dst->alpha_blend(pal[*src++]);
        	++dst;
    	}
}

static void convert_palette(uint32_t* pal, const gPalette& clut)
{
    	int i = 0;
    	if (clut.data)
    	{
        	while (i < clut.colors)
        	{
            		pal[i] = clut.data[i].argb() ^ 0xFF000000;
            		++i;
        	}
    	}
    	
    	for(; i != 256; ++i)
    	{
        	pal[i] = (0x010101*i) | 0xFF000000;
    	}
}

#define FIX 0x10000

// blit m_surface to surface
int blitBox(gUnmanagedSurface *m_surface, const int src_w, const int src_h, const CBox &_pos, gUnmanagedSurface * surface, int flag)
{
	CBox pos = _pos;

    	pixmap_printf("source size: %d %d", pos.iWidth, pos.iHeight);

    	int scale_x = FIX, scale_y = FIX;

    	if (!(flag & blitScale))
    	{
        	// pos' size is ignored if left or top aligning.
        	// if its size isn't set, centre and right/bottom aligning is ignored
        	/*
        	if (_pos.size().isValid())
        	{
            		if (flag & blitHAlignCenter)
                		pos.setLeft(_pos.left() + (_pos.width() - src_w) / 2);
            		else if (flag & blitHAlignRight)
                		pos.setLeft(_pos.right() - src_w);

            		if (flag & blitVAlignCenter)
                		pos.setTop(_pos.top() + (_pos.height() - src_h) / 2);
            		else if (flag & blitVAlignBottom)
                		pos.setTop(_pos.bottom() - src_h);
        	}
        	*/

        	pos.iWidth = src_w;
        	pos.iHeight = src_h;
    	}
    	else if (pos.iWidth == src_w && pos.iHeight == src_h) /* no scaling required */
        	flag &= ~blitScale;
    	/*
    	else // blitScale is set
    	{
        	scale_x = pos.iWidth * FIX / src_w; //NOSONAR
        	scale_y = pos.iHeight() * FIX / src_h; //NOSONAR
        
        	if (flag & blitKeepAspectRatio)
        	{
            		if (scale_x > scale_y)
            		{
                		// vertical is full height, adjust horizontal to be smaller
                		scale_x = scale_y;
                
                		pos.iWidth= (src_w * _pos.iHeight / src_h);
                
                		if (flag & blitHAlignCenter)
                    			pos.iWidth = ((_pos.iWidth - pos.iWidth) / 2, 0);
                		else if (flag & blitHAlignRight)
                    			pos.iHeight = (_pos.iWidth - pos.iWidth());
            		}
            		else
            		{
                		// horizontal is full width, adjust vertical to be smaller
                		scale_y = scale_x;
                		pos.setHeight(src_h * _pos.width() / src_w);
                		if (flag & blitVAlignCenter)
                    			pos.moveBy(0, (_pos.height() - pos.height()) / 2);
                		else if (flag & blitVAlignBottom)
                    			pos.moveBy(0, _pos.height() - pos.height());
            		}
        	}
    	}
    	*/

	pixmap_printf("SCALE %x %x\n", scale_x, scale_y);

    	for (unsigned int cci = 0; cci < 1; ++cci)
    	{
        	CBox area = pos; /* pos is the virtual (pre-clipping) area on the dest, which can be larger/smaller than src if scaling is enabled */
        
 //       	area &= eRect(ePoint(0, 0), src_size);

 //       	if (area.empty())
   //         		continue;

        	CBox srcarea = area;
//        	srcarea.moveBy(-pos.x(), -pos.y());

//        	pixmap_printf("srcarea before scale: %d %d %d %d\n", srcarea.x(), srcarea.y(), srcarea.width(), srcarea.height());

//        	if (flag & blitScale)
  //          		srcarea = eRect(srcarea.x() * FIX / scale_x, srcarea.y() * FIX / scale_y, srcarea.width() * FIX / scale_x, srcarea.height() * FIX / scale_y);

//        	pixmap_printf("srcarea after scale: %d %d %d %d\n", srcarea.x(), srcarea.y(), srcarea.width(), srcarea.height());


        	if (flag & blitScale)
        	{
            		if ((surface->bpp == 32) && (m_surface->bpp == 8))
            		{
                		const uint8_t *srcptr = (uint8_t*)m_surface->data;
                		uint8_t *dstptr = (uint8_t*)surface->data; // !!
                		uint32_t pal[256];
                		convert_palette(pal, m_surface->clut);

                		const int src_stride = m_surface->stride;
                		srcptr += srcarea.iX*m_surface->bypp + srcarea.iY*src_stride;
                		dstptr += area.iX*surface->bypp + area.iY*surface->stride;
                		const int width = area.iWidth;
                		const int height = area.iHeight;
                		const int src_height = srcarea.iHeight;
                		const int src_width = srcarea.iWidth;
                
                		if (flag & blitAlphaTest)
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const uint8_t *src_row_ptr = srcptr + (((y * src_height) / height) * src_stride);
                        			uint32_t *dst = (uint32_t*)dstptr;
                        			for (int x = 0; x < width; ++x)
                        			{
                            				uint32_t pixel = pal[src_row_ptr[(x *src_width) / width]];
                            				if (pixel & 0x80000000)
                                				*dst = pixel;
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
                		else if (flag & blitAlphaBlend)
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const uint8_t *src_row_ptr = srcptr + (((y * src_height) / height) * src_stride);
                        			gRGB *dst = (gRGB*)dstptr;
                        			for (int x = 0; x < width; ++x)
                        			{
                            				dst->alpha_blend(pal[src_row_ptr[(x * src_width) / width]]);
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
                		else
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const uint8_t *src_row_ptr = srcptr + (((y * src_height) / height) * src_stride);
                        			uint32_t *dst = (uint32_t*)dstptr;
                        			for (int x = 0; x < width; ++x)
                        			{
                            				*dst = pal[src_row_ptr[(x * src_width) / width]];
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
            		}
            		else if ((surface->bpp == 32) && (m_surface->bpp == 32))
            		{
                		const int src_stride = m_surface->stride;
                		const uint8_t* srcptr = (const uint8_t*)m_surface->data + srcarea.iX*m_surface->bypp + srcarea.iY*src_stride;
                		uint8_t* dstptr = (uint8_t*)surface->data + area.iX*surface->bypp + area.iY*surface->stride;
                		const int width = area.iWidth;
                		const int height = area.iHeight;
                		const int src_height = srcarea.iHeight;
                		const int src_width = srcarea.iWidth;
                
                		if (flag & blitAlphaTest)
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const uint32_t *src_row_ptr = (uint32_t*)(srcptr + (((y * src_height) / height) * src_stride));
                        			uint32_t *dst = (uint32_t*)dstptr;
                        
                        			for (int x = 0; x < width; ++x)
                        			{
                            				uint32_t pixel = src_row_ptr[(x *src_width) / width];
                            				if (pixel & 0x80000000)
                                				*dst = pixel;
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
                		else if (flag & blitAlphaBlend)
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const gRGB *src_row_ptr = (gRGB *)(srcptr + (((y * src_height) / height) * src_stride));
                        			gRGB *dst = (gRGB*)dstptr;
                        
                        			for (int x = 0; x < width; ++x)
                        			{
                            				dst->alpha_blend(src_row_ptr[(x * src_width) / width]);
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
                		else
                		{
                    			for (int y = 0; y < height; ++y)
                    			{
                        			const uint32_t *src_row_ptr = (uint32_t*)(srcptr + (((y * src_height) / height) * src_stride));
                        			uint32_t *dst = (uint32_t*)dstptr;
                        
                        			for (int x = 0; x < width; ++x)
                        			{
                            				*dst = src_row_ptr[(x * src_width) / width];
                            				++dst;
                        			}
                        			dstptr += surface->stride;
                    			}
                		}
            		}
            		else
            		{
                		pixmap_printf("unimplemented: scale on non-accel surface %d->%d bpp\n", m_surface->bpp, surface->bpp);
            		}
            		continue;
        	}

        	if ((surface->bpp == 8) && (m_surface->bpp == 8))
        	{
            		uint8_t *srcptr = (uint8_t*)m_surface->data;
            		uint8_t *dstptr = (uint8_t*)surface->data;

            		srcptr+=srcarea.iX*m_surface->bypp+srcarea.iY*m_surface->stride;
            		dstptr+=area.iX*surface->bypp+area.iY*surface->stride;
            
            		if (flag & (blitAlphaTest|blitAlphaBlend))
            		{
                		for (int y = area.iHeight; y != 0; --y)
                		{
                    			// no real alphatest yet
                    			int width=area.iWidth;
                    			unsigned char *s = (unsigned char*)srcptr;
                    			unsigned char *d = (unsigned char*)dstptr;
                    			
                    			// use duff's device here!
                    			while (width--)
                    			{
                        			if (!*s)
                        			{
                            				s++;
                            				d++;
                        			}
                        			else
                        			{
                            				*d++ = *s++;
                        			}
                    			}
                    			srcptr += m_surface->stride;
                    			dstptr += surface->stride;
                		}
            		}
            		else
            		{
                		int linesize = area.iWidth*surface->bypp;
                		for (int y = area.iHeight; y != 0; --y)
                		{
                    			memcpy(dstptr, srcptr, linesize);
                    			srcptr += m_surface->stride;
                    			dstptr += surface->stride;
                		}
            		}
        	}
        	else if ((surface->bpp == 32) && (m_surface->bpp == 32))
        	{
            		uint32_t *srcptr=(uint32_t*)m_surface->data;
            		uint32_t *dstptr=(uint32_t*)surface->data;

            		srcptr+=srcarea.iX+srcarea.iY*m_surface->stride/4;
            		dstptr+=area.iX+area.iY*surface->stride/4;
            		
            		for (int y = area.iHeight; y != 0; --y)
            		{
                		if (flag & blitAlphaTest)
                		{
                    			int width = area.iWidth;
                    			uint32_t *src = srcptr;
                    			uint32_t *dst = dstptr;

                    			while (width--)
                    			{
                        			if (!((*src)&0xFF000000))
                        			{
                            				src++;
                            				dst++;
                        			} 
                        			else
                            				*dst++=*src++;
                    			}
                		} 
                		else if (flag & blitAlphaBlend)
                		{
                    			int width = area.iWidth;
                    			gRGB *src = (gRGB*)srcptr;
                    			gRGB *dst = (gRGB*)dstptr;
                    			
                    			while (width--)
                    			{
                        			dst->alpha_blend(*src++);
                        			++dst;
                    			}
                		}
                		else
                    			memcpy(dstptr, srcptr, area.iWidth*surface->bypp);
                    
                		srcptr = (uint32_t*)((uint8_t*)srcptr + m_surface->stride);
                		dstptr = (uint32_t*)((uint8_t*)dstptr + surface->stride);
            		}
        	}
        	else if ((surface->bpp == 32) && (m_surface->bpp == 8))
        	{
            		const uint8_t *srcptr = (uint8_t*)m_surface->data;
            		uint8_t *dstptr=(uint8_t*)surface->data; // !!
            		uint32_t pal[256];
            		convert_palette(pal, m_surface->clut);

            		srcptr+=srcarea.iX*m_surface->bypp+srcarea.iY*m_surface->stride;
            		dstptr+=area.iX*surface->bypp+area.iY*surface->stride;
            		const int width=area.iWidth;
            		
            		for (int y = area.iHeight; y != 0; --y)
            		{
                		if (flag & blitAlphaTest)
                    			blit_8i_to_32_at((uint32_t*)dstptr, srcptr, pal, width);
                		else if (flag & blitAlphaBlend)
                    			blit_8i_to_32_ab((gRGB*)dstptr, srcptr, (const gRGB*)pal, width);
                		else
                    			blit_8i_to_32((uint32_t*)dstptr, srcptr, pal, width);
                		srcptr += m_surface->stride;
                		dstptr += surface->stride;
            		}
        	}
        	else if ((surface->bpp == 16) && (m_surface->bpp == 8))
        	{
            		uint8_t *srcptr=(uint8_t*)m_surface->data;
            		uint8_t *dstptr=(uint8_t*)surface->data; // !!
            		uint32_t pal[256];

            		for (int i = 0; i != 256; ++i)
            		{
                		uint32_t icol;
                		if (m_surface->clut.data && (i<m_surface->clut.colors))
                    			icol = m_surface->clut.data[i].argb();
                		else
                    			icol=0x010101*i;
#if BYTE_ORDER == LITTLE_ENDIAN
                		pal[i] = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                		pal[i] = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                		pal[i]^=0xFF000000;
            		}

            		srcptr+=srcarea.iX*m_surface->bypp+srcarea.iY*m_surface->stride;
            		dstptr+=area.iX*surface->bypp+area.iY*surface->stride;

            		if (flag & blitAlphaBlend)
                		printf("[uPNG] ignore unsupported 8bpp -> 16bpp alphablend!\n");

            		for (int y=0; y<area.iHeight; y++)
            		{
                		int width=area.iWidth;
                		unsigned char *psrc=(unsigned char*)srcptr;
                		uint16_t *dst=(uint16_t*)dstptr;
                		
                		if (flag & blitAlphaTest)
                    			blit_8i_to_16_at(dst, psrc, pal, width);
                		else
                    			blit_8i_to_16(dst, psrc, pal, width);
                    			
                		srcptr+=m_surface->stride;
                		dstptr+=surface->stride;
            		}
        	}
        	else if ((surface->bpp == 16) && (m_surface->bpp == 32))
        	{
            		uint8_t *srcptr=(uint8_t*)m_surface->data;
            		uint8_t *dstptr=(uint8_t*)surface->data;

            		srcptr+=srcarea.iX*m_surface->bypp+srcarea.iY*m_surface->stride;
            		dstptr+=area.iX*surface->bypp+area.iY*surface->stride;

            		for (int y=0; y<area.iHeight; y++)
            		{
                		int width=area.iWidth;
                		uint32_t *srcp=(uint32_t*)srcptr;
                		uint16_t *dstp=(uint16_t*)dstptr;

                		if (flag & blitAlphaBlend)
                		{
                    			while (width--)
                    			{
                        			if (!((*srcp)&0xFF000000))
                        			{
                            				srcp++;
                            				dstp++;
                        			} 
                        			else
                        			{
                            				gRGB icol = *srcp++;
#if BYTE_ORDER == LITTLE_ENDIAN
                            				uint32_t jcol = bswap_16(*dstp);
#else
                            				uint32_t jcol = *dstp;
#endif
                            				int bg_b = (jcol >> 8) & 0xF8;
                            				int bg_g = (jcol >> 3) & 0xFC;
                            				int bg_r = (jcol << 3) & 0xF8;

                            				int a = icol.a;
                            				int r = icol.r;
                            				int g = icol.g;
                            				int b = icol.b;

                            				r = ((r-bg_r)*a)/255 + bg_r;
                            				g = ((g-bg_g)*a)/255 + bg_g;
                            				b = ((b-bg_b)*a)/255 + bg_b;

#if BYTE_ORDER == LITTLE_ENDIAN
                            				*dstp++ = bswap_16( (b >> 3) << 11 | (g >> 2) << 5 | r  >> 3 );
#else
                            				*dstp++ = (b >> 3) << 11 | (g >> 2) << 5 | r  >> 3 ;
#endif
                        			}
                    			}
                		}
                		else if (flag & blitAlphaTest)
                		{
                    			while (width--)
                    			{
                        			if (!((*srcp)&0xFF000000))
                        			{
                            				srcp++;
                            				dstp++;
                        			} 
                        			else
                        			{
                            				uint32_t icol = *srcp++;
#if BYTE_ORDER == LITTLE_ENDIAN
                            				*dstp++ = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                            				*dstp++ = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                        			}
                    			}
                		} 
                		else
                		{
                    			while (width--)
                    			{
                        			uint32_t icol = *srcp++;
                        			
#if BYTE_ORDER == LITTLE_ENDIAN
                        			*dstp++ = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                        			*dstp++ = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                    			}
                		}
                		srcptr+=m_surface->stride;
                		dstptr+=surface->stride;
            		}
        	}
        	else 
        	{
            		pixmap_printf("cannot blit %dbpp from %dbpp\n", surface->bpp, m_surface->bpp);
            		return -1;
        	}
    	}
    	
    	return 0;
}

