/*
	$Id: framebox.h 09.02.2019 mohousch Exp $


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

#if !defined(_FRAMEBOX_H_)
#define _FRAMEBOX_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>

#include <gui/widget/window.h>
#include <gui/widget/widget.h>


class CFrameBox;

enum {
	FRAME_BOX = 0, // caption, option and left icon | optionInfo and 2lines
	FRAME_PICTURE,
	FRAME_ICON,
	FRAME_TEXT,
	FRAME_LABEL,
	FRAME_PLUGIN,
	FRAME_HLINE,
	FRAME_VLINE,
	FRAME_PIG,
	//FRAME_TIME,
	//FRAME_SLIDER
};

class CFrame
{
	public:
		CWindow window;
		
		int halign;
		int valign;
		
		std::string iconName;
		std::string caption;
		std::string option;

		unsigned int captionFont;
		unsigned int optionFont;

		CMenuTarget* jumpTarget;
		std::string actionKey;
		neutrino_msg_t directKey;
		neutrino_msg_t msg;
		
		//
		bool active;
		bool marked;

		int mode;	//frame mode
		bool border;
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
		CFrame();
		virtual ~CFrame(){};

		int paint(bool selected = false, bool AfterPulldown = false);
		
		virtual void setMode(int m = FRAME_BOX);
		int getMode(void){return mode;};
		
		virtual void setTitle(const char *text){if (text != NULL) caption = text;};
		virtual void setIconName(const char *icon){ if (icon != NULL) iconName = icon;};
		virtual void setOption(const char *text){if (text != NULL) option = text;};
		virtual void setPlugin(const char * const pluginName);
		virtual void showPluginName(){pluginOrigName = true;};
		
		virtual void setHAlign(int h){halign = h;};

		//
		virtual void setActionKey(CMenuTarget *Target, const char *const ActionKey){jumpTarget = Target; actionKey = ActionKey;};
		virtual void setDirectKey(neutrino_msg_t key){directKey = key;};
		
		//
		virtual void setCaptionFont(unsigned int font){captionFont = font;};
		virtual void setOptionFont(unsigned int font){optionFont = font;}; 

		int exec(CMenuTarget *parent);
		
		//
		virtual void setActive(const bool Active);
		virtual void setMarked(const bool Marked);

		virtual bool isSelectable(void)
		{
			return active;				
		}

		virtual void enableBorder(void){border = true;};
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
class CFrameBox : public CWidgetItem
{
	private:
		CFrameBuffer* frameBuffer;

		int selected;
		int pos;

		std::vector<CFrame*> frames;

		virtual void paintFrames();

		std::string actionKey;
		
		//
		bool paintFrame;
		fb_pixel_t bgcolor;
		int radius;
		int corner;
		
		//
		fb_pixel_t * background;
		bool savescreen;
		void saveScreen();
		void restoreScreen();
		
		// head
		std::string iconfile;
		std::string l_name;
		int hheight;
		fb_pixel_t headColor;
		int headRadius;
		int headCorner;
		int headGradient;
		int hbutton_count;
		button_label_list_t hbutton_labels;
		bool paintDate;
		bool logo;
		bool paintTitle;
		
		// foot
		int fheight;
		fb_pixel_t footColor;
		int footRadius;
		int footCorner;
		int footGradient;
		int fbutton_count;
		int fbutton_width;
		button_label_list_t fbutton_labels;
		bool paint_Foot;

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

		virtual void addFrame(CFrame *frame, const bool defaultselected = false);
		bool hasItem();
		void clearFrames(void){frames.clear();};
		void setSelected(unsigned int _new) { /*if(_new <= frames.size())*/ selected = _new; };

		virtual void initFrames();
		virtual void paint();
		virtual void hide();

		virtual void swipRight();
		virtual void swipLeft();
		virtual void scrollLineDown(const int lines = 1);
		virtual void scrollLineUp(const int lines = 1);

		int getSelected(){return selected;};
		
		//
		void paintMainFrame(bool p){paintFrame = p;};
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		//
		void enableSaveScreen();

		//
		bool isSelectable(void);

		int oKKeyPressed(CMenuTarget* _parent, neutrino_msg_t _msg = RC_ok);
		void homeKeyPressed(){selected = -1;};

		// lua compatibility
		std::string getActionKey(void){return actionKey;};
		
		// head
		void enablePaintHead(){paintTitle = true;};
		virtual void paintHead();
		void enablePaintDate(void){paintDate = true;};
		void setTitle(const char* title = "", const char* icon = NULL, bool logo_ok = false){l_name = title; if(icon != NULL) iconfile = icon; logo = logo_ok;};
		void setHeadButtons(const struct button_label *_hbutton_label, const int _hbutton_count = 1);
		void setHeadColor(fb_pixel_t col) {headColor = col;};
		void setHeadRadius(int ra){headRadius = ra;};
		void setHeadCorner(int co){headCorner = co;};
		void setHeadGradient(int grad){headGradient = grad;};
		
		// foot
		void enablePaintFoot(){paint_Foot = true;};
		virtual void paintFoot();
		void setFootButtons(const struct button_label *_fbutton_label, const int _fbutton_count = 1, const int _fbutton_width = 0);
		void setFootColor(fb_pixel_t col) {footColor = col;};
		void setFootRadius(int ra){footRadius = ra;};
		void setFootCorner(int co){footCorner = co;};
		void setFootGradient(int grad){footGradient = grad;};
};

#endif
