/*
 * $Id: descriptors.cpp,v 1.66 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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

#include <cstdio>
#include <map>
#include <string>

#include <cerrno>
#include <cstdio>
#include <cstring>

/* system */
#include <system/debug.h>

/* libevent */
#include <eventserver.h>

#include <global.h>
#include <neutrinoMessages.h>

#include <zapit/bouquets.h>
#include <zapit/zapit.h>
#include <zapit/descriptors.h>
#include <zapit/dvbstring.h>
#include <zapit/frontend_c.h>
#include <zapit/getservices.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>

#include <dmx_cs.h>
#include <math.h>


extern CBouquetManager* scanBouquetManager;
extern CBouquetManager *g_bouquetManager;
extern CZapit::scanType _scanType;
extern tallchans allchans;   			//  defined in zapit.cpp
extern tallchans curchans;   			//  defined in zapit.cpp
std::string curr_chan_name;
uint32_t  found_transponders;
uint32_t  found_channels;
std::string lastProviderName;
uint32_t  found_tv_chans;
uint32_t  found_radio_chans;
uint32_t  found_data_chans;
std::string lastServiceName;
std::map <t_channel_id, uint8_t> service_types;

extern CEventServer *eventServer;

void CDescriptors::generic_descriptor(const unsigned char * const)
{
#if 0
	dprintf(DEBUG_NORMAL, "generic descriptor dump:");

	for (unsigned short i = 0; i < buffer[1] + 2; i++)
		printf(" %02x", buffer[i]);
	printf("\n");
#endif
}

/* 0x02 */
void CDescriptors::video_stream_descriptor(const unsigned char * const)
{
}

/* 0x03 */
void CDescriptors::audio_stream_descriptor(const unsigned char * const)
{
}

/* 0x04 */
void CDescriptors::hierarchy_descriptor(const unsigned char * const)
{
}

/* 0x05 */
void CDescriptors::registration_descriptor(const unsigned char * const)
{
}

/* 0x06 */
void CDescriptors::data_stream_alignment_descriptor(const unsigned char * const)
{
}

/* 0x07 */
void CDescriptors::target_background_grid_descriptor(const unsigned char * const)
{
}

/* 0x08 */
void CDescriptors::Video_window_descriptor(const unsigned char * const)
{
}

/* 0x09 */
void CDescriptors::CA_descriptor(const unsigned char * const buffer, uint16_t /*ca_system_id*/, uint16_t* ca_pid)
{
	//if ((((buffer[2] & 0x1F) << 8) | buffer[3]) == ca_system_id)
		*ca_pid = ((buffer[4] & 0x1F) << 8) | buffer[5];
}

/* 0x0A */
void CDescriptors::ISO_639_language_descriptor(const unsigned char * const)
{
}

/* 0x0B */
void CDescriptors::System_clock_descriptor(const unsigned char * const)
{
}

/* 0x0C */
void CDescriptors::Multiplex_buffer_utilization_descriptor(const unsigned char * const)
{
}

/* 0x0D */
void CDescriptors::Copyright_descriptor(const unsigned char * const)
{
}

/* 0x0E */
void CDescriptors::Maximum_bitrate_descriptor(const unsigned char * const)
{
}

/* 0x0F */
void CDescriptors::Private_data_indicator_descriptor(const unsigned char * const)
{
}

/* 0x10 */
void CDescriptors::Smoothing_buffer_descriptor(const unsigned char * const)
{
}

/* 0x11 */
void CDescriptors::STD_descriptor(const unsigned char * const)
{
}

/* 0x12 */
void CDescriptors::IBP_descriptor(const unsigned char * const)
{
}

/*
 * 0x13 ... 0x1A: Defined in ISO/IEC 13818-6
 */

/* 0x1B */
void CDescriptors::MPEG4_video_descriptor(const unsigned char * const)
{
}

/* 0x1C */
void CDescriptors::MPEG4_audio_descriptor(const unsigned char * const)
{
}

/* 0x1D */
void CDescriptors::IOD_descriptor(const unsigned char * const)
{
}

/* 0x1E */
void CDescriptors::SL_descriptor(const unsigned char * const)
{
}

/* 0x1F */
void CDescriptors::FMC_descriptor(const unsigned char * const)
{
}

/* 0x20 */
void CDescriptors::External_ES_ID_descriptor(const unsigned char * const)
{
}

/* 0x21 */
void CDescriptors::MuxCode_descriptor(const unsigned char * const)
{
}

/* 0x22 */
void CDescriptors::FmxBufferSize_descriptor(const unsigned char * const)
{
}

/* 0x23 */
void CDescriptors::MultiplexBuffer_descriptor(const unsigned char * const)
{
}

/* 0x24 */
void CDescriptors::FlexMuxTiming_descriptor(const unsigned char * const)
{
}

/*
 * 0x25 ... 0x39:  ITU-T H.222.0 | ISO/IEC 13818-1 Reserved
 */

/* 0x40 */
void CDescriptors::network_name_descriptor(const unsigned char * const /*buffer*/)
{
#if 0
	unsigned char tag = buffer[0];
	unsigned char len = buffer[1];
	char name[255];
	int i;

	for(i = 0; i < len; i++)
		name[i] = buffer[2+i];
	name[i] = 0;
	printf("[nit] network name: %s\n", name);
#endif
}

/* 0x41 */
void CDescriptors::service_list_descriptor(const unsigned char * const buffer, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	for (int i = 0; i < buffer[1]; i += 3) 
	{
		t_service_id service_id = buffer[i + 2] << 8 | buffer[i + 3];
		t_channel_id channel_id = CREATE_CHANNEL_ID;
		uint8_t service_type = buffer[i+4];

		//printf("[service_list] type %X sid %X\n", service_type, service_id);

		if(service_type == 0x9A) 
			service_type = 1;

		if(service_type == 0x86) 
			service_type = 1;

		service_types[channel_id] = service_type;
	}
}

/* 0x42 */
void CDescriptors::stuffing_descriptor(const unsigned char * const)
{
}

/* 0x43 */
int CDescriptors::satellite_delivery_system_descriptor(const unsigned char * const buffer, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, int feindex)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	FrontendParameters feparams;
	uint8_t polarization;
	stiterator tI;
	transponder_id_t TsidOnid;
	int modulationSystem, modulationType, rollOff, fec_inner;

	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type != FE_QPSK)
		return -1;

	//freq
	feparams.frequency =
		(
		 ((buffer[2] >> 4)	* 100000000) +
		 ((buffer[2] & 0x0F)	* 10000000) +
		 ((buffer[3] >> 4)	* 1000000) +
		 ((buffer[3] & 0x0F)	* 100000) +
		 ((buffer[4] >> 4)	* 10000) +
		 ((buffer[4] & 0x0F)	* 1000) +
		 ((buffer[5] >> 4)	* 100) +
		 ((buffer[5] & 0x0F)	* 10)
		);

	//inv
	feparams.inversion = INVERSION_AUTO;

	//rollof
	rollOff = (buffer[8] >> 4) & 0x03; 	//alpha_0_35, alpha_0_25, alpha_0_20, alpha_auto

	//mod system
	modulationSystem = (buffer[8] >> 2) & 0x01; 	// 1= DVB_S2

	//mod
	modulationType = (buffer[8]) & 0x03; 	// 1=QPSK, 2=M8PSK

	//rate
	feparams.u.qpsk.symbol_rate =
		(
		 ((buffer[9] >> 4)	* 100000000) +
		 ((buffer[9] & 0x0F)	* 10000000) +
		 ((buffer[10] >> 4)	* 1000000) +
		 ((buffer[10] & 0x0F)	* 100000) +
		 ((buffer[11] >> 4)	* 10000) +
		 ((buffer[11] & 0x0F)	* 1000) +
		 ((buffer[12] >> 4)	* 100)
		);

	//fec
	fec_inner = CFrontend::getCodeRate(buffer[12] & 0x0F, modulationSystem);

	if(modulationType == 2)
		fec_inner += 9;

	feparams.u.qpsk.fec_inner = (fe_code_rate_t) fec_inner;

	//pol
	polarization = (buffer[8] >> 5) & 0x03;

	/* workarounds for braindead broadcasters (e.g. on Telstar 12 at 15.0W) */
	if (feparams.frequency >= 100000000)
		feparams.frequency /= 10;

	if (feparams.u.qpsk.symbol_rate >= 50000000)
		feparams.u.qpsk.symbol_rate /= 10;

	freq = feparams.frequency/1000;

	if(feparams.frequency > 15000000) 
	{
		printf("[NIT] Bogus TP: freq %d SR %d fec %d pol %d\n", feparams.frequency, feparams.u.qpsk.symbol_rate, fec_inner, polarization);
		return 0;
	}

	TsidOnid = CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id);

	CScan::getInstance()->addToScan(TsidOnid, &feparams, polarization, true, feindex);

	return 0;
}

/* 0x44 */
int CDescriptors::cable_delivery_system_descriptor(const unsigned char * const buffer, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, int feindex)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	transponder_id_t TsidOnid;

	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type != FE_QAM)
		return -1;

	FrontendParameters feparams;

	//freq
	feparams.frequency =
	(
		((buffer[2] >> 4)	* 1000000000) +
		((buffer[2] & 0x0F)	* 100000000) +
		((buffer[3] >> 4)	* 10000000) +
		((buffer[3] & 0x0F)	* 1000000) +
		((buffer[4] >> 4)	* 100000) +
		((buffer[4] & 0x0F)	* 10000) +
		((buffer[5] >> 4)	* 1000) +
		((buffer[5] & 0x0F)	* 100)
	);

	//inv
	feparams.inversion = INVERSION_AUTO;

	//rate
	feparams.u.qam.symbol_rate =
	(
		((buffer[9] >> 4)	* 100000000) +
		((buffer[9] & 0x0F)	* 10000000) +
		((buffer[10] >> 4)	* 1000000) +
		((buffer[10] & 0x0F)	* 100000) +
		((buffer[11] >> 4)	* 10000) +
		((buffer[11] & 0x0F)	* 1000) +
		((buffer[12] >> 4)	* 100)
	);

	if(feparams.frequency > 1000*1000)
                feparams.frequency /= 1000;

	//fec
	feparams.u.qam.fec_inner = CFrontend::getCodeRate(buffer[12] & 0x0F);

	//mod
	feparams.u.qam.modulation = CFrontend::getModulation(buffer[8]);

	freq = feparams.frequency/100;

       TsidOnid = CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id);

        CScan::getInstance()->addToScan(TsidOnid, &feparams, 0, true, feindex);

	return 0;
}

/* 0x45 */
void CDescriptors::VBI_data_descriptor(const unsigned char * const)
{
}

/* 0x46 */
void CDescriptors::VBI_teletext_descriptor(const unsigned char * const)
{
}

/* 0x47 */
void CDescriptors::bouquet_name_descriptor(const unsigned char * const)
{
}

uint8_t CDescriptors::fix_service_type(uint8_t type)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	if( (type == 0x9A) || (type == 0x86) || (type==0xc3) || (type==0xc5) || (type==0xc6) )
			return 1;
	return type;
}

/* 0x48 */
void CDescriptors::service_descriptor(const unsigned char * const buffer, const t_service_id service_id, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, bool free_ca, int feindex)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	bool service_wr = false;
	uint8_t service_type = buffer[2];
	CZapitChannel *channel = NULL;
	bool tpchange = false;
	static transponder_id_t last_tpid = 0;

	service_type = fix_service_type(service_type);
	uint8_t real_type = service_type;

	if(service_type == 0x11 || service_type == 0x19)
		service_type = ST_DIGITAL_TELEVISION_SERVICE;

	switch ( _scanType ) 
	{
		case CZapit::ST_TVRADIO:
			if ( (service_type == ST_DIGITAL_TELEVISION_SERVICE ) || (service_type == ST_DIGITAL_RADIO_SOUND_SERVICE) )
				service_wr = true;
			break;
			
		case CZapit::ST_TV:
			if ( service_type == ST_DIGITAL_TELEVISION_SERVICE )
				service_wr = true;
			break;
			
		case CZapit::ST_RADIO:
			if ( service_type == ST_DIGITAL_RADIO_SOUND_SERVICE )
				service_wr = true;
			break;
			
		case CZapit::ST_ALL:
			service_wr = true;
			break;
	}

	if ( !service_wr )
		return;

	uint8_t service_provider_name_length = buffer[3];

	std::string providerName((const char*)&(buffer[4]), service_provider_name_length);
	std::string serviceName;
	std::string satelliteName = "unknown";

	bool in_blacklist = false;

	if (providerName == "CanalSat\xE9lite") 
	{
		providerName = "CanalSat\xC3\xA9lite";
		in_blacklist = true;
	} 
	else if (providerName == "Chambre des D\xE9" "put\xE9" "es") 
	{
		providerName = "Chambre des D\xC3\xA9" "put\xC3\xA9" "es";
		in_blacklist = true;
	} 
	else if (providerName == "PREMIERE") 
	{
		providerName = "Premiere"; // well the name PREMIERE itself is not a problem
		in_blacklist = true;
	} 
	else if( (strncasecmp("POLSAT",providerName.c_str(),6) == 0) || 
			(strncmp("D1",providerName.c_str(),2) == 0)  || (strncmp("OTV", providerName.c_str(), 3) == 0)  ||
			(providerName == "REALITY TV")  || (providerName == "PL Media")  || 
			(providerName == "ANIMAL PL")  || (providerName == "DISCOVERY") )
	{
		providerName = "Polsat"; 
		in_blacklist = true;
	} 
	else if(strncasecmp("TVN ", providerName.c_str(), 4)==0) 
	{
		providerName = "TVN"; 
		in_blacklist = true;
	} 
	else if (providerName == "bt" || providerName == "BT") 
	{
		providerName = "BT Broadcast Services"; 
		in_blacklist = true;
	} 
	else if (strncasecmp("FT GLOBECAST", providerName.c_str(), 12) == 0) 
	{
		providerName = "GlobeCast"; 
		in_blacklist = true;
	} 
	else if(strncasecmp("RR Sat", providerName.c_str(), 6) == 0) 
	{
		providerName = "RRSat"; 
		in_blacklist = true;
	}

	if (in_blacklist) 
	{
		if (((unsigned char)buffer[4 + service_provider_name_length + 1]) >= 0x20) // no encoding info
			serviceName  = CDVBString(("\x05" + std::string((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1))).c_str(), (2 + buffer[1]) - (4 + service_provider_name_length + 1) + 1).getContent(); // add artificial encoding info
		else
			serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	}
	else
	{
		providerName = CDVBString((const char*)&(buffer[4]), service_provider_name_length).getContent();
		serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	}

	found_channels++;
	eventServer->sendEvent ( NeutrinoMessages::EVT_SCAN_NUM_CHANNELS, CEventServer::INITID_NEUTRINO, &found_channels, sizeof(found_channels));

	t_channel_id channel_id = CREATE_CHANNEL_ID;
	tallchans_iterator I = allchans.find(channel_id);

	if (I != allchans.end()) 
	{	
		service_wr = 0;
		I->second.setName(serviceName);
		I->second.setServiceType(real_type);
		I->second.scrambled = free_ca;
		channel = &I->second;
	}

	transponder_id_t tpid = CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id);
	if(service_wr) 
	{
		pair<map<t_channel_id, CZapitChannel>::iterator,bool> ret;
		
		dprintf(DEBUG_DEBUG, "New channel %llx:::%llx %s\n", channel_id, tpid, serviceName.c_str());
		
		if(freq == 11758 || freq == 11778) 
			printf("New channel %llx:::%llx %s\n", channel_id, tpid, serviceName.c_str()); //FIXME debug

		ret = allchans.insert (
				std::pair <t_channel_id, CZapitChannel> (channel_id, CZapitChannel (serviceName, service_id, transport_stream_id, original_network_id, real_type, satellitePosition, freq )));

		ret.first->second.scrambled = free_ca;
		channel = &ret.first->second;
	}

#define UNKNOWN_PROVIDER_NAME "Unknown Provider"

	if(last_tpid != tpid) 
	{
		last_tpid = tpid;
		tpchange = true;
	}

	if (providerName == "") 
	{
		unsigned char buff[1024];
		unsigned short network_descriptors_length = 0;
		unsigned short pos = 0;

		unsigned char filter[DMX_FILTER_SIZE];
		unsigned char mask[DMX_FILTER_SIZE];

		memset(filter, 0x00, DMX_FILTER_SIZE);
		memset(mask, 0x00, DMX_FILTER_SIZE);

		filter[0] = 0x40; // nit tid
		filter[4] = 0x00;
		mask[0] = 0xFF;
		mask[4] = 0xFF;

		if(tpchange) 
		{
			cDemux * dmx = new cDemux(); 
#if defined (PLATFORM_COOLSTREAM)
			dmx->Open(DMX_PSI_CHANNEL);
#else			
			dmx->Open(DMX_PSI_CHANNEL, 1024, CZapit::getInstance()->getFE(feindex));
#endif			
			
			if (!((dmx->sectionFilter(0x10, filter, mask, 5, 10000) < 0) || (dmx->Read(buff, 1024) < 0))) 
			{
				network_descriptors_length = ((buff[8] & 0x0F) << 8) | buff[9];
				for (pos = 10; pos < network_descriptors_length + 10; pos += buff[pos + 1] + 2) 
				{
					switch (buff[pos]) 
					{
						case 0x40:
							if (buff[pos + 1] > 30)
								buff[pos + 1] = 30;

							providerName = CDVBString((const char*)&(buff[pos+2]), buff[pos+1]).getContent();
							break;

						default:
							dprintf(DEBUG_DEBUG, "first_descriptor_tag: %02x\n", buff[pos]);
							break;
					}
				}
			}

			delete dmx;
		} 
		else 
		{
			providerName = lastProviderName;
		}

		if (providerName == "")
			providerName = CDVBString(UNKNOWN_PROVIDER_NAME, strlen(UNKNOWN_PROVIDER_NAME)).getContent();
	}

	//
	lastProviderName = providerName;
	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_PROVIDER, CEventServer::INITID_NEUTRINO, (void *) lastProviderName.c_str(), lastProviderName.length() + 1);

	switch (service_type) 
	{
		case ST_DIGITAL_TELEVISION_SERVICE:
			found_tv_chans++;
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN, CEventServer::INITID_NEUTRINO, &found_tv_chans, sizeof(found_tv_chans));
			break;
			
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
			found_radio_chans++;
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN, CEventServer::INITID_NEUTRINO, &found_radio_chans, sizeof(found_radio_chans));
			break;
			
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
		case ST_DATA_BROADCAST_SERVICE:
		case ST_RCS_MAP:
		case ST_RCS_FLS:
		default:
			found_data_chans++;
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN, CEventServer::INITID_NEUTRINO, &found_data_chans, sizeof(found_data_chans));
			break;
	}

	switch (service_type) 
	{
		case ST_DIGITAL_TELEVISION_SERVICE:
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
			{
				CZapitBouquet * bouquet;
				int bouquetId;
				char pname[100];
				
				if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
					snprintf(pname, 100, "[%c%03d.%d] %s", satellitePosition > 0? 'E' : 'W', abs((int)satellitePosition)/10, abs((int)satellitePosition)%10, providerName.c_str());
				else
					snprintf(pname, 100, "%s", providerName.c_str());

				bouquetId = scanBouquetManager->existsBouquet(pname);

				if (bouquetId == -1)
					bouquet = scanBouquetManager->addBouquet(std::string(pname), false);
				else
					bouquet = scanBouquetManager->Bouquets[bouquetId];

				lastServiceName = serviceName;
				eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SERVICENAME, CEventServer::INITID_NEUTRINO, (void *) lastServiceName.c_str(), lastServiceName.length() + 1);

				CZapitChannel *chan = scanBouquetManager->findChannelByChannelID(channel_id);
				if(chan)
					bouquet->addService(chan);

				break;
			}
		default:
			break;
	}
}

void CDescriptors::current_service_descriptor(const unsigned char * const buffer, const t_service_id service_id, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	bool service_wr = false;
	uint8_t service_type = buffer[2];

	service_type = fix_service_type(service_type);
	uint8_t real_type = service_type;

	if(service_type == 0x11 || service_type == 0x19)
		service_type = ST_DIGITAL_TELEVISION_SERVICE;

	if ( (service_type == ST_DIGITAL_TELEVISION_SERVICE ) || (service_type == ST_DIGITAL_RADIO_SOUND_SERVICE) )
		service_wr = true;

        if ( !service_wr )
		return;

	tallchans_iterator I = curchans.find(CREATE_CHANNEL_ID);

	if (I != curchans.end())
		return;
	
	uint8_t service_provider_name_length = buffer[3];

	std::string providerName((const char*)&(buffer[4]), service_provider_name_length);
	std::string serviceName;
	std::string satelliteName = "unknown";

	bool in_blacklist = false;

	if (providerName == "CanalSat\xE9lite") 
	{
		in_blacklist = true;
	} 
	else if (providerName == "Chambre des D\xE9" "put\xE9" "es") 
	{
		in_blacklist = true;
	} 
	else if (providerName == "PREMIERE") 
	{
		in_blacklist = true;
	}
        else if( (strncasecmp("POLSAT", providerName.c_str(), 6) == 0) || 
        (strncmp("D1", providerName.c_str(), 2) == 0)  || (strncmp("OTV", providerName.c_str(), 3) == 0)  ||
        (providerName == "REALITY TV")  || (providerName == "PL Media")  || 
        (providerName == "ANIMAL PL")  || (providerName == "DISCOVERY") )
        {
                in_blacklist = true;
        } 
	else if(strncasecmp("TVN ", providerName.c_str(), 4) == 0) 
	{
                in_blacklist = true;
        } 
	else if (providerName == "bt" || providerName == "BT") 
	{
                in_blacklist = true;
        } 
	else if (strncasecmp("FT GLOBECAST", providerName.c_str(), 12) == 0) 
	{
                in_blacklist = true;
        } 
	else if(strncasecmp("RR Sat", providerName.c_str(), 6) == 0) 
	{
                in_blacklist = true;
        }

	if (in_blacklist) 
	{
		if (((unsigned char)buffer[4 + service_provider_name_length + 1]) >= 0x20) // no encoding info
			serviceName  = CDVBString(("\x05" + std::string((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1))).c_str(), (2 + buffer[1]) - (4 + service_provider_name_length + 1) + 1).getContent(); // add artificial encoding info
		else
			serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	} 
	else 
	{
		serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	}

	// insert new channel
	curchans.insert (std::pair <t_channel_id, CZapitChannel> (CREATE_CHANNEL_ID, CZapitChannel (serviceName, service_id, transport_stream_id, original_network_id, real_type, satellitePosition, freq )));
}

/* 0x49 */
void CDescriptors::country_availability_descriptor(const unsigned char * const)
{
}

/* 0x4A */
void CDescriptors::linkage_descriptor(const unsigned char * const)
{
}

/* 0x4B */
int CDescriptors::NVOD_reference_descriptor(
	const unsigned char * const buffer,
	const unsigned int num,
	t_transport_stream_id * const tsid,
	t_original_network_id * const onid,
	t_service_id * const sid)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	if ((unsigned int)(buffer[1] / 6) + 1 >= num) {
		*tsid = (buffer[2 + (6 * num)] << 16) | buffer[3 + (6 * num)];
		*onid = (buffer[4 + (6 * num)] << 16) | buffer[5 + (6 * num)];
		*sid =  (buffer[6 + (6 * num)] << 16) | buffer[7 + (6 * num)];
		return 0;
	}

	return -1;
}

/* 0x4C */
void CDescriptors::time_shifted_service_descriptor(const unsigned char * const)
{
}

/* 0x4D */
void CDescriptors::short_event_descriptor(const unsigned char * const)
{
}

/* 0x4E */
void CDescriptors::extended_event_descriptor(const unsigned char * const)
{
}

/* 0x4F */
void CDescriptors::time_shifted_event_descriptor(const unsigned char * const)
{
}

/* 0x50 */
void CDescriptors::component_descriptor(const unsigned char * const)
{
}

/* 0x51 */
void CDescriptors::mosaic_descriptor(const unsigned char * const)
{
}

/* 0x52 */
void CDescriptors::stream_identifier_descriptor(const unsigned char * const)
{
}

/* 0x53 */
void CDescriptors::CA_identifier_descriptor(const unsigned char * const)
{
}

/* 0x54 */
void CDescriptors::content_descriptor(const unsigned char * const)
{
}

/* 0x55 */
void CDescriptors::parental_rating_descriptor(const unsigned char * const)
{
}

/* 0x56 */
void CDescriptors::teletext_descriptor(const unsigned char * const)
{
}

/* 0x57 */
void CDescriptors::telephone_descriptor(const unsigned char * const)
{
}

/* 0x58 */
void CDescriptors::local_time_offset_descriptor(const unsigned char * const)
{
}

/* 0x59 */
void CDescriptors::subtitling_descriptor(const unsigned char * const)
{
}

//test
#include <byteswap.h>

#define UINT32(p)	bswap_32(*(const uint32_t * const)p)
//

/* 0x5A */ //FIXME is brocken :-(
int CDescriptors::terrestrial_delivery_system_descriptor(const unsigned char * const buffer, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, int feindex)
{
	dprintf(DEBUG_NORMAL, "[descriptor] %s:\n", __FUNCTION__);
	
	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type != FE_OFDM)
		return -1;

	FrontendParameters feparams;
	transponder_id_t TsidOnid;
	
	/* inv */
	feparams.inversion = INVERSION_AUTO;
	
	/* freq */
	feparams.frequency = UINT32(&buffer[2]);
	feparams.frequency *= 10;
	
	/* band */
	feparams.u.ofdm.bandwidth = (fe_bandwidth_t)((buffer[6] >> 5) & 0x07);
	
	/* const */
	feparams.u.ofdm.constellation = CFrontend::getModulation((buffer[7] >> 6) & 0x03);
	
	/* hierarchy */
	feparams.u.ofdm.hierarchy_information = (fe_hierarchy_t)((buffer[7] >> 3) & 0x07);
	
	/* HP */
	feparams.u.ofdm.code_rate_HP = CFrontend::getCodeRate(buffer[7] & 0x07);
	
	/* LP */
	feparams.u.ofdm.code_rate_LP = CFrontend::getCodeRate((buffer[8] >> 5) & 0x07);
	
	/* guard */
	feparams.u.ofdm.guard_interval = (fe_guard_interval_t)((buffer[8] >> 3) & 0x03);
	 
	/* trans */
	feparams.u.ofdm.transmission_mode = (fe_transmit_mode_t)((buffer[8] >> 1) & 0x03);

	freq = feparams.frequency/1000000;

	TsidOnid = CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id);

	CScan::getInstance()->addToScan(TsidOnid, &feparams, 0, true, feindex);

	return 0;
}

/* 0x5B */
void CDescriptors::multilingual_network_name_descriptor(const unsigned char * const)
{
}

/* 0x5C */
void CDescriptors::multilingual_bouquet_name_descriptor(const unsigned char * const)
{
}

/* 0x5D */
void CDescriptors::multilingual_service_name_descriptor(const unsigned char * const)
{
}

/* 0x5E */
void CDescriptors::multilingual_component_descriptor(const unsigned char * const)
{
}

/* 0x5F */
void CDescriptors::private_data_specifier_descriptor(const unsigned char * const)
{
}

/* 0x60 */
void CDescriptors::service_move_descriptor(const unsigned char * const)
{
}

/* 0x61 */
void CDescriptors::short_smoothing_buffer_descriptor(const unsigned char * const)
{
}

/* 0x62 */
void CDescriptors::frequency_list_descriptor(const unsigned char * const)
{
}

/* 0x63 */
void CDescriptors::partial_transport_stream_descriptor(const unsigned char * const)
{
}

/* 0x64 */
void CDescriptors::data_broadcast_descriptor(const unsigned char * const)
{
}

/* 0x65 */
void CDescriptors::CA_system_descriptor(const unsigned char * const)
{
}

/* 0x66 */
void CDescriptors::data_broadcast_id_descriptor(const unsigned char * const)
{
}

/* 0x67 */
void CDescriptors::transport_stream_descriptor(const unsigned char * const)
{
}

/* 0x68 */
void CDescriptors::DSNG_descriptor(const unsigned char * const)
{
}

/* 0x69 */
void CDescriptors::PDC_descriptor(const unsigned char * const)
{
}

/* 0x6A */
void CDescriptors::AC3_descriptor(const unsigned char * const)
{
}

/* 0x6B */
void CDescriptors::ancillary_data_descriptor(const unsigned char * const)
{
}

/* 0x6C */
void CDescriptors::cell_list_descriptor(const unsigned char * const)
{
}

/* 0x6D */
void CDescriptors::cell_frequency_link_descriptor(const unsigned char * const)
{
}

/* 0x6E */
void CDescriptors::announcement_support_descriptor(const unsigned char * const)
{
}

/* 0x6F ... 0x7F: reserved */
/* 0x80 ... 0xFE: user private */
/* 0xFF: forbidden */
