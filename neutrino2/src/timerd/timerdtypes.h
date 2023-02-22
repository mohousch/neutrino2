/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdtypes.h,v 1.20 2006/02/28 21:51:01 zwen Exp $

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


#ifndef __timerdtypes__
#define __timerdtypes__

#include <zapit/zapittypes.h>
#include <sectionsd/sectionsdtypes.h>

#include <vector>


#define REMINDER_MESSAGE_MAXLEN 31
#define EXEC_PLUGIN_NAME_MAXLEN 31
#define RECORD_DIR_MAXLEN 100
#define EPG_TITLE_MAXLEN 50

#define TIMERD_APIDS_CONF 0x00
#define TIMERD_APIDS_STD  0x01
#define TIMERD_APIDS_ALT  0x02
#define TIMERD_APIDS_AC3  0x04
#define TIMERD_APIDS_ALL  0xFF

#endif

