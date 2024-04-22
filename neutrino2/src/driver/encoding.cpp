/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/src/driver/encoding.cpp,v 1.2 2003/09/27 11:48:09 thegoodguy Exp $
 *
 * conversion of character encodings - d-box2 linux project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <driver/encoding.h>


std::string UTF8_to_Latin1(const char * s)
{
	std::string r;

	while ((*s) != 0)
	{
		if (((*s) & 0xf0) == 0xf0)      /* skip (can't be encoded in Latin1) */
		{
			s++;
			if ((*s) == 0)
				return r;
			s++;
			if ((*s) == 0)
				return r;
			s++;
			if ((*s) == 0)
				return r;
		}
		else if (((*s) & 0xe0) == 0xe0) /* skip (can't be encoded in Latin1) */
		{
			s++;
			if ((*s) == 0)
				return r;
			s++;
			if ((*s) == 0)
				return r;
		}
		else if (((*s) & 0xc0) == 0xc0)
		{
			char c = (((*s) & 3) << 6);
			s++;
			if ((*s) == 0)
				return r;
			r += (c | ((*s) & 0x3f));
		}
		else r += *s;
		s++;
	}
	return r;
}

std::string UTF8_to_UTF8XML(const char * s)
{
	std::string r;
	
	while ((*s) != 0)
	{
		// cf. http://www.w3.org/TR/xhtml1/dtds.html
		switch (*s)
		{
			case '<':           
				r += "&lt;";
				break;
			case '>':
				r += "&gt;";
				break;
			case '&':
				r += "&amp;";
				break;
			case '\"':
				r += "&quot;";
				break;
			case '\'':
				r += "&apos;";
				break;
			default:
				r += *s;
		}
		s++;
	}

	return r;
}

std::string Latin1_to_UTF8(const char * s)
{
	std::string r;
	
	while((*s) != 0)
	{
		unsigned char c = *s;

		if (c < 0x80)
			r += c;
		else
		{
			unsigned char d = 0xc0 | (c >> 6);
			r += d;
			d = 0x80 | (c & 0x3f);
			r += d;
		}

		s++;
	}		
	return r;
}

std::string UTF8ToString(const char*&text)
{
	std::string res;

	res = *text;
	if ((((unsigned char)(*text)) & 0x80) != 0)
	{
		int remaining_unicode_length = 0;
		if ((((unsigned char)(*text)) & 0xf8) == 0xf0)
			remaining_unicode_length = 3;
		else if ((((unsigned char)(*text)) & 0xf0) == 0xe0)
			remaining_unicode_length = 2;
		else if ((((unsigned char)(*text)) & 0xe0) == 0xc0)
			remaining_unicode_length = 1;

		for (int i = 0; i < remaining_unicode_length; i++)
		{
			text++;
			if (((*text) & 0xc0) != 0x80)
				break;
			res += *text;
		}
	}
	
	return res;
}


