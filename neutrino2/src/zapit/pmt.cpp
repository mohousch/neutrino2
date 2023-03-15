/*
 * $Id: pmt.cpp,v 1.41 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
 * (C) 2002 by Frank Bormann <happydude@berlios.de>
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <system/debug.h>

/* zapit */
#include <zapit/settings.h>
#include <zapit/descriptors.h>
#include <zapit/pmt.h>
#include <dmx_cs.h>

#include <linux/dvb/dmx.h>

#include <zapit/frontend_c.h>


#define PMT_SIZE 1024

#define RECORD_MODE 0x4
extern int currentMode;
extern short scan_runs;


/*
 * Stream types
 * ------------
 * 0x01 ISO/IEC 11172 Video
 * 0x02 ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
 * 0x03 ISO/IEC 11172 Audio
 * 0x04 ISO/IEC 13818-3 Audio
 * 0x05 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections, e.g. MHP Application signalling stream
 * 0x06 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data, e.g. teletext or ac3
 * 0x0b ISO/IEC 13818-6 type B
 * 0x81 User Private (MTV)
 * 0x90 User Private (Premiere Mail, BD_DVB)
 * 0xc0 User Private (Canal+)
 * 0xc1 User Private (Canal+)
 * 0xc6 User Private (Canal+)
 */

unsigned short CPmt::parseESInfo(const unsigned char * const buffer, CZapitChannel * const channel, CCaPmt * const caPmt)
{
	dprintf(DEBUG_NORMAL, "CPmt::parseESInfo:\n");
	
	unsigned short ES_info_length;
	unsigned short pos;
	unsigned char descriptor_tag;
	unsigned char descriptor_length;
	unsigned char i;

	bool isAC3 = false;
	bool isDTS = false;
	bool isAAC = false;
	bool isDTSHD = false;
	bool isEAC3 = false;
	bool isAACPLUS = false;
	bool isLPCM = false;
	
	bool descramble = false;
	std::string description = "";
	unsigned char componentTag = 0xFF;

	// elementary stream info for ca pmt
	CEsInfo * esInfo = new CEsInfo();

	esInfo->stream_type = buffer[0];
	esInfo->reserved1 = buffer[1] >> 5;
	esInfo->elementary_PID = ((buffer[1] & 0x1F) << 8) | buffer[2];
	esInfo->reserved2 = buffer[3] >> 4;

	ES_info_length = ((buffer[3] & 0x0F) << 8) | buffer[4];

	// parse descriptor tag
	for (pos = 5; pos < ES_info_length + 5; pos += descriptor_length + 2) 
	{
		descriptor_tag = buffer[pos];
		descriptor_length = buffer[pos + 1];
		unsigned char fieldCount = descriptor_length / 5;

		switch (descriptor_tag) 
		{
			case VIDEO_STREAM_DESCRIPTOR:
				CDescriptors::getInstance()->video_stream_descriptor(buffer + pos);
				break;

			case AUDIO_STREAM_DESCRIPTOR:
				CDescriptors::getInstance()->audio_stream_descriptor(buffer + pos);
				break;

			case REGISTRATION_DESCRIPTOR:
				if (descriptor_length >= 3)
					if (!strncmp((const char*)&buffer[pos + 2], "DTS", 3))
						isDTS = true;
				break;

			case CA_DESCRIPTOR:
				esInfo->addCaDescriptor(buffer + pos);
				break;

			case ISO_639_LANGUAGE_DESCRIPTOR:
				for (i = 0; i < 3; i++)
					description += buffer[pos + i + 2];
				break;

			case MAXIMUM_BITRATE_DESCRIPTOR:
				CDescriptors::getInstance()->Maximum_bitrate_descriptor(buffer + pos);
				break;

			case PRIVATE_DATA_INDICATOR_DESCRIPTOR:
				CDescriptors::getInstance()->Private_data_indicator_descriptor(buffer + pos);
				break;

			case STD_DESCRIPTOR:
				CDescriptors::getInstance()->STD_descriptor(buffer + pos);
				break;
				
			case 0x1C:
				isAACPLUS = true;
				break;
				
			case 0x2B:
				isAAC = true;
				break;
				
			case CAROUSEL_IDENTIFIER_DESCRIPTOR:
				break;

			case VBI_DATA_DESCRIPTOR:
				CDescriptors::getInstance()->VBI_data_descriptor(buffer + pos);
				break;

			case STREAM_IDENTIFIER_DESCRIPTOR:
				componentTag = buffer[pos + 2];
				break;

			case TELETEXT_DESCRIPTOR:
				for (unsigned char fIdx = 0; fIdx < fieldCount; fIdx++) 
				{
					char tmpLang[4];
					memcpy(tmpLang, &buffer[pos + 5*fIdx + 2], 3);
					tmpLang[3] = '\0';
					unsigned char teletext_type = buffer[pos + 5*fIdx + 5]>> 3;
					unsigned char teletext_magazine_number = buffer[pos + 5*fIdx + 5] & 7;
					unsigned char teletext_page_number = buffer[pos + 5*fIdx + 6];
					if (teletext_type == 0x02)
					{
						channel->addTTXSubtitle(esInfo->elementary_PID, tmpLang, teletext_magazine_number, teletext_page_number);
					} 
					else 
					{
						if (teletext_type == 0x05)
						{
							channel->addTTXSubtitle(esInfo->elementary_PID, tmpLang, teletext_magazine_number, teletext_page_number, true);
						}
					}
				}

				channel->setTeletextPid(esInfo->elementary_PID);
				descramble = true;//FIXME ORF HD scramble txt ?
				break;

			case SUBTITLING_DESCRIPTOR:
				if (esInfo->stream_type == 0x06) 
				{
					unsigned char fieldCount1 = descriptor_length/8;
					for (unsigned char fIdx = 0; fIdx < fieldCount1; fIdx++)
					{
						char tmpLang[4];
						memcpy(tmpLang, &buffer[pos + 8*fIdx + 2], 3);
						tmpLang[3] = '\0';
						unsigned char subtitling_type = buffer[pos + 8*fIdx + 5];
						unsigned short composition_page_id = *((unsigned short*)(&buffer[pos + 8*fIdx + 6]));
						unsigned short ancillary_page_id = *((unsigned short*)(&buffer[pos + 8*fIdx + 8]));
							
						/*dvbsub */
						channel->addDVBSubtitle(esInfo->elementary_PID, tmpLang, subtitling_type, composition_page_id, ancillary_page_id);
					}
					descramble = true;//FIXME MGM / 10E scrambling subtitles ?
				}

				CDescriptors::getInstance()->subtitling_descriptor(buffer + pos);
				break;

			case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
				CDescriptors::getInstance()->private_data_specifier_descriptor(buffer + pos);
				break;

			case DATA_BROADCAST_ID_DESCRIPTOR:
				CDescriptors::getInstance()->data_broadcast_id_descriptor(buffer + pos);
				break;

			case AC3_DESCRIPTOR:
				isAC3 = true;
				break;

			case APPLICATION_SIGNALLING_DESCRIPTOR:
				channel->setaitPid(esInfo->elementary_PID);
				dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: channel->setaitPid(0x%x)\n", esInfo->elementary_PID);			
				break;
				
			case ENHANCED_AC3_DESCRIPTOR:
				isEAC3 = true;
				break;

			case DTS_DESCRIPTOR:
				isDTS = true;
				break;

			case AAC_DESCRIPTOR:
				isAACPLUS = true;
				break;

			case 0xC5:
				for (i = 0; i < 24; i++)
					description += buffer[pos + i + 3];
				break;

			default:
				dprintf(DEBUG_INFO, "CPmt::parseESInfo: descriptor_tag: 0x%02x\n", descriptor_tag);
				break;
		}
	}

	// parse stream type
	switch (esInfo->stream_type) 
	{
		case 0x01:	// MPEG 1 video
		case 0x02:	// MPEG 2 video
		case 0x1b: 	// AVC Video Stream (MPEG4 H264)
		case 0x10:	// MPEG 4 Part 2
		case 0x24:	// H265 HEVC
		case 0x27:	// H265 HEVC
		case 0x42: 	// CAVS
			channel->setVideoPid(esInfo->elementary_PID);
			descramble = true;

			if(esInfo->stream_type == 0x1b || esInfo->stream_type == 0x10)
				channel->videoType = CHANNEL_VIDEO_MPEG4;
			else if(esInfo->stream_type == 0x24 || esInfo->stream_type == 0x27)
				channel->videoType = CHANNEL_VIDEO_HEVC;
			else if(esInfo->stream_type == 0x42)
				channel->videoType = CHANNEL_VIDEO_CAVS;

			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: vpid 0x%x stream 0x%02x type 0x%02x\n", esInfo->elementary_PID, esInfo->stream_type, channel->videoType);
			break;

		case 0x03:
		case 0x04: // audio es_pids
			if (description == "")
				description = esInfo->elementary_PID;
			
			if(scan_runs) 
			{
				if(channel->getPreAudioPid() == 0)
					channel->setAudioPid(esInfo->elementary_PID);
			} 
			else
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::MPEG, description, componentTag);
			
			descramble = true;
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;

		case 0x05:// private section
			{
				int tmp = 0;	
				// Houdini: shameless stolen from enigma dvbservices.cpp
				for (pos = 5; pos < ES_info_length + 5; pos += descriptor_length + 2) 
				{
					descriptor_tag = buffer[pos];
					descriptor_length = buffer[pos + 1];

					switch (descriptor_tag) 
					{
						case PRIVATE_DATA_SPECIFIER_DESCRIPTOR:
							if ( ((buffer[pos + 2]<<24) | (buffer[pos + 3]<<16) | (buffer[pos + 4]<<8) | (buffer[pos + 5])) == 190 )
								tmp |= 1;
							break;
							
						case 0x90:
							{
								if ( descriptor_length == 4 && !buffer[pos + 2] && !buffer[pos + 3] && buffer[pos + 4] == 0xFF && buffer[pos + 5] == 0xFF )
									tmp |= 2;
							}
							//break;??
	
						case APPLICATION_SIGNALLING_DESCRIPTOR:
							channel->setaitPid(esInfo->elementary_PID);
							dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: channel->setaitPid(0x%x)\n", esInfo->elementary_PID);
							break;
							
						default:
							break;
					}
				}
				
				if ( tmp == 3 ) 
				{
					channel->setPrivatePid(esInfo->elementary_PID);
					dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: channel->setPrivatePid(0x%x)\n", esInfo->elementary_PID);
				}
				descramble = true;
				break;
			}
			
		case 0x81: 	// AC3
		case 0xA1: 	// bluray secondary AC3
			esInfo->stream_type = 0x6;
			if (description == "")
				description = esInfo->elementary_PID;
			description += " (AC3)";
			isAC3 = true;
			descramble = true;
			
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::AC3, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;
			
		case 0x06:
			if ( (isAC3) || (isDTS) || (isAAC) || (isAACPLUS) || (isEAC3)) 
			{
				if (description == "") 
				{
					description = esInfo->elementary_PID;
					if (isAC3)
						description += " (AC3)";
					else if (isDTS)
						description += " (DTS)";
					else if (isAAC)
	                                        description += " (AAC)";
					else if (isAACPLUS)
	                                        description += " (AACPLUS)";
					else if (isEAC3)
	                                        description += " (EAC3)";
				}
				
				if(!scan_runs)
				{
					CZapitAudioChannel::ZapitAudioChannelType Type;
					if (isAC3)
						Type = CZapitAudioChannel::AC3;
					else if (isDTS)
						Type = CZapitAudioChannel::DTS;
					else if (isAAC)
						Type = CZapitAudioChannel::AAC;
					else if (isAACPLUS)
						Type = CZapitAudioChannel::AACPLUS;
					else if (isEAC3)
						Type = CZapitAudioChannel::EAC3;
					else
						Type = CZapitAudioChannel::UNKNOWN;
					
					channel->addAudioChannel(esInfo->elementary_PID, Type, description, componentTag);
				}
				descramble = true;
				
				dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			}
			break;
			
		case 0x0F: 	// AAC 
			if (description == "")
				description = esInfo->elementary_PID;
			
			description += " (AAC)";
			isAAC = true;
			descramble = true;
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::AAC, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;
	        case 0x11:	 // AACPLUS
			if (description == "")
				description = esInfo->elementary_PID;
			
			description += " (AACPLUS)";
			isAACPLUS = true;
			descramble = true;
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::AACPLUS, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;
			
		case 0x80: // user private ... but bluray LPCM
		case 0xA0: // bluray secondary LPCM
			if (description == "")
				description = esInfo->elementary_PID;
			
			description += " (LPCM)";
			isLPCM = true;
			descramble = true;
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::LPCM, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;
			
		case 0x82: // bluray DTS (dvb user private...)
		case 0xA2: // bluray secondary DTS
			if (description == "")
				description = esInfo->elementary_PID;
			
			description += " (DTS)";
			isDTS = true;
			descramble = true;
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::DTS, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;
		
		case 0x85: // bluray DTS-HD HRA(dvb user private...)
		case 0x86: // bluray DTS-HD MA(dvb user private...)
		case 0xA6: // bluray secondary DTS-HD
			if (description == "")
				description = esInfo->elementary_PID;
			
			description += " (DTSHD)";
			isDTSHD = true;
			descramble = true;
			if(!scan_runs)
				channel->addAudioChannel(esInfo->elementary_PID, CZapitAudioChannel::DTSHD, description, componentTag);
			
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: apid 0x%x %s\n", esInfo->elementary_PID, description.c_str());
			break;

		default:
			dprintf(DEBUG_NORMAL, "CPmt::parseESInfo: stream_type: 0x%02x\n", esInfo->stream_type);
			break;
	}

	if (descramble)
		caPmt->es_info.insert(caPmt->es_info.end(), esInfo);
	else
		delete esInfo;

	return ES_info_length;
}

int curpmtpid;
int pmt_caids[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int CPmt::parsePMT(CZapitChannel * const channel, CFrontend * fe)
{
	dprintf(DEBUG_NORMAL, "CPmt::parsePMT:\n");
	
	if(!channel)
		return -1;
	
	if(!fe)
		return -1;
	
	unsigned short i;
	unsigned char buffer[PMT_SIZE];

	// length of elementary stream description
	unsigned short ES_info_length;

	// TS_program_map_section elements
	unsigned short section_length;
	unsigned short program_info_length;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	dprintf(DEBUG_NORMAL, "CPmt::parsePMT: pid 0x%X\n", channel->getPmtPid());

	if (channel->getPmtPid() == 0)
	{
		return -1;
	}
	
	cDemux * dmx = new cDemux(); 
	
	// open
#if defined (PLATFORM_COOLSTREAM)
	dmx->Open(DMX_PSI_CHANNEL);
#else	
	dmx->Open( DMX_PSI_CHANNEL, PMT_SIZE, fe );
#endif	

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x02;	// pmt tid 
	filter[1] = channel->getServiceId() >> 8;
	filter[2] = channel->getServiceId();
	filter[3] = 0x01;	// current_next_indicator 
	filter[4] = 0x00;	// section_number
	mask[0] = 0xFF;
	mask[1] = 0xFF;
	mask[2] = 0xFF;
	mask[3] = 0x01;
	mask[4] = 0xFF;

	if ( (dmx->sectionFilter(channel->getPmtPid(), filter, mask, 5) < 0) || (dmx->Read(buffer, PMT_SIZE) < 0) ) 
	{
		dprintf(DEBUG_NORMAL, "CPmt::parsePMT: dmx read failed\n");
		
		delete dmx;
		return -1;
	}
	
	delete dmx;
	
	// current pmt pid
	curpmtpid = channel->getPmtPid();

	int pmtlen;
	
	pmtlen = ((buffer[1]&0xf)<<8) + buffer[2] + 3;
	
	// pmt.tmp
	FILE *fout;

	if( !(currentMode & RECORD_MODE) && !scan_runs) 
	{
		// write /tmp/pmt.tmp
		fout = fopen("/tmp/pmt.tmp","wb"); 
		
		if(fout != NULL) 
		{
			if ((int) fwrite(buffer, sizeof(unsigned char), pmtlen, fout) != pmtlen) 
			{
				unlink("/tmp/pmt.tmp");
			}
			fclose(fout);
		}
	}

	//caids[]
	int ia, dpmtlen, pos;
	unsigned char descriptor_length = 0;
	
	for(i = 0; i < 11; i++)
		pmt_caids[i] = 0;
	
	dpmtlen = 0;
	pos = 10;
	if(!scan_runs) 
	{
		while(pos + 2 < pmtlen) 
		{
			dpmtlen = ((buffer[pos] & 0x0f) << 8) | buffer[pos + 1];
			for ( ia = pos + 2; ia < (dpmtlen + pos + 2); ia += descriptor_length + 2 ) 
			{
				descriptor_length = buffer[ia + 1];
				if ( ia < pmtlen - 4 )
				{
					if(buffer[ia] == 0x09 && buffer[ia + 1] > 0) 
					{
						switch(buffer[ia+2]) 
						{
							case 0x06: pmt_caids[0] = 1;
								   break;
							case 0x17: pmt_caids[1] = 1;
								   break;
							case 0x01: pmt_caids[2] = 1;
								   break;
							case 0x05: pmt_caids[3] = 1;
								   break;
							case 0x18: pmt_caids[4] = 1;
								   break;
							case 0x0B: pmt_caids[5] = 1;
								   break;
							case 0x0D: pmt_caids[6] = 1;
								   break;
							case 0x09: pmt_caids[7] = 1;
								   break;
							case 0x26: pmt_caids[8] = 1;
								   break;
							case 0x4a: pmt_caids[9] = 1;
								   break;
							case 0x0E: pmt_caids[10] = 1;
								   break;
						} //switch
					} // if
				}
			} // for
			pos += dpmtlen + 5;
		} // while
	} // if !scan_runs
	
	// ca pmt
	CCaPmt * caPmt = new CCaPmt();

	// ca pmt 
	caPmt->program_number = (buffer[3] << 8) + buffer[4];
	caPmt->reserved1 = buffer[5] >> 6;
	caPmt->version_number = (buffer[5] >> 1) & 0x1F;
	caPmt->current_next_indicator = buffer[5] & 0x01;
	caPmt->reserved2 = buffer[10] >> 4;

	dprintf(DEBUG_NORMAL, "CPmt::parsePMT: pcr pid: old 0x%x new 0x%x\n", channel->getPcrPid(), ((buffer[8] & 0x1F) << 8) + buffer[9]);

	// reset pids
	if(channel->getCaPmt() != 0) 
	{
		if(channel->getCaPmt()->version_number != caPmt->version_number)
			channel->resetPids();
	}
	
	// capmt descriptor
	section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
	channel->setPcrPid(((buffer[8] & 0x1F) << 8) + buffer[9]);
	program_info_length = ((buffer[10] & 0x0F) << 8) | buffer[11];

	if (program_info_length)
	{
		for (i = 12; i < 12 + program_info_length; i += buffer[i + 1] + 2)
		{
			switch (buffer[i]) 
			{
				case CA_DESCRIPTOR:
					caPmt->addCaDescriptor(buffer + i);
					break;
				default:
					dprintf(DEBUG_DEBUG, "decriptor_tag: %02x\n", buffer[i]);
					break;
			}
		}
	}

	// capmt parse ES_Info
	for (i = 12 + program_info_length; i < section_length - 1; i += ES_info_length + 5)
		ES_info_length = parseESInfo(buffer + i, channel, caPmt);

	if(scan_runs) 
	{
		channel->setCaPmt(NULL);
		channel->setRawPmt(NULL);
		
		delete caPmt;
	} 
	else 
	{
		channel->setCaPmt(caPmt);
		
		// setpmtraw
		unsigned char * p = new unsigned char[pmtlen];
		memmove(p, buffer, pmtlen);
		channel->setRawPmt(p, pmtlen);
	}

	channel->setPidsFlag();

	return 0;
}

//
cDemux * pmtDemux = NULL;

int CPmt::pmt_set_update_filter( CZapitChannel * const channel, int * fd, CFrontend * fe)
{
	dprintf(DEBUG_NORMAL, "CPmt::pmt_set_update_filter:\n");
	
	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];
	unsigned char mode[DMX_FILTER_SIZE];
	
	if(!fe)
		return -1;

	if(pmtDemux == NULL) 
		pmtDemux = new cDemux();
	
	// open 
#if defined (PLATFORM_COOLSTREAM)
	pmtDemux->Open(DMX_PSI_CHANNEL);
#else	
	pmtDemux->Open(DMX_PSI_CHANNEL, PMT_SIZE, fe );
#endif	

	if (channel->getPmtPid() == 0)
		return -1;

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);
	memset(mode, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x02;	// pmt tid 
	filter[1] = channel->getServiceId() >> 8;
	filter[2] = channel->getServiceId();
	filter[4] = 0x00;	// section_number 

	mask[0] = 0xFF;
	mask[1] = 0xFF;
	mask[2] = 0xFF;
	mask[4] = 0xFF;

	dprintf(DEBUG_NORMAL, "[pmt] pmt_set_update_filter: sid 0x%x pid 0x%x version 0x%x\n", channel->getServiceId(), channel->getPmtPid(), channel->getCaPmt()->version_number);
	
	filter[3] = (channel->getCaPmt()->version_number << 1) | 0x01;
	mask[3] = (0x1F << 1) | 0x01;
	mode[3] = 0x1F << 1;
	
	pmtDemux->sectionFilter(channel->getPmtPid(), filter, mask, 5, 0, mode);

	*fd = 1;

	return 0;
}

int CPmt::pmt_stop_update_filter(int * fd)
{
	dprintf(DEBUG_NORMAL, "[pmt] pmt_stop_update_filter\n");

	if (pmtDemux)
		pmtDemux->Stop();

	*fd = -1;
	
        return 0;
}



