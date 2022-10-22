/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_bouquets.cpp 2018/08/22 mohousch Exp $

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

#include <gui/bedit/bouqueteditor_bouquets.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>

#include <gui/bedit/bouqueteditor_channels.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>

//
#include <client/zapitclient.h>

#include <system/debug.h>


extern CBouquetManager * g_bouquetManager;	// defined in der zapit.cpp

#define BUTTONS_COUNT	3

const struct button_label CBEBouquetWidgetButtons[BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete")},
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Add")},
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Move")}
};

const struct button_label HButton = {NEUTRINO_ICON_BUTTON_SETUP, ""};

CBEBouquetWidget::CBEBouquetWidget()
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = -1;
	state = beDefault;
	blueFunction = beRename;
	
	Bouquets = &g_bouquetManager->Bouquets;

	widget = NULL;
	listBox = NULL;
	item = NULL;
	
	sec_timer_id = 0;

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
	
	/*
	if (CNeutrinoApp::getInstance()->widget_exists("bqeditbq"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("bqeditbq");
		listBox = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		//listBox->enablePaintHead();
		//listBox->enablePaintDate();
	
		widget->addWidgetItem(listBox);
	}
	*/	
}

CBEBouquetWidget::~CBEBouquetWidget()
{
}

void CBEBouquetWidget::paint()
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::paint:\n");

	listBox->clear();

	for (unsigned int count = 0; count < Bouquets->size(); count++)
	{
		if (!(*Bouquets)[count]->bWebTV)
		{
			item = new ClistBoxItem((*Bouquets)[count]->bFav ? _("My Favorites") : (*Bouquets)[count]->Name.c_str());

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
	listBox->setTitle(_("Bouquets"));
	//listBox->enablePaintHead();
	//listBox->enablePaintDate();
	listBox->setHeadButtons(&HButton, 1);
	
	//
	listBox->enablePaintFoot();

	struct button_label Button[4];
	Button[0] = CBEBouquetWidgetButtons[0];
	Button[1] = CBEBouquetWidgetButtons[1];
	Button[2] = CBEBouquetWidgetButtons[2];
	Button[3].button = NEUTRINO_ICON_BUTTON_BLUE;

	switch( blueFunction)
	{
		case beRename:
			Button[3].localename = "Rename";
			break;
			
		case beHide:
			Button[3].localename = "Hide";
			break;
			
		case beLock:
			Button[3].localename = "Lock";
			break;
	}

	listBox->setFootButtons(Button, 4);
	
	widget->paint();
}

void CBEBouquetWidget::hide()
{
	widget->hide();
}

int CBEBouquetWidget::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CBEBouquetWidget::exec:\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();
	
	Bouquets = &g_bouquetManager->Bouquets;
	
	//
	if (CNeutrinoApp::getInstance()->widget_exists("bqeditbq"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("bqeditbq");
		listBox = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		listBox->enablePaintHead();
		listBox->enablePaintDate();
	
		widget->addWidgetItem(listBox);
	}	
	
	//
	paint();
	frameBuffer->blit();

	bouquetsChanged = false;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		if ((msg == RC_timeout) || (msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
		{
			if (state == beDefault)
			{
				if (bouquetsChanged)
				{
					int result = MessageBox(_("Bouquet Editor"), _("Do you want to save the changes?"), mbrYes, mbAll, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);

					switch( result )
					{
						case mbrYes :
							loop = false;
							saveChanges();
						break;
						
						case mbrNo :
							loop = false;
							discardChanges();
						break;
						
						case mbrCancel :
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
		else if (msg == RC_page_up)
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
		else if (msg == RC_page_down)
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
		else if (msg == RC_up)
		{
			if (!(Bouquets->empty()))
			{
				if (state == beDefault)
				{
					listBox->scrollLineUp();
				}
				else if (state == beMoving)
				{
					selected = listBox->getSelected();

					int prev_selected = selected;
					int next_selected = selected - 1;
					if (next_selected < 0)
						next_selected = 0;

					internalMoveBouquet(prev_selected, next_selected);
				}
			}
		}
		else if (msg == RC_down)
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
					next_selected = Bouquets->size();

				internalMoveBouquet(prev_selected, next_selected);
			}
		}
		else if(msg == RC_red)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				deleteBouquet();
		}
		else if(msg == RC_green)
		{
			selected = listBox->getSelected();

			if (state == beDefault)
				addBouquet();
		}
		else if(msg == RC_yellow)
		{
			selected = listBox->getSelected();

			if (selected < Bouquets->size()) // Bouquets->size() might be 0
			{
				if (state == beDefault)
					beginMoveBouquet();

				paint();
			}
		}
		else if(msg == RC_blue)
		{
			if (selected < Bouquets->size()) // Bouquets->size() might be 0
			{
				selected = listBox->getSelected();

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
		else if(msg == RC_setup)
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
		else if(msg == RC_ok)
		{
			if (state == beDefault)
			{
				selected = listBox->getSelected();

				if (selected < Bouquets->size()) /* Bouquets->size() might be 0 */
				{
					CBEChannelWidget* channelWidget = new CBEChannelWidget((*Bouquets)[selected]->bFav ? _("My Favorites") : (*Bouquets)[selected]->Name, selected);

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

	if (MessageBox(_("Delete"), (*Bouquets)[selected]->bFav ? _("Favorites") : (*Bouquets)[selected]->Name.c_str(), mbrNo, mbYes | mbNo) != mbrYes)
	{
		paint();
		return;
	}

	g_bouquetManager->deleteBouquet(selected);
	Bouquets = &g_bouquetManager->Bouquets;
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
		g_bouquetManager->addBouquet(newName, true);
		Bouquets = &g_bouquetManager->Bouquets;
		selected = Bouquets->empty() ? 0 : (Bouquets->size() - 1);
		bouquetsChanged = true;
	}
	
	paint();
}

void CBEBouquetWidget::beginMoveBouquet()
{
	state = beMoving;
	origPosition = selected;
	newPosition = selected;
}

void CBEBouquetWidget::finishMoveBouquet()
{
	state = beDefault;
	if (newPosition != origPosition)
	{
		Bouquets = &g_bouquetManager->Bouquets;
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
	if ( (int) toPosition == -1 ) 
		return;
	if ( toPosition == Bouquets->size()) 
		return;

	g_bouquetManager->moveBouquet(fromPosition, toPosition);
	Bouquets = &g_bouquetManager->Bouquets;
	bouquetsChanged = true;

	selected = toPosition;
	newPosition = toPosition;

	paint();
}

void CBEBouquetWidget::renameBouquet()
{
	if ((*Bouquets)[selected]->bFav)
		return;

	std::string newName = inputName((*Bouquets)[selected]->Name.c_str(), _("Name of bouquets"));
	if (newName != (*Bouquets)[selected]->Name)
	{
		g_bouquetManager->Bouquets[selected]->Name = newName;
		g_bouquetManager->Bouquets[selected]->bUser = true;

		bouquetsChanged = true;
	}

	paint();
}

void CBEBouquetWidget::switchHideBouquet()
{
	bouquetsChanged = true;
	(*Bouquets)[selected]->bHidden = !(*Bouquets)[selected]->bHidden;

	paint();
}

void CBEBouquetWidget::switchLockBouquet()
{
	bouquetsChanged = true;
	(*Bouquets)[selected]->bLocked = !(*Bouquets)[selected]->bLocked;

	paint();
}

std::string CBEBouquetWidget::inputName(const char * const defaultName, const char* const caption)
{
	//FIXME: max input it too long than bqt window width
	char Name[MAX_INPUT_CHARS + 1];

	strncpy(Name, defaultName, MAX_INPUT_CHARS + 1);

	CStringInputSMS *nameInput = new CStringInputSMS(caption, Name);
	nameInput->exec(this, "");
	delete nameInput;

	return std::string(Name);
}

void CBEBouquetWidget::saveChanges()
{
	CHintBox* hintBox= new CHintBox(_("Bouquet Editor"), _("Do you want to save the changes?"), 500); // UTF-8
	hintBox->setBorderMode(BORDER_ALL);
	hintBox->paint();
	
	g_Zapit->saveBouquets();
	g_Zapit->reinitChannels();
	
	hintBox->hide();
	delete hintBox;
}

void CBEBouquetWidget::discardChanges()
{
	CHintBox* hintBox= new CHintBox(_("Bouquet Editor"), _("Discarding changes. Please be patient."), 500); // UTF-8
	hintBox->setBorderMode(BORDER_ALL);
	hintBox->paint();
	
	g_Zapit->restoreBouquets();
	
	hintBox->hide();
	delete hintBox;
}


