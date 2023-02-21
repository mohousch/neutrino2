/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/lib/timerdclient/timerdmsg.h,v 1.11 2006/02/14 22:38:28 zwen Exp $
 *
 * types used for clientlib <-> timerd communication - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __timerdmsg_h__
#define __timerdmsg_h__

#include <string>
#include <cstring>

#include <timerd/timerdtypes.h> // REMINDER_MESSAGE_MAXLEN


class CTimerdMsg
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
		
		//
		#if 0
		struct commandAddTimer
		{
			CTimerd::CTimerEventTypes  eventType;
			CTimerd::CTimerEventRepeat eventRepeat;
			time_t                     alarmTime;
			time_t                     announceTime;
			time_t                     stopTime;			
			uint32_t                   repeatCount;
		};
		#endif
		struct commandGetTimer
		{
			int   eventID;
		};

		#if 0
		struct commandModifyTimer
		{
			int                        eventID;
			time_t                     announceTime;
			time_t                     alarmTime;
			time_t                     stopTime;
			CTimerd::CTimerEventRepeat eventRepeat;
			uint32_t                   repeatCount;
		};


		struct commandRemind
		{
			char message[REMINDER_MESSAGE_MAXLEN];
		};

		struct commandExecPlugin
		{
			char name[EXEC_PLUGIN_NAME_MAXLEN];
		};
		#endif
		struct commandRecordDir
		{
			char recDir[RECORD_DIR_MAXLEN];
		};

		#if 0
		struct commandSetAPid
		{
			int   eventID;
			unsigned char  apids;
		};

		struct commandRemoveTimer
		{
			int   eventID;
		};

		struct commandSetStandby
		{
			bool standby_on;
		};

		struct commandRecordingSafety
		{
			int pre;
			int post;
		};
		#endif

		struct generalInteger
		{
			int number;
		};

		#if 0
		struct responseAddTimer
		{
			int   eventID;
		};

		struct responseAvailable
		{
			bool available;
		};
		#endif
			
		struct responseGetSleeptimer
		{
			int   eventID;
		};

		#if 0
		struct responseStatus
		{
			bool status;
		};
		#endif
};

#endif /* __timerdmsg_h__ */


