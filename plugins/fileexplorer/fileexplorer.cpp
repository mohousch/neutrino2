/*
  $Id: fileexplorer.cpp 2018/07/22 mohousch Exp $

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


class CFileExplorer : public CMenuTarget
{
	private:
		CFileBrowser fileBrowser;
		std::string Path_local;

		CPictureViewerGui tmpPictureViewerGui;
		CMoviePlayerGui tmpMoviePlayerGui;
		CAudioPlayerGui tmpAudioPlayerGui;

		neutrino_msg_t msg;
		neutrino_msg_data_t data;

		void showMenu(void);

	public:
		CFileExplorer(){};
		~CFileExplorer(){};
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

//
void CFileExplorer::showMenu()
{	
	Path_local = "/media/hdd";
	
BROWSER:	
	if (fileBrowser.exec(Path_local.c_str())) 
	{
		Path_local = fileBrowser.getCurrentDir(); // remark path
		
		// get the current file name
		CFile * file;

		if ((file = fileBrowser.getSelectedFile()) != NULL) 
		{
			// parse file extension
			if(file->getType() == CFile::FILE_PICTURE)
			{				
				tmpPictureViewerGui.addToPlaylist(*file);
				tmpPictureViewerGui.exec(NULL, "urlplayback");
			}
			else if(file->getType() == CFile::FILE_VIDEO)
			{
				MI_MOVIE_INFO movieInfo;
				CMovieInfo m_movieInfo;
				
				m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
				movieInfo.file.Name = file->Name;
				
				// load movie infos (from xml file)
				m_movieInfo.loadMovieInfo(&movieInfo);
				
				CMovieInfoWidget movieInfoWidget;
				movieInfoWidget.setMovie(movieInfo);
		
				movieInfoWidget.exec(NULL, "");
			}
			else if(file->getType() == CFile::FILE_AUDIO)
			{
				tmpAudioPlayerGui.addToPlaylist(*file);
				tmpAudioPlayerGui.exec(NULL, "urlplayback");
			}
			else
			{
				std::string buffer;
				buffer.clear();

				buffer = CFileHelpers::getInstance()->loadFile(*file, 6000);

				CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
					
				CInfoBox * infoBox = new CInfoBox(&position, file->getFileName().c_str(), NEUTRINO_ICON_FILE);
				infoBox->setText(buffer.c_str());
				infoBox->exec();
				delete infoBox;
			}
		}

		g_RCInput->getMsg_ms(&msg, &data, 10);
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
}

int CFileExplorer::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CFileExplorer::exec: %s\n", actionKey.c_str());

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
	CFileExplorer* fileExplorerHandler = new CFileExplorer();

	fileExplorerHandler->exec(NULL, "");

	delete fileExplorerHandler;
	fileExplorerHandler = NULL;
}


