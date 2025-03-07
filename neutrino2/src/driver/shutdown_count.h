//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: shutdown_count.h 21122024 mohousch Exp $
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

#ifndef __shutdown_count__
#define __shutdown_count__

#include <configfile.h>
#include <pthread.h>


class SHTDCNT
{
	private:

		pthread_t	thrTime;
		unsigned int	shutdown_cnt;
		CConfigFile	configfile;

		void shutdown_counter();
		SHTDCNT();

		static void* TimeThread(void*);

	public:
		static SHTDCNT* getInstance();
		void init();
};

#endif
