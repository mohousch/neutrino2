//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: component.cpp 04092025 mohousch Exp $
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <system/settings.h>
#include <system/debug.h>
#include <system/helpers.h>

#include <gui/widget/component.h>
#include <gui/widget/framebox.h>

#include <video_cs.h>


//// globals
extern cVideo * videoDecoder;

//// CComponent
CComponent::CComponent()
{
	//
	itemBox.iX = 0;
	itemBox.iY = 0;
	itemBox.iWidth = 0;
	itemBox.iHeight = 0;
	
	oldPosition = itemBox;
	//
	paintframe = false;
	rePaint = false;
	painted = false;
	savescreen = false;
	inFocus = true;
	adjustToParent = false;
	paint_Head = false;
	paint_Foot = false;
	has_Title = false;
	htitle = "";
	hicon = "";
	//
	halign = CC_ALIGN_LEFT;
	//
	jumpTarget = NULL;
	actionKey = "";
	isExecutable = false;
	parent = NULL;
	timeout = g_settings.timing_menu;
	sec_timer_id = 0;
	sec_timer_interval = 1;
	//
	cc_type = -1;
	cc_name = "";
}

void CComponent::initFrames()
{
	dprintf(DEBUG_INFO, "CComponent::initFrames\n");
	
	// sanity check
	if(itemBox.iHeight > ((int)CFrameBuffer::getInstance()->getScreenHeight(true)))
		itemBox.iHeight = CFrameBuffer::getInstance()->getScreenHeight(true) - 4;  	// 4 pixels for border

	// sanity check
	if(itemBox.iWidth > (int)CFrameBuffer::getInstance()->getScreenWidth(true))
		itemBox.iWidth = CFrameBuffer::getInstance()->getScreenWidth(true) - 4; 	// 4 pixels for border
}

void CComponent::adjustToParentPosition()
{
	dprintf(DEBUG_INFO, "CComponent::adjustToParentPosition\n");
	
	// do stuff here
	bool parentMoved = false;
	CBox pOldPos;
	CBox pNewPos;
	
	if (parent)
	{
		pOldPos = parent->getOldPosition();
		pNewPos = parent->getWindowsPos();
		
		printf("CComponent::adjustToParentPosition: parent oldPos: %d %d %d %d\n", pOldPos.iX, pOldPos.iY, pOldPos.iWidth, pOldPos.iHeight);
		
		printf("CComponent::adjustToParentPosition: parent newPos: %d %d %d %d\n", pNewPos.iX, pNewPos.iY, pNewPos.iWidth, pNewPos.iHeight);
		
		if (pOldPos.iX != pNewPos.iX || pOldPos.iY != pNewPos.iY)
			parentMoved = true;
	}
	
	// FIXME:
	int xOffset = pNewPos.iX - pOldPos.iX;
	int yOffset = pNewPos.iY - pOldPos.iY;
	
	printf("CComponent::adjustToParentPosition; xOffset:%d yOffset:%d\n", xOffset, yOffset);
	
	// calculate oldOffsets
	CBox iOldPos = getOldPosition();
	
	printf("CComponent::adjustToParentPosition: item oldPos: %d %d %d %d\n", iOldPos.iX, iOldPos.iY, iOldPos.iWidth, iOldPos.iHeight);
	
	// calculate newPos
	if (parentMoved)
	{
		CBox iNewPos;
		
		iNewPos.iX = iOldPos.iX + xOffset;
		iNewPos.iY = iOldPos.iY + yOffset;
		iNewPos.iWidth = iOldPos.iWidth;
		iNewPos.iHeight = iOldPos.iHeight; 
		
		itemBox = iNewPos;
		
		printf("CComponent::adjustToParentPosition: item newPos: %d %d %d %d\n", iNewPos.iX, iNewPos.iY, iNewPos.iWidth, iNewPos.iHeight);
	}
	
	// 
	initFrames();
}

//
void CComponent::addKey(neutrino_msg_t key, CTarget *target, const std::string & action)
{
	dprintf(DEBUG_DEBUG, "CComponent::addKey: %s\n", action.c_str());
	
	keyActionMap[key].target = target;
	keyActionMap[key].action = action;
}

//
int CComponent::exec(CTarget *target)
{
	dprintf(DEBUG_NORMAL, "CComponent::exec:\n");
	
	bool show = true;
	exit_pressed = false;
	int retval = CTarget::RETURN_REPAINT;
	
	if (!isSelectable())
		return CTarget::RETURN_NONE;
	
	if (target)
		target->hide();

	//
	paint();
	CFrameBuffer::getInstance()->blit();
		
	// add sec timer
	sec_timer_id = g_RCInput->addTimer(sec_timer_interval*1000*1000, false);
		
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);

	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		
		int handled = false;
		
		if ( msg <= CRCInput::RC_MaxRC ) 
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);
			
			std::map<neutrino_msg_t, keyAction>::iterator it = keyActionMap.find(msg);
						
			if (it != keyActionMap.end()) 
			{
				actionKey = it->second.action; // FIXME:

				if (it->second.target != NULL)
				{
					int rv = it->second.target->exec(target, it->second.action);

					//
					switch ( rv ) 
					{
						case CTarget::RETURN_EXIT_ALL:
							retval = CTarget::RETURN_EXIT_ALL;
						case CTarget::RETURN_EXIT:
							msg = CRCInput::RC_timeout;
							break;
						case CTarget::RETURN_REPAINT:
							paint();
							break;
					}
				}
				else
				{
					handled = true;
					break;
				}
				
				CFrameBuffer::getInstance()->blit();
				continue;
			}
			
			//
			int rv = directKeyPressed(msg, target);

			switch ( rv ) 
			{
				case CTarget::RETURN_EXIT_ALL:
					retval = CTarget::RETURN_EXIT_ALL; //fall through
				case CTarget::RETURN_EXIT:
					msg = CRCInput::RC_timeout;
					break;
				case CTarget::RETURN_REPAINT:
					paint();
					break;
			}
		}
		
		if (!handled)
		{
			if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
			{
				show = !show;
				
				if (update())
				{
					refresh(show);
				}
			}
			
			switch (msg)
			{
				case (NeutrinoMessages::EVT_TIMER):
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = CTarget::RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
					break;
					
				case CRCInput::RC_up:
					{
						scrollLineUp();
					}
					break;
					
				case CRCInput::RC_down:
					{
						scrollLineDown();
					}
					break;
					
				case CRCInput::RC_left:
					{
						swipLeft(target);
					}
					break;
					
				case CRCInput::RC_right:
					{
						swipRight(target);
					}
					break;
					
				case CRCInput::RC_page_up:
					{
						scrollPageUp();
					}
					break;
					
				case CRCInput::RC_page_down:
					{
						scrollPageDown();
					}
					break;
				
				case CRCInput::RC_ok:
					{
						int rv = oKKeyPressed(target);
					
						switch ( rv ) 
						{
							case CTarget::RETURN_EXIT_ALL:
								retval = CTarget::RETURN_EXIT_ALL;
							case CTarget::RETURN_EXIT:
								msg = CRCInput::RC_timeout;
								break;
							case CTarget::RETURN_REPAINT:
								paint();
								break;
						}
					}
					break;
				
				case CRCInput::RC_home:
					{
						homeKeyPressed();
						exit_pressed = true;
						msg = CRCInput::RC_timeout;
					}
					break;
				case (CRCInput::RC_timeout):
					exit_pressed = true;
					break;

				default:
					if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
					{
						retval = CTarget::RETURN_EXIT_ALL;
						msg = CRCInput::RC_timeout;
					}
			}
			
			if ( msg <= CRCInput::RC_MaxRC )
			{
				// recalculate timeout for RC-Tasten
				timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);
			}
		}
		
		CFrameBuffer::getInstance()->blit();
		
	} while ( msg != CRCInput::RC_timeout );

	hide();	
	
	//
	if (sec_timer_id)
	{
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	return retval;	
}

//// CCIcon
CCIcon::CCIcon(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCIcon::CCIcon: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	iconName = ""; 
	width = 0; 
	height = 0; 
	
	background = NULL;
	
	cc_type = CC_ICON;
}

//
void CCIcon::setIcon(const char* const icon)
{
	iconName = icon? icon : ""; 
	
	if (!iconName.empty()) frameBuffer->getIconSize(iconName.c_str(), &width, &height);
	
	if (width > itemBox.iWidth && itemBox.iWidth != 0)
		width = itemBox.iWidth;
		
	if (height > itemBox.iHeight && itemBox.iHeight != 0)
		height = itemBox.iHeight;
}

void CCIcon::saveScreen(void)
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
			
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCIcon::restoreScreen(void)
{
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

//// FIXME:
void CCIcon::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCIcon::paint\n");
	
	////
	initFrames();
	
	//
	if (savescreen || rePaint)
	{
		saveScreen();
	}
	
	if (!iconName.empty()) frameBuffer->paintIcon(iconName.c_str(), itemBox.iX + (itemBox.iWidth - width)/2, itemBox.iY + (itemBox.iHeight - height)/2);
};

void CCIcon::hide()
{
	dprintf(DEBUG_DEBUG, "CCIcon::hide\n");
	
	if (savescreen || rePaint)
	{
		restoreScreen();
	}
}

void CCIcon::refresh(bool show)
{
	dprintf(DEBUG_DEBUG, "CCIcon::refresh\n");
	
	if (show)
		paint();
	else
		hide();
}

//// CCImage
CCImage::CCImage(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCImage::CCImage: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	 
	imageName = ""; 
	iWidth = 0; 
	iHeight = 0; 
	scale = false;
	
	background = NULL;
	
	cc_type = CC_IMAGE;
}

//
void CCImage::setImage(const char* const image)
{
	imageName = image? image : "";
	int nbpp = 0;
	int nchans = 0;
	 
	if (!imageName.empty()) getSize(imageName, &iWidth, &iHeight, &nbpp, &nchans);
}

void CCImage::saveScreen(void)
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
			
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCImage::restoreScreen(void)
{
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

//
void CCImage::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCImage::paint\n");
	
	////
	initFrames();
	
	if (iWidth > itemBox.iWidth && itemBox.iWidth != 0) 
		iWidth = itemBox.iWidth;
	if (iHeight > itemBox.iHeight && itemBox.iHeight != 0) 
		iHeight = itemBox.iHeight;
		
	//
	if (rePaint)
	{
		saveScreen();
	}
			
	int startPosX = itemBox.iX + (itemBox.iWidth - iWidth)/2;
			
	if (scale)
	{		
		// image
		if (!imageName.empty()) 
			frameBuffer->displayImage(imageName.c_str(), itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	}
	else
	{		
		// image
		if (!imageName.empty()) 
			frameBuffer->displayImage(imageName.c_str(), startPosX, itemBox.iY + (itemBox.iHeight - iHeight)/2, iWidth, iHeight);
	}
}

void CCImage::hide()
{
	dprintf(DEBUG_DEBUG, "CCImage::hide\n");
	
	restoreScreen();
}

void CCImage::refresh(bool show)
{
	dprintf(DEBUG_DEBUG, "CCImage::refresh\n");
	
	if (show)
		paint();
	else
		hide();
}

//// CCButtons
CCButtons::CCButtons(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCButtons::CCButtons: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	buttons.clear(); 
	count = 0;
	head = false;
	mode = BUTTON_ICON;
	
	cc_type = CC_BUTTON;
}

void CCButtons::setButtons(const struct button_label *button_label, const int button_count, bool _head)
{
	if (button_count)
	{
		for (unsigned int i = 0; i < (unsigned int)button_count; i++)
		{
			buttons.push_back(button_label[i]);
		}
	}

	count = buttons.size();
	head = _head;
}

void CCButtons::addButton(const char *btn, const char *lname, const fb_pixel_t col)
{
	dprintf(DEBUG_DEBUG, "CCButtons::addButton: btn:%s name:%s\n", btn, lname);
	
	button_label_struct button;
	
	button.button = btn;
	button.localename = lname? lname : "";
	button.color = col;
	
	buttons.push_back(button);
	count++;
}

void CCButtons::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCButtons::CCButtons:paint:\n");
	
	////
	initFrames();

	count = buttons.size();
	
	dprintf(DEBUG_DEBUG, "CCButtons::CCButtons:paint: count: %d\n", count);
	
	//
	int buttonWidth = 0;
	int maxButtonTextWidth = buttonWidth;
	int iw[count];
	int ih[count];
	int f_w[count];
	
	if (head)
	{
		int startx = itemBox.iX + itemBox.iWidth - BORDER_LEFT;
		
		if(count)
		{
			// get max width
			for (unsigned int i = 0; i < count; i++)
			{
				if (!buttons[i].localename.empty())
				{
					f_w[i] = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_(buttons[i].localename.c_str()));
					if (i > 0)
						maxButtonTextWidth = std::max(f_w[i], f_w[i - 1]) + 10;
					else
						maxButtonTextWidth = f_w[i] + 10;
				}
			}
			
			for (unsigned int i = 0; i < count; i++)
			{
				if (mode == BUTTON_ICON)
				{
					if(!buttons[i].button.empty())
					{
						frameBuffer->getIconSize(buttons[i].button.c_str(), &iw[i], &ih[i]);
						
						// scale icon
						if(ih[i] >= itemBox.iHeight)
						{
							ih[i] = itemBox.iHeight - 2;
						}
					
						startx -= (iw[i] + ICON_TO_ICON_OFFSET);

						frameBuffer->paintIcon(buttons[i].button, startx, itemBox.iY + (itemBox.iHeight - ih[i])/2, 0, iw[i], ih[i]);
					}
				}
				else if (mode == BUTTON_FRAME)
				{
					startx -= (maxButtonTextWidth + 10 + ICON_TO_ICON_OFFSET);
							
					//
					CCFrameLine frame;
						
					frame.setPosition(startx, itemBox.iY + 2, maxButtonTextWidth + 10, itemBox.iHeight - 4);
					frame.paint();
					
					// label
					CCLabel label;
					label.setPosition(startx, itemBox.iY + 2, maxButtonTextWidth + 10, itemBox.iHeight - 4);
					label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
					label.setText(_(buttons[i].localename.c_str()));
					label.setHAlign(CC_ALIGN_CENTER);
					label.paint();
				}
			}
		}
	}
	else
	{
		if(count)
		{
			buttonWidth = (itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT)/count;
			
			// get maxButtonWidth
			for (unsigned int i = 0; i < count; i++)
			{
				if (!buttons[i].localename.empty())
				{
					f_w[i] = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_(buttons[i].localename.c_str()));
					
					if (i > 0)
						maxButtonTextWidth = std::max(f_w[i], f_w[i - 1]) + 10;
					else
						maxButtonTextWidth = f_w[i] + 10;
						
					if ((maxButtonTextWidth + 20) > buttonWidth)
						maxButtonTextWidth = buttonWidth;
				}
			}
			
			if (maxButtonTextWidth > buttonWidth)
				maxButtonTextWidth = buttonWidth;
		
			for (unsigned int i = 0; i < count; i++)
			{
				if (mode == BUTTON_ICON)
				{
					if (!buttons[i].button.empty())
					{
						iw[i] = 0;
						ih[i] = 0;

						CFrameBuffer::getInstance()->getIconSize(buttons[i].button.c_str(), &iw[i], &ih[i]);
							
						// scale icon
						if(ih[i] >= itemBox.iHeight)
						{
							ih[i] = itemBox.iHeight - 2;
						}
							
						int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
					
						CFrameBuffer::getInstance()->paintIcon(buttons[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + (itemBox.iHeight - ih[i])/2, 0, iw[i], ih[i]);

						// FIXME: i18n
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + f_h + (itemBox.iHeight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(buttons[i].localename.c_str()), COL_MENUFOOT_TEXT_PLUS_0, 0, true); // UTF-8
					}
				}
				else if (mode == BUTTON_FRAME)
				{
					//
					CCFrameLine frame;
						
					frame.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 2, buttonWidth, itemBox.iHeight - 4);
					frame.paint();
					
					// label
					CCLabel label;
					label.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 2, buttonWidth, itemBox.iHeight - 4);
					label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
					label.setHAlign(CC_ALIGN_CENTER);
					label.setText(_(buttons[i].localename.c_str()));
					label.paint();
				}
				else if (mode == BUTTON_COLOREDFRAME)
				{
					//
					CFrameItem frame;
					frame.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 2, buttonWidth, itemBox.iHeight - 4);
					frame.setBorderMode();
					frame.paintMainFrame(true);
					frame.setColor(buttons[i].color);
					frame.setCaptionFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
					frame.setTitle(_(buttons[i].localename.c_str()));
					frame.setHAlign(CC_ALIGN_CENTER);
					if (!buttons[i].localename.empty()) frame.paint();
				}
				else if (mode == BUTTON_COLOREDLINE)
				{
					// button label
					CCLabel label;
					
					label.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 1, buttonWidth, itemBox.iHeight - 2);
					//label.setColor();
					label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
					label.setText(_(buttons[i].localename.c_str()));
					label.setHAlign(CC_ALIGN_CENTER);
					label.paint();
					
					//
					CCHline line;
					
					line.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - 10, buttonWidth, 8);
					line.setColor(buttons[i].color);
					//line.setGradient();
					line.paint();
				}
			}
		}
	}
}

//// CCHline
CCHline::CCHline(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCHline::CCHline: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	if (itemBox.iHeight > 2)
		itemBox.iHeight = 2;
	
	color = COL_MENUCONTENT_PLUS_5;
	gradient = g_settings.sep_gradient;
	 
	cc_type = CC_HLINE;
}

void CCHline::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCHline::paint\n");
	
	////
	initFrames();
	
	if (itemBox.iHeight > 2)
		itemBox.iHeight = 2;
	
	//
	frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, color, 0, CORNER_NONE, gradient, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
}

//// CCVline
CCVline::CCVline(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCVline::CCVline: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	if (itemBox.iWidth > 2)
		itemBox.iWidth = 2;
	
	color = COL_MENUCONTENT_PLUS_5;
	gradient = g_settings.sep_gradient;
	
	cc_type = CC_VLINE;
}

void CCVline::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCVline::paint\n");
	
	////
	initFrames();
	
	if (itemBox.iWidth > 2)
		itemBox.iWidth = 2;
	
	//
	frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, color, 0, CORNER_NONE, gradient, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
}

//// CFrameLine
CCFrameLine::CCFrameLine(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCFrameLine::CCFrameLine: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	color = COL_WHITE_PLUS_0; 
	cc_type = CC_FRAMELINE;
}

//
void CCFrameLine::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCFrameLine::paint\n");
	
	////
	initFrames();
	
	frameBuffer->paintFrameBox(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, color);
}

//// CCGrid
CCGrid::CCGrid(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCGrid::CCGrid: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	init();
}

CCGrid::CCGrid(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CCGrid::CCGrid:\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;
	
	oldPosition = itemBox;

	init();
}

void CCGrid::init()
{
	rgb = COL_NOBEL_PLUS_0;
	inter_frame = 15;
	
	//
	cc_type = CC_GRID;
}

void CCGrid::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCGrid::paint\n");
	
	////
	initFrames();
	
	// hlines grid
	for(int count = 0; count < itemBox.iHeight; count += inter_frame)
		frameBuffer->paintHLine(itemBox.iX, itemBox.iX + itemBox.iWidth, itemBox.iY + count, rgb );

	// vlines grid
	for(int count = 0; count < itemBox.iWidth; count += inter_frame)
		frameBuffer->paintVLine(itemBox.iX + count, itemBox.iY, itemBox.iY + itemBox.iHeight, rgb );
}

void CCGrid::hide()
{
	frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	
	CFrameBuffer::getInstance()->blit();
}


//// CCLabel
CCLabel::CCLabel(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCLabel::CCLabel: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	background = NULL;
	
	//
	color = COL_MENUCONTENT_TEXT_PLUS_0;
	font = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	
	label = "";
	
	cc_type = CC_LABEL;
}

CCLabel::~CCLabel()
{
	if (savescreen)
	{
		if (background)
		{
			delete [] background;
			background = NULL;
		}
	}
}

void CCLabel::saveScreen(void)
{
	if (background)
	{
		delete [] background;
		background = NULL;
	}
		
	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
		
	if (background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCLabel::restoreScreen(void)
{
	//
	if (savescreen && background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCLabel::enableSaveScreen()
{
	savescreen = true;
	
	saveScreen();
}

void CCLabel::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCLabel::paint\n");
	
	////
	initFrames();
	
	//
	if (rePaint)
		saveScreen();
	else
		restoreScreen();
	
	//
	int stringWidth = itemBox.iWidth;
	int height = 0;
	
	height = g_Font[font]->getHeight();
	
	if (!label.empty()) stringWidth = g_Font[font]->getRenderWidth(label.c_str());
	
	if (stringWidth > itemBox.iWidth && itemBox.iWidth != 0)
	{
		stringWidth = itemBox.iWidth;
	}
		
	int startPosX = itemBox.iX;
	
	if (halign == CC_ALIGN_CENTER)
		startPosX = itemBox.iX + (itemBox.iWidth - stringWidth)/2;
	else if (halign == CC_ALIGN_RIGHT)
		startPosX = itemBox.iX + itemBox.iWidth - stringWidth;
		
	g_Font[font]->RenderString(startPosX, itemBox.iY + height + (itemBox.iHeight - height)/2, stringWidth, label.c_str(), color, 0, true, paintframe);
}

void CCLabel::hide()
{
	if (rePaint)
		restoreScreen();
	else
	{
		if (savescreen)
		{
			if (background)
			{
				delete [] background;
				background = NULL;
			}
		}
	}
}

void CCLabel::refresh(bool show)
{
	dprintf(DEBUG_DEBUG, "CCLabel::refresh\n");
	
	if (show)
		paint();
	else
		hide();
}

//// CCText
CCText::CCText(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCText::CCText: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	background = NULL;
	
	font = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	color = COL_MENUCONTENT_TEXT_PLUS_0;
	
	//
	Text.clear();
	
	medlineheight = g_Font[font]->getHeight();
	medlinecount  = itemBox.iHeight / medlineheight;

	cc_type = CC_TEXT;
}

void CCText::saveScreen(void)
{
	if (background)
	{
		delete [] background;
		background = NULL;
	}
		
	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
		
	if (background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCText::restoreScreen(void)
{
	if (savescreen && background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCText::enableSaveScreen()
{
	savescreen = true;
	
	saveScreen();
}

CCText::~CCText()
{
	if (savescreen)
	{
		if (background)
		{
			delete [] background;
			background = NULL;
		}
	}
}

//
void CCText::addTextToArray(const std::string &text) // UTF-8
{
	if (text == " ")
	{
		emptyLineCount ++;
		
		if(emptyLineCount < 2)
		{
			Text.push_back(text);
		}
	}
	else
	{
		emptyLineCount = 0;
		Text.push_back(text);
	}
}

void CCText::processTextToArray(std::string text) // UTF-8
{
	std::string aktLine = "";
	std::string aktWord = "";
	int aktWidth = 0;
	text += ' ';
	char *text_ = (char *)text.c_str();

	while(*text_ != 0)
	{
		if ( (*text_ == ' ') || (*text_ == '\n') || (*text_ == '-') || (*text_ == '.') )
		{
			if(*text_ != '\n')
				aktWord += *text_;

			// check the wordwidth - add to this line if size ok
			int aktWordWidth = g_Font[font]->getRenderWidth(aktWord);
			
			if((aktWordWidth + aktWidth) < (itemBox.iWidth))
			{	
				//space ok, add
				aktWidth += aktWordWidth;
				aktLine += aktWord;
			
				if(*text_ == '\n')
				{	//enter-handler
					addTextToArray(aktLine);
					aktLine = "";
					aktWidth = 0;
				}	
				aktWord = "";
			}
			else
			{	
				//new line needed
				addTextToArray( aktLine );
				aktLine = aktWord;
				aktWidth = aktWordWidth;
				aktWord = "";
				
				if(*text_ == '\n')
					continue;
			}
		}
		else
		{
			aktWord += *text_;
		}
		text_++;
	}
	
	//add the rest
	addTextToArray( aktLine + aktWord );
}

void CCText::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCText::paint\n");
	
	////
	initFrames();
	
	//
	if (rePaint)
		saveScreen();
	else
		restoreScreen();

	// recalculate
	medlineheight = g_Font[font]->getHeight();
	medlinecount = itemBox.iHeight / medlineheight;

	int textSize = Text.size();
	int y = itemBox.iY;
	
	//
	int startPosX = itemBox.iX;

	for(int i = 0; i < textSize && i < medlinecount; i++, y += medlineheight)
	{
		//
		if (halign == CC_ALIGN_CENTER)
			startPosX = itemBox.iX + (itemBox.iWidth - g_Font[font]->getRenderWidth(Text[i]))/2;
		else if (halign == CC_ALIGN_RIGHT)
			startPosX = itemBox.iX + itemBox.iWidth - g_Font[font]->getRenderWidth(Text[i]);
		
		//
		g_Font[font]->RenderString(startPosX, y + medlineheight, itemBox.iWidth, Text[i], color, 0, true, paintframe); // UTF-8
	}
}

void CCText::hide()
{
	if (rePaint)
		restoreScreen();
	else
	{
		if (savescreen)
		{
			if (background)
			{
				delete [] background;
				background = NULL;
			}
		}
	}
}

void CCText::refresh(bool show)
{
	dprintf(DEBUG_DEBUG, "CCText::refresh\n");
	
	if (show)
		paint();
	else
		hide();
}

//// CCTime
CCTime::CCTime(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCTime::CCTime: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	font = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	color = COL_MENUHEAD_TEXT_PLUS_0;
	
	background = NULL;
	
	format = "%d.%m.%Y %H:%M";
	
	rePaint = true;
	
	cc_type = CC_TIME;
}

CCTime::~CCTime()
{
	dprintf(DEBUG_DEBUG, "CCTime::~CCTime\n");
	
	//
	if (background)
	{
		delete [] background; 
		background = NULL;
	}
}

void CCTime::setFormat(const char* const f)
{
	format = f? _(f) : "";
}

void CCTime::paintDigits(void)
{
	std::string timestr = getNowTimeStr(format.c_str());
		
	int timestr_len = g_Font[font]->getRenderWidth(timestr.c_str(), true); // UTF-8
	
	if (timestr_len > itemBox.iWidth && itemBox.iWidth != 0)
		timestr_len = itemBox.iWidth;
		
	int startPosX = itemBox.iX + (itemBox.iWidth - timestr_len)/2;
	
	g_Font[font]->RenderString(startPosX, itemBox.iY + (itemBox.iHeight - g_Font[font]->getHeight())/2 + g_Font[font]->getHeight(), timestr_len, timestr.c_str(), color);
}

void CCTime::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCTime::paint: x:%d y:%d dx:%d dy:%d\n", itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	
	////
	initFrames();
	
	//
	saveScreen();
	
	//
	paintDigits();
}

//
void CCTime::refresh(bool show)
{
	//
	restoreScreen();
	
	//
	paintDigits();
}

void CCTime::hide()
{
	dprintf(DEBUG_DEBUG, "CCTime::hide\n");
	
	if (background)
	{
		delete [] background; 
		background = NULL;
	}
}

void CCTime::saveScreen(void)
{
	if (background)
	{
		delete [] background; 
		background = NULL;
	}
	
	//
	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
	
	if (background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCTime::restoreScreen(void)
{
	if (background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

//// CCCounter
CCCounter::CCCounter(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCCounter::CCCounter: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	font = SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO;
	color = COL_INFOBAR_TEXT_PLUS_0;
	
	background = NULL;
	
	total_time = 0;
	play_time = 0;
	
	//
	itemBox.iWidth = g_Font[font]->getRenderWidth("00:00:00 / 00:00:00");
	itemBox.iHeight = g_Font[font]->getHeight();
	
	//
	rePaint = true;
	
	cc_type = CC_COUNTER;
}

CCCounter::~CCCounter()
{
	if (background)
	{
		delete [] background; 
		background = NULL;
	}
}

void CCCounter::paintDigits(void)
{
	// play_time
	char playTime[11];
	strftime(playTime, 11, "%T/", gmtime(&play_time));//FIXME
	
	g_Font[font]->RenderString(itemBox.iX, itemBox.iY + (itemBox.iHeight - g_Font[font]->getHeight())/2 + g_Font[font]->getHeight(), itemBox.iWidth/2, playTime, color, 0, true);
	
	// total_time
	char totalTime[10];
	strftime(totalTime, 10, "%T", gmtime(&total_time));//FIXME
	g_Font[font]->RenderString(itemBox.iX + itemBox.iWidth/2, itemBox.iY + (itemBox.iHeight - g_Font[font]->getHeight())/2 + g_Font[font]->getHeight(), itemBox.iWidth/2, totalTime, color, 0, true);
}

void CCCounter::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCCounter::paint\n");
	
	////
	initFrames();
	
	//
	saveScreen();
	
	//
	paintDigits();
}

void CCCounter::refresh(bool show)
{
	//
	restoreScreen();
	
	//
	paintDigits();
}

void CCCounter::hide()
{
	dprintf(DEBUG_DEBUG, "CCCounter::hide\n");

	if (background)
	{
		delete [] background; 
		background = NULL;
	}
}

void CCCounter::saveScreen(void)
{
	if (background)
	{
		delete [] background; 
		background = NULL;
	}
	
	//
	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
	
	if (background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCCounter::restoreScreen(void)
{
	if (background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

//// CCSpinner
CCSpinner::CCSpinner(const int x, const int y, const int dx, const int dy)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	int iw, ih;
	frameBuffer->getIconSize("hourglass0", &iw, &ih);
	
	if (iw > dx)
		itemBox.iWidth = iw;
		
	if (ih > dy)
		itemBox.iHeight = ih;
	
	//
	filename = "hourglass";
	
	//
	count = 0;
	background = NULL;
	
	rePaint = true;
	
	//
	cc_type = CC_SPINNER;
}

CCSpinner::~CCSpinner()
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}
}

void CCSpinner::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCSpinner::paint\n");
	
	////
	initFrames();
	
	//
	saveScreen();
	
	filename += toString(count);
	
	frameBuffer->paintIcon(filename, itemBox.iX, itemBox.iY);
}

void CCSpinner::hide()
{
	dprintf(DEBUG_DEBUG, "CCSpinner::hide\n");
	
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
	else //FIXME:
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
}

//
void CCSpinner::refresh(bool show)
{
	dprintf(DEBUG_DEBUG, "CCSpinner::refresh\n");
	
	filename.clear();
	
	filename = "hourglass";
	
	//
	restoreScreen();
		
	count = (count + 1) % 9;
	
	filename += toString(count);
	
	frameBuffer->paintIcon(filename, itemBox.iX, itemBox.iY);
}

void CCSpinner::saveScreen(void)
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
		
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCSpinner::restoreScreen(void)
{
	if (background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

////
CCSlider::CCSlider(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCSlider::CCSlider\n");
	
	//
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	//
	value = 0;
	min_value = 0;
	max_value = 100;
	step = 1;
	
	//
	cc_type = CC_SLIDER;
}

void CCSlider::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCSlider::paint:\n");
	
	////
	initFrames();
	
	paintSlider(itemBox.iX, itemBox.iY, value, "test CCSlider", NEUTRINO_ICON_VOLUMESLIDER2RED);
}

//
void CCSlider::paintSlider(const int _x, const int _y, const unsigned int spos, const char* const text, const char * const iconname)
{
	dprintf(DEBUG_NORMAL, "CCSlider::paintSlider:\n");
	
	char wert[5];
	
	//int icon_w = 120;
	//int icon_h = 11;
	//frameBuffer->getIconSize(NEUTRINO_ICON_VOLUMEBODY, &icon_w, &icon_h);
	int slider_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("100", true); //UTF-8
	//int startx = itemBox.iWidth - icon_w - slider_w - 50;
	//frameBuffer->paintBoxRel(_x + itemBox.iX, _y, 120, itemBox.iHeight, COL_MENUCONTENT_PLUS_0);
	//frameBuffer->paintIcon(NEUTRINO_ICON_VOLUMEBODY, _x + startx, _y + 2 + itemBox.iHeight / 4);
	//frameBuffer->paintIcon(iconname, _x + startx + 3 + spos, _y + itemBox.iHeight / 4);
	
	////
	//frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + (itemBox.iHeight - 6)/2, itemBox.iWidth - slider_w - 5, /*itemBox.iHeight*/6, COL_MENUCONTENT_PLUS_2, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);

	//
	//g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(itemBox.iX + itemBox.iWidth - slider_w - 5, _y + itemBox.iHeight + (itemBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2, itemBox.iWidth - slider_w - 5, text, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
	
	sprintf(wert, "%3d", spos); // UTF-8 encoded

	// refreshBox
	frameBuffer->paintBoxRel(itemBox.iX + itemBox.iWidth - slider_w - 5, _y, 50, itemBox.iHeight, COL_BACKGROUND);
	
	//
	//g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(itemBox.iX + itemBox.iWidth - slider_w - 5, itemBox.iY + itemBox.iHeight + (itemBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2, slider_w, wert, COL_MENUCONTENT_TEXT_PLUS_0, 0, true, true); // UTF-8
}

//
int CCSlider::swipRight(CTarget *target)
{
	dprintf(DEBUG_NORMAL, "CCSlider::swipRight:\n");
	
	if (value > max_value)
		value = max_value;
	
	if (value < max_value)
	{
		value++;
	
		paintSlider(itemBox.iX, itemBox.iY, value, "test CCSlider", NEUTRINO_ICON_VOLUMESLIDER2RED);
	}
	
	return value;
}

//
int CCSlider::swipLeft(CTarget *target)
{
	dprintf(DEBUG_NORMAL, "CCSlider::swipLeft:\n");
	
	if (value < min_value)
		value = min_value; 
	
	if (value > min_value)
	{
		value--;
	
		paintSlider(itemBox.iX, itemBox.iY, value--, "test CCSlider", NEUTRINO_ICON_VOLUMESLIDER2RED);
	}
	
	return value;
}

//// progressbar
CCProgressBar::CCProgressBar(int x, int y, int w, int h, int r, int g, int b, bool inv)
{
	dprintf(DEBUG_DEBUG, "CCProgressBar::CCProgressBar: x:%d y:%d dx:%d dy:%d\n", x, y, w, h);
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = w;
	itemBox.iHeight = h;
	inverse = inv;
	
	oldPosition = itemBox;
	
	div = (double) itemBox.iWidth / (double) 100;
	
	red = (double) r * (double) div ;
	green = (double) g * (double) div;
	yellow = (double) g * (double) div;
	
	while ((red + yellow + green) < itemBox.iWidth) 
	{
		if (green)
			green++;
		if (yellow && ((red + yellow + green) < itemBox.iWidth))
			yellow++;
		if (red && ((red + yellow + green) < itemBox.iWidth))
			red++;
	}
	
	rgb = 0xFF0000;
	
	percent = 255;
	
	cc_type = CC_PROGRESSBAR;
}

CCProgressBar::CCProgressBar(const CBox* position, int r, int g, int b, bool inv)
{
	dprintf(DEBUG_DEBUG, "CCProgressBar::CCProgressBar: x:%d y:%d dx:%d dy:%d\n", position->iX, position->iY, position->iWidth, position->iHeight);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;
	inverse = inv;
	
	oldPosition = itemBox;
	
	div = (double) itemBox.iWidth / (double) 100;
	
	red = (double) r * (double) div ;
	green = (double) g * (double) div;
	yellow = (double) g * (double) div;
	
	while ((red + yellow + green) < itemBox.iWidth) 
	{
		if (green)
			green++;
		if (yellow && ((red + yellow + green) < itemBox.iWidth))
			yellow++;
		if (red && ((red + yellow + green) < itemBox.iWidth))
			red++;
	}
	
	rgb = 0xFF0000;
	
	percent = 255;
	
	cc_type = CC_PROGRESSBAR;
}

void CCProgressBar::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCProgressBar::paint:\n");
	
	////
	initFrames();
	
	// body
	frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_2, NO_RADIUS, CORNER_ALL, g_settings.progressbar_color? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
}

void CCProgressBar::refresh(unsigned char pcr)
{
	dprintf(DEBUG_DEBUG, "CCProgressBar::refresh: pcr:%d\n", pcr);
	
	int i = 0;
	int b = 0;
	int siglen = 0;
	
	// body
	frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_2, NO_RADIUS, CORNER_ALL, g_settings.progressbar_color? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
	
	if (pcr != percent) 
	{
		if(percent == 255) 
			percent = 0;

		siglen = (double) pcr * (double) div;
		int step = 0;
		uint32_t diff = 0;

		if(g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_COLORED)
		{
			//red
			for (i = 0; (i < red) && (i < siglen); i++) 
			{
				diff = i * 255 / red;

				if(inverse) 
					rgb = 0x00FF00 + (diff << 16); // adding red
				else
					rgb = 0xFF0000 + (diff << 8); // adding green
				
				frameBuffer->paintBoxRel(itemBox.iX + i, itemBox.iY, 1, itemBox.iHeight, ::rgbaToColor(rgb), NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
			}
			
			//yellow
			step = yellow - red - 1;
			if (step < 1)
				step = 1;
		
			for (; (i < yellow) && (i < siglen); i++) 
			{
				diff = b++ * 255 / step / 2;

				if(inverse) 
					rgb = 0xFFFF00 - (diff << 8); // removing green
				else
					rgb = 0xFFFF00 - (diff << 16); // removing red
	
				frameBuffer->paintBoxRel(itemBox.iX + i, itemBox.iY, 1, itemBox.iHeight, ::rgbaToColor(rgb), NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
			}

			//green
			int off = diff;
			b = 0;
			step = green - yellow - 1;
			if (step < 1)
				step = 1;
			for (; (i < green) && (i < siglen); i++) 
			{
				diff = b++ * 255 / step / 2 + off;

				if(inverse) 
					rgb = 0xFFFF00 - (diff << 8); // removing green
				else
					rgb = 0xFFFF00 - (diff << 16); // removing red
				
				frameBuffer->paintBoxRel(itemBox.iX + i, itemBox.iY, 1, itemBox.iHeight, ::rgbaToColor(rgb), NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
			}
		}
		else if (g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_MONOCHROM)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, siglen, itemBox.iHeight, COL_MENUCONTENT_PLUS_6, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
		}
		else if (g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_RED)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, siglen, itemBox.iHeight, COL_RED_PLUS_0, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
		}
		else if (g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_GREEN)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, siglen, itemBox.iHeight, COL_GREEN_PLUS_0, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
		}
		else if (g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_YELLOW)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, siglen, itemBox.iHeight, COL_YELLOW_PLUS_0, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
		}
		else if (g_settings.progressbar_color == CCProgressBar::PROGRESSBAR_BLUE)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, siglen, itemBox.iHeight, COL_BLUE_PLUS_0, NO_RADIUS, CORNER_ALL, DARK2LIGHT2DARK, GRADIENT_VERTICAL, INT_LIGHT, GRADIENT_ONECOLOR);
		}
		
		percent = pcr;
	}
}

void CCProgressBar::reset()
{
  	percent = 255;
}

//// CItemInfo
CCItemInfo::CCItemInfo()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	oldPosition = itemBox;
	
	//
	mode = ITEMINFO_HINTITEM; 
	hint = "";
	icon = "";
	
	paintframe = false;
	background = NULL;
	
	// hintitem / hinticon
	tFont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
	borderMode = g_settings.Hint_border;
	borderColor = COL_MENUCONTENT_PLUS_6;
	savescreen = false;
	color = COL_MENUCONTENT_PLUS_0;
	scale = false;
	radius = g_settings.Hint_radius;
	corner = g_settings.Hint_corner;
	gradient = NOGRADIENT;
	
	cc_type = CC_ITEMINFO;
}

CCItemInfo::~CCItemInfo()
{
	hint.clear();
	icon.clear();
	
	if (savescreen)
	{
		if (background)
		{
			delete [] background;
			background = NULL;
		}
	}
}

void CCItemInfo::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCItemInfo::paint:\n");
	
	// frame
	if (paintframe)
	{
		if (borderMode) 
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, borderColor, g_settings.Hint_radius, g_settings.Hint_corner);
				
		// infoBox
		frameBuffer->paintBoxRel(borderMode? itemBox.iX + 2 : itemBox.iX, borderMode? itemBox.iY + 2 : itemBox.iY, borderMode? itemBox.iWidth - 4 : itemBox.iWidth, borderMode? itemBox.iHeight - 4 : itemBox.iHeight, color, radius, corner, gradient);
	}
	else
		restoreScreen();
	
	//
	if (mode == ITEMINFO_HINTITEM)
	{
		// icon (top)
		int iw = 0;
		int ih = 0;
		
		if (!icon.empty())
		{
			::scaleImage(icon, &iw, &ih);
			
			if (iw > itemBox.iWidth && itemBox.iWidth != 0)
				iw = itemBox.iWidth - 4;
				
			if (ih > (itemBox.iHeight - 4) && itemBox.iHeight != 0)
				ih = (itemBox.iHeight - 4)/2;
		
			CCImage DImage(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, ih - 4);
			DImage.setImage(icon.c_str());
			DImage.setScaling(scale);
			//DImage.setColor(color);
			DImage.paint();
		}
		
		// hint
		CCText Dline(itemBox.iX + 10, itemBox.iY + ih + 10, itemBox.iWidth - 20, itemBox.iHeight - ih - 20);
		Dline.setFont(tFont);
		Dline.setText(hint.c_str());		
		Dline.paint();
		
	}
	else if (mode == ITEMINFO_ICONONLY)	
	{
		CCImage DImage(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
		DImage.setImage(icon.c_str());
		DImage.setScaling(scale);
		//DImage.setColor(color);
		DImage.paint();
	}
	else if (mode == ITEMINFO_HINTONLY)
	{
		CCText Dline(itemBox.iX + 10, itemBox.iY + 10, itemBox.iWidth - 20, itemBox.iHeight - 20);
		Dline.setFont(tFont);
		Dline.setText(hint.c_str());		
		Dline.paint();
	}
}

//
void CCItemInfo::hide()
{
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
	else //FIXME:
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
}

//
void CCItemInfo::saveScreen(void)
{
	dprintf(DEBUG_DEBUG, "CCItemInfo::saveScreen\n");
	
	if (background)
	{
		delete [] background;
		background = NULL;
	}
		
	background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
		
	if (background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCItemInfo::restoreScreen(void)
{
	dprintf(DEBUG_DEBUG, "CCItemInfo::restoreScreen\n");
	
	//
	if (savescreen && background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

//
void CCItemInfo::enableSaveScreen()
{
	dprintf(DEBUG_DEBUG, "CCItemInfo::enableSaveScreen\n");
	
	savescreen = true;
	
	saveScreen();
}

////
CCWindow::CCWindow(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_INFO, "CCWindow::%s\n", __FUNCTION__);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	initFrames();

	initVars();
}

CCWindow::CCWindow(CBox* position)
{
	dprintf(DEBUG_INFO, "CCWindow::%s\n", __FUNCTION__);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;
	oldPosition = itemBox;

	initFrames();

	initVars();
}

CCWindow::~CCWindow()
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}
}

void CCWindow::initVars()
{
	dprintf(DEBUG_DEBUG, "CCWindow::%s\n", __FUNCTION__);

	radius = NO_RADIUS;
	corner = CORNER_NONE;
	bgcolor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = GRADIENT_ONECOLOR;
	//
	borderMode = BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	paintframe = true;
	//
	background = NULL;
	//
	current_page = 0;
	total_pages = 1;
	//
	cc_type = CC_WINDOW;
}

void CCWindow::setPosition(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCWindow::%s\n", __FUNCTION__);
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	initFrames();
}

void CCWindow::setPosition(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CCWindow::%s\n", __FUNCTION__);
	
	itemBox = *position;
	
	oldPosition = itemBox;
	
	initFrames();
}

void CCWindow::saveScreen()
{
	dprintf(DEBUG_DEBUG, "CCWindow::%s\n", __FUNCTION__);

	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[borderMode? (itemBox.iWidth)*(itemBox.iHeight) : itemBox.iWidth*itemBox.iHeight];
		
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCWindow::restoreScreen()
{
	dprintf(DEBUG_DEBUG, "CCWindow::%s\n", __FUNCTION__);
	
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CCWindow::paintPage(void)
{
	dprintf(DEBUG_DEBUG, "CCWindow::paintPage:\n");

	if (paintframe)
	{
		// border
		if (borderMode)
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, borderColor, radius, corner);
		
		// frame
		if (borderMode == BORDER_NO)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_ALL)
		{
			frameBuffer->paintBoxRel(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, itemBox.iHeight - 4, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			frameBuffer->paintBoxRel(itemBox.iX + 2, itemBox.iY, itemBox.iWidth - 4, itemBox.iHeight, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + 2, itemBox.iWidth, itemBox.iHeight - 4, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
	}
}

void CCWindow::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "CCWindow::%s\n", __FUNCTION__);
	
	////
	initFrames();

	//
	if (!paintframe)
	{
		saveScreen();
	}

	//
	paintPage();
}

void CCWindow::hide()
{
	dprintf(DEBUG_INFO, "CCWindow::%s\n", __FUNCTION__);
	
	if(!paintframe)
		restoreScreen();
	else
	{
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	}
		
	CFrameBuffer::getInstance()->blit();
}

void CCWindow::refresh(bool show)
{
	dprintf(DEBUG_INFO, "CCWindow::%s\n", __FUNCTION__);

	restoreScreen();
	paintPage();
}

//// CCPig
CCPig::CCPig(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCPig::CCPig: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	//
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	init();
}

CCPig::CCPig(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CCPig::CCPig:\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	//
	itemBox = *position;
	oldPosition = itemBox;

	init();
}

void CCPig::init()
{
	//
	cc_type = CC_PIG;
}

void CCPig::paint(bool _selected)
{
	dprintf(DEBUG_DEBUG, "CCPig::paint\n");
	
	////
	initFrames();
	
	frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);	
		

	if(videoDecoder)
		videoDecoder->Pig(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);	
}

void CCPig::hide()
{
	if(videoDecoder)  
		videoDecoder->Pig(-1, -1, -1, -1);

	frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	
	CFrameBuffer::getInstance()->blit();
}

//// CCHeaders
CCHeaders::CCHeaders(const int x, const int y, const int dx, const int dy, const char * const title, const char * const icon)
{
	dprintf(DEBUG_DEBUG, "CCHeaders::CCHeaders: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	htitle = title? title : "";
	hicon = icon? icon : "";

	//
	color = COL_MENUHEAD_PLUS_0;
	radius = g_settings.Head_radius;
	corner = g_settings.Head_corner;
	gradient = g_settings.Head_gradient;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = g_settings.Head_gradient_type;
	line = g_settings.Head_line;
	line_gradient = g_settings.Head_line_gradient;

	//
	paintframe = true;
	paintDate = false;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;
	//
	count	= 0;
	buttons.clear();
	mode = CCButtons::BUTTON_ICON;
	
	halign = CC_ALIGN_LEFT;
	
	paint_Head = true;
	has_Title = true;

	cc_type = CC_HEAD;
}

CCHeaders::CCHeaders(CBox* position, const char * const title, const char * const icon)
{
	dprintf(DEBUG_DEBUG, "CCHeaders::CCHeaders: x:%d y:%d dx:%d dy:%d\n", position->iX, position->iY, position->iWidth, position->iHeight);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;
	oldPosition = itemBox;

	htitle = title? title : "";
	hicon = icon? icon : "";
	//
	color = COL_MENUHEAD_PLUS_0;
	radius = g_settings.Head_radius;
	corner = g_settings.Head_corner;
	gradient = g_settings.Head_gradient;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = g_settings.Head_gradient_type;
	line = g_settings.Head_line;
	line_gradient = g_settings.Head_line_gradient;
	//
	paintframe = true;
	paintDate = false;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;
	//
	count	= 0;
	buttons.clear();
	mode = CCButtons::BUTTON_ICON;
	
	halign = CC_ALIGN_LEFT;
	
	paint_Head = true;
	has_Title = true;

	cc_type = CC_HEAD;
}

CCHeaders::~CCHeaders()
{
	if (timer)
	{
		timer->hide();
		delete timer;
		timer = NULL;
	}
	
	buttons.clear();
}

void CCHeaders::setButtons(const struct button_label* _button_labels, const int _count)		
{
	if (_count)
	{
		for (unsigned int i = 0; i < (unsigned int)_count; i++)
		{
			buttons.push_back(_button_labels[i]);
		}
	}

	count = buttons.size();
}

void CCHeaders::addButton(const char *btn, const char *lname, const fb_pixel_t col)
{
	dprintf(DEBUG_DEBUG, "CCHeaders::addButton: btn:%s name:%s\n", btn, lname);
	
	button_label_struct button;
	
	button.button = btn;
	button.localename = lname? lname : "";
	button.color = col;
	
	buttons.push_back(button);
	count++;
}		

void CCHeaders::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "CCHeaders::paint: (%s) (%s)\n", htitle.c_str(), hicon.c_str());
	
	//
	initFrames();
	
	// box
	if (paintframe)
		CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, color, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
	
	// line
	if (line)
		frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + itemBox.iHeight - 2, itemBox.iWidth, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

	// left icon
	int i_w = 0;
	int i_h = 0;

	if(!hicon.empty())
	{
		CFrameBuffer::getInstance()->getIconSize(hicon.c_str(), &i_w, &i_h);

		// scale icon
		if(i_h >= itemBox.iHeight)
		{
			i_h = itemBox.iHeight - 4;
			i_w = i_h*1.67;
		}

		CFrameBuffer::getInstance()->paintIcon(hicon.c_str(), itemBox.iX + BORDER_LEFT, itemBox.iY + (itemBox.iHeight - i_h)/2, 0, i_w, i_h);
	}

	// right buttons
	count = buttons.size();

	int iw[count], ih[count];
	int startx = itemBox.iX + itemBox.iWidth - BORDER_RIGHT;
	int buttonWidth = 0;
	int maxButtonTextWidth = buttonWidth;
	int f_w[count];

	if(count)
	{
		// get max width
		for (unsigned int i = 0; i < count; i++)
		{
			if (!buttons[i].localename.empty())
			{
				f_w[i] = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_(buttons[i].localename.c_str()));
				if (i > 0)
					maxButtonTextWidth = std::max(f_w[i], f_w[i - 1]) + 10;
				else
					maxButtonTextWidth = f_w[i] + 10;
			}
		}
		
		for (unsigned int i = 0; i < (unsigned int)count; i++)
		{
			if (mode == CCButtons::BUTTON_ICON)
			{
				if(!buttons[i].button.empty())
				{
					frameBuffer->getIconSize(buttons[i].button.c_str(), &iw[i], &ih[i]);
						
					// scale icon
					if(ih[i] >= itemBox.iHeight)
					{
						ih[i] = itemBox.iHeight - 2;
					}
					
					startx -= (iw[i] + ICON_TO_ICON_OFFSET);

					frameBuffer->paintIcon(buttons[i].button, startx, itemBox.iY + (itemBox.iHeight - ih[i])/2, 0, iw[i], ih[i]);
				}
			}
			else if (mode == CCButtons::BUTTON_FRAME)
			{
				startx -= (maxButtonTextWidth + 10 + ICON_TO_ICON_OFFSET);
							
				//
				CCFrameLine frame;
						
				frame.setPosition(startx, itemBox.iY + 4, maxButtonTextWidth + 10, itemBox.iHeight - 8);
				frame.paint();
					
				// label
				CCLabel label;
				label.setPosition(startx, itemBox.iY + 4, maxButtonTextWidth + 10, itemBox.iHeight - 8);
				label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
				label.setText(_(buttons[i].localename.c_str()));
				label.setHAlign(CC_ALIGN_CENTER);
				label.paint();
			}
		}
	}

	// paint time/date
	int timestr_len = 0;
	if(paintDate)
	{
		std::string timestr = getNowTimeStr(format.c_str());
		
		timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
		
		if (timer)
		{
			timer->hide();
			delete timer;
			timer = NULL;
		}
	
		timer = new CCTime(startx - timestr_len, itemBox.iY, timestr_len, itemBox.iHeight);

		timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
		timer->setFormat(format.c_str());
		
		timer->paint();
	}
	
	// title
	int startPosX = itemBox.iX + BORDER_LEFT + i_w + ICON_OFFSET;
	int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(htitle);
	
	if (halign == CC_ALIGN_CENTER)
		startPosX = itemBox.iX + (itemBox.iWidth >> 1) - (stringWidth >> 1);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, itemBox.iY + (itemBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - buttonWidth - (count - 1)*ICON_TO_ICON_OFFSET - timestr_len, htitle, COL_MENUHEAD_TEXT_PLUS_0);
}

void CCHeaders::hide()
{
	dprintf(DEBUG_INFO, "CCHeaders::hide:\n");
	
	if (paintframe)
		CFrameBuffer::getInstance()->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
		
	if (timer)
	{
		timer->hide();
		delete timer;
		timer = NULL;
	}
}

//// CCFooters
CCFooters::CCFooters(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CCFooters::CCFooters: x:%d y:%d dx:%d dy:%d\n", x, y, dx, dy);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	//
	paintframe = true;
	color = COL_MENUFOOT_PLUS_0;
	radius = g_settings.Foot_radius;
	corner = g_settings.Foot_corner;
	gradient = g_settings.Foot_gradient;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = g_settings.Foot_gradient_type;
	line = g_settings.Foot_line;
	line_gradient = g_settings.Foot_line_gradient;
	//
	buttons.clear();
	count = 0;
	button_width = itemBox.iWidth;
	mode = CCButtons::BUTTON_ICON;
	
	paint_Foot = true;
	
	//
	cc_type = CC_FOOT;
}

CCFooters::CCFooters(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CCFooters::CCFooters: x:%d y:%d dx:%d dy:%d\n", position->iX, position->iY, position->iWidth, position->iHeight);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;
	oldPosition = itemBox;

	//
	paintframe = true;
	color = COL_MENUFOOT_PLUS_0;
	radius = g_settings.Foot_radius;
	corner = g_settings.Foot_corner;
	gradient = g_settings.Foot_gradient;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = g_settings.Foot_gradient_type;
	line = g_settings.Foot_line;
	line_gradient = g_settings.Foot_line_gradient;
	//
	buttons.clear();
	count = 0;
	button_width = itemBox.iWidth;
	mode = CCButtons::BUTTON_ICON;
	
	paint_Foot = true;

	//
	cc_type = CC_FOOT;
}

CCFooters::~CCFooters()
{
	buttons.clear();
}

void CCFooters::setButtons(const struct button_label *_button_labels, const int _count, const int _button_width)
{
	if (_count)
	{
		for (unsigned int i = 0; i < (unsigned int)_count; i++)
		{
			buttons.push_back(_button_labels[i]);
		}
	}

	count = buttons.size();
	button_width = (_button_width == 0)? itemBox.iWidth : _button_width;	
}

void CCFooters::addButton(const char *btn, const char *lname, const fb_pixel_t col)
{
	dprintf(DEBUG_DEBUG, "CCFooters::addButton: btn:%s name:%s\n", btn, lname);
	
	button_label_struct button;
	
	button.button = btn;
	button.localename = lname? lname : "";
	button.color = col;
	
	buttons.push_back(button);
	count++;
}

void CCFooters::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "CCFooters::paint:\n");
	
	//
	initFrames();
	
	// box
	if (paintframe)
		CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, color, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
	
	// line
	if (line)
		frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

	int buttonWidth = 0;

	count = buttons.size();

	//
	int maxButtonTextWidth = buttonWidth;
	int iw[count];
	int ih[count];
	int f_w[count];
	
	if(count)
	{
		buttonWidth = (itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT)/count;
			
		// get maxButtonWidth
		for (unsigned int i = 0; i < count; i++)
		{
				if (!buttons[i].localename.empty())
				{
					f_w[i] = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_(buttons[i].localename.c_str()));
					
					if (i > 0)
						maxButtonTextWidth = std::max(f_w[i], f_w[i - 1]) + 10;
					else
						maxButtonTextWidth = f_w[i] + 10;
						
					if ((maxButtonTextWidth + 20) > buttonWidth)
						maxButtonTextWidth = buttonWidth;
				}
		}
			
		if (maxButtonTextWidth > buttonWidth)
			maxButtonTextWidth = buttonWidth;
		
		for (unsigned int i = 0; i < count; i++)
		{
			if (mode == CCButtons::BUTTON_ICON)
			{
				if (!buttons[i].button.empty())
				{
					iw[i] = 0;
					ih[i] = 0;

					CFrameBuffer::getInstance()->getIconSize(buttons[i].button.c_str(), &iw[i], &ih[i]);
							
					// scale icon
					if(ih[i] >= itemBox.iHeight)
					{
						ih[i] = itemBox.iHeight - 2;
					}
							
					int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
					
					CFrameBuffer::getInstance()->paintIcon(buttons[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + (itemBox.iHeight - ih[i])/2, 0, iw[i], ih[i]);

					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + f_h + (itemBox.iHeight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(buttons[i].localename.c_str()), COL_MENUFOOT_TEXT_PLUS_0, 0, true); // UTF-8
				}
			}
			else if (mode == CCButtons::BUTTON_FRAME)
			{
				//
				CCFrameLine frame;
						
				frame.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 4, buttonWidth, itemBox.iHeight - 4);
				frame.paint();
					
				// label
				CCLabel label;
				label.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 4, buttonWidth, itemBox.iHeight - 8);
				label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
				label.setHAlign(CC_ALIGN_CENTER);
				label.setText(_(buttons[i].localename.c_str()));
				label.paint();
			}
			else if (mode == CCButtons::BUTTON_COLOREDFRAME)
			{
				//
				CFrameItem frame;
				frame.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 4, buttonWidth, itemBox.iHeight - 8);
				frame.setBorderMode();
				frame.paintMainFrame(true);
				frame.setColor(buttons[i].color);
				frame.setCaptionFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
				frame.setTitle(_(buttons[i].localename.c_str()));
				frame.setHAlign(CC_ALIGN_CENTER);
				frame.paint();
			}
			else if (mode == CCButtons::BUTTON_COLOREDLINE)
			{
				// button label
				CCLabel label;
					
				label.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + 1, buttonWidth, itemBox.iHeight - 2);
				//label.setColor();
				label.setFont(SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL);
				label.setText(_(buttons[i].localename.c_str()));
				label.setHAlign(CC_ALIGN_CENTER);
				label.paint();
					
				//
				CCHline line;
					
				line.setPosition(itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - 10, buttonWidth, 8);
				line.setColor(buttons[i].color);
				//line.setGradient();
				line.paint();
			}
		}
	}
}

void CCFooters::hide()
{
	dprintf(DEBUG_INFO, "CFooters::hide:\n");
	
	if (paintframe)
		CFrameBuffer::getInstance()->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
}

//// Component helpers
//// CCScrollBar
void CCScrollBar::paint(const int x, const int y, const int dy, const int NrOfPages, const int CurrentPage)
{
	// scrollBar
	CBox cFrameScrollBar;
	CCWindow cScrollBarWindow;

	cFrameScrollBar.iX = x;
	cFrameScrollBar.iY = y;
	cFrameScrollBar.iWidth = SCROLLBAR_WIDTH;
	cFrameScrollBar.iHeight = dy;


	cScrollBarWindow.setPosition(&cFrameScrollBar);
	cScrollBarWindow.setColor(COL_MENUCONTENT_PLUS_1);
	cScrollBarWindow.setCorner(NO_RADIUS, CORNER_ALL);
	cScrollBarWindow.paint();
		
	// scrollBar slider
	CBox cFrameSlider;
	CCWindow cSliderWindow;	

	cFrameSlider.iX = cFrameScrollBar.iX + 2;
	cFrameSlider.iY = cFrameScrollBar.iY + CurrentPage*(cFrameScrollBar.iHeight/NrOfPages);
	cFrameSlider.iWidth = cFrameScrollBar.iWidth - 4;
	cFrameSlider.iHeight = cFrameScrollBar.iHeight/NrOfPages;

	cSliderWindow.setPosition(&cFrameSlider);
	cSliderWindow.setColor(COL_MENUCONTENT_PLUS_3);
	cSliderWindow.setCorner(NO_RADIUS, CORNER_ALL);
	cSliderWindow.paint();
}

void CCScrollBar::paint(CBox* position, const int NrOfPages, const int CurrentPage)
{
	// scrollBar
	CBox cFrameScrollBar;
	CCWindow cScrollBarWindow;

	cFrameScrollBar = *position;

	cScrollBarWindow.setPosition(&cFrameScrollBar);
	cScrollBarWindow.setColor(COL_MENUCONTENT_PLUS_1);
	cScrollBarWindow.setCorner(NO_RADIUS, CORNER_ALL);
	cScrollBarWindow.paint();
		
	// scrollBar slider
	CBox cFrameSlider;
	CCWindow cSliderWindow;	

	cFrameSlider.iX = cFrameScrollBar.iX + 2;
	cFrameSlider.iY = cFrameScrollBar.iY + CurrentPage*(cFrameScrollBar.iHeight/NrOfPages);
	cFrameSlider.iWidth = cFrameScrollBar.iWidth - 4;
	cFrameSlider.iHeight = cFrameScrollBar.iHeight/NrOfPages;

	cSliderWindow.setPosition(&cFrameSlider);
	cSliderWindow.setColor(COL_MENUCONTENT_PLUS_3);
	cSliderWindow.setCorner(NO_RADIUS, CORNER_ALL);
	cSliderWindow.paint();
}

