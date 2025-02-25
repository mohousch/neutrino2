//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: listframe.cpp 21122024 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Author: Günther@tuxbox.berlios.org
//		based on code of Steffen Hehn 'McClean'
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

#include <stdlib.h>

#include <global.h>

#include <gui/widget/icons.h>
#include <gui/widget/listframe.h>

#include <system/debug.h>
#include <system/settings.h>


////
#define TEXT_BORDER_WIDTH			8
#define ROW_BORDER_WIDTH             		4
#define SCROLL_FRAME_WIDTH			SCROLLBAR_WIDTH
#define SCROLL_MARKER_BORDER			2	

#define TITLE_BACKGROUND_COLOR 			COL_MENUHEAD_PLUS_0
#define LIST_BACKGROUND_COLOR 			COL_MENUCONTENT_PLUS_0
#define LIST_BACKGROUND_COLOR_SELECTED 		COL_MENUCONTENTSELECTED_PLUS_0

#define TITLE_FONT_COLOR 			COL_MENUHEAD_TEXT_PLUS_0
#define HEADER_LIST_FONT_COLOR 			COL_MENUCONTENT_TEXT_PLUS_0
#define LIST_FONT_COLOR 			COL_MENUCONTENT_TEXT_PLUS_0
#define LIST_FONT_COLOR_SELECTED 		COL_MENUCONTENTSELECTED_TEXT_PLUS_0

#define FONT_LIST 				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]
#define FONT_HEADER_LIST 			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]
#define FONT_TITLE 				g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE];

//// globals
CFont* CListFrame::m_pcFontTitle = NULL;
CFont* CListFrame::m_pcFontList = NULL;
CFont* CListFrame::m_pcFontHeaderList = NULL;

////
CListFrame::CListFrame(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CListFrame::CListFrame\r\n");
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	m_nMaxHeight = itemBox.iHeight;
	m_nMaxWidth = itemBox.iWidth;
	
	initVar();
	initFrames();
	
	m_pLines = NULL;
}

CListFrame::CListFrame(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CListFrame::CListFrame\r\n");
	
	frameBuffer = CFrameBuffer::getInstance();
	
	itemBox	= *position;
	oldPosition = itemBox;

	m_nMaxHeight = itemBox.iHeight;
	m_nMaxWidth = itemBox.iWidth;
	
	initVar();
	initFrames();
	
	m_pLines = NULL;
}

CListFrame::~CListFrame()
{
	dprintf(DEBUG_DEBUG, "CListFrame::~CListFrame\r\n");
	hide();
}

void CListFrame::initVar(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::InitVar\r\n");
	
	m_nMode = 0;
	
	m_showSelection = true;

	m_pcFontList = FONT_LIST;
	m_nFontListHeight = m_pcFontList->getHeight();
	
	m_pcFontHeaderList = FONT_HEADER_LIST ;
	m_nFontHeaderListHeight = m_pcFontHeaderList->getHeight();
	
	m_pcFontTitle = FONT_TITLE;
	m_textTitle = "";
	m_nFontTitleHeight = m_pcFontTitle->getHeight();
	
	m_nNrOfPages = 1;
	m_nNrOfLines = 0;
	m_nNrOfRows = 1;
	m_nLinesPerPage = 0;
	m_nCurrentLine = 0;
	m_nCurrentPage = 0;
	m_nSelectedLine = 0;

	if (itemBox.iWidth == 0 || itemBox.iHeight == 0)
	{
		itemBox.iWidth	= MENU_WIDTH;
		itemBox.iHeight = MENU_HEIGHT;
		itemBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - itemBox.iWidth) >>1);
		itemBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - itemBox.iHeight) >>1);
	}

	m_nMaxHeight = MAX_WINDOW_HEIGHT;
	m_nMaxWidth = MAX_WINDOW_WIDTH;
	
	//
	iconOffset = 0;
	
	//
	paintframe = true;
	inFocus = true;
	
	cc_type = CC_LISTFRAME;
}

void CListFrame::initFrames(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::InitFrames\r\n");
	
	// sanity check
	if(itemBox.iHeight > ((int)CFrameBuffer::getInstance()->getScreenHeight(true)))
		itemBox.iHeight = CFrameBuffer::getInstance()->getScreenHeight(true) - 4;  // 4 pixels for border

	// sanity check
	if(itemBox.iWidth > (int)CFrameBuffer::getInstance()->getScreenWidth(true))
		itemBox.iWidth = CFrameBuffer::getInstance()->getScreenWidth(true) - 4; // 4 pixels for border

	if(m_nMode & TITLE)
	{
		m_cFrameTitleRel.iX = 0;
		m_cFrameTitleRel.iY = 0;
		m_cFrameTitleRel.iHeight = m_nFontTitleHeight + 2 ;
		m_cFrameTitleRel.iWidth = itemBox.iWidth ;
	}
	else
	{
		m_cFrameTitleRel.iX = 0;
		m_cFrameTitleRel.iY = 0;
		m_cFrameTitleRel.iHeight = 0;
		m_cFrameTitleRel.iWidth	= 0;
	}

	if(m_nMode & HEADER_LINE)
	{
		m_cFrameHeaderListRel.iX = 0;
		m_cFrameHeaderListRel.iY = 0 + m_cFrameTitleRel.iHeight;
		m_cFrameHeaderListRel.iHeight = m_nFontHeaderListHeight ;
	}
	else
	{
		m_cFrameHeaderListRel.iX = 0;
		m_cFrameHeaderListRel.iY = 0;
		m_cFrameHeaderListRel.iHeight = 0;
		m_cFrameHeaderListRel.iWidth = 0;
	}

	m_cFrameListRel.iX = 0;
	m_cFrameListRel.iY = m_cFrameHeaderListRel.iHeight + m_cFrameTitleRel.iHeight;
	m_cFrameListRel.iHeight = itemBox.iHeight - m_cFrameHeaderListRel.iHeight - m_cFrameTitleRel.iHeight;

	m_cFrameScrollRel.iX = itemBox.iWidth - SCROLL_FRAME_WIDTH;
	m_cFrameScrollRel.iY = m_cFrameTitleRel.iHeight;
	m_cFrameScrollRel.iWidth = SCROLL_FRAME_WIDTH;
	m_cFrameScrollRel.iHeight = m_cFrameListRel.iHeight + m_cFrameHeaderListRel.iHeight;

	m_cFrameListRel.iWidth	= itemBox.iWidth;
		
	if(m_nMode & HEADER_LINE)
	{
		m_cFrameHeaderListRel.iWidth = itemBox.iWidth;
	}

	m_nLinesPerPage = (m_cFrameListRel.iHeight) / m_nFontListHeight;
	
	LinesPerPage = m_nLinesPerPage;
}

void CListFrame::onNewLineArray(void)
{      
	dprintf(DEBUG_DEBUG, "CListFrame::onNewLineArray \r\n");

	m_nNrOfLines = m_pLines->lineArray[0].size();
	
	if(m_nNrOfLines > 0 )
	{
		m_nLinesPerPage = (m_cFrameListRel.iHeight) / m_nFontListHeight;

		if(m_nLinesPerPage <= 0)
			m_nLinesPerPage = 1;

		m_nNrOfPages =	((m_nNrOfLines-1) / m_nLinesPerPage) + 1;

		if(m_nCurrentLine >= m_nNrOfLines)
		{
			m_nCurrentPage = m_nNrOfPages - 1;
			m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		}
		
		if(m_nSelectedLine >= m_nNrOfLines)
		{
			m_nSelectedLine = m_nCurrentLine;
		}
	}
	else
	{
		m_nNrOfLines = 0;
		m_nCurrentLine = 0;
		m_nSelectedLine = 0;
		m_nLinesPerPage = 1;
		m_nNrOfPages = 0;
	}
}

void CListFrame::refreshTitle(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::refreshTitle:\r\n");
	
	frameBuffer->paintBoxRel(m_cFrameTitleRel.iX + itemBox.iX, m_cFrameTitleRel.iY + itemBox.iY, m_cFrameTitleRel.iWidth, m_cFrameTitleRel.iHeight, TITLE_BACKGROUND_COLOR);
	
	int iw = 0;
	int ih = 0;
	
	if(!m_iconTitle.empty())
	{
		frameBuffer->getIconSize(m_iconTitle.c_str(), &iw, &ih);
		frameBuffer->paintIcon(m_iconTitle, m_cFrameTitleRel.iX + itemBox.iX + BORDER_LEFT, m_cFrameTitleRel.iY + itemBox.iY + (m_cFrameTitleRel.iHeight - ih)/2);
	}
	
	m_pcFontTitle->RenderString(m_cFrameTitleRel.iX + itemBox.iX + BORDER_LEFT + iw + ICON_OFFSET, m_cFrameTitleRel.iY + m_cFrameTitleRel.iHeight + itemBox.iY, m_cFrameTitleRel.iWidth - iw - (TEXT_BORDER_WIDTH << 1), m_textTitle.c_str(), TITLE_FONT_COLOR, 0, true); // UTF-8
}

void CListFrame::refreshScroll(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::refreshScroll\r\n");

	if (m_nNrOfPages > 1) 
	{
		scrollBar.paint(m_cFrameScrollRel.iX + itemBox.iX, m_cFrameScrollRel.iY + itemBox.iY, m_cFrameScrollRel.iHeight, m_nNrOfPages, m_nCurrentPage);
	}
}

void CListFrame::refreshList(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::refreshList: %d\r\n", m_nCurrentLine);
	
	// mainBox
	frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, m_cFrameListRel.iY + itemBox.iY, m_cFrameListRel.iWidth, m_cFrameListRel.iHeight, LIST_BACKGROUND_COLOR);

	if(m_nNrOfLines <= 0) 
		return;

	// lines
	int y = m_cFrameListRel.iY;
	for(int line = m_nCurrentLine; line < m_nNrOfLines && line < m_nCurrentLine + m_nLinesPerPage; line++)
	{
		uint32_t color = LIST_FONT_COLOR;
		
		// draw line / icon
		if(line == m_nSelectedLine && m_showSelection == true)
		{
			color = LIST_FONT_COLOR_SELECTED;

			if(m_nNrOfPages > 1)
				frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth - m_cFrameScrollRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR_SELECTED, true);
			else
				frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR_SELECTED, true);
		}
		
		int width;
		int x = m_cFrameListRel.iX + TEXT_BORDER_WIDTH;
		
		// icon
		if (!m_pLines->icon.empty())
		{
			//get icon size
			int iw, ih;
			frameBuffer->getIconSize(m_pLines->icon.c_str(), &iw, &ih);
				
			if (ih >= m_nFontListHeight)
			{
				ih = m_nFontListHeight - 4;
				iw = 1.63*ih;
			}
			
			iconOffset = iw + ICON_OFFSET;
			
			//
			frameBuffer->paintIcon(m_pLines->icon, m_cFrameListRel.iX + itemBox.iX + ICON_OFFSET, y + itemBox.iY, m_nFontListHeight, iw, ih);
		}
		
		// lines
		y += m_nFontListHeight;
		
		for(int row = 0; row < m_pLines->rows; row++)
		{
			width = m_pLines->rowWidth[row] ;
			
			if(width > m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - TEXT_BORDER_WIDTH)
			{
				width = m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - TEXT_BORDER_WIDTH;
				dprintf(DEBUG_DEBUG, "   normalize width to %d , x:%d \r\n",width,x);
			}
			
			m_pcFontList->RenderString(x + itemBox.iX + iconOffset, y + itemBox.iY, width, m_pLines->lineArray[row][line].c_str(), color, 0, true); // UTF-8
			x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
		}
	}	
}

void CListFrame::refreshLine(int line)
{
	if( m_nNrOfLines <= 0) 
		return;

	if((line < m_nCurrentLine) && (line > m_nCurrentLine + m_nLinesPerPage))
		return;

	uint32_t color;
	int rel_line = line - m_nCurrentLine;
	int y = m_cFrameListRel.iY + (rel_line*m_nFontListHeight);

	// itemBox
	if(line == m_nSelectedLine && m_showSelection == true)
	{
		color = LIST_FONT_COLOR_SELECTED;

		if(m_nNrOfPages > 1)
			frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth - m_cFrameScrollRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR_SELECTED);
		else
			frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR_SELECTED);
	}
	else
	{
		color = LIST_FONT_COLOR;
		
		if(m_nNrOfPages > 1)
			frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth - m_cFrameScrollRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR);
		else
			frameBuffer->paintBoxRel(m_cFrameListRel.iX + itemBox.iX, y + itemBox.iY, m_cFrameListRel.iWidth, m_nFontListHeight, LIST_BACKGROUND_COLOR);
	}
	
	int width;
	int x = m_cFrameListRel.iX + TEXT_BORDER_WIDTH;
	
	// icon
	if (!m_pLines->icon.empty())
	{
		//get icon size
		int iw, ih;
		frameBuffer->getIconSize(m_pLines->icon.c_str(), &iw, &ih);
				
		if (ih >= m_nFontListHeight)
		{
			ih = m_nFontListHeight - 4;
			iw = 1.63*ih;
		}
			
		iconOffset = iw + 2*ICON_OFFSET;
			
		//
		frameBuffer->paintIcon(m_pLines->icon, m_cFrameListRel.iX + itemBox.iX + ICON_OFFSET, y + itemBox.iY, m_nFontListHeight, iw, ih);
	}
	
	// lines
	y += m_nFontListHeight;
	
	for(int row = 0; row < m_pLines->rows; row++)
	{
		width = m_pLines->rowWidth[row] ;
		if(width > m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - 2*TEXT_BORDER_WIDTH)
		{
			width = m_cFrameListRel.iWidth - x + m_cFrameListRel.iX - 2*TEXT_BORDER_WIDTH;
			dprintf(DEBUG_DEBUG, "   normalize to %d,x:%d\r\n",width,x);
		}

		m_pcFontList->RenderString(x + itemBox.iX + iconOffset, y + itemBox.iY, width, m_pLines->lineArray[row][line].c_str(), color, 0, true); // UTF-8
		x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
	}
}

void CListFrame::refreshHeaderList(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::refreshHeaderList\r\n");
	
	if(!(m_nMode & HEADER_LINE))
		return;

	frameBuffer->paintBoxRel(m_cFrameHeaderListRel.iX + itemBox.iX, m_cFrameHeaderListRel.iY + itemBox.iY, m_cFrameHeaderListRel.iWidth, m_cFrameHeaderListRel.iHeight, LIST_BACKGROUND_COLOR);

	int width;
	int x = m_cFrameHeaderListRel.iX + TEXT_BORDER_WIDTH;
	int y = m_cFrameHeaderListRel.iY + m_nFontHeaderListHeight + 2;
	bool loop = true;
	
	// calculate icon offset
	if (!m_pLines->icon.empty())
	{
		//get icon size
		int iw, ih;
		frameBuffer->getIconSize(m_pLines->icon.c_str(), &iw, &ih);
				
		if (ih >= m_nFontListHeight)
		{
			ih = m_nFontListHeight - 4;
			iw = 1.63*ih;
		}
			
		iconOffset = iw + 2*ICON_OFFSET;
	}
	
	for(int row = 0; row < m_pLines->rows && loop == true; row++)
	{
		width = m_pLines->rowWidth[row];
		
		if(width > m_cFrameHeaderListRel.iWidth - x + m_cFrameHeaderListRel.iX - 2*TEXT_BORDER_WIDTH)
		{
			width = m_cFrameHeaderListRel.iWidth - x + m_cFrameHeaderListRel.iX - 2*TEXT_BORDER_WIDTH;

			dprintf(DEBUG_DEBUG, "   normalize width to %d , x:%d \r\n",width,x);
			loop = false;
		}
		m_pcFontHeaderList->RenderString(x + itemBox.iX + iconOffset, y + itemBox.iY, width, m_pLines->lineHeader[row].c_str(), HEADER_LIST_FONT_COLOR, 0, true); // UTF-8
		x += m_pLines->rowWidth[row] + ROW_BORDER_WIDTH;								
	}	
}

void CListFrame::scrollLineDown(const int lines)
{
	dprintf(DEBUG_DEBUG, "CListFrame::scrollLineDown \r\n");
		
	if( m_nNrOfLines <= 1) 
		return;
	
	if(m_nSelectedLine < m_nNrOfLines - 1) 
	{
		m_nSelectedLine++;
		// check if the cursor moves out of the window
		if(m_nSelectedLine - m_nCurrentLine > m_nLinesPerPage-1) 
		{
			// yes, scroll to next page
			dprintf(DEBUG_DEBUG, "[CListFrame]  m_nSelectedLine: %d, \r\n",m_nSelectedLine);
			scrollPageDown(1);
		} 
		else 
		{
			refreshLine(m_nSelectedLine - lines);
			refreshLine(m_nSelectedLine);
		}
	} 
	else 
	{
		setSelectedLine(0);
	}	
}

void CListFrame::scrollLineUp(const int lines)
{
	dprintf(DEBUG_DEBUG, "CListFrame::scrollLineUp \r\n");
	
	if( m_nNrOfLines <= 1) 
		return;

	if(m_nSelectedLine > 0) 
	{
		m_nSelectedLine--;
		// check if the cursor moves out of the window
		if(m_nSelectedLine < m_nCurrentLine ) 
		{
			// yes, scroll to next page
			dprintf(DEBUG_DEBUG, "[CListFrame]  m_nSelectedLine: %d, \r\n",m_nSelectedLine);
			scrollPageUp(1);
		} 
		else 
		{
			refreshLine(m_nSelectedLine + lines);
			refreshLine(m_nSelectedLine);
		}
	} 
	else if(m_nSelectedLine == 0) 
	{
		setSelectedLine(m_nNrOfLines - 1);
	}
}

void CListFrame::scrollPageDown(const int pages)
{
	dprintf(DEBUG_DEBUG, "CListFrame::ScrollPageDown \r\n");

	if( m_nNrOfLines <= 1) 
		return;
	
	if(m_nCurrentPage + pages < m_nNrOfPages)
	{
		m_nCurrentPage += pages; 
	}
	else 
	{
		m_nCurrentPage = m_nNrOfPages - 1;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	if(m_nSelectedLine < m_nCurrentLine || m_nSelectedLine - m_nCurrentLine >= m_nLinesPerPage )
	{
		m_nSelectedLine = m_nCurrentLine;
	}
	
	dprintf(DEBUG_DEBUG, "[CListFrame]  m_nCurrentLine: %d, m_nCurrentPage: %d \r\n", m_nCurrentLine,m_nCurrentPage);
	
	refreshPage();
};

void CListFrame::scrollPageUp(const int pages)
{
	dprintf(DEBUG_DEBUG, "CListFrame::ScrollPageUp \r\n");
	
	if( m_nNrOfLines <= 1) 
		return;

	if(m_nCurrentPage - pages > 0)
	{
		m_nCurrentPage -= pages; 
	}
	else 
	{
		m_nCurrentPage = 0;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	
	if(m_nSelectedLine < m_nCurrentLine || m_nSelectedLine - m_nCurrentLine >= m_nLinesPerPage )
	{
		m_nSelectedLine = m_nCurrentLine;
	}
	
	dprintf(DEBUG_DEBUG, "[CListFrame]  m_nCurrentLine: %d, m_nCurrentPage: %d \r\n", m_nCurrentLine, m_nCurrentPage);
	refreshPage();
}

void CListFrame::refreshPage(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::RefreshPage\r\n");

	refreshTitle();
	refreshHeaderList();
	refreshList();
	refreshScroll();
}

void CListFrame::setLines(LF_LINES* lines)
{
	if(lines == NULL) 
		return;
	
	dprintf(DEBUG_DEBUG, "CListFrame::setLines \r\n");

	m_pLines = lines;
	m_nNrOfRows = lines->rows;

	if(m_nNrOfRows > LF_MAX_ROWS)
		m_nNrOfRows = LF_MAX_ROWS;
	
	onNewLineArray();
	refreshPage();
}

void CListFrame::setTitle(const char* title, const char * const icon)
{
	dprintf(DEBUG_DEBUG, "CListFrame::setTitle:\r\n");
	
	if(!(m_nMode & TITLE)) 
		return;

	m_textTitle = title? title : "";
	m_iconTitle = icon? icon : "";
}

void CListFrame::setSelectedLine(int selection)
{
	dprintf(DEBUG_DEBUG, "CListFrame::setSelectedLine %d \r\n",selection);

	if(selection >= 0 && selection < m_nNrOfLines)
	{ 
		m_nSelectedLine = selection;
		m_nCurrentPage =  selection / m_nLinesPerPage;
		m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		refreshList();
		refreshScroll();
		
		dprintf(DEBUG_DEBUG, " selected line: %d,%d,%d \r\n",m_nSelectedLine, m_nCurrentPage, m_nCurrentLine);
	}
}

void CListFrame::hide(void)
{
	dprintf(DEBUG_DEBUG, "CListFrame::hide: %s\n", m_textTitle.c_str());

	frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	CFrameBuffer::getInstance()->blit();
}

void CListFrame::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "CListFrame::paint: %s\n", m_textTitle.c_str());

	refreshPage();
}

