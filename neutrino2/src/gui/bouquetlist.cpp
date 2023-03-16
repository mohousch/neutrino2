/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouquetlist.cpp 16.11.2020 mohousch Exp $

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

#include <string>
#include <algorithm>

#include <gui/bouquetlist.h>

#include <driver/color.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/filebrowser.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>


extern CBouquetManager * g_bouquetManager;

CBouquetList::CBouquetList(const char* const Name)
{
	frameBuffer = CFrameBuffer::getInstance();
	selected    = 0;
	
	if(Name == NULL)
		name = _("Bouquets");
	else
		name = Name;
		
	//
	bqWidget = NULL;
	listBox = NULL;
	item = NULL;
	
	sec_timer_id = 0;

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() / 20 * 17;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() / 20 * 18;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;	
}

CBouquetList::~CBouquetList()
{
        for (std::vector<CBouquet *>::iterator it = Bouquets.begin(); it != Bouquets.end(); it++) 
	{
               	delete (*it);
        }
        
	Bouquets.clear();
}

CBouquet * CBouquetList::addBouquet(CZapitBouquet* zapitBouquet)
{
	CBouquet * tmp = addBouquet(zapitBouquet->Name.c_str(), -1, zapitBouquet->bLocked);
	tmp->zapitBouquet = zapitBouquet;
	
	return tmp;
}

CBouquet * CBouquetList::addBouquet(const char * const _name, int BouquetKey, bool locked)
{
	if ( BouquetKey == -1 )
		BouquetKey = Bouquets.size();

	CBouquet * tmp = new CBouquet( BouquetKey, _name, locked );
	Bouquets.push_back(tmp);
	
	return(tmp);
}

void CBouquetList::deleteBouquet(CBouquet * bouquet)
{
	if (bouquet != NULL) 
	{
		std::vector<CBouquet *>::iterator it = find(Bouquets.begin(), Bouquets.end(), bouquet);

		if (it != Bouquets.end()) 
		{
			Bouquets.erase(it);
			delete bouquet;
		}
	}
}

int CBouquetList::getActiveBouquetNumber()
{
	return selected;
}

void CBouquetList::adjustToChannel( int nChannelNr)
{
	for (uint32_t i = 0; i < Bouquets.size(); i++) 
	{
		int nChannelPos = Bouquets[i]->channelList->hasChannel(nChannelNr);
		
		if (nChannelPos > -1) 
		{
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return;
		}
	}
}

void CBouquetList::adjustToChannelID(t_channel_id channel_id)
{
	if(selected < Bouquets.size()) 
	{
		int nChannelPos = Bouquets[selected]->channelList->hasChannelID(channel_id);
		
		if(nChannelPos > -1) 
		{
			Bouquets[selected]->channelList->setSelected(nChannelPos);
			return;
		}
	}
	
	dprintf(DEBUG_NORMAL, "CBouquetList::adjustToChannelID to %llx\n", channel_id);
	
	for (uint32_t i = 0; i < Bouquets.size(); i++) 
	{
		if(i == selected)
			continue;
		
		int nChannelPos = Bouquets[i]->channelList->hasChannelID(channel_id);
		if (nChannelPos > -1) 
		{
			selected = i;
			Bouquets[i]->channelList->setSelected(nChannelPos);
			return;
		}
	}
}

// used in channellist to switch bouquets up/down
int CBouquetList::showChannelList( int nBouquet, bool zap)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::showChannelList\n");

	if (nBouquet == -1)
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->exec(zap);
	
	if (nNewChannel > -1) 
	{
		selected = nBouquet;
		nNewChannel = -2;
	}
	
	return nNewChannel;
}

// bShowChannelList default to false , return seems not checked anywhere
int CBouquetList::activateBouquet( int id, bool bShowChannelList, bool zap)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::activateBouquet: id:%d showChannelList:%s zap:%s\n", id, bShowChannelList? "yes" : "no", zap? "yes" : "no");

	int res = -1;

	if(id < (int) Bouquets.size())
		selected = id;

	if (bShowChannelList) 
	{
		res = Bouquets[selected]->channelList->exec(zap);

		if(zap)
		{
			if(res > -1)
				res = -2;
		}
	}
	
	return res;
}

int CBouquetList::exec(bool bShowChannelList, bool zap, bool customMode)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::exec: showChannelList:%s, zap:%s customMode:%s\n", bShowChannelList? "yes" : "no", zap? "yes" : "no", customMode? "yes" : "no");
	
	//
	bqWidget = CNeutrinoApp::getInstance()->getWidget("bouquetlist");
	
	if (bqWidget)
	{
		listBox = (ClistBox*)bqWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		bqWidget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		bqWidget->addWidgetItem(listBox);
	}

	// select bouquet to show
	int res = show(bShowChannelList, customMode);

	dprintf(DEBUG_NORMAL, "CBouquetList::exec: res:%d\n", res);

	if(!bShowChannelList)
		return res;
	
	// if >= 0, call activateBouquet to show channel list
	if ( res > -1) 
	{
		return activateBouquet(selected, bShowChannelList, zap);
	}
	
	return res;
}

int CBouquetList::doMenu()
{
	int i = 0;
	int select = -1;
	static int old_selected = 0;
	int ret = RETURN_NONE;
	signed int bouquet_id;
	CZapitBouquet * tmp, * zapitBouquet;
	ZapitChannelList * channels;

	if( !Bouquets.size() )
		return 0;

	zapitBouquet = Bouquets[selected]->zapitBouquet;
	
	// zapitBouquet not NULL only on real bouquets, not on virtual SAT or HD 
	if(!zapitBouquet)
		return 0;
	
	//
	CWidget* widget = NULL;
	ClistBox* menu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("bqedit");
	
	if (widget)
	{
		menu = (ClistBox*)CNeutrinoApp::getInstance()->getWidget("bqedit")->getWidgetItem(WIDGETITEM_LISTBOX);
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

	if(!zapitBouquet->bUser) 
	{
		menu->addItem(new CMenuForwarder(_("Copy bouquet to Favorites")), old_selected == i ++);
		ret = widget->exec(NULL, "");
		select = menu->getSelected();
		
		dprintf(DEBUG_NORMAL, "CBouquetList::doMenu: %d selected\n", select);

		if(select >= 0) 
		{
			old_selected = select;

			switch(select) 
			{
				case 0:
					hide();
					bouquet_id = g_bouquetManager->existsBouquet(Bouquets[selected]->channelList->getName());
					if(bouquet_id < 0) 
						tmp = g_bouquetManager->addBouquet(Bouquets[selected]->channelList->getName(), true);
					else
						tmp = g_bouquetManager->Bouquets[bouquet_id];
  
					// tv
					channels = &zapitBouquet->tvChannels;
					for(int i1 = 0; i1 < (int) channels->size(); i1++)
						tmp->addService((*channels)[i1]);
					
					// radio
					channels = &zapitBouquet->radioChannels;
					for(int li = 0; li < (int) channels->size(); li++)
						tmp->addService((*channels)[li]);
					return 1;
					break;
					
				default:
					break;
			}
		}
	} 
	else if(!zapitBouquet->bWebTV) 
	{
		menu->addItem(new CMenuForwarder(_("Delete")), old_selected == i ++);
		ret = widget->exec(NULL, "");
		select = menu->getSelected();
		
		dprintf(DEBUG_NORMAL, "CBouquetList::doMenu: %d selected\n", select);
		
		if(select >= 0) 
		{
			old_selected = select;
			switch(select) 
			{
				case 0:
					hide();
					bouquet_id = g_bouquetManager->existsBouquet(Bouquets[selected]->channelList->getName());
					if(bouquet_id >= 0) 
					{
						g_bouquetManager->deleteBouquet(bouquet_id);
						return 1;
					}
					break;
					
				default:
					break;
			}
		}
	}
	
	return ret;
}

// bShowChannelList default to true, returns new bouquet or -1/-2
int CBouquetList::show(bool bShowChannelList, bool customMode)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::show: showChannelList:%s customMode:%s\n", bShowChannelList? "yes" : "no", customMode? "yes" : "no");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = -1;
	
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);	

	paint();
	CFrameBuffer::getInstance()->blit();

	int zapOnExit = false;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	int mode = CNeutrinoApp::getInstance()->getMode();

	if(customMode)
		mode = CNeutrinoApp::getInstance()->getChMode();

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if ((msg == RC_timeout) || (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			loop = false;
		}
		else if(msg == RC_red || msg == RC_favorites) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(LIST_MODE_FAV, mode);

			hide();
			return -3;
		} 
		else if(msg == RC_green) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(LIST_MODE_PROV, mode);
			hide();
			return -3;
		} 
		else if(msg == RC_yellow || msg == RC_sat)
		{
			CNeutrinoApp::getInstance()->setChannelMode(LIST_MODE_SAT, mode);

			hide();
			return -3;
		} 
		else if(msg == RC_blue) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(LIST_MODE_ALL, mode);

			hide();
			return -3;
		}
		else if(Bouquets.size() == 0)
		{
			continue; //FIXME msgs not forwarded to neutrino !!
		}
		else if ( msg == RC_setup ) 
		{
			selected = listBox->getSelected();

			if (!Bouquets[selected]->zapitBouquet->bWebTV)
			{
				int ret = doMenu();
				if(ret) 
				{
					res = -4;
					loop = false;
				}
			}
		}
		else if ( msg == (neutrino_msg_t) g_settings.key_list_start ) 
		{
			selected = 0;
			paint();
		}
		else if ( msg == (neutrino_msg_t) g_settings.key_list_end ) 
		{
			selected = Bouquets.size() - 1;

			paint();
		}
		else if (msg == RC_up)
		{
			listBox->scrollLineUp();
		}
		else if (msg == RC_page_up || (int) msg == g_settings.key_channelList_pageup )
		{
			listBox->scrollPageUp();
		}
		else if ( msg == RC_down)
		{
			listBox->scrollLineDown();
		}
		else if ( msg == RC_page_down || (int) msg == g_settings.key_channelList_pagedown )
		{
			listBox->scrollPageDown();
		}
		else if ( msg == RC_ok ) 
		{
			selected = listBox->getSelected();

			if(!bShowChannelList || Bouquets[selected]->channelList->getSize() > 0) 
			{
				zapOnExit = true;
				loop = false;
			}
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			//listBox->paintHead();
			listBox->refresh();
		} 
		else 
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
			{
				loop = false;
				res = -2;
			}
		}
			
		frameBuffer->blit();	
	}
	
	hide();
	
	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	if(zapOnExit) 
		return (selected);
	else 
		return (res);
}

void CBouquetList::hide()
{
	if (bqWidget)
		bqWidget->hide();
	else
		CFrameBuffer::getInstance()->clearFrameBuffer();
		
	frameBuffer->blit();
}

const struct button_label HButton = {NEUTRINO_ICON_BUTTON_SETUP, " " };

const struct button_label CBouquetListButtons[4] =
{
        { NEUTRINO_ICON_BUTTON_RED, _("Favorites") },
        { NEUTRINO_ICON_BUTTON_GREEN, _("Providers")},
        { NEUTRINO_ICON_BUTTON_YELLOW, _("Satellites")},
        { NEUTRINO_ICON_BUTTON_BLUE, _("All Services")}
};

void CBouquetList::paint()
{
	dprintf(DEBUG_NORMAL, "CBouquetList::paint\n");

	listBox->clear();

	for (unsigned int count = 0; count < Bouquets.size(); count++)
	{
		item = new ClistBoxItem(_(Bouquets[count]->channelList->getName()));

		item->setNumber(count + 1);
		listBox->addItem(item);
	}

	// head
	listBox->enablePaintHead();
	listBox->setTitle(name.c_str());
	listBox->enablePaintDate();

	listBox->setHeadButtons(&HButton, 1);

	// foot
	listBox->enablePaintFoot();
	
	listBox->setFootButtons(CBouquetListButtons, 4);

	//
	listBox->setSelected(selected);
	
	//
	bqWidget->paint();
}




