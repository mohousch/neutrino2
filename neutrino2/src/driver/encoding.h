//
//	conversion of character encodings - d-box2 linux project
//
//	$Id: encoding.h 21122024 mohousch Exp $
//
//	(C) 2003 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __neutrino__encoding_h__
#define __neutrino__encoding_h__

#include <string>


std::string UTF8_to_Latin1(const char * s);
std::string UTF8_to_UTF8XML(const char *);
std::string Latin1_to_UTF8(const char *);
std::string UTF8ToString(const char*&text);

#endif /* __neutrino__encoding_h__ */

