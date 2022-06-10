/*
  $Id: mediaplayer.cpp 2018/07/22 mohousch Exp $

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


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);


class CMediaPlayer : CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;

		CMoviePlayerGui tmpMoviePlayerGui;
					
		CFileBrowser * fileBrowser;
		CFileFilter fileFilter;

		MI_MOVIE_INFO mfile;
		CMovieInfo cMovieInfo;
		
		std::string Path;

		void showMenu();

	public:
		CMediaPlayer();
		~CMediaPlayer();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CMediaPlayer::CMediaPlayer()
{
	frameBuffer = CFrameBuffer::getInstance();

	//					
	fileBrowser = NULL;

	fileFilter.addFilter("ts");
	fileFilter.addFilter("mpg");
	fileFilter.addFilter("mpeg");
	fileFilter.addFilter("divx");
	fileFilter.addFilter("avi");
	fileFilter.addFilter("mkv");
	fileFilter.addFilter("asf");
	fileFilter.addFilter("aiff");
	fileFilter.addFilter("m2p");
	fileFilter.addFilter("mpv");
	fileFilter.addFilter("m2ts");
	fileFilter.addFilter("vob");
	fileFilter.addFilter("mp4");
	fileFilter.addFilter("mov");	
	fileFilter.addFilter("flv");	
	fileFilter.addFilter("dat");
	fileFilter.addFilter("trp");
	fileFilter.addFilter("vdr");
	fileFilter.addFilter("mts");
	fileFilter.addFilter("wmv");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("wma");
	fileFilter.addFilter("ogg");
}

CMediaPlayer::~CMediaPlayer()
{
}

void CMediaPlayer::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CMediaPlayer::showMenu()
{
	fileBrowser = new CFileBrowser();
	
	fileBrowser->Multi_Select = true;
	fileBrowser->Filter = &fileFilter;
	
	Path = g_settings.network_nfs_moviedir;

BROWSER:
	if (fileBrowser->exec(Path.c_str()))
	{
		Path = fileBrowser->getCurrentDir();

		CFileList::const_iterator files = fileBrowser->getSelectedFiles().begin();
		for(; files != fileBrowser->getSelectedFiles().end(); files++)
		{
			cMovieInfo.clearMovieInfo(&mfile);

			mfile.file.Name = files->Name;

			cMovieInfo.loadMovieInfo(&mfile, &mfile.file);
					
			tmpMoviePlayerGui.addToPlaylist(mfile);
		}
		
		tmpMoviePlayerGui.exec(NULL, "");
		
		neutrino_msg_t msg;
		neutrino_msg_data_t data;

		g_RCInput->getMsg_ms(&msg, &data, 10);
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

int CMediaPlayer::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMediaPlayer::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();

	showMenu();
	
	return RETURN_REPAINT;
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CMediaPlayer* mediaPlayerHandler = new CMediaPlayer();
	
	mediaPlayerHandler->exec(NULL, "");
	
	delete mediaPlayerHandler;
	mediaPlayerHandler = NULL;
}
