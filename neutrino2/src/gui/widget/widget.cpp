/*
	$Id: widget.cpp 12.03.2020 mohousch Exp $


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

#include <unistd.h> //acces
#include <cctype>

#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>
#include <gui/widget/widget.h>
#include <gui/widget/framebox.h>
#include <system/debug.h>


CWidget::CWidget(const int x, const int y, const int dx, const int dy)
{
	frameBuffer = CFrameBuffer::getInstance();

	mainFrameBox.iX = x;
	mainFrameBox.iY = y;
	mainFrameBox.iWidth = dx;
	mainFrameBox.iHeight = dy;

	savescreen = false;
	background = NULL;
	
	//
	enablePos = false;
	menu_position = MENU_POSITION_CENTER;

	timeout = 0;
	selected = -1;
	sec_timer_interval = 1;

	//
	paintframe = false;
	backgroundColor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	radius = NO_RADIUS;
	corner = CORNER_NONE;

	actionKey = "";
	
	//
	name = "";
}

CWidget::CWidget(CBox *position)
{
	frameBuffer = CFrameBuffer::getInstance();

	mainFrameBox = *position;

	savescreen = false;
	background = NULL;
	
	//
	enablePos = false;
	menu_position = MENU_POSITION_CENTER;

	timeout = 0;
	selected = -1;
	sec_timer_interval = 1;

	//
	paintframe = false;
	backgroundColor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	radius = RADIUS_MID;
	corner = CORNER_ALL;

	actionKey = "";
	
	//
	name = "";
}

CWidget::~CWidget()
{
	dprintf(DEBUG_INFO, "CWidget:: del\n");

	// WIDGETITEMS
	if (hasWidgetItem())
	{
		items.clear();
	}
		
	// CCITEMS
	if (hasCCItem())
	{
		CCItems.clear();
	}
}

void CWidget::addWidgetItem(CWidgetItem *widgetItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	
	items.push_back(widgetItem);
	widgetItem->setParent(this);
}

void CWidget::removeWidgetItem(long pos)
{
	items.erase(items.begin() + pos); 
}

//
void CWidget::addCCItem(CComponent* CCItem)
{
	CCItems.push_back(CCItem);
}

void CWidget::removeCCItem(long pos)
{
	CCItems.erase(CCItems.begin() + pos); 
}

//
void CWidget::initFrames()
{
	dprintf(DEBUG_DEBUG, "CWidget::initFrames\n");
	
	// sanity check
	if(mainFrameBox.iHeight > ((int)frameBuffer->getScreenHeight(true)))
		mainFrameBox.iHeight = frameBuffer->getScreenHeight(true);

	// sanity check
	if(mainFrameBox.iWidth > (int)frameBuffer->getScreenWidth(true))
		mainFrameBox.iWidth = frameBuffer->getScreenWidth(true);
		
	// menu position (x/y)
	if (enablePos)
	{
		if(menu_position == MENU_POSITION_CENTER)
		{
			mainFrameBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - mainFrameBox.iWidth ) >> 1 );
			mainFrameBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - mainFrameBox.iHeight) >> 1 );
		}
		else if(menu_position == MENU_POSITION_LEFT)
		{
			mainFrameBox.iX = frameBuffer->getScreenX();
			mainFrameBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - mainFrameBox.iHeight) >> 1 );
		}
		else if(menu_position == MENU_POSITION_RIGHT)
		{
			mainFrameBox.iX = frameBuffer->getScreenX() + frameBuffer->getScreenWidth() - mainFrameBox.iWidth;
			mainFrameBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - mainFrameBox.iHeight) >> 1 );
		}
	}

	if(savescreen) 
		saveScreen();
}

void CWidget::paintWidgetItems()
{
	dprintf(DEBUG_INFO, "CWidget:: paintWidgetItems\n");

	for (unsigned int i = 0; i < (unsigned int)items.size(); i++)
	{
		if( (items[i]->isSelectable()) && (selected == -1)) 
		{
			selected = i;
		}

		items[i]->paint();
	}

}

//
void CWidget::paintCCItems()
{
	dprintf(DEBUG_INFO, "CWidget::paintCCItems:\n");

	for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
	{
		CCItems[count]->paint();
	}
}

void CWidget::paint()
{
	dprintf(DEBUG_NORMAL, "CWidget:: paint (%s)\n", name.c_str());
	
	//
	initFrames();

	// paint mainFrame
	if (paintframe)
		frameBuffer->paintBoxRel(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, backgroundColor, radius, corner, gradient);

	// WidgetItems
	if (hasWidgetItem())
		paintWidgetItems();
	
	// CCItems	
	if (hasCCItem())
		paintCCItems();
}

void CWidget::saveScreen()
{
	dprintf(DEBUG_INFO, "CWidget::saveScreen\n");
	
	if(!savescreen)
		return;

	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[mainFrameBox.iWidth*mainFrameBox.iHeight];
	
	if(background)
	{
		frameBuffer->saveScreen(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, background);
	}
}

void CWidget::restoreScreen()
{
	dprintf(DEBUG_INFO, "CWidget::restoreScreen\n");
	
	if(background) 
	{
		if(savescreen)
			frameBuffer->restoreScreen(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, background);
	}
}

void CWidget::enableSaveScreen()
{
	savescreen = true;
	
	if(!savescreen && background) 
	{
		delete[] background;
		background = NULL;
	}
}

void CWidget::hide()
{
	dprintf(DEBUG_NORMAL, "CWidget:: hide (%s)\n", name.c_str());

	//
	if (hasCCItem())
	{
		for(unsigned int i = 0; i < (unsigned int)CCItems.size(); i++)
		{
			if (CCItems[i]->getCCType() == CC_PIG)
			{
				CCItems[i]->hide();
				break;
			}
		}
	}	

	if( savescreen && background)
	{
		restoreScreen();
	}
	else
	{
		//if (paintframe)
		frameBuffer->paintBackgroundBoxRel(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight);
	}

	frameBuffer->blit();
}

void CWidget::addKey(neutrino_msg_t key, CMenuTarget *menue, const std::string & action)
{
	dprintf(DEBUG_DEBUG, "CWidget::addKey: %s\n", action.c_str());
	
	keyActionMap[key].menue = menue;
	keyActionMap[key].action = action;
}

int CWidget::exec(CMenuTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CWidget:: exec: (%s)\n", name.c_str());

	retval = RETURN_REPAINT;
	pos = 0;
	exit_pressed = false;

	if (parent)
		parent->hide();

	// set in focus
	if (hasWidgetItem() && items.size() > 1)
	{
		for (unsigned int i = 0; i < (unsigned int)items.size(); i++)
		{
			if((items[i]->isSelectable()) && (items[i]->inFocus == true))
			{
				selected = i;
				break;
			}
		}
	}

	paint();
	CFrameBuffer::getInstance()->blit();

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(sec_timer_interval*1000*1000, false);
	
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);

	//control loop
	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		
		int handled = false;

		//dprintf(DEBUG_DEBUG, "CWidget::exec: msg:%s\n", CRCInput::getSpecialKeyName(msg));

		if ( msg <= RC_MaxRC ) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);

			// keymap
			std::map<neutrino_msg_t, keyAction>::iterator it = keyActionMap.find(msg);
			
			if (it != keyActionMap.end()) 
			{
				actionKey = it->second.action;

				if (it->second.menue != NULL)
				{
					int rv = it->second.menue->exec(this, it->second.action);

					//FIXME:review this
					switch ( rv ) 
					{
						case RETURN_EXIT_ALL:
							retval = RETURN_EXIT_ALL; //fall through
						case RETURN_EXIT:
							msg = RC_timeout;
							break;
						case RETURN_REPAINT:
							hide();
							paint();
							break;
					}
				}
				else
				{
					selected = -1;
					handled = true;
					//FIXME: TEST
					exit_pressed = true;

					break;
				}

				frameBuffer->blit();
				continue;
			}
			
			// handle directKey
			onDirectKeyPressed(msg);
		}

		if (!handled) 
		{
			if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
			{
				// refresh items
				for (unsigned int i = 0; i < (unsigned int)items.size(); i++)
				{
					if (items[i]->update())
					{
						items[i]->refresh();
					}
				}
				
				// refresh CCItems
				for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
				{
					if (CCItems[count]->update())
					{
						CCItems[count]->refresh();
					}
				}
			} 

			switch (msg) 
			{
				case (NeutrinoMessages::EVT_TIMER):
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = RETURN_EXIT_ALL;
						msg = RC_timeout;
					}
					break;
					
				//
				case (RC_up):
					onUpKeyPressed();
					break;

				case (RC_down):
					onDownKeyPressed();
					break;

				case (RC_right):
					onRightKeyPressed();
					break;

				case (RC_left):
					onLeftKeyPressed();
					break;

				case (RC_page_up):
					onPageUpKeyPressed();
					break;

				case (RC_page_down):
					onPageDownKeyPressed();
					break;

				case (RC_yellow):
					onYellowKeyPressed();
					break;

				case (RC_home):
					onHomeKeyPressed();
					break;

				case (RC_ok):
					onOKKeyPressed();
					break;
				
				//	
				case (RC_timeout):
					exit_pressed = true;
					selected = -1;
					break;

				default:
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = RETURN_EXIT_ALL;
						msg = RC_timeout;
					}
			}

			if ( msg <= RC_MaxRC )
			{
				// recalculate timeout for RC-Tasten
				timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);
			}
		}
		
		frameBuffer->blit();
	}
	while ( msg != RC_timeout );

	dprintf(DEBUG_INFO, "CWidget: retval: (%d) selected:%d\n", retval, selected);
	
	hide();

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;	

	// vfd
	if(!parent)
	{
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	}
	
	return retval;
}

// events
void CWidget::onOKKeyPressed(neutrino_msg_t _msg)
{
	dprintf(DEBUG_INFO, "CWidget::onOKKeyPressed:0x%x\n", _msg);
	
	if(hasWidgetItem() && selected >= 0)
	{
		if (items[selected]->hasItem() && items[selected]->isSelectable())
		{
			int rv = items[selected]->oKKeyPressed(this, _msg);

			actionKey = items[selected]->getActionKey();

			//FIXME:review this
			switch ( rv ) 
			{
				case RETURN_EXIT_ALL:
					retval = RETURN_EXIT_ALL; //fall through
				case RETURN_EXIT:
					msg = RC_timeout;
					break;
				case RETURN_REPAINT:
					hide();
					paint();
					break;
			}
		}
	}
}

void CWidget::onHomeKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onHomeKeyPressed\n");
	
	exit_pressed = true;
	dprintf(DEBUG_INFO, "CWidget::onHomeKeyPressed: exit_pressed\n");
	msg = RC_timeout;
	
	if (hasWidgetItem())
	{
		for (unsigned int count = 0; count < (unsigned int)items.size(); count++) 
		{
			items[count]->homeKeyPressed();
		}
	}

	selected = -1;
}

void CWidget::onYellowKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onYellowKeyPressed\n");
	
	if(hasWidgetItem())
	{
		for (unsigned int count = 1; count < (unsigned int)items.size(); count++) 
		{
			pos = (selected + count)%items.size();

			CWidgetItem * item = items[pos];

			if(item->isSelectable() && item->hasItem())
			{
				items[selected]->setOutFocus();
				items[selected]->paint();

				selected = pos;

				item->setOutFocus(false);
				item->paint();

				break;
			}
		}
	}
}

void CWidget::onUpKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onUpKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
#if 0
		if( ((items[selected]->widgetItem_type == WIDGETITEM_FRAMEBOX) ) /*|| ((items[selected]->widgetItem_type == WIDGETITEM_LISTBOX) && (items[selected]->getWidgetType() == WIDGET_TYPE_FRAME) )*/ )
		{
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = (selected - count)%items.size();

				if ( pos < 0 )
					pos += items.size();

				CWidgetItem * item = items[pos];

				if(item->isSelectable() && item->hasItem())
				{
					items[selected]->setOutFocus();
					items[selected]->paint();

					item->setOutFocus(false);
					item->paint();

					selected = pos;

					break;
				}
			}
		}
		else
#endif
			items[selected]->scrollLineUp();
	}
}

void CWidget::onDownKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onDownKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
#if 0
		if( ((items[selected]->widgetItem_type == WIDGETITEM_FRAMEBOX) ) /*|| ((items[selected]->widgetItem_type == WIDGETITEM_LISTBOX) && (items[selected]->getWidgetType() == WIDGET_TYPE_FRAME) )*/ )
		{
			//onYellowKeyPressed();
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = (selected + count)%items.size();

				if (pos >= (int)items.size())
					pos = 0;

				printf("CWidget::onDownKeyPressed: (pos:%d) (selected:%d)\n", pos, selected);

				CWidgetItem * item = items[pos];

				if(item->isSelectable())
				{
					items[selected]->setOutFocus();
					items[selected]->paint();

					item->setOutFocus(false);
					item->paint();

					selected = pos;

					break;
				}
			}
		}
		else
#endif
		{
			items[selected]->scrollLineDown();
		}
	}
}

void CWidget::onRightKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onRightKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
	/*
		if( (items[selected]->widgetItem_type == WIDGETITEM_LISTBOX) && ( (items[selected]->getWidgetType() != WIDGET_TYPE_FRAME) && (items[selected]->getWidgetType() != WIDGET_TYPE_EXTENDED)) )
		{
			for (unsigned int count = 1; count < (unsigned int)items.size(); count++) 
			{
				pos = (selected + count)%items.size();

				CWidgetItem * item = items[pos];

				if(item->isSelectable() && item->hasItem())
				{
					items[selected]->setOutFocus();
					items[selected]->paint();

					selected = pos;

					item->setOutFocus(false);
					item->paint();

					break;
				}
			}
		}
		else
		*/
			items[selected]->swipRight();
	}
}

void CWidget::onLeftKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onLeftKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
	/*
		if( (items[selected]->widgetItem_type == WIDGETITEM_LISTBOX) && ((items[selected]->getWidgetType() != WIDGET_TYPE_FRAME) && (items[selected]->getWidgetType() != WIDGET_TYPE_EXTENDED)) )
		{
			for (unsigned int count = 1; count < (unsigned int)items.size(); count++) 
			{
				pos = (selected - count)%items.size();

				CWidgetItem * item = items[pos];

				if(item->isSelectable() && item->hasItem())
				{
					items[selected]->setOutFocus();
					items[selected]->paint();

					selected = pos;

					item->setOutFocus(false);
					item->paint();

					break;
				}
			}
		}
		else
		*/
			items[selected]->swipLeft();
	}
}

void CWidget::onPageUpKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onPageUpKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
		items[selected]->scrollPageUp();
	}
}

void CWidget::onPageDownKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onPageDownKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
		items[selected]->scrollPageDown();
	}
}

//
void CWidget::onDirectKeyPressed(neutrino_msg_t _msg)
{
	dprintf(DEBUG_DEBUG, "CWidget::onDirectKeyPressed\n");
	
	if(hasWidgetItem() && selected >= 0)
	{
		items[selected]->directKeyPressed(_msg);
	}
}

//
CWidgetItem* CWidget::getWidgetItem(const int type, const std::string& name)
{
	dprintf(DEBUG_INFO, "CWidget::getWidgetItem: (%d) (%s)\n", type, name.c_str());
	
	CWidgetItem* ret = NULL;
	
	for (unsigned int count = 0; count < (unsigned int)items.size(); count++) 
	{
		switch (type)
		{
			case WIDGETITEM_LISTBOX:
				if ( (items[count]->widgetItem_type == WIDGETITEM_LISTBOX) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_WINDOW:
				if ( (items[count]->widgetItem_type == WIDGETITEM_WINDOW) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_HEAD:
				if ( (items[count]->widgetItem_type == WIDGETITEM_HEAD) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_FOOT:
				if ( (items[count]->widgetItem_type == WIDGETITEM_FOOT) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_TEXTBOX:
				if ( (items[count]->widgetItem_type == WIDGETITEM_TEXTBOX) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_FRAMEBOX:
				if ( (items[count]->widgetItem_type == WIDGETITEM_FRAMEBOX) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			case WIDGETITEM_LISTFRAME:
				if ( (items[count]->widgetItem_type == WIDGETITEM_LISTFRAME) && (items[count]->widgetItem_name == name) )
					ret = items[count]; 
				break;
				
			default:
				break;
		}
	}
	
	return ret;
}

//
CComponent* CWidget::getCCItem(const int type, const std::string& name)
{
	dprintf(DEBUG_INFO, "CWidget::getCCItem: (%d) (%s)\n", type, name.c_str());
	
	CComponent* ret = NULL;
	
	for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
	{
		switch (type)
		{
			case CC_ICON:
				if ( (CCItems[count]->cc_type == CC_ICON) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_IMAGE:
				if ( (CCItems[count]->cc_type == CC_IMAGE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_LABEL:
				if ( (CCItems[count]->cc_type == CC_LABEL) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_TEXT:
				if ( (CCItems[count]->cc_type == CC_TEXT) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_BUTTON:
				if ( (CCItems[count]->cc_type == CC_BUTTON) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_HLINE:
				if ( (CCItems[count]->cc_type == CC_HLINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_VLINE:
				if ( (CCItems[count]->cc_type == CC_VLINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_FRAMELINE:
				if ( (CCItems[count]->cc_type == CC_FRAMELINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_PIG:
				if ( (CCItems[count]->cc_type == CC_PIG) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_GRID:
				if ( (CCItems[count]->cc_type == CC_GRID) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_TIME:
				if ( (CCItems[count]->cc_type == CC_TIME) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_COUNTER:
				if ( (CCItems[count]->cc_type == CC_COUNTER) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_SCROLLBAR:
				if ( (CCItems[count]->cc_type == CC_SCROLLBAR) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_PROGRESSBAR:
				if ( (CCItems[count]->cc_type == CC_PROGRESSBAR) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_DETAILSLINE:
				if ( (CCItems[count]->cc_type == CC_DETAILSLINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CC_SLIDER:
				if ( (CCItems[count]->cc_type == CC_SLIDER) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			default:
				break;
		}
	}
	
	return ret;
}



