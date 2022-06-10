/*
  $Id: nseasons.cpp 2018/08/03 mohousch Exp $

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

#include "nseasons.h"
#include "nepisodes.h"


CNSeasons::CNSeasons(int id)
{
	frameBuffer = CFrameBuffer::getInstance();

	listBox = NULL;
	item = NULL;

	m_vMovieInfo.clear();

	selected = 0;

	tmdb = NULL;
	thumbnail_dir = "/tmp/nseasons";
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	season_id = id;
}

CNSeasons::~CNSeasons()
{
	m_vMovieInfo.clear();
	seasons.clear();

	fileHelper.removeDir(thumbnail_dir.c_str()); 
}

void CNSeasons::hide()
{
	CFrameBuffer::getInstance()->clearFrameBuffer();
	CFrameBuffer::getInstance()->blit();
}

void CNSeasons::createThumbnailDir()
{
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

void CNSeasons::removeThumbnailDir()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CNSeasons::loadSeasonsTitle()
{
	m_vMovieInfo.clear();
	seasons.clear();

	removeThumbnailDir();
	createThumbnailDir();

	CHintBox loadBox(_("Serien Trailer"), _("Scan for Movies ..."));
	loadBox.paint();

	tmdb = new CTmdb();

	tmdb->clearSeasonList();
	tmdb->getSeasonsList(season_id);

	std::vector<tmdbinfo> &seasonlist = tmdb->getSeasons();
	
	for (unsigned int count = 0; count < seasonlist.size(); count++) 
	{
		MI_MOVIE_INFO Info;
		m_movieInfo.clearMovieInfo(&Info);
		
		Info.epgTitle = seasonlist[count].title;

		//
		Info.epgInfo1 = seasonlist[count].overview;
		Info.ytdate = seasonlist[count].release_date;

		std::string tname = thumbnail_dir;
		tname += "/";
		tname += seasonlist[count].poster_path;
		tname += ".jpg";

		if (!seasonlist[count].poster_path.empty())
		{

			::downloadUrl("http://image.tmdb.org/t/p/w185" + seasonlist[count].poster_path, tname);
		}

		if(!tname.empty())
			Info.tfile = tname;
		//
		tmdbinfo tmp;

		tmp.title = seasonlist[count].title;
		tmp.id = seasonlist[count].id;
		tmp.episodes = seasonlist[count].episodes;

		seasons.push_back(tmp);
		
		m_vMovieInfo.push_back(Info);
	}

	delete tmdb;
	tmdb = NULL;
}

void CNSeasons::showMovieInfo(MI_MOVIE_INFO& movie)
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

#define HEAD_BUTTONS_COUNT	1
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
};

void CNSeasons::showMenu()
{
	dprintf(DEBUG_NORMAL, "CNSeasons::showMenu:\n");

	listBox = new CMenuWidget("Staffeln", NEUTRINO_ICON_MOVIE, CFrameBuffer::getInstance()->getScreenWidth(), CFrameBuffer::getInstance()->getScreenHeight());
	
	
	// load playlist
	 loadSeasonsTitle();

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		if(seasons[i].title == m_vMovieInfo[i].epgTitle)
			episode_id = seasons[i].id; 

		std::string tmp = m_vMovieInfo[i].ytdate;
		tmp += " ";
		tmp += m_vMovieInfo[i].epgInfo1;

		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, new CNEpisodes(season_id, i), NULL, RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/neutrino/icons/nopreview.jpg");

		item->setHint(tmp.c_str());

		listBox->addItem(item);
	}

	listBox->setWidgetMode(MODE_LISTBOX);
	listBox->setWidgetType(WIDGET_TYPE_FRAME);
	listBox->setItemsPerPage(6, 2);
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

int CNSeasons::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CNSeasons::exec: %s\n", actionKey.c_str());

	if(parent)
		hide();

	if(actionKey == "RC_info")
	{
		selected = listBox->getSelected();

		showMovieInfo(m_vMovieInfo[selected]);

		return RETURN_REPAINT;
	}

	showMenu();

	return RETURN_REPAINT;
}


