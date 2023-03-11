/*
	Timerd  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerd.cpp,v 1.60 2006/02/28 21:51:00 zwen Exp $

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

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <signal.h>
#include <unistd.h> /* fork */
#include <syscall.h>

#include <timerd/timermanager.h>
#include <timerd.h>

#include <system/debug.h>


//
void CTimerd::Start(void)
{
	dprintf(DEBUG_NORMAL, "CTimerd::Start:\n");

	// Start timer thread
	CTimerManager::getInstance();
}

//
void CTimerd::stopTimerEvent(int evId)
{
	dprintf(DEBUG_NORMAL, "CTimerd::stopTimerEvent\n");
	
	CTimerManager::getInstance()->stopEvent(evId);
}

void CTimerd::removeTimerEvent(int evId)
{
	dprintf(DEBUG_NORMAL, "CTimerd::removeTimerEvent\n");
	
	CTimerManager::getInstance()->removeEvent(evId);
}

int CTimerd::addTimerEvent(CTimerEventTypes evType, void *data, time_t alarmtime, time_t announcetime, time_t stoptime, CTimerd::CTimerEventRepeat evrepeat, uint32_t repeatcount, bool forceadd)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addTimerEvent\n");

	int eventID = 0;
	CTimerEvent *event = NULL;
	
	if (!forceadd)
	{
		dprintf(DEBUG_NORMAL, "CTimerd::addTimerEvent: checking for overlapping timers\n");
		
		TimerList overlappingTimer;
		overlappingTimer = getOverlappingTimers(alarmtime, stoptime);
		
		if (overlappingTimer.size() > 0)
		{
			// timerd starts eventID at 0 so we can return -1
			return -1;
		}
	}
	
	//			
	switch(evType)
	{
		case TIMER_STANDBY :
		{
			bool standby = static_cast<bool>(data);

			event = new CTimerEvent_Standby(
						announcetime,
						alarmtime,
						standby,
						evrepeat,
						repeatcount);
						
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}

		case TIMER_SHUTDOWN :
		{
			event = new CTimerEvent_Shutdown(
						announcetime,
						alarmtime,
						evrepeat,
						repeatcount);
						
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}

		case TIMER_SLEEPTIMER :
		{
			event = new CTimerEvent_Sleeptimer(
						announcetime,
						alarmtime,
						evrepeat,
						repeatcount);
						
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}

		case TIMER_RECORD :
		{		
			RecordingInfo *ri = static_cast<RecordingInfo*>(data); 
			
			if(ri->recordingSafety)
			{
				int pre, post;
				CTimerManager::getInstance()->getRecordingSafety(pre, post);
				announcetime -= pre;
				alarmtime -= pre;
				stoptime += post;
			}
			
			event = new CTimerEvent_Record(
						announcetime,
						alarmtime,
						stoptime,
						ri->channel_id,
						ri->epgID,
						ri->epg_starttime,
						ri->apids,
						evrepeat,
						repeatcount,
						ri->recordingDir);
						
			eventID = CTimerManager::getInstance()->addEvent(event);
				
			break;
		}
		
		case TIMER_IMMEDIATE_RECORD :
		{
			EventInfo *ei = static_cast<EventInfo*>(data);
		
			event = new CTimerEvent_Record(
						announcetime,
						alarmtime,
						stoptime,
						ei->channel_id,
						ei->epgID,
						ei->epg_starttime,
						ei->apids,
						evrepeat,
						repeatcount);
						
			event->eventState = TIMERSTATE_ISRUNNING;
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}

		case TIMER_ZAPTO :
		{
			EventInfo *ei = static_cast<EventInfo*>(data);
		
			if(ei->channel_id > 0)
			{
				event = new CTimerEvent_Zapto(
							announcetime,
							alarmtime,
							ei->channel_id,
							ei->epgID,
							ei->epg_starttime,
							evrepeat,
							repeatcount);
							
				eventID = CTimerManager::getInstance()->addEvent(event);
			}
			break;
		}

		case TIMER_NEXTPROGRAM :
		{
			EventInfo *ei = static_cast<EventInfo*>(data); 

			event = new CTimerEvent_NextProgram(
							announcetime,
							alarmtime,
							stoptime,
							ei->channel_id,
							ei->epgID,
							ei->epg_starttime,
							evrepeat,
							repeatcount);
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}
					
		case TIMER_REMIND :
		{
			char message[REMINDER_MESSAGE_MAXLEN] = "";
			
			strncpy(message, static_cast<char*>(data), REMINDER_MESSAGE_MAXLEN - 1);
			
			event = new CTimerEvent_Remind(
							announcetime,
							alarmtime,
							message,
							evrepeat,
							repeatcount);
							
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}
					
		case TIMER_EXEC_PLUGIN :
		{
			char pluginname[EXEC_PLUGIN_NAME_MAXLEN] = "";
			
			strncpy(pluginname, static_cast<char*>(data), EXEC_PLUGIN_NAME_MAXLEN - 1);
			
			event = new CTimerEvent_ExecPlugin(
							announcetime,
							alarmtime,
							pluginname,
							evrepeat,
							repeatcount);
							
			eventID = CTimerManager::getInstance()->addEvent(event);
			break;
		}
		default:
			printf("CTimerd::addTimerEvent: Unknown TimerType\n");
	}

	return eventID;
}

int CTimerd::setSleeptimer(time_t announcetime, time_t alarmtime, int timerid)
{
	dprintf(DEBUG_INFO, "CTimerd::setSleepTimer\n");
	
	int timerID;

	if(timerid == 0)
		timerID = getSleeptimerID();
	else
		timerID = timerid;

	if(timerID != 0)
	{
		modifyTimerEvent(timerID, announcetime, alarmtime);
	}
	else
	{
		timerID = addTimerEvent(TIMER_SLEEPTIMER, NULL, announcetime, alarmtime, 0);				
	}

	return timerID;   
}

int CTimerd::getSleeptimerID()
{
	dprintf(DEBUG_NORMAL, "CTimerd::getSleeptimerID\n");
	
	CTimerEventMap events;
	CTimerEventMap::iterator pos;
	
	int eventID = 0;
	
	if (CTimerManager::getInstance()->listEvents(events))
	{
		for (pos = events.begin(); pos != events.end(); pos++)
		{
			dprintf(DEBUG_INFO, "CTimerd::getSleeptimerID: ID: %u type: %u\n", pos->second->eventID, pos->second->eventType);
					
			if(pos->second->eventType == TIMER_SLEEPTIMER)
			{
				eventID = pos->second->eventID;
				break;
			}
		}
	}
	
	return eventID;
}

int CTimerd::getSleepTimerRemaining()
{
	dprintf(DEBUG_NORMAL, "CTimerd::getSleepTimerRemaining\n");
	
	int timerID;
	
	if((timerID = getSleeptimerID()) != 0)
	{
		responseGetTimer timer;
		getTimer( timer, timerID);
		
		int min = (((timer.alarmTime + 1 - time(NULL)) / 60) + 1); //aufrunden auf n�chst gr��erere Min.
		if(min < 1)
			min = 1;
			
		return min;
	}
	else
		return 0;
}

int CTimerd::addSleepTimerEvent(time_t announcetime, time_t alarmtime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addSleepTimerEvent\n");
	
	return addTimerEvent(TIMER_SLEEPTIMER, NULL, announcetime, alarmtime, 0);
}

int CTimerd::addShutdownTimerEvent(time_t alarmtime, time_t announcetime, time_t stoptime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addShutdownTimerEvent\n");
	
	return addTimerEvent(TIMER_SHUTDOWN, NULL, announcetime, alarmtime, stoptime);
}

int CTimerd::addRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID, time_t epg_starttime, time_t announcetime, unsigned char apids, bool safety, std::string recDir, bool forceAdd)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addRecordTimerEvent\n");
	
	RecordingInfo eventInfo;
	
	eventInfo.channel_id = channel_id;
	eventInfo.epgID = epgID;
	eventInfo.epg_starttime = epg_starttime;
	eventInfo.apids = apids;
	eventInfo.recordingSafety = safety;
	strncpy(eventInfo.recordingDir, recDir.c_str(), RECORD_DIR_MAXLEN);
	
	return addTimerEvent(TIMER_RECORD, &eventInfo, announcetime, alarmtime, stoptime, TIMERREPEAT_ONCE, 0, forceAdd);
}

int CTimerd::addImmediateRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID, time_t epg_starttime, unsigned char apids)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addImmediateRecordTimerEvent\n");
	
	EventInfo eventInfo;
	
	eventInfo.channel_id = channel_id;
	eventInfo.epgID = epgID;
	eventInfo.epg_starttime = epg_starttime;
	eventInfo.apids = apids;
	eventInfo.recordingSafety = false;
	
	return addTimerEvent(TIMER_IMMEDIATE_RECORD, &eventInfo, 0, alarmtime, stoptime);
}

int CTimerd::addStandbyTimerEvent(bool standby_on, time_t alarmtime, time_t announcetime, time_t stoptime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addStandbyTimerEvent\n");
	
	return addTimerEvent(TIMER_STANDBY, &standby_on,  announcetime, alarmtime, stoptime);
}

int CTimerd::addZaptoTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t announcetime, time_t stoptime, unsigned long long epgID, time_t epg_starttime, unsigned char apids)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addZaptoTimerEvent\n");
	
	EventInfo eventInfo;
	
	eventInfo.channel_id = channel_id;
	eventInfo.epgID = epgID;
	eventInfo.epg_starttime = epg_starttime;
	eventInfo.apids = apids;
	
	return addTimerEvent(TIMER_ZAPTO, &eventInfo, announcetime, alarmtime, stoptime);
}

int CTimerd::addNextProgramTimerEvent(CTimerd::EventInfo eventInfo, time_t alarmtime, time_t announcetime, time_t stoptime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::addNextProgramTimerEvent\n");
	
	return addTimerEvent(TIMER_NEXTPROGRAM, &eventInfo, alarmtime, announcetime, stoptime);
}

bool CTimerd::rescheduleTimerEvent(int eventid, time_t announcediff, time_t alarmdiff, time_t stoptime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::resheduleTimerEvent\n");
	
	int ret = CTimerManager::getInstance()->rescheduleEvent(eventid, announcediff, alarmdiff, stoptime);
	
	bool status = (ret != 0);
	
	return status;
}

bool CTimerd::modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEventRepeat evrepeat, uint32_t repeatcount, void *data)
{
	dprintf(DEBUG_NORMAL, "CTimerd::modifyTimerEvent\n");
	
	//
	responseGetTimer *timerInfo = static_cast<responseGetTimer*>(data);
			
	int ret = CTimerManager::getInstance()->modifyEvent(eventid, announcetime, alarmtime, stoptime, repeatcount, evrepeat, *timerInfo);
	
	bool status = (ret != 0);
	
	return status;
}

bool CTimerd::modifyRecordTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerd::CTimerEventRepeat evrepeat, uint32_t repeatcount, const char * const recordingdir)
{
	dprintf(DEBUG_NORMAL, "CTimerd::modifyRecordTimerEvent\n");
	
	responseGetTimer resp;
	
	strncpy(resp.recordingDir, recordingdir, RECORD_DIR_MAXLEN - 1);
	
	return modifyTimerEvent(eventid, announcetime, alarmtime, stoptime, evrepeat, repeatcount, &resp);
}

void CTimerd::setRecordingSafety(int pre, int post)
{
	dprintf(DEBUG_NORMAL, "CTimerd::setRecordingSafety\n");
	
	CTimerManager::getInstance()->setRecordingSafety(pre, post);
}

void CTimerd::getRecordingSafety(int &pre, int &post)
{
	dprintf(DEBUG_NORMAL, "CTimerd::getRecordingSafety\n");
	
	CTimerManager::getInstance()->getRecordingSafety(pre, post);
}

void CTimerd::modifyTimerAPid(int eventid, unsigned char apids)
{
	dprintf(DEBUG_NORMAL, "CTimerd::modifyTimerAPid\n");
	
	CTimerManager::getInstance()->modifyEvent(eventid, apids);
}

bool CTimerd::isTimerdAvailable()
{
	dprintf(DEBUG_NORMAL, "CTimerd::isTimerAvailable\n");
	
	return true;
}

void CTimerd::getTimer(CTimerd::responseGetTimer &timer, unsigned timerID)
{
	dprintf(DEBUG_NORMAL, "CTimerd::getTimer\n");
	
	CTimerEventMap events;
	responseGetTimer resp;
	
	if(CTimerManager::getInstance()->listEvents(events))
	{
		if(events[timerID])
		{
			CTimerEvent *event = events[timerID];
			
			resp.eventID = event->eventID;
			resp.eventState = event->eventState;
			resp.eventType = event->eventType;
			resp.eventRepeat = event->eventRepeat;
			resp.announceTime = event->announceTime;
			resp.alarmTime = event->alarmTime;
			resp.stopTime = event->stopTime;
			resp.repeatCount = event->repeatCount;
					
			if(event->eventType == TIMER_STANDBY)
				resp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
			else if(event->eventType == TIMER_NEXTPROGRAM)
			{
				resp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
				resp.epg_starttime = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epg_starttime;
				resp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
				resp.apids = static_cast<CTimerEvent_Record*>(event)->eventInfo.apids;
			}
			else if(event->eventType == TIMER_RECORD)
			{
				CTimerEvent_Record* ev= static_cast<CTimerEvent_Record*>(event);
				resp.epgID = ev->eventInfo.epgID;
				resp.epg_starttime = ev->eventInfo.epg_starttime;
				resp.channel_id = ev->eventInfo.channel_id;
				resp.apids = ev->eventInfo.apids;
				strcpy(resp.recordingDir, ev->recordingDir.substr(0, sizeof(resp.recordingDir)-1).c_str());						
				strcpy(resp.epgTitle, ev->epgTitle.substr(0,sizeof(resp.epgTitle)-1).c_str());						
			}
			else if(event->eventType == TIMER_ZAPTO)
			{
				CTimerEvent_Zapto* ev= static_cast<CTimerEvent_Zapto*>(event);
				resp.epgID = ev->eventInfo.epgID;
				resp.epg_starttime = ev->eventInfo.epg_starttime;
				resp.channel_id = ev->eventInfo.channel_id;
				resp.apids = ev->eventInfo.apids;
				strcpy(resp.epgTitle, ev->epgTitle.substr(0,sizeof(resp.epgTitle)-1).c_str());						
			}
			else if(event->eventType == TIMER_REMIND)
			{
				memset(resp.message, 0, sizeof(resp.message));
				strncpy(resp.message, static_cast<CTimerEvent_Remind*>(event)->message, sizeof(resp.message) - 1);
			}
			else if (event->eventType == TIMER_EXEC_PLUGIN)
			{
				memset(resp.pluginName, 0, sizeof(resp.pluginName));
				strncpy(resp.pluginName, static_cast<CTimerEvent_ExecPlugin*>(event)->name, sizeof(resp.message)-1);						
			}
			
			timer = resp;
		}
	}
}

void CTimerd::getTimerList( CTimerd::TimerList &timerlist)
{
	dprintf(DEBUG_NORMAL, "CTimerd::getTimerList\n");
	
	CTimerEventMap events;
	
	timerlist.clear();
	
	if(CTimerManager::getInstance()->listEvents(events))
	{
		for(CTimerEventMap::iterator lpos = events.begin(); lpos != events.end(); lpos++)
		{
			responseGetTimer lresp;

			CTimerEvent *event = lpos->second;

			lresp.eventID = event->eventID;
			lresp.eventState = event->eventState;
			lresp.eventType = event->eventType;
			lresp.eventRepeat = event->eventRepeat;
			lresp.announceTime = event->announceTime;
			lresp.alarmTime = event->alarmTime;
			lresp.stopTime = event->stopTime;
			lresp.repeatCount = event->repeatCount;

			if(event->eventType == TIMER_STANDBY)
				lresp.standby_on = static_cast<CTimerEvent_Standby*>(event)->standby_on;
			else if(event->eventType == TIMER_NEXTPROGRAM)
			{
				lresp.epgID = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epgID;
				lresp.epg_starttime = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.epg_starttime;
				lresp.channel_id = static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.channel_id;
				lresp.apids = static_cast<CTimerEvent_Record*>(event)->eventInfo.apids;
			}
			else if(event->eventType == TIMER_RECORD)
			{
				CTimerEvent_Record* ev= static_cast<CTimerEvent_Record*>(event);
				lresp.epgID = ev->eventInfo.epgID;
				lresp.epg_starttime = ev->eventInfo.epg_starttime;
				lresp.channel_id = ev->eventInfo.channel_id;
				lresp.apids = ev->eventInfo.apids;
				strcpy(lresp.recordingDir, ev->recordingDir.substr(0, sizeof(lresp.recordingDir)-1).c_str());
				strcpy(lresp.epgTitle, ev->epgTitle.substr(0, sizeof(lresp.epgTitle)-1).c_str());						
			}
			else if(event->eventType == TIMER_ZAPTO)
			{
				CTimerEvent_Zapto* ev= static_cast<CTimerEvent_Zapto*>(event);
				lresp.epgID = ev->eventInfo.epgID;
				lresp.epg_starttime = ev->eventInfo.epg_starttime;
				lresp.channel_id = ev->eventInfo.channel_id;
				lresp.apids = ev->eventInfo.apids;
				strcpy(lresp.epgTitle, ev->epgTitle.substr(0, sizeof(lresp.epgTitle)-1).c_str());						
			}
			else if(event->eventType == TIMER_REMIND)
			{
				strcpy(lresp.message, static_cast<CTimerEvent_Remind*>(event)->message);
			}
			else if(event->eventType == TIMER_EXEC_PLUGIN)
			{
				strcpy(lresp.pluginName, static_cast<CTimerEvent_ExecPlugin*>(event)->name);
			}
				
			timerlist.push_back(lresp);
		}
	}
}

CTimerd::TimerList CTimerd::getOverlappingTimers(time_t& startTime, time_t& stopTime)
{
	dprintf(DEBUG_NORMAL, "CTimerd::getOverlappingTimers\n");
	
	TimerList timerlist; 
	TimerList overlapping;
	int timerPre;
	int timerPost;

	getTimerList(timerlist);
	getRecordingSafety(timerPre, timerPost);

	for (TimerList::iterator it = timerlist.begin(); it != timerlist.end();it++)
	{
		if(it->stopTime != 0 && stopTime != 0)
		{
			// Check if both timers have start and end. In this case do not show conflict, if endtime is the same than the starttime of the following timer
			if ((stopTime + timerPost > it->alarmTime) && (startTime - timerPre < it->stopTime)) 
			{
				overlapping.push_back(*it);
			}
		}
		else
		{
			if (!((stopTime < it->announceTime) || (startTime > it->stopTime)))
			{
				overlapping.push_back(*it);
			}
		}
	}
	
	return overlapping;
}

void CTimerd::getWeekdaysFromStr(CTimerEventRepeat *eventRepeat, const char* str)
{
	int rep = (int) *eventRepeat;
	
	if(rep >= (int)TIMERREPEAT_WEEKDAYS)
	{
		for(int n = 0; n < 7; n++)
		{
			if(str[n] == 'X' || str[n] == 'x')
			{
				rep |= (1 << (n + 9));
			}
			else
			{
				rep &= (~(1 << (n + 9)));
			}
		}
	}
	
	*eventRepeat = (CTimerEventRepeat) rep;
}

void CTimerd::setWeekdaysToStr(CTimerEventRepeat rep, char* str)
{
	if(rep >= TIMERREPEAT_WEEKDAYS)
	{
		for(int n = 0; n < 7; n++)
		{
			if(rep & (1 << (n + 9)))
				str[n] = 'X';
			else
				str[n] = '-';
		}
		str[7] = 0;
	}
}

void CTimerd::Stop(void)
{
	dprintf(DEBUG_NORMAL, "CTimerd::Stop:\n");
	
	CTimerManager::getInstance()->shutdown();
}

