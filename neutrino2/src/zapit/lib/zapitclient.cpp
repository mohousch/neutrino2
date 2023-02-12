/*
 * $Header: zapitclient.cpp 2013/08/18 11:23:30 mohousch Exp $ *
 *
 * Zapit client interface - DBoxII-Project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de> & the DBoxII-Project
 *
 * License: GPL
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

#include <config.h>
#include <cstdio>
#include <cstring>

/* libevent */
#include <eventserver.h>

#include <zapit/client/zapitclient.h>
#include <zapit/client/msgtypes.h>

#include <src/driver/encoding.h>


unsigned char CZapitClient::getVersion() const
{
	return CZapitMessages::ACTVERSION;
}

const char * CZapitClient::getSocketName() const
{
	return ZAPIT_UDS_NAME;
}

void CZapitClient::shutdown()
{
	send(CZapitMessages::CMD_SHUTDOWN);
	close_connection();
}

/*********************************/
/* general functions for zapping */
/*********************************/

/* zaps to channel of specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::zapTo(const unsigned int bouquet, const unsigned int channel)
{
	CZapitMessages::commandZapto msg;

	msg.bouquet = bouquet;
	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO, (char*)&msg, sizeof(msg));

	close_connection();
}

/* zaps to channel by nr */
void CZapitClient::zapTo(const unsigned int channel)
{
	CZapitMessages::commandZaptoChannelNr msg;

	msg.channel = channel - 1;

	send(CZapitMessages::CMD_ZAPTO_CHANNELNR, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* get current SID */
t_channel_id CZapitClient::getCurrentServiceID()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEID);

	CZapitMessages::responseGetCurrentServiceID response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.channel_id;
}

/* get current Service Infos */
CZapitClient::CCurrentServiceInfo CZapitClient::getCurrentServiceInfo()
{
	send(CZapitMessages::CMD_GET_CURRENT_SERVICEINFO);

	CZapitClient::CCurrentServiceInfo response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	
	return response;
}

/* get record SID */
t_channel_id CZapitClient::getRecordServiceID()
{
	send(CZapitMessages::CMD_GET_RECORD_SERVICEID);

	CZapitMessages::responseGetCurrentServiceID response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.channel_id;
}

/* get record Service Infos */
CZapitClient::CCurrentServiceInfo CZapitClient::getRecordServiceInfo()
{
	send(CZapitMessages::CMD_GET_RECORD_SERVICEINFO);

	CZapitClient::CCurrentServiceInfo response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	
	return response;
}

/* get lastchannel */
void CZapitClient::getLastChannel(unsigned int &channumber, char &mode)
{
	send(CZapitMessages::CMD_GET_LAST_CHANNEL);

	CZapitClient::responseGetLastChannel response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	channumber = response.channelNumber + 1;
	mode = response.mode;

	close_connection();
}

/* get current satpos*/
int32_t CZapitClient::getCurrentSatellitePosition(void)
{
	send(CZapitMessages::CMD_GET_CURRENT_SATELLITE_POSITION);

	int32_t response;
	CBasicClient::receive_data((char *)&response, sizeof(response));

	close_connection();
	return response;
}

/* set audio channel */
void CZapitClient::setAudioChannel(const unsigned int channel)
{
	CZapitMessages::commandSetAudioChannel msg;

	msg.channel = channel;

	send(CZapitMessages::CMD_SET_AUDIOCHAN, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* zaps to onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_serviceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;
	msg.record = false;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID, (const char *) & msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

// zap to record
unsigned int CZapitClient::zapTo_record(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;
	msg.record = true;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID, (const char *) & msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

/* zaps to sub onid_sid, returns the "zap-status" */
unsigned int CZapitClient::zapTo_subServiceID(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID, (const char *) & msg, sizeof(msg));

	CZapitMessages::responseZapComplete response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();

	return response.zapStatus;
}

/* zaps to channel, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_serviceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SERVICEID_NOWAIT, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* zaps to subservice, does NOT wait for completion (uses event) */
void CZapitClient::zapTo_subServiceID_NOWAIT(const t_channel_id channel_id)
{
	CZapitMessages::commandZaptoServiceID msg;

	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_ZAPTO_SUBSERVICEID_NOWAIT, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* set mode tv/radio */
/*
void CZapitClient::setMode(const channelsMode mode)
{
	CZapitMessages::commandSetMode msg;

	msg.mode = mode;

	send(CZapitMessages::CMD_SET_MODE, (const char *) & msg, sizeof(msg));

	close_connection();
}
*/

/* get mode */
/*
int CZapitClient::getMode()
{
	send(CZapitMessages::CMD_GET_MODE);

	CZapitMessages::responseGetMode response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.mode;
}
*/

// set SubServices
void CZapitClient::setSubServices( subServiceList& subServices )
{
	unsigned int i;

	send(CZapitMessages::CMD_SETSUBSERVICES);

	for (i = 0; i < subServices.size(); i++)
		send_data((char*)&subServices[i], sizeof(subServices[i]));

	close_connection();
}

// get Pids
void CZapitClient::getPIDS(responseGetPIDs& pids)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetAPIDs                       responseAPID;
	responseGetSubPIDs                     responseSubPID;

	send(CZapitMessages::CMD_GETPIDS);

	CBasicClient::receive_data((char* )&(pids.PIDs), sizeof(pids.PIDs));

	pids.APIDs.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.APIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseAPID, sizeof(responseAPID));
			pids.APIDs.push_back(responseAPID);
		};
	}
	
	pids.SubPIDs.clear();
	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.SubPIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseSubPID, sizeof(responseSubPID));
			pids.SubPIDs.push_back(responseSubPID);
		};
	}

	close_connection();
}

//TEST
void CZapitClient::getRecordPIDS(responseGetPIDs& pids)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetAPIDs                       responseAPID;
	responseGetSubPIDs                     responseSubPID;

	send(CZapitMessages::CMD_GETRECORDPIDS);

	CBasicClient::receive_data((char* )&(pids.PIDs), sizeof(pids.PIDs));

	pids.APIDs.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.APIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseAPID, sizeof(responseAPID));
			pids.APIDs.push_back(responseAPID);
		};
	}
	
	pids.SubPIDs.clear();
	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		pids.SubPIDs.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			CBasicClient::receive_data((char*)&responseSubPID, sizeof(responseSubPID));
			pids.SubPIDs.push_back(responseSubPID);
		};
	}

	close_connection();
}

// zapto Nvod subservice
void CZapitClient::zaptoNvodSubService(const int num)
{
	CZapitMessages::commandInt msg;

	msg.val = num;

	send(CZapitMessages::CMD_NVOD_SUBSERVICE_NUM, (const char *) & msg, sizeof(msg));

	close_connection();
}

/* gets all bouquets */
/* bouquets are numbered starting at 0 */
void CZapitClient::getBouquets(BouquetList& bouquets, const bool emptyBouquetsToo, const bool utf_encoded, channelsMode mode)
{
	char buffer[30 + 1];

	CZapitMessages::commandGetBouquets msg;

	msg.emptyBouquetsToo = emptyBouquetsToo;
	msg.mode = mode;

	send(CZapitMessages::CMD_GET_BOUQUETS, (char*)&msg, sizeof(msg));

	responseGetBouquets response;
	while (CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquets)))
	{
		if (response.bouquet_nr == RESPONSE_GET_BOUQUETS_END_MARKER)
			break;

		if (!utf_encoded)
		{
			buffer[30] = (char) 0x00;
			strncpy(buffer, response.name, 30);
			strncpy(response.name, UTF8_to_Latin1(buffer).c_str(), 30);
		}
		bouquets.push_back(response);
	}

	close_connection();
}

/* receive channel list */
bool CZapitClient::receive_channel_list(BouquetChannelList& channels, const bool utf_encoded)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetBouquetChannels             response;

	channels.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		channels.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			if (!CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetChannels)))
				return false;

			response.nr++;
			if (!utf_encoded)
			{
                                char buffer[CHANNEL_NAME_SIZE + 1];
                                buffer[CHANNEL_NAME_SIZE] = (char) 0x00;
                                strncpy(buffer, response.name, CHANNEL_NAME_SIZE);
                                strncpy(response.name, UTF8_to_Latin1(buffer).c_str(), CHANNEL_NAME_SIZE);
			}
			channels.push_back(response);
		}
	}
	return true;
}

/* receive_nchannel_list */
bool CZapitClient::receive_nchannel_list(BouquetNChannelList& channels)
{
	CZapitMessages::responseGeneralInteger responseInteger;
	responseGetBouquetNChannels             response;

	channels.clear();

	if (CBasicClient::receive_data((char* )&responseInteger, sizeof(responseInteger)))
	{
		channels.reserve(responseInteger.number);

		while (responseInteger.number-- > 0)
		{
			if (!CBasicClient::receive_data((char*)&response, sizeof(responseGetBouquetNChannels)))
				return false;

			response.nr++;
			channels.push_back(response);
		}
	}
	return true;
}


/* gets all channels that are in specified bouquet */
/* bouquets are numbered starting at 0 */
bool CZapitClient::getBouquetChannels(const unsigned int bouquet, BouquetChannelList& channels, channelsMode mode, const bool utf_encoded)
{
	bool return_value;
	CZapitMessages::commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	return_value = (send(CZapitMessages::CMD_GET_BOUQUET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}

/* get bouqutes channels */
bool CZapitClient::getBouquetNChannels(const unsigned int bouquet, BouquetNChannelList& channels, channelsMode mode, const bool /*utf_encoded*/)
{
	bool                                      return_value;
	CZapitMessages::commandGetBouquetChannels msg;

	msg.bouquet = bouquet;
	msg.mode = mode;

	return_value = (send(CZapitMessages::CMD_GET_BOUQUET_NCHANNELS, (char*)&msg, sizeof(msg))) ? receive_nchannel_list(channels) : false;

	close_connection();
	return return_value;
}

/* gets all channels */
bool CZapitClient::getChannels( BouquetChannelList& channels, channelsMode mode, channelsOrder order, const bool utf_encoded)
{
	bool return_value;
	CZapitMessages::commandGetChannels msg;

	msg.mode = mode;
	msg.order = order;

	return_value = (send(CZapitMessages::CMD_GET_CHANNELS, (char*)&msg, sizeof(msg))) ? receive_channel_list(channels, utf_encoded) : false;

	close_connection();
	return return_value;
}

/* request information about a particular channel_id */
/* channel name */
std::string CZapitClient::getChannelName(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_GET_CHANNEL_NAME, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGetChannelName response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return std::string(response.name);
}

/* channel number */
int CZapitClient::getChannelNumber(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_GET_CHANNEL_NUMBER, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGetChannelNumber response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.number;
}

/* channel url */
std::string CZapitClient::getChannelURL(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_GET_CHANNEL_URL, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGetChannelURL response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return std::string(response.url);
}

/* channel description */
std::string CZapitClient::getChannelDescription(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_GET_CHANNEL_DESCRIPTION, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGetChannelDescription response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return std::string(response.description);
}

/* is channel a TV channel ? */
bool CZapitClient::isChannelTVChannel(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_IS_TV_CHANNEL, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGeneralTrueFalse response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.status;
}

/* is channel a Radio channel ? */
bool CZapitClient::isChannelRadioChannel(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_IS_RADIO_CHANNEL, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGeneralTrueFalse response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.status;
}

/* is channel a WEBTV channel ? */
bool CZapitClient::isChannelWEBTVChannel(const t_channel_id channel_id)
{
	send(CZapitMessages::CMD_IS_WEBTV_CHANNEL, (char *) & channel_id, sizeof(channel_id));

	CZapitMessages::responseGeneralTrueFalse response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.status;
}

/* restore bouquets so as if they were just loaded */
void CZapitClient::restoreBouquets()
{
	send(CZapitMessages::CMD_BQ_RESTORE);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
}

/* reloads channels and services*/
void CZapitClient::reinitChannels()
{
	send(CZapitMessages::CMD_REINIT_CHANNELS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response), true);
	close_connection();
}

/* reload current services */
/* called when sectionsd updates currentservices.xml */
void CZapitClient::reloadCurrentServices()
{
	send(CZapitMessages::CMD_RELOAD_CURRENTSERVICES);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response), true);
	close_connection();
}

/* mute audio */
void CZapitClient::muteAudio(const bool mute)
{
	CZapitMessages::commandBoolean msg;

	msg.truefalse = mute;

	send(CZapitMessages::CMD_MUTE, (char*)&msg, sizeof(msg));

	close_connection();
}

/* Get mute status */
bool CZapitClient::getMuteStatus()
{
	CZapitMessages::commandBoolean msg;

	send(CZapitMessages::CMD_GET_MUTE_STATUS, (char*)&msg, sizeof(msg));
	CBasicClient::receive_data((char*)&msg, sizeof(msg));
	close_connection();
	return msg.truefalse;
}

/* set volume */
void CZapitClient::setVolume(const unsigned int left, const unsigned int right)
{
	CZapitMessages::commandVolume msg;

	msg.left = left;
	msg.right = right;

	send(CZapitMessages::CMD_SET_VOLUME, (char*)&msg, sizeof(msg));

	close_connection();
}

/* get volume */
void CZapitClient::getVolume(unsigned int * left, unsigned int * right)
{
        CZapitMessages::commandVolume msg;

        send(CZapitMessages::CMD_GET_VOLUME, 0, 0);

        CBasicClient::receive_data((char*)&msg, sizeof(msg));
        *left = msg.left;
        *right = msg.right;

        close_connection();
}

/* set volume percent */
void CZapitClient::setVolumePercent(unsigned int percent, t_channel_id channel_id, const unsigned int apid)
{
	CZapitMessages::commandVolumePercent msg;

	msg.apid = apid;
	msg.channel_id = channel_id;
	msg.percent = percent;

	send(CZapitMessages::CMD_SET_VOLUME_PERCENT, (char*)&msg, sizeof(msg));

	close_connection();
}

/* get volume percent */
void CZapitClient::getVolumePercent(unsigned int * percent, t_channel_id channel_id, const unsigned int apid, const bool is_ac3)
{
	CZapitMessages::commandVolumePercent msg;

	msg.apid = apid;
	msg.is_ac3 = is_ac3;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_GET_VOLUME_PERCENT, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char*)&msg, sizeof(msg));
	*percent = msg.percent;

	close_connection();
}

/* delivery_system */
delivery_system_t CZapitClient::getDeliverySystem(int /*feindex*/)
{
	send(CZapitMessages::CMD_GET_DELIVERY_SYSTEM, 0, 0);

	CZapitMessages::responseDeliverySystem response;

	if (!CBasicClient::receive_data((char* )&response, sizeof(response)))
		response.system = DVB_S;  // return DVB_S if communication fails

	close_connection();

	return response.system;
}

// get current TP
bool CZapitClient::get_current_TP(TP_params * TP)
{
	TP_params TP_temp;
	send(CZapitMessages::CMD_GET_CURRENT_TP);
	bool reply = CBasicClient::receive_data((char*)&TP_temp, sizeof(TP_temp));
	memcpy(TP, &TP_temp, sizeof(TP_temp));
	close_connection();
	return reply;
}

/* sends diseqc 1.2 motor command */
void CZapitClient::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2, int feindex)
{
	CZapitMessages::commandMotor msg;

	msg.cmdtype = cmdtype;
	msg.address = address;
	msg.cmd = cmd;
	msg.num_parameters = num_parameters;
	msg.param1 = param1;
	msg.param2 = param2;
	
	msg.feindex = feindex;

	send(CZapitMessages::CMD_SEND_MOTOR_COMMAND, (char*)&msg, sizeof(msg));

	close_connection();
}

/***********************************************/
/*                                             */
/*  Scanning stuff                             */
/*                                             */
/***********************************************/

/* start TS-Scan */
bool CZapitClient::startScan(const int scan_mode, int feindex)
{
	CZapitMessages::commandStartScan msg;

	msg.scan_mode = scan_mode;
	msg.feindex = feindex;
	
	bool reply = send(CZapitMessages::CMD_SCANSTART, (char*)&msg, sizeof(msg));
	close_connection();
	
	return reply;
}

/* stop scan */
bool CZapitClient::stopScan()
{
        bool reply = send(CZapitMessages::CMD_SCANSTOP);
        close_connection();
        return reply;
}

/* set zapit config */
bool CZapitClient::setConfig(Zapit_config Cfg)
{
	bool reply = send(CZapitMessages::CMD_SETCONFIG, (char*)&Cfg, sizeof(Cfg));
        close_connection();
        return reply;
}

/* get zapit config */
void CZapitClient::getConfig (Zapit_config * Cfg)
{
	send(CZapitMessages::CMD_GETCONFIG);
	CBasicClient::receive_data((char *) Cfg, sizeof(Zapit_config));
	close_connection();
}

/* rezap */
bool CZapitClient::Rezap()
{
        bool reply = send(CZapitMessages::CMD_REZAP);
        close_connection();
        return reply;
}

/* start manual scan */
bool CZapitClient::scan_TP(TP_params TP, int feindex)
{
	CZapitMessages::commandScanTP msg;

	msg.TP = TP;
	msg.feindex = feindex;
	
	bool reply = send(CZapitMessages::CMD_SCAN_TP, (char*)&msg, sizeof(msg));
	close_connection();
	
	return reply;
}

/* tune TP */
bool CZapitClient::tune_TP(TP_params TP, int feindex)
{
	CZapitMessages::commandTuneTP msg;

	msg.TP = TP;
	msg.feindex = feindex;
	
	bool reply = send(CZapitMessages::CMD_TUNE_TP, (char*)&msg, sizeof(msg));
	close_connection();
	
	return reply;
}

/* query if ts-scan is ready - response gives status */
bool CZapitClient::isScanReady(unsigned int &satellite,  unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services )
{
	send(CZapitMessages::CMD_SCANREADY);

	CZapitMessages::responseIsScanReady response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	satellite = response.satellite;
	processed_transponder = response.processed_transponder;
	transponder = response.transponder;
	services = response.services;

	close_connection();
	return response.scanReady;
}

/* query possible satellits*/
void CZapitClient::getScanSatelliteList(SatelliteList& satelliteList)
{
	uint32_t  satlength;

	send(CZapitMessages::CMD_SCANGETSATLIST);

	responseGetSatelliteList response;
	while (CBasicClient::receive_data((char*)&satlength, sizeof(satlength)))
	{
		if (satlength == SATNAMES_END_MARKER)
			break;

		if (!CBasicClient::receive_data((char*)&(response), satlength))
			break;

		satelliteList.push_back(response);
	}

	close_connection();

}

// tell zapit which satellites to scan
void CZapitClient::setScanSatelliteList( ScanSatelliteList &satelliteList )
{
	send(CZapitMessages::CMD_SCANSETSCANSATLIST);

	for (uint32_t i=0; i<satelliteList.size(); i++)
	{
		send_data((char*)&satelliteList[i], sizeof(satelliteList[i]));
	}
	close_connection();
}

/* tell zapit stored satellite positions in diseqc 1.2 motor */
void CZapitClient::setScanMotorPosList( ScanMotorPosList& motorPosList )
{
	send(CZapitMessages::CMD_SCANSETSCANMOTORPOSLIST);

	for (uint32_t i = 0; i < motorPosList.size(); i++)
	{
		send_data((char*)&motorPosList[i], sizeof(motorPosList[i]));
	}
	close_connection();
}

/* set diseqcType */
void CZapitClient::setDiseqcType(const diseqc_t diseqc, int feindex)
{
	//printf("CZapitClient::setDiseqcType\n");
	CZapitMessages::commandSetDiseqcType msg;

	msg.diseqc = diseqc;
	msg.feindex = feindex;

	send(CZapitMessages::CMD_SCANSETDISEQCTYPE, (char*)&msg, sizeof(msg));
	close_connection();
}

/* set diseqcRepeat */
void CZapitClient::setDiseqcRepeat(const uint32_t  repeat, int feindex)
{
	CZapitMessages::commandSetDiseqcRepeat msg;

	msg.repeat = repeat;
	msg.feindex = feindex;
	
	send(CZapitMessages::CMD_SCANSETDISEQCREPEAT, (char *) &msg, sizeof(msg));
	close_connection();
}

/* set diseqcRepeat */
void CZapitClient::setScanBouquetMode(const bouquetMode mode)
{
	send(CZapitMessages::CMD_SCANSETBOUQUETMODE, (const char *) & mode, sizeof(mode));
	close_connection();
}

/* set Scan-TYpe for channelsearch */
void CZapitClient::setScanType(const scanType mode)
{
	send(CZapitMessages::CMD_SCANSETTYPE, (const char *) & mode, sizeof(mode));
  	close_connection();
}

/* set fe mode */
void CZapitClient::setFEMode(const fe_mode_t mode, int feindex)
{
	CZapitMessages::commandSetFEMode msg;

	msg.mode = mode;
	msg.feindex = feindex;

	send(CZapitMessages::CMD_SCANSETFEMODE, (char*)&msg, sizeof(msg));
	close_connection();
}

/***********************************************/
/*                                             */
/* Bouquet editing functions                   */
/*                                             */
/***********************************************/

/* adds bouquet at the end of the bouquetlist  */
void CZapitClient::addBouquet(const char * const name)
{
	if (send(CZapitMessages::CMD_BQ_ADD_BOUQUET))
		send_string(name);

	close_connection();
}

/* moves a bouquet from one position to another */
/* bouquets are numbered starting at 0 */
void CZapitClient::moveBouquet(const unsigned int bouquet, const unsigned int newPos)
{
	CZapitMessages::commandMoveBouquet msg;

	msg.bouquet = bouquet;
	msg.newPos = newPos;

	send(CZapitMessages::CMD_BQ_MOVE_BOUQUET, (char*)&msg, sizeof(msg));
	close_connection();
}

/* deletes a bouquet with all its channels*/
/* bouquets are numbered starting at 0 */
void CZapitClient::deleteBouquet(const unsigned int bouquet)
{
	CZapitMessages::commandDeleteBouquet msg;

	msg.bouquet = bouquet;

	send(CZapitMessages::CMD_BQ_DELETE_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* assigns new name to bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::renameBouquet(const unsigned int bouquet, const char * const newName)
{
	CZapitMessages::commandRenameBouquet msg;

	msg.bouquet = bouquet;

	if (send(CZapitMessages::CMD_BQ_RENAME_BOUQUET, (char*)&msg, sizeof(msg)))
		send_string(newName);

	close_connection();
}

/* -- check if Bouquet-Name exists */
/* -- Return: Bouquet-ID  or  -1 == no Bouquet found */
/* bouquets are numbered starting at 0 */
signed int CZapitClient::existsBouquet(const char * const name)
{
	CZapitMessages::responseGeneralInteger response;

	if (send(CZapitMessages::CMD_BQ_EXISTS_BOUQUET))
		send_string(name);

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return response.number;
}

/* -- check if Channel already is in Bouquet */
/* -- Return: true/false */
/* bouquets are numbered starting at 0 */
bool CZapitClient::existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandExistsChannelInBouquet msg;
	CZapitMessages::responseGeneralTrueFalse response;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET, (char*)&msg, sizeof(msg));

	CBasicClient::receive_data((char* )&response, sizeof(response));
	close_connection();
	return (unsigned int) response.status;
}

/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
/* bouquets are numbered starting at 0 */
void CZapitClient::moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode)
{
	CZapitMessages::commandMoveChannel msg;

	msg.bouquet = bouquet;
	msg.oldPos  = oldPos - 1;
	msg.newPos  = newPos - 1;
	msg.mode    = mode;

	send(CZapitMessages::CMD_BQ_MOVE_CHANNEL, (char*)&msg, sizeof(msg));

	close_connection();
}

/* adds a channel at the end of then channel list to specified bouquet */
/* same channels can be in more than one bouquet */
/* bouquets can contain both tv and radio channels */
/* bouquets are numbered starting at 0 */
void CZapitClient::addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandAddChannelToBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_ADD_CHANNEL_TO_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* removes a channel from specified bouquet */
/* bouquets are numbered starting at 0 */
void CZapitClient::removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id)
{
	CZapitMessages::commandRemoveChannelFromBouquet msg;

	msg.bouquet    = bouquet;
	msg.channel_id = channel_id;

	send(CZapitMessages::CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's lock-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetLock(const unsigned int bouquet, const bool lock)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = lock;

	send(CZapitMessages::CMD_BQ_SET_LOCKSTATE, (char*)&msg, sizeof(msg));

	close_connection();
}

/* set a bouquet's hidden-state*/
/* bouquets are numbered starting at 0 */
void CZapitClient::setBouquetHidden(const unsigned int bouquet, const bool hidden)
{
	CZapitMessages::commandBouquetState msg;

	msg.bouquet = bouquet;
	msg.state   = hidden;

	send(CZapitMessages::CMD_BQ_SET_HIDDENSTATE, (char*)&msg, sizeof(msg));
	close_connection();
}

/* renums the channellist, means gives the channels new numbers */
/* based on the bouquet order and their order within bouquets */
/* necessarily after bouquet editing operations*/
void CZapitClient::renumChannellist()
{
	send(CZapitMessages::CMD_BQ_RENUM_CHANNELLIST);
	close_connection();
}


/* saves current bouquet configuration to bouquets.xml*/
void CZapitClient::saveBouquets()
{
	send(CZapitMessages::CMD_BQ_SAVE_BOUQUETS);

	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
}

/* set standby */
void CZapitClient::setStandby(const bool enable)
{
	CZapitMessages::commandBoolean msg;
	msg.truefalse = enable;
	send(CZapitMessages::CMD_SET_STANDBY, (char*)&msg, sizeof(msg));
	
	//if(enable) //striper
	{
		CZapitMessages::responseCmd response;
		CBasicClient::receive_data((char* )&response, sizeof(response));
	}
	
	close_connection();
}

/* set video system */
void CZapitClient::setVideoSystem(int video_system)
{
	CZapitMessages::commandInt msg;
	msg.val = video_system;
	send(CZapitMessages::CMD_SET_VIDEO_SYSTEM, (char*)&msg, sizeof(msg));
	close_connection();
}

/* start playback */
/*
void CZapitClient::startPlayBack()
{
	send(CZapitMessages::CMD_SB_START_PLAYBACK);
	close_connection();
}
*/

/* stop playback */
/*
void CZapitClient::stopPlayBack()
{
	send(CZapitMessages::CMD_SB_STOP_PLAYBACK);
	
	CZapitMessages::responseCmd response;
	CBasicClient::receive_data((char* )&response, sizeof(response));
	
	close_connection();
}
*/

// lock playback
/*
void CZapitClient::lockPlayBack()
{
	send(CZapitMessages::CMD_SB_LOCK_PLAYBACK);
	
	close_connection();
}
*/

// unlock playback
/*
void CZapitClient::unlockPlayBack()
{
	send(CZapitMessages::CMD_SB_UNLOCK_PLAYBACK);
	close_connection();
}
*/

// pause playback
/*
void CZapitClient::pausePlayBack()
{
	send(CZapitMessages::CMD_SB_PAUSE_PLAYBACK);
	close_connection();
}
*/

// pause playback
/*
void CZapitClient::continuePlayBack()
{
	send(CZapitMessages::CMD_SB_CONTINUE_PLAYBACK);
	close_connection();
}
*/

/* is playback activ? */
bool CZapitClient::isPlayBackActive()
{
	send(CZapitMessages::CMD_SB_GET_PLAYBACK_ACTIVE);

	CZapitMessages::responseGetPlaybackState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

/* set audio mode */
void CZapitClient::setAudioMode(const int mode)
{
	CZapitMessages::commandInt msg;
	msg.val = mode;
	send(CZapitMessages::CMD_SET_AUDIO_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

/* get audio mode */
void CZapitClient::getAudioMode(int * mode)
{
        CZapitMessages::commandInt msg;
        send(CZapitMessages::CMD_GET_AUDIO_MODE, 0, 0);
        CBasicClient::receive_data((char* )&msg, sizeof(msg));
        * mode = msg.val;
        close_connection();
}

/* set record mode */
void CZapitClient::setRecordMode(const bool activate)
{
	CZapitMessages::commandSetRecordMode msg;
	msg.activate = activate;
	send(CZapitMessages::CMD_SET_RECORD_MODE, (char*)&msg, sizeof(msg));
	close_connection();
}

// is record mode active
bool CZapitClient::isRecordModeActive()
{
	send(CZapitMessages::CMD_GET_RECORD_MODE);

	CZapitMessages::responseGetRecordModeState response;
	CBasicClient::receive_data((char* )&response, sizeof(response));

	close_connection();
	return response.activated;
}

/* register event */
void CZapitClient::registerEvent(const unsigned int eventID, const unsigned int clientID, const char * const udsName)
{
	CEventServer::commandRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	strcpy(msg.udsName, udsName);

	send(CZapitMessages::CMD_REGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}

/* unregister event */
void CZapitClient::unRegisterEvent(const unsigned int eventID, const unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg;

	msg.eventID = eventID;
	msg.clientID = clientID;

	send(CZapitMessages::CMD_UNREGISTEREVENTS, (char*)&msg, sizeof(msg));

	close_connection();
}


