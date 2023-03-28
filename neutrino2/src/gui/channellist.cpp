/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: channellist.cpp 16.11.2020 mohousch Exp $

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

#include <gui/channellist.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <system/settings.h>
#include <system/lastchannel.h>
#include <gui/filebrowser.h>

#include <gui/bouquetlist.h>
#include <daemonc/remotecontrol.h>
#include <driver/vcrcontrol.h>

//
#include <zapit/bouquets.h>
#include <zapit/satconfig.h>
#include <zapit/getservices.h>
#include <zapit/frontend_c.h>

#include <gui/epgplus.h>
#include <system/debug.h>

#include <video_cs.h>
#include <misc_setup.h>


extern CRemoteControl * g_RemoteControl; 		// neutrino.cpp	

extern CBouquetList * bouquetList;      		// neutrino.cpp
extern CBouquetList   * TVbouquetList;
extern CBouquetList   * TVsatList;
extern CBouquetList   * TVfavList;
extern CBouquetList   * TVallList;
extern CBouquetList   * RADIObouquetList;
extern CBouquetList   * RADIOsatList;
extern CBouquetList   * RADIOfavList;
extern CBouquetList   * RADIOallList;
extern CBouquetList   * WEBTVbouquetList;
extern CBouquetList   * WEBTVallList;
extern CBouquetList   * WEBTVfavList;

extern t_channel_id rec_channel_id;
extern t_channel_id live_channel_id;
bool pip_selected = false;
extern bool autoshift;
int info_height = 0;
bool new_mode_active = 0;
extern int FrontendCount;				// defined in zapit.cpp
extern CBouquetManager * g_bouquetManager;
extern cVideo * videoDecoder;
extern CZapitChannel * live_channel;
extern int old_b_id;

#define NUM_LIST_BUTTONS 4
struct button_label CChannelListButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Event list")},
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Next")},
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Bouquets")},
	{ NEUTRINO_ICON_BUTTON_BLUE, _("EPG Plus")},
};

#define HEAD_BUTTONS_COUNT	3
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " },
	{ NEUTRINO_ICON_BUTTON_MUTE_ZAP_INACTIVE, " " }
};

const struct button_label HeadNewModeButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " },
	{ NEUTRINO_ICON_BUTTON_MUTE_ZAP_ACTIVE, " " }
};

CChannelList::CChannelList(const char * const Name, bool _historyMode, bool _vlist)
{
	frameBuffer = CFrameBuffer::getInstance();

	name = Name;
	selected = 0;
	tuned = 0xfffffff;
	zapProtection = NULL;
	this->historyMode = _historyMode;
	vlist = _vlist;

	displayNext = false;
	
	events.clear();

	//
	chWidget = NULL;
	
	listBox = NULL;
	item = NULL;
	
	window = NULL;
	
	head = NULL;
	foot = NULL;
	
	vline = NULL;
	hline = NULL;
	
	sec_timer_id = 0;

	// widget screen	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 20;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 20;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CChannelList::~CChannelList()
{
	chanlist.clear();
	events.clear();
}

void CChannelList::ClearList(void)
{
	chanlist.clear();
	chanlist.resize(1);
}

void CChannelList::setSize(int newsize)
{
	chanlist.reserve(newsize);
}

void CChannelList::addChannel(CZapitChannel * channel, int num)
{
	if(num)
		channel->number = num;
	
	chanlist.push_back(channel);
}

void CChannelList::updateEvents(void)
{
	dprintf(DEBUG_NORMAL, "CChannelList::updateEvents\n");

	//CChannelEventList events;
	
	events.clear();

	if (displayNext) 
	{
		if (chanlist.size()) 
		{
			time_t atime = time(NULL);
			unsigned int count;
			
			for (count = 0; count < chanlist.size(); count++)
			{		
				events.clear();

				CSectionsd::getInstance()->getEventsServiceKey(chanlist[count]->channel_id & 0xFFFFFFFFFFFFULL, events);
				chanlist[count]->nextEvent.startTime = (long)0x7fffffff;
				
				for ( CChannelEventList::iterator e = events.begin(); e != events.end(); ++e ) 
				{
					if (((long)(e->startTime) > atime) && ((e->startTime) < (long)(chanlist[count]->nextEvent.startTime)))
					{
						chanlist[count]->nextEvent= *e;
					
						break;
					}
				}
			}
		}
	} 
	else 
	{
		t_channel_id *p_requested_channels = NULL;
		int size_requested_channels = 0;

		if (chanlist.size()) 
		{
			size_requested_channels = chanlist.size()*sizeof(t_channel_id);
			p_requested_channels = (t_channel_id*)malloc(size_requested_channels);
			
			for (uint32_t count = 0; count < chanlist.size(); count++)
			{
				p_requested_channels[count] = chanlist[count]->channel_id & 0xFFFFFFFFFFFFULL;
			}

			CChannelEventList pevents;

			pevents.clear();
			
			CSectionsd::getInstance()->getChannelEvents(pevents, (CNeutrinoApp::getInstance()->getMode()) != NeutrinoMessages::mode_radio, p_requested_channels, size_requested_channels);
			
			for (uint32_t count = 0; count < chanlist.size(); count++) 
			{
				for ( CChannelEventList::iterator e = pevents.begin(); e != pevents.end(); ++e )
				{
					if ((chanlist[count]->channel_id & 0xFFFFFFFFFFFFULL) == (e->channelID & 0xFFFFFFFFFFFFULL))//FIXME: get_channel_id()
					{
						chanlist[count]->currentEvent= *e;

						break;
					}
				}
			}

			if (p_requested_channels != NULL) 
				free(p_requested_channels);
		}
	}
}

struct CmpChannelBySat: public binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
        static bool comparetolower(const char a, const char b)
        {
		    return tolower(a) < tolower(b);
        };

        bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
        {
		    if(c1->getSatellitePosition() == c2->getSatellitePosition())
			    return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
		    else
			    return c1->getSatellitePosition() < c2->getSatellitePosition();
;
	};
};

struct CmpChannelByFreq: public binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
        static bool comparetolower(const char a, const char b)
        {
                return tolower(a) < tolower(b);
        };

        bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
        {
		    if(c1->getFreqId() == c2->getFreqId())
			    return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
		    else
			    return c1->getFreqId() < c2->getFreqId();
;
	};
};

void CChannelList::SortAlpha(void)
{
	sort(chanlist.begin(), chanlist.end(), CmpChannelByChName()); //FIXME:
}

void CChannelList::SortSat(void)
{
	sort(chanlist.begin(), chanlist.end(), CmpChannelBySat());
}

CZapitChannel * CChannelList::getChannel(int number)
{
	for (uint32_t i = 0; i< chanlist.size();i++) 
	{
		if (chanlist[i]->number == number)
			return chanlist[i];
	}
	
	return(NULL);
}

CZapitChannel * CChannelList::getChannel(t_channel_id channel_id)
{
	for (uint32_t i = 0; i< chanlist.size();i++) 
	{
		if (chanlist[i]->channel_id == channel_id)
			return chanlist[i];
	}
	
	return(NULL);
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->number;
}

static const std::string empty_string;

const std::string & CChannelList::getActiveChannelName(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->name;
	else
		return empty_string;
}

t_satellite_position CChannelList::getActiveSatellitePosition(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->getSatellitePosition();
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_ChannelID(void) const
{
	if (selected < chanlist.size()) 
	{
		dprintf(DEBUG_NORMAL, "CChannelList::getActiveChannel_ChannelID:%llx\n", chanlist[selected]->channel_id);
		
		return chanlist[selected]->channel_id;
	} 
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_EPGID(void) const
{
	if (selected < chanlist.size()) 
	{
		printf("CChannelList::getActiveChannel_EPGID:%llx\n", chanlist[selected]->epgid);
		
		return chanlist[selected]->epgid;
	} 
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_LogoID(void) const
{
	if (selected < chanlist.size()) 
	{
		printf("CChannelList::getActiveChannel_LogoID:%llx\n", chanlist[selected]->logoid);
		
		return chanlist[selected]->logoid;
	} 
	else
		return 0;
}

int CChannelList::getActiveChannelNumber(void) const
{
	return (selected + 1);
}

int CChannelList::doChannelMenu(void)
{
	int i = 0;
	int select = -1;
	static int old_selected = 0;
	signed int bouquet_id, old_bouquet_id, new_bouquet_id;
	int result;
	t_channel_id channel_id;
	
	if( !bouquetList )
		return 0;
	
	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("channellistedit");
	
	if (widget)
	{
		menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		menu->setWidgetMode(MODE_MENU);
		menu->enableShrinkMenu();
		
		menu->enablePaintHead();
		menu->setTitle(_("Edit"), NEUTRINO_ICON_SETTINGS);

		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		menu->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(menu);
	}
	
	widget->enableSaveScreen();
	menu->clearItems();


	menu->addItem(new ClistBoxItem(_("delete")), old_selected == i++);
	menu->addItem(new ClistBoxItem(_("Move")), old_selected == i++);
	menu->addItem(new ClistBoxItem(_("Add to Bouquets")), old_selected == i++);
	menu->addItem(new ClistBoxItem(_("add channel to my favorites")), old_selected == i++);

	widget->exec(NULL, "");
	select = menu->getSelected();

	if(select >= 0) 
	{
		hide();
		
		old_selected = select;
		channel_id = chanlist[selected]->channel_id;
		
		switch(select) 
		{
			case 0: //delete
				result = MessageBox(_("Delete"), "Delete channel from bouquet?", mbrNo, mbYes | mbNo );

				if(result == mbrYes) 
				{
					bouquet_id = bouquetList->getActiveBouquetNumber();
					bouquet_id = g_bouquetManager->existsBouquet(bouquetList->Bouquets[bouquet_id]->channelList->getName());

					if (bouquet_id == -1)
						return 0;
					
					if(g_bouquetManager->existsChannelInBouquet(bouquet_id, channel_id)) 
					{
						g_bouquetManager->Bouquets[bouquet_id]->removeService(channel_id);
						return 1;
					}
				}
				break;
				
			case 1: // move
				old_bouquet_id = bouquetList->getActiveBouquetNumber();
				old_bouquet_id = g_bouquetManager->existsBouquet(bouquetList->Bouquets[old_bouquet_id]->channelList->getName());

				do {
					new_bouquet_id = bouquetList->exec(false, false);
				} while(new_bouquet_id == -3);

				hide();
				if(new_bouquet_id < 0)
					return 0;
				new_bouquet_id = g_bouquetManager->existsBouquet(bouquetList->Bouquets[new_bouquet_id]->channelList->getName());
				if ((new_bouquet_id == -1) || (new_bouquet_id == old_bouquet_id))
					return 0;

				if(!g_bouquetManager->existsChannelInBouquet(new_bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(new_bouquet_id, channel_id);
				}
				
				if(g_bouquetManager->existsChannelInBouquet(old_bouquet_id, channel_id)) 
				{
					g_bouquetManager->Bouquets[old_bouquet_id]->removeService(channel_id);
				}
				return 1;

				break;
				
			case 2: // add to
				do {
					bouquet_id = bouquetList->exec(false, false);
				} while(bouquet_id == -3);
				
				hide();
				
				if(bouquet_id < 0)
					return 0;
				
				bouquet_id = g_bouquetManager->existsBouquet(bouquetList->Bouquets[bouquet_id]->channelList->getName());
				if (bouquet_id == -1)
					return 0;
				
				if(!g_bouquetManager->existsChannelInBouquet(bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(bouquet_id, channel_id);
					return 1;
				}
				break;
				
			case 3: // add to my favorites
				bouquet_id = g_bouquetManager->existsBouquet("Favorites");

				if(bouquet_id == -1) 
				{
					g_bouquetManager->addBouquet("Favorites", true);
					bouquet_id = g_bouquetManager->existsBouquet("Favorites");
				}
				
				if(!g_bouquetManager->existsChannelInBouquet(bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(bouquet_id, channel_id);
					return 1;
				}
				
				break;
				
			default:
				break;
		}
	}
	
	return 0;
}

int CChannelList::exec(bool zap)
{
	dprintf(DEBUG_NORMAL, "CChannelList::exec: zap:%s\n", zap? "yes" : "no");

	displayNext = false; // always start with current events
	
	// show list
	int nNewChannel = show(zap);

	dprintf(DEBUG_NORMAL, "CChannelList::exec: chanlist.size:%d nNewChannel:%d\n", (int)chanlist.size(), nNewChannel);
	
	// zapto
	if(zap)
	{
		if ( nNewChannel > -1 && nNewChannel < (int) chanlist.size()) 
			CNeutrinoApp::getInstance()->channelList->zapTo(getKey(nNewChannel) - 1);
	}

	return nNewChannel;
}

#define CHANNEL_SMSKEY_TIMEOUT 800
//return: >= 0 to zap, -1 on cancel, -3 on list mode change, -4 list edited, -2 zap but no restore old list/chan
int CChannelList::show(bool zap, bool customMode)
{
	dprintf(DEBUG_NORMAL, "CChannelList::show: zap:%s\n", zap? "yes" : "no");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	bool actzap = 0;
	int res = -1;

	new_mode_active = 0;

	// display channame in vfd	
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8 );	

	displayNext = false;
	
	// update events
	updateEvents();
	
	//
	chWidget = CNeutrinoApp::getInstance()->getWidget("channellist");
	
	if (chWidget)
	{
		listBox = (ClistBox*)chWidget->getWidgetItem(WIDGETITEM_LISTBOX);
		head = (CHeaders*)chWidget->getWidgetItem(WIDGETITEM_HEAD);
		foot = (CFooters*)chWidget->getWidgetItem(WIDGETITEM_FOOT);
		window = (CWindow*)chWidget->getWidgetItem(WIDGETITEM_WINDOW);
		vline = (CCVline*)chWidget->getCCItem(CC_VLINE);
		hline = (CCHline*)chWidget->getCCItem(CC_HLINE);
	}
	else
	{
		chWidget = new CWidget(&cFrameBox);
		chWidget->name = "channellist";
		chWidget->paintMainFrame(true);
		
		// listBox
		listBox = new ClistBox(chWidget->getWindowsPos().iX, chWidget->getWindowsPos().iY + 50, (chWidget->getWindowsPos().iWidth/3)*2, chWidget->getWindowsPos().iHeight - 100);
		listBox->paintMainFrame(false);
		
		//
		head = new CHeaders(chWidget->getWindowsPos().iX, chWidget->getWindowsPos().iY, chWidget->getWindowsPos().iWidth, 50);
		head->setTitle(name.c_str());
		head->enablePaintDate();
		
		// foot
		foot = new CFooters(chWidget->getWindowsPos().iX, chWidget->getWindowsPos().iY + chWidget->getWindowsPos().iHeight - 50, chWidget->getWindowsPos().iWidth, 50);
		
		//
		window = new CWindow(chWidget->getWindowsPos().iX + (chWidget->getWindowsPos().iWidth/3)*2, chWidget->getWindowsPos().iY + 50, chWidget->getWindowsPos().iWidth/3, chWidget->getWindowsPos().iHeight - 100);
		
		// vline
		vline = new CCVline(chWidget->getWindowsPos().iX + (chWidget->getWindowsPos().iWidth/3)*2, chWidget->getWindowsPos().iY + 60, 2, chWidget->getWindowsPos().iHeight - 120);
		vline->setGradient(3);
			
		// hline
		hline = new CCHline(chWidget->getWindowsPos().iX + (chWidget->getWindowsPos().iWidth/3)*2 + 10, chWidget->getWindowsPos().iY + 50 + (chWidget->getWindowsPos().iHeight - 100)/2, chWidget->getWindowsPos().iWidth/3 - 20, 2);
		hline->setGradient(3);
		
		chWidget->addWidgetItem(listBox);
		chWidget->addWidgetItem(head);
		chWidget->addWidgetItem(foot);
		chWidget->addWidgetItem(window);
		
		//	
		chWidget->addCCItem(vline);
		chWidget->addCCItem(hline);
	}
	
	// wionTop
	winTopBox.iX = window->getWindowsPos().iX + 2;
	winTopBox.iY = window->getWindowsPos().iY + 2;
	winTopBox.iWidth = window->getWindowsPos().iWidth - 4;
	winTopBox.iHeight = window->getWindowsPos().iHeight/2 - 4;
	
	// winBottom
	winBottomBox.iX = window->getWindowsPos().iX + 2;
	winBottomBox.iY = window->getWindowsPos().iY + window->getWindowsPos().iHeight/2 + 2;
	winBottomBox.iWidth = window->getWindowsPos().iWidth - 4;
	winBottomBox.iHeight = window->getWindowsPos().iHeight/2 - 4;
	
	// 
	paint();
	
	CFrameBuffer::getInstance()->blit();

	int oldselected = selected;
	int zapOnExit = false;
	bool bShowBouquetList = false;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	// loop control
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		
		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if ( ( msg == RC_timeout ) || ( msg == (neutrino_msg_t)g_settings.key_channelList_cancel) ) 
		{
			if(!actzap)
				selected = oldselected;
			
			loop = false;
			res = -1;
		}
		else if ((msg == RC_red) || (msg == RC_epg)) // epg
		{
			selected = listBox->getSelected();

			hide();

			if ( g_EventList->exec(chanlist[selected]->epgid, chanlist[selected]->name) == RETURN_EXIT_ALL) 
			{
				res = -2;
				loop = false;
			}

			paint();
		}
		else if ( msg == RC_yellow && ( bouquetList != NULL ) ) //bouquets
		{
			//FIXME: show bqt list
			bShowBouquetList = true;

			loop = false;
		}
		else if( msg == RC_blue ) //epgplus
		{
			selected = listBox->getSelected();

			hide();
			
			CEPGplusHandler eplus;
			eplus.exec(NULL, "");

			paint();
		}
		else if ( msg == RC_sat || msg == RC_favorites)
		{
			g_RCInput->postMsg (msg, 0);
			loop = false;
			res = -1;
		}
		else if ( msg == RC_setup ) 
		{
			selected = listBox->getSelected();
			
			if (!IS_WEBTV(chanlist[selected]->channel_id))
			{
				// chan list setup (add/move)
				old_b_id = bouquetList->getActiveBouquetNumber();
				int ret = doChannelMenu();
		
				if(ret) 
				{
					res = -4;
					loop = false;
				} 
				else 
				{
					old_b_id = -1;

					paint();
				}
			}
		}
		else if (msg == (neutrino_msg_t) g_settings.key_list_start) 
		{
			selected = 0;

			paint();
			
			if(new_mode_active) 
			{ 
				actzap = true; 
				zapTo(selected); 
			}
		}
		else if (msg == (neutrino_msg_t) g_settings.key_list_end) 
		{
			selected = chanlist.size() - 1;

			paint();
			
			if(new_mode_active) 
			{ 
				actzap = true; 
				zapTo(selected); 
			}
		}
                else if (msg == RC_up)
                {
			listBox->scrollLineUp();
			
			selected = listBox->getSelected();
				
			//
			paintCurrentNextEvent(selected);

			if(new_mode_active) 
			{
				actzap = true; 
				zapTo(selected); 
			}
                }
		else if ( msg == RC_page_up || (int) msg == g_settings.key_channelList_pageup)
                {
			listBox->scrollPageUp();
			
			selected = listBox->getSelected();
				
			//
			paintCurrentNextEvent(selected);

			if(new_mode_active) 
			{
				actzap = true; 
				zapTo(selected); 
			}
                }
                else if (msg == RC_down)
                {
			listBox->scrollLineDown();
			
			selected = listBox->getSelected();
				
			//
			paintCurrentNextEvent(selected);

			if(new_mode_active) 
			{ 
				actzap = true; 
				zapTo(selected); 
			}
                }
		else if (msg == RC_page_down || (int) msg == g_settings.key_channelList_pagedown)
                {
			listBox->scrollPageDown();
			
			selected = listBox->getSelected();
				
			//
			paintCurrentNextEvent(selected);

			if(new_mode_active) 
			{ 
				actzap = true; 
				zapTo(selected); 
			}
                }
		else if ((msg == (neutrino_msg_t)g_settings.key_bouquet_up) && (bouquetList != NULL)) 
		{
			if (bouquetList->Bouquets.size() > 0) 
			{
				bool found = true;
				uint32_t nNext = (bouquetList->getActiveBouquetNumber()+1) % bouquetList->Bouquets.size();
				if(bouquetList->Bouquets[nNext]->channelList->getSize() <= 0) 
				{
					found = false;
					nNext = nNext < bouquetList->Bouquets.size() - 1 ? nNext + 1 : 0;
					for(uint32_t i = nNext; i < bouquetList->Bouquets.size(); i++) 
					{
						if(bouquetList->Bouquets[i]->channelList->getSize() > 0) 
						{
							found = true;
							nNext = i;
							break;
						}
					}
				}
				
				if(found) 
				{
					bouquetList->activateBouquet(nNext, false);
					res = bouquetList->showChannelList();
					loop = false;
				}
			}
		}
		else if ((msg == (neutrino_msg_t)g_settings.key_bouquet_down) && (bouquetList != NULL)) 
		{
			if (bouquetList->Bouquets.size() > 0) 
			{
				bool found = true;
				int nNext = (bouquetList->getActiveBouquetNumber()+bouquetList->Bouquets.size()-1) % bouquetList->Bouquets.size();
				if(bouquetList->Bouquets[nNext]->channelList->getSize() <= 0) 
				{
					found = false;
					nNext = nNext > 0 ? nNext-1 : bouquetList->Bouquets.size()-1;
					
					for(int i = nNext; i > 0; i--) 
					{
						if(bouquetList->Bouquets[i]->channelList->getSize() > 0) 
						{
							found = true;
							nNext = i;
							break;
						}
					}
				}
				
				if(found) 
				{
					bouquetList->activateBouquet(nNext, false);
					res = bouquetList->showChannelList();
					loop = false;
				}
			}
		}
		else if ( msg == RC_ok ) 
		{
			selected = listBox->getSelected();
	  
			zapOnExit = true;
			
			loop = false;
		}
		else if ( msg == RC_spkr ) 
		{
			new_mode_active = (new_mode_active ? 0 : 1);
			
			paint();
		}
		else if (CRCInput::isNumeric(msg) && this->historyMode) 
		{ 
			if (this->historyMode) 
			{ 
				selected = CRCInput::getNumericValue(msg);
				zapOnExit = true;
				loop = false;
    			}
		}
		else if(CRCInput::isNumeric(msg)) 
		{
			//pushback key if...
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop = false;
		}
		else if ( msg == RC_green ) //next
		{
			selected = listBox->getSelected();

			displayNext = !displayNext;

			updateEvents();

			paint();
		} 
		else if ( (msg == RC_info) )
		{
			selected = listBox->getSelected();

			hide();
			g_EpgData->show(chanlist[selected]->epgid); 

			paint();
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (head) head->refresh();
		} 
		else 
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
			{
				loop = false;
				res = - 2;
			}
		}
			
		frameBuffer->blit();	
	}
	
	hide();
	
	// bouquets mode
	if (bShowBouquetList)
	{
		res = bouquetList->exec(true, zap, customMode);
	}
	
	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	
	new_mode_active = 0;

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	if(NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode())
		return -1;

	if(zapOnExit)
		res = selected;

	dprintf(DEBUG_NORMAL, "CChannelList::show res %d\n", res);
			
	return(res);
}

void CChannelList::hide()
{
	dprintf(DEBUG_NORMAL, "CChannelList::hide\n");

	
	if (chWidget) 
		chWidget->hide();
	else
		CFrameBuffer::getInstance()->clearFrameBuffer();
		
	frameBuffer->blit();
}

bool CChannelList::showInfo(int pos, int epgpos)
{
	if((pos >= (signed int) chanlist.size()) || (pos < 0))
		return false;

	CZapitChannel * chan = chanlist[pos];
	
	// channel infobar
	g_InfoViewer->showTitle(pos + 1, chan->name, chan->getSatellitePosition(), chan->channel_id, true, epgpos); // UTF-8
	
	return true;
}

int CChannelList::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS) 
	{
		// 0x100 als FSK-Status zeigt an, dass (noch) kein EPG zu einem Kanal der NICHT angezeigt
		// werden sollte (vorgesperrt) da ist
		// oder das bouquet des Kanals ist vorgesperrt

		if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = data;
			else 
			{
				// require password if either
				// CHANGETOLOCK mode and channel/bouquet is pre locked (0x100)
				// ONSIGNAL mode and fsk(data) is beyond configured value
				// if programm has already been unlocked, dont require pin
				if ((data >= (neutrino_msg_data_t)g_settings.parentallock_lockage) &&
					 ((chanlist[selected]->last_unlocked_EPGid != g_RemoteControl->current_EPGid) || (g_RemoteControl->current_EPGid == 0)) &&
					 ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED) || (data >= 0x100)))
				{
					g_RemoteControl->stopvideo();
					
					zapProtection = new CZapProtection( g_settings.parentallock_pincode, data );
					
					if ( zapProtection->check() )
					{
						g_RemoteControl->startvideo(chanlist[selected]->channel_id);
						
						// remember it for the next time
						chanlist[selected]->last_unlocked_EPGid = g_RemoteControl->current_EPGid;
					}
					delete zapProtection;
					zapProtection = NULL;
				}
				else
					g_RemoteControl->startvideo(chanlist[selected]->channel_id);
			}
		}
		else
			g_RemoteControl->startvideo(chanlist[selected]->channel_id);

		return messages_return::handled;
	}
	else
		return messages_return::unhandled;
}

// bToo default to true
bool CChannelList::adjustToChannelID(const t_channel_id channel_id, bool bToo)
{
	unsigned int i;

	dprintf(DEBUG_NORMAL, "CChannelList::adjustToChannelID: channel_id %llx\n", channel_id);
	
	for (i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i] != NULL)
		{
			if (chanlist[i]->channel_id == channel_id) 
			{
				selected = i;
				lastChList.store(selected, channel_id, false);

				tuned = i;
				
				if (bToo && (bouquetList != NULL)) 
				{
					//FIXME
					if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv) 
					{
						TVbouquetList->adjustToChannelID(channel_id);
						TVsatList->adjustToChannelID(channel_id);
						TVfavList->adjustToChannelID(channel_id);
						TVallList->adjustToChannelID(channel_id);
					} 
					else if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio) 
					{
						RADIObouquetList->adjustToChannelID(channel_id);
						RADIOsatList->adjustToChannelID(channel_id);
						RADIOfavList->adjustToChannelID(channel_id);
						RADIOallList->adjustToChannelID(channel_id);
					}
				}
				
				return true;
			}
		}
	}

	return false;
}

int CChannelList::hasChannel(int nChannelNr)
{
	for (uint32_t i = 0; i < chanlist.size(); i++) 
	{
		if (getKey(i) == nChannelNr)
			return(i);
	}
	
	return(-1);
}

int CChannelList::hasChannelID(t_channel_id channel_id)
{
	for (uint32_t i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i]->channel_id == channel_id)
			return i;
	}
	
	return -1;
}

// for adjusting bouquet's channel list after numzap or quickzap
void CChannelList::setSelected( int nChannelNr)
{
	selected = nChannelNr;
}

// -- Zap to channel with channel_id
bool CChannelList::zapTo_ChannelID(const t_channel_id channel_id, bool rezap)
{
	dprintf(DEBUG_NORMAL, "CChannelList::zapTo_ChannelID %llx\n", channel_id);
	
	for (unsigned int i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i]->channel_id == channel_id) 
		{
			zapTo(i, rezap);
			return true;
		}
	}
	
	return false;
}

// 
void CChannelList::zapTo(int pos, bool rezap)
{
	// show emty channellist error msg
	if (chanlist.empty()) 
	{
		if (FrontendCount >= 1) 
			MessageBox(_("Error"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> System)"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
		return;
	}

	if ( (pos >= (signed int) chanlist.size()) || (pos < 0) ) 
	{
		pos = 0;
	}

	CZapitChannel * chan = chanlist[pos];
	
	dprintf(DEBUG_NORMAL, "CChannelList::zapTo me %s tuned %d new %d %s -> %llx\n", name.c_str(), tuned, pos, chan->name.c_str(), chan->channel_id);
	
	if ( (pos != (int)tuned) || rezap ) //FIXME: allow after scan to tun
	{  
		if ((g_settings.radiotext_enable) && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio) && (g_Radiotext))
		{
			// stop radiotext PES decoding before zapping
			g_Radiotext->radiotext_stop();
		}		
		
		tuned = pos;
		g_RemoteControl->zapTo_ChannelID(chan->channel_id, chan->name, !chan->bAlwaysLocked); // UTF-8
		
		// TODO check is it possible bouquetList is NULL ?
		if (bouquetList != NULL) 
		{
			CNeutrinoApp::getInstance()->channelList->adjustToChannelID(chan->channel_id);
		}
		
		if(new_mode_active)
			selected = pos;
	}

	if(!new_mode_active) 
	{
		selected = pos;

		// remove recordModeActive from infobar
		if(g_settings.auto_timeshift && !CNeutrinoApp::getInstance()->recordingstatus) 
		{
			g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, 0);
		}

		// show infoBar
		g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	}
}

// -1: channellist not found
int CChannelList::numericZap(int key)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	if (chanlist.empty()) 
	{
		if (FrontendCount >= 1) 
			MessageBox(_("Error"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			
		return res;
	}

	// -- quickzap "0" (recall) to last seen channel...
	if (key == g_settings.key_lastchannel) 
	{
		t_channel_id channel_id = lastChList.getlast(1);
		if(channel_id) 
		{
			lastChList.clear_storedelay(); // ignore store delay
			zapTo_ChannelID(channel_id);
		}
		
		return res;
	}

	if (key == g_settings.key_zaphistory) 
	{
		// recording status
		if(!autoshift && CNeutrinoApp::getInstance()->recordingstatus) 
		{
			CChannelList * orgList = bouquetList->orgChannelList;
			CChannelList * channelList = new CChannelList(_("Current transponder"), false, true);
			
			t_channel_id recid = rec_channel_id >> 16;
			
			// get channels from the same tp as recording channel
			for ( unsigned int i = 0 ; i < orgList->chanlist.size(); i++) 
			{
				if((orgList->chanlist[i]->channel_id >> 16) == recid) 
				{
					channelList->addChannel(orgList->chanlist[i]);
				}
			}

			if (channelList->getSize() != 0) 
			{
				channelList->adjustToChannelID(orgList->getActiveChannel_ChannelID(), false);
				
				this->frameBuffer->paintBackground();
				
				this->frameBuffer->blit();

				res = channelList->exec();
			}
			
			delete channelList;
			return res;
		}
		
		// -- zap history bouquet, similar to "0" quickzap, but shows a menue of last channels
		if (this->lastChList.size() > 1) 
		{
			CChannelList * channelList = new CChannelList(_("History"), true, true);

			for(unsigned int i = 1 ; i < this->lastChList.size() ; ++i) 
			{
				t_channel_id channel_id = this->lastChList.getlast(i);

				if(channel_id) 
				{
					CZapitChannel * channel = getChannel(channel_id);
					if(channel) 
						channelList->addChannel(channel);
				}
			}

			if (channelList->getSize() != 0) 
			{
				this->frameBuffer->paintBackground();

				this->frameBuffer->blit();

				res = channelList->exec();
			}
			delete channelList;
		}
		return res;
	}
	
	//TEST: PiP
	if(key == g_settings.key_pip )
	{
		CChannelList * orgList = bouquetList->orgChannelList;
		CChannelList * channelList = new CChannelList(_("Current transponder"), false, true);
			
		t_channel_id pipid = live_channel_id >> 16;
			
		for ( unsigned int i = 0 ; i < orgList->chanlist.size(); i++) 
		{
			if((orgList->chanlist[i]->channel_id >> 16) == pipid) 
			{
				channelList->addChannel(orgList->chanlist[i]);
			}
		}
			
		pip_selected = true;

		if (channelList->getSize() != 0) 
		{
			channelList->adjustToChannelID(orgList->getActiveChannel_ChannelID(), false);
			this->frameBuffer->paintBackground();

			this->frameBuffer->blit();

			res = channelList->exec();
		}
		delete channelList;
		return res;
	}
	//

	int sx = 4 * g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(widest_number) + 14;
	int sy = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight() + 6;

	int ox = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - sx)/2;
	int oy = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - sy)/2;
	char valstr[10];
	int chn = CRCInput::getNumericValue(key);
	int pos = 1;
	int lastchan= -1;
	bool doZap = true;
	bool showEPG = false;

	while(1) 
	{
		if (lastchan != chn) 
		{
			sprintf((char*) &valstr, "%d", chn);
			
			while(strlen(valstr)<4)
				strcat(valstr,"-");   //"_"

			frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR_PLUS_0);

			for (int i = 3; i >= 0; i--) 
			{
				valstr[i+ 1]= 0;
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(ox + 7 + i*((sx - 14)>>2), oy + sy - 3, sx, &valstr[i], COL_INFOBAR);
			}
			
			frameBuffer->blit();

			// show infobar
			showInfo(chn - 1);
			lastchan = chn;
		}

		g_RCInput->getMsg( &msg, &data, g_settings.timing[SNeutrinoSettings::TIMING_NUMERICZAP] * 10 );

		if ( msg == RC_timeout ) 
		{
			if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
				chn = tuned + 1;
			break;
		}
		else if (CRCInput::isNumeric(msg)) 
		{
			if (pos == 4) 
			{
				chn = 0;
				pos = 1;
			} 
			else 
			{
				chn *= 10;
				pos++;
			}
			chn += CRCInput::getNumericValue(msg);
		}
		else if ( msg == RC_ok ) 
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) ) 
			{
				chn = tuned + 1;
			}
			break;
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_quickzap_down ) 
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else {
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_quickzap_up ) 
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if ( ( msg == RC_home ) || ( msg == RC_left ) || ( msg == RC_right) )
		{
			doZap = false;
			break;
		}
		else if ( msg == RC_red ) 
		{
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) ) 
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
		{
			doZap = false;
			break;
		}
	}

	frameBuffer->paintBackgroundBoxRel(ox, oy, sx, sy);
	
	frameBuffer->blit();

	chn--;
	
	if (chn < 0)
		chn = 0;
	
	if ( doZap ) 
	{
		// kill infobar
		if(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0)
			g_InfoViewer->killTitle();
		
		// zapto selected channel
		zapTo( chn );
	} 
	else 
	{
		// show infobar
		showInfo(tuned);
		
		// kill infobar
		g_InfoViewer->killTitle();

		if ( showEPG )
			g_EventList->exec(chanlist[chn]->epgid, chanlist[chn]->name);
	}
	
	return res;
}

void CChannelList::virtual_zap_mode(bool up)
{
	dprintf(DEBUG_NORMAL, "CChannelList::virtual_zap_mode\n");

        neutrino_msg_t      msg;
        neutrino_msg_data_t data;

        if (chanlist.empty()) 
	{
		if (FrontendCount >= 1) 
			MessageBox(_("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			
                return;
        }

        int chn = getActiveChannelNumber() + (up ? 1 : -1);
        if (chn > (int)chanlist.size())
		chn = 1;
	
        if (chn == 0)
		chn = (int)chanlist.size();
	
        int lastchan = -1;
        bool doZap = true;
        bool showEPG = false;
	int epgpos = 0;

        while(1)
        {
                if (lastchan != chn || (epgpos != 0))
                {
                        showInfo(chn - 1, epgpos);
                        lastchan = chn;
                }

		epgpos = 0;
		g_RCInput->getMsg( &msg, &data, 15*10 ); // 15 seconds, not user changable

                if ( msg == RC_ok )
                {
                        if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
                        {
                                chn = tuned + 1;
                        }
                        break;
                }
                else if ( msg == RC_left )
                {
                        if ( chn == 1 )
                                chn = chanlist.size();
                        else
                        {
                                chn--;

                                if (chn > (int)chanlist.size())
                                        chn = (int)chanlist.size();
                        }
                }
                else if ( msg == RC_right )
                {
                        chn++;

                        if (chn > (int)chanlist.size())
                                chn = 1;
                }
                else if ( msg == RC_up )
                {
                        epgpos = -1;
                }
                else if ( msg == RC_down )
                {
                        epgpos = 1;
                }
                else if ( ( msg == RC_home ) || ( msg == RC_timeout ) )
                {
                        // Abbruch ohne Channel zu wechseln
                        doZap = false;
                        break;
                }
                else if ( msg == RC_red )
                {
                        // Rote Taste zeigt EPG fuer gewaehlten Kanal an
                        if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
                        {
                                doZap = false;
                                showEPG = true;
                                break;
                        }
                }
                else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
                {
                        doZap = false;
                        break;
                }
        }
        
	g_InfoViewer->clearVirtualZapMode();

        chn--;
        if (chn < 0)
                chn = 0;

        if ( doZap )
        {
		if(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0)
			g_InfoViewer->killTitle();

                zapTo(chn);
        }
        else
        {
                showInfo(tuned);
                g_InfoViewer->killTitle();

                // Rote Taste zeigt EPG fuer gewaehlten Kanal an
                if ( showEPG )
                        g_EventList->exec(chanlist[chn]->epgid, chanlist[chn]->name);
        }
}

void CChannelList::quickZap(int key, bool cycle)
{
        if(chanlist.size() == 0)
                return;

	if ( (key == g_settings.key_quickzap_down) || (key == RC_left) )
	{
                if(selected == 0)
                        selected = chanlist.size() - 1;
                else
                        selected--;
        }
	else if ((key == g_settings.key_quickzap_up) || (key == RC_right) )
	{
                selected = (selected + 1)%chanlist.size();
        }

	dprintf(DEBUG_NORMAL, "CChannelList::quickZap: quick zap selected = %d getActiveBouquetNumber %d\n", selected, bouquetList->getActiveBouquetNumber());

	if(cycle)
		bouquetList->orgChannelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(selected) - 1);
	else
        	zapTo(selected);

	g_RCInput->clearRCMsg(); //FIXME test for n.103
}

bool CChannelList::canZap(CZapitChannel * channel)
{
	bool iscurrent = true;

	if(!autoshift && CNeutrinoApp::getInstance()->recordingstatus) 
	{
		if(channel == NULL)
			channel = chanlist[selected];
		
		iscurrent = CZapit::getInstance()->CanZap(channel);
	}
	
	return iscurrent;
}

void CChannelList::paint()
{
	dprintf(DEBUG_NORMAL, "CChannelList::paint\n");

	if (head) 
		head->clear();
		
	if (listBox) 
		listBox->clear();
		
	if (foot) 
		foot->clear();
	
	CChannelEvent * p_event = NULL;
	time_t jetzt = time(NULL);
	unsigned int runningPercent = 0;

	char cSeit[11] = " ";
	char cNoch[11] = " ";

	if(chanlist.size())
	{
		for(unsigned int i = 0; i < chanlist.size(); i++)
		{
			p_event = NULL;
			jetzt = time(NULL);
			runningPercent = 0;

			std::string desc = chanlist[i]->description;

			if (displayNext) 
			{
				p_event = &chanlist[i]->nextEvent;
			} 
			else 
			{
				p_event = &chanlist[i]->currentEvent;
			}

			// runningPercent	
			if (((jetzt - p_event->startTime + 30) / 60) < 0 )
			{
				runningPercent = 0;
			}
			else
			{
				if(p_event->duration > 0)
					runningPercent = (unsigned) ((float) (jetzt - p_event->startTime) / (float) p_event->duration * 100.);
					
				if(runningPercent > 100)
					runningPercent = 0;
			}

			// description
			if (p_event != NULL && !(p_event->description.empty())) 
			{
				desc = p_event->description;
			}
			
			std::string option = "";
			
			if (!p_event->description.empty())
			{
			 	option = " - ";
				option += p_event->description.c_str();
			}

			item = new ClistBoxItem(chanlist[i]->name.c_str(), true, option.c_str());

			// channel number
			item->setNumber(chanlist[i]->number);
			
			// timescale
			if (g_settings.channellist_timescale) item->setPercent(runningPercent);
			
			// hd / ca / webtv icon
			if (g_settings.channellist_ca)
			{
				item->setIcon1(chanlist[i]->isWEBTV()? NEUTRINO_ICON_WEBTV_SMALL : chanlist[i]->isHD() ? NEUTRINO_ICON_HD : chanlist[i]->isUHD()? NEUTRINO_ICON_UHD : "");
				
				//
				item->setIcon2(chanlist[i]->scrambled? NEUTRINO_ICON_SCRAMBLED : "");
			}
			
			// logo
			if (g_settings.epgplus_show_logo)
			{
				std::string logo;

				logo = CChannellogo::getInstance()->getLogoName(chanlist[i]->getLogoID());
				
				item->setIconName(logo.c_str());
			}
			
			// option font color
			if (!displayNext) item->setOptionFontColor(COL_INFOBAR_COLORED_EVENTS);
			

			if (listBox) listBox->addItem(item);
		}
	}
	
	// head
	if (head) head->setTitle(_(name.c_str()));
	if (head) head->enablePaintDate();
	if (head) head->setButtons(new_mode_active? HeadNewModeButtons : HeadButtons, HEAD_BUTTONS_COUNT);
	
	//
	if (displayNext) 
	{
		CChannelListButtons[1].localename = _("Now");
	} 
	else 
	{
		CChannelListButtons[1].localename = _("Next");
	}

	// foot
	if (foot) foot->setButtons(CChannelListButtons, NUM_LIST_BUTTONS);

	//
	if (listBox) listBox->setSelected(selected);
	
	if (chWidget) chWidget->paint();
	
	//
	if (window) window->saveScreen();
	
	paintCurrentNextEvent(selected);
}

void CChannelList::paintCurrentNextEvent(int _selected)
{
	if (window)
	{	window->restoreScreen(); 
		window->paint();
	}
	
	if (hline) 
		hline->paint();
	if (vline) 
		vline->paint();
	
	CChannelEvent * p_event = NULL;
	time_t jetzt = time(NULL);
	unsigned int runningPercent = 0;

	char cSeit[11] = " ";
	char cNoch[11] = " ";
	
	// now
	p_event = &chanlist[_selected]->currentEvent;
	
	// title
	CCLabel epgTitle(winTopBox.iX + 10, winTopBox.iY + 10, winTopBox.iWidth - 20, 60);
	
	epgTitle.setText(p_event->description.c_str());
	epgTitle.setHAlign(CC_ALIGN_CENTER);
	epgTitle.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	// runningPercent
	runningPercent = 0;
		
	if (((jetzt - p_event->startTime + 30) / 60) < 0 )
	{
		runningPercent = 0;
	}
	else
	{
		if(p_event->duration > 0)
			runningPercent = (unsigned) ((float) (jetzt - p_event->startTime) / (float) p_event->duration * 100.);
			
		if(runningPercent > 100)
			runningPercent = 0;
	}
	
	CProgressBar pb(winTopBox.iX + 100, winTopBox.iY + 10 + 60 + 20, winTopBox.iWidth - 200, 5);
	
	//
	if (p_event != NULL && !(p_event->description.empty())) 
	{
		// start
		struct tm * pStartZeit = localtime(&p_event->startTime);
		sprintf(cSeit, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		
		// end
		long int uiEndTime(p_event->startTime + p_event->duration);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);

		sprintf(cNoch, "%02d:%02d", pEndeZeit->tm_hour, pEndeZeit->tm_min);
	}
	
	CCLabel startTime(winTopBox.iX + 10, winTopBox.iY + 10 + 60 + 10, 80, 20);
	startTime.setText(cSeit);
	startTime.setHAlign(CC_ALIGN_CENTER);
	startTime.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	
	CCLabel restTime(winTopBox.iX + 100 + winTopBox.iWidth - 200 + 10, winTopBox.iY + 10 + 60 + 10, 80, 20);
	restTime.setText(cNoch);
	restTime.setHAlign(CC_ALIGN_CENTER);
	restTime.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	
	// text
	CCText text(winTopBox.iX + 10, winTopBox.iY + 10 + 60 + 10 + 10, winTopBox.iWidth - 20, winTopBox.iHeight - 100);
	text.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	text.setText(p_event->text.c_str());
	
	// next
	CChannelEventList events;

	time_t atime = time(NULL);
					
	events.clear();

	CSectionsd::getInstance()->getEventsServiceKey(chanlist[_selected]->channel_id & 0xFFFFFFFFFFFFULL, events);
	chanlist[_selected]->nextEvent.startTime = (long)0x7fffffff;
				
	for ( CChannelEventList::iterator e = events.begin(); e != events.end(); ++e ) 
	{
		if (((long)(e->startTime) > atime) && ((e->startTime) < (long)(chanlist[_selected]->nextEvent.startTime)))
		{
			chanlist[_selected]->nextEvent= *e;
					
			break;
		}
	}
	
	p_event = &chanlist[_selected]->nextEvent;
	
	//
	CCLabel nextTitle(winBottomBox.iX + 10, winBottomBox.iY + 10, winBottomBox.iWidth - 20, 20);
	nextTitle.setText(p_event->description.c_str());
	nextTitle.setHAlign(CC_ALIGN_CENTER);
	nextTitle.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	// vom/bis
	std::string fromto = "";
	CCLabel nextTime(winBottomBox.iX + 10, winBottomBox.iY + 10 + 30, winBottomBox.iWidth - 20, 20);
	nextTime.setHAlign(CC_ALIGN_CENTER);
	nextTime.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	//
	if (p_event != NULL && !(p_event->description.empty())) 
	{
		// start
		struct tm * pStartZeit = localtime(&p_event->startTime);
		sprintf(cSeit, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		
		// end
		long int uiEndTime(p_event->startTime + p_event->duration);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);

		sprintf(cNoch, "%02d:%02d", pEndeZeit->tm_hour, pEndeZeit->tm_min);
	}
	
	fromto = cSeit;
	fromto += "   -   ";
	fromto += cNoch;
	
	nextTime.setText(fromto.c_str());
	
	// nextText
	CCText nextText(winBottomBox.iX + 10, winBottomBox.iY + 10 + 60, winBottomBox.iWidth - 20, winBottomBox.iHeight - 80);
	nextText.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	nextText.setText(p_event->text.c_str());
	
	//
	//winTop->paint();
	epgTitle.paint();
	pb.reset();
	pb.paint(runningPercent);
	startTime.paint();
	restTime.paint();
	text.paint();
	//
	//winBottom->paint();
	nextTitle.paint();
	nextTime.paint();
	nextText.paint();
}

int CChannelList::getSize() const
{
	return this->chanlist.size();
}

int CChannelList::getSelectedChannelIndex() const
{
	return this->selected;
}

