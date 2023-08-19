/*
	NeutrinoNG  -   DBoxII-Project

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


#ifndef __neutrino_debug__
#define __neutrino_debug__


#define ANSI_RESET   "\33[0m"
#define ANSI_BOLD    "\33[1m"
#define ANSI_BLINK   "\33[5m"
//foreground color
#define ANSI_BLACK   "\33[30m"
#define ANSI_RED     "\33[31m"
#define ANSI_GREEN   "\33[32m"
#define ANSI_YELLOW  "\33[33m"
#define ANSI_BLUE    "\33[34m"	//do not use, because the contrast is bad
#define ANSI_MAGENTA "\33[35m"
#define ANSI_CYAN    "\33[36m"
#define ANSI_WHITE   "\33[37m"
//foreground bright color
#define ANSI_BBLACK   "\33[90m"
#define ANSI_BRED     "\33[91m"
#define ANSI_BGREEN   "\33[92m"
#define ANSI_BYELLOW  "\33[93m"
#define ANSI_BBLUE    "\33[94m"	//do not use, because the contrast is bad
#define ANSI_BMAGENTA "\33[95m"
#define ANSI_BCYAN    "\33[96m"
#define ANSI_BWHITE   "\33[97m"
//background colors
#define ANSI_bBLACK   "\33[40m"
#define ANSI_bRED     "\33[41m"
#define ANSI_bGREEN   "\33[42m"
#define ANSI_bYELLOW  "\33[43m"
#define ANSI_bBLUE    "\33[44m"
#define ANSI_bMAGENTA "\33[45m"
#define ANSI_bCYAN    "\33[46m"
#define ANSI_bWHITE   "\33[47m"

extern int debug;

#define DEBUG_NORMAL	0
#define DEBUG_INFO	1
#define DEBUG_DEBUG	2

void setDebugLevel( int level );

#define dprintf(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(fmt ANSI_RESET, ## args); \
		}}
	
#endif

