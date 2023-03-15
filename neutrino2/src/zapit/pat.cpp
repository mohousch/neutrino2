/*
 * $Id: pat.cpp,v 1.45 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <system/debug.h>

#include <zapit/pat.h>
#include <dmx_cs.h>
#include <zapit/frontend_c.h>


#define PAT_SIZE 1024

int CPat::parsePAT(CZapitChannel * const channel, CFrontend * fe)
{
	dprintf(DEBUG_NORMAL, "CPat::parsePAT:\n");
	
	if (!channel)
		return -1;
	
	if(!fe)
		return -1;
	
	dprintf(DEBUG_NORMAL, "%s\n", __FUNCTION__);

	cDemux * dmx = new cDemux();
	
	//open
#if defined (PLATFORM_COOLSTREAM)
	dmx->Open(DMX_PSI_CHANNEL);
#else	
	dmx->Open(DMX_PSI_CHANNEL, PAT_SIZE, fe );
#endif	

	// buffer for program association table
	unsigned char buffer[PAT_SIZE];

	// current positon in buffer
	unsigned short i;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	mask[0] = 0xFF;
	mask[4] = 0xFF;

	do {
		if ( (dmx->sectionFilter(0, filter, mask, 5) < 0) || (i = dmx->Read(buffer, PAT_SIZE) < 0))
		{
			dprintf(DEBUG_NORMAL, "CPat::parsePAT: dmx read failed\n");
			
			delete dmx;
			return -1;
		}
		
		// set pids
		for (i = 8; i < (((buffer[1] & 0x0F) << 8) | buffer[2]) + 3; i += 4) 
		{
			if (channel->getServiceId() == ((buffer[i] << 8) | buffer[i + 1])) 
			{
				channel->setPmtPid(((buffer[i + 2] & 0x1F) << 8) | buffer[i + 3]);

				delete dmx;

				return 0;
			}
		}
	
	} while (filter[4]++ != buffer[7]);
	
	delete dmx;

	dprintf(DEBUG_NORMAL, "CPat::parsePAT: sid 0x%X not found..\n", channel->getServiceId());
	
	return -1;
}

