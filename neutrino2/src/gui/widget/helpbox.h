/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: helpbox.h 03112024 mohousch Exp $

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


#ifndef __helpbox__
#define __helpbox__

#include <string>
#include <vector>

#include <system/localize.h>
#include <system/settings.h>

#include <gui/widget/messagebox.h>


class CHelpBox
{	
	protected:
		CBox cFrameBox;
		CWidget *widget;
		CCHeaders *headers;
		CCScrollBar scrollBar;

		int m_width;
		int m_height;
		int m_iheight;
		int m_fheight;
		int m_theight;

		std::string  m_iconfile;
		std::string m_caption;
		////
		std::vector<char *> m_lines;
		unsigned int entries_per_page;
		unsigned int current_page;
		unsigned int pages;
		////
		int borderMode;
		fb_pixel_t borderColor;
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
		CHelpBox(const char* const Caption = NULL, const int Width = HELPBOX_WIDTH, const char * const Icon = NULL);
		virtual ~CHelpBox();

		////
		void addLine(const char *text);
		void addLine(const char *icon, const char *text);
		void addSeparator();
		void addPagebreak();
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};
		////
		int exec(int timeout = -1);
};

#endif

