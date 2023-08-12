/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: textbox.cpp 2013/10/12 mohousch Exp $

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

#include <unistd.h>

#include <global.h>

#include <gui/widget/textbox.h>
#include <gui/widget/icons.h>

#include <system/debug.h>
#include <system/settings.h>


CTextBox::CTextBox(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_INFO, "CTextBox::CTextBox:\r\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	initVar();

	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	//
	initFrames();
}

CTextBox::CTextBox(CBox* position)
{
	dprintf(DEBUG_INFO, "CTextBox::CTextBox:\r\n");
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	initVar();

	if(position != NULL)
	{
		itemBox = *position;
	}
	
	//
	initFrames();
}

CTextBox::~CTextBox()
{
	dprintf(DEBUG_INFO, "CTextBox::~CTextBox\r\n");
	
	m_cLineArray.clear();
	
	if (bigFonts) 
	{
		bigFonts = false;
		g_Font[m_pcFontText]->setSize((int)(g_Font[m_pcFontText]->getSize() / BIG_FONT_FAKTOR));
	}
	
	if(background)
	{
		delete[] background;
		background = NULL;
	}
}

void CTextBox::initVar(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::InitVar:\r\n");
	
	m_cText	= "";
	m_nMode = SCROLL;
	m_tMode = PIC_RIGHT;

	m_pcFontText = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	m_nFontTextHeight = g_Font[m_pcFontText]->getHeight();

	m_nNrOfPages = 1;
	m_nNrOfLines = 0;
	m_nLinesPerPage = 1;
	m_nCurrentLine = 0;
	m_nCurrentPage = 0;

	itemBox.iWidth	= MENU_WIDTH;
	itemBox.iHeight = MENU_HEIGHT;

	itemBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - itemBox.iWidth) >>1);
	itemBox.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - itemBox.iHeight) >>1);
	
	m_textBackgroundColor = COL_MENUCONTENT_PLUS_0;
	m_textColor = COL_MENUCONTENT;
	m_textRadius = RADIUS_MID;
	m_textCorner = CORNER_NONE;

	m_cLineArray.clear();
	
	lx = itemBox.iX + 10;
	ly = itemBox.iY + 10;

	tw = th = 0;
	thumbnail = "";

	//
	bigFonts = false;
	painted = false;
	
	//
	paintframe = true;
	enableFrame = false;
	useBG = false;
	borderMode = BORDER_NO;
	
	savescreen = false;
	background = NULL;
	
	//
	widgetItem_type = WIDGETITEM_TEXTBOX;
}

void CTextBox::setPosition(const int x, const int y, const int dx, const int dy)
{
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;

	initFrames();
}

void CTextBox::setPosition(const CBox * position)
{
	if(position != NULL)
	{
		itemBox = *position;
	}

	initFrames();
}

void CTextBox::setMode(const int mode)
{
	m_nMode = mode; 

	if( !(mode & NO_AUTO_LINEBREAK))
	{
		m_nMode = m_nMode & ~AUTO_WIDTH;
	}
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

	refreshTextLineArray();
	refresh();
}

void CTextBox::reSizeTextFrameWidth(int textWidth)
{
	dprintf(DEBUG_DEBUG, "CTextBox::ReSizeTextFrameWidth: %d, current: %d\r\n", textWidth, m_cFrameTextRel.iWidth);

	int iNewWindowWidth = textWidth + m_cFrameScrollRel.iWidth + BORDER_LEFT + BORDER_RIGHT;

	if( iNewWindowWidth > itemBox.iWidth) 
		iNewWindowWidth = itemBox.iWidth;
	if( iNewWindowWidth < MIN_WINDOW_WIDTH) 
		iNewWindowWidth = MIN_WINDOW_WIDTH;

	m_cFrameTextRel.iWidth	= iNewWindowWidth;

	// Re-Init the children frames due to new main window
	initFrames();
}

void CTextBox::reSizeTextFrameHeight(int textHeight)
{
	dprintf(DEBUG_DEBUG, "CTextBox::ReSizeTextFrameHeight: %d, current: %d\r\n", textHeight, m_cFrameTextRel.iHeight);

	int iNewWindowHeight = textHeight + BORDER_LEFT + BORDER_RIGHT;

	if( iNewWindowHeight > itemBox.iHeight) 
		iNewWindowHeight = itemBox.iHeight;
	if( iNewWindowHeight < MIN_WINDOW_HEIGHT) 
		iNewWindowHeight = MIN_WINDOW_HEIGHT;

	m_cFrameTextRel.iHeight = iNewWindowHeight;

	// reinit the children frames due to new main window
	initFrames();
}

void CTextBox::initFrames(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::InitFrames:\r\n");

	m_cFrameTextRel.iX = itemBox.iX + BORDER_LEFT;
	m_cFrameTextRel.iY = itemBox.iY + 10;
	m_cFrameTextRel.iHeight = itemBox.iHeight - 20;
	
	if(m_nMode & SCROLL)
	{
		m_cFrameScrollRel.iX = itemBox.iX + itemBox.iWidth - SCROLLBAR_WIDTH;
		m_cFrameScrollRel.iY = itemBox.iY;
		m_cFrameScrollRel.iWidth = SCROLLBAR_WIDTH;
		m_cFrameScrollRel.iHeight = itemBox.iHeight;
	}
	else
	{
		m_cFrameScrollRel.iX = 0;
		m_cFrameScrollRel.iY = 0;
		m_cFrameScrollRel.iHeight = 0;
		m_cFrameScrollRel.iWidth = 0;
	}

	m_cFrameTextRel.iWidth = itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - m_cFrameScrollRel.iWidth;

	m_nLinesPerPage = m_cFrameTextRel.iHeight/m_nFontTextHeight;
}

void CTextBox::saveScreen()
{
	//
	if (savescreen)
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
}

void CTextBox::restoreScreen()
{
	if(savescreen && background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
}

void CTextBox::refreshTextLineArray(void)
{      
	dprintf(DEBUG_DEBUG, "CTextBox::RefreshLineArray:\r\n");
	
	int loop = true;
	int pos_prev = 0;
	int pos = 0;
	int aktWidth = 0;
	int aktWordWidth = 0;
	int lineBreakWidth;
	int maxTextWidth = 0;

	m_nNrOfNewLine = 0;

	std::string aktLine = "";
	std::string aktWord = "";

	// clear current line vector
	m_cLineArray.clear();
	m_nNrOfLines = 0;

	if( m_nMode & AUTO_WIDTH)
	{
		// In case of autowidth, we calculate the max allowed width of the textbox
		lineBreakWidth = m_cFrameTextRel.iWidth - BORDER_LEFT - BORDER_RIGHT - m_cFrameScrollRel.iWidth;
	}
	else
	{
		// If not autowidth, we just take the actuall textframe width
		lineBreakWidth = m_cFrameTextRel.iWidth;
	}
	
	//
	if( (!access(thumbnail.c_str(), F_OK) && m_nCurrentPage == 0) && m_tMode != PIC_CENTER)
		lineBreakWidth = m_cFrameTextRel.iWidth - tw - 10;
	
	const int TextChars = m_cText.size();
	
	// do not parse, if text is empty 
	if(!m_cText.empty())
	{
		//m_cText += "\n";

		while(loop)
		{
			if(m_nMode & NO_AUTO_LINEBREAK)
			{
				pos = m_cText.find_first_of("\n", pos_prev);
			}
			else
			{
				pos = m_cText.find_first_of("\n-. ", pos_prev);
			}

			//if(pos == -1)
			if(pos > TextChars || pos < 0)
			{
				pos = TextChars + 1;
				loop = false; // note, this is not 100% correct. if the last characters does not fit in one line, the characters after are cut
				//break;
			}

			aktWord = m_cText.substr(pos_prev, pos - pos_prev + 1);
			aktWordWidth = g_Font[m_pcFontText]->getRenderWidth(aktWord, true);
			pos_prev = pos + 1;
			
			// if(aktWord.find("&quot;") == )
			if(1)
			{
				if( aktWidth + aktWordWidth > lineBreakWidth && !(m_nMode & NO_AUTO_LINEBREAK))
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
				if (aktWidth > maxTextWidth) 
					maxTextWidth = aktWidth;

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
					if( m_nMode & AUTO_WIDTH)
					{
						lineBreakWidth = m_cFrameTextRel.iWidth - m_cFrameScrollRel.iWidth - BORDER_LEFT - BORDER_RIGHT;
					}
					else
					{
						lineBreakWidth = m_cFrameTextRel.iWidth;
					}
				}

				// 2nd page and over
				if(m_nNrOfLines > ((m_cFrameTextRel.iHeight) / m_nFontTextHeight))
				{
					if( m_nMode & AUTO_WIDTH)
					{
						lineBreakWidth = m_cFrameTextRel.iWidth - m_cFrameScrollRel.iWidth - BORDER_LEFT - BORDER_RIGHT;
					}
					else
					{
						lineBreakWidth = m_cFrameTextRel.iWidth;
					}
				}
			}
		}

		// check if we have to recalculate the window frame size, due to auto width and auto height
		if( m_nMode & AUTO_WIDTH)
		{
			reSizeTextFrameWidth(maxTextWidth);
		}

		if(m_nMode & AUTO_HIGH)
		{
			reSizeTextFrameHeight(m_nNrOfLines * m_nFontTextHeight);
		}

		// linesPerPage
		m_nLinesPerPage = (m_cFrameTextRel.iHeight) / m_nFontTextHeight;

		//if(m_tMode == PIC_CENTER && m_nCurrentPage == 0)
		//{
		//	m_nLinesPerPage = (m_cFrameTextRel.iHeight - th - 10) / m_nFontTextHeight;
		//}
		//else if(m_tMode == PIC_CENTER && m_nCurrentPage > 0)
		//	m_nLinesPerPage = (m_cFrameTextRel.iHeight) / m_nFontTextHeight; // FIXME:

		// NrOfPages
		if(m_nLinesPerPage > 0)
			m_nNrOfPages = ((m_nNrOfLines - 1) / m_nLinesPerPage) + 1; //FIXME: 
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

void CTextBox::refreshScroll(void)
{
	if(!(m_nMode & SCROLL)) 
		return;

	if (m_nNrOfPages > 1) 
	{
		scrollBar.paint(&m_cFrameScrollRel, m_nNrOfPages, m_nCurrentPage);
	}
}

void CTextBox::refreshText(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::refreshText:\r\n");

	// restore screen
	restoreScreen();
	
	// paint background	
	if(paintframe)
	{
		// border
		CFrameBuffer::getInstance()->paintFrameBox(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_6);
	
		// bg
		if (borderMode == BORDER_NO)
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, m_textBackgroundColor, m_textRadius, m_textCorner);
		else if (borderMode == BORDER_ALL)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, itemBox.iHeight - 4, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX + 2, itemBox.iY, itemBox.iWidth - 4, itemBox.iHeight, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			CFrameBuffer::getInstance()->paintBoxRel(itemBox.iX, itemBox.iY + 2, itemBox.iWidth, itemBox.iHeight - 4, m_textBackgroundColor, m_textRadius, m_textCorner);
		}
	}
	
	// paint thumbnail (paint picture only on first page)
	if(m_nCurrentPage == 0 && !access(thumbnail.c_str(), F_OK) )
	{
		if (enableFrame) CFrameBuffer::getInstance()->paintFrameBox(lx, ly, tw, th, COL_WHITE_PLUS_0);
		
		// picture
		CFrameBuffer::getInstance()->displayImage(thumbnail.c_str(), lx + THUMBNAIL_OFFSET, ly + THUMBNAIL_OFFSET, tw - THUMBNAIL_OFFSET, th - THUMBNAIL_OFFSET);
	}
	
	// paint text
	int y = m_cFrameTextRel.iY + 10;
	int i;
	int x_start = 0;

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
			}
		}

		g_Font[m_pcFontText]->RenderString(m_cFrameTextRel.iX + x_start, y, m_cFrameTextRel.iWidth, m_cLineArray[i].c_str(), m_textColor, 0, true, useBG); // UTF-8
	}
}

void CTextBox::scrollPageDown(const int pages)
{
	if( !(m_nMode & SCROLL)) 
		return;
	
	if( m_nNrOfLines <= 0) 
		return;
	
	dprintf(DEBUG_DEBUG, "CTextBox::ScrollPageDown:\r\n");

	if(m_nCurrentPage + pages < m_nNrOfPages)
	{
		m_nCurrentPage += pages; 
	}
	else 
	{
		m_nCurrentPage = m_nNrOfPages - 1;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refresh();
}

void CTextBox::scrollPageUp(const int pages)
{
	if( !(m_nMode & SCROLL)) 
		return;
	
	if( m_nNrOfLines <= 0) 
		return;
	
	dprintf(DEBUG_DEBUG, "CTextBox::ScrollPageUp:\r\n");

	if(m_nCurrentPage - pages > 0)
	{
		m_nCurrentPage -= pages; 
	}
	else 
	{
		m_nCurrentPage = 0;
	}
	
	m_nCurrentLine = m_nCurrentPage * m_nLinesPerPage; 
	refresh();
}

void CTextBox::refresh(void)
{
	dprintf(DEBUG_DEBUG, "CTextBox::Refresh:\r\n");	

	// paint text
	refreshText();

	// paint scrollbar
	refreshScroll();	
}

bool CTextBox::setText(const char * const newText, const char * const _thumbnail, int _tw, int _th, int _tmode, bool enable_frame, const bool useBackground)
{
	dprintf(DEBUG_DEBUG, "CTextBox::setText:\r\n");

	m_tMode = _tmode;
	enableFrame = enable_frame;
	useBG = useBackground;

	// thumbnail
	thumbnail = "";
	
	if(_thumbnail != NULL && !access(_thumbnail, F_OK))
	{
		thumbnail = _thumbnail;

		tw = _tw;
		th = _th;

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
	
	// text
	m_nNrOfPages = 0;
	m_nNrOfLines = 0;
	m_nCurrentPage = 0;
	m_nCurrentLine = 0;
	m_nLinesPerPage = 1;
		
	bool result = false;
	
	if (newText != NULL /*|| thumbnail.empty()*/)
	{
		m_cText = newText;
		
		refreshTextLineArray();
		
		result = true;
	}
	
	return(result);
}

void CTextBox::paint(void)
{
	dprintf(DEBUG_INFO, "CTextBox::paint:\n");
	
	//
	initFrames();
	
	//
	saveScreen(); //FIXME:
	
	//
	refresh();	
	
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
	
	if(savescreen && background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
	else if (paintframe)
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	
	CFrameBuffer::getInstance()->blit();

	painted = false;
}

