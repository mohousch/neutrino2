//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: bouquetlist.cpp 21122024 mohousch Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <algorithm>

#include <gui/bouquetlist.h>

#include <driver/gdi/color.h>
#include <driver/gdi/fontrenderer.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/filebrowser.h>

#include <gui/widget/icons.h>

#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>
#include <system/settings.h>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>


////
CBouquetList::CBouquetList(const char* const Name)
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = -1;
	
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
	
	if (bqWidget)
	{
		delete bqWidget;
		bqWidget = NULL;
	}
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
	int res = 0;
	
	if (selected < 0)
		res = 0;
	else
		res = selected;
		
	return res;
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
	
	dprintf(DEBUG_NORMAL, "CBouquetList::adjustToChannelID to 0x%llx\n", channel_id);
	
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
int CBouquetList::showChannelList(int nBouquet, bool customMode)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::showChannelList: id:%d zap:%s\n", nBouquet, customMode? "no" : "yes");

	if (nBouquet == -1)
		nBouquet = selected;

	int nNewChannel = Bouquets[nBouquet]->channelList->exec(customMode);
	
	if (nNewChannel > -1) 
	{
		selected = nBouquet;
		nNewChannel = -2;
	}
	
	dprintf(DEBUG_NORMAL, "CBouquetList::showChannelList: nNewChannel:%d\n", nNewChannel);
	
	return nNewChannel;
}

// bShowChannelList default to false , return seems not checked anywhere
void CBouquetList::activateBouquet(int id)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::activateBouquet: id:%d\n", id);

	if(id < (int) Bouquets.size())
		selected = id;
}

//
// exec
// -1 = timeout / cancel
// -2 = forward msgs to neutrino
// -3 = mode chnage fav / prov / sat / all
// -4 = setup
// or selected bouquet pos
int CBouquetList::exec(bool bShowChannelList, bool customMode)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::exec: showChannelList:%s, zap:%s\n", bShowChannelList? "yes" : "no", customMode? "no" : "eys");

	// select bouquet to show
	int res = show(customMode);

	dprintf(DEBUG_NORMAL, "CBouquetList::exec: res:%d\n", res);

	if(!bShowChannelList)
		return res;
	
	// if >= 0, call activateBouquet to show channellist
	if ( res > -1) 
	{
		activateBouquet(selected);
		res = showChannelList(selected, customMode);
	}
	
	dprintf(DEBUG_NORMAL, "CBouquetList::exec: res:%d\n", res);
	
	return res;
}

//
// bShowChannelList
// -1 = timeout / cancel
// -2 = forward msgs to neutrino
// -3 = mode chnage fav / prov / sat / all
// -4 = setup
// or selected bouquet pos
int CBouquetList::show(bool customMode)
{
	dprintf(DEBUG_NORMAL, "CBouquetList::show: zap:%s\n", customMode? "no" : "yes");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = -1;
	
	CLCD::MODES oldLcdMode = CLCD::getInstance()->getMode();
	std::string oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, name.c_str());
	
	//
	paint();
	CFrameBuffer::getInstance()->blit();

	int zapOnExit = false;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	int mode = CNeutrinoApp::getInstance()->getMode();

	if(customMode)
		mode = CNeutrinoApp::getInstance()->getChMode();

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

		if ((msg == CRCInput::RC_timeout) || (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			loop = false;
		}
		else if(msg == CRCInput::RC_red || msg == CRCInput::RC_favorites) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(CChannelList::LIST_MODE_FAV, mode);

			hide();
			return -3;
		} 
		else if(msg == CRCInput::RC_green) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(CChannelList::LIST_MODE_PROV, mode);
			hide();
			return -3;
		} 
		else if(msg == CRCInput::RC_yellow || msg == CRCInput::RC_sat)
		{
			CNeutrinoApp::getInstance()->setChannelMode(CChannelList::LIST_MODE_SAT, mode);

			hide();
			return -3;
		} 
		else if(msg == CRCInput::RC_blue) 
		{
			CNeutrinoApp::getInstance()->setChannelMode(CChannelList::LIST_MODE_ALL, mode);

			hide();
			return -3;
		}
		else if(Bouquets.size() == 0)
		{
			continue;
		}
		else if ( msg == CRCInput::RC_setup ) 
		{
			selected = listBox? listBox->getSelected() : 0;

			if (Bouquets.size() && Bouquets[selected]->channelList->getSize() && !Bouquets[selected]->zapitBouquet->bWebTV)
			{
				//
				int ret = doMenu();
				
				dprintf(DEBUG_NORMAL, "CRCInput::RC_setup: doMenu: %d\n", ret);
				
				if(ret) 
				{
					res = -4;
					loop = false;
				}
				else
				{
					paint();
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
		else if (msg == CRCInput::RC_up)
		{
			if (listBox) listBox->scrollLineUp();
		}
		else if (msg == CRCInput::RC_page_up || (int) msg == g_settings.key_channelList_pageup )
		{
			if (listBox) listBox->scrollPageUp();
		}
		else if ( msg == CRCInput::RC_down)
		{
			if (listBox) listBox->scrollLineDown();
		}
		else if ( msg == CRCInput::RC_page_down || (int) msg == g_settings.key_channelList_pagedown )
		{
			listBox->scrollPageDown();
		}
		else if ( msg == CRCInput::RC_ok ) 
		{
			selected = listBox? listBox->getSelected() : 0;

			zapOnExit = !customMode;
			loop = false;
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (listBox) listBox->refresh();
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
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	if(zapOnExit) 
	{
		res = selected;
	}
		
	if (customMode)
	{
		res = selected;
	}
		
	dprintf(DEBUG_NORMAL, "CBouquetList::show: res:%d\n", res);
		
	return (res);
}

void CBouquetList::hide()
{
	if (bqWidget)
		bqWidget->hide();
	else
		CFrameBuffer::getInstance()->clearFrameBuffer();
		
	frameBuffer->blit();
	
	if (bqWidget)
	{
		delete bqWidget;
		bqWidget = NULL;
	}
}

const struct button_label HButton = { NEUTRINO_ICON_BUTTON_SETUP, " ", 0 };

const struct button_label CBouquetListButtons[4] =
{
        { NEUTRINO_ICON_BUTTON_RED, _("Favorites"), 0 },
        { NEUTRINO_ICON_BUTTON_GREEN, _("Providers"), 0},
        { NEUTRINO_ICON_BUTTON_YELLOW, _("Satellites"), 0},
        { NEUTRINO_ICON_BUTTON_BLUE, _("All Services"), 0}
};

void CBouquetList::paint()
{
	dprintf(DEBUG_NORMAL, "CBouquetList::paint\n");
	
	//
	if (bqWidget)
	{
		delete bqWidget;
		bqWidget = NULL;
	}
	
	//
	bqWidget = CNeutrinoApp::getInstance()->getWidget("bouquetlist");
	
	if (bqWidget)
	{
		listBox = (ClistBox*)bqWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		bqWidget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		//
		listBox->enablePaintHead();
		
		//
		listBox->enablePaintFoot();
		
		bqWidget->name = "bouquetlist";
		bqWidget->addCCItem(listBox);
	}	

	if (listBox) listBox->clear();

	for (unsigned int count = 0; count < Bouquets.size(); count++)
	{
		item = new CMenuForwarder(_(Bouquets[count]->channelList->getName()));

		item->setNumber(count + 1);
		if (listBox) listBox->addItem(item);
	}

	// head
	if (listBox)
	{
		if (!name.empty()) listBox->setTitle(name.c_str());
		listBox->enablePaintDate();
		listBox->setHeadButtons(&HButton, 1);
		listBox->setFootButtons(CBouquetListButtons, 4);
		listBox->setSelected(selected);
	}
	
	//
	bqWidget->paint();
}

int CBouquetList::doMenu()
{
	dprintf(DEBUG_NORMAL, "CBouquetList::doMenu\n");
	
	int i = 0;
	int select = -1;
	static int old_selected = 0;
	//int ret = CTarget::RETURN_NONE;
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
	
	widget = CNeutrinoApp::getInstance()->getWidget("bouquetlistedit");
	
	if (widget)
	{
		menu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = 500;
		box.iHeight = 150;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;		
		
		widget = new CWidget(&box);
		widget->name = "bouquetlistedit";
		
		//
		menu = new ClistBox(&box);

		menu->setMode(ClistBox::MODE_MENU);
		menu->setBorderMode();
		
		//
		menu->enablePaintHead();
		menu->setTitle(_("Edit"), NEUTRINO_ICON_SETTINGS);

		//
		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		menu->setFootButtons(&btn);
		
		//
		widget->addCCItem(menu);
	}
	
	//
	widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	widget->paintMainFrame(true);
	widget->enableSaveScreen();

	//
	if(!zapitBouquet->bUser) 
	{
		menu->addItem(new CMenuForwarder(_("Copy bouquet to Favorites")), old_selected == i ++);
		widget->exec(NULL, "");
		select = menu->getSelected();

		if (widget)
		{		
			delete widget;
			widget = NULL;
		}
		
		dprintf(DEBUG_NORMAL, "CBouquetList::doMenu: %d selected\n", select);

		if(select >= 0) 
		{
			old_selected = select;

			switch(select) 
			{
				case 0:
					hide();
					bouquet_id = CZapit::getInstance()->existsBouquet(Bouquets[selected]->channelList->getName());
					if(bouquet_id < 0) 
						tmp = CZapit::getInstance()->addBouquet(Bouquets[selected]->channelList->getName(), true);
					else
						tmp = CZapit::getInstance()->Bouquets[bouquet_id];
  
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
	else //if(!zapitBouquet->bWebTV) 
	{
		menu->addItem(new CMenuForwarder(_("Delete")), old_selected == i ++);
		widget->exec(NULL, "");
		select = menu->getSelected();
		
		dprintf(DEBUG_NORMAL, "CBouquetList::doMenu: %d selected\n", select);
		
		if(select >= 0) 
		{
			old_selected = select;
			switch(select) 
			{
				case 0:
					hide();
					bouquet_id = CZapit::getInstance()->existsBouquet(Bouquets[selected]->channelList->getName());
					if(bouquet_id >= 0) 
					{
						CZapit::getInstance()->deleteBouquet(bouquet_id);
						return 1;
					}
					break;
					
				default:
					break;
			}
		}
	}
	
	return 0;
}

