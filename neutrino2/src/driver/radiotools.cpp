//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: radiotools-cpp 21122024 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <radiotools.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>


unsigned short crc16_ccitt(unsigned char *daten, int len, bool skipfirst)
{
	// CRC16-CCITT: x^16 + x^12 + x^5 + 1
	// with start 0xffff and result invers
	register unsigned short crc = 0xffff;

	if (skipfirst) 
		daten++;
		
	while (len--) 
	{
		crc = (crc >> 8) | (crc << 8);
		crc ^= *daten++;
		crc ^= (crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}

	return ~(crc);
}

char *rtrim(char *text)
{
	char *s = text + strlen(text) - 1;
	
	while (s >= text && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
		*s-- = 0;

	return text;
}

