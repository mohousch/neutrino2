/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: screenshot.cpp 21072024 mohousch Exp $

	Copyright (C) 2011 CoolStream International Ltd
	Copyright (C) 2017 M. Liebmann (micha-bbg)

	parts based on AiO Screengrabber (C) Seddi seddi@ihad.tv

	License: GPLv2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation;

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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <png.h>
#include <zlib.h>

#include <system/screenshot.h>
#include <system/debug.h>

#include <libdvbapi/video_cs.h>

extern "C" {
#include <jpeglib.h>
}

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/version.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


extern cVideo *videoDecoder;

CScreenshot *CScreenshot::getInstance()
{
	static CScreenshot *screenshot = NULL;
	if(!screenshot)
		screenshot = new CScreenshot();
		
	return screenshot;
}

CScreenshot::CScreenshot()
{
	format = CScreenshot::FORMAT_PNG;
	filename = "";
	pixel_data = NULL;
	fd = NULL;
	xres = 0;
	yres = 0;
	get_osd = true;
	get_video = false;
	scale_to_video = false;
}

bool getvideo2(unsigned char *video, int xres, int yres)
{
	bool ret = false;
	
#ifdef USE_OPENGL
	if (video ==  NULL)
		return ret;
		
	char videosnapshot[] = "/dev/dvb/adapter0/video0";
	int fd_video = open(videosnapshot, O_RDONLY);
	
	if (fd_video < 0)
	{
		perror(videosnapshot);
		return ret;
	}
	
	ssize_t r = read(fd_video, video, xres * yres * 3);
	if (r)
	{
		ret = true;
	}
	close(fd_video);
#endif
	
	return ret;
}

static bool swscale(unsigned char *src, unsigned char *dst, int sw, int sh, int dw, int dh, AVPixelFormat sfmt)
{
	bool ret = false;
	int len = 0;
	struct SwsContext *scale = NULL;
	
	scale = sws_getCachedContext(scale, sw, sh, sfmt, dw, dh, AV_PIX_FMT_RGB32, SWS_BICUBIC, 0, 0, 0);
	
	if (!scale)
	{
		ng2_printf(DEBUG_NORMAL, "ERROR setting up SWS context\n");
		return ret;
	}
	
	AVFrame *sframe = av_frame_alloc();
	AVFrame *dframe = av_frame_alloc();
	
	if (sframe && dframe)
	{
		len = av_image_fill_arrays(sframe->data, sframe->linesize, &(src)[0], sfmt, sw, sh, 1);
		
		if (len > -1)
			ret = true;

		if (ret && (len = av_image_fill_arrays(dframe->data, dframe->linesize, &(dst)[0], AV_PIX_FMT_RGB32, dw, dh, 1) < 0))
			ret = false;

		if (ret && (len = sws_scale(scale, sframe->data, sframe->linesize, 0, sh, dframe->data, dframe->linesize) < 0))
			ret = false;
		else
			ret = true;
	}
	else
	{
		ng2_printf(DEBUG_NORMAL, "could not alloc sframe (%p) or dframe (%p)\n", sframe, dframe);
		ret = false;
	}

	if (sframe)
	{
		av_frame_free(&sframe);
		sframe = NULL;
	}
	
	if (dframe)
	{
		av_frame_free(&dframe);
		dframe = NULL;
	}
	
	if (scale)
	{
		sws_freeContext(scale);
		scale = NULL;
	}
	
	ng2_printf(DEBUG_NORMAL, "Error scale %ix%i to %ix%i ,len %i\n", sw, sh, dw, dh, len);

	return ret;
}

inline void rgb24torgb32(unsigned char  *src, unsigned char *dest, int picsize)
{
	for (int i = 0; i < picsize; i++)
	{
		*dest++ = *src++;
		*dest++ = *src++;
		*dest++ = *src++;
		*dest++ = 255;
	}
}

void get_osd_buf(uint8_t *osd_data)
{
	struct fb_var_screeninfo *var = CFrameBuffer::getInstance()->getScreenInfo();
	
	if (var->bits_per_pixel == 32)
	{
		ng2_printf(0, "Grabbing 32bit Framebuffer ...\n");
		
		// get 32bit framebuffer
		memcpy(osd_data, (uint8_t *)CFrameBuffer::getInstance()->getFrameBufferPointer(), CFrameBuffer::getInstance()->getStride() * var->yres);
	}
}

bool CScreenshot::getData()
{
	ng2_printf(0, "osd:%d video:%d scale:%d\n", get_osd, get_video, scale_to_video);
	
#define VDEC_PIXFMT AV_PIX_FMT_BGR24
	
	int aspect = 0;
	videoDecoder->getPictureInfo(xres, yres, aspect); // aspect is dummy here
	
	aspect = videoDecoder->getAspectRatio();
	if (xres <= 0 || yres <= 0)
		get_video = false;
		
	ng2_printf(0, "xres:%d yres:%d aspect:%d\n", xres, xres, aspect);

	if (!get_video && !get_osd)
		return false;

	// get screeninfo
	int osd_w = 0;
	int osd_h = 0;
	int bits_per_pixel = 0;
	struct fb_var_screeninfo *var;
	
	if (get_osd)
	{
		var = CFrameBuffer::getInstance()->getScreenInfo();
		
		bits_per_pixel = var->bits_per_pixel;
		osd_w = var->xres;
		osd_h = var->yres;
		
		ng2_printf(0, "osd_w:%d osd_h:%d bpp:%d stride:%d\n", osd_w, osd_h, bits_per_pixel, CFrameBuffer::getInstance()->getStride());
		
		if (osd_w <= 0 || osd_h <= 0 || bits_per_pixel != 32)
			get_osd = false;
			
		if (!scale_to_video && get_osd)
		{
			xres = osd_w;
			yres = osd_h;
		}
	}
	
	ng2_printf(0, "xres:%d yres:%d\n", xres, yres);
	
	uint8_t *osd_data = NULL;
	pixel_data = (uint8_t *)malloc(xres * yres * 4);
	
	if (pixel_data == NULL)
		return false;

	// get videobuffer
#ifndef USE_OPENGL
/*
	if (get_video)
	{
		const int grab_w = 1920;
		const int grab_h = 1080;
		
		unsigned char *video_src = (unsigned char *)malloc(grab_w * grab_h * 3);
		
		if (video_src == NULL)
			return false;
			
		if (getvideo2(video_src, grab_w, grab_h) == false)
		{
			free(pixel_data);
			free(video_src);
			
			return false;
		}
		
		// scale
		if (grab_w != xres || grab_h != yres)  // scale video into data...
		{
			bool ret = swscale(video_src, pixel_data, grab_w, grab_h, xres, yres, VDEC_PIXFMT);
			
			if (!ret)
			{
				free(pixel_data);
				free(video_src);
				return false;
			}
		}
		else   // get_video and no fancy scaling needed
		{
			rgb24torgb32(video_src, pixel_data, grab_w * grab_h);
		}
		free(video_src);
	}
*/
#endif

	if (get_osd)
	{
		osd_data = (uint8_t *)malloc(osd_w * osd_h * 4);
		
		if (osd_data)
			get_osd_buf(osd_data);
	}

	// scale osd to video
	/*
	if (get_osd && (osd_w != xres || osd_h != yres))
	{
		// rescale osd
		unsigned char *osd_src = (unsigned char *)malloc(xres * yres * 4);
		
		if (osd_src)
		{
			bool ret = swscale(osd_data, osd_src, osd_w, osd_h, xres, yres, AV_PIX_FMT_RGB32);
			
			if (!ret)
			{
				free(pixel_data);
				free(osd_data);
				free(osd_src);
				
				return false;
			}
			
			free(osd_data);
			osd_data = NULL;
			osd_data = osd_src;
		}
		else
		{
			free(pixel_data);
			free(osd_data);
			return false;
		}
	}
	*/

	// merge osd / video + alpha_blending
	/*
	if (get_video && get_osd)
	{
		// alpha blend osd onto pixel_data (video). TODO: maybe libavcodec can do this?
		uint32_t *d = (uint32_t *)pixel_data;
		uint32_t *pixpos = (uint32_t *) osd_data;
		
		for (int count = 0; count < yres; count++)
		{
			for (int count2 = 0; count2 < xres; count2++)
			{
				uint32_t pix = *pixpos;
				if ((pix & 0xff000000) == 0xff000000)
					*d = pix;
				else
				{
					uint8_t *in = (uint8_t *)(pixpos);
					uint8_t *out = (uint8_t *)d;
					
#if __BYTE_ORDER == __LITTLE_ENDIAN
					int a = in[3];
#elif __BYTE_ORDER == __BIG_ENDIAN
					int a = in[0];
					out++; 
					in++;
#endif
					
					*out = (*out + ((*in - *out) * a) / 256);
					in++;
					out++;
					*out = (*out + ((*in - *out) * a) / 256);
					in++;
					out++;
					*out = (*out + ((*in - *out) * a) / 256);
				}
				d++;
				pixpos++;
			}
		}
	}
	else*/
	if (get_osd) // only get_osd, pixel_data is not yet populated 
	{
		memcpy(pixel_data, /*osd_data*/(uint8_t *)CFrameBuffer::getInstance()->getFrameBufferPointer(), xres * yres * sizeof(uint32_t));
	}

	if (osd_data)
		free(osd_data);
	
	return true;
}

bool CScreenshot::saveFile()
{
	bool ret = true;

	switch (format)
	{
		case FORMAT_JPG:
			ret = saveJpg();
			break;
			
		default:
		case FORMAT_PNG:
			ret = savePng();
			break;
	}

	free((void *) pixel_data);
	return ret;
}

bool CScreenshot::openFile()
{
	fd = fopen(filename.c_str(), "w");
	
	if (!fd)
	{
		ng2_printf(DEBUG_NORMAL, "failed to open %s\n", filename.c_str());
		return false;
	}
	
	return true;
}

bool CScreenshot::savePng()
{
//	png_bytep *row_pointers;
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte *   fbptr;

	if (!openFile())
		return false;
		
	ng2_printf(0, "xres: %d yres: %d\n\n", xres, yres);
/*
	row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * yres);
	if (!row_pointers)
	{
		ng2_err("malloc error\n");
		fclose(fd);
		return false;
	}
*/

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);
	info_ptr = png_create_info_struct(png_ptr);
#if (PNG_LIBPNG_VER < 10500)
	if (setjmp(png_ptr->jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		ng2_err("%s save error\n", filename.c_str());
		png_destroy_write_struct(&png_ptr, &info_ptr);
//		free(row_pointers);
		fclose(fd);
		return false;
	}

	png_init_io(png_ptr, fd);
/*
	int y;
	for (y = 0; y < yres; y++)
	{
		row_pointers[y] = pixel_data + (y * xres * sizeof(uint32_t));
	}
*/

//	png_set_IHDR(png_ptr, info_ptr, xres, yres, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	//png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
//	png_set_filter(png_ptr, 0, PNG_FILTER_NONE|PNG_FILTER_SUB|PNG_FILTER_PAETH);
//	png_set_compression_level(png_ptr, Z_BEST_SPEED);
//	png_set_bgr(png_ptr);
	png_set_IHDR(png_ptr, info_ptr, xres, yres, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE|PNG_FILTER_SUB|PNG_FILTER_PAETH);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
	png_set_bgr(png_ptr);
        png_write_info(png_ptr, info_ptr);
	png_set_packing(png_ptr);
	
	//
//	png_write_info(png_ptr, info_ptr);
//	png_write_image(png_ptr, row_pointers);

	fbptr = (png_byte *)pixel_data;
	for (unsigned int i = 0; i < (unsigned int)yres; i++)
	{
		png_write_row(png_ptr, fbptr);
		fbptr += xres*sizeof(uint32_t);
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

//	free(row_pointers);
	fclose(fd);

	return true;
}

#define SWAP(x,y)       { x ^= y; y ^= x; x ^= y; }

/* from libjpg example.c */
struct my_error_mgr
{
	/* "public" fields */
	struct jpeg_error_mgr pub;
	
	/* for return to caller */
	jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr *my_error_ptr;

void my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message)(cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

/* save screenshot in jpg format, return true if success, or false */
bool CScreenshot::saveJpg()
{
	int quality = 90;

	if (!openFile())
		return false;

	// ???
	for (int y = 0; y < yres; y++)
	{
		int xres1 = y * xres * 3;
		int xres2 = xres1 + 2;
		
		for (int x = 0; x < xres; x++)
		{
			int x2 = x * 3;
			memmove(pixel_data + x2 + xres1, pixel_data + x * 4 + y * xres * 4, 3);
			SWAP(pixel_data[x2 + xres1], pixel_data[x2 + xres2]);
		}
	}

	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];
	unsigned int row_stride;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer))
	{
		ng2_err("%s save error\n", filename.c_str());
		jpeg_destroy_compress(&cinfo);
		fclose(fd);
		return false;
	}

	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, fd);

	cinfo.image_width = xres;
	cinfo.image_height = yres;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	cinfo.dct_method = JDCT_IFAST;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = xres * 3;

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = (uint8_t *)&pixel_data[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(fd);

	return true;
}

bool CScreenshot::dumpFile(const std::string &fname, screenshot_format_t fmt, bool osd, bool video, bool scale)
{
	bool ret = false;
	
	format = fmt;
	filename = fname;
	get_osd = osd;
	get_video = video;
	scale_to_video = scale;
	
	getData();
	ret = saveFile();
	
	return ret;
}

