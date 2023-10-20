/*
 * $Id: widget_helpers.h 20.10.2023 mohousch Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __gui_widget_helpers_h__
#define __gui_widget_helpers_h__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <OpenThreads/Thread>

#include <driver/fontrenderer.h>
#include <driver/framebuffer.h>
#include <driver/color.h>

#include <system/localize.h>
#include <system/settings.h>
#include <system/debug.h>

#include <driver/rcinput.h>


extern CFont * g_Font[FONT_TYPE_COUNT];
//
class CMenuTarget;
class CWidget;

// dimension helper
class CBox
{
	public:
		// Variables
		int iX;
		int iY;
		int iWidth;
		int iHeight;

		//
		inline CBox()
		{
			iX = 0;
			iY = 0;
			iWidth = 0;
			iHeight = 0;
		};
		
		inline CBox( const int _iX, const int _iY, const int _iWidth, const int _iHeight)
		{
			iX =_iX; 
			iY=_iY; 
			iWidth =_iWidth; 
			iHeight =_iHeight;
		};
		
		inline ~CBox(){};
};

//// CComponent
class CComponent
{
	public:
		// cc_type
		enum 
		{
			CC_ICON,
			CC_IMAGE,
			CC_LABEL,
			CC_TEXT,
			CC_BUTTON,
			CC_HLINE,
			CC_VLINE,
			CC_FRAMELINE,
			CC_PIG,
			CC_GRID,
			CC_TIME,
			CC_COUNTER,
			CC_SPINNER,
			CC_WINDOW,
			//// not to be added with addCCItem method.
			CC_SCROLLBAR,
			CC_PROGRESSBAR,
			CC_ITEMINFO,
			CC_SLIDER,
			////
			CC_HEAD,
			CC_FOOT,
			CC_LISTBOX,
			CC_FRAMEBOX,
			CC_LISTFRAME,
			CC_TEXTBOX
		};
		
		// border
		enum {
			BORDER_NO,
			BORDER_ALL,
			BORDER_LEFTRIGHT,
			BORDER_TOPBOTTOM
		};

		// halign
		enum {
			CC_ALIGN_LEFT,
			CC_ALIGN_CENTER,
			CC_ALIGN_RIGHT
		};
	
	public:
		//
		int cc_type;
		std:: string cc_name;
		
		//
		CBox itemBox;
		int halign;
		
		//
		bool rePaint;
		bool savescreen;
		bool paintframe;
		bool inFocus;
		bool painted;
		////
		CWidget* parent;
		//
		struct keyAction { std::string action; CMenuTarget *menue; };
		std::map<neutrino_msg_t, keyAction> keyActionMap;
		uint32_t sec_timer_id;
		uint64_t sec_timer_interval;
		std::string actionKey; // lua
		
		//
		CComponent();
		virtual ~CComponent(){};
		
		//
		virtual bool isSelectable(void){return false;};
		virtual bool hasItem(){return false;};
		//
		virtual void paint(void){};
		virtual void hide(void){};
		virtual void enableRepaint(){rePaint = true;};
		virtual bool update() const {return rePaint;};
		virtual void refresh(void){};
		virtual void blink(bool){};
		virtual void stopRefresh(){};
		virtual inline bool isPainted(void){return painted;};
		virtual void clear(void){};
		
		//
		virtual int getCCType(){return cc_type;};
		virtual std::string getCCName(){return cc_name;};
		
		//
		virtual void setPosition(const int _x, const int _y, const int _width, const int _height)
		{
			dprintf(DEBUG_INFO, "CComponent::setPosition: x:%d y:%d dx:%d dy:%d\n", _x, _y, _width, _height);
			
			itemBox.iX = _x;
			itemBox.iY = _y;
			itemBox.iWidth = _width;
			itemBox.iHeight = _height;
		};
		
		virtual void setPosition(const CBox * position)
		{
			itemBox = *position;
		};
		
		//
		virtual inline CBox getWindowsPos(void){return itemBox;};
		
		//
		virtual void initFrames(){};
		virtual void setIcon(const char* const icon){};
		virtual void setHint(const char * const hint){};
		virtual void setImage(const char* const image){};
		virtual void setScaling(bool s){};
		virtual void setColor(uint32_t col){};
		virtual void setButtons(const struct button_label *button_label, const int button_count = 1){};
		virtual void setFont(unsigned int f){};
		virtual void setText(const char* text){};
		virtual void paintMainFrame(bool p){paintframe = p;};
		virtual void setHAlign(int h){halign = h;};
		virtual void setMode(int m){};
		virtual void useBackground(void){};
		virtual void setFormat(const char* f){};
		virtual void setInterFrame(int iframe = 15){};
		virtual void setTotalTime(time_t tot_time){};
		virtual void setPlayTime(time_t p_time){};
		//
		virtual void saveScreen(void){};
		virtual void restoreScreen(void){};
		virtual void enableSaveScreen(void){savescreen = true;};
		////
		virtual void scrollLineDown(const int lines = 1){};
		virtual void scrollLineUp(const int lines = 1){};
		virtual void scrollPageDown(const int pages = 1){};
		virtual void scrollPageUp(const int pages = 1){};
		virtual int swipLeft(){return 0;};
		virtual int swipRight(){return 0;};
		//
		virtual void setOutFocus(bool focus = true){inFocus = !focus;};
		virtual void setSelected(unsigned int _new) {};
		//
		virtual int oKKeyPressed(CMenuTarget* target, neutrino_msg_t _msg = CRCInput::RC_ok){return 0;};
		virtual void homeKeyPressed(){};
		virtual int directKeyPressed(neutrino_msg_t ){return 0;};
		//
		virtual std::string getActionKey(void){ return actionKey;}; // lua
		virtual int getSelected(void){return -1;};
		//
		virtual void setParent(CWidget* p){parent = p;};
		//
		virtual void addKey(neutrino_msg_t key, CMenuTarget *menue = NULL, const std::string &action = "");
		void setSecTimerInterval(uint64_t sec){sec_timer_interval = sec;}; // in sec
		virtual bool onButtonPress(neutrino_msg_t msg, neutrino_msg_data_t data);
		virtual void exec(int timeout = -1); // in sec
};

typedef std::vector<CComponent*> CCITEMLIST;

//// window
class CCWindow : public CComponent
{
	private:
		CFrameBuffer* frameBuffer;
		//
		int radius;
		int corner;
		fb_pixel_t bgcolor;
		int gradient;
		int grad_direction;
		int grad_intensity;
		int grad_type;
		//
		fb_pixel_t * background;
		//
		int borderMode;
		fb_pixel_t borderColor;
		//
		unsigned int current_page;
		unsigned int total_pages;
		//
		void initVars(void);
		void initFrames();
		void saveScreen();
		void restoreScreen();
		void paintPage(void);
		
	public:
		CCWindow(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_XRES);
		CCWindow(CBox* position);
		virtual ~CCWindow();
		//
		void setPosition(const int x, const int y, const int dx, const int dy);
		void setPosition(CBox* position);
		//
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		void setGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){gradient = grad; grad_direction = direction; grad_intensity = intensity; grad_type = type;};
		void setBorderMode(int sm){borderMode = sm;};
		void setBorderColor(fb_pixel_t col){borderColor = col;};
		//
		void paint(void);
		void hide(void);
		//
		void refresh(void);
};

//// CCIcon
class CCIcon : public CComponent
{
	public:
		//		
		int width;
		int height;
		std::string iconName;
		
	private:
		CFrameBuffer* frameBuffer;
		//
		fb_pixel_t* background;

	public:
		CCIcon(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCIcon()
		{
			if (background)
			{
				delete [] background; 
				background = NULL;
			}
		};
		
		//
		void setIcon(const char* const icon);
		//
		void paint();
		void hide();
		//
		void saveScreen(void);
		void restoreScreen(void);
		void blink(bool show);
};

//// CCImage
class CCImage : public CComponent
{
	public:
		//
		int iWidth;
		int iHeight;
		int iNbp;
		std::string imageName;
		
	private:
		CFrameBuffer* frameBuffer;
		//
		bool scale;

	public:
		CCImage(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCImage(){};
		//
		void setImage(const char* const image);
		void setScaling(bool s){scale = s;};
		//
		void paint();
};

//// CCButtons
typedef struct button_label
{
	std::string button;
	std::string localename;
	fb_pixel_t color;
} button_label_struct;

typedef std::vector<button_label_struct> button_label_list_t;

class CCButtons : public CComponent
{
	private:
		button_label_list_t buttons;
		unsigned int count;
		bool head;
		int mode;

	public:
		enum {
			BUTTON_ICON,
			BUTTON_FRAME,
			BUTTON_COLOREDFRAME,
			BUTTON_COLOREDLINE
		};

		//
		CFrameBuffer* frameBuffer;
		
		//
		CCButtons(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCButtons(){};
		
		//
		void setButtonMode(int m){mode = m;};
		//
		void setButtons(const struct button_label *button_label, const int button_count = 1, bool _head = false);
		void addButton(const char *btn, const char *lname = NULL, const fb_pixel_t col = 0);
		//
		void paint();
		//
		void clear(){buttons.clear();};
};

//// CHline
class CCHline : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		fb_pixel_t color;
		int gradient;
	
		//
		CCHline(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCHline(){};
		
		//
		void setColor(fb_pixel_t col){color = col;};
		void setGradient(int gr){gradient = gr;};
		//
		void paint();
};

//// CVline
class CCVline : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		fb_pixel_t color;
		int gradient;
		
		//
		CCVline(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCVline(){};
		
		//
		void setColor(fb_pixel_t col){color = col;};
		void setGradient(int gr){gradient = gr;};
		
		//
		void paint();
};

//// CFrameline
class CCFrameLine : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		fb_pixel_t color;
		
		//
		CCFrameLine(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCFrameLine(){};
		
		//
		void setColor(fb_pixel_t col){color = col;};
		
		//
		void paint();
};

//// CLabel
class CCLabel : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint32_t color;
		unsigned int font;
		std::string label;
		
		//
		fb_pixel_t* background;
		
		//
		CCLabel(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCLabel();
		
		//
		void setColor(uint32_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setText(const char* const text){label = text? text : "";};
		//
		void saveScreen(void);
		void restoreScreen(void);
		void enableSaveScreen();
		//
		void paint();
		void hide();
};

//// CCText
class CCText : public CComponent
{
	private:
		int emptyLineCount;
		int medlineheight;
		int medlinecount;
		//
		void addTextToArray(const std::string & text );
		void processTextToArray(std::string text);
		
	public:
		CFrameBuffer* frameBuffer;
		
		//
		unsigned int font;
		std::vector<std::string> Text;
		uint32_t color;
		//
		fb_pixel_t* background;
		
		//
		CCText(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCText();
		
		//
		void setFont(unsigned int f){font = f;};
		void setColor(uint32_t col){color = col;};
		void setText(const char* const text){processTextToArray(text);};
		//
		void enableSaveScreen();
		void saveScreen(void);
		void restoreScreen(void);
		//
		void paint();
		void hide();
};

//// CCGrid
class CCGrid : public CComponent
{
	private:
		//
		fb_pixel_t rgb;
		int inter_frame;

	public:
		CFrameBuffer* frameBuffer;
		
		//
		CCGrid(const int x = 0, const int y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		CCGrid(CBox* position);
		virtual ~CCGrid(){};

		//
		void init();
		void setColor(fb_pixel_t col){rgb = col;};
		void setInterFrame(int iframe = 15){inter_frame = iframe;};
		//
		void paint();
		void hide();
};

//// CCPig
class CCPig : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		CCPig(const int x = 0, const int y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		CCPig(CBox* position);
		virtual ~CCPig(){};

		//
		void init();
		//
		void paint();
		void hide();
};

//// CCTime
class CCTime : public CComponent/*, public OpenThreads::Thread*/
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint32_t color;
		unsigned int font;
		std::string format;
		//
		fb_pixel_t* background;
		void saveScreen(void);
		void restoreScreen(void);
		
		//
		CCTime(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCTime();
		
		//
		void setColor(uint32_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setFormat(const char* const f);
		//
		void paint();
		void hide();
		//
		void refresh();
		
		////
		//bool started;
		//void run(void);
		//void Start();
		//void Stop();
};

//// CCCounter
class CCCounter : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint32_t color;
		unsigned int font;
		time_t total_time;
		time_t play_time;
		
		fb_pixel_t* background;
		void saveScreen(void);
		void restoreScreen(void);
		
		//
		CCCounter(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCCounter();
		
		//
		void setColor(uint32_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setTotalTime(time_t tot_time){total_time = tot_time;};
		void setPlayTime(time_t p_time){play_time = p_time;};
		//
		void paint();
		void hide();
		//
		void refresh();
		//
		CFont* getFont(){return g_Font[font];};
};

//// CCSpinner
class CCSpinner : public CComponent/*, public OpenThreads::Thread*/
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		std::string filename;
		int count;
		fb_pixel_t* background;
		void saveScreen(void);
		void restoreScreen(void);
		
		//
		CCSpinner(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCSpinner();
		
		//
		void paint();
		void hide();
		//
		void refresh();
		
		////
		//bool started;
		//void run();
};

//// CScrollBar
class CScrollBar : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		CScrollBar(){frameBuffer = CFrameBuffer::getInstance(); cc_type = CC_SCROLLBAR;};
		virtual ~CScrollBar(){};

		// currentPage start with 0
		void paint(const int x, const int y, const int dy, const int NrOfPages, const int CurrentPage);
		void paint(CBox* position, const int NrOfPages, const int CurrentPage);
};

//// CProgressBar
class CProgressBar : public CComponent
{
	private:
		unsigned char percent;
		short red, green, yellow;
		bool inverse;
		uint32_t rgb;
		double div;

	public:
		CFrameBuffer* frameBuffer;
		
		//
		CProgressBar(int x, int y, int w, int h, int r = 40, int g = 100, int b = 70, bool inv = true);
		CProgressBar(const CBox* psoition, int r = 40, int g = 100, int b = 70, bool inv = true);
		
		virtual ~CProgressBar(){};
		
		//
		void paint();
		void refresh(unsigned char pcr);
		void reset();
		int getPercent() { return percent; };
		//
		void setColor(uint32_t c){rgb = c;};
};

//// detailsLine
class CItemInfo : public CComponent
{
	public:
		enum 
		{
			ITEMINFO_INFO,
			ITEMINFO_HINTITEM,
			ITEMINFO_HINTICON,
			ITEMINFO_ICON,
			ITEMINFO_HINT
		};

		//
		CFrameBuffer* frameBuffer;
		
		//
		std::string info1, option_info1;
		std::string info2, option_info2;
		std::string hint;
		std::string icon;
		int mode;
		
		// cutom mode
		unsigned int tFont;
		int borderMode;
		uint32_t color;
		bool scale;
		
		//
		fb_pixel_t *background;
		
		//
		CItemInfo();
		virtual ~CItemInfo();
		
		//
		void paint();
		void hide();
		//
		void setMode(int m){mode = m;};
		void setInfo1(const char* const text){if (text) info1 = text;};
		void setInfo2(const char* const text){if (text)  info2 = text;};
		void setOptionInfo1(const char* const text){if (text) option_info1 = text;};
		void setOptionInfo2(const char* const text){if (text) option_info2 = text;};
		void setHint(const char* const Text){if (Text) hint =  Text;};
		void setIcon(const char* const ic){if (ic) icon = ic;};
		// custom mode
		void setFont(unsigned int f){tFont = f;};
		void setBorderMode(int m){borderMode = m;};
		void setColor(uint32_t col){color = col;};
		void setScaling(bool s){scale = s;};
		//
		void saveScreen(void);
		void restoreScreen(void);
		void enableSaveScreen();
};

//// CCSlider
class CCSlider : public CComponent
{
	private:
		int value;
		int max_value;
		int min_value;
		int step;
		
	public:
		//
		CFrameBuffer *frameBuffer;
		
		//
		CCSlider(const int x, const int y, const int dx, const int dy);
		virtual ~CCSlider(){};
		
		//
		void paint();
		//
		void paintSlider(const int _x, const int _y, const unsigned int spos, const char* const text, const char * const iconname);
		int swipRight();
		int swipLeft();
		//
		void setMaxValue(int val){max_value = val;};
		void setMinValue(int val){min_value = val;};
		void setStepValue(int val){step = val;};
};

//// CHeaders
class CHeaders : public CComponent
{
	private:
		CFrameBuffer* frameBuffer;
		
		//
		fb_pixel_t color;
		int radius;
		int corner;
		int gradient;
		int grad_direction;
		int grad_intensity;
		int grad_type;
		bool line;
		bool line_gradient;
		//
		int count;
		button_label_list_t buttons;
		int mode;
		//
		std::string htitle;
		std::string hicon;
		bool paintDate;
		std::string format;
		CCTime* timer;
	
	public:
		CHeaders(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_XRES, const char * const title = NULL, const char * const icon = NULL);
		CHeaders(CBox* position, const char * const title = NULL, const char * const icon = NULL);
		virtual ~CHeaders();

		//
		void setTitle(const char * const title){htitle.clear(); if (title) htitle = title;};
		void setIcon(const char * const icon){hicon.clear(); if (icon) hicon = icon;};
		//void setHAlign(const int m){halign = m;};
		void setColor(fb_pixel_t col){color = col;};
		void setGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){gradient = grad; grad_direction = direction; grad_intensity = intensity; grad_type = type;};
		void setCorner(int ra, int co = CORNER_TOP){radius = ra; corner = co;};
		void enablePaintDate(void){paintDate = true;};
		void setFormat(const char* f){if (f) format.clear(); format = f;};
		void setLine(bool l, bool g = false){line = l; line_gradient = g;};
		//
		void setButtons(const struct button_label* _button_labels, const int _count = 1);
		void addButton(const char *btn, const char *lname = NULL, const fb_pixel_t col = 0);
		void setButtonMode(int m){mode = m;};
		//
		void paint();
		void hide();
		void refresh(void){if (paintDate) timer->refresh();};
		bool update() const {return paintDate;};
		void stopRefresh();
		//
		void clear(){buttons.clear();};
};

//// CFooters
class CFooters : public CComponent
{
	private:
		CFrameBuffer* frameBuffer;
		
		//
		fb_pixel_t color;
		int radius;
		int corner;
		int gradient;
		int grad_direction;
		int grad_intensity;
		int grad_type;
		bool line;
		bool line_gradient;
		//
		unsigned int count;
		int button_width;
		button_label_list_t buttons;
		int mode;
	
	public:
		CFooters(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_XRES);
		CFooters(CBox* position);
		virtual ~CFooters();
		
		//
		void setColor(fb_pixel_t col){color = col;};
		void setGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){gradient = grad; grad_direction = direction; grad_intensity = intensity; grad_type = type;};
		void setCorner(int ra, int co = CORNER_BOTTOM){radius = ra; corner = co;};
		void setLine(bool l, bool g = false){line = l; line_gradient = g;};
		//
		void setButtons(const struct button_label *_button_labels, const int _count = 1, const int _button_width = 0);
		void addButton(const char *btn, const char *lname = NULL, const fb_pixel_t col = 0);
		void setButtonMode(int m){mode = m;};
		//
		void paint();
		void hide();
		//
		void clear(){buttons.clear();};
};

#endif /* __gui_widget_helpers_h__ */

