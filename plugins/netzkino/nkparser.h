/*
        Copyright (C) 2013 CoolStream International Ltd
        Copyright (C) 2013 martii

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

#ifndef __NK_PARSER__
#define __NK_PARSER__

#include <vector>
#include <string>
#include <map>

#include <system/helpers.h>


struct sNKVideoInfo
{
	std::string id;
	std::string title;
	std::string description;// content
	std::string url;	// stream url
	std::string thumbnail;	// thumbnail url
	std::string tfile;	// thumbnail local file
	std::string published;	// modified, actually
};

struct sNKCategory
{
	int id;
	std::string title;
	int post_count;
};

typedef std::vector<sNKVideoInfo> nk_video_list_t;
typedef std::vector<sNKCategory> nk_category_list_t;

class cNKFeedParser
{
	private:

		nk_category_list_t categories;
		nk_video_list_t videos;

		std::string thumbnail_dir;
		CFileHelpers fileHelper;

		int max_results;
		bool parsed;

		bool parseFeedJSON(std::string &answer);
		bool parseCategoriesJSON(std::string &answer);
		bool ParseFeed(std::string &url);
	public:
		enum nk_feed_mode_t
		{
			CATEGORY,
			SEARCH,
			MODE_LAST
		};
		
		cNKFeedParser();
		~cNKFeedParser();

		bool ParseFeed(nk_feed_mode_t mode, std::string search, int category);
		bool ParseCategories(void);
		bool DownloadThumbnails(/*unsigned int start, unsigned int end*/);
		void Cleanup();
		void CleanupThumbnails();

		nk_video_list_t &GetVideoList() { return videos; }
		nk_category_list_t &GetCategoryList() { ParseCategories(); return categories; }
		bool Parsed() { return parsed; }
};

#endif
