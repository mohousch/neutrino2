/*
  $Id: ntvshows.cpp 24112024 mohousch Exp $

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

#include <plugin.h>

#include <fstream>
#include <jsoncpp/include/json/json.h>
#include <system/ytparser.h>

#include "nseasons.h"


//
extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

//// defines
//FIXME: make this global
#undef _
#define _(string) dgettext("ntvshows", string)

class CTVShows : public CTarget
{
	private:
		//
		int selected;

		//
		ClistBox *mlist;
		CMenuItem *item;

		std::string caption;

		//
		CTmdb* tmdb;
		std::string thumbnail_dir;
		CFileHelpers fileHelper;
		cYTFeedParser ytparser;

		CMovieInfo m_movieInfo;
		std::vector<MI_MOVIE_INFO> m_vMovieInfo;
		std::vector<MI_MOVIE_INFO> list;

		//
		std::vector<tmdbinfo> db_movies;

		std::string plist;
		unsigned int page;
		int season_id;
		
		CMoviePlayerGui tmpMoviePlayerGui;

		void loadPlaylist();
		void createThumbnailDir();
		void removeThumbnailDir();
		void showMovieInfo(MI_MOVIE_INFO& movie);

		void loadMoviesTitle(void);
		int showCategoriesMenu();

		void showMenu();
	
	public:
		CTVShows(std::string tvlist = "popular");
		~CTVShows();
		int exec(CTarget* parent, const std::string& actionKey);
		void hide();
};

CTVShows::CTVShows(std::string tvlist)
{
	//
	mlist = NULL;
	item = NULL;

	tmdb = NULL;
	thumbnail_dir = "/tmp/ntvshows";
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	//
	selected = 0;

	plist = tvlist;
	page = 1;
	season_id = 0;
}

CTVShows::~CTVShows()
{
	m_vMovieInfo.clear();
	list.clear();
	db_movies.clear();

	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CTVShows::hide()
{
	CFrameBuffer::getInstance()->clearFrameBuffer();
	CFrameBuffer::getInstance()->blit();
}

void CTVShows::createThumbnailDir()
{
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

void CTVShows::removeThumbnailDir()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CTVShows::loadMoviesTitle(void)
{
	list.clear();
	db_movies.clear();

	removeThumbnailDir();
	createThumbnailDir();

	CHintBox loadBox(_("Series Trailer"), _("Scan for Movies ..."));
	loadBox.paint();

	//
	tmdb = new CTmdb();

	tmdb->clearMovieList();

	tmdb->getMovieTVList("tv", plist, page);

	std::vector<tmdbinfo> &mvlist = tmdb->getMovies();
	
	for (unsigned int count = 0; count < mvlist.size(); count++) 
	{
		MI_MOVIE_INFO Info;
		m_movieInfo.clearMovieInfo(&Info);
		
		Info.epgTitle = mvlist[count].title;
		Info.epgInfo1 = mvlist[count].overview;
		Info.ytdate = mvlist[count].release_date;

		std::string tname = thumbnail_dir;
		tname += "/";
		tname += mvlist[count].poster_path;
		tname += ".jpg";

		if (!mvlist[count].poster_path.empty())
		{

			::downloadUrl("http://image.tmdb.org/t/p/w185" + mvlist[count].poster_path, tname);
		}

		if(!tname.empty())
			Info.tfile = tname;

		//
		tmdbinfo tmp;

		tmp.title = mvlist[count].title;
		tmp.id = mvlist[count].id;
		tmp.seasons = mvlist[count].seasons;

		db_movies.push_back(tmp);
		
		list.push_back(Info);
	}
	
	delete tmdb;
	tmdb = NULL;
}

void CTVShows::loadPlaylist()
{
	m_vMovieInfo.clear();

	//
	tmdb = new CTmdb();
	
	// refill our structure
	for (unsigned int i = 0; i < db_movies.size(); i++)
	{
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); 

		movieInfo.epgTitle = db_movies[i].title;

		tmdb->clearMovieInfo();
		tmdb->getMovieTVInfo("tv", db_movies[i].id);
		std::vector<tmdbinfo>& movieInfo_list = tmdb->getMovieInfos();

		movieInfo.epgInfo1 = movieInfo_list[0].overview;
		movieInfo.ytdate = movieInfo_list[0].release_date;
		movieInfo.vote_average = movieInfo_list[0].vote_average;
		movieInfo.vote_count = movieInfo_list[0].vote_count;
		movieInfo.original_title = movieInfo_list[0].original_title;
		//movieInfo.release_date = movieInfo_list[0].release_date;
		movieInfo.media_type = movieInfo_list[0].media_type;
		movieInfo.length = movieInfo_list[0].runtime;
		movieInfo.runtimes = movieInfo_list[0].runtimes;
		movieInfo.genres = movieInfo_list[0].genres;
		movieInfo.cast = movieInfo_list[0].cast;
		movieInfo.seasons = movieInfo_list[0].seasons;
		movieInfo.episodes = movieInfo_list[0].episodes;
			
		std::string tname = thumbnail_dir;
		tname += "/";
		tname += movieInfo.epgTitle;
		tname += ".jpg";

		tmdb->getSmallCover(movieInfo_list[0].poster_path, tname);

		if(!tname.empty())
			movieInfo.tfile = tname;
					
		// 
		m_vMovieInfo.push_back(movieInfo);
	}

	delete tmdb;
	tmdb = NULL;
}

void CTVShows::showMovieInfo(MI_MOVIE_INFO& movie)
{
	//// FIXME:
	hide();
	
	//
	std::string buffer;
	
	// prepare print buffer 
	buffer = movie.epgTitle;
	buffer += "\n"; 
	buffer += movie.epgInfo1;
	buffer += "\n";
	buffer += movie.epgInfo2;

	// thumbnail
	int pich = 246;		//FIXME
	int picw = 162; 	//FIXME

	std::string thumbnail = movie.tfile;
	if(access(thumbnail.c_str(), F_OK))
		thumbnail = "";
	
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, movie.epgTitle.c_str(), NEUTRINO_ICON_MOVIE);

	infoBox->setText(buffer.c_str(), thumbnail.c_str(), picw, pich);
	infoBox->exec();
	delete infoBox;
}

#define HEAD_BUTTONS_COUNT	2
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " }
};

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Next Page") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Prev Page") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, " " },
	{ NEUTRINO_ICON_BUTTON_BLUE, " " }
};

void CTVShows::showMenu()
{
	dprintf(DEBUG_NORMAL, "CTVShows::showMenu:");

	if(plist == "airing_today")
		caption = _("Airing today");
	else if(plist == "on_the_air")
		caption = _("On the air");
	else if(plist == "popular")
		caption = _("Popular");
	else if(plist == "top_rated")
		caption = _("Top rated");

	mlist = new ClistBox(CFrameBuffer::getInstance()->getScreenX() + 20, CFrameBuffer::getInstance()->getScreenY() + 20, CFrameBuffer::getInstance()->getScreenWidth() - 40, CFrameBuffer::getInstance()->getScreenHeight() - 40);
	
	mlist->enablePaintHead();
	mlist->setTitle(caption.c_str(), NEUTRINO_ICON_TMDB);
	mlist->enablePaintDate();
	mlist->enablePaintFoot();
	mlist->enablePaintItemInfo();

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		//
		if(db_movies[i].title == m_vMovieInfo[i].epgTitle)
			season_id = db_movies[i].id; 
		//

		std::string tmp = m_vMovieInfo[i].ytdate;
		tmp += " ";
		tmp += m_vMovieInfo[i].epgInfo1;

		item = new CMenuForwarder(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, new CNSeasons(season_id), NULL, CRCInput::RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->setHint(tmp.c_str());

		mlist->addItem(item);
	}

	mlist->setWidgetMode(ClistBox::MODE_LISTBOX);
	mlist->setWidgetLayout(ClistBox::LAYOUT_FRAME);
	mlist->setItemsPerPage(6, 2);

	mlist->setSelected(selected);

	mlist->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	mlist->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);

	mlist->addKey(CRCInput::RC_info, this, CRCInput::getSpecialKeyName(CRCInput::RC_info));
	mlist->addKey(CRCInput::RC_setup, this, CRCInput::getSpecialKeyName(CRCInput::RC_setup));
	mlist->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	mlist->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));

	mlist->exec(this);
	
	if (mlist)
	{
		delete mlist;
		mlist = NULL;
	}
}

int CTVShows::showCategoriesMenu()
{
	dprintf(DEBUG_NORMAL, "showCategoriesMenu:\n");

	int res = -1;

	ClistBox * menu = new ClistBox(CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - 600)/2, CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - 600)/2, 600, 600);
	
	menu->enablePaintHead();
	menu->setTitle(_("Series Trailer"));
	menu->enablePaintDate();
	menu->enablePaintFoot();
	
	menu->setWidgetMode(ClistBox::MODE_MENU);
	menu->enableShrinkMenu();

	menu->addItem(new CMenuForwarder(_("Airing today"), true, NULL, new CTVShows("airing_today"), "airing_today"));
	menu->addItem(new CMenuForwarder(_("On the air"), true, NULL, new CTVShows("on_the_air"), "on_the_air"));
	menu->addItem(new CMenuForwarder(_("Popular"), true, NULL, new CTVShows("popular"), "popular"));
	menu->addItem(new CMenuForwarder(_("Top rated"), true, NULL, new CTVShows("top_rated"), "top_rated"));

	menu->exec(this);

	res = menu->getSelected();
	
	if (menu)
	{
		delete menu;
		menu = NULL;
	}

	return res;
}

int CTVShows::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CTVShows::exec: actionKey: %s\n", actionKey.c_str());

	hide();

	if(actionKey == "RC_info")
	{
		selected = mlist->getSelected();

		showMovieInfo(m_vMovieInfo[selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_setup")
	{
		int res = showCategoriesMenu();

		if(res >= 0)
		{
			loadMoviesTitle();
			loadPlaylist();
			showMenu();

			return RETURN_EXIT_ALL;
		}
		else
			return RETURN_REPAINT;
	}
	else if(actionKey == "RC_red")
	{
		page++;
		selected = 0;
		loadMoviesTitle();
		loadPlaylist();
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		page--;

		if(page <= 1)
			page = 1;

		selected = 0;

		loadMoviesTitle();
		loadPlaylist();
		showMenu();

		return RETURN_EXIT_ALL;
	}

	loadMoviesTitle();
	loadPlaylist();
	showMenu();

	return RETURN_EXIT_ALL;
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CTVShows* nTVShowsHandler = new CTVShows();

	nTVShowsHandler->exec(NULL, "");
	
	delete nTVShowsHandler;
	nTVShowsHandler = NULL;
}

