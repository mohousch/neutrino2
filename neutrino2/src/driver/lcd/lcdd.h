/*
	$Id: lcdd.h 2013/10/12 mohousch Exp $

	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#include <configfile.h>
#include <pthread.h>

#include <driver/lcd/fontrenderer.h>


#define LCDDIR_VAR CONFIGDIR "/lcdd"

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

class CLCDPainter;
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
			MODE_FILEBROWSER,
			MODE_PROGRESSBAR,
			MODE_PROGRESSBAR2,
			MODE_INFOBOX
		};
		
		enum AUDIOMODES
		{
			AUDIO_MODE_PLAY,
			AUDIO_MODE_STOP,
			AUDIO_MODE_FF,
			AUDIO_MODE_PAUSE,
			AUDIO_MODE_REV
		};


	private:

		class FontsDef
		{
			public:
				LcdFont *channelname;
				LcdFont *time; 
				LcdFont *menutitle;
				LcdFont *menu;
		};

		CLCDDisplay			display;
		LcdFontRenderClass		*fontRenderer;
		FontsDef			fonts;

#define LCD_NUMBER_OF_ELEMENTS 7
		raw_lcd_element_t               element[LCD_NUMBER_OF_ELEMENTS];

		MODES				mode;
		AUDIOMODES			movie_playmode;

		std::string			servicename;
		std::string			epg_title;
		std::string			movie_big;
		std::string			movie_small;
		std::string			menutitle;
		char				volume;
		unsigned char			percentOver;
		bool				muted;
		bool				showclock;
		bool				movie_centered;
		bool				movie_is_ac3;
		bool				icon_dolby;
		CConfigFile			configfile;
		pthread_t			thrTime;
		int                             last_toggle_state_power;
		int				clearClock;
		unsigned int                    timeout_cnt;

		void count_down();

		static void* TimeThread(void*);
		bool lcdInit(const char * fontfile1, const char * fontname1, 
		             const char * fontfile2 = NULL, const char * fontname2 = NULL,
		             const char * fontfile3 = NULL, const char * fontname3 = NULL);
		void setlcdparameter(int dimm, int contrast, int power, int inverse, int bias);
		void displayUpdate();
		void showTextScreen(const std::string & big, const std::string & small, int showmode, bool perform_wakeup, bool centered = false);
		void drawBanner();

	public:
		CLCD();
		~CLCD();

		bool has_lcd;
		bool is4digits;	
		bool istftlcd;
		void wake_up();
		void setled(void) { return; };
		void setlcdparameter(void);

		static CLCD* getInstance();
		void init(const char * fontfile, const char * fontname,
		          const char * fontfile2=NULL, const char * fontname2=NULL,
		          const char * fontfile3=NULL, const char * fontname3=NULL); 

		void setMode(const MODES m, const char * const title = "");
		MODES getMode() { return mode; };

		void showServicename(const std::string name, const bool perform_wakeup = true, int pos = 0); // UTF-8
		void setEPGTitle(const std::string title);
		void setMovieInfo(const AUDIOMODES playmode, const std::string big, const std::string small, const bool centered = false);
		void setMovieAudio(const bool is_ac3);
		std::string getMenutitle() { return menutitle; };
		void showTime();
		/** blocks for duration seconds */
		void showRCLock(int duration = 2);
		void showVolume(const char vol, const bool perform_update = true);
		void showPercentOver(const unsigned char perc, const bool perform_update = true, const MODES m = MODE_TVRADIO);
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string & artist, const std::string & title, const std::string & album, int pos = 0);
		void showAudioPlayMode(AUDIOMODES m=AUDIO_MODE_PLAY);
		void showAudioProgress(const char perc, bool isMuted);
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

		void setAutoDimm(int);
		int getAutoDimm();
		void setBrightnessDeepStandby(int) { return ; };
		int getBrightnessDeepStandby() { return 0; };

		void setMuted(bool);

		void resume();
		void pause();

		void Lock();
		void Unlock();
		void Clear();
		void ShowIcon(vfd_icon icon, bool show);
		void ShowText(const char *s) { showServicename(std::string(s)); };
		void LCDshowText(int /*pos*/) { return ; };
		
		bool ShowPng(char *filename);
		bool DumpPng(char *filename);
		
	private:
		CFileList* m_fileList;
		int m_fileListPos;
		std::string m_fileListHeader;

		std::string m_infoBoxText;
		std::string m_infoBoxTitle;
		int m_infoBoxTimer;   // for later use
		bool m_infoBoxAutoNewline;
		
		bool m_progressShowEscape;
		std::string  m_progressHeaderGlobal;
		std::string  m_progressHeaderLocal;
		int m_progressGlobal;
		int m_progressLocal;
	public:
		void showFilelist(int flist_pos = -1,CFileList* flist = NULL,const char * const mainDir=NULL);
		void showInfoBox(const char * const title = NULL,const char * const text = NULL,int autoNewline = -1,int timer = -1);
		void showProgressBar(int global = -1,const char * const text = NULL,int show_escape = -1,int timer = -1);
		void showProgressBar2(int local = -1,const char * const text_local = NULL,int global = -1,const char * const text_global = NULL,int show_escape = -1);
};

#endif

