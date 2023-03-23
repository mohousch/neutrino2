/*
  $Id: nfilm.cpp 2018/07/31 mohousch Exp $

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

#include <system/ytparser.h>	

extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

class CNFilm : public CMenuTarget
{
	public:
		enum{
			WIDGET_LEFT = 1,
			WIDGET_RIGHT
		};

	private:
		// variables
		CFrameBuffer* frameBuffer;
		
		CBox mainBox;
		CBox headBox;
		CBox footBox;
		CBox leftBox;
		CBox rightBox;

		CHeaders *headersWidget;
		CWidget *mainWidget;
		ClistBox *leftWidget;
		ClistBox *rightWidget;
		CFooters *footersWidget;

		CMenuItem* item;

		int selected;
		int left_selected;
		int right_selected;

		//
		CTmdb* tmdb;
		std::string thumbnail_dir;
		CFileHelpers fileHelper;
		cYTFeedParser ytparser;

		bool loadGenres;
		int genre_id;
		std::string plist;
		int page;
		std::string TVShows;

		//
		CMovieInfo m_movieInfo;
		std::vector<MI_MOVIE_INFO> m_vMovieInfo;

		std::vector<tmdbinfo> genres;
		std::vector<tmdbinfo> mvlist;
		std::vector<tmdbinfo> mglist;

		std::string tmdbsearch;
		
		CMoviePlayerGui tmpMoviePlayerGui;

		void loadGenreList(bool genre = false);
		void loadTMDBPlaylist(bool genre = false, const char *txt = "movie", const char *list = "popular", const int seite = 1, bool search = false);

		void createThumbnailDir();
		void removeThumbnailDir();
		void showMovieInfo(MI_MOVIE_INFO& movie);
		void getMovieVideoUrl(MI_MOVIE_INFO& movie);

		void paintLeftWidgetItems(ClistBox *listBox, bool genre = false);
		void paintRightWidgetItems(ClistBox *listBox);

		void showMenu(bool genre = false);

	public:
		CNFilm(int id = 0, bool genre = false);
		~CNFilm();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CNFilm::CNFilm(int id, bool genre)
{
	frameBuffer = CFrameBuffer::getInstance();

	loadGenres = genre;

	//
	mainWidget = NULL;
	leftWidget = NULL;
	rightWidget = NULL;
	headersWidget = NULL;
	footersWidget = NULL;

	item = NULL;

	tmdb = NULL;
	thumbnail_dir = "/tmp/nfilm";
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	TVShows = "movies";
	plist = "popular";
	genre_id = id;
	page = 1;

	//
	selected = 0;
	left_selected = 0;
	right_selected = 0;

	tmdbsearch.clear();
}

CNFilm::~CNFilm()
{
	m_vMovieInfo.clear();
	fileHelper.removeDir(thumbnail_dir.c_str());

	mvlist.clear();
	genres.clear();
	mglist.clear();

	tmdbsearch.clear();
}

void CNFilm::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CNFilm::createThumbnailDir()
{
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

void CNFilm::removeThumbnailDir()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CNFilm::loadGenreList(bool genre)
{
	if(genre)
	{
		CHintBox loadBox(_("Movie Trailer"), _("Scan for Movies ..."));
		loadBox.paint();

		genres.clear();

		tmdb = new CTmdb();
		tmdb->clearGenreList();
		tmdb->getGenreList();

		genres = tmdb->getGenres();

		delete tmdb;
		tmdb = NULL;

		loadBox.hide();
	}
}

void CNFilm::loadTMDBPlaylist(bool genre, const char *txt, const char *list, const int seite, bool search)
{
	dprintf(DEBUG_NORMAL, "CNFilm::loadPlaylist:\n");

	CHintBox loadBox(_("Movie Trailer"), _("Scan for Movies ..."));
	loadBox.paint();

	thumbnail_dir = "/tmp/nfilm";
	page = seite;
	plist = list;
	TVShows = txt;

	m_vMovieInfo.clear();

	fileHelper.removeDir(thumbnail_dir.c_str());
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	//
	tmdb = new CTmdb();

	if(genre)
	{
		mglist.clear();
		tmdb->clearGenreMovieList();
		tmdb->getGenreMovieList(genre_id);

		mglist = tmdb->getGenreMovies();
	
		for (unsigned int i = 0; i < mglist.size(); i++) 
		{
			MI_MOVIE_INFO movieInfo;
			m_movieInfo.clearMovieInfo(&movieInfo);
		
			movieInfo.epgTitle = mglist[i].title;
		
			movieInfo.epgInfo1 = mglist[i].overview;
			movieInfo.ytdate = mglist[i].release_date;
			movieInfo.vote_average = mglist[i].vote_average;
			movieInfo.vote_count = mglist[i].vote_count;
			movieInfo.original_title = mglist[i].original_title;
			//movieInfo.release_date = mglist[i].release_date;
			movieInfo.media_type = mglist[i].media_type;
			movieInfo.length = mglist[i].runtime;
			movieInfo.runtimes = mglist[i].runtimes;
			movieInfo.genres = mglist[i].genres;
			movieInfo.cast = mglist[i].cast;
			movieInfo.seasons = mglist[i].seasons;
			movieInfo.episodes = mglist[i].episodes;
			
			std::string tname = thumbnail_dir;
			tname += "/";
			tname += movieInfo.epgTitle;
			tname += ".jpg";

			tmdb->getSmallCover(mglist[i].poster_path, tname);

			if(!tname.empty())
				movieInfo.tfile = tname;

			// video url (the first one)
			tmdb->clearVideoInfo();
			tmdb->getVideoInfo("movie", mglist[i].id);

			std::vector<tmdbinfo>& videoInfo_list = tmdb->getVideoInfos();

			movieInfo.vid = videoInfo_list[0].vid;
			movieInfo.vkey = videoInfo_list[0].vkey;
			movieInfo.vname = videoInfo_list[0].vname;
		
			m_vMovieInfo.push_back(movieInfo);
		}
	}
	else
	{
		mvlist.clear();

		if(search)
		{
			tmdb->clearMInfo();
			tmdb->searchMovieInfo(txt);
			mvlist = tmdb->getMInfos();
		}
		else
		{
			tmdb->clearMovieList();

			tmdb->getMovieTVList(TVShows, plist, page);

			mvlist = tmdb->getMovies();
		}
	
		// fill our structure
		for(unsigned int i = 0; i < mvlist.size(); i++)
		{
			MI_MOVIE_INFO movieInfo;
			m_movieInfo.clearMovieInfo(&movieInfo);

			movieInfo.epgTitle = mvlist[i].title;
			movieInfo.epgInfo1 = mvlist[i].overview;
			movieInfo.ytdate = mvlist[i].release_date;
			movieInfo.vote_average = mvlist[i].vote_average;
			movieInfo.vote_count = mvlist[i].vote_count;
			movieInfo.original_title = mvlist[i].original_title;
			//movieInfo.release_date = mvlist[i].release_date;
			movieInfo.media_type = mvlist[i].media_type;
			movieInfo.length = mvlist[i].runtime;
			movieInfo.runtimes = mvlist[i].runtimes;
			movieInfo.genres = mvlist[i].genres;
			movieInfo.cast = mvlist[i].cast;
			movieInfo.seasons = mvlist[i].seasons;
			movieInfo.episodes = mvlist[i].episodes;
			
			std::string tname = thumbnail_dir;
			tname += "/";
			tname += movieInfo.epgTitle;
			tname += ".jpg";

			tmdb->getSmallCover(mvlist[i].poster_path, tname);

			if(!tname.empty())
				movieInfo.tfile = tname;

			// video url (the first one)
			tmdb->clearVideoInfo();
			tmdb->getVideoInfo("movie", mvlist[i].id);

			std::vector<tmdbinfo>& videoInfo_list = tmdb->getVideoInfos();

			movieInfo.vid = videoInfo_list[0].vid;
			movieInfo.vkey = videoInfo_list[0].vkey;
			movieInfo.vname = videoInfo_list[0].vname;

			m_vMovieInfo.push_back(movieInfo);
		}
	}

	delete tmdb;
	tmdb = NULL;

	loadBox.hide();
}

void CNFilm::showMovieInfo(MI_MOVIE_INFO& movie)
{
	dprintf(DEBUG_NORMAL, "CNFilm::showMovieInfo:\n");

	m_movieInfo.showMovieInfo(movie);
}

void CNFilm::getMovieVideoUrl(MI_MOVIE_INFO& movie)
{
/*
	ytparser.Cleanup();

	// setregion
	ytparser.SetRegion("DE");

	// set max result
	ytparser.SetMaxResults(1);
			
	// parse feed
	if (ytparser.ParseFeed(cYTFeedParser::SEARCH_BY_ID, movie.vname, movie.vkey))
	{
		yt_video_list_t &ylist = ytparser.GetVideoList();
	
		for (unsigned int j = 0; j < ylist.size(); j++) 
		{
			movie.ytid = ylist[j].id;
			movie.file.Name = ylist[j].GetUrl();
		}
	}
*/
	movie.file.Name = "https://www.youtube.com/watch?v=";
	movie.file.Name += movie.vkey;
}

void CNFilm::paintLeftWidgetItems(ClistBox *listBox, bool genre)
{
	if(genre)
	{
		for (unsigned int count = 0; count < genres.size(); count++) 
		{
			listBox->addItem(new ClistBoxItem(genres[count].title.c_str(), true, NULL, new CNFilm(genres[count].id, true)));
		}

		delete tmdb;
		tmdb = NULL;
	}
	else
	{
		ClistBoxItem *item1 = new ClistBoxItem("In den Kinos", true, NULL, this, "movie_in_cinema");
		ClistBoxItem *item2 = new ClistBoxItem("Am", true, NULL, this, "movie_popular");
		item2->setOption("populärsten");
		item2->set2lines();
		ClistBoxItem *item3 = new ClistBoxItem("Am besten", true, NULL, this, "movie_top_rated");
		item3->setOption("bewertet");
		item3->set2lines();
		ClistBoxItem *item4 = new ClistBoxItem("Neue Filme", true, NULL, this, "movie_new");
		ClistBoxItem *item5 = new ClistBoxItem("Beenden", true, NULL, this, "exit");
	
		listBox->addItem(new ClistBoxItem("Suche", true, tmdbsearch.c_str(), this, "search"));
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(item1);
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(item2);
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(item3);
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(item4);
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(new ClistBoxItem("Genres", true, NULL, this, "genres"));
		listBox->addItem(new CMenuSeparator(LINE));
		listBox->addItem(item5);
	}
}

void CNFilm::paintRightWidgetItems(ClistBox *listBox)
{
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		std::string tmp = m_vMovieInfo[i].ytdate;
		tmp += " ";
		tmp += m_vMovieInfo[i].epgInfo1;

		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "mplay", RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->setHint(tmp.c_str());

		listBox->addItem(item);
	}
}
	
const struct button_label HeadButtons = { NEUTRINO_ICON_BUTTON_HELP, "" };

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, "Next Page" },
	{ NEUTRINO_ICON_BUTTON_GREEN, "Next Page" },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("next focus") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Start Menu") }
};

void CNFilm::showMenu(bool genre)
{
	dprintf(DEBUG_NORMAL, "CNFilm::showMenu:\n");

	mainWidget = new CWidget(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	mainWidget->enableSaveScreen();
	mainWidget->setSelected(selected);
	//mainWidget->enablePaintMainFrame();
	//mainWidget->setBackgroundColor(COL_DARK_TURQUOISE);

	// headwidget
	headBox.iWidth = frameBuffer->getScreenWidth();
	headBox.iHeight = 40;
	headBox.iX = frameBuffer->getScreenX();
	headBox.iY = frameBuffer->getScreenY();

	headersWidget = new CHeaders(headBox.iX, headBox.iY, headBox.iWidth, headBox.iHeight, "Movie Trailer", NEUTRINO_ICON_TMDB);

	headersWidget->setButtons(&HeadButtons, 1);
	headersWidget->enablePaintDate();
	//headersWidget->setColor(COL_BLUE);
	//headersWidget->setGradient(NOGRADIENT);
	//headersWidget->setCorner(NO_RADIUS);

	// foot
	footBox.iWidth = frameBuffer->getScreenWidth();
	footBox.iHeight = 40;
	footBox.iX = frameBuffer->getScreenX();
	footBox.iY = frameBuffer->getScreenY() + frameBuffer->getScreenHeight() - footBox.iHeight;

	footersWidget = new CFooters(footBox.iX, footBox.iY, footBox.iWidth, footBox.iHeight);

	//footersWidget->setColor(COL_BLUE);
	footersWidget->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	//footersWidget->setGradient(NOGRADIENT);
	//footersWidget->setCorner(NO_RADIUS);

	// leftwidget
	leftBox.iWidth = 250;
	leftBox.iHeight = mainWidget->getWindowsPos().iHeight - headBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	leftBox.iX = mainWidget->getWindowsPos().iX;
	leftBox.iY = mainWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE;

	leftWidget = new ClistBox(&leftBox);

	leftWidget->setSelected(left_selected);
	//leftWidget->setBackgroundColor(COL_BLUE);
	leftWidget->setOutFocus();

	loadGenreList(genre);
	paintLeftWidgetItems(leftWidget, genre);

	// rightwidget
	rightBox.iWidth = mainWidget->getWindowsPos().iWidth - INTER_FRAME_SPACE - leftBox.iWidth;
	rightBox.iHeight = mainWidget->getWindowsPos().iHeight - headBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	rightBox.iX = mainWidget->getWindowsPos().iX + leftBox.iWidth + INTER_FRAME_SPACE;
	rightBox.iY = mainWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE;

	//
	rightWidget = new ClistBox(&rightBox);
	rightWidget->setWidgetType(TYPE_FRAME);
	rightWidget->setItemsPerPage(5,2);
	rightWidget->setSelected(right_selected);
	rightWidget->enablePaintItemInfo();

	paintRightWidgetItems(rightWidget);

	mainWidget->addWidgetItem(headersWidget);
	mainWidget->addWidgetItem(leftWidget);
	mainWidget->addWidgetItem(rightWidget);
	mainWidget->addWidgetItem(footersWidget);

	mainWidget->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));
	mainWidget->addKey(RC_red, this, "nextPage");
	mainWidget->addKey(RC_green, this, "prevPage");
	mainWidget->addKey(RC_blue, this, "startMenu");

	mainWidget->exec(NULL, ""); // handler

	delete mainWidget;
	mainWidget = NULL;

	delete headersWidget;
	headersWidget = NULL;

	delete leftWidget;
	leftWidget = NULL;

	delete rightWidget;
	rightWidget = NULL;

	delete footersWidget;
	footersWidget = NULL;
}

int CNFilm::exec(CMenuTarget *parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CNFilm::exec: actionKey: %s\n", actionKey.c_str());

	if(parent)
		hide();

	if(actionKey == "RC_info")
	{
		if(mainWidget->getSelected() == WIDGET_RIGHT)
		{
			right_selected = rightWidget->getSelected();

			showMovieInfo(m_vMovieInfo[right_selected]);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "mplay")
	{
		if(mainWidget->getSelected() == WIDGET_RIGHT)
		{
			right_selected = rightWidget->getSelected();

			// get video url
			getMovieVideoUrl(m_vMovieInfo[right_selected]);
		
			// play
			if (&m_vMovieInfo[right_selected].file != NULL) 
			{
				CMovieInfoWidget movieInfoWidget;
				movieInfoWidget.setMovie(m_vMovieInfo[right_selected]);
		
				movieInfoWidget.exec(NULL, "");
			}
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "genres")
	{
		loadGenres = true;

		right_selected = 0;
		rightWidget->clearItems();

		left_selected = 0;
		leftWidget->clearItems();

		loadGenreList(loadGenres);

		if(genres.size())
			genre_id = genres[0].id; // action

		sleep(2);

		paintLeftWidgetItems(leftWidget, loadGenres);
		leftWidget->setSelected(0);

		loadTMDBPlaylist(loadGenres);

		// load items
		paintRightWidgetItems(rightWidget);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "search")
	{
		tmdbsearch.clear();

		CStringInputSMS stringInput(_("Search"), tmdbsearch.c_str());
		stringInput.exec(NULL, "");

		if(!stringInput.getExitPressed())
		{
			rightWidget->clearItems();

			loadTMDBPlaylist(false, tmdbsearch.c_str(), "", 1, true);

			// load items
			paintRightWidgetItems(rightWidget);

			rightWidget->setSelected(0);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_in_cinema")
	{
		right_selected = 0;
		rightWidget->clearItems();
		TVShows = "movie";
		plist = "now_playing";
		page = 1;

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_popular")
	{
		right_selected = 0;
		rightWidget->clearItems();
		TVShows = "movie";
		plist = "popular";
		page = 1;

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_top_rated")
	{
		right_selected = 0;
		rightWidget->clearItems();
		TVShows = "movie";
		plist = "top_rated";
		page = 1;

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_new")
	{
		right_selected = 0;
		rightWidget->clearItems();
		TVShows = "movie";
		plist = "upcoming";
		page = 1;

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "nextPage")
	{
		page++;
		right_selected = 0;
		rightWidget->clearItems();

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);

		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "prevPage")
	{
		page--;
		if(page <= 1)
			page = 1;
		right_selected = 0;
		rightWidget->clearItems();

		loadTMDBPlaylist(false, TVShows.c_str(), plist.c_str(), page);

		// load items
		paintRightWidgetItems(rightWidget);

		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "startMenu")
	{
		loadGenres = false;

		rightWidget->clearItems();
		leftWidget->clearItems();

		loadTMDBPlaylist();
		showMenu();

		return RETURN_REPAINT;
	}
	else if(actionKey == "exit")
	{
		return RETURN_EXIT_ALL;
	}

	loadTMDBPlaylist(loadGenres);
	showMenu(loadGenres);

	return RETURN_EXIT;
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CNFilm* nFilmHandler = new CNFilm();
	
	nFilmHandler->exec(NULL, "");
	
	delete nFilmHandler;
	nFilmHandler = NULL;
}


