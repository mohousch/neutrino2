/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'
        Copyright (C) 2005 SnowHead

        $Id: lcdapi.h,v 1.2 2007/11/26 20:48:41 yjogol Exp $

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
#ifndef __nhttpd_lcdapi_h__
#define __nhttpd_lcdapi_h__

// tuxbox
#include <driver/lcdd.h>
#include <liblcddisplay/lcddisplay.h>
#include <liblcddisplay/fontrenderer.h>

class CLCDDisplay;
class LcdFontRenderClass;

//-------------------------------------------------------------------------
class CLCDAPI
{
	CLCD				*vfd;
public:
	CLCDAPI();
	~CLCDAPI(void);
	void LockDisplay(int lock);

	bool ShowPng(char *filename);

	void Clear(void);
	bool ShotPng(char *filename);
	
#ifdef ENABLE_GRAPHLCD
	bool ShowNgPng(char *filename);
	bool ShotNgPng(char *filename);
#endif
};

#endif /* __nhttpd_lcdapi_h__ */
 
