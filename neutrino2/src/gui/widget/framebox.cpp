//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: framebox.cpp 21122024 mohousch Exp $
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

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/framebox.h>
#include <gui/widget/textbox.h>

#include <gui/pluginlist.h>

#include <system/settings.h>
#include <system/debug.h>

#include <video_cs.h>


// globals
extern cVideo * videoDecoder;

// CFrameItem
CFrameItem::CFrameItem()
{
	captionFont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2; 
	optionFont = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;

	caption = "";
	mode = FRAME_BOX;

	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	paintFrame = true;
	pluginOrigName = false;

	iconName.clear();
	option.clear();

	jumpTarget = NULL;
	actionKey.clear();
	
	//
	fcolor = COL_MENUCONTENT_PLUS_0;
	radius = RADIUS_MID;
	corner = NO_RADIUS;
	gradient = NOGRADIENT;
	
	halign = CComponent::CC_ALIGN_LEFT;

	// init
	window.setPosition(-1, -1, -1, -1);
	
	// 
	active = true;
	marked = false;
	
	//
	parent = NULL;	
}

void CFrameItem::setMode(int m)
{
	mode = m;
			
	if ( (mode == FRAME_HLINE) || (mode == FRAME_VLINE) ) 
	{
		paintFrame = false;
		
		setActive(false);
	}
}

void CFrameItem::setActive(const bool Active)
{
	active = Active;
	
	//if (window.getWindowsPos().iX != -1)
	//	paint();
}

void CFrameItem::setMarked(const bool Marked)
{
	marked = Marked;
	
	//if (window.getWindowsPos().iX != -1)
	//	paint();
}

void CFrameItem::setPlugin(const char * const pluginName)
{
	if (mode == FRAME_PLUGIN)
	{
		if (g_PluginList->plugin_exists(pluginName))
		{
			unsigned int count = g_PluginList->find_plugin(pluginName);

			//iconName
			iconName = NEUTRINO_ICON_MENUITEM_PLUGIN;

			std::string icon("");
			icon = g_PluginList->getIcon(count);

			if(!icon.empty())
			{
					iconName = PLUGINDIR;
					iconName += "/";
					iconName += g_PluginList->getFileName(count);
					iconName += "/";
					iconName += g_PluginList->getIcon(count);
			}

			// caption
			if (caption.empty())
				caption = g_PluginList->getName(count);

			// option
			if (option.empty())
				option = g_PluginList->getDescription(count);

			// jumpTarget
			jumpTarget = CPluginsExec::getInstance();

			// actionKey
			actionKey = g_PluginList->getFileName(count);
		}
	}
}

int CFrameItem::paint(bool selected)
{
	dprintf(DEBUG_DEBUG, "CFrameItem::paint:\n");

	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = marked? COL_MENUCONTENTSELECTED_PLUS_1 : fcolor;

	if (selected)
	{
		if (parent)
		{
			if (parent->inFocus)
			{
					color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
					bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
			}
			else
			{
					color = COL_MENUCONTENT_TEXT_PLUS_0;	
					bgcolor = fcolor;
			}
		}
		else
		{
			color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
	}
	else if (!active)
	{
		//color = COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
		//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	// paint frameBackground
	window.setBorderMode(borderMode);
	window.setBorderColor(borderColor);
	window.setColor(bgcolor);
	window.setCorner(radius, corner);
	window.setGradient(gradient);
	window.paintMainFrame(paintFrame);
	
	//
	if (selected)	
		window.paint();
	else
		window.refresh();

	// icon
	int iw = 0;
	int ih = 0;
	int iconOffset = 0;

	if (mode == FRAME_BOX)
	{
		// icon
		if(!iconName.empty())
		{
			iconOffset = ICON_OFFSET;

			CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &iw, &ih);

			CFrameBuffer::getInstance()->paintIcon(iconName, window.getWindowsPos().iX + ICON_OFFSET, window.getWindowsPos().iY + (window.getWindowsPos().iHeight - ih)/2);
		}

		// caption
		if(!option.empty())
		{
			// caption
			if(!caption.empty())
			{
				int c_w = g_Font[captionFont]->getRenderWidth(caption);
				
				if (c_w > window.getWindowsPos().iWidth)
					c_w = window.getWindowsPos().iWidth;
				
				int startPosX = window.getWindowsPos().iX + iconOffset + iw + iconOffset;
			
				if (halign == CComponent::CC_ALIGN_CENTER)
					startPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (c_w >> 1);
				else if (halign == CComponent::CC_ALIGN_RIGHT)
					startPosX = window.getWindowsPos().iX + window.getWindowsPos().iWidth - c_w;

				g_Font[captionFont]->RenderString(startPosX, window.getWindowsPos().iY + 3 + g_Font[captionFont]->getHeight(), window.getWindowsPos().iWidth - iconOffset - iw - iconOffset, caption.c_str(), (bgcolor == COL_YELLOW_PLUS_0)? COL_BLACK_PLUS_0 : color, 0, true); //
			}

			// option
			if(!option.empty())
			{
				int o_w = g_Font[optionFont]->getRenderWidth(option);
				
				int o_startPosX = window.getWindowsPos().iX + iconOffset + iw + iconOffset;
			
				if (halign == CComponent::CC_ALIGN_CENTER)
					o_startPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (o_w >> 1);
				else if (halign == CComponent::CC_ALIGN_RIGHT)
					o_startPosX = window.getWindowsPos().iX + window.getWindowsPos().iWidth - o_w;

				g_Font[optionFont]->RenderString(o_startPosX, window.getWindowsPos().iY + window.getWindowsPos().iHeight, window.getWindowsPos().iWidth - iconOffset - iw - iconOffset, option.c_str(), color, 0, true);
			}
		}
		else
		{
			if(!caption.empty())
			{
				int c_w = g_Font[captionFont]->getRenderWidth(caption);
				
				if (c_w > window.getWindowsPos().iWidth)
					c_w = window.getWindowsPos().iWidth;
					
				int c_h = g_Font[captionFont]->getHeight();
				
				int startPosX = window.getWindowsPos().iX + iconOffset + iw + iconOffset;
			
				if (halign == CComponent::CC_ALIGN_CENTER)
					startPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (c_w >> 1);
				else if (halign == CComponent::CC_ALIGN_RIGHT)
					startPosX = window.getWindowsPos().iX + window.getWindowsPos().iWidth - c_w;

				g_Font[captionFont]->RenderString(startPosX, window.getWindowsPos().iY + (window.getWindowsPos().iHeight - c_h)/2 + g_Font[captionFont]->getHeight(), window.getWindowsPos().iWidth - iconOffset - iw - iconOffset, caption.c_str(), (bgcolor == COL_YELLOW_PLUS_0)? COL_BLACK_PLUS_0 : color);
			}
		}
	}
	else if (mode == FRAME_PICTURE)
	{
		int c_h = 0;
		int stringStartPosX = 0;
		int c_w = 0;

		if(!caption.empty())
			c_h = g_Font[captionFont]->getHeight() + 20;

		if (selected)
		{
			//
			if(!iconName.empty())
			{
				CFrameBuffer::getInstance()->displayImage(iconName, window.getWindowsPos().iX + 2, window.getWindowsPos().iY + 2, window.getWindowsPos().iWidth - 4, window.getWindowsPos().iHeight - c_h - 4);
			}

			//
			if(!caption.empty())
			{
				c_w = g_Font[captionFont]->getRenderWidth(caption);
				
				if (c_w > window.getWindowsPos().iWidth)
					c_w = window.getWindowsPos().iWidth;
					
				stringStartPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (c_w >> 1);

				g_Font[captionFont]->RenderString(stringStartPosX, window.getWindowsPos().iY + window.getWindowsPos().iHeight, window.getWindowsPos().iWidth - 6, caption.c_str(), color);
			}
		}
		else
		{
			//
			if(!iconName.empty())
			{
				CFrameBuffer::getInstance()->displayImage(iconName, window.getWindowsPos().iX + 10, window.getWindowsPos().iY + 10, window.getWindowsPos().iWidth - 20, window.getWindowsPos().iHeight - c_h - 20);
				
				if (!active)
				{
					CFrameBuffer::getInstance()->displayImage(iconName, window.getWindowsPos().iX, window.getWindowsPos().iY, window.getWindowsPos().iWidth, window.getWindowsPos().iHeight - c_h);
				}
			}

			//
			if(!caption.empty())
			{
				c_w = g_Font[captionFont]->getRenderWidth(caption);
				
				if (c_w > window.getWindowsPos().iWidth)
					c_w = window.getWindowsPos().iWidth;
					
				stringStartPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (c_w >> 1);

				g_Font[captionFont]->RenderString(stringStartPosX, window.getWindowsPos().iY + window.getWindowsPos().iHeight - 5, window.getWindowsPos().iWidth - 6, caption.c_str(), color);
			}
		}
	}
	else if (mode == FRAME_ICON)
	{
		iconOffset = ICON_OFFSET;

		if (!active)
			iconOffset = 0;

		// iconName
		if(caption.empty())
		{
			if(!iconName.empty())
			{
				CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &iw, &ih);

				CFrameBuffer::getInstance()->paintIcon(iconName, window.getWindowsPos().iX + (window.getWindowsPos().iWidth - iw)/2, window.getWindowsPos().iY + (window.getWindowsPos().iHeight - ih)/2);
			}
		}
		else
		{
			// iconName
			if(!iconName.empty())
			{
				CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &iw, &ih);

				CFrameBuffer::getInstance()->paintIcon(iconName, window.getWindowsPos().iX + iconOffset, window.getWindowsPos().iY + (window.getWindowsPos().iHeight - ih)/2);
			}		

			// caption
			int c_w = g_Font[captionFont]->getRenderWidth(caption);

			g_Font[captionFont]->RenderString(window.getWindowsPos().iX + iconOffset + iw + iconOffset, window.getWindowsPos().iY + g_Font[captionFont]->getHeight() + (window.getWindowsPos().iHeight - g_Font[captionFont]->getHeight())/2, window.getWindowsPos().iWidth - iconOffset - iw - iconOffset, caption.c_str(), color, 0, true); //
		}
	}
	else if (mode == FRAME_TEXT)
	{
		CCText textBox(window.getWindowsPos().iX, window.getWindowsPos().iY, window.getWindowsPos().iWidth, window.getWindowsPos().iHeight);

		textBox.setFont(captionFont);
		textBox.enableSaveScreen();

		// caption
		if(!caption.empty())
		{
			textBox.setText(caption.c_str());
		}
		
		textBox.paint();
	}
	else if (mode == FRAME_PLUGIN)
	{
		int c_h = 0;

		if(!caption.empty() && pluginOrigName)
			c_h = g_Font[captionFont]->getHeight() + 20;

		// icon
		if(!iconName.empty())
		{
			CFrameBuffer::getInstance()->displayImage(iconName, window.getWindowsPos().iX + 2, window.getWindowsPos().iY + 2, window.getWindowsPos().iWidth - 4, window.getWindowsPos().iHeight - 4 - c_h);
		}

		// caption
		if(!caption.empty() && pluginOrigName)
		{
			int c_w = g_Font[captionFont]->getRenderWidth(caption);

			g_Font[captionFont]->RenderString(window.getWindowsPos().iX + 2, window.getWindowsPos().iY + window.getWindowsPos().iHeight, window.getWindowsPos().iWidth - 4, caption.c_str(), color);
		}
	}
	else if (mode == FRAME_VLINE)
	{
		CFrameBuffer::getInstance()->paintVLineRel(window.getWindowsPos().iX, window.getWindowsPos().iY, window.getWindowsPos().iHeight, COL_MENUCONTENTDARK_PLUS_0);
	}
	else if (mode == FRAME_HLINE)
	{
		CFrameBuffer::getInstance()->paintHLineRel(window.getWindowsPos().iX, window.getWindowsPos().iWidth, window.getWindowsPos().iY, COL_MENUCONTENTDARK_PLUS_0);
	}
	else if (mode == FRAME_LABEL)
	{
		if(!caption.empty())
		{
			int c_w = g_Font[captionFont]->getRenderWidth(caption);
			
			if (c_w > window.getWindowsPos().iWidth)
				c_w = window.getWindowsPos().iWidth;
				
			int startPosX = window.getWindowsPos().iX;
			
			if (halign == CComponent::CC_ALIGN_CENTER)
				startPosX = window.getWindowsPos().iX + (window.getWindowsPos().iWidth >> 1) - (c_w >> 1);
			else if (halign == CComponent::CC_ALIGN_RIGHT)
				startPosX = window.getWindowsPos().iX + window.getWindowsPos().iWidth - c_w;
			
			g_Font[captionFont]->RenderString(startPosX, window.getWindowsPos().iY + g_Font[captionFont]->getHeight() + (window.getWindowsPos().iHeight - g_Font[captionFont]->getHeight())/2, c_w, caption.c_str(), color);
		}
	}	
	else if (mode == FRAME_PIG)
	{
		CFrameBuffer::getInstance()->paintBackgroundBoxRel(window.getWindowsPos().iX, window.getWindowsPos().iY, window.getWindowsPos().iWidth, window.getWindowsPos().iHeight);	
		

		if(videoDecoder)
			videoDecoder->Pig(window.getWindowsPos().iX, window.getWindowsPos().iY, window.getWindowsPos().iWidth, window.getWindowsPos().iHeight);
		
	}

	return 0;
}

int CFrameItem::exec(CMenuTarget *target)
{
	dprintf(DEBUG_NORMAL, "CFrameItem::exec: actionKey:(%s)\n", actionKey.c_str());

	int ret = CMenuTarget::RETURN_EXIT;

	if(jumpTarget)
		ret = jumpTarget->exec(target, actionKey);

	return ret;
}

//// CFrameBox
CFrameBox::CFrameBox(const int x, int const y, const int dx, const int dy)
{
	dprintf(DEBUG_INFO, "CFrameBox::CFrameBox:\n");

	frameBuffer = CFrameBuffer::getInstance();

	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;

	selected = -1;
	pos = 0;

	inFocus = true;

	cc_type = CC_FRAMEBOX;
	paintframe = true;
	
	//
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	background = NULL;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	
	//
	current_page = 0;
	total_pages = 1;
}

CFrameBox::CFrameBox(CBox* position)
{
	dprintf(DEBUG_INFO, "CFrameBox::CFrameBox:\n");

	frameBuffer = CFrameBuffer::getInstance();

	itemBox = *position;
	oldPosition = itemBox;

	selected = -1;
	pos = 0;

	inFocus = true;

	cc_type = CC_FRAMEBOX;
	paintframe = true;
	
	//
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	background = NULL;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	
	//
	current_page = 0;
	total_pages = 1;
}

CFrameBox::~CFrameBox()
{
	frames.clear();
	
	if (background)
	{
		delete[] background;
		background = NULL;
	}
}

void CFrameBox::addFrame(CFrameItem *frame, const bool defaultselected)
{
	if (frame != NULL)
	{
		if (defaultselected)
			selected = frames.size();
		
		frames.push_back(frame);
		frame->setParent(this);
	}
}

bool CFrameBox::hasItem()
{
	return !frames.empty();
}

void CFrameBox::initFrames()
{
	dprintf(DEBUG_INFO, "CFrameBox::initFrames:\n");
	
	// sanity check
	if(itemBox.iHeight > ((int)CFrameBuffer::getInstance()->getScreenHeight(true)))
		itemBox.iHeight = CFrameBuffer::getInstance()->getScreenHeight(true) - 4;  // 4 pixels for border

	// sanity check
	if(itemBox.iWidth > (int)CFrameBuffer::getInstance()->getScreenWidth(true))
		itemBox.iWidth = CFrameBuffer::getInstance()->getScreenWidth(true) - 4; // 4 pixels for border
}

void CFrameBox::paintFrames()
{
	dprintf(DEBUG_INFO, "CFrameBox::paintFrames:\n");

	for (unsigned int count = 0; count < (unsigned int)frames.size(); count++) 
	{
		CFrameItem *frame = frames[count];
		
		//
		if((frame->isSelectable()) && (selected == -1)) 
		{
			selected = count;
		} 

		frame->paint( selected == ((signed int) count));
	}
}

void CFrameBox::paint()
{
	dprintf(DEBUG_INFO, "CFrameBox::paint:\n");
	
	//
	initFrames();

	//
	if (paintframe)
	{
		// border
		if (borderMode != CComponent::BORDER_NO)
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, borderColor, radius, corner);
		
		// mainframe
		frameBuffer->paintBoxRel(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, itemBox.iHeight - 4, bgcolor, radius, corner, NOGRADIENT);		
	}
	else
	{
		saveScreen();
	}

	paintFrames();
}

void CFrameBox::saveScreen()
{
	dprintf(DEBUG_INFO, "CFrameBox::saveScreen:\n");

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

void CFrameBox::restoreScreen()
{
	dprintf(DEBUG_INFO, "CFrameBox::restoreScreen:\n");
	
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CFrameBox::hide()
{
	dprintf(DEBUG_INFO, "CFrameBox::hide:\n");
	
	// reset pig
	if (hasItem())
	{
		for (int i = 0; i < frames.size(); i++)
		{
			if (frames[i]->getMode() == CFrameItem::FRAME_PIG)
			{
				if(videoDecoder)  
					videoDecoder->Pig(-1, -1, -1, -1);
			}
		}
	}
	
	if(!paintframe && background)
	{
		restoreScreen();
	}
	else
	{
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	}
	
	frameBuffer->blit();
}

bool CFrameBox::isSelectable(void)
{
	if (hasItem())
	{
		for (int i = 0; i < frames.size(); i++)
		{
			if (frames[i]->isSelectable())
				return true;
		}
	}

	return false;
}

int CFrameBox::swipRight()
{
	dprintf(DEBUG_INFO, "CFrameBox::swipRight:\n");
	
	int ret = CMenuTarget::RETURN_NONE;

	for (unsigned int count = 1; count < frames.size(); count++) 
	{
		pos = (selected + count)%frames.size();

		CFrameItem * frame = frames[pos];

		if(frame->isSelectable())
		{
			frames[selected]->paint(false);
			frame->paint(true);

			selected = pos;
				
			break;
		}
	}
	
	return ret;
}

int CFrameBox::swipLeft()
{
	dprintf(DEBUG_INFO, "CFrameBox::swipLeft:\n");
	
	int ret = CMenuTarget::RETURN_NONE;

	for (unsigned int count = 1; count < frames.size(); count++) 
	{
		pos = selected - count;
		if ( pos < 0 )
			pos += frames.size();

		CFrameItem * frame = frames[pos];

		if(frame->isSelectable())
		{
			frames[selected]->paint(false);
			frame->paint(true);

			selected = pos;

			break;
		}
	}
	
	return ret;
}

void CFrameBox::scrollLineDown(const int lines)
{
	dprintf(DEBUG_INFO, "CFrameBox::scrollLineDown:\n");

	for (unsigned int count = 1; count < frames.size(); count++) 
	{
		pos = (selected + count)%frames.size();

		CFrameItem * frame = frames[pos];

		if(frame->isSelectable())
		{
			frames[selected]->paint(false);
			frame->paint(true);

			selected = pos;

			break;
		}
	}
}

void CFrameBox::scrollLineUp(const int lines)
{
	dprintf(DEBUG_INFO, "CFrameBox::scrollLineUp:\n");

	for (unsigned int count = 1; count < frames.size(); count++) 
	{
		pos = selected - count;
		if ( pos < 0 )
			pos += frames.size();

		CFrameItem * frame = frames[pos];

		if(frame->isSelectable())
		{
			frames[selected]->paint(false);
			frame->paint(true);

			selected = pos;

			break;
		}
	}
}

int CFrameBox::oKKeyPressed(CMenuTarget *target, neutrino_msg_t _msg)
{
	dprintf(DEBUG_INFO, "CFrameBox::okKeyPressed:\n");
	
	int ret = CMenuTarget::RETURN_EXIT;

	if (hasItem() && selected >= 0 && frames[selected]->isSelectable())
	{
		actionKey = frames[selected]->actionKey;
		frames[selected]->msg = _msg;
	}

	ret = frames[selected]->exec(target);
		
	return ret;
}

