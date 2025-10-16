//
// $Id: zapit.h 20.10.2023 mohousch Exp $
//
// zapit - d-box2 linux project
//
// (C) 2001, 2002 by Philipp Leusmann <faralla@berlios.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef __zapit__
#define __zapit__


#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <ctype.h>

#include <inttypes.h>

// libxmltree
#include <lib/libxmltree/xmlinterface.h>

// zapit
#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>
#include <zapit/channel.h>
#include <zapit/cam.h>
#include <zapit/bouquets.h>
#include <zapit/nit.h>
#include <zapit/pat.h>
#include <zapit/sdt.h>
#include <zapit/pmt.h>


//// settings
#define SERVICES_XML    		CONFIGDIR "/zapit/services.xml"
#define BOUQUETS_XML    		CONFIGDIR "/zapit/bouquets.xml"
#define UBOUQUETS_XML    		CONFIGDIR "/zapit/ubouquets.xml"
#define EPG_MAPPING_XML			CONFIGDIR "/zapit/epgmap.xml"
// services
#define SERVICES_TMP    		"/tmp/services.tmp"
// sdt update
#define CURRENTSERVICES_XML     	"/tmp/currentservices.xml"
#define CURRENTSERVICES_TMP     	"/tmp/currentservices.tmp"
// providers
#define CABLES_XML      		CONFIGDIR "/cables.xml"
#define SATELLITES_XML  		CONFIGDIR "/satellites.xml"
#define TERRESTRIALS_XML 		CONFIGDIR "/terrestrial.xml"
#define ATSC_XML                	CONFIGDIR "/atsc.xml"
// zapit/frontend/audio
#define ZAPIT_CONFIGFILE      		CONFIGDIR "/zapit/zapit.conf"
#define SATCONFIG			CONFIGDIR "/zapit/sat.conf"
#define FRONTEND_CONFIGFILE 		CONFIGDIR "/zapit/frontend.conf"
#define VOLUME_CONFIG_FILE 		CONFIGDIR "/zapit/audiovolume.conf"
#define AUDIO_CONFIG_FILE 		CONFIGDIR "/zapit/audio.conf"
// frontend
#define DVBADAPTER_MAX			4
#define FRONTEND_MAX			8
////
#define CHANNEL_NAME_SIZE 		40


////
class CZapit
{
	public:
		enum zapitMode
		{
			TV_MODE 	= 0x01,
			RADIO_MODE 	= 0x02,
			RECORD_MODE 	= 0x04,
		};
		
		enum zapStatus
		{
			ZAP_OK 			= 0x01,
			ZAP_IS_NVOD 		= 0x02,
			ZAP_INVALID_PARAM 	= 0x04
		};

		enum bouquetMode
		{
			BM_DELETEBOUQUETS,
			BM_UPDATEBOUQUETS
		};

		enum scanType
  		{
  			ST_TVRADIO,
  			ST_TV,
  			ST_RADIO,
  			ST_ALL
  		};
  		
  		enum scanMode
  		{
  			SM_NIT,
  			SM_FAST
  		};

		typedef enum channelsMode_
		{
			MODE_CURRENT,
			MODE_TV,
			MODE_RADIO
		} channelsMode;

		struct subServices
		{
			t_original_network_id original_network_id;
			t_service_id          service_id;
			t_transport_stream_id transport_stream_id;
		};
		
		typedef std::vector<subServices> subServiceList;

		struct lastChannel
		{
			t_channel_id id;
			unsigned int channelNumber;
			int mode;
		};

		struct bouquetChannels
		{
			unsigned int nr;
			t_channel_id channel_id;
			char name[CHANNEL_NAME_SIZE];
			t_satellite_position satellitePosition;
			unsigned char service_type;
		};
		
		typedef std::vector<bouquetChannels> BouquetChannelList;

		struct APIDs
		{
			uint32_t    pid;
			char    desc[25];
			int     is_ac3;
			int     component_tag;
		};
		
		typedef std::vector<APIDs> APIDList;
		
		struct SubPIDs
		{
			uint    pid;
			char    desc[4];
			uint	composition_page;
			uint	ancillary_page;
			bool	hearingImpaired;
		};
		
		typedef std::vector<SubPIDs> SubPIDList;

		struct OtherPIDs
		{
			uint		vpid;
			uint		vtxtpid;
			uint		pcrpid;
			uint		selected_apid;
			uint		pmtpid;
			uint		privatepid;
		};
		
		struct PIDs
		{
			OtherPIDs	otherPIDs;
			APIDList	APIDs;
			SubPIDList  	SubPIDs;
		};

		class CServiceInfo
		{
			public:
				t_original_network_id onid;
				t_service_id           sid;
				t_transport_stream_id tsid;
				unsigned short	vpid;
				unsigned short  vtype;
				unsigned short	apid;
				unsigned short	pcrpid;
				unsigned short	vtxtpid;
				unsigned short  pmtpid;
				unsigned short  pmt_version;
				unsigned int	tsfrequency;
				unsigned char	polarisation;
				uint32_t	rate;
				fe_code_rate	fec;
		};
		
		struct scanSatelliteList_t
		{
			char satName[50];
			t_satellite_position  position;
		};
		
		typedef std::vector<scanSatelliteList_t> ScanSatelliteList;
		
		//
		struct scanTP_t
		{
			transponder TP;
			int scanmode;
			CFrontend *fe;
		};
		
		struct scanProvider_t
		{
			int scanmode;
			CFrontend *fe;
		};
		
		typedef struct zapit_config 
		{
			bool makeRemainingChannelsBouquet;
			int scanSDT;
			bool saveLastChannel;
			int lastChannelMode;
			t_channel_id lastChannelTV_id;
			t_channel_id lastChannelRadio_id;
			uint32_t lastChannelTV;
			uint32_t lastChannelRadio;
		} zapit_config_t;
		
		// cammanager
		CCam * cam0;
		CCam * cam1;
		//// femanager
		fe_map_t femap;
		CFrontend * live_fe;
		CFrontend * record_fe;
		bool have_s;
		bool have_c;
		bool have_t;
		bool have_a;
		//
		t_channel_id live_channel_id;
		t_channel_id rec_channel_id;
		CZapitChannel * live_channel;
		CZapitChannel * rec_channel;
		//
		bool standby;
		bool retune;
		//
		int currentMode;
		bool playbackStopForced;
		bool avDecoderOpen;
		//
		bool sdt_wakeup;
		sdt_tp_t sdt_tp;
		//
		bool firstzap;
		bool playing;
		bool g_list_changed;
		//
		bool current_is_nvod;
		//
		unsigned int volume_left;
		unsigned int volume_right;
		int audio_mode;
		int def_audio_mode;
		//
		int volume_percent;
		//
		bool saveLastChannel;
		int lastChannelMode;
		uint32_t  lastChannelRadio;
		t_channel_id lastChannelRadio_id;
		uint32_t  lastChannelTV;
		t_channel_id lastChannelTV_id;
		bool makeRemainingChannelsBouquet;
		int scanSDT;
		//// channelManager
		int newtpid;
		int tcnt;
		int scnt;
		uint32_t fake_tid;
		uint32_t fake_nid;
		//// scanManager
		bouquetMode _bouquetMode;
		scanType _scanType;
		////bouquetManager
		BouquetList Bouquets;
		
	private:
		CZapitBouquet* remainChannels;

		//
		void makeRemainingChannelsBouquets(void);
		void parseBouquetsXml(const char* fname, bool ub = false);
		void writeBouquetHeader(FILE * bouq_fd, uint32_t i, const char * bouquetName);
		void writeBouquetFooter(FILE * bouq_fd);
		void writeBouquetChannels(FILE * bouq_fd, uint32_t i);
		void makeBouquetfromCurrentservices (const xmlNodePtr root);
		
	public:
		class ChannelIterator
		{
			private:
				CZapit * Owner;
				CZapit::channelsMode mode;
				unsigned int b;
				int c;
				
				ZapitChannelList *getBouquet() 
				{ 
					ZapitChannelList * ret = &(Owner->Bouquets[b]->tvChannels);
					
					if (mode == CZapit::MODE_TV)
						ret = &(Owner->Bouquets[b]->tvChannels);
					else if (mode == CZapit::MODE_RADIO)
						ret = &(Owner->Bouquets[b]->radioChannels);
						
					return ret;
				};
				
			public:
				ChannelIterator(CZapit *owner = NULL, const CZapit::channelsMode Mode = CZapit::MODE_TV);
				ChannelIterator operator ++(int);
				CZapitChannel* operator *();
				////
				int getLowestChannelNumberWithChannelID(const t_channel_id channel_id);
				int getNrofFirstChannelofBouquet(const unsigned int bouquet_nr);
				bool EndOfChannels() { return (c == -2); };
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, CZapit::MODE_TV); };
		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, CZapit::MODE_RADIO); };
		
		////
		void saveZapitBouquets(void);
		void saveZapitUBouquets(void);
		void loadBouquets(bool loadCurrentBouquet = false);
		CZapitBouquet* addBouquet(const std::string& name, bool ub = false, bool webtvb = false);
		CZapitBouquet *addBouquet(const std::string &name, BouquetList &list, bool ub = false, bool webtvb = false);
		CZapitBouquet* addBouquetIfNotExist(const std::string& name, bool ub = false, bool webtvb = false);
		CZapitBouquet* addBouquetIfNotExist(const std::string& name, BouquetList &list, bool ub = false, bool webtvb = false);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CZapitBouquet* bouquet);
		int  existsBouquet(const char * const name);
		int existsBouquet(const char * const name, BouquetList &list);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet(unsigned int bq_id, const t_channel_id channel_id);
		void renameBouquet(const unsigned int bouquet, const char * const newName); // UTF-8 encoded
		void setBouquetLock(const unsigned int bouquet, const bool lock);
		void setBouquetHidden(const unsigned int bouquet, const bool hidden);
		void clearAll();
		void parseWebTVBouquet(std::string &filename);
		void loadWebTVBouquets(const std::string &dirname);
		void sortBouquets(void);
		
	private:
		//
		void OpenFE(void);
		void CloseFE(void);
		void resetFE(void);
		//
		void loadAudioMap();
		void saveAudioMap();
		void loadVolumeMap();
		void saveVolumeMap();
		void saveZapitSettings(bool write, bool write_a);
		void loadZapitSettings();
		void sendCaPmtPlayBackStart(CZapitChannel * thischannel, CFrontend * fe);
		void sendcapmtPlayBackStop(bool _sendPmt);
		void sendCaPmtRecordStop(void);
		void saveChannelPids(CZapitChannel * thischannel);
		CZapitChannel * findChannelToZap(const t_channel_id channel_id, bool in_nvod = false);
		bool tuneToChannel(CFrontend * frontend, CZapitChannel * thischannel, bool &transponder_change);
		bool parseChannelPatPmt(CZapitChannel * thischannel, CFrontend * fe);
		void restoreChannelPids(CZapitChannel * thischannel);
		int zapit(const t_channel_id channel_id, bool in_nvod, bool forupdate = 0);
		void setPidVolume(t_channel_id channel_id, int pid, int percent);
		int getPidVolume(t_channel_id channel_id, int pid, bool ac3);
		void setVolPercent(int percent);
		int changeAudioPid(uint8_t index);
		void enableRecordMode(void);
		void disableRecordMode(void);
		void setRadioMode(void);
		void setTVMode(void);
		int prepareChannels();
		void renumServices(void);
		void readEPGMapping();
		//
		unsigned int zapToChannelID(const t_channel_id channel_id, const bool isSubService);
		void sendAPIDs(t_channel_id chid, APIDList &apids);
		void sendSubPIDs(t_channel_id chid, SubPIDList &subpids);
		//
		void internalSendChannels(ZapitChannelList *channels, const unsigned int first_channel_nr, BouquetChannelList &Bchannels);
		void sendBouquetChannels(BouquetChannelList &Bchannels, const unsigned int bouquet, const channelsMode mode);
		//// channelManager
		void parseTransponders(xmlNodePtr node, t_satellite_position satellitePosition, fe_type_t frontendType);
		void parseChannels(xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq);
		void findTransponder(xmlNodePtr root);
		void parseSatTransponders(fe_type_t frontendType, xmlNodePtr search, t_satellite_position satellitePosition);
		void initSat(t_satellite_position position);
		int loadTransponders();
		int loadServices(bool only_current);
		void saveServices(bool tocopy = false);
		//// scanManager
		bool tuneFrequency(FrontendParameters *feparams, t_satellite_position satellitePosition, CFrontend* fe);
		bool getSDTS(t_satellite_position satellitePosition, CFrontend* fe);
		bool scanTransponder(xmlNodePtr transponder, t_satellite_position satellitePosition, CFrontend* fe);
		bool scanProvider(xmlNodePtr search, t_satellite_position satellitePosition, CFrontend* fe);
		void saveScanBouquets(const CZapit::bouquetMode bouquetMode, const char * const providerName);
		//
		void closeAVDecoder(void);
		void openAVDecoder(void);
		void enterStandby(void);
		void leaveStandby(void);
		//
		pthread_t scan_thread; 
		static void * scanThread(void * data);
		static void * scanTransponderThread(void * data);
		//
		pthread_t tsdt;
		static void * sdtThread(void * arg);
		//
		pthread_t tpmt;
		static void *updatePMTFilter(void *);
		
		CZapit();
		
	public:
		virtual ~CZapit(){ Stop();};
		
		static CZapit *getInstance()
		{
			static CZapit * zapit = NULL;

			if(!zapit) 
			{
				zapit = new CZapit();
			} 

			return zapit;
		};
		
		//
		void initFrontend();
		CFrontend * getFE(int index);
		void initTuner(CFrontend * fe);
		void saveFrontendConfig();
		void loadFrontendConfig();
		int getFrontendCount(void){return femap.size();};
		bool loopCanTune(CFrontend * fe, CZapitChannel * thischannel);
		bool CanZap(CZapitChannel * thischannel);
		bool FrontendIsTwin(CFrontend* fe);
		CFrontend * getFreeFrontend(CZapitChannel * thischannel);
		CFrontend * getFrontend(CZapitChannel * thischannel);
		CFrontend * getRecordFrontend(CZapitChannel * thischannel);
		void lockFrontend(CFrontend *fe);
		void unlockFrontend(CFrontend *fe);
		//
		void setZapitConfig(zapit_config * Cfg);
		void getZapitConfig(zapit_config *Cfg);
		//
		void getLastChannel(t_channel_id &channelid, unsigned int &channumber, int &mode);
		int getMode(void);
		void setMode(const channelsMode mode);
		// playback
		int startPlayBack(CZapitChannel *);
		int stopPlayBack(bool sendPmt = false);
		void pausePlayBack(void);
		void continuePlayBack(void);
		void lockPlayBack();
		void unlockPlayBack();
		bool isPlayBackActive();
		void setStandby(bool enable);
		bool isStandbyActive(){return standby;};
		//
		void zapToServiceIDNOWAIT(const t_channel_id channel_id);
		void zapToSubServiceIDNOWAIT(const t_channel_id channel_id);
		unsigned int zapToServiceID(const t_channel_id channel_id);
		unsigned int zapToSubServiceID(const t_channel_id channel_id);
		int zapToRecordID(const t_channel_id channel_id);
		//
		CZapitChannel* findChannelByChannelID(const t_channel_id channel_id);
		CZapitChannel* findChannelByName(std::string name, const t_service_id sid);
		std::string getChannelName(const t_channel_id channel_id);
		int getChannelNumber(const t_channel_id channel_id);
		std::string getChannelURL(const t_channel_id channel_id);
		std::string getChannelDescription(const t_channel_id channel_id);
		t_channel_id getChannelEPGID(const t_channel_id channel_id);
		t_channel_id getChannelLogoID(const t_channel_id channel_id);
		bool isChannelTVChannel(const t_channel_id channel_id);
		bool isChannelWEBTVChannel(const t_channel_id channel_id);
		bool isChannelRadioChannel(const t_channel_id channel_id);
		std::string getSatelliteName(t_satellite_position position);
		// current service
		CZapitChannel* getCurrentChannel();
		t_channel_id getCurrentChannelID();
		const std::string& getCurrentChannelName(void) const;
		t_channel_id getCurrentChannelEPGID();
		int32_t getCurrentSatellitePosition();
		bool getCurrentTP(transponder *TP);
		CZapit::CServiceInfo getCurrentServiceInfo();
		void getCurrentPIDS(PIDs &pids);
		CFrontend* getCurrentFrontend();
		// novd
		void setSubServices( subServiceList& subServices );
		// record
		void setRecordMode(const bool activate);
		bool isRecordModeActive();
		t_channel_id getRecordChannelID();
		CZapit::CServiceInfo getRecordServiceInfo();
		void getRecordPIDS(PIDs &pids);
		//
		CZapit::CServiceInfo getServiceInfo(t_channel_id chid);
		void getPIDS(t_channel_id chid, PIDs &pids);
		//
		void reinitChannels();
		void reloadCurrentServices();
		// audio / video
		void muteAudio(const bool mute);
		bool getMuteStatus();
		void getAudioMode(int * mode);
		void setAudioMode(int mode);
		void setVolume(const unsigned int left, const unsigned int right);
		void getVolume(unsigned int *left, unsigned int *right);
		void setVolumePercent(const unsigned int percent, t_channel_id channel_id, const unsigned int apid);
		void getVolumePercent(unsigned int *percent, t_channel_id channel_id, const unsigned int apid, const bool is_ac3 = false);
		void setAudioChannel(const unsigned int channel);
		void setVideoSystem(int video_system);
		//// needed by nhttpd controlapi
		bool getBouquetChannels(const unsigned int bouquet, BouquetChannelList &channels, const channelsMode mode = MODE_CURRENT);
		//// bouquetManager
		void saveBouquets();
		void restoreBouquets();
		void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id);
		void removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id);
		//// serviceManager
		int loadMotorPositions(void);
		void saveMotorPositions();
		//// scanManager
		int addToScan(transponder_id_t TsidOnid, FrontendParameters *feparams, bool fromnit = false, CFrontend* fe = NULL);
		bool tuneTP(transponder TP, CFrontend* fe);
		bool scanTP(scanTP_t &msg);
		void setScanSatelliteList( ScanSatelliteList &satelliteList );
		void setScanType(const scanType mode);
		void setFEMode(const CFrontend::fe_mode_t mode, CFrontend* fe);
		void setScanBouquetMode(const bouquetMode mode);
		bool startScan(scanProvider_t &msg);
		bool stopScan();
		//
		void Start();
		void Stop();
};

#endif

