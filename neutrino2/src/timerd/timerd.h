//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: timerd.h 21122024 mohousch Exp $
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

#ifndef __TIMERD_H__
#define __TIMERD_H__

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <configfile.h>
#include <config.h>

#include <timerd/timerdtypes.h>


class CTimerd
{
	public:
		enum CTimerEventTypes
		{
			TIMER_SHUTDOWN = 1,
			TIMER_NEXTPROGRAM,
			TIMER_ZAPTO,
			TIMER_STANDBY,
			TIMER_RECORD,
			TIMER_REMIND,
			TIMER_SLEEPTIMER,
			TIMER_EXEC_PLUGIN,
 			TIMER_IMMEDIATE_RECORD
		};
		
		enum CTimerEventStates 
		{ 
			TIMERSTATE_SCHEDULED, 
			TIMERSTATE_PREANNOUNCE, 
			TIMERSTATE_ISRUNNING, 
			TIMERSTATE_HASFINISHED, 
			TIMERSTATE_TERMINATED 
		};
		
		enum CTimerEventRepeat 
		{ 
			TIMERREPEAT_ONCE = 0,
			TIMERREPEAT_DAILY, 
			TIMERREPEAT_WEEKLY, 
			TIMERREPEAT_BIWEEKLY, 
			TIMERREPEAT_FOURWEEKLY, 
			TIMERREPEAT_MONTHLY, 
			TIMERREPEAT_BYEVENTDESCRIPTION,
			TIMERREPEAT_WEEKDAYS = 0x100 // Bits 9-15 specify weekdays (9=mo,10=di,...)
		};

		struct EventInfo
		{
			event_id_t    	epgID;
			time_t        	epg_starttime;
			t_channel_id  	channel_id;
			unsigned char 	apids;
			bool          	recordingSafety;
			int 		eventID;
			char 		recordingDir[RECORD_DIR_MAXLEN];
			char 		epgTitle[EPG_TITLE_MAXLEN];
		};

		struct RecordingStopInfo
		{
			int eventID;
		};

		struct timerEvent
		{		
			int               eventID;
			CTimerEventTypes  eventType;
			CTimerEventStates eventState;
			CTimerEventRepeat eventRepeat;
			uint32_t	  repeatCount;
			time_t            alarmTime;
			time_t            announceTime;
			time_t            stopTime;
			t_channel_id      channel_id;                       	//only filled if applicable
			event_id_t        epgID;                            	//only filled if applicable
			time_t            epg_starttime;                    	//only filled if applicable
			unsigned char     apids;                            	//only filled if applicable
			bool              standby_on;                       	//only filled if applicable
			char              message[REMINDER_MESSAGE_MAXLEN];	//only filled if applicable
			char              pluginName[EXEC_PLUGIN_NAME_MAXLEN];	//only filled if applicable
			char              recordingDir[RECORD_DIR_MAXLEN];	//only filled if applicable
			char              epgTitle[EPG_TITLE_MAXLEN];		//only filled if applicable
			
			bool operator< (const timerEvent& a) const
			{
				return this->alarmTime < a.alarmTime ;
			}
		};
		
		typedef std::vector<timerEvent> TimerList;

	private:
		CTimerd(){};
		
	public:
		virtual ~CTimerd(){};
		
		static CTimerd *getInstance()
		{
			static CTimerd * timerd = NULL;

			if(!timerd) 
			{
				timerd = new CTimerd();
			} 

			return timerd;
		};
		
		void stopTimerEvent(int evId);
		void removeTimerEvent(int evId);

		int addTimerEvent(CTimerEventTypes evType, void* data, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE, uint32_t repeatcount = 0, bool forceadd = true);
		int setSleeptimer(time_t announcetime, time_t alarmtime, int timerid = 0);
		int getSleeptimerID();
		int getSleepTimerRemaining();
		int addSleepTimerEvent(time_t announcetime,time_t alarmtime);
		int addShutdownTimerEvent(time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
		int addRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID = 0, time_t epg_starttime = 0, time_t announcetime = 0, unsigned char apids = TIMERD_APIDS_STD, bool safety = false, std::string recDir = "", bool forceAdd = true);
		int addImmediateRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID = 0, time_t epg_starttime = 0, unsigned char apids = TIMERD_APIDS_STD);
		int addStandbyTimerEvent(bool standby_on,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
		int addZaptoTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, unsigned long long epgID = 0, time_t epg_starttime = 0,unsigned char apids = TIMERD_APIDS_STD);
		int addNextProgramTimerEvent(EventInfo eventInfo, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);

		bool rescheduleTimerEvent(int eventid, time_t announcediff, time_t alarmdiff, time_t stoptime);
		bool modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime = 0, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE, uint32_t repeatcount = 0, void *data = NULL);
		bool modifyRecordTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEventRepeat evrepeat, uint32_t repeatcount, const char * const recordingdir);

		void getTimer(timerEvent &timer, unsigned timerID);
		void getTimerList(TimerList &timerlist);

		TimerList getOverlappingTimers(time_t& startTime, time_t& stopTime);

		void getWeekdaysFromStr(CTimerEventRepeat *eventRepeat, const char* str);
		void setWeekdaysToStr(CTimerEventRepeat rep, char* str);

		void setRecordingSafety(int pre, int post);
		void getRecordingSafety(int &pre, int &post);

		void modifyTimerAPid(int eventid, unsigned char apids);

		bool isTimerdAvailable();

		//
		void Start(void);
		void Stop(void);
};

#endif

