/*
 * $Id: ci.cpp,v 1.13 2013/08/18 11:23:30 mohousch Exp $
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

#include <stdio.h>

#include <cstring>

#include <ci.h>
#include <messagetools.h>	/* get_length_field_size */


extern int curpmtpid;

/*
 * conditional access descriptors
 */
CCaDescriptor::CCaDescriptor(const unsigned char * const buffer)
{
	descriptor_tag = buffer[0];
	descriptor_length = buffer[1];
	CA_system_ID = (buffer[2] << 8) | buffer[3];
	reserved1 = buffer[4] >> 5;
	CA_PID = ((buffer[4] & 0x1F) << 8) | buffer[5];

	private_data_byte = std::vector<unsigned char>(&(buffer[6]), &(buffer[descriptor_length + 2]));
}

unsigned int CCaDescriptor::writeToBuffer(unsigned char * const buffer) // returns number of bytes written
{
	buffer[0] = descriptor_tag;
	buffer[1] = descriptor_length;
	buffer[2] = CA_system_ID >> 8;
	buffer[3] = CA_system_ID;
	buffer[4] = (reserved1 << 5) | (CA_PID >> 8);
	buffer[5] = CA_PID;

	std::copy(private_data_byte.begin(), private_data_byte.end(), &(buffer[6]));

	return descriptor_length + 2;
}


/*
 * generic table containing conditional access descriptors
 */
void CCaTable::addCaDescriptor(const unsigned char * const buffer)
{
	CCaDescriptor * dummy = new CCaDescriptor(buffer);
	ca_descriptor.push_back(dummy);
	
	if (info_length == 0)
		info_length = 1;
	
	info_length += dummy->getLength();
}

// ci
unsigned int CCaTable::writeToBuffer(unsigned char * const buffer) // returns number of bytes written
{
	buffer[0] = (reserved2 << 4) | (info_length >> 8);
	buffer[1] = info_length;

	if (info_length == 0)
		return 2;

	buffer[2] = 1;                  // ca_pmt_cmd_id: ok_descrambling= 1;
	
	unsigned int pos = 3;

	for (unsigned int i = 0; i < ca_descriptor.size(); i++)
		pos += ca_descriptor[i]->writeToBuffer(&(buffer[pos]));
	
	return pos;
}

// cam
unsigned int CCaTable::CamwriteToBuffer(unsigned char * const buffer) // returns number of bytes written
{ 
	unsigned int pos = 0;

	for (unsigned int i = 0; i < ca_descriptor.size(); i++)
		pos += ca_descriptor[i]->writeToBuffer(&(buffer[pos]));
	
	return pos;
}

CCaTable::~CCaTable(void)
{
	for (unsigned int i = 0; i < ca_descriptor.size(); i++)
		delete ca_descriptor[i];
}


/*
 * elementary stream information
 */
// ci
unsigned int CEsInfo::writeToBuffer(unsigned char * const buffer) // returns number of bytes written
{
	buffer[0] = stream_type;
	buffer[1] = (reserved1 << 5) | (elementary_PID >> 8);
	buffer[2] = elementary_PID;
	
	return 3 + CCaTable::writeToBuffer(&(buffer[3]));
}

// cam
unsigned int CEsInfo::CamwriteToBuffer(unsigned char * const buffer) // returns number of bytes written
{
	int len = 0;
	
	buffer[0] = stream_type;
	buffer[1] = ((reserved1 << 5) | (elementary_PID >> 8)) & 0xff;
	buffer[2] = elementary_PID & 0xff;
	
	/* len! */
	len = CCaTable::writeToBuffer(&(buffer[6]));
	
	if(len) 
	{
		buffer[5] = 0x1; // ca_pmt_cmd_id: ok_descrambling= 1;
		len++;
	}
	
	buffer[3] = ((len & 0xf00)>>8);
	buffer[4] = (len & 0xff);
	
	return len + 5;	
}

/*
 * contitional access program map table
 */
CCaPmt::~CCaPmt(void)
{
	for (unsigned int i = 0; i < es_info.size(); i++)
		delete es_info[i];
}

// ci
unsigned int CCaPmt::writeToBuffer(unsigned char * const buffer, int /*demux*/, int /*camask*/) // returns number of bytes written
{
	unsigned int pos = 0;
	unsigned int i;

	buffer[pos++] = 0x9F;    // ca_pmt_tag
	buffer[pos++] = 0x80;    // ca_pmt_tag
	buffer[pos++] = 0x32;    // ca_pmt_tag

	pos += write_length_field(&(buffer[pos]), getLength());
	
	buffer[pos++] = ca_pmt_list_management;
	buffer[pos++] = program_number >> 8;
	buffer[pos++] = program_number;
	buffer[pos++] = (reserved1 << 6) | (version_number << 1) | current_next_indicator;

	pos += CCaTable::writeToBuffer(&(buffer[pos]));

	for (i = 0; i < es_info.size(); i++)
		pos += es_info[i]->writeToBuffer(&(buffer[pos]));

	return pos;
}

// ci
unsigned int CCaPmt::getLength(void)  // the (3 + length_field()) initial bytes are not counted !
{
	unsigned int size = 4 + CCaTable::getLength();

	for (unsigned int i = 0; i < es_info.size(); i++)
		size += es_info[i]->getLength();

	return size;	
}

// Cam
unsigned int CCaPmt::CamwriteToBuffer(CZapitChannel * thischannel, unsigned char * const buffer, int demux, int camask) // returns number of bytes written
{
	unsigned int i;

	memcpy(buffer, "\x9f\x80\x32\x82\x00\x00", 6);

	buffer[6] = ca_pmt_list_management; 						//6
	buffer[7] = program_number >> 8; 						//7 
	buffer[8] = program_number; 							// 8
	buffer[9] = (reserved1 << 6) | (version_number << 1) | current_next_indicator;
	buffer[10] = 0x00; 								// //reserved - prg-info len
	buffer[11] = 0x00; 								// prg-info len
	buffer[12] = 0x01;  								// ca pmt command id
	buffer[13] = 0x81;  								// private descr.. dvbnamespace
	buffer[14] = 0x08; 								//14
	
	buffer[15] = thischannel->getSatellitePosition() >> 8;				// getSatellitePosition() >> 8;	
	buffer[16] = thischannel->getSatellitePosition() & 0xFF;			// getSatellitePosition() & 0xFF;
	buffer[17] = thischannel->getFreqId() >> 8;					// getFreqId() >> 8;
	buffer[18] = thischannel->getFreqId() & 0xFF;					// getFreqId() & 0xFF;
	buffer[19] = thischannel->getTransportStreamId() >> 8;				// getTransportStreamId() >> 8;
	buffer[20] = thischannel->getTransportStreamId() & 0xFF;			// getTransportStreamId() & 0xFF;
	buffer[21] = thischannel->getOriginalNetworkId() >> 8;				// getOriginalNetworkId() >> 8;
	buffer[22] = thischannel->getOriginalNetworkId() & 0xFF; 			// getOriginalNetworkId() & 0xFF;
	
	buffer[23] = 0x82;  								// demuxer kram..
	buffer[24] = 0x02;
	buffer[25] = camask; 								// descramble on caNum
	buffer[26] = demux; 								// get section data from demuxNum
	buffer[27] = 0x84;  								// pmt pid
	buffer[28] = 0x02;
	buffer[29] = (curpmtpid >> 8) & 0xFF;
	buffer[30] = curpmtpid & 0xFF; 							// 30

        int lenpos = 10;
        int len = 19;
        int wp = 31;

	i = CCaTable::CamwriteToBuffer(&(buffer[wp]));
	wp += i;
	len += i;

	buffer[lenpos]=((len & 0xf00)>>8);
	buffer[lenpos+1]=(len & 0xff);

	for (i = 0; i < es_info.size(); i++) {
		wp += es_info[i]->writeToBuffer(&(buffer[wp]));
	}
	buffer[4] = ((wp-6)>>8) & 0xff;
	buffer[5]=(wp-6) & 0xff;

	return wp;
}

// Cam
unsigned int CCaPmt::CamgetLength(void) 
{
	unsigned int size = 25 + CCaTable::getLength();
	
	for (unsigned int i = 0; i < es_info.size(); i++)
		size += es_info[i]->getLength();

	return size;	
}
