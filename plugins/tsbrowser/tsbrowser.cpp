/*
  $Id: tsbrowser.cpp 24.12.2018 mohousch Exp $

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

class CTSBrowser : public CMenuTarget
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
		CMovieInfo m_movieInfo;
		std::vector<MI_MOVIE_INFO> m_vMovieInfo;
		CFileFilter fileFilter;
		CFileList filelist;
		std::string Path;
		CMoviePlayerGui tmpMoviePlayerGui;

		//
		void loadPlaylist();
		void doTMDB(MI_MOVIE_INFO& movieFile);
		bool delFile(CFile& file);
		void onDeleteFile(MI_MOVIE_INFO& movieFile);
		void openFileBrowser();
		void showMenu();

	public:
		CTSBrowser();
		~CTSBrowser();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CTSBrowser::CTSBrowser()
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
}

CTSBrowser::~CTSBrowser()
{
	m_vMovieInfo.clear();
}

void CTSBrowser::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CTSBrowser::loadPlaylist()
{
	m_vMovieInfo.clear();

	// recordingdir
	Path = g_settings.network_nfs_recordingdir;

	CHintBox loadBox(_("TS Browser"), _("Scan for Movies ..."));
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

void CTSBrowser::openFileBrowser()
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

		CHintBox loadBox(_("TS Browser"), _("Scan for Movies ..."));
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

void CTSBrowser::doTMDB(MI_MOVIE_INFO& movieFile)
{
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
		buffer += "Vote: " + to_string(minfo_list[0].vote_average) + "/10 Votecount: " + to_string(minfo_list[0].vote_count);
		buffer += "\n\n";
		buffer += minfo_list[0].overview;
		buffer += "\n";

		buffer += (std::string)_("Length (Min)") + ": " + to_string(minfo_list[0].runtime);
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

		infoBox->setText(buffer.c_str(), tname.c_str(), p_w, p_h);
		infoBox->exec();
		delete infoBox;

		if(MessageBox(_("Information"), _("Prefer TMDB infos ?"), mbrNo, mbYes | mbNo) == mbrYes) 
		{
			// tfile
			std::string tname = movieFile.file.Name;
			changeFileNameExt(tname, ".jpg");

			if(tmdb->getBigCover(minfo_list[0].poster_path, tname)) 
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
		MessageBox(_("Information"), _("Not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
	}

	delete tmdb;
	tmdb = NULL;
}

bool CTSBrowser::delFile(CFile& file)
{
	bool result = true;
	unlink(file.Name.c_str()); // fix: use full path
	dprintf(DEBUG_NORMAL, "CTSBrowser::delete file: %s\r\n", file.Name.c_str());
	return(result);
}

void CTSBrowser::onDeleteFile(MI_MOVIE_INFO& movieFile)
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

	if (MessageBox(_("Delete"), msg.c_str(), mbrNo, mbYes | mbNo) == mbrYes)
	{
		delFile(movieFile.file);
			
                int i = 1;
                char newpath[1024];
                do {
			sprintf(newpath, "%s.%03d", movieFile.file.Name.c_str(), i);
			if(access(newpath, R_OK)) 
			{
				break;
                        } 
                        else 
			{
				unlink(newpath);
				dprintf(DEBUG_NORMAL, "  delete file: %s\r\n", newpath);
                        }
                        i++;
                } while(1);
			
                std::string fname = movieFile.file.Name;
                       
		int ext_pos = 0;
		ext_pos = fname.rfind('.');
		if( ext_pos > 0)
		{
			std::string extension;
			extension = fname.substr(ext_pos + 1, fname.length() - ext_pos);
			extension = "." + extension;
			strReplace(fname, extension.c_str(), ".jpg");
		}
			
                unlink(fname.c_str());

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

void CTSBrowser::showMenu()
{
	widget = new CWidget();
	mlist = new ClistBox(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	mlist->clearAll();

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "mplay");

		item->setHintIcon(m_vMovieInfo[i].tfile.c_str());
	
		item->setHint(m_vMovieInfo[i].epgInfo1.empty() ? m_vMovieInfo[i].epgInfo2.c_str() : m_vMovieInfo[i].epgInfo1.c_str());

		mlist->addItem(item);
	}

	//
	mlist->setWidgetMode(MODE_LISTBOX);
	mlist->setWidgetType(WIDGET_TYPE_FRAME);
	mlist->setItemsPerPage(6, 2);
	
	//
	mlist->enablePaintHead();
	mlist->setTitle(_("Movieplayer"), NEUTRINO_ICON_MOVIE);
	//mlist->setTitleHAlign(CC_ALIGN_CENTER);
	mlist->enablePaintDate();
	mlist->setFormat("%A %d.%m.%Y %H:%M:%S");
	mlist->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	
	//
	mlist->enablePaintFoot();
	mlist->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	//
	mlist->enablePaintItemInfo();

	//
	widget->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));
	widget->addKey(RC_red, this, CRCInput::getSpecialKeyName(RC_red));
	widget->addKey(RC_green, this, CRCInput::getSpecialKeyName(RC_green));
	widget->addKey(RC_spkr, this, CRCInput::getSpecialKeyName(RC_spkr));
	widget->addKey(RC_yellow, this, CRCInput::getSpecialKeyName(RC_yellow));
	widget->addKey(RC_blue, this, CRCInput::getSpecialKeyName(RC_blue));
	
	//
	widget->addItem(mlist);

	widget->exec(NULL, "");
	
	delete mlist;
	mlist = NULL;
	
	delete widget;
	widget = NULL;
}

int CTSBrowser::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "\nCTSBrowser::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();
	
	if(actionKey == "mplay")
	{
		selected = mlist? mlist->getSelected() : 0;

		CMovieInfoWidget movieInfoWidget;
		movieInfoWidget.setMovie(m_vMovieInfo[selected]);
		
		movieInfoWidget.exec(NULL, "");

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_info")
	{
		selected = mlist? mlist->getSelected() : 0;
		m_movieInfo.showMovieInfo(m_vMovieInfo[mlist->getSelected()]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_red")
	{
		selected = mlist? mlist->getSelected() : 0;
		hide();
		doTMDB(m_vMovieInfo[mlist->getSelected()]);
		showMenu();

		return RETURN_EXIT_ALL;
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
			if (&m_vMovieInfo[mlist->getSelected()].file != NULL) 
			{
			 	onDeleteFile(m_vMovieInfo[mlist->getSelected()]);
			}
		}

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
	CTSBrowser* movieBrowserHandler = new CTSBrowser();
	
	movieBrowserHandler->exec(NULL, "");
	
	delete movieBrowserHandler;
	movieBrowserHandler = NULL;
}


