//
//  $Id: sectionsd.cpp,v 1.31 2013/08/18 11:23:30 mohousch Exp $
//
//    sectionsd.cpp (network daemon for SI-sections)
//    (dbox-II-project)
//
//    Copyright (C) 2001 by fnbrd
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2008, 2009 Stefan Seyfried
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//
//

#include <config.h>
#include <malloc.h>
#include <dmxapi.h>
#include <dmx.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
//#include <sys/resource.h> // getrusage
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <limits>
#include <functional>

#include <sys/wait.h>
#include <sys/time.h>

#include <syscall.h>

#include <connection/basicsocket.h>
#include <connection/basicserver.h>

#include <libxmltree/xmlinterface.h>

// zapit includes
#include <zapit/settings.h>
#include <configfile.h>

#include <sectionsd/sectionsd.h>
#include <eventserver.h>

#include "abstime.h"

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"

#include "SIsections.hpp"
#include "SIlanguage.hpp"

#include <driver/encoding.h>

#include <system/debug.h>
#include <system/helpers.h>
#include <system/settings.h>

#include <global.h>
#include <neutrinoMessages.h>

#include <zapit/bouquets.h>
#include <zapit/frontend_c.h>


extern CBouquetManager * g_bouquetManager;	// defined in der zapit.cpp
extern tallchans allchans;	// defined in zapit.cpp.
int op_increase(int i) { return ++i; }

// 60 Minuten Zyklus...
#define TIME_EIT_SCHEDULED_PAUSE 60 * 60
// -- 5 Minutes max. pause should improve behavior  (rasc, 2005-05-02)
// Zeit die fuer die gewartet wird, bevor der Filter weitergeschaltet wird, falls es automatisch nicht klappt
#define TIME_EIT_SKIPPING 90

// a little more time for freesat epg
#define TIME_FSEIT_SKIPPING 240

static bool sectionsd_ready = false;
static bool reader_ready = true;

static unsigned int max_events;
// sleep 5 minutes
#define HOUSEKEEPING_SLEEP (5 * 60)
// meta housekeeping after XX housekeepings - every 24h -
#define META_HOUSEKEEPING (24 * 60 * 60) / HOUSEKEEPING_SLEEP

// 12h Pause fr SDT
// -- shorter time for pause should  result in better behavior  (rasc, 2005-05-02)
#define TIME_SDT_SCHEDULED_PAUSE 2* 60* 60
//We are very nice here. Start scanning for channels, if the user stays for XX secs on that channel
//Sleeping when TIME_SDT_NODATA seconds no NEW section was received
#define TIME_SDT_NONEWDATA	5
//How many BATs shall we read per transponder
#define MAX_BAT 10
//How many other SDTs shall we puzzle per transponder at the same time
//How many other SDTs shall we assume per tranponder
#define MAX_SDTs 70
//How many sections can a table consist off?
#define MAX_SECTIONS 0x1f

//Set pause for NIT
#define TIME_NIT_SCHEDULED_PAUSE 2* 60* 60
//We are very nice here. Start scanning for channels, if the user stays for XX secs on that channel
//Sleeping when TIME_NIT_NODATA seconds no NEW section was received
#define TIME_NIT_NONEWDATA	5
//How many other NITs shall we puzzle per transponder at the same time
//How many other SDTs shall we assume per tranponder
#define MAX_NIDs 10

// Timeout bei tcp/ip connections in ms
#define READ_TIMEOUT_IN_SECONDS  2
#define WRITE_TIMEOUT_IN_SECONDS 2

// Gibt die Anzahl Timeouts an, nach der die Verbindung zum DMX neu gestartet wird (wegen evtl. buffer overflow)
// for NIT and SDT threads...
#define RESTART_DMX_AFTER_TIMEOUTS 5

// Timeout in ms for reading from dmx in EIT threads. Dont make this too long
// since we are holding the start_stop lock during this read!
#define EIT_READ_TIMEOUT 100
// Number of DMX read timeouts, after which we check if there is an EIT at all
// for EIT and PPT threads...
#define CHECK_RESTART_DMX_AFTER_TIMEOUTS (2000 / EIT_READ_TIMEOUT) // 2 seconds

// Time in seconds we are waiting for an EIT version number
#define TIME_EIT_VERSION_WAIT		3
// number of timeouts after which we stop waiting for an EIT version number
#define TIMEOUTS_EIT_VERSION_WAIT	(2 * CHECK_RESTART_DMX_AFTER_TIMEOUTS)

// the maximum length of a section (0x0fff) + header (3)
//#define MAX_SECTION_LENGTH (0x0fff + 3)

// Wieviele Sekunden EPG gecached werden sollen
static long secondsToCache;
static long secondsExtendedTextCache;
// Ab wann ein Event als alt gilt (in Sekunden)
static long oldEventsAre;
int scanning = 1;

std::string epg_filter_dir = CONFIGDIR "/zapit/epgfilter.xml";
static bool epg_filter_is_whitelist = false;
static bool epg_filter_except_current_next = false;
static bool messaging_zap_detected = false;

std::string dvbtime_filter_dir = CONFIGDIR "/zapit/dvbtimefilter.xml";
static bool dvb_time_update = false;

//NTP-Config
#define CONF_FILE CONFIGDIR "/neutrino2.conf"

std::string ntp_system_cmd_prefix = "ntpdate ";

std::string ntp_system_cmd;
CConfigFile ntp_config(',');
std::string ntpserver;
int ntprefresh;
int ntpenable;

static int eit_update_fd = -1;
static bool update_eit = true;

/* messaging_current_servicekey does probably not need locking, since it is
   changed from one place */
static t_channel_id    messaging_current_servicekey = 0;
static bool channel_is_blacklisted = false;
// EVENTS...

/* messaging_eit_is_busy does not need locking, it is only written to from CN-Thread */
static bool		messaging_eit_is_busy = false;
static bool		messaging_need_eit_version = false;
std::string epg_dir("");

extern CEventServer *eventServer;

static pthread_rwlock_t eventsLock = PTHREAD_RWLOCK_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge events geschrieben und gelesen wird
static pthread_rwlock_t servicesLock = PTHREAD_RWLOCK_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge services geschrieben und gelesen wird
static pthread_rwlock_t transpondersLock = PTHREAD_RWLOCK_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge transponders geschrieben und gelesen wird
static pthread_rwlock_t bouquetsLock = PTHREAD_RWLOCK_INITIALIZER; // Unsere (fast-)mutex, damit nicht gleichzeitig in die Menge bouquets geschrieben und gelesen wird
static pthread_rwlock_t messagingLock = PTHREAD_RWLOCK_INITIALIZER;

static pthread_cond_t timeThreadSleepCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t timeThreadSleepMutex = PTHREAD_MUTEX_INITIALIZER;

extern CFrontend * live_fe;
extern int FrontendCount;


static DMX dmxEIT(0x12, 3000 );
static DMX dmxCN(0x12, 512, false);

// freesat
static DMX dmxFSEIT(3842, 320);
//viasat
static DMX dmxVIASAT(0x39, 3000);

extern cDemux * dmxUTC;			// defined in dmxapi.cpp

//
int sectionsd_stop = 0;

static bool slow_addevent = true;

inline void readLockServices(void)
{
	pthread_rwlock_rdlock(&servicesLock);
}

inline void writeLockServices(void)
{
	pthread_rwlock_wrlock(&servicesLock);
}

inline void unlockServices(void)
{
	pthread_rwlock_unlock(&servicesLock);
}

inline void readLockMessaging(void)
{
	pthread_rwlock_rdlock(&messagingLock);
}

inline void writeLockMessaging(void)
{
	pthread_rwlock_wrlock(&messagingLock);
}

inline void unlockMessaging(void)
{
	pthread_rwlock_unlock(&messagingLock);
}

inline void readLockEvents(void)
{
	pthread_rwlock_rdlock(&eventsLock);
}

inline void writeLockEvents(void)
{
	pthread_rwlock_wrlock(&eventsLock);
}

inline void unlockEvents(void)
{
	pthread_rwlock_unlock(&eventsLock);
}

inline void readLockTransponders(void)
{
	pthread_rwlock_rdlock(&transpondersLock);
}

inline void writeLockTransponders(void)
{
	pthread_rwlock_wrlock(&transpondersLock);
}

inline void unlockTransponders(void)
{
	pthread_rwlock_unlock(&transpondersLock);
}

inline void readLockBouquets(void)
{
	pthread_rwlock_rdlock(&bouquetsLock);
}

inline void writeLockBouquets(void)
{
	pthread_rwlock_wrlock(&bouquetsLock);
}

inline void unlockBouquets(void)
{
	pthread_rwlock_unlock(&bouquetsLock);
}

bool timeset = false;
#if USE_OPENGL
bool bTimeCorrect = true;
#else
bool bTimeCorrect = false;
#endif
pthread_cond_t timeIsSetCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t timeIsSetMutex = PTHREAD_MUTEX_INITIALIZER;

static int	messaging_have_CN = 0x00;	// 0x01 = CURRENT, 0x02 = NEXT
static int	messaging_got_CN = 0x00;	// 0x01 = CURRENT, 0x02 = NEXT
static time_t	messaging_last_requested = time_monotonic();
static bool	messaging_neutrino_sets_time = false;

inline bool waitForTimeset(void)
{
	pthread_mutex_lock(&timeIsSetMutex);
	while(!timeset)
		pthread_cond_wait(&timeIsSetCond, &timeIsSetMutex);
	pthread_mutex_unlock(&timeIsSetMutex);
	/* we have time synchronization issues, at least on kernel 2.4, so
	   sometimes the time in the threads is still 1.1.1970, even after
	   waitForTimeset() returns. Let's hope that we work around this issue
	   with this sleep */
	sleep(1);
	writeLockMessaging();
	messaging_last_requested = time_monotonic();
	unlockMessaging();
	return true;
}

static const SIevent nullEvt; // Null-Event

//------------------------------------------------------------
// Wir verwalten die events in SmartPointers
// und nutzen verschieden sortierte Menge zum Zugriff
//------------------------------------------------------------
typedef SIevent *SIeventPtr;

typedef std::map<event_id_t, SIeventPtr, std::less<event_id_t> > MySIeventsOrderUniqueKey;
static MySIeventsOrderUniqueKey mySIeventsOrderUniqueKey;
static MySIeventsOrderUniqueKey mySIeventsNVODorderUniqueKey;

static SIevent * myCurrentEvent = NULL;
static SIevent * myNextEvent = NULL;

//
struct OrderServiceUniqueKeyFirstStartTimeEventUniqueKey
{
	bool operator()(const SIeventPtr &p1, const SIeventPtr &p2)
	{
		return
			(p1->get_channel_id() == p2->get_channel_id()) ?
			(p1->times.begin()->startzeit == p2->times.begin()->startzeit ? p1->eventID < p2->eventID : p1->times.begin()->startzeit < p2->times.begin()->startzeit )
				:
				(p1->get_channel_id() < p2->get_channel_id());
	}
};

typedef std::set<SIeventPtr, OrderServiceUniqueKeyFirstStartTimeEventUniqueKey> MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey;

static MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey;

struct OrderFirstEndTimeServiceIDEventUniqueKey
{
	bool operator()(const SIeventPtr &p1, const SIeventPtr &p2)
	{
		return
			p1->times.begin()->startzeit + (long)p1->times.begin()->dauer == p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ?
			(p1->service_id == p2->service_id ? p1->uniqueKey() > p2->uniqueKey() : p1->service_id < p2->service_id)
				:
				( p1->times.begin()->startzeit + (long)p1->times.begin()->dauer < p2->times.begin()->startzeit + (long)p2->times.begin()->dauer ) ;
	}
};

typedef std::set<SIeventPtr, OrderFirstEndTimeServiceIDEventUniqueKey > MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey;
static MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey;

// Hier landen alle Service-Ids von Meta-Events inkl. der zugehoerigen Event-ID (nvod)
// d.h. key ist der Unique Service-Key des Meta-Events und Data ist der unique Event-Key
typedef std::map<t_channel_id, event_id_t, std::less<t_channel_id> > MySIeventUniqueKeysMetaOrderServiceUniqueKey;
static MySIeventUniqueKeysMetaOrderServiceUniqueKey mySIeventUniqueKeysMetaOrderServiceUniqueKey;

struct EPGFilter
{
	t_original_network_id onid;
	t_transport_stream_id tsid;
	t_service_id sid;
	EPGFilter *next;
};

struct ChannelBlacklist
{
	t_channel_id chan;
	t_channel_id mask;
	ChannelBlacklist *next;
};

struct ChannelNoDVBTimelist
{
	t_channel_id chan;
	t_channel_id mask;
	ChannelNoDVBTimelist *next;
};

EPGFilter *CurrentEPGFilter = NULL;
ChannelBlacklist *CurrentBlacklist = NULL;
ChannelNoDVBTimelist *CurrentNoDVBTime = NULL;

bool CSectionsd::checkEPGFilter(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid)
{
	EPGFilter *filterptr = CurrentEPGFilter;
	
	while (filterptr)
	{
		if (((filterptr->onid == onid) || (filterptr->onid == 0)) &&
				((filterptr->tsid == tsid) || (filterptr->tsid == 0)) &&
				((filterptr->sid == sid) || (filterptr->sid == 0)))
			return true;
		filterptr = filterptr->next;
	}
	
	return false;
}

bool CSectionsd::checkBlacklist(t_channel_id channel_id)
{
	ChannelBlacklist *blptr = CurrentBlacklist;
	
	while (blptr)
	{
		if (blptr->chan == (channel_id & blptr->mask))
			return true;
		blptr = blptr->next;
	}
	
	return false;
}

bool CSectionsd::checkNoDVBTimelist(t_channel_id channel_id)
{
	ChannelNoDVBTimelist *blptr = CurrentNoDVBTime;
	
	while (blptr)
	{
		if (blptr->chan == (channel_id & blptr->mask))
			return true;
		blptr = blptr->next;
	}
	return false;
}

void CSectionsd::addEPGFilter(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid)
{
	if (!checkEPGFilter(onid, tsid, sid))
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] Add EPGFilter for onid=\"%04x\" tsid=\"%04x\" service_id=\"%04x\"\n", onid, tsid, sid);

		EPGFilter *node = new EPGFilter;
		node->onid = onid;
		node->tsid = tsid;
		node->sid = sid;
		node->next = CurrentEPGFilter;
		CurrentEPGFilter = node;
	}
}

void CSectionsd::addBlacklist(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid)
{
	t_channel_id channel_id = create_channel_id(sid, onid, tsid);
	t_channel_id mask = create_channel_id((sid ? 0xFFFF : 0), (onid ? 0xFFFF : 0), (tsid ? 0xFFFF : 0));
	
	if (!checkBlacklist(channel_id))
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] Add Channel Blacklist for channel 0x%012llx, mask 0x%012llx\n", channel_id, mask);

		ChannelBlacklist *node = new ChannelBlacklist;
		node->chan = channel_id;
		node->mask = mask;
		node->next = CurrentBlacklist;
		CurrentBlacklist = node;
	}
}

void CSectionsd::addNoDVBTimelist(t_original_network_id onid, t_transport_stream_id tsid, t_service_id sid)
{
	t_channel_id channel_id = create_channel_id(sid, onid, tsid);

	t_channel_id mask = create_channel_id((sid ? 0xFFFF : 0), (onid ? 0xFFFF : 0), (tsid ? 0xFFFF : 0));

	if (!checkNoDVBTimelist(channel_id))
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] Add channel 0x%012llx, mask 0x%012llx to NoDVBTimelist\n", channel_id, mask);
		ChannelNoDVBTimelist *node = new ChannelNoDVBTimelist;
		node->chan = channel_id;
		node->mask = mask;
		node->next = CurrentNoDVBTime;
		CurrentNoDVBTime = node;
	}
}

// Loescht ein Event aus allen Mengen
bool CSectionsd::deleteEvent(const event_id_t uniqueKey)
{
	writeLockEvents();
	MySIeventsOrderUniqueKey::iterator e = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (e != mySIeventsOrderUniqueKey.end())
	{
		if (e->second->times.size())
		{
			mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.erase(e->second);
			mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.erase(e->second);
		}
		
		delete e->second;

		mySIeventsOrderUniqueKey.erase(uniqueKey);
		mySIeventsNVODorderUniqueKey.erase(uniqueKey);

		//printf("Deleting: %04x\n", (int) uniqueKey);
		
		unlockEvents();
		return true;
	}
	else
	{
		unlockEvents();
		return false;
	}
}

// Fuegt ein Event in alle Mengen ein
/* if cn == true (if called by cnThread), then myCurrentEvent and myNextEvent is updated, too */
void CSectionsd::addEvent(const SIevent &evt, const time_t zeit, bool cn)
{
	bool EPG_filtered = checkEPGFilter(evt.original_network_id, evt.transport_stream_id, evt.service_id);

	/* more readable in "plain english":
	   if current/next are not to be filtered and table_id is current/next -> continue
	   else {
		if epg filter is blacklist and filter matched -> stop. (return)
		if epg filter is whitelist and filter did not match -> stop also.
	   }
	 */

	if (!(epg_filter_except_current_next && (evt.table_id == 0x4e || evt.table_id == 0x4f)) && (evt.table_id != 0xFF)) 
	{
		if (!epg_filter_is_whitelist && EPG_filtered) 
		{
			//dprintf(DEBUG_DEBUG, "addEvent: blacklist and filter did match\n");
			return;
		}
		
		if (epg_filter_is_whitelist && !EPG_filtered) 
		{
			//dprintf(DEBUG_DEBUG, "addEvent: whitelist and filter did not match\n");
			return;
		}
	}

	if (cn) 
	{ 
		// current-next => fill current or next event...
		readLockMessaging();

		if (evt.get_channel_id() == messaging_current_servicekey && (messaging_got_CN != 0x03))
		{ 
			// ...and if we don't have them already.
			unlockMessaging();

			writeLockEvents();

			if (evt.runningStatus() > 2)
			{ 
				// paused or currently running
				if (!myCurrentEvent || (myCurrentEvent && (*myCurrentEvent).uniqueKey() != evt.uniqueKey())) 
				{
					if (myCurrentEvent)
						delete myCurrentEvent;
						
					myCurrentEvent = new SIevent(evt);
					
					writeLockMessaging();
					messaging_got_CN |= 0x01;
					
					if (myNextEvent && (*myNextEvent).uniqueKey() == evt.uniqueKey()) 
					{
						dprintf(DEBUG_DEBUG, "[sectionsd] addevent-cn: removing next-event\n");
						/* next got "promoted" to current => trigger re-read */
						delete myNextEvent;
						myNextEvent = NULL;
						messaging_got_CN &= 0x01;
					}
					
					unlockMessaging();
					dprintf(DEBUG_DEBUG, "[sectionsd] addevent-cn: added running (%d) event 0x%04x '%s'\n", evt.runningStatus(), evt.eventID, evt.getName().c_str());
				} 
				else 
				{
					writeLockMessaging();
					messaging_got_CN |= 0x01;
					unlockMessaging();
					dprintf(DEBUG_DEBUG, "[sectionsd] addevent-cn: not add runn. (%d) event 0x%04x '%s'\n", evt.runningStatus(), evt.eventID, evt.getName().c_str());
				}
			} 
			else 
			{
				if ((!myNextEvent || (myNextEvent && (*myNextEvent).uniqueKey() != evt.uniqueKey() && (*myNextEvent).times.begin()->startzeit < evt.times.begin()->startzeit)) && (!myCurrentEvent || (myCurrentEvent && (*myCurrentEvent).uniqueKey() != evt.uniqueKey()))) 
				{
					if (myNextEvent)
						delete myNextEvent;

					myNextEvent = new SIevent(evt);
					writeLockMessaging();
					messaging_got_CN |= 0x02;
					unlockMessaging();

					dprintf(DEBUG_DEBUG, "[sectionsd] addevent-cn: added next    (%d) event 0x%04x '%s'\n", evt.runningStatus(), evt.eventID, evt.getName().c_str());
				} 
				else 
				{
					dprintf(DEBUG_DEBUG, "[sectionsd] addevent-cn: not added next(%d) event 0x%04x '%s'\n", evt.runningStatus(), evt.eventID, evt.getName().c_str());

					writeLockMessaging();
					messaging_got_CN |= 0x02;
					unlockMessaging();
				}
			}
			unlockEvents();
		} 
		else
			unlockMessaging();
	}

	//
	readLockEvents();
	MySIeventsOrderUniqueKey::iterator si = mySIeventsOrderUniqueKey.find(evt.uniqueKey());
	bool already_exists = (si != mySIeventsOrderUniqueKey.end());
	
	if (already_exists && (evt.table_id < si->second->table_id))
	{
		/* if the new event has a lower (== more recent) table ID, replace the old one */
		already_exists = false;
		dprintf(DEBUG_DEBUG, "[sectionsd] replacing event %016llx:%02x with %04x:%02x '%.40s'\n", si->second->uniqueKey(), si->second->table_id, evt.eventID, evt.table_id, evt.getName().c_str());
	}
	else if (already_exists && ( (evt.table_id == 0x51 || evt.table_id == 0x50 || evt.table_id == 0x4e) && evt.table_id == si->second->table_id && evt.version != si->second->version ))
	{
		//replace event if new version
		dprintf(DEBUG_DEBUG, "[sectionsd] replacing event version old 0x%02x new 0x%02x'\n", si->second->version, evt.version );
		already_exists = false;
	}

	/* 
	   Check size of some descriptors of the new event before comparing
	   them with the old ones, because the same event can be complete
	   on one German Sky channel and incomplete on another one. So we
	   make sure to keep the complete event, if applicable. 
	*/

	if ((already_exists) && ( !evt.components.empty() )) 
	{
		if (si->second->components.size() != evt.components.size())
			already_exists = false;
		else 
		{
			SIcomponents::iterator c1 = si->second->components.begin();
			SIcomponents::iterator c2 = evt.components.begin();
			
			while ((c1 != si->second->components.end()) && (c2 != evt.components.end())) 
			{
				if ((c1->componentType != c2->componentType) ||
						(c1->componentTag != c2->componentTag) ||
						(c1->streamContent != c2->streamContent) ||
						(strcmp(c1->component.c_str(),c2->component.c_str()) != 0)) 
				{
					already_exists = false;
					break;
				}
				c1++;
				c2++;
			}
		}
	}

	if ((already_exists) && ( !evt.linkage_descs.empty() )) 
	{
		if (si->second->linkage_descs.size() != evt.linkage_descs.size())
			already_exists = false;
		else 
		{
			for (unsigned int i = 0; i < si->second->linkage_descs.size(); i++) 
			{
				if ((si->second->linkage_descs[i].linkageType !=
						evt.linkage_descs[i].linkageType) ||
						(si->second->linkage_descs[i].originalNetworkId !=
						 evt.linkage_descs[i].originalNetworkId) ||
						(si->second->linkage_descs[i].transportStreamId !=
						 evt.linkage_descs[i].transportStreamId) ||
						(strcmp(si->second->linkage_descs[i].name.c_str(),
							evt.linkage_descs[i].name.c_str()) != 0)) 
				{
					already_exists = false;
					break;
				}
			}
		}
	}

	if ((already_exists) && ( !evt.ratings.empty() )) 
	{
		if (si->second->ratings.size() != evt.ratings.size())
			already_exists = false;
		else 
		{
			SIparentalRatings::iterator p1 = si->second->ratings.begin();
			SIparentalRatings::iterator p2 = evt.ratings.begin();
			
			while ((p1 != si->second->ratings.end()) && (p2 != evt.ratings.end())) 
			{
				if ((p1->rating != p2->rating) || (strcmp(p1->countryCode.c_str(),p2->countryCode.c_str()) != 0)) 
				{
					already_exists = false;
					break;
				}
				p1++;
				p2++;
			}
		}
	}

	if (already_exists) 
	{
		if (si->second->times.size() != evt.times.size())
			already_exists = false;
		else 
		{
			SItimes::iterator t1 = si->second->times.begin();
			SItimes::iterator t2 = evt.times.begin();
			
			while ((t1 != si->second->times.end()) && (t2 != evt.times.end())) 
			{
				if ((t1->startzeit != t2->startzeit) || (t1->dauer != t2->dauer)) 
				{
					already_exists = false;
					break;
				}
				t1++;
				t2++;
			}
		}
	}

	if ((already_exists) && (SIlanguage::getMode() == LANGUAGE_MODE_OFF)) 
	{
		si->second->contentClassification = evt.contentClassification;
		si->second->userClassification = evt.userClassification;
		si->second->itemDescription = evt.itemDescription;
		si->second->item = evt.item;
		si->second->vps = evt.vps;
		
		if ((evt.getExtendedText().length() > 0) && (evt.times.begin()->startzeit < zeit + secondsExtendedTextCache))
			si->second->setExtendedText("OFF",evt.getExtendedText().c_str());
			
		if (evt.getText().length() > 0)
			si->second->setText("OFF",evt.getText().c_str());
			
		if (evt.getName().length() > 0)
			si->second->setName("OFF",evt.getName().c_str());
	}
	else 
	{
		SIevent *eptr = new SIevent(evt);

		if (!eptr)
		{
			dprintf(DEBUG_INFO, "[sectionsd] addEvent new SIevent failed.\n");
			unlockEvents();
			return;
		}

		SIeventPtr e(eptr);

		//Strip ExtendedDescription if too far in the future
		if ((e->times.begin()->startzeit > zeit + secondsExtendedTextCache) && (SIlanguage::getMode() == LANGUAGE_MODE_OFF) && (zeit != 0))
			e->setExtendedText("OFF","");

		/*
		 * this is test code, so indentation is deliberately wrong :-)
		 * we'll hopefully remove this if clause after testing is done
		 */
		if (slow_addevent)
		{
			std::vector<event_id_t> to_delete;
			unsigned short eventID = e->eventID;
			event_id_t e_key = e->uniqueKey();
			t_channel_id e_chid = e->get_channel_id();
			time_t start_time = e->times.begin()->startzeit;
			time_t end_time = e->times.begin()->startzeit + (long)e->times.begin()->dauer;
			/* create an event that's surely behind the one to check in the sort order */
			e->eventID = 0xFFFF; /* lowest order sort criteria is eventID */
			/* returns an iterator that's behind 'e' */
			MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator x =         mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.upper_bound(e);
			e->eventID = eventID;

			/* the first decrement of the iterator gives us an event that's a potential
			 * match *or* from a different channel, then no event for this channel is stored */
			while (x != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin())
			{
				x--;
				if ((*x)->get_channel_id() != e_chid)
					break;
				else
				{
					event_id_t x_key = (*x)->uniqueKey();
					/* do we need this check? */
					if (x_key == e_key)
						continue;
					if ((*x)->times.begin()->startzeit >= end_time)
						continue;
					/* iterating backwards: if the endtime of the stored events
					 * is earlier than the starttime of the new one, we'll never
					 * find an identical one => bail out */
					if ((*x)->times.begin()->startzeit + (long)(*x)->times.begin()->dauer <= start_time)
						break;
					/* here we have an overlapping event */
					dprintf(DEBUG_DEBUG, "[sectionsd] %s: delete 0x%016llx.%02x time = 0x%016llx.%02x\n", __func__, x_key, (*x)->table_id, e_key, e->table_id);
					to_delete.push_back(x_key);
				}
			}
			unlockEvents();

			while (! to_delete.empty())
			{
				deleteEvent(to_delete.back());
				to_delete.pop_back();
			}
		} 
		else 
		{
			// Damit in den nicht nach Event-ID sortierten Mengen
			// Mehrere Events mit gleicher ID sind, diese vorher loeschen
			unlockEvents();
		}
		
		deleteEvent(e->uniqueKey());
		readLockEvents();

		if (mySIeventsOrderUniqueKey.size() >= max_events) 
		{
			MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator lastEvent =
				mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin();

/* if you don't want the new "delete old events first" method but
 * the old-fashioned "delete future events always", invert this */
			time_t now = time(NULL);
			bool back = false;
			
			//if ((*lastEvent)->times.size() == 1)
			if (*lastEvent != NULL && (*lastEvent)->times.size() == 1)
			{
				if ((*lastEvent)->times.begin()->startzeit + (long)(*lastEvent)->times.begin()->dauer >= now - oldEventsAre)
					back = true;
			} 
			else
				printf("[sectionsd] addevent: times.size != 1, please report\n");

			if (back)
			{
				// fprintf(stderr, "<");
				lastEvent = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end();
				lastEvent--;

				//preserve events of current channel
				readLockMessaging();
				while ((lastEvent != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin()) &&
					((*lastEvent)->get_channel_id() == messaging_current_servicekey)) 
				{
					lastEvent--;
				}
				unlockMessaging();
			}
			// else fprintf(stderr, ">");
			unlockEvents();

			if(*lastEvent != NULL)
				deleteEvent((*lastEvent)->uniqueKey());
		}
		else
			unlockEvents();
		
		readLockEvents();
		
		// Pruefen ob es ein Meta-Event ist
		MySIeventUniqueKeysMetaOrderServiceUniqueKey::iterator i = mySIeventUniqueKeysMetaOrderServiceUniqueKey.find(e->get_channel_id());

		if (i != mySIeventUniqueKeysMetaOrderServiceUniqueKey.end())
		{
			// ist ein MetaEvent, d.h. mit Zeiten fuer NVOD-Event

			if (e->times.size())
			{
				// D.h. wir fuegen die Zeiten in das richtige Event ein
				MySIeventsOrderUniqueKey::iterator ie = mySIeventsOrderUniqueKey.find(i->second);

				if (ie != mySIeventsOrderUniqueKey.end())
				{
					// Event vorhanden
					// Falls das Event in den beiden Mengen mit Zeiten nicht vorhanden
					// ist, dieses dort einfuegen
					MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator i2 = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.find(ie->second);
					unlockEvents();
					writeLockEvents();

					if (i2 == mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end())
					{
						// nicht vorhanden -> einfuegen
						mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(ie->second);
						mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(ie->second);
					}

					// Und die Zeiten im Event updaten
					ie->second->times.insert(e->times.begin(), e->times.end());
				}
			}
		}
		unlockEvents();
		writeLockEvents();
		//printf("Adding: %04x\n", (int) e->uniqueKey());

		// normales Event
		mySIeventsOrderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));

		if (e->times.size())
		{
			// diese beiden Mengen enthalten nur Events mit Zeiten
			mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(e);
			mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(e);
		}
	}
	
	unlockEvents();
}

void CSectionsd::addNVODevent(const SIevent &evt)
{
	SIevent *eptr = new SIevent(evt);

	if (!eptr)
	{
		dprintf(DEBUG_INFO, "[sectionsd] addNVODevent: new SIevent failed.\n");
		return;
		//throw std::bad_alloc();
	}

	SIeventPtr e(eptr);

	readLockEvents();
	MySIeventsOrderUniqueKey::iterator e2 = mySIeventsOrderUniqueKey.find(e->uniqueKey());

	if (e2 != mySIeventsOrderUniqueKey.end())
	{
		// bisher gespeicherte Zeiten retten
		unlockEvents();
		writeLockEvents();
		e->times.insert(e2->second->times.begin(), e2->second->times.end());
	}
	unlockEvents();

	// Damit in den nicht nach Event-ID sortierten Mengen
	// mehrere Events mit gleicher ID sind, diese vorher loeschen
	deleteEvent(e->uniqueKey());
	readLockEvents();
	
	if (mySIeventsOrderUniqueKey.size() >= max_events) 
	{
		//FIXME: Set Old Events to 0 if limit is reached...
		MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator lastEvent =
			mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end();
		lastEvent--;

		//preserve events of current channel
		readLockMessaging();
		while ((lastEvent != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin()) &&
				((*lastEvent)->get_channel_id() == messaging_current_servicekey)) 
		{
			lastEvent--;
		}
		unlockMessaging();
		unlockEvents();
		deleteEvent((*lastEvent)->uniqueKey());
	}
	else
		unlockEvents();
	
	writeLockEvents();
	mySIeventsOrderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));

	mySIeventsNVODorderUniqueKey.insert(std::make_pair(e->uniqueKey(), e));
	unlockEvents();
	
	if (e->times.size())
	{
		// diese beiden Mengen enthalten nur Events mit Zeiten
		writeLockEvents();
		mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.insert(e);
		mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.insert(e);
		unlockEvents();
	}
}

void CSectionsd::removeOldEvents(const long seconds)
{
	bool goodtimefound;
	std::vector<event_id_t> to_delete;

	// Alte events loeschen
	time_t zeit = time(NULL);

	readLockEvents();

	MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin();

	while ((e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end()) && (!messaging_zap_detected)) 
	{
		goodtimefound = false;
		for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); t++) 
		{
			if (t->startzeit + (long)t->dauer >= zeit - seconds) 
			{
				goodtimefound=true;
				// one time found -> exit times loop
				break;
			}
		}

		if (false == goodtimefound)
			to_delete.push_back((*e)->uniqueKey());
		e++;
	}
	unlockEvents();

	for (std::vector<event_id_t>::iterator i = to_delete.begin(); i != to_delete.end(); i++)
		deleteEvent(*i);

	return;
}

//#ifdef REMOVE_DUPS
/* Remove duplicate events (same Service, same start and endtime)
 * with different eventID. Use the one from the lower table_id.
 * This routine could be extended to remove overlapping events also,
 * but let's keep that for later
 */
void CSectionsd::removeDupEvents(void)
{
	MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e1, e2, del;
	/* list of event IDs to delete */
	std::vector<event_id_t>to_delete;

	readLockEvents();
	e1 = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin();

	while ((e1 != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end()) && !messaging_zap_detected)
	{
		e2 = e1;
		e1++;
		if (e1 == mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end())
			break;

		/* check for the same service */
		if ((*e1)->get_channel_id() != (*e2)->get_channel_id())
			continue;
		/* check for same time */
		if (((*e1)->times.begin()->startzeit != (*e2)->times.begin()->startzeit) ||
			((*e1)->times.begin()->dauer     != (*e2)->times.begin()->dauer))
			continue;

		if ((*e1)->table_id == (*e2)->table_id)
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] %s: not removing events %llx %llx, t:%02x '%s'\n", __func__, (*e1)->uniqueKey(), (*e2)->uniqueKey(), (*e1)->table_id, (*e1)->getName().c_str());
			continue;
		}

		if ((*e1)->table_id > (*e2)->table_id)
			del = e1;
		if ((*e1)->table_id < (*e2)->table_id)
			del = e2;

		dprintf(DEBUG_DEBUG, "[sectionsd] %s: removing event %llx.%02x '%s'\n", __func__, (*del)->uniqueKey(), (*del)->table_id, (*del)->getName().c_str());
		/* remember the unique ID for later deletion */
		to_delete.push_back((*del)->uniqueKey());
	}
	unlockEvents();

	/* clean up outside of the iterator loop */
	for (std::vector<event_id_t>::iterator i = to_delete.begin(); i != to_delete.end(); i++)
		deleteEvent(*i);

	return;
}
//#endif

// SIservicePtr;
typedef SIservice *SIservicePtr;

typedef std::map<t_channel_id, SIservicePtr, std::less<t_channel_id> > MySIservicesOrderUniqueKey;
static MySIservicesOrderUniqueKey mySIservicesOrderUniqueKey;

typedef std::map<t_channel_id, SIservicePtr, std::less<t_channel_id> > MySIservicesNVODorderUniqueKey;
static MySIservicesNVODorderUniqueKey mySIservicesNVODorderUniqueKey;

const SIevent& CSectionsd::findSIeventForEventUniqueKey(const event_id_t eventUniqueKey)
{
	// Event (eventid) suchen
	MySIeventsOrderUniqueKey::iterator e = mySIeventsOrderUniqueKey.find(eventUniqueKey);

	if (e != mySIeventsOrderUniqueKey.end())
		return *(e->second);

	return nullEvt;
}

const SIevent& CSectionsd::findActualSIeventForServiceUniqueKey(const t_channel_id serviceUniqueKey, SItime& zeit, long plusminus, unsigned *flag)
{
	time_t azeit = time(NULL);

	if (flag != 0)
		*flag = 0;

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); ++e)
	{
		if ((*e)->get_channel_id() == serviceUniqueKey)
		{
			if (flag != 0)
				*flag |= epgflags::has_anything; // berhaupt was da...

			for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); ++t) 
			{
				if ((long)(azeit + plusminus) < (long)(t->startzeit + t->dauer))
				{
					if (flag != 0)
						*flag |= epgflags::has_later; // later events are present...

					if (t->startzeit <= (long)(azeit + plusminus))
					{
						//printf("azeit %d, startzeit+t->dauer %d \n", azeit, (long)(t->startzeit+t->dauer) );

						if (flag != 0)
							*flag |= epgflags::has_current; // aktuelles event da...

						zeit = *t;

						return *(*e);
					}
				}
			}
		}
	}

	return nullEvt;
}

const SIevent& CSectionsd::findNextSIeventForServiceUniqueKey(const t_channel_id serviceUniqueKey, SItime& zeit)
{
	time_t azeit = time(NULL);

	for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e != mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); e++)
	{
		if ((*e)->get_channel_id() == serviceUniqueKey)
		{
			for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); ++t)
				if ((long)(azeit) < (long)(t->startzeit + t->dauer))
				{
					zeit = *t;
					return *(*e);
				}
		}
	}

	return nullEvt;
}

// Sucht das naechste Event anhand unique key und Startzeit
const SIevent& CSectionsd::findNextSIevent(const event_id_t uniqueKey, SItime &zeit)
{
	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		SItimes::iterator nextnvodtimes = eFirst->second->times.end();
		SItimes::iterator nexttimes = eFirst->second->times.end();

		if (eFirst->second->times.size() > 1)
		{
			//find next nvod
			nextnvodtimes = eFirst->second->times.begin();
			while ( nextnvodtimes != eFirst->second->times.end() ) 
			{
				if ( nextnvodtimes->startzeit == zeit.startzeit )
					break;
				else
					++nextnvodtimes;
			}
		}

		MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator eNext;

		//Startzeit not first - we can't use the ordered list...
		for (MySIeventsOrderFirstEndTimeServiceIDEventUniqueKey::iterator e = mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.begin(); e !=
					mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.end(); ++e ) 
		{
			if ((*e)->get_channel_id() == eFirst->second->get_channel_id()) 
			{
				for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); ++t) 
				{
					if (t->startzeit > zeit.startzeit) 
					{
						if (nexttimes != eFirst->second->times.end()) 
						{
							if (t->startzeit < nexttimes->startzeit) 
							{
								eNext = e;
								nexttimes = t;
							}
						}
						else 
						{
							eNext = e;
							nexttimes = t;
						}
					}
				}
			}
		}

		if (nextnvodtimes != eFirst->second->times.end())
			++nextnvodtimes;
		//Compare
		if (nexttimes != eFirst->second->times.end()) 
		{
			if (nextnvodtimes != eFirst->second->times.end()) 
			{
				//both times are set - take the first
				if (nexttimes->startzeit < nextnvodtimes->startzeit) 
				{
					zeit = *nexttimes;
					return *(*eNext);

				} 
				else 
				{
					zeit = *nextnvodtimes;
					return *(eFirst->second);
				}
			} 
			else 
			{
				//only nexttimes set
				zeit = *nexttimes;
				return *(*eNext);
			}
		} 
		else if (nextnvodtimes != eFirst->second->times.end()) 
		{
			//only nextnvodtimes set
			zeit = *nextnvodtimes;
			return *(eFirst->second);
		}
	}

	return nullEvt;
}

// Sucht das naechste UND vorhergehende Event anhand unique key und Startzeit
void CSectionsd::findPrevNextSIevent(const event_id_t uniqueKey, SItime &zeit, SIevent &prev, SItime &prev_zeit, SIevent &next, SItime &next_zeit)
{
	prev = nullEvt;
	next = nullEvt;
	bool prev_ok = false;
	bool next_ok = false;

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end())
	{
		if (eFirst->second->times.size() > 1)
		{
			// Wir haben ein NVOD-Event
			// d.h. wir suchen die aktuelle Zeit und nehmen die naechste davon, falls existent

			for (SItimes::iterator t = eFirst->second->times.begin(); t != eFirst->second->times.end(); ++t)
			{
				if (t->startzeit == zeit.startzeit)
				{
					if (t != eFirst->second->times.begin())
					{
						--t;
						prev_zeit = *t;
						prev = *(eFirst->second);
						prev_ok = true;
						++t;
					}

					++t;

					if (t != eFirst->second->times.end())
					{
						next_zeit = *t;
						next = *(eFirst->second);
						next_ok = true;
					}

					if ( prev_ok && next_ok )
						return ; // beide gefunden...
					else
						break;
				}
			}
		}

		MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator eNext = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.find(eFirst->second);

		if ( (!prev_ok) && (eNext != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin() ) )
		{
			--eNext;

			if ((*eNext)->get_channel_id() == eFirst->second->get_channel_id())
			{
				prev_zeit = *((*eNext)->times.begin());
				prev = *(*eNext);
			}

			++eNext;
		}

		++eNext;

		if ( (!next_ok) && (eNext != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end()) )
		{
			if ((*eNext)->get_channel_id() == eFirst->second->get_channel_id())
			{
				next_zeit = *((*eNext)->times.begin());
				next = *(*eNext);
			}
		}
	}
}
 
//
void CSectionsd::pauseScanning(const bool doPause)
{
	int pause = doPause;

	if (pause && pause != 1)
		return ;

	dprintf(DEBUG_DEBUG, "CSectionsd::pauseScanning: Request of %s scanning.\n", pause ? "stop" : "continue" );

	if (scanning && pause)
	{
		dmxCN.request_pause();
		dmxEIT.request_pause();
		dmxFSEIT.request_pause();
		dmxVIASAT.request_pause();

		scanning = 0;
	}
	else if (!pause && !scanning)
	{
		dmxCN.request_unpause();
		dmxEIT.request_unpause();
		dmxFSEIT.request_unpause();
		dmxVIASAT.request_unpause();

		writeLockEvents();
		
		if (myCurrentEvent) 
		{
			delete myCurrentEvent;
			myCurrentEvent = NULL;
		}
		if (myNextEvent) 
		{
			delete myNextEvent;
			myNextEvent = NULL;
		}
		unlockEvents();
		writeLockMessaging();
		messaging_have_CN = 0x00;
		messaging_got_CN = 0x00;
		unlockMessaging();
		scanning = 1;
		
		if (!bTimeCorrect && !ntpenable)
		{
			pthread_mutex_lock(&timeThreadSleepMutex);
			pthread_cond_broadcast(&timeThreadSleepCond);
			pthread_mutex_unlock(&timeThreadSleepMutex);
		}

		scanning = 1;
		dmxCN.change(0);
		dmxEIT.change(0);
		dmxFSEIT.change(0);
		dmxVIASAT.change(0);
	}

	return ;
}

//
void CSectionsd::dumpStatus(void)
{
	dprintf(DEBUG_DEBUG, "[sectionsd] sectionsd_dumpStatus: Request of status information");

	readLockEvents();
	unsigned anzEvents = mySIeventsOrderUniqueKey.size();
	unsigned anzNVODevents = mySIeventsNVODorderUniqueKey.size();
	unsigned anzMetaServices = mySIeventUniqueKeysMetaOrderServiceUniqueKey.size();
	unlockEvents();

	readLockServices();
	unsigned anzServices = mySIservicesOrderUniqueKey.size();
	unsigned anzNVODservices = mySIservicesNVODorderUniqueKey.size();
	unlockServices();

	struct mallinfo speicherinfo = mallinfo();

	time_t zeit = time(NULL);

#define MAX_SIZE_STATI	2024
	char stati[MAX_SIZE_STATI];

	snprintf(stati, MAX_SIZE_STATI,
		 "$Id: sectionsd.cpp,v 1.35 05.03.2023 mohousch $\n"
		 "Current time: %s"
		 "Hours to cache: %ld\n"
		 "Hours to cache extended text: %ld\n"
		 "Events are old %ldmin after their end time\n"
		 "Number of cached services: %u\n"
		 "Number of cached nvod-services: %u\n"
		 "Number of cached events: %u\n"
		 "Number of cached nvod-events: %u\n"
		 "Number of cached meta-services: %u\n"
		 //    "Resource-usage: maxrss: %ld ixrss: %ld idrss: %ld isrss: %ld\n"
		 "Total size of memory occupied by chunks\n"
		 "handed out by malloc: %d (%dkb)\n"
		 "Total bytes memory allocated with `sbrk' by malloc,\n"
		 "in bytes: %d (%dkb)\n"
		 ,ctime(&zeit),
		 secondsToCache / (60*60L), secondsExtendedTextCache / (60*60L), oldEventsAre / 60, anzServices, anzNVODservices, anzEvents, anzNVODevents, anzMetaServices,
		 //    resourceUsage.ru_maxrss, resourceUsage.ru_ixrss, resourceUsage.ru_idrss, resourceUsage.ru_isrss,
		 speicherinfo.uordblks, speicherinfo.uordblks / 1024,
		 speicherinfo.arena, 
		 speicherinfo.arena / 1024
		);
	dprintf(DEBUG_NORMAL, "%s\n", stati);

	return ;
}

//
void CSectionsd::setServiceChanged(t_channel_id channel_id, bool requestEvent)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::setServiceChanged: Service changed to:%llx\n", channel_id);

	messaging_last_requested = time_monotonic();

	if(checkBlacklist(channel_id))
	{
		if (!channel_is_blacklisted) 
		{
			channel_is_blacklisted = true;
			dmxCN.request_pause();
			dmxEIT.request_pause();
		}
		
		dprintf(DEBUG_NORMAL, "CSectionsd::setServiceChanged: service is filtered!\n");
	}
	else
	{
		if (channel_is_blacklisted) 
		{
			channel_is_blacklisted = false;
			dmxCN.request_unpause();
			dmxEIT.request_unpause();

			dprintf(DEBUG_NORMAL, "CSectionsd::setServiceChanged: service is no longer filtered!\n");
		}
	}

	// dvbtime
	if(checkNoDVBTimelist(channel_id))
	{
		if (dvb_time_update) 
		{
			dvb_time_update = false;
		}
		
		dprintf(DEBUG_NORMAL, "CSectionsd::setServiceChanged: DVB time update is blocked!\n");
	}
	else
	{
		if (!dvb_time_update) 
		{
			dvb_time_update = true;
			dprintf(DEBUG_NORMAL, "CSectionsd::setServiceChanged: DVB time update is allowed!\n");
		}
	}

	// current
	if (messaging_current_servicekey != channel_id)
	{
		writeLockEvents();

		if (myCurrentEvent) 
		{
			delete myCurrentEvent;
			myCurrentEvent = NULL;
		}
		
		if (myNextEvent) 
		{
			delete myNextEvent;
			myNextEvent = NULL;
		}

		unlockEvents();
		
		//
		writeLockMessaging();
		messaging_current_servicekey = channel_id;
		messaging_have_CN = 0x00;
		messaging_got_CN = 0x00;
		messaging_zap_detected = true;

		messaging_need_eit_version = false;
		unlockMessaging();
		
		dmxCN.setCurrentService(messaging_current_servicekey & 0xffff);
		dmxEIT.setCurrentService(messaging_current_servicekey & 0xffff);
		dmxFSEIT.setCurrentService(messaging_current_servicekey & 0xffff);
		dmxVIASAT.setCurrentService(messaging_current_servicekey & 0xffff);
		
		// add localtv here
		readSIfromLocalTV(messaging_current_servicekey);
		// xmltv
		//readSIfromXMLTV(messaging_current_servicekey);
	}

	return;
}

//
bool CSectionsd::channel_in_requested_list(t_channel_id * clist, t_channel_id chid, int len)
{
	if(len == 0) 
		return true;
		
	for(int i = 0; i < len; i++) 
	{
		if(clist[i] == chid)
		{
			return true;
		}
	}
	
	return false;
}

//
void CSectionsd::setConfig(const epg_config config)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::setConfig\n");

	if (secondsToCache != (long)(config.epg_cache)*24*60L*60L) 
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new epg_cache = %d\n", config.epg_cache);
		
		writeLockEvents();
		secondsToCache = (long)(config.epg_cache)*24*60L*60L;
		unlockEvents();
	}

	if (oldEventsAre != (long)(config.epg_old_events)*60L*60L) 
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new epg_old_events = %d\n", config.epg_old_events);
		
		writeLockEvents();
		oldEventsAre = (long)(config.epg_old_events)*60L*60L;
		unlockEvents();
	}
	
	if (secondsExtendedTextCache != (long)(config.epg_extendedcache)*60L*60L) 
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new epg_extendedcache = %d\n", config.epg_extendedcache);
		
		//lockEvents();
		writeLockEvents();
		secondsExtendedTextCache = (long)(config.epg_extendedcache)*60L*60L;
		unlockEvents();
	}
	
	if (max_events != config.epg_max_events) 
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new epg_max_events = %d\n", config.epg_max_events);
		writeLockEvents();
		max_events = config.epg_max_events;
		unlockEvents();
	}

	if (ntprefresh != config.network_ntprefresh) 
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new network_ntprefresh = %d\n", config.network_ntprefresh);
		
		pthread_mutex_lock(&timeThreadSleepMutex);
		ntprefresh = config.network_ntprefresh;
		if (timeset) 
		{
			// wake up time thread
			pthread_cond_broadcast(&timeThreadSleepCond);
		}
		pthread_mutex_unlock(&timeThreadSleepMutex);
	}

	if (ntpenable ^ (config.network_ntpenable == 1))	
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] new network_ntpenable = %d\n", config.network_ntpenable);
		
		pthread_mutex_lock(&timeThreadSleepMutex);
		ntpenable = (config.network_ntpenable == 1);
		
		if (timeset) 
		{
			// wake up time thread
			pthread_cond_broadcast(&timeThreadSleepCond);
		}
		pthread_mutex_unlock(&timeThreadSleepMutex);
	}

	if (!config.network_ntpserver.empty()) 
	{
		ntpserver = config.network_ntpserver;
		dprintf(DEBUG_DEBUG, "[sectionsd] new network_ntpserver = %s\n", ntpserver.c_str());
		
		// check for rdate
		if( !access("/bin/rdate", F_OK) || !access("/sbin/rdate", F_OK) || !access("/usr/bin/rdate", F_OK) || !access("/usr/sbin/rdate", F_OK)) 
			ntp_system_cmd_prefix = "rdate -s ";
		ntp_system_cmd = ntp_system_cmd_prefix + ntpserver;
	}

	if (!config.epg_dir.empty())
	{
		epg_dir = config.epg_dir;
		dprintf(DEBUG_DEBUG, "[sectionsd] new epg_dir = %s\n", epg_dir.c_str());
	}
	
	return ;
}

void CSectionsd::deleteSIexceptEPG()
{
	writeLockServices();
	mySIservicesOrderUniqueKey.clear();
	unlockServices();
	
	dmxEIT.dropCachedSectionIDs();
}

//
void CSectionsd::freeMemory()
{
	dprintf(DEBUG_NORMAL, "CSectionsd::freeMemory:\n");
	
	deleteSIexceptEPG();

	writeLockEvents();
	
	mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.clear();
	mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.clear();
	mySIeventsOrderUniqueKey.clear();
	mySIeventsNVODorderUniqueKey.clear();
	
	unlockEvents();
	
	return ;
}

// fromFile
void *CSectionsd::insertEventsfromFile(void *)
{
	dprintf(DEBUG_INFO, "[sectionsd] insertEventsfromFile: tid %ld\n", syscall(__NR_gettid));

	_xmlDocPtr event_parser = NULL;
	_xmlNodePtr eventfile = NULL;
	_xmlNodePtr service = NULL;
	_xmlNodePtr event = NULL;
	_xmlNodePtr node = NULL;
	t_original_network_id onid = 0;
	t_transport_stream_id tsid = 0;
	t_service_id sid = 0;
	char cclass[20] = {0};
	char cuser[20] = {0};;
	std::string indexname;
	std::string filename;
	std::string epgname;
	int ev_count = 0;

	struct stat buf;
	indexname = epg_dir + "index.tmp";

	//skip read EPG cache if index.tmp available
	if (stat(indexname.c_str(), &buf) != 0)
	{
		indexname = epg_dir + "index.xml";

		_xmlDocPtr index_parser = parseXmlFile(indexname.c_str());

		if (index_parser != NULL) 
		{
			time_t now = time_monotonic_ms();
			
			dprintf(DEBUG_INFO, "[sectionsd] Reading Information from file %s:\n", indexname.c_str());

			eventfile = xmlDocGetRootElement(index_parser)->xmlChildrenNode;

			while (eventfile) 
			{
				filename = xmlGetAttribute(eventfile, "name");
				epgname = epg_dir + filename;
				
				if (!(event_parser = parseXmlFile(epgname.c_str()))) 
				{
					dprintf(DEBUG_DEBUG, "[sectionsd] unable to open %s for reading\n", epgname.c_str());
				}
				else 
				{
					service = xmlDocGetRootElement(event_parser)->xmlChildrenNode;

					while (service) 
					{
						onid = xmlGetNumericAttribute(service, "original_network_id", 16);
						tsid = xmlGetNumericAttribute(service, "transport_stream_id", 16);
						sid = xmlGetNumericAttribute(service, "service_id", 16);

						event = service->xmlChildrenNode;

						while (event) 
						{
							SIevent e(onid, tsid, sid, xmlGetNumericAttribute(event, "id", 16));

							node = event->xmlChildrenNode;

							//
							while (xmlGetNextOccurence(node, "name") != NULL) 
							{
								e.setName(std::string(UTF8_to_Latin1(xmlGetAttribute(node, "lang"))), std::string(xmlGetAttribute(node, "string")));

								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "text") != NULL) 
							{
								e.setText(std::string(UTF8_to_Latin1(xmlGetAttribute(node, "lang"))), std::string(xmlGetAttribute(node, "string")));

								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "item") != NULL) 
							{
								e.item = std::string(xmlGetAttribute(node, "string"));
								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "item_description") != NULL) 
							{
								e.itemDescription = std::string(xmlGetAttribute(node, "string"));
								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "extended_text") != NULL) 
							{
								e.appendExtendedText(std::string(UTF8_to_Latin1(xmlGetAttribute(node, "lang"))), std::string(xmlGetAttribute(node, "string")));

								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "time") != NULL) 
							{
								e.times.insert(SItime(xmlGetNumericAttribute(node, "start_time", 10), xmlGetNumericAttribute(node, "duration", 10)));

								node = node->xmlNextNode;
							}

							//
							unsigned int count = 0;
							while (xmlGetNextOccurence(node, "content") != NULL) 
							{
								cclass[count] = xmlGetNumericAttribute(node, "class", 16);
								cuser[count] = xmlGetNumericAttribute(node, "user", 16);
								node = node->xmlNextNode;
								count++;
								if(count > sizeof(cclass)-1)
									break;
							}
							e.contentClassification = std::string(cclass, count);
							e.userClassification = std::string(cuser, count);

							//
							while (xmlGetNextOccurence(node, "component") != NULL) 
							{
								SIcomponent c;
								c.streamContent = xmlGetNumericAttribute(node, "stream_content", 16);
								c.componentType = xmlGetNumericAttribute(node, "type", 16);
								c.componentTag = xmlGetNumericAttribute(node, "tag", 16);
								c.component = std::string(xmlGetAttribute(node, "text"));
								e.components.insert(c);
								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "parental_rating") != NULL) 
							{
								e.ratings.insert(SIparentalRating(std::string(UTF8_to_Latin1(xmlGetAttribute(node, "country"))), (unsigned char) xmlGetNumericAttribute(node, "rating", 10)));
								node = node->xmlNextNode;
							}

							//
							while (xmlGetNextOccurence(node, "linkage") != NULL) 
							{
								SIlinkage l;
								l.linkageType = xmlGetNumericAttribute(node, "type", 16);
								l.transportStreamId = xmlGetNumericAttribute(node, "transport_stream_id", 16);
								l.originalNetworkId = xmlGetNumericAttribute(node, "original_network_id", 16);
								l.serviceId = xmlGetNumericAttribute(node, "service_id", 16);
								l.name = std::string(xmlGetAttribute(node, "linkage_descriptor"));
								e.linkage_descs.insert(e.linkage_descs.end(), l);

								node = node->xmlNextNode;
							}
							
							//writeLockEvents();
							CSectionsd::getInstance()->addEvent(e, 0);
							ev_count++;
							//unlockEvents();

							event = event->xmlNextNode;
						}

						service = service->xmlNextNode;
					}
					xmlFreeDoc(event_parser);
				}

				eventfile = eventfile->xmlNextNode;
			}

			xmlFreeDoc(index_parser);

			dprintf(DEBUG_INFO, "[sectionsd] Reading Information finished after %ld miliseconds (%d events)\n", time_monotonic_ms() - now, ev_count);
		}
	}

	reader_ready = true;

	pthread_exit(NULL);
}

// xmltv thread
void *CSectionsd::insertEventsfromXMLTV(void* data)
{
	dprintf(DEBUG_INFO, "[sectionsd] insertEventsfromXMLTV: tid %ld\n", syscall(__NR_gettid));
	
	//
	if (!data)
	{
		reader_ready = true;
		pthread_exit(NULL);
	}
	
	std::string url = (std::string)(char *) data;
	//t_channel_id chid = (t_channel_id)data;
	
	dprintf(DEBUG_INFO, "[sectionsd] sectionsd:insertEventsfromXMLTV: chid:%s\n", url);
	
	//
	//CZapitChannel *channel = g_bouquetManager->findChannelByChannelID(chid);
	//std::string url;
	//if (channel) url = channel->getEPGUrl();
	//if (url.empty())
	//	return 0;

	std::string answer;

	//
	unsigned short id = 0;
	time_t start_time;
	time_t stop_time;
	unsigned duration = 0;
	char* title = NULL;
	char* description = NULL;
	char* descriptionextended = NULL;
	
	//
	unsigned int ev_count = 0;

	answer = randomFile(::getFileExt(url), "/tmp", 8);
	
	if (!::downloadUrl(url, answer))
		return 0;
	
	//
	_xmlNodePtr event = NULL;
	_xmlNodePtr node = NULL;

	//
	_xmlDocPtr index_parser = parseXmlFile(answer.c_str());

	if (index_parser != NULL) 
	{
		event = xmlDocGetRootElement(index_parser);
		node = event->xmlChildrenNode;
			
		if (node)
		{
			while ((node = xmlGetNextOccurence(node, "programme")))
			{
				const char *chan = xmlGetAttribute(node, "channel");
				const char *start = xmlGetAttribute(node, "start");
				const char *stop  = xmlGetAttribute(node, "stop");
				
				for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				{
					if ( IS_WEBTV(it->second.getChannelID()) )
					{
						// check epgidname // channel
						if ( (strcmp(chan, it->second.getEPGIDName().c_str()) == 0) && (!it->second.getEPGUrl().empty()) )
						{
							struct tm starttime, stoptime;
							strptime(start, "%Y%m%d%H%M%S %z", &starttime);
							strptime(stop, "%Y%m%d%H%M%S %z", &stoptime);

							time_t start_time = mktime(&starttime) + starttime.tm_gmtoff;
							time_t duration = mktime(&stoptime) + stoptime.tm_gmtoff - start_time;

							//
							time_t current_time;
							time(&current_time);
							double time_diff = difftime(current_time, start_time + duration);

							//
							SIevent e(GET_ORIGINAL_NETWORK_ID_FROM_CHANNEL_ID(it->second.getChannelID()), GET_TRANSPORT_STREAM_ID_FROM_CHANNEL_ID(it->second.getChannelID()), GET_SERVICE_ID_FROM_CHANNEL_ID(it->second.getChannelID()), ev_count + 0x8000);
							e.table_id = 0x50;
							e.times.insert(SItime(start_time, duration));
								
							_xmlNodePtr _node = node->xmlChildrenNode;	
							while ((_node = xmlGetNextOccurence(_node, "title")))
							{
								const char *title = xmlGetData(_node);
									
								if(title != NULL)
									e.setName(std::string(::UTF8_to_Latin1("deu")), std::string(title));
								_node = _node->xmlChildrenNode;
							}
								
							//
							_node = node->xmlChildrenNode;
							while ((_node = xmlGetNextOccurence(_node, "sub-title")))
							{
								const char *subtitle = xmlGetData(_node);
								if(subtitle != NULL)
									e.setText(std::string(::UTF8_to_Latin1("deu")), std::string(subtitle));
								_node = _node->xmlChildrenNode;
							}
								
							//
							_node = node->xmlChildrenNode;;
							while ((_node = xmlGetNextOccurence(_node, "desc")))
							{
								const char *description = xmlGetData(_node);
								if(description != NULL)													 										e.appendExtendedText(std::string(::UTF8_to_Latin1("deu")), std::string(description));
								_node = _node->xmlChildrenNode;
							}

							CSectionsd::getInstance()->addEvent(e, ev_count);

							ev_count++;
						}
					}
				}

				node = node->xmlNextNode;	
			}
		}
	}
	
	xmlFreeDoc(index_parser);	

	unlink(answer.c_str());

	reader_ready = true;
	pthread_exit(NULL);
}

void *CSectionsd::insertEventsfromLocalTV(void *data)
{
	dprintf(DEBUG_INFO, "CSectionsd:insertEventsfromLocalTV: chid:%llx\n", (t_channel_id)data);
	
	//
	t_channel_id chid = (t_channel_id)data;
	t_channel_id epgid = 0;
	CZapitChannel *chan = NULL;
	t_satellite_position  satellitePosition = 0;
	
	t_original_network_id _onid = GET_ORIGINAL_NETWORK_ID_FROM_CHANNEL_ID(chid);
	t_transport_stream_id _tsid = GET_TRANSPORT_STREAM_ID_FROM_CHANNEL_ID(chid);
	t_service_id _sid = GET_SERVICE_ID_FROM_CHANNEL_ID(chid);
	
	chan = g_bouquetManager->findChannelByChannelID(chid);
	
	if (chan)
	{
		satellitePosition = chan->getSatellitePosition();
		epgid = chan->getEPGID();
	}
	
	dprintf(DEBUG_INFO, "CSectionsd:insertEventsfromLocalTV:epgid: %llx\n", epgid);
	
	// localtv
	std::string evUrl;

	if(g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2)
	{
		evUrl = "http://";
		evUrl += g_settings.epg_serverbox_ip;
		evUrl += "/web/epgservice?sRef=1:0:"; 

		evUrl += to_hexstring(1);
		evUrl += ":";
		evUrl += to_hexstring(GET_SERVICE_ID_FROM_CHANNEL_ID(epgid)); //sid
		evUrl += ":";
		evUrl += to_hexstring(GET_TRANSPORT_STREAM_ID_FROM_CHANNEL_ID(epgid)); //tsid
		evUrl += ":";
		evUrl += to_hexstring(GET_ORIGINAL_NETWORK_ID_FROM_CHANNEL_ID(epgid)); //onid
		evUrl += ":";

		if(g_settings.epg_serverbox_type == DVB_C)
		{
			evUrl += "FFFF"; // namenspace for cable
		}
		else if (g_settings.epg_serverbox_type == DVB_T)
		{
			evUrl += "EEEE"; // namenspace for terrestrial
		}
		else if (g_settings.epg_serverbox_type == DVB_S)
		{
			// namenspace for sat
			evUrl += to_hexstring(satellitePosition); //satpos
		}

		evUrl += "0000";
		evUrl += ":";
		evUrl += "0:0:0:";
	}
	else if( (g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_NMP) || (g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_NHD2) )
	{
		evUrl = "http://";
		evUrl += g_settings.epg_serverbox_ip;
		evUrl += "/control/epg?channelid=";

         	evUrl += to_hexstring(epgid);

		evUrl += "&xml=true&details=true";
	}

	//
	std::string answer;

	//
	unsigned short id = 0;
	time_t start_time;
	time_t stop_time;
	unsigned duration = 0;
	char* title = NULL;
	char* description = NULL;
	char* descriptionextended = NULL;

	answer = "/tmp/epg.xml";
	
	if (!::downloadUrl(evUrl, answer))
		return 0;

	if(g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_NHD2)
	{
		//N2
		/*
		<epglist>
			<channel_id>bf270f2b5e</channel_id>
			<channel_short_id>bf270f2b5e</channel_short_id>
			<channel_name>Disney SD</channel_name>
			<prog>
				<bouquetnr>0</bouquetnr>
				<channel_id>bf270f2b5e</channel_id>
				<eventid>365560600707</eventid>
				<eventid_hex>551d1c1883</eventid_hex>
				<start_sec>1609424700</start_sec>
				<start_t>15:25</start_t>
				<date>31.12.2020</date>
				<stop_sec>1609426200</stop_sec>
				<stop_t>15:50</stop_t>
				<duration_min>25</duration_min>
				<description>Phineas und Ferb</description>
				<info1>
				Carl liest Major Monogram eine Geschichte vor diese erzählt von einer abenteuerlichen Reise von Phineas und Ferb.
				</info1>
				<info2>
				Phineas, Ferb und ihre nervige Schwester haben Sommerferien. Die zwei erfinderischen Jungen denken sich immer neue, abenteuerliche Dinge aus, um Spass zu haben und den Sommer zu nutzen.Candice dagegen findet wenig Vergnügen an den Abenteuern ihrer Brüder. Bei jeder Gelegenheit versucht sie, die Beiden bei ihrer Mutter zu verpetzen.
				</info2>
			</prog>
		-</epglist>
		*/

		//
		_xmlNodePtr event = NULL;
		_xmlNodePtr node = NULL;

		//
		_xmlDocPtr index_parser = parseXmlFile(answer.c_str());

		if (index_parser != NULL) 
		{
			event = xmlDocGetRootElement(index_parser)->xmlChildrenNode;

			while (event) 
			{
				node = event->xmlChildrenNode;

				// bouquetnr
				while(xmlGetNextOccurence(node, "bouquetnr") != NULL)
				{
					node = node->xmlNextNode;
				}

				// channel_id
				while(xmlGetNextOccurence(node, "channel_id") != NULL)
				{
					node = node->xmlNextNode;
				}

				// eventid
				while(xmlGetNextOccurence(node, "eventid") != NULL)
				{
					id = atoi(xmlGetData(node) + 10);

					node = node->xmlNextNode;
				}

				// eventid_hex
				while(xmlGetNextOccurence(node, "eventid_hex") != NULL)
				{
					node = node->xmlNextNode;
				}

				//start_sec
				while(xmlGetNextOccurence(node, "start_sec") != NULL)
				{
					start_time = (time_t)atoi(xmlGetData(node));

					node = node->xmlNextNode;
				}

				// start_t
				while(xmlGetNextOccurence(node, "start_t") != NULL)
				{
					node = node->xmlNextNode;
				}

				// date
				while(xmlGetNextOccurence(node, "date") != NULL)
				{
					node = node->xmlNextNode;
				}
	
				//stop_sec
				while(xmlGetNextOccurence(node, "stop_sec") != NULL)
				{
					stop_time = (time_t)atoi(xmlGetData(node));
					duration = stop_time - start_time;

					node = node->xmlNextNode;
				}

				// stop_t
				while(xmlGetNextOccurence(node, "stop_t") != NULL)
				{
					node = node->xmlNextNode;
				}

				// duration_min
				while(xmlGetNextOccurence(node, "duration_min") != NULL)
				{
					node = node->xmlNextNode;
				}

				//description (title)
				while(xmlGetNextOccurence(node, "description") != NULL)
				{
					title = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				// info1 (description)
				while(xmlGetNextOccurence(node, "info1") != NULL)
				{
					description = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				//info2 (descriptionextended)
				while(xmlGetNextOccurence(node, "info2") != NULL)
				{
					descriptionextended = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				//
				SIevent e(_onid, _tsid, _sid, id);

				e.times.insert(SItime(start_time, duration));
				if(title != NULL)
					e.setName(std::string(UTF8_to_Latin1("ger")), std::string(title));

				if(description != NULL)
					e.setText(std::string(UTF8_to_Latin1("ger")), std::string(description));

				if(descriptionextended != NULL)
					e.appendExtendedText(std::string(UTF8_to_Latin1("ger")), std::string(descriptionextended));

				CSectionsd::getInstance()->addEvent(e, 0);
				
				event = event->xmlNextNode;
			}

			xmlFreeDoc(index_parser);
		}
	}
	else if(g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_NMP)
	{
		//NMP
		/*
		-<epglist>
			<channel_name><![CDATA[XITE]]></channel_name>
			<channel_id>f1270f5e38</channel_id>
			<channel_short_id>f1270f5e38</channel_short_id>
			<epg_id></epg_id>
			<short_epg_id></short_epg_id>
			-<prog>
				<bouquetnr>0</bouquetnr>
				<channel_id>f1270f5e38</channel_id>
				<epg_id>b24403f300012b66</epg_id>
				<eventid>67878416345993535</eventid>
				<eventid_hex>f1270f5e38113f</eventid_hex>
				<start_sec>1483102800</start_sec>
				<start_t>14:00</start_t>
				<date>30.12.2016</date>
				<stop_sec>1483117200</stop_sec>
				<stop_t>18:00</stop_t>
				<duration_min>240</duration_min>
				<info1><![CDATA[Xite wishes you happy holidays! To complete the holiday spirit, we have made a mix of all of your favourite music.]]></info1>
				<info2><![CDATA[Xite wishes you happy holidays! To complete the holiday spirit, we have made a mix of all of your favourite music.]]></info2>
				<description><![CDATA[Happy Holidays]]></description>
			-</prog>
		-</epglist>
		*/

		//
		_xmlNodePtr event = NULL;
		_xmlNodePtr node = NULL;

		//
		_xmlDocPtr index_parser = parseXmlFile(answer.c_str());

		if (index_parser != NULL) 
		{
			event = xmlDocGetRootElement(index_parser)->xmlChildrenNode;

			while (event) 
			{
				node = event->xmlChildrenNode;

				// bouquetnr
				while(xmlGetNextOccurence(node, "bouquetnr") != NULL)
				{
					node = node->xmlNextNode;
				}

				// channel_id
				while(xmlGetNextOccurence(node, "channel_id") != NULL)
				{
					node = node->xmlNextNode;
				}

				// epg_id
				while(xmlGetNextOccurence(node, "epg_id") != NULL)
				{
					node = node->xmlNextNode;
				}

				// eventid
				while(xmlGetNextOccurence(node, "eventid") != NULL)
				{
					id = atoi(xmlGetData(node) + 10);

					node = node->xmlNextNode;
				}

				// eventid_hex
				while(xmlGetNextOccurence(node, "eventid_hex") != NULL)
				{
					node = node->xmlNextNode;
				}

				//start_sec
				while(xmlGetNextOccurence(node, "start_sec") != NULL)
				{
					start_time = (time_t)atoi(xmlGetData(node));

					node = node->xmlNextNode;
				}

				// start_t
				while(xmlGetNextOccurence(node, "start_t") != NULL)
				{
					node = node->xmlNextNode;
				}

				// date
				while(xmlGetNextOccurence(node, "date") != NULL)
				{
					node = node->xmlNextNode;
				}
	
				//stop_sec
				while(xmlGetNextOccurence(node, "stop_sec") != NULL)
				{
					stop_time = (time_t)atoi(xmlGetData(node));
					duration = stop_time - start_time;

					node = node->xmlNextNode;
				}

				// stop_t
				while(xmlGetNextOccurence(node, "stop_t") != NULL)
				{
					node = node->xmlNextNode;
				}

				// duration_min
				while(xmlGetNextOccurence(node, "duration_min") != NULL)
				{
					node = node->xmlNextNode;
				}

				// info1 (description)
				while(xmlGetNextOccurence(node, "info1") != NULL)
				{
					description = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				//info2 (descriptionextended)
				while(xmlGetNextOccurence(node, "info2") != NULL)
				{
					descriptionextended = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				//description (title)
				while(xmlGetNextOccurence(node, "description") != NULL)
				{
					title = xmlGetData(node);
					
					node = node->xmlNextNode;
				}

				//
				SIevent e(_onid, _tsid, _sid, id);

				e.times.insert(SItime(start_time, duration));
				if(title != NULL)
					e.setName(std::string(UTF8_to_Latin1("ger")), std::string(title));

				if(description != NULL)
					e.setText(std::string(UTF8_to_Latin1("ger")), std::string(description));

				if(descriptionextended != NULL)
					e.appendExtendedText(std::string(UTF8_to_Latin1("ger")), std::string(descriptionextended));

				CSectionsd::getInstance()->addEvent(e, 0);
				
				event = event->xmlNextNode;
			}

			xmlFreeDoc(index_parser);
		}
	}
	else if(g_settings.epg_serverbox_gui == SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2)
	{
		/*
		<e2eventlist>
			<e2event>
				<e2eventid></e2eventid>
				<e2eventstart></e2eventstart>
				<e2eventduration></e2eventduration>
				<e2eventcurrenttime></e2eventcurrenttime>
				<e2eventtitle></e2eventtitle>
				<e2eventdescription></e2eventdescription>
				<e2eventdescriptionextended></e2eventdescriptionextended>
				<e2eventservicereference></e2eventservicereference>
				<e2eventservicename></e2eventservicename>
			</e2event>
		</e2eventlist>
		*/

		//
		_xmlNodePtr event = NULL;
		_xmlNodePtr node = NULL;

		//
		_xmlDocPtr index_parser = parseXmlFile(answer.c_str());

		if (index_parser != NULL) 
		{
			event = xmlDocGetRootElement(index_parser)->xmlChildrenNode;

			while (event) 
			{
				node = event->xmlChildrenNode;

				//e2eventid
				while(xmlGetNextOccurence(node, "e2eventid") != NULL)
				{
					id = atoi(xmlGetData(node));
					node = node->xmlNextNode;
				}

				//e2eventstart
				while(xmlGetNextOccurence(node, "e2eventstart") != NULL)
				{
					start_time = (time_t)atoi(xmlGetData(node));
					node = node->xmlNextNode;
				}
	
				//e2eventduration
				while(xmlGetNextOccurence(node, "e2eventduration") != NULL)
				{
					duration = (unsigned)atoi(xmlGetData(node));
					node = node->xmlNextNode;
				}

				//e2eventcurrenttime
				while(xmlGetNextOccurence(node, "e2eventcurrenttime") != NULL)
				{
					node = node->xmlNextNode;
				}

				//e2eventtitle
				while(xmlGetNextOccurence(node, "e2eventtitle") != NULL)
				{
					title = xmlGetData(node);
					node = node->xmlNextNode;
				}

				//e2eventdescription
				while(xmlGetNextOccurence(node, "e2eventdescription") != NULL)
				{
					description = xmlGetData(node);
					node = node->xmlNextNode;
				}

				//e2eventdescriptionextended
				while(xmlGetNextOccurence(node, "e2eventdescriptionextended") != NULL)
				{
					descriptionextended = xmlGetData(node);
					node = node->xmlNextNode;
				}

				//e2eventservicereference
				while(xmlGetNextOccurence(node, "e2eventservicereference") != NULL)
				{
					node = node->xmlNextNode;
				}

				//e2eventservicename
				while(xmlGetNextOccurence(node, "e2eventservicename") != NULL)
				{
					node = node->xmlNextNode;
				}

				//
				SIevent e(_onid, _tsid, _sid, id);

				e.times.insert(SItime(start_time, duration));
				if(title != NULL)
					e.setName(std::string(UTF8_to_Latin1("ger")), std::string(title));

				if(description != NULL)
					e.setText(std::string(UTF8_to_Latin1("ger")), std::string(description));

				if(descriptionextended != NULL)
					e.appendExtendedText(std::string(UTF8_to_Latin1("ger")), std::string(descriptionextended));

				CSectionsd::getInstance()->addEvent(e, 0);
				
				event = event->xmlNextNode;
			}

			xmlFreeDoc(index_parser);
		}
	}

	unlink(answer.c_str());
	reader_ready = true;
	pthread_exit(NULL);
}

//
void CSectionsd::readSIfromXML(const char *epgxmlname)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::readSIfromXML: %s\n", epgxmlname);

	pthread_t thrInsert;

	writeLockMessaging();
	epg_dir = (std::string)epgxmlname + "/";
	unlockMessaging();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create (&thrInsert, &attr, insertEventsfromFile, 0 ))
	{
		perror("sectionsd: sectionsd_readSIfromXML: pthread_create()");
	}

	pthread_attr_destroy(&attr);

	return ;
}

// fromXMLTV
//
void CSectionsd::readSIfromXMLTV(const char *url)
//void CSectionsd::readSIfromXMLTV(const t_channel_id chid)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::readSIfromXMLTV: %s\n", url);

	pthread_t thrInsert;

	if (!url)
		return ;
	//if ( !chid && !IS_WEBTV(chid) )
	//	return;
		
	//if(!g_settings.epg_xmltv)
	//	return;

	writeLockMessaging();
	static std::string url_tmp = url;
	unlockMessaging();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create (&thrInsert, &attr, insertEventsfromXMLTV, (void *)url_tmp.c_str()))
	{
		perror("sectionsd: sectionsd_readSIfromXMLTV: pthread_create()");
	}

	pthread_attr_destroy(&attr);

	return ;
}

//
void CSectionsd::readSIfromLocalTV(const t_channel_id chid)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::readSIfromLocalTV: %llx\n", chid);

	pthread_t thrInsert;

	if ( !chid && !IS_WEBTV(chid) )
		return;
		
	if(!g_settings.epg_enable_localtv_epg)
		return;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create (&thrInsert, &attr, insertEventsfromLocalTV, (void *)chid))
	{
		perror("sectionsd: sectionsd_readSIfromLocalTV: pthread_create failed");
	}

	pthread_attr_destroy(&attr);

	return ;
}

//
void CSectionsd::write_epg_xml_header(FILE * fd, const t_original_network_id onid, const t_transport_stream_id tsid, const t_service_id sid)
{
	fprintf(fd,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!--\n"
		"  This file was automatically generated by the sectionsd.\n"
		"  It contains all event entries which have been cached\n"
		"  at time the box was shut down.\n"
		"-->\n"
		"<dvbepg>\n");
	fprintf(fd,"\t<service original_network_id=\"%04x\" transport_stream_id=\"%04x\" service_id=\"%04x\">\n", onid, tsid, sid);
}

void CSectionsd::write_index_xml_header(FILE * fd)
{
	fprintf(fd,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!--\n"
		"  This file was automatically generated by the sectionsd.\n"
		"  It contains all event entries which have been cached\n"
		"  at time the box was shut down.\n"
		"-->\n"
		"<dvbepgfiles>\n");
}

void CSectionsd::write_epgxml_footer(FILE *fd)
{
	fprintf(fd, "\t</service>\n");
	fprintf(fd, "</dvbepg>\n");
}

void CSectionsd::write_indexxml_footer(FILE *fd)
{
	fprintf(fd, "</dvbepgfiles>\n");
}

//
void CSectionsd::writeSI2XML(const char *epgxmlname)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::writeSI2XML:\n");
	
	FILE * indexfile = NULL;
	FILE * eventfile =NULL;
	char filename[100] = "";
	char tmpname[100] = "";
	char epgdir[100] = "";
	char eventname[17] = "";
	
	t_original_network_id onid = 0;
	t_transport_stream_id tsid = 0;
	t_service_id sid = 0;
	t_channel_id chid = 0;

	strcpy(epgdir, epgxmlname);
	sprintf(tmpname, "%s/index.tmp", epgdir);

	if (!(indexfile = fopen(tmpname, "w"))) 
	{
		dprintf(DEBUG_NORMAL, "[sectionsd] CSectionsd::writeSI2XML: unable to open %s for writing\n", tmpname);
		goto _ret;
	}
	else 
	{

		dprintf(DEBUG_NORMAL, "[sectionsd] CSectionsd::writeSI2XML: Writing Information to file: %s\n", tmpname);

		write_index_xml_header(indexfile);

		readLockEvents();

		MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e =
			mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin();
		if (e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end()) 
		{
			onid = (*e)->original_network_id;
			tsid = (*e)->transport_stream_id;
			sid = (*e)->service_id;
			//
			chid = create_channel_id(sid, onid, tsid);
			
			//snprintf(eventname, 17, "%04x%04x%04x.xml", onid, tsid, sid);
			sprintf(eventname, "%s.xml", to_hexstring(chid).c_str());
			sprintf(filename, "%s/%s", epgdir, eventname);

			if (!(eventfile = fopen(filename, "w"))) 
			{
				write_indexxml_footer(indexfile);
				fclose(indexfile);
				goto _done;
			}
			
			fprintf(indexfile, "\t<eventfile name=\"%s\"/>\n",eventname);
			write_epg_xml_header(eventfile, onid, tsid, sid);

			while (e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end()) 
			{
				if ( (onid != (*e)->original_network_id) || (tsid != (*e)->transport_stream_id) || (sid != (*e)->service_id) ) 
				{
					onid = (*e)->original_network_id;
					tsid = (*e)->transport_stream_id;
					sid = (*e)->service_id;
					//
					chid = create_channel_id(sid, onid, tsid);
					
					write_epgxml_footer(eventfile);
					fclose(eventfile);
					
					//snprintf(eventname, 17, "%04x%04x%04x.xml", onid, tsid, sid);
					sprintf(eventname, "%s.xml", to_hexstring(chid).c_str());
					sprintf(filename, "%s/%s", epgdir, eventname);
					
					if (!(eventfile = fopen(filename, "w"))) 
					{
						goto _done;
					}
					fprintf(indexfile, "\t<eventfile name=\"%s\"/>\n", eventname);
					write_epg_xml_header(eventfile, onid, tsid, sid);
				}
				(*e)->saveXML(eventfile);
				e ++;
			}
			write_epgxml_footer(eventfile);
			fclose(eventfile);
		}
_done:
		unlockEvents();
		write_indexxml_footer(indexfile);
		fclose(indexfile);

		dprintf(DEBUG_NORMAL, "[sectionsd] CSectionsd::writeSI2XML: Writing Information finished\n");
	}

	strcpy(filename, epgxmlname);
	strncat(filename, "/index.xml", 10);

	CFileHelpers::getInstance()->copyFile(tmpname, filename);
	unlink(tmpname);
_ret:
	eventServer->sendEvent(NeutrinoMessages::EVT_SI_FINISHED, CEventServer::INITID_NEUTRINO);
	
	return ;
}

//---------------------------------------------------------------------
// Time-thread
// updates system time according TOT every 30 minutes
//---------------------------------------------------------------------
void *CSectionsd::timeThread(void *)
{
	UTC_t UTC;
	time_t tim;
	unsigned int seconds;
	bool first_time = true; /* we don't sleep the first time (we try to get a TOT header) */
	struct timespec restartWait;
	struct timeval now;
	bool time_ntp = false;
	bool success = true;

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	dprintf(DEBUG_NORMAL, "[sectionsd] [timeThread] pid %d start\n", getpid());

	while(!sectionsd_stop)
	{
		while (!scanning || !reader_ready) 
		{
			if(sectionsd_stop)
				break;
			sleep(1);
		}
		
		if (bTimeCorrect == true) 
		{		
			// sectionsd started with parameter "-tc"
			if (first_time == true) 
			{	
				// only do this once!
				time_t actTime;
				actTime = time(NULL);
				pthread_mutex_lock(&timeIsSetMutex);
				timeset = true;
				pthread_cond_broadcast(&timeIsSetCond);
				pthread_mutex_unlock(&timeIsSetMutex );
				
				eventServer->sendEvent(NeutrinoMessages::EVT_TIMESET, CEventServer::INITID_NEUTRINO, &actTime, sizeof(actTime) );
				
				dprintf(DEBUG_NORMAL, "[sectionsd] timeThread: Time is already set by system, no further timeThread work!\n");
				break;
			}
		}
		else if ( ntpenable && system( ntp_system_cmd.c_str() ) == 0)
		{
			time_t actTime;
			actTime = time(NULL);
			first_time = false;
			pthread_mutex_lock(&timeIsSetMutex);
			timeset = true;
			time_ntp = true;
			pthread_cond_broadcast(&timeIsSetCond);
			pthread_mutex_unlock(&timeIsSetMutex );
			
			eventServer->sendEvent(NeutrinoMessages::EVT_TIMESET, CEventServer::INITID_NEUTRINO, &actTime, sizeof(actTime) );
			
			dprintf(DEBUG_NORMAL, "[sectionsd] timeThread: Time is already set by system\n");
		} 
		else 
		{
			if (dvb_time_update) 
			{
				success = getUTC(&UTC, first_time); // for first time, get TDT, then TOT
				
				if (success)
				{
					tim = changeUTCtoCtime((const unsigned char *) &UTC);

					if (tim) 
					{
						//if ((!messaging_neutrino_sets_time) && (geteuid() == 0)) 
						{
							struct timeval tv;
							tv.tv_sec = tim;
							tv.tv_usec = 0;
							
							if (settimeofday(&tv, NULL) < 0) 
							{
								perror("[sectionsd] settimeofday");
								pthread_exit(NULL);
							}
						}
					}

					//
					time_t actTime;
					struct tm *tmTime;
					actTime = time(NULL);
					tmTime = localtime(&actTime);
					
					//dprintf(DEBUG_NORMAL, "[sectionsd] timeThread: current= %02d.%02d.%04d %02d:%02d:%02d, tim: %s\n", tmTime->tm_mday, tmTime->tm_mon+1, tmTime->tm_year+1900, tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec, ctime(&tim));
					
					pthread_mutex_lock(&timeIsSetMutex);
					timeset = true;
					time_ntp= false;
					pthread_cond_broadcast(&timeIsSetCond);
					pthread_mutex_unlock(&timeIsSetMutex );
					
					eventServer->sendEvent(NeutrinoMessages::EVT_TIMESET, CEventServer::INITID_NEUTRINO, &tim, sizeof(tim));
					
					dprintf(DEBUG_NORMAL, "[sectionsd] timeThread: Time is already set by DVB\n");
				}
			}
		}

		if (timeset && dvb_time_update) 
		{
			if (first_time)
				seconds = 5; /* retry a second time immediately */
			else
				seconds = ntprefresh * 60;

			if(time_ntp) 
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [%sThread] Time set via NTP, going to sleep for %d seconds.\n", "time", seconds);
			}
			else 
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [%sThread] Time %sset via DVB(%s), going to sleep for %d seconds.\n", "time", success?"":"not ", first_time?"TDT":"TOT", seconds);
			}
			first_time = false;
		}
		else 
		{
			if (!first_time) 
			{
				/* time was already set, no need to do it again soon when DVB time-blocked channel is tuned */
				seconds = ntprefresh * 60;
			}
			else if (!scanning) 
			{
				seconds = 60;
			}
			else 
			{
				seconds = 1;
			}
			
			if (!dvb_time_update && !first_time) 
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [%sThread] Time NOT set via DVB due to blocked channel, going to sleep for %d seconds.\n", "time", seconds);
			}
		}
		
		if(sectionsd_stop)
			break;

		gettimeofday(&now, NULL);
		TIMEVAL_TO_TIMESPEC(&now, &restartWait);
		restartWait.tv_sec += seconds;
		pthread_mutex_lock( &timeThreadSleepMutex );
		int ret = pthread_cond_timedwait( &timeThreadSleepCond, &timeThreadSleepMutex, &restartWait );
		if (ret == ETIMEDOUT)
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] TDT-Thread sleeping is over - no signal received\n");
		}
		else if (ret == EINTR)
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] TDT-Thread sleeping interrupted\n");
		}
		// else if (ret == 0) //everything is fine :) e.g. timeThreadSleepCond maybe signalled @zap time to get a valid time
		pthread_mutex_unlock( &timeThreadSleepMutex );
	}

	dprintf(DEBUG_DEBUG, "[sectionsd] timeThread ended\n");
	
	pthread_exit(NULL);
}

static cDemux * eitDmx;
int eit_set_update_filter(int *fd)
{
	dprintf(DEBUG_DEBUG, "[sectionsd] eit_set_update_filter\n");
	
	if (!FrontendCount)
		return 0;

	unsigned char cur_eit = dmxCN.get_eit_version();
	
	dprintf(DEBUG_DEBUG, "[sectionsd] eit_set_update_filter, servicekey = 0x" "%llx" ", current version 0x%x got events %d\n", messaging_current_servicekey, cur_eit, messaging_have_CN);

	if (cur_eit == 0xff) 
	{
		*fd = -1;
		return -1;
	}

	if(eitDmx == NULL) 
	{
		eitDmx = new cDemux();
#if defined (PLATFORM_COOLSTREAM)
		eitDmx->Open(DMX_PSI_CHANNEL);
#else
		eitDmx->Open( DMX_PSI_CHANNEL, MAX_SECTION_LENGTH, live_fe );
#endif		
	}

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];
	unsigned char mode[DMX_FILTER_SIZE];
	memset(&filter, 0, DMX_FILTER_SIZE);
	memset(&mask, 0, DMX_FILTER_SIZE);
	memset(&mode, 0, DMX_FILTER_SIZE);

	filter[0] = 0x4e;   /* table_id */
	filter[1] = (unsigned char)(messaging_current_servicekey >> 8);
	filter[2] = (unsigned char)messaging_current_servicekey;

	mask[0] = 0xFF;
	mask[1] = 0xFF;
	mask[2] = 0xFF;

	int timeout = 0;

	filter[3] = (cur_eit << 1) | 0x01;
	mask[3] = (0x1F << 1) | 0x01;
	mode[3] = 0x1F << 1;
	eitDmx->sectionFilter(0x12, filter, mask, 4, timeout, mode);
	
	*fd = 1;
	
	return 0;
}

int eit_stop_update_filter(int *fd)
{
	printf("[sectionsd] stop eit update filter\n");
	
	if(eitDmx && FrontendCount)
	{
		eitDmx->Stop();
		
		//delete eitDmx;
		//eitDmx = NULL;
	}

	*fd = -1;
	return 0;
}


//
// Freesat EIT-thread
// reads Freesat EPG-data
//
void *CSectionsd::fseitThread(void *)
{
	struct SI_section_header *header;
	
	/* we are holding the start_stop lock during this timeout, so don't make it too long... */
	
	unsigned timeoutInMSeconds = EIT_READ_TIMEOUT;
	bool sendToSleepNow = false;
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	dmxFSEIT.addfilter(0x60, 0xfe); //other TS, scheduled, freesat epg is only broadcast using table_ids 0x60 (scheduled) and 0x61 (scheduled later)

	int policy;
	struct sched_param parm;
	int rc = pthread_getschedparam(pthread_self(), &policy, &parm);
	dprintf(DEBUG_DEBUG, "[sectionsd] freesatEitThread getschedparam: %d pol %d, prio %d\n", rc, policy, parm.sched_priority);

	dprintf(DEBUG_NORMAL, "[sectionsd] [fseitThread] pid %d start\n", getpid());
	
	int timeoutsDMX = 0;
	uint8_t *static_buf = new uint8_t[MAX_SECTION_LENGTH];
	//rc;

	if (static_buf == NULL)
		throw std::bad_alloc();

	dmxFSEIT.start(); // -> unlock
	if (!scanning)
		dmxFSEIT.request_pause();

	waitForTimeset();
	dmxFSEIT.lastChanged = time_monotonic();

	while(!sectionsd_stop) 
	{
		while (!scanning) 
		{
			if(sectionsd_stop)
				break;
			sleep(1);
		}
		
		if(sectionsd_stop)
			break;
		time_t zeit = time_monotonic();

		rc = dmxFSEIT.getSection(static_buf, timeoutInMSeconds, timeoutsDMX);

		if (rc < 0)
			continue;

		if (timeoutsDMX < 0)
		{
			if ( dmxFSEIT.filter_index + 1 < (signed) dmxFSEIT.filters.size() )
			{
				if (timeoutsDMX == -1)
					printf("[freesatEitThread] skipping to next filter(%d) (> DMX_HAS_ALL_SECTIONS_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				if (timeoutsDMX == -2)
					printf("[freesatEitThread] skipping to next filter(%d) (> DMX_HAS_ALL_CURRENT_SECTIONS_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				timeoutsDMX = 0;
				dmxFSEIT.change(dmxFSEIT.filter_index + 1);
			}
			else 
			{
				sendToSleepNow = true;
				timeoutsDMX = 0;
			}
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS - 1)
		{
			readLockServices();
			readLockMessaging();

			MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();
			//dprintf("timeoutsDMX %x\n",currentServiceKey);

			if ( messaging_current_servicekey )
				si = mySIservicesOrderUniqueKey.find( messaging_current_servicekey );

			if (si != mySIservicesOrderUniqueKey.end())
			{
				// 1 and 3 == scheduled
				// 2 == current/next
				if ((dmxFSEIT.filter_index == 2 && !si->second->eitPresentFollowingFlag()) ||
						((dmxFSEIT.filter_index == 1 || dmxFSEIT.filter_index == 3) && !si->second->eitScheduleFlag()))
				{
					timeoutsDMX = 0;
					dprintf(DEBUG_DEBUG, "[sectionsd] [freesatEitThread] timeoutsDMX for 0x"
						"%llx"
						" reset to 0 (not broadcast)\n", messaging_current_servicekey );

					dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", dmxFSEIT.filter_index + 1, (signed) dmxFSEIT.filters.size() );
					dmxFSEIT.change( dmxFSEIT.filter_index + 1 );
				}
				else if (dmxFSEIT.filter_index >= 1)
				{
					if (dmxFSEIT.filter_index + 1 < (signed) dmxFSEIT.filters.size() )
					{
						dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", dmxFSEIT.filter_index + 1, (signed) dmxFSEIT.filters.size() );
						dmxFSEIT.change(dmxFSEIT.filter_index + 1);
						//dprintf(DEBUG_DEBUG, "[eitThread] timeoutsDMX for 0x%x reset to 0 (skipping to next filter)\n" );
						timeoutsDMX = 0;
					}
					else
					{
						sendToSleepNow = true;
						dprintf(DEBUG_DEBUG, "[sectionsd] sendToSleepNow = true\n");
					}
				}
			}
			unlockMessaging();
			unlockServices();
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS && scanning)
		{
			if ( dmxFSEIT.filter_index + 1 < (signed) dmxFSEIT.filters.size() )
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [freesatEitThread] skipping to next filter(%d) (> DMX_TIMEOUT_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				dmxFSEIT.change(dmxFSEIT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			timeoutsDMX = 0;
		}

		if (sendToSleepNow)
		{
			sendToSleepNow = false;

			if(sectionsd_stop)
				break;
			dmxFSEIT.real_pause();
			pthread_mutex_lock( &dmxFSEIT.start_stop_mutex );
			writeLockMessaging();
			messaging_zap_detected = false;
			unlockMessaging();

			struct timespec abs_wait;
			struct timeval now;
			gettimeofday(&now, NULL);
			TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
			abs_wait.tv_sec += TIME_EIT_SCHEDULED_PAUSE;
			dprintf(DEBUG_DEBUG, "[sectionsd] dmxFSEIT: going to sleep for %d seconds...\n", TIME_EIT_SCHEDULED_PAUSE);

			int rs = pthread_cond_timedwait( &dmxFSEIT.change_cond, &dmxFSEIT.start_stop_mutex, &abs_wait );

			pthread_mutex_unlock( &dmxFSEIT.start_stop_mutex );

			if (rs == ETIMEDOUT)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxFSEIT: waking up again - timed out\n");
				// must call dmxFSEIT.change after! unpause otherwise dev is not open,
				// dmxFSEIT.lastChanged will not be set, and filter is advanced the next iteration
				// maybe .change should imply .real_unpause()? -- seife
				dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", 2, (signed) dmxFSEIT.filters.size() );
				dmxFSEIT.change(1); // -> restart
			}
			else if (rs == 0)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxFSEIT: waking up again - requested from .change()\n");
			}
			else
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxFSEIT:  waking up again - unknown reason %d\n",rs);
			}
			// update zeit after sleep
			zeit = time_monotonic();
		}
		else if (zeit > dmxFSEIT.lastChanged + TIME_FSEIT_SKIPPING )
		{
			readLockMessaging();

			if ( dmxFSEIT.filter_index + 1 < (signed) dmxFSEIT.filters.size() )
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [freesatEitThread] skipping to next filter(%d) (> TIME_FSEIT_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				dmxFSEIT.change(dmxFSEIT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			unlockMessaging();
		}

		if (rc <= (int)sizeof(struct SI_section_header))
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] %s rc < sizeof(SI_Section_header) (%d < %d)\n", __FUNCTION__, rc, sizeof(struct SI_section_header));
			continue;
		}

		header = (SI_section_header*)static_buf;
		unsigned short section_length = header->section_length_hi << 8 | header->section_length_lo;



		if ((header->current_next_indicator) && (!dmxFSEIT.real_pauseCounter ))
		{
			// Wir wollen nur aktuelle sections

			// Houdini: added new constructor where the buffer is given as a parameter and must be allocated outside
			// -> no allocation and copy of data into a 2nd buffer
			//				SIsectionEIT eit(SIsection(section_length + 3, buf));
			SIsectionEIT eit(section_length + 3, static_buf);
			// Houdini: if section is not parsed (too short) -> no need to check events
			if (eit.is_parsed() && eit.header())
			{
				// == 0 -> kein event

				//dprintf(DEBUG_DEBUG, "[eitThread] adding %d events [table 0x%x] (begin)\n", eit.events().size(), header.table_id);
				zeit = time(NULL);
				// Nicht alle Events speichern
				for (SIevents::iterator e = eit.events().begin(); e != eit.events().end(); e++)
				{
					if (!(e->times.empty()))
					{
						if ( ( e->times.begin()->startzeit < zeit + secondsToCache ) &&
								( ( e->times.begin()->startzeit + (long)e->times.begin()->dauer ) > zeit - oldEventsAre ) )
						{
							//fprintf(stderr, "%02x ", header.table_id);
							CSectionsd::getInstance()->addEvent(*e, zeit);
						}
					}
					else
					{
						// pruefen ob nvod event
						readLockServices();
						MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(e->get_channel_id());

						if (si != mySIservicesNVODorderUniqueKey.end())
						{
							// Ist ein nvod-event
							writeLockEvents();

							for (SInvodReferences::iterator i = si->second->nvods.begin(); i != si->second->nvods.end(); i++)
								mySIeventUniqueKeysMetaOrderServiceUniqueKey.insert(std::make_pair(i->uniqueKey(), e->uniqueKey()));

							unlockEvents();
							CSectionsd::getInstance()->addNVODevent(*e);
						}
						unlockServices();
					}
				} // for
				//dprintf(DEBUG_DEBUG, "[eitThread] added %d events (end)\n",  eit.events().size());
			} // if
		} // if
		else
		{
			delete[] static_buf;

			//dprintf(DEBUG_DEBUG, "[eitThread] skipped sections for table 0x%x\n", header.table_id);
		}
	} // for
	dprintf(DEBUG_DEBUG, "[sectionsd] [freesatEitThread] end\n");

	pthread_exit(NULL);
}

void *CSectionsd::viasateitThread(void *)
{
	struct SI_section_header *header;
	/* 
	 * we are holding the start_stop lock during this timeout, so don't
	   make it too long... 
	*/
	unsigned timeoutInMSeconds = EIT_READ_TIMEOUT;
	bool sendToSleepNow = false;
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	dmxVIASAT.addfilter(0x60, 0xfe); //other TS, scheduled, freesat epg is only broadcast using table_ids 0x60 (scheduled) and 0x61 (scheduled later)

	int policy;
	struct sched_param parm;
	int rc = pthread_getschedparam(pthread_self(), &policy, &parm);
	
	dprintf(DEBUG_DEBUG, "[sectionsd] viasatEitThread getschedparam: %d pol %d, prio %d\n", rc, policy, parm.sched_priority);

	dprintf(DEBUG_NORMAL, "[sectionsd] [viasateitThread] pid %d start\n", getpid());
	
	int timeoutsDMX = 0;
	uint8_t *static_buf = new uint8_t[MAX_SECTION_LENGTH];

	if (static_buf == NULL)
		throw std::bad_alloc();

	dmxVIASAT.start(); // -> unlock
	
	if (!scanning)
		dmxVIASAT.request_pause();

	waitForTimeset();
	dmxVIASAT.lastChanged = time_monotonic();

	while(!sectionsd_stop) 
	{
		while (!scanning) 
		{
			if(sectionsd_stop)
				break;
			sleep(1);
		}
		
		if(sectionsd_stop)
			break;
		time_t zeit = time_monotonic();

		rc = dmxVIASAT.getSection(static_buf, timeoutInMSeconds, timeoutsDMX);

		if (rc < 0)
			continue;

		if (timeoutsDMX < 0)
		{
			if ( dmxVIASAT.filter_index + 1 < (signed) dmxVIASAT.filters.size() )
			{
				if (timeoutsDMX == -1)
					printf("[viasatEitThread] skipping to next filter(%d) (> DMX_HAS_ALL_SECTIONS_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				if (timeoutsDMX == -2)
					printf("[viasatEitThread] skipping to next filter(%d) (> DMX_HAS_ALL_CURRENT_SECTIONS_SKIPPING)\n", dmxFSEIT.filter_index+1 );
				timeoutsDMX = 0;
				dmxVIASAT.change(dmxFSEIT.filter_index + 1);
			}
			else 
			{
				sendToSleepNow = true;
				timeoutsDMX = 0;
			}
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS - 1)
		{
			readLockServices();
			readLockMessaging();

			MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();

			if ( messaging_current_servicekey )
				si = mySIservicesOrderUniqueKey.find( messaging_current_servicekey );

			if (si != mySIservicesOrderUniqueKey.end())
			{
				// 1 and 3 == scheduled
				// 2 == current/next
				if ((dmxVIASAT.filter_index == 2 && !si->second->eitPresentFollowingFlag()) || ((dmxVIASAT.filter_index == 1 || dmxVIASAT.filter_index == 3) && !si->second->eitScheduleFlag()))
				{
					timeoutsDMX = 0;
					dprintf(DEBUG_DEBUG, "[sectionsd] [freesatEitThread] timeoutsDMX for 0x" "%llx" " reset to 0 (not broadcast)\n", messaging_current_servicekey );

					dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", dmxVIASAT.filter_index + 1, (signed) dmxVIASAT.filters.size() );
					dmxVIASAT.change(dmxVIASAT.filter_index + 1);
				}
				else if (dmxVIASAT.filter_index >= 1)
				{
					if (dmxVIASAT.filter_index + 1 < (signed) dmxVIASAT.filters.size() )
					{
						dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", dmxVIASAT.filter_index + 1, (signed) dmxVIASAT.filters.size() );
						dmxVIASAT.change(dmxFSEIT.filter_index + 1);
						
						timeoutsDMX = 0;
					}
					else
					{
						sendToSleepNow = true;
						dprintf(DEBUG_DEBUG, "[sectionsd] sendToSleepNow = true\n");
					}
				}
			}
			unlockMessaging();
			unlockServices();
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS && scanning)
		{
			if ( dmxVIASAT.filter_index + 1 < (signed) dmxVIASAT.filters.size() )
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [viasatEitThread] skipping to next filter(%d) (> DMX_TIMEOUT_SKIPPING)\n", dmxVIASAT.filter_index+1 );
				dmxVIASAT.change(dmxVIASAT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			timeoutsDMX = 0;
		}

		if (sendToSleepNow)
		{
			sendToSleepNow = false;

			if(sectionsd_stop)
				break;
			
			dmxVIASAT.real_pause();
			pthread_mutex_lock( &dmxVIASAT.start_stop_mutex );
			writeLockMessaging();
			messaging_zap_detected = false;
			unlockMessaging();

			struct timespec abs_wait;
			struct timeval now;
			gettimeofday(&now, NULL);
			TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
			abs_wait.tv_sec += TIME_EIT_SCHEDULED_PAUSE;
			
			dprintf(DEBUG_DEBUG, "[sectionsd] dmxVIASAT: going to sleep for %d seconds...\n", TIME_EIT_SCHEDULED_PAUSE);

			int rs = pthread_cond_timedwait( &dmxVIASAT.change_cond, &dmxVIASAT.start_stop_mutex, &abs_wait );

			pthread_mutex_unlock( &dmxVIASAT.start_stop_mutex );

			if (rs == ETIMEDOUT)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxVIASAT: waking up again - timed out\n");
				// must call dmxFSEIT.change after! unpause otherwise dev is not open,
				// dmxFSEIT.lastChanged will not be set, and filter is advanced the next iteration
				// maybe .change should imply .real_unpause()? -- seife
				dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", 2, (signed) dmxVIASAT.filters.size() );
				dmxVIASAT.change(1); // -> restart
			}
			else if (rs == 0)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxVIASAT: waking up again - requested from .change()\n");
			}
			else
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxVIASAT:  waking up again - unknown reason %d\n",rs);
			}
			// update zeit after sleep
			zeit = time_monotonic();
		}
		else if (zeit > dmxVIASAT.lastChanged + TIME_FSEIT_SKIPPING )
		{
			readLockMessaging();

			if ( dmxVIASAT.filter_index + 1 < (signed) dmxVIASAT.filters.size() )
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] [viasatEitThread] skipping to next filter(%d) (> TIME_FSEIT_SKIPPING)\n", dmxVIASAT.filter_index+1 );
				dmxVIASAT.change(dmxVIASAT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			unlockMessaging();
		}

		if (rc <= (int)sizeof(struct SI_section_header))
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] %s rc < sizeof(SI_Section_header) (%d < %d)\n", __FUNCTION__, rc, sizeof(struct SI_section_header));
			continue;
		}

		header = (SI_section_header*)static_buf;
		unsigned short section_length = header->section_length_hi << 8 | header->section_length_lo;



		if ((header->current_next_indicator) && (!dmxVIASAT.real_pauseCounter ))
		{
			// Wir wollen nur aktuelle sections

			// Houdini: added new constructor where the buffer is given as a parameter and must be allocated outside
			// -> no allocation and copy of data into a 2nd buffer
			//				SIsectionEIT eit(SIsection(section_length + 3, buf));
			SIsectionEIT eit(section_length + 3, static_buf);
			// Houdini: if section is not parsed (too short) -> no need to check events
			if (eit.is_parsed() && eit.header())
			{
				// == 0 -> kein event

				//dprintf(DEBUG_DEBUG, "[eitThread] adding %d events [table 0x%x] (begin)\n", eit.events().size(), header.table_id);
				zeit = time(NULL);
				// Nicht alle Events speichern
				for (SIevents::iterator e = eit.events().begin(); e != eit.events().end(); e++)
				{
					if (!(e->times.empty()))
					{
						if ( ( e->times.begin()->startzeit < zeit + secondsToCache ) && ( ( e->times.begin()->startzeit + (long)e->times.begin()->dauer ) > zeit - oldEventsAre ) )
						{
							//fprintf(stderr, "%02x ", header.table_id);
							CSectionsd::getInstance()->addEvent(*e, zeit);
						}
					}
					else
					{
						// pruefen ob nvod event
						readLockServices();
						MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(e->get_channel_id());

						if (si != mySIservicesNVODorderUniqueKey.end())
						{
							// Ist ein nvod-event
							writeLockEvents();

							for (SInvodReferences::iterator i = si->second->nvods.begin(); i != si->second->nvods.end(); i++)
								mySIeventUniqueKeysMetaOrderServiceUniqueKey.insert(std::make_pair(i->uniqueKey(), e->uniqueKey()));

							unlockEvents();
							CSectionsd::getInstance()->addNVODevent(*e);
						}
						unlockServices();
					}
				} // for
				//dprintf(DEBUG_DEBUG, "[eitThread] added %d events (end)\n",  eit.events().size());
			} // if
		} // if
		else
		{
			delete[] static_buf;

			//dprintf(DEBUG_DEBUG, "[eitThread] skipped sections for table 0x%x\n", header.table_id);
		}
	} // for
	dprintf(DEBUG_DEBUG, "[sectionsd] [viasatEitThread] end\n");

	pthread_exit(NULL);
}

//
// EIT-thread
// reads EPG-datas
//
void *CSectionsd::eitThread(void *)
{
	struct SI_section_header *header;
	/* we are holding the start_stop lock during this timeout, so don't
	   make it too long... */
	unsigned timeoutInMSeconds = EIT_READ_TIMEOUT;
	bool sendToSleepNow = false;

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	/* These filters are a bit tricky (index numbers):
	   - 0   Dummy filter, to make this thread sleep for some seconds
	   - 1   then get other TS's current/next (this TS's cur/next are
	   handled in dmxCN)
	   - 2/3 then get scheduled events on this TS
	   - 4   then get the other TS's scheduled events,
	   - 4ab (in two steps to reduce the POLLERRs on the DMX device)
	   */
	// -- set EIT filter  0x4e-0x6F
	dmxEIT.addfilter(0x00, 0x00); //0 dummy filter
	dmxEIT.addfilter(0x50, 0xf0); //1  current TS, scheduled
	dmxEIT.addfilter(0x4f, 0xff); //2  other TS, current/next
#if 1
	dmxEIT.addfilter(0x60, 0xf1); //3a other TS, scheduled, even
	dmxEIT.addfilter(0x61, 0xf1); //3b other TS, scheduled, odd
#else
	dmxEIT.addfilter(0x60, 0xf0); //3  other TS, scheduled
#endif

	// debug
	int policy;
	struct sched_param parm;
	int rc = pthread_getschedparam(pthread_self(), &policy, &parm);
	dprintf(DEBUG_DEBUG, "[sectionsd] eitThread getschedparam: %d pol %d, prio %d\n", rc, policy, parm.sched_priority);
	
	dprintf(DEBUG_NORMAL, "[sectionsd] [eitThread] pid %d start\n", getpid());
	
	int timeoutsDMX = 0;
	uint8_t *static_buf = new uint8_t[MAX_SECTION_LENGTH];

	if (static_buf == NULL)
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] %s: could not allocate static_buf\n", __FUNCTION__);
		pthread_exit(NULL);
		//throw std::bad_alloc();
	}

	dmxEIT.start(); // -> unlock
	
	if (!scanning)
		dmxEIT.request_pause();

	waitForTimeset();
	dmxEIT.lastChanged = time_monotonic();

	while (!sectionsd_stop) 
	{
		while (!scanning) 
		{
			if(sectionsd_stop)
				break;
			sleep(1);
		}
		
		if(sectionsd_stop)
			break;
			
		time_t zeit = time_monotonic();

		rc = dmxEIT.getSection(static_buf, timeoutInMSeconds, timeoutsDMX);
		if(sectionsd_stop)
			break;

		if (timeoutsDMX < 0 && !channel_is_blacklisted)
		{
			if (timeoutsDMX == -1)
				printf("[eitThread] skipping to next filter(%d) (> DMX_HAS_ALL_SECTIONS_SKIPPING)\n", dmxEIT.filter_index+1 );
			else if (timeoutsDMX == -2)
				printf("[eitThread] skipping to next filter(%d) (> DMX_HAS_ALL_CURRENT_SECTIONS_SKIPPING)\n", dmxEIT.filter_index+1 );
			else
				printf("[eitThread] skipping to next filter(%d) (timeouts %d)\n", dmxEIT.filter_index+1, timeoutsDMX);
			if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
			{
				timeoutsDMX = 0;
				dmxEIT.change(dmxEIT.filter_index + 1);
			}
			else 
			{
				sendToSleepNow = true;
				timeoutsDMX = 0;
			}
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS - 1 && !channel_is_blacklisted)
		{
			readLockServices();
			MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();
			//dprintf(DEBUG_DEBUG, "timeoutsDMX %x\n",currentServiceKey);

			if ( messaging_current_servicekey )
				si = mySIservicesOrderUniqueKey.find( messaging_current_servicekey );

			if (si != mySIservicesOrderUniqueKey.end())
			{
				/* I'm not 100% sure what this is good for... */
				// 1 == scheduled
				// 2 == current/next
				if ((dmxEIT.filter_index == 2 && !si->second->eitPresentFollowingFlag()) ||
						(dmxEIT.filter_index == 1 && !si->second->eitScheduleFlag()))
				{
					timeoutsDMX = 0;
					dprintf(DEBUG_DEBUG, "[sectionsd] [eitThread] timeoutsDMX for 0x"
						"%llx"
						" reset to 0 (not broadcast)\n", messaging_current_servicekey );

					dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", dmxEIT.filter_index + 1, (signed) dmxEIT.filters.size() );
					dmxEIT.change( dmxEIT.filter_index + 1 );
				}
				else if (dmxEIT.filter_index >= 1)
				{
					if (dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
					{
						dprintf(DEBUG_DEBUG, "[sectionsd] [eitThread] New Filterindex: %d (ges. %d)\n", dmxEIT.filter_index + 1, (signed) dmxEIT.filters.size() );
						dmxEIT.change(dmxEIT.filter_index + 1);
						//dprintf(DEBUG_DEBUG, "[eitThread] timeoutsDMX for 0x%x reset to 0 (skipping to next filter)\n" );
						timeoutsDMX = 0;
					}
					else
					{
						sendToSleepNow = true;
						dprintf(DEBUG_DEBUG, "[sectionsd] sendToSleepNow = true\n");
					}
				}
			}
			unlockServices();
		}

		if (timeoutsDMX >= CHECK_RESTART_DMX_AFTER_TIMEOUTS && scanning && !channel_is_blacklisted)
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] [eitThread] skipping to next filter(%d) (> DMX_TIMEOUT_SKIPPING %d)\n", dmxEIT.filter_index+1, timeoutsDMX);
			if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
			{
				dmxEIT.change(dmxEIT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			timeoutsDMX = 0;
		}

		if (sendToSleepNow || channel_is_blacklisted)
		{
			sendToSleepNow = false;

			dmxEIT.real_pause();
			writeLockMessaging();
			messaging_zap_detected = false;
			unlockMessaging();

			int rs = 0;
			do {
				struct timespec abs_wait;
				struct timeval now;
				gettimeofday(&now, NULL);
				TIMEVAL_TO_TIMESPEC(&now, &abs_wait);
				abs_wait.tv_sec += TIME_EIT_SCHEDULED_PAUSE;
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxEIT: going to sleep for %d seconds...\n", TIME_EIT_SCHEDULED_PAUSE);
				if(sectionsd_stop)
					break;

				pthread_mutex_lock( &dmxEIT.start_stop_mutex );
				rs = pthread_cond_timedwait( &dmxEIT.change_cond, &dmxEIT.start_stop_mutex, &abs_wait );
				pthread_mutex_unlock( &dmxEIT.start_stop_mutex );
			} while (channel_is_blacklisted);

			if (rs == ETIMEDOUT)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxEIT: waking up again - timed out\n");
				dprintf(DEBUG_DEBUG, "[sectionsd] New Filterindex: %d (ges. %d)\n", 2, (signed) dmxEIT.filters.size() );
				dmxEIT.change(1); // -> restart
			}
			else if (rs == 0)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxEIT: waking up again - requested from .change()\n");
			}
			else
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxEIT:  waking up again - unknown reason %d\n",rs);
				dmxEIT.real_unpause();
			}
			// update zeit after sleep
			zeit = time_monotonic();
		}
		else if (zeit > dmxEIT.lastChanged + TIME_EIT_SKIPPING )
		{
			readLockMessaging();

			dprintf(DEBUG_DEBUG, "[sectionsd] [eitThread] skipping to next filter(%d) (> TIME_EIT_SKIPPING)\n", dmxEIT.filter_index+1 );
			if ( dmxEIT.filter_index + 1 < (signed) dmxEIT.filters.size() )
			{
				dmxEIT.change(dmxEIT.filter_index + 1);
			}
			else
				sendToSleepNow = true;

			unlockMessaging();
		}

		if (rc < 0)
			continue;

		if (rc < (int)sizeof(struct SI_section_header))
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] %s rc < sizeof(SI_Section_header) (%d < %d)\n", __FUNCTION__, rc, sizeof(struct SI_section_header));
			continue;
		}

		header = (SI_section_header*)static_buf;
		unsigned short section_length = header->section_length_hi << 8 | header->section_length_lo;

		if(sectionsd_stop)
			break;
		if (header->current_next_indicator)
		{
			// Wir wollen nur aktuelle sections

			// Houdini: added new constructor where the buffer is given as a parameter and must be allocated outside
			// -> no allocation and copy of data into a 2nd buffer
			//				SIsectionEIT eit(SIsection(section_length + 3, buf));
			SIsectionEIT eit(section_length + 3, static_buf);
			// Houdini: if section is not parsed (too short) -> no need to check events
			if (eit.is_parsed() && eit.header())
			{
				// == 0 -> kein event

				//dprintf(DEBUG_DEBUG, "[eitThread] adding %d events [table 0x%x] (begin)\n", eit.events().size(), header.table_id);
				zeit = time(NULL);
				// Nicht alle Events speichern
				for (SIevents::iterator e = eit.events().begin(); e != eit.events().end(); e++)
				{
					if (!(e->times.empty()))
					{
						if ( ( e->times.begin()->startzeit < zeit + secondsToCache ) &&
								( ( e->times.begin()->startzeit + (long)e->times.begin()->dauer ) > zeit - oldEventsAre ) )
						{
							//fprintf(stderr, "%02x ", header.table_id);
							if(sectionsd_stop)
								break;
							//printf("Adding event 0x%llx table %x version %x running %d\n", e->uniqueKey(), header->table_id, header->version_number, e->runningStatus());
							CSectionsd::getInstance()->addEvent(*e, zeit);
						}
					}
					else
					{
						// pruefen ob nvod event
						readLockServices();
						MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(e->get_channel_id());

						if (si != mySIservicesNVODorderUniqueKey.end())
						{
							// Ist ein nvod-event
							writeLockEvents();

							for (SInvodReferences::iterator i = si->second->nvods.begin(); i != si->second->nvods.end(); i++)
								mySIeventUniqueKeysMetaOrderServiceUniqueKey.insert(std::make_pair(i->uniqueKey(), e->uniqueKey()));

							unlockEvents();
							CSectionsd::getInstance()->addNVODevent(*e);
						}
						unlockServices();
					}
				} // for
				//dprintf(DEBUG_DEBUG, "[eitThread] added %d events (end)\n",  eit.events().size());
			} // if
		} // if
		else
		{
			printf("[sectionsd] [eitThread] skipped sections for table 0x%x\n", header->table_id);
		}
	} // for
	delete[] static_buf;

	dprintf(DEBUG_DEBUG, "[sectionsd] [sectionsd] eitThread ended\n");

	pthread_exit(NULL);
}

//
// CN-thread: eit thread, but only current/next
//
void *CSectionsd::cnThread(void *)
{
	struct SI_section_header *header;
	/* we are holding the start_stop lock during this timeout, so don't
	   make it too long... */
	unsigned timeoutInMSeconds = EIT_READ_TIMEOUT;
	bool sendToSleepNow = false;
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	// set EIT filter  0x4e
	dmxCN.addfilter(0x4e, 0xff); //0  current TS, current/next

	dprintf(DEBUG_NORMAL, "[sectionsd] [cnThread] pid %d start\n", getpid());
	
	t_channel_id time_trigger_last = 0;
	int timeoutsDMX = 0;
	uint8_t *static_buf = new uint8_t[MAX_SECTION_LENGTH];
	int rc;

	if (static_buf == NULL)
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] %s: could not allocate static_buf\n", __FUNCTION__);
		pthread_exit(NULL);
		//throw std::bad_alloc();
	}

	dmxCN.start(); // -> unlock
	
	if (!scanning)
		dmxCN.request_pause();

	writeLockMessaging();
	messaging_eit_is_busy = true;
	messaging_need_eit_version = false;
	unlockMessaging();

	waitForTimeset();

	time_t eit_waiting_since = time_monotonic();
	dmxCN.lastChanged = eit_waiting_since;

	while(!sectionsd_stop)
	{
		while (!scanning) 
		{
			sleep(1);
			if(sectionsd_stop)
				break;
		}
		
		if(sectionsd_stop)
			break;

		rc = dmxCN.getSection(static_buf, timeoutInMSeconds, timeoutsDMX);
		time_t zeit = time_monotonic();
		
		if (update_eit) 
		{
			if (dmxCN.get_eit_version() != 0xff) 
			{
				writeLockMessaging();
				messaging_need_eit_version = false;
				unlockMessaging();
			} 
			else 
			{
				readLockMessaging();
				
				if (!messaging_need_eit_version) 
				{
					unlockMessaging();
					dprintf(DEBUG_DEBUG, "[sectionsd] waiting for eit_version...\n");
					zeit = time_monotonic();  /* reset so that we don't get negative */
					eit_waiting_since = zeit; /* and still compensate for getSection */
					dmxCN.lastChanged = zeit; /* this is ugly - needs somehting better */
					sendToSleepNow = false;   /* reset after channel change */
					writeLockMessaging();
					messaging_need_eit_version = true;
				}
				
				unlockMessaging();
				
				if (zeit - eit_waiting_since > TIME_EIT_VERSION_WAIT) 
				{
					dprintf(DEBUG_DEBUG, "[sectionsd] waiting for more than %d seconds - bail out...\n", TIME_EIT_VERSION_WAIT);
					/* send event anyway, so that we know there is no EPG */
					eventServer->sendEvent(NeutrinoMessages::EVT_CURRENTNEXT_EPG, CEventServer::INITID_NEUTRINO, &messaging_current_servicekey, sizeof(messaging_current_servicekey));
					
					writeLockMessaging();
					messaging_need_eit_version = false;
					unlockMessaging();
					sendToSleepNow = true;
				}
			}

		} // if (update_eit)

		readLockMessaging();
		
		if (messaging_got_CN != messaging_have_CN) // timeoutsDMX < -1)
		{
			unlockMessaging();
			writeLockMessaging();
			messaging_have_CN = messaging_got_CN;
			unlockMessaging();
			dprintf(DEBUG_DEBUG, "[sectionsd] [cnThread] got current_next (0x%x) - sending event!\n", messaging_have_CN);
			eventServer->sendEvent(NeutrinoMessages::EVT_CURRENTNEXT_EPG, CEventServer::INITID_NEUTRINO, &messaging_current_servicekey, sizeof(messaging_current_servicekey));
			
			/* we received an event => reset timeout timer... */
			eit_waiting_since = zeit;
			dmxCN.lastChanged = zeit; /* this is ugly - needs somehting better */
			readLockMessaging();
		}
		
		if (messaging_have_CN == 0x03) // current + next
		{
			unlockMessaging();
			sendToSleepNow = true;
			//timeoutsDMX = 0;
		}
		else 
		{
			unlockMessaging();
		}

		if ((sendToSleepNow && !messaging_need_eit_version) || channel_is_blacklisted)
		{
			sendToSleepNow = false;

			dmxCN.real_pause();
			
			dprintf(DEBUG_DEBUG, "[sectionsd] dmxCN: going to sleep...\n");

			writeLockMessaging();
			messaging_eit_is_busy = false;
			unlockMessaging();

			/* 
			re-fetch time if transponder changed
			Why I'm doing this here and not from commandserviceChanged?
			commandserviceChanged is called on zap *start*, not after zap finished
			this would lead to often actually fetching the time on the transponder
			you are switching away from, not the one you are switching onto.
			Doing it here at least gives us a good chance to have actually tuned
			to the channel we want to get the time from...
			*/
			if (time_trigger_last != (messaging_current_servicekey & 0xFFFFFFFF0000ULL))
			{
				time_trigger_last = messaging_current_servicekey & 0xFFFFFFFF0000ULL;
				pthread_mutex_lock(&timeThreadSleepMutex);
				pthread_cond_broadcast(&timeThreadSleepCond);
				pthread_mutex_unlock(&timeThreadSleepMutex);
			}

			int rs;
			do {
				pthread_mutex_lock( &dmxCN.start_stop_mutex );
				if (!channel_is_blacklisted)
					eit_set_update_filter(&eit_update_fd);
				rs = pthread_cond_wait(&dmxCN.change_cond, &dmxCN.start_stop_mutex);
				eit_stop_update_filter(&eit_update_fd);
				pthread_mutex_unlock(&dmxCN.start_stop_mutex);
			} while (channel_is_blacklisted);

			writeLockMessaging();
			messaging_need_eit_version = false;
			messaging_eit_is_busy = true;
			unlockMessaging();

			if (rs == 0)
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxCN: waking up again - requested from .change()\n");
				
				// fix EPG problems on IPBox
				// http://tuxbox-forum.dreambox-fan.de/forum/viewtopic.php?p=367937#p367937
#if 1 //FIXME
				dmxCN.change(0);
#endif
			}
			else
			{
				dprintf(DEBUG_DEBUG, "[sectionsd] dmxCN:  waking up again - unknown reason %d\n",rs);
				dmxCN.real_unpause();
			}
			zeit = time_monotonic();
		}
		else if (zeit > dmxCN.lastChanged + TIME_EIT_VERSION_WAIT && !messaging_need_eit_version)
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] zeit > dmxCN.lastChanged + TIME_EIT_VERSION_WAIT\n");
			sendToSleepNow = true;
			/* we can get here if we got the EIT version but no events */
			/* send a "no epg" event anyway before going to sleep */
			if (messaging_have_CN == 0x00)
			{
				eventServer->sendEvent(NeutrinoMessages::EVT_CURRENTNEXT_EPG, CEventServer::INITID_NEUTRINO, &messaging_current_servicekey, sizeof(messaging_current_servicekey));
			}
			continue;
		}

		if (rc < 0)
			continue;

		if (rc < (int)sizeof(struct SI_section_header))
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] %s: rc < sizeof(SI_Section_header) (%d < %d)\n", __FUNCTION__, rc, sizeof(struct SI_section_header));
			continue;
		}

		header = (SI_section_header *)static_buf;
		unsigned short section_length = (header->section_length_hi << 8) | header->section_length_lo;

		if (!header->current_next_indicator)
		{
			// Wir wollen nur aktuelle sections
			//dprintf(DEBUG_DEBUG, "[cnThread] skipped sections for table 0x%x\n", header->table_id);
			continue;
		}

		SIsectionEIT eit(section_length + 3, static_buf);
		// Houdini: if section is not parsed (too short) -> no need to check events
		if (!eit.is_parsed() || !eit.header())
			continue;

		// == 0 -> kein event
		//dprintf(DEBUG_DEBUG, "[cnThread] adding %d events [table 0x%x] (begin)\n", eit.events().size(), header->table_id);
		zeit = time(NULL);
		// Nicht alle Events speichern
		for (SIevents::iterator e = eit.events().begin(); e != eit.events().end(); e++)
		{
			if (!(e->times.empty()))
			{
				CSectionsd::getInstance()->addEvent(*e, zeit, true); /* cn = true => fill in current / next event */
			}
		} // for
		//dprintf(DEBUG_DEBUG, "[cnThread] added %d events (end)\n",  eit.events().size());
	} // for
	
	delete[] static_buf;

	dprintf(DEBUG_DEBUG, "[sectionsd] cnThread ended\n");

	pthread_exit(NULL);
}

/* helper function for the housekeeping-thread */
void CSectionsd::print_meminfo(void)
{
	struct mallinfo meminfo = mallinfo();
	
	dprintf(DEBUG_DEBUG, "[sectionsd] total size of memory occupied by chunks handed out by malloc: %d\n"
		"total bytes memory allocated with `sbrk' by malloc, in bytes: %d (%dkB)\n",
		meminfo.uordblks, meminfo.arena, meminfo.arena / 1024);
}

//
// housekeeping-thread
// does cleaning on fetched datas
//
void *CSectionsd::houseKeepingThread(void *)
{
	int count = 0;

	dprintf(DEBUG_NORMAL, "[sectionsd] housekeeping-thread started.\n");
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	while (!sectionsd_stop)
	{
		//
		if (FrontendCount)
		{
			if (eit_update_fd != -1) 
			{
				unsigned char buf[MAX_SECTION_LENGTH];
				int ret = eitDmx->Read(buf, MAX_SECTION_LENGTH, 10);

				// dirty hack eitDmx read sucked always //FIXME???
				if (ret > 0) 
				{
					writeLockMessaging();
					//messaging_skipped_sections_ID[0].clear();
					//messaging_sections_max_ID[0] = -1;
					//messaging_sections_got_all[0] = false;
					messaging_have_CN = 0x00;
					messaging_got_CN = 0x00;
					messaging_last_requested = time_monotonic();
					unlockMessaging();
					sched_yield();
					dmxCN.change(0);
					sched_yield();
				}
			}
		}
		
		//
		if (count == META_HOUSEKEEPING) 
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] meta housekeeping - deleting all transponders, services, bouquets.\n");
			CSectionsd::getInstance()->deleteSIexceptEPG();
			count = 0;
		}

		int rc = HOUSEKEEPING_SLEEP;

		while (rc)
			rc = sleep(rc);

		while (!scanning) 
		{
			sleep(1);	// wait for streaming to end...
			if(sectionsd_stop)
				break;
		}

		dprintf(DEBUG_DEBUG, "[sectionsd] housekeeping.\n");

		// TODO: maybe we need to stop scanning here?...

		readLockEvents();

		unsigned anzEventsAlt = mySIeventsOrderUniqueKey.size();
		dprintf(DEBUG_DEBUG, "[sectionsd] before removeoldevents\n");
		unlockEvents();
		CSectionsd::getInstance()->removeOldEvents(oldEventsAre); // alte Events
		dprintf(DEBUG_DEBUG, "[sectionsd] after removeoldevents\n");
		readLockEvents();
		dprintf(DEBUG_DEBUG, "[sectionsd] Removed %d old events.\n", anzEventsAlt - mySIeventsOrderUniqueKey.size());
		
		if (mySIeventsOrderUniqueKey.size() != anzEventsAlt)
		{
			CSectionsd::getInstance()->print_meminfo();
			dprintf(DEBUG_DEBUG, "[sectionsd] Removed %d old events.\n", anzEventsAlt - mySIeventsOrderUniqueKey.size());
		}
		
		anzEventsAlt = mySIeventsOrderUniqueKey.size();
		unlockEvents();
		//usleep(100);
		//lockEvents();
//#ifdef REMOVE_DUPS
		CSectionsd::getInstance()->removeDupEvents();
		readLockEvents();
		dprintf(DEBUG_DEBUG, "[sectionsd] Removed %d dup events.\n", anzEventsAlt - mySIeventsOrderUniqueKey.size());
		anzEventsAlt = mySIeventsOrderUniqueKey.size();
		unlockEvents();
//#endif
		dprintf(DEBUG_DEBUG, "[sectionsd] before removewasteepg\n");

		readLockEvents();
		
		if (mySIeventsOrderUniqueKey.size() != anzEventsAlt)
		{
			CSectionsd::getInstance()->print_meminfo();
			dprintf(DEBUG_DEBUG, "[sectionsd] Removed %d waste events.\n", anzEventsAlt - mySIeventsOrderUniqueKey.size());
		}

		dprintf(DEBUG_DEBUG, "[sectionsd] Number of sptr events (event-ID): %u\n", mySIeventsOrderUniqueKey.size());
		dprintf(DEBUG_DEBUG, "[sectionsd] Number of sptr events (service-id, start time, event-id): %u\n", mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.size());
		dprintf(DEBUG_DEBUG, "[sectionsd] Number of sptr events (end time, service-id, event-id): %u\n", mySIeventsOrderFirstEndTimeServiceIDEventUniqueKey.size());
		dprintf(DEBUG_DEBUG, "[sectionsd] Number of sptr nvod events (event-ID): %u\n", mySIeventsNVODorderUniqueKey.size());
		dprintf(DEBUG_DEBUG, "[sectionsd] Number of cached meta-services: %u\n", mySIeventUniqueKeysMetaOrderServiceUniqueKey.size());

		unlockEvents();

		CSectionsd::getInstance()->print_meminfo();

		count++;

	} // for endlos
	dprintf(DEBUG_DEBUG, "[sectionsd] housekeeping-thread ended.\n");

	pthread_exit(NULL);
}

void CSectionsd::readEPGFilter(void)
{
	dprintf(DEBUG_NORMAL, "[sectionsd] CSectionsd::readEPGFilter:\n");
	
	_xmlDocPtr filter_parser = parseXmlFile(epg_filter_dir.c_str());

	t_original_network_id onid = 0;
	t_transport_stream_id tsid = 0;
	t_service_id sid = 0;

	if (filter_parser != NULL)
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] Reading EPGFilters\n");

		_xmlNodePtr filter = xmlDocGetRootElement(filter_parser);
		if (xmlGetNumericAttribute(filter, "is_whitelist", 10) == 1)
			epg_filter_is_whitelist = true;
		if (xmlGetNumericAttribute(filter, "except_current_next", 10) == 1)
			epg_filter_except_current_next = true;
		filter = filter->xmlChildrenNode;

		while (filter) 
		{
			onid = xmlGetNumericAttribute(filter, "onid", 16);
			tsid = xmlGetNumericAttribute(filter, "tsid", 16);
			sid  = xmlGetNumericAttribute(filter, "serviceID", 16);
			if (xmlGetNumericAttribute(filter, "blacklist", 10) == 1)
				addBlacklist(onid, tsid, sid);
			else
				addEPGFilter(onid, tsid, sid);

			filter = filter->xmlNextNode;
		}
	}
	xmlFreeDoc(filter_parser);
}

void CSectionsd::readDVBTimeFilter(void)
{
	dprintf(DEBUG_NORMAL, "[sectionsd] CSectionsd::readDVBTimeFilter:\n");
	
	_xmlDocPtr filter_parser = parseXmlFile(dvbtime_filter_dir.c_str());

	t_original_network_id onid = 0;
	t_transport_stream_id tsid = 0;
	t_service_id sid = 0;

	if (filter_parser != NULL)
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] Reading DVBTimeFilters\n");

		_xmlNodePtr filter = xmlDocGetRootElement(filter_parser);
		filter = filter->xmlChildrenNode;

		while (filter) 
		{
			onid = xmlGetNumericAttribute(filter, "onid", 16);
			tsid = xmlGetNumericAttribute(filter, "tsid", 16);
			sid  = xmlGetNumericAttribute(filter, "serviceID", 16);
			addNoDVBTimelist(onid, tsid, sid);

			filter = filter->xmlNextNode;
		}
		xmlFreeDoc(filter_parser);
	}
	else
	{
		dvb_time_update = true;
	}
}

void CSectionsd::getEventsServiceKey(t_channel_id serviceUniqueKey, CChannelEventList &eList, char search, std::string search_text)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::getEventsServiceKey:sendAllEvents for:%llx\n", serviceUniqueKey);

	if (serviceUniqueKey != 0) 
	{ 
		// service Found
		readLockEvents();
		int serviceIDfound = 0;

		if (search_text.length()) 
			std::transform(search_text.begin(), search_text.end(), search_text.begin(), op_increase);

		for (MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin(); e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end(); ++e)
		{
			if ((*e)->get_channel_id() == (serviceUniqueKey & 0xFFFFFFFFFFFFULL)) 
			{ 
				serviceIDfound = 1;

				bool copy = true;
				if(search == 0); // nothing to do here
				else if(search == 1) 
				{
					std::string eName = (*e)->getName();
					std::transform(eName.begin(), eName.end(), eName.begin(), op_increase);
					if(eName.find(search_text) == std::string::npos)
						copy = false;
				}
				else if(search == 2) 
				{
					std::string eText = (*e)->getText();
					std::transform(eText.begin(), eText.end(), eText.begin(), op_increase);
					if(eText.find(search_text) == std::string::npos)
						copy = false;
				}
				else if(search == 3) 
				{
					std::string eExtendedText = (*e)->getExtendedText();
					std::transform(eExtendedText.begin(), eExtendedText.end(), eExtendedText.begin(), op_increase);
					if(eExtendedText.find(search_text) == std::string::npos)
						copy = false;
				}

				if(copy) 
				{
					for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); ++t)
					{
						CChannelEvent aEvent;
						aEvent.eventID = (*e)->uniqueKey();
						aEvent.startTime = t->startzeit;
						aEvent.duration = t->dauer;
						aEvent.description = (*e)->getName();
						if (((*e)->getText()).empty())
							aEvent.text = (*e)->getExtendedText().substr(0, 120);
						else
							aEvent.text = (*e)->getText();
						aEvent.channelID = serviceUniqueKey;
						eList.push_back(aEvent);
					}
				} // if = serviceID
			}
			else if ( serviceIDfound )
				break; // sind nach serviceID und startzeit sortiert -> nicht weiter suchen
		}

		unlockEvents();
	}
}

void CSectionsd::getCurrentNextServiceKey(t_channel_id uniqueServiceKey, CurrentNextInfo& current_next )
{
	dprintf(DEBUG_NORMAL, "CSectionsd::getCurrentNextServiceKey: Request of current/next information for: %llx\n", uniqueServiceKey);

	SIevent currentEvt;
	SIevent nextEvt;
	unsigned flag = 0, flag2 = 0;
	/* ugly hack: retry fetching current/next by restarting dmxCN if this is true */
	bool change = false;

	readLockEvents();

	/* if the currently running program is requested... */
	if (uniqueServiceKey == messaging_current_servicekey) 
	{
		/* ...check for myCurrentEvent and myNextEvent */
		if (!myCurrentEvent) 
		{
			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: !myCurrentEvent\n");
			change = true;
			flag |= epgflags::not_broadcast;
		} 
		else 
		{
			currentEvt = *myCurrentEvent;
			flag |= epgflags::has_current; // aktuelles event da...
			flag |= epgflags::has_anything;
		}
		
		if (!myNextEvent) 
		{
			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: !myNextEvent\n");
			change = true;
		} 
		else 
		{
			nextEvt = *myNextEvent;
			if (flag & epgflags::not_broadcast) 
			{
				dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: CSectionsd::epgflags::has_no_current\n");
				flag = epgflags::has_no_current;
			}
			flag |= epgflags::has_next; // aktuelles event da...
			flag |= epgflags::has_anything;
		}
	}

	/* if another than the currently running program is requested, then flag will still be 0
	if either the current or the next event is not found, this condition will be true, too.
	*/
	if ((flag & (epgflags::has_current|epgflags::has_next)) != (epgflags::has_current|epgflags::has_next)) 
	{
		SItime zeitEvt1(0, 0);
		if (!(flag & epgflags::has_current)) 
		{
			currentEvt = findActualSIeventForServiceUniqueKey(uniqueServiceKey, zeitEvt1, 0, &flag2);
		} 
		else 
		{
			zeitEvt1.startzeit = currentEvt.times.begin()->startzeit;
			zeitEvt1.dauer = currentEvt.times.begin()->dauer;
		}
		SItime zeitEvt2(zeitEvt1);

		if (currentEvt.getName().empty() && flag2 != 0)
		{
			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: change1\n");
			change = true;
		}

		if (currentEvt.service_id != 0)
		{	//Found
			flag &= (epgflags::has_no_current|epgflags::not_broadcast)^(unsigned)-1;
			flag |= epgflags::has_current;
			flag |= epgflags::has_anything;

			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: current EPG found. service_id: %x, flag: 0x%x\n",currentEvt.service_id, flag);

			if (!(flag & epgflags::has_next)) 
			{
				dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: *nextEvt not from cur/next V1!\n");
				nextEvt = findNextSIevent(currentEvt.uniqueKey(), zeitEvt2);
			}
		}
		else
		{	// no current event...
			readLockServices();

			MySIservicesOrderUniqueKey::iterator si = mySIservicesOrderUniqueKey.end();
			si = mySIservicesOrderUniqueKey.find(uniqueServiceKey);

			if (si != mySIservicesOrderUniqueKey.end())
			{
				dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: current service has%s scheduled events, and has%s present/following events\n", si->second->eitScheduleFlag() ? "" : " no", si->second->eitPresentFollowingFlag() ? "" : " no" );

				if ( /*( !si->second->eitScheduleFlag() ) || */
					( !si->second->eitPresentFollowingFlag() ) )
				{
					flag |= epgflags::not_broadcast;
				}
			}
			unlockServices();

			if ( flag2 & epgflags::has_anything )
			{
				flag |= epgflags::has_anything;
				if (!(flag & epgflags::has_next)) 
				{
					dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: *nextEvt not from cur/next V2!\n");
					nextEvt = findNextSIeventForServiceUniqueKey(uniqueServiceKey, zeitEvt2);
				}

				if (nextEvt.service_id != 0)
				{
					MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueServiceKey);

					if (eFirst != mySIeventsOrderUniqueKey.end())
					{
						// this is a race condition if first entry found is == mySIeventsOrderUniqueKey.begin()
						// so perform a check
						if (eFirst != mySIeventsOrderUniqueKey.begin())
							--eFirst;

						if (eFirst != mySIeventsOrderUniqueKey.begin())
						{
							time_t azeit = time(NULL);

							if (eFirst->second->times.begin()->startzeit < azeit &&
									eFirst->second->uniqueKey() == nextEvt.uniqueKey() - 1)
								flag |= epgflags::has_no_current;
						}
					}
				}
			}
		}
		
		if (nextEvt.service_id != 0)
		{
			flag &= epgflags::not_broadcast^(unsigned)-1;
			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: next EPG found. service_id: %x, flag: 0x%x\n",nextEvt.service_id, flag);
			flag |= epgflags::has_next;
		}
		else if (flag != 0)
		{
			dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: change2 flag: 0x%02x\n", flag);
			change = true;
		}
	}

	if (currentEvt.service_id != 0)
	{
		/* check for nvod linkage */
		for (unsigned int i = 0; i < currentEvt.linkage_descs.size(); i++)
			if (currentEvt.linkage_descs[i].linkageType == 0xB0)
			{
				dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: linkage in current EPG found.\n");
				flag |= epgflags::current_has_linkagedescriptors;
				break;
			}
	} 
	else
		flag |= epgflags::has_no_current;

	time_t now;

	dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: currentEvt: '%s' (%04x) nextEvt: '%s' (%04x) flag: 0x%02x\n",
		currentEvt.getName().c_str(), currentEvt.eventID,
		nextEvt.getName().c_str(), nextEvt.eventID, flag);

	sectionsdTime time_cur;
	sectionsdTime time_nxt;
	now = time(NULL);
	time_cur.startzeit = currentEvt.times.begin()->startzeit;
	time_cur.dauer = currentEvt.times.begin()->dauer;
	time_nxt.startzeit = nextEvt.times.begin()->startzeit;
	time_nxt.dauer = nextEvt.times.begin()->dauer;
	
	// for nvod events that have multiple times, find the one that matches the current time... 
	if (currentEvt.times.size() > 1) 
	{
		for (SItimes::iterator t = currentEvt.times.begin(); t != currentEvt.times.end(); ++t) 
		{
			if ((long)now < (long)(t->startzeit + t->dauer) && (long)now > (long)t->startzeit) 
			{
				time_cur.startzeit = t->startzeit;
				time_cur.dauer =t->dauer;
				break;
			}
		}
	}

	//...and the one after that.
	if (nextEvt.times.size() > 1) 
	{
		for (SItimes::iterator t = nextEvt.times.begin(); t != nextEvt.times.end(); ++t) 
		{
			if ((long)(time_cur.startzeit + time_cur.dauer) <= (long)(t->startzeit)) 
			{ // TODO: it's not "long", it's "time_t"
				time_nxt.startzeit = t->startzeit;
				time_nxt.dauer =t->dauer;
				break;
			}
		}
	}

	current_next.current_uniqueKey = currentEvt.uniqueKey();
	current_next.current_zeit.startzeit = time_cur.startzeit;
	current_next.current_zeit.dauer = time_cur.dauer;
	current_next.current_name = currentEvt.getName();

	current_next.next_uniqueKey = nextEvt.uniqueKey();
	current_next.next_zeit.startzeit = time_nxt.startzeit;
	current_next.next_zeit.dauer = time_nxt.dauer;
	current_next.next_name = nextEvt.getName();

	current_next.flags = flag;
	current_next.current_fsk = currentEvt.getFSK();

	unlockEvents();

	if (change && !messaging_eit_is_busy && (time_monotonic() - messaging_last_requested) < 11) 
	{
		/* restart dmxCN, but only if it is not already running, and only for 10 seconds */
		dprintf(DEBUG_INFO, "CSectionsd::getCurrentNextServiceKey: change && !messaging_eit_is_busy => dmxCN.change(0)\n");
		dmxCN.change(0);
	}
}

bool CSectionsd::getEPGidShort(event_id_t epgID, CShortEPGData * epgdata)
{
	bool ret = false;
	dprintf(DEBUG_DEBUG, "CSectionsd::getEPGidShort: Request of current EPG for 0x%llx\n", epgID);

	readLockEvents();

	const SIevent& e = findSIeventForEventUniqueKey(epgID);

	if (e.service_id != 0)
	{	// Event found
		dprintf(DEBUG_DEBUG, "CSectionsd::getEPGidShort:: EPG found.\n");
		epgdata->title = e.getName();
		epgdata->info1 = e.getText();
		epgdata->info2 = e.getExtendedText();
		ret = true;
	} 
	else
		dprintf(DEBUG_DEBUG, "CSectionsd::getEPGidShort:: EPG not found!\n");
		
	unlockEvents();
	
	return ret;
}

bool CSectionsd::getEPGid(const event_id_t epgID, const time_t startzeit, CEPGData * epgdata)
{
	bool ret = false;
	dprintf(DEBUG_DEBUG, "CSectionsd::getEPGid: Request of actual EPG for 0x%llx 0x%lx\n", epgID, startzeit);

	const SIevent& evt = findSIeventForEventUniqueKey(epgID);

	epgdata->itemDescriptions.clear();
	epgdata->items.clear();

	readLockEvents();
	if (evt.service_id != 0) 
	{ // Event found
		SItimes::iterator t = evt.times.begin();

		for (; t != evt.times.end(); ++t)
			if (t->startzeit == startzeit)
				break;

		if (t == evt.times.end()) 
		{
			dprintf(DEBUG_DEBUG, "CSectionsd::getEPGid EPG not found!\n");
		} 
		else 
		{
			dprintf(DEBUG_DEBUG, "[sectionsd] EPG found.\n");
			
			epgdata->eventID = evt.uniqueKey();
			epgdata->title = evt.getName();
			epgdata->info1 = evt.getText();
			epgdata->info2 = evt.getExtendedText();
			/* FIXME printf("itemDescription: %s\n", evt.itemDescription.c_str()); */
			epgdata->contentClassification = std::string(evt.contentClassification.data(), evt.contentClassification.length());
			epgdata->userClassification = std::string(evt.userClassification.data(), evt.userClassification.length());
			epgdata->fsk = evt.getFSK();
			epgdata->table_id = evt.table_id;

			epgdata->epg_times.startzeit = t->startzeit;
			epgdata->epg_times.dauer = t->dauer;

			ret = true;
		}
	} 
	else 
	{
		dprintf(DEBUG_DEBUG, "CSectionsd::getEPGid: EPG not found!\n");
	}
	
	unlockEvents();
	
	return ret;
}

bool CSectionsd::getActualEPGServiceKey(const t_channel_id uniqueServiceKey, CEPGData * epgdata)
{
	bool ret = false;
	SIevent evt;
	SItime zeit(0, 0);

	dprintf(DEBUG_DEBUG, "CSectionsd::getActualEPGServiceKey Request of current EPG for:%llx\n", uniqueServiceKey);

	readLockEvents();
	if (uniqueServiceKey == messaging_current_servicekey) 
	{
		if (myCurrentEvent) 
		{
			evt = *myCurrentEvent;
			zeit.startzeit = evt.times.begin()->startzeit;
			zeit.dauer = evt.times.begin()->dauer;
			if (evt.times.size() > 1) 
			{
				time_t now = time(NULL);
				for (SItimes::iterator t = evt.times.begin(); t != evt.times.end(); ++t) 
				{
					if ((long)now < (long)(t->startzeit + t->dauer) && (long)now > (long)t->startzeit) 
					{
						zeit.startzeit = t->startzeit;
						zeit.dauer = t->dauer;
						break;
					}
				}
			}
		}
	}

	if (evt.service_id == 0)
	{
		dprintf(DEBUG_DEBUG, "CSectionsd::getActualEPGchannelID evt.service_id == 0 ==> no myCurrentEvent!\n");
		evt = findActualSIeventForServiceUniqueKey(uniqueServiceKey, zeit);
	}

	if (evt.service_id != 0)
	{
		dprintf(DEBUG_DEBUG, "[sectionsd] EPG found.\n");
		epgdata->eventID = evt.uniqueKey();
		epgdata->title = evt.getName();
		epgdata->info1 = evt.getText();
		epgdata->info2 = evt.getExtendedText();
		/* FIXME printf("itemDescription: %s\n", evt.itemDescription.c_str());*/
		epgdata->contentClassification = std::string(evt.contentClassification.data(), evt.contentClassification.length());
		epgdata->userClassification = std::string(evt.userClassification.data(), evt.userClassification.length());
		epgdata->fsk = evt.getFSK();
		epgdata->table_id = evt.table_id;

		epgdata->epg_times.startzeit = zeit.startzeit;
		epgdata->epg_times.dauer = zeit.dauer;

		ret = true;
	} 
	
	unlockEvents();
	return ret;
}

void CSectionsd::getChannelEvents(CChannelEventList &eList, bool tv_mode, t_channel_id *chidlist, int clen)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::getChannelEvents\n");
	
	unsigned char serviceTyp1, serviceTyp2;
	clen = clen / sizeof(t_channel_id);

	t_channel_id uniqueNow = 0;
	t_channel_id uniqueOld = 0;
	bool found_already = false;
	time_t azeit = time(NULL);

	if(tv_mode) 
	{
		serviceTyp1 = 0x01;
		serviceTyp2 = 0x04;
	} 
	else 
	{
		serviceTyp1 = 0x02;
		serviceTyp2 = 0x00;
	}

	readLockEvents();

	/* !!! FIXME: if the box starts on a channel where there is no EPG sent, it hangs!!!	*/
	for (MySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey::iterator e = mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.begin(); e != mySIeventsOrderServiceUniqueKeyFirstStartTimeEventUniqueKey.end(); ++e)
	{
		uniqueNow = (*e)->get_channel_id();
		
		if (!channel_in_requested_list(chidlist, uniqueNow, clen)) 
			continue;

		if ( uniqueNow != uniqueOld )
		{
			found_already = true;
			readLockServices();

			// new service, check service- type
			MySIservicesOrderUniqueKey::iterator s = mySIservicesOrderUniqueKey.find(uniqueNow);

			if (s != mySIservicesOrderUniqueKey.end())
			{
				if (s->second->serviceTyp == serviceTyp1 || (serviceTyp2 && s->second->serviceTyp == serviceTyp2))
				{
					found_already = false;
				}
			}
			else
			{
				// wenn noch nie hingetuned wurde, dann gibts keine Info ber den ServiceTyp...
				// im Zweifel mitnehmen
				found_already = false;
			}
			unlockServices();

			uniqueOld = uniqueNow;
		}

		if ( !found_already )
		{
			for (SItimes::iterator t = (*e)->times.begin(); t != (*e)->times.end(); ++t)
			{
				if (t->startzeit <= azeit && azeit <= (long)(t->startzeit + t->dauer))
				{
					CChannelEvent aEvent;

					aEvent.channelID = (*e)->get_channel_id();
					aEvent.eventID = (*e)->uniqueKey();
					aEvent.startTime = t->startzeit;
					aEvent.duration = t->dauer;
					aEvent.description = (*e)->getName();
					
					if (((*e)->getText()).empty())
						aEvent.text = (*e)->getExtendedText().substr(0, 120);
					else
						aEvent.text = (*e)->getText();

					eList.push_back(aEvent);

					found_already = true;
					break;
				}
			}
		}
	}

	unlockEvents();
}

bool CSectionsd::getComponentTagsUniqueKey(const event_id_t uniqueKey, ComponentTagList& tags)
{
	bool ret = false;
	dprintf(DEBUG_DEBUG, "CSectionsd::getComponentTagsUniqueKey: Request of ComponentTags for 0x%llx\n", uniqueKey);

	tags.clear();

	readLockEvents();

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end()) 
	{
		responseGetComponentTags response;
		ret = true;

		for (SIcomponents::iterator cmp = eFirst->second->components.begin(); cmp != eFirst->second->components.end(); ++cmp) 
		{
			response.component = cmp->component;
			response.componentType = cmp->componentType;
			response.componentTag = cmp->componentTag;
			response.streamContent = cmp->streamContent;

			tags.insert(tags.end(), response);
		}
	}

	unlockEvents();
	
	return ret;

}

bool CSectionsd::getLinkageDescriptorsUniqueKey(const event_id_t uniqueKey, LinkageDescriptorList& descriptors)
{
	bool ret = false;
	dprintf(DEBUG_DEBUG, "CSectionsd::getLinkageDescriptorsUniqueKey: Request of LinkageDescriptors for 0x%llx\n", uniqueKey);

	descriptors.clear();
	readLockEvents();

	MySIeventsOrderUniqueKey::iterator eFirst = mySIeventsOrderUniqueKey.find(uniqueKey);

	if (eFirst != mySIeventsOrderUniqueKey.end()) 
	{
		for (SIlinkage_descs::iterator linkage_desc = eFirst->second->linkage_descs.begin(); linkage_desc != eFirst->second->linkage_descs.end(); ++linkage_desc)
		{
			if (linkage_desc->linkageType == 0xB0) 
			{

				responseGetLinkageDescriptors response;
				
				response.name = linkage_desc->name.c_str();
				response.transportStreamId = linkage_desc->transportStreamId;
				response.originalNetworkId = linkage_desc->originalNetworkId;
				response.serviceId = linkage_desc->serviceId;
				descriptors.insert( descriptors.end(), response);
				ret = true;
			}
		}
	}

	unlockEvents();
	
	return ret;

}
	
bool CSectionsd::getNVODTimesServiceKey(const t_channel_id uniqueServiceKey, NVODTimesList& nvod_list)
{
	bool ret = false;
	dprintf(DEBUG_DEBUG, "CSectionsd::getNVODTimesServiceKey: Request of NVOD times for:%llx\n", uniqueServiceKey);

	nvod_list.clear();

	readLockServices();
	readLockEvents();

	MySIservicesNVODorderUniqueKey::iterator si = mySIservicesNVODorderUniqueKey.find(uniqueServiceKey);
	if (si != mySIservicesNVODorderUniqueKey.end())
	{
		dprintf(DEBUG_DEBUG, "CSectionsd::getNVODTimesServiceKey: NVODServices: %u\n", si->second->nvods.size());

		if (si->second->nvods.size()) 
		{
			for (SInvodReferences::iterator ni = si->second->nvods.begin(); ni != si->second->nvods.end(); ++ni) 
			{
				SItime zeitEvt1(0, 0);
				findActualSIeventForServiceUniqueKey(ni->uniqueKey(), zeitEvt1, 15*60);

				responseGetNVODTimes response;

				response.service_id =  ni->service_id;
				response.original_network_id = ni->original_network_id;
				response.transport_stream_id = ni->transport_stream_id;
				response.zeit.startzeit = zeitEvt1.startzeit;
				response.zeit.dauer = zeitEvt1.dauer;

				nvod_list.insert( nvod_list.end(), response);
				ret = true;
			}
		}
	}

	unlockEvents();
	unlockServices();
	
	return ret;
}

void CSectionsd::setLanguages(const std::vector<std::string>& newLanguages)
{
	SIlanguage::setLanguages(newLanguages);
	SIlanguage::saveLanguages();
}

bool CSectionsd::isReady(void)
{
	return sectionsd_ready;
}

//
void CSectionsd::Start(void)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::Start:\n");

	int rc;

	// load languages
	SIlanguage::loadLanguages();

	//
	tzset(); // TZ auswerten

	//NTP-Config laden
	if (!ntp_config.loadConfig(CONF_FILE))
	{
		// set defaults if no configuration file exists
		printf("sectionsd_Start: %s not found\n", CONF_FILE);
	}

	ntpserver = ntp_config.getString("network_ntpserver", "de.pool.ntp.org");
	ntprefresh = atoi(ntp_config.getString("network_ntprefresh","30").c_str() );
	ntpenable = ntp_config.getBool("network_ntpenable", false);
	
	// check for rdate
	if( !access("/bin/rdate", F_OK) || !access("/sbin/rdate", F_OK) || !access("/usr/bin/rdate", F_OK) || !access("/usr/sbin/rdate", F_OK)) 
		ntp_system_cmd_prefix = "rdate -s ";
	
	ntp_system_cmd = ntp_system_cmd_prefix + ntpserver;

	// epg config
	secondsToCache = (atoi(ntp_config.getString("epg_cache_time","14").c_str() ) *24*60L*60L); //Tage
	secondsExtendedTextCache = (atoi(ntp_config.getString("epg_extendedcache_time","360").c_str() ) *60L*60L); //Stunden
	oldEventsAre = (atoi(ntp_config.getString("epg_old_events","1").c_str() ) *60L*60L); //Stunden
	max_events= atoi(ntp_config.getString("epg_max_events","50000").c_str() );

	dprintf(DEBUG_NORMAL, "[sectionsd] Caching max %d events\n", max_events);
	dprintf(DEBUG_NORMAL, "[sectionsd] Caching %ld days\n", secondsToCache / (24*60*60L));
	dprintf(DEBUG_NORMAL, "[sectionsd] Caching %ld hours Extended Text\n", secondsExtendedTextCache / (60*60L));
	dprintf(DEBUG_NORMAL, "[sectionsd] Events are old %ldmin after their end time\n", oldEventsAre / 60);

	//
	readEPGFilter();
	readDVBTimeFilter();
	readEncodingFile();
	
	messaging_neutrino_sets_time = true;

	// time-Thread starten
	rc = pthread_create(&threadTOT, 0, timeThread, 0);

	if (rc) 
	{
		dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create time-thread (rc=%d)\n", rc);
	}

	if(FrontendCount)
	{
		// EIT-Thread starten
		rc = pthread_create(&threadEIT, 0, eitThread, 0);

		if (rc) 
		{
			dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create eit-thread (rc=%d)\n", rc);
		}

		// CN-Thread starten
		rc = pthread_create(&threadCN, 0, cnThread, 0);

		if (rc) 
		{
			dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create cn-thread (rc=%d)\n", rc);
		}

		// freesat
		rc = pthread_create(&threadFSEIT, 0, fseitThread, 0);

		if (rc) 
		{
			dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create fseit-thread (rc=%d)\n", rc);
		}
		
		// viasat
		rc = pthread_create(&threadVIASATEIT, 0, viasateitThread, 0);

		if (rc) 
		{
			dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create viasateit-thread (rc=%d)\n", rc);
		}
	}
	
	// housekeeping-Thread starten
	rc = pthread_create(&threadHouseKeeping, 0, houseKeepingThread, 0);

	if (rc) 
	{
		dprintf(DEBUG_NORMAL, "[sectionsd] sectionsd_main_thread: failed to create housekeeping-thread (rc=%d)\n", rc);
	}
	
	//
	if (g_settings.epg_xmltv)
        {
        	for (unsigned long i = 0; i < g_settings.xmltv.size(); i++)
        	{
			readSIfromXMLTV(g_settings.xmltv[i].c_str());
		}
	}
	
	sectionsd_ready = true;
	
	//if(FrontendCount)
	eit_update_fd = -1;
}

void CSectionsd::Stop(void)
{
	dprintf(DEBUG_NORMAL, "CSectionsd::Stop:\n");
	
	scanning = 0;
	timeset = true;

	// timethread
	pthread_cancel(threadTOT);
	pthread_join(threadTOT, NULL);
	
	if(FrontendCount)
	{
		if(dmxUTC) 
			delete dmxUTC;
	
		pthread_cancel(threadEIT);
		pthread_join(threadEIT, NULL);
		
		pthread_cancel(threadCN);
		pthread_join(threadCN, NULL);
		
		pthread_cancel(threadFSEIT);
		pthread_join(threadFSEIT, NULL);
		
		pthread_cancel(threadVIASATEIT);
		pthread_join(threadVIASATEIT, NULL);

		//
		eit_stop_update_filter(&eit_update_fd);
		
		if(eitDmx)
			delete eitDmx;

		// close eitdmx
		dmxEIT.close();
		
		// close cndmx
		dmxCN.close();
		
		// close freesatdmx
		dmxFSEIT.close();
		
		//
		dmxVIASAT.close();
	}
	
	sectionsd_stop = 1;
}

