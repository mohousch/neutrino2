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

#ifndef __zapit_h__
#define __zapit_h__

#include <zapit/zapitclient.h>
#include <zapit/msgtypes.h>
#include <zapit/bouquets.h>


void save_settings (bool write);

// scan functions
void *start_scanthread(void *);
int start_scan(CZapitMessages::commandStartScan StartScan);

/**************************************************************/
/*  functions for new command handling via CZapitClient       */
/*  these functions should be encapsulated in a class CZapit  */
/**************************************************************/
void addChannelToBouquet (const unsigned int bouquet, const t_channel_id channel_id);
void sendBouquets(int connfd, const bool emptyBouquetsToo, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
void internalSendChannels(int connfd, ZapitChannelList* channels, bool nonames);
void sendBouquetChannels (int connfd, const unsigned int bouquet, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, bool nonames = false);
void sendChannels(int connfd, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET);


unsigned int zapTo(const unsigned int channel);
unsigned int zapTo(const unsigned int bouquet, const unsigned int channel);
unsigned int zapTo_ChannelID(const t_channel_id channel_id, const bool isSubService);

void sendAPIDs(int connfd);
void sendSubPIDs(int connfd);
void sendRecordAPIDs(int connfd);
void sendRecordSubPIDs(int connfd);

void closeAVDecoder(void);
void openAVDecoder(void);

void enterStandby(void);
void leaveStandby(void);

////
void zapit_getLastChannel(unsigned int &channumber, char &mode);
int zapit_getMode(void);
void zapit_setMode(const CZapitClient::channelsMode mode);

int zapit_startPlayBack(CZapitChannel *);
int zapit_stopPlayBack(bool sendPmt = false);
void zapit_pausePlayBack(void);
void zapit_continuePlayBack(void);
void zapit_lockPlayBack();
void zapit_unlockPlayBack();
bool zapit_isPlayBackActive();
void zapit_setStandby(bool enable);

bool zapit_isChannelTVChannel(const t_channel_id channel_id);
bool zapit_isChannelWEBTVChannel(const t_channel_id channel_id);
bool zapit_isChannelRadioChannel(const t_channel_id channel_id);

void zapit_zapTo_serviceID_NOWAIT(const t_channel_id channel_id);
void zapit_zapTo_subServiceID_NOWAIT(const t_channel_id channel_id);
void zapit_zapTo(const unsigned int bouquet, const unsigned int channel);
void zapit_zapTo(const unsigned int channel);
unsigned int zapit_zapTo_serviceID(const t_channel_id channel_id);
unsigned int zapit_zapTo_subServiceID(const t_channel_id channel_id);
unsigned int zapit_zapTo_record(const t_channel_id channel_id);

std::string zapit_getChannelName(const t_channel_id channel_id);
int zapit_getChannelNumber(const t_channel_id channel_id);
std::string zapit_getChannelURL(const t_channel_id channel_id);
std::string zapit_getChannelDescription(const t_channel_id channel_id);

// current service
void zapit_getPIDS( CZapitClient::responseGetPIDs& pids );
t_channel_id zapit_getCurrentServiceID();
CZapitClient::CCurrentServiceInfo zapit_getCurrentServiceInfo();
int32_t zapit_getCurrentSatellitePosition();
bool zapit_getCurrentTP(TP_params *TP);

// novd
void zapit_setSubServices( CZapitClient::subServiceList& subServices );
void zapit_zaptoNvodSubService(const int num);

// record
void zapit_setRecordMode(const bool activate);
bool zapit_isRecordModeActive();
t_channel_id zapit_getRecordServiceID();
CZapitClient::CCurrentServiceInfo zapit_getRecordServiceInfo();
void zapit_getRecordPIDS(CZapitClient::responseGetPIDs& pids);

void zapit_reinitChannels();
void zapit_reloadCurrentServices();
bool zapit_reZap();

// frontend
void zapit_sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2, int feindex = 0);
delivery_system_t zapit_getDeliverySystem(int feindex = 0);

// audio / video
void zapit_muteAudio(const bool mute);
bool zapit_getMuteStatus();
void zapit_getAudioMode(int * mode);
void zapit_setAudioMode(int mode);
void zapit_setVolume(const unsigned int left, const unsigned int right);
void zapit_getVolume(unsigned int *left, unsigned int *right);
void zapit_setVolumePercent(const unsigned int percent, t_channel_id channel_id = 0, const unsigned int apid = 0);
void zapit_getVolumePercent(unsigned int *percent, t_channel_id channel_id = 0, const unsigned int apid = 0, const bool is_ac3 = false);
void zapit_setAudioChannel(const unsigned int channel);
void zapit_setVideoSystem(int video_system);

// channels / bouquets
bool zapit_getChannels(CZapitClient::BouquetChannelList& channels, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const CZapitClient::channelsOrder order = CZapitClient::SORT_BOUQUET, const bool utf_encoded = false);
void zapit_getBouquets( CZapitClient::BouquetList& bouquets, const bool emptyBouquetsToo = false, const bool utf_encoded = false, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
bool zapit_getBouquetChannels(const unsigned int bouquet, CZapitClient::BouquetChannelList& channels, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const bool utf_encoded = false);
bool zapit_getBouquetNChannels(const unsigned int bouquet, CZapitClient::BouquetNChannelList& channels, const CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT, const bool utf_encoded = false);

// bouquetManager
void zapit_renumChannellist();
void zapit_saveBouquets();
void zapit_restoreBouquets();
void zapit_addBouquet(const char * const name); // UTF-8 encoded
void zapit_deleteBouquet(const unsigned int bouquet);
void zapit_renameBouquet(const unsigned int bouquet, const char * const newName); // UTF-8 encoded
void zapit_moveBouquet(const unsigned int bouquet, const unsigned int newPos);
void zapit_moveChannel(const unsigned int bouquet, unsigned int oldPos, unsigned int newPos, CZapitClient::channelsMode mode = CZapitClient::MODE_CURRENT);
signed int zapit_existsBouquet(const char * const name); // UTF-8 encoded
void zapit_setBouquetLock(const unsigned int bouquet, const bool lock);
void zapit_setBouquetHidden(const unsigned int bouquet, const bool hidden);
void zapit_addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id);
bool zapit_existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id);
void zapit_removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id);

// scan
bool zapit_tuneTP(TP_params TP, int feindex = 0);
bool zapit_scanTP(TP_params TP, int feindex = 0);
bool zapit_isScanReady(unsigned int &satellite, unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services );
void zapit_getScanSatelliteList( CZapitClient::SatelliteList &satelliteList );
void zapit_setScanSatelliteList( CZapitClient::ScanSatelliteList &satelliteList );
void zapit_setScanType(const CZapitClient::scanType mode);
void zapit_setFEMode(const fe_mode_t mode, int feindex = 0);
void zapit_setScanBouquetMode(const CZapitClient::bouquetMode mode);
void zapit_setDiseqcType(const diseqc_t diseqc, int feindex = 0);
void zapit_setDiseqcRepeat(const uint32_t repeat, int feindex = 0);
void zapit_setScanMotorPosList( CZapitClient::ScanMotorPosList& motorPosList );
bool zapit_startScan(int scan_mode, int feindex = 0);
bool zapit_stopScan();

//
bool zapit_setConfig(Zapit_config Cfg);
void zapit_getConfig(Zapit_config * Cfg);

void zapit_Start(void *data);
//void zapit_Stop(void){};
////

#endif /* __zapit_h__ */
