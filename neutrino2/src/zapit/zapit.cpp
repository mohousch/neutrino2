/*
 * $Id: zapit.cpp,v 1.3 2013/08/18 11:23:30 mohousch Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrinoMessages.h>

// tuxbox headers
#include <configfile.h>
#include <connection/basicserver.h>

// system
#include <system/debug.h>
#include <system/settings.h>

//
#include <driver/encoding.h>

// zapit headers
#include <zapit/cam.h>
#include <zapit/getservices.h>
#include <zapit/pat.h>
#include <zapit/pmt.h>
#include <zapit/scan.h>
#include <zapit/settings.h>
#include <zapit/zapit.h>
#include <zapit/satconfig.h>
#include <zapit/frontend_c.h>
#include <zapit/bouquets.h>

// libxmltree
#include <xmlinterface.h>

// libcoolstream
#include <dmx_cs.h>
#include <audio_cs.h>
#include <video_cs.h>
#if defined (ENABLE_CI)
#include <ca_ci.h>
#endif

#include <playback_cs.h>


// scan
extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern CBouquetManager *scanBouquetManager;
extern uint32_t failed_transponders;
extern uint32_t  found_tv_chans;
extern uint32_t  found_radio_chans;
extern uint32_t  found_data_chans;
extern std::map <transponder_id_t, transponder> scantransponders;		// TP list to scan
extern std::map <transponder_id_t, transponder> scanedtransponders;		// global TP list for current scan
extern std::map <transponder_id_t, transponder> nittransponders;
extern int scanmode;								//
extern int scan_sat_mode;
extern uint32_t fake_tid, fake_nid;
extern int prov_found;
extern int found_transponders;		// defined in descriptors.cpp
extern int processed_transponders;	// defined in scan.cpp
extern int found_channels;		// defined in descriptors.cpp
extern short curr_sat;			// defined in scan.cpp
extern short scan_runs;			// defined in scan.cpp
extern short abort_scan;		// defined in scan.cpp
CZapit::bouquetMode _bouquetMode = CZapit::BM_UPDATEBOUQUETS;
CZapit::scanType _scanType = CZapit::ST_TVRADIO;
scan_list_t scanProviders;

// opengl liveplayback
#if defined (USE_OPENGL)
int startOpenGLplayback();
void stopOpenGLplayback();
#endif

// webtv
extern cPlayback *playback;

// ci
#if defined (ENABLE_CI)
cCA *ci = NULL;
#endif

// audio conf
map<t_channel_id, audio_map_set_t> audio_map;
map<t_channel_id, audio_map_set_t>::iterator audio_map_it;
unsigned int volume_left = 100;
unsigned int volume_right = 100;
int audio_mode = 0;
int def_audio_mode = 0;

// volume percent conf
#define VOLUME_DEFAULT_PCM 0
#define VOLUME_DEFAULT_AC3 25
typedef std::pair<int, int> pid_pair_t;
typedef std::pair<t_channel_id, pid_pair_t> volume_pair_t;
typedef std::multimap<t_channel_id, pid_pair_t> volume_map_t;
volume_map_t vol_map;
typedef volume_map_t::iterator volume_map_iterator_t;
typedef std::pair<volume_map_iterator_t, volume_map_iterator_t> volume_map_range_t;

int volume_percent = 0;
extern int current_volume;
extern int current_muted;

// live/record channel id
t_channel_id live_channel_id = 0;
t_channel_id rec_channel_id = 0;

bool firstzap = true;
bool playing = false;
bool g_list_changed = false; 		// flag to indicate, allchans was changed

// SDT
int scanSDT = 0;
bool sdt_wakeup = false;
sdt_tp_t sdt_tp;			// defined in getservices.h

// the conditional access module
CCam * cam0 = NULL;
CCam * cam1 = NULL;

// the configuration file
CConfigFile config(',', true);

// the event server
extern CEventServer *eventServer;

// the current channel
CZapitChannel * live_channel = NULL;

// record channel
CZapitChannel * rec_channel = NULL;

// bouquet manager
CBouquetManager * g_bouquetManager = NULL;

// Audio/Video Decoder
extern cVideo * videoDecoder;			// defined in video_cs.pp (libdvbapi)
extern cAudio * audioDecoder;			// defined in audio_cs.pp (libdvbapi)

// Demuxes
extern cDemux * audioDemux;			// defined in dmx_cs.pp (libdvbapi)
extern cDemux * videoDemux;			// defined in dmx_cs.pp (libdvbapi)
cDemux * pcrDemux = NULL;			// defined in dmx_cs.pp (libdvbapi)
extern cDemux * pmtDemux;			// defined in pmt.cpp

// zapit mode
enum {
	TV_MODE = 0x01,
	RADIO_MODE = 0x02,
	RECORD_MODE = 0x04,
};

int currentMode = 1;
bool playbackStopForced = false;
bool avDecoderOpen = false;

// list of near video on demand
tallchans nvodchannels;         	// tallchans defined in "bouquets.h"
bool current_is_nvod = false;

// list of all channels (services)
tallchans allchans;             	// tallchans defined in "bouquets.h"
tallchans curchans;             	// tallchans defined in "bouquets.h"
transponder_list_t transponders;    	// from services.xml

//
bool standby = true;

// zapit config
bool saveLastChannel = true;
int lastChannelMode = TV_MODE;
uint32_t  lastChannelRadio = 0;
uint32_t  lastChannelTV = 0;
bool makeRemainingChannelsBouquet = false;

// pmt update filter
static int pmt_update_fd = -1;

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

// multi frontend stuff
int FrontendCount = 0;
fe_map_t femap;

// frontend config
CConfigFile fe_configfile(',', false);
CFrontend * live_fe = NULL;
CFrontend * record_fe = NULL;

//
bool retune = false;

bool have_s = false;
bool have_c = false;
bool have_t = false;
bool have_a = false;

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
				if (fe->getDelSysMasked() & DVB_S || fe->getDelSysMasked() & DVB_S2 || fe->getDelSysMasked() & DVB_S2X)
					have_s = true;
				if (fe->getDelSysMasked() & DVB_C)
					have_c = true;
				if (fe->getDelSysMasked() & DVB_T || fe->getDelSysMasked() & DVB_T2)
					have_t = true;
				if (fe->getDelSysMasked() & DVB_A)
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
				
				// set it to standby
				fe->Close();
				
				usleep(150000);
			}
			else
				delete fe;
		}
	}

#if defined (ENABLE_FAKE_TUNER)
    	for(j = 0; j < 4; j++)
	{
		fe = new CFrontend(j, 0); // adapter_num = 0
			
		if (j == 0)
		{
			fe->info.type = FE_QPSK;
			strcpy(fe->info.name, "Sat Fake Tuner");
			have_s = true;
		}
		else if (j == 1)
		{
			fe->info.type = FE_QAM;
			strcpy(fe->info.name, "Cable Fake Tuner");
			have_c = true;
		}
		else if(j == 2)
		{
			fe->info.type = FE_OFDM;
			strcpy(fe->info.name, "Terrestrial Fake Tuner");
			have_t = true;
		}
		else if (j == 3)
		{
			fe->info.type = FE_ATSC;
			strcpy(fe->info.name, "ATSC Fake Tuner");
			have_a = true;
		}

		index++;
		femap.insert(std::pair <unsigned short, CFrontend*> (index, fe));
	}
#endif

	FrontendCount = femap.size();
	
	dprintf(DEBUG_INFO, "CZapit::initFrontend: found %d frontends\n", femap.size());
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

void CZapit::setFEMode(fe_mode_t newmode, int feindex)
{
	// set mode
	getFE(feindex)->mode = newmode;
	
	// set not connected frontend to standby
	if( getFE(feindex)->mode == (fe_mode_t)FE_NOTCONNECTED )
		getFE(feindex)->Close();

	// set loop frontend as slave
	bool setslave = ( getFE(feindex)->mode == FE_LOOP );
	
	if(setslave)
	{
		dprintf(DEBUG_INFO, "CZapit::setFEMode: Frontend (%d,%d) as slave: %s\n", getFE(feindex)->feadapter, getFE(feindex)->fenumber, setslave ? "yes" : "no");
		getFE(feindex)->setMasterSlave(setslave);
	}
}

void CZapit::initTuner(CFrontend * fe)
{
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
		
		//
		//fe->changeDelSys(fe->forcedDelSys); //FIXME:???
	}
}

// compare polarization and band with fe values
bool CZapit::loopCanTune(CFrontend * fe, CZapitChannel * thischannel)
{
	if(fe->getInfo()->type != FE_QPSK)
		return true;

	if(fe->tuned && (fe->getCurrentSatellitePosition() != thischannel->getSatellitePosition()))
		return false;

	bool tp_band = ((int)thischannel->getFreqId()*1000 >= fe->lnbSwitch);
	uint8_t tp_pol = thischannel->polarization & 1;
	uint8_t fe_pol = fe->getPolarization() & 1;

	dprintf(DEBUG_DEBUG, "CZapit::loopCanTune: fe(%d,%d): locked %d pol:band %d:%d vs %d:%d (%d:%d)\n", fe->feadapter, fe->fenumber, fe->locked, fe_pol, fe->getHighBand(), tp_pol, tp_band, fe->getFrequency(), thischannel->getFreqId()*1000);
		
	if(!fe->tuned || (fe_pol == tp_pol && fe->getHighBand() == tp_band))
		return true;
	
	return false;
}

// getPreferredFrontend
CFrontend * CZapit::getPreferredFrontend(CZapitChannel * thischannel)
{
	// check for frontend
	CFrontend * pref_frontend = NULL;
	
	t_satellite_position satellitePosition = thischannel->getSatellitePosition();
	sat_iterator_t sit = satellitePositions.find(satellitePosition);
	
	// get preferred frontend
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_DEBUG, "CZapit::getPreferredFrontend: fe(%d,%d): tuned:%d (locked:%d) fe_freq: %d fe_TP: %llx - chan_freq: %d chan_TP: %llx sat-position: %d sat-name:%s input-type:%d\n",
				fe->feadapter,
				fe->fenumber,
				fe->tuned,
				fe->locked,
				fe->getFrequency(), 
				fe->getTsidOnid(), 
				thischannel->getFreqId(), 
				thischannel->getTransponderId(), 
				satellitePosition,
				sit->second.name.c_str(),
				sit->second.system);
				
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
		else if (sit != satellitePositions.end()) 
		{
			if( (fe->getDeliverySystem() & sit->second.system) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				pref_frontend = fe;
				break;
			}
		}
	}
	
	return pref_frontend;
}

// NOTE: this can be used only after we found our record_fe???
bool CZapit::CanZap(CZapitChannel * thischannel)
{	
	CFrontend * fe = getPreferredFrontend(thischannel);
	return (fe != NULL);
}

CFrontend * CZapit::getFrontend(CZapitChannel * thischannel)
{
	const char *FEMODE[] = {
		"SINGLE",
		"LOOP",
		"NOTCONNECTED"
	 };
	 
	// check for frontend
	CFrontend * free_frontend = NULL;
	
	//t_satellite_position satellitePosition = thischannel->getSatellitePosition();
	//sat_iterator_t sit = satellitePositions.find(satellitePosition);
	transponder_list_t::iterator transponder = transponders.find(thischannel->getTransponderId());
	
	// close unused frontend
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
			
		// skip tuned frontend and have same tid or same type as channel to tune
		if( (fe->tuned) && (fe->getTsidOnid() == thischannel->getTransponderId() || fe->getDeliverySystem() & /*sit->second.system*/transponder->second.feparams.delsys) )
			continue;

		// close not locked tuner
		if(!fe->locked && femap.size() > 1)
			fe->Close();
	}
	
	// get preferred frontend and initialize it
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_NORMAL, "CZapit::getFrontend: fe(%d:%d): (delsys:0x%x) (%s) tuned:%d (locked:%d) fe_TP: %llx - %d chan_TP: %llx\n",
				fe->feadapter,
				fe->fenumber,
				fe->deliverySystemMask,
				FEMODE[fe->mode],
				fe->tuned,
				fe->locked,
				//fe->getFrequency(), 
				fe->getTsidOnid(), 
				//thischannel->getFreqId(), 
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
		//else if (sit != satellitePositions.end()) 
		else if (transponder != transponders.end())
		{	
			//if ( (fe->getDeliverySystem() & sit->second.system) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			if ( (fe->getDeliverySystem() & transponder->second.feparams.delsys) && (!fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				free_frontend = fe;
				break;
			}
		}
	}
	
	if(free_frontend)
	{
		printf("%s Selected fe: (%d:%d) (delsys:0x%x)\n", __FUNCTION__, free_frontend->feadapter, free_frontend->fenumber,free_frontend->deliverySystemMask);
		
		if(free_frontend->standby)
			initTuner(free_frontend);
		
	}
	else
		printf("CZapit::%s can not get free frontend\n", __FUNCTION__);
	
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
	
	t_satellite_position satellitePosition = thischannel->getSatellitePosition();
	sat_iterator_t sit = satellitePositions.find(satellitePosition);
	
	// get record frontend
	for(fe_map_iterator_t fe_it = femap.begin(); fe_it != femap.end(); fe_it++) 
	{
		CFrontend * fe = fe_it->second;
		
		dprintf(DEBUG_INFO, "CZapit::getRecordFrontend: fe(%d,%d): (%s) tuned:%d (locked:%d) fe_freq: %d fe_TP: %llx - chan_freq: %d chan_TP: %llx sat-position: %d sat-name:%s input-type:%d\n",
				fe->feadapter,
				fe->fenumber,
				FEMODE[fe->mode],
				fe->tuned,
				fe->locked,
				fe->getFrequency(), 
				fe->getTsidOnid(), 
				thischannel->getFreqId(), 
				thischannel->getTransponderId(), 
				satellitePosition,
				sit->second.name.c_str(),
				sit->second.system);
				
		// skip not connected frontend
		if( fe->mode == (fe_mode_t)FE_NOTCONNECTED )
			continue;
		
		// frontend on same tid
		if( (fe->tuned) && (fe->getTsidOnid() == thischannel->getTransponderId()) )
		{
			rec_frontend = fe;
			break;
		}
		
		// second tuner (twin)
		else if (sit != satellitePositions.end()) 
		{
			bool twin = false;
			
			if( (fe->getInfo()->type == live_fe->getInfo()->type) && (fe->fenumber != live_fe->fenumber) )
				twin = true;
			
			if ( (fe->getDeliverySystem() & sit->second.system) && (twin? !fe->tuned : !fe->locked) && ( fe->mode == (fe_mode_t)FE_SINGLE || (fe->mode == (fe_mode_t)FE_LOOP && loopCanTune(fe, thischannel)) ) )
			{
				rec_frontend = fe;
				//break;
			}
		}
	}
	
	if(rec_frontend)
	{
		printf("%s Selected fe: (%d,%d)\n", __FUNCTION__, rec_frontend->feadapter, rec_frontend->fenumber);
		
		if(rec_frontend->standby)
			initTuner(rec_frontend);
		
	}
	else
		printf("%s can not get record frontend\n", __FUNCTION__);
	
	return rec_frontend;
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

// borrowed from cst neutrino-hd (femanager.cpp)
uint32_t getConfigValue(int num, const char * name, uint32_t defval)
{
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_%s", num, name);
	
	return fe_configfile.getInt32(cfg_key, defval);
}

// borrowed from cst neutrino-hd (femanger.cpp)
void setConfigValue(int num, const char * name, uint32_t val)
{
	char cfg_key[81];
	
	sprintf(cfg_key, "fe%d_%s", num, name);
	fe_configfile.setInt32(cfg_key, val);
}

// save frontend config
void CZapit::saveFrontendConfig(int feindex)
{
	dprintf(DEBUG_INFO, "CZapit::saveFrontendConfig\n");
	
	for(feindex = 0; feindex < FrontendCount; feindex++)
	{
		// mode
		setConfigValue(feindex, "mode", getFE(feindex)->mode);
		
		// mode
		setConfigValue(feindex, "powered", getFE(feindex)->powered);
		
		// delsys
		setConfigValue(feindex, "delsys", getFE(feindex)->forcedDelSys);
			
		// sat
		if(getFE(feindex)->getInfo()->type == FE_QPSK)
		{
			setConfigValue(feindex, "lastSatellitePosition", getFE(feindex)->getCurrentSatellitePosition());
			setConfigValue(feindex, "diseqcRepeats", getFE(feindex)->getDiseqcRepeats());
			setConfigValue(feindex, "diseqcType", getFE(feindex)->getDiseqcType() );

			// unicable
			setConfigValue(feindex, "uni_scr", getFE(feindex)->getUniScr() );
			setConfigValue(feindex, "uni_qrg", getFE(feindex)->getUniQrg() );
					
			char tempd[12];
			char cfg_key[81];
				
			sprintf(tempd, "%3.6f", getFE(feindex)->gotoXXLatitude);
			sprintf(cfg_key, "fe%d_gotoXXLatitude", feindex);
			fe_configfile.setString(cfg_key, tempd );
				
			sprintf(tempd, "%3.6f", getFE(feindex)->gotoXXLongitude);
			sprintf(cfg_key, "fe%d_gotoXXLongitude", feindex);
			fe_configfile.setString(cfg_key, tempd );
				
			setConfigValue(feindex, "gotoXXLaDirection", getFE(feindex)->gotoXXLaDirection);
			setConfigValue(feindex, "gotoXXLoDirection", getFE(feindex)->gotoXXLoDirection);
			setConfigValue(feindex, "useGotoXX", getFE(feindex)->useGotoXX);
			setConfigValue(feindex, "repeatUsals", getFE(feindex)->repeatUsals);
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
		fe->mode = (fe_mode_t)getConfigValue(fe_it->first, "mode", (fe_mode_t)FE_SINGLE);
		
		fe->powered = getConfigValue(fe_it->first, "powered", 0);
		
		// delsys
		if (fe->info.type == FE_QPSK)
		{
			fe->forcedDelSys = getConfigValue(fe_it->first, "delsys", DVB_S);
		}
		else if (fe->info.type == FE_QAM)
		{
			fe->forcedDelSys = getConfigValue(fe_it->first, "delsys", DVB_C);
		}
		else if (fe->info.type == FE_OFDM)
		{
			fe->forcedDelSys = getConfigValue(fe_it->first, "delsys", DVB_T);
		}
		else if (fe->info.type == FE_ATSC)
		{
			fe->forcedDelSys = getConfigValue(fe_it->first, "delsys", DVB_A);
		}
		
		// sat
		if(fe->getInfo()->type == FE_QPSK)
		{
			fe->useGotoXX = getConfigValue(fe_it->first, "useGotoXX", 0);
			
			char cfg_key[81];
			
			sprintf(cfg_key, "fe%d_gotoXXLatitude", fe_it->first );
			fe->gotoXXLatitude = strtod( fe_configfile.getString(cfg_key, "0.0").c_str(), NULL);
			
			sprintf(cfg_key, "fe%d_gotoXXLongitude", fe_it->first );
			fe->gotoXXLongitude = strtod(fe_configfile.getString(cfg_key, "0.0").c_str(), NULL);
			
			fe->gotoXXLaDirection = getConfigValue(fe_it->first, "gotoXXLaDirection", 0);
			fe->gotoXXLoDirection = getConfigValue(fe_it->first, "gotoXXLoDirection", 0);
			
			fe->repeatUsals = getConfigValue(fe_it->first, "repeatUsals", 0);
			fe->diseqcType = (diseqc_t)getConfigValue(fe_it->first, "diseqcType", (diseqc_t)NO_DISEQC);
			fe->diseqcRepeats = getConfigValue(fe_it->first, "diseqcRepeats", 0);

			// unicable
			fe->uni_scr = getConfigValue(fe_it->first, "uni_scr", -1);
			fe->uni_qrg = getConfigValue(fe_it->first, "uni_qrg", 0);

			fe->motorRotationSpeed = getConfigValue(fe_it->first, "motorRotationSpeed", 18); // default: 1.8 degrees per second
			
			fe->lastSatellitePosition = getConfigValue(fe_it->first, "lastSatellitePosition", 0);
		}
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
		if (config.getBool("saveLastChannel", true)) 
		{
			if (currentMode & RADIO_MODE)
				config.setInt32("lastChannelMode", RADIO_MODE);
			else if (currentMode & TV_MODE)
				config.setInt32("lastChannelMode", TV_MODE);

			config.setInt32("lastChannelRadio", lastChannelRadio);
			config.setInt32("lastChannelTV", lastChannelTV);
			config.setInt64("lastChannel", live_channel_id);
		}
		
		config.setBool("makeRemainingChannelsBouquet", makeRemainingChannelsBouquet);

		config.setInt32("scanSDT", scanSDT);

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
	lastChannelTV = config.getInt32("lastChannelTV", 0);
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
#if defined (ENABLE_CI)
			//if(live_fe != NULL)
			//	ci->SendCaPMT(NULL, live_fe->fenumber);
			//SendCaPmt(live_channel_id, NULL, 0, CA_SLOT_TYPE_ALL);
#endif
		}
	} 
	else 
	{
		if(live_channel != NULL)
			cam0->setCaPmt(live_channel, live_channel->getCaPmt(), demux_index, ca_mask); //cam0 start
#if defined (ENABLE_CI)
		if(rec_channel != NULL)
		{
			//if(record_fe != NULL)
			//	ci->SendCaPMT(NULL, record_fe->fenumber);
		}
#endif
	}
	
	// ci cam
#if defined (ENABLE_CI)
	if(live_channel != NULL)
	{
		//if(live_fe != NULL)
		//	ci->SendCaPMT(live_channel->getCaPmt(), live_fe->fenumber);
	}
#endif
}

// save pids
void CZapit::saveChannelPids(CZapitChannel * thischannel)
{
	if(thischannel == NULL)
		return;

	dprintf(DEBUG_INFO, "CZapit::saveChannelPids: (%llx), apid %x mode %d volume %d\n", thischannel->getChannelID(), thischannel->getAudioPid(), audio_mode, volume_right);
	
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
	
	if (in_nvod) //nvod
	{
		current_is_nvod = true;

		cit = nvodchannels.find(channel_id);

		if (cit == nvodchannels.end()) 
		{
			dprintf(DEBUG_INFO, "CZapit::findChannelToZap: channel_id (%llx) AS NVOD not found\n", channel_id);
			return NULL;
		}
	} 
	else 
	{
		current_is_nvod = false;

		cit = allchans.find(channel_id);

		if (cit == allchans.end()) 
		{
			dprintf(DEBUG_INFO, "CZapit::findChannelToZap: channel_id (%llx) not found\n", channel_id);
			return NULL;
		}
	}
	
	return &cit->second;
}

bool CZapit::tuneToChannel(CFrontend * frontend, CZapitChannel * thischannel, bool &transponder_change)
{
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
			eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_MOTOR, CEventServer::INITID_NEUTRINO, &waitForMotor, sizeof(waitForMotor));
				
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
	dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: looking up pids for channel_id (%llx)\n", thischannel->getChannelID());
	
	bool failed = false;
	
	// get program map table pid from program association table
	if (thischannel->getPmtPid() == 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: no pmt pid, going to parse pat\n");	
		
		if (CPat::getInstance()->parsePAT(thischannel, fe) < 0)
		{
			dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pat parsing failed\n");
			
			failed = true;
		}
	}

	// parse program map table and store pids
	if ( !failed && CPmt::getInstance()->parsePMT(thischannel, fe) < 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pmt parsing failed\n");	
		
		if (CPat::getInstance()->parsePAT(thischannel, fe) < 0) 
		{
			dprintf(DEBUG_NORMAL, "CZapit::parseChannelPatPmt: pat parsing failed\n");
			
			failed = true;
		}
		else if (CPmt::getInstance()->parsePMT(thischannel, fe) < 0) 
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

// return 0, -1 fails
int CZapit::zapit(const t_channel_id channel_id, bool in_nvod, bool forupdate)
{
	bool transponder_change = false;
	tallchans_iterator cit;
	bool failed = false;
	CZapitChannel * newchannel;

	dprintf(DEBUG_NORMAL, ANSI_BLUE"CZapit::zapit: channel id %llx nvod %d\n", channel_id, in_nvod);

	// find channel to zap
	if( (newchannel = findChannelToZap(channel_id, in_nvod)) == NULL ) 
	{
		dprintf(DEBUG_INFO, "CZapit::zapit: channel_id:%llx not found\n", channel_id);
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
		CPmt::getInstance()->pmt_stop_update_filter(&pmt_update_fd);
	}
	
	// FIXME: how to stop ci_capmt or we dont need this???
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
	
		dprintf(DEBUG_NORMAL, ANSI_BLUE"CZapit::zapit: zap to %s(%llx) fe(%d:%d)\n", live_channel->getName().c_str(), live_channel_id, live_fe->feadapter, live_fe->fenumber );

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

	//
	if  (IS_WEBTV(channel_id))
		return res;
	else
	{
		// cam
		sendCaPmtPlayBackStart(live_channel, live_fe);
	
		// ci cam
#if defined (ENABLE_CI)	
		if(live_channel != NULL)
		{
			//if(live_fe != NULL)
			//	ci->SendCaPMT(live_channel->getCaPmt(), live_fe->fenumber);
		}
#endif		
	
		// send caid
		int caid = 1;

		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_CA_ID, CEventServer::INITID_NEUTRINO, &caid, sizeof(int));

		// start pmt update filter
		CPmt::getInstance()->pmt_set_update_filter(live_channel, &pmt_update_fd, live_fe);
	}	

	return 0;
}

int CZapit::zapToRecordID(const t_channel_id channel_id)
{
	bool transponder_change = false;
	
	// find channel
	if((rec_channel = findChannelToZap(channel_id, false)) == NULL) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToRecordID: channel_id (%llx) not found\n", channel_id);
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
		
	record_fe = frontend;
	
	// single/multi on the same frontend
	if(record_fe == live_fe)
	{
		if( (rec_channel_id != live_channel_id) && !SAME_TRANSPONDER(live_channel_id, rec_channel_id) )
		{
			// zap to record channel
			zapToChannelID(rec_channel_id, false);
			return 0;
		}
	}
	// twin/multi other frontend as live frontend
	else
	{
		// just tune
		if(!tuneToChannel(record_fe, rec_channel, transponder_change))
			return -1;
	}
	
	// parse channel pat_pmt
	if(!parseChannelPatPmt(rec_channel, record_fe))
		return -1;
						
	// cam
	sendCaPmtPlayBackStart(rec_channel, record_fe);
	
	// ci cam
#if defined (ENABLE_CI)	
	if(rec_channel != NULL)
	{
		//if(record_fe != NULL)
		//	ci->SendCaPMT(rec_channel->getCaPmt(), record_fe->fenumber);
	}
#endif		
	
	dprintf(DEBUG_NORMAL, "CZapit::zapToRecordID: zapped to %s (%llx) fe(%d,%d)\n", rec_channel->getName().c_str(), rec_channel_id, record_fe->feadapter, record_fe->fenumber);
	
	return 0;
}

// set channel/pid volume percent, using current channel_id and pid
void CZapit::setPidVolume(t_channel_id channel_id, int pid, int percent)
{
	if (!channel_id)
		channel_id = live_channel_id;

	if (!pid && (channel_id == live_channel_id) && live_channel)
		pid = live_channel->getAudioPid();

	dprintf(DEBUG_INFO, "CZapit::setPidVolume: channel %llx pid %x percent %d\n", channel_id, pid, percent);
	
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
	
	dprintf(DEBUG_INFO, "CZapit::getPidVolume: channel %llx pid %x percent %d\n", channel_id, pid, percent);
	
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
#if defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_FMT_DOLBY_DIGITAL);				
#else
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AC3);
#endif
				break;
			
			case CZapitAudioChannel::MPEG:
#if defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_FMT_MPEG);
#else
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_MPEG);
#endif
				break;
				
			case CZapitAudioChannel::AAC:
#if defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_FMT_AAC);
#else				
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC);
#endif				
				break;
			
			case CZapitAudioChannel::AACPLUS:
#if defined (PLATFORM_COOLSTREAM)			  
				audioDecoder->SetStreamType(AUDIO_FMT_AAC_PLUS);
#else
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AACPLUS);
#endif
				break;
			
			case CZapitAudioChannel::DTS:
#if defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_FMT_DTS);
#else
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTS);
#endif
				break;
				
			case CZapitAudioChannel::DTSHD:
#if !defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTSHD);
#endif
				break;
				
			case CZapitAudioChannel::EAC3:
#if defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_FMT_DD_PLUS);
#else
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_EAC3);
#endif
				break;
				
			case CZapitAudioChannel::LPCM:
#if !defined (PLATFORM_COOLSTREAM)
				audioDecoder->SetStreamType(AUDIO_STREAMTYPE_LPCMDVD);
#endif
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

	if (!avDecoderOpen)
		openAVDecoder();
}

void CZapit::setTVMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::setTVMode:\n");

	currentMode |= TV_MODE;
	currentMode &= ~RADIO_MODE;

	if (!avDecoderOpen)
		openAVDecoder();
}

int CZapit::getMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::getMode:\n");
	
	int mode = 0;
	
	if (currentMode & TV_MODE)
		mode = MODE_TV;
	else if (currentMode & RADIO_MODE)
		mode = MODE_RADIO;

	return mode;
}

void CZapit::setRecordMode(void)
{
	dprintf(DEBUG_NORMAL, "CZapit::setRecordMode:\n");
	
	if(currentMode & RECORD_MODE) 
		return;

	currentMode |= RECORD_MODE;
	
	// lock frontend
	lockFrontend(record_fe);
	 
	eventServer->sendEvent(NeutrinoMessages::EVT_RECORDMODE, CEventServer::INITID_NEUTRINO, (void *)true, sizeof(bool) );
}

void CZapit::unsetRecordMode(void)
{
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
 
	eventServer->sendEvent(NeutrinoMessages::EVT_RECORDMODE, CEventServer::INITID_NEUTRINO, (void *)false, sizeof(bool) );
}

int CZapit::prepareChannels()
{
	dprintf(DEBUG_NORMAL, "CZapit::prepareChannels:\n");
	
	live_channel = 0;
	
	// clear all channels/bouquets/TP's lists
	transponders.clear();
	g_bouquetManager->clearAll();
	allchans.clear();  				// <- this invalidates all bouquets, too!
	
	// load frontend config
	loadFrontendConfig();
        
    	// load sats/tps
    	CServices::getInstance()->loadTransponders();

	// load services
	if (CServices::getInstance()->loadServices(false) < 0)
	{
		dprintf(DEBUG_NORMAL, "CZapit::prepareChannels: loadServices: failed\n");
		return -1;
	}

	dprintf(DEBUG_NORMAL, "CZapit::prepareChannels: loadServices: success\n");

	// load bouquets
	g_bouquetManager->loadBouquets();		// 2004.08.02 g_bouquetManager->storeBouquets();

	return 0;
}

//
void CZapit::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitChannel * chan = g_bouquetManager->findChannelByChannelID(channel_id);

	if (chan != NULL)
	{
		if (bouquet < g_bouquetManager->Bouquets.size())
			g_bouquetManager->Bouquets[bouquet]->addService(chan);
		else
			printf("bouquet not found\n");
	}
	else
		printf("channel_id not found in channellist\n");
}

void CZapit::sendAPIDs(APIDList &apids)
{
	for (uint32_t  i = 0; i < live_channel->getAudioChannelCount(); i++) 
	{
		responseGetAPIDs response;
		response.pid = live_channel->getAudioPid(i);
		strncpy(response.desc, live_channel->getAudioChannel(i)->description.c_str(), 25);

		response.is_ac3 = 0;
		
		if (live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AC3
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AAC
			|| live_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACPLUS
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

void CZapit::sendSubPIDs(SubPIDList &subpids)
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
			|| rec_channel->getAudioChannel(i)->audioChannelType == CZapitAudioChannel::AACPLUS
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

void CZapit::internalSendNChannels(ZapitChannelList *channels, const unsigned int first_channel_nr, BouquetNChannelList &Bchannels)
{
	for (uint32_t  i = 0; i < channels->size();i++) 
	{
		responseGetBouquetNChannels response;
		
		response.nr = first_channel_nr + i;
		
		Bchannels.push_back(response);
	}
}

void CZapit::sendBouquets(responseGetBouquets &msgBouquet, const bool emptyBouquetsToo, channelsMode mode)
{
        int curMode;
	
        switch(mode) 
	{
                case MODE_TV:
                        curMode = TV_MODE;
                        break;
                case MODE_RADIO:
                        curMode = RADIO_MODE;
                        break;
                case MODE_CURRENT:
                default:
                        curMode = currentMode;
                        break;
        }

        for (uint32_t i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{	
                if (emptyBouquetsToo || (!g_bouquetManager->Bouquets[i]->bHidden && g_bouquetManager->Bouquets[i]->bUser)
                    || ((!g_bouquetManager->Bouquets[i]->bHidden)
                     && (((curMode & RADIO_MODE) && !g_bouquetManager->Bouquets[i]->radioChannels.empty())
                   )))
		{
			{
				msgBouquet.bouquet_nr = i;
				strncpy(msgBouquet.name, g_bouquetManager->Bouquets[i]->Name.c_str(), 30);
				msgBouquet.name[29] = 0;
				msgBouquet.locked     = g_bouquetManager->Bouquets[i]->bLocked;
				msgBouquet.hidden     = g_bouquetManager->Bouquets[i]->bHidden;
			}
		}
	}
	
	msgBouquet.bouquet_nr = RESPONSE_GET_BOUQUETS_END_MARKER;
}

void CZapit::sendBouquetChannels(BouquetChannelList &Bchannels, const unsigned int bouquet, const channelsMode mode)
{
	if (bouquet >= g_bouquetManager->Bouquets.size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::sendBouquetChannels: invalid bouquet number: %d\n", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_RADIO))
		internalSendChannels(&(g_bouquetManager->Bouquets[bouquet]->radioChannels), g_bouquetManager->radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
	else if (((currentMode & TV_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_TV))
		internalSendChannels(&(g_bouquetManager->Bouquets[bouquet]->tvChannels), g_bouquetManager->tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
}

void CZapit::sendBouquetNChannels(BouquetNChannelList &Bchannels, const unsigned int bouquet, const channelsMode mode)
{
	if (bouquet >= g_bouquetManager->Bouquets.size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::sendBouquetNChannels: invalid bouquet number: %d\n", bouquet);
		return;
	}

	if (((currentMode & RADIO_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_RADIO))
		internalSendNChannels(&(g_bouquetManager->Bouquets[bouquet]->radioChannels), g_bouquetManager->radioChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
	else if (((currentMode & TV_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_TV))
		internalSendNChannels(&(g_bouquetManager->Bouquets[bouquet]->tvChannels), g_bouquetManager->tvChannelsBegin().getNrofFirstChannelofBouquet(bouquet), Bchannels);
}

void CZapit::sendChannels(BouquetChannelList &Bchannels, const channelsMode mode, const channelsOrder order)
{
	ZapitChannelList channels;
	
	if (order == SORT_BOUQUET) 
	{
		CBouquetManager::ChannelIterator cit = g_bouquetManager->tvChannelsBegin();

		if( (currentMode & RADIO_MODE) && ((mode == MODE_CURRENT) || (mode == MODE_RADIO)))
			cit = g_bouquetManager->radioChannelsBegin();
		else if( (currentMode & TV_MODE) && ((mode == MODE_CURRENT) || (mode == MODE_TV)))
			cit = g_bouquetManager->tvChannelsBegin();

		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
	}
	else if (order == SORT_ALPHA)   // ATTENTION: in this case response.nr does not return the actual number of the channel for zapping!
	{
		if (((currentMode & RADIO_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_RADIO)) 
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		} 
		else if (((currentMode & TV_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_TV))  
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}

		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendChannels(&channels, 0, Bchannels);
}

void CZapit::sendNChannels(BouquetNChannelList &Bchannels, const channelsMode mode, const channelsOrder order)
{
	ZapitChannelList channels;
	
	if (order == SORT_BOUQUET) 
	{
		CBouquetManager::ChannelIterator cit = g_bouquetManager->tvChannelsBegin();

		if( (currentMode & RADIO_MODE) && ((mode == MODE_CURRENT) || (mode == MODE_RADIO)))
			cit = g_bouquetManager->radioChannelsBegin();
		else if( (currentMode & TV_MODE) && ((mode == MODE_CURRENT) || (mode == MODE_TV)))
			cit = g_bouquetManager->tvChannelsBegin();

		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
	}
	else if (order == SORT_ALPHA)   // ATTENTION: in this case response.nr does not return the actual number of the channel for zapping!
	{
		if (((currentMode & RADIO_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_RADIO)) 
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		} 
		else if (((currentMode & TV_MODE) && (mode == MODE_CURRENT)) || (mode == MODE_TV))  
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)
					channels.push_back(&(it->second));
		}

		sort(channels.begin(), channels.end(), CmpChannelByChName());
	}

	internalSendNChannels(&channels, 0, Bchannels);
}

// startplayback return: 0=playing, -1= failed
int CZapit::startPlayBack(CZapitChannel * thisChannel)
{
	dprintf(DEBUG_NORMAL, "CZapit::startPlayBack: chid:%llx\n", thisChannel->getChannelID());

	if(!thisChannel)
		thisChannel = live_channel;

	if(standby) 
		return 0;

	if (!thisChannel || playing)
		return -1;

	if (IS_WEBTV(thisChannel->getChannelID()))
	{
		//
		if (avDecoderOpen)
			closeAVDecoder();
			
		//
		//playback->Close(); // not needed???
		
		playback->Open();
	
		if (!playback->Start((char *)thisChannel->getUrl().c_str()))
			return -1;
	}
	else
	{
		if (playbackStopForced)
			return -1;
			
		// 
		if (!avDecoderOpen)
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
#if defined (PLATFORM_COOLSTREAM)
			pcrDemux->Open(DMX_PCR_ONLY_CHANNEL, videoDemux->getBuffer());
#else		
			if( pcrDemux->Open(DMX_PCR_ONLY_CHANNEL, VIDEO_STREAM_BUFFER_SIZE, live_fe) < 0 )
				return -1;
#endif		
		
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
#if defined (PLATFORM_COOLSTREAM)
			audioDemux->Open(DMX_AUDIO_CHANNEL);
#else		
			if( audioDemux->Open(DMX_AUDIO_CHANNEL, AUDIO_STREAM_BUFFER_SIZE, live_fe ) < 0 )
				return -1;
#endif		
		
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
#if defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_FMT_DOLBY_DIGITAL);
#else
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AC3);
#endif
						break;
					
					case CZapitAudioChannel::MPEG:
						audioStr = "MPEG2";
#if defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_FMT_MPEG);
#else
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_MPEG);
#endif
						break;
					
					case CZapitAudioChannel::AAC:
						audioStr = "AAC";
#if defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_FMT_AAC);
#else
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AAC);
#endif					
						break;
					
					case CZapitAudioChannel::AACPLUS:
						audioStr = "AAC-PLUS";
#if defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_FMT_AAC);
#else
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_AACPLUS);
#endif
						break;
					
					case CZapitAudioChannel::DTS:
						audioStr = "DTS";
#if defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_FMT_DTS);
#else
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTS);
#endif
						break;
					
					case CZapitAudioChannel::DTSHD:
						audioStr = "DTSHD";
#if !defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_DTSHD);
#endif
						break;
					
					case CZapitAudioChannel::EAC3:
						audioStr = "EAC3";
#if defined (PLATFORM_COOLSTREAM)
					audioDecoder->SetStreamType(AUDIO_FMT_DD_PLUS);
#else
					audioDecoder->SetStreamType(AUDIO_STREAMTYPE_EAC3);
#endif
						break;
					
					case CZapitAudioChannel::LPCM:
						audioStr = "LPCM";
#if !defined (PLATFORM_COOLSTREAM)
						audioDecoder->SetStreamType(AUDIO_STREAMTYPE_LPCMDVD);
#endif
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
#if !defined (__sh__) && !defined (PLATFORM_COOLSTREAM)			
				audioDecoder->Resume();
				audioDecoder->Stop();
				//audioDecoder->Pause();
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
#if defined (PLATFORM_COOLSTREAM)
			videoDemux->Open(DMX_VIDEO_CHANNEL);
#else		
			if( videoDemux->Open(DMX_VIDEO_CHANNEL, VIDEO_STREAM_BUFFER_SIZE, live_fe ) < 0 )
				return -1;
#endif
			// set stream type
			const char *videoStr = "UNKNOWN";
		
			if(videoDecoder)
			{
				if(thisChannel->videoType == CHANNEL_VIDEO_MPEG2)
				{
					videoStr = "MPEG2";
#if defined (PLATFORM_COOLSTREAM)
					videoDecoder->SetStreamType(VIDEO_FORMAT_MPEG2);
#else
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_MPEG2);
#endif
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_MPEG4)
				{
					videoStr = "H.264/MPEG-4 AVC";
#if defined (PLATFORM_COOLSTREAM)
					videoDecoder->SetStreamType(VIDEO_FORMAT_MPEG4);
#else
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_MPEG4_H264);
#endif				
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_HEVC)
				{
					videoStr = "H.265 HEVC";
#if !defined (PLATFORM_COOLSTREAM)
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_H265_HEVC);
#endif				
				}
				else if(thisChannel->videoType == CHANNEL_VIDEO_CAVS)
				{
					videoStr = "AVS";
#if !defined (PLATFORM_COOLSTREAM)
					videoDecoder->SetStreamType(VIDEO_STREAMTYPE_AVS);
#endif				
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
#if !defined (__sh__) && !defined (PLATFORM_COOLSTREAM)
				videoDecoder->Resume();
				videoDecoder->Stop();
				//videoDecoder->Pause();
#endif
								  
#if defined (PLATFORM_COOLSTREAM)
				videoDecoder->Start(0, thisChannel->getPcrPid(), thisChannel->getVideoPid());
#else			
				videoDecoder->Start();
#endif	
			}
		}

#if defined (USE_OPENGL) // opengl playback
		startOpenGLplayback();
#endif
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

		// stop video demux
		if (videoDemux)
		{
			// stop
			videoDemux->Stop();
			delete videoDemux;	//destructor closes dmx
			videoDemux = NULL;
		}
	
		// stop video decoder (blanking)
		videoDecoder->Stop(true);
	
		if (pcrDemux)
		{
			// stop
			pcrDemux->Stop();
			delete pcrDemux; //destructor closes dmx
			pcrDemux = NULL;
		}

#if defined (USE_OPENGL) // opengl playback
		stopOpenGLplayback();
#endif
	
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
}

void CZapit::continuePlayBack(void)
{
	dprintf(DEBUG_DEBUG, "CZapit::continuePlayBack\n");

	//
	if (IS_WEBTV(live_channel->getChannelID()))
		playback->SetSpeed(1);
}

void CZapit::closeAVDecoder(void)
{
#if !defined (USE_OPENGL)
	// close videodecoder
	if(videoDecoder)
		videoDecoder->Close();
	
	// close audiodecoder
	if(audioDecoder)
		audioDecoder->Close();

	avDecoderOpen = false;
#endif
}

void CZapit::openAVDecoder(void)
{
#if !defined (USE_OPENGL)
	if(videoDecoder)
	{
		// open video decoder
		videoDecoder->Open(/*live_fe*/);
	
		// set source
		videoDecoder->setSource(VIDEO_SOURCE_DEMUX);	
	}	
	
	if(audioDecoder)
	{
		// open audiodecoder
		audioDecoder->Open(/*live_fe*/);
		
		// set source
		audioDecoder->setSource(AUDIO_SOURCE_DEMUX);
	}

	avDecoderOpen = true;	
#endif
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
	
#if !defined (PLATFORM_COOLSTREAM)
	// close AVdecoder
	closeAVDecoder();
#endif	
	
	//close frontend	
	CloseFE();
}

void CZapit::leaveStandby(void)
{ 
	dprintf(DEBUG_NORMAL, "CZapit::leaveStandby\n");
	
	if(!standby) 
		return;
	
	standby = false;
	
	// live cam
	if (!cam0) 
		cam0 = new CCam();
	
	// record cam
	if(!cam1)
		cam1 = new CCam();
	
#if !defined (PLATFORM_COOLSTREAM)	
	openAVDecoder();
#endif	

	// zap
	zapToChannelID(live_channel_id, current_is_nvod);
}

unsigned CZapit::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	if (bouquet >= g_bouquetManager->Bouquets.size()) 
	{
		dprintf(DEBUG_INFO, "CZapit::zapTo: Invalid bouquet %d\n", bouquet);
		return ZAP_INVALID_PARAM;
	}

	ZapitChannelList * channels;

	if (currentMode & RADIO_MODE)
		channels = &(g_bouquetManager->Bouquets[bouquet]->radioChannels);
	else if (currentMode & TV_MODE)
		channels = &(g_bouquetManager->Bouquets[bouquet]->tvChannels);

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

	dprintf(DEBUG_NORMAL, ANSI_BLUE"CZapit::zapToChannelID: chid %llx\n", channel_id);

	if (zapit(channel_id, isSubService) < 0) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: zapit failed, chid %llx\n", channel_id);
		
		eventServer->sendEvent((isSubService ? NeutrinoMessages::EVT_ZAP_SUB_FAILED : NeutrinoMessages::EVT_ZAP_FAILED), CEventServer::INITID_NEUTRINO, &channel_id, sizeof(channel_id));
		
		return result;
	}

	result |= ZAP_OK;

	dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: zapit OK, chid %llx\n", channel_id);

	//
	if (IS_WEBTV(channel_id))
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: isWEBTV chid %llx\n", channel_id);
	}
	
	if (isSubService) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: isSubService chid %llx\n", channel_id);
		
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_SUB_COMPLETE, CEventServer::INITID_NEUTRINO, &channel_id, sizeof(channel_id));
	}
	else if (current_is_nvod) 
	{
		dprintf(DEBUG_NORMAL, "CZapit::zapToChannelID: NVOD chid %llx\n", channel_id);
		
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_ISNVOD, CEventServer::INITID_NEUTRINO, &channel_id, sizeof(channel_id));
		
		result |= ZAP_IS_NVOD;
	}
	else
		eventServer->sendEvent(NeutrinoMessages::EVT_ZAP_COMPLETE, CEventServer::INITID_NEUTRINO, &channel_id, sizeof(channel_id));

	return result;
}

unsigned CZapit::zapTo(const unsigned int channel)
{
	//FIXME:
	CBouquetManager::ChannelIterator cit = (g_bouquetManager->tvChannelsBegin()).FindChannelNr(channel);

	if (currentMode & RADIO_MODE)
		cit = (g_bouquetManager->radioChannelsBegin()).FindChannelNr(channel);
	else if (currentMode & TV_MODE)
		cit = (g_bouquetManager->tvChannelsBegin()).FindChannelNr(channel);
	
	if (!(cit.EndOfChannels()))
		return zapToChannelID((*cit)->getChannelID(), false);
	else
		return 0;
}

void CZapit::setZapitConfig(Zapit_config * Cfg)
{
	makeRemainingChannelsBouquet = Cfg->makeRemainingChannelsBouquet;
	config.setBool("saveLastChannel", Cfg->saveLastChannel);
	scanSDT = Cfg->scanSDT;
	
	// save it
	saveZapitSettings(true, false);
}

void CZapit::getZapitConfig(Zapit_config *Cfg)
{
        Cfg->makeRemainingChannelsBouquet = makeRemainingChannelsBouquet;
        Cfg->saveLastChannel = config.getBool("saveLastChannel", true);
        Cfg->scanSDT = scanSDT;
}

//
void * CZapit::sdtThread(void */*arg*/)
{
	dprintf(DEBUG_NORMAL, "CZapit::sdtThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	if (!FrontendCount)
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

	tcur = time(0);
	tstart = time(0);
	sdt_tp.clear();
	
	dprintf(DEBUG_INFO, "CZaoit::sdtThread: sdt monitor started\n");

	while(true) 
	{
		sleep(1);

		if(sdt_wakeup) 
		{
			sdt_wakeup = 0;

			if(live_channel) 
			{
				wtime = time(0);
				transport_stream_id = live_channel->getTransportStreamId();
				original_network_id = live_channel->getOriginalNetworkId();
				satellitePosition = live_channel->getSatellitePosition();
				freq = live_channel->getFreqId();
				tpid = live_channel->getTransponderId();
			}
		}
		
		if(!scanSDT)
			continue;

		tcur = time(0);
		
		if(wtime && ((tcur - wtime) > 2) && !sdt_wakeup) 
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
			stI = sdt_tp.find(tpid);

			if((stI != sdt_tp.end()) && stI->second) 
			{
				dprintf(DEBUG_INFO, "CZapit::sdtThread: TP already updated.\n");
				continue;
			}

			if(live_channel) 
			{
				if( CSdt::getInstance()->parseCurrentSDT(transport_stream_id, original_network_id, satellitePosition, freq, live_fe) < 0 )
					continue;
			}

			sdt_tp.insert(std::pair <transponder_id_t, bool> (tpid, true) );

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

			if(live_channel) 
			{
				switch(spos_it->second.system)
				{
					case DVB_S: /* satellite */
						sprintf(satstr, "\t<%s name=\"%s\" position=\"%hd\">\n", "sat", spos_it->second.name.c_str(), satellitePosition);
						sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" sr=\"%u\" fec=\"%hu\" pol=\"%hu\">\n",
						tI->second.transport_stream_id, tI->second.original_network_id,
						tI->second.feparams.frequency, tI->second.feparams.inversion,
						tI->second.feparams.symbol_rate, tI->second.feparams.fec_inner,
						tI->second.polarization);
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
						sprintf(satstr, "\t<%s name=\"%s\">\n", "terrestrial", spos_it->second.name.c_str());
						sprintf(tpstr, "\t\t<TS id=\"%04x\" on=\"%04x\" frq=\"%u\" inv=\"%hu\" band=\"%hu\" HP=\"%hu\" LP=\"%hu\" const=\"%hu\" trans=\"%hu\" guard=\"%hu\" hierarchy=\"%hu\">\n",
						tI->second.transport_stream_id, tI->second.original_network_id,
						tI->second.feparams.frequency, tI->second.feparams.inversion,
						tI->second.feparams.bandwidth, tI->second.feparams.code_rate_HP,
						tI->second.feparams.code_rate_LP, tI->second.feparams.modulation,tI->second.feparams.transmission_mode, tI->second.feparams.guard_interval, tI->second.feparams.hierarchy_information);
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

			if(updated && (scanSDT == 1))
			{
				CServices::getInstance()->loadServices(true);
			  	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICES_UPD, CEventServer::INITID_NEUTRINO);
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
	
	if (!FrontendCount)
		return 0;
	
	while (true) 
	{	
		// pmt update
		if (pmt_update_fd != -1) 
		{
			unsigned char buf[4096];
			int ret = pmtDemux->Read(buf, 4095, 10); /* every 10 msec */

			if (ret > 0) 
			{
				CPmt::getInstance()->pmt_stop_update_filter(&pmt_update_fd);

				dprintf(DEBUG_INFO, "CZapit::updatePMTFilter: pmt updated, sid 0x%x new version 0x%x\n", (buf[3] << 8) + buf[4], (buf[5] >> 1) & 0x1F);

				// zap channel
				if(live_channel) 
				{
					t_channel_id channel_id = live_channel->getChannelID();
					int vpid = live_channel->getVideoPid();
					int apid = live_channel->getAudioPid();
					
					CPmt::getInstance()->parsePMT(live_channel, live_fe);
					
					bool apid_found = false;
					// check if selected audio pid still present
					for (int i = 0; i <  live_channel->getAudioChannelCount(); i++) 
					{
						if (live_channel->getAudioChannel(i)->pid == apid) 
						{
							apid_found = true;
							break;
						}
					}
					
					if(!apid_found || vpid != live_channel->getVideoPid()) 
					{
						CZapit::getInstance()->zapit(live_channel->getChannelID(), current_is_nvod, true);
					} 
					else 
					{
						CZapit::getInstance()->sendCaPmtPlayBackStart(live_channel, live_fe);
						
						// ci cam
#if defined (ENABLE_CI)
						if(live_channel != NULL)
						{
							//if(live_fe != NULL)
							//	ci->SendCaPMT(live_channel->getCaPmt(), live_fe->fenumber);
						}
#endif	

						CPmt::getInstance()->pmt_set_update_filter(live_channel, &pmt_update_fd, live_fe);
					}
						
					eventServer->sendEvent(NeutrinoMessages::EVT_PMT_CHANGED, CEventServer::INITID_NEUTRINO, &channel_id, sizeof(channel_id));
				}
			}
		}

		usleep(0);
	}
	
	// stop update pmt filter
	CPmt::getInstance()->pmt_stop_update_filter(&pmt_update_fd);
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
			
#if !defined (PLATFORM_COOLSTREAM)			
	closeAVDecoder();
#endif			
			
	playbackStopForced = true;
}

void CZapit::unlockPlayBack()
{
	playbackStopForced = false;
			
#if !defined (PLATFORM_COOLSTREAM)
	openAVDecoder();
#endif			

	if(live_channel != NULL)
	{
		startPlayBack(live_channel);
			
		// cam
		if (!IS_WEBTV(live_channel->getChannelID()))
			sendCaPmtPlayBackStart(live_channel, live_fe);
	}
			
			// ci cam
#if defined (ENABLE_CI)	
	if(live_channel != NULL)
	{
		//if(live_fe != NULL)
		//	ci->SendCaPMT(live_channel->getCaPmt(), live_fe->fenumber);
	}
#endif					

#if defined (ENABLE_GSTREAMER)
	if (! (currentMode & RECORD_MODE))
		if(live_channel)
			zapit(live_channel->getChannelID(), current_is_nvod);
#endif
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

/*
unsigned int CZapit::zapTo_record(const t_channel_id channel_id)
{
	unsigned int zapStatus = 0;
			
	zapStatus = zapToRecordID(channel_id);
		
	return zapStatus;
}
*/

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

void CZapit::getPIDS( responseGetPIDs &pids )
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
		
		sendAPIDs(apids);
		sendSubPIDs(subpids);
		
		//
		pids.PIDs = responseGetOtherPIDs;
		pids.APIDs = apids;
		pids.SubPIDs = subpids;
	}
}

t_channel_id CZapit::getCurrentServiceID()
{
	return (live_channel != 0) ? live_channel->getChannelID() : 0;
}

CZapit::CCurrentServiceInfo CZapit::getCurrentServiceInfo()
{
	CCurrentServiceInfo msgCurrentServiceInfo;
	memset(&msgCurrentServiceInfo, 0, sizeof(CCurrentServiceInfo));
			
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

bool CZapit::getCurrentTP(TP_params *TP)
{
	//#FIXME:
	return true;
}

void CZapit::setSubServices( subServiceList& subServices )
{
	//#FIXME:
	#if 0
	t_satellite_position  satellitePosition = 0;
	freq_id_t freq = 0;

	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
	{
		if(it->second.getServiceId() == subServices.service_id)
		{
			satellitePosition = it->second.getSatellitePosition();
			freq = it->second.getFreqId();
		}
	}
	
	for (i = 0; i < subServices.size(); i++)
	{
		nvodchannels.insert (
				std::pair <t_channel_id, CZapitChannel> (
					CREATE_CHANNEL_ID(subServices.service_id, subServices.original_network_id, subServices.transport_stream_id, satellitePosition, freq),
					CZapitChannel (
					"NVOD",
					subServices.service_id,
					subServices.transport_stream_id,
					subServices.original_network_id,
					ST_DIGITAL_TELEVISION_SERVICE,
					satellitePosition,
					freq) //FIXME: global for more than one tuner???
				)
				);
	}
	#endif
	current_is_nvod = true;
}

void CZapit::setRecordMode(const bool activate)
{		
	if (activate)
		setRecordMode();
	else
		unsetRecordMode();
}

bool CZapit::isRecordModeActive()
{
	bool activated = (currentMode & RECORD_MODE);
	
	return activated;
}

t_channel_id CZapit::getRecordServiceID()
{
	return (rec_channel != 0) ? rec_channel->getChannelID() : 0;
}

CZapit::CCurrentServiceInfo CZapit::getRecordServiceInfo()
{
	CCurrentServiceInfo msgRecordServiceInfo;
	memset(&msgRecordServiceInfo, 0, sizeof(CCurrentServiceInfo));
			
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
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED, CEventServer::INITID_NEUTRINO);
}

void CZapit::reloadCurrentServices()
{
	eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED, CEventServer::INITID_NEUTRINO);
}

void CZapit::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2, int feindex)
{
	if(cmdtype > 0x20)
		getFE(feindex)->sendMotorCommand(cmdtype, address, cmd, num_parameters, param1, param2);
}

//
bool CZapit::reZap()
{
	bool ret = false;
	
	if ( !(currentMode & RECORD_MODE) )
	{		
		if(live_channel)
			ret = zapit(live_channel->getChannelID(), current_is_nvod);
	}
	
	return ret;
}

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

/*
bool CZapit::getChannels(BouquetChannelList& channels, const channelsMode mode, const channelsOrder order, const bool utf_encoded)
{
#if 0
	if (utf_encoded)
		sendNChannels(channels, mode, order);
	else
		sendChannels(channels, mode, order);
#endif
	
	return true;
}
*/

/*
void CZapit::getBouquets( BouquetList& bouquets, const bool emptyBouquetsToo, const bool utf_encoded, channelsMode mode)
{
	sendBouquets(bouquets, emptyBouquetsToo, mode);
}
*/

bool CZapit::getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, const channelsMode mode, const bool utf_encoded)
{
	sendBouquetChannels(channels, bouquet, mode);
	
	return true;
}

bool CZapit::getBouquetNChannels(const unsigned int bouquet, BouquetNChannelList& channels, const channelsMode mode, const bool utf_encoded)
{
	sendBouquetNChannels(channels, bouquet, mode);
	
	return true;
}

// bqt
void CZapit::saveBouquets()
{
	g_bouquetManager->saveBouquets();
	g_bouquetManager->saveUBouquets();
			
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED, CEventServer::INITID_NEUTRINO);
	
	if(g_list_changed) 
	{
		CServices::getInstance()->saveServices(true); //FIXME
		g_list_changed = 0;
	}
}

void CZapit::restoreBouquets()
{
	if(g_list_changed) 
	{
		prepareChannels();
				
		g_list_changed = 0;
	} 
	else 
	{
		g_bouquetManager->clearAll();
		g_bouquetManager->loadBouquets();
	}
}

void CZapit::addBouquet(const char * const name)
{
	g_bouquetManager->addBouquet(name, true);
}

void CZapit::deleteBouquet(const unsigned int bouquet)
{
	g_bouquetManager->deleteBouquet(bouquet);
}

void CZapit::renameBouquet(const unsigned int bouquet, const char * const newName)
{
	if (bouquet < g_bouquetManager->Bouquets.size()) 
	{
		g_bouquetManager->Bouquets[bouquet]->Name = newName;
		g_bouquetManager->Bouquets[bouquet]->bUser = true;
	}
}

void CZapit::moveBouquet(const unsigned int bouquet, const unsigned int newPos)
{
	g_bouquetManager->moveBouquet(bouquet, newPos);
}

void CZapit::moveChannel(const unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	if (bouquet < g_bouquetManager->Bouquets.size())
		g_bouquetManager->Bouquets[bouquet]->moveService(oldPos, newPos,
						(((currentMode & RADIO_MODE) && mode == MODE_CURRENT)
						|| (mode == MODE_RADIO)) ? 2 : 1);
}

signed int CZapit::existsBouquet(const char * const name)
{
	return g_bouquetManager->existsBouquet(name);
}

void CZapit::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	if (bouquet < g_bouquetManager->Bouquets.size())
		g_bouquetManager->Bouquets[bouquet]->bLocked = lock;
}

void CZapit::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	if (bouquet < g_bouquetManager->Bouquets.size())
		g_bouquetManager->Bouquets[bouquet]->bHidden = hidden;
}

bool CZapit::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	return g_bouquetManager->existsChannelInBouquet(bouquet, channel_id);
}

void CZapit::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	if (bouquet < g_bouquetManager->Bouquets.size())
		g_bouquetManager->Bouquets[bouquet]->removeService(channel_id);

	bool status = 0;
	for (unsigned int i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{
		status = g_bouquetManager->existsChannelInBouquet(i, channel_id);
		if(status)
			break;
	}
	
	if(!status) 
	{
		allchans.erase(channel_id);
		live_channel = 0;
		g_list_changed = 1;
	}
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SERVICESCHANGED, CEventServer::INITID_NEUTRINO);
}

// scan
bool CZapit::tuneTP(TP_params TP, int feindex)
{
	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::tuneTP: fe(%d)\n", feindex);
	
	bool ret = false;
	
	//		
	initTuner(getFE(feindex));
			
	// satname
	const char *name = scanProviders.size() > 0  ? scanProviders.begin()->second.c_str() : "unknown";
			
	t_satellite_position satellitePosition = scanProviders.begin()->first;
	
	dprintf(DEBUG_NORMAL, "CZapit::tuneTP: (fe:%d delsys:0x%x) satname:%s satpos:%d (TP.delsys:0x%x)\n", feindex, getFE(feindex)->getForcedDelSys(), name, satellitePosition, TP.feparams.delsys);
	
	// setinput
	getFE(feindex)->setInput(satellitePosition, TP.feparams.frequency, TP.polarization);
	
	// drivetosatpos
	if (getFE(feindex)->getInfo()->type == FE_QPSK)
		getFE(feindex)->driveToSatellitePosition(satellitePosition);
		
	// tuneFreq
	ret = getFE(feindex)->tuneFrequency(&TP.feparams, TP.polarization, true);
			
	// set retune flag
	retune = true;
	
	return ret;
}

//
bool CZapit::scanTP(commandScanTP &msg)
{
	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::scanTP fe:(%d) scanmode:%d\n", msg.feindex, msg.scanmode);
	
	bool ret = true;
	
	CZapit::stopPlayBack();
				
	// stop update pmt filter
	CPmt::getInstance()->pmt_stop_update_filter(&pmt_update_fd);
	
	scan_runs = 1;
	
	if (pthread_create(&scan_thread, 0, scanTransponderThread, (void *) &msg)) 
	{
		dprintf(DEBUG_INFO, "CZapit::scanTP: pthread_create\n");
		scan_runs = 0;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED, CEventServer::INITID_NEUTRINO);
		
		ret = false;
	} 
			
	retune = true;
	
	return ret;
}

//
bool CZapit::isScanReady(unsigned int &satellite, unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services )
{
	bool scanReady = false;
	
	satellite = curr_sat;
	transponder = found_transponders;
	processed_transponder = processed_transponders;
	services = found_channels;
	
	if (scan_runs > 0)
	{
		scanReady = false;
		dprintf(DEBUG_INFO, "CZapit::isScanReady: scan not ready\n");
	}
	else
	{
		scanReady = true;
		dprintf(DEBUG_INFO, "CZapit::isScanReady: scan ready\n");
	}
	
	return scanReady;
}

void CZapit::getScanSatelliteList( SatelliteList &satelliteList )
{
	CZapit::responseGetSatelliteList sat;
	
	sat_iterator_t sit;
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		strncpy(sat.satName, sit->second.name.c_str(), 50);
		sat.satName[49] = 0;
		sat.satPosition = sit->first;
		sat.motorPosition = sit->second.motor_position;

		sat.system = sit->second.system;
		
		satelliteList.push_back(sat);
	}
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

void CZapit::setDiseqcType(const diseqc_t diseqc, int feindex)
{
	if( getFE(feindex)->getInfo()->type == FE_QPSK)
	{
		getFE(feindex)->setDiseqcType(diseqc );
	}
}

void CZapit::setDiseqcRepeat(const uint32_t repeat, int feindex)
{		
	if( getFE(feindex)->getInfo()->type == FE_QPSK)
	{
		getFE(feindex)->setDiseqcRepeats(repeat);
		dprintf(DEBUG_INFO, "CZapit::setDiseqcRepeat: repeats to %d", repeat);
	}
}

void CZapit::setScanMotorPosList( ScanMotorPosList& motorPosList )
{
	bool changed = false;
	
	/*
	for (uint32_t i = 0; i < motorPosList.size(); i++)
	{
		changed |= (motorPositions[pos.satPosition] != pos.motorPos);
		motorPositions[pos.satPosition] = pos.motorPos;
	}
	*/
	//#FIXME
	
	//if (changed) 
	//	SaveMotorPositions();
}

//
bool CZapit::startScan(commandScanProvider &msg)
{		
	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::startScan: fe(%d) scanmode: %d\n", msg.feindex, msg.scanmode);
	
	bool ret = true;
	
	scan_runs = 1;
	
	//stop playback
	stopPlayBack();
	
	// stop pmt update filter
    	CPmt::getInstance()->pmt_stop_update_filter(&pmt_update_fd);	

	if (pthread_create(&scan_thread, 0, scanThread, (void *) &msg)) 
	{
		dprintf(DEBUG_INFO, "CZapit::startScan: pthread_create failed\n");
		scan_runs = 0;
		
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED, CEventServer::INITID_NEUTRINO);
		
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
	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::scanThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	CZapit::commandScanProvider params = *(CZapit::commandScanProvider*)data;
	
	int mode = params.scanmode;
	int feindex = params.feindex;
	
	scan_list_iterator_t spI;
	char providerName[80] = "";
	char *frontendType;
	uint8_t diseqc_pos = 0;
	scanBouquetManager = new CBouquetManager();
	processed_transponders = 0;
	failed_transponders = 0;
	found_transponders = 0;
 	found_tv_chans = 0;
 	found_radio_chans = 0;
 	found_data_chans = 0;
	found_channels = 0;
	bool satfeed = false;
	
	curr_sat = 0;
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	scanmode = mode & 0xFF;	// NIT (0) or fast (1)
	scan_sat_mode = mode & 0xFF00; 	// single = 0, all = 1

	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::scanThread: fe(%d) scan mode %s, satellites %s\n", feindex, scanmode ? "fast" : "NIT", scan_sat_mode ? "all" : "single");

	fake_tid = fake_nid = 0;
	
	//
	xmlDocPtr scanInputParser = NULL;

#if HAVE_DVB_API_VERSION >= 5 
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C)
#else
	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM )
#endif
	{
		frontendType = (char *) "cable";
		scanInputParser = parseXmlFile(CABLES_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) 
#endif
	{
		frontendType = (char *) "terrestrial";
		scanInputParser = parseXmlFile(TERRESTRIALS_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
	{
		frontendType = (char *) "sat";
		scanInputParser = parseXmlFile(SATELLITES_XML);
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_A)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
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
    			if ( (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C) || (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T) || (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2) || (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_A) )
#else
			if( ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM) || ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) || ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC) )
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
				// get name of current satellite oder cable provider
				strcpy(providerName, xmlGetAttribute(search,  "name"));

				// satfeed
#if HAVE_DVB_API_VERSION >= 5
    				if ( (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2 || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_A) && xmlGetAttribute(search, "satfeed") )
#else
				if( ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM || CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM) && xmlGetAttribute(search, "satfeed") )
#endif
				{
					if (!strcmp(xmlGetAttribute(search, "satfeed"), "true"))
						satfeed = true;
				}

				// increase sat counter
				curr_sat++;

				scantransponders.clear();
				scanedtransponders.clear();
				nittransponders.clear();

				dprintf(DEBUG_INFO, "CZapit::scanThread: scanning %s at %d bouquetMode %d\n", providerName, position, _bouquetMode);
				
				// this invoke addToScan	
				if ( !CScan::getInstance()->scanProvider(search, position, diseqc_pos, satfeed, feindex) )
				{
					found_channels = 0;
					break;
				}
						
				if(abort_scan) 
				{
					found_channels = 0;
					break;
				}

				if(scanBouquetManager->Bouquets.size() > 0) 
				{
					scanBouquetManager->saveBouquets(_bouquetMode, providerName);
				}
						
				scanBouquetManager->clearAll();
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
		CServices::getInstance()->saveServices(true);
		
		dprintf(DEBUG_NORMAL, "CZapit::scanThread: save services done\n"); 
		
	        g_bouquetManager->saveBouquets();
	        g_bouquetManager->clearAll();
		g_bouquetManager->loadBouquets();
		
		dprintf(DEBUG_INFO, "CZapit::scanThread: save bouquets done\n");
		
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_COMPLETE, CEventServer::INITID_NEUTRINO);
		
		if (scanBouquetManager) 
		{
			scanBouquetManager->clearAll();
			delete scanBouquetManager;
			scanBouquetManager = NULL;
		}

		eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED, CEventServer::INITID_NEUTRINO);
	} 
	else 
	{
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED, CEventServer::INITID_NEUTRINO);
		
		if (scanBouquetManager) 
		{
			scanBouquetManager->clearAll();
			delete scanBouquetManager;
			scanBouquetManager = NULL;
		}
	}

	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	pthread_exit(NULL);
}

//
void * CZapit::scanTransponderThread(void * data)
{
	dprintf(DEBUG_NORMAL, ANSI_MAGENTA "CZapit::scanTransponderThread: starting... tid %ld\n", syscall(__NR_gettid));
	
	CZapit::commandScanTP params = *(CZapit::commandScanTP*)data;
	
	TP_params * TP = &params.TP;
	scanmode = params.scanmode;
	int feindex = params.feindex;
	
	char providerName[32] = "";
	t_satellite_position satellitePosition = 0;

	prov_found = 0;
        found_transponders = 0;
        found_channels = 0;
        processed_transponders = 0;
        found_tv_chans = 0;
        found_radio_chans = 0;
        found_data_chans = 0;
	fake_tid = fake_nid = 0;
	scanBouquetManager = new CBouquetManager();
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	// provider name
	strcpy(providerName, scanProviders.size() > 0 ? scanProviders.begin()->second.c_str() : "unknown provider");

	// satpos
	satellitePosition = scanProviders.begin()->first;
	
	dprintf(DEBUG_NORMAL, "CZapit::scanTransponderThread: (fe:%d delsys:0x%x) scanning sat: %s position: %d fe(%d)\n", feindex, CZapit::getInstance()->getFE(feindex)->getForcedDelSys(), providerName, satellitePosition, feindex);
	
	eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_SATELLITE, CEventServer::INITID_NEUTRINO, providerName, strlen(providerName) + 1);

	//
	freq_id_t freq;

#if HAVE_DVB_API_VERSION >= 5 
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C)
#else
	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM )
#endif
		freq = TP->feparams.frequency/100;
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	else if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
		freq = TP->feparams.frequency/1000;
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) 
#endif
		freq = TP->feparams.frequency/1000000;

	// add TP to scan
	fake_tid++; 
	fake_nid++;

	CScan::getInstance()->addToScan(CREATE_TRANSPONDER_ID(freq, satellitePosition, fake_nid, fake_tid), &TP->feparams, TP->polarization, false, feindex);

	// scanSDTS
	if (!CScan::getInstance()->getSDTS(satellitePosition, feindex))
		found_channels = 0;

	if(abort_scan)
		found_channels = 0;

	if(found_channels) 
	{
		CServices::getInstance()->saveServices(true);
		
		scanBouquetManager->saveBouquets(_bouquetMode, providerName);
	        g_bouquetManager->saveBouquets();
	        g_bouquetManager->clearAll();
		g_bouquetManager->loadBouquets();
		
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_COMPLETE, CEventServer::INITID_NEUTRINO);
		
		if (scanBouquetManager) 
		{
			scanBouquetManager->clearAll();
			delete scanBouquetManager;
			scanBouquetManager = NULL;
		}

		eventServer->sendEvent(NeutrinoMessages::EVT_BOUQUETSCHANGED, CEventServer::INITID_NEUTRINO);
	} 
	else 
	{
		// notify client about end of scan
		scan_runs = 0;
		eventServer->sendEvent(NeutrinoMessages::EVT_SCAN_FAILED, CEventServer::INITID_NEUTRINO);
		
		if (scanBouquetManager) 
		{
			scanBouquetManager->clearAll();
			delete scanBouquetManager;
			scanBouquetManager = NULL;
		}
	}
	
	scantransponders.clear();
	scanedtransponders.clear();
	nittransponders.clear();

	pthread_exit(NULL);
}

//
void CZapit::Start(Z_start_arg *ZapStart_arg)
{
	dprintf(DEBUG_NORMAL, "CZapit::Start\n");	
	
	//scan for dvb adapter/frontend and feed them in map
	initFrontend();
	
#if defined (__sh__)
	if(FrontendCount > 1)
	{
		//lib-stb-hal/libspark
		/* 
		* this is a strange hack: the drivers seem to only work correctly after
		* demux0 has been used once. After that, we can use demux1,2,... 
		*/
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
		
	// video/audio decoder
	int video_mode = ZapStart_arg->video_mode;
	
	// video decoder
#if defined (PLATFORM_COOLSTREAM)
	videoDecoder = new cVideo(video_mode, videoDemux->getChannel(), videoDemux->getBuffer());
	videoDecoder->Standby(false);
	
	audioDecoder = new cAudio(audioDemux->getBuffer(), videoDecoder->GetTVEnc(), NULL);
	videoDecoder->SetAudioHandle(audioDecoder->GetHandle());
#else	
	videoDecoder = new cVideo();
		
	// set video system
	if(videoDecoder)
		videoDecoder->SetVideoSystem(video_mode);	
	
	// audio decoder
	audioDecoder = new cAudio();
#endif	

	//CI init
#if defined (ENABLE_CI)	
	ci = cCA::GetInstance();
	ci->Start();
#endif	
	
	//globals
	scan_runs = 0;
	found_channels = 0;
	curr_sat = 0;

	// load configuration or set defaults if no configuration file exists
	loadZapitSettings();

	//create Bouquet Manager
	g_bouquetManager = new CBouquetManager();
	
	// load services
	prepareChannels();
	
	//start channel
	if(ZapStart_arg->uselastchannel == 0)
	{
		// mode
		if (ZapStart_arg->lastchannelmode == RADIO_MODE)
			setRadioMode();
		else if (ZapStart_arg->lastchannelmode == TV_MODE)
			setTVMode();
		
		// live channel id
		if (currentMode & RADIO_MODE)
			live_channel_id = ZapStart_arg->startchannelradio_id;
		else if (currentMode & TV_MODE)
			live_channel_id = ZapStart_arg->startchanneltv_id;

		lastChannelRadio = ZapStart_arg->startchannelradio_nr;
		lastChannelTV    = ZapStart_arg->startchanneltv_nr;
	}
	else
	{
		if (lastChannelMode == RADIO_MODE)
			setRadioMode();
		else if (lastChannelMode == TV_MODE)
			setTVMode();
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
	//pthread_cancel(tsdt);
	//pthread_join(tsdt, NULL);
	
	// stop pmt update filter thread
	//pthread_cancel(tpmt);
	//pthread_join(tpmt, NULL);
	
	////
#if defined (ENABLE_CI)
	if (ci) 
	{

		ci->Stop();
		delete ci;
	}
#endif
	////

	if (pmtDemux)
		delete pmtDemux;
	
	if(audioDecoder)
		delete audioDecoder;
	
	if(videoDecoder)
		delete videoDecoder;

	//close frontend	
	for(fe_map_iterator_t it = femap.begin(); it != femap.end(); it++)
		delete it->second;
}

