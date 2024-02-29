/*
 * $Id: sectionsdtypes.h,v 1.1 2004/02/13 14:36:19 thegoodguy Exp $
 *
 * (C) 2004 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __sectionsdtypes_h__
#define __sectionsdtypes_h__

#include <string>
#include <vector>

// zapit includes
#include <zapit/zapittypes.h>  /* t_channel_id, t_service_id, t_original_network_id, t_transport_stream_id; */


typedef uint64_t event_id_t;

#define CREATE_EVENT_ID(channel_id, event_nr) ((((event_id_t)channel_id) << 16) | event_nr)
#define GET_CHANNEL_ID_FROM_EVENT_ID(event_id) ((t_channel_id)((event_id) >> 16))

////
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

////
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

struct sectionsdTime
{
	time_t starttime;
	unsigned duration;
};

////
class CEPGData
{
	public:
		uint64_t 			eventID;
		sectionsdTime			epg_times;
		std::string                     title;
		std::string                     info1;
		std::string                     info2;
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

#endif /* __sectionsdtypes_h__ */

