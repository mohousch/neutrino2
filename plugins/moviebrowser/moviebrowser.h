/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project
 
 	Homepage: http://dbox.cyberphoria.org/

	$Id: moviebrowser.h,v 1.5 2006/09/11 21:11:35 guenther Exp $

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

	***********************************************************

	Module Name: moviebrowser.h .

	Description: implementation of the CMovieBrowser class

	Date:	   Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'

	$Log: moviebrowser.h,v $
	Revision 1.5  2006/09/11 21:11:35  guenther
	General menu clean up
	Dir menu updated
	Add options menu
	In movie info menu  "update all" added
	Serie option added (hide serie, auto serie)
	Update movie info on delete movie
	Delete Background when menu is entered
	Timeout updated (MB does not exit after options menu is left)
	
	Revision 1.4  2006/02/20 01:10:34  guenther
	- temporary parental lock updated - remove 1s debug prints in movieplayer- Delete file without rescan of movies- Crash if try to scroll in list with 2 movies only- UTF8XML to UTF8 conversion in preview- Last file selection recovered- use of standard folders adjustable in config- reload and remount option in config
	
	Revision 1.3  2005/12/18 09:23:53  metallica
	fix compil warnings
	
	Revision 1.2  2005/12/12 07:58:02  guenther
	- fix bug on deleting CMovieBrowser - speed up parse time (20 ms per .ts file now)- update stale function- refresh directories on reload- print scan time in debug console
	

****************************************************************************/
#ifndef MOVIEBROWSER_H_
#define MOVIEBROWSER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <configfile.h>

#include <string>
#include <vector>
#include <list>

#include <gui/widget/listframe.h>
#include <gui/widget/menue.h>
#include <gui/widget/textbox.h>
#include <gui/movieinfo.h>
#include <driver/file.h>


#define MAX_NUMBER_OF_BOOKMARK_ITEMS MI_MOVIE_BOOK_USER_MAX // we just use the same size as used in Movie info (MAX_NUMBER_OF_BOOKMARK_ITEMS is used for the number of menu items)
#define MOVIEBROWSER_SETTINGS_FILE          PLUGINDIR "/moviebrowser/moviebrowser.conf"

#define MIN_BROWSER_FRAME_HEIGHT 100
#define MAX_BROWSER_FRAME_HEIGHT 400

/* !!!! Do NOT change the order of the enum, just add items at the end !!!! */
typedef enum
{
	MB_INFO_FILENAME 		= 0,
	MB_INFO_FILEPATH 		= 1,
	MB_INFO_TITLE 			= 2,
	MB_INFO_SERIE 			= 3,
	MB_INFO_INFO1 			= 4,
	MB_INFO_MAJOR_GENRE 		= 5,
	MB_INFO_MINOR_GENRE 		= 6,
	MB_INFO_INFO2 			= 7,
	MB_INFO_PARENTAL_LOCKAGE	= 8,
	MB_INFO_CHANNEL 		= 9,
	MB_INFO_BOOKMARK		= 10,
	MB_INFO_QUALITY 		= 11,
	MB_INFO_PREVPLAYDATE 		= 12,
	MB_INFO_RECORDDATE 		= 13,
	MB_INFO_PRODDATE 		= 14,
	MB_INFO_COUNTRY 		= 15,
	MB_INFO_GEOMETRIE 		= 16,
	MB_INFO_AUDIO 			= 17,
	MB_INFO_LENGTH 			= 18,
	MB_INFO_SIZE 			= 19, 
	MB_INFO_MAX_NUMBER		= 20 	// MUST be allways the last item in the list
}MB_INFO_ITEM;

typedef enum
{	
	MB_DIRECTION_AUTO = 0,
	MB_DIRECTION_UP = 1,
	MB_DIRECTION_DOWN = 2,
	MB_DIRECTION_MAX_NUMBER = 3	// MUST be allways the last item in the list
}MB_DIRECTION;

typedef struct
{
	MB_INFO_ITEM item;
	MB_DIRECTION direction;	
}MB_SORTING;

typedef enum
{
	MB_STORAGE_TYPE_UNDEFINED = 0,
	MB_STORAGE_TYPE_NFS = 1,
	MB_STORAGE_TYPE_VLC = 2,
	MB_STORAGE_MAX_NUMBER = 3	// MUST be allways the last item in the list
}MB_STORAGE_TYPE;

typedef struct
{
	MB_INFO_ITEM item;
	std::string optionString;
	int optionVar;
}MB_FILTER;

typedef enum
{
	MB_FOCUS_BROWSER = 0,
	MB_FOCUS_LAST_PLAY = 1,
	MB_FOCUS_LAST_RECORD = 2,
	MB_FOCUS_MOVIE_INFO = 3,
	MB_FOCUS_FILTER = 4,
	MB_FOCUS_MAX_NUMBER = 5	// MUST be allways the last item in the list
}MB_FOCUS;

typedef enum
{
	MB_GUI_BROWSER_ONLY = 0,
	MB_GUI_MOVIE_INFO = 1,
	MB_GUI_LAST_PLAY = 2,
	MB_GUI_LAST_RECORD = 3,
	MB_GUI_FILTER = 4,
	MB_GUI_MAX_NUMBER = 5	// MUST be allways the last item in the list
}MB_GUI;


typedef enum
{
	MB_PARENTAL_LOCK_OFF = 0,
	MB_PARENTAL_LOCK_ACTIVE = 1,
	MB_PARENTAL_LOCK_OFF_TMP = 2, // use this to activate the lock temporarily until next dbox start up
	MB_PARENTAL_LOCK_MAX_NUMBER = 3  // MUST be allways the last item in the list
}MB_PARENTAL_LOCK;

typedef struct
{
    std::string name;
    int* used;
}MB_DIR;

#define MB_MAX_ROWS 6
#define MB_MAX_DIRS 10
/* MB_SETTINGS to be stored in g_settings anytime ....*/
typedef struct
{
	// moviebrowser
	MB_GUI gui; 				//MB_GUI
	MB_SORTING sorting; 			//MB_SORTING
	MB_FILTER filter;			//MB_FILTER
	MI_PARENTAL_LOCKAGE parentalLockAge ;	//MI_PARENTAL_LOCKAGE
	MB_PARENTAL_LOCK parentalLock;		//MB_PARENTAL_LOCK
	
	std::string storageDir[MB_MAX_DIRS];
	int storageDirUsed[MB_MAX_DIRS];
	int storageDirRecUsed;
	int storageDirMovieUsed;
	
	int reload;
	int remount;
	
	int browser_serie_mode;
	int serie_auto_create;
	int prefer_tmdb_info;
	
	// these variables are used for the listframes
	int browserFrameHeight;
	int browserRowNr;
	MB_INFO_ITEM browserRowItem[MB_MAX_ROWS];//MB_INFO_ITEM
	int browserRowWidth[MB_MAX_ROWS];

	// to be added to config later 
    	int lastPlayMaxItems;
	int lastPlayRowNr;
	MB_INFO_ITEM lastPlayRow[MB_MAX_ROWS];
	int lastPlayRowWidth[MB_MAX_ROWS];

    	int lastRecordMaxItems;
	int lastRecordRowNr;
	MB_INFO_ITEM lastRecordRow[MB_MAX_ROWS];
	int lastRecordRowWidth[MB_MAX_ROWS];
}MB_SETTINGS;

// Priorities for Developmemt: P1: critical feature, P2: important feature, P3: for next release, P4: looks nice, lets see
class CMovieBrowser : public CMenuTarget
{
	public: // Variables /////////////////////////////////////////////////
		int Multi_Select;    // for FileBrowser compatibility, not used in MovieBrowser
		int Dirs_Selectable; // for FileBrowser compatibility, not used in MovieBrowser
		
	private: // Variables
		CFrameBuffer * m_pcWindow;

		CListFrame * m_pcBrowser;
		CListFrame * m_pcLastPlay;
		CListFrame * m_pcLastRecord;
		CTextBox * m_pcInfo;
		CListFrame * m_pcFilter; 
	
		CBox m_cBoxFrame;
		CBox m_cBoxFrameLastPlayList;
		CBox m_cBoxFrameLastRecordList;
		CBox m_cBoxFrameBrowserList;
		CBox m_cBoxFrameInfo;
		CBox m_cBoxFrameBookmarkList;
		CBox m_cBoxFrameFilter;
		CBox m_cBoxFrameFootRel;
		CBox m_cBoxFrameTitleRel;
		
		LF_LINES m_browserListLines;
		LF_LINES m_recordListLines;
		LF_LINES m_playListLines;
		LF_LINES m_FilterLines;

		std::vector<MI_MOVIE_INFO> m_vMovieInfo;
		std::vector<MI_MOVIE_INFO*> m_vHandleBrowserList;
		std::vector<MI_MOVIE_INFO*> m_vHandleRecordList;
		std::vector<MI_MOVIE_INFO*> m_vHandlePlayList;
		std::vector<std::string> m_dirNames;
        	std::vector<MI_MOVIE_INFO*> m_vHandleSerienames;

		unsigned int m_currentBrowserSelection;
		unsigned int m_currentRecordSelection;
		unsigned int m_currentPlaySelection;
		unsigned int m_currentFilterSelection;
 		unsigned int m_prevBrowserSelection;
		unsigned int m_prevRecordSelection;
		unsigned int m_prevPlaySelection;

		bool m_showBrowserFiles;
		bool m_showLastRecordFiles;
		bool m_showLastPlayFiles;
		bool m_showMovieInfo;
		bool m_showFilter;

		MI_MOVIE_INFO * m_movieSelectionHandler;
		std::string m_selectedDir;
		MB_FOCUS m_windowFocus;

		bool m_file_info_stale; // if this bit is set, MovieBrowser shall reload all movie infos from HD
		bool m_seriename_stale;

		static CFont * m_pcFontFoot;
		static CFont * m_pcFontTitle;
		
		std::string m_textTitle;
		
		MB_PARENTAL_LOCK m_parentalLock;
		MB_STORAGE_TYPE m_storageType;
	
		CConfigFile configfile;
		CMovieInfo m_movieInfo;
		MB_SETTINGS m_settings;
		std::vector<MB_DIR> m_dir;
			
		int movieInfoUpdateAll[MB_INFO_MAX_NUMBER];
		int movieInfoUpdateAllIfDestEmptyOnly;
		
	public:  // Functions //////////////////////////////////////////////////////////7
		CMovieBrowser(const char * path); //P1 
		CMovieBrowser(); //P1 
		~CMovieBrowser(); //P1 
		int exec(const char* path); //P1 
        	int exec(CMenuTarget* parent, const std::string & actionKey);
		std::string getCurrentDir(void); //P1 for FileBrowser compatibility
		CFile * getSelectedFile(void); //P1 for FileBrowser compatibility
		MI_MOVIE_BOOKMARKS* getCurrentMovieBookmark(void){if(m_movieSelectionHandler == NULL) return NULL; return(&(m_movieSelectionHandler->bookmarks));};
		
		MI_MOVIE_INFO* getCurrentMovieInfo(void){return(m_movieSelectionHandler);}; //P1 return start position in [s]
		void fileInfoStale(void); // call this function to force the Moviebrowser to reload all movie information from HD

		bool readDir(const std::string & dirname, CFileList* flist);

		bool delFile(CFile& file);
	
	private: //Functions
		///// MovieBrowser init /////////////// 
		void init(void); //P1
		void initGlobalSettings(void); //P1
		void initFrames(void);
		void initDevelopment(void); //P1 for development testing only
		void initRows(void);
		void reinit(void); //P1

		///// MovieBrowser Main Window////////// 
		int paint(void); //P1
		void refresh(void); //P1
        	void hide(void); //P1
		void refreshLastPlayList(void); //P2
		void refreshLastRecordList(void); //P2
		void refreshBrowserList(void); //P1
		void refreshFilterList(void); //P1
		void refreshMovieInfo(void); //P1
		void refreshBookmarkList(void); // P3
		void refreshFoot(void); //P2
		void refreshTitle(void); //P2
		void refreshInfo(void); // P2
		void refreshLCD(void); // P2
	
		///// Events ///////////////////////////
		bool onButtonPress(neutrino_msg_t msg); // P1
		bool onButtonPressMainFrame(neutrino_msg_t msg); // P1
		bool onButtonPressBrowserList(neutrino_msg_t msg); // P1
		bool onButtonPressLastPlayList(neutrino_msg_t msg); // P2
		bool onButtonPressLastRecordList(neutrino_msg_t msg); // P2
		bool onButtonPressFilterList(neutrino_msg_t msg); // P2
		bool onButtonPressMovieInfoList(neutrino_msg_t msg); // P2
		void onSetFocus(MB_FOCUS new_focus); // P2
		void onSetFocusNext(void); // P2
		void onSetGUIWindow(MB_GUI gui);
		void onSetGUIWindowNext(void);
		void onSetGUIWindowPrev(void);
		void onDeleteFile(MI_MOVIE_INFO& movieSelectionHandler);  // P4
		bool onSortMovieInfoHandleList(std::vector<MI_MOVIE_INFO*>& pv_handle_list, MB_INFO_ITEM sort_type, MB_DIRECTION direction);
		
		///// parse Storage Directories ///////////// 
        	bool addDir(std::string& dirname, int* used);
        	void updateDir(void);
		void loadAllTsFileNamesFromStorage(void); // P1
		bool loadTsFileNamesFromDir(const std::string & dirname); // P1
		void getStorageInfo(void); // P3
		
		///// Menu //////////////////////////////////// 
		bool showMenu(); // P2
        	void showMovieInfoMenu(MI_MOVIE_INFO* movie_info); // P2
		
		///// settings /////////////////////////////////// 
		bool loadSettings(MB_SETTINGS* settings); // P2
		bool saveSettings(MB_SETTINGS* settings); // P2
        	void defaultSettings(MB_SETTINGS* settings);
		
		///// EPG_DATA /XML /////////////////////////////// 
		void loadMovies();
		void loadAllMovieInfo(void); // P1
		void saveMovieInfo(std::string* filename, MI_MOVIE_INFO* movie_info); // P2
	
		// misc
		void showHelp(void);
		bool isFiltered(MI_MOVIE_INFO& movie_info);
		bool isParentalLock(MI_MOVIE_INFO& movie_info);
		bool getMovieInfoItem(MI_MOVIE_INFO& movie_info, MB_INFO_ITEM item, std::string* item_string);
		void updateMovieSelection(void);
		void updateFilterSelection(void);
		void updateSerienames(void);
        	void autoFindSerie(void);
};

// Class to show Moviebrowser Information, to be used by menu
class CMovieHelp : public CMenuTarget
{
	public:
		CMovieHelp(){};
		~CMovieHelp(){};
		int exec( CMenuTarget* parent, const std::string & actionKey );
};

//
class CFileChooser : public CMenuWidget
{
	private:
		std::string *dirPath;

	public:
		CFileChooser(std::string *path){dirPath = path;};
		int exec(CMenuTarget *parent, const std::string & actionKey);
 };

typedef enum
{
	DIR_STATE_UNKNOWN = 0,
	DIR_STATE_SERVER_DOWN = 1,
	DIR_STATE_NOT_MOUNTED = 2,
	DIR_STATE_MOUNTED = 3,
	DIR_STATE_DISABLED = 4
}DIR_STATE;

#define MAX_DIR 10
class CDirMenu : public CMenuWidget
{
	private:
		std::vector<MB_DIR>* dirList;
		DIR_STATE dirState[MAX_DIR];
		std::string dirOptionText[MAX_DIR];
		int dirNfsMountNr[MAX_DIR];
		bool changed;

		void updateDirState(void);

	public:
		CDirMenu(std::vector<MB_DIR>* dir_list);
		int exec(CMenuTarget* parent, const std::string & actionKey);
		void show(void);
		bool isChanged(){return changed;};
 };


// EPG Genre , taken from epgview, TODO: migth be splitted in major/minor to increase handling, might be moved to CMovieInfo
#define GENRE_ALL_COUNT 76
const keyval GENRE_ALL[GENRE_ALL_COUNT] =
{
	{ 0x00, _("unknown") },
	{ 0x10, _("movie/drama") },
	{ 0x11, _("detective/thriller") },
	{ 0x12, _("adventure/western/war") },
	{ 0x13, _("science fiction/fantasy/horror") },
	{ 0x14, _("comedy") },
	{ 0x15, _("soap/melodrama/folkloric") },
	{ 0x16, _("romance") },
	{ 0x17, _("serious/classical/religious/historical movie/drama") },
	{ 0x18, _("adult movie/drama") },
	{ 0x20, _("news") },
	{ 0x21, _("news/weather report") },
	{ 0x22, _("news magazine") },
	{ 0x23, _("documentary") },
	{ 0x24, _("discussion/interview/debate") },
	{ 0x30, _("Show / Gameshow") },
	{ 0x31, _("game show/quiz/contest") },
	{ 0x32, _("variety show") },
	{ 0x33, _("talk show") },
	{ 0x40, _("sports") },
	{ 0x41, _("special events (Olympic Games,World Cup etc.)") },
	{ 0x42, _("sports magazines") },
	{ 0x43, _("football/soccer") },
	{ 0x44, _("tennis/squash") },
	{ 0x45, _("team sports (excluding football)") },
	{ 0x46, _("athletics") },
	{ 0x47, _("motor sports") },
	{ 0x48, _("water sports") },
	{ 0x49, _("winter sports") },
	{ 0x4A, _("equestrian") },
	{ 0x4B, _("martial sports") },
	{ 0x50, _("children / juvenile program") },
	{ 0x51, _("pre-school children's programmes") },
	{ 0x52, _("entertainment programmes for 6 to 14") },
	{ 0x53, _("entertainment programmes for 10 to 16") },
	{ 0x54, _("informational/educational/school programmes") },
	{ 0x55, _("cartoons/puppets") },
	{ 0x60, _("music / ballet / dance") },
	{ 0x61, _("rock/pop") },
	{ 0x62, _("serious music/classical music") },
	{ 0x63, _("folk/traditional music") },
	{ 0x64, _("jazz") },
	{ 0x65, _("musical/opera") },
	{ 0x66, _("ballet") },
	{ 0x70, _("arts / culture") },
	{ 0x71, _("performing arts") },
	{ 0x72, _("fine arts") },
	{ 0x73, _("religion") },
	{ 0x74, _("popular culture/traditional arts") },
	{ 0x75, _("literature") },
	{ 0x76, _("film/cinema") },
	{ 0x77, _("experimental film/video") },
	{ 0x78, _("broadcasting/press") },
	{ 0x79, _("new media") },
	{ 0x7A, _("arts/culture magazines") },
	{ 0x7B, _("fashion") },
	{ 0x80, _("social & politic events / business") },
	{ 0x81, _("magazines/reports/documentary") },
	{ 0x82, _("economics/social advisory") },
	{ 0x83, _("remarkable people") },
	{ 0x90, _("documentation / magazine") },
	{ 0x91, _("nature/animals/environment") },
	{ 0x92, _("technology/natural sciences") },
	{ 0x93, _("medicine/physiology/psychology") },
	{ 0x94, _("foreign countries/expeditions") },
	{ 0x95, _("social/spiritual sciences") },
	{ 0x96, _("further education") },
	{ 0x97, _("languages") },
	{ 0xA0, _("travel & recreation") },
	{ 0xA1, _("tourism/travel") },
	{ 0xA2, _("handicraft") },
	{ 0xA3, _("motoring") },
	{ 0xA4, _("fitness & health") },
	{ 0xA5, _("cooking") },
	{ 0xA6, _("advertisement/shopping") },
	{ 0xA7, _("gardening") }
};

#define GENRE_MOVIE_COUNT 9
const keyval genre_movie[GENRE_MOVIE_COUNT] =
{
	{ 0, _("movie/drama") },
	{ 1, _("detective/thriller") },
	{ 2, _("adventure/western/war") },
	{ 3, _("science fiction/fantasy/horror") },
	{ 4, _("comedy") },
	{ 5, _("soap/melodrama/folkloric") },
	{ 6, _("romance") },
	{ 7, _("serious/classical/religious/historical movie/drama") },
	{ 8, _("adult movie/drama") }
};

#define GENRE_NEWS_COUNT 5
const keyval genre_news[GENRE_NEWS_COUNT] =
{
	{ 0, _("news") },
	{ 1, _("news/weather report") },
	{ 2, _("news magazine") },
	{ 3, _("documentary") },
	{ 4, _("discussion/interview/debate") }
};

#define GENRE_SHOW_COUNT 4
const keyval genre_show[GENRE_SHOW_COUNT] =
{
	{ 0, _("Show / Gameshow") },
	{ 1, _("game show/quiz/contest") },
	{ 2, _("variety show") },
	{ 3, _("talk show") }
};

#define GENRE_SPORTS_COUNT 12
const keyval genre_sports[GENRE_SPORTS_COUNT] =
{
	{ 0, _("sports") },
	{ 1, _("special events (Olympic Games,World Cup etc.)") },
	{ 2, _("sports magazines") },
	{ 3, _("football/soccer") },
	{ 4, _("tennis/squash") },
	{ 5, _("team sports (excluding football)") },
	{ 6, _("athletics") },
	{ 7, _("motor sports") },
	{ 8, _("water sports") },
	{ 9, _("winter sports") },
	{ 10, _("equestrian") },
	{ 11, _("martial sports") },
};

#define GENRE_CHILDRENS_PROGRAMMES_COUNT 6
const keyval genre_childrens_programmes[GENRE_CHILDRENS_PROGRAMMES_COUNT] =
{
	{ 0, _("children / juvenile program") },
	{ 1, _("pre-school children's programmes") },
	{ 2, _("entertainment programmes for 6 to 14") },
	{ 3, _("entertainment programmes for 10 to 16") },
	{ 4, _("informational/educational/school programmes") },
	{ 5, _("cartoons/puppets") },
};

#define GENRE_MUSIC_DANCE_COUNT 7
const keyval genre_music_dance[GENRE_MUSIC_DANCE_COUNT] =
{
	{ 0, _("music / ballet / dance") },
	{ 1, _("rock/pop") },
	{ 2, _("serious music/classical music") },
	{ 3, _("folk/traditional music") },
	{ 4, _("jazz") },
	{ 5, _("musical/opera") },
	{ 6, _("ballet") },
};

#define GENRE_ARTS_COUNT 12
const keyval genre_arts_dance[GENRE_ARTS_COUNT] =
{
	{ 0, _("arts / culture") },
	{ 1, _("performing arts") },
	{ 2, _("fine arts") },
	{ 3, _("religion") },
	{ 4, _("popular culture/traditional arts") },
	{ 5, _("literature") },
	{ 6, _("film/cinema") },
	{ 7, _("experimental film/video") },
	{ 8, _("broadcasting/press") },
	{ 9, _("new media") },
	{ 10, _("arts/culture magazines") },
	{ 11, _("fashion") },
};

#define GENRE_SOCIAL_POLITICAL_COUNT 4
const keyval genre_social_political[GENRE_SOCIAL_POLITICAL_COUNT] =
{
	{ 0, _("social & politic events / business") },
	{ 1, _("magazines/reports/documentary") },
	{ 2, _("economics/social advisory") },
	{ 3, _("remarkable people") },
};

#define GENRE_DOCUS_MAGAZINES_COUNT 8
const keyval genre_docus_magazines[GENRE_DOCUS_MAGAZINES_COUNT] =
{
	{ 0, _("documentation / magazine") },
	{ 1, _("nature/animals/environment") },
	{ 2, _("technology/natural sciences") },
	{ 3, _("medicine/physiology/psychology") },
	{ 4, _("foreign countries/expeditions") },
	{ 5, _("social/spiritual sciences") },
	{ 6, _("further education") },
	{ 7, _("languages") },
};

#define GENRE_TRAVEL_HOBBIES_COUNT 8
const keyval genre_travel_hobbies[GENRE_TRAVEL_HOBBIES_COUNT] =
{
	{ 0, _("travel & recreation") },
	{ 1, _("tourism/travel") },
	{ 2, _("tourism/travel") },
	{ 3, _("handicraft") },
	{ 4, _("motoring") },
	{ 5, _("fitness & health") },
	{ 6, _("cooking") },
	{ 7, _("gardening") }
};

#define GENRE_MAJOR_COUNT 10
const keyval genre_major[GENRE_MAJOR_COUNT] =
{
	{ 1, _("movie/drama") },
	{ 2, _("news") },
	{ 3, _("Show / Gameshow") },
	{ 4, _("sports") },
	{ 5, _("children / juvenile program") },
	{ 6, _("music / ballet / dance") },
	{ 7, _("arts / culture") },
	{ 8, _("social & politic events / business") },
	{ 9, _("documentation / magazine") },
	{ 10, _("travel & recreation") }
};

// onoff notifier //needed by moviebrowser
class COnOffNotifier : public CChangeObserver
{
        private:
                int number;
                CMenuItem * toDisable[15];
        public:
                COnOffNotifier(CMenuItem* a1, CMenuItem* a2 = NULL, CMenuItem* a3 = NULL, CMenuItem* a4 = NULL, CMenuItem* a5 = NULL);
		COnOffNotifier(int OffValue = 0);
                bool changeNotify(const std::string&, void *Data);
		void addItem(CMenuItem* menuItem);
};

#endif /*MOVIEBROWSER_H_*/




