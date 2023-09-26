/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: hintbox.h 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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


#ifndef __hintbox__
#define __hintbox__

#include <string>
#include <vector>

#include <gui/widget/window.h>

#include <system/localize.h>
#include <system/settings.h>

#include <gui/widget/icons.h>
#include <gui/widget/widget_helpers.h>


////
class CHintBox
{
	protected:
		CBox cFrameBox;
		CBox cFrameBoxTitle;
		CBox cFrameBoxItem;
		
		CWidget *widget;
		CHeaders *headers;

		unsigned int entries_per_page;
		unsigned int current_page;

		std::string caption;
		char * message;
		std::vector<char *>line;
		std::string iconfile;

		CScrollBar scrollBar;
		
		void refreshPage(void);
		
		// hourglass
		bool paintHG;
		uint32_t sec_timer_id;
		CCSpinner * spinner;
		
		//
		int borderMode;
		fb_pixel_t borderColor;

	public:
		//
		CHintBox(const char * Caption, const char * const Text, const int Width = HINTBOX_WIDTH, const char * const Icon = NEUTRINO_ICON_INFO);
		virtual ~CHintBox(void);

		bool has_scrollbar(void);
		void scroll_up(void);
		void scroll_down(void);

		void paint(void);
		void hide(void);
		
		void enablePaintHG(void){paintHG = true;};
		void setBorderMode(int sm = BORDER_ALL){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};

		//
		int exec(int timeout = -1);
};

////
int HintBox(const char * const Caption, const char * const Text, const int Width = HINTBOX_WIDTH, int timeout = -1, const char * const Icon = NEUTRINO_ICON_INFO, const int border = BORDER_NO);

#endif

