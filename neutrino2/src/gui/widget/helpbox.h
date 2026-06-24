//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: helpbox.h 24062026 mohousch Exp $
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

#include <gui/widget/listbox.h>


////
class CHelpBox
{
	protected:
		CBox cFrameBox;
		CWidget *widget;
		ClistBox *listBox;
		CMenuItem *item;
		////
		int m_width;
		int m_height;
		////
		std::string m_caption;
		std::string  m_iconfile;
		////
		int borderMode;
		fb_pixel_t borderColor;
		////
		void refreshPage();
		void init();
		void initFrames(void);
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

