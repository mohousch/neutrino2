/*
 * $Id: cam.cpp 20.10.2023 11:23:30 mohousch Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>,
 *             thegoodguy         <thegoodguy@berlios.de>
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

/* zapit */
#include <zapit/cam.h>
#include <zapit/settings.h> /* CAMD_UDS_NAME         */

#include <messagetools.h>   /* get_length_field_size */

#include <string.h>


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

//
unsigned int CCaTable::writeToBuffer(unsigned char * const buffer) // returns number of bytes written
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
unsigned int CEsInfo::writeToBuffer(unsigned char * const buffer) // returns number of bytes written
{
	int len = 0;
	
	buffer[0] = stream_type;
	buffer[1] = ((reserved1 << 5) | (elementary_PID >> 8)) & 0xff;
	buffer[2] = elementary_PID & 0xff;
	
	//
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

//
unsigned int CCaPmt::writeToBuffer(CZapitChannel * thisChannel, unsigned char * const buffer, int demux, int camask, bool addPrivate) // returns number of bytes written
{
	unsigned int i;

	memcpy(buffer, "\x9f\x80\x32\x82\x00\x00", 6);

	buffer[6] = ca_pmt_list_management; 						// 6
	buffer[7] = program_number >> 8; 						// 7 
	buffer[8] = program_number; 							// 8
	buffer[9] = (reserved1 << 6) | (version_number << 1) | current_next_indicator;
	buffer[10] = 0x00; 								// //reserved - prg-info len
	buffer[11] = 0x00; 								// prg-info len
	buffer[12] = 0x01;  
	
	//
	if (addPrivate)
	{											// ca pmt command id
		buffer[13] = 0x81;  								// private descr.. dvbnamespace
		buffer[14] = 0x08; 								//14
		
		buffer[15] = thisChannel->getSatellitePosition() >> 8;				// getSatellitePosition() >> 8;	
		buffer[16] = thisChannel->getSatellitePosition() & 0xFF;			// getSatellitePosition() & 0xFF;
		buffer[17] = thisChannel->getFreqId() >> 8;					// getFreqId() >> 8;
		buffer[18] = thisChannel->getFreqId() & 0xFF;					// getFreqId() & 0xFF;
		buffer[19] = thisChannel->getTransportStreamId() >> 8;				// getTransportStreamId() >> 8;
		buffer[20] = thisChannel->getTransportStreamId() & 0xFF;			// getTransportStreamId() & 0xFF;
		buffer[21] = thisChannel->getOriginalNetworkId() >> 8;				// getOriginalNetworkId() >> 8;
		buffer[22] = thisChannel->getOriginalNetworkId() & 0xFF; 			// getOriginalNetworkId() & 0xFF;
		
		buffer[23] = 0x82;  								// demuxer kram..
		buffer[24] = 0x02;
		buffer[25] = camask; 								// descramble on caNum
		buffer[26] = demux; 								// get section data from demuxNum
		buffer[27] = 0x84;  								// pmt pid
		buffer[28] = 0x02;
		buffer[29] = (thisChannel->getPmtPid() >> 8) & 0xFF;
		buffer[30] = thisChannel->getPmtPid() & 0xFF;
	}							// 30

        int lenpos = 10;
        int len = 19;
        int wp = addPrivate? 31 : 13;

	i = CCaTable::writeToBuffer(&(buffer[wp]));
	wp += i;
	len += i;

	buffer[lenpos] = ((len & 0xf00)>>8);
	buffer[lenpos + 1] = (len & 0xff);

	for (i = 0; i < es_info.size(); i++) 
	{
		wp += es_info[i]->writeToBuffer(&(buffer[wp]));
	}
	
	buffer[4] = ((wp - 6)>>8) & 0xff;
	buffer[5] = (wp - 6) & 0xff;

	return wp;
}

unsigned int CCaPmt::getLength(bool addPrivate) 
{
	unsigned int size = (addPrivate? 31 : 13) + CCaTable::getLength();
	
	for (unsigned int i = 0; i < es_info.size(); i++)
		size += es_info[i]->getLength();

	return size;	
}

////
unsigned char CCam::getVersion(void) const
{
	return 0x9F;
}

const char *CCam::getSocketName(void) const
{
	return CAMD_UDS_NAME;
}

bool CCam::sendMessage(const char * const data, const size_t length, bool update)
{
	/* send_data return false without trying, if no opened connection */
	if(update) 
	{
		if(!sendData(data, length)) 
		{
			if (!openConnection())
				return false;
				
			return sendData(data, length);
		}
		
		return true;
	}

	closeConnection();

	if(!length) 
		return false;
	
	if (!openConnection())
		return false;

	return sendData(data, length);
}

bool CCam::setCaPmt(CZapitChannel * thischannel, CCaPmt * const caPmt, int demux, int camask, bool update)
{
	if (!caPmt)
		return true;

	printf("CCam::setCaPmt: demux_index:(%d) camask:(%d) update:(%s)\n", demux, camask, update ? "yes" : "no" );
	
	//
	unsigned int size = caPmt->getLength(true);
	unsigned char buffer[3 + get_length_field_size(size) + size];
	size_t pos = caPmt->writeToBuffer(thischannel, buffer, demux, camask, true);

	return sendMessage((char *)buffer, pos, update);
}

