/*
	$Id: listbox.h 2018.08.19 mohousch Exp $


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


class ClistBox;

// item type
enum 
{
	MENUITEM_OPTION_CHOOSER = 0,
	MENUITEM_OPTION_NUMBER_CHOOSER,
	MENUITEM_OPTION_STRING_CHOOSER,
	MENUITEM_OPTION_LANGUAGE_CHOOSER,
	MENUITEM_SEPARATOR,
	MENUITEM_FORWARDER,
	MENUITEM_LOCKED_FORWARDER,
	//
	MENUITEM_LISTBOXITEM,
	MENUITEM_PLUGINITEM
};

enum
{
	TYPE_STANDARD = 0,
	TYPE_CLASSIC,
	TYPE_EXTENDED,
	TYPE_FRAME
};

enum 
{
	MODE_LISTBOX = 0,
	MODE_MENU,
	MODE_SETUP
};

enum
{
	ITEMINFO_INFO_MODE = 0,
	ITEMINFO_HINT_MODE,
	ITEMINFO_HINTITEM_MODE,
	ITEMINFO_HINTICON_MODE,
	ITEMINFO_HINTHINT_MODE
};

// line separator
enum
{
	EMPTY =	0,
	LINE = 1,
	STRING = 2,
	ALIGN_CENTER = 4,
	ALIGN_LEFT = 8,
	ALIGN_RIGHT = 16
};

typedef struct keyval
{
	int key;
	const char* valname;
} keyval_struct;

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
	protected:
		int x, y, dx, offx;
		
	public:
		bool active;
		bool marked;
		
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
		int menuItem_type;
		
		//
		int widgetType;
		int widgetMode;
		bool isPlugin;
		
		// HACK for TYPE_FRAME
		int item_height;
		int item_width;

		//
		unsigned int nameFont;
		unsigned int optionFont;
		uint8_t optionFontColor;

		bool nLinesItem; // 2 lines Item 

		CMenuTarget *jumpTarget;
		std::string actionKey;
		
		//
		bool paintFrame;
		int borderMode;
		int itemGradient;
		
		//
		ClistBox* parent;
		
		//
		fb_pixel_t* background;
		
		//
		CChangeObserver* observ;
		bool pulldown;

		CMenuItem();
		virtual ~CMenuItem(){};

		virtual void init(const int X, const int Y, const int DX, const int OFFX);
		virtual int paint(bool selected = false, bool AfterPulldown = false) = 0;
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

		//
		virtual int getYPosition(void) const { return y; }
		virtual int getMenuItemType(){ return menuItem_type;};

		//
		virtual void setOption(const char* text){if (text) option = text;};
		virtual void setOptionInfo(const char* text){if (text) optionInfo = text;};

		//
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

		virtual void setNameFont(unsigned int font){nameFont = font;};
		virtual void setOptionFont(unsigned int font){optionFont = font;};
		virtual void setOptionFontColor(uint8_t c){optionFontColor = c;};

		virtual void set2lines(void){nLinesItem = true;};
		virtual void setWidgetType(int type){widgetType = type;};
		virtual void setBorderMode(int m = BORDER_ALL){borderMode = m;};
		virtual void setGradient(int gr){itemGradient = gr;};

		//
		virtual void setDirectKey(neutrino_msg_t key){directKey = key;};
		virtual void setActionKey(CMenuTarget *Target, const char *const ActionKey){jumpTarget = Target; actionKey = ActionKey;};
		
		//
		virtual void setParent(ClistBox* p){parent = p;};
		
		//
		virtual void setChangeObserver(CChangeObserver* c){observ = c;};
		virtual void enablePullDown(){pulldown = true;};
};

// CMenuOptionChooser
class CMenuOptionChooser : public CMenuItem
{
	protected:
		int height;
		int * optionValue;

		int getHeight(void) const
		{
			return height;
		}
		
		bool isSelectable(void) const
		{
			return active;
		}

	private:
		const struct keyval* options;
		unsigned number_of_options;
		CChangeObserver* observ;
		
		std::string optionNameString;
		
		bool pulldown;

	public:
		CMenuOptionChooser(const char* const OptionName, int * const OptionValue, const struct keyval * const Options, const unsigned Number_Of_Options, const bool Active = false, CChangeObserver * const Observ = NULL, const neutrino_msg_t DirectKey = RC_nokey, const std::string& IconName= "", bool Pulldown = false);

		~CMenuOptionChooser(){};

		void setOptionValue(const int newvalue);
		int getOptionValue(void) const;

		int paint(bool selected, bool AfterPulldown = false);

		int exec(CMenuTarget* target);
};

// CMenuOptionNumberChooser
class CMenuOptionNumberChooser : public CMenuItem
{
	const char* optionString;

	int lower_bound;
	int upper_bound;

	int display_offset;

	int localized_value;
	
	std::string nameString;

	protected:
		int height;
		int* optionValue;

		int getHeight(void) const
		{
			return height;
		}
		
		bool isSelectable(void) const
		{
			return active;
		}

	private:
		CChangeObserver * observ;

	public:
		CMenuOptionNumberChooser(const char * const Name, int * const OptionValue, const bool Active, const int min_value, const int max_value, CChangeObserver * const Observ = NULL, const int print_offset = 0, const int special_value = 0, const char * non_localized_name = NULL);

		~CMenuOptionNumberChooser(){};
		
		int paint(bool selected, bool AfterPulldown = false);

		int exec(CMenuTarget *target);
};

// CMenuOptionStringChooser
class CMenuOptionStringChooser : public CMenuItem
{
	std::string nameString;
	int height;
	char * optionValue;
	std::vector<std::string> options;
	CChangeObserver * observ;
	bool pulldown;

	public:
		CMenuOptionStringChooser(const char* const Name, char * OptionValue, bool Active = false, CChangeObserver* Observ = NULL, const neutrino_msg_t DirectKey = RC_nokey, const std::string & IconName= "", bool Pulldown = false);
		
		~CMenuOptionStringChooser();

		void addOption(const char * value);

		int paint(bool selected, bool AfterPulldown = false);
		int getHeight(void) const {return height;}

		bool isSelectable(void) const {return active;}

		int exec(CMenuTarget* target);
};

// CMenuSeparator
class CMenuSeparator : public CMenuItem
{
	int type;

	public:
		const char * textString;

		CMenuSeparator(const int Type = EMPTY, const char * const Text = NULL);
		~CMenuSeparator(){};

		int paint(bool selected = false, bool AfterPulldown = false);
		int getHeight(void) const;
		int getWidth(void) const;

		virtual const char * getString(void);
};

// CPINProtection
class CPINProtection
{
	protected:
		char * validPIN;
		bool check();
		virtual CMenuTarget * getParent() = 0;
	public:
		CPINProtection( char *validpin){ validPIN = validpin;};
		virtual ~CPINProtection(){}
};

// CZapProtection
class CZapProtection : public CPINProtection
{
	protected:
		virtual CMenuTarget * getParent() { return( NULL);};
	public:
		int fsk;

		CZapProtection(char * validpin, int FSK) : CPINProtection(validpin){ fsk = FSK; };
		~CZapProtection(){};
		bool check();
};

// CMenulistBoxItem
class ClistBoxItem : public CMenuItem
{
	std::string optionValueString;

	protected:
		//
		std::string textString;

		//
		virtual const char *getName(void);
		virtual const char *getOption(void);

	public:
		ClistBoxItem(const char* const Text, const bool Active = true, const char* const Option = NULL, CMenuTarget * Target = NULL, const char* const ActionKey = NULL, const neutrino_msg_t DirectKey = RC_nokey, const char* const IconName = NULL, const char* const ItemIcon = NULL, const char* const Hint = NULL);
		
		~ClistBoxItem(){};
		
		int paint(bool selected = false, bool AfterPulldown = false);
		int getHeight(void) const;
		int getWidth(void) const;

		int exec(CMenuTarget* target);
		bool isSelectable(void) const {return active;};
		
		void setName(const char * const name){textString = name;};
};

// CLockedlistBoxItem
class CLockedlistBoxItem : public ClistBoxItem, public CPINProtection
{
	CMenuTarget * Parent;
	bool AlwaysAsk;

	protected:
		virtual CMenuTarget* getParent(){ return Parent;};
		
	public:   
		CLockedlistBoxItem(const char * const Text, char * _validPIN, bool alwaysAsk = false, const bool Active = true, char * Option = NULL, CMenuTarget * Target = NULL, const char * const ActionKey = NULL, neutrino_msg_t DirectKey = RC_nokey, const char * const IconName = NULL, const char * const ItemIcon = NULL, const char* const Hint = NULL)
		 : ClistBoxItem(Text, Active, Option, Target, ActionKey, DirectKey, IconName, ItemIcon, Hint),
		   CPINProtection( _validPIN){AlwaysAsk = alwaysAsk;};

		virtual int exec(CMenuTarget* target);
};

//
class ClistBox : public CWidgetItem
{
	public:
		std::vector<CMenuItem*> items;
	private:
		CFrameBuffer* frameBuffer;

		//		
		int wanted_height;
		int wanted_width;
		int start_x;
		int start_y;
		
		CTextBox * textBox;

		int selected;

		//
		std::vector<unsigned int> page_start;
		unsigned int current_page;
		unsigned int total_pages;
		int sb_width;
		int listmaxshow;
		int iconOffset;
		int pos;
		unsigned int item_start_y;
		int items_height;
		int items_width;

		CScrollBar scrollBar;

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
		std::vector<int> widget;
		int cnt;

		// head
		bool paintTitle;
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
		int fbutton_count;
		int fbutton_width;
		button_label_list_t fbutton_labels;
		bool foot_line;
		bool foot_line_gradient;

		// itemInfo
		int cFrameFootInfoHeight;
		int footInfoHeight;
		CItems2DetailsLine itemsLine;
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

		// methods
		virtual void paintItems();

		//
		fb_pixel_t * background;
		bool savescreen;
		void saveScreen();
		void restoreScreen();
		int full_height;
		int full_width;

		std::string actionKey; // lua
		
		//
		bool paintFrame;
		fb_pixel_t bgcolor;
		int radius;
		int corner;
		bool scrollbar;
		fb_pixel_t* items_background;
		
		//
		int widgetMode;
		//int menu_position;
		
		// item
		int itemBorderMode;
		int itemGradient;
		bool item2Lines;
		
	public:
		ClistBox(const int x = 0, int const y = 0, const int dx = MENU_WIDTH, const int dy = MENU_HEIGHT);
		ClistBox(CBox* position);
		virtual ~ClistBox();

		virtual void addItem(CMenuItem * menuItem, const bool defaultselected = false);
		bool hasItem();
		void clearItems(void){items.clear(); current_page = 0;};
		void clear(void){items.clear(); hbutton_labels.clear(); fbutton_labels.clear(); widget.clear();current_page = 0;};
		void setSelected(unsigned int _new) { selected = _new; };

		virtual void initFrames();
		virtual void paintHead();
		virtual void paintFoot();
		virtual void paintItemInfo(int pos);
		virtual void hideItemInfo();
		
		//
		virtual void paint();
		virtual void hide();
		void refresh(){if (paintDate && paintTitle) timer->refresh();};
		bool update() const {return paintDate;};

		// head
		void enablePaintHead(){paintTitle = true;};
		void enablePaintDate(void){paintDate = true;};
		void setTitle(const char* title = "", const char* icon = NULL){if (title) l_name = title; if(icon) iconfile = icon;};
		void setTitleHAlign(const int m){thalign = m;};
		void setHeadButtons(const struct button_label *_hbutton_label, const int _hbutton_count = 1);
		void setHeadColor(fb_pixel_t col) {headColor = col;};
		void setHeadRadius(int ra){headRadius = ra;};
		void setHeadCorner(int co){headCorner = co;};
		void setHeadGradient(int grad){headGradient = grad;};
		void setHeadLine(bool l, bool g = false){head_line = l; head_line_gradient = g;};
		void setFormat(const char* f){format = f;};
		
		// foot
		void enablePaintFoot(){paint_Foot = true;};
		void setFootButtons(const struct button_label *_fbutton_label, const int _fbutton_count = 1, const int _fbutton_width = 0);
		void setFootColor(fb_pixel_t col) {footColor = col;};
		void setFootRadius(int ra){footRadius = ra;};
		void setFootCorner(int co){footCorner = co;};
		void setFootGradient(int grad){footGradient = grad;};
		void setFootLine(bool l, bool g = false){foot_line = l; foot_line_gradient = g;};

		// itemInfo
		void enablePaintItemInfo(int fh = 0){paintFootInfo = true; footInfoHeight = fh;};
		void setItemInfoMode(int mode){footInfoMode = mode;};
		void setItemInfoPos(int x, int y, int dx, int dy)
		{
			if ( (footInfoMode == ITEMINFO_HINTITEM_MODE) || (footInfoMode == ITEMINFO_HINTICON_MODE) || (footInfoMode == ITEMINFO_HINTHINT_MODE))
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

		// mainFrame
		void enableShrinkMenu(){shrinkMenu = true;};
		void enableSaveScreen();
		void paintMainFrame(bool p){paintFrame = p;};
		void setColor(fb_pixel_t col){bgcolor = col;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		void paintScrollBar(bool sb){scrollbar = sb;};

		//
		virtual void scrollLineDown(const int lines = 1);
		virtual void scrollLineUp(const int lines = 1);
		virtual void scrollPageDown(const int pages = 1);
		virtual void scrollPageUp(const int pages = 1);
		virtual void swipLeft();
		virtual void swipRight();

		//
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
		void setItemsPerPage(int itemsX = 6, int itemsY = 3){itemsPerX = itemsX; itemsPerY = itemsY; maxItemsPerPage = itemsPerX*itemsPerY;};
		int getItemsPerX()const{return itemsPerX;};
		int getItemsPerY()const{return itemsPerY;};
		int getMaxItemsPerPage()const{return maxItemsPerPage;};

		// widget type/mode/pos
		void setWidgetType(int type){widgetType = type; widget.push_back(widgetType);};
		int getWidgetType(){return widgetType;};
		void addWidgetType(int wtype){widget.push_back(wtype);};
		void changeWidgetType();
		void setWidgetMode(int mode){widgetMode = mode;};
		
		// item
		void setItemBorderMode(int m = BORDER_ALL){itemBorderMode = m;};
		void setItemGradient(int gr = NOGRADIENT){itemGradient = gr;};
		void setItem2Lines(){item2Lines = true;};

		//
		bool isSelectable(void){return true;};

		//
		int oKKeyPressed(CMenuTarget* _parent, neutrino_msg_t _msg = RC_ok);
		void homeKeyPressed(){selected = -1;};
		void directKeyPressed(neutrino_msg_t _msg);

		//
		std::string getActionKey(void){return actionKey;}; // lua
		
		virtual void integratePlugins(CPlugins::i_type_t integration = CPlugins::I_TYPE_DISABLED, const unsigned int shortcut = RC_nokey, bool enabled = true, int imode = MODE_MENU, int itype = TYPE_STANDARD, bool i2lines = false, int iBorder = BORDER_NO);
		
		//
		inline bool isPainted(void){return painted;};
		bool hasHead(){return paintTitle;};
		bool hasFoot(){return paint_Foot;};
};

#endif // LISTBOX_H_
