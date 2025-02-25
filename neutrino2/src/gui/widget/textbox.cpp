//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: textbox.cpp 07022025 mohousch Exp $
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

#include <unistd.h>

#include <global.h>

#include <gui/widget/textbox.h>

#include <system/debug.h>
#include <system/settings.h>


CTextBox::CTextBox(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_INFO, "CTextBox::CTextBox:\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 

	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	//
	initVar();
	
	//
	initFrames();
}

CTextBox::CTextBox(CBox* position)
{
	dprintf(DEBUG_INFO, "CTextBox::CTextBox:\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 

	if(position != NULL)
	{
		itemBox = *position;
		oldPosition = itemBox;
	}
	
	//
	initVar();
	
	//
	initFrames();
}

CTextBox::~CTextBox()
{
	dprintf(DEBUG_INFO, "CTextBox::~CTextBox\n");
	
	//
	m_cLineArray.clear();
	
	//
	if (bigFonts) 
	{
		bigFonts = false;
		g_Font[m_pcFontText]->setSize((int)(g_Font[m_pcFontText]->getSize() / BIG_FONT_FAKTOR));
	}
	
	//
	if(background)
	{
		delete[] background;
		background = NULL;
	}
}

void CTextBox::initVar(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::InitVar:\n");
	
	m_cText	= "";
	m_tMode = PIC_RIGHT;

	m_pcFontText = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	m_nFontTextHeight = g_Font[m_pcFontText]->getHeight();

	m_nNrOfPages = 1;
	m_nNrOfLines = 0;
	m_nLinesPerPage = 1;
	m_nCurrentLine = 0;
	m_nCurrentPage = 0;
	
	m_textBackgroundColor = COL_MENUCONTENT_PLUS_0;
	m_textColor = COL_MENUCONTENT_TEXT_PLUS_0;
	m_textRadius = NO_RADIUS;
	m_textCorner = CORNER_NONE;

	m_cLineArray.clear();

	tw = th = 0;
	thumbnail = "";

	//
	bigFonts = false;
	painted = false;
	
	//
	paintframe = true;
	inFocus = true;
	enableFrame = false;
	borderMode = CComponent::BORDER_NO;
	
	background = NULL;
	
	//
	cc_type = CC_TEXTBOX;
}

void CTextBox::setPosition(const int x, const int y, const int dx, const int dy)
{
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;

	initFrames(true);
}

void CTextBox::setPosition(const CBox * position)
{
	if(position != NULL)
	{
		itemBox = *position;
	}

	initFrames(true);
}

void CTextBox::initFrames(bool force)
{
	dprintf(DEBUG_INFO, "CTextBox::InitFrames:\n");
	
	// sanity check
	if(itemBox.iHeight > ((int)CFrameBuffer::getInstance()->getScreenHeight(true)))
		itemBox.iHeight = CFrameBuffer::getInstance()->getScreenHeight(true) - 4;  	// 4 pixels for border

	// sanity check
	if(itemBox.iWidth > (int)CFrameBuffer::getInstance()->getScreenWidth(true))
		itemBox.iWidth = CFrameBuffer::getInstance()->getScreenWidth(true) - 4; 	// 4 pixels for border

	m_cFrameTextRel.iX = itemBox.iX + BORDER_LEFT;
	m_cFrameTextRel.iY = itemBox.iY + 10;
	m_cFrameTextRel.iHeight = itemBox.iHeight - 20;
	
	m_cFrameScrollRel.iX = itemBox.iX + itemBox.iWidth - SCROLLBAR_WIDTH;
	m_cFrameScrollRel.iY = itemBox.iY;
	m_cFrameScrollRel.iWidth = SCROLLBAR_WIDTH;
	m_cFrameScrollRel.iHeight = itemBox.iHeight;

	m_cFrameTextRel.iWidth = itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - m_cFrameScrollRel.iWidth;
	
	lx = itemBox.iX + 10;
	ly = itemBox.iY + 10;

	m_nLinesPerPage = m_cFrameTextRel.iHeight/m_nFontTextHeight;
}

void CTextBox::setBigFonts()
{
	bigFonts = bigFonts? false : true;
	
	dprintf(DEBUG_INFO, "CTextBox::setBigFonts: %d\n", bigFonts? 1 : 0);

	if(bigFonts)
	{
		g_Font[m_pcFontText]->setSize((int)(g_Font[m_pcFontText]->getSize() * BIG_FONT_FAKTOR));
	}
	else
	{
		g_Font[m_pcFontText]->setSize((int)(g_Font[m_pcFontText]->getSize() / BIG_FONT_FAKTOR));
	}

	refreshPage();
}

void CTextBox::saveScreen()
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

void CTextBox::restoreScreen()
{
	if (background)
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CTextBox::refreshTextLineArray(void)
{      
	dprintf(DEBUG_DEBUG, "CTextBox::RefreshLineArray:\n");
	
	int loop = true;
	int pos_prev = 0;
	int pos = 0;
	int aktWidth = 0;
	int aktWordWidth = 0;
	int lineBreakWidth = 0;

	m_nNrOfNewLine = 0;

	std::string aktLine = "";
	std::string aktWord = "";

	// clear current line vector
	m_cLineArray.clear();
	m_nNrOfLines = 0;

	// If not autowidth, we just take the actuall textframe width
	lineBreakWidth = m_cFrameTextRel.iWidth;
	
	//
	if( (!access(thumbnail.c_str(), F_OK) && m_nCurrentPage == 0) && m_tMode != PIC_CENTER)
	{
		lineBreakWidth = m_cFrameTextRel.iWidth - tw - 10;
		
		if (halign == CC_ALIGN_CENTER) // FIXME:
			lineBreakWidth = m_cFrameTextRel.iWidth - 2*tw - 20;	
	}
	
	const int TextChars = m_cText.size();
	
	// do not parse, if text is empty 
	if(!m_cText.empty())
	{
		while(loop)
		{
			pos = m_cText.find_first_of("\n-. ", pos_prev);

			//
			if(pos > TextChars || pos < 0)
			{
				pos = TextChars + 1;
				loop = false;
			}

			aktWord = m_cText.substr(pos_prev, pos - pos_prev + 1);
			aktWordWidth = g_Font[m_pcFontText]->getRenderWidth(aktWord, true);
			pos_prev = pos + 1;
			
			//
			if(1)
			{
				if( aktWidth + aktWordWidth > lineBreakWidth)
				{
					// we need a new line before we can continue
					m_cLineArray.push_back(aktLine);
					
					m_nNrOfLines++;
					aktLine = "";
					aktWidth = 0;

					if(pos_prev >= TextChars) 
						loop = false;
				}

				aktLine  += aktWord;
				aktWidth += aktWordWidth;

				//
				if( m_cText[pos] == '\n' || loop == false)
				{
					// current line ends with an carriage return, make new line
					if (m_cText[pos] == '\n')
						aktLine.erase(aktLine.size() - 1, 1);
						
					m_cLineArray.push_back(aktLine);
					m_nNrOfLines++;
					aktLine = "";
					aktWidth = 0;
					m_nNrOfNewLine++;
					
					if(pos_prev >= TextChars) 
						loop = false;
				}
				
				//recalculate breaklinewidth for other pages or when pic dont exists
				if(m_nNrOfLines > (th / m_nFontTextHeight))
				{
					lineBreakWidth = m_cFrameTextRel.iWidth;	
				}

				// 2nd page and over
				if(m_nNrOfLines > ((m_cFrameTextRel.iHeight) / m_nFontTextHeight))
				{
					lineBreakWidth = m_cFrameTextRel.iWidth;
				}
			}
		}

		// linesPerPage
		m_nLinesPerPage = (m_cFrameTextRel.iHeight) / m_nFontTextHeight;

		// NrOfPages
		if(m_nLinesPerPage > 0)
			m_nNrOfPages = ((m_nNrOfLines - 1) / m_nLinesPerPage) + 1;
		else
			m_nNrOfPages = 1;

		if(m_nCurrentPage >= m_nNrOfPages)
		{
			m_nCurrentPage = m_nNrOfPages - 1;
			m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage;
		}
	}
	else
	{
		m_nNrOfPages = 0;
		m_nNrOfLines = 0;
		m_nCurrentPage = 0;
		m_nCurrentLine = 0;
		m_nLinesPerPage = 1;
	}
}

void CTextBox::refreshThumbnail(void)
{
	dprintf(DEBUG_NORMAL, "CTextBox::refreshThumbnail\n");
	
	// check th
	if(itemBox.iHeight > MAX_WINDOW_HEIGHT/2)
	{
		if(th > itemBox.iHeight/2)
			th = itemBox.iHeight/2 - 20;
	}
	else
	{
		if(th >= (itemBox.iHeight - 20))
			th = itemBox.iHeight - 20;
	}

	if(th >= (itemBox.iHeight - 20))
		th = itemBox.iHeight - 20;
		
	// check tw
	if(itemBox.iWidth > MAX_WINDOW_WIDTH/2)
	{
		if(tw > itemBox.iWidth/2)
			tw = itemBox.iWidth/2 - 20;
	}
	else if(itemBox.iWidth <= MAX_WINDOW_WIDTH/2)
	{
		if(tw >= itemBox.iWidth)
			tw = itemBox.iWidth - 20;
	}

	// position
	if(m_tMode == PIC_RIGHT)
	{
		lx = itemBox.iX + itemBox.iWidth - (tw + m_cFrameScrollRel.iWidth + 10);
		ly = itemBox.iY + 10;
	}
	else if(m_tMode == PIC_LEFT)
	{
		lx = itemBox.iX + 10;
		ly = itemBox.iY + 10;
	}
	else if(m_tMode == PIC_CENTER)
	{
		lx = itemBox.iX + (itemBox.iWidth - tw)/2;
		ly = itemBox.iY + 10;
	}
}

void CTextBox::refreshScroll(void)
{
	if (m_nNrOfPages > 1) 
	{
		scrollBar.paint(&m_cFrameScrollRel, m_nNrOfPages, m_nCurrentPage);
	}
}

void CTextBox::refreshText(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::refreshText:\n");

	// restoreScreen/paint background	
	if (!paintframe)
		restoreScreen();
	else if(paintframe)
	{
		// border
		CFrameBuffer::getInstance()->paintFrameBox(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_6);
	
		// bg
		if (borderMode == CComponent::BORDER_NO)
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, m_textBackgroundColor, m_textRadius, m_textCorner);
		else if (borderMode == CComponent::BORDER_ALL)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, itemBox.iHeight - 4, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
		else if (borderMode == CComponent::BORDER_LEFTRIGHT)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX + 2, itemBox.iY, itemBox.iWidth - 4, itemBox.iHeight, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
		else if (borderMode == CComponent::BORDER_TOPBOTTOM)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY + 2, itemBox.iWidth, itemBox.iHeight - 4, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
	}
	
	// paint thumbnail (paint picture only on first page)
	if(m_nCurrentPage == 0 && !access(thumbnail.c_str(), F_OK) )
	{
		if (enableFrame) 
			CFrameBuffer::getInstance()->paintFrameBox(lx, ly, tw, th, COL_MENUCONTENTSELECTED_PLUS_0);
		
		// picture
		CFrameBuffer::getInstance()->displayImage(thumbnail.c_str(), lx + THUMBNAIL_OFFSET, ly + THUMBNAIL_OFFSET, tw - 2*THUMBNAIL_OFFSET, th - 2*THUMBNAIL_OFFSET);
	}
	
	// paint text
	int y = m_cFrameTextRel.iY + 10;
	int i;
	int x_start = 0;
	int startPosX = m_cFrameTextRel.iX + x_start;

	if(m_tMode == PIC_CENTER && m_nCurrentPage == 0)
	{
		y = y + th + 10;
		m_nLinesPerPage = (m_cFrameTextRel.iHeight - th - 10) / m_nFontTextHeight;
	}

	for(i = m_nCurrentLine; i < m_nNrOfLines && i < m_nCurrentLine + m_nLinesPerPage; i++)
	{
		y += m_nFontTextHeight;

		// x_start FIXME:		
		if( !access(thumbnail.c_str(), F_OK) && (m_nCurrentPage == 0))
		{
			if (m_tMode == PIC_LEFT)
			{
				if(i <= (th / m_nFontTextHeight))
					x_start = tw + 10;
				else
					x_start = 0;
			}
		}
		
		if (halign == CC_ALIGN_CENTER)
			startPosX = m_cFrameTextRel.iX + x_start + (m_cFrameTextRel.iWidth - g_Font[m_pcFontText]->getRenderWidth(m_cLineArray[i]))/2;
		else if (halign == CC_ALIGN_RIGHT)
			startPosX = m_cFrameTextRel.iX + x_start + m_cFrameTextRel.iWidth - g_Font[m_pcFontText]->getRenderWidth(m_cLineArray[i]);
		else
			startPosX = m_cFrameTextRel.iX + x_start;

		g_Font[m_pcFontText]->RenderString(startPosX, y, m_cFrameTextRel.iWidth, m_cLineArray[i].c_str(), m_textColor); // UTF-8
	}
}

void CTextBox::scrollPageDown(const int pages)
{	
	if( m_nNrOfLines <= 0) 
		return;
	
	dprintf(DEBUG_DEBUG, "CTextBox::ScrollPageDown:\n");

	if(m_nCurrentPage + pages < m_nNrOfPages)
	{
		m_nCurrentPage += pages; 
	}
	else 
	{
		m_nCurrentPage = m_nNrOfPages - 1;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refreshPage();
}

void CTextBox::scrollPageUp(const int pages)
{	
	if( m_nNrOfLines <= 0) 
		return;
	
	dprintf(DEBUG_DEBUG, "CTextBox::ScrollPageUp:\n");

	if(m_nCurrentPage - pages > 0)
	{
		m_nCurrentPage -= pages; 
	}
	else 
	{
		m_nCurrentPage = 0;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refreshPage();
}

void CTextBox::refreshPage(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::RefreshPage:\n");
	
	////
	refreshTextLineArray();
	refreshThumbnail();

	// paint text
	refreshText();

	// paint scrollbar
	refreshScroll();	
}

void CTextBox::setText(const char * const newText, const char * const _thumbnail, int _tw, int _th, int _tmode, bool enable_frame)
{
	dprintf(DEBUG_INFO, "CTextBox::setText:\n");

	m_tMode = _tmode;
	enableFrame = enable_frame;

	// thumbnail
	thumbnail = "";
	
	if(_thumbnail != NULL && !access(_thumbnail, F_OK))
	{
		thumbnail = _thumbnail;

		tw = _tw;
		th = _th;
	}

	// text	
	if (newText != NULL)
	{
		m_cText = newText;
	}
}

void CTextBox::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "CTextBox::paint:\n");
	
	//
//	initFrames();
	
	//
	if (!paintframe)
		saveScreen();
	
	//
	refreshPage();	
	
	painted = true;
}

void CTextBox::hide(void)
{
	dprintf(DEBUG_INFO, "CTextBox::hide:\n");

	if (bigFonts) 
	{
		bigFonts = false;
		g_Font[m_pcFontText]->setSize((int)(g_Font[m_pcFontText]->getSize() / BIG_FONT_FAKTOR));
	}
	
	//
	if (!paintframe)
	{
		restoreScreen();
	}
	else if (paintframe)
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	
	CFrameBuffer::getInstance()->blit();

	painted = false;
}

