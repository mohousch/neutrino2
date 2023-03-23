/***************************************************************************
	Neutrino-GUI  -   DBoxII-Project

 	Homepage: http://dbox.cyberphoria.org/

	$Id: moviebrowser.cpp,v 1.10 2013/08/18 11:23:30 mohousch Exp $

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

	Module Name: moviebrowser.cpp .

	Description: Implementation of the CMovieBrowser class
	             This class provides a filebrowser window to view, select and start a movies from HD.
	             This class does replace the Filebrowser

	Date:	   Nov 2005

	Author: Günther@tuxbox.berlios.org
		based on code of Steffen Hehn 'McClean'
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cstdlib>

#include "moviebrowser.h"

#include <gui/filebrowser.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput_ext.h>

#include <dirent.h>
#include <sys/stat.h>

#include <unistd.h>

#include <gui/nfs.h>
#include <neutrino2.h>
#include <gui/widget/stringinput.h>
#include <sys/vfs.h> // for statfs
#include <gui/widget/icons.h>
#include <sys/mount.h>
#include <utime.h>

#include <system/debug.h>
#include <system/helpers.h>
#include <system/settings.h>

#include <driver/vcrcontrol.h>

#include <gui/widget/infobox.h>
#include <system/tmdbparser.h>


#define my_scandir scandir64
#define my_alphasort alphasort64
typedef struct stat64 stat_struct;
typedef struct dirent64 dirent_struct;
#define my_stat stat64

// tstool
extern off64_t get_full_len(char * startname);
extern off64_t truncate_movie(MI_MOVIE_INFO * minfo);
extern off64_t cut_movie(MI_MOVIE_INFO * minfo, CMovieInfo * cmovie);
extern off64_t copy_movie(MI_MOVIE_INFO * minfo, CMovieInfo * cmovie, bool onefile);

#define NUMBER_OF_MOVIES_LAST 40 // This is the number of movies shown in last recored and last played list
 
#define MESSAGEBOX_BROWSER_ROW_ITEM_COUNT 20
const keyval MESSAGEBOX_BROWSER_ROW_ITEM[MESSAGEBOX_BROWSER_ROW_ITEM_COUNT] =
{
	{ MB_INFO_FILENAME, _("Name") },
	{ MB_INFO_FILEPATH, _("Path") },
	{ MB_INFO_TITLE, _("Title") },
	{ MB_INFO_SERIE, _("Serie") },
	{ MB_INFO_INFO1, _("Info 1") },
	{ MB_INFO_MAJOR_GENRE, _("Genre") },
	{ MB_INFO_MINOR_GENRE, _("Genre") },
	{ MB_INFO_PARENTAL_LOCKAGE, _("Age") },
	{ MB_INFO_CHANNEL, _("Channel") },
	{ MB_INFO_QUALITY, _("Quality") },
	{ MB_INFO_PREVPLAYDATE, _("Last play date") },
	{ MB_INFO_RECORDDATE, _("Date") },
	{ MB_INFO_PRODDATE, _("Year") },
	{ MB_INFO_COUNTRY, _("Country") },
	{ MB_INFO_GEOMETRIE, _("Picture") },
	{ MB_INFO_AUDIO, _("Audio") },
	{ MB_INFO_LENGTH, _("Length (Min)") },
	{ MB_INFO_SIZE, _("Size") },
	{ MB_INFO_BOOKMARK, _("Bookmarks") },
	{ MB_INFO_FILENAME, _("Name") }
 };

#define MESSAGEBOX_YES_NO_OPTIONS_COUNT 2
const keyval MESSAGEBOX_YES_NO_OPTIONS[MESSAGEBOX_YES_NO_OPTIONS_COUNT] =
{
	{ 0, _("No") },
	{ 1, _("Yes") }
};

#define MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT 3
const keyval MESSAGEBOX_PARENTAL_LOCK_OPTIONS[MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT] =
{
	{ 1, _("yes") },
	{ 0, _("no") },
	{ 2, _("no (temporary)") }
};

#define MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT 6
const keyval MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS[MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT] =
{
	{ 0,  _("always") },
	{ 6,  _("6 years") },
	{ 12, _("12 years") },
	{ 16, _("16 years") },
	{ 18, _("18 years") },
	{ 99, _("never") }
};	

#define TITLE_FONT 			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]
#define FOOT_FONT 			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]

CFont* CMovieBrowser::m_pcFontFoot = NULL;
CFont* CMovieBrowser::m_pcFontTitle = NULL;

const char* const m_localizedItemName[MB_INFO_MAX_NUMBER + 1] =
{
	_("Name"),
	_("Path"),
	_("Title"),
	_("Serie"),
	_("Info 1"),
	_("Genre"),
	_("Genre"),
	_("Info 2"),
	_("Age"),
	_("Channel"),
	_("Bookmark"),
	_("Quality"),
	_("Last play date"),
	_("Record date"),
	_("Year"),
	_("Country"),
	_("Format"),
	_("Audio"),
	_("Length (Min)"),
	_("Size"), 
	""
};

// default row size in pixel for any element
#define	MB_ROW_WIDTH_FILENAME 		150
#define	MB_ROW_WIDTH_FILEPATH		150
#define	MB_ROW_WIDTH_TITLE		500
#define	MB_ROW_WIDTH_SERIE		150 
#define	MB_ROW_WIDTH_INFO1		200
#define	MB_ROW_WIDTH_MAJOR_GENRE 	150
#define	MB_ROW_WIDTH_MINOR_GENRE 	36
#define	MB_ROW_WIDTH_INFO2 		36
#define	MB_ROW_WIDTH_PARENTAL_LOCKAGE   45 
#define	MB_ROW_WIDTH_CHANNEL		100
#define	MB_ROW_WIDTH_BOOKMARK		50
#define	MB_ROW_WIDTH_QUALITY 		120
#define	MB_ROW_WIDTH_PREVPLAYDATE	80
#define	MB_ROW_WIDTH_RECORDDATE 	120
#define	MB_ROW_WIDTH_PRODDATE 		50
#define	MB_ROW_WIDTH_COUNTRY 		50
#define	MB_ROW_WIDTH_GEOMETRIE		50
#define	MB_ROW_WIDTH_AUDIO		50 	
#define	MB_ROW_WIDTH_LENGTH		40
#define	MB_ROW_WIDTH_SIZE 		80

const int m_defaultRowWidth[MB_INFO_MAX_NUMBER + 1] = 
{
	MB_ROW_WIDTH_FILENAME ,
	MB_ROW_WIDTH_FILEPATH,
	MB_ROW_WIDTH_TITLE,
	MB_ROW_WIDTH_SERIE,
	MB_ROW_WIDTH_INFO1,
	MB_ROW_WIDTH_MAJOR_GENRE ,
	MB_ROW_WIDTH_MINOR_GENRE ,
	MB_ROW_WIDTH_INFO2 ,
	MB_ROW_WIDTH_PARENTAL_LOCKAGE ,
	MB_ROW_WIDTH_CHANNEL,
	MB_ROW_WIDTH_BOOKMARK,
	MB_ROW_WIDTH_QUALITY ,
	MB_ROW_WIDTH_PREVPLAYDATE,
	MB_ROW_WIDTH_RECORDDATE ,
	MB_ROW_WIDTH_PRODDATE ,
	MB_ROW_WIDTH_COUNTRY ,
	MB_ROW_WIDTH_GEOMETRIE,
	MB_ROW_WIDTH_AUDIO 	,
	MB_ROW_WIDTH_LENGTH,
	MB_ROW_WIDTH_SIZE, 
	0 //MB_ROW_WIDTH_MAX_NUMBER 
};

static MI_MOVIE_INFO* playing_info;

// sorting
#define FILEBROWSER_NUMBER_OF_SORT_VARIANTS 5

bool sortDirection = 0;

bool compare_to_lower(const char a, const char b)
{
	return tolower(a) < tolower(b);
};

// sort operators
bool sortByTitle (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgTitle.begin(), a->epgTitle.end(), b->epgTitle.begin(), b->epgTitle.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgTitle.begin(), b->epgTitle.end(), a->epgTitle.begin(), a->epgTitle.end(), compare_to_lower))
		return false;
	
	return a->file.Time < b->file.Time;
}

bool sortByGenre (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgInfo1.begin(), a->epgInfo1.end(), b->epgInfo1.begin(), b->epgInfo1.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgInfo1.begin(), b->epgInfo1.end(), a->epgInfo1.begin(), a->epgInfo1.end(), compare_to_lower))
		return false;
	
	return sortByTitle(a,b);
}

bool sortByChannel (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->epgChannel.begin(), a->epgChannel.end(), b->epgChannel.begin(), b->epgChannel.end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->epgChannel.begin(), b->epgChannel.end(), a->epgChannel.begin(), a->epgChannel.end(), compare_to_lower))
		return false;
	
	return sortByTitle(a,b);
}

bool sortByFileName (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if (std::lexicographical_compare(a->file.getFileName().begin(), a->file.getFileName().end(), b->file.getFileName().begin(), b->file.getFileName().end(), compare_to_lower))
		return true;
	if (std::lexicographical_compare(b->file.getFileName().begin(), b->file.getFileName().end(), a->file.getFileName().begin(), a->file.getFileName().end(), compare_to_lower))
		return false;
	
	return a->file.Time < b->file.Time;
}

bool sortByRecordDate (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->file.Time > b->file.Time ;
	else
		return a->file.Time < b->file.Time ;
}

bool sortBySize (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->file.Size > b->file.Size;
	else
		return a->file.Size < b->file.Size;
}

bool sortByAge (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->parentalLockAge > b->parentalLockAge;
	else
		return a->parentalLockAge < b->parentalLockAge;
}

bool sortByQuality (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->quality > b->quality;
	else
		return a->quality < b->quality;
}

bool sortByDir (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->dirItNr > b->dirItNr;
	else
		return a->dirItNr < b->dirItNr;
}

bool sortByLastPlay (const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b)
{
	if(sortDirection)
		return a->dateOfLastPlay > b->dateOfLastPlay;
	else
		return a->dateOfLastPlay < b->dateOfLastPlay;
}

bool (* const sortBy[MB_INFO_MAX_NUMBER+1])(const MI_MOVIE_INFO* a, const MI_MOVIE_INFO* b) =
{
	&sortByFileName ,	//MB_INFO_FILENAME		= 0,
	&sortByDir, 		//MB_INFO_FILEPATH		= 1,
	&sortByTitle, 		//MB_INFO_TITLE			= 2,
	NULL, 			//MB_INFO_SERIE 		= 3,
	&sortByGenre, 		//MB_INFO_INFO1			= 4,
	NULL, 			//MB_INFO_MAJOR_GENRE 		= 5,
	NULL, 			//MB_INFO_MINOR_GENRE 		= 6,
	NULL, 			//MB_INFO_INFO2 		= 7,
	&sortByAge, 		//MB_INFO_PARENTAL_LOCKAGE	= 8,
	&sortByChannel, 	//MB_INFO_CHANNEL		= 9,
	NULL, 			//MB_INFO_BOOKMARK		= 10,
	&sortByQuality, 	//MB_INFO_QUALITY		= 11,
	&sortByLastPlay, 	//MB_INFO_PREVPLAYDATE 		= 12,
	&sortByRecordDate, 	//MB_INFO_RECORDDATE		= 13,
	NULL, 			//MB_INFO_PRODDATE 		= 14,
	NULL, 			//MB_INFO_COUNTRY 		= 15,
	NULL, 			//MB_INFO_GEOMETRIE 		= 16,
	NULL, 			//MB_INFO_AUDIO 		= 17,
	NULL, 			//MB_INFO_LENGTH 		= 18,
	&sortBySize, 		//MB_INFO_SIZE 			= 19, 
	NULL			//MB_INFO_MAX_NUMBER		= 20
};

CMovieBrowser::CMovieBrowser(const char* path): configfile ('\t')
{
	m_selectedDir = path; 
	//addDir(m_selectedDir);
	CMovieBrowser();
}

CMovieBrowser::CMovieBrowser(): configfile ('\t')
{
	dprintf(DEBUG_NORMAL, "$Id: moviebrowser.cpp,v 1.10 2016/01/03 19:28:30 mohousch Exp $\r\n");
	init();
}

CMovieBrowser::~CMovieBrowser()
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::del:\r\n");
	
	m_dir.clear();

	m_dirNames.clear();
	
	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		m_vMovieInfo[i].audioPids.clear();
	}
	
	m_vMovieInfo.clear();
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();
	m_vHandleSerienames.clear();
	m_movieSelectionHandler = NULL;

	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
		m_FilterLines.lineArray[i].clear();
	}
}

void CMovieBrowser::fileInfoStale(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::fileInfoStale:\n");
	
	m_file_info_stale = true;
	m_seriename_stale = true;
	
	 // Also release memory buffers, since we have to reload this stuff next time anyhow 
	m_dirNames.clear();
	
	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		m_vMovieInfo[i].audioPids.clear();
	}
	
	m_vMovieInfo.clear();
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();
	m_vHandleSerienames.clear();
	m_movieSelectionHandler = NULL;
	
	//
	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
		m_FilterLines.lineArray[i].clear();
	}
}

void CMovieBrowser::init(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::init:\r\n");
	
	initGlobalSettings();
	
	// load settings
	loadSettings(&m_settings);
		
	m_file_info_stale = true;
	m_seriename_stale = true;

	m_pcWindow = CFrameBuffer::getInstance();
	
	widget = NULL;
	
	headers = NULL;
	m_pcBrowser = NULL;
	m_pcLastPlay = NULL;
	m_pcLastRecord = NULL;
	m_pcInfo = NULL;
	footers = NULL;
	
	m_windowFocus = MB_FOCUS_BROWSER;
	
	m_textTitle = _("Movie Browser");
	
	//
	m_movieSelectionHandler = NULL;
	m_currentBrowserSelection = 0;
	m_currentRecordSelection = 0;
	m_currentPlaySelection = 0;
 	m_prevBrowserSelection = 0;
	m_prevRecordSelection = 0;
	m_prevPlaySelection = 0;
	
	m_storageType = MB_STORAGE_TYPE_NFS;
    
	m_parentalLock = m_settings.parentalLock;
	
	// check g_setting values 
	if(m_settings.gui >= MB_GUI_MAX_NUMBER)
		m_settings.gui = MB_GUI_MOVIE_INFO;
	
	if(m_settings.sorting.direction >= MB_DIRECTION_MAX_NUMBER)
		m_settings.sorting.direction = MB_DIRECTION_DOWN;
	if(m_settings.sorting.item >=  MB_INFO_MAX_NUMBER)
		m_settings.sorting.item =  MB_INFO_TITLE;

	if(m_settings.filter.item >= MB_INFO_MAX_NUMBER)
		m_settings.filter.item = MB_INFO_MAX_NUMBER;
	
	if(m_settings.parentalLockAge >= MI_PARENTAL_MAX_NUMBER)
		m_settings.parentalLockAge = MI_PARENTAL_OVER18;
	if(m_settings.parentalLock >= MB_PARENTAL_LOCK_MAX_NUMBER)
		m_settings.parentalLock = MB_PARENTAL_LOCK_OFF;
	
	if(m_settings.browserFrameHeight < MIN_BROWSER_FRAME_HEIGHT )
       		m_settings.browserFrameHeight = MIN_BROWSER_FRAME_HEIGHT;
	if(m_settings.browserFrameHeight > MAX_BROWSER_FRAME_HEIGHT)
        	m_settings.browserFrameHeight = MAX_BROWSER_FRAME_HEIGHT;
	
	// Browser List 
	if(m_settings.browserRowNr == 0)
	{
		dprintf(DEBUG_NORMAL, " row error\r\n");
		
		// init browser row elements if not configured correctly by neutrino.config
		m_settings.browserRowNr = 6;
		m_settings.browserRowItem[0] = MB_INFO_TITLE;
		m_settings.browserRowItem[1] = MB_INFO_INFO1;
		m_settings.browserRowItem[2] = MB_INFO_RECORDDATE;
		m_settings.browserRowItem[3] = MB_INFO_SIZE;
		m_settings.browserRowItem[4] = MB_INFO_PARENTAL_LOCKAGE;
		m_settings.browserRowItem[5] = MB_INFO_QUALITY;
		m_settings.browserRowWidth[0] = m_defaultRowWidth[m_settings.browserRowItem[0]];		//300;
		m_settings.browserRowWidth[1] = m_defaultRowWidth[m_settings.browserRowItem[1]]; 		//100;
		m_settings.browserRowWidth[2] = m_defaultRowWidth[m_settings.browserRowItem[2]]; 		//80;
		m_settings.browserRowWidth[3] = m_defaultRowWidth[m_settings.browserRowItem[3]]; 		//50;
		m_settings.browserRowWidth[4] = m_defaultRowWidth[m_settings.browserRowItem[4]]; 		//30;
		m_settings.browserRowWidth[5] = m_defaultRowWidth[m_settings.browserRowItem[5]]; 		//30;
	}

	initFrames();
	initRows();
	//initDevelopment();

	refreshLastPlayList();	
	refreshLastRecordList();	
	refreshBrowserList();	
	refreshFilterList();
}

void CMovieBrowser::initGlobalSettings(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::initGlobalSettings:\r\n");
	
	m_settings.gui = MB_GUI_MOVIE_INFO;
	
	m_settings.lastPlayMaxItems = NUMBER_OF_MOVIES_LAST;
	m_settings.lastRecordMaxItems = NUMBER_OF_MOVIES_LAST;
	
	m_settings.sorting.direction = MB_DIRECTION_DOWN;
	m_settings.sorting.item =  MB_INFO_RECORDDATE;

	m_settings.filter.item = MB_INFO_MAX_NUMBER;
	m_settings.filter.optionString = "";
	m_settings.filter.optionVar = 0;
	
	m_settings.parentalLockAge = MI_PARENTAL_OVER18;
	m_settings.parentalLock = MB_PARENTAL_LOCK_OFF;
	
	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		m_settings.storageDir[i] = "";
		m_settings.storageDirUsed[i] = 0;
	}

	// Browser List
	m_settings.browserFrameHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20 - ((g_settings.screen_EndY - g_settings.screen_StartY - 20)>>1) - (INTER_FRAME_SPACE>>1);
	
	m_settings.browserRowNr = 6;
	m_settings.browserRowItem[0] = MB_INFO_TITLE;
	m_settings.browserRowItem[1] = MB_INFO_INFO1;
	m_settings.browserRowItem[2] = MB_INFO_RECORDDATE;
	m_settings.browserRowItem[3] = MB_INFO_SIZE;
	m_settings.browserRowItem[4] = MB_INFO_PARENTAL_LOCKAGE;
	m_settings.browserRowItem[5] = MB_INFO_QUALITY;
	m_settings.browserRowWidth[0] = m_defaultRowWidth[m_settings.browserRowItem[0]];		//300;
	m_settings.browserRowWidth[1] = m_defaultRowWidth[m_settings.browserRowItem[1]]; 		//100;
	m_settings.browserRowWidth[2] = m_defaultRowWidth[m_settings.browserRowItem[2]]; 		//80;
	m_settings.browserRowWidth[3] = m_defaultRowWidth[m_settings.browserRowItem[3]]; 		//50;
	m_settings.browserRowWidth[4] = m_defaultRowWidth[m_settings.browserRowItem[4]]; 		//30;
	m_settings.browserRowWidth[5] = m_defaultRowWidth[m_settings.browserRowItem[5]]; 		//30;
	
	//
	m_settings.storageDirMovieUsed = true;
	m_settings.storageDirRecUsed = true;
	m_settings.reload = true;	// not used reload for first time or when show mode changes
	m_settings.remount = false;
	m_settings.browser_serie_mode = 0;
	m_settings.serie_auto_create = 0;
}

void CMovieBrowser::initFrames(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::initFrames:\r\n");
	
	m_pcFontFoot  = FOOT_FONT;
	m_pcFontTitle = TITLE_FONT;
	
	m_cBoxFrame.iX = 			g_settings.screen_StartX + 10;
	m_cBoxFrame.iY = 			g_settings.screen_StartY + 10;
	m_cBoxFrame.iWidth = 			g_settings.screen_EndX - g_settings.screen_StartX - 20;
	m_cBoxFrame.iHeight = 			g_settings.screen_EndY - g_settings.screen_StartY - 20;

	m_cBoxFrameTitleRel.iX =		m_cBoxFrame.iX;
	m_cBoxFrameTitleRel.iY = 		m_cBoxFrame.iY;
	m_cBoxFrameTitleRel.iWidth = 		m_cBoxFrame.iWidth;
	m_cBoxFrameTitleRel.iHeight = 		m_pcFontTitle->getHeight();

	m_cBoxFrameBrowserList.iX = 		m_cBoxFrame.iX;
	m_cBoxFrameBrowserList.iY = 		m_cBoxFrame.iY + m_cBoxFrameTitleRel.iHeight;
	m_cBoxFrameBrowserList.iWidth = 	m_cBoxFrame.iWidth;
	m_cBoxFrameBrowserList.iHeight = 	m_settings.browserFrameHeight;

	m_cBoxFrameFootRel.iX = 		m_cBoxFrame.iX;
	m_cBoxFrameFootRel.iY = 		m_cBoxFrame.iY + m_cBoxFrame.iHeight - m_pcFontFoot->getHeight()*2;//FIXME
	m_cBoxFrameFootRel.iWidth = 		m_cBoxFrameBrowserList.iWidth;
	m_cBoxFrameFootRel.iHeight = 		m_pcFontFoot->getHeight()*2;//FIXME

	m_cBoxFrameLastPlayList.iX = 		m_cBoxFrameBrowserList.iX;
	m_cBoxFrameLastPlayList.iY = 		m_cBoxFrameBrowserList.iY ;
	m_cBoxFrameLastPlayList.iWidth = 	(m_cBoxFrameBrowserList.iWidth>>1) - (INTER_FRAME_SPACE>>1);
	m_cBoxFrameLastPlayList.iHeight = 	m_cBoxFrameBrowserList.iHeight;
	
	m_cBoxFrameLastRecordList.iX = 		m_cBoxFrameLastPlayList.iX + m_cBoxFrameLastPlayList.iWidth + INTER_FRAME_SPACE;
	m_cBoxFrameLastRecordList.iY = 		m_cBoxFrameLastPlayList.iY;
	m_cBoxFrameLastRecordList.iWidth = 	m_cBoxFrame.iWidth - m_cBoxFrameLastPlayList.iWidth - INTER_FRAME_SPACE;
	m_cBoxFrameLastRecordList.iHeight =	m_cBoxFrameLastPlayList.iHeight;
	
	m_cBoxFrameInfo.iX = 			m_cBoxFrameBrowserList.iX;
	m_cBoxFrameInfo.iY = 			m_cBoxFrameBrowserList.iY + m_cBoxFrameBrowserList.iHeight + INTER_FRAME_SPACE;
	m_cBoxFrameInfo.iWidth = 		m_cBoxFrameBrowserList.iWidth;
	m_cBoxFrameInfo.iHeight = 		m_cBoxFrame.iHeight - m_cBoxFrameBrowserList.iHeight - INTER_FRAME_SPACE - m_cBoxFrameFootRel.iHeight - m_cBoxFrameTitleRel.iHeight;

	m_cBoxFrameFilter.iX = 			m_cBoxFrameInfo.iX;
	m_cBoxFrameFilter.iY = 			m_cBoxFrameInfo.iY;
	m_cBoxFrameFilter.iWidth = 		m_cBoxFrameInfo.iWidth;
	m_cBoxFrameFilter.iHeight = 		m_cBoxFrameInfo.iHeight;
}

void CMovieBrowser::initRows(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::initRows:\r\n");

	// Last Play List
	m_settings.lastPlayRowNr = 2;
	m_settings.lastPlayRow[0] = MB_INFO_TITLE;
	m_settings.lastPlayRow[1] = MB_INFO_PREVPLAYDATE;
	m_settings.lastPlayRow[2] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[3] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[4] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRow[5] = MB_INFO_MAX_NUMBER;
	m_settings.lastPlayRowWidth[0] = 190;
	m_settings.lastPlayRowWidth[1]  = m_defaultRowWidth[m_settings.lastPlayRow[1]];
	m_settings.lastPlayRowWidth[2] = m_defaultRowWidth[m_settings.lastPlayRow[2]];
	m_settings.lastPlayRowWidth[3] = m_defaultRowWidth[m_settings.lastPlayRow[3]];
	m_settings.lastPlayRowWidth[4] = m_defaultRowWidth[m_settings.lastPlayRow[4]];
	m_settings.lastPlayRowWidth[5] = m_defaultRowWidth[m_settings.lastPlayRow[5]];
	
	// Last Record List
	m_settings.lastRecordRowNr = 2;
	m_settings.lastRecordRow[0] = MB_INFO_TITLE;
	m_settings.lastRecordRow[1] = MB_INFO_RECORDDATE;
	m_settings.lastRecordRow[2] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[3] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[4] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRow[5] = MB_INFO_MAX_NUMBER;
	m_settings.lastRecordRowWidth[0] = 190;
	m_settings.lastRecordRowWidth[1] = m_defaultRowWidth[m_settings.lastRecordRow[1]];
	m_settings.lastRecordRowWidth[2] = m_defaultRowWidth[m_settings.lastRecordRow[2]];
	m_settings.lastRecordRowWidth[3] = m_defaultRowWidth[m_settings.lastRecordRow[3]];
	m_settings.lastRecordRowWidth[4] = m_defaultRowWidth[m_settings.lastRecordRow[4]];
	m_settings.lastRecordRowWidth[5] = m_defaultRowWidth[m_settings.lastRecordRow[5]];
}
 
void CMovieBrowser::initDevelopment(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::initDevelopment:\r\n");
	
	std::string name;
	name = "/mnt/movie/";
	//addDir(name);
	name = "/mnt/record/";
	//addDir(name);
	name = "/mnt/nfs/";
	//addDir(name);	
}

void CMovieBrowser::defaultSettings(MB_SETTINGS *settings)
{
	CFile file;
	file.Name = MOVIEBROWSER_SETTINGS_FILE;
	delFile(file);
	configfile.clear();
	loadSettings(settings);
}

bool CMovieBrowser::loadSettings(MB_SETTINGS *settings)
{
	bool result = true;
	
	dprintf(DEBUG_NORMAL, "CMovieBrowser::loadSettings\r\n");
	
	if(configfile.loadConfig(MOVIEBROWSER_SETTINGS_FILE))
	{
		settings->lastPlayMaxItems = configfile.getInt32("mb_lastPlayMaxItems", NUMBER_OF_MOVIES_LAST);
		settings->lastRecordMaxItems = configfile.getInt32("mb_lastRecordMaxItems", NUMBER_OF_MOVIES_LAST);
		settings->browser_serie_mode = configfile.getInt32("mb_browser_serie_mode", 0);
		settings->serie_auto_create = configfile.getInt32("mb_serie_auto_create", 0);

		settings->gui = (MB_GUI)configfile.getInt32("mb_gui", MB_GUI_MOVIE_INFO);
			
		settings->sorting.item = (MB_INFO_ITEM)configfile.getInt32("mb_sorting_item", MB_INFO_TITLE);
		settings->sorting.direction = (MB_DIRECTION)configfile.getInt32("mb_sorting_direction", MB_DIRECTION_UP);
			
		settings->filter.item = (MB_INFO_ITEM)configfile.getInt32("mb_filter_item", MB_INFO_INFO1);
		settings->filter.optionString = configfile.getString("mb_filter_optionString", "");
		settings->filter.optionVar = configfile.getInt32("mb_filter_optionVar", 0);
			
		settings->parentalLockAge = (MI_PARENTAL_LOCKAGE)configfile.getInt32("mb_parentalLockAge", MI_PARENTAL_OVER18);
		settings->parentalLock = (MB_PARENTAL_LOCK)configfile.getInt32("mb_parentalLock", MB_PARENTAL_LOCK_ACTIVE);
		
		settings->storageDirRecUsed = (bool)configfile.getInt32("mb_storageDir_rec", true );
		settings->storageDirMovieUsed = (bool)configfile.getInt32("mb_storageDir_movie", true );

		settings->remount = (bool)configfile.getInt32("mb_remount", false );

		char cfg_key[81];
		for(int i = 0; i < MB_MAX_DIRS; i++)
		{
			sprintf(cfg_key, "mb_dir_%d", i);
			settings->storageDir[i] = configfile.getString( cfg_key, "" );
			sprintf(cfg_key, "mb_dir_used%d", i);
			settings->storageDirUsed[i] = configfile.getInt32( cfg_key, false );
		}
		
		// these variables are used for the listframes
		settings->browserFrameHeight  = configfile.getInt32("mb_browserFrameHeight", 250);
		settings->browserRowNr  = configfile.getInt32("mb_browserRowNr", 0);
		
		for(int i = 0; i < MB_MAX_ROWS && i < settings->browserRowNr; i++)
		{
			sprintf(cfg_key, "mb_browserRowItem_%d", i);
			settings->browserRowItem[i] = (MB_INFO_ITEM)configfile.getInt32(cfg_key, MB_INFO_MAX_NUMBER);
			sprintf(cfg_key, "mb_browserRowWidth_%d", i);
			settings->browserRowWidth[i] = configfile.getInt32(cfg_key, 50);
		}
	}
	else
	{
		dprintf(DEBUG_NORMAL, "CMovieBrowser::loadSettings failed\r\n"); 
		configfile.clear();
		result = false;
	}
	
	return (result);
}

bool CMovieBrowser::saveSettings(MB_SETTINGS *settings)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::saveSettings\r\n"); 

	bool result = true;
	dprintf(DEBUG_NORMAL, "[mb] saveSettings\r\n");

	configfile.setInt32("mb_lastPlayMaxItems", settings->lastPlayMaxItems);
	configfile.setInt32("mb_lastRecordMaxItems", settings->lastRecordMaxItems);
	configfile.setInt32("mb_browser_serie_mode", settings->browser_serie_mode);
	configfile.setInt32("mb_serie_auto_create", settings->serie_auto_create);

	configfile.setInt32("mb_gui", settings->gui);
	
	configfile.setInt32("mb_sorting_item", settings->sorting.item);
	configfile.setInt32("mb_sorting_direction", settings->sorting.direction);
	
	configfile.setInt32("mb_filter_item", settings->filter.item);
	configfile.setString("mb_filter_optionString", settings->filter.optionString);
	configfile.setInt32("mb_filter_optionVar", settings->filter.optionVar);
	
	configfile.setInt32("mb_storageDir_rec", settings->storageDirRecUsed );
	configfile.setInt32("mb_storageDir_movie", settings->storageDirMovieUsed );

	configfile.setInt32("mb_parentalLockAge", settings->parentalLockAge);
	configfile.setInt32("mb_parentalLock", settings->parentalLock);

	configfile.setInt32("mb_remount", settings->remount);

	char cfg_key[81];
	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		sprintf(cfg_key, "mb_dir_%d", i);
		configfile.setString( cfg_key, settings->storageDir[i] );
		sprintf(cfg_key, "mb_dir_used%d", i);
		configfile.setInt32( cfg_key, settings->storageDirUsed[i] ); // do not save this so far
	}
	
	// these variables are used for the listframes
	configfile.setInt32("mb_browserFrameHeight", settings->browserFrameHeight);
	configfile.setInt32("mb_browserRowNr",settings->browserRowNr);
	
	for(int i = 0; i < MB_MAX_ROWS && i < settings->browserRowNr; i++)
	{
		sprintf(cfg_key, "mb_browserRowItem_%d", i);
		configfile.setInt32(cfg_key, settings->browserRowItem[i]);
		sprintf(cfg_key, "mb_browserRowWidth_%d", i);
		configfile.setInt32(cfg_key, settings->browserRowWidth[i]);
	}
 
 	if (configfile.getModifiedFlag())
		configfile.saveConfig(MOVIEBROWSER_SETTINGS_FILE);
	
	return (result);
}

int CMovieBrowser::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::exec: actionKey:%s\n", actionKey.c_str());

	int returnval = RETURN_REPAINT;

	if(actionKey == "loaddefault")
	{
		CHintBox * hintBox = new CHintBox(_("Moviebrowser"), _("Load default settings"));
		hintBox->paint();
		
		defaultSettings(&m_settings);
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
	}
	else if(actionKey == "save_options")
	{
		CHintBox * hintBox = new CHintBox(_("Moviebrowser"), _("Save settings now")); // UTF-8

		hintBox->paint();
		
		saveSettings(&m_settings);
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
	}
	else if(actionKey == "show_movie_info_menu")
	{
		if(m_movieSelectionHandler != NULL)
			showMovieInfoMenu(m_movieSelectionHandler);
	}
	else if(actionKey == "save_movie_info")
	{
		if(m_movieSelectionHandler != NULL)
		{
			//
			CHintBox * hintBox = new CHintBox(_("Moviebrowser"), _("save settings now")); // UTF-8
			hintBox->paint();
		
			m_movieInfo.saveMovieInfo( *m_movieSelectionHandler);
			
			hintBox->hide();
			delete hintBox;
		}
	}
	else if(actionKey == "save_movie_info_all")
	{
		std::vector<MI_MOVIE_INFO*> * current_list = NULL;

		if(m_movieSelectionHandler != NULL)
		{
			if(m_windowFocus == MB_FOCUS_BROWSER)          
				current_list = &m_vHandleBrowserList;
			else if(m_windowFocus == MB_FOCUS_LAST_PLAY)   
				current_list = &m_vHandlePlayList;
			else if(m_windowFocus == MB_FOCUS_LAST_RECORD) 
				current_list = &m_vHandleRecordList ;
			
			//
			CHintBox * hintBox = new CHintBox(_("Moviebrowser"), _("Save settings now")); // UTF-8
			hintBox->paint();

			if(current_list != NULL)
			{
				CHintBox loadBox(_("Moviebrowser"), _("Save changes in all movie info files"));
				loadBox.paint();
				
				for(unsigned int i = 0; i< current_list->size();i++)
				{
					if( !((*current_list)[i]->parentalLockAge != 0 && movieInfoUpdateAllIfDestEmptyOnly == true) && movieInfoUpdateAll[MB_INFO_TITLE] )
						(*current_list)[i]->parentalLockAge = m_movieSelectionHandler->parentalLockAge;

					if( !(!(*current_list)[i]->serieName.empty() && movieInfoUpdateAllIfDestEmptyOnly == true) && movieInfoUpdateAll[MB_INFO_SERIE] )
						(*current_list)[i]->serieName = m_movieSelectionHandler->serieName;

					if( !(!(*current_list)[i]->productionCountry.empty() && movieInfoUpdateAllIfDestEmptyOnly == true) && movieInfoUpdateAll[MB_INFO_COUNTRY] )
						(*current_list)[i]->productionCountry = m_movieSelectionHandler->productionCountry;

					if( !((*current_list)[i]->genreMajor!=0 && movieInfoUpdateAllIfDestEmptyOnly == true) && movieInfoUpdateAll[MB_INFO_MAJOR_GENRE] )
						(*current_list)[i]->genreMajor = m_movieSelectionHandler->genreMajor;

					if( !((*current_list)[i]->quality!=0 && movieInfoUpdateAllIfDestEmptyOnly == true) && movieInfoUpdateAll[MB_INFO_QUALITY] )
						(*current_list)[i]->quality = m_movieSelectionHandler->quality;

					m_movieInfo.saveMovieInfo( *((*current_list)[i]) );
				}
				loadBox.hide();
			}
			
			//
			hintBox->hide();
			delete hintBox;
		}
	}
	else if(actionKey == "reload_movie_info")
	{
		loadMovies();
		updateMovieSelection();
		refresh();
	}
	else if(actionKey == "run")
	{
		if(parent) 
			parent->hide ();
		
		exec(NULL);
	}
	else if(actionKey == "book_clear_all")
	{
		if(m_movieSelectionHandler != NULL)
		{
			m_movieSelectionHandler->bookmarks.start = 0;
			m_movieSelectionHandler->bookmarks.end = 0;
			m_movieSelectionHandler->bookmarks.lastPlayStop = 0;
			
			for(int i = 0; i < MI_MOVIE_BOOK_USER_MAX; i++)
			{
				m_movieSelectionHandler->bookmarks.user[i].name.empty();
				m_movieSelectionHandler->bookmarks.user[i].length = 0;
				m_movieSelectionHandler->bookmarks.user[i].pos = 0;
			}
		}
	}
	else if(actionKey == "cut_jumps")
	{
		if(m_movieSelectionHandler != NULL)
		{
			if(MessageBox(_("Information"), _("Cut jumps from movie ?"), mbrNo, mbYes | mbNo) == mbrYes) 
			{
				CHintBox * hintBox = new CHintBox(_("Moviebrowser"), _("Cutting, please wait"));
				hintBox->paint();
				sleep(1);
				hintBox->hide();
				delete hintBox;
				//m_pcWindow->paintBackground(); // clear screen
				//m_pcWindow->blit();
		
				off64_t res = cut_movie(m_movieSelectionHandler, &m_movieInfo);

				if(res == 0)
					MessageBox(_("Error"), _("Cut failed, is there jump bookmarks ? Or check free space."), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			}
		}
	}
	else if(actionKey == "truncate_jumps")
	{
		if(m_movieSelectionHandler != NULL) 
		{
			if((m_movieSelectionHandler == playing_info) && (NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode()))
				MessageBox(_("Error"), _("Impossible to truncate playing movie."), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			else if(m_movieSelectionHandler->bookmarks.end == 0)
				MessageBox(_("Error"), _("No End bookmark defined!"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			else 
			{
				if(MessageBox(_("Information"), _("Truncate movie ?"), mbrNo, mbYes | mbNo) == mbrYes) 
				{
					CHintBox * hintBox = new CHintBox(_("Information"), _("Truncating, please wait"));
					hintBox->paint();
					off64_t res = truncate_movie(m_movieSelectionHandler);
					hintBox->hide();
					delete hintBox;

					if(res == 0)
						MessageBox(_("Error"), _("Truncate failed."), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
				}
			}
		}
	}
	else if(actionKey == "copy_jumps")
	{
		if(m_movieSelectionHandler != NULL)
		{

			if(MessageBox(_("Information"), _("Copy jumps from movie to new file ?"), mbrNo, mbYes | mbNo) == mbrYes) 
			{
				CHintBox * hintBox = new CHintBox(_("Information"), _("Coping, please wait"));
				hintBox->paint();
				sleep(1);
				hintBox->hide();
				delete hintBox;
				//m_pcWindow->paintBackground(); // clear screen
				//m_pcWindow->blit();	

				off64_t res = copy_movie(m_movieSelectionHandler, &m_movieInfo, true);
			
				if(res == 0)
					MessageBox(_("Information"), _("Copy failed, is there jump bookmarks ? Or check free space."), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			}
		}
	}
	else if(actionKey == "tmdb")
	{
		if(m_movieSelectionHandler != NULL)
		{
			m_pcWindow->paintBackground();
			m_pcWindow->blit();

			//				
			CTmdb * tmdb = new CTmdb();
			if(tmdb->getMovieInfo(m_movieSelectionHandler->epgTitle))
			{
				if ((!tmdb->getDescription().empty())) 
				{
					std::string buffer;

					buffer = m_movieSelectionHandler->epgTitle;
					buffer += "\n";
	
					// prepare print buffer  
					buffer += tmdb->createInfoText();
	
					//
					std::string tname = tmdb->getThumbnailDir();
					tname += "/";
					tname += m_movieSelectionHandler->epgTitle;
					tname += ".jpg";

					tmdb->getSmallCover(tmdb->getPosterPath(), tname);
				
					// scale pic
					int p_w = 0;
					int p_h = 0;
				
					scaleImage(tname, &p_w, &p_h);

					CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
					CInfoBox * infoBox = new CInfoBox(&position, m_movieSelectionHandler->epgTitle.c_str(), NEUTRINO_ICON_TMDB);

					infoBox->setText(buffer.c_str(), tname.c_str(), p_w, p_h);
					infoBox->exec();
					delete infoBox;

					if(MessageBox(_("Information"), _("Prefer TMDB infos"), mbrNo, mbYes | mbNo) == mbrYes) 
					{
						// rewrite tfile
						std::string tname = m_movieSelectionHandler->file.Name;
						changeFileNameExt(tname, ".jpg");
						if(tmdb->getSmallCover(tmdb->getPosterPath(), tname)) 
							m_movieSelectionHandler->tfile = tname;

						if(m_movieSelectionHandler->epgInfo1.empty())
							m_movieSelectionHandler->epgInfo1 = tmdb->getDescription();

						m_movieInfo.saveMovieInfo( *m_movieSelectionHandler);
					}  
				}
				else
				{
					MessageBox(_("Information"), _("Not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
				}
			}
			else
			{
				MessageBox(_("Information"), _("Not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
			}

			delete tmdb;
			tmdb = NULL;
		}
	}
	else if(actionKey == "remove_screenshot")
	{
		if (m_movieSelectionHandler != NULL) 
		{
                	if(MessageBox(_("Information"), _("remove screenshot?"), mbrNo, mbYes | mbNo) == mbrYes) 
			{
                        	std::string fname = m_movieSelectionHandler->file.Name;
				
				int ext_pos = 0;
				ext_pos = fname.rfind('.');
				if( ext_pos > 0)
				{
					std::string extension;
					extension = fname.substr(ext_pos + 1, fname.length() - ext_pos);
					extension = "." + extension;
					strReplace(fname, extension.c_str(), ".jpg");
				}
				
                        	unlink(fname.c_str());
			}
          	}
	}
	
	return returnval;
}

int CMovieBrowser::exec(const char * path)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::exec:\n");

	bool res = false;
	
	int timeout = -1;
	int returnDefaultOnTimeout = true;
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, _("Movie Browser"));
	
	// load settings
	loadSettings(&m_settings);
		
	// reload movie 
	dprintf(DEBUG_NORMAL, "CMovieBrowser::::exec: force reload:\r\n");
	fileInfoStale();
	
	// init frames
	initFrames();

	// Clear all, to avoid 'jump' in screen 
	m_vHandleBrowserList.clear();
	m_vHandleRecordList.clear();
	m_vHandlePlayList.clear();
	m_movieSelectionHandler = NULL;

	for(int i = 0; i < LF_MAX_ROWS; i++)
	{
		m_browserListLines.lineArray[i].clear();
		m_recordListLines.lineArray[i].clear();
		m_playListLines.lineArray[i].clear();
	}

	m_selectedDir = path; 

	// paint mb
	if(paint() == false)
		return res;// paint failed due to less memory , exit 

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER];

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	// remount
	if(m_settings.remount == true)
	{
		dprintf(DEBUG_NORMAL, "CMovieBrowser::::exec: remount:\r\n");
		
		//umount automount dirs
		for(int i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			if(g_settings.network_nfs_automount[i])
				umount2(g_settings.network_nfs_local_dir[i],MNT_FORCE);
		}
		
		// mount
		CFSMounter::automount();
	}
	
	// reload movies
	if(m_file_info_stale == true)
	{
		dprintf(DEBUG_NORMAL, "CMovieBrowser::::exec: reload:\r\n");
		loadMovies();
	}
	else
	{
		// since we cleared everything above, we have to refresh the list now.
		refreshBrowserList();	
		refreshLastPlayList();	
		refreshLastRecordList();
	}

	// get old movie selection and set position in windows	
	m_currentBrowserSelection = m_prevBrowserSelection;
	m_currentRecordSelection = m_prevRecordSelection;
	m_currentPlaySelection = m_prevPlaySelection;

	m_pcBrowser->setSelectedLine(m_currentBrowserSelection);
	m_pcLastRecord->setSelectedLine(m_currentRecordSelection);
	m_pcLastPlay->setSelectedLine(m_currentPlaySelection);

	// update movie selection
	updateMovieSelection();
	
	// on set guiwindow
	onSetGUIWindow(m_settings.gui);

/*
	// refresh title
	refreshTitle();
	
	// on set guiwindow
	//onSetGUIWindow(m_settings.gui);
	
	// browser paint 
	m_pcBrowser->paint();
*/
	refresh();
	
	m_pcWindow->blit();

	bool loop = true;
	bool result;
	
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		result = onButtonPress(msg);
		
		if(result == false)
		{
			if (msg == RC_timeout && returnDefaultOnTimeout)
			{
				dprintf(DEBUG_DEBUG, "CMovieBrowser::exec: Timerevent\n");
				
				loop = false;
			}
			else if(msg == RC_ok)
			{
				if(m_movieSelectionHandler != NULL)
				{
					playing_info = m_movieSelectionHandler;
					res = true;
					loop = false;
				}
			}
			else if (msg == RC_home)
			{
				loop = false;
			}
			else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			{
				loop = false;
			}
		}
		
		m_pcWindow->blit();	

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(timeout); // calcualate next timeout
	}
	
	hide();
	
	//
	m_prevBrowserSelection = m_currentBrowserSelection;
	m_prevRecordSelection = m_currentRecordSelection;
	m_prevPlaySelection = m_currentPlaySelection;

	saveSettings(&m_settings);	// might be better done in ~CMovieBrowser, but for any reason this does not work if MB is killed by neutrino shutdown	
	
	return (res);
}

void CMovieBrowser::hide(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::Hide\r\n");
	
	
	//m_pcWindow->paintBackground();
	//m_pcWindow->blit();
	
	widget->hide();
	
	if (m_pcFilter != NULL)
	{
		m_currentFilterSelection  = m_pcFilter->getSelectedLine();
		delete m_pcFilter;
		m_pcFilter = NULL;
	}
	
	if (m_pcBrowser != NULL)
	{
		m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
		delete m_pcBrowser;
		m_pcBrowser = NULL;
	}
	
	if (m_pcLastPlay != NULL)
	{
		m_currentPlaySelection = m_pcLastPlay->getSelectedLine();
		delete m_pcLastPlay;
		m_pcLastPlay = NULL;
	}
	
	if (m_pcLastRecord != NULL) 
	{
		m_currentRecordSelection = m_pcLastRecord->getSelectedLine();
		delete m_pcLastRecord;
		m_pcLastRecord = NULL;
	}
	
	if (m_pcInfo != NULL) 
	{
		delete m_pcInfo;
		m_pcInfo = NULL;
	}
}

int CMovieBrowser::paint(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::Paint\r\n");

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, _("Movie Browser"));
	
	widget = new CWidget(&m_cBoxFrame);
	
	headers = new CHeaders(&m_cBoxFrameTitleRel);

	m_pcBrowser = new CListFrame(&m_browserListLines, NULL, CListFrame::SCROLL | CListFrame::HEADER_LINE, &m_cBoxFrameBrowserList);

	m_pcLastPlay = new CListFrame(&m_playListLines, NULL, CListFrame::SCROLL | CListFrame::HEADER_LINE | CListFrame::TITLE, &m_cBoxFrameLastPlayList, _("Last played:"), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);

	m_pcLastRecord = new CListFrame(&m_recordListLines, NULL, CListFrame::SCROLL | CListFrame::HEADER_LINE | CListFrame::TITLE, &m_cBoxFrameLastRecordList, _("Last recorded:"), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);

	m_pcFilter = new CListFrame(&m_FilterLines, NULL, CListFrame::SCROLL | CListFrame::TITLE, &m_cBoxFrameFilter, _("Filter movies by category:"), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]);

	m_pcInfo = new CTextBox(&m_cBoxFrameInfo);
	
	footers = new CFooters(&m_cBoxFrameFootRel);

	if(m_pcBrowser == NULL || m_pcLastPlay == NULL || m_pcLastRecord == NULL || m_pcInfo == NULL || m_pcFilter == NULL)
	{
		
		if (m_pcFilter != NULL)
			delete m_pcFilter;

		if (m_pcBrowser != NULL)
			delete m_pcBrowser;

		if (m_pcLastPlay != NULL) 
			delete m_pcLastPlay;

		if (m_pcLastRecord != NULL)
			delete m_pcLastRecord;

		if (m_pcInfo != NULL) 
			delete m_pcInfo;

		m_pcInfo = NULL;
		m_pcLastPlay = NULL;
		m_pcLastRecord = NULL;
		m_pcBrowser = NULL;
		m_pcFilter = NULL;

		return (false);
	}
	
	widget->addWidgetItem(headers);
	widget->addWidgetItem(m_pcBrowser);
	widget->addWidgetItem(m_pcInfo);
	widget->addWidgetItem(footers);
	
	return (true);
}

void CMovieBrowser::refresh(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::refresh\r\n");
	
	refreshTitle();

/*
	if (m_pcBrowser != NULL && m_showBrowserFiles == true )
		 m_pcBrowser->refresh();
	
	if (m_pcLastPlay != NULL && m_showLastPlayFiles == true ) 
		m_pcLastPlay->refresh();
	
	if (m_pcLastRecord != NULL && m_showLastRecordFiles == true)
		 m_pcLastRecord->refresh();
	
	if (m_pcInfo != NULL && m_showMovieInfo == true) 
		refreshMovieInfo();
		//m_pcInfo->refresh();
		
	if (m_pcFilter != NULL && m_showFilter == true) 
		m_pcFilter->refresh();
*/
		
	refreshFoot();
	refreshLCD();
	
	widget->paint();
}

std::string CMovieBrowser::getCurrentDir(void)
{
	return(m_selectedDir);
}

CFile * CMovieBrowser::getSelectedFile(void)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::getSelectedFile: %s\r\n",m_movieSelectionHandler->file.Name.c_str());

	if(m_movieSelectionHandler != NULL)
		return(&m_movieSelectionHandler->file);
	else
		return(NULL);
}

void CMovieBrowser::refreshMovieInfo(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::refreshMovieInfo:\n");
	
	std::string emptytext = " ";
	
	if(m_vMovieInfo.size() <= 0) 
	{
		if(m_pcInfo != NULL)
			m_pcInfo->setText(emptytext.c_str());
		return;
	}
	
	if (m_movieSelectionHandler == NULL)
	{
		// There is no selected element, clear LCD
		m_pcInfo->setText(emptytext.c_str());
	}
	else
	{
		std::string fname = m_movieSelectionHandler->tfile;

		int p_w = 0;
		int p_h = 0;
		
		scaleImage(fname, &p_w, &p_h);
		
		m_pcInfo->setText(m_movieSelectionHandler->epgInfo2.c_str(), fname.c_str(), p_w, p_h);
	}
	
	m_pcInfo->paint();
	m_pcWindow->blit();
}

void CMovieBrowser::refreshLCD(void)
{
	if(m_vMovieInfo.size() <= 0) 
		return;

	//CVFD * lcd = CVFD::getInstance();
	if(m_movieSelectionHandler == NULL)
	{
		// There is no selected element, clear LCD
		//lcd->showMenuText(0, " ", -1, true); // UTF-8
		//lcd->showMenuText(1, " ", -1, true); // UTF-8
	}
	else
	{
		CVFD::getInstance()->showMenuText(0, m_movieSelectionHandler->epgTitle.c_str(), -1, true); // UTF-8
	} 	
}

void CMovieBrowser::refreshFilterList(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::refreshFilterList: %d\r\n",m_settings.filter.item);
		
	std::string string_item;
	
	m_FilterLines.rows = 1;
	m_FilterLines.lineArray[0].clear();
	m_FilterLines.rowWidth[0] = 400;
	m_FilterLines.lineHeader[0] = "";

	if(m_vMovieInfo.size() <= 0) 
		return; // exit here if nothing else is to do

	if(m_settings.filter.item == MB_INFO_MAX_NUMBER)
	{
		// show Main List
		string_item = _("Genre");
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = _("Info 1");
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = _("Path");
		m_FilterLines.lineArray[0].push_back(string_item);
		string_item = _("Serie");
		m_FilterLines.lineArray[0].push_back(string_item);
	}
	else
	{
		std::string tmp = _("back");
		m_FilterLines.lineArray[0].push_back(tmp);
		
		 if(m_settings.filter.item == MB_INFO_FILEPATH)
		{
			for(unsigned int i =0 ; i < m_dirNames.size() ;i++)
			{
				m_FilterLines.lineArray[0].push_back(m_dirNames[i]);
			}
		}
		else if(m_settings.filter.item == MB_INFO_INFO1)
		{
			for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
			{
				bool found = false;
				for(unsigned int t = 0; t < m_FilterLines.lineArray[0].size() && found == false; t++)
				{
					if(strcmp(m_FilterLines.lineArray[0][t].c_str(),m_vMovieInfo[i].epgInfo1.c_str()) == 0)
						found = true;
				}
				if(found == false)
					m_FilterLines.lineArray[0].push_back(m_vMovieInfo[i].epgInfo1);
			}
		}
		else if(m_settings.filter.item == MB_INFO_MAJOR_GENRE)
		{
			for(int i = 0; i < GENRE_ALL_COUNT; i++)
			{
				std::string _tmp = GENRE_ALL[i].valname;
				m_FilterLines.lineArray[0].push_back(_tmp);
			}
		}
		else if(m_settings.filter.item == MB_INFO_SERIE)
		{
			updateSerienames();
			for(unsigned int i = 0; i < m_vHandleSerienames.size(); i++)
			{
				m_FilterLines.lineArray[0].push_back(m_vHandleSerienames[i]->serieName);
			}
		}
	}
	m_pcFilter->setLines(&m_FilterLines);
}

void CMovieBrowser::refreshLastPlayList(void) //P2
{
	dprintf(DEBUG_INFO, "CMovieBrowser::refreshlastPlayList:\r\n");
	
	std::string string_item;

	// Initialise and clear list array
	m_playListLines.rows = m_settings.lastPlayRowNr;
	for(int row = 0 ;row < m_settings.lastPlayRowNr; row++)
	{
		m_playListLines.lineArray[row].clear();
		m_playListLines.rowWidth[row] = m_settings.lastPlayRowWidth[row];
		m_playListLines.lineHeader[row]= m_localizedItemName[m_settings.lastPlayRow[row]];
	}
	m_vHandlePlayList.clear();

	if(m_vMovieInfo.size() <= 0) 
	{
		if(m_pcLastPlay != NULL)
			m_pcLastPlay->setLines(&m_playListLines);
		return; // exit here if nothing else is to do
	}

	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file=0; file < m_vMovieInfo.size(); file++)
	{
		if(/*isFiltered(m_vMovieInfo[file]) == false &&*/ isParentalLock(m_vMovieInfo[file]) == false) 
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandlePlayList.push_back(movie_handle);
		}
	}
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandlePlayList, MB_INFO_PREVPLAYDATE, MB_DIRECTION_DOWN);

	for( int handle = 0; handle < (int) m_vHandlePlayList.size() && handle < m_settings.lastPlayMaxItems ;handle++)
	{
		for(int row = 0; row < m_settings.lastPlayRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandlePlayList[handle], m_settings.lastPlayRow[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.lastPlayRow[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandlePlayList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_playListLines.lineArray[row].push_back(string_item);
		}
	}
	m_pcLastPlay->setLines(&m_playListLines);

	m_currentPlaySelection = m_pcLastPlay->getSelectedLine();
	
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
		updateMovieSelection();	
	}
}

void CMovieBrowser::refreshLastRecordList(void) //P2
{
	dprintf(DEBUG_INFO, "CMovieBrowser::refreshLastRecordList:\r\n");
	
	std::string string_item;

	// Initialise and clear list array
	m_recordListLines.rows = m_settings.lastRecordRowNr;
	for(int row = 0 ;row < m_settings.lastRecordRowNr; row++)
	{
		m_recordListLines.lineArray[row].clear();
		m_recordListLines.rowWidth[row] = m_settings.lastRecordRowWidth[row];
		m_recordListLines.lineHeader[row]= m_localizedItemName[m_settings.lastRecordRow[row]];
	}
	m_vHandleRecordList.clear();

	if(m_vMovieInfo.size() <= 0) 
	{
		if(m_pcLastRecord != NULL)
			m_pcLastRecord->setLines(&m_recordListLines);
		return; // exit here if nothing else is to do
	}

	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file=0; file < m_vMovieInfo.size()  ;file++)
	{
		if(/*isFiltered(m_vMovieInfo[file]) == false &&*/ isParentalLock(m_vMovieInfo[file]) == false) 
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandleRecordList.push_back(movie_handle);
		}
	}
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandleRecordList,MB_INFO_RECORDDATE, MB_DIRECTION_DOWN);

	for( int handle=0; handle < (int) m_vHandleRecordList.size() && handle < m_settings.lastRecordMaxItems ;handle++)
	{
		for(int row = 0; row < m_settings.lastRecordRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandleRecordList[handle], m_settings.lastRecordRow[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.lastRecordRow[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandleRecordList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_recordListLines.lineArray[row].push_back(string_item);
		}
	}

	m_pcLastRecord->setLines(&m_recordListLines);

	m_currentRecordSelection = m_pcLastRecord->getSelectedLine();
	
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
		updateMovieSelection();	
	}
}

void CMovieBrowser::refreshBrowserList(void) //P1
{
	dprintf(DEBUG_INFO, "CMovieBrowser::refreshBrowserList: \r\n");
	
	std::string string_item;

	// Initialise and clear list array
	m_browserListLines.rows = m_settings.browserRowNr;
	for(int row = 0; row < m_settings.browserRowNr; row++)
	{
		m_browserListLines.lineArray[row].clear();
		m_browserListLines.rowWidth[row] = m_settings.browserRowWidth[row];
		m_browserListLines.lineHeader[row]= m_localizedItemName[m_settings.browserRowItem[row]];
	}

	m_vHandleBrowserList.clear();
	
	if(m_vMovieInfo.size() <= 0) 
	{
		m_currentBrowserSelection = 0;
		m_movieSelectionHandler = NULL;
		if(m_pcBrowser != NULL)
			m_pcBrowser->setLines(&m_browserListLines);//FIXME last delete test
		return; // exit here if nothing else is to do
	}
	
	MI_MOVIE_INFO* movie_handle;
	// prepare Browser list for sorting and filtering
	for(unsigned int file = 0; file < m_vMovieInfo.size(); file++)
	{
		if(isFiltered(m_vMovieInfo[file]) == false &&
			isParentalLock(m_vMovieInfo[file]) == false  &&
			(m_settings.browser_serie_mode == 0 || m_vMovieInfo[file].serieName.empty() || m_settings.filter.item == MB_INFO_SERIE) )
		{
			movie_handle = &(m_vMovieInfo[file]);
			m_vHandleBrowserList.push_back(movie_handle);
		}
	}
	
	// sort the not filtered files
	onSortMovieInfoHandleList(m_vHandleBrowserList,m_settings.sorting.item, MB_DIRECTION_AUTO);

	for(unsigned int handle = 0; handle < m_vHandleBrowserList.size() ;handle++)
	{
		for(int row = 0; row < m_settings.browserRowNr ;row++)
		{
			if ( getMovieInfoItem(*m_vHandleBrowserList[handle], m_settings.browserRowItem[row], &string_item) == false)
			{
				string_item = "n/a";
				if(m_settings.browserRowItem[row] == MB_INFO_TITLE)
					getMovieInfoItem(*m_vHandleBrowserList[handle], MB_INFO_FILENAME, &string_item);
			}
			m_browserListLines.lineArray[row].push_back(string_item);
		}
	}
	m_pcBrowser->setLines(&m_browserListLines);

	m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
	
	// update selected movie if browser is in the focus
	if (m_windowFocus == MB_FOCUS_BROWSER)
	{
		updateMovieSelection();	
	}
}

void CMovieBrowser::refreshBookmarkList(void) // P3
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::refreshBookmarkList:\r\n");
}

#define MB_HEAD_BUTTONS_COUNT	4
const struct button_label MBHeadButtons[MB_HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_0, "" },
	{ NEUTRINO_ICON_BUTTON_HELP, "" },
	{ NEUTRINO_ICON_BUTTON_SETUP, "" },
	{ NEUTRINO_ICON_BUTTON_MUTE_SMALL, "" },
};

void CMovieBrowser::refreshTitle(void) 
{
	//Paint Text Background
	dprintf(DEBUG_INFO, "CMovieBrowser::refreshTitle: %s\r\n",m_textTitle.c_str());
	
	headers->clear();
	
	// title
	std::string title = m_textTitle.c_str();
	std::string mb_icon = NEUTRINO_ICON_MOVIE;
	
	title = _("Movie Browser");

	//CHeaders headers(&m_cBoxFrameTitleRel, title.c_str(), NEUTRINO_ICON_MOVIE);
	headers->setTitle(title.c_str());
	headers->setIcon(NEUTRINO_ICON_MOVIE);
	headers->enablePaintDate();
	headers->setButtons(MBHeadButtons, MB_HEAD_BUTTONS_COUNT);
	//headers.paint();
}

#define MB_FOOT_BUTTONS_COUNT	4
struct button_label MBFootButtons[MB_FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, "" },
	{ NEUTRINO_ICON_BUTTON_GREEN, "" },
	{ NEUTRINO_ICON_BUTTON_YELLOW, "" },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("scan for Movies ...") },
	
};

void CMovieBrowser::refreshFoot(void) 
{
	dprintf(DEBUG_INFO, "CMovieBrowser::refreshFoot:\r\n");
	
	footers->clear();

	if (m_settings.gui != MB_GUI_LAST_PLAY && m_settings.gui != MB_GUI_LAST_RECORD)
	{
		// red
		std::string sort_text = _("Filter:");
		//sort_text += m_localizedItemName[m_settings.sorting.item]; //FIXME:
	
		//MBFootButtons[0].localename = sort_text.c_str();
		MBFootButtons[0].localename = _("Filter:");
		
		
		// green
		std::string filter_text = _("Filter:");
		filter_text += m_settings.filter.optionString;

		//MBFootButtons[1].localename = filter_text.c_str();
		MBFootButtons[1].localename = _("Filter:");
	}

	// yellow
	std::string next_text = _("Next focus");

	if(m_settings.gui == MB_GUI_FILTER && m_windowFocus == MB_FOCUS_FILTER)
	{
		next_text = "select";
	}
	else
	{
		next_text = _("Next focus");
	}

	MBFootButtons[2].localename = _("next focus");

	//CFooters footers(&m_cBoxFrameFootRel);

	footers->setButtons(MBFootButtons, MB_FOOT_BUTTONS_COUNT);
	//footers.paint();
}

bool CMovieBrowser::onButtonPress(neutrino_msg_t msg)
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPress: %d\r\n", msg);
	
	bool result = false;
	
	result = onButtonPressMainFrame(msg);

	if(result == false)
	{
		// if Main Frame didnot process the button, the focused window may do
		switch(m_windowFocus)
		{
			case MB_FOCUS_BROWSER:
			 	result = onButtonPressBrowserList(msg);		
				break;
				
			case MB_FOCUS_LAST_PLAY:
			 	result = onButtonPressLastPlayList(msg);		
				break;
				
			case MB_FOCUS_LAST_RECORD:
			 	result = onButtonPressLastRecordList(msg);		
				break;
				
			case MB_FOCUS_MOVIE_INFO:
			 	result = onButtonPressMovieInfoList(msg);		
				break;
				
			case MB_FOCUS_FILTER:
			 	result = onButtonPressFilterList(msg);		
				break;
				
			default:
				break;
		}
	}
	
	return (result);
}

bool CMovieBrowser::onButtonPressMainFrame(neutrino_msg_t msg)
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressMainFrame: %d\r\n", msg);
	
	bool result = true;

	if (msg == RC_home)
	{
		if(m_settings.gui == MB_GUI_FILTER)
			onSetGUIWindow(MB_GUI_MOVIE_INFO);
		else
			result = false;
	}
	else if (msg == RC_minus) 
	{
		onSetGUIWindowPrev();
	}
	else if (msg == RC_plus) 
	{
		onSetGUIWindowNext();
	}
	else if (msg == RC_green) 
	{		
		if(m_settings.gui == MB_GUI_MOVIE_INFO)
			onSetGUIWindow(MB_GUI_FILTER);
		else if(m_settings.gui == MB_GUI_FILTER)
			onSetGUIWindow(MB_GUI_MOVIE_INFO);
		// no effect if gui is last play or record			
	}
	else if (msg == RC_yellow) 
	{
		onSetFocusNext();
	}
	else if (msg == RC_blue) 
	{
		loadMovies();
		refresh();
	}
	else if (msg == RC_red ) 
	{	
		if(m_settings.gui != MB_GUI_LAST_PLAY && m_settings.gui != MB_GUI_LAST_RECORD)
		{
			// sorting is not avialable for last play and record
			do
			{
				if(m_settings.sorting.item + 1 >= MB_INFO_MAX_NUMBER)
					m_settings.sorting.item = (MB_INFO_ITEM)0;
				else
					m_settings.sorting.item = (MB_INFO_ITEM)(m_settings.sorting.item + 1);
			}while(sortBy[m_settings.sorting.item] == NULL);
					
			refreshBrowserList();	
			refreshFoot();
		}
	}
	else if (msg == RC_spkr) 
	{
		if(m_vMovieInfo.size() > 0)
		{	
			if(m_movieSelectionHandler != NULL)
			{
			 	onDeleteFile(*m_movieSelectionHandler);
			}
		}
	}
	else if ( msg == RC_info) 
	{
		if(m_movieSelectionHandler != NULL)
		{
			m_pcWindow->paintBackground();
			m_pcWindow->blit();
	  
			m_movieInfo.showMovieInfo(*m_movieSelectionHandler);
			
			refresh();
		}
	}
	else if (msg == RC_setup) 
	{
		showMenu();
	}
	else if(msg == RC_0)
	{
		if(m_movieSelectionHandler != NULL)
		{
			m_pcWindow->paintBackground();
			m_pcWindow->blit();

			//				
			CTmdb * tmdb = new CTmdb();

			if(tmdb->getMovieInfo(m_movieSelectionHandler->epgTitle))
			{
				if ((!tmdb->getDescription().empty())) 
				{
					std::string buffer;

					buffer = m_movieSelectionHandler->epgTitle;
					buffer += "\n";
	
					// prepare print buffer  
					buffer += tmdb->createInfoText();
	
					std::string tname = tmdb->getThumbnailDir();
					tname += "/";
					tname += m_movieSelectionHandler->epgTitle;
					tname += ".jpg";

					tmdb->getSmallCover(tmdb->getPosterPath(), tname);
				
					// scale pic
					int p_w = 0;
					int p_h = 0;
				
					scaleImage(tname, &p_w, &p_h);
	
					CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
					CInfoBox * infoBox = new CInfoBox(&position, m_movieSelectionHandler->epgTitle.c_str(), NEUTRINO_ICON_TMDB);

					infoBox->setText(buffer.c_str(), tname.c_str(), p_w, p_h);
					infoBox->exec();
					delete infoBox;

					if(MessageBox("Information", _("Prefer TMBD infos ?"), mbrNo, mbYes | mbNo) == mbrYes) 
					{
						// rewrite tfile
						std::string tname = m_movieSelectionHandler->file.Name;
						changeFileNameExt(tname, ".jpg");
						if(tmdb->getSmallCover(tmdb->getPosterPath(), tname)) 
							m_movieSelectionHandler->tfile = tname;

						if(m_movieSelectionHandler->epgInfo1.empty())
							m_movieSelectionHandler->epgInfo1 = tmdb->getDescription();

						m_movieInfo.saveMovieInfo( *m_movieSelectionHandler);
					}  
				}
				else
				{
					MessageBox(_("Information"), _("Not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
				}
			}
			else
			{
				MessageBox(_("Information"), _("Not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
			}

			delete tmdb;
			tmdb = NULL;
		}

		refresh();
	}	
	else
	{
		result = false;
	}

	return (result);
}

bool CMovieBrowser::onButtonPressBrowserList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressBrowserList: %d\r\n", msg);
	
	bool result = true;
	
	if(msg == RC_up)
	{
		m_pcBrowser->scrollLineUp(1);
	}
	else if (msg == RC_down)
	{
		m_pcBrowser->scrollLineDown(1);
	}
	else if (msg == RC_page_up)
	{
		m_pcBrowser->scrollPageUp(1);
	}
	else if (msg == RC_page_down)
	{
		m_pcBrowser->scrollPageDown(1);
	}
	else if (msg == RC_left)
	{
		m_pcBrowser->scrollPageUp(1);
	}
	else if (msg == RC_right)
	{
		m_pcBrowser->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	
	if(result == true)
		updateMovieSelection();

	return (result);
}

bool CMovieBrowser::onButtonPressLastPlayList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressLastPlayList %d\r\n", msg);
	
	bool result = true;
	
	if(msg == RC_up)
	{
		m_pcLastPlay->scrollLineUp(1);
	}
	else if (msg == RC_down)
	{
		m_pcLastPlay->scrollLineDown(1);
	}
	else if (msg == RC_left)
	{
		m_pcLastPlay->scrollPageUp(1);
	}
	else if (msg == RC_right)
	{
		m_pcLastPlay->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	
	if(result == true)
		updateMovieSelection();

	return (result);
}

bool CMovieBrowser::onButtonPressLastRecordList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressLastRecordList: %d\r\n", msg);
	
	bool result = true;
	
	if(msg == RC_up)
	{
		m_pcLastRecord->scrollLineUp(1);
	}
	else if (msg == RC_down)
	{
		m_pcLastRecord->scrollLineDown(1);
	}
	else if (msg == RC_left)
	{
		m_pcLastRecord->scrollPageUp(1);
	}
	else if (msg == RC_right)
	{
		m_pcLastRecord->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}

	if(result == true)
		updateMovieSelection();

	return (result);
}

/*
bool CMovieBrowser::onButtonPressBookmarkList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressBookmarkList: %d\r\n", msg);
	
	bool result = true;
	
	result = false;
	return (result);
}
*/

bool CMovieBrowser::onButtonPressFilterList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressFilterList: %d,%d\r\n", msg, m_settings.filter.item);
	
	bool result = true;

	if(msg == RC_up)
	{
		m_pcFilter->scrollLineUp(1);
	}
	else if (msg == RC_down)
	{
		m_pcFilter->scrollLineDown(1);
	}
	else if (msg == RC_page_up)
	{
		m_pcFilter->scrollPageUp(1);
	}
	else if (msg == RC_page_down)
	{
		m_pcFilter->scrollPageDown(1);
	}
	else if (msg == RC_ok)
	{
		int selected_line = m_pcFilter->getSelectedLine();
		
		if(m_settings.filter.item == MB_INFO_MAX_NUMBER)
		{
			if(selected_line == 0) m_settings.filter.item = MB_INFO_MAJOR_GENRE;
			if(selected_line == 1) m_settings.filter.item = MB_INFO_INFO1;
			if(selected_line == 2) m_settings.filter.item = MB_INFO_FILEPATH;
			if(selected_line == 3) m_settings.filter.item = MB_INFO_SERIE;
			refreshFilterList();
			m_pcFilter->setSelectedLine(0);
		}
		else
		{
			if(selected_line == 0)
			{
				m_settings.filter.item = MB_INFO_MAX_NUMBER;
				m_settings.filter.optionString = "";
				m_settings.filter.optionVar = 0;
				refreshFilterList();
				m_pcFilter->setSelectedLine(0);
				refreshBrowserList();	
				refreshLastPlayList();	
				refreshLastRecordList();	
				refreshFoot();
			}
			else
			{
				updateFilterSelection();
			}
		}
	}
	else if (msg == RC_left)
	{
		m_pcFilter->scrollPageUp(1);
	}
	else if (msg == RC_right)
	{
		m_pcFilter->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}
	
	return (result);
}

bool CMovieBrowser::onButtonPressMovieInfoList(neutrino_msg_t msg) 
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onButtonPressEPGInfoList: %d\r\n",msg);
	
	bool result = true;
	
	if(msg == RC_up)
	{
		m_pcInfo->scrollPageUp(1);
	}
	else if (msg == RC_down)
	{
		m_pcInfo->scrollPageDown(1);
	}
	else
	{
		// default
		result = false;
	}	

	return (result);
}

void CMovieBrowser::onDeleteFile(MI_MOVIE_INFO& movieSelectionHandler)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::onDeleteFile:");

	std::string msg = _("Delete");
	msg += "\r\n ";
	if (movieSelectionHandler.file.Name.length() > 40)
	{
			msg += movieSelectionHandler.file.Name.substr(0, 40);
			msg += "...";
	}
	else
		msg += movieSelectionHandler.file.Name;
			
	msg += "\r\n ";
	msg += _("?");
	
	if (MessageBox(_("Delete"), msg.c_str(), mbrNo, mbYes | mbNo) == mbrYes)
	{
		delFile(movieSelectionHandler.file);
			
                int i = 1;
                char newpath[1024];
                do {
			sprintf(newpath, "%s.%03d", movieSelectionHandler.file.Name.c_str(), i);
			if(access(newpath, R_OK)) 
			{
				break;
                        } 
                        else 
			{
				unlink(newpath);
				dprintf(DEBUG_NORMAL, "  delete file: %s\r\n", newpath);
                        }
                        i++;
                } while(1);
			
                std::string fname = movieSelectionHandler.file.Name;
                       
		int ext_pos = 0;
		ext_pos = fname.rfind('.');
		if( ext_pos > 0)
		{
			std::string extension;
			extension = fname.substr(ext_pos + 1, fname.length() - ext_pos);
			extension = "." + extension;
			strReplace(fname, extension.c_str(), ".jpg");
		}
			
                unlink(fname.c_str());

		CFile file_xml  = movieSelectionHandler.file; 
		if(m_movieInfo.convertTs2XmlName(&file_xml.Name) == true)  
		{
			delFile(file_xml);
	    	}
	    	
		m_vMovieInfo.erase( (std::vector<MI_MOVIE_INFO>::iterator)&movieSelectionHandler);
		dprintf(DEBUG_NORMAL, "List size: %d\n", m_vMovieInfo.size());
		//if(m_vMovieInfo.size() == 0) fileInfoStale();
		//if(m_vMovieInfo.size() == 0) onSetGUIWindow(m_settings.gui);
		updateSerienames();
		refreshBrowserList();
		refreshLastPlayList();	
		refreshLastRecordList();	
		refreshMovieInfo();
	    		
		//loadMovies(); // //TODO we might remove the handle from the handle list only, to avoid reload .....
		refresh();
	}
}

void CMovieBrowser::onSetGUIWindow(MB_GUI gui)
{
	m_settings.gui = gui;
	
	if(gui == MB_GUI_MOVIE_INFO)
	{
		dprintf(DEBUG_NORMAL, "[mb] browser info\r\n");
		
		// Paint these frames ...
		m_showMovieInfo = true;
		m_showBrowserFiles = true;

		// ... and hide these frames
		m_showLastRecordFiles = false;
		m_showLastPlayFiles = false;
		m_showFilter = false;

		m_pcLastPlay->hide();
		m_pcLastRecord->hide();
		m_pcFilter->hide();
		
		m_pcBrowser->paint();
		onSetFocus(MB_FOCUS_BROWSER);
		m_pcInfo->paint();
		refreshMovieInfo();
	}
	else if(gui == MB_GUI_LAST_PLAY)
	{
		dprintf(DEBUG_NORMAL, "[mb] last play \r\n");
		
		// Paint these frames ...
		m_showLastRecordFiles = true;
		m_showLastPlayFiles = true;
		m_showMovieInfo = true;

		// ... and hide these frames
		m_showBrowserFiles = false;
		m_showFilter = false;

		m_pcBrowser->hide();
		m_pcFilter->hide();
		
		m_pcLastRecord->paint();
		m_pcLastPlay->paint();

		onSetFocus(MB_FOCUS_LAST_PLAY);
		m_pcInfo->paint();
		refreshMovieInfo();
	}
	else if(gui == MB_GUI_LAST_RECORD)
	{
		dprintf(DEBUG_NORMAL, "[mb] last record \r\n");
		// Paint these frames ...
		m_showLastRecordFiles = true;
		m_showLastPlayFiles = true;
		m_showMovieInfo = true;

		// ... and hide these frames
		m_showBrowserFiles = false;
		m_showFilter = false;

		m_pcBrowser->hide();
		m_pcFilter->hide();
		
		m_pcLastRecord->paint();
		m_pcLastPlay->paint();

		onSetFocus(MB_FOCUS_LAST_RECORD);
		m_pcInfo->paint();
		refreshMovieInfo();
	}
 	else if(gui == MB_GUI_FILTER)
	{
		dprintf(DEBUG_NORMAL, "[mb] filter \r\n");
		
		// Paint these frames ...
		m_showFilter = true;
		
		// ... and hide these frames
		m_showMovieInfo = false;
		
		m_pcInfo->hide();
		
		m_pcFilter->paint();
		onSetFocus(MB_FOCUS_FILTER);
	}	
}

void CMovieBrowser::onSetGUIWindowNext(void) 
{
	if(m_settings.gui == MB_GUI_MOVIE_INFO )
	{
		onSetGUIWindow(MB_GUI_LAST_PLAY);
	}
	else if(m_settings.gui == MB_GUI_LAST_PLAY)
	{
		onSetGUIWindow(MB_GUI_LAST_RECORD);
	}
	else
	{
		onSetGUIWindow(MB_GUI_MOVIE_INFO);
	}
}

void CMovieBrowser::onSetGUIWindowPrev(void) 
{
	if(m_settings.gui == MB_GUI_MOVIE_INFO )
	{
		onSetGUIWindow(MB_GUI_LAST_RECORD);
	}
	else if(m_settings.gui == MB_GUI_LAST_RECORD)
	{
		onSetGUIWindow(MB_GUI_LAST_PLAY);
	}
	else
	{
		onSetGUIWindow(MB_GUI_MOVIE_INFO);
	}
}

void CMovieBrowser::onSetFocus(MB_FOCUS new_focus)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::onSetFocus: %d \r\n", new_focus);
	
	m_windowFocus = new_focus;
	
	if(m_windowFocus == MB_FOCUS_BROWSER)
	{
			m_pcBrowser->showSelection(true);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(true);
			m_pcFilter->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(true);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(false);
	}
	else if(m_windowFocus == MB_FOCUS_FILTER)
	{
			m_pcBrowser->showSelection(false);
			m_pcLastRecord->showSelection(false);
			m_pcLastPlay->showSelection(false);
			m_pcFilter->showSelection(true);
	}
	
	updateMovieSelection();
	refreshFoot();	
}

void CMovieBrowser::onSetFocusNext(void) 
{
	dprintf(DEBUG_INFO, "CMovieBrowser::onSetFocusNext: \r\n");
	
	if(m_settings.gui == MB_GUI_FILTER)
	{
		if(m_windowFocus == MB_FOCUS_BROWSER)
		{
			dprintf(DEBUG_NORMAL, "[mb] MB_FOCUS_FILTER\r\n");
			onSetFocus(MB_FOCUS_FILTER);
		}
		else
		{
			dprintf(DEBUG_NORMAL, "[mb] MB_FOCUS_BROWSER\r\n");
			onSetFocus(MB_FOCUS_BROWSER);
		}
	}
	else if(m_settings.gui == MB_GUI_MOVIE_INFO)
	{
		if(m_windowFocus == MB_FOCUS_BROWSER)
		{
			dprintf(DEBUG_NORMAL, "[mb] MB_FOCUS_MOVIE_INFO\r\n");
			onSetFocus(MB_FOCUS_MOVIE_INFO);
			m_windowFocus = MB_FOCUS_MOVIE_INFO;
		}
		else
		{
			dprintf(DEBUG_NORMAL, "[mb] MB_FOCUS_BROWSER\r\n");
			onSetFocus(MB_FOCUS_BROWSER);
		}
	}
	else if(m_settings.gui == MB_GUI_LAST_PLAY)
	{
		if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
		{
			onSetFocus(MB_FOCUS_LAST_PLAY);
		}
		else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
		{
			onSetFocus(MB_FOCUS_MOVIE_INFO);
		}
	}
	else if(m_settings.gui == MB_GUI_LAST_RECORD)
	{
		if(m_windowFocus == MB_FOCUS_MOVIE_INFO)
		{
			onSetFocus(MB_FOCUS_LAST_RECORD);
		}
		else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
		{
			onSetFocus(MB_FOCUS_MOVIE_INFO);
		}
	}
}

bool CMovieBrowser::onSortMovieInfoHandleList(std::vector<MI_MOVIE_INFO*>& handle_list, MB_INFO_ITEM sort_item, MB_DIRECTION direction)
{
	dprintf(DEBUG_DEBUG, "CMovieBrowser::onSortMovieInfoHandleList: %d\r\n", direction);
	
	if(handle_list.size() <= 0) 
		return (false); // nothing to sort, return immedately
	if(sortBy[sort_item] == NULL) 
		return (false);
	
	if(direction == MB_DIRECTION_AUTO)
	{
		if( sort_item == MB_INFO_QUALITY || 
			sort_item == MB_INFO_PARENTAL_LOCKAGE || 
			sort_item == MB_INFO_PREVPLAYDATE || 
			sort_item == MB_INFO_RECORDDATE || 
			sort_item == MB_INFO_PRODDATE ||
			sort_item == MB_INFO_SIZE)
	 	{
			sortDirection = 1;
		}
		else
		{
			sortDirection = 0;
		}
	}
	else if(direction == MB_DIRECTION_UP)
	{
		sortDirection = 0;
	}
	else
	{
		sortDirection = 1;
	}
	
	dprintf(DEBUG_DEBUG, "sort: %d\r\n", sortDirection);
	
	sort(handle_list.begin(), handle_list.end(), sortBy[sort_item]);
	
	return (true);
}

void CMovieBrowser::updateDir(void)
{
	m_dir.clear();
	
	// check if there is a movie dir and if we should use it
	if(g_settings.network_nfs_moviedir[0] != 0 )
	{
		std::string name = g_settings.network_nfs_moviedir;
		addDir(name, &m_settings.storageDirMovieUsed);
	}
	
	// check if there is a record dir and if we should use it
	if(g_settings.network_nfs_recordingdir[0] != 0 )
	{
		std::string name = g_settings.network_nfs_recordingdir;
		addDir(name, &m_settings.storageDirRecUsed);
	}

	for(int i = 0; i < MB_MAX_DIRS; i++)
	{
		if(!m_settings.storageDir[i].empty())
			addDir(m_settings.storageDir[i], &m_settings.storageDirUsed[i]);
	}
}

void CMovieBrowser::loadAllTsFileNamesFromStorage(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::loadAllTsFileNamesFromStorage:\r\n");
	
	bool result;
	int i, size; 

	m_movieSelectionHandler = NULL;
	m_dirNames.clear();
	m_vMovieInfo.clear();

	updateDir();

	size = m_dir.size();
	for(i = 0; i < size; i++)
	{
		if(*m_dir[i].used == true )
			result = loadTsFileNamesFromDir(m_dir[i].name);
	}
}

// Note: this function is used recursive, do not add any return within the body due to the recursive counter
bool CMovieBrowser::loadTsFileNamesFromDir(const std::string & dirname)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::loadTsFileNamesFromDir: %s\r\n", dirname.c_str());

	static int recursive_counter = 0; // recursive counter to be used to avoid hanging
	bool result = false;
	int file_found_in_dir = false;

	if (recursive_counter > 10)
	{
		return (false); // do not go deeper than 10 directories
	}

	// check if directory was already searched once
	int size = m_dirNames.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(m_dirNames[i].c_str(), dirname.c_str()) == 0)	
		{
			// string is identical to previous one
			return (false); 
		}
	}
	
	// warning: no return statement within the body after here !!!!
	recursive_counter++;

	CFileList flist;
	if(readDir(dirname, &flist) == true)
	{
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
		
		for(unsigned int i = 0; i < flist.size(); i++)
		{
			if( S_ISDIR(flist[i].Mode)) 
			{
				flist[i].Name += '/';
				
				loadTsFileNamesFromDir(flist[i].Name);
			}
			else
			{
				int test = -1;
				
				// use filter
				CFileFilter fileFilter;
	
				fileFilter.addFilter("ts");
				fileFilter.addFilter("mpg");
				fileFilter.addFilter("mpeg");
				fileFilter.addFilter("divx");
				fileFilter.addFilter("avi");
				fileFilter.addFilter("mkv");
				fileFilter.addFilter("asf");
				fileFilter.addFilter("aiff");
				fileFilter.addFilter("m2p");
				fileFilter.addFilter("mpv");
				fileFilter.addFilter("m2ts");
				fileFilter.addFilter("vob");
				fileFilter.addFilter("mp4");
				fileFilter.addFilter("mov");	
				fileFilter.addFilter("flv");	
				fileFilter.addFilter("dat");
				fileFilter.addFilter("trp");
				fileFilter.addFilter("vdr");
				fileFilter.addFilter("mts");
				fileFilter.addFilter("wmv");
				fileFilter.addFilter("wav");
				fileFilter.addFilter("flac");
				fileFilter.addFilter("mp3");
				fileFilter.addFilter("wma");
				fileFilter.addFilter("ogg");

				if(fileFilter.matchFilter(flist[i].Name))
					test = 0;
				
				if( test != -1)
				{
					m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
					movieInfo.file.Name = flist[i].Name;
					
					// load movie infos (from xml file)
					m_movieInfo.loadMovieInfo(&movieInfo);
					
					//
					movieInfo.file.Mode = flist[i].Mode;
					//movieInfo.file.Size = flist[i].Size;
					movieInfo.file.Size = get_full_len((char *)flist[i].Name.c_str());
					movieInfo.file.Time = flist[i].Time;
					
					if(file_found_in_dir == false)
					{
						// first file in directory found, add directory to list 
						m_dirNames.push_back(dirname);
						file_found_in_dir = true;
					}
					
					movieInfo.dirItNr = m_dirNames.size()-1;
					
					// 
					m_vMovieInfo.push_back(movieInfo);
				}
			}
		}
		
		result = true;
	}	
 	
	recursive_counter--;
	
	if(result == false)
		m_file_info_stale = true;
	
	return (result);
}

bool CMovieBrowser::readDir(const std::string & dirname, CFileList* flist)
{
	bool result = true;
	
	dprintf(DEBUG_INFO, "CMovieBrowser::readDir: %s\n",dirname.c_str());
	
	stat_struct statbuf;
	dirent_struct **namelist;
	int n;

	n = my_scandir(dirname.c_str(), &namelist, 0, my_alphasort);
	if (n < 0)
	{
		perror(("[mb] scandir: "+dirname).c_str());
		return false;
	}
	
	CFile file;
	for(int i = 0; i < n;i++)
	{
		if(namelist[i]->d_name[0] != '.')
		{
			file.Name = dirname;
			file.Name += namelist[i]->d_name;
			
			if(my_stat((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Time = statbuf.st_mtime;
				file.Size = statbuf.st_size;
				
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return(result);
}

bool CMovieBrowser::delFile(CFile& file)
{
	bool result = true;
	unlink(file.Name.c_str()); // fix: use full path
	dprintf(DEBUG_NORMAL, "CMovieBrowser::delete file: %s\r\n", file.Name.c_str());
	return(result);
}

void CMovieBrowser::updateMovieSelection(void)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::updateMovieSelection: %d\r\n", m_windowFocus);
	
	if (m_vMovieInfo.size() == 0) 
		return;
	
	bool new_selection = false;
	 
	unsigned int old_movie_selection;
	
	if(m_windowFocus == MB_FOCUS_BROWSER)
	{
		if(m_vHandleBrowserList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentBrowserSelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentBrowserSelection;
			m_currentBrowserSelection = m_pcBrowser->getSelectedLine();
			//dprintf(DEBUG_NORMAL, "    sel1:%d\r\n",m_currentBrowserSelection);
			if(m_currentBrowserSelection != old_movie_selection)
				new_selection = true;
			
			if(m_currentBrowserSelection < m_vHandleBrowserList.size())
				m_movieSelectionHandler = m_vHandleBrowserList[m_currentBrowserSelection];
		}
	}
	else if(m_windowFocus == MB_FOCUS_LAST_PLAY)
	{
		if(m_vHandlePlayList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentPlaySelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentPlaySelection;
			m_currentPlaySelection = m_pcLastPlay->getSelectedLine();
			//dprintf(DEBUG_NORMAL, "    sel2:%d\r\n",m_currentPlaySelection);
			if(m_currentPlaySelection != old_movie_selection)
				new_selection = true;
	
			 if(m_currentPlaySelection < m_vHandlePlayList.size())
				m_movieSelectionHandler = m_vHandlePlayList[m_currentPlaySelection];
		}
	}
	else if(m_windowFocus == MB_FOCUS_LAST_RECORD)
	{
		if(m_vHandleRecordList.size() == 0)
		{
			// There are no elements in the Filebrowser, clear all handles
			m_currentRecordSelection = 0;
			m_movieSelectionHandler = NULL;
			new_selection = true;
		}
		else
		{
			old_movie_selection = m_currentRecordSelection;
			m_currentRecordSelection = m_pcLastRecord->getSelectedLine();

			if(m_currentRecordSelection != old_movie_selection)
				new_selection = true;
	
			if(m_currentRecordSelection < m_vHandleRecordList.size())
				m_movieSelectionHandler = m_vHandleRecordList[m_currentRecordSelection];
		}
	}	
	
	if(new_selection == true)
	{
		refreshMovieInfo();
		refreshLCD();
	}
}

void CMovieBrowser::updateFilterSelection(void)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::updateFilterSelection:\r\n");
	
	if( m_FilterLines.lineArray[0].size() == 0) 
		return;

	bool result = true;
	int selected_line = m_pcFilter->getSelectedLine();
	if(selected_line > 0)
		selected_line--;

	if(m_settings.filter.item == MB_INFO_FILEPATH)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
		m_settings.filter.optionVar = selected_line;
	}
	else if(m_settings.filter.item == MB_INFO_INFO1)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
	}
	else if(m_settings.filter.item == MB_INFO_MAJOR_GENRE)
	{
		m_settings.filter.optionString = GENRE_ALL[selected_line].valname;
		m_settings.filter.optionVar = GENRE_ALL[selected_line].key;
	}
	else if(m_settings.filter.item == MB_INFO_SERIE)
	{
		m_settings.filter.optionString = m_FilterLines.lineArray[0][selected_line+1];
	}
	else
	{
		result = false;
	}
	
	if(result == true)
	{
		refreshBrowserList();	
		refreshLastPlayList();	
		refreshLastRecordList();	
		refreshFoot();
	}	
}

bool CMovieBrowser::addDir(std::string& dirname, int* used)
{
	if(dirname.empty()) 
		return false;
	
	if(dirname == "/") 
		return false;
	
	MB_DIR newdir;
	newdir.name = dirname;

	if(newdir.name.rfind('/') != newdir.name.length()-1 || newdir.name.length() == 0)
	{
		newdir.name += '/';
	}

	int size = m_dir.size();
	for(int i = 0; i < size; i++)
	{
		if(strcmp(m_dir[i].name.c_str(),newdir.name.c_str()) == 0)
		{
			// string is identical to previous one
			dprintf(DEBUG_NORMAL, "[mb] Dir already in list: %s\r\n",newdir.name.c_str());
			return (false); 
		}
	}
	
	dprintf(DEBUG_NORMAL, "[mb] new Dir: %s\r\n",newdir.name.c_str());
	newdir.used = used;
	m_dir.push_back(newdir);
	
	if(*used == true)
	{
		m_file_info_stale = true; // we got a new Dir, search again for all movies next time
		m_seriename_stale = true;
	}
	    
	return (true);
}

void CMovieBrowser::loadMovies(void)
{
	dprintf(DEBUG_NORMAL, "CMovieBrowser::loadMovies:\n");
	
	//first clear screen */
	m_pcWindow->paintBackground();
	m_pcWindow->blit();	

	CHintBox loadBox(_("Moviebrowser"), _("Scan for Movies ..."));
	
	loadBox.paint();

	loadAllTsFileNamesFromStorage(); // P1

	m_seriename_stale = true; // we reloaded the movie info, so make sure the other list are  updated later on as well
	updateSerienames();
		
	if(m_settings.serie_auto_create == 1)
	{
		autoFindSerie();
	}
	
	loadBox.hide();
	
	m_file_info_stale = false;

	refreshBrowserList();	
	refreshLastPlayList();	
	refreshLastRecordList();
	refreshFilterList();
	refreshMovieInfo();	// is done by refreshBrowserList if needed
}

void CMovieBrowser::loadAllMovieInfo(void)
{
	dprintf(DEBUG_INFO, "CMovieBrowser::loadAllMovieInfo:\r\n");

	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		m_movieInfo.loadMovieInfo( &(m_vMovieInfo[i]));
	}
}

void CMovieBrowser::showHelp(void)
{
	CMovieHelp help;

	help.exec(NULL, "");
}

void CMovieBrowser::showMovieInfoMenu(MI_MOVIE_INFO * movie_info)
{
	unsigned int i = 0;

	// bookmark Menu
	CStringInputSMS * pBookNameInput[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	CIntInput * pBookPosIntInput[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	CIntInput * pBookTypeIntInput[MAX_NUMBER_OF_BOOKMARK_ITEMS];
	
	//
	CMenuWidget * pBookItemMenu[MAX_NUMBER_OF_BOOKMARK_ITEMS];

	CIntInput bookStartIntInput(_("Bookmark change"), (int&)movie_info->bookmarks.start, 5, _("Enter new Position (s)"), _("Enter new Position (s)"));
	CIntInput bookLastIntInput(_("Bookmark change"), (int&)movie_info->bookmarks.lastPlayStop, 5, _("Enter new Position (s)"), _("Enter new Position (s)"));
	CIntInput bookEndIntInput(_("Bookmark change"), (int&)movie_info->bookmarks.end, 5, _("Enter new Position (s)"), _("Enter new Position (s)"));

	CMenuWidget bookmarkMenu(_("Bookmarks"), NEUTRINO_ICON_MOVIE);
	//bookmarkMenu.enableSaveScreen(true);

	// intros
	bookmarkMenu.addItem(new ClistBoxItem(_("Clear all"), true, NULL, this, "book_clear_all", RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	bookmarkMenu.addItem(new CMenuSeparator(LINE));
	bookmarkMenu.addItem(new ClistBoxItem(_("Movie start:"), true, bookStartIntInput.getValue(), &bookStartIntInput));
	bookmarkMenu.addItem(new ClistBoxItem(_("Movie end:"), true, bookLastIntInput.getValue(),  &bookLastIntInput));
	bookmarkMenu.addItem(new ClistBoxItem(_("Last play stop:"), true, bookEndIntInput.getValue(),   &bookEndIntInput));
	bookmarkMenu.addItem(new CMenuSeparator(LINE));

	for(int i1 = 0 ; i1 < MI_MOVIE_BOOK_USER_MAX && i1 < MAX_NUMBER_OF_BOOKMARK_ITEMS; i1++ )
	{
		pBookNameInput[i1] = new CStringInputSMS (_("Bookmark change"), movie_info->bookmarks.user[i1].name.c_str(), MAX_INPUT_CHARS, _("Enter new Position (s)"), _("Enter new Position (s)"));
		pBookPosIntInput[i1] =  new CIntInput (_("Bookmark change"), (int&) movie_info->bookmarks.user[i1].pos, 20, _("Enter new Position (s)"), _("Enter new Position (s)"));
		pBookTypeIntInput[i1] = new CIntInput (_("Bookmark change"), (int&) movie_info->bookmarks.user[i1].length, 20, _("Enter new Position (s)"), _("Enter new Position (s)"));

		pBookItemMenu[i1] = new CMenuWidget(_("Bookmarks"), NEUTRINO_ICON_MOVIE);
		//pBookItemMenu[i1]->enableSaveScreen();

		pBookItemMenu[i1]->setWidgetMode(MODE_MENU);
		pBookItemMenu[i1]->enableShrinkMenu();
		
		
		pBookItemMenu[i1]->addItem( new ClistBoxItem(_("Bookmarks"), true,  movie_info->bookmarks.user[i1].name.c_str(), pBookNameInput[i1]));
		pBookItemMenu[i1]->addItem( new ClistBoxItem(_("Position:"), true,  pBookPosIntInput[i1]->getValue(), pBookPosIntInput[i1]));
		pBookItemMenu[i1]->addItem( new ClistBoxItem(_("Jump (<0 back , >0 for):"), true,  pBookTypeIntInput[i1]->getValue(),pBookTypeIntInput[i1]));

		bookmarkMenu.addItem( new ClistBoxItem(movie_info->bookmarks.user[i1].name.c_str(),   true, pBookPosIntInput[i1]->getValue(),pBookItemMenu[i1]));
	}

	// serie Menu
	CStringInputSMS serieUserInput(_("Serie"), movie_info->serieName.c_str());

	CMenuWidget serieMenu(_("Serie"), NEUTRINO_ICON_MOVIE);
	//serieMenu.enableSaveScreen();

	serieMenu.setWidgetMode(MODE_MENU);
	serieMenu.enableShrinkMenu();
	
	serieMenu.addItem( new ClistBoxItem(_("Serie"), true, movie_info->serieName.c_str(), &serieUserInput));
	serieMenu.addItem(new CMenuSeparator(LINE));
	
	for(unsigned int i2 = 0; i2 < m_vHandleSerienames.size(); i2++)
		serieMenu.addItem( new ClistBoxItem(m_vHandleSerienames[i2]->serieName.c_str(), true,  movie_info->serieName.c_str()));

	// update movie info Menu
        for(i = 0; i < MB_INFO_MAX_NUMBER; i++)
		movieInfoUpdateAll[i] = 0;
	
        movieInfoUpdateAllIfDestEmptyOnly = true;

	// movieInfoMenuUpdate
        CMenuWidget movieInfoMenuUpdate(_("Save changes in all movie info files"), NEUTRINO_ICON_MOVIE);
	//movieInfoMenuUpdate.enableSaveScreen();
	movieInfoMenuUpdate.setWidgetMode(MODE_MENU);
	movieInfoMenuUpdate.enableShrinkMenu();
	
	// save 
        movieInfoMenuUpdate.addItem(new ClistBoxItem(_("Start update of movie info files"), true, NULL, this, "save_movie_info_all", RC_red, NEUTRINO_ICON_BUTTON_RED));

        movieInfoMenuUpdate.addItem(new CMenuSeparator(LINE));

	// save all
        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Copy if destination is empty only"), (&movieInfoUpdateAllIfDestEmptyOnly), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true, NULL, RC_blue, NEUTRINO_ICON_BUTTON_BLUE ));

	// title
        movieInfoMenuUpdate.addItem(new CMenuSeparator(LINE));
        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Titel"),             &movieInfoUpdateAll[MB_INFO_TITLE], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_1, NEUTRINO_ICON_BUTTON_1));

	// epgInfo1
        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Info 1"),             &movieInfoUpdateAll[MB_INFO_INFO1], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true, NULL, RC_2, NEUTRINO_ICON_BUTTON_2));

	// epgInfo2
	movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Info 2"),             &movieInfoUpdateAll[MB_INFO_INFO2], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_3, NEUTRINO_ICON_BUTTON_3));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Serie"),             &movieInfoUpdateAll[MB_INFO_SERIE], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_4, NEUTRINO_ICON_BUTTON_4));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Quality"),           &movieInfoUpdateAll[MB_INFO_QUALITY], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true , NULL, RC_5, NEUTRINO_ICON_BUTTON_5));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Parental Lock"),  &movieInfoUpdateAll[MB_INFO_PARENTAL_LOCKAGE], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_6, NEUTRINO_ICON_BUTTON_6 ));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Genre"),       &movieInfoUpdateAll[MB_INFO_MAJOR_GENRE], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_7, NEUTRINO_ICON_BUTTON_7));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Year"),          &movieInfoUpdateAll[MB_INFO_PRODDATE], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_8, NEUTRINO_ICON_BUTTON_8));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Country"),       &movieInfoUpdateAll[MB_INFO_COUNTRY], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true,NULL, RC_9, NEUTRINO_ICON_BUTTON_9));

        movieInfoMenuUpdate.addItem(new CMenuOptionChooser(_("Length (Min)"),            &movieInfoUpdateAll[MB_INFO_LENGTH], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true, NULL, RC_0, NEUTRINO_ICON_BUTTON_0));

	// movieInfo Menu
#define BUFFER_SIZE 100
	char dirItNr[BUFFER_SIZE];
	char size[BUFFER_SIZE];

	if(movie_info != NULL)
	{
		strncpy(dirItNr, m_dirNames[movie_info->dirItNr].c_str(),BUFFER_SIZE);
		snprintf(size, BUFFER_SIZE, "%5llu", movie_info->file.Size>>20);
	}

	// title
	CStringInputSMS titelUserInput(_("Title"), movie_info->epgTitle.c_str());

	// channel
	CStringInputSMS channelUserInput(_("Channel"), movie_info->epgChannel.c_str());

	// epgInfo1
	CStringInputSMS epgUserInput(_("Info 1"), movie_info->epgInfo1.c_str());

	// epgInfo2
	CStringInputSMS epgUser2Input(_("Info 2"), movie_info->epgInfo2.c_str());

	// date of last play
	CDateInput dateUserDateInput(_("Length (Min)"), &movie_info->dateOfLastPlay);
	
	// record date
	CDateInput recUserDateInput(_("Length (Min)"), &movie_info->file.Time);

	// length
	CIntInput lengthUserIntInput(_("Length (Min)"), (int&)movie_info->length, 3);

	// country
	CStringInputSMS countryUserInput(_("Country"), movie_info->productionCountry.c_str(), MAX_INPUT_CHARS, NULL, NULL, "ABCDEFGHIJKLMNOPQRSTUVWXYZ ");
	
	// prod date
	CIntInput yearUserIntInput(_("Year"), (int&)movie_info->productionDate, 4);

	// movieInfoMenu
	CMenuWidget movieInfoMenu(_("Film Informationen"), NEUTRINO_ICON_MOVIE, m_cBoxFrame.iWidth);

	movieInfoMenu.setWidgetMode(MODE_MENU);
	movieInfoMenu.enableShrinkMenu();

	// save changes
	movieInfoMenu.addItem(new ClistBoxItem(_("Save changes"), true, NULL, this, "save_movie_info", RC_red, NEUTRINO_ICON_BUTTON_RED));

	// save change in all menu 
	movieInfoMenu.addItem(new ClistBoxItem(_("Save changes in all movie info files"), true, NULL,      &movieInfoMenuUpdate, NULL, RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	// bookmark menu
	movieInfoMenu.addItem(new ClistBoxItem(_("Bookmarks"), true, NULL,      &bookmarkMenu, NULL, RC_blue,  NEUTRINO_ICON_BUTTON_BLUE));

	// title
	movieInfoMenu.addItem(new CMenuSeparator(LINE));
	movieInfoMenu.addItem(new ClistBoxItem(_("Titel"), true, movie_info->epgTitle.c_str(), &titelUserInput, NULL, RC_1, NEUTRINO_ICON_BUTTON_1));

	// serie name
	movieInfoMenu.addItem(new ClistBoxItem(_("Serie"), true, movie_info->serieName.c_str(), &serieMenu, NULL, RC_2, NEUTRINO_ICON_BUTTON_2));

	// epgInfo1
	movieInfoMenu.addItem(new ClistBoxItem(_("Info 1"), true, movie_info->epgInfo1.c_str(), &epgUserInput, NULL, RC_3, NEUTRINO_ICON_BUTTON_3));

	// epgInfo2
	movieInfoMenu.addItem(new ClistBoxItem(_("Info 2"), true, movie_info->epgInfo2.c_str(), &epgUser2Input, NULL, RC_4, NEUTRINO_ICON_BUTTON_4));

	// genre
	movieInfoMenu.addItem(new CMenuOptionChooser(_("Genre"), &movie_info->genreMajor, GENRE_ALL, GENRE_ALL_COUNT, true, NULL, RC_5, NEUTRINO_ICON_BUTTON_5, true));

	// quality
	movieInfoMenu.addItem(new CMenuSeparator(LINE));
	movieInfoMenu.addItem(new CMenuOptionNumberChooser(_("Quality"), &movie_info->quality, true, 0, 3, NULL));

	// parentallock
	movieInfoMenu.addItem(new CMenuOptionChooser(_("Age"), &movie_info->parentalLockAge, MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS, MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT, true, NULL, RC_6, NEUTRINO_ICON_BUTTON_6, true));

	// prod date
	movieInfoMenu.addItem(new ClistBoxItem(_("Year"), true, yearUserIntInput.getValue(), &yearUserIntInput, NULL, RC_7, NEUTRINO_ICON_BUTTON_7));

	// prod country
	movieInfoMenu.addItem(new ClistBoxItem(_("Country"), true, movie_info->productionCountry.c_str(), &countryUserInput, NULL, RC_8, NEUTRINO_ICON_BUTTON_8));

	// length
	movieInfoMenu.addItem(new ClistBoxItem(_("Length (Min)"), true, lengthUserIntInput.getValue(), &lengthUserIntInput,NULL, RC_9, NEUTRINO_ICON_BUTTON_9));

	// channel
	movieInfoMenu.addItem(new ClistBoxItem(_("Channel"), true, movie_info->epgChannel.c_str(), &channelUserInput, NULL, RC_0, NEUTRINO_ICON_BUTTON_0));//LOCALE_TIMERLIST_CHANNEL

	// path
	movieInfoMenu.addItem(new CMenuSeparator(LINE));
	movieInfoMenu.addItem(new ClistBoxItem(_("Path"), false, dirItNr)); //LOCALE_TIMERLIST_RECORDING_DIR

	// prev play date
	movieInfoMenu.addItem(new ClistBoxItem(_("Last play date"), false, dateUserDateInput.getValue()));//LOCALE_FLASHUPDATE_CURRENTVERSIONDATE

	// record date
	movieInfoMenu.addItem(new ClistBoxItem(_("Record date"), false, recUserDateInput.getValue()));//LOCALE_FLASHUPDATE_CURRENTVERSIONDATE

	// file size
	movieInfoMenu.addItem(new ClistBoxItem(_("File size (MB)"), false, size, NULL));

	// tmdb
	movieInfoMenu.addItem(new CMenuSeparator(LINE));
	movieInfoMenu.addItem(new ClistBoxItem(_("tmdb Info"), true, NULL, this, "tmdb"));

	// cut jumps
	movieInfoMenu.addItem(new CMenuSeparator(LINE));
	movieInfoMenu.addItem(new ClistBoxItem(_("Cut jumps"), true, NULL, this, "cut_jumps"));

	// truncate jumps
	movieInfoMenu.addItem(new ClistBoxItem(_("Truncate jumps"), true, NULL, this, "truncate_jumps"));

	// copy jumps
	movieInfoMenu.addItem(new ClistBoxItem(_("Copy jumps"), true, NULL, this, "copy_jumps"));

	// remove screenshot
	movieInfoMenu.addItem(new ClistBoxItem(_("remove screenshot?"), true, NULL, this, "remove_screenshot"));

	movieInfoMenu.exec(NULL, "");

	for(int i3 = 0 ; i3 < MI_MOVIE_BOOK_USER_MAX && i3 < MAX_NUMBER_OF_BOOKMARK_ITEMS; i3++ )
	{
		delete pBookNameInput[i3] ;
		delete pBookPosIntInput[i3] ;
		delete pBookTypeIntInput[i3];
		delete pBookItemMenu[i3];
	}
}

extern "C" int pinghost( const char *hostname );
bool CMovieBrowser::showMenu()
{
	//first clear screen
	m_pcWindow->paintBackground();
	m_pcWindow->blit();

	int i;

	// directory menu
	CDirMenu dirMenu(&m_dir);

	// parentallock menu
	CMenuWidget parentalMenu(_("Parental Lock"), NEUTRINO_ICON_MOVIE);

	parentalMenu.setWidgetMode(MODE_SETUP);
	parentalMenu.enableShrinkMenu();
	
	parentalMenu.addItem( new CMenuOptionChooser(_("activated"), (int*)(&m_parentalLock), MESSAGEBOX_PARENTAL_LOCK_OPTIONS, MESSAGEBOX_PARENTAL_LOCK_OPTIONS_COUNT, true ));

	parentalMenu.addItem( new CMenuOptionChooser(_("Lock movies from"), (int*)(&m_settings.parentalLockAge), MESSAGEBOX_PARENTAL_LOCKAGE_OPTIONS, MESSAGEBOX_PARENTAL_LOCKAGE_OPTION_COUNT, true ));

	// optionsMenuDir
	CMenuWidget optionsMenuDir(_("Additional paths"), NEUTRINO_ICON_MOVIE);

	optionsMenuDir.setWidgetMode(MODE_SETUP);
	optionsMenuDir.enableShrinkMenu();
	
	optionsMenuDir.addItem( new CMenuOptionChooser(_("Use record directory"), (int*)(&m_settings.storageDirRecUsed), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
	optionsMenuDir.addItem( new ClistBoxItem(_("Path"), false ,g_settings.network_nfs_recordingdir));

	optionsMenuDir.addItem( new CMenuOptionChooser(_("Use movie directory"), (int*)(&m_settings.storageDirMovieUsed), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
	optionsMenuDir.addItem( new ClistBoxItem (_("Path"), false , g_settings.network_nfs_moviedir));
	
	optionsMenuDir.addItem(new CMenuSeparator(LINE | STRING, _("Paths")));
	
	CFileChooser * dirInput[MB_MAX_DIRS];
	CMenuOptionChooser * chooser[MB_MAX_DIRS];
	COnOffNotifier * notifier[MB_MAX_DIRS];
	ClistBoxItem * forwarder[MB_MAX_DIRS];
	
	for(i = 0; i < MB_MAX_DIRS ;i++)
	{
		dirInput[i] =  new CFileChooser(&m_settings.storageDir[i]);
		forwarder[i] = new ClistBoxItem(_("Path"), m_settings.storageDirUsed[i], m_settings.storageDir[i].c_str(), dirInput[i]);
		notifier[i] =  new COnOffNotifier(forwarder[i]);
		chooser[i] =   new CMenuOptionChooser(_("Use directory"), &m_settings.storageDirUsed[i], MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true, notifier[i]);
		optionsMenuDir.addItem(chooser[i] );
		optionsMenuDir.addItem(forwarder[i] );
		
		if(i != (MB_MAX_DIRS - 1))
			optionsMenuDir.addItem(new CMenuSeparator(EMPTY));
	}

	// optionsMenuBrowser
	CIntInput playMaxUserIntInput(_("last play max items"), (int&) m_settings.lastPlayMaxItems, 3);
	CIntInput recMaxUserIntInput(_("record max items"), (int&) m_settings.lastRecordMaxItems, 3);
	CIntInput browserFrameUserIntInput(_("frame hight"), (int&) m_settings.browserFrameHeight, 4);
	CIntInput browserRowNrIntInput(_("row nr"), (int&) m_settings.browserRowNr, 1);
	CIntInput *browserRowWidthIntInput[MB_MAX_ROWS];
	
	for(i = 0; i < MB_MAX_ROWS ;i++)
		browserRowWidthIntInput[i] = new CIntInput(_("row width"),(int&) m_settings.browserRowWidth[i], 3);

	CMenuWidget optionsMenuBrowser(_("Browser Options"), NEUTRINO_ICON_MOVIE);

	optionsMenuBrowser.setWidgetMode(MODE_SETUP);
	optionsMenuBrowser.enableShrinkMenu();
	
	optionsMenuBrowser.addItem( new ClistBoxItem(_("Number of lines last play"), true, playMaxUserIntInput.getValue(),   &playMaxUserIntInput));
	optionsMenuBrowser.addItem( new ClistBoxItem(_("Number of lines last record"), true, recMaxUserIntInput.getValue(), &recMaxUserIntInput));
	optionsMenuBrowser.addItem( new ClistBoxItem(_("Browser hight [Pixel]"), true, browserFrameUserIntInput.getValue(), &browserFrameUserIntInput));
	optionsMenuBrowser.addItem(new CMenuSeparator(LINE | STRING, _("Row settings")));
	optionsMenuBrowser.addItem( new ClistBoxItem(_("Row settings"), true, browserRowNrIntInput.getValue(), &browserRowNrIntInput));
	
	for(i = 0; i < MB_MAX_ROWS; i++)
	{
		optionsMenuBrowser.addItem( new CMenuOptionChooser(_("Row item"), (int*)(&m_settings.browserRowItem[i]), MESSAGEBOX_BROWSER_ROW_ITEM, MESSAGEBOX_BROWSER_ROW_ITEM_COUNT, true ));
		optionsMenuBrowser.addItem( new ClistBoxItem(_("Row width"),    true, browserRowWidthIntInput[i]->getValue(), browserRowWidthIntInput[i]));

		if(i < MB_MAX_ROWS - 1)
			optionsMenuBrowser.addItem(new CMenuSeparator(EMPTY));
	}

	// optionsMenu
	CMenuWidget optionsMenu(_("Options"), NEUTRINO_ICON_MOVIE);

	optionsMenu.setWidgetMode(MODE_SETUP);
	optionsMenu.enableShrinkMenu();

	optionsMenu.addItem( new ClistBoxItem(_("Save changes"), true, NULL, this, "save_options", RC_red, NEUTRINO_ICON_BUTTON_RED));
	optionsMenu.addItem( new CMenuSeparator(LINE));
	optionsMenu.addItem( new ClistBoxItem(_("Load default settings"), true, NULL, this, "loaddefault", RC_green,  NEUTRINO_ICON_BUTTON_GREEN));
	optionsMenu.addItem( new ClistBoxItem(_("Browser Options"), true, NULL, &optionsMenuBrowser,NULL, RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	optionsMenu.addItem( new ClistBoxItem(_("Paths"), true, NULL, &optionsMenuDir,NULL, RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	optionsMenu.addItem( new CMenuSeparator(LINE));
	
	if(m_parentalLock != MB_PARENTAL_LOCK_OFF)
		optionsMenu.addItem( new CLockedlistBoxItem(_("Parental Lock"), g_settings.parentallock_pincode, true,  true, NULL, &parentalMenu, NULL, RC_nokey, NULL));
	else
		optionsMenu.addItem( new ClistBoxItem(_("Parental Lock"),   true, NULL, &parentalMenu, NULL, RC_nokey, NULL));
	
	optionsMenu.addItem( new CMenuSeparator(LINE));
	optionsMenu.addItem( new CMenuOptionChooser(_("Reload movie info at start"), (int*)(&m_settings.reload), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
	optionsMenu.addItem( new CMenuOptionChooser(_("Remount at start"), (int*)(&m_settings.remount), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
	optionsMenu.addItem( new CMenuSeparator(LINE));
	optionsMenu.addItem( new CMenuOptionChooser(_("Hide series"), (int*)(&m_settings.browser_serie_mode), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
	optionsMenu.addItem( new CMenuOptionChooser(_("Serie auto create"), (int*)(&m_settings.serie_auto_create), MESSAGEBOX_YES_NO_OPTIONS, MESSAGEBOX_YES_NO_OPTIONS_COUNT, true ));
 
	// main menu
	CMovieHelp * movieHelp = new CMovieHelp();
	CNFSSmallMenu * nfs = new CNFSSmallMenu();

	CMenuWidget mainMenu(_("Settings"), NEUTRINO_ICON_MOVIE);

	mainMenu.setWidgetMode(MODE_MENU);
	mainMenu.enableShrinkMenu();
	
	// show movie info
	mainMenu.addItem( new ClistBoxItem(_("Film Informationen"), (m_movieSelectionHandler != NULL), NULL, this, "show_movie_info_menu", RC_red, NEUTRINO_ICON_BUTTON_RED));

	// option menu
	mainMenu.addItem( new CMenuSeparator(LINE));
	mainMenu.addItem( new ClistBoxItem(_("Options"), true, NULL, &optionsMenu, NULL, RC_green,  NEUTRINO_ICON_BUTTON_GREEN));

	// dirs
	mainMenu.addItem( new ClistBoxItem(_("Paths"), true, NULL, &dirMenu, NULL, RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	// nfs
	mainMenu.addItem( new ClistBoxItem(_("NFS settings"), true, NULL, nfs, NULL, RC_blue,  NEUTRINO_ICON_BUTTON_BLUE));

	// help
	mainMenu.addItem( new CMenuSeparator(LINE));
	mainMenu.addItem( new ClistBoxItem(_("Help"), true, NULL, movieHelp, NULL, RC_info, NEUTRINO_ICON_BUTTON_HELP_SMALL));
    
	mainMenu.exec(NULL, " ");

	// post menu handling
	if (m_parentalLock != MB_PARENTAL_LOCK_OFF_TMP)
		m_settings.parentalLock = m_parentalLock;
	if(m_settings.browserFrameHeight < MIN_BROWSER_FRAME_HEIGHT )
		m_settings.browserFrameHeight = MIN_BROWSER_FRAME_HEIGHT;
	if(m_settings.browserFrameHeight > MAX_BROWSER_FRAME_HEIGHT)
		m_settings.browserFrameHeight = MAX_BROWSER_FRAME_HEIGHT;
	if (m_settings.browserRowNr > MB_MAX_ROWS ) 
		m_settings.browserRowNr = MB_MAX_ROWS;
	if (m_settings.browserRowNr < 1 ) 
		m_settings.browserRowNr = 1;
	
	for(i = 0; i < m_settings.browserRowNr; i++)
	{
		if( m_settings.browserRowWidth[i] > 500)
			m_settings.browserRowWidth[i] = 500;
		if( m_settings.browserRowWidth[i] < 10)
			m_settings.browserRowWidth[i] = 10;
	}

	if(dirMenu.isChanged())
		loadMovies();

	updateSerienames();
	refreshBrowserList();
	refreshLastPlayList();
	refreshLastRecordList();
	refreshFilterList();
	refresh();

	for(i = 0; i < MB_MAX_DIRS ;i++)
	{
		delete dirInput[i];
		delete notifier[i];
	}

	for(i = 0; i < MB_MAX_ROWS ;i++)
		delete browserRowWidthIntInput[i];

	delete movieHelp;
	delete nfs;

	return(true);
}

bool CMovieBrowser::isParentalLock(MI_MOVIE_INFO& movie_info)
{
	bool result = false;
	if(m_parentalLock == MB_PARENTAL_LOCK_ACTIVE && m_settings.parentalLockAge <= movie_info.parentalLockAge )
	{
		result = true;
	}
	return (result);
}

bool CMovieBrowser::isFiltered(MI_MOVIE_INFO& movie_info)
{
	bool result = true;
	
	switch(m_settings.filter.item)
	{
		case MB_INFO_FILEPATH:
			if(m_settings.filter.optionVar == movie_info.dirItNr)
				result = false;
			break;
				
		case MB_INFO_INFO1:
			if(strcmp(m_settings.filter.optionString.c_str(), movie_info.epgInfo1.c_str()) == 0) 
				result = false;
			break;
				
		case MB_INFO_MAJOR_GENRE:
			if(m_settings.filter.optionVar == movie_info.genreMajor)
				result = false;
			break;
				
		case MB_INFO_SERIE:
			if(strcmp(m_settings.filter.optionString.c_str(), movie_info.serieName.c_str()) == 0) 
				result = false;
			break;
				
		default:
			result = false;
			break;
	}
	
	return (result);
}

bool CMovieBrowser::getMovieInfoItem(MI_MOVIE_INFO& movie_info, MB_INFO_ITEM item, std::string* item_string)
{
	#define MAX_STR_TMP 100
	char str_tmp[MAX_STR_TMP];
	bool result = true;
	*item_string="";
	tm* tm_tmp;
	
	char text[20];
	int i=0;
	int counter=0;

	switch(item)
	{
		case MB_INFO_FILENAME: 				// 		= 0,
			*item_string = movie_info.file.getFileName();
			break;
			
		case MB_INFO_FILEPATH: 				// 		= 1,
			if(m_dirNames.size() > 0)
				*item_string = m_dirNames[movie_info.dirItNr];
			break;
			
		case MB_INFO_TITLE: 				// 		= 2,
			*item_string = movie_info.epgTitle;
			if(strcmp("not available",movie_info.epgTitle.c_str()) == 0)
				result = false;
			if(movie_info.epgTitle.empty())
				result = false;
			break;
			
		case MB_INFO_SERIE: 				// 		= 3,
			*item_string = movie_info.serieName;
			break;
			
		case MB_INFO_INFO1: 			//		= 4,
			*item_string = movie_info.epgInfo1;
			break;
		case MB_INFO_MAJOR_GENRE: 			// 		= 5,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.genreMajor);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_MINOR_GENRE: 			// 		= 6,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.genreMinor);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_INFO2: 					// 		= 7,
			*item_string = movie_info.epgInfo2;
			break;
			
		case MB_INFO_PARENTAL_LOCKAGE: 					// 		= 8,
			snprintf(str_tmp,MAX_STR_TMP,"%2d",movie_info.parentalLockAge);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_CHANNEL: 				// 		= 9,
			*item_string = movie_info.epgChannel;
			break;
			
		case MB_INFO_BOOKMARK: 				//		= 10,
			// we just return the number of bookmarks
			for(i = 0; i < MI_MOVIE_BOOK_USER_MAX; i++)
			{
				if(movie_info.bookmarks.user[i].pos != 0) 
					counter++;
			}
			snprintf(text, 8,"%d",counter);
			text[9] = 0; // just to make sure string is terminated
			*item_string = text;
			break;
			
		case MB_INFO_QUALITY: 				// 		= 11,
			snprintf(str_tmp,MAX_STR_TMP,"%d",movie_info.quality);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_PREVPLAYDATE: 			// 		= 12,
			tm_tmp = localtime(&movie_info.dateOfLastPlay);
			snprintf(str_tmp,MAX_STR_TMP,"%02d.%02d.%02d",tm_tmp->tm_mday,(tm_tmp->tm_mon)+ 1, tm_tmp->tm_year >= 100 ? tm_tmp->tm_year-100 : tm_tmp->tm_year);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_RECORDDATE: 			// 		= 13,
			tm_tmp = localtime(&movie_info.file.Time);
			snprintf(str_tmp,MAX_STR_TMP,"%02d.%02d.%02d",tm_tmp->tm_mday,(tm_tmp->tm_mon) + 1,tm_tmp->tm_year >= 100 ? tm_tmp->tm_year-100 : tm_tmp->tm_year);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_PRODDATE: 				// 		= 14,
			snprintf(str_tmp,MAX_STR_TMP,"%d",movie_info.productionDate);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_COUNTRY: 				// 		= 15,
			*item_string = movie_info.productionCountry;
			break;
			
		case MB_INFO_GEOMETRIE: 			// 		= 16,
			result = false;
			break;
			
		case MB_INFO_AUDIO: 				// 		= 17,
#if 1  // MB_INFO_AUDIO test
			// we just return the number of audiopids
			char _text[10];
			snprintf(_text, 8,"%d",movie_info.audioPids.size());
			_text[9] = 0; // just to make sure string is terminated
			*item_string = _text;
#else // MB_INFO_AUDIO test
			for(i=0; i < movie_info.audioPids.size() && i < 10; i++)
			{
				if(movie_info.audioPids[i].epgAudioPidName[0].size() < 2)
				{
					_text[counter++] = '?'; // two chars ??? -> strange name
					continue;
				}
				
				// check for Dolby Digital / AC3 Audio audiopids (less than 5.1 is not remarkable)
				if(	(movie_info.audioPids[i].epgAudioPidName.find("AC3") != -1 ) || 
					(movie_info.audioPids[i].epgAudioPidName.find("5.1") != -1 ))
				{
					ac3_found = true;
				}
				// Check for german audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'D' || // Deutsch
					movie_info.audioPids[i].epgAudioPidName[0] == 'd' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'G' || // German
					movie_info.audioPids[i].epgAudioPidName[0] == 'g' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'M' || // for Mono, mono and Stereo, stereo we assume German ;)
					movie_info.audioPids[i].epgAudioPidName[0] == 'n' || 
					(movie_info.audioPids[i].epgAudioPidName[0] == 'S' && movie_info.audioPids[i].epgAudioPidName[1] == 't' ) || 
					(movie_info.audioPids[i].epgAudioPidName[0] == 's' && movie_info.audioPids[i].epgAudioPidName[1] == 't' ))
				{
					text[counter++] = 'D';
					continue;
				}
				// Check for english audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'E' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'e')
				{
					_text[counter++] = 'E';
					continue;
				}
				// Check for french audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'F' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'f')
				{
					_text[counter++] = 'F';
					continue;
				}
				// Check for italian audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'I' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'i')
				{
					_text[counter++] = 'I';
					continue;
				}
				// Check for spanish audio pids
				if( movie_info.audioPids[i].epgAudioPidName[0] == 'E' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'e' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 'S' ||
					movie_info.audioPids[i].epgAudioPidName[0] == 's')
				{
					_text[counter++] = 'S';
					continue;
				}
				_text[counter++] = '?'; // We have not found any language for this pid
			}
			if(ac3_found == true)
			{
				_text[counter++] = '5';
				_text[counter++] = '.';
				_text[counter++] = '1';
			}
			_text[counter] = 0; // terminate string 
#endif	// MB_INFO_AUDIO test
			break;
			
		case MB_INFO_LENGTH: 				// 		= 18,
			snprintf(str_tmp,MAX_STR_TMP,"%4d",movie_info.length);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_SIZE: 					// 		= 19, 
			snprintf(str_tmp,MAX_STR_TMP,"%4llu",movie_info.file.Size>>20);
			*item_string = str_tmp;
			break;
			
		case MB_INFO_MAX_NUMBER: 			//		= 20 
		default:
			*item_string="";
			result = false;
			break;
	}
	
	return(result);
}

void CMovieBrowser::updateSerienames(void)
{
	//if(m_seriename_stale == false) 
	//	return;
		
	m_vHandleSerienames.clear();

	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		if(!m_vMovieInfo[i].serieName.empty())
		{
			// current series name is not empty, lets see if we already have it in the list, and if not save it to the list.
			bool found = false;
			for(unsigned int t = 0; t < m_vHandleSerienames.size() && found == false; t++)
			{
				if(strcmp(m_vHandleSerienames[t]->serieName.c_str(), m_vMovieInfo[i].serieName.c_str()) == 0)
					found = true;
			}
			
			if(found == false)
				m_vHandleSerienames.push_back(&m_vMovieInfo[i]);
		}
	}
	
	//dprintf(DEBUG_NORMAL, "[mb]->updateSerienames: %d\r\n",m_vHandleSerienames.size());
	// TODO sort(m_serienames.begin(), m_serienames.end(), my_alphasort);
	
	//m_seriename_stale = false;
}	

void CMovieBrowser::autoFindSerie(void)
{
	dprintf(DEBUG_INFO, "autoFindSerie\n");
	
	updateSerienames(); // we have to make sure that the seriename array is up to date, otherwise this does not work
			    // if the array is not stale, the function is left immediately
			    
	for(unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		// For all movie infos, which do not have a seriename, we try to find one.
		// We search for a movieinfo with seriename, and than we do check if the title is the same
		// in case of same title, we assume both belongs to the same serie
		//dprintf(DEBUG_NORMAL, "%s ",m_vMovieInfo[i].serieName);
		if( m_vMovieInfo[i].serieName.empty())
		{
			for(unsigned int t = 0; t < m_vHandleSerienames.size(); t++)
			{
				if(m_vMovieInfo[i].epgTitle == m_vHandleSerienames[t]->epgTitle )
				{
					 //dprintf(DEBUG_NORMAL, "x");
					 m_vMovieInfo[i].serieName = m_vHandleSerienames[t]->serieName;
					 break; // we  found a maching serie, nothing to do else, leave for(t=0)
				}
			}
		}
	}
}

int CMovieHelp::exec(CMenuTarget* /*parent*/, const std::string&/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CMovieHelp::exec:\n");

	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();

	CHelpBox helpbox;

	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, "Sortierung ändern");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, "Filterfenster einblenden");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_YELLOW, "Aktives Fenster wechseln");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, "Filminfos neu laden");
	helpbox.addLine(NEUTRINO_ICON_BUTTON_SETUP, "Hauptmenü");
	helpbox.addLine("'+/-'  Ansicht wechseln");

	helpbox.show(_("Information"));

	return RETURN_REPAINT;
}

/////////////////////////////////////////////////
// MenuTargets
////////////////////////////////////////////////
int CFileChooser::exec(CMenuTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CFileChooser::exec:\n");

	if(parent != NULL)
		parent->hide();

	CFileBrowser browser;
	browser.Dir_Mode = true;
	
	if (browser.exec(dirPath->c_str()))
	{
		*dirPath = browser.getSelectedFile()->Name;
		short a = dirPath->compare(0, 5, "/mnt/");
		short b = dirPath->compare(0, 7, "/media/");
		short c = dirPath->compare(0, 5, "/hdd/");
		if(a != 0 && b != 0 && c != 0)
			*dirPath = "";   // We clear the  string if the selected folder is not at leaset /mnt/ or /hdd (There is no other possibility to clear this) 
	}
	  
	return RETURN_REPAINT;
}

CDirMenu::CDirMenu(std::vector<MB_DIR>* dir_list)
{
	unsigned int i;
	changed = false;
	dirList = dir_list;

	if( dirList->empty())
	    return;

	for(i = 0; i < MAX_DIR; i++)
	    dirNfsMountNr[i]=-1;

	for(i = 0; i < dirList->size() && i < MAX_DIR; i++)
	{
		for(int nfs = 0; nfs < NETWORK_NFS_NR_OF_ENTRIES; nfs++)
		{
			std::string tmp = g_settings.network_nfs_local_dir[nfs];
			int result = -1;
			if(tmp.size())
				result = (*dirList)[i].name.compare( 0,tmp.size(),tmp) ;
			printf("[CDirMenu] (nfs%d) %s == (mb%d) %s (%d)\n",nfs,g_settings.network_nfs_local_dir[nfs],i,(*dirList)[i].name.c_str(),result);
			
			if(result == 0)
			{
				dirNfsMountNr[i] = nfs;
				break;
			}
		}
	}
};

int CDirMenu::exec(CMenuTarget* parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CDirMenu::exec:\n");

	int returnval = RETURN_REPAINT;

	if(actionKey == "")
	{
		if(parent)
			parent->hide();

		changed = false;
		show();
	}
	else if(actionKey.size() == 1)
	{
		printf("[CDirMenu].exec %s\n",actionKey.c_str());
		int number = atoi(actionKey.c_str());
		
		if(number < MAX_DIR)
		{
			if(dirState[number] == DIR_STATE_SERVER_DOWN)
			{
				std::string command = "etherwake ";
				command += g_settings.network_nfs_mac[dirNfsMountNr[number]];
				printf("try to start server: %s\n",command.c_str());
				if(system(command.c_str()) != 0)
					perror("etherwake failed");

				dirOptionText[number]= "STARTE SERVER";
			}
			else if(dirState[number] == DIR_STATE_NOT_MOUNTED)
			{
				printf("[CDirMenu] try to mount %d,%d\n",number,dirNfsMountNr[number]);
				CFSMounter::MountRes res;
				res = CFSMounter::mount(  g_settings.network_nfs_ip[dirNfsMountNr[number]].c_str(),
						    g_settings.network_nfs_dir[dirNfsMountNr[number]] ,
						    g_settings.network_nfs_local_dir[dirNfsMountNr[number]] ,
						    (CFSMounter::FSType)g_settings.network_nfs_type[dirNfsMountNr[number]] ,
						    g_settings.network_nfs_username[dirNfsMountNr[number]] ,
						    g_settings.network_nfs_password[dirNfsMountNr[number]] ,
						    g_settings.network_nfs_mount_options1[dirNfsMountNr[number]] ,
						    g_settings.network_nfs_mount_options2[dirNfsMountNr[number]] );
				if(res ==  CFSMounter::MRES_OK) // if mount is successful we set the state to active in any case
				{
					*(*dirList)[number].used = true;
				}
				// try to mount
				updateDirState();
				changed = true;
			}
			else if(dirState[number] == DIR_STATE_MOUNTED)
			{
				if(*(*dirList)[number].used == true)
				{
					*(*dirList)[number].used = false;
				}
				else
				{
					*(*dirList)[number].used = true;
				}
				//CFSMounter::umount(g_settings.network_nfs_local_dir[dirNfsMountNr[number]]);
				updateDirState();
				changed = true;
			}
		}
	}

	return returnval;
}

void CDirMenu::updateDirState(void)
{
	unsigned int drivefree = 0;
	struct statfs s;

	for(unsigned int i = 0; i < dirList->size() && i < MAX_DIR; i++)
	{
		dirOptionText[i]="UNBEKANNT";
		dirState[i]=DIR_STATE_UNKNOWN;
		// 1st ping server
		printf("updateDirState: %d: state %d nfs %d\n", i, dirState[i], dirNfsMountNr[i]);
		if(dirNfsMountNr[i] != -1)
		{
			int retvalue = pinghost(g_settings.network_nfs_ip[dirNfsMountNr[i]].c_str());
			if (retvalue == 0)//LOCALE_PING_UNREACHABLE
			{
				dirOptionText[i]="Server, offline";
				dirState[i] = DIR_STATE_SERVER_DOWN;
			}
			else if (retvalue == 1)//LOCALE_PING_OK
			{
				if(CFSMounter::isMounted (g_settings.network_nfs_local_dir[dirNfsMountNr[i]]) == 0)
				{
					dirOptionText[i] = "Not mounted";
					dirState[i] = DIR_STATE_NOT_MOUNTED;
				}
				else
				{
					dirState[i]=DIR_STATE_MOUNTED;
				}
			}
		}
		else
		{
			// not a nfs dir, probably IDE? we accept this so far
			dirState[i]=DIR_STATE_MOUNTED;
		}
		if(dirState[i] == DIR_STATE_MOUNTED)
		{
			if(*(*dirList)[i].used == true)
			{
				if (statfs((*dirList)[i].name.c_str(), &s) >= 0 )
				{
					drivefree = (s.f_bfree * s.f_bsize)>>30;
					char tmp[20];
					snprintf(tmp, 19,"%3d GB free",drivefree);
					tmp[19]=0;
					dirOptionText[i]=tmp;
				}
				else
				{
					dirOptionText[i]="? GB free";
				}
			}
			else
			{
				dirOptionText[i] = "Inactive";
			}
		}
	}
}

void CDirMenu::show(void)
{
	if(dirList->empty())
		return;
	
	char tmp[20];
	
	CMenuWidget dirMenu(_("Additional paths"), NEUTRINO_ICON_MOVIE);
	dirMenu.enableSaveScreen();

	dirMenu.setWidgetMode(MODE_MENU);
	dirMenu.enableShrinkMenu();
	
	updateDirState();
	for(unsigned int i = 0; i < dirList->size() && i < MAX_DIR; i++)
	{
		sprintf(tmp,"%d",i);
		tmp[1]=0;
		dirMenu.addItem( new ClistBoxItem( (*dirList)[i].name.c_str(), (dirState[i] != DIR_STATE_UNKNOWN), dirOptionText[i].c_str(), this, tmp));
	}
	
	dirMenu.exec(NULL," ");
	return;

}

// onoff notifier needed by moviebrowser
COnOffNotifier::COnOffNotifier( CMenuItem* a1,CMenuItem* a2,CMenuItem* a3,CMenuItem* a4,CMenuItem* a5)
{
        number = 0;
        if(a1 != NULL){ toDisable[0] = a1; number++;};
        if(a2 != NULL){ toDisable[1] = a2; number++;};
        if(a3 != NULL){ toDisable[2] = a3; number++;};
        if(a4 != NULL){ toDisable[3] = a4; number++;};
        if(a5 != NULL){ toDisable[4] = a5; number++;};
}

COnOffNotifier::COnOffNotifier(int /*OffValue*/)
{
	number = 0;
}

bool COnOffNotifier::changeNotify(const std::string&, void *Data)
{
	if(*(int*)(Data) == 0)
	{
		for (int i = 0; i < number ; i++)
			toDisable[i]->setActive(false);
	}
	else
	{
		for (int i = 0; i < number ; i++)
			toDisable[i]->setActive(true);
	}
	
	return true;
}

void COnOffNotifier::addItem(CMenuItem* menuItem)
{
	if (number < 15)
	{
		toDisable[number] = menuItem;
		number++;
	}
}



