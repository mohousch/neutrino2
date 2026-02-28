/*
  $Id: movieplayer.cpp 06.07.2022 mohousch Exp $

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


//// defines
//FIXME: make this global
#undef _
#define _(string) dgettext("movieplayer", string)

//// globals
extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

class CMoviePlayer : public CTarget
{
	private:
		// variables
		CFrameBuffer* frameBuffer;
		int selected;

		//
		CWidget* widget;
		ClistBox* mlist;
		CMenuItem* item;

		//
		CMoviePlayerGui tmpMoviePlayerGui;
		CMovieInfo m_movieInfo;
		CMoviePlayList m_vMovieInfo;
		CFileFilter fileFilter;
		CFileList filelist;
		std::string Path;

		//
		void loadPlaylist();
		void doTMDB(MI_MOVIE_INFO& movieFile);
		bool delFile(CFile& file);
		void onDeleteFile(MI_MOVIE_INFO& movieFile);
		void openFileBrowser();
		void showMenu();

	public:
		CMoviePlayer();
		~CMoviePlayer();
		int exec(CTarget* parent, const std::string& actionKey);
		void hide();
};

CMoviePlayer::CMoviePlayer()
{
	frameBuffer = CFrameBuffer::getInstance();

	//
	widget = NULL;
	mlist = NULL;
	item = NULL;

	//
	selected = 0;

	//
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
	fileFilter.addFilter("m3u8");
	fileFilter.addFilter("webm");
}

CMoviePlayer::~CMoviePlayer()
{
	m_vMovieInfo.clear();
}

void CMoviePlayer::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CMoviePlayer::loadPlaylist()
{
	m_vMovieInfo.clear();

	// recordingdir
	Path = g_settings.network_nfs_recordingdir;

	CHintBox loadBox(_("Movie Player"), _("Scan for Movies ..."));
	loadBox.paint();
	
	//
	//if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
	CFileHelpers::getInstance()->addRecursiveDir(&filelist, Path, &fileFilter);

	if(filelist.size() > 0)
	{
		// filter them
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure

		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			//
			m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
			movieInfo.file.Name = files->Name;
					
			// load movie infos
			m_movieInfo.loadMovieInfo(&movieInfo);
					
			// 
			m_vMovieInfo.push_back(movieInfo);
		}
	}

	// movie dir
	Path = g_settings.network_nfs_moviedir;
	filelist.clear();

	//if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
	CFileHelpers::getInstance()->addRecursiveDir(&filelist, Path, &fileFilter);

	if(filelist.size() > 0)
	{
		// filter them
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure

		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			//
			m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
			movieInfo.file.Name = files->Name;
					
			// load movie infos (from xml file)
			m_movieInfo.loadMovieInfo(&movieInfo);

			// skip duplicate
			for (unsigned long i = 0; i < m_vMovieInfo.size(); i++)
			{
				if(m_vMovieInfo[i].file.getFileName() == movieInfo.file.getFileName())
					m_vMovieInfo.erase(m_vMovieInfo.begin() + i); 
			}
					
			// 
			m_vMovieInfo.push_back(movieInfo);
		}
	}

	loadBox.hide();
}

void CMoviePlayer::openFileBrowser()
{
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_picturedir : "");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	if (filebrowser.exec(Path.c_str()))
	{
		Path = filebrowser.getCurrentDir();

		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure

		CHintBox loadBox(_("Movie Player"), _("Scan for Movies ..."));
		loadBox.paint();

		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for(; files != filebrowser.getSelectedFiles().end(); files++)
		{
			// filter them
			MI_MOVIE_INFO movieInfo;
			m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
			movieInfo.file.Name = files->Name;
					
			// load movie infos (from xml file)
			m_movieInfo.loadMovieInfo(&movieInfo);

			// skip duplicate
			for (unsigned long i = 0; i < m_vMovieInfo.size(); i++)
			{
				if(m_vMovieInfo[i].file.getFileName() == movieInfo.file.getFileName())
					m_vMovieInfo.erase(m_vMovieInfo.begin() + i); 
			}
					
			// 
			m_vMovieInfo.push_back(movieInfo);
		}

		loadBox.hide();
	}
}

void CMoviePlayer::doTMDB(MI_MOVIE_INFO& movieFile)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayer::doTMDB:\n");
	
	//				
	CTmdb * tmdb = new CTmdb();

	tmdb->clearMInfo();

	if(tmdb->getMovieInfo(movieFile.epgTitle))
	{
		std::vector<tmdbinfo>& minfo_list = tmdb->getMInfos();

		std::string buffer;

		buffer = movieFile.epgTitle;
		buffer += "\n\n";
	
		// prepare print buffer  
		buffer += "Vote: " + toString(minfo_list[0].vote_average) + "/10 Votecount: " + toString(minfo_list[0].vote_count);
		buffer += "\n\n";
		buffer += minfo_list[0].overview;
		buffer += "\n";

		buffer += (std::string)_("Length (Min)") + ": " + toString(minfo_list[0].runtime);
		buffer += "\n";

		buffer += (std::string)_("Genre") + ": " + minfo_list[0].genres;
		buffer += "\n";
		buffer += (std::string)_("Original Title") + " : " + minfo_list[0].original_title;
		buffer += "\n";
		buffer += (std::string)_("Year of production") + " : " + minfo_list[0].release_date.substr(0,4);
		buffer += "\n";

		if (!minfo_list[0].cast.empty())
			buffer += (std::string)_("Actors") + ":\n" + minfo_list[0].cast;

		// thumbnail
		std::string tname = tmdb->getThumbnailDir();
		tname += "/";
		tname += movieFile.epgTitle;
		tname += ".jpg";

		tmdb->getSmallCover(minfo_list[0].poster_path, tname);
		
		// scale pic
		int p_w = 0;
		int p_h = 0;

		scaleImage(tname, &p_w, &p_h);
	
		CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
		CInfoBox * infoBox = new CInfoBox(&position, movieFile.epgTitle.c_str(), NEUTRINO_ICON_TMDB);

		infoBox->setText(buffer.c_str(), tname.c_str(), p_w, p_h, CTextBox::PIC_RIGHT, true);
		infoBox->exec();
		delete infoBox;

		if(MessageBox(_("Information"), _("Prefer TMDB infos ?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes) 
		{
			// tfile
			std::string tname = movieFile.file.Name;
			changeFileNameExt(tname, ".jpg");

			if(tmdb->getSmallCover(minfo_list[0].poster_path, tname)) 
				movieFile.tfile = tname;

			// epgInfo1
			if(movieFile.epgInfo1.empty())
				movieFile.epgInfo1 = buffer;
			
			// productionDate	
			if (movieFile.productionDate == 0)
				movieFile.productionDate = atoi(minfo_list[0].release_date.substr(0,4));
			
			// genres	
			if(movieFile.genres.empty())
				movieFile.genres = minfo_list[0].genres;
				
			// average
			if (movieFile.vote_average == 0)
				movieFile.vote_average = minfo_list[0].vote_average;

			m_movieInfo.saveMovieInfo(movieFile);
		}  
	}
	else
	{
		MessageBox(_("Information"), _("Not available"), CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
	}

	delete tmdb;
	tmdb = NULL;
}

bool CMoviePlayer::delFile(CFile& file)
{
	bool result = true;
	unlink(file.Name.c_str()); // fix: use full path
	dprintf(DEBUG_NORMAL, "CMoviePlayer::delete file: %s\r\n", file.Name.c_str());
	return(result);
}

void CMoviePlayer::onDeleteFile(MI_MOVIE_INFO& movieFile)
{
	std::string msg = _("Delete");
	msg += "\r\n ";

	if (movieFile.file.Name.length() > 40)
	{
		msg += movieFile.file.Name.substr(0, 40);
		msg += "...";
	}
	else
		msg += movieFile.file.Name;
			
	msg += "\r\n ";
	msg += "?";

	if (MessageBox(_("Delete"), msg.c_str(), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes)
	{
		delFile(movieFile.file);
                       
		// delete cover
		if (movieFile.tfile != DATADIR "/icons/no_coverArt.png" && movieFile.tfile != DATADIR "/icons/nopreview.jpg") 
                	unlink(movieFile.tfile.c_str());

		CFile file_xml  = movieFile.file; 
		if(m_movieInfo.convertTs2XmlName(&file_xml.Name) == true)  
		{
			delFile(file_xml);
	    	}
	    	
		m_vMovieInfo.erase( (std::vector<MI_MOVIE_INFO>::iterator)&movieFile);
	}
}

#define HEAD_BUTTONS_COUNT	2
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, "" },
	{ NEUTRINO_ICON_BUTTON_MUTE_SMALL, "" },
};

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("TMDB") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Add")  },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Delete all") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Scan for Movies ...") },
};

void CMoviePlayer::showMenu()
{
	//
	widget = new CWidget(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());
	widget->name = "movieplayer";
	
	//
	mlist = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);


	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new CMenuForwarder(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "mplay");

		item->setHintIcon(m_vMovieInfo[i].tfile.c_str());
	
		item->setHint(m_vMovieInfo[i].epgInfo1.empty() ? m_vMovieInfo[i].epgInfo2.c_str() : m_vMovieInfo[i].epgInfo1.c_str());

		mlist->addItem(item);
	}
	
	mlist->setSelected(selected);

	//
	mlist->setMode(ClistBox::MODE_LISTBOX);
	mlist->setLayout(ClistBox::LAYOUT_FRAME);
	mlist->setItemsPerPage(6, 2);
	
	//
	mlist->enablePaintHead();
	mlist->setTitle(_("Movie Player"), NEUTRINO_ICON_MOVIE);
	mlist->enablePaintDate();
	mlist->setFormat("%A %d.%m.%Y %H:%M:%S");
	mlist->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	
	//
	mlist->enablePaintFoot();
	mlist->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	//
	mlist->enablePaintItemInfo();

	//
	widget->addKey(CRCInput::RC_info, this, CRCInput::getSpecialKeyName(CRCInput::RC_info));
	widget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	widget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	widget->addKey(CRCInput::RC_spkr, this, CRCInput::getSpecialKeyName(CRCInput::RC_spkr));
	widget->addKey(CRCInput::RC_yellow, this, CRCInput::getSpecialKeyName(CRCInput::RC_yellow));
	widget->addKey(CRCInput::RC_blue, this, CRCInput::getSpecialKeyName(CRCInput::RC_blue));
	widget->addKey(CRCInput::RC_help, this, CRCInput::getSpecialKeyName(CRCInput::RC_help));
	
	//
	widget->addCCItem(mlist);
	widget->setTimeOut(g_settings.timing_filebrowser);

	widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CMoviePlayer::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "\nMoviePlayer::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();
		
	selected = mlist? mlist->getSelected() : 0;
	
	if (selected > m_vMovieInfo.size())
		selected = 0;
	
	if(actionKey == "mplay")
	{
		if(m_vMovieInfo.size() > 0)
		{
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[selected]);
		
			tmpMoviePlayerGui.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_info")
	{
		if(m_vMovieInfo.size() > 0)
		{
			m_movieInfo.showMovieInfo(m_vMovieInfo[mlist->getSelected()]);
		}

		return RETURN_REPAINT;
	}
	else if (actionKey == "RC_help")
	{
		tmpMoviePlayerGui.showHelp();
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_red")
	{
		if(m_vMovieInfo.size() > 0)
		{
			hide();
			doTMDB(m_vMovieInfo[mlist->getSelected()]);
			showMenu();

			return RETURN_EXIT_ALL;
		}
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_green")
	{
		openFileBrowser();
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if (actionKey == "RC_spkr") 
	{
		if(m_vMovieInfo.size() > 0)
		{	
			if (&m_vMovieInfo[selected].file != NULL) 
			{
			 	onDeleteFile(m_vMovieInfo[selected]);
			}
		}
		
		if (selected > m_vMovieInfo.size())
			selected = 0;

		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_yellow")
	{
		m_vMovieInfo.clear();
		selected = 0;
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_blue")
	{
		loadPlaylist();
		selected = 0;
		showMenu();

		return RETURN_EXIT_ALL;
	}

	loadPlaylist();
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

