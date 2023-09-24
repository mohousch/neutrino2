/*
	$Id: dmx_cs.cpp,v 1.0 2013/08/18 11:23:30 mohousch Exp $
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>

#include <errno.h>

#include "dmx_cs.h"
#include "video_cs.h"
#include <system/debug.h>


////
extern cVideo *videoDecoder;		// defined in video_cs.cpp

static const char * FILENAME = "[dmx_cs.cpp]";

////
#define NUM_DEMUXDEV 3
static bool init[NUM_DEMUXDEV] = { false, false, false };

static const char * aDMXCHANNELTYPE[] = {
	"",
	"DMX_VIDEO_CHANNEL",
	"DMX_AUDIO_CHANNEL",
	"DMX_PES_CHANNEL",
	"DMX_PSI_CHANNEL",
	"DMX_PIP_CHANNEL",
	"DMX_TP_CHANNEL",
	"DMX_PCR_ONLY_CHANNEL"
};

cDemux::cDemux(int)
{  
	// dmx file descriptor
	demux_fd = -1;
	
	demux_adapter = 0;
	demux_num = 0;
	demux_source = DMX_SOURCE_FRONT0;
}

cDemux::~cDemux()
{  
	dprintf(DEBUG_INFO, "cDemux::%s(%d)\n", __FUNCTION__, demux_num);
	
	Close();
}

bool cDemux::Open(DMX_CHANNEL_TYPE Type, int uBufferSize, CFrontend * fe)
{
	if(fe)
	{
		demux_adapter = fe->feadapter;
		demux_num = fe->fenumber;
		demux_source = fe->fenumber;
	}
	
	int flags = O_RDWR;
	type = Type;
	
	if (type != DMX_PSI_CHANNEL)
		flags |= O_NONBLOCK;	
	
	// close device
	if (demux_fd > -1) 
		close(demux_fd);
	
	char devname[256];

	// open/reopen
	sprintf(devname, "/dev/dvb/adapter%d/demux%d", demux_adapter, demux_num);

	demux_fd = ::open(devname, flags);

	// can not open
	if (demux_fd < 0 && demux_num > 0)
	{
		sprintf(devname, "/dev/dvb/adapter%d/demux0", demux_adapter);
		
		demux_fd = ::open(devname, flags);
	}
	
	if (demux_fd < 0)
		return false;

	dprintf(DEBUG_NORMAL, "cDemux::Open %s type:%s BufferSize:%d source(%d)\n", devname, aDMXCHANNELTYPE[Type], uBufferSize, demux_source);

	// set demux source	
	if (!init[demux_num])
	{	
		if (::ioctl(demux_fd, DMX_SET_SOURCE, &demux_source) < 0)
		{
			perror("DMX_SET_SOURCE");
		}
		else
			init[demux_num] = true;
	}	

	// set demux buffer size
	if (uBufferSize > 0)
	{
		if (::ioctl(demux_fd, DMX_SET_BUFFER_SIZE, uBufferSize) < 0)
			perror("DMX_SET_BUFFER_SIZE");
	}

	return true;
}

void cDemux::Close(void)
{ 
	if(demux_fd < 0)
		return;
	
	dprintf(DEBUG_INFO, "%s:%s dmx(%d,%d) type=%s Pid 0x%x\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type], pid);	

	close(demux_fd);

	demux_fd = -1;
}

bool cDemux::Start(void)
{  
	if (demux_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s dmx(%d,%d) type=%s Pid 0x%x\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type], pid);

#if !defined (__sh__)
        if (::ioctl(demux_fd , DMX_START) < 0)
                perror("DMX_START"); 
#endif      

	return true;
}

bool cDemux::Stop(void)
{  
	if(demux_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s dmx(%d,%d) type=%s Pid 0x%x\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type], pid);
	
	if( ::ioctl(demux_fd, DMX_STOP) < 0)
		perror("DMX_STOP");
	
	return true;
}

int cDemux::Read(unsigned char * buff, int len, int Timeout)
{
	int rc;
	struct pollfd ufds;
	ufds.fd = demux_fd;
	ufds.events = POLLIN;
	ufds.revents = 0;
	
	if (demux_fd < 0 || buff == NULL)
		return -1;
	
	if (type == DMX_PSI_CHANNEL && Timeout <= 0)
		Timeout = 60 * 1000;

	if (Timeout > 0)
	{
retry:	  
		rc = ::poll(&ufds, 1, Timeout);
		if (!rc)
		{
			return 0; // timeout
		}
		else if (rc < 0)
		{
			if (errno == EINTR)
				goto retry;
			/* we consciously ignore EINTR, since it does not happen in practice */
			return -1;
		}
		
		if (ufds.revents & POLLHUP) /* we get POLLHUP if e.g. a too big DMX_BUFFER_SIZE was set */
		{
			return -1;
		}
		
		if (!(ufds.revents & POLLIN)) /* we requested POLLIN but did not get it? */
		{
			return 0;
		}
	}

	rc = ::read(demux_fd, buff, len);
	
	if (rc < 0)
		perror ("cDemux::Read");

	return rc;
}

bool cDemux::sectionFilter(unsigned short Pid, const unsigned char * const Tid, const unsigned char * const Mask, int len, int Timeout, const unsigned char * const nMask )
{
	if (demux_fd < 0)
		return false;
	
	pid = Pid;
	
	dmx_sct_filter_params sct;
	memset(&sct, 0, sizeof(dmx_sct_filter_params));
	
	if (len > DMX_FILTER_SIZE)
	{
		printf("%s #%d: len too long: %d, DMX_FILTER_SIZE %d\n", __func__, demux_num, len, DMX_FILTER_SIZE);
		len = DMX_FILTER_SIZE;
	}

	/* Pid */
	sct.pid = Pid;

	/* filter */
	memcpy(sct.filter.filter, Tid, len );

	/* mask */
	memcpy(sct.filter.mask, Mask, len );

	/* mode */
	if(nMask)
		memcpy(sct.filter.mode, nMask, len );
	
	/* flag */
	sct.flags = DMX_IMMEDIATE_START|DMX_CHECK_CRC;
	
	/* timeout */
	int to = 0;
	switch (Tid[0]) 
	{
		case 0x00: /* program_association_section */
			to = 2000;
			break;

		case 0x01: /* conditional_access_section */
			to = 6000;
			break;

		case 0x02: /* program_map_section */
			to = 1500;
			break;

		case 0x03: /* transport_stream_description_section */
			to = 10000;
			break;

		/* 0x04 - 0x3F: reserved */

		case 0x40: /* network_information_section - actual_network */
			to = 10000;
			break;

		case 0x41: /* network_information_section - other_network */
			to = 15000;
			break;

		case 0x42: /* service_description_section - actual_transport_stream */
			to = 10000;
			break;

		/* 0x43 - 0x45: reserved for future use */

		case 0x46: /* service_description_section - other_transport_stream */
			to = 10000;
			break;

		/* 0x47 - 0x49: reserved for future use */

		case 0x4A: /* bouquet_association_section */
			to = 11000;
			break;

		/* 0x4B - 0x4D: reserved for future use */

		case 0x4E: /* event_information_section - actual_transport_stream, present/following */
			to = 2000;
			break;

		case 0x4F: /* event_information_section - other_transport_stream, present/following */
			to = 10000;
			break;

		case 0x70: /* time_date_section */ /* UTC */
			sct.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
			sct.flags |= DMX_ONESHOT;
			//sct.pid     = 0x0014;
			to = 30000;
			break;

		case 0x71: /* running_status_section */
			sct.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
			to = 0;
			break;

		case 0x72: /* stuffing_section */
			sct.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
			to = 0;
			break;

		case 0x73: /* time_offset_section */ /* UTC */
			sct.flags |= DMX_ONESHOT;
			//sct.pid     = 0x0014;
			to = 30000;
			break;

		/* 0x74 - 0x7D: reserved for future use */

		case 0x7E: /* discontinuity_information_section */
			sct.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
			to = 0;
			break;

		case 0x7F: /* selection_information_section */
			to = 0;
			break;

		/* 0x80 - 0x8F: ca_message_section */
		/* 0x90 - 0xFE: user defined */
		/*        0xFF: reserved */
		default:
			//return -1;
			break;
	}
	
	if (Timeout == 0 && nMask == NULL)
		sct.timeout = to;
	
	dprintf(DEBUG_INFO, "%s:%s dmx(%d,%d) type=%s Pid=0x%x Len=%d Timeout=%d\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type], Pid, len, sct.timeout);

	/* Set Demux Section Filter() */
	if (ioctl(demux_fd, DMX_SET_FILTER, &sct) < 0)
	{
		perror("DMX_SET_FILTER");
		return false;
	}

	return true;
}

bool cDemux::pesFilter(const unsigned short Pid, const dmx_input_t Input)
{  
	if (demux_fd < 0)
		return false;
	
	dprintf(DEBUG_NORMAL, "%s:%s dmx(%d,%d) type=%s Pid=0x%x\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type], Pid);
	
	if ((Pid >= 0x0002 && Pid <= 0x000f) || Pid >= 0x1fff)
		return false;
	
	if(demux_fd <= 0)
		return false;

	struct dmx_pes_filter_params pes;
	memset(&pes, 0, sizeof(struct dmx_pes_filter_params));

	pid = Pid;

	pes.pid      	= Pid;
	pes.input    	= Input;
	pes.output   	= DMX_OUT_DECODER;
#if defined (__sh__)
	pes.flags    	= DMX_IMMEDIATE_START;
#else
	pes.flags	= 0;
#endif

	switch(type) 
	{
		case DMX_VIDEO_CHANNEL:		  
			pes.pes_type = DMX_PES_VIDEO;
			break;
			
		case DMX_AUDIO_CHANNEL:		  
			pes.pes_type = DMX_PES_AUDIO;
			break;
			
		case DMX_PCR_ONLY_CHANNEL:		  
			pes.pes_type = DMX_PES_PCR;
			break;
			
		case DMX_PES_CHANNEL:		  
			pes.output   = DMX_OUT_TAP; 		/* to memory */
			pes.pes_type = DMX_PES_OTHER;
			break;
		
		case DMX_TP_CHANNEL:
#if HAVE_DVB_API_VERSION >= 5
			pes.output   = DMX_OUT_TSDEMUX_TAP;     /* to demux */	/* Output multiplexed into a new TS  */
#else
			pes.output   = DMX_OUT_TS_TAP; 
#endif	
			pes.pes_type = DMX_PES_OTHER;
			break;
			
		case DMX_PIP_CHANNEL:
			pes.pes_type = DMX_PES_VIDEO1; //for pip channel we need only video
			break;
			
		default:
			printf("[%s] %s unknown pesFilter type %s\n", FILENAME, __FUNCTION__, aDMXCHANNELTYPE[type]);
			return false;
	}

	if (ioctl(demux_fd, DMX_SET_PES_FILTER, &pes) < 0)
	{
		perror("DMX_SET_PES_FILTER");
		return false;
	}

	return true;
}

// addPid
void cDemux::addPid(unsigned short Pid)
{ 
	dprintf(DEBUG_NORMAL, "%s:%s type=%s Pid=0x%x\n", FILENAME, __FUNCTION__, aDMXCHANNELTYPE[type], Pid);
	
	if(demux_fd <= 0)
		return;

	if (ioctl(demux_fd, DMX_ADD_PID, &Pid) < 0)
		perror("DMX_ADD_PID");

	return;
}

// remove pid
void cDemux::removePid(unsigned short Pid)
{
	if(demux_fd <= 0)
		return;
	
	dprintf(DEBUG_INFO, "%s:%s type=%s Pid=0x%x\n", FILENAME, __FUNCTION__, aDMXCHANNELTYPE[type], Pid);	

	if (ioctl(demux_fd, DMX_REMOVE_PID, &Pid) < 0)
		perror("DMX_ADD_PID");
	
	return;
}

void cDemux::getSTC(int64_t * STC)
{ 
#if 1
	if (demux_fd < 0)
		return;
	
	dprintf(DEBUG_DEBUG, "%s:%s dmx(%d,%d) type=%s STC=\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type]);	
	
	struct dmx_stc stc;

	stc.num =  demux_num;
	stc.base = 1;
	
	if(ioctl(demux_fd, DMX_GET_STC, &stc) < 0 )
		perror("DMX_GET_STC");
	
	*STC = (int64_t)stc.stc;
#else
	dprintf(DEBUG_DEBUG, "%s:%s dmx(%d,%d) type=%s STC=\n", FILENAME, __FUNCTION__, demux_adapter, demux_num, aDMXCHANNELTYPE[type]);	
	
	int64_t pts = 0;
	if (videoDecoder)
		pts = videoDecoder->GetPTS();
	*STC = pts;
#endif
}


