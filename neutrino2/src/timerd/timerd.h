#ifndef __TIMERD_H__
#define __TIMERD_H__

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <configfile.h>
#include <config.h>

#include <libeventserver/eventserver.h>
#include <timerd/timerdtypes.h>


class CTimerd
{
	public:
		enum events
		{
			EVT_SHUTDOWN = 1,
			EVT_ANNOUNCE_SHUTDOWN,
			EVT_ZAPTO,
			EVT_ANNOUNCE_ZAPTO,
			EVT_NEXTPROGRAM,
			EVT_ANNOUNCE_NEXTPROGRAM,
			EVT_STANDBY_ON,
			EVT_STANDBY_OFF,
			EVT_RECORD_START,
			EVT_RECORD_STOP,
			EVT_ANNOUNCE_RECORD,
			EVT_ANNOUNCE_SLEEPTIMER,
			EVT_SLEEPTIMER,
			EVT_REMIND,
			EVT_EXEC_PLUGIN
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

		struct EventInfo
		{
			event_id_t    epgID;
			time_t        epg_starttime;
			t_channel_id  channel_id;
			unsigned char apids;
			bool          recordingSafety;
		};

		struct TransferEventInfo
		{
			event_id_t    epgID;
			time_t        epg_starttime;
			t_channel_id  channel_id;
			unsigned char apids;
			bool          recordingSafety;
		};

		struct TransferRecordingInfo : TransferEventInfo
		{
			char recordingDir[RECORD_DIR_MAXLEN];
			char epgTitle[EPG_TITLE_MAXLEN];

		};

		class RecordingInfo : public EventInfo
		{
			public:
				RecordingInfo(){};
				
				RecordingInfo(EventInfo& e)
				{
					apids = e.apids;
					channel_id = e.channel_id;
					epgID = e.epgID;
					epg_starttime = e.epg_starttime;
					recordingSafety = e.recordingSafety;
				};
				
				RecordingInfo& operator = (EventInfo& e)
				{
					apids = e.apids;
					channel_id = e.channel_id;
					epgID = e.epgID;
					epg_starttime = e.epg_starttime;
					recordingSafety = e.recordingSafety;
					return *this;
				}
				
				unsigned char apids;
				int eventID;
				char recordingDir[RECORD_DIR_MAXLEN];
				char epgTitle[EPG_TITLE_MAXLEN];
		};

		struct RecordingStopInfo
		{
			int eventID;
		};

		struct responseGetTimer
		{		
			int               eventID;
			CTimerEventTypes  eventType;
			CTimerEventStates eventState;
			CTimerEventRepeat eventRepeat;
			uint32_t	  repeatCount;
			time_t            alarmTime;
			time_t            announceTime;
			time_t            stopTime;
			t_channel_id      channel_id;                       //only filled if applicable
			event_id_t        epgID;                            //only filled if applicable
			time_t            epg_starttime;                    //only filled if applicable
			unsigned char     apids;                            //only filled if applicable
			bool              standby_on;                       //only filled if applicable
			char              message[REMINDER_MESSAGE_MAXLEN];         //only filled if applicable
			char              pluginName[EXEC_PLUGIN_NAME_MAXLEN];      //only filled if applicable
			char              recordingDir[RECORD_DIR_MAXLEN];       //only filled if applicable
			char              epgTitle[EPG_TITLE_MAXLEN];       //only filled if applicable
			
			bool operator< (const responseGetTimer& a) const
			{
				return this->alarmTime < a.alarmTime ;
			}
		};
		
		typedef std::vector<responseGetTimer> TimerList;
		
		//
		struct commandAddTimer
		{
			CTimerd::CTimerEventTypes  eventType;
			CTimerd::CTimerEventRepeat eventRepeat;
			time_t                     alarmTime;
			time_t                     announceTime;
			time_t                     stopTime;			
			uint32_t                   repeatCount;
		};

		struct commandGetTimer
		{
			int   eventID;
		};
		
		struct commandRemind
		{
			char message[REMINDER_MESSAGE_MAXLEN];
		};

		struct commandExecPlugin
		{
			char name[EXEC_PLUGIN_NAME_MAXLEN];
		};

		struct commandRecordDir
		{
			char recDir[RECORD_DIR_MAXLEN];
		};
		
		struct commandSetStandby
		{
			bool standby_on;
		};
		
		struct generalInteger
		{
			int number;
		};

		struct responseAddTimer
		{
			int   eventID;
		};
		
		struct responseGetSleeptimer
		{
			int   eventID;
		};
		////
		
	private:
		CTimerd(){};
		
	public:
		~CTimerd(){};
		
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

		int addTimerEvent(CTimerd::CTimerEventTypes evType, void* data, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 0, bool forceadd = true);
		int setSleeptimer(time_t announcetime, time_t alarmtime, int timerid = 0);
		int getSleeptimerID();
		int getSleepTimerRemaining();
		int addSleepTimerEvent(time_t announcetime,time_t alarmtime);
		int addShutdownTimerEvent(time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
		int addRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID = 0, time_t epg_starttime = 0, time_t announcetime = 0, unsigned char apids = TIMERD_APIDS_STD, bool safety = false, std::string recDir = "", bool forceAdd = true);
		int addImmediateRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID = 0, time_t epg_starttime = 0, unsigned char apids = TIMERD_APIDS_STD);
		int addStandbyTimerEvent(bool standby_on,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
		int addZaptoTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, unsigned long long epgID=0, time_t epg_starttime = 0,unsigned char apids = TIMERD_APIDS_STD);
		int addNextProgramTimerEvent(CTimerd::EventInfo eventInfo, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);

		bool rescheduleTimerEvent(int eventid, time_t announcediff, time_t alarmdiff, time_t stoptime);
		bool modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime = 0, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 0, void *data = NULL, int datalen = 0);
		bool modifyRecordTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerd::CTimerEventRepeat evrepeat, uint32_t repeatcount, const char * const recordingdir);

		void getTimer(CTimerd::responseGetTimer &timer, unsigned timerID);
		void getTimerList( CTimerd::TimerList &timerlist);

		CTimerd::TimerList getOverlappingTimers(time_t& startTime, time_t& stopTime);

		void getWeekdaysFromStr(CTimerd::CTimerEventRepeat *eventRepeat, const char* str);
		void setWeekdaysToStr(CTimerd::CTimerEventRepeat rep, char* str);

		void setRecordingSafety(int pre, int post);
		void getRecordingSafety(int &pre, int &post);

		void modifyTimerAPid(int eventid, unsigned char apids);

		bool isTimerdAvailable();

		//
		void Start();
		void Stop();
};

#endif

