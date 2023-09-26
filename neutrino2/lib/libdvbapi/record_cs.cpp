/*
	$Id: record_cs.cpp,v 1.0 2013/08/18 11:23:30 mohousch Exp $
	
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
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h> 

#include <sys/types.h>
#include <unistd.h>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>
#include <system/helpers.h>

#include "record_cs.h"


static const char * FILENAME = "[record_cs.cpp]";

/* helper function to call the cpp thread loop */
void * execute_record_thread(void *c)
{
	cRecord * obj = (cRecord *)c;
	obj->RecordThread();
	
	return NULL;
}

cRecord::cRecord(int /*num*/)
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	dmx = NULL;
	record_thread_running = false;
	file_fd = -1;
	exit_flag = RECORD_STOPPED;
}

cRecord::~cRecord()
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	Stop();
}

bool cRecord::Open()
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	exit_flag = RECORD_STOPPED;
	return true;
}

bool cRecord::Start(int fd, unsigned short vpid, unsigned short * apids, int numpids, CFrontend *fe)
{
	dprintf(DEBUG_INFO, "%s %s\n", FILENAME, __FUNCTION__);
	
	int i = 0;

	if (!dmx)
		dmx = new cDemux();

	dmx->Open(DMX_TP_CHANNEL, RECORD_STREAM_BUFFER_SIZE, fe);
	dmx->pesFilter(vpid);

	for (i = 0; i < numpids; i++)
		dmx->addPid(apids[i]);

	file_fd = fd;
	exit_flag = RECORD_RUNNING;

	i = pthread_create(&record_thread, 0, execute_record_thread, this);

	if (i != 0)
	{
		exit_flag = RECORD_FAILED_READ;
		errno = i;
		dprintf(DEBUG_INFO, "cRecord::Start: error creating thread!\n");
		
		if (dmx)
		{
			dmx->Stop();
			delete dmx;
			dmx = NULL;
		}

		return false;
	}
	record_thread_running = true;
	
	return true;
}

bool cRecord::Stop(void)
{
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	exit_flag = RECORD_STOPPED;

	if (record_thread_running)
		pthread_join(record_thread, NULL);

	record_thread_running = false;

	if (dmx)
	{
		dmx->Stop();
		delete dmx;
		dmx = NULL;
	}

	if (file_fd != -1)
	{
		::close(file_fd);
		file_fd = -1;
	}
	
	return true;
}

void cRecord::RecordThread()
{
#define BUFSIZE (1 << 20) /* 1024 kB */
#define READSIZE (BUFSIZE / 16)

	ssize_t r = 0;
	int buf_pos = 0;
	uint8_t * buf;
	buf = (uint8_t *)malloc(BUFSIZE);

	if (!buf)
	{
		exit_flag = RECORD_FAILED_MEMORY;
		dprintf(DEBUG_INFO, "unable to allocate buffer! (out of memory)\n");
	}

	if(dmx)
		dmx->Start();
	
	while (exit_flag == RECORD_RUNNING)
	{
		if (buf_pos < BUFSIZE)
		{
			int toread = BUFSIZE - buf_pos;
			if (toread > READSIZE)
				toread = READSIZE;
			
			r = dmx->Read(buf + buf_pos, toread, 50);

			if (r < 0)
			{
				if (errno != EAGAIN && errno != EOVERFLOW )
				{
					dprintf(DEBUG_INFO, "read failed\n");
					exit_flag = RECORD_FAILED_READ;
					break;
				}
				
				printf("%s: %s\n", __FUNCTION__, errno == EOVERFLOW ? "EOVERFLOW" : "EAGAIN");
			}
			else
			{
				buf_pos += r;
			}
		}
		
		// start writeout
		if (buf_pos > (BUFSIZE / 3))
		{
			size_t towrite = BUFSIZE / 2;
			if (buf_pos < BUFSIZE / 2)
				towrite = buf_pos;
				
			r = write(file_fd, buf, towrite);
			if (r < 0)
			{
				exit_flag = RECORD_FAILED_FILE;

				break;
			}
			buf_pos -= r;
			memmove(buf, buf + r, buf_pos);
		}
	}
	
	if(dmx)
		dmx->Stop();
	
	// write
	while (buf_pos > 0)
	{
		r = write(file_fd, buf, buf_pos);
		
		if (r < 0)
		{
			exit_flag = RECORD_FAILED_FILE;
			dprintf(DEBUG_INFO, "write error\n");
			break;
		}
		
		buf_pos -= r;
		memmove(buf, buf + r, buf_pos);
	}
	
	free(buf);
	
	pthread_exit(NULL);
}

