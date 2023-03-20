/*
 * $Id: scan.cpp,v 1.14 2013/08/18 11:23:30 mohousch Exp $
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

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <syscall.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <system/debug.h>

#include <eventserver.h>

#include <global.h>
#include <neutrinoMessages.h>

#include <zapit/bouquets.h>
#include <zapit/zapit.h>
#include <zapit/getservices.h>
#include <zapit/nit.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/satconfig.h>
#include <zapit/frontend_c.h>

#include <xmlinterface.h>


extern satellite_map_t satellitePositions;		// defined in getServices.cpp
extern scan_list_t scanProviders;			// defined in zapit.cpp
extern CBouquetManager *g_bouquetManager;
extern transponder_list_t transponders; 		//  defined in zapit.cpp
extern tallchans allchans;  				//  defined in zapit.cpp
extern int found_transponders;
extern int found_channels;
extern std::map <t_channel_id, uint8_t> service_types;
extern uint32_t  found_tv_chans;
extern uint32_t  found_radio_chans;
extern uint32_t  found_data_chans;
extern t_channel_id live_channel_id;
extern CZapit::bouquetMode _bouquetMode;
extern CEventServer *eventServer;
extern CFrontend * live_fe;
extern _xmlDocPtr scanInputParser;

//
int prov_found = 0;
short abort_scan;
short scan_runs;
short curr_sat;
uint32_t processed_transponders;
uint32_t failed_transponders;
int scanMode = 0;
int scan_sat_mode = 0;
CBouquetManager *scanBouquetManager = NULL;
uint32_t fake_tid = 0;
uint32_t fake_nid = 0;
std::map <transponder_id_t, transponder> scantransponders;		// TP list to scan
std::map <transponder_id_t, transponder> scanedtransponders;		// global TP list for current scan
std::map <transponder_id_t, transponder> nittransponders;

#define TIMER_START()			\
        static struct timeval tv, tv2;	\
        static unsigned int msec;	\
        gettimeofday(&tv, NULL)

#define TIMER_STOP(label)			\
        gettimeofday(&tv2, NULL);		\
	msec = tv2.tv_sec - tv.tv_sec;		\
        printf("%s: %d sec\n", label, msec)

bool CScan::tuneFrequency(FrontendParameters *feparams, uint8_t polarization, t_satellite_position satellitePosition, int feindex)
{
	dprintf(DEBUG_NORMAL, "CScan::%s:\n", __FUNCTION__);
	
	// init tuner
	CZapit::getInstance()->initTuner(CZapit::getInstance()->getFE(feindex));
	
	//Set Input
	CZapit::getInstance()->getFE(feindex)->setInput(satellitePosition, feparams->frequency, polarization);

	//Drive Rotor	(SAT)
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
	{
		int ret = CZapit::getInstance()->getFE(feindex)->driveToSatellitePosition(satellitePosition, false); //true);
		
		if(ret > 0) 
		{
			dprintf(DEBUG_INFO, "CScan::tuneFrequency: waiting %d seconds for motor to turn satellite dish.\n", ret);
			
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_PROVIDER, CEventServer::INITID_NEUTRINO, (void *) "moving rotor", 13);
		
			for(int i = 0; i < ret; i++) 
			{
				sleep(1);
					
				if(abort_scan)
					return false;
			}
		}
	}

	return CZapit::getInstance()->getFE(feindex)->tuneFrequency(feparams, polarization, false);
}

int CScan::addToScan(transponder_id_t TsidOnid, FrontendParameters *feparams, uint8_t polarity, bool fromnit, int feindex)
{
	dprintf(DEBUG_NORMAL, "CScan::addToScan: freq %d pol %d tpid %llx from (nit:%d) fe(%d)\n", feparams->frequency, polarity, TsidOnid, fromnit, feindex);

	freq_id_t freq;

	if(CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM)
		freq = feparams->frequency / 100;
	else if(CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
		freq = feparams->frequency / 1000;
	else if(CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM || CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
		freq = feparams->frequency / 1000000;

	uint8_t poltmp1 = polarity & 1;
	uint8_t poltmp2;

	stiterator tI;

	tI = scanedtransponders.find(TsidOnid);

	if (tI != scanedtransponders.end()) 
	{
		poltmp2 = tI->second.polarization & 1;

		if(poltmp2 != poltmp1) 
		{
			t_transport_stream_id transport_stream_id = tI->second.transport_stream_id;
			t_original_network_id original_network_id = tI->second.original_network_id;

			freq_id_t freq1 = GET_FREQ_FROM_TRANSPONDER_ID(tI->first);

			t_satellite_position satellitePosition = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;

			if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
				satellitePosition = -satellitePosition;

			freq++;
			
			TsidOnid = CREATE_TRANSPONDER_ID( freq1, satellitePosition, original_network_id, transport_stream_id);

			dprintf(DEBUG_INFO, "CScan::addToScan: SAME freq %d pol1 %d pol2 %d tpid %llx\n", feparams->frequency, poltmp1, poltmp2, TsidOnid);

			feparams->frequency = feparams->frequency + 1000;
			tI = scanedtransponders.find(TsidOnid);
		}
	}
        else for (tI = scanedtransponders.begin(); tI != scanedtransponders.end(); tI++) 
	{
		poltmp2 = tI->second.polarization & 1;
		
		if((abs(GET_FREQ_FROM_TRANSPONDER_ID(tI->first) - freq) <= 3))
			if(poltmp2 == poltmp1)
                        	break;
        }

	if(tI == scanedtransponders.end()) 
	{
		if(fromnit) 
		{
			if(nittransponders.find(TsidOnid) == nittransponders.end()) 
			{
				nittransponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams, polarity)));
			}
		}
		else 
		{
			found_transponders++;
			scantransponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams, polarity)));

			scanedtransponders.insert (std::pair <transponder_id_t, transponder> ( TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams, polarity)));
		}
		
		return 0;
	}
	else
		tI->second.feparams.u.qpsk.fec_inner = feparams->u.qpsk.fec_inner;

	return 1;
}

int CScan::getSDTS(t_satellite_position satellitePosition, int feindex)
{
	transponder_id_t TsidOnid = 0;
	stiterator tI;
	stiterator stI;
	std::map <transponder_id_t, transponder>::iterator sT;

	dprintf(DEBUG_NORMAL, "CScan::getSDTS: scanning tp from sat/service\n");

_repeat:
	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++) 
	{
		if(abort_scan)
			return 0;

		dprintf(DEBUG_NORMAL, "CScan::getSDTS: scanning: %llx\n", tI->first);

		//
		actual_freq = tI->second.feparams.frequency;

		processed_transponders++;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, CEventServer::INITID_NEUTRINO, &processed_transponders, sizeof(processed_transponders));
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_PROVIDER, CEventServer::INITID_NEUTRINO, (void *) " ", 2);
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SERVICENAME, CEventServer::INITID_NEUTRINO, (void *) " ", 2);
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY,CEventServer::INITID_NEUTRINO, &actual_freq, sizeof(actual_freq));

		// by sat send pol to neutrino
		if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
		{
			actual_polarisation = ((tI->second.feparams.u.qpsk.symbol_rate/1000) << 16) | (tI->second.feparams.u.qpsk.fec_inner << 8) | (uint)tI->second.polarization;

			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP, CEventServer::INITID_NEUTRINO, &actual_polarisation,sizeof(actual_polarisation));
		}
		
		// tune TP
		if (!tuneFrequency(&(tI->second.feparams), tI->second.polarization, satellitePosition, feindex)) 
		{
			failed_transponders++;
			continue;
		}

		if(abort_scan)
			return 0;

		//
		freq_id_t freq;

		if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM)
			freq = tI->second.feparams.frequency/100;
		else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
			freq = tI->second.feparams.frequency/1000;
		else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM || CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
			freq = tI->second.feparams.frequency/1000000;
			
		// parse sdt
		dprintf(DEBUG_NORMAL, "CScan::getSDTS: parsing SDT (tsid:onid %04x:%04x)\n", tI->second.transport_stream_id, tI->second.original_network_id);
		
		if(CSdt::getInstance()->parseSDT(&tI->second.transport_stream_id, &tI->second.original_network_id, satellitePosition, freq, feindex) < 0)
		{
			dprintf(DEBUG_INFO, "CScan::getSDTS: SDT failed !\n");
			continue;
		}

		TsidOnid = CREATE_TRANSPONDER_ID(freq, satellitePosition, tI->second.original_network_id, tI->second.transport_stream_id);

		stI = transponders.find(TsidOnid);
		if(stI == transponders.end())
			transponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder(tI->second.transport_stream_id,tI->second.feparams,tI->second.polarization,tI->second.original_network_id)));
		else
			stI->second.feparams.u.qpsk.fec_inner = tI->second.feparams.u.qpsk.fec_inner;
		
		// parse nit
		if(!scanMode) 
		{
			dprintf(DEBUG_INFO, "CScan::getSDTS: parsing NIT\n");
			
			if( CNit::getInstance()->parseNIT(satellitePosition, freq, feindex) < 0 )
			{
				dprintf(DEBUG_INFO, "CScan::getSDTS: NIT failed !\n");
			}
		}

		dprintf(DEBUG_INFO, "CScan::getSDTS: tpid ready: %llx\n", TsidOnid);
	}

	// add found transponder by nit to scan
	if(!scanMode) 
	{
		dprintf(DEBUG_INFO, "CScan::getSDTS: found %d transponders (%d failed) and %d channels\n", found_transponders, failed_transponders, found_channels);
		
		scantransponders.clear();
		
		for (tI = nittransponders.begin(); tI != nittransponders.end(); tI++) 
		{
			addToScan(tI->first, &tI->second.feparams, tI->second.polarization, false, feindex);
		}

		nittransponders.clear();
		
		dprintf(DEBUG_INFO, "CScan::getSDTS: found %d additional transponders from nit\n", scantransponders.size());
		
		if(scantransponders.size()) 
		{
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_NEUTRINO, &found_transponders, sizeof(found_transponders));

			goto _repeat;
		}
	}

	return 0;
}

int CScan::scanTransponder(_xmlNodePtr transponder, uint8_t diseqc_pos, t_satellite_position satellitePosition, bool /*satfeed*/, int feindex)
{
	dprintf(DEBUG_INFO, "CScan::scanTransponder:\n");
	
	uint8_t polarization = 0;
	uint8_t system = 0, modulation = 1;
	int xml_fec;
	FrontendParameters feparams;
	
	memset(&feparams, 0x00, sizeof(FrontendParameters));

	freq_id_t freq;
	feparams.inversion = INVERSION_AUTO;

	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM)
		feparams.frequency = xmlGetNumericAttribute(transponder, "centre_frequency", 0);
	else
		feparams.frequency = xmlGetNumericAttribute(transponder, "frequency", 0);

	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM)
		freq = feparams.frequency/100;
	else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
		freq = feparams.frequency/1000;
	else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM || CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
		freq = feparams.frequency/1000000;
		
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM)	//DVB-C
	{
		feparams.u.qam.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.u.qam.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		feparams.u.qam.modulation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "modulation", 0);
		diseqc_pos = 0;
	}
	else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM)
	{
		feparams.u.ofdm.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(transponder, "bandwidth", 0);
		feparams.u.ofdm.code_rate_HP = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "code_rate_hp", 0);
		feparams.u.ofdm.code_rate_LP = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "code_rate_lp", 0);
		feparams.u.ofdm.constellation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "constellation", 0);
		feparams.u.ofdm.transmission_mode = (fe_transmit_mode_t) xmlGetNumericAttribute(transponder, "transmission_mode", 0);
		feparams.u.ofdm.guard_interval = (fe_guard_interval_t) xmlGetNumericAttribute(transponder, "guard_interval", 0);
		feparams.u.ofdm.hierarchy_information = (fe_hierarchy_t) xmlGetNumericAttribute(transponder, "hierarchy_information", 0);
		feparams.inversion = (fe_spectral_inversion_t)xmlGetNumericAttribute(transponder, "inversion", 0);
			
		diseqc_pos = 0;
	}
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK) 
	{
		feparams.u.qpsk.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		polarization = xmlGetNumericAttribute(transponder, "polarization", 0);
		system = xmlGetNumericAttribute(transponder, "system", 0);
		modulation = xmlGetNumericAttribute(transponder, "modulation", 0); 
		xml_fec = xmlGetNumericAttribute(transponder, "fec_inner", 0); // S_QPSK + S2_QPSK

		xml_fec = CFrontend::getCodeRate(xml_fec, system);

		if(modulation == 2)		// S2_8PSK
			xml_fec += 9;

		feparams.u.qpsk.fec_inner = (fe_code_rate_t) xml_fec;
	}
	// FIXME: add atsc

	// read network information table
	fake_tid++; fake_nid++;

	addToScan(CREATE_TRANSPONDER_ID(freq, satellitePosition, fake_nid, fake_tid), &feparams, polarization, false, feindex);

	return 0;
}

void CScan::scanProvider(_xmlNodePtr search, t_satellite_position satellitePosition, uint8_t diseqc_pos, bool satfeed, int feindex)
{
	dprintf(DEBUG_NORMAL, "CScan::%s:\n", __FUNCTION__);
	
	_xmlNodePtr tps = NULL;
	found_transponders = 0;
	processed_transponders = 0;

	TIMER_START();
	
	sat_iterator_t sit = satellitePositions.find(satellitePosition);

	if(sit == satellitePositions.end()) 
	{
		dprintf(DEBUG_NORMAL, "[scan] scanProvider: WARNING satellite position %d not found!\n", satellitePosition);
		
		return;
	}

	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, CEventServer::INITID_NEUTRINO, &processed_transponders, sizeof(processed_transponders));
	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SATELLITE, CEventServer::INITID_NEUTRINO, sit->second.name.c_str(), sit->second.name.size() + 1);
	
	tps = search->xmlChildrenNode;

	// read all transponders
	while ((tps = xmlGetNextOccurence(tps, "transponder")) != NULL) 
	{
		if(abort_scan)
			return;

		scanTransponder(tps, diseqc_pos, satellitePosition, satfeed, feindex);

		// next transponder
		tps = tps->xmlNextNode;
	}
	
	eventServer->sendEvent( NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS, CEventServer::INITID_NEUTRINO, &found_transponders, sizeof(found_transponders));

	// start scanning
	getSDTS(satellitePosition, feindex);

	/* 
	 * channels from PAT do not have service_type set.
	 * some channels set the service_type in the BAT or the NIT.
	 * should the NIT be parsed on every transponder? 
	 */
	std::map <t_channel_id, uint8_t>::iterator stI;
	for (stI = service_types.begin(); stI != service_types.end(); stI++) 
	{
		tallchans_iterator scI = allchans.find(stI->first);

		if (scI != allchans.end()) 
		{
			if (scI->second.getServiceType() != stI->second) 
			{
				switch (scI->second.getServiceType()) 
				{
					case ST_DIGITAL_TELEVISION_SERVICE:
					case ST_DIGITAL_RADIO_SOUND_SERVICE:
					case ST_NVOD_REFERENCE_SERVICE:
					case ST_NVOD_TIME_SHIFTED_SERVICE:
						break;
						
					default:
						dprintf(DEBUG_INFO, "CScan::scanProvider: setting service_type of channel_id:%llx %s from %02x to %02x", stI->first, scI->second.getName().c_str(), scI->second.getServiceType(), stI->second);
						
						scI->second.setServiceType(stI->second);
						break;
				}
			}
		}
	}
	
	TIMER_STOP("CScan::scanProvider: scanning took");
}

