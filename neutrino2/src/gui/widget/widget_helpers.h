/*
 * $Id: widget_helpers.h 2016/01/12 mohousch Exp $
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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

#include <driver/fontrenderer.h>
#include <driver/framebuffer.h>

#include <system/localize.h>
#include <system/settings.h>
#include <system/debug.h>

#include <driver/rcinput.h>


extern CFont * g_Font[FONT_TYPE_COUNT];
//
class CMenuTarget;
class CWidgetItem;
class CWidget;
class CWindow;

// border
enum {
	BORDER_NO,
	BORDER_ALL,
	BORDER_LEFTRIGHT,
	BORDER_TOPBOTTOM
};

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
		inline CBox(){};
		inline CBox( const int _iX, const int _iY, const int _iWidth, const int _iHeight){iX =_iX; iY=_iY; iWidth =_iWidth; iHeight =_iHeight;};
		~CBox(){};
};

//
typedef struct button_label
{
	std::string button;
	std::string localename;
} button_label_struct;

typedef std::vector<button_label_struct> button_label_list_t;

// CComponent
enum {
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
	// not to be added with addCCItem method.
	CC_SCROLLBAR,
	CC_PROGRESSBAR,
	CC_DETAILSLINE,
	CC_SLIDER,
};

// halign
enum {
	CC_ALIGN_LEFT,
	CC_ALIGN_CENTER,
	CC_ALIGN_RIGHT
};

class CComponent
{
	public:
		//
		int cc_type;
		std:: string cc_name;
		
		//
		CBox cCBox;
		int halign;
		
		//
		bool rePaint;
		
		//
		CComponent();
		virtual ~CComponent(){};
		
		virtual bool isSelectable(void){return false;};
		
		//
		virtual void paint(void){};
		virtual void hide(void){};
		virtual void refresh(void){};
		virtual void enableRepaint(){rePaint = true;};
		virtual bool update() const {return rePaint;};
		
		//
		virtual int getCCType(){return cc_type;};
		virtual std::string getCCName(){return cc_name;};
		
		//
		virtual void setPosition(const int _x, const int _y, const int _width, const int _height)
		{
			dprintf(DEBUG_INFO, "CComponent::setPosition: x:%d y:%d dx:%d dy:%d\n", _x, _y, _width, _height);
			
			cCBox.iX = _x;
			cCBox.iY = _y;
			cCBox.iWidth = _width;
			cCBox.iHeight = _height;
		};
		
		virtual void setPosition(const CBox * position)
		{
			cCBox = *position;
		};
		
		//
		virtual inline CBox getWindowsPos(void){return (cCBox);};
		
		//
		virtual void init(){};
		virtual void setIcon(const char* const icon){};
		virtual void setImage(const char* const image){};
		virtual void setScaling(bool s){};
		virtual void paintMainFrame(bool p){};
		virtual void setColor(uint32_t col){};
		virtual void setButtons(const struct button_label *button_label, const int button_count = 1){};
		virtual void setFont(unsigned int f){};
		virtual void setText(const char* text){};
		virtual void enablePaintBG(){};
		virtual void setHAlign(int h){};
		//virtual void setFont(CFont* f){font = f;};
		virtual void setMode(int m){};
		//void setColor(uint8_t c){color = c;};
		//void setText(const char* const text){Text = text;};
		virtual void useBackground(void){};
		//void setColor(uint8_t col){color = col;};
		//void setFont(CFont *f){font = f;};
		virtual void setFormat(const char* f){};
		//void setHAlign(int h){halign = h;};
		virtual void setInterFrame(int iframe = 15){};
		virtual void setTotalTime(time_t tot_time){};
		virtual void setPlayTime(time_t p_time){};
};

typedef std::vector<CComponent*> CCITEMLIST;

class CCIcon : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//		
		int iWidth;
		int iHeight;
		
		//
		std::string iconName;

		CCIcon(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCIcon(){};
		
		//
		void setIcon(const char* const icon);

		// h/v aligned
		void paint();
};

class CCImage : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		int iWidth;
		int iHeight;
		int iNbp;
		
		//
		std::string imageName;
		bool scale;
		uint32_t color;
		bool paintframe;

		CCImage(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCImage(){};
		
		//
		void setImage(const char* const image);
		void setScaling(bool s){scale = s;};
		void paintMainFrame(bool p){paintframe = p;};
		void setColor(uint32_t col){color = col;};
		
		// h/v aligned
		void paint();
};

// buttons
enum {
	BUTTON_BUTTON,
	BUTTON_LINE_RIGHT,
	BUTTON_LINE_BOTTOM,
	BUTTON_FRAME_COLORED,
	BUTTON_FRAME_BORDER
};

// CButtons
class CCButtons : public CComponent
{
	private:
		button_label_list_t buttons;
		unsigned int count;
		int mode;

	public:
		CFrameBuffer* frameBuffer;
		
		//
		CCButtons(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCButtons(){};
		
		//
		void setButtons(const struct button_label *button_label, const int button_count = 1);
		
		//
		void paint();

		//
		void paintFootButtons(const int x, const int y, const int dx, const int dy, const unsigned int count, const struct button_label* const content);
		void paintHeadButtons(const int x, const int y, const int dx, const int dy, const unsigned int count, const struct button_label * const content);
};

//CScrollBar
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

// CProgressBar
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
		void paint(unsigned char pcr, bool paintBG = true);
		void reset();
		int getPercent() { return percent; };
		
		//
		void setColor(uint32_t c){rgb = c;};
};

// detailsLine
enum {
	DL_INFO,
	DL_HINT,
	DL_HINTITEM,
	DL_HINTICON,
	DL_HINTHINT
};

class CItems2DetailsLine : public CComponent
{
	public:
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
		bool savescreen;
		bool paintframe;
		uint32_t color;
		bool scale;
		
		//
		CItems2DetailsLine();
		virtual ~CItems2DetailsLine();
		
		//
		void paint(int x, int y, int width, int height, int info_height = 0, int iheight = 0, int iy = 0);
		void clear(int x, int y, int width, int height, int info_height);
		
		//
		virtual void setMode(int m){mode = m;};
		virtual void setInfo1(const char* const text){if (text) info1 = text;};
		virtual void setInfo2(const char* const text){if (text)  info2 = text;};
		virtual void setOptionInfo1(const char* const text){if (text) option_info1 = text;};
		virtual void setOptionInfo2(const char* const text){if (text) option_info2 = text;};
		virtual void setHint(const char* const Text){if (Text) hint =  Text;};
		virtual void setIcon(const char* const ic){if (ic) icon = ic;};
		
		// custom mode
		void setFont(unsigned int f){tFont = f;};
		void setBorderMode(int m){borderMode = m;};
		void enableSaveScreen(){savescreen = true;};
		void paintFrame(bool p){paintframe = p;};
		void setColor(uint32_t col){color = col;};
		void setScaling(bool s){scale = s;};
};

// CHline
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

// CVline
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

// CFrameline
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

// CLabel
class CCLabel : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint8_t color;
		unsigned int font;
		std::string label;
		bool paintBG;
		
		//
		bool savescreen;
		uint32_t* background;
		
		//
		CCLabel(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0, bool save = false);
		virtual ~CCLabel();
		
		//
		void setColor(uint8_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setText(const char* const text){label = text? text : "";};
		void enablePaintBG(){paintBG = true;};
		void setHAlign(int h){halign = h;};
		
		//
		void paint();
};

//CText
class CCText : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		unsigned int font;
		int mode;
		std::string Text;
		uint8_t color;
		bool useBG;
		
		//
		CCText(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCText(){};
		
		//
		void setFont(unsigned int f){font = f;};
		void setMode(int m){mode = m;};
		void setColor(uint8_t c){color = c;};
		void setText(const char* const text){Text = text? text : "";};
		void useBackground(void){useBG = true;};
		
		//
		void paint();
};

//
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
		void setPosition(const int x, const int y, const int dx, const int dy);
		void setPosition(CBox* position);
		void setColor(fb_pixel_t col){rgb = col;};
		void setInterFrame(int iframe = 15){inter_frame = iframe;};

		void paint();
		void hide();
};

// pig
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
		void setPosition(const int x, const int y, const int dx, const int dy);
		void setPosition(CBox* position);

		void paint();
		void hide();
};

// CCTime
class CCTime : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint8_t color;
		unsigned int font;
		std::string format;
		
		fb_pixel_t* background;
		
		//
		CCTime(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCTime(){delete [] background; background = NULL;};
		
		//
		void setColor(uint8_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setFormat(const char* const f);
		void setHAlign(int h){halign = h;};
		
		//
		void paint();
		void hide(){delete [] background; background = NULL;};
		
		//
		void refresh();
};

// CCCounter
class CCCounter : public CComponent
{
	public:
		CFrameBuffer* frameBuffer;
		
		//
		uint8_t color;
		unsigned int font;
		time_t total_time;
		time_t play_time;
		
		fb_pixel_t* background;
		
		//
		CCCounter(const int x = 0, const int y = 0, const int dx = 0, const int dy = 0);
		virtual ~CCCounter(){delete [] background; background = NULL;};
		
		//
		void setColor(uint8_t col){color = col;};
		void setFont(unsigned int f){font = f;};
		void setHAlign(int h){halign = h;};
		void setTotalTime(time_t tot_time){total_time = tot_time;};
		void setPlayTime(time_t p_time){play_time = p_time;};
		
		//
		void paint();
		void hide(){delete [] background; background = NULL;};
		
		//
		void refresh();
		
		//
		CFont* getFont(){return g_Font[font];};
};

// CWidgetItem
enum {
	WIDGETITEM_WINDOW,
	WIDGETITEM_HEAD,
	WIDGETITEM_FOOT,
	WIDGETITEM_LISTBOX,
	WIDGETITEM_FRAMEBOX,
	WIDGETITEM_LISTFRAME,
	WIDGETITEM_TEXTBOX
};

class CWidgetItem
{
	public:
		//
		CBox itemBox;

		//
		int widgetItem_type;
		std::string widgetItem_name;
		
		//
		bool inFocus;
		bool rePaint;
		std::string actionKey; // lua
		
		//
		bool painted;
		
		//
		CWidget* parent;
		
		//
		struct keyAction { std::string action; CMenuTarget *menue; };
		std::map<neutrino_msg_t, keyAction> keyActionMap;
		
		uint32_t sec_timer_id;

		//
		CWidgetItem();
		virtual ~CWidgetItem(){};
		
		//
		virtual int getWidgetItemType(){return widgetItem_type;};
		virtual std::string getWidgetItemName(){return widgetItem_name;};

		virtual bool isSelectable(void){return false;}
		virtual bool hasItem(){return false;};

		//
		virtual void paint(void){painted = true;};
		virtual void hide(void){painted = false;};
		virtual void refresh(void){};
		virtual void enableRepaint(){rePaint = true;};
		virtual bool update() const {return rePaint;};

		//
		virtual void scrollLineDown(const int lines = 1){};
		virtual void scrollLineUp(const int lines = 1){};
		virtual void scrollPageDown(const int pages = 1){};
		virtual void scrollPageUp(const int pages = 1){};
		virtual void swipLeft(){};
		virtual void swipRight(){};

		//
		virtual void setOutFocus(bool focus = true){inFocus = !focus;};
		virtual void setSelected(unsigned int _new) {};

		//
		virtual void setPosition(const int x, const int y, const int dx, const int dy)
		{
			itemBox.iX = x;
			itemBox.iY = y;
			itemBox.iWidth = dx;
			itemBox.iHeight = dy;
		};
		virtual void setPosition(CBox* position){itemBox = *position;};
		virtual inline CBox getWindowsPos(void){return (itemBox);};

		//
		virtual int getWidgetType(){return (4);};

		//
		virtual int oKKeyPressed(CMenuTarget* _parent, neutrino_msg_t _msg = RC_ok){return 0;};
		virtual void homeKeyPressed(){};
		virtual void directKeyPressed(neutrino_msg_t _msg){};
		
		//
		virtual std::string getActionKey(void){ return actionKey;}; // lua
		
		//
		virtual void setParent(CWidget* p){parent = p;};
		//
		virtual void addKey(neutrino_msg_t key, CMenuTarget *menue = NULL, const std::string &action = "");
		virtual void setSecTimer(uint32_t sec){sec_timer_id = sec;};
		
		//
		virtual bool onButtonPress(neutrino_msg_t msg, neutrino_msg_data_t data);
		
		//
		virtual inline bool isPainted(void){return painted;};
};

typedef std::vector<CWidgetItem*> WIDGETITEMLIST;

// CHeaders
class CHeaders : public CWidgetItem
{
	private:
		CFrameBuffer* frameBuffer;
		
		//
		bool paintFrame;
		fb_pixel_t bgcolor;
		int radius;
		int corner;
		int gradient;
		bool head_line;
		bool head_line_gradient;

		int hbutton_count;
		button_label_list_t hbutton_labels;
		
		std::string htitle;
		std::string hicon;
		int thalign;
		
		bool paintDate;
		std::string format;
		CCTime* timer;
	
	public:
		CHeaders(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_XRES, const char * const title = NULL, const char * const icon = NULL);
		CHeaders(CBox* position, const char * const title = NULL, const char * const icon = NULL);
		virtual ~CHeaders(){};
		
		//
		void paintMainFrame(bool p){paintFrame = p;};

		//
		void setTitle(const char * const title){htitle.clear(); if (title) htitle = title;};
		void setIcon(const char * const icon){hicon.clear(); if (icon) hicon = icon;};
		void setHAlign(const int m){thalign = m;};
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setGradient(const int grad){gradient = grad;};
		void setRadius(const int ra){radius = ra;};
		void setCorner(const int co){corner = co;};
		void enablePaintDate(void){paintDate = true;};
		void setFormat(const char* f){if (f) format.clear(); format = f;};
		void setHeadLine(bool l, bool g = false){head_line = l; head_line_gradient = g;};
		
		//
		void setButtons(const struct button_label* _hbutton_labels, const int _hbutton_count = 1);

		//
		void paint();
		void hide();
		void refresh(void){if (paintDate) timer->refresh();};
		bool update() const {return paintDate;};
		
		//
		void clear(){hbutton_labels.clear();};
};

// CFooters
class CFooters : public CWidgetItem
{
	private:
		CFrameBuffer* frameBuffer;
		
		//
		unsigned int fcount;
		int fbutton_width;
		button_label_list_t fbuttons;
		
		bool paintFrame;

		fb_pixel_t fbgcolor;
		int fradius;
		int fcorner;
		int fgradient;
		bool foot_line;
		bool foot_line_gradient;
	
	public:
		CFooters(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_XRES);
		CFooters(CBox* position);
		virtual ~CFooters(){};
		
		//
		void paintMainFrame(bool p){paintFrame = p;};

		void setColor(fb_pixel_t col){fbgcolor = col;};
		void setGradient(const int grad){fgradient = grad;};
		void setRadius(const int ra){fradius = ra;};
		void setCorner(const int co){fcorner = co;};
		void setFootLine(bool l, bool g = false){foot_line = l; foot_line_gradient = g;};
		
		//
		void setButtons(const struct button_label *button_label, const int button_count = 1, const int _fbutton_width = 0);
		
		//
		void paint();
		void hide();
		
		//
		void clear(){fbuttons.clear();};
};

#endif /* __gui_widget_helpers_h__ */
