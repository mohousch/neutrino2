/*
	$Id: widget.h 11.03.2020 mohousch Exp $

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
#if !defined(WIDGET_H_)
#define WIDGET_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/widget/widget_helpers.h>


////
class CMenuTarget
{
	protected:
		std::string valueString;
	public:
		enum
		{
			RETURN_NONE		= 0,
			RETURN_REPAINT 		= 1,
			RETURN_EXIT 		= 2,
			RETURN_EXIT_ALL 	= 4
		};
				
	public:
		CMenuTarget(){valueString = "";};
		virtual ~CMenuTarget(){valueString.clear();};
		virtual void hide(){valueString.clear();};
		virtual int exec(CMenuTarget* parent, const std::string& actionKey) = 0;
		
		//
		virtual std::string& getValueString() {return valueString;};
		virtual void setValueString(const char *const value){valueString = value;};
		virtual void clearValueString(void){valueString.clear();};
};

////
class CWidget : public CMenuTarget
{
	public:
		enum 
		{
			MENU_POSITION_NONE = 0,
			MENU_POSITION_LEFT = 1,
			MENU_POSITION_CENTER = 2,
			MENU_POSITION_RIGHT = 3
		};

		//
		std::string name;
			
	protected:
		CFrameBuffer *frameBuffer;
		CBox mainFrameBox;
		// 
		CCITEMLIST CCItems;
		//
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;
		int selected;
		bool exit_pressed;
		int retval;
		int pos;
		struct keyAction { std::string action; CMenuTarget *menue; };
		std::map<neutrino_msg_t, keyAction> keyActionMap;
		uint64_t timeout;
		uint32_t sec_timer_id;
		uint64_t sec_timer_interval;
		//
		std::string actionKey; // for lua
		// screen
		fb_pixel_t * background;
		bool savescreen;
		void saveScreen();
		void restoreScreen();
		//
		int menu_position;
		// mainframe		
		bool paintframe;
		fb_pixel_t backgroundColor;
		int gradient;
		int grad_direction;
		int grad_intensity;
		int grad_type;
		int radius;
		int corner;
		int borderMode;
		fb_pixel_t borderColor;
		//
		unsigned int current_page;
		unsigned int total_pages;
		//
		void initFrames();

	public:
		CWidget(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_YRES);
		CWidget(CBox *position);
		virtual ~CWidget();
		
		//
		virtual void setPosition(const int x, const int y, const int dx, const int dy)
		{
			mainFrameBox.iX = x;
			mainFrameBox.iY = y;
			mainFrameBox.iWidth = dx;
			mainFrameBox.iHeight = dy;
		};
		virtual void setPosition(CBox* position){mainFrameBox = *position;};

		// CCITEMS
		virtual void addCCItem(CComponent* CCItem, const bool defaultselected = false);
		bool hasCCItem(){return !CCItems.empty();};
		virtual void clearCCItems(){CCItems.clear();};
		virtual void paintCCItems();
		virtual void removeCCItem(long pos);
		//
		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget *parent, const std::string &actionKey);
		virtual void refresh(bool show = false);
		//
		void setTimeOut(uint64_t to = 0){timeout = to;};
		void setSecTimerInterval(uint64_t sec){sec_timer_interval = sec;};
		//
		void addKey(neutrino_msg_t key, CMenuTarget *menue = NULL, const std::string &action = "");
		//
		void paintMainFrame(bool p){paintframe = p;};
		void setColor(fb_pixel_t col) {backgroundColor = col;};
		void setGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){gradient = grad; grad_direction = direction; grad_intensity = intensity; grad_type = type;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		void setBorderMode(int sm = CComponent::BORDER_ALL){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};
		//
		void enableSaveScreen();
		void setMenuPosition(int p){menu_position = p; initFrames();};
		
		//// events
		virtual void onOKKeyPressed();
		virtual void onHomeKeyPressed();
		virtual void onUpKeyPressed();
		virtual void onDownKeyPressed();
		virtual void onRightKeyPressed();
		virtual void onLeftKeyPressed();
		virtual void onPageUpKeyPressed();
		virtual void onPageDownKeyPressed();
		virtual void onYellowKeyPressed();
		virtual void onDirectKeyPressed(neutrino_msg_t _msg);
		
		//
		void setSelected(unsigned int _new) {selected = _new; if (selected < 0) selected = 0;};
		
		////
		std::string getWidgetName(){return name;};
		//
		CComponent* getCCItem(const int type, const std::string& name = "");
		//
		inline CBox getWindowsPos(void){return mainFrameBox;};
		int getMenuPosition(){return menu_position;};
		// lua compatibility
		std::string getActionKey(){return actionKey;};
		neutrino_msg_t getKey(){return msg;};
		bool getExitPressed(){return exit_pressed;};
		//
		CCITEMLIST getCCItems(){return CCItems;};
		int getCCItemsCount(){return CCItems.size();};
		//
		int getSelected(){return exit_pressed ? -1 : selected;};
};

#endif // WIDGET_H_

