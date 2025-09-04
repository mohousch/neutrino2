//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: png.cpp 04092025 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#include <png.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <zlib.h>

#include "bitmap.h"


//#define PNG_DEBUG
#define PNG_SILENT

static short debug_level = 10;

#ifdef PNG_DEBUG
#define png_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define png_printf(level, fmt, x...)
#endif

#ifndef PNG_SILENT
#define png_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define png_err(fmt, x...)
#endif

#define PNG_BYTES_TO_CHECK 4
#define min(x,y) ((x) < (y) ? (x) : (y))

int fh_png_id(const char * name)
{
	int fd;
	char id[4];
	fd = open(name, O_RDONLY); 
	if(fd == -1) 
		return(0);
	read(fd, id, 4);
	close(fd);
	
	if(id[1] == 'P' && id[2] == 'N' && id[3] == 'G') 
		return(1);
		
	return(0);
}

int int_png_load(const char *name, unsigned char **buffer, int *xp, int *yp, int *bpp, int *channels)
{
	static const png_color_16 my_background = {0, 0, 0, 0, 0};
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int chans = 0;
	int trns = 0;
	unsigned int i;
	int bit_depth = 0, color_type, interlace_type, number_passes, pass, int_bpp = 3;
	png_byte * fbptr;
	FILE     * fh;

	if(!(fh = fopen(name, "rb")))
		return(FH_ERROR_FILE);
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) 
	{
		fclose(fh);
		
		return(FH_ERROR_FORMAT);
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		fclose(fh);
		
		return(FH_ERROR_FORMAT);
	}
	
#if (PNG_LIBPNG_VER < 10500)
	if (setjmp(png_ptr->jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fh);
		
		return(FH_ERROR_FORMAT);
	}
	
	png_init_io(png_ptr,fh);	
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	chans = png_get_channels(png_ptr, info_ptr);
	trns = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);
	
	png_printf(10, "%s %dx%dx%d, type %d interlace %d channel %d trans %d\n", name, width, height, bit_depth, color_type, interlace_type, chans, trns);
	
	if (channels != NULL) *channels = chans;
	if (bpp != NULL) *bpp = chans*bit_depth;
		
	if (chans == 4 && (color_type & PNG_COLOR_MASK_ALPHA))
	{
		// 24bit PNGs with alpha-channel
		int_bpp = 4;
		
		if (trns)
			png_set_tRNS_to_alpha(png_ptr);
	}
	else if ((chans == 2) && (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) 
	{
		if (bit_depth < 8) 
		{
			// Extract multiple pixels with bit depths of 1, 2, and 4
			// from a single byte into separate bytes
			// (useful for paletted and grayscale images)
			png_set_packing(png_ptr);
			
			// Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 2 && PNG_LIBPNG_VER_RELEASE < 9
			png_set_gray_1_2_4_to_8(png_ptr);
#else
			png_set_expand_gray_1_2_4_to_8(png_ptr);
#endif
		}
			
		// Expand the grayscale to 24-bit RGB if necessary.
		png_set_gray_to_rgb(png_ptr);
			
	}
	else
	{
		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			png_set_palette_to_rgb(png_ptr);
			png_set_background(png_ptr, (png_color_16*)&my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
		}
			
		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			png_set_gray_to_rgb(png_ptr);
			png_set_background(png_ptr, (png_color_16*)&my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
		}
				
		if (bit_depth < 8)
			png_set_packing(png_ptr);
	}
	
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	
	//
	number_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	
	unsigned long rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	
	if (width * int_bpp != rowbytes)
	{
		png_err("[Error processing %s - please report (including image).\n", name);
		png_err("width: %lu int_bpp: %lu rowbytes: %lu\n", (unsigned long)width, (unsigned long)int_bpp, (unsigned long)rowbytes);
		fclose(fh);
		
		return(FH_ERROR_FORMAT);
	}
		
	for (pass = 0; pass < number_passes; pass++)
	{
		fbptr = (png_byte *)(*buffer);
		
		for (i = 0; i < height; i++, fbptr += width * int_bpp)
		{
			png_read_row(png_ptr, fbptr, NULL);
		}
	}
	
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(fh);
	
	return(FH_ERROR_OK);
}

int fh_png_load(const char *name, unsigned char **buffer, int* xp, int* yp)
{
	return int_png_load(name, buffer, xp, yp, 0, 0);
}

int fh_png_getsize(const char *name,int *x,int *y, int /*wanted_width*/, int /*wanted_height*/)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	FILE *fh;

	if(!(fh = fopen(name, "rb")))	
		return(FH_ERROR_FILE);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if(png_ptr == NULL) 
	{
		fclose(fh);
		return(FH_ERROR_FORMAT);
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	
	if(info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		fclose(fh);
		return(FH_ERROR_FORMAT);
	}

#if (PNG_LIBPNG_VER < 10500)
	if (setjmp(png_ptr->jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(fh);
		return(FH_ERROR_FORMAT);
	}

	png_init_io(png_ptr,fh);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	*x = width;
	*y = height;
	fclose(fh);
	
	return(FH_ERROR_OK);
}

