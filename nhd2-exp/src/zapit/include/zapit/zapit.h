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

int startPlayBack(CZapitChannel *);
int stopPlayBack(bool sendPmt = false);
void pausePlayBack(void);
void continuePlayBack(void);

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

#endif /* __zapit_h__ */
