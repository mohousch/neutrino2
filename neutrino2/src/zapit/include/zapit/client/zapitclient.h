/*
  Client-Interface f�r zapit  -   DBoxII-Project

  License: GPL

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __zapitclient__
#define __zapitclient__


#include <string>
#include <vector>

// zapit
#include "zapittypes.h"
#include <connection/basicclient.h>


#define CHANNEL_NAME_SIZE 40

class CZapitClient : public CBasicClient
{
	private:
		virtual unsigned char getVersion() const;
		virtual const char * getSocketName() const;

	public:
		enum events
		{
			FIRST_EVENT_MARKER,           // <- no actual event, needed by pzapit
			EVT_ZAP_COMPLETE = FIRST_EVENT_MARKER,
			EVT_ZAP_COMPLETE_IS_NVOD,
			EVT_ZAP_FAILED,
			EVT_ZAP_SUB_COMPLETE,
			EVT_ZAP_SUB_FAILED,
			EVT_ZAP_MOTOR,
			EVT_RECORDMODE_ACTIVATED,
			EVT_RECORDMODE_DEACTIVATED,
			EVT_SCAN_COMPLETE,
			EVT_SCAN_FAILED,
			EVT_SCAN_NUM_TRANSPONDERS,
			EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS,
 			EVT_SCAN_REPORT_FREQUENCY,
 			EVT_SCAN_REPORT_FREQUENCYP,
 			EVT_SCAN_SERVICENAME,
 			EVT_SCAN_FOUND_A_CHAN,
 			EVT_SCAN_FOUND_TV_CHAN,
 			EVT_SCAN_FOUND_RADIO_CHAN,
 			EVT_SCAN_FOUND_DATA_CHAN,
			EVT_SCAN_SATELLITE,
			EVT_SCAN_NUM_CHANNELS,
			EVT_SCAN_PROVIDER,
			EVT_BOUQUETS_CHANGED,
			EVT_ZAP_CA_CLEAR,
			EVT_ZAP_CA_LOCK,
			EVT_ZAP_CA_FTA,
			EVT_ZAP_CA_ID,
			EVT_SDT_CHANGED,
			EVT_SERVICES_CHANGED,
			EVT_PMT_CHANGED,

			EVT_ZAP_COMPLETE_IS_WEBTV,
			
			LAST_EVENT_MARKER             // <- no actual event, needed by pzapit
		};

		enum zapStatus
		{
			ZAP_OK = 0x01,
			ZAP_IS_NVOD = 0x02,
			ZAP_INVALID_PARAM = 0x04,
			ZAP_IS_WEBTV = 0x08
		};

		enum bouquetMode
		{
			BM_CREATEBOUQUETS,
			BM_DELETEBOUQUETS,
			BM_DONTTOUCHBOUQUETS,
			BM_UPDATEBOUQUETS,
			BM_CREATESATELLITEBOUQUET
		};

		enum scanType
  		{
  			ST_TVRADIO,
  			ST_TV,
  			ST_RADIO,
  			ST_ALL
  		};

		typedef enum channelsMode_
		{
			MODE_CURRENT,
			MODE_TV,
			MODE_RADIO
		} channelsMode;

		typedef enum channelsOrder_
		{
			SORT_ALPHA,
			SORT_BOUQUET
		} channelsOrder;

		struct commandAddSubServices
		{
			t_original_network_id original_network_id;
			t_service_id          service_id;
			t_transport_stream_id transport_stream_id;
		};
		typedef std::vector<commandAddSubServices> subServiceList;

		struct commandSetScanSatelliteList
		{
			char satName[50];
			int  position;
			
			int type;
		};
		typedef std::vector<commandSetScanSatelliteList> ScanSatelliteList;

		struct commandSetScanMotorPosList
		{
			t_satellite_position satPosition;
			int motorPos;
		};
		typedef std::vector<commandSetScanMotorPosList> ScanMotorPosList;

		struct responseGetLastChannel
		{
			unsigned int	channelNumber;
			char		mode;
		};

		struct responseGetBouquets
		{
			unsigned int bouquet_nr;
			char	 name[30];
			bool	 locked;
			bool	 hidden;
		};
		typedef std::vector<responseGetBouquets> BouquetList;

		struct responseChannels
		{
			unsigned int nr;
			t_channel_id channel_id;
			char	 name[CHANNEL_NAME_SIZE];
			t_satellite_position satellitePosition;
			unsigned char service_type;
		};

		struct responseGetBouquetChannels : public responseChannels
		{};

		typedef std::vector<responseGetBouquetChannels> BouquetChannelList;

		struct responseNChannels
		{
			unsigned int nr;
		};

		struct responseGetBouquetNChannels : public responseNChannels
		{};
		typedef std::vector<responseGetBouquetNChannels> BouquetNChannelList;

		struct responseGetAPIDs
		{
			uint32_t    pid;
			char    desc[25];
			int     is_ac3;
			int     component_tag;
		};
		typedef std::vector<responseGetAPIDs> APIDList;
		
		struct responseGetSubPIDs
		{
			uint    pid;
			char    desc[4];
			uint	composition_page;
			uint	ancillary_page;
			bool	hearingImpaired;
		};
		typedef std::vector<responseGetSubPIDs> SubPIDList;

		struct responseGetOtherPIDs
		{
			uint		vpid;
			uint		vtxtpid;
			uint		pcrpid;
			uint		selected_apid;
			uint		pmtpid;
			uint		privatepid;
		};

		class CCurrentServiceInfo
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
				unsigned int	tsfrequency;
				unsigned char	polarisation;
				unsigned char	diseqc;
				unsigned short  pmtpid;
				unsigned short  pmt_version;
				uint32_t	rate;
				fe_code_rate	fec;
		};

		struct responseGetPIDs
		{
			responseGetOtherPIDs	PIDs;
			APIDList		APIDs;
			SubPIDList  		SubPIDs;
		};

		struct responseGetSatelliteList
		{
			char satName[50];
			t_satellite_position satPosition;
			uint8_t motorPosition;
			int satDiseqc;

			delivery_system_t system;
		};
		typedef std::vector<responseGetSatelliteList> SatelliteList;

	public:
		/*****************************/
		/*                           */
		/* shutdown the zapit daemon */
		/*                           */
		/*****************************/
		void shutdown();


		/****************************************/
		/*					*/
		/* general functions for zapping	*/
		/*					*/
		/****************************************/

		/* zaps to channel of specified bouquet */
		/* bouquets are numbered starting at 0 */
		void zapTo(const unsigned int bouquet, const unsigned int channel);

		/* zaps to channel  */
		void zapTo(const unsigned int channel);

		/* zaps to channel, returns the "zap-status" */
		unsigned int zapTo_serviceID(const t_channel_id channel_id);
		
		/* zap to record channel */
		unsigned int zapTo_record(const t_channel_id channel_id);

		/* zaps to subservice, returns the "zap-status" */
		unsigned int zapTo_subServiceID(const t_channel_id channel_id);

		/* zaps to channel, does NOT wait for completion (uses event) */
		void zapTo_serviceID_NOWAIT(const t_channel_id channel_id);

		/* zaps to subservice, does NOT wait for completion (uses event) */
		void zapTo_subServiceID_NOWAIT(const t_channel_id channel_id);

		/* return the current (tuned) ServiceID */
		t_channel_id getCurrentServiceID();
		
		/* return the record (tuned) ServiceID */
		t_channel_id getRecordServiceID();

		/* return the current satellite position */
		int32_t getCurrentSatellitePosition();

		/* get last channel-information */
		void getLastChannel(unsigned int &channumber, char &mode);

		/* audiochan set */
		void setAudioChannel(const unsigned int channel);

		/* gets all bouquets */
		/* bouquets are numbered starting at 0 */
		void getBouquets( BouquetList& bouquets, const bool emptyBouquetsToo = false, const bool utf_encoded = false, channelsMode mode = MODE_CURRENT);

	private:
		bool receive_channel_list(BouquetChannelList& channels, const bool utf_encoded);
		bool receive_nchannel_list(BouquetNChannelList& channels);

	public:
		/* gets all channels that are in specified bouquet */
		/* bouquets are numbered starting at 0 */
		bool getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, const channelsMode mode = MODE_CURRENT, const bool utf_encoded = false);
		bool getBouquetNChannels(const unsigned int bouquet, BouquetNChannelList& channels, const channelsMode mode = MODE_CURRENT, const bool utf_encoded = false);

		/* gets all channels */
		bool getChannels(BouquetChannelList& channels, const channelsMode mode = MODE_CURRENT, const channelsOrder order = SORT_BOUQUET, const bool utf_encoded = false);

		/* request information about a particular channel_id */
		/* channel name */
		std::string getChannelName(const t_channel_id channel_id);

		int getChannelNumber(const t_channel_id channel_id);

		// webtv channel url
		std::string getChannelURL(const t_channel_id channel_id);

		// webtv channel description
		std::string getChannelDescription(const t_channel_id channel_id);

		/* is channel a TV channel ? */
		bool isChannelTVChannel(const t_channel_id channel_id);
		bool isChannelRadioChannel(const t_channel_id channel_id);
		bool isChannelWEBTVChannel(const t_channel_id channel_id);

		/* get the current_TP */
		bool get_current_TP(TP_params *TP);

		/* restore bouquets so as if they where just loaded*/
		void restoreBouquets();

		/* reloads channels and services*/
		void reinitChannels();

		/* called when sectionsd updates currentservices.xml */
		void reloadCurrentServices();

		/* get current APID-List */
		void getPIDS( responseGetPIDs& pids );
		
		/* get record channel APID-List */
		void getRecordPIDS(responseGetPIDs& pids);

		/* get info about the current service */
		CZapitClient::CCurrentServiceInfo getCurrentServiceInfo();
		
		/* get info about the record service */
		CZapitClient::CCurrentServiceInfo getRecordServiceInfo();

		/* transfer SubService-List to zapit */
		void setSubServices( subServiceList& subServices );

		/* set Mode */
		void setMode(const channelsMode mode);

		/* set Mode */
		int getMode();

		/* set RecordMode */
		void setRecordMode(const bool activate);
		
		/* get RecordMode */
		bool isRecordModeActive();

		/* mute audio */
		void muteAudio(const bool mute);
		bool getMuteStatus();

		/* set audio volume */
		void setVolume(const unsigned int left, const unsigned int right);
		void getVolume(unsigned int *left, unsigned int *right);
		
		/* set audio adjustement in percent */
		void setVolumePercent(const unsigned int percent, t_channel_id channel_id = 0, const unsigned int apid = 0);
		void getVolumePercent(unsigned int * percent, t_channel_id channel_id = 0, const unsigned int apid = 0, const bool is_ac3 = false);

		/* get dvb transmission type */
		delivery_system_t getDeliverySystem(int feindex = 0);

		void zaptoNvodSubService(const int num);

		/* send diseqc 1.2 motor command */
		void sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2, int feindex = 0);

		/****************************************/
		/*					*/
		/* Scanning stuff			*/
		/*					*/
		/****************************************/
		/* start TS-Scan */
		bool setConfig(Zapit_config Cfg);
		void getConfig(Zapit_config * Cfg);
		bool Rezap();
		bool startScan(int scan_mode, int feindex = 0);
		bool stopScan();
		/* start manual scan */
		bool scan_TP(TP_params TP, int feindex = 0);

		/* query if ts-scan is ready - response gives status */
		bool isScanReady(unsigned int &satellite, unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services );

		/* query possible satellits*/
		void getScanSatelliteList( SatelliteList &satelliteList );

		/* tell zapit which satellites to scan*/
		void setScanSatelliteList( ScanSatelliteList &satelliteList );

		/* tell zapit stored satellite positions in diseqc 1.2 motor */
		void setScanMotorPosList( ScanMotorPosList& motorPosList );

		/* set diseqcType*/
		void setDiseqcType(const diseqc_t diseqc, int feindex = 0);

		/* set diseqcRepeat*/
		void setDiseqcRepeat(const uint32_t repeat, int feindex = 0);

		/* set diseqcRepeat*/
		void setScanBouquetMode(const bouquetMode mode);

		/* set Scan-Type for channel search */
		void setScanType(const scanType mode);
		
		/* set fe mode */
		void setFEMode(const fe_mode_t mode, int feindex = 0);

		/****************************************/
		/*					*/
		/* Bouquet editing functions		*/
		/*					*/
		/****************************************/

		/* adds bouquet at the end of the bouquetlist*/
		void addBouquet(const char * const name); // UTF-8 encoded

		/* moves a bouquet from one position to another */
		/* bouquets are numbered starting at 0 */
		void moveBouquet(const unsigned int bouquet, const unsigned int newPos);
		/* deletes a bouquet with all its channels */
		/* bouquets are numbered starting at 0 */
		void deleteBouquet(const unsigned int bouquet);

		/* assigns new name to bouquet*/
		/* bouquets are numbered starting at 0 */
		void renameBouquet(const unsigned int bouquet, const char * const newName); // UTF-8 encoded

		/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
		/* bouquets are numbered starting at 0 */
		void moveChannel(const unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode = MODE_CURRENT);

		// -- check if Bouquet-Name exists (2002-04-02 rasc)
		// -- Return bq_id or -1
		/* bouquets are numbered starting at 0 */
		signed int existsBouquet(const char * const name); // UTF-8 encoded


		// -- check if Channel already in Bouquet (2002-04-05 rasc)
		// -- Return true/false
		/* bouquets are numbered starting at 0 */
		bool  existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id);


		/* adds a channel at the end of then channel list to specified bouquet */
		/* same channels can be in more than one bouquet */
		/* bouquets can contain both tv and radio channels */
		/* bouquets are numbered starting at 0 */
		void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id);

		/* removes a channel from specified bouquet */
		/* bouquets are numbered starting at 0 */
		void removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id);

		/* set a bouquet's lock-state*/
		/* bouquets are numbered starting at 0 */
		void setBouquetLock(const unsigned int bouquet, const bool lock);

		/* set a bouquet's hidden-state*/
		/* bouquets are numbered starting at 0 */
		void setBouquetHidden(const unsigned int bouquet, const bool hidden);

		/* renums the channellist, means gives the channels new numbers */
		/* based on the bouquet order and their order within bouquets */
		/* necessarily after bouquet editing operations*/
		void renumChannellist();

		/* saves current bouquet configuration to bouquets.xml */
		void saveBouquets();

		/****************************************/
		/*					*/
		/* blah functions			*/
		/*					*/
		/****************************************/

		void setStandby(const bool enable);

		void startPlayBack();
		void stopPlayBack();
		void lockPlayBack();
		void unlockPlayBack();
		void pausePlayBack();
		void continuePlayBack();

		bool tune_TP(TP_params TP, int feindex = 0);
		
		bool isPlayBackActive();
		
		void setAudioMode(int mode);
		void getAudioMode(int * mode);
		void setVideoSystem(int video_system);
		
		//void addFrontend();

		/****************************************/
		/*					*/
		/* Event functions			*/
		/*					*/
		/****************************************/

		/*
		  ein beliebiges Event anmelden
		*/
		void registerEvent(const unsigned int eventID, const unsigned int clientID, const char * const udsName);

		/*
		  ein beliebiges Event abmelden
		*/
		void unRegisterEvent(const unsigned int eventID, const unsigned int clientID);
};

#endif
