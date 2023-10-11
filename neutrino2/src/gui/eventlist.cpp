/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: eventlist.cpp 18.01.2019 mohousch Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

#include <global.h>

#include <gui/eventlist.h>
#include <gui/timerlist.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/hintbox.h>
#include <gui/bouquetlist.h>
#include <gui/widget/stringinput.h>

#include <gui/epgplus.h>

//
#include <zapit/zapit.h>

#include <timerd/timerd.h>

#include <algorithm>

#include <system/debug.h>
#include <system/tmdbparser.h>
#include <system/channellogo.h>

#include <gui/widget/infobox.h>


extern CBouquetList * bouquetList;
extern t_channel_id live_channel_id;
extern char recDir[255];			// defined in neutrino.cpp

// sort operators
bool sortById(const CChannelEvent& a, const CChannelEvent& b)
{
	return a.eventID < b.eventID ;
}

bool sortByDescription(const CChannelEvent& a, const CChannelEvent& b)
{
	if(a.description == b.description)
		return a.eventID < b.eventID;
	else
		return a.description < b.description ;
}

static bool sortByDateTime(const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

EventList::EventList()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = 0;
	current_event = 0;
	sort_mode = SORT_DESCRIPTION;

	m_search_list = SEARCH_LIST_NONE;
	m_search_epg_item = SEARCH_LIST_NONE;
	m_search_epg_item = SEARCH_EPG_TITLE;
	m_search_channel_id = 1;
	m_search_bouquet_id= 1;
	
	evtlist.clear();
	timerlist.clear();

	//
	evlWidget = NULL;
	listBox = NULL;
	item = NULL;
	sec_timer_id = 0;

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;			
}

EventList::~EventList()
{
	evtlist.clear();
	timerlist.clear();
	
	if (listBox)
	{
		delete listBox;
		listBox = NULL;
	}
	
	if (evlWidget)
	{
		delete evlWidget;
		evlWidget = NULL;
	}
}

void EventList::readEvents(const t_channel_id channel_id)
{
	evtlist.clear();
	
	CSectionsd::getInstance()->getEventsServiceKey(channel_id & 0xFFFFFFFFFFFFULL, evtlist);
	time_t azeit = time(NULL);

	CChannelEventList::iterator e;
	
	if ( evtlist.size() != 0 ) 
	{
		CEPGData epgData;
		
		// todo: what if there are more than one events in the Portal
		if (CSectionsd::getInstance()->getActualEPGServiceKey(channel_id & 0xFFFFFFFFFFFFULL, &epgData))
		{
			CSectionsd::LinkageDescriptorList linkedServices;

			if ( CSectionsd::getInstance()->getLinkageDescriptorsUniqueKey( epgData.eventID, linkedServices ) )
			{
				if ( linkedServices.size() > 1 )
				{
					CChannelEventList evtlist2; // stores the temporary eventlist of the subchannel channelid
					t_channel_id channel_id2;
				
					for (unsigned int i = 0; i < linkedServices.size(); i++)
					{
						channel_id2 = create_channel_id(
								linkedServices[i].serviceId,
								linkedServices[i].originalNetworkId,
								linkedServices[i].transportStreamId);
							
						// do not add parent events
						if (channel_id != channel_id2) 
						{
							evtlist2.clear();
							CSectionsd::getInstance()->getEventsServiceKey(channel_id2 & 0xFFFFFFFFFFFFULL, evtlist2);

							for (unsigned int loop = 0 ; loop < evtlist2.size(); loop++ )
							{
								//FIXME: bad ?evtlist2[loop].sub = true;
								evtlist.push_back(evtlist2[loop]);
							}
							evtlist2.clear();
						}
					}
				}
			}
		}
		
		// Houdini added for Private Premiere EPG, start sorted by start date/time
		sort(evtlist.begin(), evtlist.end(), sortByDateTime);
		
  		// Houdini: dirty workaround for RTL double events, remove them
  		CChannelEventList::iterator e2;
  		for ( e = evtlist.begin(); e != evtlist.end(); ++e )
  		{
  			e2 = e + 1;
  			if ( e2 != evtlist.end() && (e->startTime == e2->startTime)) 
			{
  				evtlist.erase(e2);
  			}
  		}
	}
	
	current_event = (unsigned int) - 1;
	for ( e = evtlist.begin(); e != evtlist.end(); ++e )
	{
		if ( e->startTime > azeit ) 
		{
			break;
		}
		current_event++;
	}

	if ( evtlist.size() == 0 )
	{
		CChannelEvent evt;

		evt.description = _("EPG is not available");
		evt.eventID = 0;
		evt.channelID = 0;
		evt.startTime = 0;
		evt.duration = 0;
		
		evtlist.push_back(evt);

	}
	
	if (current_event == (unsigned int) - 1)
		current_event = 0;
		
	selected = current_event;
}

int EventList::exec(const t_channel_id channel_id, const std::string& channelname) // UTF-8
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	bool in_search = 0;

	int res = CMenuTarget::RETURN_REPAINT;
	
	if(m_search_list == SEARCH_LIST_NONE) // init globals once only
	{
		m_search_epg_item = SEARCH_EPG_TITLE;
		m_search_list = SEARCH_LIST_CHANNEL;
		m_search_bouquet_id = bouquetList->getActiveBouquetNumber();
	}
	
	m_search_channel_id = channel_id;
	m_showChannel = false; // do not show the channel in normal mode, we just need it in search mode

	name = channelname;
	sort_mode = SORT_DESCRIPTION;

	//
	paint(channel_id);
	CFrameBuffer::getInstance()->blit();

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	int oldselected = selected;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

		if (msg == CRCInput::RC_up)
		{
			listBox->scrollLineUp();
		}
		else if (msg == CRCInput::RC_down)
		{
			listBox->scrollLineDown();
		}
		else if (msg == CRCInput::RC_page_up)
		{
			listBox->scrollPageUp();
		}
		else if (msg == CRCInput::RC_page_down)
		{
			listBox->scrollPageDown();
		}
		// sort
		else if (msg == CRCInput::RC_blue)
		{
			uint64_t selected_id = evtlist[selected].eventID;
			
			if(sort_mode == SORT_DESCRIPTION) // by description
			{
				sort_mode++;
				sort(evtlist.begin(), evtlist.end(), sortByDescription);
			}
			else// datetime
			{
				sort_mode = SORT_DESCRIPTION;
				sort(evtlist.begin(), evtlist.end(), sortByDateTime);
			}
			
			// find selected
			for ( selected = 0 ; selected < evtlist.size(); selected++ )
			{
				if ( evtlist[selected].eventID == selected_id )
					break;
			}

			hide();
			paint(channel_id);
		}
		// epg reload
		else if (msg == CRCInput::RC_setup)
		{
			sort_mode = SORT_DESCRIPTION;
			hide();
			paint(channel_id);			
		}
		// add record
		else if ( msg == CRCInput::RC_red )
		{
			selected = listBox->getSelected();

			// delete if scheduled
			int tID = -1;
			CTimerd::CTimerEventTypes etype = isScheduled(channel_id, &evtlist[selected], &tID);
			if(etype == CTimerd::TIMER_RECORD) 
			{
				CTimerd::getInstance()->removeTimerEvent(tID);

				paint(channel_id);
				continue;
			}
				
			if (recDir != NULL)
			{
				if (CTimerd::getInstance()->addRecordTimerEvent(channel_id,
						evtlist[selected].startTime,
						evtlist[selected].startTime + evtlist[selected].duration,
						evtlist[selected].eventID, evtlist[selected].startTime,
						evtlist[selected].startTime - (ANNOUNCETIME + 120),
						TIMERD_APIDS_CONF, true, recDir,false) == -1)
				{
					if(askUserOnTimerConflict(evtlist[selected].startTime - (ANNOUNCETIME + 120), evtlist[selected].startTime + evtlist[selected].duration))
					{
						CTimerd::getInstance()->addRecordTimerEvent(channel_id,
							evtlist[selected].startTime,
							evtlist[selected].startTime + evtlist[selected].duration,
							evtlist[selected].eventID, evtlist[selected].startTime,
							evtlist[selected].startTime - (ANNOUNCETIME + 120),
							TIMERD_APIDS_CONF, true, recDir,true);
									
						MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
					}
				} 
				else 
				{
					MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
				}
			}
				
			//
			paint(channel_id);					
		}
		// add remind
		else if ( msg == CRCInput::RC_yellow )		  
		{
			selected = listBox->getSelected();

			// delete if scheduled
			int tID = -1;
			CTimerd::CTimerEventTypes etype = isScheduled(channel_id, &evtlist[selected], &tID);
			
			if(etype == CTimerd::TIMER_ZAPTO) 
			{
				CTimerd::getInstance()->removeTimerEvent(tID);

				paint(channel_id);
				continue;
			}

			CTimerd::getInstance()->addZaptoTimerEvent(channel_id, 
					evtlist[selected].startTime,
					evtlist[selected].startTime - ANNOUNCETIME, 0,
					evtlist[selected].eventID, evtlist[selected].startTime, 0);
					
			MessageBox(_("Schedule Event"), _("The event is scheduled.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
			
			//
			paint(channel_id);
		}
		else if (msg == CRCInput::RC_timeout)
		{
			selected = oldselected;
			loop = false;
		}
		else if (msg == (neutrino_msg_t)g_settings.key_channelList_cancel) 
		{
			if(in_search) 
			{
				in_search = false;
				name = channelname;

				paint(channel_id);
			} 
			else 
			{
				selected = oldselected;
				loop = false;
			}
		}
		else if (msg == CRCInput::RC_epg)
		{
			hide();
			CEPGplusHandler eplus;
			eplus.exec(NULL, "");
			//
			paint(channel_id);
		}
		else if ( msg == CRCInput::RC_left )		  
		{
			loop = false;
		}
		else if ( msg == CRCInput::RC_right || msg == CRCInput::RC_ok || msg == CRCInput::RC_info)
		{
			selected = listBox->getSelected();

			if ( evtlist[selected].eventID != 0 )
			{
				hide();

				res = g_EpgData->show(channel_id, evtlist[selected].eventID, &evtlist[selected].startTime);
				
				if ( res == CMenuTarget::RETURN_EXIT_ALL )
				{
					loop = false;
				}
				else
				{
					g_RCInput->getMsg( &msg, &data, 0 );

					if ( ( msg != CRCInput::RC_red ) && ( msg != CRCInput::RC_timeout ) )
					{
						// RC_red schlucken
						g_RCInput->postMsg( msg, data );
					}

					paint(channel_id);
				}
			}
		}
		else if(msg == CRCInput::RC_0)
		{
			selected = listBox->getSelected();

			if ( evtlist[selected].eventID != 0 )
			{
				hide();
				
				//
				::getTMDBInfo(evtlist[selected].description.c_str());

				paint(channel_id);
			}	
		}
		else if ( msg == CRCInput::RC_green )
		{
			in_search = findEvents();
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);
		}
		else if (msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
		{
			g_RCInput->postMsg (msg, 0);
			res = CMenuTarget::RETURN_EXIT_ALL;
			loop = false;
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			evlWidget->refresh();
		} 
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = CMenuTarget::RETURN_EXIT_ALL;
			}
		}

		// blit
		frameBuffer->blit();	
	}

	hide();

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	return res;
}

void EventList::hide()
{
	evlWidget->hide();
	
	//
	if (listBox)
	{
		delete listBox;
		listBox = NULL;
	}
	
	if (evlWidget)
	{
		delete evlWidget;
		evlWidget = NULL;
	}
}

CTimerd::CTimerEventTypes EventList::isScheduled(t_channel_id channel_id, CChannelEvent * event, int * tID)
{
	CTimerd::TimerList::iterator timer = timerlist.begin();
	
	for(; timer != timerlist.end(); timer++) 
	{
		if(timer->channel_id == channel_id && (timer->eventType == CTimerd::TIMER_ZAPTO || timer->eventType == CTimerd::TIMER_RECORD)) 
		{
			if(timer->epgID == event->eventID) 
			{
				if(timer->epg_starttime == event->startTime) 
				{
					if(tID)
						*tID = timer->eventID;
						
					return timer->eventType;
				}
			}
		}
	}

	return (CTimerd::CTimerEventTypes) 0;
}

#define NUM_LIST_BUTTONS 5
struct button_label FootButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("record") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("search") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("schedule") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("sorting(Time)") },
	{ NEUTRINO_ICON_BUTTON_SETUP_SMALL, _("Refresh EPG") }
};

struct button_label HeadButtons[3] = 
{
	{ NEUTRINO_ICON_BUTTON_HELP, "" },
	{ NEUTRINO_ICON_BUTTON_EPG, "" },
	{ NEUTRINO_ICON_BUTTON_0, "" }
};

void EventList::paint(t_channel_id channel_id)
{
	dprintf(DEBUG_NORMAL, "EventList::paint\n");
	
	//
	readEvents(channel_id);
	
	// 
	timerlist.clear();
	CTimerd::getInstance()->getTimerList(timerlist);
	
	//
	if (listBox)
	{
		delete listBox;
		listBox = NULL;
	}
	
	if (evlWidget)
	{
		delete evlWidget;
		evlWidget = NULL;
	}
	
	//
	evlWidget = CNeutrinoApp::getInstance()->getWidget("eventlist");
	
	if (evlWidget)
	{
		listBox = (ClistBox*)evlWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		evlWidget = new CWidget(&cFrameBox);
		evlWidget->name = "eventlist";
		
		//
		listBox = new ClistBox(&cFrameBox);
		
		//
		listBox->enablePaintHead();
		listBox->setHeadLine(true, true);
		
		//
		listBox->enablePaintDate();
		listBox->setFootLine(true, true);
		
		//
		evlWidget->addCCItem(listBox);
	}

	listBox->clear();

	for (unsigned int count = 0; count < evtlist.size(); count++)
	{
		item = new CMenuForwarder(evtlist[count].description.c_str());

		//
		std::string datetime1_str, datetime2_str, duration_str;
		std::string icontype;
		icontype.clear();

		// option
		if ( evtlist[count].eventID != 0 )
		{
			char tmpstr[256];
			struct tm *tmStartZeit = localtime(&evtlist[count].startTime);

			strftime(tmpstr, sizeof(tmpstr), "%A", tmStartZeit );
			datetime1_str = _(tmpstr);

			strftime(tmpstr, sizeof(tmpstr), " %H:%M ", tmStartZeit );
			datetime1_str += tmpstr;

			strftime(tmpstr, sizeof(tmpstr), " %d.%m.%Y ", tmStartZeit );
			datetime1_str += tmpstr;

			if ( m_showChannel ) // show the channel if we made a event search only (which could be made through all channels ).
			{
				t_channel_id channel = evtlist[count].get_channel_id();
				datetime1_str += "      ";
				datetime1_str += CZapit::getInstance()->getChannelName(channel);
			}

			sprintf(tmpstr, "[%d min]", evtlist[count].duration / 60 );
			duration_str = tmpstr;
		}

		item->setOption(datetime1_str.c_str());

		int seit = ( evtlist[count].startTime - time(NULL) ) / 60;
		if ( (seit> 0) && (seit<100) && (duration_str.length()!=0) )
		{
			char beginnt[100];
			sprintf((char*) &beginnt, "in %d min ", seit);

			datetime2_str = beginnt;
		}

		datetime2_str += duration_str;

		item->setOptionInfo(datetime2_str.c_str());
		
		// icon
		CTimerd::CTimerEventTypes etype = isScheduled(channel_id, &evtlist[count]);
		icontype = etype == CTimerd::TIMER_ZAPTO ? NEUTRINO_ICON_BUTTON_YELLOW : etype == CTimerd::TIMER_RECORD ? NEUTRINO_ICON_BUTTON_RED : "";

		item->setIconName(icontype.c_str());
		item->set2lines();

		if(count == current_event)
			item->setMarked(true);

		listBox->addItem(item);
	}

	// head
	std::string logo;

	logo = CChannellogo::getInstance()->getLogoName(channel_id);

	listBox->setTitle(name.c_str(), g_settings.logos_show_logo? logo.c_str() : NULL);
	listBox->setHeadButtons(HeadButtons, 3);

	// foot
	listBox->enablePaintFoot();

	if(sort_mode == SORT_DESCRIPTION)
		FootButtons[3].localename = _("sorting(A..Z)");
	else
		FootButtons[3].localename = _("sorting(Time)");

	listBox->setFootButtons(FootButtons, NUM_LIST_BUTTONS);

	//
	listBox->setSelected(selected);
	
	//
	evlWidget->paint();
}

int CEventListHandler::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CEventListHandler::exec:\n");

	int res = CMenuTarget::RETURN_REPAINT;
	EventList* e;
	CChannelList* channelList;

	if (parent)
		parent->hide();

	e = new EventList;

	channelList = CNeutrinoApp::getInstance()->channelList;

	res = e->exec(channelList->getActiveChannel_ChannelID(), channelList->getActiveChannelName());

	delete e;

	return res;
}

int EventList::findEvents(void)
{
	int res = 0;
	int event = 0;
	t_channel_id channel_id;  //g_Zapit->getCurrentServiceID()
	
	CEventFinderMenu menu(&event, &m_search_epg_item, &m_search_keyword, &m_search_list, &m_search_channel_id, &m_search_bouquet_id);
	hide();
	menu.exec(NULL, "");
	
	if(event == 1)
	{
		res = 1;
		m_showChannel = true;   // force the event list to paint the channel name
		
		if (evtlist.size())
			evtlist.clear();
		
		if(m_search_list == SEARCH_LIST_CHANNEL)
		{
			CSectionsd::getInstance()->getEventsServiceKey(m_search_channel_id & 0xFFFFFFFFFFFFULL, evtlist, m_search_epg_item, m_search_keyword);
		}
		else if(m_search_list == SEARCH_LIST_BOUQUET)
		{
			int channel_nr = bouquetList->Bouquets[m_search_bouquet_id]->channelList->getSize();
			for(int channel = 0; channel < channel_nr; channel++)
			{
				channel_id = bouquetList->Bouquets[m_search_bouquet_id]->channelList->getChannelFromIndex(channel)->channel_id;
				
				CSectionsd::getInstance()->getEventsServiceKey(channel_id & 0xFFFFFFFFFFFFULL, evtlist, m_search_epg_item, m_search_keyword);
			}
		}
		else if(m_search_list == SEARCH_LIST_ALL)
		{
			CHintBox box(_("Event list"), _("Search for keyword in EPG..."));
			box.paint();
			int bouquet_nr = bouquetList->Bouquets.size();
			
			for(int bouquet = 0; bouquet < bouquet_nr; bouquet++)
			{
				int channel_nr = bouquetList->Bouquets[bouquet]->channelList->getSize();
				for(int channel = 0; channel < channel_nr; channel++)
				{
					channel_id = bouquetList->Bouquets[bouquet]->channelList->getChannelFromIndex(channel)->channel_id;
					
					CSectionsd::getInstance()->getEventsServiceKey(channel_id & 0xFFFFFFFFFFFFULL, evtlist, m_search_epg_item, m_search_keyword);
				}
			}
			box.hide();
		}
		
		sort(evtlist.begin(), evtlist.end(), sortByDateTime);
		current_event = (unsigned int) -1;
		time_t azeit = time(NULL);
		
		CChannelEventList::iterator e;
		for ( e = evtlist.begin(); e != evtlist.end(); ++e )
		{
			if ( e->startTime > azeit ) 
			{
				break;
			}
			current_event++;
		}
		
		if(evtlist.empty())
		{
			if ( evtlist.size() == 0 )
			{
				CChannelEvent evt;

				evt.description = _("no epg found");
				evt.eventID = 0;
				evtlist.push_back(evt);
			}
		}  
		          
		if (current_event == (unsigned int) - 1)
			current_event = 0;
			
		selected = current_event;
		
		name = _("Search");
		name += ": '";
		name += m_search_keyword;
		name += "'";
	}
	
	paint();
	
	return(res);
}
  
#define SEARCH_LIST_OPTION_COUNT 3
const keyval SEARCH_LIST_OPTIONS[SEARCH_LIST_OPTION_COUNT] =
{
	{ EventList::SEARCH_LIST_CHANNEL, _("Channel") },
	{ EventList::SEARCH_LIST_BOUQUET, _("Bouquets") },
	{ EventList::SEARCH_LIST_ALL, _("Channellist") }
};

#define SEARCH_EPG_OPTION_COUNT 3
const keyval SEARCH_EPG_OPTIONS[SEARCH_EPG_OPTION_COUNT] =
{
	{ EventList::SEARCH_EPG_TITLE, _("EPG Title") },
	{ EventList::SEARCH_EPG_INFO1, _("EPG Info1") },
	{ EventList::SEARCH_EPG_INFO2, _("EPG Info2") }
};

CEventFinderMenu::CEventFinderMenu(int * event, int * search_epg_item, std::string * search_keyword, int * search_list, t_channel_id * search_channel_id, t_bouquet_id * search_bouquet_id)
{
	m_event = event;
	m_search_epg_item = search_epg_item;
	m_search_keyword = search_keyword;
	m_search_list = search_list;
	m_search_channel_id = search_channel_id;
	m_search_bouquet_id = search_bouquet_id;
}

int CEventFinderMenu::exec(CMenuTarget * parent, const std::string &actionKey)
{
	int res = CMenuTarget::RETURN_REPAINT;
	
	if(actionKey == "")
	{
		if(parent != NULL)
			parent->hide();

		showMenu();
	}
	else if(actionKey == "1")
	{
		*m_event = true;
		res = CMenuTarget::RETURN_EXIT_ALL;
	}	
	else if(actionKey == "2")
	{
		//printf("2\n");
		
		/*
		if(*m_search_list == EventList::SEARCH_LIST_CHANNEL)
		{
			mf[1]->setActive(true);
			m_search_channelname = CZapit::getInstance()->getChannelName(*m_search_channel_id);;
		}
		else if(*m_search_list == EventList::SEARCH_LIST_BOUQUET)
		{
			mf[1]->setActive(true);
			m_search_channelname = bouquetList->Bouquets[*m_search_bouquet_id]->channelList->getName();
		}
		else if(*m_search_list == EventList::SEARCH_LIST_ALL)
		{
			mf[1]->setActive(false);
			m_search_channelname = "";
		}
		*/
	}	
	else if(actionKey == "3")
	{
		// get channel id / bouquet id
		if(*m_search_list == EventList::SEARCH_LIST_CHANNEL)
		{
			int nNewChannel;
			int nNewBouquet;
			nNewBouquet = bouquetList->show();
			
			//printf("new_bouquet_id %d\n",nNewBouquet);
			
			if (nNewBouquet > -1)
			{
				nNewChannel = bouquetList->Bouquets[nNewBouquet]->channelList->show();
				//printf("nNewChannel %d\n",nNewChannel);
				
				if (nNewChannel > -1)
				{
					*m_search_bouquet_id = nNewBouquet;
					*m_search_channel_id = bouquetList->Bouquets[nNewBouquet]->channelList->getActiveChannel_ChannelID();
					m_search_channelname = CZapit::getInstance()->getChannelName(*m_search_channel_id);
				}
			}
		}
		else if(*m_search_list == EventList::SEARCH_LIST_BOUQUET)
		{
			int nNewBouquet;
			nNewBouquet = bouquetList->show();
			//printf("new_bouquet_id %d\n",nNewBouquet);
			if (nNewBouquet > -1)
			{
				*m_search_bouquet_id = nNewBouquet;
				m_search_channelname = bouquetList->Bouquets[nNewBouquet]->channelList->getName();
			}
		}
	}	
	else if(actionKey == "4")
	{
		//printf("4\n");
	}	
	
	return res;
}

int CEventFinderMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CEventFinderMenu::showMenu:\n");

	int res = CMenuTarget::RETURN_REPAINT;
	*m_event = false;
	
	if(*m_search_list == EventList::SEARCH_LIST_CHANNEL)
	{
		m_search_channelname = CZapit::getInstance()->getChannelName(*m_search_channel_id);
	}
	else if(*m_search_list == EventList::SEARCH_LIST_BOUQUET)
	{
		m_search_channelname = bouquetList->Bouquets[*m_search_bouquet_id]->channelList->getName();
	}
	else if(*m_search_list == EventList::SEARCH_LIST_ALL)
	{
		m_search_channelname == "";
	}
	
	CStringInputSMS stringInput(_("Keyword"), m_search_keyword->c_str());
	CMenuForwarder * mf2 = new CMenuForwarder(_("Keyword"), true, m_search_keyword->c_str(), &stringInput);
	
	CMenuOptionChooser * mo0 = new CMenuOptionChooser(_("Search within"), m_search_list, SEARCH_LIST_OPTIONS, SEARCH_LIST_OPTION_COUNT, true);
	CMenuForwarder * mf1 = new CMenuForwarder("", *m_search_list != EventList::SEARCH_LIST_ALL, m_search_channelname.c_str(), this, "3");
	CMenuOptionChooser * mo1 = new CMenuOptionChooser(_("Search in EPG"), m_search_epg_item, SEARCH_EPG_OPTIONS, SEARCH_EPG_OPTION_COUNT, true);
	CMenuForwarder * mf0 = new CMenuForwarder(_("Start Search"), true, NULL, this, "1");
	
	//
	CWidget* widget = NULL;
	ClistBox* searchMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("epgsearch");
	
	if (widget)
	{
		searchMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "epgsearch";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		searchMenu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		searchMenu->setWidgetMode(ClistBox::MODE_SETUP);
		searchMenu->enableShrinkMenu();
		
		searchMenu->enablePaintHead();
		searchMenu->setTitle(_("Search in EPG"), NEUTRINO_ICON_FEATURES);

		searchMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		searchMenu->setFootButtons(&btn);
		
		//
		widget->addCCItem(searchMenu);
	}

        searchMenu->addItem(mf2);
        searchMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
        searchMenu->addItem(mo0);
        searchMenu->addItem(mf1);
        searchMenu->addItem(mo1);
        searchMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
        searchMenu->addItem(mf0);
	
	res = widget->exec(NULL, "");
	
	delete searchMenu;
	searchMenu = NULL;
	delete widget;
	widget = NULL;	
	
	return(res);
}

