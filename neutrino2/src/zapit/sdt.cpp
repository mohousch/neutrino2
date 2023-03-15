/*
 * $Id: sdt.cpp,v 1.45 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

/* system */
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <system/debug.h>

/* zapit */
#include <zapit/descriptors.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/zapittypes.h>
#include <zapit/bouquets.h>

#include <dmx_cs.h>


#define SDT_SIZE 	MAX_SECTION_LENGTH

// sdt scan
int CSdt::parseSDT(t_transport_stream_id *p_transport_stream_id,t_original_network_id *p_original_network_id,t_satellite_position satellitePosition, freq_id_t freq, int feindex)
{
	dprintf(DEBUG_NORMAL, "CSdt::parseSDT:\n");
	
	int secdone[255];
	int sectotal = -1;

	memset(secdone, 0, 255);
	
	dprintf(DEBUG_NORMAL, "[sdt] %s:\n", __FUNCTION__);

	cDemux * dmx = new cDemux();
	
	//open
#if defined (PLATFORM_COOLSTREAM)
	dmx->Open(DMX_PSI_CHANNEL);
#else	
	dmx->Open( DMX_PSI_CHANNEL, SDT_SIZE, CZapit::getInstance()->getFE(feindex) );
#endif	

	unsigned char buffer[SDT_SIZE];

	// position in buffer 
	unsigned short pos;
	unsigned short pos2;

	// service_description_section elements
	unsigned short section_length;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
	unsigned short service_id;
	unsigned short descriptors_loop_length;
	unsigned short running_status;

	bool EIT_schedule_flag;
	bool EIT_present_following_flag;
	bool free_CA_mode;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x42;	// sdt tid 
	mask[0] = 0xFF;

	if (dmx->sectionFilter(0x11, filter, mask, 1) < 0) 
	{
		delete dmx;
		return -1;
	}

	do {
		if (dmx->Read(buffer, SDT_SIZE) < 0) 
		{
			dprintf(DEBUG_NORMAL, "CSdt::parseSDT:: dmx read failed\n");
			
			delete dmx;
			return -1;
		}

		if(buffer[0] != 0x42)
		        printf("parse_sdt: fe(%d) Bogus section received: 0x%x\n", feindex, buffer[0]);

		section_length = ((buffer[1] & 0x0F) << 8) | buffer[2];
		transport_stream_id = (buffer[3] << 8) | buffer[4];
		original_network_id = (buffer[8] << 8) | buffer[9];

		unsigned char secnum = buffer[6];
		dprintf(DEBUG_NORMAL, "CSdt::parseSDT:: section %X last %X tsid 0x%x onid 0x%x -> %s\n", buffer[6], buffer[7], transport_stream_id, original_network_id, secdone[secnum] ? "skip" : "use");

		if(secdone[secnum])
			continue;

		secdone[secnum] = 1;
		sectotal++;

		*p_transport_stream_id = transport_stream_id;
		*p_original_network_id = original_network_id;

		for (pos = 11; pos < section_length - 1; pos += descriptors_loop_length + 5) 
		{
			service_id = (buffer[pos] << 8) | buffer[pos + 1];
			EIT_schedule_flag = buffer[pos + 2] & 0x02;
			EIT_present_following_flag = buffer[pos + 2] & 0x01;
			running_status = buffer [pos + 3] & 0xE0;
			free_CA_mode = buffer [pos + 3] & 0x10;
			descriptors_loop_length = ((buffer[pos + 3] & 0x0F) << 8) | buffer[pos + 4];

			for (pos2 = pos + 5; pos2 < pos + descriptors_loop_length + 5; pos2 += buffer[pos2 + 1] + 2) 
			{
				switch (buffer[pos2]) 
				{
					case 0x0A:
						CDescriptors::getInstance()->ISO_639_language_descriptor(buffer + pos2);
						break;
	
					/*
					case 0x40:
						CDescriptors::getInstance()->network_name_descriptor(buffer + pos2);
						break;
					*/

					case 0x42:
						CDescriptors::getInstance()->stuffing_descriptor(buffer + pos2);
						break;
	
					case 0x47:
						CDescriptors::getInstance()->bouquet_name_descriptor(buffer + pos2);
						break;
	
					case 0x48:
						CDescriptors::getInstance()->service_descriptor(buffer + pos2, service_id, transport_stream_id, original_network_id, satellitePosition, freq, free_CA_mode, feindex);
						break;
	
					case 0x49:
						CDescriptors::getInstance()->country_availability_descriptor(buffer + pos2);
						break;
	
					case 0x4A:
						CDescriptors::getInstance()->linkage_descriptor(buffer + pos2);
						break;
	
					case 0x4B:
						//CDescriptors::getInstance()->NVOD_reference_descriptor(buffer + pos2);
						break;
	
					case 0x4C:
						CDescriptors::getInstance()->time_shifted_service_descriptor(buffer + pos2);
						break;
	
					case 0x51:
						CDescriptors::getInstance()->mosaic_descriptor(buffer + pos2);
						break;
	
					case 0x53:
						CDescriptors::getInstance()->CA_identifier_descriptor(buffer + pos2);
						break;
	
					case 0x5D:
						CDescriptors::getInstance()->multilingual_service_name_descriptor(buffer + pos2);
						break;
	
					case 0x5F:
						CDescriptors::getInstance()->private_data_specifier_descriptor(buffer + pos2);
						break;
	
					case 0x64:
						CDescriptors::getInstance()->data_broadcast_descriptor(buffer + pos2);
						break;
	
					case 0x80: // unknown, Eutelsat 13.0E
						break;
	
					case 0x84: // unknown, Eutelsat 13.0E
						break;
	
					case 0x86: // unknown, Eutelsat 13.0E
						break;
	
					case 0x88: // unknown, Astra 19.2E
						break;
	
					case 0xB2: // unknown, Eutelsat 13.0E
						break;
	
					case 0xC0: // unknown, Eutelsat 13.0E
						break;
	
					case 0xE4: // unknown, Astra 19.2E
						break;
	
					case 0xE5: // unknown, Astra 19.2E 
						break;
	
					case 0xE7: // unknown, Eutelsat 13.0E
						break;
	
					case 0xED: // unknown, Astra 19.2E
						break;
	
					case 0xF8: // unknown, Astra 19.2E
						break;
	
					case 0xF9: // unknown, Astra 19.2E
						break;
	
					default:
						//CDescriptors::getInstance()->generic_descriptor(buffer + pos2);
						break;
				}
			}
		}
	} while(sectotal < buffer[7]);

	delete dmx;

	return 0;
}

//
extern tallchans curchans;

int CSdt::parseCurrentSDT( const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, t_satellite_position satellitePosition, freq_id_t freq, CFrontend * fe)
{ 
	if(!fe)
		return -1;
	
	cDemux * dmx = new cDemux();
	
	// open
#if defined (PLATFORM_COOLSTREAM)
	dmx->Open(DMX_PSI_CHANNEL);
#else	
	dmx->Open( DMX_PSI_CHANNEL, SDT_SIZE, fe );
#endif	
	
	int ret = -1;

	unsigned char buffer[SDT_SIZE];

	// position in buffer
	unsigned short pos;
	unsigned short pos2;

	// service_description_section elements
	unsigned short section_length;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
	unsigned short service_id;
	unsigned short descriptors_loop_length;
	unsigned short running_status;

	bool EIT_schedule_flag;
	bool EIT_present_following_flag;
	bool free_CA_mode;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	curchans.clear();
	
	//
	filter[0] = 0x42;	// sdt tid
	filter[1] = (p_transport_stream_id >> 8) & 0xff;
	filter[2] = p_transport_stream_id & 0xff;
	filter[3] = 0x00;
	filter[4] = 0x00;
	filter[5] = 0x00;
	filter[6] = (p_original_network_id >> 8) & 0xff;
	filter[7] = p_original_network_id & 0xff;
	memset(&filter[8], 0x00, 8);

	mask[0] = 0xFF;
	mask[1] = 0xFF;
	mask[2] = 0xFF;
	mask[3] = 0x00;
	mask[4] = 0xFF;
	mask[5] = 0x00;
	mask[6] = 0xFF;
	mask[7] = 0xFF;
	memset(&mask[8], 0x00, 8);

	do {
		if ((dmx->sectionFilter(0x11, filter, mask, 8) < 0) || (dmx->Read(buffer, SDT_SIZE) < 0)) 
		{
			dprintf(DEBUG_NORMAL, "parse_current_sdt: dmx read failed\n");
			
			delete dmx;
			return ret;
		}
		
		//
		section_length = ((buffer[1] & 0x0F) << 8) | buffer[2];
		transport_stream_id = (buffer[3] << 8) | buffer[4];
		original_network_id = (buffer[8] << 8) | buffer[9];

		for (pos = 11; pos < section_length - 1; pos += descriptors_loop_length + 5) 
		{
			service_id = (buffer[pos] << 8) | buffer[pos + 1];
			EIT_schedule_flag = buffer[pos + 2] & 0x02;
			EIT_present_following_flag = buffer[pos + 2] & 0x01;
			running_status = buffer [pos + 3] & 0xE0;
			free_CA_mode = buffer [pos + 3] & 0x10;
			descriptors_loop_length = ((buffer[pos + 3] & 0x0F) << 8) | buffer[pos + 4];

			for (pos2 = pos + 5; pos2 < pos + descriptors_loop_length + 5; pos2 += buffer[pos2 + 1] + 2) 
			{
				switch (buffer[pos2]) 
				{
					case 0x48:
						CDescriptors::getInstance()->current_service_descriptor(buffer + pos2, service_id, transport_stream_id, original_network_id, satellitePosition, freq);
						ret = 0;
						break;
	
					default:
						//CDescriptors::getInstance()->generic_descriptor(buffer + pos2);
						break;
				}
			}
		}
	}
	while (filter[4]++ != buffer[7]);

	delete dmx;

	return ret;
}

