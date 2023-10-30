/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: messagebox.h 2013/10/12 mohousch Exp $

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


#ifndef __messagebox__
#define __messagebox__

#include <stdint.h>
#include <string>

#include <driver/framebuffer.h>

#include <system/localize.h>
#include <system/settings.h>

#include <gui/widget/drawable.h>
#include <gui/widget/icons.h>
#include <gui/widget/widget_helpers.h>


////
class CMessageBox
{
	public:
		enum result_
		{
			mbrYes    = 0,
			mbrNo     = 1,
			mbrCancel = 2,
			mbrBack   = 3,
			mbrOk     = 4
		} result;
			
		enum buttons_
		{
			mbYes	 = 0x01,
			mbNo 	 = 0x02,
			mbCancel = 0x04,
			mbAll    = 0x07,
			mbBack   = 0x08,
			mbOk     = 0x10,
			mbNone   = 0x20
		} buttons;

	protected:
		CBox cFrameBox;
		CWidget *m_cBoxWindow;
		CCHeaders *headers;
		CCScrollBar scrollBar;

		unsigned int m_currentPage;
		std::vector<int>m_startEntryOfPage;
		int m_maxEntriesPerPage;
		int m_pages;

		int m_width;
		int m_height;
		int m_iheight;
		int m_fheight;
		int m_theight;

		std::string m_caption;
		char * m_message;
		ContentLines m_lines;
		std::string  m_iconfile;
		
		void refreshPage();

		void init(const char* const Caption, const int Width, const char * const Icon);
		void initFrames(void);

		bool has_scrollbar(void);
		void scroll_up(void);
		void scroll_down(void);

		void paint(void);
		void hide(void);
		
		//
		int borderMode;
		fb_pixel_t borderColor;

	private:
		uint32_t showbuttons;
		bool returnDefaultOnTimeout;

		void paintButtons();

	public:
		//
		CMessageBox(const char * const Caption, ContentLines& Lines, const int Width = MESSAGEBOX_WIDTH, const char * const Icon = NULL, const result_ Default = mbrYes, const uint32_t ShowButtons = mbAll);
	
		// Text & Caption are always UTF-8 encoded
		CMessageBox(const char * const Caption, const char * const Text, const int Width = MESSAGEBOX_WIDTH, const char * const Icon = NULL, const result_ Default = mbrYes, const uint32_t ShowButtons = mbAll);

		virtual ~CMessageBox(void);
		
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};

		int exec(int timeout = -1);
		void returnDefaultValueOnTimeout(bool returnDefault);
};

int MessageBox(const char * const Caption, const char * const Text, const CMessageBox::result_ Default, const uint32_t ShowButtons, const char * const Icon = NULL, const int Width = MENU_WIDTH, const int timeout = -1, bool returnDefaultOnTimeout = false, const int border = CComponent::BORDER_NO); // UTF-8

#endif

