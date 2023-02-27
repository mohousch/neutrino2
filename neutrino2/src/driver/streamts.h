/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: streamts.h 27.02.2023 mohousch Exp $

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
#ifndef __streamts__
#define __streamts__

#include <string>


class CStreamTS
{
	private:
		CStreamTS(){};
	public:
		~CStreamTS(){};
		
		static CStreamTS *getInstance()
		{
			static CStreamTS * instance = NULL;

			if(!instance) 
			{
				instance = new CStreamTS();
			} 

			return instance;
		};
		
		void Start();
		void Stop();
};

#endif
