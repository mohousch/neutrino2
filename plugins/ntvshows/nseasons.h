/*
  $Id: nseasons.h 2018/08/03 mohousch Exp $

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


class CNSeasons : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;
		CMenuWidget* listBox;
		CMenuItem* item;
		int selected;

		CMovieInfo m_movieInfo;
		std::vector<MI_MOVIE_INFO> m_vMovieInfo; // items

		CTmdb* tmdb;
		std::string thumbnail_dir;
		CFileHelpers fileHelper;

		int season_id;

		std::vector<tmdbinfo> seasons;
		int episode_id;

		void createThumbnailDir();
		void removeThumbnailDir();

		void loadSeasonsTitle(void);
		void showMovieInfo(MI_MOVIE_INFO& movie);

		void showMenu();

	public:
		CNSeasons(int id);
		~CNSeasons();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};


