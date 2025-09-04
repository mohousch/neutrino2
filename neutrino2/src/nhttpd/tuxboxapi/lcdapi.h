/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'
        Copyright (C) 2005 SnowHead

        $Id: lcdapi.h 04092025 mohousch Exp $

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

//
#include <driver/lcdd.h>


//
class CLCDAPI
{
	CLCD *lcdd;
	
	public:
		CLCDAPI();
		~CLCDAPI(void){};
		
		void lockDisplay(int lock);
		bool showPng(char *filename);
		void clear(void);
		bool shotPng(char *filename);
};

#endif /* __nhttpd_lcdapi_h__ */
 
