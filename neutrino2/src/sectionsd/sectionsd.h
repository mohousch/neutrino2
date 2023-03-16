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

#include <sectionsd/sectionsdtypes.h>
#include <sectionsd/SIevents.hpp>


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

		struct CurrentNextInfo
		{
			event_id_t                      current_uniqueKey;
			sectionsdTime 			current_zeit;
			std::string                     current_name;
			char                            current_fsk;
			event_id_t                      next_uniqueKey;
			sectionsdTime 			next_zeit;
			std::string                     next_name;
			unsigned                        flags;
		};

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
		
	private:
		//
		pthread_t threadTOT, threadEIT, threadCN, threadFSEIT, threadVIASATEIT, threadHouseKeeping;
		static void *timeThread(void *);
		static void *eitThread(void *);
		static void *cnThread(void *);
		static void *fseitThread(void *);
		static void *viasateitThread(void *);
		//
		static void *houseKeepingThread(void *);
		//
		static void *insertEventsfromFile(void *);
		static void *insertEventsfromXMLTV(void* data);
		static void *insertEventsfromLocalTV(void *data);
		
		//
		bool checkEPGFilter(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid);
		bool checkBlacklist(t_channel_id channel_id);
		bool checkNoDVBTimelist(t_channel_id channel_id);
		void addEPGFilter(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid);
		void addBlacklist(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid);
		void addNoDVBTimelist(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid);
		bool deleteEvent(const event_id_t uniqueKey);
		void addEvent(const SIevent &evt, const time_t zeit, bool cn = false);
		void addNVODevent(const SIevent &evt);
		void removeOldEvents(const long seconds);
		void removeDupEvents(void);
		const SIevent& findSIeventForEventUniqueKey(const event_id_t eventUniqueKey);
		const SIevent& findActualSIeventForServiceUniqueKey(const t_channel_id serviceUniqueKey, SItime& zeit, long plusminus = 0, unsigned *flag = 0);
		const SIevent& findNextSIeventForServiceUniqueKey(const t_channel_id serviceUniqueKey, SItime& zeit);
		const SIevent &findNextSIevent(const event_id_t uniqueKey, SItime &zeit);
		void findPrevNextSIevent(const event_id_t uniqueKey, SItime &zeit, SIevent &prev, SItime &prev_zeit, SIevent &next, SItime &next_zeit);
		bool channel_in_requested_list(t_channel_id * clist, t_channel_id chid, int len);
		void write_epg_xml_header(FILE * fd, const t_original_network_id onid, const t_transport_stream_id tsid, const t_service_id sid);
		void write_index_xml_header(FILE * fd);
		void write_epgxml_footer(FILE *fd);
		void write_indexxml_footer(FILE *fd);
		void readEPGFilter(void);
		void readDVBTimeFilter(void);
		bool isReady(void);
		void print_meminfo(void);
		
		//
		CSectionsd(){};
	
	public:
		~CSectionsd(){};
		static CSectionsd *getInstance()
		{
			static CSectionsd * sectionsd = NULL;

			if(!sectionsd) 
			{
				sectionsd = new CSectionsd();
			} 

			return sectionsd;
		};
		
		//
		void Start(void);
		void Stop(void);
		
		//
		void getChannelEvents(CChannelEventList &eList, bool tv_mode = true, t_channel_id *chidlist = NULL, int clen = 0);
		void getEventsServiceKey(t_channel_id serviceUniqueKey, CChannelEventList &eList, char search = 0, std::string search_text = "");
		void getCurrentNextServiceKey(t_channel_id uniqueServiceKey, CurrentNextInfo& current_next );
		bool getActualEPGServiceKey(const t_channel_id uniqueServiceKey, CEPGData * epgdata);
		bool getEPGidShort(event_id_t epgID, CShortEPGData * epgdata);
		bool getEPGid(const event_id_t epgID, const time_t startzeit, CEPGData * epgdata);
		bool getComponentTagsUniqueKey(const event_id_t uniqueKey, ComponentTagList& tags);
		bool getLinkageDescriptorsUniqueKey(const event_id_t uniqueKey, LinkageDescriptorList& descriptors);
		bool getNVODTimesServiceKey(const t_channel_id uniqueServiceKey, NVODTimesList& nvod_list);
		void setServiceChanged(t_channel_id channel_id, bool requestEvent = false);
		void pauseScanning(const bool doPause);
		void freeMemory();
		void readSIfromXML(const char *epgxmlname);
		void writeSI2XML(const char *epgxmlname);
		
		//
		void readSIfromXMLTV(const char *url);
		//void readSIfromXMLTV(const t_channel_id chid);
		void readSIfromLocalTV(const t_channel_id chid);
		//
		void setConfig(const epg_config config);
		void deleteSIexceptEPG();
		void setLanguages(const std::vector<std::string>& newLanguages);
		
		//
		void dumpStatus(void);
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
		std::vector<std::string>	itemDescriptions;
		std::vector<std::string>	items;
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

#endif

