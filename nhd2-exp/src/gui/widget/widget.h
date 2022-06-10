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
#include <gui/widget/listbox.h>


class ClistBox;
//
#define WIDGET_COUNT	83
enum {
	WIDGET_NEUTRINO = 			0,
	//MENU
	WIDGET_MAINMENU = 			1,	// mainmenu
	WIDGET_MAINSETTINGS = 			2,	
	WIDGET_EPGTIMER = 			3,
	WIDGET_SYSTEM = 			4,
	WIDGET_INFORMATION =			5,
	WIDGET_FEATURES = 			6,
	WIDGET_POWERMENU = 			7,
	WIDGET_MEDIAPLAYER = 			8,
	//SETUPS
	WIDGET_VIDEOSETUP = 			9,
	WIDGET_AUDIOSETUP = 			10,
	WIDGET_PARENTALSETUP = 		11,
	WIDGET_NETWORKSETUP = 			12,
	WIDGET_RECORDINGSETUP = 		13,
	WIDGET_MOVIEPLAYERSETUP = 		14,
	WIDGET_ODSETTINGS = 			15,
	WIDGET_LCDSETUP = 			16,
	WIDGET_REMOTECONTROLSETUP = 		17,
	WIDGET_AUDIOPLAYERSETUP = 		18,
	WIDGET_PICTUREVIEWERSETUP = 		19,
	WIDGET_MISCSETTINGS = 			20,
	WIDGET_HDDSETUP = 			21,
	// osdsetup
	WIDGET_SKINSETUP = 			22,
	WIDGET_MENUSETUP = 			23,
	WIDGET_INFOBARSETUP = 			24,
	WIDGET_THEMESETUP = 			25,
	WIDGET_LANGUAGESETUP = 		26,
	WIDGET_FONTSETUP = 			27,
	WIDGET_OSDTIMINGSETUP = 		28,
	WIDGET_SCREENSETUP = 			29,
	WIDGET_OSDMISCSETUP = 			30,
	WIDGET_ALPHASETUP = 			31,
	WIDGET_SKINSTYLESELECTIONSETUP = 	32,
	// miscsetup
	WIDGET_MISCSETUP = 			33,
	WIDGET_EPGSETUP = 			34,
	WIDGET_CHANNELSSETUP = 		35,
	WIDGET_ZAPITSETUP = 			36,
	WIDGET_FILEBROWSERSETUP = 		37,
	// networksetup	
	WIDGET_PROXYSETUP = 			38,
	WIDGET_NFS = 				39,
	//service
	WIDGET_TUNERSETUP = 			40,
	WIDGET_CICAMSETUP = 			41,
	WIDGET_UPDATESETUP = 			42,
	//LISTS
	WIDGET_PLUINGSLIST = 			43,
	WIDGET_EPGVIEW = 			44,
	WIDGET_EVENTLIST = 			45,
	WIDGET_EPGPLUS = 			46,
	WIDGET_TIMERLIST = 			47,
	WIDGET_CHANNELSLIST = 			48,
	WIDGET_BOUQUETSLIST = 			49,
	WIDGET_BOUQUETEDITOR = 		50,
	WIDGET_FILEBROWSER = 			51,
	//CORE/PLAYER
	WIDGET_AUDIOPLAYER = 			52,
	WIDGET_MOVIEPLAYER = 			53,
	WIDGET_PICTUREVIEWER = 		54,
	//INFOBAR
	WIDGET_INFOBAR = 			55,
	//DIVERS
	WIDGET_AUDIOSELECT = 			56,
	WIDGET_AVSELECT = 			57,
	WIDGET_SUBSELECT = 			58,
	WIDGET_CHANNELSELECT = 		59,
	WIDGET_DBOXINFO = 			60,
	WIDGET_IMAGEINFO = 			61,
	WIDGET_MOTORCONTROL = 			62,
	WIDGET_MOVIEINFO = 			63,
	WIDGET_SCAN = 				64,
	WIDGET_SLEEPTIMER = 			65,
	WIDGET_STREAMINFO =			66,
	WIDGET_VFDCONTROLLER = 		67,
	WIDGET_COLORCHOOSER = 			68,
	WIDGET_KEYCHOOSER = 			69,
	WIDGET_MOUNTCHOOSER = 			70,
	//
	WIDGET_HINTBOX,
	WIDGET_MESSAGEBOX,
	WIDGET_HELPBOX,
	WIDGET_INFOBOX,
	WIDGET_PROGRESSWINDOW,
	WIDGET_STRINGINPUT,
	WIDGET_HTTPTOOL,
	WIDGET_VOLUME,
	WIDGET_MUTE,
	//
	WIDGET_PLUGIN = 			80,
	WIDGET_NVOD = 				81,		// yellow
	
	//
	WIDGET_NEWTIMER = 			90,
	WIDGET_MODIFYTIMER = 			91,
	
	//
	WIDGET_TRANSPONDER =			100,
	WIDGET_UNICABLESETUP = 		101,
	WIDGET_MANUALSCAN = 			102,
	WIDGET_AUTOSCAN = 			103,
	WIDGET_AUTOSCANALL = 			104,
	WIDGET_SATSETUP = 			105,
	WIDGET_MOTORSETUP = 			106,
	
	//
	WIDGET_FLASHEXPERT = 			110,
	WIDGET_MTDSELECTOR = 			111,
	WIDGET_FILESELECTOR = 			112,
	
	//
	WIDGET_MAX = 				999
};

//
enum
{
	RETURN_NONE		= 0,
	RETURN_REPAINT 	= 1,
	RETURN_EXIT 		= 2,
	RETURN_EXIT_ALL 	= 4
};

//
class CMenuTarget
{
	protected:
		std::string *valueString;
		std::string valueStringTmp;

	public:
		int id;
		
	public:
		CMenuTarget(){ valueStringTmp = std::string(); valueString = &valueStringTmp; id = -1;};
		virtual ~CMenuTarget(){};
		virtual void hide(){valueString->clear();};
		virtual int exec(CMenuTarget* parent, const std::string& actionKey) = 0;
		virtual std::string& getString(void) { return *valueString; };
};

//
class CWidget : public CMenuTarget
{
	public:
		std::string name;
		
	protected:
		CFrameBuffer *frameBuffer;
		CBox mainFrameBox;

		// 
		std::vector<CWidgetItem*> items;
		std::vector<CComponent*> CCItems;

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
		std::string actionKey; // for lua
		
		// screen
		fb_pixel_t * background;
		bool savescreen;
		void saveScreen();
		void restoreScreen();
		//
		bool enablePos;
		int menu_position;

		// mainframe		
		bool paintframe;
		fb_pixel_t backgroundColor;
		int gradient;
		int radius;
		int corner;
		
		//
		void initFrames();

	public:
		CWidget(const int x = 0, const int y = 0, const int dx = DEFAULT_XRES, const int dy = DEFAULT_YRES);
		CWidget(CBox *position);
		virtual ~CWidget();

		// WIDGETITEMS
		virtual void addItem(CWidgetItem* widgetItem, const bool defaultselected = false);
		bool hasItem(){return !items.empty();};
		int getItemsCount(){return items.size();};
		virtual void clearItems(void){return items.clear();};
		virtual void paintItems();
		virtual void removeItem(long pos);
		
		void setSelected(unsigned int _new) {selected = _new; if (selected < 0) selected = 0;};
		
		// CCITEMS
		virtual void addCCItem(CComponent* CCItem);
		bool hasCCItem(){return !CCItems.empty();};
		int getCCItemsCount(){return CCItems.size();};
		virtual void clearCCItems(){CCItems.clear();};
		virtual void paintCCItems();
		virtual void removeCCItem(long pos);
		
		//
		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget *parent, const std::string &actionKey);

		//
		void setTimeOut(unsigned long long int to = 0){timeout = to;};
		void setSecTimerInterval(uint64_t interval){sec_timer_interval = interval;};
		
		//
		void addKey(neutrino_msg_t key, CMenuTarget *menue = NULL, const std::string &action = "");

		//
		void paintMainFrame(bool p){paintframe = p;};
		void setColor(fb_pixel_t col) {backgroundColor = col;};
		void setGradient(int gra){gradient = gra;};
		void setCorner(int ra, int co){radius = ra; corner = co;};
		//
		void enableSaveScreen();
		void setMenuPosition(int p){enablePos = true; menu_position = p;};

		// lua compatibility
		int getSelected(){return exit_pressed ? -1 : selected;};
		std::string getActionKey(){return actionKey;};
		neutrino_msg_t getKey(){return msg;};
		inline CBox getWindowsPos(void){return(mainFrameBox);};
		bool getExitPressed(){return exit_pressed;};

		// events
		virtual void onOKKeyPressed(neutrino_msg_t _msg = RC_ok);
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
		int getWidgetID(){return id;};
		std::string getWidgetName(){return name;};
		
		//
		CWidgetItem* getWidgetItem(const int type, const std::string& name = "");
		CComponent* getCCItem(const int type, const std::string& name = "");
};

typedef std::vector<CWidget*> WIDGETLIST;

#endif // WIDGET_H_

