/*
  $Id: youtube.h 2014/10/03 mohousch Exp $

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

#ifndef __YT__
#define __YT__

#include <plugin.h>
#include <system/ytparser.h>


#define YTBROWSER_SETTINGS_FILE          PLUGINDIR "/youtube/yt.conf"

// settings
typedef struct
{
	int ytorderby;
	std::string ytregion;
	int ytautoplay;
}YTB_SETTINGS;

class CYTBrowser : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;
		
		CConfigFile configfile;

		int ytmode;
		std::string ytvid;
		std::string ytsearch;
		
		void init(void);
		bool loadSettings(YTB_SETTINGS* settings);
		bool saveSettings(YTB_SETTINGS* settings);

		//
		CMoviePlayList m_vMovieInfo;
		CMovieInfo m_movieInfo;
		CMoviePlayerGui tmpMoviePlayerGui;

		cYTFeedParser ytparser;

		std::string getFeedLocale(void);
		void loadYTTitles(int mode, std::string search = "", std::string id = "", bool show_hint = true);
		int showCategoriesMenu(void);

		void playMovie(void);
		void showMovieInfo(void);
		void recordMovie(void);
		
		//
		CWidget* widget;
		ClistBox* moviesMenu;
		CMenuItem* item;

		void showMenu();
		
	public:
		CYTBrowser(int mode = cYTFeedParser::MOST_POPULAR);
		~CYTBrowser();
		int exec(CMenuTarget* parent, const std::string & actionKey);
		void hide();
}; 

#endif //__YT__
