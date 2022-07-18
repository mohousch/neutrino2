/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: infobox.h 2016.12.02 12:26:30 mohousch Exp $

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

#if !defined(INFOBOX_H)
#define INFOBOX_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <global.h>

#include <gui/widget/textbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/widget_helpers.h>

#include <driver/framebuffer.h>


class CInfoBox  
{
	private:
		CFrameBuffer * frameBuffer;

		// body
		CBox m_cBoxFrame;

		// head
		CBox m_cBoxFrameTitleRel;
		std::string m_cIcon;
		std::string m_cTitle;
		fb_pixel_t headColor;
		int headRadius;
		int headCorner;
		int headGradient;
		
		// text
		CBox m_cBoxFrameText;
		CTextBox *m_pcTextBox;
		unsigned int m_pcFontText;
		int m_nMode;
		int borderMode;
		
		// foot
		CBox m_cBoxFrameFootRel;
		fb_pixel_t footColor;
		int footRadius;
		int footCorner;
		int footGradient;
		
		//
		bool hide(void);
		void initVar(void);
		void initFramesRel(void);
		void refreshTitle(void);
		void refreshText(void);
		void refreshFoot(void);

		bool paint(void);
		void refresh(void);

		void scrollPageDown(const int pages);
		void scrollPageUp(const int pages);

		void setBigFonts();

	public:
		CInfoBox();
		CInfoBox(
				   const CBox* position, 
				   const char * title,
				   const char * icon = NEUTRINO_ICON_INFO);
		virtual ~CInfoBox();

		// head
		void setTitle(const char *title){m_cTitle = title;};
		void setIcon(const char *icon){m_cIcon = icon;};
		void setHeadColor(fb_pixel_t col) {headColor = col;};
		void setHeadCorner(int ra, int co){headRadius = ra; headCorner = co;};
		void setHeadGradient(int grad){headGradient = grad;};
		
		// text
		bool setText(const char * const newText, const char * const _thumbnail = NULL, int _tw = 0, int _th = 0, int tmode = PIC_RIGHT, bool enable_frame = false, const bool useBackground = false);
		void setMode(const int mode){m_nMode = mode;};
		void setBackgroundColor(fb_pixel_t col);
		void setTextColor(uint8_t col);
		void setFont(unsigned int font_text);
		void setBorderMode(int sm = BORDER_ALL){borderMode = sm;};
		
		// foot
		void setFootColor(fb_pixel_t col) {footColor = col;};
		void setFootCorner(int ra, int co){footRadius = ra; footCorner = co;};
		void setFootGradient(int grad){footGradient = grad;};

		//
		int exec(int timeout = -1);
};

//
void InfoBox(const char * const text, const char * const title, const char * const icon = NEUTRINO_ICON_INFO, const char * const thumbnail = NULL, int tw = 0, int th = 0, int tmode = PIC_RIGHT);

#endif
