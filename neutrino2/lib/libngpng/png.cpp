/*
*/

#include <png.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <zlib.h>

#include "libngpng.h"


//#define PNG_DEBUG
//#define PNG_SILENT

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
	fd = open(name, O_RDONLY); if(fd == -1) return(0);
	read(fd,id,4);
	close(fd);
	if(id[1] == 'P' && id[2] == 'N' && id[3] == 'G') return(1);
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
	int bit_depth, color_type, interlace_type, number_passes, pass, int_bpp = 3;
	png_byte * fbptr;
	FILE     * fh;

	if(!(fh = fopen(name,"rb")))
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
	
	*channels = chans;
	*bpp = chans*bit_depth;
		
	if (chans == 4 && (color_type & PNG_COLOR_MASK_ALPHA))
	{
		// 24bit PNGs with alpha-channel
		int_bpp = 4;
		
		//png_set_swap_alpha(png_ptr);
		
		if (trns)
			png_set_tRNS_to_alpha(png_ptr);
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
			
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 2 && PNG_LIBPNG_VER_RELEASE < 36
		if (color_type & PNG_COLOR_MASK_ALPHA)
#endif
			png_set_strip_alpha(png_ptr);
				
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
	return int_png_load(name, buffer, xp, yp, NULL, NULL);
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

bool fh_png_save(const char *filename, int xres, int yres)
{
	png_bytep *row_pointers;
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char *pixel_data;

	FILE *fd = fopen(filename, "w");
	if (!fd)
	{
		printf("fh_png_save: failed to open %s\n", filename);
		return false;
	}

	row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * yres);
	
	if (!row_pointers)
	{
		printf("fh_png_save: malloc error\n");
		fclose(fd);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);
	info_ptr = png_create_info_struct(png_ptr);
#if (PNG_LIBPNG_VER < 10500)
	if (setjmp(png_ptr->jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		printf("fh_png_save: %s save error\n", filename);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		free(row_pointers);
		fclose(fd);
		return false;
	}

	png_init_io(png_ptr, fd);

	int y;
	for (y = 0; y < yres; y++)
	{
		row_pointers[y] = pixel_data + (y * xres * 4);
	}

	png_set_IHDR(png_ptr, info_ptr, xres, yres, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_set_compression_level(png_ptr, Z_BEST_SPEED);

	png_set_bgr(png_ptr);
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(row_pointers);
	fclose(fd);

	return true;
}


