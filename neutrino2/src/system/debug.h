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


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern int debug;

#define DEBUG_NORMAL	0
#define DEBUG_INFO	1
#define DEBUG_DEBUG	2

void setDebugLevel( int level );

#define dprintf(debuglevel, fmt, args...) {if(debug>=debuglevel) printf(fmt, ## args);}

#define dprintfgreen(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(ANSI_COLOR_GREEN);\
			printf(fmt, ## args); \
			printf(ANSI_COLOR_RESET); \
		}}
		
#define dprintfred(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(ANSI_COLOR_RED);\
			printf(fmt, ## args); \
			printf(ANSI_COLOR_RESET); \
		}}
		
#define dprintfyellow(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(ANSI_COLOR_YELLOW);\
			printf(fmt, ## args); \
			printf(ANSI_COLOR_RESET); \
		}}
		
#define dprintfblue(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(ANSI_COLOR_BLUE);\
			printf(fmt, ## args); \
			printf(ANSI_COLOR_RESET); \
		}}
		
#define dprintfmagenta(debuglevel, fmt, args...) {if (debug >= debuglevel) { \
			printf(ANSI_COLOR_BLUE);\
			printf(fmt, ## args); \
			printf(ANSI_COLOR_RESET); \
		}}

#endif

