//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: hintbox.cpp 21122024 mohousch Exp $
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

#include <system/debug.h>
#include <system/settings.h>

#include <gui/widget/hintbox.h>


CHintBox::CHintBox(const char * Caption, const char * const Text, const int Width, const char * const Icon)
{
	//
	widget = NULL;
	headers = NULL;
	line.clear();

	caption = Caption;
	message = strdup(Text);

	// initFrames
	cFrameBox.iWidth = Width;

	cFrameBoxTitle.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	cFrameBoxItem.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();			// min 1 line
	cFrameBox.iHeight = cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight + cFrameBoxTitle.iHeight;	// 

	//
	char * begin;
	char * pos;
	int    nw = cFrameBox.iWidth;
	unsigned int additional_width = 0;
	
	begin = message;

	// recalculate height
	while (true)
	{
		cFrameBox.iHeight += cFrameBoxItem.iHeight;
		
		if (cFrameBox.iHeight > HINTBOX_MAX_HEIGHT)
			cFrameBox.iHeight -= cFrameBoxItem.iHeight;

		line.push_back(begin);
		
		pos = strchr(begin, '\n');
		
		if (pos != NULL)
		{
			*pos = 0;
			begin = pos + 1;
		}
		else
			break;
	}
	
	entries_per_page = ((cFrameBox.iHeight - 2*cFrameBoxTitle.iHeight ) / cFrameBoxItem.iHeight);
	current_page = 0;
	pages = (line.size() + entries_per_page - 1) / entries_per_page;

	if (entries_per_page < line.size())
		additional_width = BORDER_LEFT + BORDER_RIGHT + SCROLLBAR_WIDTH;
	else
		additional_width = BORDER_LEFT + BORDER_RIGHT;
	
	if (Icon != NULL)
	{
		iconfile = Icon;
		
		int iw, ih;
		CFrameBuffer::getInstance()->getIconSize(iconfile.c_str(), &iw, &ih);
		additional_width += iw + ICON_OFFSET; 
	}
	else
		iconfile = "";

	// check if title is bigger than our box
	nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(caption); // UTF-8

	if (nw > cFrameBox.iWidth)
		cFrameBox.iWidth = nw;

	for (std::vector<char *>::const_iterator it = line.begin(); it != line.end(); it++)
	{
		nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(*it, true); // UTF-8
		
		if (nw > cFrameBox.iWidth)
		{
			cFrameBox.iWidth = nw;

			if(cFrameBox.iWidth > HINTBOX_MAX_WIDTH)
				cFrameBox.iWidth = HINTBOX_MAX_WIDTH;		
		}
	}

	// Box
	cFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - cFrameBox.iWidth ) >> 1);
	cFrameBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - cFrameBox.iHeight) >> 2);
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("hintbox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		//
		widget = new CWidget(cFrameBox.iX, cFrameBox.iY, cFrameBox.iWidth, cFrameBox.iHeight);
		widget->name = "hintbox";
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
		
		//
		headers = new CCHeaders();
		
		widget->addCCItem(headers);
	}
	
	widget->paintMainFrame(true);
	
	// HG
	paintHG = true;
	sec_timer_id = 0;
	sec_timer_interval = 1;
	spinner = NULL;
	
	//
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
}

CHintBox::~CHintBox(void)
{
	dprintf(DEBUG_INFO, "CHintBox::del: (%s)\n", caption.c_str());

	free(message);
	line.clear();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CHintBox::paint(void)
{
	dprintf(DEBUG_NORMAL, "CHintBox::paint: (%s)\n", caption.c_str());
	
	// title
	cFrameBoxTitle.iX = borderMode? cFrameBox.iX + 2 : cFrameBox.iX;
	cFrameBoxTitle.iY = borderMode? cFrameBox.iY + 2 : cFrameBox.iY;
	cFrameBoxTitle.iWidth = borderMode? cFrameBox.iWidth - 4 : cFrameBox.iWidth;

	if (headers)
	{
		headers->setPosition(&cFrameBoxTitle);
		headers->setTitle(caption.c_str(), iconfile.c_str());
	}
	
	widget->setPosition(&cFrameBox);
	widget->setBorderMode(borderMode);
	widget->setBorderColor(borderColor);
	widget->enableSaveScreen();

	//
	refreshPage();
	
	CFrameBuffer::getInstance()->blit();
}

void CHintBox::refreshPage(void)
{
	//
	widget->paint();
	
	// body text
	int count = entries_per_page;
	int ypos  = cFrameBoxTitle.iY + cFrameBoxTitle.iHeight + (cFrameBoxItem.iHeight >> 1);

	for (std::vector<char *>::const_iterator it = line.begin() + (entries_per_page * current_page); ((it != line.end()) && (count > 0)); it++, count--)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(cFrameBox.iX + BORDER_LEFT, (ypos += cFrameBoxItem.iHeight), cFrameBox.iWidth - 20, *it, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); 
	}

	// scrollBar #TODO
	if (entries_per_page < line.size())
	{
		int ypos = cFrameBox.iY + cFrameBoxTitle.iHeight;

		scrollBar.paint(cFrameBox.iX + cFrameBox.iWidth - SCROLLBAR_WIDTH, ypos, entries_per_page*cFrameBoxItem.iHeight, (line.size() + entries_per_page - 1) / entries_per_page, current_page);
	}	
}

bool CHintBox::has_scrollbar(void)
{
	return (entries_per_page < line.size());
}

void CHintBox::scrollPageUp(void)
{
	if (current_page > 0)
	{
		current_page--;
		refreshPage();
	}
}

void CHintBox::scrollPageDown(void)
{
	if ((entries_per_page * (current_page + 1)) <= line.size())
	{
		current_page++;
		refreshPage();
	}
}

void CHintBox::hide(void)
{
	dprintf(DEBUG_NORMAL, "CHintBox::hide: (%s)\n", caption.c_str());

	widget->hide();
}

int CHintBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CHintBox::exec: timeout:%d\n", timeout);
	
	int res = messages_return::none;

	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	
	// HG
	sec_timer_id = g_RCInput->addTimer(sec_timer_interval*1000*1000, false);

	paint();
	
	if (paintHG)
	{
		spinner = new CCSpinner(CFrameBuffer::getInstance()->getScreenX() + 10, CFrameBuffer::getInstance()->getScreenY() + 10, 30, 30);
		spinner->paint();
		
	}
		
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing_numericzap;
		
	dprintf(DEBUG_NORMAL, "CHintBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
	{			
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ((msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_ok))
		{
			res = messages_return::cancel_info;
		}
		else if (has_scrollbar() && ((msg == CRCInput::RC_up) || (msg == CRCInput::RC_down) || (msg == CRCInput::RC_page_up) || (msg == CRCInput::RC_page_down)))
		{
			if ( (msg == CRCInput::RC_up) || (msg == CRCInput::RC_page_up))
				scrollPageUp();
			else
				scrollPageDown();
		}
		else if((msg == CRCInput::RC_mode) || (msg == CRCInput::RC_next) || (msg == CRCInput::RC_prev)) 
		{
			res = messages_return::cancel_info;
			g_RCInput->postMsg(msg, data);
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (paintHG)
				spinner->refresh();
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg(msg, data);
		}

		CFrameBuffer::getInstance()->blit();	
	}

	hide();
	
	if (paintHG)
	{
		spinner->hide();
		delete spinner;
		spinner = NULL;
	}
	
	if (sec_timer_id)
	{	
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}

	return res;
}

////
int HintBox(const char * const Caption, const char * const Text, const int Width, int timeout, const char * const Icon, const int border)
{
	int res = messages_return::none;

 	CHintBox * hintBox = new CHintBox(Caption, Text, Width, Icon);
//	hintBox->setBorderMode(border);

	res = hintBox->exec(timeout);
		
	delete hintBox;
	hintBox = NULL;

	return res;
}



