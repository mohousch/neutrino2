/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: hintbox.cpp 2013/10/12 mohousch Exp $

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

#include <global.h>
#include <neutrino.h>

#include <system/debug.h>
#include <system/settings.h>

#include <gui/widget/hintbox.h>


CHintBox::CHintBox(const char * Caption, const char * const Text, const int Width, const char * const Icon)
{
	char * begin;
	char * pos;
	int    nw;

	message = strdup(Text);

	cFrameBox.iWidth   = Width;

	cFrameBoxTitle.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	cFrameBoxItem.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	cFrameBox.iHeight = cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight;

	caption = Caption;

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
	entries_per_page = ((cFrameBox.iHeight - cFrameBoxTitle.iHeight) / cFrameBoxItem.iHeight) - 1;
	current_page = 0;

	unsigned int additional_width;

	if (entries_per_page < line.size())
		additional_width = BORDER_LEFT + BORDER_RIGHT + SCROLLBAR_WIDTH;
	else
		additional_width = BORDER_LEFT + BORDER_RIGHT;

	if (Icon != NULL)
	{
		iconfile = Icon;
		additional_width += BORDER_LEFT + BORDER_RIGHT + 2*ICON_OFFSET;
	}
	else
		iconfile = "";

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
	
	// head & body
	m_cBoxWindow = new CWindow(cFrameBox.iX, cFrameBox.iY, cFrameBox.iWidth, cFrameBox.iHeight);
	m_cBoxWindow->enableSaveScreen();
	
	// HG
	paintHG = true;
	count = 0;
	sec_timer_id = 0;
	background = NULL;
	borderMode = BORDER_NO;
}

CHintBox::~CHintBox(void)
{
	dprintf(DEBUG_INFO, "CHintBox::del: (%s)\n", caption.c_str());

	free(message);
}

void CHintBox::paint(void)
{
	dprintf(DEBUG_NORMAL, "CHintBox::paint: (%s)\n", caption.c_str());

	refresh();
	
	CFrameBuffer::getInstance()->blit();
}

void CHintBox::refresh(void)
{
	//body
	m_cBoxWindow->setBorderMode(borderMode);
	m_cBoxWindow->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	m_cBoxWindow->paint();
	
	// title
	cFrameBoxTitle.iX = borderMode? cFrameBox.iX + 2 : cFrameBox.iX;
	cFrameBoxTitle.iY = borderMode? cFrameBox.iY + 2 : cFrameBox.iY;
	cFrameBoxTitle.iWidth = borderMode? cFrameBox.iWidth - 4 : cFrameBox.iWidth;

	headers.setPosition(&cFrameBoxTitle);
	
	headers.setTitle(caption.c_str());
	headers.setIcon(iconfile.c_str());

	headers.paint();

	// body text
	int count = entries_per_page;
	int ypos  = cFrameBoxTitle.iY + cFrameBoxTitle.iHeight + (cFrameBoxItem.iHeight >> 1);

	for (std::vector<char *>::const_iterator it = line.begin() + (entries_per_page * current_page); ((it != line.end()) && (count > 0)); it++, count--)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(cFrameBox.iX + BORDER_LEFT, (ypos += cFrameBoxItem.iHeight), cFrameBox.iWidth, *it, COL_MENUCONTENT, 0, true); 
	}

	// scrollBar #TODO
	if (entries_per_page < line.size())
	{
		ypos = cFrameBox.iY + cFrameBoxTitle.iHeight;

		scrollBar.paint(cFrameBox.iX + cFrameBox.iWidth - SCROLLBAR_WIDTH, ypos, entries_per_page*cFrameBoxItem.iHeight, (line.size() + entries_per_page - 1) / entries_per_page, current_page);
	}	
}

bool CHintBox::has_scrollbar(void)
{
	return (entries_per_page < line.size());
}

void CHintBox::scroll_up(void)
{
	if (current_page > 0)
	{
		current_page--;
		refresh();
	}
}

void CHintBox::scroll_down(void)
{
	if ((entries_per_page * (current_page + 1)) <= line.size())
	{
		current_page++;
		refresh();
	}
}

void CHintBox::hide(void)
{
	dprintf(DEBUG_NORMAL, "CHintBox::hide: (%s)\n", caption.c_str());

	m_cBoxWindow->hide();

	CFrameBuffer::getInstance()->blit();	
}

void CHintBox::paintHourGlass()
{
	dprintf(DEBUG_DEBUG, "\nCHintBox::paintHourGlass:\n");
	
	std::string filename = "hourglass";
	filename += to_string(count);
		
	count = (count + 1) % 9;
	
	int ih = 0;
	int iw = 0;
	
	CFrameBuffer::getInstance()->getIconSize("hourglass0", &iw, &ih);
	
	CFrameBuffer::getInstance()->paintIcon(filename, CFrameBuffer::getInstance()->getScreenX() + 10, CFrameBuffer::getInstance()->getScreenY() + 10);
}

void CHintBox::hideHourGlass()
{
	dprintf(DEBUG_DEBUG, "\nCHintBox::hideHourGlass:\n");
	
	int ih = 0;
	int iw = 0;
	
	CFrameBuffer::getInstance()->getIconSize("hourglass0", &iw, &ih);
	
	if(background) 
	{
		CFrameBuffer::getInstance()->restoreScreen(CFrameBuffer::getInstance()->getScreenX() + 10, CFrameBuffer::getInstance()->getScreenY() + 10, iw, ih, background);
		
		delete[] background;
		background = NULL;
	}
	else //FIXME:
		CFrameBuffer::getInstance()->paintBackgroundBoxRel(CFrameBuffer::getInstance()->getScreenX() + 10, CFrameBuffer::getInstance()->getScreenY() + 10, iw, ih);
}

int CHintBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CHintBox::exec: timeout:%d\n", timeout);
	
	int res = messages_return::none;

	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	
	// HG
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	paint();
	
	if (paintHG)
	{
		int ih = 0;
		int iw = 0;
		
		CFrameBuffer::getInstance()->getIconSize("hourglass0", &iw, &ih);
		
		if(background)
		{
			delete[] background;
			background = NULL;
		}

		background = new fb_pixel_t[iw*ih];
		
		if(background)
		{
			CFrameBuffer::getInstance()->saveScreen(CFrameBuffer::getInstance()->getScreenX() + 10, CFrameBuffer::getInstance()->getScreenY() + 10, iw, ih, background);
		}
		
		paintHourGlass();
	}
		
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR];
		
	dprintf(DEBUG_NORMAL, "CHintBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
	{			
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ((msg == RC_timeout) || (msg == RC_home) || (msg == RC_ok))
		{
			res = messages_return::cancel_info;
		}
		else if ((has_scrollbar()) && ((msg == RC_up) || (msg == RC_down)))
		{
			if (msg == RC_up)
				scroll_up();
			else
				scroll_down();
		}
		else if((msg == RC_mode) || (msg == RC_next) || (msg == RC_prev)) 
		{
			res = messages_return::cancel_info;
			g_RCInput->postMsg(msg, data);
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (paintHG)
				paintHourGlass();
		}
		else
		{
			res = CNeutrinoApp::getInstance()->handleMsg(msg, data);
			
			/*
			if (res & messages_return::unhandled)
			{
				dprintf(DEBUG_NORMAL, "CHintBox::exec: message unhandled\n");

				res = messages_return::cancel_info;
				g_RCInput->postMsg(msg, data);
			}
			*/
		}

		CFrameBuffer::getInstance()->blit();	
	}

	hide();
	
	if (paintHG)
	{
		hideHourGlass();
	}
		
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	return res;
}

int HintBox(const char * const Caption, const char * const Text, const int Width, int timeout, const char * const Icon, const int border)
{
	int res = messages_return::none;

 	CHintBox * hintBox = new CHintBox(Caption, Text, Width, Icon);
	hintBox->setBorderMode(border);
	res = hintBox->exec(timeout);
		
	delete hintBox;
	hintBox = NULL;

	return res;
}



