//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: timerlist.h 26112025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef __timerlist__
#define __timerlist__

#include <string>

#include <timerd/timerd.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <driver/gdi/framebuffer.h>

#include <gui/pluginlist.h>


class CTimerList : public CTarget
{
	private:
		CFrameBuffer* frameBuffer;

		//
		CWidget* timerlistWidget;
		ClistBox *listBox;
		CMenuItem *item, *m6, *m7, *m10;
		CBox cFrameBox;
		uint32_t sec_timer_id;
		unsigned int selected;
		////
		CTimerd::TimerList timerlist;             // List of timers		
		CTimerd::timerEvent timerNew;
		CTimerd::timerEvent *timer;
		int timerNew_standby_on;
		std::string m_weekdaysStr;
		//
		t_channel_id timerNew_chan_id;
		std::string timerNew_channel_name;
		//
		int timer_apids_dflt;
		int timer_apids_std;
		int timer_apids_ac3;
		int timer_apids_alt;
		//
		int skipEventID;
		CPluginChooser * plugin_chooser;
		////
		int showMainMenu();
		int showModifyTimerMenu();
		int showNewTimerMenu();
		//
		void updateEvents(void);

	public:
		CTimerList();
		~CTimerList();
		void hide();
		int  exec(CTarget* parent, const std::string & actionKey);

		//
		static const char * convertTimerType2String(const CTimerd::CTimerEventTypes type); // UTF-8
		static std::string convertTimerRepeat2String(const CTimerd::CTimerEventRepeat rep); // UTF-8
		static std::string convertChannelId2String(const t_channel_id id); // UTF-8
};

bool askUserOnTimerConflict(time_t announceTime, time_t stopTime);

#endif

