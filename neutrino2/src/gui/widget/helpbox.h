//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: helpbox.h 21122024 mohousch Exp $
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

#ifndef __helpbox__
#define __helpbox__

#include <string>
#include <vector>

#include <system/localize.h>
#include <system/settings.h>


////
class Drawable
{
	public:

		enum DType 
		{
			DTYPE_DRAWABLE,
			DTYPE_TEXT,
			DTYPE_ICON,
			DTYPE_SEPARATOR,
			DTYPE_PAGEBREAK
		};

		virtual ~Drawable();
		
		/**
		* Overwrite this method in subclasses to draw on the window
		*
		* @param window the window to draw on
		* @param x x component of the top left corner
		* @param y y component of the top left corner
		*/
		virtual void draw(int x, int y, int width) = 0;
		virtual int getWidth(void);
		virtual int getHeight(void);

		/**
		* Returns the type of this drawable. Used to distinguish between
		* drawing objects and control objects like pagebreaks.
		* @return the type of this drawable.
		*/
		virtual DType getType();

	protected:

		Drawable();

		int m_height;
		int m_width;		
};

typedef std::vector<std::vector<Drawable*> > ContentLines;

/**
 * This class draws a given string.
 */
class DText : public Drawable
{
	public:
		DText(std::string& text);
		DText(const char *text);
		void init();
		void draw(int x, int y, int width);
		DType getType();

	protected:
		std::string m_text;
		CFont* m_font;
		uint32_t m_color;
		bool m_background;
};

/**
 * This class draws a given icon.
 */
class DIcon : public Drawable
{
	public:
		DIcon(std::string& icon);
		DIcon(const char  *icon);
		void init();
		void draw(int x, int y, int width);
		DType getType();

	protected:
		std::string m_icon;
};

/**
 * This class draws separator line.
 */
class DSeparator : public Drawable
{
	public:
		DSeparator();

		void draw(int x, int y, int width);
		DType getType();
};

/**
 * This class is used as a control object and forces a new page
 * in scrollable windows.
 */
class DPagebreak : public Drawable
{
	public:
		DPagebreak();

		void draw(int x, int y, int width);
		DType getType();	
};

////
class CHelpBox
{
	protected:
		CBox cFrameBox;
		CWidget *widget;
		CCHeaders *headers;
		CCScrollBar scrollBar;
		////
		unsigned int m_currentPage;
		std::vector<int>m_startEntryOfPage;
		int m_maxEntriesPerPage;
		int m_pages;
		////
		int m_width;
		int m_height;
		int m_iheight;
		int m_fheight;
		int m_theight;
		////
		std::string m_caption;
		std::string  m_iconfile;
		////
		int borderMode;
		fb_pixel_t borderColor;
		////
		ContentLines m_lines;
		////
		void refreshPage();
		void init();
		void initFrames(void);
		bool has_scrollbar(void);
		void scroll_up(void);
		void scroll_down(void);
		void paint(void);
		void hide(void);

	public:
		//
		CHelpBox(const char * const Caption, const int Width = HELPBOX_WIDTH, const char * const Icon = NULL);
		virtual ~CHelpBox(void);
		
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};
		
		void addLine(const char *text);
		void addLine(const char *icon, const char *text);
		void addSeparator();
		void addPagebreak();

		int exec(int timeout = -1);
};

#endif

