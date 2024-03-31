/*
	Neutrino-GUI  -   DBoxII-Project

 	$id: movieinfo.h 01.04.2024 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

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

#ifndef MOVIEINFO_WIDGET_H_
#define MOVIEINFO_WIDGETH_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>

#include <driver/file.h>

#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/framebox.h>

#include <driver/movieinfo.h>


// CMovieInfoWidget
class CMovieInfoWidget : public CMenuTarget
{
	private:
		MI_MOVIE_INFO movieFile;
		CMovieInfo m_movieInfo;

		void funArt();
	public:
		CMovieInfoWidget();
		~CMovieInfoWidget();

		void hide();
		int exec(CMenuTarget *parent, const std::string &actionKey);

		void setMovie(MI_MOVIE_INFO& file);
		void setMovie(const CFile& file, std::string title = "", std::string info1 = "", std::string info2 = "", std::string tfile = "", std::string duration = "", std::string rating = "");
		void setMovie(const char* fileName, std::string title = "", std::string info1 = "", std::string info2 = "", std::string tfile = "", std::string duration = "", std::string rating = "");
};

#endif /*MOVIEINFO_WIDGET_H_*/

