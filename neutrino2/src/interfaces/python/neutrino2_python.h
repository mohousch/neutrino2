//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: neutrino2_python.h 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifndef __neutrino_python_h__
#define __neutrino_python_h__

#include <string>


class neutrinoPython
{
	private:

	public:
		neutrinoPython();
		~neutrinoPython();

		int execFile(const char *fileName);
		int execute(const std::string &moduleName, const std::string &funcName);
};

#endif



