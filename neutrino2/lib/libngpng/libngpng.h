/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: libngpng.h 01062024 mohousch Exp $

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


#ifndef __libngpng__
#define __libngpng__

#include <stdint.h>
#include <string>


#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

enum 
{
	TM_EMPTY  = 0,
	TM_NONE   = 1,
	TM_BLACK  = 2,
	TM_INI    = 3
};

enum ScalingMode
{
	SCALE_SIMPLE = 1,
	SCALE_COLOR = 2
};

struct cformathandler 
{
	struct cformathandler * next;
	int (*get_size)(const char *, int *, int*, int, int);
	int (*get_pic)(const char *, unsigned char **, int *, int *);
	int (*id_pic)(const char *);
};
	
typedef struct cformathandler CFormathandler;

////
void init_handlers(void);
void deinit_handlers(void);
CFormathandler *fh_getsize(const char * name, int * x, int * y, int width_wanted, int height_wanted);
////
void getSize(const std::string &name, int * width, int * height, int * nbpp, int *channels);
uint8_t *resize(uint8_t * origin, int ox, int oy, int dx, int dy, ScalingMode type = SCALE_COLOR, bool alpha = false);
uint8_t * convertRGB2FB(uint8_t *rgbbuff, unsigned long x, unsigned long y, bool alpha = false, int transp = 0xFF, int m_transparent = TM_BLACK);
uint8_t *getImage(const std::string &name, int width = 0, int height = 0, int transp = 0xFF, ScalingMode scaletype = SCALE_COLOR);
uint8_t *getBitmap(const std::string &name);

#endif

