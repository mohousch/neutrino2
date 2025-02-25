//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: timermanager.h 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifndef __neutrino_timermanager__
#define __neutrino_timermanager__

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <global.h>

#include <configfile.h>
#include <config.h>

#include <timerd/timerdtypes.h>
#include <timerd/timerd.h>


#define TIMERD_CONFIGFILE CONFIGDIR "/timerd.conf"

class CTimerEvent
{
	public:
		int                        eventID;       // event identifier
		CTimerd::CTimerEventTypes  eventType;     // Type of event
		CTimerd::CTimerEventStates eventState;    // actual event state
		CTimerd::CTimerEventStates previousState; // previous event state
		CTimerd::CTimerEventRepeat eventRepeat;
		uint32_t repeatCount;                     // how many times timer will be executed

		// time values
		time_t                     alarmTime;     // event start time
		time_t                     stopTime;      // 0 = one time shot
		time_t                     announceTime;  // when should event be announced (0=none)

		//
		CTimerEvent( CTimerd::CTimerEventTypes evtype, int mon = 0, int day = 0, int hour = 0, int min = 0, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 1);
		CTimerEvent( CTimerd::CTimerEventTypes evtype, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 1);
		CTimerEvent( CTimerd::CTimerEventTypes evtype, CConfigFile *config, int iId);
			
		void setState(CTimerd::CTimerEventStates newstate){previousState = eventState; eventState = newstate;};

		static int remain_min(const time_t t) {return (t - time(NULL)) / 60;};
		
		virtual void Reschedule();
		virtual void fireEvent(){};
		virtual void stopEvent(){};
		virtual void announceEvent(){};
		virtual void saveToConfig(CConfigFile *config);
		
		virtual void Refresh(){};
		virtual ~CTimerEvent(){};
};

typedef std::map<int, CTimerEvent*> CTimerEventMap;

//// shutdown
class CTimerEvent_Shutdown : public CTimerEvent
{
	public:
		CTimerEvent_Shutdown( time_t lannounceTime, time_t lalarmTime, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 1) :
			CTimerEvent(CTimerd::TIMER_SHUTDOWN, lannounceTime, lalarmTime, (time_t) 0, evrepeat, repeatcount ){};
		CTimerEvent_Shutdown(CConfigFile *config, int iId):
			CTimerEvent(CTimerd::TIMER_SHUTDOWN, config, iId){};
			
		virtual void fireEvent();
		virtual void announceEvent();
};

//// sleeptimer
class CTimerEvent_Sleeptimer : public CTimerEvent
{
	public:
		CTimerEvent_Sleeptimer( time_t lannounceTime, time_t lalarmTime, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 1) :
			CTimerEvent(CTimerd::TIMER_SLEEPTIMER, lannounceTime, lalarmTime, (time_t) 0,evrepeat,repeatcount ){};
		CTimerEvent_Sleeptimer(CConfigFile *config, int iId):
			CTimerEvent(CTimerd::TIMER_SLEEPTIMER, config, iId){};
			
		virtual void fireEvent();
		virtual void announceEvent();
};

//// standby
class CTimerEvent_Standby : public CTimerEvent
{
	public:
		bool standby_on;

		CTimerEvent_Standby(time_t lannounceTime,
				    time_t lalarmTime, 
				    bool sb_on,
				    CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
				    uint32_t repeatcount = 1);
		CTimerEvent_Standby(CConfigFile *config, int iId);
		
		virtual void fireEvent();
		virtual void saveToConfig(CConfigFile *config);
};

//// record
class CTimerEvent_Record : public CTimerEvent
{
	public:
		CTimerd::EventInfo eventInfo;
		std::string recordingDir;
		std::string epgTitle;
		
		CTimerEvent_Record(time_t lannounceTime, time_t lalarmTime, time_t lstopTime, 
				  t_channel_id channel_id,
				  event_id_t epgID = 0,
				  time_t epg_starttime = 0, 
				  unsigned char apids = TIMERD_APIDS_STD,
				  CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
				  uint32_t repeatcount = 1, const std::string recDir = "");
		CTimerEvent_Record(CConfigFile *config, int iId);
		virtual ~CTimerEvent_Record(){};
		virtual CTimerd::CTimerEventTypes getEventType(void) const { return CTimerd::TIMER_RECORD; };
		
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
		virtual void saveToConfig(CConfigFile *config);
		virtual void Reschedule();
		void getEpgId();
		virtual void Refresh();
};

//// zapto
class CTimerEvent_Zapto : public CTimerEvent_Record
{
	public:
		CTimerEvent_Zapto(time_t lannounceTime, time_t lalarmTime, 
				  t_channel_id channel_id,
				  event_id_t epgID = 0,
				  time_t epg_starttime = 0, 
				  CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
				  uint32_t repeatcount = 1):
			CTimerEvent_Record(lannounceTime, lalarmTime, (time_t) 0, channel_id, epgID, epg_starttime, 0, evrepeat,repeatcount)
		{eventType = getEventType();};
		CTimerEvent_Zapto(CConfigFile *config, int iId):
			CTimerEvent_Record(config, iId)
			{eventType = getEventType();};
		virtual CTimerd::CTimerEventTypes getEventType(void) const { return CTimerd::TIMER_ZAPTO; };
		
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent(){};
		void getEpgId();
};

//// next program
class CTimerEvent_NextProgram : public CTimerEvent
{
	public:
		CTimerd::EventInfo eventInfo;

		CTimerEvent_NextProgram(time_t lannounceTime, time_t lalarmTime, time_t lstopTime, 
					t_channel_id channel_id,
					event_id_t epgID = 0,
					time_t epg_starttime = 0,
					CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
					uint32_t repeatcount = 1);
		CTimerEvent_NextProgram(CConfigFile *config, int iId);
		virtual ~CTimerEvent_NextProgram(){};
		
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void saveToConfig(CConfigFile *config);
		virtual void Reschedule();
};

//// remind
class CTimerEvent_Remind : public CTimerEvent
{
	public:
		char message[REMINDER_MESSAGE_MAXLEN];

		CTimerEvent_Remind(time_t lannounceTime,
				  time_t lalarmTime, 
				  const char * const msg,
				  CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
				  uint32_t repeatcount = 1);
		CTimerEvent_Remind(CConfigFile *config, int iId);
		
		virtual void fireEvent();
		virtual void saveToConfig(CConfigFile *config);
};

//// exec plugin
class CTimerEvent_ExecPlugin : public CTimerEvent
{
	public:
		char name[EXEC_PLUGIN_NAME_MAXLEN];

		CTimerEvent_ExecPlugin(time_t lannounceTime,
				      time_t lalarmTime, 
				      const char * const plugin,
				      CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE,
				      uint32_t repeatcount = 1);
		CTimerEvent_ExecPlugin(CConfigFile *config, int iId);
		
		virtual void fireEvent();
		virtual void saveToConfig(CConfigFile *config);
};

//// CTimerManager
class CTimerManager
{
	//singleton
	private:
		void Init(void);
		int			eventID;
		CTimerEventMap		events;
		pthread_t		thrTimer;
		bool              	m_saveEvents;
		bool              	m_isTimeSet;
		int               	m_extraTimeStart;
		int               	m_extraTimeEnd;
		bool              	wakeup;
		
		CTimerManager();
		static void* timerThread(void *);
		CTimerEvent *nextEvent();
	public:
		static CTimerManager* getInstance();
		~CTimerManager(){};
		
		//
		int addEvent(CTimerEvent*, bool save = true);
		bool removeEvent(int leventID);
		bool stopEvent(int leventID);
		CTimerEvent* getNextEvent();
		bool listEvents(CTimerEventMap &Events);
		CTimerd::CTimerEventTypes *getEventType(int leventID);
		int modifyEvent(int leventID, time_t announceTime, time_t alarmTime, time_t stopTime, uint32_t repeatcount, CTimerd::CTimerEventRepeat evrepeat, CTimerd::timerEvent &data);
		int modifyEvent(int leventID, unsigned char apids);
		int rescheduleEvent(int leventID, time_t announceTime, time_t alarmTime, time_t stopTime);
		void saveEventsToConfig();
		void loadEventsFromConfig();
		bool shutdown();
		void shutdownOnWakeup(int currEventId);
		void getRecordingSafety(int &pre, int &post){pre=m_extraTimeStart; post=m_extraTimeEnd;}
		void setRecordingSafety(int pre, int post);  
		void loadRecordingSafety();
};

#endif

