/*
  $Id: tsplayer.cpp 2018/07/10 mohousch Exp $

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
#include "moviebrowser.h"


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

class CMoviePlayer : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;

		//
		CMoviePlayerGui tmpMoviePlayerGui;	
		CMovieBrowser * movieBrowser;
		MI_MOVIE_INFO * mfile;

		std::string Path;

		void showMenu();		
		
	public:
		CMoviePlayer();
		~CMoviePlayer();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CMoviePlayer::CMoviePlayer()
{
	frameBuffer = CFrameBuffer::getInstance();

	//	
	movieBrowser = NULL;
	mfile = NULL;
}

CMoviePlayer::~CMoviePlayer()
{
}

void CMoviePlayer::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CMoviePlayer::showMenu()
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	
	movieBrowser = new CMovieBrowser();
	
	Path = g_settings.network_nfs_moviedir;

BROWSER:
	if (movieBrowser->exec(Path.c_str()))
	{
		Path = movieBrowser->getCurrentDir();
		
		if (movieBrowser->getSelectedFile()!= NULL) 
		{
			mfile = movieBrowser->getCurrentMovieInfo();
					
			tmpMoviePlayerGui.addToPlaylist(*mfile);
			tmpMoviePlayerGui.exec(NULL, "");
		}

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != CRCInput::RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete movieBrowser;
}

int CMoviePlayer::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayer::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();

	showMenu();
	
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
	CMoviePlayer* moviePlayerHandler = new CMoviePlayer();
	
	moviePlayerHandler->exec(NULL, "");
	
	delete moviePlayerHandler;
	moviePlayerHandler = NULL;
}


