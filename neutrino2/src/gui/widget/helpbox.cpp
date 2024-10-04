/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: helpbox.cpp 03112024 mohousch Exp $

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

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>

#include <system/debug.h>

#include <gui/widget/helpbox.h>


CHelpBox::CHelpBox(const char* const Caption, const int Width, const char * const Icon)
{
	dprintf(DEBUG_INFO, "Helpbox::\n");
	
	m_width = Width;
	m_theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6; 	// title
	m_fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6; 	// foot
	m_iheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();		// item
	m_height = HELPBOX_HEIGHT;

	m_caption = Caption? Caption : "";
	m_iconfile = Icon? Icon : "";
	
	widget = NULL;
	headers = NULL;
	m_lines.clear();
	
	pages = 1;
	
	//
	init();
		
	//
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
		
	// initFrames
	initFrames();
}

CHelpBox::~CHelpBox()
{
	dprintf(DEBUG_INFO, "~Helpbox::\n");

	m_lines.clear();
	
	hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CHelpBox::init()
{
	dprintf(DEBUG_INFO, "CHelpBox::init\n");
			
	//		
	pages = 1;
	entries_per_page = ((m_height - m_theight - m_fheight ) / m_iheight);
	current_page = 0;
	pages = (m_lines.size() + entries_per_page - 1) / entries_per_page;
	
	//
	int nw = m_width;
	unsigned int additional_width = 0;

	if (entries_per_page < m_lines.size())
		additional_width = BORDER_LEFT + BORDER_RIGHT + SCROLLBAR_WIDTH;
	else
		additional_width = BORDER_LEFT + BORDER_RIGHT;

	if (!m_iconfile.empty())
	{
		int iw, ih;
		CFrameBuffer::getInstance()->getIconSize(m_iconfile.c_str(), &iw, &ih);
		additional_width += iw + ICON_OFFSET; 
	}
	else
		m_iconfile = "";

	nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(m_caption); // UTF-8

	if (nw > m_width)
		m_width = nw;
		
	//
	for (std::vector<char *>::const_iterator it = m_lines.begin(); it != m_lines.end(); it++)
	{
		nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(*it, true); // UTF-8
		
		if (nw > m_width)
		{
			m_width = nw;

			if(m_width > HINTBOX_MAX_WIDTH)
				m_width = HINTBOX_MAX_WIDTH;		
		}
	}
}

void CHelpBox::initFrames(void)
{
	dprintf(DEBUG_INFO, "CMessageBox::initFrames\n");
	
	cFrameBox.iWidth = m_width;
	cFrameBox.iHeight = m_height;
	cFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - cFrameBox.iWidth ) >> 1);
	cFrameBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - cFrameBox.iHeight) >> 2);
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("messagebox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		widget->name = "messagebox";
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
		
		headers = new CCHeaders();
		
		widget->addCCItem(headers);
	}

	widget->paintMainFrame(true);
}

void CHelpBox::paint(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::paint\n");
	
	//
	init();
	initFrames();
	
	// 
	widget->setPosition(&cFrameBox);
	widget->setBorderMode(borderMode);
	widget->setBorderColor(borderColor);
	widget->enableSaveScreen();

	// title
	if (headers)
	{
		headers->setPosition(borderMode? CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1) + 2 : CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1), borderMode? CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + 2 : CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2), borderMode? m_width - 4 : m_width, m_theight);
	
		headers->setTitle(m_caption.c_str());
		headers->setIcon(m_iconfile.c_str());
	}

	refreshPage();
}

void CHelpBox::refreshPage()
{
	dprintf(DEBUG_INFO, "CHelpBox::refreshPage\n");
	
	//
	widget->paint();

	//
	int count = entries_per_page;
	int ypos  = cFrameBox.iY + m_theight;
	
	for (std::vector<char *>::const_iterator it = m_lines.begin() + (entries_per_page * current_page); ((it != m_lines.end()) && (count > 0)); it++, count--)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(cFrameBox.iX + BORDER_LEFT, (ypos += g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight()), m_width - 20, *it, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); 
	}

	// scrollBar #TODO
	if (entries_per_page < m_lines.size())
	{
		int ypos  = cFrameBox.iY + m_theight;
		scrollBar.paint(cFrameBox.iX + cFrameBox.iWidth - SCROLLBAR_WIDTH, ypos, m_height - m_theight - m_fheight, pages, current_page);
	}
}

bool CHelpBox::has_scrollbar(void)
{
	return (entries_per_page < m_lines.size());
}

void CHelpBox::scroll_up(void)
{
	if (current_page > 0)
	{
		current_page--;
		refreshPage();
	}
}

void CHelpBox::scroll_down(void)
{
	if ((entries_per_page * (current_page + 1)) <= m_lines.size())
	{
		current_page++;
		refreshPage();
	}
}

void CHelpBox::hide(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::hide:\n");

	widget->hide();
}


void CHelpBox::addLine(const char *text)
{
	char *begin = strdup(text);
	m_lines.push_back(begin);
}

void CHelpBox::addLine(const char *icon, const char *text)
{
	char *begin = strdup(text);
	m_lines.push_back(begin);
}

void CHelpBox::addSeparator(void)
{
}

void CHelpBox::addPagebreak(void)
{
}

int CHelpBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CHelpBox::exec: timeout:%d\n", timeout);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	
	int result = messages_return::none;

	// paint
	paint();
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg;
		
	dprintf(DEBUG_NORMAL, "CHelpBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (((msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home)))
		{
			loop   = false;
		}
		else if (has_scrollbar() && ((msg == CRCInput::RC_up) || (msg == CRCInput::RC_down) || (msg == CRCInput::RC_page_up) || (msg == CRCInput::RC_page_down)))
		{
			if ( (msg == CRCInput::RC_up) || (msg == CRCInput::RC_page_up))
				scroll_up();
			else
				scroll_down();
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			loop = false;
		}

		CFrameBuffer::getInstance()->blit();
	}

	// hide
	hide();
	
	return result;
}

