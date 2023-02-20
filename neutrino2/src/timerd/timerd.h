#ifndef __TIMERD_H__
#define __TIMERD_H__

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <configfile.h>
#include <config.h>

#include <eventserver.h>
#include <timerdclient/timerdtypes.h>


void timerd_stopTimerEvent(int evId);
void timerd_removeTimerEvent(int evId);

int timerd_addTimerEvent(CTimerd::CTimerEventTypes evType, void* data, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, CTimerd::CTimerEventRepeat evrepeat = CTimerd::TIMERREPEAT_ONCE, uint32_t repeatcount = 0, bool forceadd = true);
int timerd_setSleeptimer(time_t announcetime, time_t alarmtime, int timerid = 0);
int timerd_getSleeptimerID();
int timerd_getSleepTimerRemaining();
int timerd_addSleepTimerEvent(time_t announcetime,time_t alarmtime);
int timerd_addShutdownTimerEvent(time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
int timerd_addRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID=0, time_t epg_starttime=0, time_t announcetime = 0, unsigned char apids=TIMERD_APIDS_STD, bool safety=false,std::string recDir="", bool forceAdd=true);
int timerd_addImmediateRecordTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t stoptime, unsigned long long epgID=0, time_t epg_starttime=0,unsigned char apids=TIMERD_APIDS_STD);
int timerd_addStandbyTimerEvent(bool standby_on,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);
int timerd_addZaptoTimerEvent(const t_channel_id channel_id, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0, unsigned long long epgID=0, time_t epg_starttime=0,unsigned char apids=TIMERD_APIDS_STD);
int timerd_addNextProgramTimerEvent(CTimerd::EventInfo eventInfo, time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0);

void timerd_setRecordingSafety(int pre, int post);
void timerd_getRecordingSafety(int &pre, int &post);

#endif

