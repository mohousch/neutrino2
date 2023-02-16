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

#include "client/zapitclient.h"
#include "client/msgtypes.h"

#include "bouquets.h"


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

void zapit_setRecordMode(const bool activate);
bool zapit_isRecordModeActive();
t_channel_id zapit_getRecordServiceID();
CZapitClient::CCurrentServiceInfo zapit_getRecordServiceInfo();
void zapit_getRecordPIDS(CZapitClient::responseGetPIDs& pids);

void zapit_reinitChannels();
void zapit_reloadCurrentServices();

bool zapit_tune_TP(TP_params TP, int feindex = 0);

bool zapit_reZap();

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

bool zapit_setConfig(Zapit_config Cfg);
void zapit_getConfig(Zapit_config * Cfg);
////

#endif /* __zapit_h__ */
