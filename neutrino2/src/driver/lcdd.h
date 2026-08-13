//
//	$Id: lcdd.h 04092025 mohousch Exp $
//
//	LCD-Daemon  -   DBoxII-Project
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#ifndef __lcdd__
#define __lcdd__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// TODO Why is USE_FILE_OFFSET64 not defined, if file.h is included here????
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64 1
#endif
#include <driver/file.h>

#include <pthread.h>

#include <driver/lcd/lcddisplay.h>
#include <driver/lcd/fontrenderer.h>


#if defined (__sh__)
# include <driver/lcdd_sh4.h>
#endif


// neutrino commom vfd icons
#ifndef __sh__
typedef enum
{
	VFD_ICON_BAR8       = 0x00000004,
	VFD_ICON_BAR7       = 0x00000008,
	VFD_ICON_BAR6       = 0x00000010,
	VFD_ICON_BAR5       = 0x00000020,
	VFD_ICON_BAR4       = 0x00000040,
	VFD_ICON_BAR3       = 0x00000080,
	VFD_ICON_BAR2       = 0x00000100,
	VFD_ICON_BAR1       = 0x00000200,
	VFD_ICON_FRAME      = 0x00000400,
	VFD_ICON_HDD        = 0x00000800,
	VFD_ICON_MUTE       = 0x00001000,
	VFD_ICON_DOLBY      = 0x00002000,
	VFD_ICON_POWER      = 0x00004000,
	VFD_ICON_TIMESHIFT  = 0x00008000,
	VFD_ICON_SIGNAL     = 0x00010000,
	VFD_ICON_TV         = 0x00020000,
	VFD_ICON_RADIO      = 0x00040000,
	VFD_ICON_HD         = 0x01000001,
	VFD_ICON_1080P      = 0x02000001,
	VFD_ICON_1080I      = 0x03000001,
	VFD_ICON_720P       = 0x04000001,
	VFD_ICON_480P       = 0x05000001,
	VFD_ICON_480I       = 0x06000001,
	VFD_ICON_USB        = 0x07000001,
	VFD_ICON_MP3        = 0x08000001,
	VFD_ICON_PLAY       = 0x09000001,
	VFD_ICON_COL1       = 0x09000002,
	VFD_ICON_PAUSE      = 0x0A000001,
	VFD_ICON_CAM1       = 0x0B000001,
	VFD_ICON_COL2       = 0x0B000002,
	VFD_ICON_CAM2       = 0x0C000001,
	VFD_ICON_LOCK,
} vfd_icon;
#endif

class LcdFontRenderClass;

class CLCD
{
	public:
		enum MODES
		{
			MODE_TVRADIO,
			MODE_SCART,
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8,
			MODE_AUDIO,
			MODE_MOVIE,
			MODE_PIC,
			MODE_PROGRESSBAR,
			MODE_PROGRESSBAR2
		};
		
		enum PLAYMODES
		{
			PLAY_MODE_PLAY,
			PLAY_MODE_STOP,
			PLAY_MODE_FF,
			PLAY_MODE_PAUSE,
			PLAY_MODE_REV
		};

		enum LEDCOLOR
		{
			LEDCOLOR_OFF,
			LEDCOLOR_BLUE,
			LEDCOLOR_RED,
			LEDCOLOR_PURPLE
		};
		
		enum MINITV
		{
			MINITV_NORMAL,
			MINITV_TV,
			MINITV_OSD,
			MINITV_OSD_TV
		};
		
		enum EPGALIGN
		{
			EPGALIGN_LEFT,
			EPGALIGN_CENTER
		};
		
		enum MODE
		{
			MODE_CHANNEL_INFO		= 1,
			MODE_TIME			= 2,
		};
		
		enum EPGMODE
		{
			EPGMODE_CHANNEL 		= 1,
			EPGMODE_TITLE			= 2,
			EPGMODE_CHANNEL_TITLE		= 3
		};
		
		enum STATUSLINE
		{
			STATUSLINE_PLAYTIME,
			STATUSLINE_VOLUME
		};
		
		enum STANDBYCLOCK
		{
			STANDBYCLOCK_DIGITAL,
			STANDBYCLOCK_ANALOG
		};

	private:
		class FontsDef
		{
			public:
				LcdFont *channelname;
				LcdFont *time; 
				LcdFont *menutitle;
				LcdFont *menu;
				LcdFont *timestandby;
		};
		
		int 				fd;
		int 				lcd_width;
		int 				lcd_height;
		CLCDDisplay			*display;
		LcdFontRenderClass		*fontRenderer;
		FontsDef			fonts;

#define LCD_NUMBER_OF_ELEMENTS 			15
		raw_lcd_element_t               element[LCD_NUMBER_OF_ELEMENTS];

		MODES				mode;
		PLAYMODES			movie_playmode;

		std::string			servicename;
		unsigned int			servicenumber;
		std::string			epg_title;
		std::string			movie_big;
		std::string			movie_small;
		std::string			menutitle;
		char				volume;
		unsigned char			percentOver;
		bool				muted;
		bool				showclock;
		bool				movie_is_ac3;
		bool				icon_dolby;
		pthread_t			thrTime;
		int                             last_toggle_state_power;
		int				clearClock;
		unsigned int                    timeout_cnt;
		////
		std::string  m_progressHeaderGlobal;
		std::string  m_progressHeaderLocal;
		int m_progressGlobal;
		int m_progressLocal;
		////
		int logo_x;
		int logo_y;
		int logo_w;
		int logo_h;
		int w_icon_w;
		int w_icon_h;
		////
		void count_down();
		static void* TimeThread(void*);
		bool lcdInit(const char * fontfile1, const char * fontname1);
		void setlcdparameter(int dimm, int contrast, int power, int inverse, int bias);
		void displayUpdate();
		void drawBanner();

	public:
		CLCD();
		~CLCD();

		bool has_lcd;
		bool has_nglcd;
		void wake_up();
		void setLED(int value, int option);
		void setMiniTV(int value);
		void setlcdparameter(void);

		static CLCD* getInstance();
		void init(const char * fontfile, const char * fontname); 

		void setMode(const MODES m, const char * const title = "");
		MODES getMode() { return mode; };

		////
		void showText(const char *str);
		void showTextScreen(const std::string &big, const std::string &small, int showmode, bool perform_wakeup, bool centered = false);
		void showServicename(const std::string &name, const bool perform_wakeup = true, int pos = 0); // UTF-8
		////
		void setEPGTitle(const std::string title);
		void showMovieInfo(const PLAYMODES playmode, const std::string big, const std::string small, const bool centered = false);
		void setMovieAudio(const bool is_ac3);
		////
		std::string getMenutitle() { return menutitle; };
		void showTime(bool force = false);
		void showRCLock(int duration = 2);
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true, const MODES m = MODE_TVRADIO);
		void showMenuText(const int position, const char * text, const int selected = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album, int pos = 0);
		void showPlayMode(PLAYMODES m = PLAY_MODE_PLAY);
		////
		void showWeather();
		void showProgressBar(int global = -1,const char * const text = NULL);
		void showProgressBar2(int local = -1, const char * const text_local = NULL, int global = -1, const char * const text_global = NULL);
		////
		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setContrast(int);
		int getContrast();

		void setPower(int);
		int getPower();

		void togglePower(void);

		void setInverse(int);
		int getInverse();

		void setBrightnessDeepStandby(int) { return ; };
		int getBrightnessDeepStandby() { return 0; };

		void setMuted(bool);

		void resume();
		void pause();

		void Clear();
		void ShowIcon(vfd_icon icon, bool show);
		void ShowDiskLevel();
		void ClearIcons();
		
		bool ShowPng(char *filename);
		bool DumpPng(char *filename);
		
#if defined (__sh__)
		void openDevice();
		void closeDevice();
#endif

#ifdef ENABLE_GRAPHLCD
		int GetConfigSize();
		std::string GetConfigName(int);
		void reinitGLCD();
		void setGLCDBrightness(int brightness);
#endif
};

#endif

