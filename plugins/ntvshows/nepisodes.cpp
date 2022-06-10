/*
  $Id: nepisodes.cpp 2018/08/04 mohousch Exp $

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

#include "nepisodes.h"


CNEpisodes::CNEpisodes(int id, int nr)
{
	frameBuffer = CFrameBuffer::getInstance();

	listBox = NULL;
	item = NULL;

	m_vMovieInfo.clear();

	selected = 0;

	tmdb = NULL;
	thumbnail_dir = "/tmp/episodes";
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	season_id = id;
	ep_nr = nr;
}

CNEpisodes::~CNEpisodes()
{
	m_vMovieInfo.clear();

	fileHelper.removeDir(thumbnail_dir.c_str()); 
}

void CNEpisodes::hide()
{
	CFrameBuffer::getInstance()->clearFrameBuffer();
	CFrameBuffer::getInstance()->blit();
}

void CNEpisodes::createThumbnailDir()
{
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

void CNEpisodes::removeThumbnailDir()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CNEpisodes::loadEpisodesTitle()
{
	m_vMovieInfo.clear();

	removeThumbnailDir();
	createThumbnailDir();

	CHintBox loadBox(_("Serien Trailer"), _("Scan for Movies ..."));
	loadBox.paint();

	tmdb = new CTmdb();

	tmdb->clearEpisodeList();
	tmdb->getEpisodesList(season_id, ep_nr);

	std::vector<tmdbinfo> &episodelist = tmdb->getEpisodes();
	
	for (unsigned int count = 0; count < episodelist.size(); count++) 
	{
		MI_MOVIE_INFO Info;
		m_movieInfo.clearMovieInfo(&Info);
		
		Info.epgTitle = episodelist[count].title;

		//
		Info.epgInfo1 = episodelist[count].overview;
		Info.ytdate = episodelist[count].release_date;

		std::string tname = thumbnail_dir;
		tname += "/";
		tname += episodelist[count].poster_path;
		tname += ".jpg";

		if (!episodelist[count].poster_path.empty())
		{

			::downloadUrl("http://image.tmdb.org/t/p/w185" + episodelist[count].poster_path, tname);
		}

		if(!tname.empty())
			Info.tfile = tname;

		tmdb->clearVideoInfo();


		if(tmdb->getVideoInfo("tv", episodelist[count].id))
		{
			std::vector<tmdbinfo>& videoInfo_list = tmdb->getVideoInfos();
	
			
			Info.vid = videoInfo_list[0].vid;
			Info.vkey = videoInfo_list[0].vkey;
			Info.vname = videoInfo_list[0].vname;
		} 
		
		m_vMovieInfo.push_back(Info);
	}

	delete tmdb;
	tmdb = NULL;
}

void CNEpisodes::showMovieInfo(MI_MOVIE_INFO& movie)
{
	std::string buffer;
	
	// prepare print buffer  
	buffer += movie.epgInfo1;
	buffer += "\n";
	buffer += movie.epgInfo2;

	// thumbnail
	int pich = 246;	//FIXME
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

void CNEpisodes::getMovieVideoUrl(MI_MOVIE_INFO& movie)
{
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
}

#define HEAD_BUTTONS_COUNT	1
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
};

void CNEpisodes::showMenu()
{
	dprintf(DEBUG_NORMAL, "CNEpisodes::showMenu:\n");

	listBox = new CMenuWidget("Folgen", NEUTRINO_ICON_MOVIE, CFrameBuffer::getInstance()->getScreenWidth(), CFrameBuffer::getInstance()->getScreenHeight());
	
	
	// load playlist
	loadEpisodesTitle();

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		std::string tmp = m_vMovieInfo[i].ytdate;
		tmp += " ";
		tmp += m_vMovieInfo[i].epgInfo1;

		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "mplay", RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/neutrino/icons/nopreview.jpg");

		item->setHint(m_vMovieInfo[i].epgInfo1.c_str());

		listBox->addItem(item);
	}

	listBox->setWidgetMode(MODE_LISTBOX);
	listBox->setWidgetType(WIDGET_TYPE_FRAME);
	listBox->setItemsPerPage(3, 2);
	//listBox->setItemBoxColor(COL_YELLOW);
	listBox->setSelected(selected);
	listBox->enablePaintDate();
	//listBox->enablePaintFootInfo();

	listBox->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);

	listBox->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));

	listBox->exec(NULL, "");
	//listBox->hide();
	delete listBox;
	listBox = NULL;
}

int CNEpisodes::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CNEpisodes::exec: %s\n", actionKey.c_str());

	if(parent)
		hide();

	if(actionKey == "RC_info")
	{
		selected = listBox->getSelected();

		showMovieInfo(m_vMovieInfo[selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "mplay")
	{
		selected = listBox->getSelected();

		// get video url
		getMovieVideoUrl(m_vMovieInfo[selected]);
		
		if (&m_vMovieInfo[selected].file != NULL) 
		{
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[selected]);
			tmpMoviePlayerGui.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}

	showMenu();

	return RETURN_REPAINT;
}


