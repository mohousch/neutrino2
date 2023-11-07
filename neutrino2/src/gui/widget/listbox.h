/*
	$Id: listbox.h 06.11.2023 mohousch Exp $

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

#if !defined(LISTBOX_H_)
#define LISTBOX_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/framebuffer.h>

#include <gui/widget/textbox.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/widget.h>

#include <gui/plugins.h>


////
class ClistBox;

// CChangeObserver
class CChangeObserver
{
	public:
		CChangeObserver(){}
		virtual ~CChangeObserver(){}
		
		virtual bool changeNotify(const std::string&, void *)
		{
			return false;
		}
};

// CMenuItem
class CMenuItem
{
	public:
		// type
		enum 
		{
			MENUITEM_OPTIONCHOOSER = 0,
			MENUITEM_OPTIONNUMBERCHOOSER,
			MENUITEM_OPTIONSTRINGCHOOSER,
			MENUITEM_SEPARATOR,
			MENUITEM_FORWARDER
		};
		
		// widgettype
		enum
		{
			TYPE_STANDARD = 0,
			TYPE_CLASSIC,
			TYPE_EXTENDED,
			TYPE_FRAME
		};
		
		// item state
		enum
		{
			ITEM_ACTIVE,
			ITEM_LOCKED,
			ITEM_HIDDEN,
			ITEM_MARKED,
			ITEM_INACTIVE
		};

	protected:
		int x, y, dx, dy;
		
		// pinprotection
		bool AlwaysAsk;
		char * validPIN;
		bool check();
		
	public:
		bool active;
		bool marked;
		bool hidden;
		bool locked;
		//
		int menuItem_type;
		//
		neutrino_msg_t directKey;
		neutrino_msg_t msg;
		bool can_arrow;
		std::string iconName;
		std::string itemName;
		std::string option;
		std::string optionInfo;
		std::string itemHint;
		std::string itemIcon;
		std::string info1, option_info1;
		std::string info2, option_info2;
		std::string icon1;
		std::string icon2;
		int number;
		unsigned int runningPercent;
		bool pb;
		
		//
		int widgetType;
		int widgetMode;
		bool isPlugin;
		bool paintIconName;

		//
		unsigned int nameFont;
		unsigned int optionFont;
		uint32_t optionFontColor;

		bool nLinesItem; // 2 lines Item 

		CMenuTarget *jumpTarget;
		std::string actionKey;
		
		//
		bool paintFrame;
		int borderMode;
		fb_pixel_t borderColor;
		int itemGradient;
		
		//
		ClistBox* parent;
		
		//
		fb_pixel_t* background;
		
		//
		CChangeObserver* observ;
		bool pulldown;
		
		//
		CMenuItem();
		virtual ~CMenuItem();

		//
		virtual void init(const int X, const int Y, const int DX, const int DY);
		virtual void paintItemBox(int dy, fb_pixel_t col);
		virtual void refreshItemBox(int dy, fb_pixel_t col);
		virtual int paint(bool selected = false, bool AfterPulldown = false) = 0;
		
		//
		virtual int getHeight(void) const = 0;
		virtual int getWidth(void) const
		{
			return 0;
		}

		virtual bool isSelectable(void) const {return false;}
		virtual int exec(CMenuTarget *) {return 0;}
		//
		virtual void setActive(const bool Active);
		virtual void setMarked(const bool Marked);
		virtual void setHidden(const bool Hidden);
		//
		virtual int getYPosition(void) const { return y; }
		virtual int getMenuItemType(){ return menuItem_type;};
		//
		virtual void setOption(const char* text){if (text) option = text;};
		virtual void setOptionInfo(const char* text){if (text) optionInfo = text;};
		virtual void setInfo1(const char* const text){if (text) info1 = text;};
		virtual void setInfo2(const char* const text){if (text) info2 = text;};
		virtual void setOptionInfo1(const char* const text){if (text) option_info1 = text;};
		virtual void setOptionInfo2(const char* const text){if (text) option_info2 = text;};
		//
		virtual void setHint(const char* const Text){if (Text) itemHint =  Text;};
		virtual void setHintIcon(const char* const icon){if (icon) itemIcon = icon;};
		//
		virtual void setIconName(const char* const icon){if (icon) iconName = icon;};
		//
		virtual void setIcon1(const char* const icon){if (icon)icon1 = icon;};
		virtual void setIcon2(const char* const icon){if (icon)icon2 = icon;};
		virtual void setNumber(int nr){number = nr;};
		virtual void setPercent(unsigned int percent){pb = true; runningPercent = percent;};
		//
		virtual void setNameFont(unsigned int font){nameFont = font;};
		virtual void setOptionFont(unsigned int font){optionFont = font;};
		virtual void setOptionFontColor(uint32_t c){optionFontColor = c;};
		//
		virtual void set2lines(bool p){nLinesItem = p;};
		virtual void setWidgetType(int type){widgetType = type;};
		virtual void setBorderMode(int m = CComponent::BORDER_ALL){borderMode = m;};
		virtual void setBorderColor(fb_pixel_t col){borderColor = col;};
		virtual void setGradient(int gr){itemGradient = gr;};
		//
		virtual void setDirectKey(neutrino_msg_t key){directKey = key;};
		virtual void setActionKey(CMenuTarget *Target, const char *const ActionKey){jumpTarget = Target; actionKey = ActionKey;};
		//
		virtual void setParent(ClistBox* p){parent = p;};
		//
		virtual void setChangeObserver(CChangeObserver* c){observ = c;};
		virtual void enablePullDown(){pulldown = true;};
		virtual void addOption(const char *opt, const int val = 0){};
		// locked
		virtual void setLocked(char *validpin = NULL){ locked = true; AlwaysAsk = true; validPIN = validpin? validpin : (char *)"";};
		// state : active/locked/hidden/marked/inactive
		virtual void setState(int state);
};

//// CMenuOptionChooser
typedef struct keyval
{
	int key;
	const char *valname;
} keyval_struct;

typedef std::vector<keyval_struct> keyval_list_t;

//
class CMenuOptionChooser : public CMenuItem
{
	protected:
		int height;
		int * optionValue;

		int getHeight(void) const
		{
			if (hidden)
				return 0;
			else
				return height;
		}
		
		bool isSelectable(void) const
		{
			return (active && !hidden);
		}

	private:
		keyval_list_t options;
		unsigned number_of_options;

	public:
		CMenuOptionChooser(const char* const Name, int * const OptionValue, const struct keyval * const Options = NULL, const unsigned Number_Of_Options = 0, const bool Active = false, CChangeObserver * const Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string& IconName= "", bool Pulldown = false);

		virtual ~CMenuOptionChooser(){options.clear();};

		//
		void addOption(const char *optionname, const int optionvalue);

		int paint(bool selected, bool AfterPulldown = false);

		int exec(CMenuTarget *target);
};

//// CMenuOptionNumberChooser
class CMenuOptionNumberChooser : public CMenuItem
{
	int lower_bound;
	int upper_bound;
	int display_offset;
	int localized_value;

	protected:
		int height;
		int* optionValue;

		int getHeight(void) const
		{
			if (hidden)
				return 0;
			else	
				return height;
		}
		
		bool isSelectable(void) const
		{
			return (active && !hidden);
		}

	public:
		CMenuOptionNumberChooser(const char * const Name, int * const OptionValue, const bool Active, const int min_value, const int max_value, CChangeObserver * const Observ = NULL, const int print_offset = 0, const int special_value = 0);

		virtual ~CMenuOptionNumberChooser(){};
		
		int paint(bool selected, bool AfterPulldown = false);

		int exec(CMenuTarget *target);
};

//// CMenuOptionStringChooser
class CMenuOptionStringChooser : public CMenuItem
{
	int height;
	char * optionValue;
	std::vector<std::string> options;

	public:
		CMenuOptionStringChooser(const char* const Name, char * OptionValue, bool Active = false, CChangeObserver* Observ = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName= "", bool Pulldown = false);
		
		virtual ~CMenuOptionStringChooser();

		void addOption(const char * value);

		int paint(bool selected, bool AfterPulldown = false);
		int getHeight(void) const {if (hidden) return 0; else return height;}

		bool isSelectable(void) const {return (active && !hidden);}

		int exec(CMenuTarget *target);
};

//// CMenuSeparator
class CMenuSeparator : public CMenuItem
{
	int type;
	fb_pixel_t color;
	bool gradient;
	
	public:
		enum
		{
			EMPTY =	0,
			LINE = 1,
			STRING = 2,
			ALIGN_CENTER = 4,
			ALIGN_LEFT = 8,
			ALIGN_RIGHT = 16
		};

	public:
		CMenuSeparator(const int Type = EMPTY, const char * const Text = NULL, const bool Gradient = false);
		virtual ~CMenuSeparator(){};

		//
		int paint(bool selected = false, bool AfterPulldown = false);
		int getHeight(void) const;
		int getWidth(void) const;
		
		//
		void setColor(fb_pixel_t col){ color = col;};

		//
		virtual const char * getString(void);
};

//// CMenuForwarder
class CMenuForwarder : public CMenuItem
{
	protected:
		virtual const char *getName(void);
		virtual const char *getOption(void);

	public:
		CMenuForwarder(const char * const Text, const bool Active = true, const char* const Option = NULL, CMenuTarget * Target = NULL, const char* const ActionKey = NULL, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char* const IconName = NULL, const char* const ItemIcon = NULL, const char* const Hint = NULL);
		
		virtual ~CMenuForwarder();
		
		int paint(bool selected = false, bool AfterPulldown = false);
		int getHeight(void) const;
		int getWidth(void) const;

		int exec(CMenuTarget *target);
		bool isSelectable(void) const {return (active && !hidden);};
		
		void setName(const char * const name){itemName = name;};
};

////
class ClistBox : public CComponent
{
	public:
		// listBox mode
		enum 
		{
			MODE_LISTBOX = 0,
			MODE_MENU,
			MODE_SETUP
		};
		
		enum 
		{
			FLAG_NONE,
			FLAG_PAINTICON,
			FLAG_ADJUSTLOCALE,
			FLAG_ADJUSTOPTION,
		};

		//
		std::vector<CMenuItem*> items;
		
	private:
		CFrameBuffer* frameBuffer;

		//		
		int wanted_height;
		int wanted_width;
		int start_x;
		int start_y;

		int selected;

		//
		std::vector<unsigned int> page_start;
		unsigned int current_page;
		unsigned int total_pages;
		int sb_width;
		int listmaxshow;
		int pos;
		unsigned int item_start_y;
		int items_height;
		int items_width;

		CCScrollBar scrollBar;

		//
		bool shrinkMenu;

		// frame
		int itemsPerX;
		int itemsPerY;
		int maxItemsPerPage;
		int item_height;
		int item_width;

		// widget type
		int widgetType;

		// head
		bool paintTitle;
		std::string iconfile;
		std::string l_name;
		int hheight;
		fb_pixel_t headColor;
		int headRadius;
		int headCorner;
		int headGradient;
		int headGradient_type;
		int headGradient_direction;
		int headGradient_intensity;
		int hbutton_count;
		button_label_list_t hbutton_labels;
		bool paintDate;
		int thalign;
		bool head_line;
		bool head_line_gradient;
		const char* format;
		CCTime* timer;

		// foot
		bool paint_Foot;
		int fheight;
		fb_pixel_t footColor;
		int footRadius;
		int footCorner;
		int footGradient;
		int footGradient_type;
		int footGradient_direction;
		int footGradient_intensity;
		int fbutton_count;
		int fbutton_width;
		button_label_list_t fbutton_labels;
		bool foot_line;
		bool foot_line_gradient;

		// itemInfo
		int cFrameFootInfoHeight;
		int footInfoHeight;
		CCItemInfo itemsLine;
		bool paintFootInfo;
		int footInfoMode;
		CBox itemInfoBox;
		bool iteminfoborder;
		bool iteminfosavescreen;
		int iteminfobordermode;
		bool iteminfoframe;
		unsigned int iteminfofont;
		uint32_t iteminfocolor;
		bool iteminfoscale;
		CCLabel label;

		// methods
		virtual void paintItems();

		//
		fb_pixel_t * background;
		void saveScreen();
		void restoreScreen();
		int full_height;
		int full_width;

		std::string actionKey; // lua
		
		// mainframe
		fb_pixel_t bgcolor;
		int radius;
		int corner;
		bool scrollbar;
		int gradient;
		//
		int widgetMode;
		// item
		int itemBorderMode;
		fb_pixel_t itemBorderColor;
		int itemGradient;
		bool item2Lines;
		bool paintIconName;
		
	public:
		ClistBox(const int x = 0, int const y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		ClistBox(CBox* position);
		virtual ~ClistBox();
		
		//
		bool isSelectable(void){return true;};

		////
		void addItem(CMenuItem * menuItem, const bool defaultselected = false);
		bool hasItem();
		void clearItems(void){items.clear(); current_page = 0;};
		void clear(void){hbutton_labels.clear(); fbutton_labels.clear(); current_page = 0; items.clear();};
		void setSelected(unsigned int _new) { selected = _new; };
		void integratePlugins(CPlugins::i_type_t integration = CPlugins::I_TYPE_DISABLED, const unsigned int shortcut = CRCInput::RC_nokey, bool enabled = true, int imode = MODE_MENU, int itype = CMenuItem::TYPE_STANDARD, bool i2lines = false, int iBorder = CComponent::BORDER_NO);
		////
		void initFrames(); // public
		void paintHead();
		void paintFoot();
		void paintItemInfo(int pos);
		void hideItemInfo();
		void paint();
		void hide();
		void refresh(){if (paintDate && paintTitle) timer->refresh();};
		bool update() const {return paintDate;};
		inline bool isPainted(void){return painted;};
		bool hasHead(){return paintTitle;};
		bool hasFoot(){return paint_Foot;};
		
		//// main properties
		void enableShrinkMenu(){shrinkMenu = true;};
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		void paintScrollBar(bool sb){scrollbar = sb;};
		void setGradient(int grad){ gradient = grad;};
		// frame method
		void setItemsPerPage(int itemsX = 6, int itemsY = 3){itemsPerX = itemsX; itemsPerY = itemsY; maxItemsPerPage = itemsPerX*itemsPerY;};
		//
		void setWidgetType(int type){widgetType = type;};
		void setWidgetMode(int mode){widgetMode = mode;};
		
		//// item properties
		void setItemBorderMode(int m = CComponent::BORDER_ALL){itemBorderMode = m;};
		void setItemBorderColor(fb_pixel_t col){itemBorderColor = col;};
		void setItemGradient(int gr = NOGRADIENT){itemGradient = gr;};
		void setItem2Lines(){item2Lines = true; initFrames();};
		void disablePaintIconName(void) { paintIconName = false;};
		
		//// itemInfo properties
		void enablePaintItemInfo(int fh = 0){paintFootInfo = true; footInfoHeight = fh;};
		void setItemInfoMode(int mode){footInfoMode = mode;};
		void setItemInfoPos(int x, int y, int dx, int dy)
		{
			if ( (footInfoMode == CCItemInfo::ITEMINFO_HINTICON) || (footInfoMode == CCItemInfo::ITEMINFO_ICON) || (footInfoMode == CCItemInfo::ITEMINFO_HINT))
			{
				itemInfoBox.iX = x; 
				itemInfoBox.iY = y; 
				itemInfoBox.iWidth = dx; 
				itemInfoBox.iHeight = dy; 
				footInfoHeight = 0;
			}
		};
		void paintItemInfoBorder(int m){iteminfoborder = true; iteminfobordermode = m;};
		void enableItemInfoSaveScreen(){iteminfosavescreen = true;};
		void paintItemInfoFrame(bool p){iteminfoframe = p;};
		void setItemInfoFont(unsigned int f){iteminfofont = f;};
		void setItemInfoColor(uint32_t col){iteminfocolor = col;};
		void setItemInfoScaling(bool s){iteminfoscale = s;};
		
		//// head properties
		void enablePaintHead(){paintTitle = true;};
		void enablePaintDate(void){paintDate = true;};
		void setTitle(const char* title = "", const char* icon = NULL){if (title) l_name = title; if(icon) iconfile = icon;};
		void setTitleHAlign(const int m){thalign = m;};
		void setHeadButtons(const struct button_label *_hbutton_label, const int _hbutton_count = 1);
		void setHeadColor(fb_pixel_t col) {headColor = col;};
		void setHeadCorner(int ra, int co = CORNER_TOP){headRadius = ra; headCorner = co;};
		void setHeadGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){headGradient = grad; headGradient_direction = direction; headGradient_intensity = intensity; headGradient_type = type;};
		void setHeadLine(bool l, bool g = false){head_line = l; head_line_gradient = g;};
		void setFormat(const char* f){format = f;};
		
		//// foot properties
		void enablePaintFoot(){paint_Foot = true;};
		void setFootButtons(const struct button_label *_fbutton_label, const int _fbutton_count = 1, const int _fbutton_width = 0);
		void setFootColor(fb_pixel_t col) {footColor = col;};
		void setFootCorner(int ra, int co = CORNER_BOTTOM){footRadius = ra; footCorner = co;};
		void setFootGradient(int grad, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int type = GRADIENT_COLOR2TRANSPARENT){footGradient = grad; footGradient_direction = direction; footGradient_intensity = intensity; footGradient_type = type;};
		void setFootLine(bool l, bool g = false){foot_line = l; foot_line_gradient = g;};

		//// events / actionKey methodes
		void scrollLineDown(const int lines = 1);
		void scrollLineUp(const int lines = 1);
		void scrollPageDown(const int pages = 1);
		void scrollPageUp(const int pages = 1);
		int swipLeft();
		int swipRight();
		//
		int oKKeyPressed(CMenuTarget* target, neutrino_msg_t _msg = CRCInput::RC_ok);
		void homeKeyPressed(){selected = -1;};
		int directKeyPressed(neutrino_msg_t _msg);

		//// get methods
		int getItemsCount()const{return items.size();};
		int getCurrentPage()const{return current_page;};
		int getTotalPages()const{return total_pages;};
		int getSelected(){return selected;};
		int getTitleHeight(){return hheight;};
		int getFootHeight(){return fheight;};
		int getItemHeight(){return item_height;};
		int getFootInfoHeight(){return footInfoHeight;};
		int getListMaxShow(void) const {return listmaxshow;};
		int getPageStart(int page) const {return page_start[page];};
		std::string getItemName(){if (hasItem()) return items[selected]->itemName; else return "";};
		std::string getItemHint(){if (hasItem()) return items[selected]->itemHint; else return "";};
		std::string getItemIcon(){if (hasItem()) return items[selected]->itemIcon; else return "";};
		CBox getWindowsPos(void){initFrames(); return (itemBox);};
		// frame type methods
		int getItemsPerX()const{return itemsPerX;};
		int getItemsPerY()const{return itemsPerY;};
		int getMaxItemsPerPage()const{return maxItemsPerPage;};
		// widget type/mode/pos
		int getWidgetType(){return widgetType;};
};

#endif // LISTBOX_H_

