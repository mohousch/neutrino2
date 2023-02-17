/*
	$Id: record_cs.h,v 1.0 2013/08/18 11:23:30 mohousch Exp $
	
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

#ifndef __RECORD_TD_H
#define __RECORD_TD_H

#include <pthread.h>

#include <string>

#include <zapit/frontend_c.h>
#include "dmx_cs.h"


typedef enum {
	RECORD_RUNNING,
	RECORD_STOPPED,
	RECORD_FAILED_READ,	/* failed to read from DMX */
	RECORD_FAILED_OVERFLOW,	/* cannot write fast enough */
	RECORD_FAILED_FILE,	/* cannot write to file */
	RECORD_FAILED_MEMORY	/* out of memory */
} record_state_t;

class cRecord
{
	private:
		int file_fd;
		cDemux * dmx;
		pthread_t record_thread;
		bool record_thread_running;
		record_state_t exit_flag;

		// file recording
		FILE * fp;
		std::string url;
		
	public:
		cRecord(int num = 0);
		~cRecord();

		bool Open();
		bool Start(int fd, unsigned short vpid, unsigned short *apids, int numpids, CFrontend *fe = NULL);
		bool Start(int fd, std::string uri);
		bool Stop(void);

		void RecordThread();
};


#endif	/* record_cs.h*/
