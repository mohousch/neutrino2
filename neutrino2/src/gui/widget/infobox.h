//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: infobox.h 10022025 mohousch Exp $
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

#if !defined(INFOBOX_H)
#define INFOBOX_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <global.h>

#include <gui/widget/icons.h>
#include <gui/widget/textbox.h>
#include <gui/widget/component.h>

#include <driver/gdi/framebuffer.h>


class CInfoBox  
{
	private:
		CFrameBuffer * frameBuffer;

		// widget
		CBox cFrameBox;
		CWidget * widget;
		
		// head
		CBox cFrameBoxTitleRel;
		CCHeaders *headers;
		std::string m_cIcon;
		std::string m_cTitle;
		fb_pixel_t headColor;
		int headRadius;
		int headCorner;
		int headGradient;
		
		// text
		CBox cFrameBoxText;
		CTextBox *textBox;
		unsigned int m_pcFontText;
		int m_nMode;
		int borderMode;
		
		// foot
		CBox cFrameBoxFootRel;
		CCFooters *footers;
		fb_pixel_t footColor;
		int footRadius;
		int footCorner;
		int footGradient;

	public:
		CInfoBox();
		CInfoBox(const int x, const int y, const int dx, const int dy, const char * title = NULL, const char * icon = NEUTRINO_ICON_INFO);
		CInfoBox(const CBox* position, const char * title = NULL, const char * icon = NEUTRINO_ICON_INFO);
		virtual ~CInfoBox();
		
		////
		void initVar(void);
		void initFrames(void);
		void refreshTitle(void);
		void refreshFoot(void);
		void paint(void);
		void hide(void);
		
		////
		void setBigFonts();

		//// head properties
		void setTitle(const char *title){m_cTitle = title;};
		void setIcon(const char *icon){m_cIcon = icon;};
		void setHeadColor(fb_pixel_t col) {headColor = col;};
		void setHeadCorner(int ra, int co){headRadius = ra; headCorner = co;};
		void setHeadGradient(int grad){headGradient = grad;};
		
		//// text properties
		void setText(const char * const newText, const char * const _thumbnail = NULL, int _tw = 0, int _th = 0, int tmode = CTextBox::PIC_RIGHT, bool enable_frame = false);
		void setMode(const int mode){m_nMode = mode;};
		void setBackgroundColor(fb_pixel_t col);
		void setTextColor(uint32_t col);
		void setFont(unsigned int font_text);
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		
		//// foot properties
		void setFootColor(fb_pixel_t col) {footColor = col;};
		void setFootCorner(int ra, int co){footRadius = ra; footCorner = co;};
		void setFootGradient(int grad){footGradient = grad;};

		//
		int exec(int timeout = -1);
		
		////
		void scrollPageDown(const int pages);
		void scrollPageUp(const int pages);
};

//
void InfoBox(const char * const title, const char * const text, const char * const icon = NEUTRINO_ICON_INFO, const char * const thumbnail = NULL, int tw = 0, int th = 0, int tmode = CTextBox::PIC_RIGHT);

#endif

