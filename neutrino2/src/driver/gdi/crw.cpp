//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: crw.cpp 21122024 mohousch Exp $
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

#include "libngpng.h"

#include <cstring>
#include <cstdlib>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <setjmp.h>

#undef HAVE_STDLIB_H // -Werror complain
extern "C" {
#include <jpeglib.h>
}
/*
   Get a 2-byte integer, making no assumptions about CPU byte order.
   Nor should we assume that the compiler evaluates left-to-right.
 */
unsigned short fget2 (FILE *f,int order)
{
  	unsigned char a, b;

  	a = fgetc(f);
  	b = fgetc(f);
  	if (order == 0x4949)		/* "II" means little-endian */
    		return a + (b << 8);
  	else				/* "MM" means big-endian */
    		return (a << 8) + b;
}

/*
   Same for a 4-byte integer.
 */
int fget4 (FILE *f,int order)
{
  	unsigned char a, b, c, d;

  	a = fgetc(f);
  	b = fgetc(f);
  	c = fgetc(f);
  	d = fgetc(f);
  	if (order == 0x4949)
    		return a + (b << 8) + (c << 16) + (d << 24);
  	else
    		return (a << 24) + (b << 16) + (c << 8) + d;
}

int fh_crw_parsedirs(FILE *fh, int pos, int length, int order)
{
	fseek(fh,pos+length-4,SEEK_SET);
	int off=pos+fget4(fh,order);
	fseek(fh,off,SEEK_SET);
	int nEntrys=fget2(fh,order);

	for (int i = 0; i < nEntrys; i++)
	{
		int type = fget2(fh,order);
		int len  = fget4(fh,order);
		int roff = fget4(fh,order);

		switch(type)
		{
		case 0x2005:	// Image
			break;
		case 0x2007:	// Thumbnail
			fseek(fh,pos+roff,SEEK_SET);
			return 1;
			break;
		case 0x0810:	// Owner
			break;
		case 0x0816:	// Filename
			break;
		case 0x0817:	// Thumbname
			break;
		case 0x580b:	// SerNo.
			break;
		case 0x0805:	// comment, if subdir 0x300a. "EOS 300D DIGITAL CMOS RAW" if subdir 0x2804
			break;
		case 0x080a:	// vendor \0 name of the camera
			break;
		case 0x080b:	// firmware
			break;
		case 0x0815:
			break;
		case 0x180e:	// time
			break;
		case 0x102a:	// White balance
			break;
		case 0x1031:	// size of the image
			break;
		case 0x1835:	// decoder table
			break;
		default:
			if (type >> 8 == 0x28 || type >> 8 == 0x30)	// Subdirs
			{
				if (fh_crw_parsedirs(fh,pos+roff,len,order)==1)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

void fh_crw_find_jpeg_thumbnail(FILE *fh)
{
	char header[26];
	int order=0;
	long fsize=0;
	long hlength=0;

	fseek (fh, 0, SEEK_SET);
	fread (header, 1, 26, fh);
	fseek (fh, 0, SEEK_END);
	fsize = ftell(fh);
	fseek (fh, 0, SEEK_SET);

	order = header[0] << 8 | header[1];

	if (order == 0x4949)
		hlength = header[2] + (header[3] << 8) + (header[4] << 16) + (header[5] << 24);
	else
		hlength = (header[2] << 24) + (header[3] << 16) + (header[4] << 8) + header[5];

	fh_crw_parsedirs(fh,hlength,fsize-hlength,order);
}

struct r_crw_jpeg_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf envbuffer;
};

int fh_crw_id(const char *name)
{
	int fd;
	unsigned char id[14];
	fd=open(name,O_RDONLY); if(fd==-1) return(0);
	read(fd,id,14);
	close(fd);

	if(id[6] == 'H' && id[7] == 'E' && id[8] == 'A' && id[9] == 'P' &&
		id[10] == 'C' && id[11] == 'C' && id[12] == 'D' && id[13] == 'R')	
		return(1);

	return(0);
}


void crw_cb_error_exit(j_common_ptr cinfo)
{
	struct r_crw_jpeg_error_mgr *mptr;
	mptr=(struct r_crw_jpeg_error_mgr*) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(mptr->envbuffer,1);
}

int fh_crw_load(const char *filename,unsigned char **buffer,int* xp,int* /*yp*/)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_decompress_struct *ciptr;
	struct r_crw_jpeg_error_mgr emgr;
	unsigned char *bp;
	int px,c,x=*xp;
	FILE *fh;
	JSAMPLE *lb;

	ciptr=&cinfo;
	if(!(fh=fopen(filename,"rb"))) return(FH_ERROR_FILE);
	ciptr->err=jpeg_std_error(&emgr.pub);
	emgr.pub.error_exit=crw_cb_error_exit;
	if(setjmp(emgr.envbuffer)==1)
	{
		// FATAL ERROR - Free the object and return...
		jpeg_destroy_decompress(ciptr);
		fclose(fh);

		return(FH_ERROR_FORMAT);
	}

	jpeg_create_decompress(ciptr);
	fh_crw_find_jpeg_thumbnail(fh);
	jpeg_stdio_src(ciptr,fh);
	jpeg_read_header(ciptr,TRUE);
	ciptr->out_color_space=JCS_RGB;
	if(x==(int)ciptr->image_width)
		ciptr->scale_denom=1;
	else if(abs(x*2 - ciptr->image_width) < 2)
		ciptr->scale_denom=2;
	else if(abs(x*4 - ciptr->image_width) < 4)
		ciptr->scale_denom=4;
	else if(abs(x*8 - ciptr->image_width) < 8)
		ciptr->scale_denom=8;
	else
		ciptr->scale_denom=1;

	jpeg_start_decompress(ciptr);

	px=ciptr->output_width; //py=ciptr->output_height;
	c=ciptr->output_components;


	if(c==3)
	{
		lb=(JSAMPLE*)(*ciptr->mem->alloc_small)((j_common_ptr) ciptr,JPOOL_PERMANENT,c*px);
		bp=*buffer;
		while(ciptr->output_scanline < ciptr->output_height)
		{
			jpeg_read_scanlines(ciptr, &lb, 1);
			memmove(bp,lb,px*c);
			bp+=px*c;
		}

	}
	jpeg_finish_decompress(ciptr);
	jpeg_destroy_decompress(ciptr);
	fclose(fh);

	return(FH_ERROR_OK);
}

int fh_crw_getsize(const char *filename,int *x,int *y, int wanted_width, int wanted_height)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_decompress_struct *ciptr;
	struct r_crw_jpeg_error_mgr emgr;

	int px,py;
	FILE *fh;

	ciptr=&cinfo;
	if(!(fh=fopen(filename,"rb"))) return(FH_ERROR_FILE);

	ciptr->err=jpeg_std_error(&emgr.pub);
	emgr.pub.error_exit=crw_cb_error_exit;
	if(setjmp(emgr.envbuffer)==1)
	{
		// FATAL ERROR - Free the object and return...
		jpeg_destroy_decompress(ciptr);
		fclose(fh);

		return(FH_ERROR_FORMAT);
	}

	jpeg_create_decompress(ciptr);
	fh_crw_find_jpeg_thumbnail(fh);
	jpeg_stdio_src(ciptr,fh);
	jpeg_read_header(ciptr,TRUE);
	ciptr->out_color_space=JCS_RGB;
	// should be more flexible...
	if((int)ciptr->image_width/8 >= wanted_width ||
      (int)ciptr->image_height/8 >= wanted_height)
		ciptr->scale_denom=8;
	else if((int)ciptr->image_width/4 >= wanted_width ||
      (int)ciptr->image_height/4 >= wanted_height)
		ciptr->scale_denom=4;
	else if((int)ciptr->image_width/2 >= wanted_width ||
           (int)ciptr->image_height/2 >= wanted_height)
		ciptr->scale_denom=2;
	else
		ciptr->scale_denom=1;

	jpeg_start_decompress(ciptr);
	px=ciptr->output_width; py=ciptr->output_height;
//	c=ciptr->output_components;
	*x=px; *y=py;
//	jpeg_finish_decompress(ciptr);
	jpeg_destroy_decompress(ciptr);
	fclose(fh);

	return(FH_ERROR_OK);
}

