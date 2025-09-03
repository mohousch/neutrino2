//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: framebox.h 21122024 mohousch Exp $
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

#if !defined(_FRAMEBOX_H_)
#define _FRAMEBOX_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/fontrenderer.h>

#include <gui/widget/widget.h>


class CFrameBox;

class CFrameItem
{
	public:
		enum {
			FRAME_BOX = 0, // caption, option and left icon | optionInfo and 2lines
			FRAME_PICTURE,
			FRAME_ICON,
			FRAME_TEXT,
			FRAME_LABEL,
			FRAME_PLUGIN,
			FRAME_HLINE,
			FRAME_VLINE,
			FRAME_PIG
		};

		CCWindow window;
		
		int halign;
		int valign;
		
		std::string iconName;
		std::string caption;
		std::string option;

		unsigned int captionFont;
		unsigned int optionFont;

		CWidgetTarget *jumpTarget;
		std::string actionKey;
		neutrino_msg_t directKey;
		neutrino_msg_t msg;
		
		//
		bool active;
		bool marked;

		int mode;	//frame mode
		int borderMode;
		fb_pixel_t borderColor;
		bool paintFrame;
		bool pluginOrigName;
		
		//
		fb_pixel_t fcolor;
		int radius;
		int corner;
		int gradient;
		
		//
		CFrameBox* parent;
		
		//
		CFrameItem();
		virtual ~CFrameItem(){};

		int paint(bool selected = false);
		
		virtual void setMode(int m = FRAME_BOX);
		int getMode(void){return mode;};
		
		virtual void setTitle(const char *text){if (text != NULL) caption = text;};
		virtual void setIconName(const char *icon){ if (icon != NULL) iconName = icon;};
		virtual void setOption(const char *text){if (text != NULL) option = text;};
		virtual void setPlugin(const char * const pluginName);
		virtual void showPluginName(){pluginOrigName = true;};
		virtual void setHAlign(int h){halign = h;};
		//
		virtual void setActionKey(CWidgetTarget *Target, const char *const ActionKey){jumpTarget = Target; actionKey = ActionKey;};
		virtual void setDirectKey(neutrino_msg_t key){directKey = key;};
		//
		virtual void setCaptionFont(unsigned int font){captionFont = font;};
		virtual void setOptionFont(unsigned int font){optionFont = font;}; 
		//
		int exec(CWidgetTarget *target);
		//
		virtual void setActive(const bool Active);
		virtual void setMarked(const bool Marked);
		//
		virtual bool isSelectable(void)
		{
			return active;				
		}
		//
		virtual void setBorderMode(int b = CComponent::BORDER_ALL){borderMode =b;};
		virtual void setBorderColor(fb_pixel_t col){borderColor = col;};
		virtual void paintMainFrame(bool p){paintFrame = p;};
		virtual void setPosition(int x, int y, int dx, int dy){window.setPosition(x, y, dx, dy);};
		virtual void setPosition(CBox *position){window.setPosition(position);};
		//
		virtual void setColor(fb_pixel_t col) {fcolor = col;};
		virtual void setCorner(int ra, int co){radius = ra; corner = co;};
		virtual void setGradient(int grad){gradient = grad;};
		//
		virtual void setParent(CFrameBox* p){parent = p;};
};

//// CFrameBox
class CFrameBox : public CComponent
{
	private:
		CFrameBuffer* frameBuffer;

		int selected;
		int pos;

		std::vector<CFrameItem*> frames;
		
		//
		fb_pixel_t bgcolor;
		int radius;
		int corner;
		fb_pixel_t * background;
		void saveScreen();
		void restoreScreen();
		int borderMode;
		uint32_t borderColor;
		
		//
		unsigned int current_page;
		unsigned int total_pages;
		
		virtual void paintFrames();

	public:
		CFrameBox(const int x = 0, int const y = 0, const int dx = 0, const int dy = 0);
		CFrameBox(CBox* position);
		virtual ~CFrameBox();

		void setPosition(const int x, const int y, const int dx, const int dy)
		{
			itemBox.iX = x;
			itemBox.iY = y;
			itemBox.iWidth = dx;
			itemBox.iHeight = dy;

			initFrames();
		};
		void setPosition(CBox* position){itemBox = *position; initFrames();};
		//
		bool isSelectable(void);
		//// 
		void addFrame(CFrameItem *frame, const bool defaultselected = false);
		bool hasItem();
		void clearFrames(void){frames.clear();};
		void setSelected(unsigned int _new) { /*if(_new <= frames.size())*/ selected = _new; };
		//// 
		void initFrames();
		void paint(bool _selected = false);
		void hide();
		//// main properties
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		void setBorderColor(uint32_t col){borderColor = col;};
		//// events
		int swipRight();
		int swipLeft();
		void scrollLineDown(const int lines = 1);
		void scrollLineUp(const int lines = 1);
		int oKKeyPressed(CWidgetTarget *target, neutrino_msg_t _msg = CRCInput::RC_ok);
		void homeKeyPressed(){selected = -1;};
		//// get methods
		int getSelected(){return selected;};
};

#endif

