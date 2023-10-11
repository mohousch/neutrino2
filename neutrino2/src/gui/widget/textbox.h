/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: textbox.h 2013/10/12 mohousch Exp $

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

#if !defined(TEXTBOX_H)
#define TEXTBOX_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <driver/framebuffer.h>
#include <driver/color.h>
#include <driver/fontrenderer.h>

#include <gui/widget/widget_helpers.h>


// mode
enum
{
	AUTO_WIDTH		= 0x01,
	AUTO_HIGH		= 0x02,
	SCROLL			= 0x04,
	CENTER			= 0x40,
	NO_AUTO_LINEBREAK 	= 0x80
};

// pic
enum 
{
	PIC_RIGHT,
	PIC_LEFT,
	PIC_CENTER
};

class CTextBox : public CComponent
{
	private:
		CFrameBuffer* frameBuffer;
		
		//
		CBox m_cFrameTextRel;
		CBox m_cFrameScrollRel;

		// variables
		std::string m_cText;
		std::vector<std::string> m_cLineArray;

		int m_nMode;
		int m_tMode;

		int m_nNrOfPages;
		int m_nNrOfLines;
		int m_nNrOfNewLine;
		int m_nMaxLineWidth;
		int m_nLinesPerPage;
		int m_nCurrentLine;
		int m_nCurrentPage;
		
		CScrollBar scrollBar;

		// text
		unsigned int m_pcFontText;
		unsigned int m_nFontTextHeight;
		bool useBG;

		// backgrond
		fb_pixel_t m_textBackgroundColor;
		uint8_t m_textColor;
		int m_textRadius;
		int m_textCorner;
		
		std::string thumbnail;
		int lx; 
		int ly; 
		int tw; 
		int th;
		bool enableFrame;

		bool bigFonts;
		bool painted;
		int borderMode;
		
		fb_pixel_t* background;
		void saveScreen();
		void restoreScreen();

		// Functions
		void refreshTextLineArray(void);
		void initVar(void);
		void initFrames(void);
		void refreshScroll(void);
		void refreshText(void);
		void refreshPage(void);
		void reSizeTextFrameWidth(int maxTextWidth);
		void reSizeTextFrameHeight(int maxTextHeight);

	public:
		CTextBox(const int x = 0, const int y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		CTextBox(CBox* position);

		virtual ~CTextBox();

		// Functions
		void scrollPageDown(const int pages = 1);
		void scrollPageUp(const int pages = 1);

		//				
		bool setText(const char * const newText, const char * const _thumbnail = NULL, int _tw = 0, int _th = 0, int _tmode = PIC_RIGHT, bool enable_frame = false, const bool useBackground = false);
		
		inline bool isPainted(void){return painted;};

		//
		inline int getMaxLineWidth(void){return(m_nMaxLineWidth);};
		inline int getLines(void){return(m_nNrOfLines);};
		inline int getPages(void){return(m_nNrOfPages);};
		inline void movePosition(int x, int y){itemBox.iX = x; itemBox.iY = y;};

		void setPosition(const int x, const int y, const int dx, const int dy);
		void setPosition(const CBox * position);
		void setCorner(int ra, int co){m_textRadius = ra; m_textCorner = co;};
		void setBackgroundColor(fb_pixel_t col){m_textBackgroundColor = col;};
		void setTextColor(uint8_t col){m_textColor = col;};
		void setFont(unsigned int font_text){m_pcFontText = font_text;};
		void setMode(const int mode);
		void setBorderMode(int m = BORDER_ALL){borderMode = m;};

		void paint(void);
		void hide(void);

		void setBigFonts();

		//
		bool isSelectable(void){return true;}
};

#endif // !defined(AFX_TEXTBOX_H__208DED01_ABEC_491C_A632_5B21057DC5D8__INCLUDED_)

