/*
 * $Id: nit.cpp 20.10.2023 mohousch Exp $
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

#include <syscall.h>

/* system c++ */
#include <map>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <unistd.h>

/* system */
#include <system/debug.h>

/* zapit */
#include <zapit/zapit.h>
#include <zapit/descriptors.h>
#include <zapit/nit.h>

// libdvbapi
#include <dmx_cs.h>


#define NIT_SIZE 1024

int CNit::parseNIT(t_satellite_position satellitePosition, freq_id_t freq, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, "CNit::parseNIT:\n");
	
	int ret = 0;
	int secdone[255];
	int sectotal = -1;

	memset(secdone, 0, 255);
	
	cDemux * dmx = new cDemux();
		
	dmx->Open(DMX_PSI_CHANNEL, NIT_SIZE, fe);	

	unsigned char buffer[NIT_SIZE];

	// position in buffer
	unsigned short pos;
	unsigned short pos2;

	// network_information_section elements 
	unsigned short section_length;
	unsigned short network_descriptors_length;
	unsigned short transport_descriptors_length;
	unsigned short transport_stream_loop_length;

	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	unsigned short network_id;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x40;	// nit tid
	mask[0] = 0xFF;

	if (dmx->sectionFilter(0x10, filter, mask, 5) < 0) 
	{
		delete dmx;
		return -1;
	}

	do {
		if (dmx->Read(buffer, NIT_SIZE) < 0) 
		{
			dprintf(DEBUG_NORMAL, "CNit::parseNIT: dmx read failed\n");
			
			delete dmx;
			return -1;
		}

		if(buffer[0] != 0x40)
			printf("CNit::parseNIT: Bogus section received: 0x%x\n", buffer[0]);

		section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
		network_id = ((buffer[3] << 8)| buffer [4]);
		network_descriptors_length = ((buffer[8] & 0x0F) << 8) | buffer[9];
		unsigned char secnum = buffer[6];
		
		dprintf(DEBUG_NORMAL, "CNit::parseNIT: section 0x%x last 0x%x network_id 0x%x -> %s\n", secnum, buffer[7], network_id, secdone[secnum] ? "skip" : "use");

		if(secdone[secnum]) // mark sec XX done
			continue;

		secdone[secnum] = 1;
		sectotal++;

		for (pos = 10; pos < network_descriptors_length + 10; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
				case NETWORK_NAME_DESCRIPTOR:
					descriptor.network_name_descriptor(buffer + pos);
					break;

				case LINKAGE_DESCRIPTOR:
					descriptor.linkage_descriptor(buffer + pos);
					break;

				case MULTILINGUAL_NETWORK_NAME_DESCRIPTOR:
					descriptor.multilingual_network_name_descriptor(buffer + pos);
					break;

				case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
					descriptor.private_data_specifier_descriptor(buffer + pos);
					break;

				default:
					dprintf(DEBUG_DEBUG, "first_descriptor_tag: %02x\n", buffer[pos]);
					descriptor.generic_descriptor(buffer + pos);
					break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];

		if (!transport_stream_loop_length)
			continue;

		for (pos += 2; pos < section_length - 3; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos] << 8) | buffer[pos + 1];
			original_network_id = (buffer[pos + 2] << 8) | buffer[pos + 3];
			transport_descriptors_length = ((buffer[pos + 4] & 0x0F) << 8) | buffer[pos + 5];

			//
			for (pos2 = pos + 6; pos2 < pos + transport_descriptors_length + 6; pos2 += buffer[pos2 + 1] + 2)
			{
				switch (buffer[pos2])
				{
					case SERVICE_LIST_DESCRIPTOR:
						descriptor.service_list_descriptor(buffer + pos2, transport_stream_id, original_network_id, satellitePosition, freq);
						break;

					case STUFFING_DESCRIPTOR:
						descriptor.stuffing_descriptor(buffer + pos2);
						break;

					case SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR:
						if (descriptor.satellite_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, satellitePosition, freq, fe) < 0)
						{
							ret = -2;
							goto _return;
						}
						break;

					case CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
						if (descriptor.cable_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, satellitePosition, freq, fe) < 0)
						{
							ret = -2;
							goto _return;
						}
						break;

					case TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
						if(descriptor.terrestrial_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, satellitePosition, freq, fe) < 0)
						{
							ret = -2;
							goto _return;
						}
						break;

					case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
						descriptor.private_data_specifier_descriptor(buffer + pos2);
						break;

					case FREQUENCY_LIST_DESCRIPTOR:
						descriptor.frequency_list_descriptor(buffer + pos2);
						break;
						
					case EXTENSION_DESCRIPTOR: // T2
						if(descriptor.terrestrial2_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, satellitePosition, freq, fe) < 0)
						{
							ret = -2;
							goto _return;
						}
						break;

					default:
						dprintf(DEBUG_DEBUG, "CNit::parseNIT: second_descriptor_tag: %02x\n", buffer[pos]);
						descriptor.generic_descriptor(buffer + pos2);
						break;
				}
			}
		}
	} while(sectotal < buffer[7]);

_return:
	delete dmx;
	
	return ret;
}

