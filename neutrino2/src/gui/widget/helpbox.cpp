//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: helpbox.cpp 21122024 mohousch Exp $
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

#include <gui/widget/icons.h>

#include <system/debug.h>

#include <gui/widget/helpbox.h>


////
Drawable::Drawable()
{	
}

Drawable::~Drawable()
{
}

int Drawable::getWidth(void)
{
	return m_width;
}

int Drawable::getHeight(void)
{
	return m_height;
}

Drawable::DType Drawable::getType(void)
{
	return Drawable::DTYPE_DRAWABLE;
}

// DText
DText::DText(std::string& text) 
{
	m_text = text;
	m_font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU];
	m_color = COL_MENUCONTENT_TEXT_PLUS_0;
	m_background = false;

	init();
}

DText::DText(const char *text)
{
	m_text = std::string(text);
	m_font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU];
	m_color = COL_MENUCONTENT_TEXT_PLUS_0;
	m_background = false;

	init();
}

void DText::init()
{
	m_width = m_font->getRenderWidth(m_text, true); // UTF-8
	m_height = m_font->getHeight() + 4;	
}

void DText::draw(int x, int y, int width)
{
	m_font->RenderString(x, y + m_font->getHeight() + 2, width, m_text.c_str(), m_color, 0, true, m_background); // UTF-8	
}

Drawable::DType DText::getType(void)
{
	return Drawable::DTYPE_TEXT;
}

// DIcon
DIcon::DIcon(std::string& icon)
{
	m_icon = icon;
	init();
}

DIcon::DIcon(const char *icon)
{
	m_icon = std::string(icon);
	init();
}

void DIcon::init()
{
	CFrameBuffer::getInstance()->getIconSize(m_icon.c_str(), &m_width, &m_height);

	m_height = m_height + 4;
}

void DIcon::draw(int x, int y, int)
{
	CFrameBuffer::getInstance()->paintIcon(m_icon.c_str(), x, y + 2);	
}

Drawable::DType DIcon::getType(void)
{
	return Drawable::DTYPE_ICON;
}

// DSeparator
DSeparator::DSeparator()
{
	m_height = 10;
	m_width = 0;
}

void DSeparator::draw(int x, int y, int width)
{
	int height = 10;

	// line
	CFrameBuffer::getInstance()->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );

	CFrameBuffer::getInstance()->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );	
}

Drawable::DType DSeparator::getType(void)
{
	return Drawable::DTYPE_SEPARATOR;
}

// DPageBreak
DPagebreak::DPagebreak()
{
	m_height = 0;
	m_width = 0;
}

void DPagebreak::draw(int, int, int)
{
}

Drawable::DType DPagebreak::getType(void)
{
	return Drawable::DTYPE_PAGEBREAK;
}

////
CHelpBox::CHelpBox(const char* const Caption, const int Width, const char * const Icon)
{
	dprintf(DEBUG_INFO, "CHelpBox::CMessageBox\n");
	
	widget = NULL;
	headers = NULL;
	
	m_lines.clear();
	
	m_width = Width;
	
	m_iconfile = Icon? Icon : "";
	m_caption = Caption? Caption : "";
		
	//
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
}

CHelpBox::~CHelpBox(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::del:\n");

	//
	for (ContentLines::iterator it = m_lines.begin(); it != m_lines.end(); it++)
	{
		for (std::vector<Drawable*>::iterator it2 = it->begin(); it2 != it->end(); it2++)
		{
			delete *it2;
		}
	}
	
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
	
	int nw = 0;
	m_theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	m_fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	m_iheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	m_height  = HELPBOX_HEIGHT;
	m_maxEntriesPerPage = 0;

	int page = 0;
	int line = 0;
	int maxWidth = HELPBOX_MAX_WIDTH;
	int maxOverallHeight = 0;
	m_startEntryOfPage.push_back(0);
	
	for (ContentLines::iterator it = m_lines.begin(); it!= m_lines.end(); it++)
	{
		bool pagebreak = false;
		int maxHeight = 0;
		int lineWidth = 0;
		int count = 0;
		
		for (std::vector<Drawable*>::iterator item = it->begin(); item != it->end(); item++) 
		{
			//
			if ((*item)->getType() == Drawable::DTYPE_TEXT)
				m_iheight = (*item)->getHeight();
			
			//
			if ((*item)->getHeight() > maxHeight)
				maxHeight += (*item)->getHeight();

			lineWidth += (*item)->getWidth();

			if ((*item)->getType() == Drawable::DTYPE_PAGEBREAK)
				pagebreak = true;
			
			count++;
		}
		
		// 10 pixels left and right of every item. determined empirically :-(
		lineWidth += count * (BORDER_LEFT + BORDER_RIGHT);
		
		if (lineWidth > m_width)
			maxWidth = lineWidth;
		else if (lineWidth > (HELPBOX_MAX_WIDTH - 20))
			maxWidth = HELPBOX_MAX_WIDTH - 20;
		else if (lineWidth < m_width)
			maxWidth = m_width;

//		m_height += maxHeight;
//		m_height += m_iheight;
		
		if (maxHeight > HELPBOX_MAX_HEIGHT || pagebreak) 
		{
			if (maxHeight > maxOverallHeight)
				maxOverallHeight = maxHeight;
//			if (m_height - m_iheight > maxOverallHeight)
//				maxOverallHeight = m_height - m_iheight;
			
//			m_theight + m_iheight + m_fheight;
//			m_height = HELPBOX_HEIGHT; 
			
			if (pagebreak)
				m_startEntryOfPage.push_back(line + 1);
			else 
				m_startEntryOfPage.push_back(line);
			
			page++;
			
			if (m_maxEntriesPerPage < (m_startEntryOfPage[page] - m_startEntryOfPage[page -1]))
			{
				m_maxEntriesPerPage = m_startEntryOfPage[page] - m_startEntryOfPage[page -1];
			}
		}
		
		line++;
	}

	m_width = maxWidth;
	
	// if there is only one page m_height is already correct 
	//but m_maxEntries has not been set
	if (m_startEntryOfPage.size() > 1)
	{
		m_height = maxOverallHeight;
		m_width += SCROLLBAR_WIDTH; // scroll bar
	} 
	else 
	{
		m_maxEntriesPerPage = line;
	}

	m_startEntryOfPage.push_back(line + 1); // needed to calculate amount of items on last page

	m_currentPage = 0;
	m_pages = page + 1;
	unsigned int additional_width;

	if (m_startEntryOfPage.size() > 1)
		additional_width = BORDER_LEFT + BORDER_RIGHT + SCROLLBAR_WIDTH;
	else
		additional_width = BORDER_LEFT + BORDER_RIGHT;

	if (!m_iconfile.empty())
	{
		int iw, ih;
		CFrameBuffer::getInstance()->getIconSize(m_iconfile.c_str(), &iw, &ih);
		additional_width += BORDER_LEFT + iw + ICON_OFFSET; 
	}
	else
		m_iconfile = "";

	nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(m_caption); // UTF-8

	if (nw > m_width)
		m_width = nw;
}

void CHelpBox::initFrames(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::initFrames\n");
	
	cFrameBox.iWidth = m_width;
	cFrameBox.iHeight = m_height;
	cFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - cFrameBox.iWidth) >> 1);
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
	widget->setPosition(&cFrameBox);
//	widget->setBorderMode(borderMode);
//	widget->setBorderColor(borderColor);
	widget->enableSaveScreen();

	// title
	if (headers)
	{
		headers->setPosition(borderMode? cFrameBox.iX + 2 : cFrameBox.iX, borderMode? cFrameBox.iY + 2 : cFrameBox.iY, borderMode? m_width - 4 : m_width, m_theight);
	
		headers->setTitle(m_caption.c_str(), m_iconfile.c_str());
	}

	refreshPage();
}

void CHelpBox::refreshPage()
{
	dprintf(DEBUG_INFO, "CHelpBox::refreshPage\n");
	
	//
	widget->paint();

	//TextBody
	int yPos  = cFrameBox.iY + m_theight;

	for (ContentLines::iterator it = m_lines.begin() + m_startEntryOfPage[m_currentPage]; it != m_lines.begin() + m_startEntryOfPage[m_currentPage + 1] && it != m_lines.end(); it++)
	{
		int xPos = cFrameBox.iX + BORDER_LEFT;
		int maxHeight = 0;
		
		for (std::vector<Drawable*>::iterator d = it->begin(); d != it->end(); d++)
		{
  			(*d)->draw(xPos, yPos, m_width - BORDER_LEFT - BORDER_RIGHT);

			xPos += (*d)->getWidth() + BORDER_LEFT + BORDER_RIGHT;
			
			if ((*d)->getHeight() > maxHeight)
				maxHeight = (*d)->getHeight();
		}
		
		yPos += maxHeight;
	}

	// paint scrollbar #TODO
	if (has_scrollbar()) 
	{
		yPos = cFrameBox.iY + m_theight;

		scrollBar.paint(cFrameBox.iX + m_width - 1 - SCROLLBAR_WIDTH, yPos, m_height - m_theight - m_fheight, m_pages, m_currentPage);
	}
}

bool CHelpBox::has_scrollbar(void)
{
	return (m_startEntryOfPage.size() > 2);
}

void CHelpBox::scroll_up(void)
{
	if (m_currentPage > 0)
	{
		m_currentPage--;
		refreshPage();
	}
}

void CHelpBox::scroll_down(void)
{
	if (m_currentPage + 1 < m_startEntryOfPage.size() - 1)
	{
		m_currentPage++;
		refreshPage();
	}
}

void CHelpBox::hide(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::hide:\n");

	widget->hide();
}

void CHelpBox::addLine(const char* const text)
{
	std::vector<Drawable*> v;
	Drawable *d = new DText(text);
	v.push_back(d);
	m_lines.push_back(v);
}

void CHelpBox::addLine(const char* const icon, const char* const text)
{
	std::vector<Drawable*> v;
	Drawable *di = new DIcon(icon);
	Drawable *dt = new DText(text);
	v.push_back(di);
	v.push_back(dt);
	m_lines.push_back(v);
}

void CHelpBox::addSeparator(void)
{
	std::vector<Drawable*> v;
	Drawable *p = new DSeparator();
	v.push_back(p);
	m_lines.push_back(v);
}

void CHelpBox::addPagebreak(void)
{
	std::vector<Drawable*> v;
	Drawable *p = new DPagebreak();
	v.push_back(p);
	m_lines.push_back(v);
}

int CHelpBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CHelpBox::exec: timeout:%d\n", timeout);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	
	int result = messages_return::none;
	
	//
	init();
	initFrames();

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

