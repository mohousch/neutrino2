/*
 * radiotools.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This is a "plugin" for the Video Disk Recorder (VDR).
 *
 * Written by:                  Lars Tegeler <email@host.dom>
 *
 * Project's homepage:          www.math.uni-paderborn.de/~tegeler/vdr
 *
 * Latest version available at: URL
 *
 * See the file COPYING for license information.
 *
 * Description:
 *
 * This Plugin display an background image while the vdr is switcht to radio channels.
 *
 * $Id: radiotools.cpp,v 1.1 2009/08/07 07:22:31 rhabarber1848 Exp $
 
*/

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


