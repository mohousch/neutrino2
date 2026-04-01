//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: bouqueteditor.cpp 01042026 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <driver/gdi/fontrenderer.h>

#include <gui/bouqueteditor.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/keyboard_input.h>

//
#include <system/debug.h>


//// globals
extern tallchans allchans;			// defined in zapit.cpp

////
#define BUTTONS_COUNT	3

const struct button_label CBEBouquetWidgetButtons[BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete"), 0},
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Add"), 0},
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Move"), 0}
};

const struct button_label HButton = {NEUTRINO_ICON_BUTTON_SETUP, "", 0};

CBEBouquetWidget::CBEBouquetWidget()
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = -1;
	state = beDefault;
	blueFunction = beRename;
	
	Bouquets = &CZapit::getInstance()->Bouquets;

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

CBEBouquetWidget::~CBEBouquetWidget()
{
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CBEBouquetWidget::paint()
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::paint:\n");
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("bqeditbq");
	
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
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Bouquets"));	

	for (unsigned int count = 0; count < Bouquets->size(); count++)
	{
		if (!(*Bouquets)[count]->bWebTV)
		{
			item = new CMenuForwarder(_((*Bouquets)[count]->Name.c_str()));

			if(state == beMoving && count == selected)
				item->setIconName(NEUTRINO_ICON_BUTTON_YELLOW);

			// locked
			std::string locked_icon = "";
			if((*Bouquets)[count]->bLocked)
				locked_icon = NEUTRINO_ICON_LOCK;

			item->setIcon1(locked_icon.c_str());

			// hidden
			std::string hiden_icon = "";
			if((*Bouquets)[count]->bHidden)
				hiden_icon = NEUTRINO_ICON_HIDDEN;

			item->setIcon2(hiden_icon.c_str()); 

			listBox->addItem(item);
		}
	}

	//
	listBox->setTitle(_("Bouquets"), NEUTRINO_ICON_EDIT);
	listBox->setHeadButtons(&HButton, 1);

	struct button_label Button[4];
	Button[0] = CBEBouquetWidgetButtons[0];
	Button[1] = CBEBouquetWidgetButtons[1];
	Button[2] = CBEBouquetWidgetButtons[2];
	Button[3].button = NEUTRINO_ICON_BUTTON_BLUE;

	switch(blueFunction)
	{
		case beRename:
			Button[3].localename = _("Rename");
			break;
			
		case beHide:
			Button[3].localename = _("Hide");
			break;
			
		case beLock:
			Button[3].localename = _("Lock");
			break;
	}

	listBox->setFootButtons(Button, 4);
	
	listBox->setSelected(selected);
	
	widget->paint();
}

void CBEBouquetWidget::hide()
{
	widget->hide();
	
	//
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

int CBEBouquetWidget::exec(CTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::exec:\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();
	
	Bouquets = &CZapit::getInstance()->Bouquets;
	
	//
	paint();
	frameBuffer->blit();

	bouquetsChanged = false;

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
				if (bouquetsChanged)
				{
					int result = MessageBox(_("Bouquet Editor"), _("Do you want to save the changes?"), CMessageBox::mbrYes, CMessageBox::mbAll, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, CComponent::BORDER_ALL);

					switch( result )
					{
						case CMessageBox::mbrYes :
							loop = false;
							saveChanges();
						break;
						
						case CMessageBox::mbrNo :
							loop = false;
							discardChanges();
						break;
						
						case CMessageBox::mbrCancel :
							paint();
						break;
					}
				}
				else
				{
					loop = false;
				}
			}
			else if (state == beMoving)
			{
				cancelMoveBouquet();
			}
		}
		else if (msg == CRCInput::RC_page_up)
		{
			if (state == beDefault)
				listBox->scrollPageUp();
			else if(state == beMoving)
			{
				selected = listBox? listBox->getSelected() : 0;
				int next_selected = selected - listBox->getListMaxShow();

				if (next_selected < 0)
					next_selected = 0;

				internalMoveBouquet(selected, next_selected);
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

				if (next_selected > (int)Bouquets->size())
					next_selected = Bouquets->size();

				internalMoveBouquet(selected, next_selected);
			}
		}
		else if (msg == CRCInput::RC_up)
		{
			if (!(Bouquets->empty()))
			{
				if (state == beDefault)
				{
					listBox->scrollLineUp();
				}
				// FIXME: moving bqe up crashes
				/*
				else if (state == beMoving)
				{
					selected = listBox->getSelected();

					int prev_selected = selected;
					int next_selected = selected - 1;
					if (next_selected < 0)
						next_selected = 0;

					internalMoveBouquet(prev_selected, next_selected);
				}
				*/
			}
		}
		else if (msg == CRCInput::RC_down)
		{
			if (state == beDefault)
			{	
				listBox->scrollLineDown();
			}
			else if (state == beMoving)
			{
				selected = listBox->getSelected();

				int prev_selected = selected;
				int next_selected = selected + 1;
				
				if (next_selected > (int)Bouquets->size())
					next_selected = 0;

				internalMoveBouquet(prev_selected, next_selected);
			}
		}
		else if(msg == CRCInput::RC_red)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				deleteBouquet();
		}
		else if(msg == CRCInput::RC_green)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				addBouquet();
		}
		else if(msg == CRCInput::RC_yellow)
		{
			selected = listBox->getSelected();

			if (selected < Bouquets->size()) // Bouquets->size() might be 0
			{
				if (state == beDefault)
					beginMoveBouquet();

				paint();
			}
		}
		else if(msg == CRCInput::RC_blue)
		{
			selected = listBox->getSelected();
			
			if (selected < Bouquets->size()) // Bouquets->size() might be 0
			{
				if (state == beDefault)
				{
					switch (blueFunction)
					{
						case beRename:
							renameBouquet();
							break;
						case beHide:
							switchHideBouquet();
							break;
						case beLock:
							switchLockBouquet();
							break;
					}
				}
			}
		}
		else if(msg == CRCInput::RC_setup)
		{
			if (state == beDefault)
			{
				switch (blueFunction)
				{
					case beRename:
						blueFunction = beHide;
					break;
					case beHide:
						blueFunction = beLock;
					break;
					case beLock:
						blueFunction = beRename;
					break;
				}

				paint();
			}
		}
		else if(msg == CRCInput::RC_ok)
		{
			if (state == beDefault)
			{
				selected = listBox->getSelected();

				if (selected < Bouquets->size()) /* Bouquets->size() might be 0 */
				{
					CBEChannelWidget* channelWidget = new CBEChannelWidget((*Bouquets)[selected]->Name, selected);

					channelWidget->exec(this, "");
					if (channelWidget->hasChanged())
						bouquetsChanged = true;
					delete channelWidget;

					paint();
				}
			}
			else if (state == beMoving)
			{
				finishMoveBouquet();
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
				cancelMoveBouquet();
			}
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			//
			widget->refresh();
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
			// kein canceling...
		}

		frameBuffer->blit();	
	}
	
	hide();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	return res;
}

void CBEBouquetWidget::deleteBouquet()
{
	if (selected >= Bouquets->size()) /* Bouquets->size() might be 0 */
		return;
		
	if ((*Bouquets)[selected]->bWebTV)
		return;

	if (MessageBox(_("Delete"), (*Bouquets)[selected]->Name.c_str(), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) != CMessageBox::mbrYes)
	{
		paint();
		return;
	}

	CZapit::getInstance()->deleteBouquet(selected);
	Bouquets = &CZapit::getInstance()->Bouquets;
	if (selected >= Bouquets->size())
		selected = Bouquets->empty() ? 0 : (Bouquets->size() - 1);
	bouquetsChanged = true;

	paint();
}

void CBEBouquetWidget::addBouquet()
{
	std::string newName = inputName("", _("Name of bouquets"));
	
	if (!(newName.empty()))
	{
		CZapit::getInstance()->addBouquet(newName, true);
		Bouquets = &CZapit::getInstance()->Bouquets;
		selected = Bouquets->empty() ? 0 : (Bouquets->size() - 1);
		bouquetsChanged = true;
	}
	
	paint();
}

void CBEBouquetWidget::beginMoveBouquet()
{
	if ((*Bouquets)[selected]->bWebTV)
		return;
		
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEBouquetWidget::finishMoveBouquet()
{
	if ((*Bouquets)[selected]->bWebTV)
		return;
		
	state = beDefault;
	if (newPosition != origPosition)
	{
		Bouquets = &CZapit::getInstance()->Bouquets;
		bouquetsChanged = true;
	}

	paint();
}

void CBEBouquetWidget::cancelMoveBouquet()
{
	state = beDefault;
	internalMoveBouquet( newPosition, origPosition);
	bouquetsChanged = false;
}

void CBEBouquetWidget::internalMoveBouquet( unsigned int fromPosition, unsigned int toPosition)
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::internalMoveBouquet\n");
	
	if ((*Bouquets)[selected]->bWebTV)
		return;
	
	if ( (int) toPosition == -1 ) 
		return;
		
	if ( toPosition == Bouquets->size()) 
		return;

	CZapit::getInstance()->moveBouquet(fromPosition, toPosition);
	Bouquets = &CZapit::getInstance()->Bouquets;
	bouquetsChanged = true;

	selected = toPosition;
	newPosition = toPosition;

	paint();
}

void CBEBouquetWidget::renameBouquet()
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::renameBouquet\n");
	
	if ((*Bouquets)[selected]->bFav)
		return;
		
	if ((*Bouquets)[selected]->bWebTV)
		return;

	std::string newName = inputName((*Bouquets)[selected]->Name.c_str(), _("Name of bouquets"));
	
	if (newName != (*Bouquets)[selected]->Name)
	{
		CZapit::getInstance()->Bouquets[selected]->Name = newName;
		CZapit::getInstance()->Bouquets[selected]->bUser = true;

		bouquetsChanged = true;
	}

	paint();
}

void CBEBouquetWidget::switchHideBouquet()
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::switchHideBouquet\n");
	
	if ((*Bouquets)[selected]->bWebTV)
		return;
	
	bouquetsChanged = true;
	(*Bouquets)[selected]->bHidden = !(*Bouquets)[selected]->bHidden;

	paint();
}

void CBEBouquetWidget::switchLockBouquet()
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::switchLockBouquet\n");
	
	if ((*Bouquets)[selected]->bWebTV)
		return;
	
	bouquetsChanged = true;
	(*Bouquets)[selected]->bLocked = !(*Bouquets)[selected]->bLocked;

	paint();
}

std::string CBEBouquetWidget::inputName(const char * const defaultName, const char *const caption)
{
	//FIXME: max input it too long than bqt window width
	char Name[MAX_INPUT_CHARS + 1];

	strncpy(Name, defaultName, MAX_INPUT_CHARS + 1);

	CKeyboardInput *nameInput = new CKeyboardInput(caption, Name);
	nameInput->exec(this, "");
	delete nameInput;

	return std::string(Name);
}

void CBEBouquetWidget::saveChanges()
{
	CHintBox* hintBox= new CHintBox(_("Bouquet Editor"), _("Saving changes. Please be patient."), 500); // UTF-8
	hintBox->setBorderMode(CComponent::BORDER_ALL);
	hintBox->paint();
	
	CZapit::getInstance()->saveBouquets();
	CZapit::getInstance()->reinitChannels();
	
	hintBox->hide();
	delete hintBox;
}

void CBEBouquetWidget::discardChanges()
{
	CHintBox* hintBox= new CHintBox(_("Bouquet Editor"), _("Discarding changes. Please be patient."), 500); // UTF-8
	hintBox->setBorderMode(CComponent::BORDER_ALL);
	hintBox->paint();
	
	CZapit::getInstance()->restoreBouquets();
	
	hintBox->hide();
	delete hintBox;
}

////
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
	{ NEUTRINO_ICON_BUTTON_RED   , _("Delete"), 0 },
	{ NEUTRINO_ICON_BUTTON_GREEN , _("Add"), 0 },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Move"), 0 },
	{ NEUTRINO_ICON_BUTTON_BLUE  , _("TV/Radio"), 0 }
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
		
		listBox->paintMainFrame(true);
		
		//
		listBox->enablePaintHead();
		listBox->enablePaintDate();
		
		//
		listBox->enablePaintFoot();
		
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

int CBEChannelWidget::exec(CTarget* parent, const std::string &/*actionKey*/)
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

int CBEChannelSelectWidget::exec(CTarget* parent, const std::string& actionKey)
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

