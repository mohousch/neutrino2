/*
 * $Id: dmx.cpp 2013/10/12 mohousch Exp $
 *
 * DMX class (sectionsd) - d-box2 linux project
 *
 * (C) 2001 by fnbrd,
 *     2003 by thegoodguy <thegoodguy@berlios.de>
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

#include <stdio.h>

#include <dmx.h>
#include <dmxapi.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <string>
#include <cstring>
#include <map>

#include "abstime.h"
#include <system/debug.h>

/*zapit includes*/
#include <zapit/zapit.h>


typedef std::map<sections_id_t, version_number_t, std::less<sections_id_t> > MyDMXOrderUniqueKey;
static MyDMXOrderUniqueKey myDMXOrderUniqueKey;

extern void showProfiling(std::string text);

DMX::DMX(const unsigned short p, const unsigned short bufferSizeInKB, const bool c, int dmx_source)
{
	fd = -1;
	dmx_num = dmx_source;
	lastChanged = time_monotonic();
	filter_index = 0;
	pID = p;
	dmxBufferSizeInKB = bufferSizeInKB;
	pthread_mutex_init(&pauselock, NULL);        // default = fast mutex
	pthread_mutex_init(&start_stop_mutex, NULL); // default = fast mutex
	pthread_cond_init (&change_cond, NULL);
	real_pauseCounter = 0;
	current_service = 0;

	first_skipped = 0;
	cache = c;
}

DMX::~DMX()
{
	first_skipped = 0;
	myDMXOrderUniqueKey.clear();
	pthread_mutex_destroy(&pauselock);
	pthread_mutex_destroy(&start_stop_mutex);
	pthread_cond_destroy (&change_cond);
	//closefd();
	close();
	if(dmx)
	{
		delete dmx;
		dmx = NULL;
	}
}

ssize_t DMX::read(char * const /*buf*/, const size_t /*buflength*/, const unsigned /*timeoutMInSeconds*/)
{
	return 0;
}

void DMX::close(void)
{
//	if(dmx)
//		delete dmx;
//	dmx = NULL;
}

void DMX::closefd(void)
{
	if (isOpen())
	{
		dmx->Stop();

		fd = -1;
	}
}

void DMX::addfilter(const unsigned char filter, const unsigned char mask)
{
	s_filters tmp;
	tmp.filter = filter;
	tmp.mask   = mask;
	
	filters.push_back(tmp);
}

int DMX::immediate_stop(void)
{
	if (!isOpen())
		return 1;

	closefd();

	return 0;
}

int DMX::stop(void)
{
	int rc;

	lock();

	rc = immediate_stop();

	unlock();

	return rc;
}

void DMX::lock(void)
{
	pthread_mutex_lock(&start_stop_mutex);
}

void DMX::unlock(void)
{
	pthread_mutex_unlock(&start_stop_mutex);

	sched_yield();
}

sections_id_t DMX::create_sections_id(const unsigned char table_id, const unsigned short extension_id, const unsigned char section_number, const unsigned short onid, const unsigned short tsid)
{
	return 	(sections_id_t) (	((sections_id_t) table_id 	<< 56) |
					((sections_id_t) extension_id 	<< 40) |
					((sections_id_t) section_number << 32) |
					((sections_id_t) onid		<< 16) |
					((sections_id_t) tsid));
}

bool DMX::check_complete(const unsigned char table_id, const unsigned short extension_id, const unsigned short onid, const unsigned short tsid, const unsigned char last)
{
	int current_section_number = 0;

	if (((table_id == 0x4e) || (table_id == 0x50)) && (current_service == extension_id)) 
	{
		if (last == 0)
			return true;
			
		MyDMXOrderUniqueKey::iterator di = myDMXOrderUniqueKey.find(create_sections_id(
				table_id,
				extension_id,
				current_section_number,
				onid,
				tsid));
		if (di != myDMXOrderUniqueKey.end()) 
		{
			di++;
		}
		
		while ((di != myDMXOrderUniqueKey.end()) && ((uint8_t) ((di->first >> 56) & 0xff) == table_id) &&
				((uint16_t) ((di->first >> 40) & 0xffff) == extension_id) &&
				(((uint8_t) ((di->first >> 32) & 0xff) == current_section_number + 1) ||
				 ((uint8_t) ((di->first >> 32) & 0xff) == current_section_number + 8)) &&
				((uint16_t) ((di->first >> 16) & 0xffff) == onid) &&
				((uint16_t) (di->first & 0xffff) == tsid))
		{
			if ((uint8_t) ((di->first >> 32) & 0xff) == last) 
			{
				return true;
			}
			else 
			{
				current_section_number = (uint8_t) (di->first >> 32) & 0xff;
				di++;
			}
		}
	}
	
	return false;
}

int DMX::getSection(uint8_t *buf, const unsigned timeoutInMSeconds, int &timeouts)
{
	struct minimal_section_header 
	{
		unsigned table_id                 : 8;
#if __BYTE_ORDER == __BIG_ENDIAN
		unsigned section_syntax_indicator : 1;
		unsigned reserved_future_use      : 1;
		unsigned reserved1                : 2;
		unsigned section_length_hi        : 4;
#else
		unsigned section_length_hi        : 4;
		unsigned reserved1                : 2;
		unsigned reserved_future_use      : 1;
		unsigned section_syntax_indicator : 1;
#endif
		unsigned section_length_lo        : 8;
	} __attribute__ ((packed));  // 3 bytes total

	struct extended_section_header 
	{
		unsigned table_extension_id_hi    : 8;
		unsigned table_extension_id_lo    : 8;
#if __BYTE_ORDER == __BIG_ENDIAN
		unsigned reserved		  : 2;
		unsigned version_number		  : 5;
		unsigned current_next_indicator	  : 1;
#else
		unsigned current_next_indicator	  : 1;
		unsigned version_number		  : 5;
		unsigned reserved		  : 2;
#endif
		unsigned section_number		  : 8;
		unsigned last_section_number	  : 8;
	} __attribute__ ((packed));  // 5 bytes total

	struct eit_extended_section_header 
	{
		unsigned transport_stream_id_hi	  : 8;
		unsigned transport_stream_id_lo	  : 8;
		unsigned original_network_id_hi   : 8;
		unsigned original_network_id_lo   : 8;
		unsigned segment_last_section_number : 8;
		unsigned last_table_id		  : 8;
	} __attribute__ ((packed));  // 6 bytes total

	minimal_section_header *initial_header;
	extended_section_header *extended_header;
	eit_extended_section_header *eit_extended_header;
	int    rc;
	short section_length = 0;
	unsigned short current_onid = 0;
	unsigned short current_tsid = 0;

	if ( (pID == 0x12 || pID == 0x39) && filters[filter_index].filter == 0 && filters[filter_index].mask == 0)
	{
		usleep(timeoutInMSeconds * 1000);
		timeouts++;
		return -1;
	}

	lock();

	rc = dmx->Read(buf, MAX_SECTION_LENGTH, timeoutInMSeconds);

	if (rc < 3)
	{
		unlock();
		
		if (rc <= 0)
		{
			timeouts++;
		}
		else
		{
			real_pause();
			real_unpause();
		}
		
		return -1;
	}

	initial_header = (minimal_section_header*)buf;
	section_length = (initial_header->section_length_hi * 256) | initial_header->section_length_lo;

	if (section_length <= 0)
	{
		unlock();

		return -1;
	}

	timeouts = 0;

	if (rc != section_length + 3)
	{
		unlock();
		// DMX restart required? This should never happen anyway.
		real_pause();
		real_unpause();
		return -1;
	}

	// check if the filter worked correctly
	if (((initial_header->table_id ^ filters[filter_index].filter) & filters[filter_index].mask) != 0)
	{
		unlock();
		real_pause();
		real_unpause();
		return -1;
	}

	unlock();
	// skip sections which are too short
	if ((section_length < 5) || (initial_header->table_id >= 0x4e && initial_header->table_id <= 0x6f && section_length < 14))
	{
		return -1;
	}

	// check if it's extended syntax, e.g. NIT, BAT, SDT, EIT
	if (initial_header->section_syntax_indicator != 0)
	{
		extended_header = (extended_section_header *)(buf+3);

		// only current sections
		if (extended_header->current_next_indicator != 0) 
		{
			// if ((initial_header.table_id >= 0x4e) && (initial_header.table_id <= 0x6f))
			if (pID == 0x12 || pID == 0x39) 
			{
				eit_extended_header = (eit_extended_section_header *)(buf+8);
				current_onid = 	eit_extended_header->original_network_id_hi * 256 +
						eit_extended_header->original_network_id_lo;
				current_tsid = 	eit_extended_header->transport_stream_id_hi * 256 +
						eit_extended_header->transport_stream_id_lo;
			}
			else 
			{
				current_onid = 0;
				current_tsid = 0;
			}

			int eh_tbl_extension_id = extended_header->table_extension_id_hi * 256 + extended_header->table_extension_id_lo;

			/* if we are not caching the already read sections (CN-thread), check EIT version and get out */
			if (!cache)
			{
				if (initial_header->table_id == 0x4e && eh_tbl_extension_id == current_service && extended_header->version_number != eit_version) {
					//dprintf(DEBUG_DEBUG, "EIT old: %d new version: %d\n",eit_version,extended_header->version_number);
					eit_version = extended_header->version_number;
				}
				
				return rc;
			}

			// the current section
			sections_id_t s_id = create_sections_id(initial_header->table_id,
								eh_tbl_extension_id,
								extended_header->section_number,
								current_onid,
								current_tsid);
			//find current section in list
			MyDMXOrderUniqueKey::iterator di = myDMXOrderUniqueKey.find(s_id);
			if (di != myDMXOrderUniqueKey.end())
			{
				//the current section was read before
				if (di->second == extended_header->version_number) 
				{
					//the version number is still up2date
					if (first_skipped == 0) 
					{
						//the last section was new - this is the 1st dup
						first_skipped = s_id;
					}
					else 
					{
						//this is not the 1st new - check if it's the last
						//or to be more precise only dups occured since
						if (first_skipped == s_id)
							timeouts = -1;
					}
					
					//since version is still up2date, check if table complete
					if (check_complete(initial_header->table_id,
							   eh_tbl_extension_id,
							   current_onid,
							   current_tsid,
							   extended_header->last_section_number))
						timeouts = -2;
						
					return -1;
				}
				else 
				{
					//update version number
					di->second = extended_header->version_number;
				}
			}
			else
			{
				//section was not read before - insert in list
				uint8_t version_number = extended_header->version_number;
				myDMXOrderUniqueKey.insert(std::make_pair(s_id, version_number));

				//check if table is now complete
				if (check_complete(initial_header->table_id,
						   eh_tbl_extension_id,
						   current_onid,
						   current_tsid,
						   extended_header->last_section_number))
					timeouts = -2;
			}
			//if control comes to here the sections skipped counter must be restarted
			first_skipped = 0;
		}
	}

	return rc;
}

int DMX::immediate_start(void)
{
	if (isOpen())
	{
		closefd();
	}

	if (real_pauseCounter != 0) 
	{
		return 0;
	}

	if(dmx == NULL) 
	{
		dmx = new cDemux();
		dmx->Open( DMX_PSI_CHANNEL, dmxBufferSizeInKB*1024UL, CZapit::getInstance()->getCurrentFrontend() );		
	}

	fd = 1;

	if (CZapit::getInstance()->getCurrentFrontend() && (filters[filter_index].filter && filters[filter_index].mask))
	{
		unsigned char filter[DMX_FILTER_SIZE];
		unsigned char mask[DMX_FILTER_SIZE];

		filter[0] = filters[filter_index].filter;
		mask[0] = filters[filter_index].mask;
		dmx->sectionFilter(pID, filter, mask, 1);
	}
	
	/* this is for dmxCN only... */
	eit_version = 0xff;
	
	return 0;
}

int DMX::start(void)
{
	int rc;

	lock();

	rc = immediate_start();

	unlock();

	return rc;
}

int DMX::real_pause(void)
{
	if (!isOpen()) 
	{
		return 1;
	}

	lock();

	if (real_pauseCounter == 0)
	{
		immediate_stop();
	}

	unlock();

	return 0;
}

int DMX::real_unpause(void)
{
	lock();

	if (real_pauseCounter == 0)
	{
		immediate_start();
	}

	unlock();

	return 0;
}

int DMX::request_pause(void)
{
	real_pause(); // unlocked

	lock();

	real_pauseCounter++;

	unlock();

	return 0;
}


int DMX::request_unpause(void)
{
	lock();

	--real_pauseCounter;

	unlock();

	real_unpause(); // unlocked

	return 0;
}

const char *dmx_filter_types [] = 
{
	"dummy filter",
	"actual transport stream, scheduled",
	"other transport stream, now/next",
	"other transport stream, scheduled 1",
	"other transport stream, scheduled 2"
};

int DMX::change(const int new_filter_index, const int new_current_service)
{
	dprintf(DEBUG_DEBUG, "changeDMX: before pthread_mutex_lock(&start_stop_mutex)\n");
	
	lock();

	dprintf(DEBUG_DEBUG, "changeDMX: after pthread_mutex_lock(&start_stop_mutex)\n");

	filter_index = new_filter_index;
	first_skipped = 0;

	if (new_current_service != -1)
		current_service = new_current_service;

	if (real_pauseCounter > 0)
	{
		dprintf(DEBUG_DEBUG, "changeDMX: for 0x%x not ignored! even though real_pauseCounter> 0 (%d)\n", filters[new_filter_index].filter, real_pauseCounter);
	}

	closefd();
	
	//FIXME:HACK
	close();

	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}

	dprintf(DEBUG_DEBUG, "after DMX_SET_FILTER\n");

	pthread_cond_signal(&change_cond);

	lastChanged = time_monotonic();

	unlock();

	return 0;
}

ssize_t DMX::readNbytes(int _fd, char *buf, const size_t n, unsigned timeoutInMSeconds)
{
	int rc;
	struct pollfd ufds;
	ufds.fd = _fd;
	ufds.events = POLLIN;
	ufds.revents = 0;

	rc = ::poll(&ufds, 1, timeoutInMSeconds);

	if (!rc)
		return 0; // timeout
	else if (rc < 0)
	{
		return -1;
	}
	
	if ((ufds.revents & POLLERR) != 0) /* POLLERR means buffer error, i.e. buffer overflow */
	{
		return -1;
	}
	
	if (!(ufds.revents&POLLIN))
	{
		return 0; // timeout
	}

	int r = ::read(_fd, buf, n);

	if (r >= 0)
		return r;

	perror ("[sectionsd] DMX::readNbytes read");
	
	return -1;
}



int DMX::setPid(const unsigned short new_pid)
{
	lock();

	if (!isOpen())
	{
		pthread_cond_signal(&change_cond);
		unlock();
		return 1;
	}

	if (real_pauseCounter > 0)
	{
		unlock();
		return 0;	// not running (e.g. streaming)
	}
	closefd();

	pID = new_pid;
	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}

	pthread_cond_signal(&change_cond);

	lastChanged = time_monotonic();

	unlock();

	return 0;
}

int DMX::setCurrentService(int new_current_service)
{
	return change(0, new_current_service);
}

int DMX::dropCachedSectionIDs()
{
	lock();


	myDMXOrderUniqueKey.clear();

	pthread_cond_signal(&change_cond);

	unlock();

	return 0;
}

unsigned char DMX::get_eit_version()
{
	return eit_version;
}

unsigned int DMX::get_current_service()
{
	return current_service;
}

