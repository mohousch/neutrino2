/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_chanselect.cpp 2018/08/21 mohousch Exp $

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

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>

#include <gui/widget/icons.h>
#include <gui/widget/widget_helpers.h>

//
#include <zapit/channel.h>
#include <zapit/bouquets.h>
#include <zapit/satconfig.h>
#include <zapit/getservices.h>

#include <system/debug.h>
#include <system/helpers.h>

#include <gui/bedit/bouqueteditor_chanselect.h>


extern satellite_map_t satellitePositions;	// defined in getServices.cpp
extern transponder_list_t transponders;		// defined in zapit.cpp
extern tallchans allchans;			// defined in zapit.cpp
extern CBouquetManager* g_bouquetManager;	// defined in zapit.cpp

CBEChannelSelectWidget::CBEChannelSelectWidget(const std::string& Caption, unsigned int Bouquet, CZapit::channelsMode Mode)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	widget = NULL;
	listBox = NULL;
	item = NULL;
	sec_timer_id = 0;

	//
	selected =  -1;

	caption = Caption;

	modified = false;

	//	
	bouquet = Bouquet;
	mode = Mode;

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;	
}

CBEChannelSelectWidget::~CBEChannelSelectWidget()
{
	if (listBox)
	{
		delete listBox;
		listBox = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

bool CBEChannelSelectWidget::isChannelInBouquet(int index)
{
	for (unsigned int i = 0; i < bouquetChannels->size(); i++)
	{
		if ((*bouquetChannels)[i]->channel_id == Channels[index]->channel_id)
			return true;
	}
	
	return false;
}

bool CBEChannelSelectWidget::hasChanged()
{
	return modified;
}

#define BUTTONS_COUNT 2
const struct button_label Buttons[BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_OKAY, _("TV/Radio") },
	{ NEUTRINO_ICON_BUTTON_HOME, _("ready") }	
};

void CBEChannelSelectWidget::paint()
{
	dprintf(DEBUG_DEBUG, "CBEChannelSelectWidget::paint\n");
	
	widget = CNeutrinoApp::getInstance()->getWidget("bqeditchselect");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		listBox->enablePaintHead();
		listBox->enablePaintDate();
		
		widget->addWidgetItem(listBox);
	}	

	listBox->clear();

	for (unsigned int count = 0; count < Channels.size(); count++)
	{
		item = new CMenuForwarder(Channels[count]->getName().c_str());

		// marked
		std::string marked_icon = "";
		if(isChannelInBouquet(count))
			marked_icon = NEUTRINO_ICON_MARK;

		item->setIconName(marked_icon.c_str());

		item->setIcon1(Channels[count]->isHD() ? NEUTRINO_ICON_HD : Channels[count]->isUHD()? NEUTRINO_ICON_UHD : "");
		item->setIcon2(Channels[count]->scrambled ? NEUTRINO_ICON_SCRAMBLED : "");

		// info1
		char buf[128];
		transponder_id_t ct = Channels[count]->getTransponderId();
		transponder_list_t::iterator tpI = transponders.find(ct);
		int len = snprintf(buf, sizeof(buf), "%d ", Channels[count]->getFreqId());

		// satname
		sat_iterator_t sit = satellitePositions.find(Channels[count]->getSatellitePosition());
		
		if(sit != satellitePositions.end()) 
		{
			snprintf(&buf[len], sizeof(buf) - len, "(%s)\n", sit->second.name.c_str());
		}

		item->setInfo1(buf);

		listBox->addItem(item);
	}

	//
	listBox->setTitle(caption.c_str(), NEUTRINO_ICON_EDIT);
	
	//
	listBox->enablePaintFoot();
	listBox->setFootButtons(Buttons, BUTTONS_COUNT);

	//
	listBox->setSelected(selected);
	
	//
	widget->paint();
}

void CBEChannelSelectWidget::hide()
{
	widget->hide();
	
	if (listBox)
	{
		delete listBox;
		listBox = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CBEChannelSelectWidget::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CBEChannelSelectWidget::exec: actionKey:%s\n", actionKey.c_str());

	//
	if (mode == CZapit::MODE_TV)
		bouquetChannels = &(g_bouquetManager->Bouquets[bouquet]->tvChannels);
	else if (mode == CZapit::MODE_RADIO)
		bouquetChannels = &(g_bouquetManager->Bouquets[bouquet]->radioChannels);

	Channels.clear();
	
	if (mode == CZapit::MODE_TV) 
	{
		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
			if ( (it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE) && (!IS_WEBTV(it->second.getChannelID())) )
				Channels.push_back(&(it->second));
	}
	else if (mode == CZapit::MODE_RADIO) 
	{
		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
			if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
				Channels.push_back(&(it->second));
	}

	//
	sort(Channels.begin(), Channels.end(), CmpChannelByChName());

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;
	selected = -1;

	if (parent)
		parent->hide();
		
	//
	paint();
	frameBuffer->blit();

	bool loop = true;
	modified = false;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_epg);
	
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_epg);

		if( msg == RC_ok)
		{
			if (listBox && listBox->hasItem())
			{
				selected = listBox->getSelected();
			
				setModified();
		
				if (isChannelInBouquet(selected))
					g_bouquetManager->Bouquets[bouquet]->removeService(Channels[selected]->channel_id);
				else
					CZapit::getInstance()->addChannelToBouquet(bouquet, Channels[selected]->channel_id);

				bouquetChannels = mode == CZapit::MODE_TV ? &(g_bouquetManager->Bouquets[bouquet]->tvChannels) : &(g_bouquetManager->Bouquets[bouquet]->radioChannels);
		
				paint();
				g_RCInput->postMsg(RC_down, 0);
			}
		}
		else if (msg == RC_home)
		{
			loop = false;
		}
		else if (msg == RC_page_up)
		{
			listBox->scrollPageUp();
		}
		else if (msg == RC_page_down)
		{
			listBox->scrollPageDown();
		}
		else if (msg == RC_up)
		{
			listBox->scrollLineUp();
		}
		else if (msg == RC_down)
		{
			listBox->scrollLineDown();
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			//
			widget->refresh();
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			loop = false;
			res = RETURN_EXIT_ALL;
		}

		frameBuffer->blit();	
	}

	hide();
	
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	return res;
}

