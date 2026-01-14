//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: textbox.h 07022025 mohousch Exp $
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

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/color.h>
#include <driver/gdi/fontrenderer.h>

#include <gui/widget/icons.h>
#include <gui/widget/component.h>


////
class CTextBox : public CComponent
{
	public:
		// pic mode
		enum 
		{
			PIC_RIGHT,
			PIC_LEFT,
			PIC_CENTER
		};

	private:
		CFrameBuffer* frameBuffer;
		
		//
		CBox m_cFrameTextRel;
		CBox m_cFrameScrollRel;
		
		CCScrollBar scrollBar;

		// variables
		std::string m_cText;
		std::vector<std::string> m_cLineArray;

		int m_tMode;

		int m_nNrOfPages;
		int m_nNrOfLines;
		int m_nNrOfNewLine;
		int m_nMaxLineWidth;
		int m_nLinesPerPage;
		int m_nCurrentLine;
		int m_nCurrentPage;

		// text
		unsigned int m_pcFontText;
		unsigned int m_nFontTextHeight;

		// backgrond
		fb_pixel_t m_textBackgroundColor;
		uint32_t m_textColor;
		int m_textRadius;
		int m_textCorner;
		//
		std::string thumbnail;
		int lx; 
		int ly; 
		int tw; 
		int th;
		bool enableFrame;
		bool bigFonts;
		bool painted;
		int borderMode;
		//
		fb_pixel_t* background;
		void saveScreen();
		void restoreScreen();

		// Functions
		void refreshTextLineArray(void);
		void initVar(void);
		void refreshScroll(void);
		void refreshText(void);
		void refreshPage(void);
		void refreshThumbnail(void);

	public:
		CTextBox(const int x = 0, const int y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		CTextBox(const CBox* position);

		virtual ~CTextBox();

		//// properties			
		void setText(const char * const newText, const char * const _thumbnail = NULL, int _tw = 0, int _th = 0, int _tmode = PIC_RIGHT, bool enable_frame = false);
		void setPosition(const int x, const int y, const int dx, const int dy);
		void setPosition(const CBox * position);
		void setCorner(int ra, int co){m_textRadius = ra; m_textCorner = co;};
		void setBackgroundColor(fb_pixel_t col){m_textBackgroundColor = col;};
		void setTextColor(uint32_t col){m_textColor = col;};
		void setFont(unsigned int font_text){m_pcFontText = font_text;};
		void setBorderMode(int m = CComponent::BORDER_ALL){borderMode = m;};
		void setBigFonts();
		
		////
		void initFrames(bool force = false);
		void paint(bool _selected = false);
		void hide(void);
		inline bool isPainted(void){return painted;};
		
		//// events
		void scrollPageDown(const int pages = 1);
		void scrollPageUp(const int pages = 1);

		////
		bool isSelectable(void){return true;}
};

#endif // TEXTBOX_H

