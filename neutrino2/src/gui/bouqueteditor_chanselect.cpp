/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_chanselect.cpp 2018/08/21 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#include <driver/gdi/fontrenderer.h>

#include <gui/widget/icons.h>

//
#include <zapit/channel.h>
#include <zapit/bouquets.h>

#include <system/debug.h>
#include <system/helpers.h>

#include <gui/bouqueteditor_chanselect.h>


//// globals
extern tallchans allchans;			// defined in zapit.cpp

////
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
	{ NEUTRINO_ICON_BUTTON_OKAY, _("TV/Radio"), 0 },
	{ NEUTRINO_ICON_BUTTON_HOME, _("ready"), 0 }	
};

void CBEChannelSelectWidget::paint()
{
	dprintf(DEBUG_DEBUG, "CBEChannelSelectWidget::paint\n");
	
	widget = CNeutrinoApp::getInstance()->getWidget("bqeditchselect");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		listBox->paintMainFrame(true);
		
		//
		listBox->enablePaintHead();
		listBox->enablePaintDate();
		
		//
		listBox->enablePaintFoot();
		
		//
		widget->addCCItem(listBox);
	}	

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

		listBox->addItem(item);
	}

	//
	listBox->setTitle(caption.c_str(), NEUTRINO_ICON_EDIT);
	
	//
	listBox->setFootButtons(Buttons, BUTTONS_COUNT);

	//
	listBox->setSelected(selected);
	
	//
	widget->paint();
}

void CBEChannelSelectWidget::hide()
{
	widget->hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CBEChannelSelectWidget::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CBEChannelSelectWidget::exec: actionKey:%s\n", actionKey.c_str());

	//
	if (mode == CZapit::MODE_TV)
		bouquetChannels = &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels);
	else if (mode == CZapit::MODE_RADIO)
		bouquetChannels = &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);

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

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_epg);

		if( msg == CRCInput::RC_ok)
		{
			if (listBox && listBox->hasItem())
			{
				selected = listBox->getSelected();
			
				setModified();
		
				if (isChannelInBouquet(selected))
					CZapit::getInstance()->Bouquets[bouquet]->removeService(Channels[selected]->channel_id);
				else
					CZapit::getInstance()->addChannelToBouquet(bouquet, Channels[selected]->channel_id);

				bouquetChannels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels) : &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);
		
				paint();
			}
		}
		else if (msg == CRCInput::RC_home)
		{
			loop = false;
		}
		else if (msg == CRCInput::RC_page_up)
		{
			listBox->scrollPageUp();
		}
		else if (msg == CRCInput::RC_page_down)
		{
			listBox->scrollPageDown();
		}
		else if (msg == CRCInput::RC_up)
		{
			listBox->scrollLineUp();
		}
		else if (msg == CRCInput::RC_down)
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

