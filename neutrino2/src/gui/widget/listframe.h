//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: listframe.h 21122024 mohousch Exp $
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

#if !defined(LISTFRAME_H_)
#define LISTFRAME_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/gdi/fontrenderer.h>

#include <gui/widget/component.h>


#define LF_MAX_ROWS 6

typedef struct
{
	int rows;
	std::string icon;
	std::string lineHeader[LF_MAX_ROWS];
	std::vector<std::string> lineArray[LF_MAX_ROWS];
	int rowWidth[LF_MAX_ROWS];
}LF_LINES;

class CListFrame : public CComponent
{
	public:
		typedef enum mode_
		{
			TITLE  		= 0x01,
			HEADER_LINE 	= 0x80
		}mode;

	private:
		// Functions 
		void onNewLineArray(void);
		void initVar(void);
		void refreshTitle(void);
		void refreshScroll(void);
		void refreshList(void);
		void refreshHeaderList(void);

		// Variables 
		LF_LINES* m_pLines;

		//
		CBox m_cFrameTitleRel;
		CBox m_cFrameListRel;
		CBox m_cFrameScrollRel;
		CBox m_cFrameHeaderListRel;
		
		CCScrollBar scrollBar;

		////
		int m_nMaxHeight;
		int m_nMaxWidth;
		int m_nMode;
		int m_nNrOfPages;
		int m_nNrOfLines;
		int m_nNrOfRows;
		int m_nMaxLineWidth;
		int m_nLinesPerPage;
		int m_nCurrentLine;
		int m_nCurrentPage;
		int m_nSelectedLine;
		int LinesPerPage;
		int iconOffset;

		bool m_showSelection;
		
		static CFont* m_pcFontTitle;
		std::string m_textTitle;
		int m_nFontTitleHeight;
		
		static CFont* m_pcFontList;
		int m_nFontListHeight;
		
		static CFont* m_pcFontHeaderList;
		int m_nFontHeaderListHeight;

		CFrameBuffer * frameBuffer;
		std::string m_iconTitle;

	public:
		CListFrame(const int x = 0, const int y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		CListFrame(CBox* position);

		virtual ~CListFrame();
		
		////
		bool isSelectable(void){return true;}
		bool hasItem(){if (m_pLines != NULL) return true;};

		////
		void refreshPage(void);
		void refreshLine(int line);
		inline void showSelection(bool show = true)	{m_showSelection = show; refreshLine(m_nSelectedLine);};
		
		////
		void initFrames(void);
		void hide(void);
		void paint(bool _selected = false);
		
		//// events
		void scrollPageDown(const int pages = 1);
		void scrollPageUp(const int pages = 1);				
		void scrollLineDown(const int lines = 1);
		void scrollLineUp(const int lines = 1);
		
		//// set methods
		void setLines(LF_LINES* lines);
		void setTitle(const char* title, const char * const icon = NULL);
		void setSelectedLine(int selection = 0);
		void setFont(CFont *font_text){m_pcFontList = font_text;};
		void setMode(int m){m_nMode = m; initFrames();};
		void setTitleFont(CFont *font_title){m_pcFontTitle = font_title;};

		//// get methods
		int getSelected(void){return(m_nSelectedLine);}; 
};

#endif //LISTFRAME_H_

