
/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

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

	***********************************************************

	Module Name: listframe.h .

	Description: interface of the CTextBox class

	Date:	Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

	Revision History:
	Date			Author		Change Description
	   Nov 2005		Günther	initial implementation
****************************************************************************/

#if !defined(LISTFRAME_H_)
#define LISTFRAME_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/fontrenderer.h>

#include <gui/widget/widget_helpers.h>


#define LF_MAX_ROWS 6

typedef struct
{
	int rows;
	std::string lineHeader[LF_MAX_ROWS];
	std::vector<std::string> lineArray[LF_MAX_ROWS];
	int rowWidth[LF_MAX_ROWS];
}LF_LINES;

class CListFrame : public CComponent
{
	public:
		typedef enum mode_
		{
			AUTO_WIDTH	= 0x01,
			AUTO_HIGH	= 0x02,
			SCROLL		= 0x04,
			TITLE  		= 0x08,
			HEADER_LINE 	= 0x80
		}mode;

	private:
		// Functions 
		void onNewLineArray(void);
		void initVar(void);
		void initFrames(void);
		void refreshTitle(void);
		void refreshScroll(void);
		void refreshList(void);
		void refreshHeaderList(void);
		void reSizeMainFrameWidth(int maxTextWidth);
		void reSizeMainFrameHeight(int maxTextHeight);

		// Variables 
		LF_LINES* m_pLines;

		//
		CBox m_cFrameTitleRel;
		CBox m_cFrameListRel;
		CBox m_cFrameScrollRel;
		CBox m_cFrameHeaderListRel;

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

		bool m_showSelection;
		
		static CFont* m_pcFontTitle;
		std::string m_textTitle;
		int m_nFontTitleHeight;
		
		static CFont* m_pcFontList;
		int m_nFontListHeight;
		
		static CFont* m_pcFontHeaderList;
		int m_nFontHeaderListHeight;
		
		//
		int LinesPerPage;

		CFrameBuffer * frameBuffer;
		std::string m_iconTitle;

		CScrollBar scrollBar;
	public:
		CListFrame();
		CListFrame(LF_LINES* lines);
		CListFrame(LF_LINES* lines, 
					CFont* font_text,
					const int mode, 
					const CBox* position,
					const char* textTitle = NULL,
					CFont *font_title = NULL);

		virtual ~CListFrame();

		// Functions
		void    refreshPage(void);
		void    refreshLine(int line);
		void    scrollPageDown(const int pages = 1);
		void    scrollPageUp(const int pages = 1);				
		void 	scrollLineDown(const int lines = 1);
		void 	scrollLineUp(const int lines = 1);
		bool	setLines(LF_LINES* lines);
		bool	setTitle(const char* title = "", const std::string& icon = NULL);
		bool	setSelectedLine(int selection = 0);
		void	hide(void);
		void	paint(void);

		inline	int	getMaxLineWidth(void)		{return(m_nMaxLineWidth);};
		inline  int     getSelectedLine(void)		{return(m_nSelectedLine);};
		inline  int     getLines(void)			{return(m_nNrOfLines);};
		inline  int     getPages(void)			{return(m_nNrOfPages);};
		inline int 	getLinesPerPage(void)		{return(LinesPerPage);};
		
		//
		int getSelected(void){return(m_nSelectedLine);}; 

		//
		inline  void    showSelection(bool show = true)	{m_showSelection = show; refreshLine(m_nSelectedLine);};
		inline	void	movePosition(int x, int y)	{itemBox.iX = x; itemBox.iY = y;};

		//
		bool isSelectable(void){return true;}
		bool hasItem(){if (m_pLines != NULL) return true;};
};

#endif //LISTFRAME_H_

