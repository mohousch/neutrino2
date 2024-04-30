/*
 * $Id: zapit.cpp 02.03.2024 mohousch Exp $
 *
 * zapit - d-box2 linux project
 *
 * (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
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


// system headers
#include <csignal>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syscall.h>

#include <pthread.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>

#include <dirent.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrinoMessages.h>

#include <configfile.h>

#include <system/debug.h>
#include <system/settings.h>

#include <driver/encoding.h>

#include <zapit/cam.h>
#include <zapit/pat.h>
#include <zapit/pmt.h>
#include <zapit/sdt.h>
#include <zapit/nit.h>
#include <zapit/zapit.h>
#include <zapit/frontend_c.h>
#include <zapit/bouquets.h>

#include <xmlinterface.h>

#include <dmx_cs.h>
#include <audio_cs.h>
#include <video_cs.h>
#include <dvb-ci.h>

#include <playback_cs.h>


//// globals
// ci
cDvbCi * ci = NULL;
// audio conf
std::map<t_channel_id, audio_map_set_t> audio_map;
std::map<t_channel_id, audio_map_set_t>::iterator audio_map_it;
#define VOLUME_DEFAULT_PCM 0
#define VOLUME_DEFAULT_AC3 25
typedef std::pair<int, int> pid_pair_t;
typedef std::pair<t_channel_id, pid_pair_t> volume_pair_t;
typedef std::multimap<t_channel_id, pid_pair_t> volume_map_t;
volume_map_t vol_map;
typedef volume_map_t::iterator volume_map_iterator_t;
typedef std::pair<volume_map_iterator_t, volume_map_iterator_t> volume_map_range_t;
// the configuration file
CConfigFile config(',', true);
CConfigFile fe_configfile(',', false);
//
cDemux *videoDemux = NULL;
cDemux *audioDemux = NULL;
cDemux * pcrDemux = NULL;					// defined in dmx_cs.pp (libdvbapi)
// list of all channels (services)
tallchans allchans;
tallchans curchans;
tallchans nvodchannels;
// pmt update filter
static int pmt_update_fd = -1;
// Audio/Video Decoder
extern cVideo * videoDecoder;
extern cAudio * audioDecoder;
//// channelManager
transponder_list_t transponders;    				// TP map from services.xml
satellite_map_t satellitePositions;				// satellite position as specified in satellites.xml
transponder_list_t select_transponders;				// TP map from sats lists
static int newfound;
////scanManager
scan_list_t scanProviders;
int prov_found = 0;
short abort_scan;
short scan_runs;
short curr_sat;
uint32_t processed_transponders;
uint32_t failed_transponders;
int scanmode = CZapit::SM_NIT;
int scan_sat_mode = 0;
transponder_list_t scantransponders;				// TP list to scan
transponder_list_t scanedtransponders;				// global TP list for current scan
transponder_list_t nittransponders;
BouquetList scanBouquets;
////
extern uint32_t failed_transponders;					// defined in descriptors.cpp
extern uint32_t  found_tv_chans;					// defined in descriptors.cpp
extern uint32_t  found_radio_chans;					// defined in descriptors.cpp
extern uint32_t  found_data_chans;					// defined in descriptors.cpp
extern int prov_found;							// defined in descriptors.cpp
extern int found_transponders;						// defined in descriptors.cpp
extern int found_channels;						// defined in descriptors.cpp
//
extern int current_volume;						// defined in neutrino2.cpp
extern int current_muted;						// defined in neutrino2.cpp
//
extern cDemux * pmtDemux;						// defined in pmt.cpp
// dvbsub
extern int dvbsub_init();
extern int dvbsub_pause();
extern int dvbsub_stop();
extern int dvbsub_getpid();
extern void dvbsub_setpid(int pid);
// tuxtxt
extern void tuxtx_stop_subtitle();
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);
extern void tuxtx_set_pid(int pid, int page, const char * cc);
// scanManager
extern std::map <t_channel_id, uint8_t> service_types;

//// defines
#define TIMER_START()			\
        static struct timeval tv, tv2;	\
        static unsigned int msec;	\
        gettimeofday(&tv, NULL)

#define TIMER_STOP(label)			\
        gettimeofday(&tv2, NULL);		\
	msec = tv2.tv_sec - tv.tv_sec;		\
        printf("%s: %d sec\n", label, msec)
        
#define GET_ATTR(node, name, fmt, arg)                                  \
        do {                                                            \
                char * ptr = xmlGetAttribute(node, name);               \
                if ((ptr == NULL) || (sscanf(ptr, fmt, &arg) <= 0))     \
                        arg = 0;                                        \
        }                                                               \
        while (0)

////
CZapit::CZapit()
{
	// cammanager
	cam0 = NULL;
	cam1 = NULL;
	// femanager
	femap.clear();
	live_fe = NULL;
	record_fe = NULL;
	have_s = false;
	have_c = false;
	have_t = false;
	have_a = false;
	//
	live_channel_id = 0;
	rec_channel_id = 0;
	live_channel = NULL;
	rec_channel = NULL;
	//
	standby = true;
	retune = false;
	//
	currentMode = TV_MODE;
	playbackStopForced = false;
	avDecoderOpen = false;
	//
	sdt_wakeup = false;
	//
	firstzap = true;
	playing = false;
	g_list_changed = false;
	//
	current_is_nvod = false;
	//
	volume_left = 100;
	volume_right = 100;
	audio_mode = 0;
	def_audio_mode = 0;
	//
	volume_percent = 0;
	//
	saveLastChannel = true;
	lastChannelMode = CZapit::TV_MODE;
	lastChannelRadio = 0;
	lastChannelTV = 0;
	makeRemainingChannelsBouquet = false;
	scanSDT = 0;
	lastChannelTV_id = 0;
	lastChannelRadio_id = 0;
	// channelmanager
	tcnt = 0; 
	scnt = 0;
	// scanmanager
	actual_freq = 0; 
	actual_polarisation = 0;
	_bouquetMode = CZapit::BM_UPDATEBOUQUETS;
	_scanType = CZapit::ST_TVRADIO;
	scan_runs = 0;
	found_channels = 0;
	curr_sat = 0;
	// bouquetsmanager
	remainChannels = NULL; 
	Bouquets.clear();
}
		
//
void CZapit::initFrontend()
{
	dprintf(DEBUG_NORMAL, "CZapit::initFrontend\n");
	
	// clear femap
	femap.clear();

	// scan for frontend
	int i, j;
	
	CFrontend * fe;
	int index = -1;
	
	// fill map
	for(i = 0; i < DVBADAPTER_MAX; i++)
	{
		for(j = 0; j < FRONTEND_MAX; j++)
		{
			fe = new CFrontend(j, i);
			
			if(fe->Open()) 
			{
				index++;
				femap.insert(std::pair <unsigned short, CFrontend*> (index, fe));
				
				live_fe = fe;

#if HAVE_DVB_API_VERSION >= 5
				//
				if (fe->getDeliverySystem() & DVB_S || fe->getDeliverySystem() & DVB_S2 || fe->getDeliverySystem() & DVB_S2X)
					have_s = true;
				if (fe->getDeliverySystem() & DVB_C)
					have_c = true;
				if (fe->getDeliverySystem() & DVB_T || fe->getDeliverySystem() & DVB_T2)
					have_t = true;
				if (fe->getDeliverySystem() & DVB_A)
					have_a = true;
#else
				if (fe->info.type == FE_QPSK)
				{
				    have_s = true;
				}
				else if (fe->info.type == FE_QAM)
				{
				    have_c = true;
				}
				else if (fe->info.type == FE_OFDM)
				{
				    have_t = true;
				}
				else if (fe->info.type == FE_ATSC)
				{
				    have_a = true;
				}
#endif
			}
			else
				delete fe;
		}
	}
	
	dprintf(DEBUG_NORMAL, "CZapit::initFrontend: found %d frontends\n", femap.size());
}

void CZapit::OpenFE(void)
{
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++) 
	{
		CFrontend * fe = it->second;
		
		fe->Open();
	}
}

void CZapit::CloseFE(void)
{
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++) 
	{
		CFrontend * fe = it->second;
		
		if(!fe->locked)
			fe->Close();
	}
}

void CZapit::resetFE(void)
{
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++) 
	{
		CFrontend * fe = it->second;
		
		if(!fe->locked)
			fe->reset();
	}
}

CFrontend * CZapit::getFE(int index)
{
	if((unsigned int) index < femap.size())
		return femap[index];
	
	dprintf(DEBUG_INFO, "CZapit::getFE: Frontend #%d not found\n", index);
	
	return NULL;
}

void CZapit::setFEMode(fe_mode_t newmode, CFrontend* fe)
{
	// set mode
	fe->mode = newmode;
	
	// set not connected frontend to standby
	if(fe->mode == (fe_mode_t)FE_NOTCONNECTED )
		fe->Close();

	// set loop frontend as slave
	bool setslave = ( fe->mode == FE_LOOP );
	
	if(setslave)
	{
		dprintf(DEBUG_INFO, "CZapit::setFEMode: Frontend (%d:%d) as slave: %s\n", fe->feadapter, fe->fenumber, setslave ? "yes" : "no");
		fe->setMasterSlave(setslave);
	}
}

void CZapit::initTuner(CFrontend * fe)
{
	if (!fe)
		return;
		
	dprintf(DEBUG_NORMAL, "CZapit::initTuner: fe(%d:%d)\n", fe->feadapter, fe->fenumber);
	
	if(fe->standby)
	{
		dprintf(DEBUG_INFO, "CZapit::initTuner: Frontend (%d,%d)\n", fe->feadapter, fe->fenumber);
		
		// open frontend
		fe->Open();
				
		// set loop frontend as slave 
		bool setslave = ( fe->mode == FE_LOOP );
		
		dprintf(DEBUG_INFO, "CZapit::initTuner: Frontend (%d,%d) as slave: %s\n", fe->feadapter, fe->fenumber, setslave ? "yes" : "no");
					
		if(setslave)
			fe->setMasterSlave(setslave);
		else
			fe->Init();

		// fe functions at start
		fe->setDiseqcRepeats( fe->diseqcRepeats );
		fe->setCurrentSatellitePosition( fe->lastSatellitePosition );
		//fe->setDiseqcType( fe->diseqcType );
	}
}

// compare polarization and band with fe values
bool CZapit::loopCanTune(CFrontend * fe, CZapitChannel * thischannel)
{
	if (!fe)
		return false;
		
	if(fe->getInfo()->type != FE_QPSK)
		return true;

	if(fe->tuned && (fe->getCurrentSatellitePosition() != thischannel->getSatellitePosition()))
		return false;
		
	transponder_list_t::iterator transponder = transponders.find(thischannel->getTransponderId());

	if (transponder == transponders.end())
		return false;

	bool tp_band = ((int)thischannel->getFreqId()*1000 >= fe->lnbSwitch);
	uint8_t tp_pol = transponder->second.feparams.polarization & 1;
	uint8_t fe_pol = fe->getPolarization() & 1;

	dprintf(DEBUG_DEBUG, "CZapit::loopCanTune: fe(%d,%d): locked %d pol:band %d:%d vs %d:%d (%d:%d)\n", fe->feadapter, fe->fenumber, fe->locked, fe_pol, fe->getHighBand(), tp_pol, tp_band, fe->getFrequency(), thischannel->getFreqId()*1000);
		
	if(!fe->tuned || (fe_pol == tp_pol && fe->getHighBand() == tp_band))
		return true;
	
	return false;
}

// 
bool CZapit::CanZap(CZapitChannel * thischannel)
{
	//
	if (IS_WEBTV(thischannel->getChannelID()))
		return true;
	
	//	
	CFrontend * fe = getFreeFrontend(thischannel);
	
	return (fe != NULL);
}

//
bool CZapit::FrontendIsTwin(CFrontend* fe)
{
	bool twin = false;
			
	if( (fe->getInfo()->type == live_fe->getInfo()->type) && (fe->fenumber != live_fe->fenumber) )
		twin = true;
		
	return twin;
}

CFrontend * CZapit::getFrontend(CZapitChannel * thischannel)
{
	const char *FEMODE[] = {
		"SINGLE",
		"LOOP",
		"NOTCONNECTED"
	 };
	 
	//
	CFrontend * free_frontend = NULL;
	
	transponder_list_t::iterator transponder = transponders.find(thischannel->getTransponderId());
	
	// get preferred frontend and initialize it
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_NORMAL, "CZapit::getFrontend: fe(%d:%d): (delsys:0x%x) (%s) tuned:%d (locked:%d) fe_TP: 0x%llx chan_TP: 0x%llx\n",
				fe->feadapter,
				fe->fenumber,
				fe->deliverySystemMask,
				FEMODE[fe->mode],
				fe->tuned,
				fe->locked,
				fe->getTsidOnid(), 
				thischannel->getTransponderId());
				
		// skip not connected frontend
		if( fe->mode == (fe_mode_t)FE_NOTCONNECTED )
			continue;
		
		// same tid
		if(fe->tuned && fe->getTsidOnid() == thischannel->getTransponderId())
		{
			free_frontend = fe;
			break;
		}
		// first zap/record/other frontend type
		else if (transponder != transponders.end())
		{
			////test	
			if ( (fe->getForcedDelSys() & transponder->second.feparams.delsys) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				free_frontend = fe;
				break;
			}
		}
	}
	
	//
	if(free_frontend)
	{
		dprintf(DEBUG_NORMAL, "CZapit::getFrontend: Selected fe(%d:%d) (delsys:0x%x)\n", free_frontend->feadapter, free_frontend->fenumber,free_frontend->deliverySystemMask);
		
		if(free_frontend->standby)
			initTuner(free_frontend);
		
	}
	
	return free_frontend;
}

//
CFrontend * CZapit::getRecordFrontend(CZapitChannel * thischannel)
{
	 const char *FEMODE[] = {
		"SINGLE",
		"LOOP",
		"NOTCONNECTED"
	 };
	 
	// check for frontend
	CFrontend * rec_frontend = NULL;
	
	transponder_list_t::iterator transponder = transponders.find(thischannel->getTransponderId());
	
	// get record frontend
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_INFO, "CZapit::getRecordFrontend: fe(%d,%d): (%s) tuned:%d (locked:%d) fe_freq: %d fe_TP: 0x%llx chan_TP: 0x%llx\n",
				fe->feadapter,
				fe->fenumber,
				FEMODE[fe->mode],
				fe->tuned,
				fe->locked,
				fe->getFrequency(), 
				fe->getTsidOnid(),  
				thischannel->getTransponderId() 
				);
				
		// skip not connected frontend
		if( fe->mode == (fe_mode_t)FE_NOTCONNECTED )
			continue;
		
		// frontend on same tid
		if( (fe->tuned) && (fe->getTsidOnid() == thischannel->getTransponderId()) )
		{
			rec_frontend = fe;
			break;
		}
		// other free tuner
		else if (transponder != transponders.end())
		{
			if ( (fe->getForcedDelSys() & transponder->second.feparams.delsys) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				rec_frontend = fe;
				break;
			}
		}
	}
	
	//
	if(rec_frontend)
	{
		dprintf(DEBUG_NORMAL, "CZapit::getRecordFrontend: Selected fe(%d,%d)\n", rec_frontend->feadapter, rec_frontend->fenumber);
		
		if(rec_frontend->standby)
			initTuner(rec_frontend);
		
	}
	
	return rec_frontend;
}

//
CFrontend * CZapit::getFreeFrontend(CZapitChannel * thischannel)
{
	const char *FEMODE[] = {
		"SINGLE",
		"LOOP",
		"NOTCONNECTED"
	 };
	 
	// check for frontend
	CFrontend * pref_frontend = NULL;
	
	transponder_list_t::iterator transponder = transponders.find(thischannel->getTransponderId());
	
	// get preferred frontend
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_NORMAL, "CZapit::getFreeFrontend: fe(%d,%d): (%s) tuned:%d (locked:%d) fe_freq: %d fe_TP: 0x%llx chan_TP: 0x%llx\n",
				fe->feadapter,
				fe->fenumber,
				FEMODE[fe->mode],
				fe->tuned,
				fe->locked,
				fe->getFrequency(), 
				fe->getTsidOnid(),  
				thischannel->getTransponderId()
				);
				
		// skip not connected frontend
		if( fe->mode == (fe_mode_t)FE_NOTCONNECTED )
			continue;

		// same tid frontend (locked)
		if(fe->locked && fe->getTsidOnid() == thischannel->getTransponderId())
		{
			pref_frontend = fe;
			break;
		}
		// first zap/record/other frontend type
		else if (transponder != transponders.end())
		{
			if ( (fe->getForcedDelSys() & transponder->second.feparams.delsys) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				pref_frontend = fe;
				break;
			}
		}
	}
	
	if (pref_frontend)
		dprintf(DEBUG_NORMAL, "CZapit::getFreeFrontend: Selected fe(%d,%d)\n", pref_frontend->feadapter, pref_frontend->fenumber);
	
	return pref_frontend;
}

void CZapit::lockFrontend(CFrontend *fe)
{
	if(fe)
	{
		if(fe->tuned)
		      fe->locked = true;
	}
}

void CZapit::unlockFrontend(CFrontend *fe)
{
	if(fe)
	{
		if(fe->locked)
		      fe->locked = false;
	}
}

//
uint32_t getConfigValue(CFrontend* fe, const char * name, uint32_t defval)
{
	char cfg_key[81];
	sprintf(cfg_key, "fe%d%d_%s", fe->feadapter, fe->fenumber, name);
	
	return fe_configfile.getInt32(cfg_key, defval);
}

//
void setConfigValue(CFrontend* fe, const char * name, uint32_t val)
{
	char cfg_key[81];
	
	sprintf(cfg_key, "fe%d%d_%s", fe->feadapter, fe->fenumber, name);
	fe_configfile.setInt32(cfg_key, val);
}

// save frontend config
void CZapit::saveFrontendConfig()
{
	dprintf(DEBUG_INFO, "CZapit::saveFrontendConfig\n");
	
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++)
	{
		CFrontend * fe = it->second;
		
		// mode
		setConfigValue(fe, "mode", fe->mode);
		
		// mode
		setConfigValue(fe, "powered", fe->powered);
		
		// delsys
		setConfigValue(fe, "delsys", fe->forcedDelSys);
			
		// sat
		if(fe->getInfo()->type == FE_QPSK)
		{
			setConfigValue(fe, "lastSatellitePosition", fe->getCurrentSatellitePosition());
			setConfigValue(fe, "diseqcRepeats", fe->getDiseqcRepeats());
			setConfigValue(fe, "diseqcType", fe->getDiseqcType() );

			// unicable
			setConfigValue(fe, "uni_scr", fe->getUniScr() );
			setConfigValue(fe, "uni_qrg", fe->getUniQrg() );
					
			char tempd[12];
			char cfg_key[81];
				
			sprintf(tempd, "%3.6f", fe->gotoXXLatitude);
			sprintf(cfg_key, "fe%d%d_gotoXXLatitude", fe->feadapter, fe->fenumber);
			fe_configfile.setString(cfg_key, tempd );
				
			sprintf(tempd, "%3.6f", fe->gotoXXLongitude);
			sprintf(cfg_key, "fe%d%d_gotoXXLongitude", fe->feadapter, fe->fenumber);
			fe_configfile.setString(cfg_key, tempd );
				
			setConfigValue(fe, "gotoXXLaDirection", fe->gotoXXLaDirection);
			setConfigValue(fe, "gotoXXLoDirection", fe->gotoXXLoDirection);
			setConfigValue(fe, "useGotoXX", fe->useGotoXX);
			setConfigValue(fe, "repeatUsals", fe->repeatUsals);
		}
	}
	
	fe_configfile.saveConfig(FRONTEND_CONFIGFILE);
}

void CZapit::loadFrontendConfig()
{
	dprintf(DEBUG_NORMAL, "CZapit::loadFrontendConfig\n");
	
	if (!fe_configfile.loadConfig(FRONTEND_CONFIGFILE))
		printf("%s not found\n", FRONTEND_CONFIGFILE);
	
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		// mode
		fe->mode = (fe_mode_t)getConfigValue(fe, "mode", (fe_mode_t)FE_SINGLE);
		
		// powered
		fe->powered = getConfigValue(fe, "powered", 0);
		
		// delsys
		fe->forcedDelSys = getConfigValue(fe, "delsys", fe->getDeliverySystem());
		
		// sat
		if(fe->getInfo()->type == FE_QPSK)
		{
			fe->useGotoXX = getConfigValue(fe, "useGotoXX", 0);
			
			char cfg_key[81];
			
			sprintf(cfg_key, "fe%d%d_gotoXXLatitude", fe->feadapter, fe->fenumber);
			fe->gotoXXLatitude = strtod(config.getString(cfg_key, "0.0").c_str(), NULL);
			
			sprintf(cfg_key, "fe%d%d_gotoXXLongitude", fe->feadapter, fe->fenumber);
			fe->gotoXXLongitude = strtod(config.getString(cfg_key, "0.0").c_str(), NULL);
			
			fe->gotoXXLaDirection = getConfigValue(fe, "gotoXXLaDirection", 0);
			fe->gotoXXLoDirection = getConfigValue(fe, "gotoXXLoDirection", 0);
			
			fe->repeatUsals = getConfigValue(fe, "repeatUsals", 0);
			fe->diseqcType = (diseqc_t)getConfigValue(fe, "diseqcType", (diseqc_t)NO_DISEQC);
			fe->diseqcRepeats = getConfigValue(fe, "diseqcRepeats", 0);

			// unicable
			fe->uni_scr = getConfigValue(fe, "uni_scr", -1);
			fe->uni_qrg = getConfigValue(fe, "uni_qrg", 0);

			fe->motorRotationSpeed = getConfigValue(fe, "motorRotationSpeed", 18); // default: 1.8 degrees per second
			
			fe->lastSatellitePosition = getConfigValue(fe, "lastSatellitePosition", 0);
		}
		
		////
		// set loop frontend as slave 
		bool setslave = ( fe->mode == FE_LOOP );
		
		dprintf(DEBUG_INFO, "CZapit::loadFrontendConfig: Frontend (%d,%d) as slave: %s\n", fe->feadapter, fe->fenumber, setslave ? "yes" : "no");
					
		if(setslave)
			fe->setMasterSlave(setslave);
		else
			fe->Init();

		// fe functions at start
		fe->setDiseqcRepeats( fe->diseqcRepeats );
		fe->setCurrentSatellitePosition( fe->lastSatellitePosition );
		//fe->setDiseqcType( fe->diseqcType );
	}
}

void CZapit::loadAudioMap()
{
	dprintf(DEBUG_NORMAL, "CZapit::loadAudioMap\n");
	
        FILE *audio_config_file = fopen(AUDIO_CONFIG_FILE, "r");
	audio_map.clear();
	
        if (audio_config_file) 
	{
          	t_channel_id chan;
          	int apid = 0;
          	int subpid = 0;
		int ttxpid = 0, ttxpage = 0;
          	int mode = 0;
		int volume = 0;
          	char s[1000];

          	while (fgets(s, 1000, audio_config_file)) 
		{
			sscanf(s, "%llx %d %d %d %d %d %d", &chan, &apid, &mode, &volume, &subpid, &ttxpid, &ttxpage);
			
            		audio_map[chan].apid = apid;
            		audio_map[chan].subpid = subpid;
            		audio_map[chan].mode = mode;
            		audio_map[chan].volume = volume;
			audio_map[chan].ttxpid = ttxpid;
			audio_map[chan].ttxpage = ttxpage;
          	}

          	fclose(audio_config_file);
        }
	else
		perror(AUDIO_CONFIG_FILE);
}

void CZapit::saveAudioMap()
{
	dprintf(DEBUG_INFO, "CZapit::saveAudioMap\n");
	
	FILE *audio_config_file = fopen(AUDIO_CONFIG_FILE, "w");
        if (audio_config_file) 
	{	
		fprintf(audio_config_file, "# chan_id a_pid a_mode a_volume a_subpid a_txtpid a_txtpage\n");
			
		for (audio_map_it = audio_map.begin(); audio_map_it != audio_map.end(); audio_map_it++) 
		{
			fprintf(audio_config_file, "%llx %d %d %d %d %d %d\n", (uint64_t) audio_map_it->first,
                        (int) audio_map_it->second.apid, (int) audio_map_it->second.mode, (int) audio_map_it->second.volume, 
			(int) audio_map_it->second.subpid, (int) audio_map_it->second.ttxpid, (int) audio_map_it->second.ttxpage);
		}
			
		fdatasync(fileno(audio_config_file));
                fclose(audio_config_file);
        }
	else
		perror(AUDIO_CONFIG_FILE);
}

void CZapit::loadVolumeMap()
{
	dprintf(DEBUG_NORMAL, "CZapit::loadVolumeMap\n");
	
	vol_map.clear();
	FILE * volume_config_file = fopen(VOLUME_CONFIG_FILE, "r");
	if (!volume_config_file) 
	{
		perror(VOLUME_CONFIG_FILE);
		return;
	}
	t_channel_id chan;
	int apid = 0;
	int volume = 0;
	char s[1000];
	
	while (fgets(s, 1000, volume_config_file)) 
	{
		if (sscanf(s, "%llx %d %d", &chan, &apid, &volume) == 3)
			vol_map.insert(volume_pair_t(chan, pid_pair_t(apid, volume)));
	}
	fclose(volume_config_file);
}

void CZapit::saveVolumeMap()
{
	dprintf(DEBUG_NORMAL, "CZapit::saveVolumeMap\n");
	
	FILE * volume_config_file = fopen(VOLUME_CONFIG_FILE, "w");
	if (!volume_config_file) 
	{
		perror(VOLUME_CONFIG_FILE);
		return;
	}
	for (volume_map_iterator_t it = vol_map.begin(); it != vol_map.end(); ++it)
		fprintf(volume_config_file, "%llx %d %d\n", (uint64_t) it->first, it->second.first, it->second.second);

	fdatasync(fileno(volume_config_file));
	fclose(volume_config_file);
}

void CZapit::saveZapitSettings(bool write, bool write_a)
{
	dprintf(DEBUG_NORMAL, "CZapit::saveZapitSettings\n");
	
	// lastchannel
	if (live_channel) 
	{
		int c = 0;

		c = live_channel->number;

		if (c >= 0) 
		{
			if ((currentMode & RADIO_MODE))
				lastChannelRadio = c - 1;
			else if ((currentMode & TV_MODE))
				lastChannelTV = c - 1;
		}
	}

	// write zapit config
	if (write) 
	{
		config.setBool("saveLastChannel", saveLastChannel);
		
		//if (config.getBool("saveLastChannel", true)) 
		{
			if (currentMode & RADIO_MODE)
			{
				config.setInt32("lastChannelMode", RADIO_MODE);
			}
			else if (currentMode & TV_MODE)
			{
				config.setInt32("lastChannelMode", TV_MODE);
			}

			config.setInt32("lastChannelRadio", lastChannelRadio);
			config.setInt32("lastChannelTV", lastChannelTV);
			config.setInt64("lastChannel", live_channel_id);
		}
		
		config.setInt64("lastChannelTV_id", lastChannelTV_id);
		config.setInt64("lastChannelRadio_id", lastChannelRadio_id);
		config.setBool("makeRemainingChannelsBouquet", makeRemainingChannelsBouquet);
		config.setInt32("scanSDT", scanSDT);

		//
		//if (config.getModifiedFlag())
		config.saveConfig(ZAPIT_CONFIGFILE);

	}

	// write audio config
        if (write_a) 
	{
                // audio map
		saveAudioMap();
                
                // volume map
		saveVolumeMap();
        }
}

void CZapit::loadZapitSettings()
{
	dprintf(DEBUG_NORMAL, "CZapit::loadZapitSettings\n");
	
	if (!config.loadConfig(ZAPIT_CONFIGFILE))
		printf("%s not found\n", ZAPIT_CONFIGFILE);

	saveLastChannel = config.getBool("saveLastChannel", true);
	lastChannelMode = config.getInt32("lastChannelMode", TV_MODE);
	lastChannelRadio = config.getInt32("lastChannelRadio", 0);
	lastChannelRadio_id = config.getInt64("lastChannelRadio_id", 0);
	lastChannelTV = config.getInt32("lastChannelTV", 0);
	lastChannelTV_id = config.getInt64("lastChannelTV_id", 0);
	live_channel_id = config.getInt64("lastChannel", 0);
	makeRemainingChannelsBouquet = config.getBool("makeRemainingChannelsBouquet", false);
	scanSDT = config.getInt32("scanSDT", 0);

	//load audio map
	loadAudioMap();
	
	// load volume map
	loadVolumeMap();
}
 
void CZapit::sendCaPmtPlayBackStart(CZapitChannel * thischannel, CFrontend * fe)
{
	dprintf(DEBUG_NORMAL, "CZapit::sendCaPmtPlayBackStart\n");
	
	if(!thischannel)
		return;
	
	if(!fe)
		return;
	
	int demux_index = -1;
	int ca_mask = 0;
	
	// cam
	demux_index = fe->fenumber;
	
	ca_mask |= 1 << demux_index;

	if(currentMode & RECORD_MODE) 
	{
		if(rec_channel_id != live_channel_id) 
		{
			// zap from rec. channel
			cam1->setCaPmt(thischannel, thischannel->getCaPmt(), demux_index, ca_mask); //start cam1
                } 
                else 
		{
			// zap back to rec. channel
			cam0->setCaPmt(thischannel, thischannel->getCaPmt(), demux_index, ca_mask, true); // update
			cam1->sendMessage(0, 0); // stop/close
		}
	} 
	else 
	{
		cam0->setCaPmt(thischannel, thischannel->getCaPmt(), demux_index, ca_mask); //start cam0
	}
	
	// cam
	ci->SendCaPMT(thischannel->getCaPmt(), fe? fe->fenumber : 0);	
}

void CZapit::sendcapmtPlayBackStop(bool _sendPmt)
{
	dprintf(DEBUG_NORMAL, "CZapit::sendcapmtPlayBackStop: (sendPmt:%d)\n", _sendPmt);
	
	if(_sendPmt) 
	{
		int demux_index = -1;
		int ca_mask = 0;
	
		if(currentMode & RECORD_MODE) 
		{
			if(record_fe != NULL)
				demux_index = record_fe->fenumber;

			ca_mask |= 1 << demux_index;

			if(live_channel_id == rec_channel_id)
				cam0->setCaPmt(rec_channel, rec_channel->getCaPmt(), demux_index, ca_mask, true); //update cam0
			else
				cam1->sendMessage(0,0); // stop cam1
		} 
		else 
		{
			cam0->sendMessage(0,0); // stop cam0
		}
	}
}

void CZapit::sendCaPmtRecordStop(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::sendCaPmtRecordStop\n");
	
	int demux_index = -1;
	int ca_mask = 0;
	
	// cam1 stop
	cam1->sendMessage(0, 0); // cam1 stop
	
	// cam0 update	
	if(live_fe != NULL)
		demux_index = live_fe->fenumber;

	ca_mask |= 1 << demux_index;
	
	if(standby)
		cam0->sendMessage(0, 0); // cam0 stop
	else if(live_channel_id == rec_channel_id) 
	{
		if(live_channel != NULL)
		{
			cam0->setCaPmt(live_channel, live_channel->getCaPmt(), demux_index, ca_mask, true); // cam0 update
			ci->SendCaPMT(NULL, live_fe? live_fe->fenumber : 0); // stop
		}
	} 
	else 
	{
		if(live_channel != NULL)
			cam0->setCaPmt(live_channel, live_channel->getCaPmt(), demux_index, ca_mask); //cam0 start
			
		if(rec_channel != NULL)
		{
			ci->SendCaPMT(NULL, record_fe? record_fe->fenumber : 0);
		}
	}
	
	// ci cam
	if(live_channel != NULL)
	{
		ci->SendCaPMT(live_channel->getCaPmt(), live_fe? live_fe->fenumber : 0); // restart
	}
}

// save pids
void CZapit::saveChannelPids(CZapitChannel * thischannel)
{
	if(thischannel == NULL)
		return;

	dprintf(DEBUG_INFO, "CZapit::saveChannelPids: (0x%llx), apid %x mode %d volume %d\n", thischannel->getChannelID(), thischannel->getAudioPid(), audio_mode, volume_right);
	
	audio_map[thischannel->getChannelID()].apid = thischannel->getAudioPid();
	audio_map[thischannel->getChannelID()].mode = audio_mode;
	audio_map[thischannel->getChannelID()].volume = audioDecoder->getVolume();
	audio_map[thischannel->getChannelID()].subpid = dvbsub_getpid();
	tuxtx_subtitle_running(&audio_map[thischannel->getChannelID()].ttxpid, &audio_map[thischannel->getChannelID()].ttxpage, NULL);
	
	// save pid volume
	setPidVolume(thischannel->getChannelID(), thischannel->getAudioPid(), volume_percent);
}

CZapitChannel * CZapit::findChannelToZap(const t_channel_id channel_id, bool in_nvod)
{
	tallchans_iterator cit;
	
	if (in_nvod)
	{
		current_is_nvod = true;

		cit = nvodchannels.find(channel_id);

		if (cit == nvodchannels.end()) 
		{
			dprintf(DEBUG_INFO, "CZapit::findChannelToZap: channel_id (0x%llx) AS NVOD not found\n", channel_id);
			return NULL;
		}
	} 
	else 
	{
		current_is_nvod = false;

		cit = allchans.find(channel_id);

		if (cit == allchans.end()) 
		{
			dprintf(DEBUG_INFO, "CZapit::findChannelToZap: channel_id (0x%llx) not found\n", channel_id);
			return NULL;
		}
	}
	
	return &cit->second;
}

bool CZapit::tuneToChannel(CFrontend * frontend, CZapitChannel * thischannel, bool &transponder_change)
{
	dprintf(DEBUG_NORMAL, "CZapit::tuneToChannel: %llx\n", thischannel->getChannelID());
	
	int waitForMotor = 0;

	transponder_change = false;
		  
	transponder_change = frontend->setInput(thischannel, current_is_nvod);
	
	if(retune)
		transponder_change = true;
	
	// drive rotor
	if(transponder_change && !current_is_nvod) 
	{
		waitForMotor = frontend->driveToSatellitePosition(thischannel->getSatellitePosition());
			
		if(waitForMotor > 0) 
		{
			dprintf(DEBUG_INFO, "CZapit::tuneToChannel: waiting %d seconds for motor to turn satellite dish.\n", waitForMotor);
			eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_MOTOR, &waitForMotor, sizeof(waitForMotor));
				
			for(int i = 0; i < waitForMotor; i++) 
			{
				sleep(1);
			}
		}
	}

	// tune fe (by TP change, nvod, twin_mode)
	if (transponder_change || current_is_nvod ) 
	{
		//
		if(retune)
			retune = false;
		
		if ( frontend->tuneChannel(thischannel, current_is_nvod) == false) 
		{
			return false;
		}
	}

	return true;
}

bool CZapit::parseChannelPatPmt(CZapitChannel * thischannel, CFrontend * fe)
{
	dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: looking up pids for channel_id (0x%llx)\n", thischannel->getChannelID());
	
	bool failed = false;
	CPat pat;
	CPmt pmt;
	
	// get program map table pid from program association table
	if (thischannel->getPmtPid() == 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: no pmt pid, going to parse pat\n");	
		
		if (pat.parse(thischannel, fe) < 0)
		{
			dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pat parsing failed\n");
			
			failed = true;
		}
	}

	// parse program map table and store pids
	if ( !failed && pmt.parse(thischannel, fe) < 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pmt parsing failed\n");	
		
		if (pat.parse(thischannel, fe) < 0) 
		{
			dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pat parsing failed\n");
			
			failed = true;
		}
		else if (pmt.parse(thischannel, fe) < 0) 
		{
			dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pmt parsing failed\n");
			
			failed = true;
		}
	}
	
	return !failed;
}

void CZapit::restoreChannelPids(CZapitChannel * thischannel)
{
	audio_map_it = audio_map.find(thischannel->getChannelID());
	
	if((audio_map_it != audio_map.end()) ) 
	{
		dprintf(DEBUG_INFO, "CZapit::restoreChannelPids: channel found, audio pid %x, subtitle pid %x mode %d volume %d\n", audio_map_it->second.apid, audio_map_it->second.subpid, audio_map_it->second.mode, audio_map_it->second.volume);
				
		if(thischannel->getAudioChannelCount() > 1) 
		{
			for (int i = 0; i < thischannel->getAudioChannelCount(); i++) 
			{
				if (thischannel->getAudioChannel(i)->pid == audio_map_it->second.apid ) 
				{
					dprintf(DEBUG_INFO, "CZapit::restoreChannelPids: Setting audio!\n");
					thischannel->setAudioChannel(i);
				}
			}
		}

		volume_left = volume_right = audio_map_it->second.volume;
		audio_mode = audio_map_it->second.mode;
		//FIXME: what shall neutrino do with saved volume (volume_left/volume_right)

		// set dvbsub pid
		dvbsub_setpid(audio_map_it->second.subpid);

		// set txtsub pid
		std::string tmplang;
		for (int i = 0 ; i < (int)thischannel->getSubtitleCount() ; ++i) 
		{
			CZapitAbsSub* s = thischannel->getChannelSub(i);
			
			if(s->pId == audio_map_it->second.ttxpid) 
			{
				tmplang = s->ISO639_language_code;
				break;
			}
		}
		
		if(tmplang.empty())
			tuxtx_set_pid(audio_map_it->second.ttxpid, audio_map_it->second.ttxpage, (char *) thischannel->getTeletextLang());
		else
			tuxtx_set_pid(audio_map_it->second.ttxpid, audio_map_it->second.ttxpage, (char *) tmplang.c_str());
	} 
	else 
	{
		if(thischannel->getAudioChannel() != NULL)
			volume_left = volume_right = (thischannel->getAudioChannel()->audioChannelType == CZapitAudioChannel::AC3)? VOLUME_DEFAULT_AC3 : VOLUME_DEFAULT_PCM;
		audio_mode = def_audio_mode;
		
		// set default tuxtxt pid
		tuxtx_set_pid(0, 0, (char *) thischannel->getTeletextLang());
	}

	// set saved volume pro pid
	if(thischannel->getAudioChannel() != NULL)
		volume_percent = getPidVolume(thischannel->getChannelID(), thischannel->getAudioPid(), thischannel->getAudioChannel()->audioChannelType == CZapitAudioChannel::AC3);
	setVolumePercent(volume_percent);
	
	//FIXME: is is muted
	if(current_muted)
		audioDecoder->SetMute(true);
}

// 0 = success
// -1 = fail
int CZapit::zapit(const t_channel_id channel_id, bool in_nvod, bool forupdate)
{
	bool transponder_change = false;
	tallchans_iterator cit;
	bool failed = false;
	CZapitChannel * newchannel;
	CPmt pmt;

	dprintf(DEBUG_NORMAL, ANSI_BLUE"CZapit::zapit: channel id 0x%llx nvod %d\n", channel_id, in_nvod);

	// find channel to zap
	if( (newchannel = findChannelToZap(channel_id, in_nvod)) == NULL ) 
	{
		dprintf(DEBUG_INFO, "CZapit::zapit: channel_id: 0x%llx not found\n", channel_id);
		return -1;
	}
	
	//
	if (!IS_WEBTV(channel_id))
	{
		// save pids
		if (!firstzap && live_channel)
			saveChannelPids(live_channel);

		// firstzap right now does nothing but control saving the audio channel
		firstzap = false;

		// stop update pmt filter
		pmt.pmt_stop_update_filter(&pmt_update_fd);
	}
	
	//
	stopPlayBack(!forupdate);

	//
	if (!IS_WEBTV(channel_id))
	{
		// reset channel pids
		if(!forupdate && live_channel)
			live_channel->resetPids();
	}

	live_channel = newchannel;

	live_channel_id = live_channel->getChannelID();

	// save channel
	saveZapitSettings(false, false);
	
	//
	if  (!IS_WEBTV(channel_id))
	{
		// find live_fe to tune
		CFrontend * fe = getFrontend(live_channel);
		if(fe == NULL) 
		{
			dprintf(DEBUG_INFO, "CZapit::zapit: can not allocate live frontend\n");
			return -1;
		}
	
		live_fe = fe;
	
		dprintf(DEBUG_NORMAL, ANSI_BLUE"CZapit::zapit: zap to %s(0x%llx) fe(%d:%d)\n", live_channel->getName().c_str(), live_channel_id, live_fe->feadapter, live_fe->fenumber );

		// tune live frontend
		if(!tuneToChannel(live_fe, live_channel, transponder_change))
			return -1;

		// check if nvod
		if (live_channel->getServiceType() == ST_NVOD_REFERENCE_SERVICE) 
		{
			current_is_nvod = true;
			return 0;
		}
		
		int retry = false;
	
tune_again:
		// parse pat pmt
		failed = !parseChannelPatPmt(live_channel, live_fe);

		if(failed && !retry)
		{
			usleep(2500);  /* give some 2500us for demuxer: borrowed from e2*/
			retry = true;
			dprintf(DEBUG_NORMAL, "CZapit::zapit: trying again\n");
			goto tune_again;
		}	

		if ((!failed) && (live_channel->getAudioPid() == 0) && (live_channel->getVideoPid() == 0)) 
		{
			dprintf(DEBUG_NORMAL, "CZapit::zapit: neither audio nor video pid found\n");
			failed = true;
		}

	
		if (transponder_change)
			sdt_wakeup = true;

		if (failed)
			return -1;

		live_channel->getCaPmt()->ca_pmt_list_management = transponder_change ? 0x03 : 0x04;

		// restore channel pids
		restoreChannelPids(live_channel);
	}

	// start playback (live)
	int res = startPlayBack(live_channel);

	// ci / cam
	if  (IS_WEBTV(channel_id))
		return res;
	else
	{
		// ci / cam
		sendCaPmtPlayBackStart(live_channel, live_fe);

		// send caid
		int caid = 1;

		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_CA_ID, &caid, sizeof(int));	

		// start pmt update filter
		pmt.pmt_set_update_filter(live_channel, &pmt_update_fd, live_fe);
	}	

	return 0;
}

// 0 = success
// -1 = fail
int CZapit::zapToRecordID(const t_channel_id channel_id)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::zapToRecordID: channel_id (0x%llx)\n", channel_id);
	
	if (IS_WEBTV(channel_id))
	{
		rec_channel_id = channel_id;
		
		return 0;
	}
	
	bool transponder_change = false;
	
	// find channel
	if((rec_channel = findChannelToZap(channel_id, false)) == NULL) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToRecordID: channel_id (0x%llx) not found\n", channel_id);
		return -1;
	}
	
	rec_channel_id = channel_id;
	
	// find record frontend
	CFrontend * frontend = getRecordFrontend(rec_channel);
	if(frontend == NULL) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToRecordID: can not allocate record frontend\n");
		return -1;
	}
	
	if (record_fe == NULL)	
		record_fe = frontend;
	
	//
	if(record_fe == live_fe)
	{
		if( (rec_channel_id != live_channel_id) && !SAME_TRANSPONDER(live_channel_id, rec_channel_id) )
		{
			// zap to record channel
			zapToChannelID(rec_channel_id, false);
			return 0;
		}
	}
	else 
	{
		// just tune
		if(!tuneToChannel(record_fe, rec_channel, transponder_change))
			return -1;
	}
	
	// parse channel pat_pmt
	if(!parseChannelPatPmt(rec_channel, record_fe))
		return -1;
						
	// ci / cam
	sendCaPmtPlayBackStart(rec_channel, record_fe);		
	
	dprintf(DEBUG_NORMAL, "CZapit::zapToRecordID: zapped to %s (0x%llx) fe(%d,%d)\n", rec_channel->getName().c_str(), rec_channel_id, record_fe->feadapter, record_fe->fenumber);
	
	return 0;
}

// set channel/pid volume percent, using current channel_id and pid
void CZapit::setPidVolume(t_channel_id channel_id, int pid, int percent)
{
	if (!channel_id)
		channel_id = live_channel_id;

	if (!pid && (channel_id == live_channel_id) && live_channel)
		pid = live_channel->getAudioPid();

	dprintf(DEBUG_INFO, "CZapit::setPidVolume: channel 0x%llx pid %x percent %d\n", channel_id, pid, percent);
	
	volume_map_range_t pids = vol_map.equal_range(channel_id);
	for (volume_map_iterator_t it = pids.first; it != pids.second; ++it) 
	{
		if (it->second.first == pid) 
		{
			it->second.second = percent;
			return;
		}
	}
	
	vol_map.insert(volume_pair_t(channel_id, pid_pair_t(pid, percent)));
}

// return channel/pid volume percent, using current channel_id and pid
int CZapit::getPidVolume(t_channel_id channel_id, int pid, bool ac3)
{
	int percent = -1;

	if (!channel_id)
		channel_id = live_channel_id;

	if (!pid && (channel_id == live_channel_id) && live_channel)
		pid = live_channel->getAudioPid();

	volume_map_range_t pids = vol_map.equal_range(channel_id);
	for (volume_map_iterator_t it = pids.first; it != pids.second; ++it) 
	{
		if (it->second.first == pid) 
		{
			percent = it->second.second;
			break;
		}
	}
	
	if (percent < 0) 
	{
		percent = ac3 ? VOLUME_DEFAULT_AC3 : VOLUME_DEFAULT_PCM;
		if ((channel_id == live_channel_id) && live_channel) 
		{
			for (int  i = 0; i < live_channel->getAudioChannelCount(); i++) 
			{
				if (pid == live_channel->getAudioPid(i)) 
				{
					percent = live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3 ? VOLUME_DEFAULT_AC3 : VOLUME_DEFAULT_PCM;
					break;
				}
			}
		}
	}
	
	dprintf(DEBUG_INFO, "CZapit::getPidVolume: channel 0x%llx pid %x percent %d\n", channel_id, pid, percent);
	
	return percent;
}

void CZapit::setVolumePercent(int percent)
{
	dprintf(DEBUG_NORMAL, "CZapit::setVolumePercent: current_volume %d volume_percent %d percent %d\n", current_volume, volume_percent, percent);
		
	if (volume_percent != percent) 
		volume_percent = percent;
		
	int vol = current_volume + (current_volume*volume_percent)/100;
		
	dprintf(DEBUG_NORMAL, "CZapit::setVolumePercent: vol %d current_volume %d volume_percent %d\n", vol, current_volume, volume_percent);
		
	audioDecoder->setVolume(vol, vol);
}

int CZapit::changeAudioPid(uint8_t index)
{
	if ((!audioDemux) || (!audioDecoder) || (!live_channel))
		return -1;

	//stop audio playback
	if (audioDecoder->Stop() < 0)
		return -1;

	//stop audio demux filter
	if (audioDemux->Stop() < 0)
		return -1;

	//update current channel
	live_channel->setAudioChannel(index);

	//set bypass mode
	CZapitAudioChannel * currentAudioChannel = live_channel->getAudioChannel();

	if (!currentAudioChannel) 
	{
		dprintf(DEBUG_INFO, "CZapit::changeAudioPid: No current audio live_channel\n");
		return -1;
	}
	
	//set audio pid
	if(audioDecoder)
	{
		switch (currentAudioChannel->audioChannelType) 
		{
			case CZapitAudioChannel::AC3:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AC3);
				break;
			
			case CZapitAudioChannel::MPEG:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_MPEG);
				break;
				
			case CZapitAudioChannel::AAC:				
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC);				
				break;
			
			case CZapitAudioChannel::AACHE:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC_HE);
				break;
			
			case CZapitAudioChannel::DTS:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTS);
				break;
				
			case CZapitAudioChannel::DTSHD:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTSHD);
				break;
				
			case CZapitAudioChannel::EAC3:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_EAC3);
				break;
				
			case CZapitAudioChannel::LPCM:
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_LPCMDVD);
				break;
				
			default:
				dprintf(DEBUG_NORMAL, "CZapit::changeAudioPid: unknown audio live_channel audiotype 0x%x\n", currentAudioChannel->audioChannelType);
				break;
		}
	}

	dprintf(DEBUG_NORMAL, "CZapit::changeAudioPid: change apid to 0x%x\n", live_channel->getAudioPid());

	//set audio-demux filter
	if (audioDemux->pesFilter( live_channel->getAudioPid() ) < 0)
		return -1;

	//start demux filter
	if (audioDemux->Start() < 0)
		return -1;
	
	// set saved volume pro pid
	volume_percent = getPidVolume(live_channel_id, live_channel->getAudioPid(), currentAudioChannel->audioChannelType == CZapitAudioChannel::AC3);
	setVolumePercent(volume_percent);
	
	//FIXME: is muted
	if(current_muted)
		audioDecoder->SetMute(true);
			
	//start audio playback
	if (audioDecoder && (audioDecoder->Start() < 0))
		return -1;

	return 0;
}

void CZapit::setRadioMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::setRadioMode:\n");

	currentMode |= RADIO_MODE;
	currentMode &= ~TV_MODE;

	// FIXME: ???
	openAVDecoder();
}

void CZapit::setTVMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::setTVMode:\n");

	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;

	// FIXME: ???
	openAVDecoder();
}

// getchannelsMode
int CZapit::getMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::getMode:\n");
	
	int mode = MODE_CURRENT;
	
	if (currentMode & TV_MODE)
		mode = MODE_TV;
	else if (currentMode & RADIO_MODE)
		mode = MODE_RADIO;

	return mode;
}

void CZapit::enableRecordMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::enableRecordMode:\n");
	
	if(currentMode & RECORD_MODE) 
		return;

	currentMode |= RECORD_MODE;
	
	// lock frontend
	lockFrontend(record_fe);
	
	int status = 1;
	 
	eventServer->sendEvent(NeutrinoMessages::EVT_RECORDMODE, &status, sizeof(int) );
}

void CZapit::disableRecordMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::disableRecordMode:\n");
	
	if(!(currentMode & RECORD_MODE)) 
		return;
	
	// capmt
	sendCaPmtRecordStop();
	
	rec_channel_id = 0;
	rec_channel = NULL;
	
	// unlock record frontend
	unlockFrontend(record_fe);

	// zapit mode
	currentMode &= ~RECORD_MODE;
	
	int status = 0;
 
	eventServer->sendEvent(NeutrinoMessages::EVT_RECORDMODE, &status, sizeof(int) );
}

void CZapit::renumServices(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::renumServices:\n");
	
	int tvi = 1;
	int ri = 1;
	
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		if ((it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE)) 
		{
			it->second.setNumber(tvi++);
		}
		else if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE) 
		{
			it->second.setNumber(ri++);
		}
	}
}

int CZapit::prepareChannels()
{
	dprintf(DEBUG_NORMAL, "CZapit::prepareChannels:\n");
	
	live_channel = 0;
	
	// clear all channels/bouquets/TP's lists
	transponders.clear();
	clearAll();
	allchans.clear();  				// <- this invalidates all bouquets, too!
	
	// load frontend config
	loadFrontendConfig();
        
    	// load tps
    	loadTransponders();

	// load services
	loadServices(false);

	// load bouquets
	loadBouquets(); // this load also webtv services
	
	// renum services
	renumServices();

	return 0;
}

//
void CZapit::sendCurrentAPIDs(APIDList &apids)
{
	for (uint32_t  i = 0; i < live_channel->getAudioChannelCount(); i++) 
	{
		responseGetAPIDs response;
		response.pid = live_channel->getAudioPid(i);
		strncpy(response.desc, live_channel->getAudioChannel(i)->description.c_str(), 25);

		response.is_ac3 = 0;
		
		if (live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AAC
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACHE
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::EAC3
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTS
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTSHD
			) 
		{
			response.is_ac3 = 1;
		} 
		
		response.component_tag = live_channel->getAudioChannel(i)->componentTag;
		
		apids.push_back(response);
	}
}

void CZapit::sendCurrentSubPIDs(SubPIDList &subpids)
{
	for (int i = 0 ; i < (int)live_channel->getSubtitleCount() ; ++i) 
	{
		responseGetSubPIDs response;
		CZapitAbsSub* s = live_channel->getChannelSub(i);
		CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
		CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(s);

		response.pid = sd->pId;
		strncpy(response.desc, sd->ISO639_language_code.c_str(), 4);
		
		if (s->thisSubType == CZapitAbsSub::DVB) 
		{
			response.composition_page = sd->composition_page_id;
			response.ancillary_page = sd->ancillary_page_id;
			if (sd->subtitling_type >= 0x20) 
			{
				response.hearingImpaired = true;
			} 
			else 
			{
				response.hearingImpaired = false;
			}
		} 
		else if (s->thisSubType == CZapitAbsSub::TTX) 
		{
			response.composition_page = (st->teletext_magazine_number * 100) + ((st->teletext_page_number >> 4) * 10) + (st->teletext_page_number & 0xf);
			response.ancillary_page = 0;
			response.hearingImpaired = st->hearingImpaired;
		}
		
		subpids.push_back(response);
	}
}

//
void CZapit::sendRecordAPIDs(APIDList &apids)
{
	for (uint32_t  i = 0; i < rec_channel->getAudioChannelCount(); i++) 
	{
		responseGetAPIDs response;
		
		response.pid = rec_channel->getAudioPid(i);
		strncpy(response.desc, rec_channel->getAudioChannel(i)->description.c_str(), 25);

		response.is_ac3 = 0;
		
		if (rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AAC
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACHE
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::EAC3
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTS
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTSHD
			) 
		{
			response.is_ac3 = 1;
		} 
		
		response.component_tag = rec_channel->getAudioChannel(i)->componentTag;
		
		apids.push_back(response);
	}
}

void CZapit::sendRecordSubPIDs(SubPIDList &subpids)
{	
	for (int i = 0 ; i < (int)rec_channel->getSubtitleCount() ; ++i) 
	{
		responseGetSubPIDs response;
		
		CZapitAbsSub* s = rec_channel->getChannelSub(i);
		CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
		CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(s);

		response.pid = sd->pId;
		strncpy(response.desc, sd->ISO639_language_code.c_str(), 4);
		if (s->thisSubType == CZapitAbsSub::DVB) 
		{
			response.composition_page = sd->composition_page_id;
			response.ancillary_page = sd->ancillary_page_id;
			if (sd->subtitling_type >= 0x20) 
			{
				response.hearingImpaired = true;
			} 
			else 
			{
				response.hearingImpaired = false;
			}
		} 
		else if (s->thisSubType == CZapitAbsSub::TTX) 
		{
			response.composition_page = (st->teletext_magazine_number * 100) + ((st->teletext_page_number >> 4) * 10) + (st->teletext_page_number & 0xf);
			response.ancillary_page = 0;
			response.hearingImpaired = st->hearingImpaired;
		}
		
		subpids.push_back(response);
	}
}

////
void CZapit::sendAPIDs(t_channel_id chid, APIDList &apids)
{
	CZapitChannel * channel = findChannelByChannelID(chid);
	
	if (channel)
	{
		for (uint32_t  i = 0; i < channel->getAudioChannelCount(); i++) 
		{
			responseGetAPIDs response;
			response.pid = channel->getAudioPid(i);
			strncpy(response.desc, channel->getAudioChannel(i)->description.c_str(), 25);

			response.is_ac3 = 0;
			
			if (channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3
				|| channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AAC
				|| channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACHE
				|| channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::EAC3
				|| channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTS
				|| channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::DTSHD
				) 
			{
				response.is_ac3 = 1;
			} 
			
			response.component_tag = channel->getAudioChannel(i)->componentTag;
			
			apids.push_back(response);
		}
	}
}

void CZapit::sendSubPIDs(t_channel_id chid, SubPIDList &subpids)
{
	CZapitChannel * channel = findChannelByChannelID(chid);
	
	if (channel)
	{
		for (int i = 0 ; i < (int)channel->getSubtitleCount() ; ++i) 
		{
			responseGetSubPIDs response;
			
			CZapitAbsSub* s = channel->getChannelSub(i);
			CZapitDVBSub* sd = reinterpret_cast<CZapitDVBSub*>(s);
			CZapitTTXSub* st = reinterpret_cast<CZapitTTXSub*>(s);

			response.pid = sd->pId;
			strncpy(response.desc, sd->ISO639_language_code.c_str(), 4);
			
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				response.composition_page = sd->composition_page_id;
				response.ancillary_page = sd->ancillary_page_id;
				
				if (sd->subtitling_type >= 0x20) 
				{
					response.hearingImpaired = true;
				} 
				else 
				{
					response.hearingImpaired = false;
				}
			} 
			else if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				response.composition_page = (st->teletext_magazine_number * 100) + ((st->teletext_page_number >> 4) * 10) + (st->teletext_page_number & 0xf);
				response.ancillary_page = 0;
				response.hearingImpaired = st->hearingImpaired;
			}
			
			subpids.push_back(response);
		}
	}
}

//
void CZapit::internalSendChannels(ZapitChannelList *channels, const unsigned int first_channel_nr, BouquetChannelList &Bchannels)
{
	for (uint32_t  i = 0; i < channels->size();i++) 
	{
		responseGetBouquetChannels response;
		
		strncpy(response.name, ((*channels)[i]->getName()).c_str(), CHANNEL_NAME_SIZE);
		response.name[CHANNEL_NAME_SIZE-1] = 0;	
		response.satellitePosition = (*channels)[i]->getSatellitePosition();
		response.channel_id = (*channels)[i]->getChannelID();
		response.nr = first_channel_nr + i;
		
		Bchannels.push_back(response);
	}
}

void CZapit::sendBouquetChannels(BouquetChannelList &Bchannels, const unsigned int bouquet, const channelsMode mode)
{
	if (bouquet >= Bouquets.size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::sendBouquetChannels: invalid bouquet number: %d\n", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_RADIO))
		internalSendChannels(&(Bouquets[bouquet]->radioChannels), radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
	else if (((currentMode & TV_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_TV))
		internalSendChannels(&(Bouquets[bouquet]->tvChannels), tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
}

//// bouquetsManager
// CBouquetManager
void CZapit::writeBouquetHeader(FILE * bouq_fd, uint32_t i, const char * bouquetName)
{
	fprintf(bouq_fd, "\t<Bouquet name=\"%s\" hidden=\"%d\" locked=\"%d\">\n", bouquetName, Bouquets[i]->bHidden ? 1 : 0, Bouquets[i]->bLocked ? 1 : 0);
}

void CZapit::writeBouquetFooter(FILE * bouq_fd)
{
	fprintf(bouq_fd, "\t</Bouquet>\n");
}

void CZapit::writeBouquetChannels(FILE * bouq_fd, uint32_t i)
{
	bool write_names = 1;

	for ( unsigned int j = 0; j < Bouquets[i]->tvChannels.size(); j++) 
	{
		if(write_names) 
		{
			fprintf(bouq_fd, "\t\t<S i=\"%x\" n=\"%s\" t=\"%x\" on=\"%x\"/>\n",
				Bouquets[i]->tvChannels[j]->getServiceId(),
				UTF8_to_UTF8XML(Bouquets[i]->tvChannels[j]->getName().c_str()).c_str(),
				Bouquets[i]->tvChannels[j]->getTransportStreamId(),
				Bouquets[i]->tvChannels[j]->getOriginalNetworkId()
				);
		} 
		else 
		{
			fprintf(bouq_fd, "\t\t<S i=\"%x\" t=\"%x\" on=\"%x\"/>\n",
				Bouquets[i]->tvChannels[j]->getServiceId(),
				Bouquets[i]->tvChannels[j]->getTransportStreamId(),
				Bouquets[i]->tvChannels[j]->getOriginalNetworkId()
				);
		}
	}
	
	for ( unsigned int j = 0; j < Bouquets[i]->radioChannels.size(); j++) 
	{
		if(write_names) 
		{
			fprintf(bouq_fd, "\t\t<S i=\"%x\" n=\"%s\" t=\"%x\" on=\"%x\"/>\n",
				Bouquets[i]->radioChannels[j]->getServiceId(),
				UTF8_to_UTF8XML(Bouquets[i]->radioChannels[j]->getName().c_str()).c_str(),
				Bouquets[i]->radioChannels[j]->getTransportStreamId(),
				Bouquets[i]->radioChannels[j]->getOriginalNetworkId()
				);
		} 
		else 
		{
			fprintf(bouq_fd, "\t\t<S i=\"%x\" t=\"%x\" on=\"%x\"/>\n",
				Bouquets[i]->radioChannels[j]->getServiceId(),
				Bouquets[i]->radioChannels[j]->getTransportStreamId(),
				Bouquets[i]->radioChannels[j]->getOriginalNetworkId()
				);
		}
	}
}

void CZapit::saveZapitBouquets(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::saveZapitBouquets:\n");
	
	FILE * bouq_fd;
	
	bouq_fd = fopen(BOUQUETS_XML, "w");
	fprintf(bouq_fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");
	
	for (unsigned int i = 0; i < Bouquets.size(); i++) 
	{
		if (Bouquets[i] != remainChannels) 
		{
			dprintf(DEBUG_INFO, "CZapit::saveZapitBouquets: name %s user: %d webtv: %d\n", Bouquets[i]->Name.c_str(), Bouquets[i]->bUser, Bouquets[i]->bWebTV);
			
			if(!Bouquets[i]->bUser && !Bouquets[i]->bWebTV)
			{
				writeBouquetHeader(bouq_fd, i, UTF8_to_UTF8XML(Bouquets[i]->Name.c_str()).c_str());
				writeBouquetChannels(bouq_fd, i);
				writeBouquetFooter(bouq_fd);
			}
		}
	}
	
	fprintf(bouq_fd, "</zapit>\n");
	fdatasync(fileno(bouq_fd));
	fclose(bouq_fd);
	chmod(BOUQUETS_XML, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void CZapit::saveZapitUBouquets(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::saveZapitUBouquets:\n");
	
	FILE * ubouq_fd;
	
	ubouq_fd = fopen(UBOUQUETS_XML, "w");
	fprintf(ubouq_fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");
	
	for (unsigned int i = 0; i < Bouquets.size(); i++) 
	{
		if (Bouquets[i] != remainChannels) 
		{
			dprintf(DEBUG_INFO, "CZapit::saveZapitUBouquets: name %s user: %d webtv: %d\n", Bouquets[i]->Name.c_str(), Bouquets[i]->bUser, Bouquets[i]->bWebTV);
			
			if(Bouquets[i]->bUser && !Bouquets[i]->bWebTV) 
			{
				writeBouquetHeader(ubouq_fd, i, UTF8_to_UTF8XML(Bouquets[i]->Name.c_str()).c_str());
				writeBouquetChannels(ubouq_fd, i);
				writeBouquetFooter(ubouq_fd);
			}
		}
	}
	
	fprintf(ubouq_fd, "</zapit>\n");
	fdatasync(fileno(ubouq_fd));
	fclose(ubouq_fd);
	chmod(UBOUQUETS_XML, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

//
void CZapit::sortBouquets(void)
{
	sort(Bouquets.begin(), Bouquets.end(), CmpBouquetByChName());
}

void CZapit::parseBouquetsXml(const char *fname, bool bUser)
{
	xmlDocPtr parser = NULL;

	parser = parseXmlFile(fname);

	if (parser == NULL)
		return;

	xmlNodePtr root = xmlDocGetRootElement(parser);
	xmlNodePtr search = root->xmlChildrenNode;
	xmlNodePtr channel_node;

	if (search) 
	{
		t_original_network_id original_network_id;
		t_service_id service_id;
		t_transport_stream_id transport_stream_id;
		t_satellite_position  satellitePosition = 0;
		freq_id_t freq = 0;
		
		dprintf(DEBUG_INFO, "CZapit::parseBouquetsXml: %s\n", fname);

		while ((search = xmlGetNextOccurence(search, "Bouquet")) != NULL) 
		{
			char * name = xmlGetAttribute(search, (char *) "name");
			CZapitBouquet *newBouquet = addBouquet(name, bUser);
			char *hidden = xmlGetAttribute(search, (char *) "hidden");
			char *locked = xmlGetAttribute(search, (char *) "locked");
			newBouquet->bHidden = hidden ? (strcmp(hidden, "1") == 0) : false;
			newBouquet->bLocked = locked ? (strcmp(locked, "1") == 0) : false;
			newBouquet->bFav = (strcmp(name, "Favorites") == 0);
			
			//
			channel_node = search->xmlChildrenNode;

			while ((channel_node = xmlGetNextOccurence(channel_node, "S")) != NULL) 
			{
				std::string  service_name = xmlGetAttribute(channel_node, (char *) "n");
				GET_ATTR(channel_node, (char *) "i", "%hx", service_id);
				GET_ATTR(channel_node, (char *) "on", "%hx", original_network_id);
				GET_ATTR(channel_node, (char *) "t", "%hx", transport_stream_id);

				// grab satelliteposition / freq from channel map
				for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				{
					if(it->second.getServiceId() == service_id)
					{
						satellitePosition = it->second.getSatellitePosition();
						freq = it->second.getFreqId();
					}
				}
				
				//
				CZapitChannel *chan = findChannelByChannelID(CREATE_CHANNEL_ID);

				if (chan != NULL) 
				{
					if(!bUser)
						chan->pname = (char *) newBouquet->Name.c_str();

					chan->bAlwaysLocked = newBouquet->bLocked;
					newBouquet->addService(chan);
				}

				channel_node = channel_node->xmlNextNode;
			}

			if(!bUser)
				newBouquet->sortBouquet();
				
			search = search->xmlNextNode;
		}

		dprintf(DEBUG_DEBUG, "CZapit::parseBouquetsXml: found %d bouquets\n", (int)Bouquets.size());
	}

	xmlFreeDoc(parser);
	parser = NULL;
}

void CZapit::makeBouquetfromCurrentservices(const xmlNodePtr root)
{
	xmlNodePtr provider = root->xmlChildrenNode;
	
	// TODO: use locales
	CZapitBouquet * newBouquet = addBouquet("New channel");
	newBouquet->bHidden = false;
	newBouquet->bLocked = false;
	
	t_service_id          	service_id;		
	t_original_network_id 	original_network_id;
	t_transport_stream_id 	transport_stream_id;
	t_satellite_position  	satellitePosition;
	freq_id_t 		freq = 0;
	
	while (provider) 
	{	
		xmlNodePtr transponder = provider->xmlChildrenNode;
		
		while (xmlGetNextOccurence(transponder, "transponder") != NULL) 
		{
			xmlNodePtr channel_node = transponder->xmlChildrenNode;
			
			while (xmlGetNextOccurence(channel_node, "channel") != NULL) 
			{
				
				if (strncmp(xmlGetAttribute(channel_node, "action"), "remove", 6)) 
				{
					GET_ATTR(provider, "position", "%hd", satellitePosition);
					GET_ATTR(transponder, "onid", "%hx", original_network_id);
					GET_ATTR(transponder, "id", "%hx", transport_stream_id);
					GET_ATTR(channel_node, "service_id", "%hx", service_id);
								
					CZapitChannel *chan = findChannelByChannelID(CREATE_CHANNEL_ID);

					if (chan != NULL)
						newBouquet->addService(chan);
				}
			
				channel_node = channel_node->xmlNextNode;
			}
			transponder = transponder->xmlNextNode;
		}

		provider = provider->xmlNextNode;
	}
}

//
void CZapit::parseWebTVBouquet(std::string &filename)
{
	int cnt = 0;	

	dprintf(DEBUG_INFO, "CZapit::parseWebTVBouquet: parsing %s\n", filename.c_str());

	xmlDocPtr parser = NULL;
	
	// check for extension
	bool iptv = false;
	bool webtv = false;
	bool playlist = false;
					
	std::string extension = getFileExt(filename);
						
	if( strcasecmp("tv", extension.c_str()) == 0)
		iptv = true;
	else if( strcasecmp("m3u8", extension.c_str()) == 0)
		playlist = true;
	else if( strcasecmp("m3u", extension.c_str()) == 0)
		playlist = true;
	if( strcasecmp("xml", extension.c_str()) == 0)
		webtv = true;

	std::string name = std::string(rindex(filename.c_str(), '/') + 1);
	removeExtension(name);

	CZapitBouquet *newBouquet = addBouquetIfNotExist(name, false, true);
	
	if(iptv)
	{
		FILE * f = fopen(filename.c_str(), "r");

		std::string title;
		std::string url;
		std::string description;
		
		if(f != NULL)
		{
			while(true)
			{
				t_channel_id id = 0;
				char line[1024] = "";
				
				if (!fgets(line, 1024, f))
					break;
				
				size_t len = strlen(line);
				if (len < 2)
					// Lines with less than one char aren't meaningful
					continue;
				
				// strip newline
				line[--len] = 0;
				
				// strip carriage return (when found)
				if (line[len - 1] == '\r')
					line[len - 1 ] = 0;
				
				if (strncmp(line, "#SERVICE 4097:0:1:0:0:0:0:0:0:0:", 32) == 0)
					url = line + 32;
				else if (strncmp(line, "#DESCRIPTION", 12) == 0)
				{
					int offs = line[12] == ':' ? 14 : 13;
			
					title = line + offs;

					description = "stream";

					if(!url.empty())
					{
						id = create_channel_id(0, 0, 0, 0, 0, url.c_str());
					
						std::pair<std::map<t_channel_id, CZapitChannel>::iterator, bool> ret;

						ret = allchans.insert (std::pair<t_channel_id, CZapitChannel> (id, CZapitChannel(title, id, url, description)));

						CZapitChannel *chan = findChannelByChannelID(id);

						if (chan != NULL) 
						{
							chan->setName(title);
							chan->setDescription(description);
							
							//		
							newBouquet->addService(chan);

							cnt++;
						}
					}
				}
			}
			
			fclose(f);
		}
	}
	else if(webtv)
	{
		if (parser != NULL)
		{
			xmlFreeDoc(parser);
			parser = NULL;
		}

		parser = parseXmlFile(filename.c_str());
		
		if (parser) 
		{
			xmlNodePtr l0 = NULL;
			xmlNodePtr l1 = NULL;
			l0 = xmlDocGetRootElement(parser);
			l1 = l0->xmlChildrenNode;
			
			
			if (l1) 
			{
				while ( ((xmlGetNextOccurence(l1, "webtv")) || (xmlGetNextOccurence(l1, "station")))) 
				{
					t_channel_id id = 0;
					t_channel_id epg_id = 0;
					
					const char * title;
					const char * url;
					const char * description;
					const char* xmltv;
					const char* logo;
					const char* epgid;
					
					// 
					if(xmlGetNextOccurence(l1, "webtv"))
					{
						title = xmlGetAttribute(l1, (const char *)"title");
						url = xmlGetAttribute(l1, (const char *)"url");
						description = xmlGetAttribute(l1, (const char *)"description");
						epgid = xmlGetAttribute(l1, (const char*)"epgid");
						xmltv = xmlGetAttribute(l1, (const char*)"xmltv");
						logo = xmlGetAttribute(l1, (const char*)"logo");

						id = create_channel_id(0, 0, 0, 0, 0, url);
							
						std::pair<std::map<t_channel_id, CZapitChannel>::iterator, bool> ret;
						ret = allchans.insert(std::pair<t_channel_id, CZapitChannel> (id, CZapitChannel(title, id, url, description)));
						
						CZapitChannel * chan = findChannelByChannelID(id);

						if (chan != NULL) 
						{
							chan->setName(title);
							chan->setDescription(description);
							
							if (xmltv != NULL)
							{
								chan->setEPGUrl(xmltv);
								
								//
								g_settings.xmltv.push_back(xmltv);
							}
							
							if (logo != NULL)
							{
								chan->setLogoUrl(logo);
								chan->setLogoID(id);
							}
							
							if (epgid != NULL)
							{ 
								epg_id = strtoull(epgid, NULL, 16);
								
								chan->setEPGID(epg_id);
								chan->setEPGIDName(epgid);
							}
							
							// grab from list
							for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
							{
								if (chan->getName() == it->second.getName())
								{
									if (epg_id == 0)
										chan->setEPGID(it->second.getEPGID());
										
									if (logo == NULL)
										chan->setLogoID(it->second.getLogoID());
								}
							}
							
							//
							newBouquet->addService(chan);

							cnt++;
						}
					}

					l1 = l1->xmlNextNode;
				}
			}
		}
		
		xmlFreeDoc(parser);
		parser = NULL;
	}
	else if(playlist)
	{
		std::ifstream infile;
		char cLine[1024] = ""; // causes stack-buffer-overflow
		t_channel_id id = 0;
		std::string xmltv = "";
		std::string description = "";
		std::string title = "";
		std::string prefix = "";
		std::string group = "";
		std::string epgid = "";
		std::string alogo = "";
				
		infile.open(filename.c_str(), std::ifstream::in);

		while (infile.good())
		{
			infile.getline(cLine, sizeof(cLine));
					
			// FIXME: remove CR
			//if(cLine[strlen(cLine) - 1] == '\r')
			//	cLine[strlen(cLine) - 1] = 0;
				
			std::string strLine = cLine;
			
			if (strLine.empty())
				continue;
				
			//
			if (strLine.find("#EXTM3U") != std::string::npos)
			{
				xmltv = ReadMarkerValue(strLine, "x-tvg-url");
				
				if (xmltv.empty()) xmltv = ReadMarkerValue(strLine, "tvg-url");
				
				if (!xmltv.empty())
				{
					std::string ext = getFileExt(xmltv);
					
					if (ext == "gz")
						changeFileNameExt(xmltv, "");
					
					//
					g_settings.xmltv.push_back(xmltv);
				}
			}
			
			if (strLine.find("#EXTINF") != std::string::npos)
			{
				int iColon = (int)strLine.find_first_of(':');
				int iComma = (int)strLine.find_first_of(',');
				title = "";
				prefix = "";
				group = "";
				description = "";
				alogo = "";
				id = 0;

				if (iColon >= 0 && iComma >= 0 && iComma > iColon)
				{
					iComma++;
					iColon++;
					title = strLine.substr(iComma);
					std::string strInfoLine = strLine.substr(iColon, --iComma - iColon);
					description = ReadMarkerValue(strInfoLine, "tvg-name=");
					prefix = ReadMarkerValue(strInfoLine, "group-prefix=");
					group = ReadMarkerValue(strInfoLine, "group-title=");
					epgid = ReadMarkerValue(strInfoLine, "tvg-id=");
					alogo = ReadMarkerValue(strInfoLine, "tvg-logo=");
				}
			}		
			else if(strlen(cLine) > 0 && cLine[0] != '#')
			{
				char *url = NULL;
				if ((url = strstr(cLine, "http://")) || (url = strstr(cLine, "https://")) || (url = strstr(cLine, "rtmp://")) || (url = strstr(cLine, "rtsp://")) || (url = strstr(cLine, "rtp://")) || (url = strstr(cLine, "mmsh://"))) 
				{
					if (url != NULL) 
					{
						description = "stream";
						std::string bqName = "WEBTV";
						
						//CZapitBouquet* gBouquet = pBouquet;
						
						if (!group.empty())
						{
							bqName = group;
							bqName += " (";
							bqName += name;
							bqName += ")";
							newBouquet = addBouquetIfNotExist(bqName, false, true);	
						}
						
						//
						id = create_channel_id(0, 0, 0, 0, 0, url);
							
						std::pair<std::map<t_channel_id, CZapitChannel>::iterator, bool> ret;
						ret = allchans.insert(std::pair<t_channel_id, CZapitChannel> (id, CZapitChannel(title, id, url, description)));
					
						CZapitChannel * chan = findChannelByChannelID(id);

						if (chan != NULL) 
						{
							chan->setName(title);
							chan->setDescription(description);
							
							if (!alogo.empty()) 
							{
								chan->setLogoUrl(alogo);
								chan->setLogoID(id);
							}
							
							if (!epgid.empty()) 
							{
								chan->setEPGIDName(epgid);
							}
							
							if (!xmltv.empty()) chan->setEPGUrl(xmltv);
							
							// grab from list
							for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
							{
								if (chan->getName() == it->second.getName())
								{
									if (epgid.empty())
										chan->setEPGID(it->second.getEPGID());
										
									if (alogo.empty())
										chan->setLogoID(it->second.getLogoID());
								}
							}
							
							//
							newBouquet->addService(chan);

							cnt++;
						}
					}
				}
			}
		}

		infile.close();
	}
	
	for (unsigned int i = 0; i < Bouquets.size(); i++) 
	{
		if (Bouquets[i]->bWebTV && Bouquets[i]->tvChannels.empty())
			deleteBouquet(Bouquets[i]);
	}

	dprintf(DEBUG_INFO, "CZapit::loadWebTVBouquet: load %d WEBTV Channels (allchans:%d)\n", cnt, (int) allchans.size());
}

void CZapit::loadWebTVBouquets(const std::string &dirname)
{
	dprintf(DEBUG_INFO, "CZapit::loadWebTVBouquets: %s\n", dirname.c_str());

	dirent64 **namelist;
	int n;

	n = scandir64(dirname.c_str(), &namelist, 0, alphasort64);

	if (n < 0)
	{
		perror(("CZapit::loadWebTVBouquets: scandir: " + dirname).c_str());
		return;
	}
	
	for(int i = 0; i < n; i++)
	{
		std::string file;
		if( (strcmp(namelist[i]->d_name, ".") != 0) && (strcmp(namelist[i]->d_name, "..") != 0) )
		{
			// name
			file = dirname + "/" + namelist[i]->d_name;

			parseWebTVBouquet(file); //FIXME:
		}
		free(namelist[i]);
	}

	free(namelist);
}

//
void CZapit::loadBouquets(bool loadCurrentBouquet)
{
	dprintf(DEBUG_NORMAL, "CZapit::loadBouquets:\n");

	clearAll();
	
	// bouquets
	parseBouquetsXml(BOUQUETS_XML, false);
	sortBouquets();

	// ubouquets
	parseBouquetsXml(UBOUQUETS_XML, true);
	
	// current bouquets
	if(loadCurrentBouquet)
	{
		xmlDocPtr parser = NULL;
		parser = parseXmlFile(CURRENTSERVICES_XML);
		if (parser != NULL)
		{
			dprintf(DEBUG_INFO, "reading %s\n", CURRENTSERVICES_XML);

			makeBouquetfromCurrentservices(xmlDocGetRootElement(parser));
			xmlFreeDoc(parser);
			parser = NULL;
		}
	}

	// webTV bouquets
	loadWebTVBouquets(CONFIGDIR "/webtv"); //FIXME: ==35683==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffc0899c7df at pc 0x5624fd9cdcf8 bp 0x7ffc08999fa0 sp 0x7ffc08999f90

	//
	makeRemainingChannelsBouquets();
}

void CZapit::makeRemainingChannelsBouquets(void)
{
	ZapitChannelList unusedChannels;
	std::set<t_channel_id> chans_processed;
	bool tomake = config.getBool("makeRemainingChannelsBouquet", true);
	
	//
	if(remainChannels)
		deleteBouquet(remainChannels);
		
	remainChannels = NULL;

	for (std::vector<CZapitBouquet*>::const_iterator it = Bouquets.begin(); it != Bouquets.end(); it++) 
	{
		// tvChannels
		for (std::vector<CZapitChannel*>::iterator jt = (*it)->tvChannels.begin(); jt != (*it)->tvChannels.end(); jt++) 
		{
			if(tomake) 
				chans_processed.insert((*jt)->getChannelID());
			
			if(!(*jt)->pname && !(*it)->bUser) 
				(*jt)->pname = (char *) (*it)->Name.c_str();
		}

		// radioChannels
		for (std::vector<CZapitChannel*>::iterator jt = (*it) ->radioChannels.begin(); jt != (*it)->radioChannels.end(); jt++) 
		{
			if(tomake) 
				chans_processed.insert((*jt)->getChannelID());

			if(!(*jt)->pname && !(*it)->bUser) 
				(*jt)->pname = (char *) (*it)->Name.c_str();
		}
	}

	if(!tomake)
		return;

	//
	remainChannels = addBouquet((Bouquets.size() == 0) ? "All Channels" : "Other");

	for (tallchans::iterator it = allchans.begin(); it != allchans.end(); it++)
	{
		if (chans_processed.find(it->first) == chans_processed.end())
			unusedChannels.push_back(&(it->second));
	}

	sort(unusedChannels.begin(), unusedChannels.end(), CmpChannelByChName()); //FIXME:

	for (ZapitChannelList::const_iterator it = unusedChannels.begin(); it != unusedChannels.end(); it++) 
	{
		remainChannels->addService(findChannelByChannelID((*it)->getChannelID()));
	}

	if ((remainChannels->tvChannels.empty()) && (remainChannels->radioChannels.empty())) 
	{
		deleteBouquet(remainChannels);
		remainChannels = NULL;
		return;
	}
}

//
CZapitBouquet * CZapit::addBouquet(const std::string& name, bool ub, bool webtvb)
{
	CZapitBouquet * newBouquet = new CZapitBouquet(name);
	newBouquet->bUser = ub;
	newBouquet->bWebTV = webtvb;

	if(ub) 
	{
		BouquetList::iterator it;
		for(it = Bouquets.begin(); it != Bouquets.end(); it++)
		{
			if(!(*it)->bUser)
				break;
		}
				
		Bouquets.insert(it, newBouquet);
	} 
	else
		Bouquets.push_back(newBouquet);

	return newBouquet;
}

////
CZapitBouquet *CZapit::addBouquet(const std::string &name, BouquetList &list, bool ub, bool webtvb)
{
	CZapitBouquet * newBouquet = new CZapitBouquet(name);
	newBouquet->bUser = ub;
	newBouquet->bWebTV = webtvb;

	if(ub) 
	{
		BouquetList::iterator it;
		for(it = list.begin(); it != list.end(); it++)
		{
			if(!(*it)->bUser)
				break;
		}
				
		list.insert(it, newBouquet);
	} 
	else
		list.push_back(newBouquet);

	return newBouquet;
}

//
CZapitBouquet* CZapit::addBouquetIfNotExist(const std::string& name, bool ub, bool webtvb)
{
	CZapitBouquet* bouquet = NULL;

	int bouquetId = existsBouquet(name.c_str());
	
	if (bouquetId == -1)
		bouquet = addBouquet(name, ub, webtvb);
	else
		bouquet = Bouquets[bouquetId];

	return bouquet;
}

//
CZapitBouquet* CZapit::addBouquetIfNotExist(const std::string& name, BouquetList &list, bool ub, bool webtvb)
{
	CZapitBouquet* bouquet = NULL;

	int bouquetId = existsBouquet(name.c_str(), list);
	
	if (bouquetId == -1)
		bouquet = addBouquet(name, list, ub, webtvb);
	else
		bouquet = list[bouquetId];

	return bouquet;
}

void CZapit::deleteBouquet(const unsigned int id)
{
	if (id < Bouquets.size() && Bouquets[id] != remainChannels)
		deleteBouquet(Bouquets[id]);
}

void CZapit::deleteBouquet(const CZapitBouquet* bouquet)
{
	if (bouquet != NULL) 
	{
		BouquetList::iterator it = find(Bouquets.begin(), Bouquets.end(), bouquet);

		if (it != Bouquets.end()) 
		{
			Bouquets.erase(it);
			delete bouquet;
		}
	}
}

// -- Find Bouquet-Name, if BQ exists   (2002-04-02 rasc)
// -- Return: Bouqet-ID (found: 0..n)  or -1 (Bouquet does not exist)
int CZapit::existsBouquet(const char * const name)
{
	for (unsigned int i = 0; i < Bouquets.size(); i++) 
	{
		if ( Bouquets[i]->Name == name )
			return (int)i;
	}
	
	return -1;
}

////
int CZapit::existsBouquet(const char * const name, BouquetList &list)
{
	for (unsigned int i = 0; i < list.size(); i++) 
	{
		if ( list[i]->Name == name )
			return (int)i;
	}
	
	return -1;
}

//
bool CZapit::existsChannelInBouquet( unsigned int bq_id, const t_channel_id channel_id)
{
	bool status = false;
	CZapitChannel  *ch = NULL;

	if (bq_id <= Bouquets.size()) 
	{
		// query TV-Channels  && Radio channels
		ch = Bouquets[bq_id]->getChannelByChannelID(channel_id, 0);

		if (ch)  
			status = true;
	}

	return status;
}

void CZapit::moveBouquet(const unsigned int oldId, const unsigned int newId)
{
	if ((oldId < Bouquets.size()) && (newId < Bouquets.size())) 
	{
		BouquetList::iterator it = Bouquets.begin();

		advance(it, oldId);
		CZapitBouquet* tmp = *it;
		Bouquets.erase(it);

		advance(it, newId - oldId);
		Bouquets.insert(it, tmp);
	}
}

////
void CZapit::renameBouquet(const unsigned int bouquet, const char * const newName)
{
	if (bouquet < Bouquets.size()) 
	{
		Bouquets[bouquet]->Name = newName;
		Bouquets[bouquet]->bUser = true;
	}
}

void CZapit::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	if (bouquet < Bouquets.size())
		Bouquets[bouquet]->bLocked = lock;
}

void CZapit::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	if (bouquet < Bouquets.size())
		Bouquets[bouquet]->bHidden = hidden;
}

void CZapit::clearAll()
{
	for (unsigned int i = 0; i < Bouquets.size(); i++)
		delete Bouquets[i];

	Bouquets.clear();
	remainChannels = NULL;
}

CZapitChannel *CZapit::findChannelByChannelID(const t_channel_id channel_id)
{
	tallchans_iterator itChannel = allchans.find(channel_id);
	
	if (itChannel != allchans.end())
		return &(itChannel->second);

	return NULL;
}

CZapitChannel *CZapit::findChannelByName(std::string name, const t_service_id sid)
{
	for (tallchans_iterator itChannel = allchans.begin(); itChannel != allchans.end(); ++itChannel) 
	{
		if( (itChannel->second.getName().length() == name.length() && !strcasecmp(itChannel->second.getName().c_str(), name.c_str()) ) && (itChannel->second.getServiceId() == sid) ) 
		{
			return &itChannel->second;
		}
	}
	
	return NULL;
}

//// ChannelIterator
CZapit::ChannelIterator::ChannelIterator(CZapit *owner, const CZapit::channelsMode Mode)
{
	Owner = owner;
	mode = Mode;

	if (Owner->Bouquets.size() == 0)
		c = -2;
	else 
	{
		b = 0;
		c = -1; 
		(*this)++;
	}
}

CZapit::ChannelIterator CZapit::ChannelIterator::operator ++(int)
{
	if (c != -2)  // we can add if it's not the end marker
	{
		c++;
		if ((unsigned int) c >= getBouquet()->size()) 
		{
			for (b++; b < Owner->Bouquets.size(); b++)
			{
				if (getBouquet()->size() != 0) 
				{
					c = 0;
					goto end;
				}
			}
			
			c = -2;
		}
	}

 end:
	return(*this);
}

CZapitChannel* CZapit::ChannelIterator::operator *()
{
	return (*getBouquet())[c];               // returns junk if we are an end marker !!
}

CZapit::ChannelIterator CZapit::ChannelIterator::FindChannelNr(const unsigned int channel)
{
	c = channel;
	
	for (b = 0; b < Owner->Bouquets.size(); b++)
	{
		if (getBouquet()->size() > (unsigned int)c)
			goto end;
		else
			c -= getBouquet()->size();
	}
	
	c = -2;
	
end:
	return (*this);
}

//
int CZapit::ChannelIterator::getLowestChannelNumberWithChannelID(const t_channel_id channel_id)
{
	int i = 0;

	for (b = 0; b < Owner->Bouquets.size(); b++)
	{
		for (c = 0; (unsigned int) c < getBouquet()->size(); c++, i++)
		{
			if ((**this)->getChannelID() == channel_id)
				return (**this)->number -1;
		}
	}
		
	return -1; // not found
}

//
int CZapit::ChannelIterator::getNrofFirstChannelofBouquet(const unsigned int bouquet_nr)
{
	if (bouquet_nr >= Owner->Bouquets.size())
		return -1;  // not found

	int i = 0;

	for (b = 0; b < bouquet_nr; b++)
	{
		i += getBouquet()->size();
	}

	return i;
}

//// startplayback return: 0=playing, -1= failed
int CZapit::startPlayBack(CZapitChannel * thisChannel)
{
	dprintf(DEBUG_NORMAL, "CZapit::startPlayBack: chid: 0x%llx\n", thisChannel->getChannelID());

	if(!thisChannel)
		thisChannel = live_channel;

	if(standby) 
		return 0;

	if (!thisChannel || playing)
		return -1;

	if (IS_WEBTV(thisChannel->getChannelID()))
	{
		//
		closeAVDecoder();
		////test
		playback->Close(); // not needed???
		
		playback->Open();
	
		if (!playback->Start((char *)thisChannel->getUrl().c_str()))
			return -1;
	}
	else
	{
		if (playbackStopForced)
			return -1;
			
		// 
		openAVDecoder();

		bool have_pcr = false;
		bool have_audio = false;
		bool have_video = false;
		bool have_teletext = false;

		dprintf(DEBUG_NORMAL, "CZapit::startPlayBack: vpid 0x%X apid 0x%X pcrpid 0x%X\n", thisChannel->getVideoPid(), thisChannel->getAudioPid(), thisChannel->getPcrPid() );

		if (thisChannel->getPcrPid() != 0)
			have_pcr = true;

		if (thisChannel->getAudioPid() != 0)
			have_audio = true;
		
		if ((thisChannel->getVideoPid() != 0) && (currentMode & TV_MODE))
			have_video = true;
		
		if (thisChannel->getTeletextPid() != 0)
			have_teletext = true;

		if ((!have_audio) && (!have_video) && (!have_teletext))
			return -1;

		if(have_video && (thisChannel->getPcrPid() == 0x1FFF)) 
		{ 
			//FIXME
			thisChannel->setPcrPid(thisChannel->getVideoPid());
			have_pcr = true;
		}
		
		// pcr pid
		if (have_pcr) 
		{
			if(!pcrDemux)
				pcrDemux = new cDemux();
		
			// open pcr demux		
			if( pcrDemux->Open(DMX_PCR_ONLY_CHANNEL, VIDEO_STREAM_BUFFER_SIZE, live_fe) < 0 )
				return -1;	
		
			// set pes filter
			if( pcrDemux->pesFilter(thisChannel->getPcrPid() ) < 0 )
				return -1;
		
			// start dmx
			if ( pcrDemux->Start() < 0 )
				return -1;
		}
	
		// audio pid
		if (have_audio) 
		{
			if( !audioDemux )
				audioDemux = new cDemux();
		
			// open audio demux		
			if( audioDemux->Open(DMX_AUDIO_CHANNEL, AUDIO_STREAM_BUFFER_SIZE, live_fe ) < 0 )
				return -1;	
		
			// set pes filter
			if( audioDemux->pesFilter(thisChannel->getAudioPid() ) < 0 )
				return -1;
		
			if ( audioDemux->Start() < 0 )
				return -1;

			// start pid
			const char *audioStr = "UNKNOWN";
		
			if(audioDecoder)
			{
				switch (thisChannel->getAudioChannel()->audioChannelType) 
				{
					case CZapitAudioChannel::AC3:
						audioStr = "AC3";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AC3);
						break;
					
					case CZapitAudioChannel::MPEG:
						audioStr = "MPEG2";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_MPEG);
						break;
					
					case CZapitAudioChannel::AAC:
						audioStr = "AAC";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC);				
						break;
					
					case CZapitAudioChannel::AACHE:
						audioStr = "AAC-HE";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC_HE);
						break;
					
					case CZapitAudioChannel::DTS:
						audioStr = "DTS";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTS);
						break;
					
					case CZapitAudioChannel::DTSHD:
						audioStr = "DTSHD";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTSHD);
						break;
					
					case CZapitAudioChannel::EAC3:
						audioStr = "EAC3";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_EAC3);
						break;
					
					case CZapitAudioChannel::LPCM:
						audioStr = "LPCM";
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_LPCMDVD);
						break;
					
					default:
						dprintf(DEBUG_INFO, "CZapit::startPlayBack: unknown audio live_channel type 0x%x\n", thisChannel->getAudioChannel()->audioChannelType);
						break;
				}
			}
		
			dprintf(DEBUG_NORMAL, "CZapit::startPlayBack: starting %s audio Pid: 0x%X\n", audioStr, thisChannel->getAudioPid());		
		
			// start audio decoder
			if(audioDecoder)
			{			
#if !defined (__sh__)		
				audioDecoder->Resume();
				audioDecoder->Stop();
#endif				  
				audioDecoder->Start();
			}
		}
	
		// video pid
		if (have_video) 
		{
			if( !videoDemux )
				videoDemux = new cDemux(); 
		
			// open Video Demux		
			if( videoDemux->Open(DMX_VIDEO_CHANNEL, VIDEO_STREAM_BUFFER_SIZE, live_fe ) < 0 )
				return -1;

			// set stream type
			const char *videoStr = "UNKNOWN";
		
			if(videoDecoder)
			{
				if(thisChannel->videoType == CHANNEL_VIDEO_MPEG2)
				{
					videoStr = "MPEG2";
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_MPEG2);
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_MPEG4)
				{
					videoStr = "H.264/MPEG-4 AVC";
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_MPEG4_H264);				
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_HEVC)
				{
					videoStr = "H.265 HEVC";
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_H265_HEVC);				
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_CAVS)
				{
					videoStr = "AVS";
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_AVS);				
				}
			}
	
			dprintf(DEBUG_NORMAL, "CZapit::startPlayBack: starting %s video Pid: 0x%x\n", videoStr, thisChannel->getVideoPid());	
					
			// video pes filter
			if( videoDemux->pesFilter(thisChannel->getVideoPid() ) < 0)
				return -1;		
		
			if ( videoDemux->Start() < 0 )
				return -1;

			// start Video Decoder
			if(videoDecoder)
			{
#if !defined (__sh__)
				videoDecoder->Resume();
				videoDecoder->Stop();
#endif				  			
				videoDecoder->Start();	
			}
		}
	}

	playing = true;
	
	return 0;
}

int CZapit::stopPlayBack(bool sendPmt)
{
	dprintf(DEBUG_NORMAL, "CZapit::stopPlayBack: standby %d forced %d\n", standby, playbackStopForced);

	if (!playing)
		return 0;
	
	//
	if (IS_WEBTV(live_channel->getChannelID()))
	{
		playback->Close();
	}
	else
	{
		if (playbackStopForced)
			return -1;

		// capmt
		sendcapmtPlayBackStop(sendPmt);
		
		// stop audio decoder
		audioDecoder->Stop();

		// stop audio demux
		if (audioDemux)
		{
			// stop
			audioDemux->Stop();
			delete audioDemux;  //destructor closes dmx
			audioDemux = NULL;
		}
		
		// stop video decoder (blanking)
		videoDecoder->Stop(true);

		// stop video demux
		if (videoDemux)
		{
			// stop
			videoDemux->Stop();
			delete videoDemux;	//destructor closes dmx
			videoDemux = NULL;
		}
	
		if (pcrDemux)
		{
			// stop
			pcrDemux->Stop();
			delete pcrDemux; //destructor closes dmx
			pcrDemux = NULL;
		}
	
		// stop tuxtxt subtitle
		tuxtx_stop_subtitle();

		// stop?pause dvbsubtitle
		if(standby)
			dvbsub_pause();
		else
			dvbsub_stop();
	}

	playing = false;

	return 0;
}

void CZapit::pausePlayBack(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::pausePlayBack\n");

	//
	if (IS_WEBTV(live_channel->getChannelID()))
		playback->SetSpeed(0);
	else
	{
		audioDecoder->Pause();
		videoDecoder->Pause();
	}
}

void CZapit::continuePlayBack(void)
{
	dprintf(DEBUG_DEBUG, "CZapit::continuePlayBack\n");

	//
	if (IS_WEBTV(live_channel->getChannelID()))
		playback->SetSpeed(1);
	else
	{
		audioDecoder->Resume();
		videoDecoder->Resume();
	}
}

void CZapit::closeAVDecoder(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::closeAVDecoder:\n");
	
	if (!avDecoderOpen)
		return;
	
	// close videodecoder
	if(videoDecoder)
	{
		videoDecoder->Close();
	}
	
	// close audiodecoder
	if(audioDecoder)
	{
		audioDecoder->Close();
	}

	avDecoderOpen = false;
}

void CZapit::openAVDecoder(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::openAVDecoder:\n");
	
	if (avDecoderOpen)
		return;
	
	if(videoDecoder)
	{
		// open video decoder
		videoDecoder->Open();	//FIXME: adapter ???
	
		// set source
		videoDecoder->setSource(VIDEO_SOURCE_DEMUX);	
	}	
	
	if(audioDecoder)
	{
		// open audiodecoder
		audioDecoder->Open();
		
		// set source
		audioDecoder->setSource(AUDIO_SOURCE_DEMUX);
	}

	avDecoderOpen = true;	
}

void CZapit::enterStandby(void)
{ 
	if (standby)
		return;

	standby = true;

	// save zapitconfig
	saveZapitSettings(true, true);
	
	// stop playback
	stopPlayBack(true);
	
	// close AVdecoder
	closeAVDecoder();	
	
	//close frontend	
	CloseFE();
}

void CZapit::leaveStandby(void)
{ 
	dprintf(DEBUG_NORMAL, "CZapit::leaveStandby\n");
	
	if (!standby)
		return;
	
	standby = false;
		
	openAVDecoder();	

	// zap
	zapToChannelID(live_channel_id, current_is_nvod);
}

unsigned CZapit::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	if (bouquet >= Bouquets.size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::zapTo: Invalid bouquet %d\n", bouquet);
		return ZAP_INVALID_PARAM;
	}

	ZapitChannelList * channels;

	if (currentMode & RADIO_MODE)
		channels = &(Bouquets[bouquet]->radioChannels);
	else if (currentMode & TV_MODE)
		channels = &(Bouquets[bouquet]->tvChannels);

	if (channel >= channels->size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::zapTo: Invalid channel %d in bouquet %d\n", channel, bouquet);
		return ZAP_INVALID_PARAM;
	}

	return zapToChannelID((*channels)[channel]->getChannelID(), false);
}

unsigned int CZapit::zapToChannelID(t_channel_id channel_id, bool isSubService)
{
	unsigned int result = 0;

	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::zapToChannelID: chid 0x%llx\n", channel_id);

	if (zapit(channel_id, isSubService) < 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: zapit failed, chid 0x%llx\n", channel_id);
		
		eventServer->sendEvent((isSubService ? NeutrinoMessages::EVT_ZAP_SUB_FAILED : NeutrinoMessages::EVT_ZAP_FAILED), &channel_id, sizeof(channel_id));
		
		return result;
	}

	result |= ZAP_OK;

	dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: zapit OK, chid 0x%llx\n", channel_id);
	
	if (isSubService) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: isSubService chid 0x%llx\n", channel_id);
		
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_SUB_COMPLETE, &channel_id, sizeof(channel_id));
	}
	else if (current_is_nvod) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: NVOD chid 0x%llx\n", channel_id);
		
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_ISNVOD, &channel_id, sizeof(channel_id));
		
		result |= ZAP_IS_NVOD;
	}
	else
	{
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_COMPLETE, &channel_id, sizeof(channel_id));
	}

	return result;
}

unsigned CZapit::zapTo(const unsigned int channel)
{
	//FIXME:
	CZapit::ChannelIterator cit = (tvChannelsBegin()).FindChannelNr(channel);

	if (currentMode & RADIO_MODE)
		cit = (radioChannelsBegin()).FindChannelNr(channel);
	else if (currentMode & TV_MODE)
		cit = (tvChannelsBegin()).FindChannelNr(channel);
	
	if (!(cit.EndOfChannels()))
		return zapToChannelID((*cit)->getChannelID(), false);
	else
		return 0;
}

void CZapit::setZapitConfig(Zapit_config * Cfg)
{
	makeRemainingChannelsBouquet = Cfg->makeRemainingChannelsBouquet;
	saveLastChannel = Cfg->saveLastChannel;
	scanSDT = Cfg->scanSDT;
	////
	lastChannelMode = Cfg->lastchannelmode;
	lastChannelTV = Cfg->startchanneltv_nr;
	lastChannelRadio = Cfg->startchannelradio_nr;
	lastChannelTV_id = Cfg->startchanneltv_id;
	lastChannelRadio_id = Cfg->startchannelradio_id;
	
	// save it
	saveZapitSettings(true, false);
}

void CZapit::getZapitConfig(Zapit_config *Cfg)
{
        Cfg->makeRemainingChannelsBouquet = makeRemainingChannelsBouquet;
        Cfg->saveLastChannel = saveLastChannel;
        Cfg->scanSDT = scanSDT;
        ////
        Cfg->lastchannelmode = lastChannelMode;
        Cfg->startchanneltv_nr = lastChannelTV;
        Cfg->startchannelradio_nr = lastChannelRadio;
        Cfg->startchanneltv_id = lastChannelTV_id;
        Cfg->startchannelradio_id = lastChannelRadio_id;
}

//
void * CZapit::sdtThread(void */*arg*/)
{
	dprintf(DEBUG_NORMAL, "CZapit::sdtThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	if (!CZapit::getInstance()->getFrontendCount())
		return 0;
	
	time_t tstart, tcur, wtime = 0;
	int ret;
	t_transport_stream_id transport_stream_id = 0;
	t_original_network_id original_network_id = 0;
	t_satellite_position satellitePosition = 0;
	freq_id_t freq = 0;

	transponder_id_t tpid = 0;
	FILE * fd = 0;
	FILE * fd1 = 0;
	bool updated = 0;
	
	CSdt sdt;

	tcur = time(0);
	tstart = time(0);
	CZapit::getInstance()->sdt_tp.clear();
	
	dprintf(DEBUG_INFO, "CZaoit::sdtThread: sdt monitor started\n");

	while(true) 
	{
		sleep(1);

		if(CZapit::getInstance()->sdt_wakeup) 
		{
			CZapit::getInstance()->sdt_wakeup = 0;

			if(CZapit::getInstance()->getCurrentChannel()) 
			{
				wtime = time(0);
				transport_stream_id = CZapit::getInstance()->getCurrentChannel()->getTransportStreamId();
				original_network_id = CZapit::getInstance()->getCurrentChannel()->getOriginalNetworkId();
				satellitePosition = CZapit::getInstance()->getCurrentChannel()->getSatellitePosition();
				freq = CZapit::getInstance()->getCurrentChannel()->getFreqId();
				tpid = CZapit::getInstance()->getCurrentChannel()->getTransponderId();
			}
		}
		
		if(!CZapit::getInstance()->scanSDT)
			continue;

		tcur = time(0);
		
		if(wtime && ((tcur - wtime) > 2) && !CZapit::getInstance()->sdt_wakeup) 
		{
			dprintf(DEBUG_INFO, "Czapit::sdtThread: sdt monitor wakeup...\n");
			
			wtime = 0;

			if(scan_runs)
				continue;

			updated = 0;
			tallchans_iterator ccI;
			tallchans_iterator dI;
			transponder_list_t::iterator tI;
			sdt_tp_t::iterator stI;
			char tpstr[256];
			char satstr[256];
			bool tpdone = 0;
			bool satfound = 0;
		
			tI = transponders.find(tpid);
			if(tI == transponders.end()) 
			{
				dprintf(DEBUG_INFO, "CZapit::sdtThread: tp not found ?!\n");
				continue;
			}
			stI = CZapit::getInstance()->sdt_tp.find(tpid);

			if((stI != CZapit::getInstance()->sdt_tp.end()) && stI->second) 
			{
				dprintf(DEBUG_INFO, "CZapit::sdtThread: TP already updated.\n");
				continue;
			}

			if(CZapit::getInstance()->getCurrentChannel()) 
			{
				if( sdt.parseCurrentSDT(transport_stream_id, original_network_id, satellitePosition, freq, CZapit::getInstance()->getCurrentFrontend()) < 0 )
					continue;
			}

			CZapit::getInstance()->sdt_tp.insert(std::pair<transponder_id_t, bool> (tpid, true) );

			char buffer[256];
			fd = fopen(CURRENTSERVICES_TMP, "w");
			
			if(!fd) 
			{
				dprintf(DEBUG_INFO, "CZapit::sdtThread: " CURRENTSERVICES_TMP ": cant open!\n");
				continue;
			}

			sat_iterator_t spos_it = satellitePositions.find(satellitePosition); 
			if(spos_it == satellitePositions.end())
				continue;
				
			const char* delsys = "";

			if(CZapit::getInstance()->getCurrentChannel()) 
			{
				switch(spos_it->second.system)
				{
					case DVB_S: /* satellite */
					case DVB_S2:
					case DVB_S2X:
						sprintf(satstr, "\t<%s name=\"%s\" position=\"%hd\">\n", "sat", spos_it->second.name.c_str(), satellitePosition);
						sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" sr=\"%u\" fec=\"%hu\" pol=\"%hu\">\n",
						tI->second.transport_stream_id, tI->second.original_network_id,
						tI->second.feparams.frequency, tI->second.feparams.inversion,
						tI->second.feparams.symbol_rate, tI->second.feparams.fec_inner,
						tI->second.feparams.polarization);
						delsys = "</sat>";
						break;

					case DVB_C: /* cable */
						sprintf(satstr, "\t<%s name=\"%s\">\n", "cable", spos_it->second.name.c_str());
						sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" sr=\"%u\" fec=\"%hu\" mod=\"%hu\">\n",
						tI->second.transport_stream_id, tI->second.original_network_id,
						tI->second.feparams.frequency, tI->second.feparams.inversion,
						tI->second.feparams.symbol_rate, tI->second.feparams.fec_inner,
						tI->second.feparams.modulation);
						delsys = "</cable>";
						break;
						
					case DVB_T: /* terrestrial */
					case DVB_T2:
					case DVB_DTMB:
						sprintf(satstr, "\t<%s name=\"%s\">\n", "terrestrial", spos_it->second.name.c_str());
						sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" bw=\"%hu\" hp=\"%hu\" lp=\"%hu\" con=\"%hu\" tm=\"%hu\" gi=\"%hu\" hi=\"%hu\" syss=\"%hu\">\n",
						tI->second.transport_stream_id, tI->second.original_network_id,
						tI->second.feparams.frequency, tI->second.feparams.inversion,
						tI->second.feparams.bandwidth, tI->second.feparams.code_rate_HP,
						tI->second.feparams.code_rate_LP, tI->second.feparams.modulation,tI->second.feparams.transmission_mode, tI->second.feparams.guard_interval, tI->second.feparams.hierarchy_information,
						tI->second.feparams.delsys);
						delsys = "</terrestrial>";
						break;

					default:
						break;
				}
			}

			fd1 = fopen(CURRENTSERVICES_XML, "r");

			if(!fd1) 
			{
				fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");
			} 
			else 
			{
				fgets(buffer, 255, fd1);
				while(!feof(fd1) && !strstr(buffer, satfound ? delsys : "</zapit>")) 
				{
					if(!satfound && !strcmp(buffer, satstr))
						satfound = 1;
						
					fputs(buffer, fd);
					fgets(buffer, 255, fd1);
				}
			}

			for (tallchans_iterator cI = curchans.begin(); cI != curchans.end(); cI++) 
			{
				ccI = allchans.find(cI->second.getChannelID());
				if(ccI == allchans.end()) 
				{
					if(!tpdone) 
					{
						if(!satfound) 
							fprintf(fd, "%s", satstr);
							
						fprintf(fd, "%s", tpstr);
						tpdone = 1;
					}
					updated = 1;

					fprintf(fd, "\t\t\t<S action=\"add\" i=\"%04x\" n=\"%s\" t=\"%x\"/>\n",
                                        	cI->second.getServiceId(), UTF8_to_UTF8XML(cI->second.getName().c_str()).c_str(),
                                        	cI->second.getServiceType());
				} 
				else 
				{
					if(strcmp(cI->second.getName().c_str(), ccI->second.getName().c_str())) 
					{
						if(!tpdone) 
						{
							if(!satfound) 
								fprintf(fd, "%s", satstr);
								
							fprintf(fd, "%s", tpstr);
							tpdone = 1;
						}
						updated = 1;
						
						fprintf(fd, "\t\t\t<S action=\"replace\" i=\"%04x\" n=\"%s\" t=\"%x\"/>\n",
		                                	cI->second.getServiceId(), UTF8_to_UTF8XML(cI->second.getName().c_str()).c_str(),
		                                	cI->second.getServiceType());
					}
				}
			}

			for (ccI = allchans.begin(); ccI != allchans.end(); ccI++) 
			{
				if(ccI->second.getTransponderId() == tpid) 
				{
					dI = curchans.find(ccI->second.getChannelID());
					if(dI == curchans.end()) 
					{
					   	if(!tpdone) 
						{
							if(!satfound) 
								fprintf(fd, "%s", satstr);

							fprintf(fd, "%s", tpstr);
							tpdone = 1;
					   	}

					   	updated = 1;
					   	fprintf(fd, "\t\t\t<S action=\"remove\" i=\"%04x\" n=\"%s\" t=\"%x\"/>\n",
                                        	ccI->second.getServiceId(), UTF8_to_UTF8XML(ccI->second.getName().c_str()).c_str(),
                                        	ccI->second.getServiceType());
					}
				}
			}

			if(tpdone) 
			{
				fprintf(fd, "\t\t</TS>\n");
				fprintf(fd, "\t%s\n", delsys);
			}
			else if(satfound)
			{
				fprintf(fd, "\t%s\n", delsys);
			}

			if(fd1) 
			{
				fgets(buffer, 255, fd1);
				while(!feof(fd1)) 
				{
					fputs(buffer, fd);
					fgets(buffer, 255, fd1);
				}

				if(!satfound) 
					fprintf(fd, "</zapit>\n");

				fclose(fd1);
			} 
			else
				fprintf(fd, "</zapit>\n");
				
			fclose(fd);

			rename(CURRENTSERVICES_TMP, CURRENTSERVICES_XML);

			if(updated && (CZapit::getInstance()->scanSDT == 1))
			{
				CZapit::getInstance()->loadServices(true);
			  	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICES_UPD);
			}

			dprintf(DEBUG_NORMAL, "CZapit::sdtThread: %s\n", updated? "found changes": "no changes found");
		}
	}

	return 0;
}

//
void *CZapit::updatePMTFilter(void *)
{
	dprintf(DEBUG_NORMAL, "CZapit::updatePMTFilter: tid %ld\n", syscall(__NR_gettid));
	
	if (!CZapit::getInstance()->getFrontendCount())
		return 0;
		
	//
	CPmt pmt;
	
	while (true) 
	{	
		// pmt update
		if (pmt_update_fd != -1) 
		{
			unsigned char buf[4096];
			int ret = pmtDemux->Read(buf, 4095, 10); /* every 10 msec */

			if (ret > 0) 
			{
				pmt.pmt_stop_update_filter(&pmt_update_fd);

				dprintf(DEBUG_INFO, "CZapit::updatePMTFilter: pmt updated, sid 0x%x new version 0x%x\n", (buf[3] << 8) + buf[4], (buf[5] >> 1) & 0x1F);

				// zap channel
				if(CZapit::getInstance()->getCurrentChannel()) 
				{
					t_channel_id channel_id = CZapit::getInstance()->getCurrentChannelID();
					int vpid = CZapit::getInstance()->getCurrentChannel()->getVideoPid();
					int apid = CZapit::getInstance()->getCurrentChannel()->getAudioPid();
					
					pmt.parse(CZapit::getInstance()->getCurrentChannel(), CZapit::getInstance()->getCurrentFrontend());
					
					bool apid_found = false;
					// check if selected audio pid still present
					for (int i = 0; i <  CZapit::getInstance()->getCurrentChannel()->getAudioChannelCount(); i++) 
					{
						if (CZapit::getInstance()->getCurrentChannel()->getAudioChannel(i)->pid == apid) 
						{
							apid_found = true;
							break;
						}
					}
					
					if(!apid_found || vpid != CZapit::getInstance()->getCurrentChannel()->getVideoPid()) 
					{
						CZapit::getInstance()->zapit(channel_id, CZapit::getInstance()->current_is_nvod, true);
					} 
					else 
					{
						// ci / cam
						CZapit::getInstance()->sendCaPmtPlayBackStart(CZapit::getInstance()->getCurrentChannel(), CZapit::getInstance()->getCurrentFrontend());

						pmt.pmt_set_update_filter(CZapit::getInstance()->getCurrentChannel(), &pmt_update_fd, CZapit::getInstance()->getCurrentFrontend());
					}
						
					eventServer->sendEvent(NeutrinoMessages::EVT_PMT_CHANGED, &channel_id, sizeof(channel_id));
				}
			}
		}

		usleep(0);
	}
	
	// stop update pmt filter
	pmt.pmt_stop_update_filter(&pmt_update_fd);
	pmt_update_fd = -1;
		
	
	pthread_exit(NULL);
}

//
void CZapit::getLastChannel(unsigned int &channumber, char &mode)
{
	responseGetLastChannel lastChannel;
	
	if (currentMode & RADIO_MODE)
	{
		lastChannel.mode = 'r';
		lastChannel.channelNumber = lastChannelRadio;
	}
	else if (currentMode & TV_MODE)
	{
		lastChannel.mode = 't';
		lastChannel.channelNumber = lastChannelTV;
	}
	
	channumber = lastChannel.channelNumber;
	mode = lastChannel.mode;
}

void CZapit::setMode(const channelsMode mode)
{			
	if (mode == MODE_TV)
		setTVMode();
	else if (mode == MODE_RADIO)
		setRadioMode();
}

void CZapit::lockPlayBack()
{
	stopPlayBack(true);
						
	closeAVDecoder();			
			
	playbackStopForced = true;
}

void CZapit::unlockPlayBack()
{
	playbackStopForced = false;
		
	openAVDecoder();			

	if(live_channel != NULL)
	{
		startPlayBack(live_channel);
			
		// cam
		if (!IS_WEBTV(live_channel->getChannelID()))
			sendCaPmtPlayBackStart(live_channel, live_fe);
	}					
}

bool CZapit::isPlayBackActive()
{
	return playing;
}

void CZapit::setStandby(bool enable)
{
	if (enable)
		enterStandby();
	else
		leaveStandby();
}

bool CZapit::isChannelTVChannel(const t_channel_id channel_id)
{
	bool ret = false;
			
	tallchans_iterator it = allchans.find(channel_id);
	if (it == allchans.end()) 
	{
		it = nvodchannels.find(channel_id);

		if (it == nvodchannels.end())
			ret = true;
		else
			ret = (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE);
	} 
	else
		ret = (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE);
		
	return ret;
}

bool CZapit::isChannelWEBTVChannel(const t_channel_id channel_id)
{
	bool ret = false;
			
	tallchans_iterator it = allchans.find(channel_id);
	if (it == allchans.end()) 
	{
		ret = it->second.isWebTV;
	} 
	
	ret = it->second.isWebTV;
		
	return ret;
}

bool CZapit::isChannelRadioChannel(const t_channel_id channel_id)
{
	bool ret = false;
			
	tallchans_iterator it = allchans.find(channel_id);
	if (it == allchans.end()) 
	{
		ret = (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE);
	} 

	ret = (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE);
		
	return ret;
}

void CZapit::zapToServiceIDNOWAIT(const t_channel_id channel_id)
{
	zapToChannelID(channel_id, false);
}

void CZapit::zapToSubServiceIDNOWAIT(const t_channel_id channel_id)
{
	zapToChannelID(channel_id, true);
}

unsigned int CZapit::zapToServiceID(const t_channel_id channel_id)
{
	unsigned int zapStatus = 0;
			
	zapStatus = zapToChannelID(channel_id, false);
		
	return zapStatus;
}

unsigned int CZapit::zapToSubServiceID(const t_channel_id channel_id)
{
	unsigned int zapStatus = 0;
			
	zapStatus = zapToChannelID(channel_id, true);
		
	return zapStatus;
}

//
std::string CZapit::getChannelName(const t_channel_id channel_id)
{
	std::string name = "";
	
	tallchans_iterator it = allchans.find(channel_id);
	if (it != allchans.end())
		name = it->second.getName().c_str();
		
	return name;
}

int CZapit::getChannelNumber(const t_channel_id channel_id)
{
	int number = 0;
	
	tallchans_iterator it = allchans.find(channel_id);
	if (it != allchans.end())
		number = it->second.number;
		
	return number;
}

std::string CZapit::getChannelURL(const t_channel_id channel_id)
{
	std::string url = "";
	
	tallchans_iterator it = allchans.find(channel_id);
	if (it != allchans.end())
		url = it->second.getUrl().c_str();
		
	return url;
}

std::string CZapit::getChannelDescription(const t_channel_id channel_id)
{
	std::string desc = "";
	
	tallchans_iterator it = allchans.find(channel_id);
	if (it != allchans.end())
		desc = it->second.getDescription().c_str();
		
	return desc;
}

//
std::string CZapit::getSatelliteName(t_satellite_position position)
{
	sat_iterator_t it = satellitePositions.find(position);
	
	if(it != satellitePositions.end())
		return it->second.name;
		
	return "";
}

void CZapit::getCurrentPIDS( responseGetPIDs &pids )
{
	if (live_channel) 
	{
		responseGetOtherPIDs responseGetOtherPIDs;
		APIDList apids;
		SubPIDList subpids;
		
		responseGetOtherPIDs.vpid = live_channel->getVideoPid();
		responseGetOtherPIDs.vtxtpid = live_channel->getTeletextPid();
		responseGetOtherPIDs.pmtpid = live_channel->getPmtPid();
		responseGetOtherPIDs.pcrpid = live_channel->getPcrPid();
		responseGetOtherPIDs.selected_apid = live_channel->getAudioChannelIndex();
		responseGetOtherPIDs.privatepid = live_channel->getPrivatePid();
		
		sendCurrentAPIDs(apids);
		sendCurrentSubPIDs(subpids);
		
		//
		pids.PIDs = responseGetOtherPIDs;
		pids.APIDs = apids;
		pids.SubPIDs = subpids;
	}
}

// currentService
CZapitChannel* CZapit::getCurrentChannel()
{
	return (live_channel != NULL) ? live_channel : NULL;
}

t_channel_id CZapit::getCurrentChannelID()
{
	return (live_channel != 0) ? live_channel->getChannelID() : 0;
}

CZapit::CServiceInfo CZapit::getCurrentServiceInfo()
{
	CServiceInfo msgCurrentServiceInfo;
	memset(&msgCurrentServiceInfo, 0, sizeof(CServiceInfo));
			
	if(live_channel) 
	{
		msgCurrentServiceInfo.onid = live_channel->getOriginalNetworkId();
		msgCurrentServiceInfo.sid = live_channel->getServiceId();
		msgCurrentServiceInfo.tsid = live_channel->getTransportStreamId();
		msgCurrentServiceInfo.vpid = live_channel->getVideoPid();
		msgCurrentServiceInfo.apid = live_channel->getAudioPid();
		msgCurrentServiceInfo.vtxtpid = live_channel->getTeletextPid();
		msgCurrentServiceInfo.pmtpid = live_channel->getPmtPid();
				
		msgCurrentServiceInfo.pmt_version = (live_channel->getCaPmt() != NULL) ? live_channel->getCaPmt()->version_number : 0xff;
				
		msgCurrentServiceInfo.pcrpid = live_channel->getPcrPid();
				
		if(live_fe != NULL)
		{
			msgCurrentServiceInfo.tsfrequency = live_fe->getFrequency();
			msgCurrentServiceInfo.rate = live_fe->getRate();
			msgCurrentServiceInfo.fec = live_fe->getCFEC();
						
			if ( live_fe->getInfo()->type == FE_QPSK)
				msgCurrentServiceInfo.polarisation = live_fe->getPolarization();
			else
				msgCurrentServiceInfo.polarisation = 2;
		}
				
		msgCurrentServiceInfo.vtype = live_channel->videoType;
	}
			
	if(!msgCurrentServiceInfo.fec)
		msgCurrentServiceInfo.fec = (fe_code_rate)3;
		
	return msgCurrentServiceInfo;
}

int32_t CZapit::getCurrentSatellitePosition()
{
	return (live_channel != 0)? live_channel->getSatellitePosition() : live_fe->getCurrentSatellitePosition();
}

bool CZapit::getCurrentTP(transponder *TP)
{
	//#FIXME:
	transponder_list_t::iterator transponder = transponders.find(live_channel->getTransponderId());
	TP = &transponder->second;
	
	return true;
}

CFrontend* CZapit::getCurrentFrontend()
{
	return live_fe;
}

//
void CZapit::getPIDS( t_channel_id chid, responseGetPIDs &pids )
{
	CZapitChannel * channel = findChannelByChannelID(chid);
	
	if (channel) 
	{
		responseGetOtherPIDs responseGetOtherPIDs;
		APIDList apids;
		SubPIDList subpids;
		
		responseGetOtherPIDs.vpid = channel->getVideoPid();
		responseGetOtherPIDs.vtxtpid = channel->getTeletextPid();
		responseGetOtherPIDs.pmtpid = channel->getPmtPid();
		responseGetOtherPIDs.pcrpid = channel->getPcrPid();
		responseGetOtherPIDs.selected_apid = channel->getAudioChannelIndex();
		responseGetOtherPIDs.privatepid = channel->getPrivatePid();
		
		sendAPIDs(chid, apids);
		sendSubPIDs(chid, subpids);
		
		//
		pids.PIDs = responseGetOtherPIDs;
		pids.APIDs = apids;
		pids.SubPIDs = subpids;
	}
}

CZapit::CServiceInfo CZapit::getServiceInfo(t_channel_id chid)
{
	CServiceInfo msgServiceInfo;
	memset(&msgServiceInfo, 0, sizeof(CServiceInfo));
	
	CZapitChannel * channel = findChannelByChannelID(chid);
	
	if (channel) 
	{
		msgServiceInfo.onid = channel->getOriginalNetworkId();
		msgServiceInfo.sid = channel->getServiceId();
		msgServiceInfo.tsid = channel->getTransportStreamId();
		msgServiceInfo.vpid = channel->getVideoPid();
		msgServiceInfo.apid = channel->getAudioPid();
		msgServiceInfo.vtxtpid = channel->getTeletextPid();
		msgServiceInfo.pmtpid = channel->getPmtPid();
				
		msgServiceInfo.pmt_version = (channel->getCaPmt() != NULL) ? channel->getCaPmt()->version_number : 0xff;
				
		msgServiceInfo.pcrpid = channel->getPcrPid();
		msgServiceInfo.vtype = channel->videoType;
		
		//
		transponder_list_t::iterator transponder = transponders.find(channel->getTransponderId());
		
		msgServiceInfo.tsfrequency = transponder->second.feparams.frequency;
		msgServiceInfo.rate = transponder->second.feparams.symbol_rate;
		msgServiceInfo.fec = transponder->second.feparams.fec_inner;
		msgServiceInfo.polarisation = transponder->second.feparams.polarization;
	}
			
	if(!msgServiceInfo.fec)
		msgServiceInfo.fec = (fe_code_rate)3;
		
	return msgServiceInfo;
}

void CZapit::setSubServices( subServiceList& subServices )
{
	//
	t_satellite_position  satellitePosition = 0;
	freq_id_t freq = 0;
		
	for (int i = 0; i < subServices.size(); i++)
	{
		nvodchannels.insert (
				std::pair<t_channel_id, CZapitChannel> (
					create_channel_id(subServices[i].service_id, subServices[i].original_network_id, subServices[i].transport_stream_id, satellitePosition, freq),
					CZapitChannel (
					"NVOD",
					subServices[i].service_id,
					subServices[i].transport_stream_id,
					subServices[i].original_network_id,
					ST_DIGITAL_TELEVISION_SERVICE,
					satellitePosition,
					freq)
				)
				);
	}

	current_is_nvod = true;
}

void CZapit::setRecordMode(const bool activate)
{		
	if (activate)
		enableRecordMode();
	else
		disableRecordMode();
}

bool CZapit::isRecordModeActive()
{
	bool activated = (currentMode & RECORD_MODE);
	
	return activated;
}

t_channel_id CZapit::getRecordChannelID()
{
	return (rec_channel != 0) ? rec_channel->getChannelID() : 0;
}

CZapit::CServiceInfo CZapit::getRecordServiceInfo()
{
	CServiceInfo msgRecordServiceInfo;
	memset(&msgRecordServiceInfo, 0, sizeof(CServiceInfo));
			
	if(rec_channel) 
	{
		msgRecordServiceInfo.onid = rec_channel->getOriginalNetworkId();
		msgRecordServiceInfo.sid = rec_channel->getServiceId();
		msgRecordServiceInfo.tsid = rec_channel->getTransportStreamId();
		msgRecordServiceInfo.vpid = rec_channel->getVideoPid();
		msgRecordServiceInfo.apid = rec_channel->getAudioPid();
		msgRecordServiceInfo.vtxtpid = rec_channel->getTeletextPid();
		msgRecordServiceInfo.pmtpid = rec_channel->getPmtPid();
				
		msgRecordServiceInfo.pmt_version = (rec_channel->getCaPmt() != NULL) ? rec_channel->getCaPmt()->version_number : 0xff;
				
		msgRecordServiceInfo.pcrpid = rec_channel->getPcrPid();
				
		if(live_fe != NULL)
		{
			msgRecordServiceInfo.tsfrequency = live_fe->getFrequency();
			msgRecordServiceInfo.rate = live_fe->getRate();
			msgRecordServiceInfo.fec = live_fe->getCFEC();
						
			if ( live_fe->getInfo()->type == FE_QPSK)
				msgRecordServiceInfo.polarisation = live_fe->getPolarization();
			else
				msgRecordServiceInfo.polarisation = 2;
		}
				
		msgRecordServiceInfo.vtype = rec_channel->videoType;
	}
			
	if(!msgRecordServiceInfo.fec)
		msgRecordServiceInfo.fec = (fe_code_rate)3;
		
	return msgRecordServiceInfo;
}

void CZapit::getRecordPIDS(responseGetPIDs &pids)
{
	//FIXME:
	if (rec_channel) 
	{
		responseGetOtherPIDs responseGetOtherPIDs;
		APIDList apids;
		SubPIDList subpids;
		
		responseGetOtherPIDs.vpid = rec_channel->getVideoPid();
		responseGetOtherPIDs.vtxtpid = rec_channel->getTeletextPid();
		responseGetOtherPIDs.pmtpid = rec_channel->getPmtPid();
		responseGetOtherPIDs.pcrpid = rec_channel->getPcrPid();
		responseGetOtherPIDs.selected_apid = rec_channel->getAudioChannelIndex();
		responseGetOtherPIDs.privatepid = rec_channel->getPrivatePid();
		
		sendRecordAPIDs(apids);
		sendRecordSubPIDs(subpids);
		
		//
		pids.PIDs = responseGetOtherPIDs;
		pids.APIDs = apids;
		pids.SubPIDs = subpids;
	}
}

void CZapit::reinitChannels()
{		
	t_channel_id cid = live_channel ? live_channel->getChannelID() : 0; 
	
	prepareChannels();
			
	tallchans_iterator cit = allchans.find(cid);
	if (cit != allchans.end()) 
		live_channel = &(cit->second); 
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED);
}

void CZapit::reloadCurrentServices()
{
	eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED);
}

//
void CZapit::muteAudio(const bool mute)
{
	if(audioDecoder) 
	{
		if (mute)
			audioDecoder->SetMute(true);
		else
			audioDecoder->SetMute(false);
	}
}

bool CZapit::getMuteStatus()
{
	return current_muted;
}

void CZapit::getAudioMode(int * mode)
{
	*mode = audio_mode;
}

void CZapit::setAudioMode(int mode)
{
	if(audioDecoder) 
		audioDecoder->setChannel(mode);
			
	audio_mode = mode;
}

void CZapit::setVolume(const unsigned int left, const unsigned int right)
{
	audioDecoder->setVolume(left, right);
}

void CZapit::getVolume(unsigned int *left, unsigned int *right)
{
	*left = *right = audioDecoder->getVolume();
}

void CZapit::setVolumePercent(const unsigned int percent, t_channel_id channel_id, const unsigned int apid)
{		
	// set/save pid volume
	setPidVolume(channel_id, apid, percent);
			
	// set volume percent
	setVolumePercent(percent);
			
	//FIXME: is is muted
	if(current_muted)
		audioDecoder->SetMute(true);
}

void CZapit::getVolumePercent(unsigned int *percent, t_channel_id channel_id, const unsigned int apid, const bool is_ac3)
{		
	for (int  i = 0; i < live_channel->getAudioChannelCount(); i++)
	{
		if (apid == live_channel->getAudioPid(i)) 
		{
			*percent = getPidVolume(live_channel_id, live_channel->getAudioPid(i), live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3);
		}
	}
}

void CZapit::setAudioChannel(const unsigned int channel)
{
	changeAudioPid(channel);
}

void CZapit::setVideoSystem(int video_system)
{
	if(videoDecoder)
		videoDecoder->SetVideoSystem(video_system);
}

//
bool CZapit::getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, const channelsMode mode, const bool utf_encoded)
{
	sendBouquetChannels(channels, bouquet, mode);
	
	return true;
}

// bqtedit
void CZapit::saveBouquets()
{
	dprintf(DEBUG_NORMAL, "CZapit::saveBouquets:\n");
	
	saveZapitBouquets();
	saveZapitUBouquets();
			
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED);
	
	if(g_list_changed) 
	{
		saveServices(true); //FIXME
		g_list_changed = 0;
	}
}

// bqtedit
void CZapit::restoreBouquets()
{
	dprintf(DEBUG_NORMAL, "CZapit::restoreBouquets:\n");
	
	if(g_list_changed) 
	{
		prepareChannels();
				
		g_list_changed = 0;
	} 
	else 
	{
		clearAll();
		loadBouquets();
	}
}

//
void CZapit::saveScanBouquets(const CZapit::bouquetMode bouquetMode, const char * const providerName)
{
	dprintf(DEBUG_NORMAL, "CZapit::saveScanBouquets: mode:%d scanBouquets:%d bouquets:%d\n", bouquetMode, scanBouquets.size(), Bouquets.size());
	
	if (bouquetMode == CZapit::BM_DELETEBOUQUETS)
	{
		clearAll();
		unlink(BOUQUETS_XML);
	}
	
	//
	while (!(scanBouquets.empty())) 
	{
		CZapitBouquet * bouquet;
		int dest = existsBouquet(scanBouquets[0]->Name.c_str());
			
		dprintf(DEBUG_NORMAL, "CZapit::saveScanBouquets: dest %d for name %s\n", dest, scanBouquets[0]->Name.c_str());

		if(dest == -1) 
		{
			bouquet = addBouquet(scanBouquets[0]->Name.c_str());
			dest = existsBouquet(scanBouquets[0]->Name.c_str());
		}
		else
			bouquet = Bouquets[dest];

		// tv bouquets
		for(unsigned int i = 0; i < scanBouquets[0]->tvChannels.size(); i++) 
		{
			if(!(existsChannelInBouquet(dest, scanBouquets[0]->tvChannels[i]->getChannelID()))) 
			{
				bouquet->addService(scanBouquets[0]->tvChannels[i]);

				dprintf(DEBUG_NORMAL, "CZapit::saveScanBouquets: adding channel %s\n", scanBouquets[0]->tvChannels[i]->getName().c_str());
			}
		}
			
		// radio bouquets
		for(unsigned int i = 0; i < scanBouquets[0]->radioChannels.size(); i++) 
		{
			if(!(existsChannelInBouquet(dest, scanBouquets[0]->radioChannels[i]->getChannelID()))) 
			{
				bouquet->addService(scanBouquets[0]->radioChannels[i]);

				dprintf(DEBUG_NORMAL, "CZapit::saveScanBouquets: adding channel %s\n", scanBouquets[0]->radioChannels[i]->getName().c_str());
			}
		}

		sortBouquets();
			
		//
		delete scanBouquets[0];
		scanBouquets.erase(scanBouquets.begin());
	}
}

//
void CZapit::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitChannel * chan = findChannelByChannelID(channel_id);

	if (chan != NULL)
	{
		if (bouquet < Bouquets.size())
			Bouquets[bouquet]->addService(chan);
		else
			printf("bouquet not found\n");
	}
	else
		printf("channel_id not found in channellist\n");
}

//
void CZapit::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	if (bouquet < Bouquets.size())
		Bouquets[bouquet]->removeService(channel_id);

	bool status = 0;
	for (unsigned int i = 0; i < Bouquets.size(); i++) 
	{
		status = existsChannelInBouquet(i, channel_id);
		if(status)
			break;
	}
	
	if(!status) 
	{
		allchans.erase(channel_id);
		live_channel = 0;
		g_list_changed = 1;
	}
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED);
}

//// scanManager
bool CZapit::tuneFrequency(FrontendParameters *feparams, t_satellite_position satellitePosition, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, "CZapit::%s:\n", __FUNCTION__);
	
	// setInput
	fe->setInput(satellitePosition, feparams->frequency, feparams->polarization);

	// driveToSatPos
	if(fe->getInfo()->type == FE_QPSK)
	{
		int ret = fe->driveToSatellitePosition(satellitePosition, false);
		
		if(ret > 0) 
		{
			dprintf(DEBUG_INFO, "CZapit::tuneFrequency: waiting %d seconds for motor to turn satellite dish.\n", ret);
			
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_PROVIDER, (void *) "moving rotor", 13);
		
			for(int i = 0; i < ret; i++) 
			{
				sleep(1);
					
				if(abort_scan)
					return false;
			}
		}
	}

	return fe->tuneFrequency(feparams);
}

int CZapit::addToScan(transponder_id_t TsidOnid, FrontendParameters *feparams, bool fromnit, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::addToScan: freq %d pol %d tpid 0x%llx from (nit:%d) fe(%d:%d)\n", feparams->frequency, feparams->polarization, TsidOnid, fromnit, fe->feadapter, fe->fenumber);

	freq_id_t freq;

#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
		freq = feparams->frequency / 100;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	else if (fe->getInfo()->type == FE_QPSK)
#endif
		freq = feparams->frequency / 1000;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM)
#endif
		freq = feparams->frequency / 1000000;
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
		freq = feparams->frequency / 1000000;

	uint8_t poltmp1 = feparams->polarization & 1;
	uint8_t poltmp2;

	stiterator tI;

	tI = scanedtransponders.find(TsidOnid);

	if (tI != scanedtransponders.end()) 
	{
		poltmp2 = tI->second.feparams.polarization & 1;

		if(poltmp2 != poltmp1) 
		{
			t_transport_stream_id transport_stream_id = tI->second.transport_stream_id;
			t_original_network_id original_network_id = tI->second.original_network_id;

			freq_id_t freq1 = GET_FREQ_FROM_TRANSPONDER_ID(tI->first);

			t_satellite_position satellitePosition = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;

			if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
				satellitePosition = -satellitePosition;

			freq++;
			
			TsidOnid = CREATE_TRANSPONDER_ID(freq1, satellitePosition, original_network_id, transport_stream_id);

			dprintf(DEBUG_INFO, "CZapit::addToScan: SAME freq %d pol1 %d pol2 %d tpid 0x%llx\n", feparams->frequency, poltmp1, poltmp2, TsidOnid);

			feparams->frequency = feparams->frequency + 1000;
			tI = scanedtransponders.find(TsidOnid);
		}
	}
        else for (tI = scanedtransponders.begin(); tI != scanedtransponders.end(); tI++) 
	{
		poltmp2 = tI->second.feparams.polarization & 1;
		
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
				nittransponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams)));
			}
		}
		else 
		{
			found_transponders++;
			scantransponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams)));

			scanedtransponders.insert (std::pair <transponder_id_t, transponder> ( TsidOnid, transponder ( (TsidOnid >> 16) &0xFFFF, TsidOnid &0xFFFF, *feparams)));
		}
		
		return 0;
	}
	else
		tI->second.feparams.fec_inner = feparams->fec_inner;

	return 1;
}

bool CZapit::getSDTS(t_satellite_position satellitePosition, CFrontend* fe)
{
	transponder_id_t TsidOnid = 0;
	stiterator tI;
	stiterator stI;
	std::map <transponder_id_t, transponder>::iterator sT;
	CNit nit;
	CSdt sdt;

	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::getSDTS: scanning tp from sat/service\n");

_repeat:
	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++) 
	{
		if(abort_scan)
			return false;

		dprintf(DEBUG_NORMAL, "CZapit::getSDTS: scanning: 0x%llx\n", tI->first);

		//
		actual_freq = tI->second.feparams.frequency;

		processed_transponders++;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, &processed_transponders, sizeof(processed_transponders));
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_PROVIDER, (void *) " ", 2);
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SERVICENAME, (void *) " ", 2);
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY, &actual_freq, sizeof(actual_freq));

		// by sat send pol to neutrino
#if HAVE_DVB_API_VERSION >= 5
		if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
		if (fe->getInfo()->type == FE_QPSK)
#endif		
		{
			actual_polarisation = ((tI->second.feparams.symbol_rate/1000) << 16) | (tI->second.feparams.fec_inner << 8) | (uint)tI->second.feparams.polarization;

			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP, &actual_polarisation, sizeof(actual_polarisation));
		}
		
		// tune TP
		if (!tuneFrequency(&(tI->second.feparams), satellitePosition, fe)) 
		{
			failed_transponders++;
			continue;
		}

		if(abort_scan)
			return false;

		// parseSDT
		freq_id_t freq;

#if HAVE_DVB_API_VERSION >= 5 
		if (fe->getForcedDelSys() == DVB_C)
#else
		if (fe->getInfo()->type == FE_QAM)
#endif
			freq = tI->second.feparams.frequency/100;
#if HAVE_DVB_API_VERSION >= 5
		else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
		else if (fe->getInfo()->type == FE_QPSK)
#endif
			freq = tI->second.feparams.frequency/1000;
#if HAVE_DVB_API_VERSION >= 5
		else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
		else if (fe->getInfo()->type == FE_OFDM)
#endif
			freq = tI->second.feparams.frequency/1000000;
#if HAVE_DVB_API_VERSION >= 5
    		else if (fe->getForcedDelSys() == DVB_A)
#else
		else if (fe->getInfo()->type == FE_ATSC)
#endif
			freq = tI->second.feparams.frequency/1000000;
			
		// parse sdt
		dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::getSDTS: parsing SDT (tsid:onid %04x:%04x)\n", tI->second.transport_stream_id, tI->second.original_network_id);
		
		if(sdt.parse(&tI->second.transport_stream_id, &tI->second.original_network_id, satellitePosition, freq, fe) < 0)
		{
			dprintf(DEBUG_NORMAL, "CZapit::getSDTS: SDT failed !\n");
			continue;
		}

		TsidOnid = CREATE_TRANSPONDER_ID(freq, satellitePosition, tI->second.original_network_id, tI->second.transport_stream_id);

		stI = transponders.find(TsidOnid);
		if(stI == transponders.end())
			transponders.insert (std::pair <transponder_id_t, transponder> (TsidOnid, transponder(tI->second.transport_stream_id, tI->second.original_network_id, tI->second.feparams)));
		else
			stI->second.feparams.fec_inner = tI->second.feparams.fec_inner;
		
		// parse nit
		if(!scanmode) 
		{
			dprintf(DEBUG_INFO, "CZapit::getSDTS: parsing NIT\n");
			
			if( nit.parse(satellitePosition, freq, fe) < 0 )
			{
				dprintf(DEBUG_INFO, "CZapit::getSDTS: NIT failed !\n");
			}
		}

		dprintf(DEBUG_INFO, "CZapit::getSDTS: tpid ready: 0x%llx\n", TsidOnid);
	}

	// add found transponder by nit to scan
	if(!scanmode) 
	{
		dprintf(DEBUG_INFO, "CZapit::getSDTS: found %d transponders (%d failed) and %d channels\n", found_transponders, failed_transponders, found_channels);
		
		scantransponders.clear();
		
		for (tI = nittransponders.begin(); tI != nittransponders.end(); tI++) 
		{
			addToScan(tI->first, &tI->second.feparams, false, fe);
		}

		nittransponders.clear();
		
		dprintf(DEBUG_INFO, "CZapit::getSDTS: found %d additional transponders from nit\n", scantransponders.size());
		
		if(scantransponders.size()) 
		{
			eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS, &found_transponders, sizeof(found_transponders));

			goto _repeat;
		}
	}

	return true;
}

bool CZapit::scanTransponder(xmlNodePtr transponder, t_satellite_position satellitePosition, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanTransponder:\n");
	
	uint8_t system = 0;
	int xml_fec;
	FrontendParameters feparams;
	
	memset(&feparams, 0x00, sizeof(FrontendParameters));

	freq_id_t freq;
	feparams.inversion = (fe_spectral_inversion_t)INVERSION_AUTO;
	feparams.delsys = fe->getForcedDelSys(); // ???

	//frequency
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	if (fe->getInfo()->type == FE_OFDM) 
#endif
		feparams.frequency = xmlGetNumericAttribute(transponder, "centre_frequency", 0);
	else
		feparams.frequency = xmlGetNumericAttribute(transponder, "frequency", 0);

	// freq
#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM )
#endif
		freq = feparams.frequency/100;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif
		freq = feparams.frequency/1000;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
		freq = feparams.frequency/1000000;
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
		freq = feparams.frequency/1000000;
	
	// 	
#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
	{
		feparams.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "modulation", 0);
		feparams.delsys = DVB_C;
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		feparams.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(transponder, "bandwidth", 0);
		feparams.code_rate_HP = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "code_rate_hp", 0);
		feparams.code_rate_LP = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "code_rate_lp", 0);
		feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "constellation", 0);
		feparams.transmission_mode = (fe_transmit_mode_t) xmlGetNumericAttribute(transponder, "transmission_mode", 0);
		feparams.guard_interval = (fe_guard_interval_t) xmlGetNumericAttribute(transponder, "guard_interval", 0);
		feparams.hierarchy_information = (fe_hierarchy_t) xmlGetNumericAttribute(transponder, "hierarchy_information", 0);
		
		if (xmlGetAttribute(transponder, (char *)"inversion")) 
			feparams.inversion = (fe_spectral_inversion_t)xmlGetNumericAttribute(transponder, "inversion", 0);
		
		if (xmlGetAttribute(transponder, (char *)"system"))
		{
			system = xmlGetNumericAttribute(transponder, "system", 0);
			
			if (system == 0)
		    		feparams.delsys = DVB_T;
		    	else //if (system == 1)
		    	{
		    		feparams.delsys = DVB_T2;
		    		feparams.plp_id = xmlGetNumericAttribute(transponder, "plp_id", 0);
		    	}
            	}
            	else
            	{
            		feparams.delsys = DVB_T2;
            		feparams.plp_id = xmlGetNumericAttribute(transponder, "plp_id", 0);
            	}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		feparams.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.polarization = xmlGetNumericAttribute(transponder, "polarization", 0);
		system = xmlGetNumericAttribute(transponder, "system", 0);
		feparams.modulation = (fe_modulation_t)xmlGetNumericAttribute(transponder, "modulation", 0); 
		xml_fec = xmlGetNumericAttribute(transponder, "fec_inner", 0); // S_QPSK + S2_QPSK

		xml_fec = CFrontend::getCodeRate(xml_fec, system);

		if(feparams.modulation == QAM_32)		// S2_8PSK
			xml_fec += 9;

		feparams.fec_inner = (fe_code_rate_t) xml_fec;
		
		if (system == 0)
            		feparams.delsys = DVB_S;
            	else if (system == 1)
            		feparams.delsys = DVB_S2;
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_A)
#else
	else if(fe->getInfo()->type == FE_ATSC)
#endif
	{
	}

	// read network information table
	fake_tid++; fake_nid++;

	addToScan(CREATE_TRANSPONDER_ID(freq, satellitePosition, fake_nid, fake_tid), &feparams, false, fe);

	return true;
}

bool CZapit::scanProvider(xmlNodePtr search, t_satellite_position satellitePosition, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanProvider:\n");
	
	xmlNodePtr tps = NULL;
	found_transponders = 0;
	processed_transponders = 0;

	TIMER_START();
	
	sat_iterator_t sit = satellitePositions.find(satellitePosition);

	if(sit == satellitePositions.end()) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::scanProvider: WARNING satellite position %d not found!\n", satellitePosition);
		
		return false;
	}

	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, &processed_transponders, sizeof(processed_transponders));
	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SATELLITE, sit->second.name.c_str(), sit->second.name.size() + 1);
	
	tps = search->xmlChildrenNode;

	// read all transponders
	while ((tps = xmlGetNextOccurence(tps, "transponder")) != NULL) 
	{
		if(abort_scan)
			return false;

		scanTransponder(tps, satellitePosition, fe);

		// next transponder
		tps = tps->xmlNextNode;
	}
	
	eventServer->sendEvent( NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS, &found_transponders, sizeof(found_transponders));

	// start scanning
	getSDTS(satellitePosition, fe);

	//
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
						dprintf(DEBUG_INFO, "CZapit::scanProvider: setting service_type of channel_id: 0x%llx %s from %02x to %02x", stI->first, scI->second.getName().c_str(), scI->second.getServiceType(), stI->second);
						
						scI->second.setServiceType(stI->second);
						break;
				}
			}
		}
	}
	
	TIMER_STOP("CZapit::scanProvider: scanning took");
	
	return true;
}

//
bool CZapit::tuneTP(transponder TP, CFrontend* fe)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::tuneTP: fe(%d:%d)\n", fe->feadapter, fe->fenumber);
	
	bool ret = false;
	
	//		
//	initTuner(fe);
			
	// satname
	const char *name = scanProviders.size() > 0  ? scanProviders.begin()->second.c_str() : "unknown";
			
	t_satellite_position satellitePosition = scanProviders.begin()->first;
	
	dprintf(DEBUG_NORMAL, "CZapit::tuneTP: (fe:%d:%d delsys:0x%x) satname:%s satpos:%d (TP.delsys:0x%x)\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), name, satellitePosition, TP.feparams.delsys);
	
	// setinput
	fe->setInput(satellitePosition, TP.feparams.frequency, TP.feparams.polarization);
	
	// drivetosatpos
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	if (fe->getInfo()->type == FE_QPSK)
#endif
		fe->driveToSatellitePosition(satellitePosition);
		
	// tuneFreq
	ret = fe->tuneFrequency(&TP.feparams);
			
	// set retune flag
	retune = true;
	
	return ret;
}

//
bool CZapit::scanTP(commandScanTP &msg)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanTP fe:(%d:%d) scanmode:%d\n", msg.fe->feadapter, msg.fe->fenumber, msg.scanmode);
	
	bool ret = true;
	CPmt pmt;
	
	CZapit::stopPlayBack();
				
	// stop update pmt filter
	pmt.pmt_stop_update_filter(&pmt_update_fd);
	
	scan_runs = 1;
	
	if (pthread_create(&scan_thread, 0, scanTransponderThread, (void *) &msg)) 
	{
		dprintf(DEBUG_INFO, "CZapit::scanTP: pthread_create\n");
		scan_runs = 0;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED);
		
		ret = false;
	} 
			
	retune = true;
	
	return ret;
}

void CZapit::setScanSatelliteList( ScanSatelliteList &satelliteList )
{
	scanProviders.clear();
	
	for (uint32_t i = 0; i < satelliteList.size(); i++)
	{
		commandSetScanSatelliteList sat = satelliteList[i];
		
		scanProviders[sat.position] = sat.satName;
	}
}

void CZapit::setScanType(const scanType mode)
{
	_scanType = mode;
}

void CZapit::setScanBouquetMode(const bouquetMode mode)
{
	_bouquetMode = mode;
}

//
bool CZapit::startScan(commandScanProvider &msg)
{		
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::startScan: fe(%d:%d) scanmode: %d\n", msg.fe->feadapter, msg.fe->fenumber, msg.scanmode);
	
	bool ret = true;
	CPmt pmt;
	
	scan_runs = 1;
	
	//stop playback
	stopPlayBack();
	
	// stop pmt update filter
    	pmt.pmt_stop_update_filter(&pmt_update_fd);	

	if (pthread_create(&scan_thread, 0, scanThread, (void *) &msg)) 
	{
		dprintf(DEBUG_INFO, "CZapit::startScan: pthread_create failed\n");
		scan_runs = 0;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED);
		
		ret = false;
	}
			
	retune = true;
	
	return ret;
}

bool CZapit::stopScan()
{
	dprintf(DEBUG_NORMAL, "CZapit::stopScan:\n");
	
	if(scan_runs) 
	{
		abort_scan = 1;
		pthread_join(scan_thread, NULL);
		abort_scan = 0;
		scan_runs = 0;
	}
	
	return scan_runs;
}		

//
void * CZapit::scanThread(void * data)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	CZapit::commandScanProvider params = *(CZapit::commandScanProvider*)data;
	
	int mode = params.scanmode;
	CFrontend* fe = params.fe;
	
	scan_list_iterator_t spI;
	char providerName[80] = "";
	char *frontendType;
	processed_transponders = 0;
	failed_transponders = 0;
	found_transponders = 0;
 	found_tv_chans = 0;
 	found_radio_chans = 0;
 	found_data_chans = 0;
	found_channels = 0;
	curr_sat = 0;
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();
	scanBouquets.clear();

	scanmode = mode & 0xFF;	// NIT (0) or fast (1)
	scan_sat_mode = mode & 0xFF00; 	// single = 0, all = 1

	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanThread: fe(%d:%d) scan mode %s, satellites %s\n", fe->feadapter, fe->fenumber, scanmode ? "fast" : "NIT", scan_sat_mode ? "all" : "single");

	CZapit::getInstance()->fake_tid = CZapit::getInstance()->fake_nid = 0;
	
	//
	xmlDocPtr scanInputParser = NULL;

#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
	{
		frontendType = (char *) "cable";
		scanInputParser = parseXmlFile(CABLES_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		frontendType = (char *) "terrestrial";
		scanInputParser = parseXmlFile(TERRESTRIALS_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		frontendType = (char *) "sat";
		scanInputParser = parseXmlFile(SATELLITES_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
	{
		frontendType = (char *) "atsc";
		scanInputParser = parseXmlFile(ATSC_XML);
	}
	
	// get provider position and name
	if (scanInputParser)
	{
		xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;

		// read all sat or cable sections
		while ( (search = xmlGetNextOccurence(search, frontendType)) != NULL ) 
		{
			t_satellite_position position = xmlGetSignedNumericAttribute(search, "position", 10);
		
#if HAVE_DVB_API_VERSION >= 5
    			if ( (fe->getForcedDelSys() == DVB_C) || (fe->getForcedDelSys() == DVB_T) || (fe->getForcedDelSys() == DVB_T2) || (fe->getForcedDelSys() == DVB_A) )
#else
			if( (fe->getInfo()->type == FE_QAM) || (fe->getInfo()->type == FE_OFDM) || (fe->getInfo()->type == FE_ATSC) )
#endif
			{
				strcpy(providerName, xmlGetAttribute(search, const_cast<char*>("name")));
				
				for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
				{
					if (!strcmp(spI->second.c_str(), providerName))
					{
						// position needed because multi tuner if pos == 0 scan_provider() will abort
						position = spI->first;

						break;
					}
				}
			} 
			else //sat
			{
				for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
				{
					if(spI->first == position)
						break;
				}
			}

			// provider is not wanted - jump to the next one
			if (spI != scanProviders.end()) 
			{
				// provider name
				strcpy(providerName, xmlGetAttribute(search,  "name"));

				// increase sat counter
				curr_sat++;

				scantransponders.clear();
				scanedtransponders.clear();
				nittransponders.clear();

				dprintf(DEBUG_INFO, "CZapit::scanThread: scanning %s at %d bouquetMode %d\n", providerName, position, CZapit::getInstance()->_bouquetMode);
				
				// this invoke addToScan	
				if ( !CZapit::getInstance()->scanProvider(search, position, fe) )
				{
					found_channels = 0;
					break;
				}
						
				if(abort_scan) 
				{
					found_channels = 0;
					break;
				}

				if (scanBouquets.size())
				{
					CZapit::getInstance()->saveScanBouquets(CZapit::getInstance()->_bouquetMode, providerName);
				}
				
				scanBouquets.clear();
			}

			// go to next satellite
			search = search->xmlNextNode;
		}
		
		xmlFreeDoc(scanInputParser);
		scanInputParser = NULL;
	}

	// report status
	dprintf(DEBUG_NORMAL, "CZapit::scanThread: found %d transponders (%d failed) and %d channels\n", found_transponders, failed_transponders, found_channels);

	if (found_channels) 
	{
		CZapit::getInstance()->saveServices(true);
	        CZapit::getInstance()->saveZapitBouquets();
	        //
	        CZapit::getInstance()->clearAll();
		CZapit::getInstance()->loadBouquets();
		CZapit::getInstance()->renumServices();
		
		dprintf(DEBUG_INFO, "CZapit::scanThread: save bouquets done\n");
		
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_COMPLETE);
		
		scanBouquets.clear();
		
		eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED);
	} 
	else 
	{
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED);
		
		scanBouquets.clear();
	}

	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	pthread_exit(NULL);
}

//
void * CZapit::scanTransponderThread(void * data)
{
	dprintf(DEBUG_NORMAL, ANSI_BLUE "CZapit::scanTransponderThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	CZapit::commandScanTP params = *(CZapit::commandScanTP*)data;
	
	transponder * TP = &params.TP;
	scanmode = params.scanmode;
	CFrontend* fe = params.fe;
	
	char providerName[32] = "";
	t_satellite_position satellitePosition = 0;

	prov_found = 0;
        found_transponders = 0;
        found_channels = 0;
        processed_transponders = 0;
        found_tv_chans = 0;
        found_radio_chans = 0;
        found_data_chans = 0;
	CZapit::getInstance()->fake_tid = CZapit::getInstance()->fake_nid = 0;
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();
	scanBouquets.clear();

	// provider name
	strcpy(providerName, scanProviders.size() > 0 ? scanProviders.begin()->second.c_str() : "unknown provider");

	// satpos
	satellitePosition = scanProviders.begin()->first;
	
	dprintf(DEBUG_NORMAL, "CZapit::scanTransponderThread: (fe:%d:%d delsys:0x%x) scanning sat: %s position: %d fe(%d)\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), providerName, satellitePosition);
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SATELLITE, providerName, strlen(providerName) + 1);

	//
	freq_id_t freq;

#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
		freq = TP->feparams.frequency/100;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_S || fe->getForcedDelSys() == DVB_S2)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif
		freq = TP->feparams.frequency/1000;
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == DVB_T || fe->getForcedDelSys() == DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
		freq = TP->feparams.frequency/1000000;

	// add TP to scan
	CZapit::getInstance()->fake_tid++; 
	CZapit::getInstance()->fake_nid++;

	CZapit::getInstance()->addToScan(CREATE_TRANSPONDER_ID(freq, satellitePosition, CZapit::getInstance()->fake_nid, CZapit::getInstance()->fake_tid), &TP->feparams, false, fe);

	// scanSDTS
	if (!CZapit::getInstance()->getSDTS(satellitePosition, fe))
		found_channels = 0;

	if(abort_scan)
		found_channels = 0;

	if(found_channels) 
	{
		CZapit::getInstance()->saveServices(true);
		CZapit::getInstance()->saveScanBouquets(CZapit::getInstance()->_bouquetMode, providerName);
	        CZapit::getInstance()->saveZapitBouquets();
	        //
	        CZapit::getInstance()->clearAll();
		CZapit::getInstance()->loadBouquets();
		CZapit::getInstance()->renumServices();
		
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_COMPLETE);
		
		scanBouquets.clear();
		
		eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED);
	} 
	else 
	{
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED);
		
		scanBouquets.clear();
	}
	
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	pthread_exit(NULL);
}

//// channelManager
// parse transponder from services.xml
void CZapit::parseTransponders(xmlNodePtr node, t_satellite_position satellitePosition, fe_type_t frontendType)
{
	dprintf(DEBUG_INFO, "CZapit::parseTransponders:\n");

	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	FrontendParameters feparams;
	freq_id_t freq;
	tcnt = 0;

	memset(&feparams, 0, sizeof(FrontendParameters));

	// read all transponders
	while ((node = xmlGetNextOccurence(node, "TS")) != NULL) 
	{
		// common
		transport_stream_id = xmlGetNumericAttribute(node, "id", 16);
		original_network_id = xmlGetNumericAttribute(node, "on", 16);
		feparams.frequency = xmlGetNumericAttribute(node, "frq", 0);
		feparams.inversion = (fe_spectral_inversion) xmlGetNumericAttribute(node, "inv", 0);

		// DVB-C
		if (frontendType == FE_QAM)
		{
			feparams.symbol_rate = xmlGetNumericAttribute(node, "sr", 0);
			feparams.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(node, "fec", 0);
			feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(node, "mod", 0);
			feparams.delsys = DVB_C;
		}
		// DVB-T/T2
		else if (frontendType == FE_OFDM)
		{
			feparams.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(node, "bw", 0);
			feparams.code_rate_HP = (fe_code_rate_t) xmlGetNumericAttribute(node, "hp", 0);
			feparams.code_rate_LP = (fe_code_rate_t) xmlGetNumericAttribute(node, "lp", 0);
			feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(node, "con", 0);
			feparams.transmission_mode = (fe_transmit_mode_t) xmlGetNumericAttribute(node, "tm", 0);
			feparams.guard_interval = (fe_guard_interval_t) xmlGetNumericAttribute(node, "gi", 0);
			feparams.hierarchy_information = (fe_hierarchy_t) xmlGetNumericAttribute(node, "hi", 0);
			feparams.plp_id = (unsigned int) xmlGetNumericAttribute(node, "pli", 0);
			feparams.delsys = (uint32_t) xmlGetNumericAttribute(node, "sys", 0);
		}
		// DVB-S/S2
		else if (frontendType == FE_QPSK)
		{
			feparams.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(node, "fec", 0);
			feparams.symbol_rate = xmlGetNumericAttribute(node, "sr", 0);
			feparams.polarization = xmlGetNumericAttribute(node, "pol", 0);
			feparams.delsys = (uint32_t) xmlGetNumericAttribute(node, "sys", 0);

            		// ???
			if(feparams.symbol_rate < 50000) 
				feparams.symbol_rate = feparams.symbol_rate * 1000;
			
			// ???
			if(feparams.frequency < 20000) 
				feparams.frequency = feparams.frequency*1000;
		}

		if (frontendType == FE_QAM)
			freq = feparams.frequency/100;
		else if (frontendType == FE_QPSK)
			freq = feparams.frequency/1000;
		else if (frontendType == FE_OFDM || frontendType == FE_ATSC)
			freq = feparams.frequency/100000;

		// add current transponder to TP list
		transponder_id_t tid = CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id);

		std::pair<std::map<transponder_id_t, transponder>::iterator, bool> ret;

		ret = transponders.insert(std::pair<transponder_id_t, transponder> ( tid, transponder(transport_stream_id, original_network_id, feparams)));
		
		if (ret.second == false)
			printf("CZapit::parseTransponders: duplicate transponder id 0x%llx freq %d\n", tid, feparams.frequency);

		// read channels that belong to the current transponder
		parseChannels(node->xmlChildrenNode, transport_stream_id, original_network_id, satellitePosition, freq);

		// hop to next transponder
		node = node->xmlNextNode;
	}

	return;
}

void CZapit::parseChannels(xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq)
{
	dprintf(DEBUG_DEBUG, "CZapit::parseChannels:\n");

	t_service_id service_id;
	std::string  name;
	uint8_t service_type;
	unsigned short vpid, apid, pcrpid, pmtpid, txpid, vtype, scrambled;
	std::string desc = "";
	desc += "Preset";
	t_channel_id chid;
	int dummy;
	int * have_ptr = &dummy;

	sat_iterator_t sit = satellitePositions.find(satellitePosition);

	if(sit != satellitePositions.end())
		have_ptr = &sit->second.have_channels;

	while ((node = xmlGetNextOccurence(node, "S")) != NULL) 
	{
		*have_ptr = 1;
		service_id = xmlGetNumericAttribute(node, "i", 16);
		name = xmlGetAttribute(node, "n");
		service_type = xmlGetNumericAttribute(node, "t", 16);
		vpid = xmlGetNumericAttribute(node, "v", 16);
		apid = xmlGetNumericAttribute(node, "a", 16);
		pcrpid = xmlGetNumericAttribute(node, "p", 16);
		pmtpid = xmlGetNumericAttribute(node, "pmt", 16);
		txpid = xmlGetNumericAttribute(node, "tx", 16);
		vtype = xmlGetNumericAttribute(node, "vt", 16);
		scrambled = xmlGetNumericAttribute(node, "s", 16);

		chid = CREATE_CHANNEL_ID;
		char *ptr = xmlGetAttribute(node, "action");
		bool remove = ptr ? (!strcmp(ptr, "remove") || !strcmp(ptr, "replace")) : false;
		bool add    = ptr ? (!strcmp(ptr, "add")    || !strcmp(ptr, "replace")) : true;

		if (remove) 
		{
			int result = allchans.erase(chid);
			dprintf(DEBUG_INFO, "CZapit::parseChannels: %s '%s' (sid=0x%x): %s", add ? "replacing" : "removing", name.c_str(), service_id, result ? "succeded.\n" : "FAILED!\n");
		}

		if(!add) 
		{
			node = node->xmlNextNode;
			continue;
		}

		std::map<t_channel_id, audio_map_set_t>::iterator audio_map_it;
		audio_map_it = audio_map.find(chid);
		
		if((audio_map_it != audio_map.end()) && (audio_map_it->second.apid != 0)) 
		{
			apid = audio_map_it->second.apid;
		}

		// insert channels
		std::pair<std::map<t_channel_id, CZapitChannel>::iterator, bool> ret;

		ret = allchans.insert(std::pair<t_channel_id, CZapitChannel> (chid, CZapitChannel( name, 
												     service_id, 
												     transport_stream_id,
												     original_network_id, 
												     service_type, 
												     satellitePosition, 
												     freq)));

		if(ret.second == false) 
		{
			dprintf(DEBUG_INFO, "CZapit::parseChannels: duplicate channel %s id 0x%llx freq %d (old %s at %d)\n", name.c_str(), chid, freq, ret.first->second.getName().c_str(), ret.first->second.getFreqId());
		} 
		else 
		{
			scnt++;
			tallchans_iterator cit1 = ret.first;

			cit1->second.scrambled = scrambled;
			service_type = cit1->second.getServiceType();

			if(pmtpid != 0 && (((service_type == ST_DIGITAL_RADIO_SOUND_SERVICE) && (apid > 0)) || ( (service_type == ST_DIGITAL_TELEVISION_SERVICE)  && (vpid > 0) && (apid > 0))) ) 
			{			
				cit1->second.setVideoPid(vpid);
				cit1->second.setAudioPid(apid);
				cit1->second.setPcrPid(pcrpid);
				cit1->second.setPmtPid(pmtpid);
				cit1->second.setTeletextPid(txpid);
				cit1->second.setPidsFlag();
				cit1->second.videoType = vtype;
			}
		}

		node = node->xmlNextNode;
	}

	return;
}

// scan services.xml
void CZapit::findTransponder(xmlNodePtr search)
{
	dprintf(DEBUG_INFO, "CZapit::findTransponder:\n");

	t_satellite_position satellitePosition = 0;
    	fe_type_t type = FE_QPSK;
	newtpid = 0xC000;
	
	while (search) 
	{
		if ( !(strcmp(xmlGetName(search), "cable")) && have_c)
		{
			type = FE_QAM;
			
			for (sat_iterator_t spos_it = satellitePositions.begin(); spos_it != satellitePositions.end(); spos_it++) 
			{
				if( !strcmp(spos_it->second.name.c_str(), xmlGetAttribute(search, "name")) ) 
				{
					satellitePosition = spos_it->first;
					break;
				}
			}
			
			dprintf(DEBUG_INFO, "CZapit::findTransponder: going to parse dvb-%c provider %s\n", xmlGetName(search)[0], xmlGetAttribute(search, "name"));
		}
		else if ( !(strcmp(xmlGetName(search), "terrestrial")) && have_t)
		{
			type = FE_OFDM;
			
			for (sat_iterator_t spos_it = satellitePositions.begin(); spos_it != satellitePositions.end(); spos_it++) 
			{
				if( !strcmp(spos_it->second.name.c_str(), xmlGetAttribute(search, "name")) ) 
				{
					satellitePosition = spos_it->first;
					break;
				}
			}
			
			dprintf(DEBUG_INFO, "CZapit::findTransponder: going to parse dvb-%c provider %s\n", xmlGetName(search)[0], xmlGetAttribute(search, "name"));
		}
		else if ( !(strcmp(xmlGetName(search), "atsc")) && have_a)
		{
			type = FE_ATSC;
			
			for (sat_iterator_t spos_it = satellitePositions.begin(); spos_it != satellitePositions.end(); spos_it++) 
			{
				if( !strcmp(spos_it->second.name.c_str(), xmlGetAttribute(search, "name")) ) 
				{
					satellitePosition = spos_it->first;
					break;
				}
			}
			
			dprintf(DEBUG_INFO, "CZapit::findTransponder: going to parse dvb-%c provider %s\n", xmlGetName(search)[0], xmlGetAttribute(search, "name"));
		}
		else if ( !(strcmp(xmlGetName(search), "sat")) && have_s) 
		{
			type = FE_QPSK;
			satellitePosition = xmlGetSignedNumericAttribute(search, "position", 10);
			
			dprintf(DEBUG_INFO, "CZapit::findTransponder: going to parse dvb-%c provider %s position %d\n", xmlGetName(search)[0], xmlGetAttribute(search, "name"), satellitePosition);
		}
		else // unknow
		{
			search = search->xmlNextNode;
			continue;
		}
		
		// parseTP
		parseTransponders(search->xmlChildrenNode, satellitePosition, type);

		newfound++;
		
		search = search->xmlNextNode;
	}
}

// parse sat transponder from satellites/cables/terrestrials.xml/atsc.xml
void CZapit::parseSatTransponders(fe_type_t frontendType, xmlNodePtr search, t_satellite_position satellitePosition)
{
	dprintf(DEBUG_DEBUG, "CZapit::parseSatTransponders:\n");

	uint8_t system = 0;
	int xml_fec;
	FrontendParameters feparams;
	fake_tid = 0;
    	fake_nid = 0;

	xmlNodePtr tps = search->xmlChildrenNode;

	while ((tps = xmlGetNextOccurence(tps, "transponder")) != NULL) 
	{
		memset(&feparams, 0, sizeof(FrontendParameters));

		freq_id_t freq;

		// frequency 
		if (frontendType == FE_OFDM)
			feparams.frequency = xmlGetNumericAttribute(tps, "centre_frequency", 0);
		else
			feparams.frequency = xmlGetNumericAttribute(tps, "frequency", 0);

		// inversion
		feparams.inversion = (fe_spectral_inversion_t)INVERSION_AUTO;

		if (frontendType == FE_QAM) 		//DVB-C
		{
			feparams.symbol_rate = xmlGetNumericAttribute(tps, "symbol_rate", 0);
			feparams.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(tps, "fec_inner", 0);
			feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(tps, "modulation", 0);
            		feparams.delsys = DVB_C;
		}
		else if (frontendType == FE_OFDM)	//DVB-T/T2
		{
			feparams.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(tps, "bandwidth", 0);
			feparams.code_rate_HP = (fe_code_rate_t) xmlGetNumericAttribute(tps, "code_rate_hp", 0);
			feparams.code_rate_LP = (fe_code_rate_t) xmlGetNumericAttribute(tps, "code_rate_lp", 0);
			feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(tps, "constellation", 0);
			feparams.transmission_mode = (fe_transmit_mode_t) xmlGetNumericAttribute(tps, "transmission_mode", 0);
			feparams.guard_interval = (fe_guard_interval_t) xmlGetNumericAttribute(tps, "guard_interval", 4);
			feparams.hierarchy_information = (fe_hierarchy_t) xmlGetNumericAttribute(tps, "hierarchy_information", 0);
			
			if (xmlGetAttribute(tps, (char *)"inversion"))
				feparams.inversion = (fe_spectral_inversion_t)xmlGetNumericAttribute(tps, "inversion", 0);

			if (xmlGetAttribute(tps, (char *)"system"))
			{
		    		system = xmlGetNumericAttribute(tps, "system", 0);

		    		if (system == 0)
		    			feparams.delsys = DVB_T;
		    		else //if (system == 1)
		    		{
		    			feparams.delsys = DVB_T2;
		    			feparams.plp_id = (unsigned int) xmlGetNumericAttribute(tps, "plp_id", 0);
		    		}
            		}
            		else
            		{
            			feparams.delsys = DVB_T2;
            			feparams.plp_id = (unsigned int) xmlGetNumericAttribute(tps, "plp_id", 0);
            		}
		}
		else if (frontendType == FE_QPSK) 	//DVB-S/S2/S2X
		{
			feparams.symbol_rate = xmlGetNumericAttribute(tps, "symbol_rate", 0);
			feparams.polarization = xmlGetNumericAttribute(tps, "polarization", 0);
			system = xmlGetNumericAttribute(tps, "system", 0);
			feparams.modulation = (fe_modulation_t)xmlGetNumericAttribute(tps, "modulation", 0);
			xml_fec = xmlGetNumericAttribute(tps, "fec_inner", 0);

			xml_fec = CFrontend::getCodeRate(xml_fec, system);

			// DVB-S2
			if(feparams.modulation == QAM_32)
				xml_fec += 9;

			feparams.fec_inner = (fe_code_rate_t)xml_fec;

            		if (system == 0)
            			feparams.delsys = DVB_S;
            		else if (system == 1)
            			feparams.delsys = DVB_S2;
		}
		else if (frontendType == FE_ATSC)
		{
		    feparams.modulation = (fe_modulation_t) xmlGetNumericAttribute(tps, "modulation", 0);
		    feparams.delsys = DVB_A;
		}
		
		if (frontendType == FE_QAM) 
			freq = feparams.frequency/100;
		else if(frontendType == FE_QPSK)
			freq = feparams.frequency/1000;
		else if((frontendType == FE_OFDM) || (frontendType == FE_ATSC))
			freq = feparams.frequency/100000;
			
		transponder_id_t tid = CREATE_TRANSPONDER_ID(freq, satellitePosition, fake_nid, fake_tid);

//		polarization &= 7;
		
		// insert TPs list
		select_transponders.insert( std::pair<transponder_id_t, transponder> (tid, transponder(fake_tid, fake_nid, feparams)));
		
		fake_nid ++; 
		fake_tid ++;

		tps = tps->xmlNextNode;
	}
}

int CZapit::loadMotorPositions(void)
{
	dprintf(DEBUG_INFO, "CZapit::loadMotorPositions:\n");

	FILE *fd = NULL;
	char buffer[256] = "";
	t_satellite_position satellitePosition;
	int spos = 0, mpos = 0, diseqc = 0, uncom = 0, com = 0, usals = 0, inuse;
	int offH = 10600, offL = 9750, sw = 11700;

	if ((fd = fopen(SATCONFIG, "r"))) 
	{
		fgets(buffer, 255, fd);

		while(!feof(fd)) 
		{
			sscanf(buffer, "%d %d %d %d %d %d %d %d %d %d", &spos, &mpos, &diseqc, &com, &uncom, &offL, &offH, &sw, &inuse, &usals);

			satellitePosition = spos;
			sat_iterator_t sit = satellitePositions.find(satellitePosition);

			if(sit != satellitePositions.end()) 
			{
				sit->second.motor_position = mpos;
				sit->second.diseqc = diseqc;
				sit->second.commited = com;
				sit->second.uncommited = uncom;
				sit->second.lnbOffsetLow = offL;
				sit->second.lnbOffsetHigh = offH;
				sit->second.lnbSwitch = sw;
				sit->second.use_in_scan = inuse;
				sit->second.use_usals = usals;
			}
			fgets(buffer, 255, fd);
		}
		fclose(fd);
	}
	else
		printf("CZapit::loadMotorPositions: %s not found.\n", SATCONFIG);

	return 0;
}

void CZapit::saveMotorPositions()
{
	FILE * fd;
	sat_iterator_t sit;
	
	dprintf(DEBUG_INFO, "CZapit::saveMotorPositions: saving motor positions...\n");

	fd = fopen(SATCONFIG, "w");
	if(fd == NULL) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::saveMotorPositions: cannot open %s\n", SATCONFIG);
		return;
	}
	
	fprintf(fd, "# sat position, stored rotor, diseqc, commited, uncommited, low, high, switch, use in full scan, use usals\n");
	
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		if(sit->second.system == DVB_S)
			fprintf(fd, "%d %d %d %d %d %d %d %d %d %d\n", 
				sit->first, 
				sit->second.motor_position,
				sit->second.diseqc, 
				sit->second.commited, 
				sit->second.uncommited, 
				sit->second.lnbOffsetLow,
				sit->second.lnbOffsetHigh, 
				sit->second.lnbSwitch, 
				sit->second.use_in_scan, 
				sit->second.use_usals);
	
	}
	fdatasync(fileno(fd));
	fclose(fd);
}

void CZapit::initSat(t_satellite_position position)
{
	dprintf(DEBUG_DEBUG, "CZapit::initSat:\n");

	satellitePositions[position].position = 0;
	satellitePositions[position].diseqc = -1;
	satellitePositions[position].commited = -1;
	satellitePositions[position].uncommited = -1;
	satellitePositions[position].motor_position = 0;
	satellitePositions[position].diseqc_order = 0;
	satellitePositions[position].lnbOffsetLow = 9750;
	satellitePositions[position].lnbOffsetHigh = 10600;
	satellitePositions[position].lnbSwitch = 11700;
	satellitePositions[position].use_in_scan = 0;
	satellitePositions[position].use_usals = 0;
}

// load transponders from satellites/cables/terrestrial/atsc.xml
int CZapit::loadTransponders()
{
	bool satcleared = 0;
	scnt = 0;
	
	t_satellite_position position = 0; //first position

	dprintf(DEBUG_INFO, "CZapit::loadTransponders:\n");
	
	select_transponders.clear();
	fake_tid = 0;
    	fake_nid = 0;
	
	if(!satcleared)
		satellitePositions.clear();

	satcleared = 1;
	
	xmlDocPtr scanInputParser = NULL;
		
	if (have_s)
	{
		scanInputParser = parseXmlFile(SATELLITES_XML);
			
		if ( scanInputParser != NULL ) 
		{
			xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;

			while (search) 
			{
				if (!(strcmp(xmlGetName(search), "sat"))) 
				{
                    			// flags

					// position
					position = xmlGetSignedNumericAttribute(search, "position", 10);
					
					char * name = xmlGetAttribute(search, "name");

					if(satellitePositions.find(position) == satellitePositions.end()) 
					{
						initSat(position);
					}

					// name
					satellitePositions[position].name = name;
					
					// delsys
					satellitePositions[position].system = DVB_S;
				}
				
				// parse sat TP
				parseSatTransponders(FE_QPSK, search, position);
				
				position++;
				
				search = search->xmlNextNode;
			}
			
			xmlFreeDoc(scanInputParser);
			scanInputParser = NULL;
		}
	}
	
	if (have_c)
	{
		scanInputParser = parseXmlFile(CABLES_XML);
			
		if ( scanInputParser != NULL ) 
		{
			xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;

			while (search) 
			{
				if(!(strcmp(xmlGetName(search), "cable"))) 
				{
					char * name = xmlGetAttribute(search, "name");

					if(satellitePositions.find(position) == satellitePositions.end()) 
					{
						initSat(position);
					}

					// name
					satellitePositions[position].name = name;
					
					// delsys
					satellitePositions[position].system = DVB_C;
				}

				// parse sat TP
				parseSatTransponders(FE_QAM, search, position);
				
				position++;
				
				search = search->xmlNextNode;
			}
			
			xmlFreeDoc(scanInputParser);
			scanInputParser = NULL;
		}
	}
	
	if (have_t)
	{
		scanInputParser = parseXmlFile(TERRESTRIALS_XML);
			
		if ( scanInputParser != NULL ) 
		{
			xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;

			while (search) 
			{
				if(!(strcmp(xmlGetName(search), "terrestrial"))) 
				{
                    			// flags
                    			// countrycode

					char * name = xmlGetAttribute(search, "name");

					if(satellitePositions.find(position) == satellitePositions.end()) 
					{
						initSat(position);
					}

					// name
					satellitePositions[position].name = name;
					
					// delsys
					satellitePositions[position].system = DVB_T;
				}

				// parse sat TP
				parseSatTransponders(FE_OFDM, search, position);
				
				position++;
				
				search = search->xmlNextNode;
			}
			
			xmlFreeDoc(scanInputParser);
			scanInputParser = NULL;
		}
	}
	
	if (have_a)
	{
		scanInputParser = parseXmlFile(ATSC_XML);
			
		if ( scanInputParser != NULL ) 
		{
			xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;

			while (search) 
			{
				if(!(strcmp(xmlGetName(search), "atsc"))) 
				{
                    			// flags

					char * name = xmlGetAttribute(search, "name");

					if(satellitePositions.find(position) == satellitePositions.end()) 
					{
						initSat(position);
					}

					// name
					satellitePositions[position].name = name;
					
					// delsys
					satellitePositions[position].system = DVB_A;
				}

				// parse sat TP
				parseSatTransponders(FE_ATSC, search, position);
				
				position++;
				
				search = search->xmlNextNode;
			}
			
			xmlFreeDoc(scanInputParser);
			scanInputParser = NULL;
		}
	}
	
	return 0;
}	

// load services
int CZapit::loadServices(bool only_current)
{
	xmlDocPtr parser;
	scnt = 0;

	dprintf(DEBUG_INFO, "CZapit::loadServices:\n");

	if(only_current)
		goto do_current;

	// parse services.xml
	parser = parseXmlFile(SERVICES_XML);

	if (parser != NULL) 
	{
		xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;

		while (search) 
		{
			t_satellite_position position;
			
			if (!(strcmp(xmlGetName(search), "sat"))) 
			{
				// position
				position = xmlGetSignedNumericAttribute(search, "position", 10);
				char * name = xmlGetAttribute(search, "name");

				if(satellitePositions.find(position) == satellitePositions.end()) 
				{
					initSat(position);
				}
                
                		satellitePositions[position].name = name;
			}

			// jump to the next node
			search = search->xmlNextNode;
		}

		findTransponder( xmlDocGetRootElement(parser)->xmlChildrenNode );
		
		xmlFreeDoc(parser);
	}

	// load motor position
	for(int i = 0; i < CZapit::getInstance()->getFrontendCount(); i++)
	{
		if( CZapit::getInstance()->getFE(i)->getInfo()->type == FE_QPSK)
		{
			loadMotorPositions();
			break;
		}
	}

do_current:

	if (CZapit::getInstance()->scanSDT && (parser = parseXmlFile(CURRENTSERVICES_XML))) 
	{
		newfound = 0;
		
		findTransponder( xmlDocGetRootElement(parser)->xmlChildrenNode );
		
		xmlFreeDoc(parser);
		unlink(CURRENTSERVICES_XML);
		
		if(newfound)
			saveServices(true);
	}

	if(!only_current) 
	{
		parser = parseXmlFile(MYSERVICES_XML);
		
		if (parser != NULL) 
		{
			findTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
			
			xmlFreeDoc(parser);
		}
	}

	return 0;
}

void CZapit::saveServices(bool tocopy)
{
	dprintf(DEBUG_INFO, "CZapit::saveServices:\n");
	
	transponder_id_t tpid = 0;
	FILE * fd = 0;
	bool updated = 0;

	tallchans_iterator ccI;
	tallchans_iterator dI;
	transponder_list_t::iterator tI;
	char tpstr[256];
	bool tpdone = 0;
	bool satdone = 0;
	int processed = 0;
	sat_iterator_t spos_it;
	updated = 0;

	dprintf(DEBUG_INFO, "CZapit::saveServices: total channels: %d\n", allchans.size());
	
	fd = fopen(SERVICES_TMP, "w");
	if(!fd) 
	{
		perror(SERVICES_TMP);
		return;
	}

	// headers
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit api=\"3\">\n");
	
	// loop througth satpos
	for (spos_it = satellitePositions.begin(); spos_it != satellitePositions.end(); spos_it++) 
	{
		satdone = 0;

		// loop througth TPs
		for(tI = transponders.begin(); tI != transponders.end(); tI++) 
		{
			t_satellite_position satpos = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;
			tpdone = 0;

			if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
				satpos = -satpos;

			if(satpos != spos_it->first) 
			{
				dprintf(DEBUG_DEBUG, "CZapit::saveServices: Sat position %d not found !!\n", satpos);

				continue;
			}
			
			switch(spos_it->second.system)
			{
				case DVB_S:
				case DVB_S2:
				case DVB_S2X:
					sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" sr=\"%u\" fec=\"%hu\" pol=\"%hu\" sys=\"%hu\">\n",
							tI->second.transport_stream_id, tI->second.original_network_id,
							tI->second.feparams.frequency, tI->second.feparams.inversion,
							tI->second.feparams.symbol_rate, tI->second.feparams.fec_inner,
							tI->second.feparams.polarization,
							tI->second.feparams.delsys);
					break;

				case DVB_C:
					sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" sr=\"%u\" fec=\"%hu\" mod=\"%hu\">\n",
							tI->second.transport_stream_id, tI->second.original_network_id,
							tI->second.feparams.frequency, tI->second.feparams.inversion,
							tI->second.feparams.symbol_rate, tI->second.feparams.fec_inner,
							tI->second.feparams.modulation);
					break;

				case DVB_T:
				case DVB_T2:
				case DVB_DTMB:
					sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" bw=\"%hu\" hp=\"%hu\" lp=\"%hu\" con=\"%hu\" tm=\"%hu\" gi=\"%hu\" hi=\"%hu\" sys=\"%hu\">\n",
                                        tI->second.transport_stream_id, tI->second.original_network_id,
                                        tI->second.feparams.frequency, tI->second.feparams.inversion,
                                        tI->second.feparams.bandwidth, tI->second.feparams.code_rate_HP,
                                        tI->second.feparams.code_rate_LP, tI->second.feparams.modulation,tI->second.feparams.transmission_mode, tI->second.feparams.guard_interval, tI->second.feparams.hierarchy_information, 
                                        tI->second.feparams.delsys);
					break;

				default:
					break;
			}

			tpid = tI->first;
			
			// loop througth channels
			for (ccI = allchans.begin(); ccI != allchans.end(); ccI++) 
			{
				if(ccI->second.getTransponderId() == tpid) 
				{
					if(!satdone) 
					{
						switch(spos_it->second.system)
						{
							case DVB_S:
							{
								fprintf(fd, "\t<sat name=\"%s\" position=\"%hd\" diseqc=\"%hd\" uncommited=\"%hd\">\n",spos_it->second.name.c_str(), spos_it->first, spos_it->second.diseqc, spos_it->second.uncommited);
							}
							break;

							case DVB_C:
							{
								fprintf(fd, "\t<cable name=\"%s\">\n", spos_it->second.name.c_str());
							}
							break;

							case DVB_T:
							{
								fprintf(fd, "\t<terrestrial name=\"%s\">\n", spos_it->second.name.c_str());
							}
							break;

							default:
							break;
						}

						satdone = 1;
					}

					if(!tpdone) 
					{
						fprintf(fd, "%s", tpstr);
						tpdone = 1;
					}

					if(ccI->second.getPidsFlag()) 
					{
						fprintf(fd, "\t\t\t<S i=\"%04x\" n=\"%s\" v=\"%x\" a=\"%x\" p=\"%x\" pmt=\"%x\" tx=\"%x\" t=\"%x\" vt=\"%d\" s=\"%d\"/>\n",
								ccI->second.getServiceId(), 
								UTF8_to_UTF8XML(ccI->second.getName().c_str()).c_str(),
								ccI->second.getVideoPid(), 
								ccI->second.getPreAudioPid(),
								ccI->second.getPcrPid(), 
								ccI->second.getPmtPid(), 
								ccI->second.getTeletextPid(),
								ccI->second.getServiceType(true), 
								ccI->second.videoType, 
								ccI->second.scrambled);
					} 
					else 
					{
						fprintf(fd, "\t\t\t<S i=\"%04x\" n=\"%s\" t=\"%x\" s=\"%d\"/>\n",
								ccI->second.getServiceId(), 
								UTF8_to_UTF8XML(ccI->second.getName().c_str()).c_str(),
								ccI->second.getServiceType(true), 
								ccI->second.scrambled);
					}
					processed++;
				}
			}
			
			if(tpdone) 
                		fprintf(fd, "\t\t</TS>\n");
		}

		if(satdone) 
		{
			switch(spos_it->second.system)
			{
				case DVB_S:
					fprintf(fd, "\t</sat>\n");
					break;
					
				case DVB_C:
					fprintf(fd, "\t</cable>\n");
					break;
					
				case DVB_T:
					fprintf(fd, "\t</terrestrial>\n");
					break;
				default:
					break;
			}
		}
	}
	
	fprintf(fd, "</zapit>\n");
	fclose(fd);
	
	// create zapit ordner if missed
	struct stat statInfo;
	int res = stat(CONFIGDIR "/zapit", &statInfo);
	
	if (res == -1) 
	{
		if (errno == ENOENT) 
		{
			res = safe_mkdir((char *)CONFIGDIR "/zapit");

			if (res != 0) 
				perror("CZapit::saveServices: mkdir");
		} 
		else 
		{
			perror("CZapit::saveServices: stat");
		}
	} 

	if(tocopy) 
	{
		//
		CFileHelpers::getInstance()->copyFile(SERVICES_TMP, SERVICES_XML);
		unlink(SERVICES_TMP);
	}

	dprintf(DEBUG_INFO, "CZapit::saveServices: processed channels: %d\n", processed);
}

//
void CZapit::Start(Zapit_config zapitCfg)
{
	dprintf(DEBUG_NORMAL, "CZapit::Start\n");	
	
	//
	initFrontend();
	
	// init SH4 HW
#if defined (__sh__)
	if(getFrontendCount() > 1)
	{
		struct dmx_pes_filter_params p;
		int dmx = open("/dev/dvb/adapter0/demux0", O_RDWR );
		if (dmx < 0)
			printf("%s: ERROR open /dev/dvb/adapter0/demux0 (%m)\n", __func__);
		else
		{
			memset(&p, 0, sizeof(p));
			p.output = DMX_OUT_DECODER;
			p.input  = DMX_IN_FRONTEND;
			p.flags  = DMX_IMMEDIATE_START;
			p.pes_type = DMX_PES_VIDEO;
			ioctl(dmx, DMX_SET_PES_FILTER, &p);
			ioctl(dmx, DMX_STOP);
			close(dmx);
		}
	}
#endif
	
	// load frontend config
	loadFrontendConfig();
	
	// load configuration
	loadZapitSettings();

	// CI init	
	ci = cDvbCi::getInstance();

	// init cam
	cam0 = new CCam();
	cam1 = new CCam();	
	
	//globals
	scan_runs = 0;
	found_channels = 0;
	curr_sat = 0;
	
	// load channels / bouquets
	prepareChannels();
	
	//
	if (zapitCfg.saveLastChannel)
	{
		if (lastChannelMode == RADIO_MODE)
			setRadioMode();
		else if (lastChannelMode == TV_MODE)
			setTVMode();
	}
	else // start channel
	{
		// mode
		if (zapitCfg.lastchannelmode == RADIO_MODE)
			setRadioMode();
		else if (zapitCfg.lastchannelmode == TV_MODE)
			setTVMode();
		
		// live channel id
		if (currentMode & RADIO_MODE)
			live_channel_id = zapitCfg.startchannelradio_id;
		else if (currentMode & TV_MODE)
			live_channel_id = zapitCfg.startchanneltv_id;
			
		lastChannelTV = zapitCfg.startchanneltv_nr;
		lastChannelRadio = zapitCfg.startchannelradio_nr;
	}

	//create sdt thread
	pthread_create(&tsdt, NULL, sdtThread, (void *) NULL);

	//get live channel
	tallchans_iterator cit;
	cit = allchans.find(live_channel_id);

	if(cit != allchans.end())
		live_channel = &(cit->second);
	
	//wakeup from standby and zap it to live channel
	leaveStandby(); 
	
	//create pmt update filter thread
	pthread_create(&tpmt, NULL, updatePMTFilter, (void *) NULL);
	
	dprintf(DEBUG_NORMAL, "CZapit::Start: init succeeded\n");
}

//
void CZapit::Stop()
{
	dprintf(DEBUG_NORMAL, "CZapit::Stop:\n");

	//save audio map
	if(live_channel)
		saveChannelPids(live_channel);
	
	// save setting
	saveZapitSettings(true, true);
	
	// stop playback (stop capmt)
	stopPlayBack();
	
	// stop std thread
	pthread_cancel(tsdt);
	pthread_join(tsdt, NULL);
	
	// stop pmt update filter thread
	//pthread_cancel(tpmt);
	//pthread_join(tpmt, NULL);

	//if (pmtDemux)
	//	delete pmtDemux;

	//close frontend	
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++)
		delete it->second;
}

