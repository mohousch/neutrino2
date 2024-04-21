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
#include <neutrino2.h>

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
	//
	savescreen = false;
	background = NULL;
	//
	menu_position = MENU_POSITION_NONE;
	//
	timeout = g_settings.timing_menu;
	sec_timer_id = 0;
	sec_timer_interval = 1;
	//
	selected = -1;
	//
	paintframe = false;
	backgroundColor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = GRADIENT_COLOR2TRANSPARENT;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	//
	current_page = 0;
	total_pages = 1;
	//
	actionKey = "";
	//
	name = "";
	//
	CCItems.clear();
	
	////
	dprintf(DEBUG_INFO, "CWidget::CWidget: x:%d y:%d w:%d h:%d\n", mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight);
}

CWidget::CWidget(CBox *position)
{
	frameBuffer = CFrameBuffer::getInstance();
	//
	mainFrameBox = *position;
	//
	savescreen = false;
	background = NULL;
	//
	menu_position = MENU_POSITION_NONE;
	//
	timeout = g_settings.timing_menu;
	sec_timer_id = 0;
	sec_timer_interval = 1;
	//
	selected = -1;
	//
	paintframe = false;
	backgroundColor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = GRADIENT_COLOR2TRANSPARENT;
	radius = RADIUS_MID;
	corner = CORNER_ALL;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	//
	current_page = 0;
	total_pages = 1;
	//
	actionKey = "";
	name = "";
	//
	CCItems.clear();
	
	////
	dprintf(DEBUG_INFO, "CWidget::CWidget: x:%d y:%d w:%d h:%d\n", mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight);
}

CWidget::~CWidget()
{
	dprintf(DEBUG_NORMAL, "CWidget::del (%s)\n", name.c_str());
	
	//
	if (background)
	{
		delete [] background;
		background = NULL;
	}
	
	//
	for (unsigned int count = 0; count < CCItems.size(); count++)
	{
		CComponent *cCItem = CCItems[count];
		
		delete cCItem;
		cCItem = NULL;
	}
	
	//
	CCItems.clear();
}

//
void CWidget::addCCItem(CComponent* CCItem, const bool defaultselected)
{
	if (CCItem)
	{
		if (CCItem->isSelectable())
		{
			if (defaultselected)
				selected = CCItems.size();
		}
			 
		CCItems.push_back(CCItem);
		
		CCItem->setParent(this);
	}
}

void CWidget::removeCCItem(long pos)
{
	CCItems.erase(CCItems.begin() + pos); 
}

//
void CWidget::initFrames()
{
	dprintf(DEBUG_INFO, "CWidget::initFrames\n");
	
	// sanity check
	if(mainFrameBox.iHeight >= ((int)frameBuffer->getScreenHeight(true)))
		mainFrameBox.iHeight = frameBuffer->getScreenHeight(true);

	if(mainFrameBox.iWidth >= (int)frameBuffer->getScreenWidth(true))
		mainFrameBox.iWidth = frameBuffer->getScreenWidth(true);
		
	// menu position (x/y)
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
	
	dprintf(DEBUG_INFO, "CWidget::initFrames: x:%d y:%d w:%d h:%d\n", mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight);
}

//
void CWidget::paintCCItems()
{
	dprintf(DEBUG_INFO, "CWidget::paintCCItems:\n");

	if (hasCCItem())
	{
		for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
		{
			if( (CCItems[count]->isSelectable()) && (selected == -1)) 
			{
				selected = count;
			}
		
			CCItems[count]->paint();
		}
	}
}

void CWidget::paint()
{
	dprintf(DEBUG_NORMAL, "CWidget::paint (%s)\n", name.c_str());
	
	//
	initFrames();

	// paint mainFrame	
	if (paintframe)
	{
		// mainframe
		if (borderMode == CComponent::BORDER_NO)
			frameBuffer->paintBoxRel(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, backgroundColor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		else
		{
			// border
			frameBuffer->paintBoxRel(mainFrameBox.iX - 2, mainFrameBox.iY - 2, mainFrameBox.iWidth + 4, mainFrameBox.iHeight + 4, borderColor, radius, corner);
			
			// mainframe
			frameBuffer->paintBoxRel(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, backgroundColor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
	}
	
	// CCItems	
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

	background = new fb_pixel_t[borderMode? (mainFrameBox.iWidth + 4)* (mainFrameBox.iHeight + 4) : mainFrameBox.iWidth*mainFrameBox.iHeight];
	
	if(background)
	{
		if (borderMode)
			frameBuffer->saveScreen(mainFrameBox.iX - 2, mainFrameBox.iY - 2, mainFrameBox.iWidth + 4, mainFrameBox.iHeight + 4, background);
		else
			frameBuffer->saveScreen(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, background);		
	}
}

void CWidget::restoreScreen()
{
	dprintf(DEBUG_INFO, "CWidget::restoreScreen\n");
	
	if(savescreen && background) 
	{
		if (borderMode)
			frameBuffer->restoreScreen(mainFrameBox.iX - 2, mainFrameBox.iY - 2, mainFrameBox.iWidth + 4, mainFrameBox.iHeight + 4, background);
		else
			frameBuffer->restoreScreen(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight, background);
	}
}

//
void CWidget::enableSaveScreen()
{
	savescreen = true;
	
	//
	initFrames();
	
	//
	saveScreen();
}

void CWidget::hide()
{
	dprintf(DEBUG_NORMAL, "CWidget::hide (%s)\n", name.c_str());

	//
	if (hasCCItem())
	{
/*
		for(unsigned int i = 0; i < (unsigned int)CCItems.size(); i++)
		{
			CCItems[i]->stopRefresh();
		}
*/
		
		//
		for(unsigned int i = 0; i < (unsigned int)CCItems.size(); i++)
		{
			if ( (CCItems[i]->getCCType() == CComponent::CC_PIG) || (CCItems[i]->getCCType() == CComponent::CC_SPINNER) )
			{
				CCItems[i]->hide();
				//break;
			}
		}
	}	

	if( savescreen && background)
	{
		restoreScreen();
	}
	else
	{
		if (borderMode)
			frameBuffer->paintBackgroundBoxRel(mainFrameBox.iX - 2, mainFrameBox.iY - 2, mainFrameBox.iWidth + 4, mainFrameBox.iHeight + 4);
		else
			frameBuffer->paintBackgroundBoxRel(mainFrameBox.iX, mainFrameBox.iY, mainFrameBox.iWidth, mainFrameBox.iHeight);
	}

	frameBuffer->blit();
}

void CWidget::addKey(neutrino_msg_t key, CMenuTarget *menue, const std::string & action)
{
	dprintf(DEBUG_INFO, "CWidget::addKey: %s\n", action.c_str());
	
	keyActionMap[key].menue = menue;
	keyActionMap[key].action = action;
}

int CWidget::exec(CMenuTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CWidget::exec: (%s)\n", name.c_str());

	retval = RETURN_REPAINT;
	pos = 0;
	exit_pressed = false;
	bool show = true;
	
	bool needToBlit = false;

	if (parent)
		parent->hide();

	// set in focus
	if (hasCCItem() && CCItems.size())
	{
		for (unsigned int i = 0; i < (unsigned int)CCItems.size(); i++)
		{
			if((CCItems[i]->isSelectable()) && (CCItems[i]->inFocus == true))
			{
				selected = i;
				break;
			}
		}
	}

	initFrames();
	saveScreen();
	paint();
	CFrameBuffer::getInstance()->blit();

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(sec_timer_interval*1000*1000, false);
	
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);

	//control loop
	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		
		int handled = false;

		if ( msg <= CRCInput::RC_MaxRC ) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);
			
			needToBlit = true;

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
							msg = CRCInput::RC_timeout;
							break;
						case RETURN_REPAINT:
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
				needToBlit = true;
				
				show = !show;
				refresh(show);
			} 

			switch (msg) 
			{
				case (NeutrinoMessages::EVT_TIMER):
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
					break;
					
				//
				case (CRCInput::RC_up):
					onUpKeyPressed();
					break;

				case (CRCInput::RC_down):
					onDownKeyPressed();
					break;

				case (CRCInput::RC_right):
					onRightKeyPressed();
					break;

				case (CRCInput::RC_left):
					onLeftKeyPressed();
					break;

				case (CRCInput::RC_page_up):
					onPageUpKeyPressed();
					break;

				case (CRCInput::RC_page_down):
					onPageDownKeyPressed();
					break;

				case (CRCInput::RC_yellow):
					onYellowKeyPressed();
					break;

				case (CRCInput::RC_home):
				case (CRCInput::RC_setup):
					onHomeKeyPressed();
					break;

				case (CRCInput::RC_ok):
					onOKKeyPressed();
					break;
				
				//	
				case (CRCInput::RC_timeout):
					exit_pressed = true;
					selected = -1;
					break;

				default:
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
			}

			if ( msg <= CRCInput::RC_MaxRC )
			{
				// recalculate timeout for RC-Tasten
				timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);
			}
		}
		
		if (needToBlit)
		{
			frameBuffer->blit();
			needToBlit = false;
		}
	}
	while ( msg != CRCInput::RC_timeout );

	dprintf(DEBUG_INFO, "CWidget: retval: (%d) selected:%d\n", retval, selected);
	
	hide();

	//
	if (sec_timer_id)
	{
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}

	// vfd
	if(!parent)
	{
		CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	}
	
	return retval;
}

void CWidget::refresh(bool show)
{				
	// refresh CCItems
	if (hasCCItem())
	{
		for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
		{
			if (CCItems[count]->update())
			{
				CCItems[count]->refresh(show);
			}
		}
	}
}

// events
void CWidget::onOKKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onOKKeyPressed:\n");
	
	if(hasCCItem() && selected >= 0)
	{
		if (CCItems[selected]->hasItem() && CCItems[selected]->isSelectable())
		{
			int rv = CCItems[selected]->oKKeyPressed(this);

			actionKey = CCItems[selected]->getActionKey();	// for lua

			//
			switch ( rv ) 
			{
				case RETURN_EXIT_ALL:
					retval = RETURN_EXIT_ALL; //fall through
				case RETURN_EXIT:
					msg = CRCInput::RC_timeout;
					break;
				case RETURN_REPAINT:
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
	msg = CRCInput::RC_timeout;
	
	if (hasCCItem())
	{
		for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
		{
			CCItems[count]->homeKeyPressed();
		}
	}

	selected = -1;
}

void CWidget::onYellowKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onYellowKeyPressed\n");
	
	if(hasCCItem())
	{
		for (unsigned int count = 1; count < (unsigned int)CCItems.size(); count++) 
		{
			pos = (selected + count)%CCItems.size();

			CComponent * item = CCItems[pos];

			if(item->isSelectable() && item->hasItem())
			{
				CCItems[selected]->setOutFocus();
				CCItems[selected]->paint();

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
	
	if(hasCCItem() && selected >= 0)
	{
		CCItems[selected]->scrollLineUp();
	}
}

void CWidget::onDownKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onDownKeyPressed\n");
	
	if(hasCCItem() && selected >= 0)
	{
		CCItems[selected]->scrollLineDown();
	}
}

void CWidget::onRightKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onRightKeyPressed\n");
	
	if(hasCCItem() && selected >= 0)
	{
		int rv = CCItems[selected]->swipRight();
		
		actionKey = CCItems[selected]->getActionKey();	// lua

		//
		switch ( rv ) 
		{
			case RETURN_EXIT_ALL:
				retval = RETURN_EXIT_ALL; //fall through
			case RETURN_EXIT:
				msg = CRCInput::RC_timeout;
				break;
			case RETURN_REPAINT:
				paint();
				break;
		}
	}
}

void CWidget::onLeftKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onLeftKeyPressed\n");
	
	if(hasCCItem() && selected >= 0)
	{
		int rv = CCItems[selected]->swipLeft();
		
		actionKey = CCItems[selected]->getActionKey();	// lua

		//
		switch ( rv ) 
		{
			case RETURN_EXIT_ALL:
				retval = RETURN_EXIT_ALL; //fall through
			case RETURN_EXIT:
				msg = CRCInput::RC_timeout;
				break;
			case RETURN_REPAINT:
				paint();
				break;
		}
	}
}

void CWidget::onPageUpKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onPageUpKeyPressed\n");
	
	if(hasCCItem() && selected >= 0)
	{
		CCItems[selected]->scrollPageUp();
	}
}

void CWidget::onPageDownKeyPressed()
{
	dprintf(DEBUG_INFO, "CWidget::onPageDownKeyPressed\n");
	
	if(hasCCItem() && selected >= 0)
	{
		CCItems[selected]->scrollPageDown();
	}
}

//
void CWidget::onDirectKeyPressed(neutrino_msg_t _msg)
{
	dprintf(DEBUG_INFO, "CWidget::onDirectKeyPressed: msg:0x%x\n", _msg);
	
	if(hasCCItem() && selected >= 0)
	{
		int rv = CCItems[selected]->directKeyPressed(_msg);

		actionKey = CCItems[selected]->getActionKey();	// lua

		//
		switch ( rv ) 
		{
			case RETURN_EXIT_ALL:
				retval = RETURN_EXIT_ALL; //fall through
			case RETURN_EXIT:
				msg = CRCInput::RC_timeout;
				break;
			case RETURN_REPAINT:
				paint();
				break;
		}
	}
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
			case CComponent::CC_ICON:
				if ( (CCItems[count]->cc_type == CComponent::CC_ICON) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_IMAGE:
				if ( (CCItems[count]->cc_type == CComponent::CC_IMAGE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_LABEL:
				if ( (CCItems[count]->cc_type == CComponent::CC_LABEL) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_TEXT:
				if ( (CCItems[count]->cc_type == CComponent::CC_TEXT) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_BUTTON:
				if ( (CCItems[count]->cc_type == CComponent::CC_BUTTON) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_HLINE:
				if ( (CCItems[count]->cc_type == CComponent::CC_HLINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_VLINE:
				if ( (CCItems[count]->cc_type == CComponent::CC_VLINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_FRAMELINE:
				if ( (CCItems[count]->cc_type == CComponent::CC_FRAMELINE) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_PIG:
				if ( (CCItems[count]->cc_type == CComponent::CC_PIG) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_GRID:
				if ( (CCItems[count]->cc_type == CComponent::CC_GRID) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_TIME:
				if ( (CCItems[count]->cc_type == CComponent::CC_TIME) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_WINDOW:
				if ( (CCItems[count]->cc_type == CComponent::CC_WINDOW) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_COUNTER:
				if ( (CCItems[count]->cc_type == CComponent::CC_COUNTER) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_LISTBOX:
				if ( (CCItems[count]->cc_type == CComponent::CC_LISTBOX) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
	
			case CComponent::CC_HEAD:
				if ( (CCItems[count]->cc_type == CComponent::CC_HEAD) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_FOOT:
				if ( (CCItems[count]->cc_type == CComponent::CC_FOOT) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_TEXTBOX:
				if ( (CCItems[count]->cc_type == CComponent::CC_TEXTBOX) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			case CComponent::CC_FRAMEBOX:
				if ( (CCItems[count]->cc_type == CComponent::CC_FRAMEBOX) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
	
			case CComponent::CC_LISTFRAME:
				if ( (CCItems[count]->cc_type == CComponent::CC_LISTFRAME) && (CCItems[count]->cc_name == name) )
					ret = CCItems[count]; 
				break;
				
			default:
				break;
		}
	}
	
	return ret;
}

