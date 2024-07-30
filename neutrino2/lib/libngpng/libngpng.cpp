/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: libngpng.cpp 01062024 mohousch Exp $

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

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <stdint.h>
#include <climits>
#include <stdlib.h>

#include "libngpng.h"


#define PNG_DEBUG
#define LIBNGPNG_SILENT

static short debug_level = 10;

#ifdef LIBNGPNG_DEBUG
#define libngpng_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define libngpng_printf(level, fmt, x...)
#endif

#ifndef LIBNGPNG_SILENT
#define libngpng_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define libngpng_err(fmt, x...)
#endif

CFormathandler * fh_root = NULL;

// PNG
extern int fh_png_getsize(const char *name, int *x, int *y, int wanted_width, int wanted_height);
extern int fh_png_load(const char *name, unsigned char **buffer, int *xp, int *y);
extern int fh_png_id(const char *name);
extern int int_png_load(const char * name, unsigned char ** buffer, int * xp, int * yp, int * bpp, int *channels);

// JPG
extern int fh_jpeg_getsize (const char *, int *, int *, int, int);
extern int fh_jpeg_load (const char *, unsigned char **, int *, int *);
extern int fh_jpeg_id (const char *);

// BMP
extern int fh_bmp_getsize (const char *, int *, int *, int, int);
extern int fh_bmp_load (const char *, unsigned char **, int *, int *);
extern int fh_bmp_id (const char *);

// GIF
extern int fh_gif_getsize (const char *, int *, int *, int, int);
extern int fh_gif_load (const char *, unsigned char **, int *, int *);
extern int fh_gif_id (const char *);

// CRW
extern int fh_crw_getsize (const char *, int *, int *, int, int);
extern int fh_crw_load (const char *, unsigned char **, int *, int *);
extern int fh_crw_id (const char *);

// SVG
extern int fh_svg_getsize (const char *, int *, int *, int, int);
extern int fh_svg_load (const char *, unsigned char **, int *, int *);
extern int svg_load_resize(const char *name, unsigned char **buffer, int* ox, int* oy, int dx, int dy);
extern int fh_svg_id (const char *);

void add_format(int (*picsize) (const char *, int *, int *, int, int), int (*picread) (const char *, unsigned char **, int *, int *), int (*id) (const char *))
{
	CFormathandler * fhn = NULL;
	fhn = (CFormathandler *) malloc(sizeof(CFormathandler));
	fhn->get_size = picsize;
	fhn->get_pic = picread;
	fhn->id_pic = id;
	fhn->next = fh_root;
	fh_root = fhn;
}

void init_handlers(void)
{
	libngpng_printf(10, "init_handlers\n");
	
	// add png format
  	add_format(fh_png_getsize, fh_png_load, fh_png_id);
	
	// add jpg format
	add_format(fh_jpeg_getsize, fh_jpeg_load, fh_jpeg_id);
	
	// add bmp
	add_format(fh_bmp_getsize, fh_bmp_load, fh_bmp_id);

	// add gif
	add_format(fh_gif_getsize, fh_gif_load, fh_gif_id);

	// add crw
	add_format(fh_crw_getsize, fh_crw_load, fh_crw_id);
	
	// add svg
	add_format (fh_svg_getsize, fh_svg_load, fh_svg_id);
}

void deinit_handlers(void)
{
	libngpng_printf(10, "deinit_handlers\n");
	
	CFormathandler *fh = fh_root;
	
	while (fh) 
	{
		CFormathandler *tmp = fh->next;
		free(fh);
		fh = tmp;
	}
}

CFormathandler * fh_getsize(const char *name, int *x, int *y, int width_wanted, int height_wanted)
{
	CFormathandler * fh = NULL;
	
	for (fh = fh_root; fh != NULL; fh = fh->next) 
	{
		if (fh->id_pic (name))
		{
			if (fh->get_size (name, x, y, width_wanted, height_wanted) == FH_ERROR_OK)
				return (fh);
		}
	}

	return (NULL);
}

void getSize(const std::string &name, int *width, int *height, int *nbpp, int *channels)
{
	unsigned char *rgbbuff;
	int x = 0;
	int y = 0;
	int bpp = 0;
	int chans = 0;
	int load_ret = FH_ERROR_MALLOC;
	CFormathandler * fh = NULL;

	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
	if (fh == NULL) 
	{
		*width = 0;
		*height = 0;

		return;
	}
	
	rgbbuff = (unsigned char *)malloc(x*y*4);
	
	if (rgbbuff != NULL) 
	{
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &rgbbuff, &x, &y, &bpp, &chans);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &rgbbuff, &x, &y, *width, *height);
		}
		else
			load_ret = fh->get_pic(name.c_str(), &rgbbuff, &x, &y);
		
		if(load_ret == FH_ERROR_OK)
		{
			*width = x;
			*height = y;
			*nbpp = bpp;			
			*channels = chans;
		}
		else 
		{
			*width = 0;
			*height = 0;
			*nbpp = 0;
			channels = 0;
		}
	}

	return;
}

uint8_t *resize(uint8_t * origin, int ox, int oy, int dx, int dy, ScalingMode type, bool alpha)
{
	uint8_t * cr = NULL;
	
	cr = (uint8_t *) malloc(dx*dy*(alpha? 4 : 3));

	if(cr == NULL)
	{
		libngpng_err("Error: malloc\n");
		return(origin);
	}

	if(type == SCALE_SIMPLE) 
	{
		uint8_t *p, *l;
		int i, j, k, ip;
		l = cr;

		for(j = 0; j < dy; j++, l += dx*3)
		{
			p = origin + (j*oy/dy*ox*3);
			
			for(i = 0, k = 0; i < dx; i++, k += 3)
			{
				ip = i*ox/dx*3;
				memmove(l + k, p + ip, 3);
			}
		}
	} 
	else // average_color
	{
		uint8_t *p, *q;
		int i, j, k, l, ya, yb;
		int sq, r, g, b, a;

		p = cr;

		int xa_v[dx];
		for(i = 0; i < dx; i++)
			xa_v[i] = i*ox/dx;
			
		int xb_v[dx+1];
		
		for(i = 0; i < dx; i++)
		{
			xb_v[i] = (i+1)*ox/dx;
			if(xb_v[i] >= ox)
				xb_v[i] = ox - 1;
		}
		
		if (alpha)
		{
			for(j = 0;j < dy; j++)
			{
				ya = j*oy/dy;
				yb = (j + 1)*oy/dy;
				
				if(yb >= oy) 
					yb = oy - 1;
				
				for(i = 0; i < dx; i++, p += 4)
				{
					for(l = ya, r = 0, g = 0, b = 0, a = 0, sq = 0; l <= yb; l++)
					{
						q = origin + ((l*ox + xa_v[i])*4);
						
						for(k = xa_v[i]; k <= xb_v[i]; k++, q += 4, sq++)
						{
							r += q[0]; 
							g += q[1]; 
							b += q[2]; 
							a += q[3];
						}
					}
					
					p[0] = r/sq; 
					p[1] = g/sq; 
					p[2] = b/sq; 
					p[3] = a/sq;
				}
			}
		}
		else
		{
			for(j = 0; j < dy; j++)
			{
				ya = j*oy/dy;
				yb = (j + 1)*oy/dy;
				
				if(yb >= oy) 
					yb = oy - 1;
					
				for(i = 0; i < dx; i++, p += 3)
				{
					for(l = ya, r = 0, g = 0, b = 0, sq = 0; l <= yb; l++)
					{
						q = origin + ((l*ox+xa_v[i])*3);
							
						for(k = xa_v[i]; k <= xb_v[i]; k++, q += 3, sq++)
						{
							r += q[0]; 
							g += q[1]; 
							b += q[2];
						}
					}
					
					p[0] = r/sq; 
					p[1] = g/sq; 
					p[2] = b/sq;
				}
			}
		}
	}
	
	free(origin);
	
	return(cr);
}

// convert rgba to argb
uint32_t * convertRGB2FB32(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint32_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint32_t *)malloc(count*sizeof(uint32_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
		
	if(alpha)
	{
		for(i = 0; i < count; i++)
		{
			fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
				((rgbbuff[i*4]     << 16) & 0x00FF0000) | 
				((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
				((rgbbuff[i*4 + 2])       & 0x000000FF);
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				break;
		}
	}
	
	return (uint32_t *)fbbuff;
}

// convert rgba to abgr
uint32_t * convertBGR2FB32(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint32_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint32_t *)malloc(count*sizeof(uint32_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
	
	if(alpha)
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++)
				{
					fbbuff[i] = (0xFF000000) | 
						((rgbbuff[i*4 + 2]     << 16) & 0x00FF0000) | 
						((rgbbuff[i*4 + 1]     <<  8) & 0x0000FF00) | 
						((rgbbuff[i*4])               & 0x000000FF);
				}
				break;
				
			case TM_INI:
				for(i = 0; i < count; i++)
				{
					fbbuff[i] = ((transp << 24) & 0xFF000000) | 
						((rgbbuff[i*4 + 2] << 16) & 0x00FF0000) | 
						((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
						((rgbbuff[i*4])           & 0x000000FF);
				}
				break;
				
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
				{
					fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
						((rgbbuff[i*4 + 2]     << 16) & 0x00FF0000) | 
						((rgbbuff[i*4 + 1]     <<  8) & 0x0000FF00) | 
						((rgbbuff[i*4])               & 0x000000FF);
				}
				break;
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | ((rgbbuff[i*3]) & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);
				break;
		}
	}
	
	return (uint32_t *)fbbuff;
}

// convert rgba to argb
uint16_t * convertRGB2FB16(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint16_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint16_t *)malloc(count*sizeof(uint16_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
		
	if(alpha)
	{
		for(i = 0; i < count; i++)
		{
			fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
				((rgbbuff[i*4]     << 16) & 0x00FF0000) | 
				((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
				((rgbbuff[i*4 + 2])       & 0x000000FF);
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				break;
		}
	}
	
	return (uint16_t *)fbbuff;
}

// convert rgba to abgr
uint16_t * convertBGR2FB16(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint16_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint16_t *)malloc(count*sizeof(uint16_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
		
	if(alpha)
	{
		for(i = 0; i < count; i++)
		{
			fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
				((rgbbuff[i*4 + 2]     << 16) & 0x00FF0000) | 
				((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
				((rgbbuff[i*4])       & 0x000000FF);
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);
				break;
		}
	}
	
	return (uint16_t *)fbbuff;
}

// convert rgba to argb
uint8_t * convertRGB2FB8(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint8_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint8_t *)malloc(count*sizeof(uint8_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
		
	if(alpha)
	{
		for(i = 0; i < count; i++)
		{
			fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
				((rgbbuff[i*4]     << 16) & 0x00FF0000) | 
				((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
				((rgbbuff[i*4 + 2])       & 0x000000FF);
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3+1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3 + 2] & 0xFF);
				break;
		}
	}
	
	return (uint8_t *)fbbuff;
}

// convert rgba to abgr
uint8_t * convertBGR2FB8(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha, int transp, int m_transparent)
{
	unsigned long i;
	uint8_t *fbbuff = NULL;
	unsigned long count = x*y;
	
	fbbuff = (uint8_t *)malloc(count*sizeof(uint8_t));
		
	if ( fbbuff == NULL )
	{
		libngpng_err( "Error: malloc\n" );
		return NULL;
	}
		
	if(alpha)
	{
		for(i = 0; i < count; i++)
		{
			fbbuff[i] = ((rgbbuff[i*4 + 3] << 24) & 0xFF000000) | 
				((rgbbuff[i*4 + 2]     << 16) & 0x00FF0000) | 
				((rgbbuff[i*4 + 1] <<  8) & 0x0000FF00) | 
				((rgbbuff[i*4])       & 0x000000FF);
		}
	}
	else
	{
		switch (m_transparent) 
		{
			case TM_BLACK:
				for(i = 0; i < count; i++) 
				{
					transp = 0;
					if(rgbbuff[i*3] || rgbbuff[i*3 + 1] || rgbbuff[i*3 + 2])
						transp = 0xFF;
							
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);
				}
				break;
		
			case TM_INI:
				for(i = 0; i < count; i++)
					fbbuff[i] = (transp << 24) | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);				
				break;
								
			case TM_NONE:
			default:
				for(i = 0; i < count; i++)
					fbbuff[i] = 0xFF000000 | ((rgbbuff[i*3 + 2] << 16) & 0xFF0000) | ((rgbbuff[i*3 + 1] << 8) & 0xFF00) | (rgbbuff[i*3] & 0xFF);
				break;
		}
	}
	
	return (uint8_t *)fbbuff;
}

uint32_t * getImage(const std::string &name, int width, int height, int transp, ScalingMode scaletype)
{
	int x = 0;
	int y = 0;
	int nbpp = 0;
	int channels = 0;
	CFormathandler * fh = NULL;
	uint8_t * buffer = NULL;
	uint32_t *ret = NULL;
	int load_ret = FH_ERROR_MALLOC;

	//
  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
  	if (fh) 
	{
		buffer = (uint8_t *)malloc(x*y*4);
		
		if (buffer == NULL) 
		{
		  	libngpng_err("Error: malloc\n");
		  	return NULL;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &buffer, &x, &y, &nbpp, &channels);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &buffer, &x, &y, width, height);
			channels = 4;
		}
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret == FH_ERROR_OK) 
		{
			// resize
			if( (width != 0 && height != 0) && (x != width || y != height) )
			{
				// alpha
				if(channels == 4)
				{
					buffer = resize(buffer, x, y, width, height, scaletype, true);
				}
				else
				{
					buffer = resize(buffer, x, y, width, height, scaletype);
				}
					
				x = width ;
				y = height;
			}
			
			// convert
			if( name.find(".png") == (name.length() - 4) )
			{
				// alpha
				if (channels == 4)
					ret = (uint32_t *)convertRGB2FB32(buffer, x, y, true);
				else
					ret = (uint32_t *)convertRGB2FB32(buffer, x, y, false, transp, TM_BLACK); // TM_BLACK
			}
			else
			{
				ret = (uint32_t *)convertRGB2FB32(buffer, x, y, false, transp, TM_NONE); //TM_NONE
			}
			
			free(buffer);
		} 
		else 
		{
	  		libngpng_err("Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
	{
		libngpng_err("Error open file %s\n", name.c_str ());
	}

	return ret;
}

uint32_t * getBGR32Image(const std::string &name, int width, int height, int transp, ScalingMode scaletype)
{
	int x = 0;
	int y = 0;
	int nbpp = 0;
	int channels = 0;
	CFormathandler * fh = NULL;
	uint8_t * buffer = NULL;
	uint32_t *ret = NULL;
	int load_ret = FH_ERROR_MALLOC;

	//
  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
  	if (fh) 
	{
		buffer = (uint8_t *)malloc(x*y*4);
		
		if (buffer == NULL) 
		{
		  	libngpng_err("Error: malloc\n");
		  	return NULL;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &buffer, &x, &y, &nbpp, &channels);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &buffer, &x, &y, width, height);
			channels = 4;
		}
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret == FH_ERROR_OK) 
		{
			// resize
			if( (width != 0 && height != 0) && (x != width || y != height) )
			{
				// alpha
				if(channels == 4)
				{
					buffer = resize(buffer, x, y, width, height, scaletype, true);
				}
				else
				{
					buffer = resize(buffer, x, y, width, height, scaletype);
				}
					
				x = width ;
				y = height;
			}
			
			// convert
			ret = (uint32_t *)convertBGR2FB32(buffer, x, y, (channels == 4)? true : false, transp, TM_NONE); //TM_NONE
			
			free(buffer);
		} 
		else 
		{
	  		libngpng_err("Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
	{
		libngpng_err("Error open file %s\n", name.c_str ());
	}

	return ret;
}

uint16_t * getBGR16Image(const std::string &name, int width, int height, int transp, ScalingMode scaletype)
{
	int x = 0;
	int y = 0;
	int nbpp = 0;
	int channels = 0;
	CFormathandler * fh = NULL;
	uint8_t * buffer = NULL;
	uint16_t *ret = NULL;
	int load_ret = FH_ERROR_MALLOC;

	//
  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
  	if (fh) 
	{
		buffer = (uint8_t *)malloc(x*y*4);
		
		if (buffer == NULL) 
		{
		  	libngpng_err("Error: malloc\n");
		  	return NULL;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &buffer, &x, &y, &nbpp, &channels);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &buffer, &x, &y, width, height);
			channels = 4;
		}
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret == FH_ERROR_OK) 
		{
			// resize
			if( (width != 0 && height != 0) && (x != width || y != height) )
			{
				// alpha
				if(channels == 4)
				{
					buffer = resize(buffer, x, y, width, height, scaletype, true);
				}
				else
				{
					buffer = resize(buffer, x, y, width, height, scaletype);
				}
					
				x = width ;
				y = height;
			}
			
			// convert
			ret = (uint16_t *)convertBGR2FB16(buffer, x, y, (channels == 4)? true : false, transp, TM_NONE); //TM_NONE
			
			free(buffer);
		} 
		else 
		{
	  		libngpng_err("Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
	{
		libngpng_err("Error open file %s\n", name.c_str ());
	}

	return ret;
}

uint8_t * getBGR8Image(const std::string &name, int width, int height, int transp, ScalingMode scaletype)
{
	int x = 0;
	int y = 0;
	int nbpp = 0;
	int channels = 0;
	CFormathandler * fh = NULL;
	uint8_t * buffer = NULL;
	uint8_t *ret = NULL;
	int load_ret = FH_ERROR_MALLOC;

	//
  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
  	if (fh) 
	{
		buffer = (uint8_t *)malloc(x*y*4);
		
		if (buffer == NULL) 
		{
		  	libngpng_err("Error: malloc\n");
		  	return NULL;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &buffer, &x, &y, &nbpp, &channels);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &buffer, &x, &y, width, height);
			channels = 4;
		}
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret == FH_ERROR_OK) 
		{
			// resize
			if( (width != 0 && height != 0) && (x != width || y != height) )
			{
				// alpha
				if(channels == 4)
				{
					buffer = resize(buffer, x, y, width, height, scaletype, true);
				}
				else
				{
					buffer = resize(buffer, x, y, width, height, scaletype);
				}
					
				x = width ;
				y = height;
			}
			
			// convert
			ret = (uint8_t *)convertBGR2FB8(buffer, x, y, (channels == 4)? true : false, transp, TM_NONE); // TM_NONE
			
			
			free(buffer);
		} 
		else 
		{
	  		libngpng_err("Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
	{
		libngpng_err("Error open file %s\n", name.c_str ());
	}

	return ret;
}

uint8_t * getBitmap(const std::string &name)
{
	int x = 0;
	int y = 0;
	int _bpp = 0;
	int channels = 0;
	CFormathandler * fh = NULL;
	uint8_t * buffer = NULL;
	int load_ret = FH_ERROR_MALLOC;

	//
  	fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX); // unscaled
	
  	if (fh) 
	{
		buffer = (uint8_t *) malloc(x*y*4);
		
		if (buffer == NULL) 
		{
		  	libngpng_err("Error: malloc\n");
		  	return NULL;
		}
		
		if ((name.find(".png") == (name.length() - 4)) && (fh_png_id(name.c_str())))
			load_ret = int_png_load(name.c_str(), &buffer, &x, &y, &_bpp, &channels);
		else if (name.find(".svg") == (name.length() - 4))
		{
			load_ret = svg_load_resize(name.c_str(), &buffer, &x, &y, INT_MAX, INT_MAX);
		}
		else
			load_ret = fh->get_pic(name.c_str(), &buffer, &x, &y);

		if (load_ret != FH_ERROR_OK) 
		{
	  		libngpng_err("Error decoding file %s\n", name.c_str ());
	  		free (buffer);
	  		buffer = NULL;
		}
  	} 
	else
	{
		libngpng_err("Error open file %s\n", name.c_str ());
	}

	return buffer;
}

