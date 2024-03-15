/*
 * framebuffer output/writer handling.
 *
 * This is a hacky implementation of a framebuffer output for the subtitling.
 * This is ment as a POV, later this should be implemented in enigma2 and
 * neutrino.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* ***************************** */
/* Includes                      */
/* ***************************** */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define FB_DEBUG

#ifdef FB_DEBUG

static short debug_level = 10;

#define fb_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define fb_printf(level, fmt, x...)
#endif

#ifndef FB_SILENT
#define fb_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define fb_err(fmt, x...)
#endif

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */


/* ***************************** */
/* Writer Functions              */
/* ***************************** */
// stmfb
#ifdef __sh__
#include <linux/stmfb.h>
#endif

#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT _IOW('F', 0x21, __u8)
#define FBIO_BLIT 0x22
#endif

void blit(int fd)
{	
#if defined (__sh__)
	struct fb_var_screeninfo screeninfo;
	
	STMFBIO_BLT_DATA  bltData; 
	memset(&bltData, 0, sizeof(STMFBIO_BLT_DATA)); 

	bltData.operation  = BLT_OP_COPY;
	bltData.ulFlags |= BLT_OP_FLAGS_BLEND_SRC_ALPHA | BLT_OP_FLAGS_BLEND_DST_COLOR;

	// src
	bltData.srcOffset  = 1920 *1080 * 4;
	bltData.srcPitch   = 1280 * 4; // stride

	bltData.src_left   = 0; 
	bltData.src_top    = 0; 
	bltData.src_right  = 1280; 
	bltData.src_bottom = 720;

		
	bltData.srcFormat = SURF_ARGB8888;
	bltData.srcMemBase = STMFBGP_FRAMEBUFFER;
	
	// get variable screeninfo
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) == -1)
	{
		perror("frameBuffer <FBIOGET_VSCREENINFO>");
	}

	// dst
	bltData.dstOffset  = 0; 
	bltData.dstPitch   = screeninfo.xres * 4;

	bltData.dst_left   = 0; 
	bltData.dst_top    = 0;
	
	// right
	bltData.dst_right  = screeninfo.xres; 
	
	// buttom
	bltData.dst_bottom = screeninfo.yres;

	bltData.dstFormat = SURF_ARGB8888;		
	bltData.dstMemBase = STMFBGP_FRAMEBUFFER;

	if ( (bltData.dst_right > screeninfo.xres) || (bltData.dst_bottom > screeninfo.yres) )
	{
		printf("CFrameBuffer::blit: values out of range desXb:%d desYb:%d\n", bltData.dst_right, bltData.dst_bottom);
	}

	if (ioctl(fd, STMFBIO_BLT, &bltData ) < 0) 
		perror("STMFBIO_BLIT");
	
	// sync bliter
	if(ioctl(fd, STMFBIO_SYNC_BLITTER) < 0)
		perror("ioctl STMFBIO_SYNC_BLITTER");
#else
	// blit
	unsigned char tmp = 1;
	
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0) 
	{
		perror("FB: FBIO_SET_MANUAL_BLIT");
		printf("FB: failed\n");
	}
	else 
	{
		if (ioctl(fd, FBIO_BLIT) < 0)
			perror("FBIO_BLIT");		
	}
#endif	
}

//
uint32_t * simple_resize32(uint8_t * origin, uint32_t * colors, int nb_colors, int ox, int oy, int dx, int dy)
{
	uint32_t  *cr, *l;
	int i, j, k, ip;

	cr = (uint32_t *) malloc(dx * dy * sizeof(uint32_t));

	if(cr == NULL) 
	{
		printf("Error: malloc\n");
		return NULL;
	}
	
	l = cr;

	for(j = 0; j < dy; j++, l += dx)
	{
		uint8_t * p = origin + (j*oy/dy*ox);
		
		for(i = 0, k = 0; i < dx; i++, k++) 
		{
			ip = i*ox/dx;
			
			int idx = p[ip];
			if(idx < nb_colors)
				l[k] = colors[idx];
		}
	}
	
	return(cr);
}

//
void blit2FB(void * fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp, uint32_t yp, bool transp )
{ 
	fb_printf(10, "frameBuffer::blit2FB: width:%d height:%d xoff:%d yoff:%d xp:%d yp:%d\n", width, height, xoff, yoff, xp, yp);
	
	int xc = (width > 1280) ? 1280 : width;
	int yc = (height > 720) ? 720 : height;
	
	uint32_t * data = (uint32_t *) fbbuff;

	uint8_t * d = ((uint8_t *)destination) + xoff * sizeof(uint32_t) + destStride * yoff;
	uint32_t * d2;

	for (int count = 0; count < yc; count++ ) 
	{
		uint32_t * pixpos = &data[(count + yp) * width];
		d2 = (uint32_t *) d;
		
		for (int count2 = 0; count2 < xc; count2++ ) 
		{
			uint32_t pix = *(pixpos + xp);
			
			if (!transp || (pix & 0xff000000) == 0xff000000)
				*d2 = pix;
			else //alpha blending
			{
				uint8_t *in = (uint8_t *)(pixpos + xp);
				uint8_t *out = (uint8_t *)d2;
#if __BYTE_ORDER == __LITTLE_ENDIAN
				int a = in[3];
#elif __BYTE_ORDER == __BIG_ENDIAN
				int a = in[0];
				out++; 
				in++;
#else
#error neither big nor little endian???
#endif				
				*out = (*out + ((*in - *out) * a) / 256);
				in++; out++;
				*out = (*out + ((*in - *out) * a) / 256);
				in++; out++;
				*out = (*out + ((*in - *out) * a) / 256);
			}
			
			d2++;
			pixpos++;
		}
		d += destStride;
	}	
}

//
static int reset()
{
    return 0;
}

static int writeData(void* _call)
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
	int x,y;
	int res = 0;
	unsigned char* dst;
	
	WriterFBCallData_t * call = (WriterFBCallData_t*) _call;
	
	fb_printf(100, "\n");

	if (call == NULL)
	{
		fb_err("call data is NULL...\n");
		return 0;
	}

	if (call->destination == NULL)
	{
		fb_err("file pointer < 0. ignoring ...\n");
		return 0;
	}
	
	if (call->data != NULL)
	{
		blit2FB(call->data, call->Width, call->Height, call->x, call->y, 0, 0, false);
	}
	else
	{
		for (y = 0; y < call->Height; y++)
			memset(call->destination + ((call->y + y) * call->destStride) + call->x * 4, 0, call->Width * 4);
	}
	
	blit(call->fd);
    
	fb_printf(100, "< %d\n", res);
	
	return res;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */
// framebuffer
static WriterCaps_t caps = {
	"framebuffer",
	eGfx,
	"framebuffer",
	0
};

struct Writer_s WriterFramebuffer = {
	&reset,
	&writeData,
	NULL,
	&caps,
};

