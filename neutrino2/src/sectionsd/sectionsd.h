#ifndef __sectionsd__
#define __sectionsd__
/*
  $Id: sectionsd.h 12022023 mohousch Exp $

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

#include <string>
#include <vector>

#include <connection/basicclient.h>

#include <sectionsd/sectionsdtypes.h>


class CShortEPGData
{
	public:
		std::string title;
		std::string info1;
		std::string info2;

		CShortEPGData()
		{
			title = "";
			info1 = "";
			info2 = "";
		};
};

class CEPGData;

class CChannelEvent
{
	public:
		t_channel_id       get_channel_id(void) const { return GET_CHANNEL_ID_FROM_EVENT_ID(eventID); }
		event_id_t         eventID;
		std::string        description;
		std::string        text;
		time_t             startTime;
		unsigned           duration;
		t_channel_id 	   channelID; 
};

typedef std::vector<CChannelEvent> CChannelEventList;

class CSectionsd
{
	public:
		enum SIlanguageMode_t 
		{
			ALL,
			FIRST_FIRST,
			FIRST_ALL,
			ALL_FIRST,
			ALL_ALL,
			LANGUAGE_MODE_OFF
		};

		enum events
		{
			EVT_TIMESET,
			EVT_GOT_CN_EPG,
			EVT_SERVICES_UPDATE,
			EVT_BOUQUETS_UPDATE,
			EVT_WRITE_SI_FINISHED
		};

		struct epgflags 
		{
			enum
			{
				has_anything = 0x01,
				has_later = 0x02,
				has_current = 0x04,
				not_broadcast = 0x08,
				has_next = 0x10,
				has_no_current= 0x20,
				current_has_linkagedescriptors= 0x40
			};
		};

		struct responseGetComponentTags
		{
			std::string   component;        // Text aus dem Component Descriptor
			unsigned char componentType;    // Component Descriptor
			unsigned char componentTag;     // Component Descriptor
			unsigned char streamContent;    // Component Descriptor
		};
		typedef std::vector<responseGetComponentTags> ComponentTagList;

		struct responseGetLinkageDescriptors
		{
			std::string           name;
			t_transport_stream_id transportStreamId;
			t_original_network_id originalNetworkId;
			t_service_id          serviceId;
		};
		typedef std::vector<responseGetLinkageDescriptors> LinkageDescriptorList;

		struct sectionsdTime
		{
			time_t startzeit;
			unsigned dauer;
		} /*__attribute__ ((packed))*/ ;

		struct responseGetNVODTimes
		{
			t_service_id                    service_id;
			t_original_network_id           original_network_id;
			t_transport_stream_id           transport_stream_id;
			CSectionsd::sectionsdTime zeit;
		};
		typedef std::vector<responseGetNVODTimes> NVODTimesList;

		struct responseGetCurrentNextInfoChannelID
		{
			event_id_t                      current_uniqueKey;
			CSectionsd::sectionsdTime current_zeit;
			std::string                     current_name;
			char                            current_fsk;
			event_id_t                      next_uniqueKey;
			CSectionsd::sectionsdTime next_zeit;
			std::string                     next_name;
			unsigned                        flags;
		};

		struct CurrentNextInfo : public responseGetCurrentNextInfoChannelID
		{};

		typedef struct
		{
			int epg_cache;
			int epg_old_events;
			int epg_max_events;
			int network_ntprefresh;
			int network_ntpenable;
			int epg_extendedcache;
			std::string network_ntpserver;
			std::string epg_dir;
		} epg_config;
};

class CEPGData
{
	public:
		uint64_t eventID;
		CSectionsd::sectionsdTime	epg_times;
		std::string                     title;
		std::string                     info1;
		std::string                     info2;
		// 21.07.2005 - extended event data
		std::vector<std::string>		itemDescriptions;
		std::vector<std::string>		items;
		char                            fsk;
		unsigned char                   table_id;
		std::string                     contentClassification;
		std::string                     userClassification;

		CEPGData()
		{
			eventID               =  0;
			title                 = "";
			info1                 = "";
			info2                 = "";
			fsk                   =  0;
			table_id              = 0xff;
			contentClassification = "";
			userClassification    = "";
		};
};

//
void sectionsd_getChannelEvents(CChannelEventList &eList, bool tv_mode = true, t_channel_id *chidlist = NULL, int clen = 0);
void sectionsd_getEventsServiceKey(t_channel_id serviceUniqueKey, CChannelEventList &eList, char search = 0, std::string search_text = "");
void sectionsd_getCurrentNextServiceKey(t_channel_id uniqueServiceKey, CSectionsd::responseGetCurrentNextInfoChannelID& current_next );
bool sectionsd_getActualEPGServiceKey(const t_channel_id uniqueServiceKey, CEPGData * epgdata);
bool sectionsd_getEPGidShort(event_id_t epgID, CShortEPGData * epgdata);
bool sectionsd_getEPGid(const event_id_t epgID, const time_t startzeit, CEPGData * epgdata);
bool sectionsd_getComponentTagsUniqueKey(const event_id_t uniqueKey, CSectionsd::ComponentTagList& tags);
bool sectionsd_getLinkageDescriptorsUniqueKey(const event_id_t uniqueKey, CSectionsd::LinkageDescriptorList& descriptors);
bool sectionsd_getNVODTimesServiceKey(const t_channel_id uniqueServiceKey, CSectionsd::NVODTimesList& nvod_list);
void sectionsd_setServiceChanged(t_channel_id channel_id, bool requestEvent = false);
void sectionsd_pauseScanning(const bool doPause);
void sectionsd_freeMemory();
void sectionsd_readSIfromXML(const char *epgxmlname);
void sectionsd_writeSI2XML(const char *epgxmlname);
void sectionsd_readSIfromXMLTV(const char *url);
void sectionsd_insertEventsfromHTTP(std::string& url, t_original_network_id _onid, t_transport_stream_id _tsid, t_service_id _sid);
void sectionsd_setConfig(const CSectionsd::epg_config config);
//void sectionsd_registerEventClient(const unsigned int eventID, const unsigned int clientID, const char * const udsName);
//void sectionsd_unRegisterEventClient(const unsigned int eventID, const unsigned int clientID);
void sectionsd_Start(void);
void sectionsd_Stop(void);

#endif
