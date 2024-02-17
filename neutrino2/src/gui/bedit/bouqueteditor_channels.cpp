/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_channels.cpp 2018/08/22 mohousch Exp $

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

#include <gui/widget/widget_helpers.h>
#include <gui/widget/messagebox.h>

#include <gui/bedit/bouqueteditor_channels.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/icons.h>

#include <gui/bedit/bouqueteditor_chanselect.h>

#include <gui/widget/widget_helpers.h>

//
#include <zapit/bouquets.h>

#include <system/debug.h>


////
CBEChannelWidget::CBEChannelWidget(const std::string & Caption, unsigned int Bouquet)
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = -1;
	
	state = beDefault;
	caption = Caption;
	bouquet = Bouquet;
	mode = CZapit::MODE_TV;

	//
	widget = NULL;
	listBox = NULL;
	item = NULL;
	sec_timer_id = 0;

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CBEChannelWidget::~CBEChannelWidget()
{
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

#define BUTTONS_COUNT 4
const struct button_label CBEChannelWidgetButtons[BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , _("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN , _("Add") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Move")},
	{ NEUTRINO_ICON_BUTTON_BLUE  , _("TV/Radio")}
};

void CBEChannelWidget::paint()
{
	dprintf(DEBUG_NORMAL, "CBEChannelWidget::paint:\n");
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("bqeditch");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		//
		listBox->enablePaintHead();
		listBox->enablePaintDate();
//		listBox->setHeadLine(true, true);
		
		//
		listBox->enablePaintFoot();
//		listBox->setFootLine(true, true);
		
		widget->addCCItem(listBox);
	}	

	for (unsigned int count = 0; count < Channels->size(); count++)
	{
		item = new CMenuForwarder((*Channels)[count]->getName().c_str());

		if(state == beMoving && count == selected)
			item->setIconName(NEUTRINO_ICON_BUTTON_YELLOW);

		item->setIcon1((*Channels)[count]->isHD() ? NEUTRINO_ICON_HD : (*Channels)[count]->isUHD()? NEUTRINO_ICON_UHD : "");
		item->setIcon2((*Channels)[count]->scrambled ? NEUTRINO_ICON_SCRAMBLED : "");

		listBox->addItem(item);
	}

	//
	listBox->setTitle(_(caption.c_str()), NEUTRINO_ICON_EDIT);

	//
	listBox->setFootButtons(CBEChannelWidgetButtons, BUTTONS_COUNT);

	//
	listBox->setSelected(selected);
	
	//
	widget->paint();
}

void CBEChannelWidget::hide()
{
	widget->hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CBEChannelWidget::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CBEChannelWidget::exec:\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	if (mode == CZapit::MODE_TV)
		Channels = &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels);
	else if (mode == CZapit::MODE_RADIO)
		Channels = &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);
	
	//
	paint();
	frameBuffer->blit();	

	channelsChanged = false;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_epg);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_epg);

		if ((msg == CRCInput::RC_timeout) || (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			if (state == beDefault)
			{
				loop = false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
		else if (msg == CRCInput::RC_page_up)
		{
			if (state == beDefault)
				listBox->scrollPageUp();
			else if(state == beMoving)
			{
				selected = listBox->getSelected();
				int next_selected = selected - listBox->getListMaxShow();

				if (next_selected < 0)
					next_selected = 0;

				internalMoveChannel(selected, next_selected);
			}
		}
		else if (msg == CRCInput::RC_page_down)
		{
			if (state == beDefault)
				listBox->scrollPageDown();
			else if(state == beMoving)
			{
				selected = listBox->getSelected();

				int next_selected = selected + listBox->getListMaxShow();

				if (next_selected > (int)Channels->size())
					next_selected = Channels->size();

				internalMoveChannel(selected, next_selected);
			}
		}
		else if (msg == CRCInput::RC_up)
		{
			if (!(Channels->empty()))
			{
				selected = listBox->getSelected();

				int prev_selected = selected;
				int next_selected = selected - 1;
				if (next_selected < 0)
					next_selected = 0;

				if (state == beDefault)
				{
					listBox->scrollLineUp();
				}
				else if (state == beMoving)
				{
					internalMoveChannel(prev_selected, next_selected);
				}
			}
		}
		else if (msg == CRCInput::RC_down)
		{
			selected = listBox->getSelected();

			int prev_selected = selected;
			int next_selected = selected + 1;
			if (next_selected > (int)Channels->size())
				next_selected = Channels->size();

			if (state == beDefault)
			{
				listBox->scrollLineDown();
			}
			else if (state == beMoving)
			{
				internalMoveChannel(prev_selected, next_selected);
			}
		}
		else if(msg == CRCInput::RC_red)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				deleteChannel();
		}
		else if(msg == CRCInput::RC_green)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				addChannel();
		}
		else if(msg == CRCInput::RC_yellow)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				beginMoveChannel();

			paint();
		}
		else if(msg == CRCInput::RC_blue)
		{
			if (state == beDefault)
			{
				if (mode == CZapit::MODE_TV)
					mode = CZapit::MODE_RADIO;
				else
					mode = CZapit::MODE_TV;

				Channels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels) : &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);

				selected = 0;

				paint();
			}
		}
		else if(msg == CRCInput::RC_ok)
		{
			selected = listBox->getSelected();
			
			if (state == beMoving) 
			{
				finishMoveChannel();
			}
		}
		else if( CRCInput::isNumeric(msg) )
		{
			if (state == beDefault)
			{
				//kein pushback - wenn man versehentlich wo draufkommt is die edit-arbeit umsonst
				//selected = oldselected;
				//g_RCInput->postMsg( msg, data );
				//loop=false;
			}
			else if (state == beMoving)
			{
				cancelMoveChannel();
			}
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			widget->refresh();
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}

		frameBuffer->blit();	
	}
	
	hide();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	return res;
}

void CBEChannelWidget::deleteChannel()
{
	if (selected >= Channels->size()) /* Channels.size() might be 0 */
		return;

	if (MessageBox(_("Delete"), (*Channels)[selected]->getName().c_str(), CMessageBox::mbrNo, CMessageBox::mbYes|CMessageBox::mbNo, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, CComponent::BORDER_ALL) != CMessageBox::mbrYes)
		return;

	CZapit::getInstance()->Bouquets[bouquet]->removeService((*Channels)[selected]->channel_id);

	Channels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels) : &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);

	if (selected >= Channels->size())
		selected = Channels->empty() ? 0 : (Channels->size() - 1);
	
	channelsChanged = true;

	paint();
}

void CBEChannelWidget::addChannel()
{
	CBEChannelSelectWidget * channelSelectWidget = new CBEChannelSelectWidget(caption, bouquet, mode);

	channelSelectWidget->exec(this, "");
	
	if (channelSelectWidget->hasChanged())
	{
		channelsChanged = true;
		Channels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels) : &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);
	}
	
	delete channelSelectWidget;

	paint();
}

void CBEChannelWidget::beginMoveChannel()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEChannelWidget::finishMoveChannel()
{
	state = beDefault;

	paint();
}

void CBEChannelWidget::cancelMoveChannel()
{
	state = beDefault;
	internalMoveChannel(newPosition, origPosition);
	channelsChanged = false;
}

void CBEChannelWidget::internalMoveChannel(unsigned int fromPosition, unsigned int toPosition)
{
	if ( (int) toPosition == -1 ) 
		return;
	
	if (toPosition == Channels->size()) 
		return;

	CZapit::getInstance()->Bouquets[bouquet]->moveService(fromPosition, toPosition, mode == CZapit::MODE_TV ? ST_DIGITAL_TELEVISION_SERVICE : ST_DIGITAL_RADIO_SOUND_SERVICE);

	channelsChanged = true;
	Channels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[bouquet]->tvChannels) : &(CZapit::getInstance()->Bouquets[bouquet]->radioChannels);

	selected = toPosition;
	newPosition = toPosition;

	//
	paint();
}

bool CBEChannelWidget::hasChanged()
{
	return (channelsChanged);
}

