/*
        Copyright (C) 2013 CoolStream International Ltd

        License: GPLv2

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation;

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __YT_PARSER__
#define __YT_PARSER__

#include <vector>
#include <string>
#include <map>

#include <system/helpers.h>


class cYTVideoUrl
{
	public:
		std::string quality;
		std::string type;
		std::string sig;
		std::string url;
};

typedef std::map<int, cYTVideoUrl> yt_urlmap_t;
typedef std::pair<int, cYTVideoUrl> yt_urlmap_pair_t;
typedef yt_urlmap_t::iterator yt_urlmap_iterator_t;

class cYTVideoInfo
{
	public:
		std::string id;
		std::string title;
		std::string author;
		std::string description;
		std::string category;
		std::string thumbnail;
		std::string tfile;
		std::string published;
		int duration;
		yt_urlmap_t formats;

		std::string GetUrl(int fmt = 0);
};

typedef std::vector<cYTVideoInfo> yt_video_list_t;

class cYTFeedParser
{
	private:
		std::string error;
		std::string thumbnail_dir;
		std::string curfeed;
		std::string curfeedfile;
		std::string tquality; 		// thumbnail size
		std::string region; 		// more results
		std::string next; 		// next results
		std::string prev; 		// prev results
		std::string nextprevurl; 	// url for next/prev
		std::string key; 		// youtube dev id

		CFileHelpers fileHelper;

		int feedmode;
		int max_results;
		bool parsed;
		yt_video_list_t videos;
		
		bool parseFeedJSON(std::string &answer);
		bool parseFeedDetailsJSON(cYTVideoInfo &vinfo);
		bool ParseVideoInfo(cYTVideoInfo &vinfo);
		bool decodeVideoInfo(std::string &answer, cYTVideoInfo &vinfo);
		bool supportedFormat(int fmt);
		bool ParseFeed(std::string &url);
	public:
		enum yt_feed_mode_t
		{
			MOST_POPULAR,
			MOST_POPULAR_ALL_TIME,
			FEED_LAST,
			NEXT,
			PREV,
			RELATED,
			SEARCH,
			SEARCH_BY_ID,
			MODE_LAST
		};
		
		enum yt_feed_orderby_t
		{
			ORDERBY_PUBLISHED = 0,
			ORDERBY_RELEVANCE,
			ORDERBY_VIEWCOUNT,
			ORDERBY_RATING
		};
		
		cYTFeedParser();
		~cYTFeedParser();

		bool ParseFeed(yt_feed_mode_t mode = MOST_POPULAR, std::string search = "", std::string vid = "", yt_feed_orderby_t orderby = ORDERBY_PUBLISHED);
		bool DownloadThumbnails();
		void Cleanup(bool delete_thumbnails = true);

		yt_video_list_t &GetVideoList() { return videos; }
		bool Parsed() { return parsed; }
		int GetFeedMode() { return feedmode; }
		bool HaveNext(void) { return !next.empty(); }
		bool HavePrev(void) { return !prev.empty(); }

		void SetRegion(std::string reg) { region = reg; }
		void SetMaxResults(int count) { max_results = count; }
};

#endif
 
