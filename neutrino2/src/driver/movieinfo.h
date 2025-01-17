//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: movieinfo.h 21122024 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Author: Günther@tuxbox.berlios.org
//		based on code of Steffen Hehn 'McClean'
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

#ifndef MOVIEINFO_H_
#define MOVIEINFO_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/file.h>


/* XML tags for xml file*/
#define MI_XML_TAG_NEUTRINO 		"neutrino"
#define MI_XML_TAG_RECORD 		"record"

#define MI_XML_TAG_CHANNELNAME 		"channelname"
#define MI_XML_TAG_EPGTITLE 		"epgtitle"
#define MI_XML_TAG_ID 			"id"
#define MI_XML_TAG_INFO1 		"info1"
#define MI_XML_TAG_INFO2 		"info2"
#define MI_XML_TAG_EPGID 		"epgid"
#define MI_XML_TAG_MODE 		"mode"
#define MI_XML_TAG_VIDEOPID 		"videopid"
#define MI_XML_TAG_VIDEOTYPE 		"videotype"
#define MI_XML_TAG_AUDIOPIDS 		"audiopids"
#define MI_XML_TAG_AUDIO 		"audio"
#define MI_XML_TAG_PID 			"pid"
#define MI_XML_TAG_NAME			"name"
#define MI_XML_TAG_ATYPE	        "audiotype"
#define MI_XML_TAG_SELECTED	        "selected"
#define MI_XML_TAG_VTXTPIDS	        "vtxtpids"
#define MI_XML_TAG_VTXT		        "vtxt"
#define MI_XML_TAG_PAGE		        "page"
#define MI_XML_TAG_LANGUAGE	        "language"
#define MI_XML_TAG_GENRE_MAJOR 		"genremajor"
#define MI_XML_TAG_GENRE_MINOR 		"genreminor"
#define MI_XML_TAG_SERIE_NAME 		"seriename"
#define MI_XML_TAG_LENGTH 		"length"
#define MI_XML_TAG_PRODUCT_COUNTRY  	"productioncountry"
#define MI_XML_TAG_PRODUCT_DATE 	"productiondate"
#define MI_XML_TAG_QUALITY 		"qualitiy"
#define MI_XML_TAG_PARENTAL_LOCKAGE	"parentallockage"
#define MI_XML_TAG_BOOKMARK 		"bookmark"
#define MI_XML_TAG_BOOKMARK_START	"bookmarkstart"
#define MI_XML_TAG_BOOKMARK_END		"bookmarkend"
#define MI_XML_TAG_BOOKMARK_LAST	"bookmarklast"
#define MI_XML_TAG_BOOKMARK_USER	"bookmarkuser"
#define MI_XML_TAG_BOOKMARK_USER_POS	"bookmarkuserpos"
#define MI_XML_TAG_BOOKMARK_USER_TYPE	"bookmarkusertype"
#define MI_XML_TAG_BOOKMARK_USER_NAME	"bookmarkusername"
#define MI_XML_TAG_DATE_OF_LAST_PLAY 	"dateoflastplay"
#define MI_XML_TAG_GENRES		"genres"
#define MI_XML_TAG_VOTE_AVERAGE		"vote_average"


#define MI_MAX_AUDIO_PIDS 4       // just to avoid the buffer is filled endless, might be increased later on , but 4 audio pids might be enough
#define MI_MOVIE_BOOK_USER_MAX 20 // just to avoid the buffer is filled endless, might be increased later on. Make sure to increase the bookmark menu as well

typedef enum
{
	MI_PARENTAL_OVER0 = 0,
	MI_PARENTAL_OVER6 = 6,
	MI_PARENTAL_OVER12 = 12,
	MI_PARENTAL_OVER16 = 16,
	MI_PARENTAL_OVER18 = 18,
	MI_PARENTAL_ALWAYS = 99,
	MI_PARENTAL_MAX_NUMBER = 100
}MI_PARENTAL_LOCKAGE;

typedef struct
{
	int pos;			// position in seconds from file start	
	int length;     		// bookmark type, 0: just a bookmark, < 0 jump back (seconds), > 0 jump forward (seconds) 
	std::string name; 		// bookmark name to be displayed
}MI_BOOKMARK;

typedef struct
{
	int start;			// movie start in seconds from file start
	int end;			// movie end in seconds from file start
	int lastPlayStop;		// position of last play stop in seconds from file start
	MI_BOOKMARK user[MI_MOVIE_BOOK_USER_MAX]; // other user defined bookmarks
}MI_MOVIE_BOOKMARKS;

typedef struct
{
	int atype;
	int selected;
	int  epgAudioPid; 		// epg audio pid nr, usually filled by VCR
	std::string epgAudioPidName; 	// epg audio pid name, usually filled by VCR
}EPG_AUDIO_PIDS;

typedef struct
{
	int pid;
	int page;
	std::string language;
}EPG_VTXT_PIDS;

typedef struct
{
	CFile file;				// not stored in xml
	std::string productionCountry;		// user defined Country (not from EPG yet, but might be possible)
	std::string epgTitle;			// plain movie name, usually filled by EPG
	std::string epgInfo1;			// used for Genre (Premiere) or second title, usually filled by EPG
	std::string epgInfo2;			// detailed movie content, usually filled by EPG
	std::string epgChannel;			// Channel name, usually filled by EPG
	std::string serieName;  		// user defines series name

	time_t dateOfLastPlay; 		// last play date of movie in seconds since 1970
	char dirItNr;  			// handle for quick directory path access only, this is not saved in xml, might be used by the owner of the movie info struct
	int  genreMajor;            		// see showEPG class for more info, usually filled by EPG
	char genreMinor;			// genreMinor not used so far			
	int  length;                		// movie length in minutes, usually filled by EPG
	int  quality;                 	// user classification (3 stars: classics, 2 stars: very good, 1 star: good, 0 stars: OK)
	int  productionDate;         		// user defined Country (not from EPG yet, but might be possible)
	int  parentalLockAge;        		// used for age rating(0:never,6,12,16,18 years,99:always), usually filled by EPG (if available)
	char format;				// currently not used
	char audio;				// currently not used
	MI_MOVIE_BOOKMARKS bookmarks;		// bookmark collecton  for this movie
	std::vector<EPG_AUDIO_PIDS> audioPids; // available AudioPids, usually filled by VCR. Note: Vectors are easy to is also using the heap (memory fragmentation), might be changed to array  [MI_MAX_AUDIO_PIDS]

	uint64_t  epgId;			// currently not used, we just do not want to loose this info if movie info is saved backed
	uint64_t epgEpgId;			// off_t currently not used, we just do not want to loose this info if movie info is saved backed
	int  epgMode;				// currently not used, we just do not want to loose this info if movie info is saved backed
	int  epgVideoPid; 			// currently not used, we just do not want to loose this info if movie info is saved backed
	int  VideoType;
	//
	std::vector<EPG_VTXT_PIDS> vtxtPids;

	//
	std::string original_title;
	float vote_average;
	int vote_count;
	std::string media_type;
	std::string runtimes;
	std::string genres;
	int episodes;
	int seasons;
	std::string cast;

	// yt
	std::string vid;
	std::string vkey;
	std::string vname;
	std::string tfile; 			// thumbnail/cover file name
	std::string ytdate; 			// yt published
	std::string ytid; 			// yt published
}MI_MOVIE_INFO;

typedef std::vector<MI_MOVIE_INFO> CMoviePlayList;

class CMovieInfo
{
	public:	// Functions
		CMovieInfo(){};
		virtual ~CMovieInfo(){};
		
		//
		bool convertTs2XmlName(std::string* filename);  				// convert a ts file name in .xml file name
		bool convertTs2XmlName(char* filename, int size);				// convert a ts file name in .xml file name
		bool loadMovieInfo(MI_MOVIE_INFO* movie_info, CFile* file = NULL );
		MI_MOVIE_INFO loadMovieInfo(const char *file);		
 		// load movie information for the given .xml filename. If there is no filename, the filename (ts) from movie_info is converted to xml and used instead
		bool encodeMovieInfoXml(std::string* extMessage, MI_MOVIE_INFO * movie_info); 	// encode the movie_info structure to xml string
		bool saveMovieInfo(MI_MOVIE_INFO& movie_info, CFile* file = NULL ); 		// encode the movie_info structure to xml and save it to the given .xml filename. If there is no filename, the filename (ts) from movie_info is converted to xml and used instead
		bool saveMovieInfo(const char* fileName, std::string title = "", std::string info1 = "", std::string info2 = "" ,CFile* file = NULL);
		void showMovieInfo(MI_MOVIE_INFO& movie_info); // open a CIntbox and show the movie info
		void clearMovieInfo(MI_MOVIE_INFO* movie_info);					// Set movie info structure to initial values
		bool addNewBookmark(MI_MOVIE_INFO* movie_info,MI_BOOKMARK &new_bookmark);	// add a new bookmark to the given movie info. If there is no space false is returned
		void copy(MI_MOVIE_INFO* src, MI_MOVIE_INFO* dst);
		
	private:// Functions
		bool parseXmlQuickFix(char* text, MI_MOVIE_INFO* movie_info);			// OK, this is very quick an dirty. It does not waist execution time nor flash (this is QUICK). But, do not play to much with the xml files (e.g. with MS Notepad) since small changes in the structure could cause the parser to fail (this it DIRTY). 
		bool loadFile(CFile& file, char* buffer, int buffer_size);
		bool saveFile(const CFile& file, const char* text, const int text_size);
};

#endif /*MOVIEINFO_H_*/

