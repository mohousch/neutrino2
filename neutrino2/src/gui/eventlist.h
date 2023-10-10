/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: eventlist.h 18.01.2019 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#ifndef __EVENTLIST_HPP__
#define __EVENTLIST_HPP__

#include <timerd/timerd.h>
#include <sectionsd/sectionsd.h>

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <system/settings.h>

#include <driver/color.h>
#include <gui/infoviewer.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


//
class EventList
{
	// Eventfinder start
        public:
                typedef enum
                {
                        SEARCH_EPG_NONE,
                        SEARCH_EPG_TITLE,
                        SEARCH_EPG_INFO1,
                        SEARCH_EPG_INFO2,
                        SEARCH_EPG_GENRE,
                        SEARCH_EPG_ALL
                }SEARCH_EPG;
		
                typedef enum
                {
                        SEARCH_LIST_NONE,
                        SEARCH_LIST_CHANNEL,
                        SEARCH_LIST_BOUQUET,
                        SEARCH_LIST_ALL
                }SEARCH_LIST;
		
		enum
		{
			SORT_DESCRIPTION,
			SORT_ID,
			SORT_DATETIME
		};
		
        private:
		CFrameBuffer	* frameBuffer;

		//
		CBox cFrameBox;
		CWidget* evlWidget;
		ClistBox* listBox;
		CMenuItem* item;
		//
		uint32_t sec_timer_id;
		unsigned int selected;
		void paint(t_channel_id channel_id = 0);
		void hide();

		//
		int m_search_epg_item;
		std::string m_search_keyword;
		int m_search_list;
		t_channel_id m_search_channel_id;
		t_bouquet_id m_search_bouquet_id;
		bool m_showChannel;
                int findEvents(void);

		//
        	CChannelEventList evtlist;
		CTimerd::TimerList timerlist;
		void readEvents(const t_channel_id channel_id);

		unsigned int current_event;
		int key;
		std::string name;
		int sort_mode;

		CTimerd::CTimerEventTypes isScheduled(t_channel_id channel_id, CChannelEvent * event, int * tID = NULL);

	public:
		EventList();
		~EventList();
		int exec(const t_channel_id channel_id, const std::string& channelname); // UTF-8
};

class CEventListHandler : public CMenuTarget
{
	public:
		int  exec(CMenuTarget* parent,  const std::string &/*actionKey*/);

};

//
class CEventFinderMenu : public CMenuTarget
{
        private:
                int * m_event;
                int * m_search_epg_item;
                std::string * m_search_keyword;
                int * m_search_list;
                std::string m_search_channelname;
                t_channel_id * m_search_channel_id;
                t_bouquet_id * m_search_bouquet_id;
		int showMenu(void);

        public:
                CEventFinderMenu(int * event, int * search_epg_item, std::string * search_keyword, int * search_list, t_channel_id * search_channel_id, t_bouquet_id * search_bouquet_id);
                
                int  exec( CMenuTarget* parent,  const std::string &actionKey);
};

#endif

