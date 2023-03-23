/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: menue.cpp 2018/07/25 mohousch Exp $

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

#include <unistd.h> //acces
#include <cctype>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/menue.h>
#include <gui/widget/icons.h>
#include <gui/widget/textbox.h>
#include <gui/widget/stringinput.h> // locked menu

#include <driver/color.h>
#include <gui/pluginlist.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <system/debug.h>
#include <system/settings.h>
#include <system/helpers.h>
	

extern CPlugins * g_PluginList;    // defined in neutrino.cpp

/// CMenuWidget
CMenuWidget::CMenuWidget()
{
        l_name = "";

	Init("", MENU_WIDTH, MENU_HEIGHT);
}

CMenuWidget::CMenuWidget(const char* Name, const std::string & Icon, const int mwidth, const int mheight)
{
        l_name = Name;

	Init(Icon, mwidth, mheight);
}

void CMenuWidget::Init(const std::string &Icon, const int mwidth, const int mheight)
{
        frameBuffer = CFrameBuffer::getInstance();
        iconfile = Icon;
        selected = -1;
        width = mwidth;
	height = mheight;
	
        if(width > (int) frameBuffer->getScreenWidth() - 20)
		width = frameBuffer->getScreenWidth() - 20;

	if(height > ((int)frameBuffer->getScreenHeight() - 20))
		height = frameBuffer->getScreenHeight() - 20;

	full_width = width;
	full_height = height;

	wanted_width = width;
	wanted_height = height;

	exit_pressed = false;
        current_page = 0;

	offx = 0;
	offy = 0;
	x = 0;
	y = 0;
	
	//
	savescreen = false;
	background = NULL;
	
	bgcolor = COL_MENUCONTENT_PLUS_0;
	def_color = true;

	// head
	paintDate = false;
	//timestr_len = 0;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;
	hbutton_count	= 0;
	hbutton_labels.clear();
	hheight = 0;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = g_settings.Head_corner;
	headGradient = g_settings.Head_gradient;
	def_headColor = true;
	def_headRadius = true;
	def_headCorner = true;
	def_headGradient = true;
	thalign = CC_ALIGN_LEFT;
	head_line = g_settings.Head_line;
	
	// foot
	fbutton_count	= 0;
	fbutton_labels.clear();
	fbutton_width = width;
	fheight = 0;
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = g_settings.Foot_corner;
	footGradient = g_settings.Foot_gradient;
	def_footColor = true;
	def_footRadius = true;
	def_footCorner = true;
	def_footGradient = true;
	foot_line = g_settings.Foot_line;

	// footInfo
	paintFootInfo = false;
	footInfoHeight = 0;
	cFrameFootInfoHeight = 0;
	footInfoMode = ITEMINFO_INFO_MODE;

	timeout = g_settings.timing[SNeutrinoSettings::TIMING_MENU];
	sec_timer_id = 0;

	//
	widgetType = TYPE_STANDARD;
	shrinkMenu = false;
	widgetMode = MODE_MENU;
	
	//
	cnt = 0;

	// frame
	itemsPerX = 6;
	itemsPerY = 3;
	maxItemsPerPage = itemsPerX*itemsPerY;
	
	menu_position = MENU_POSITION_CENTER;

	actionKey = "";

	textBox = NULL;
	
	item_height = 0;
	item_width = 0;
	
	items_width = 0;
	items_height = 0;
}

void CMenuWidget::move(int xoff, int yoff)
{
	offx = xoff;
	offy = yoff;
}

CMenuWidget::~CMenuWidget()
{
	dprintf(DEBUG_INFO, "CMenuWidget:: del (%s)\n", l_name.c_str());

	items.clear();
	page_start.clear();
}

void CMenuWidget::addItem(CMenuItem *menuItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	
	items.push_back(menuItem);
}

void CMenuWidget::removeItem(long pos)
{
	items.erase(items.begin() + pos); 
}

bool CMenuWidget::hasItem()
{
	return !items.empty();
}

void CMenuWidget::initFrames()
{
	dprintf(DEBUG_INFO, "CMenuWidget::initFrames:\n");
	
	// reinit
	width = wanted_width;
	height = wanted_height;
	hheight = 0;
	fheight = 0;
	cFrameFootInfoHeight = 0;

	// widgettype forwarded to item 
	for (unsigned int count = 0; count < items.size(); count++) 
	{
		CMenuItem * item = items[count];

		item->widgetType = widgetType;
		//item->widgetMode = widgetMode;
	} 

	// init frames
	if(widgetType == TYPE_FRAME)
	{
		//
		page_start.clear();
		page_start.push_back(0);
		total_pages = 1;

		for (unsigned int i = 0; i < items.size(); i++) 
		{
			if(i == maxItemsPerPage*total_pages)
			{
				page_start.push_back(i);
				total_pages++;
			}
		}

		page_start.push_back(items.size());

		//
		x =  frameBuffer->getScreenX();
		y =  frameBuffer->getScreenY();
		width =  frameBuffer->getScreenWidth();
		height =  frameBuffer->getScreenHeight();

		full_width = width;
		full_height = height;

		//head height
		hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	
		// foot height
		fheight = fbutton_count? hheight : 0;
		
		// footInfoHeight
		cFrameFootInfoHeight = hheight;

		//
		item_width = width/itemsPerX;
		item_height = (height - hheight - cFrameFootInfoHeight - (fbutton_count? fheight : 0) - 20)/itemsPerY; // 10 pixels for hlines top 10 pixels bottom

		for (unsigned int count = 0; count < items.size(); count++) 
		{
			CMenuItem * item = items[count];

			item->item_width = item_width;
			item->item_height = item_height;
		}		 
	}
	else
	{
		// footInfo height
		if(paintFootInfo)
		{
			if( (widgetType == TYPE_STANDARD) || (widgetType == TYPE_CLASSIC && widgetMode == MODE_LISTBOX) )
			{
				cFrameFootInfoHeight = footInfoHeight;
			}
		}

		// head height
		hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	
		// foot height
		fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;

		// calculate some values
		int itemHeightTotal = 0;
		item_height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 3;
		int heightCurrPage = 0;
		page_start.clear();
		page_start.push_back(0);
		total_pages = 1;
		int heightFirstPage = 0;
	
		for (unsigned int i = 0; i < items.size(); i++) 
		{
			item_height = items[i]->getHeight();
			itemHeightTotal += item_height;
			heightCurrPage += item_height;

			if( (heightCurrPage + hheight + fheight + cFrameFootInfoHeight) > height)
			{
				page_start.push_back(i);
				heightFirstPage = std::max(heightCurrPage - item_height, heightFirstPage);
				total_pages++;
				heightCurrPage = item_height;
			}
		}

		heightFirstPage = std::max(heightCurrPage, heightFirstPage);
		page_start.push_back(items.size());

		// icon offset
		iconOffset = 0;

		for (unsigned int i = 0; i < items.size(); i++) 
		{
			if ((!(items[i]->iconName.empty())) || CRCInput::isNumeric(items[i]->directKey))
			{
				iconOffset = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
				break;
			}
		}

		// shrink menu if less items
		if(shrinkMenu)
		{
			if (hasItem())
				height = std::min(height, hheight + heightFirstPage + fheight + cFrameFootInfoHeight);
		}

		//
		full_width = width;
		full_height = height;
		
		// position
		// default centered
		x = offx + frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - full_width ) >> 1 );
		y = offy + frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );

		// menu position
		if(menu_position == MENU_POSITION_CENTER)
		{
			x = offx + frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - full_width ) >> 1 );
			y = offy + frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
		}
		else if(menu_position == MENU_POSITION_LEFT)
		{
			x = offx + frameBuffer->getScreenX();
			y = offy + frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
		}
		else if(menu_position == MENU_POSITION_RIGHT)
		{
			x = offx + frameBuffer->getScreenX() + frameBuffer->getScreenWidth() - full_width;
			y = offy + frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
		}
	}
	
	if(savescreen)
		saveScreen();
}

void CMenuWidget::paintHead()
{
	dprintf(DEBUG_DEBUG, "CMenuWidget::paintHead:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		// headBoxox
		frameBuffer->paintBoxRel(x, y, width, hheight, def_headColor? COL_MENUHEAD_PLUS_0 : headColor/*, headRadius, headCorner, headGradient*/);

		// paint horizontal line top
		frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + hheight - 2, COL_MENUCONTENT_PLUS_5);
		
		// icon
		int i_w = 0;
		int i_h = 0;

		frameBuffer->getIconSize(iconfile.c_str(), &i_w, &i_h);
			
		if(i_h >= hheight)
		{
			i_h = hheight - 4;
			//i_w = i_h*1.67;
		}
				
		frameBuffer->paintIcon(iconfile, x + BORDER_LEFT, y + (hheight - i_h)/2, 0, true, i_w, i_h);

		// Buttons
		int iw[hbutton_count], ih[hbutton_count];
		int xstartPos = x + width - BORDER_RIGHT;
		int buttonWidth = 0; //FIXME

		if (hbutton_count)
		{
			for (int i = 0; i < hbutton_count; i++)
			{
				if (!hbutton_labels[i].button.empty())
				{
					frameBuffer->getIconSize(hbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
					
					// scale icon
					if(ih[i] >= hheight)
					{
						ih[i] = hheight - 4;
					}
			
					xstartPos -= (iw[i] + ICON_TO_ICON_OFFSET);
					buttonWidth += iw[i];

					CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, y + (hheight - ih[i])/2, 0, true, iw[i], ih[i]);
				}
			}
		}

		// paint time/date
		int timestr_len = 0;
		if(paintDate)
		{
			std::string timestr = getNowTimeStr(format.c_str());
			
			timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
		
			timer = new CCTime();
			timer->setPosition(xstartPos - timestr_len - 2, y, timestr_len, hheight);
			timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
			timer->setFormat(format.c_str());
			timer->paint();
		}
		
		int startPosX = x + BORDER_LEFT + i_w + ICON_OFFSET;
		int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name.c_str());
	
		if (thalign == CC_ALIGN_CENTER)
			startPosX = x + (width >> 1) - (stringWidth >> 1);

		// title
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, y + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - buttonWidth - (hbutton_count)*ICON_TO_ICON_OFFSET - timestr_len, l_name.c_str(), COL_MENUHEAD);
	}
	else
	{
		// headBox
		frameBuffer->paintBoxRel(x, y, width, hheight, def_headColor? COL_MENUHEAD_PLUS_0 : headColor, g_settings.Head_radius, g_settings.Head_corner, g_settings.Head_gradient);
		
		// paint horizontal line top
		if (head_line)
			frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + hheight - 2, COL_MENUCONTENT_PLUS_5);
	
		//paint icon (left)
		int i_w = 0;
		int i_h = 0;
			
		frameBuffer->getIconSize(iconfile.c_str(), &i_w, &i_h);
			
		// limit icon height
		if(i_h >= hheight)
		{
			i_h = hheight - 4;
			//i_w = i_h*1.67;
		}
				
		frameBuffer->paintIcon(iconfile, x + BORDER_LEFT, y + (hheight - i_h)/2, 0, true, i_w, i_h);

		// Buttons
		int iw[hbutton_count], ih[hbutton_count];
		int xstartPos = x + width - BORDER_RIGHT;
		int buttonWidth = 0; //FIXME

		if (hbutton_count)
		{
			for (unsigned int i = 0; i < (unsigned int)hbutton_count; i++)
			{
				if (!hbutton_labels[i].button.empty())
				{
					frameBuffer->getIconSize(hbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
					
					// scale icon
					if(ih[i] >= hheight)
					{
						ih[i] = hheight - 4;
					}
					
					xstartPos -= (iw[i] + ICON_TO_ICON_OFFSET);
					buttonWidth += iw[i];

					CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, y + (hheight - ih[i])/2, 0, true, iw[i], ih[i]);
				}
			}
		}

		// paint time/date
		int timestr_len = 0;
		if(paintDate)
		{
			std::string timestr = getNowTimeStr(format.c_str());
			
			timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
		
			timer = new CCTime();
			timer->setPosition(xstartPos - timestr_len - 2, y, timestr_len, hheight);
			timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
			timer->setFormat(format.c_str());
			timer->paint();
		}
	
		// head title
		int startPosX = x + BORDER_LEFT + i_w + ICON_OFFSET;
		int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name.c_str());
	
		if (thalign == CC_ALIGN_CENTER)
			startPosX = x + (width >> 1) - (stringWidth >> 1);
			
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, y + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), width - BORDER_RIGHT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - timestr_len - buttonWidth - (hbutton_count)*ICON_TO_ICON_OFFSET, l_name.c_str(), COL_MENUHEAD, 0, true); // UTF-8
	}
}

void CMenuWidget::paintFoot()
{
	dprintf(DEBUG_INFO, "CMenuWidget::paintFoot:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		if(fbutton_count)
		{
			//
			frameBuffer->paintBoxRel(x, y + height - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor/*, footRadius, footCorner, footGradient*/);

			// paint horizontal line buttom
			frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - fheight + 2, COL_MENUCONTENT_PLUS_5);

			// buttons
			int buttonWidth = 0;
			int iw[fbutton_count]; 
			int ih[fbutton_count];

			buttonWidth = (fbutton_width)/fbutton_count;
	
			for (unsigned int i = 0; i < (unsigned int)fbutton_count; i++)
			{
				if (!fbutton_labels[i].button.empty())
				{
					iw[i] = 0;
					ih[i] = 0;

					CFrameBuffer::getInstance()->getIconSize(fbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
						
					if(ih[i] >= fheight)
					{
						ih[i] = fheight - 4;
					}
				
					int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
		
					CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, x + BORDER_LEFT + i*buttonWidth, y + height - fheight + (fheight - ih[i])/2, 0, true, iw[i], ih[i]);

					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, y + height - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT, 0, true); // UTF-8
				}
			}
		}
	}
	else
	{
		// footBox	
		frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, g_settings.Foot_gradient);
		
		// paint horizontal line buttom
		if (foot_line)
			frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);
		
		// buttons
		int buttonWidth = 0;
		int iw[fbutton_count]; 
		int ih[fbutton_count];

		if(fbutton_count)
		{
			buttonWidth = (fbutton_width)/fbutton_count;
	
			for (unsigned int i = 0; i < (unsigned int)fbutton_count; i++)
			{
				if (!fbutton_labels[i].button.empty())
				{
					iw[i] = 0;
					ih[i] = 0;

					CFrameBuffer::getInstance()->getIconSize(fbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
						
					if(ih[i] >= fheight)
					{
						ih[i] = fheight - 4;
					}
						
					int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
		
					CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, x + BORDER_LEFT + i*buttonWidth, y + height - cFrameFootInfoHeight - fheight + (fheight - ih[i])/2, 0, true, iw[i], ih[i]);

					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, y + height - cFrameFootInfoHeight - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT, 0, true); // UTF-8
				}
			}
		}
	}
}

void CMenuWidget::paint()
{
	dprintf(DEBUG_INFO, "CMenuWidget::paint: (%s)\n", l_name.c_str());

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8 );
	
	//
	initFrames();
	
	//
	paintHead();
	paintFoot();

	//
	paintItems();
}

// paint items
void CMenuWidget::paintItems()
{
	dprintf(DEBUG_INFO, "CMenuWidget::paintItems:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		item_start_y = y + hheight + 10;
		items_height = height - hheight - fheight - cFrameFootInfoHeight - 20;  //TEST
		items_width = width;
		
		// items background
		frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight - fheight, def_color? COL_MENUCONTENT_PLUS_0 : bgcolor);

		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}

		// reset items
		for (unsigned int i = 0; i < items.size(); i++) 
		{
			CMenuItem * item = items[i];	
			item->init(-1, 0, 0, 0);
		}

		int count = (int)page_start[current_page];

		if(items.size() > 0)
		{
			for (int _y = 0; _y < itemsPerY; _y++)
			{
				for (int _x = 0; _x < itemsPerX; _x++)
				{
					CMenuItem * item = items[count];

					item->init(x + _x*item_width, item_start_y + _y*item_height, item_width, iconOffset);

					if( (item->isSelectable()) && (selected == -1)) 
					{
						selected = count;
					} 

					if (selected == count) 
					{
						paintItemInfo(count);
					}

					item->paint( selected == ((signed int) count));

					count++;

					if ( (count == (int)page_start[current_page + 1]) || (count == (int)items.size()))
					{
						break;
					}
				}

				if ( (count == (int)page_start[current_page + 1]) || (count == (int)items.size()))
				{
					break;
				}		
			}
		}
	}
	else
	{
		item_start_y = y + hheight;
		
		// items height
		items_height = height - hheight - fheight - cFrameFootInfoHeight;
	
		// items width
		sb_width = 0;
	
		if(total_pages > 1)
			sb_width = SCROLLBAR_WIDTH; 
	
		items_width = width - sb_width;

		// extended
		if(widgetType == TYPE_EXTENDED)
		{
			items_width = 2*(width/3) - sb_width;
		}
	
		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}
	
		// paint items background
		frameBuffer->paintBoxRel(x, item_start_y, width, items_height, def_color? COL_MENUCONTENT_PLUS_0 : bgcolor);
		
		if(widgetType == TYPE_EXTENDED && widgetMode == MODE_MENU)
		{
			frameBuffer->paintBoxRel(x + items_width, item_start_y, width - items_width, items_height, COL_MENUCONTENTDARK_PLUS_0);
		}
	
		// paint right scrollBar if we have more then one page
		if(total_pages > 1)
		{
			if(widgetType == TYPE_EXTENDED)
				scrollBar.paint(x + 2*(width/3) - SCROLLBAR_WIDTH, item_start_y, items_height, total_pages, current_page);
			else
				scrollBar.paint(x + width - SCROLLBAR_WIDTH, item_start_y, items_height, total_pages, current_page);
		}

		// paint items
		int ypos = item_start_y;
		int xpos = x;
	
		for (unsigned int count = 0; count < items.size(); count++) 
		{
			CMenuItem * item = items[count];

			if ((count >= page_start[current_page]) && (count < page_start[current_page + 1])) 
			{
				item->init(xpos, ypos, items_width, iconOffset);

				if( (item->isSelectable()) && (selected == -1)) 
				{
					selected = count;
				} 
	
				// paint itemInfo
				if (selected == (signed int)count) 
				{
					paintItemInfo(count);
				}

				// paint item
				ypos = item->paint(selected == ((signed int) count));
			} 
			else 
			{
				// x = -1 is a marker which prevents the item from being painted on setActive changes
				item->init(-1, 0, 0, 0);
			}	
		} 
	}
}

void CMenuWidget::paintItemInfo(int pos)
{
	dprintf(DEBUG_INFO, "CMenuWidget::paintItemInfo:\n");
	
	if(widgetType == TYPE_STANDARD)
	{
		if(widgetMode == MODE_MENU || widgetMode == MODE_SETUP)
		{
			if(paintFootInfo)
			{
				//
				CMenuItem* item = items[pos];
				
				std::string icon;
				icon.clear();
				
				if (item->isPlugin)
					icon = item->itemIcon;
				else				
					icon = g_settings.hints_dir + item->itemIcon.c_str() + ".png";
				
				itemsLine.setMode(DL_HINT);
				itemsLine.setHint(item->itemHint.c_str());
				itemsLine.setIcon(icon.c_str());
					
				itemsLine.paint(x, y, width, height - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
				
				// info button
				int iw, ih = 0;
				
				if(fbutton_count == 0)
				{
					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
						
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
							
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);
				}
			}
			else 
			{
				if(fbutton_count == 0)
				{
					CMenuItem* item = items[pos];

					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					int iw, ih;
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
				
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);

					// Hint
					if(!item->itemHint.empty())
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + iw + ICON_OFFSET, y + height - fheight - cFrameFootInfoHeight + (fheight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - iw, item->itemHint.c_str(), COL_MENUFOOT, 0, true); // UTF-8
					}
				}
			}
		}
		else if(widgetMode == MODE_LISTBOX)
		{
			if(paintFootInfo)
			{
				if (footInfoMode == ITEMINFO_INFO_MODE)
				{
					CMenuItem * item = items[pos];
	
					// 
					itemsLine.setMode(DL_INFO);
					itemsLine.setInfo1(item->info1.c_str());
					itemsLine.setOptionInfo1(item->option_info1.c_str());
					itemsLine.setInfo2(item->info2.c_str());
					itemsLine.setOptionInfo2(item->option_info2.c_str());
					
					itemsLine.paint(x, y, width, height - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
				}
				else if (footInfoMode == ITEMINFO_HINT_MODE)
				{
					CMenuItem* item = items[pos];
	
					// detailslines|box
					itemsLine.setMode(DL_HINT);
					itemsLine.setHint(item->itemHint.c_str());
					itemsLine.setIcon(item->itemIcon.c_str());
				
					itemsLine.paint(x, y, width, height - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
				}
				
				// info button
				int iw, ih = 0;
			
				if(fbutton_count == 0)
				{
					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
						
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
							
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);
				}
			}
			else 
			{
				if(fbutton_count == 0)
				{
					CMenuItem* item = items[pos];

					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					int iw, ih;
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
				
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);

					// Hint
					if(!item->itemHint.empty())
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + iw + ICON_OFFSET, y + height - fheight - cFrameFootInfoHeight + (fheight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - iw, item->itemHint.c_str(), COL_MENUFOOT, 0, true); // UTF-8
					}
				}
			}
		}
	}
	else if(widgetType == TYPE_CLASSIC)
	{
		if(widgetMode == MODE_MENU)
		{
			if(fbutton_count == 0)
			{
				CMenuItem* item = items[pos];

				// refresh box
				frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
				// paint horizontal line buttom
				if (foot_line)
					frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

				// info icon
				int iw, ih;
				frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
				// limit icon height
				if(ih >= fheight)
					ih = fheight;
						
				frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);

				// Hint
				if(!item->itemHint.empty())
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + iw + ICON_OFFSET, y + height - cFrameFootInfoHeight - fheight + (fheight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - iw, item->itemHint.c_str(), COL_MENUFOOT, 0, true); // UTF-8
				}
			}
		}
		else if(widgetMode == MODE_LISTBOX)
		{
			if(paintFootInfo)
			{
				if (footInfoMode == ITEMINFO_INFO_MODE)
				{
					CMenuItem * item = items[pos];
	
					// detailslines
					itemsLine.setMode(DL_INFO);
					itemsLine.setInfo1(item->info1.c_str());
					itemsLine.setOptionInfo1(item->option_info1.c_str());
					itemsLine.setInfo2(item->info2.c_str());
					itemsLine.setOptionInfo2(item->option_info2.c_str());
					
					itemsLine.paint(x, y, width, height - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
				}
				else if (footInfoMode == ITEMINFO_HINT_MODE)
				{
					CMenuItem* item = items[pos];
	
					// detailslines
					itemsLine.setMode(DL_HINT);
					itemsLine.setHint(item->itemHint.c_str());
					itemsLine.setIcon(item->itemIcon.c_str());
				
					itemsLine.paint(x, y, width, height - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
				}
				
				// info button
				int iw, ih = 0;
				
				if(fbutton_count == 0)
				{
					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
						
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
							
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);
				}
			}
			else 
			{
				if(fbutton_count == 0)
				{
					CMenuItem* item = items[pos];

					// refresh box
					frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
					
					// paint horizontal line buttom
					if (foot_line)
						frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

					// info icon
					int iw, ih;
					frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
					// limit icon height
					if(ih >= fheight)
						ih = fheight;
				
					frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);

					// Hint
					if(!item->itemHint.empty())
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + iw + ICON_OFFSET, y + height - cFrameFootInfoHeight - fheight + (fheight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - iw, item->itemHint.c_str(), COL_MENUFOOT, 0, true); // UTF-8
					}
				}
			}
		}
	}
	else if(widgetType == TYPE_EXTENDED)
	{
		CMenuItem* item = items[pos];

		if(widgetMode == MODE_MENU)
		{
			int iw, ih;

			// info button
			if(fbutton_count == 0)
			{
				// refresh box
				frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
				
				// paint horizontal line buttom
				if (foot_line)
					frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

				// info icon
				frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
				// limit icon height
				if(ih >= fheight)
					ih = fheight;
						
				frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);

				// itemHint
				if(!item->itemHint.empty())
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT + iw + ICON_OFFSET, y + height - cFrameFootInfoHeight - fheight + (fheight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT - iw, item->itemHint.c_str(), COL_MENUFOOT, 0, true); // UTF-8
				}
			}

			// item icon (right)
			// check for minimum hight
			if(height - hheight - fheight >= ITEM_ICON_H)
			{ 
				frameBuffer->getIconSize(item->itemIcon.c_str(), &iw, &ih);

				// refreshbox
				frameBuffer->paintBoxRel(x + items_width + (width - items_width - ITEM_ICON_W)/2, y + (height - ITEM_ICON_H)/2, ITEM_ICON_W, ITEM_ICON_H, COL_MENUCONTENTDARK_PLUS_0);

				// itemIcom
				frameBuffer->paintHintIcon(item->itemIcon.c_str(), x + items_width + (width - items_width - ITEM_ICON_W)/2, y + (height - ITEM_ICON_H)/2, ITEM_ICON_W, ITEM_ICON_H);
			}
		}
		else if(widgetMode == MODE_LISTBOX)
		{
			// scale pic
			int p_w = 0;
			int p_h = 0;

			std::string fname = item->itemIcon;

			::scaleImage(fname, &p_w, &p_h);
				
			if(textBox)
			{
				delete textBox;
				textBox = NULL;
			}

			textBox = new CTextBox(x + 2*(width/3), y + hheight, width/3, items_height);

			textBox->setBackgroundColor(COL_MENUCONTENTDARK_PLUS_0);

			// hint
			textBox->setText(item->itemHint.c_str(), item->itemIcon.c_str(), p_w, p_h, PIC_CENTER);
			textBox->paint();
			
			// info button
			int iw, ih = 0;
			
			if(fbutton_count == 0)
			{
				// refresh box
				frameBuffer->paintBoxRel(x, y + height - cFrameFootInfoHeight - fheight, width, fheight, def_footColor? COL_MENUFOOT_PLUS_0 : footColor, g_settings.Foot_radius, g_settings.Foot_corner, def_footGradient? g_settings.Foot_gradient : footGradient);
				
				// paint horizontal line buttom
				if (foot_line)
					frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);

				// info icon
				frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
					
				// limit icon height
				if(ih >= fheight)
					ih = fheight;
						
				frameBuffer->paintIcon(NEUTRINO_ICON_INFO, x + BORDER_LEFT, y + height - cFrameFootInfoHeight - fheight + (fheight - ih)/2);
			}
		}
	}
	else if(widgetType == TYPE_FRAME)
	{
		// refresh footInfo Box
		frameBuffer->paintBoxRel(x, y + height - fheight - cFrameFootInfoHeight, width, cFrameFootInfoHeight, COL_MENUCONTENT_PLUS_0);

		// refresh horizontal line buttom
		frameBuffer->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + height - fheight - cFrameFootInfoHeight + 2, COL_MENUCONTENT_PLUS_5);

		if(items.size() > 0)
		{
			CMenuItem* item = items[pos];
		
			// itemName
			if(!item->itemName.empty())
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + BORDER_LEFT, y + height - fheight - cFrameFootInfoHeight + (cFrameFootInfoHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE] ->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight(), width - BORDER_LEFT - BORDER_RIGHT, item->itemName.c_str(), COL_MENUHINT);
			}

			// hint
			if(!item->itemHint.empty())
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(x + BORDER_LEFT, y + height - fheight, width - BORDER_LEFT - BORDER_RIGHT, item->itemHint.c_str(), COL_MENUHINT);
			}
		}
	}
}

void CMenuWidget::hideItemInfo()
{
	dprintf(DEBUG_INFO, "CMenuWidget::hideItemInfo:\n");
	
	if (paintFootInfo)
	{
		if( (widgetType == TYPE_STANDARD) || (widgetType == TYPE_CLASSIC && widgetMode == MODE_LISTBOX) )
		{
			itemsLine.clear(x, y, width, height, cFrameFootInfoHeight);
		}
	}  
}

void CMenuWidget::setFootButtons(const struct button_label *_fbutton_labels, const int _fbutton_count, const int _fbutton_width)
{
	dprintf(DEBUG_INFO, "CMenuWidget::setFootButtons:\n");
	
	initFrames(); // FIXME: HACK to fix FRAME_TYPE_WIDGET
	
	if (_fbutton_count)
	{
		for (unsigned int i = 0; i < (unsigned int)_fbutton_count; i++)
		{
			fbutton_labels.push_back(_fbutton_labels[i]);
		}
	}

	fbutton_count = fbutton_labels.size();	
	fbutton_width = (_fbutton_width == 0)? full_width : _fbutton_width;
}

void CMenuWidget::setHeadButtons(const struct button_label* _hbutton_labels, const int _hbutton_count)
{
	dprintf(DEBUG_INFO, "CMenuWidget::setHeadButtons:\n");
	
	if (_hbutton_count)
	{
		for (unsigned int i = 0; i < (unsigned int)_hbutton_count; i++)
		{
			hbutton_labels.push_back(_hbutton_labels[i]);
		}
	}

	hbutton_count = hbutton_labels.size();
}

void CMenuWidget::addKey(neutrino_msg_t key, CMenuTarget *menue, const std::string & action)
{
	dprintf(DEBUG_INFO, "CMenuWidget::addKey:\n");
	
	keyActionMap[key].menue = menue;
	keyActionMap[key].action = action;
}

void CMenuWidget::saveScreen()
{
	dprintf(DEBUG_INFO, "CMenuWidget::saveScreen:\n");
	
	if(!savescreen)
		return;

	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[full_width*full_height];
	
	if(background)
	{
		frameBuffer->saveScreen(x, y, full_width, full_height, background);
	}
}

void CMenuWidget::restoreScreen()
{
	dprintf(DEBUG_INFO, "CMenuWidget::restoreScreen:\n");
	
	if(background) 
	{
		if(savescreen)
			frameBuffer->restoreScreen(x, y, full_width, full_height, background);
	}
}

void CMenuWidget::enableSaveScreen()
{
	dprintf(DEBUG_INFO, "CMenuWidget::enableSaveScreen:\n");
	
	savescreen = true;
	
	if(!savescreen && background) 
	{
		delete[] background;
		background = NULL;
	}
}

void CMenuWidget::hide()
{
	dprintf(DEBUG_NORMAL, "CMenuWidget::hide: (%s)\n", l_name.c_str());

	if( savescreen && background)
		restoreScreen();
	else
		frameBuffer->paintBackgroundBoxRel(x, y, full_width, full_height);

	hideItemInfo(); 
	
	frameBuffer->blit();

	if(textBox != NULL)
	{
		delete textBox;
		textBox = NULL;
	}
	
	//
	if (timer)
	{
		delete timer;
		timer = NULL;
	}
}

void CMenuWidget::refresh()
{	
	// head
	if (paintDate)
	{
		if (timer && timer->update()) 
		{
			timer->refresh();
		}
	}
}

void CMenuWidget::integratePlugins(CPlugins::i_type_t integration, const unsigned int shortcut, bool enabled)
{
	dprintf(DEBUG_INFO, "CMenuWidget::integratePlugin:\n");
	
	unsigned int number_of_plugins = (unsigned int) g_PluginList->getNumberOfPlugins();

	std::string IconName;
	unsigned int sc = shortcut;

	for (unsigned int count = 0; count < number_of_plugins; count++)
	{
		if ((g_PluginList->getIntegration(count) == integration) && !g_PluginList->isHidden(count))
		{
			//
			IconName = NEUTRINO_ICON_MENUITEM_PLUGIN;

			std::string icon("");
			icon = g_PluginList->getIcon(count);

			if(!icon.empty())
			{
				IconName = PLUGINDIR;
				IconName += "/";
				IconName += g_PluginList->getFileName(count);
				IconName += "/";
				IconName += g_PluginList->getIcon(count);
			}

			//
			neutrino_msg_t dk = (shortcut != RC_nokey) ? CRCInput::convertDigitToKey(sc++) : RC_nokey;

			//FIXME: iconName
			ClistBoxItem *fw_plugin = new ClistBoxItem(_(g_PluginList->getName(count)), enabled, NULL, CPluginsExec::getInstance(), g_PluginList->getFileName(count), dk, NULL, IconName.c_str());

			fw_plugin->setHint(_(g_PluginList->getDescription(count).c_str()));
			//fw_plugin->setWidgetMode(MODE_LISTBOX); //FIXME:
			fw_plugin->isPlugin = true;

			addItem(fw_plugin);
		}
	}
}

//
void CMenuWidget::changeWidgetType()
{
	dprintf(DEBUG_NORMAL, "CMenuWidget::changeWidgetType:\n");

	if(widget.size())
	{
		hide();

		cnt++;

		if(cnt >= (int)widget.size())
		{
			cnt = 0;
		}
			
		widgetType = widget[cnt];

		paint();
	}
}

int CMenuWidget::exec(CMenuTarget* parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CMenuWidget::exec: (%s)\n", l_name.c_str());

	int retval = RETURN_REPAINT;

	int pos = 0;

	if (parent)
		parent->hide();

	//
	paint();
		
	CFrameBuffer::getInstance()->blit();

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout == 0 ? 0xFFFF : timeout);

	//control loop
	do {
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		
		int handled = false;

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

					switch ( rv ) 
					{
						case RETURN_EXIT_ALL:
							retval = RETURN_EXIT_ALL; //fall through
						case RETURN_EXIT:
							msg = RC_timeout;
							break;
						case RETURN_REPAINT:
							//hide();
							paint();
							break;
					}
				}
				else
				{
					handled = true;
					break;
				}

				frameBuffer->blit();
				continue;
			}

			// direkKey
			for (unsigned int i = 0; i < items.size(); i++) 
			{
				CMenuItem * titem = items[i];
			
				if ((titem->directKey != RC_nokey) && (titem->directKey == msg)) 
				{
					if (titem->isSelectable()) 
					{
						items[selected]->paint(false);
						selected = i;

						if (selected > (int)page_start[current_page + 1] || selected < (int)page_start[current_page]) 
						{
							// different page
							paintItems();
						}

						paintItemInfo(selected);
						pos = selected;
						msg = RC_ok;
						actionKey = titem->actionKey;
					} 
					else 
					{
						// swallow-key...
						handled = true;
					}
					break;
				}
			}
		}

		if (!handled) 
		{
			if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
			{
				refresh();
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
					
				case (RC_page_up) :
					if(widgetType == TYPE_FRAME)
					{
						pos = (int) page_start[current_page + 1];
						if(pos >= (int) items.size()) 
							pos = items.size() - 1;

						selected = pos;
						paintItems();
					}
					else if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC || widgetType == TYPE_EXTENDED)
					{
						if(current_page) 
						{
							pos = (int) page_start[current_page] - 1;
							for (unsigned int count = pos; count > 0; count--) 
							{
								CMenuItem * item = items[pos];
								if ( item->isSelectable() ) 
								{
									if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
									{
										// prev item
										items[selected]->paint(false);

										// new item
										paintItemInfo(pos);
										item->paint(true);
										selected = pos;
									} 
									else 
									{
										selected = pos;
										paintItems();
									}
									break;
								}
								
								pos--;
							}
						} 
						else 
						{
							pos = 0;
							for (unsigned int count = 0; count < items.size(); count++) 
							{
								CMenuItem * item = items[pos];
								if (item->isSelectable()) 
								{
									if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
									{
										// prev item
										items[selected]->paint(false);

										// new item
										paintItemInfo(pos);
										item->paint(true);
										selected = pos;
									} 
									else 
									{
										selected = pos;
										paintItems();
									}
									break;
								}
								
								pos++;
							}
						}
					}

					break;

				case (RC_page_down) :
					if(widgetType == TYPE_FRAME)
					{
						pos = (int) page_start[current_page] - 1;
			
						if(pos < 0)
							pos = 0;

						selected = pos;
						paintItems();
					}
					else if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC || widgetType == TYPE_EXTENDED)
					{
						pos = (int) page_start[current_page + 1];

						// check pos
						if(pos >= (int) items.size()) 
							pos = items.size() - 1;

						for (unsigned int count = pos ; count < items.size(); count++) 
						{
							CMenuItem * item = items[pos];
							if (item->isSelectable()) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
								{
									items[selected]->paint(false);

									// paint new item
									paintItemInfo(pos);
									item->paint(true);
									selected = pos;
								} 
								else 
								{
									selected = pos;
									paintItems();
								}
								break;
							}
							pos++;
						}
					}

					break;
					
				case (RC_up) :
					if(widgetType == TYPE_FRAME)
					{
						pos = selected - itemsPerX;

						if(pos < 0)
							pos = selected;

						CMenuItem * item = items[pos];

						if ( item->isSelectable() ) 
						{
							if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
							{ 
								// Item is currently on screen
								//clear prev. selected
								items[selected]->paint(false);
								//select new
								item->paint(true);
								paintItemInfo(pos);
								selected = pos;
							}
						}
					}
					else if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC || widgetType == TYPE_EXTENDED)
					{
						//search next / prev selectable item
						for (unsigned int count = 1; count < items.size(); count++) 
						{
							pos = selected - count;
							if ( pos < 0 )
								pos += items.size();

							CMenuItem * item = items[pos];

							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
								{ 
									// Item is currently on screen
									//clear prev. selected
									items[selected]->paint(false);
									//select new
									paintItemInfo(pos);
									item->paint(true);
									selected = pos;
								} 
								else 
								{
									selected = pos;
									paintItems();
								}
								break;
							}
						}
					}
					break;
					
				case (RC_down) :
					if(widgetType == TYPE_FRAME)
					{
						pos = selected + itemsPerX;

						//FIXME:
						if (pos >= (int)items.size())
							pos -= itemsPerX;

						CMenuItem * item = items[pos];

						if ( item->isSelectable() ) 
						{
							if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
							{ 
								// Item is currently on screen
								//clear prev. selected
								items[selected]->paint(false);
								//select new
								item->paint(true);
								paintItemInfo(pos);
								selected = pos;
							} 
						}
					}
					else if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC || widgetType == TYPE_EXTENDED)
					{
						//search next / prev selectable item
						for (unsigned int count = 1; count < items.size(); count++) 
						{
							pos = (selected + count)%items.size();

							CMenuItem * item = items[pos];

							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
								{ 
									// Item is currently on screen
									//clear prev. selected
									items[selected]->paint(false);
									//select new
									paintItemInfo(pos);
									item->paint(true);
									selected = pos;
								} 
								else 
								{
									selected = pos;
									paintItems();
								}
								break;
							}
						}
					}
					break;

				case (RC_left):
					if(widgetType == TYPE_FRAME)
					{
						for (unsigned int count = 1; count < items.size(); count++) 
						{
							pos = selected - count;
							if ( pos < 0 )
								pos += items.size();

							CMenuItem * item = items[pos];

							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
								{ 
									// Item is currently on screen
									//clear prev. selected
									items[selected]->paint(false);
									//select new
									item->paint(true);
									paintItemInfo(pos);
									selected = pos;
								}
								else 
								{
									selected = pos;
									paintItems();
								}

								break;
							}
						}

					}
					else if (widgetType == TYPE_EXTENDED)
					{
						textBox->scrollPageUp(1);
					}
					else if(widgetType == TYPE_STANDARD)
					{
						if(widgetMode == MODE_SETUP)
						{
							if(hasItem()) 
							{
								if((items[selected]->can_arrow)) 
								{
									//exec this item...
									CMenuItem * item = items[selected];
									item->msg = msg;
							
									int rv = item->exec(this);
									actionKey = item->actionKey;
							
									switch ( rv ) 
									{
										case RETURN_EXIT_ALL:
											retval = RETURN_EXIT_ALL; //fall through
									
										case RETURN_EXIT:
											msg = RC_timeout;
											break;
									
										case RETURN_REPAINT:
											//hide();
											paint();
											break;	
									}
								}
								else
									msg = RC_timeout;
							} 
							else
								msg = RC_timeout;
						}
					}
					
					break;
					
				case (RC_right):
					if(widgetType == TYPE_FRAME)
					{
						for (unsigned int count = 1; count < items.size(); count++) 
						{
							pos = (selected + count)%items.size();

							CMenuItem * item = items[pos];

							if ( item->isSelectable() ) 
							{
								if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
								{ 
									// Item is currently on screen
									//clear prev. selected
									items[selected]->paint(false);
									//select new
									item->paint(true);
									paintItemInfo(pos);
									selected = pos;
								}
								else 
								{
									selected = pos;
									paintItems();
								}
								
								break;
							}
						}
					}
					else if (widgetType == TYPE_EXTENDED)
					{
						textBox->scrollPageDown(1);
					}
					else if(widgetType == TYPE_STANDARD)
					{
						if(widgetMode == MODE_SETUP)
						{
							if(hasItem()) 
							{
								if((items[selected]->can_arrow)) 
								{
									//exec this item...
									CMenuItem * item = items[selected];
									item->msg = msg;
									actionKey = item->actionKey;
							
									int rv = item->exec(this);
							
									switch ( rv ) 
									{
										case RETURN_EXIT_ALL:
											retval = RETURN_EXIT_ALL; //fall through
									
										case RETURN_EXIT:
											msg = RC_timeout;
											break;
									
										case RETURN_REPAINT:
											//hide();
											paint();
											break;	
									}
								}
								else
									msg = RC_timeout;
							} 
							else
								msg = RC_timeout;
						}
					}

					break;

				case (RC_ok):
					{
						if(hasItem()) 
						{
							//exec this item...
							CMenuItem* item = items[selected];
							item->msg = msg;
							actionKey = item->actionKey;
							
							int rv = item->exec(this);
							
							switch ( rv ) 
							{
								case RETURN_EXIT_ALL:
									retval = RETURN_EXIT_ALL;
									
								case RETURN_EXIT:
									msg = RC_timeout;
									break;
									
								case RETURN_REPAINT:
									//hide();
									paint();
									break;
							}
						} 
						else
							msg = RC_timeout;
					}
					break;

				case (RC_home):
					exit_pressed = true;
					dprintf(DEBUG_NORMAL, "CMenuWidget::exec: exit_pressed\n");
					msg = RC_timeout;
					selected = -1; 
					break;
					
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

	dprintf(DEBUG_NORMAL, "CMenuWidget: (%s) retval: (%d) selected:%d\n", l_name.c_str(), retval, selected);
	
	hide();	

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;	

	// vfd
	if(!parent)
	{
		//if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_webtv)
		//	CVFD::getInstance()->setMode(CVFD::MODE_WEBTV);
		//else
			CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	}

	// init items
	for (unsigned int count = 0; count < items.size(); count++) 
	{
		CMenuItem * item = items[count];
		item->init(-1, 0, 0, 0);
	}
	
	return retval;
}	



