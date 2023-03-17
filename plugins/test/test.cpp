/*
  $Id: test.cpp 26.09.2021 mohousch Exp $

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

class CTestMenu : public CMenuTarget
{
	private:
		// variables
		CFrameBuffer* frameBuffer;

		//
		CAudioPlayerGui tmpAudioPlayerGui;
		CMoviePlayerGui tmpMoviePlayerGui;
		CPictureViewerGui tmpPictureViewerGui;

		//
		int selected;

		//
		CFileFilter fileFilter;
		CFileList filelist;

		// movie
		CMovieInfo m_movieInfo;
		CMoviePlayList m_vMovieInfo;

		// audio
		CAudioPlayList AudioPlaylist;

		// pictures
		CPicture pic;
		CPicturePlayList PicPlaylist;

		// tmdb
		CTmdb* tmdb;
		std::string thumbnail_dir;
		CFileHelpers fileHelper;
		cYTFeedParser ytparser;
		std::string plist;
		int page;
		std::string TVShows;
		tmdb_video_list_t mvlist;
		std::string tmdbsearch;

		// channellist
		CChannelList* webTVchannelList;
		CBouquetList* webTVBouquetList;
		
		// CHeaders
		CBox headBox;
		CHeaders * headers;
		
		// CFooters
		CBox footBox;
		CFooters *footers;

		// CWidget
		CWidget *testWidget;

		// CFrameBox
		CBox topBox;
		CFrameBox *frameBoxWidget;
		int top_selected;

		// ClistBox
		CBox leftBox;
		ClistBox *leftWidget;
		int left_selected;

		// ClistBox
		CBox rightBox;
		ClistBox *rightWidget;
		int right_selected;

		// CListFrame
		CListFrame *listFrame;

		// CWindow
		CWindow *windowWidget;
		
		// CTextBox
		CTextBox* textBoxWidget;
		
		// CMenuwidget
		CMenuWidget* menuWidget;
		CMenuItem* item;
		
		CComponent* cCItem;
		
		// pb window
		CProgressWindow * progressWindow;
		
		// pb
		CProgressBar* progressBar;
		CProgressBar* progressBar2;

		// helper functions
		void loadTMDBPlaylist(const char *txt = "movie", const char *list = "popular", const int seite = 1, bool search = false);
		void loadMoviePlaylist();
		void openMovieFileBrowser();
		void loadAudioPlaylist();
		void openAudioFileBrowser();
		void loadPicturePlaylist();
		void openPictureFileBrowser();	

		// CWidget
		void testCListFrameWidget();
		void testClistBoxWidget();
		void testCWindowWidget();
		void testCTextBoxWidget();
		void testCFrameBoxWidget();
		void testCWidget();
		void testMultiWidget();

		// CCompenents 
		void testCIcon();
		void testCImage();
		void testCProgressBar();
		void testCButtons();
		void testCHButtons();
		void testCSpinner();

		// CWidgetItem
		void testCHeaders();
		void testCFooters();
		void testCWindow();
		void testCWindowShadow();
		void testCWindowCustomColor();
		void testCWindowCComponent();
		void testCProgressWindow();
		void testCTextBox();
		void testCListFrame();
		void testClistBox();
		void testClistBox2();
		void testClistBox3();
		void testClistBox4();
		void testClistBox5();
		void testClistBox6();
		void testClistBox7();
		void testClistBox8();
		void testClistBox9();
		void testCFrameBox();
		void testCFrameBox1();
		
		// CMenuWidget
		void testCMenuWidget();
		void testCMenuWidget1();
		void testCMenuWidget2();

		// misc widgets
		void testCStringInput();
		void testCStringInputSMS();
		void testCPINInput();
		void testCPLPINInput();
		void testCPINChangeWidget();
		void testCIPInput();
		void testCDateInput();
		void testCMACInput();
		void testCTimeInput();
		void testCIntInput();
		void testCInfoBox();
		void testCMessageBox();
		void testCMessageBoxInfoMsg();
		void testCMessageBoxErrorMsg();
		void testCHintBox();
		void testCHintBoxInfo();
		void testCHelpBox();

		// misc gui widgets
		void testVFDController();
		void testColorChooser();
		void testKeyChooser();
		void testMountChooser();
		void testChannelSelectWidget();
		void testBEWidget();
		void testAVSelectWidget();
		void testAudioSelectWidget();
		void testDVBSubSelectWidget();
		void testAlphaSetupWidget();
		void testPSISetup();
		void testRCLock();
		void testSleepTimerWidget();
		void testMountGUI();
		void testUmountGUI();
		void testMountSmallMenu();
		void testPluginsList();

		//
		void testStartPlugin();

		//
		void testShowActuellEPG();

		//
		void testPlayMovieURL();
		void testPlayAudioURL();
		void testShowPictureURL();

		// players
		void testPlayMovieFolder();
		void testPlayAudioFolder();
		void testShowPictureFolder();

		//
		void testPlayMovieDir();
		void testPlayAudioDir();
		void testShowPictureDir();

		// channel/bouquet list
		void testCChannellist();
		void testCBouquetlist();
		
		// skin
		void testSkinWidget();
		void testSkinWidget2();
		void testSkinWidget3();
		
		// paint()
		void showMenu();
		

	public:
		CTestMenu();
		~CTestMenu();
		int exec(CMenuTarget* parent, const std::string& actionKey);

		// hide()
		void hide();
};

#define HEAD_BUTTONS_COUNT	3
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, _("Help"), COL_YELLOW_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_SETUP, _("Menu"), COL_GREEN_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_MUTE_SMALL, _("Mute"), COL_RED_PLUS_0 }
};

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Next Page"), COL_RED_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Prev Page"), COL_GREEN_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Focus"), COL_YELLOW_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("New Movies"), COL_BLUE_PLUS_0 },
	
};

#define WFOOT_BUTTONS_COUNT	2
const struct button_label WFootButtons[WFOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Play") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Info") }	
};

CTestMenu::CTestMenu()
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = 0;

	//
	webTVchannelList = NULL;
	webTVBouquetList = NULL;

	//
	menuWidget = NULL;
	
	//
	item = NULL;
	cCItem = NULL;

	//
	plist = "popular";
	page = 1;
	TVShows = "movies";

	//
	headers = NULL;
	footers = NULL;
	testWidget = NULL;
	frameBoxWidget = NULL;
	leftWidget = NULL;
	rightWidget = NULL;
	listFrame = NULL;
	windowWidget = NULL;
	textBoxWidget = NULL;
	progressWindow = NULL;
	progressBar = NULL;
	progressBar2 = NULL;
}

CTestMenu::~CTestMenu()
{
	filelist.clear();
	fileFilter.clear();
	m_vMovieInfo.clear();

	if(webTVchannelList)
	{
		delete webTVchannelList;
		webTVchannelList = NULL;
	}

	if(webTVBouquetList)
	{
		delete webTVBouquetList;
		webTVBouquetList = NULL;
	}
	
	if (headers)
	{
		delete headers;
		headers = NULL;
	}
	
	if (footers)
	{
		delete footers;
		footers = NULL;
	}
	
	if (testWidget)
	{
		delete testWidget;
		testWidget = NULL;
	}
	
	if (frameBoxWidget)
	{
		delete frameBoxWidget;
		frameBoxWidget = NULL;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
	
	if (leftWidget)
	{
		delete leftWidget;
		leftWidget = NULL;
	}
	
	if (listFrame)
	{
		delete listFrame;
		listFrame = NULL;
	}
	
	if (windowWidget)
	{
		delete windowWidget;
		windowWidget = NULL;
	}
	
	if (textBoxWidget)
	{
		delete textBoxWidget;
		textBoxWidget = NULL;
	}
	
	if (progressBar)
	{
		delete progressBar;
		progressBar = NULL;
	}
	
	if (progressWindow)
	{
		delete progressWindow;
		progressWindow = NULL;
	}
	
	if (progressBar2)
	{
		delete progressBar2;
		progressBar2 = NULL;
	}
}

void CTestMenu::hide()
{
	dprintf(DEBUG_NORMAL, "CTestMenu:hide:\n");
	
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

//// helpers functions
void CTestMenu::loadAudioPlaylist()
{
	dprintf(DEBUG_NORMAL, "CTestMenu:loadAudioPlaylist:\n");

	fileFilter.clear();
	filelist.clear();
	AudioPlaylist.clear();

	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("m2a");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
	fileFilter.addFilter("ogg");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");

	std::string Path = g_settings.network_nfs_audioplayerdir;

	//if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
	CFileHelpers::getInstance()->addRecursiveDir(&filelist, Path, &fileFilter);

	if(filelist.size() > 0)
	{		
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			if ( (files->getExtension() == CFile::EXTENSION_CDR)
					||  (files->getExtension() == CFile::EXTENSION_MP3)
					||  (files->getExtension() == CFile::EXTENSION_WAV)
					||  (files->getExtension() == CFile::EXTENSION_FLAC))
			{
				CAudiofile audiofile(files->Name, files->getExtension());

				// refill
				std::string title;
				std::string artist;
				std::string genre;
				std::string date;
				char duration[9] = "";

				CAudioPlayer::getInstance()->init();

				int ret = CAudioPlayer::getInstance()->readMetaData(&audiofile, true);

				if (!ret || (audiofile.MetaData.artist.empty() && audiofile.MetaData.title.empty() ))
				{
					//remove extension (.mp3)
					std::string tmp = files->getFileName().substr(files->getFileName().rfind('/') + 1);
					tmp = tmp.substr(0, tmp.length() - 4);	//remove extension (.mp3)

					std::string::size_type i = tmp.rfind(" - ");
		
					if(i != std::string::npos)
					{ 
						audiofile.MetaData.title = tmp.substr(0, i);
						audiofile.MetaData.artist = tmp.substr(i + 3);
					}
					else
					{
						i = tmp.rfind('-');
						if(i != std::string::npos)
						{
							audiofile.MetaData.title = tmp.substr(0, i);
							audiofile.MetaData.artist = tmp.substr(i + 1);
						}
						else
							audiofile.MetaData.title = tmp;
					}
				}
				
				AudioPlaylist.push_back(audiofile);
			}
		}
	}
}

void CTestMenu::openAudioFileBrowser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:loadAudioFileBrowser:\n");
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_audioplayerdir : "");

	fileFilter.clear();
	filelist.clear();
	AudioPlaylist.clear();

	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("m2a");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
	fileFilter.addFilter("ogg");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	std::string Path = g_settings.network_nfs_audioplayerdir;

	if (filebrowser.exec(Path.c_str()))
	{
		Path = filebrowser.getCurrentDir();
		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for(; files != filebrowser.getSelectedFiles().end(); files++)
		{
			if ( (files->getExtension() == CFile::EXTENSION_CDR)
					||  (files->getExtension() == CFile::EXTENSION_MP3)
					||  (files->getExtension() == CFile::EXTENSION_WAV)
					||  (files->getExtension() == CFile::EXTENSION_FLAC))
			{
				CAudiofile audiofile(files->Name, files->getExtension());

				// skip duplicate
				for (unsigned long i = 0; i < AudioPlaylist.size(); i++)
				{
					if(AudioPlaylist[i].Filename == audiofile.Filename)
						AudioPlaylist.erase(AudioPlaylist.begin() + i); 
				}

				// refill
				std::string title;
				std::string artist;
				std::string genre;
				std::string date;
				char duration[9] = "";

				CAudioPlayer::getInstance()->init();

				int ret = CAudioPlayer::getInstance()->readMetaData(&audiofile, true);

				if (!ret || (audiofile.MetaData.artist.empty() && audiofile.MetaData.title.empty() ))
				{
					//remove extension (.mp3)
					std::string tmp = files->getFileName().substr(files->getFileName().rfind('/') + 1);
					tmp = tmp.substr(0, tmp.length() - 4);	//remove extension (.mp3)

					std::string::size_type i = tmp.rfind(" - ");
		
					if(i != std::string::npos)
					{ 
						audiofile.MetaData.title = tmp.substr(0, i);
						audiofile.MetaData.artist = tmp.substr(i + 3);
					}
					else
					{
						i = tmp.rfind('-');
						if(i != std::string::npos)
						{
							audiofile.MetaData.title = tmp.substr(0, i);
							audiofile.MetaData.artist = tmp.substr(i + 1);
						}
						else
							audiofile.MetaData.title = tmp;
					}
				}
		
				AudioPlaylist.push_back(audiofile);
			}
		}
	}
}

void CTestMenu::loadMoviePlaylist()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:loadMoviePlaylist:\n");
	
	fileFilter.clear();
	filelist.clear();
	m_vMovieInfo.clear();

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
	
	// recordingdir
	std::string Path = g_settings.network_nfs_recordingdir;
	m_vMovieInfo.clear();
	
	//
	if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
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
					
			// 
			m_vMovieInfo.push_back(movieInfo);
		}
	}
}

void CTestMenu::openMovieFileBrowser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:openMovieFileBrowser:\n");
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_recordingdir : "");

	fileFilter.clear();
	filelist.clear();
	m_vMovieInfo.clear();

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

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	std::string Path = g_settings.network_nfs_recordingdir;

	if (filebrowser.exec(Path.c_str()))
	{
		Path = filebrowser.getCurrentDir();

		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure

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
	}
}

void CTestMenu::loadPicturePlaylist()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:loadPicturePlaylist:\n");
	
	fileFilter.clear();
	filelist.clear();
	PicPlaylist.clear();

	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");

	std::string Path = g_settings.network_nfs_picturedir;

	if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
	{
		struct stat statbuf;
				
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			if (files->getType() == CFile::FILE_PICTURE)
			{
				// fill 
				pic.Filename = files->Name;
				std::string tmp = files->Name.substr(files->Name.rfind('/') + 1);
				pic.Name = tmp.substr(0, tmp.rfind('.'));
				pic.Type = tmp.substr(tmp.rfind('.') + 1);
			
				if(stat(pic.Filename.c_str(), &statbuf) != 0)
					printf("stat error");
				pic.Date = statbuf.st_mtime;
				
				PicPlaylist.push_back(pic);
			}
		}
	}
}

void CTestMenu::openPictureFileBrowser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:loadPictureBrowser:\n");
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_picturedir : "");

	fileFilter.clear();
	filelist.clear();
	PicPlaylist.clear();

	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	std::string Path = g_settings.network_nfs_picturedir;

	if (filebrowser.exec(Path.c_str()))
	{
		Path = filebrowser.getCurrentDir();
		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for(; files != filebrowser.getSelectedFiles().end(); files++)
		{
			if(files->getType() == CFile::FILE_PICTURE)
			{
				CPicture pic;
				pic.Filename = files->Name;
				std::string tmp = files->Name.substr(files->Name.rfind('/') + 1);
				pic.Name = tmp.substr(0, tmp.rfind('.'));
				pic.Type = tmp.substr(tmp.rfind('.') + 1);
				struct stat statbuf;
				if(stat(pic.Filename.c_str(),&statbuf) != 0)
					printf("stat error");
				pic.Date = statbuf.st_mtime;

				// skip duplicate
				for (unsigned long i = 0; i < PicPlaylist.size(); i++)
				{
					if(PicPlaylist[i].Filename == pic.Filename)
						PicPlaylist.erase(PicPlaylist.begin() + i); 
				}
							
				PicPlaylist.push_back(pic);
			}
		}
	}
}

void CTestMenu::loadTMDBPlaylist(const char *txt, const char *list, const int seite, bool search)
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:loadTMDBPlaylist:\n");
	
	m_vMovieInfo.clear();
	
	thumbnail_dir = "/tmp/nfilm";
	page = seite;
	plist = list;
	TVShows = txt;

	//
	tmdb = new CTmdb();
	mvlist.clear();

	fileHelper.removeDir(thumbnail_dir.c_str());
	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

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

	if (mvlist.empty())
		return;
	
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
		movieInfo.productionDate = atoi(mvlist[i].release_date.substr(0,4));
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
		
		if (tmdb->getVideoInfo("movie", mvlist[i].id))
		{
			std::vector<tmdbinfo>& videoInfo_list = tmdb->getVideoInfos();

			movieInfo.vid = videoInfo_list[0].vid;
			movieInfo.vkey = videoInfo_list[0].vkey;
			movieInfo.vname = videoInfo_list[0].vname;
		}

		m_vMovieInfo.push_back(movieInfo);
	}
}

////
void CTestMenu::testCWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:testCWidget(CFrameBox|ClistBox|CHead|CFoot):\n");
	
	testWidget = new CWidget(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	testWidget->enableSaveScreen();
	testWidget->setSelected(selected);

	// head
	headBox.iWidth = frameBuffer->getScreenWidth();
	headBox.iHeight = 40;
	headBox.iX = frameBuffer->getScreenX();
	headBox.iY = frameBuffer->getScreenY();

	headers = new CHeaders(&headBox, "CWidget(CFrameBox/ClistBox)", NEUTRINO_ICON_MP3);

	headers->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	headers->enablePaintDate();

	// foot
	footBox.iWidth = frameBuffer->getScreenWidth();
	footBox.iHeight = 40;
	footBox.iX = frameBuffer->getScreenX();
	footBox.iY = frameBuffer->getScreenY() + frameBuffer->getScreenHeight() - footBox.iHeight;

	footers = new CFooters(&footBox);
	footers->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	// topwidget (frameBox)
	topBox.iWidth = testWidget->getWindowsPos().iWidth;
	topBox.iHeight = 50;
	topBox.iX = testWidget->getWindowsPos().iX;
	topBox.iY = testWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE;

	top_selected = 0;

	frameBoxWidget = new CFrameBox(&topBox);
	frameBoxWidget->setOutFocus();

	CFrame * frame = NULL;

	frame = new CFrame();
	frame->setPosition(topBox.iX, topBox.iY, topBox.iWidth/3, topBox.iHeight);
	frame->setTitle("Filme");
	frame->setActionKey(this, "movie");
	frame->setHAlign(CC_ALIGN_CENTER);
	frame->enableBorder();
	frameBoxWidget->addFrame(frame);
	
	frame = new CFrame();
	frame->setPosition(topBox.iX + topBox.iWidth/3, topBox.iY, topBox.iWidth/3, topBox.iHeight);
	frame->setTitle("Serien");
	frame->setHAlign(CC_ALIGN_CENTER);
	frame->setActionKey(this, "tv");
	frame->enableBorder();
	frameBoxWidget->addFrame(frame);

	frame = new CFrame();
	frame->setPosition(topBox.iX + 2*topBox.iWidth/3, topBox.iY, topBox.iWidth/3, topBox.iHeight);
	frame->setTitle("Suche");
	frame->setHAlign(CC_ALIGN_CENTER);
	frame->setOption(tmdbsearch.c_str());
	frame->setActionKey(this, "search");
	frame->enableBorder();
	frameBoxWidget->addFrame(frame);

	frameBoxWidget->setSelected(top_selected); 

	// leftWidget (listBox)
	leftBox.iWidth = 200;
	leftBox.iHeight = testWidget->getWindowsPos().iHeight - headBox.iHeight - INTER_FRAME_SPACE - topBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	leftBox.iX = testWidget->getWindowsPos().iX;
	leftBox.iY = testWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE + topBox.iHeight + INTER_FRAME_SPACE;

	left_selected = 0;

	leftWidget = new ClistBox(&leftBox);

	leftWidget->setSelected(left_selected);
	leftWidget->setOutFocus();

	ClistBoxItem *item1 = new ClistBoxItem("In den Kinos", true, NULL, this, "movie_in_cinema");
	ClistBoxItem *item2 = new ClistBoxItem("Am", true, NULL, this, "movie_popular");
	item2->setOption("populärsten");
	item2->set2lines();
	ClistBoxItem *item3 = new ClistBoxItem("Am besten", true, NULL, this, "movie_top_rated");
	item3->setOption("bewertet");
	item3->set2lines();
	ClistBoxItem *item4 = new ClistBoxItem("Neue Filme", true, NULL, this, "movie_new");
	CMenuSeparator *item5 = new CMenuSeparator();
	CMenuSeparator *item6 = new CMenuSeparator();
	CMenuSeparator *item7 = new CMenuSeparator();
	CMenuSeparator *item8 = new CMenuSeparator();
	ClistBoxItem *item9 = new ClistBoxItem("Beenden", true, NULL, this, "exit");

	leftWidget->addItem(item1);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item2);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item3);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item4);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item5);
	leftWidget->addItem(item6);
	leftWidget->addItem(item7);
	leftWidget->addItem(item8);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item9);
	leftWidget->addItem(new CMenuSeparator(LINE));

	// rightwidget (listBox)
	rightBox.iWidth = testWidget->getWindowsPos().iWidth - INTER_FRAME_SPACE - leftBox.iWidth;
	rightBox.iHeight = testWidget->getWindowsPos().iHeight - headBox.iHeight - INTER_FRAME_SPACE - topBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	rightBox.iX = testWidget->getWindowsPos().iX + leftBox.iWidth + INTER_FRAME_SPACE;
	rightBox.iY = testWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE + topBox.iHeight + INTER_FRAME_SPACE;

	right_selected = 0;

	//
	rightWidget = new ClistBox(&rightBox);
	rightWidget->setWidgetType(TYPE_FRAME);
	rightWidget->setItemsPerPage(6,2);
	rightWidget->setSelected(right_selected);
	rightWidget->enablePaintItemInfo(80);

	// loadPlaylist
	CHintBox loadBox("CWidget(CFrameBox/ClistBox)", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadTMDBPlaylist();
	
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());

		rightWidget->addItem(item);
	}

	//
	testWidget->addWidgetItem(headers);
	testWidget->addWidgetItem(frameBoxWidget);
	testWidget->addWidgetItem(leftWidget);
	testWidget->addWidgetItem(rightWidget);
	testWidget->addWidgetItem(footers);

	testWidget->addKey(RC_info, this, "winfo");
	testWidget->addKey(RC_red, this, "nextPage");
	testWidget->addKey(RC_green, this, "prevPage");

	testWidget->exec(NULL, "");

	delete testWidget;
	testWidget = NULL;

	delete frameBoxWidget;
	frameBoxWidget = NULL;

	delete leftWidget;
	leftWidget = NULL;

	delete rightWidget;
	rightWidget = NULL;

	delete headers;
	headers = NULL;

	delete footers;
	footers = NULL;
}

void CTestMenu::testCWindowWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCWidget(CCItems)\n");

	// CBox
	CBox Box;
	Box.iX = g_settings.screen_StartX + 50;
	Box.iY = g_settings.screen_StartY + 50;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 100);
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 100);
	
	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	//
	testWidget = new CWidget(&Box);
	
	testWidget->paintMainFrame(true);
	testWidget->setColor(COL_MENUCONTENT_PLUS_0);
	testWidget->setCorner(RADIUS_MID, CORNER_ALL);
	
	// heades
	CHeaders head(Box.iX, Box.iY, Box.iWidth, 40, "CWidget(CCItems)", NEUTRINO_ICON_COLORS);
	head.enablePaintDate();
	
	// footers
	CFooters foot(Box.iX, Box.iY + Box.iHeight - 40, Box.iWidth, 40);
	foot.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	// icon
	CCIcon testIcon;
	testIcon.setIcon(NEUTRINO_ICON_BUTTON_RED);
	testIcon.setPosition(Box.iX + 150, Box.iY + 150, testIcon.iWidth, testIcon.iHeight);
	
	testWidget->addCCItem(&testIcon);
	
	// image
	CCImage testImage;
	testImage.setPosition(Box.iX + Box.iWidth - 210, Box.iY + 50, 200, 350);
	testImage.setImage(m_vMovieInfo[0].tfile.c_str());
	
	testWidget->addCCItem(&testImage);
	
	// label
	CCLabel testLabel;
	testLabel.setFont(SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	testLabel.setColor(COL_ORANGE);
	testLabel.enablePaintBG();
	testLabel.setText("this is a CComponent label test :-)");
	testLabel.setPosition(Box.iX + 20, Box.iY + 50, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	
	testWidget->addCCItem(&testLabel);
	
	// CButtons
	CCButtons testButton;
	int icon_w, icon_h;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	testButton.setPosition(Box.iX + 10, Box.iY + Box.iHeight - 100, Box.iWidth, 40);
	testButton.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	testWidget->addCCItem(&testButton);
	
	// Hline
	CCHline testHline;
	//testHline.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, 10);
	testHline.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, 2);
	testHline.setGradient(4);
	
	testWidget->addCCItem(&testHline);
	
	// Vline
	CCVline testVline;
	//testVline.setPosition(Box.iX + Box.iWidth - 20 - 15, Box.iY + 10, 10, Box.iHeight - 20);
	testVline.setPosition(Box.iX + Box.iWidth - 20 - 15, Box.iY + 50, 2, Box.iHeight - 100);
	testVline.setGradient(4);
	
	testWidget->addCCItem(&testVline);
	
	// CCFrameLine
	CCFrameLine testFrameLine;
	testFrameLine.setPosition(Box.iX + 10, Box.iY + 140, testIcon.iWidth + 100, testIcon.iHeight + 20);
	
	testWidget->addCCItem(&testFrameLine);
	
	// text
	CCText testText;
	testText.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, Box.iHeight/4);
	testText.setMode(AUTO_WIDTH);
	testText.setText(buffer.c_str());
	
	testWidget->addCCItem(&testText);

	// grid
	CCGrid testGrid;
	testGrid.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20, Box.iY + 100, 200, 160);
	testGrid.setColor(COL_PURPLE_PLUS_0);
	
	testWidget->addCCItem(&testGrid);
	
	// pig
	CCPig testPig;
	testPig.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20 + 200 + 10, Box.iY + 100, 300, 160);
	
	testWidget->addCCItem(&testPig);
	
	testWidget->addWidgetItem(&head);
	testWidget->addWidgetItem(&foot);
	
	testWidget->addKey(RC_red, this, "mplay");
	testWidget->addKey(RC_green, this, "mplay");
	testWidget->addKey(RC_yellow, this, "mplay");
	testWidget->addKey(RC_blue, this, "mplay");
	
	testWidget->exec(NULL, "");
	
	delete testWidget;
	testWidget = NULL;
}

void CTestMenu::testCTextBoxWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCTextBoxWidget\n");
	
	// mainBox
	CBox box;
	box.iX = CFrameBuffer::getInstance()->getScreenX() + 40;
	box.iY = CFrameBuffer::getInstance()->getScreenY() + 40;
	box.iWidth = CFrameBuffer::getInstance()->getScreenWidth() - 80;
	box.iHeight = CFrameBuffer::getInstance()->getScreenHeight() - 80;
	
	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	// scale pic
	int p_w = 0;
	int p_h = 0;

	scaleImage(m_vMovieInfo[0].tfile, &p_w, &p_h);
	
	textBoxWidget = new CTextBox(&box);
	
	textBoxWidget->setText(buffer.c_str(), m_vMovieInfo[0].tfile.c_str(), p_w, p_h);
	
	testWidget = new CWidget();
	testWidget->addWidgetItem(textBoxWidget);
	testWidget->addKey(RC_ok, this, "mplay");
	testWidget->addKey(RC_info, this, "tinfo");
	
	testWidget->exec(NULL, "");
	
	delete testWidget;
	testWidget = NULL;
	
	delete textBoxWidget;
	textBoxWidget = NULL;
}

void CTestMenu::testCFrameBoxWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:testCFrameBoxWidget:\n");
	
	// mainBox
	CBox box;
	box.iX = CFrameBuffer::getInstance()->getScreenX();
	box.iY = CFrameBuffer::getInstance()->getScreenY();
	box.iWidth = CFrameBuffer::getInstance()->getScreenWidth();
	box.iHeight = CFrameBuffer::getInstance()->getScreenHeight();

	int pic_w = box.iWidth/6;

	testWidget = new CWidget(&box);
	
	headers = new CHeaders(box.iX, box.iY, box.iWidth, 40, "testCFrameBoxWidget", NEUTRINO_ICON_MP3);

	headers->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	headers->enablePaintDate();

	// frameBox
	frameBoxWidget = new CFrameBox(&box);
	//frameBoxWidget->setRadius(RADIUS_MID);
	frameBoxWidget->setCorner(RADIUS_MID, CORNER_ALL);

	CHintBox loadBox("testCFrameBoxWidget", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadMoviePlaylist();
	
	loadBox.hide();

	// home
	CFrame *homeFrame = new CFrame();
	homeFrame->setMode(FRAME_LABEL);
	homeFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	int h_w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth("Home");
	int h_h = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
	homeFrame->setPosition(box.iX + 10, box.iY + 40, h_w + 10, h_h);
	homeFrame->setTitle("Home");
	homeFrame->setHAlign(CC_ALIGN_CENTER);
	homeFrame->setActionKey(this, "home");

	frameBoxWidget->addFrame(homeFrame);

	// setup
	CFrame *setupFrame = new CFrame();
	setupFrame->setMode(FRAME_LABEL);
	setupFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	int s_w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth("Setup");
	int s_h = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
	setupFrame->setPosition(box.iX + 10 + 5 + h_w + 20, box.iY + 40, s_w + 10, h_h);
	setupFrame->setTitle("Setup");
	setupFrame->setHAlign(CC_ALIGN_CENTER);
	setupFrame->setActionKey(this, "setup");

	frameBoxWidget->addFrame(setupFrame);

	// help
	CFrame *helpFrame = new CFrame();
	helpFrame->setMode(FRAME_LABEL);
	int i_w = 0;
	int i_h = 0;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_INFO, &i_w, &i_h);
	helpFrame->setPosition(box.iX + 10 + 5 + h_w + 10 + s_w + 40, box.iY + 40, i_w + 4, h_h);
	helpFrame->setTitle("?");
	helpFrame->setHAlign(CC_ALIGN_CENTER);
	helpFrame->setActionKey(this, "help");

	frameBoxWidget->addFrame(helpFrame);

	// frameBox1
	if (!m_vMovieInfo.empty())
	{
		// title
		CFrame * titleFrame = new CFrame();
		titleFrame->setMode(FRAME_LABEL);
		titleFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
		int t_w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(m_vMovieInfo[0].epgTitle);
		int t_h = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
		titleFrame->setPosition(box.iX + 10, box.iY + 40 + h_h + 10, t_w, t_h);
		titleFrame->setTitle((m_vMovieInfo[0].epgTitle.empty())? "" : m_vMovieInfo[0].epgTitle.c_str());
		//titleFrame->paintMainFrame(false);
		titleFrame->setActive(false);

		frameBoxWidget->addFrame(titleFrame);

		// text
		CFrame *textFrame = new CFrame();
		textFrame->setMode(FRAME_TEXT);
		textFrame->setPosition(box.iX + 10, box.iY + 40 + h_h + 10 + t_h + 10, box.iWidth - 20 - pic_w - 20, 250 - t_h - 10 - 10);
		std::string buffer;
		buffer = m_vMovieInfo[0].epgInfo1;
		buffer += "\n";
		buffer += m_vMovieInfo[0].epgInfo2;
		textFrame->setTitle(buffer.c_str());
		//textFrame->paintMainFrame(false);
		textFrame->setActive(false);

		frameBoxWidget->addFrame(textFrame);

		// pic
		CFrame * artFrame = new CFrame();
		artFrame->setMode(FRAME_PICTURE);
		artFrame->setPosition(box.iX + 10 + box.iWidth - 10 - pic_w - 20, box.iY + 40 + h_h + 10, pic_w - 20, 250);
		artFrame->setIconName(m_vMovieInfo[0].tfile.c_str());
		//artFrame->paintMainFrame(false);
		artFrame->setActionKey(this, "fire1play");

		frameBoxWidget->addFrame(artFrame);
		

		// other
		CFrame *otherFrame = new CFrame();
		otherFrame->setMode(FRAME_LABEL);
		otherFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
		int o_w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth("andere Filme:");
		int o_h = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
		otherFrame->setPosition(box.iX + 10, box.iY + 40 + h_h + 10 + 250 + 10, o_w + 10, o_h);
		otherFrame->setTitle("andere Filme:");
		//otherFrame->paintMainFrame(false);
		otherFrame->setActive(false);

		frameBoxWidget->addFrame(otherFrame);

		//
		CFrame * art1Frame = NULL;
		for (int i = 1; i < 7; i++)
		{
			art1Frame = new CFrame();
			art1Frame->setMode(FRAME_PICTURE);
			art1Frame->setPosition(box.iX + 10 + (i - 1)*((box.iWidth - 20)/6) + 5, box.iY + 40 + h_h + 10 + 250 + 10 + o_h + 10, (box.iWidth - 20)/6 - 10,box.iHeight - 40 - h_h - 10 - 250 - 10 - 40);
			art1Frame->setIconName(m_vMovieInfo[i].tfile.c_str());
			//art1Frame->paintMainFrame(false);
			art1Frame->setActionKey(this, "fireplay");
			art1Frame->setTitle(m_vMovieInfo[i].epgTitle.c_str());

			frameBoxWidget->addFrame(art1Frame);
		}
	}

	testWidget->addWidgetItem(frameBoxWidget);
	testWidget->addWidgetItem(headers);
	
	testWidget->exec(NULL, "");

	delete frameBoxWidget;
	frameBoxWidget = NULL;

	delete testWidget;
	testWidget = NULL;
}

void CTestMenu::testCListFrameWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestTMenu:testCListFrameWidget:\n");
	
	testWidget = new CWidget(frameBuffer->getScreenX() + 50, frameBuffer->getScreenY() + 50, frameBuffer->getScreenWidth() - 100, frameBuffer->getScreenHeight() - 100);

	// head
	headBox.iWidth = testWidget->getWindowsPos().iWidth;
	headBox.iHeight = 40;
	headBox.iX = testWidget->getWindowsPos().iX;
	headBox.iY = testWidget->getWindowsPos().iY;

	headers = new CHeaders(&headBox, "CWidget(ClistFrame)", NEUTRINO_ICON_MP3);

	headers->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	headers->enablePaintDate();

	// foot
	footBox.iWidth = testWidget->getWindowsPos().iWidth;
	footBox.iHeight = 40;
	footBox.iX = testWidget->getWindowsPos().iX;
	footBox.iY = testWidget->getWindowsPos().iY + testWidget->getWindowsPos().iHeight - footBox.iHeight;

	footers = new CFooters(&footBox);

	footers->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	//footers->setCorner(RADIUS_MID, CORNER_BOTTOM);

	//
	CBox listFrameBox;
	LF_LINES listFrameLines;
	int selected = 0;
	
	listFrameBox.iX = testWidget->getWindowsPos().iX;
	listFrameBox.iY = testWidget->getWindowsPos().iY + headBox.iHeight;
	listFrameBox.iWidth = testWidget->getWindowsPos().iWidth;
	listFrameBox.iHeight = testWidget->getWindowsPos().iHeight - headBox.iHeight - footBox.iHeight;

	//
#define MAX_ROWS 		LF_MAX_ROWS //6

	// init
	listFrameLines.rows = MAX_ROWS;

	for(int row = 0; row < MAX_ROWS; row++)
	{
		listFrameLines.lineArray[row].clear();
	}

	// rowwidth
	listFrameLines.rowWidth[0] = MAX_WINDOW_WIDTH / 20;
	listFrameLines.rowWidth[1] = MAX_WINDOW_WIDTH / 3;
	listFrameLines.rowWidth[2] = MAX_WINDOW_WIDTH / 10;
	listFrameLines.rowWidth[3] = MAX_WINDOW_WIDTH / 8;
	listFrameLines.rowWidth[4] = MAX_WINDOW_WIDTH / 5;
	listFrameLines.rowWidth[5] = MAX_WINDOW_WIDTH / 10;

	// headertitle
	listFrameLines.lineHeader[0] = "Nr";
	listFrameLines.lineHeader[1] = "title";
	listFrameLines.lineHeader[2] = "duration";
	listFrameLines.lineHeader[3] = "genre";
	listFrameLines.lineHeader[4] = "artist";
	listFrameLines.lineHeader[5] = "date";
	

	listFrame = new CListFrame(&listFrameLines, NULL, CListFrame::CListFrame::HEADER_LINE | CListFrame::SCROLL, &listFrameBox);

	CHintBox loadBox("ListFrame Widget", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadAudioPlaylist();
	
	loadBox.hide();

	// fill lineArrays list
	int count = 0;
	for (unsigned int i = 0; i < AudioPlaylist.size(); i++)
	{
		std::string title;
		std::string artist;
		std::string genre;
		std::string date;
		char duration[9] = "";

		title = AudioPlaylist[i].MetaData.title;
		artist = AudioPlaylist[i].MetaData.artist;
		genre = AudioPlaylist[i].MetaData.genre;	
		date = AudioPlaylist[i].MetaData.date;

		snprintf(duration, 8, "(%ld:%02ld)", AudioPlaylist[i].MetaData.total_time / 60, AudioPlaylist[i].MetaData.total_time % 60);

		listFrameLines.lineArray[0].push_back(to_string(i + 1));
		listFrameLines.lineArray[1].push_back(title);
		listFrameLines.lineArray[2].push_back(duration);
		listFrameLines.lineArray[3].push_back(genre);
		listFrameLines.lineArray[4].push_back(artist);
		listFrameLines.lineArray[5].push_back(date);
	}
	
	listFrame->setLines(&listFrameLines);

	// set selected line
	listFrame->setSelectedLine(selected);
	
	// paint
	listFrame->showSelection(true);

	//
	testWidget->addWidgetItem(headers);
	testWidget->addWidgetItem(listFrame);
	testWidget->addWidgetItem(footers);

	testWidget->addKey(RC_ok, this, "aok");
	testWidget->addKey(RC_info, this, "ainfo");

	testWidget->exec(NULL, "");

	delete testWidget;
	testWidget = NULL;

	delete listFrame;
	listFrame = NULL;

	delete headers;
	headers = NULL;

	delete footers;
	footers = NULL;
}

void CTestMenu::testClistBoxWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:testClistBoxWidget:\n");

	//
	rightBox.iWidth = frameBuffer->getScreenWidth();
	rightBox.iHeight = frameBuffer->getScreenHeight();
	rightBox.iX = frameBuffer->getScreenX();
	rightBox.iY = frameBuffer->getScreenY();
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&rightBox);

	rightWidget->setWidgetType(TYPE_FRAME);
	rightWidget->addWidgetType(TYPE_STANDARD);
	rightWidget->addWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	rightWidget->setItemsPerPage(6,2);
	rightWidget->setSelected(selected);
	rightWidget->enablePaintHead();
	rightWidget->setTitle("CWidget(ClistBox)", NEUTRINO_ICON_MP3);
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->setHeadLine(true, true);
	rightWidget->enablePaintDate();
	rightWidget->enablePaintFoot();
	rightWidget->setFootLine(true, true);
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->enablePaintItemInfo(80);
	rightWidget->setItemInfoMode(ITEMINFO_HINT_MODE);
	rightWidget->enableShrinkMenu();

	// loadPlaylist
	CHintBox loadBox("CWidget", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadMoviePlaylist();
	
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		item->setBorderMode();
		//item->setWidgetMode(MODE_MENU);

		rightWidget->addItem(item);
	}
	
	testWidget = new CWidget(&rightBox);
	testWidget->addWidgetItem(rightWidget);

	testWidget->addKey(RC_info, this, "linfo");
	testWidget->addKey(RC_setup, this, "lsetup");

	testWidget->exec(NULL, "");
	
	delete testWidget;
	testWidget = NULL;

	delete rightWidget;
	rightWidget = NULL;
}

void CTestMenu::testMultiWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testMultiWidget\n");

	CBox mainBox(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	CBox headBox;
	headBox.iX = mainBox.iX;
	headBox.iY = mainBox.iY;
	headBox.iWidth = mainBox.iWidth;
	headBox.iHeight = 40;

	CBox footBox;
	footBox.iHeight = 40;
	footBox.iX = mainBox.iX;
	footBox.iY = mainBox.iY + mainBox.iHeight - footBox.iHeight;
	footBox.iWidth = mainBox.iWidth;

	headers = new CHeaders(&headBox, "CWidget(ClistBox/CWindow)", NEUTRINO_ICON_MP3);

	headers->enablePaintDate();
	headers->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);

	footers = new CFooters(&footBox);

	footers->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	// leftWidget
	leftBox.iWidth = 200;
	leftBox.iHeight = mainBox.iHeight - headBox.iHeight - footBox.iHeight;
	leftBox.iX = mainBox.iX;
	leftBox.iY = mainBox.iY + headBox.iHeight;

	left_selected = 0;

	leftWidget = new ClistBox(&leftBox);
	leftWidget->setSelected(left_selected);
	leftWidget->setItemBorderMode();

	ClistBoxItem *item1 = new ClistBoxItem("Item 1");
	ClistBoxItem *item2 = new ClistBoxItem("Item 2");
	item2->setOption("Item 2- Option");
	item2->set2lines();
	ClistBoxItem *item3 = new ClistBoxItem("Item 3");
	item3->setOption("Item 3 Option");
	item3->set2lines();
	ClistBoxItem *item4 = new ClistBoxItem("Item4");
	CMenuSeparator *item5 = new CMenuSeparator();
	CMenuSeparator *item6 = new CMenuSeparator();
	CMenuSeparator *item7 = new CMenuSeparator();
	CMenuSeparator *item8 = new CMenuSeparator();
	ClistBoxItem *item9 = new ClistBoxItem("Beenden", true, NULL, this, "exit");

	leftWidget->addItem(item1);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item2);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item3);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item4);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item5);
	leftWidget->addItem(item6);
	leftWidget->addItem(item7);
	leftWidget->addItem(item8);
	leftWidget->addItem(new CMenuSeparator(LINE));
	leftWidget->addItem(item9);
	leftWidget->addItem(new CMenuSeparator(LINE));
	
	// rightWidget
	CBox Box;
	Box.iWidth = mainBox.iWidth - leftBox.iWidth;
	Box.iHeight = mainBox.iHeight - headBox.iHeight - footBox.iHeight;
	Box.iX = mainBox.iX + leftBox.iWidth;
	Box.iY = mainBox.iY + headBox.iHeight;
	
	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	// CWindow
	windowWidget = new CWindow(&Box);
	windowWidget->setColor(COL_MENUCONTENT_PLUS_0);
	
	// icon
	CCIcon testIcon;
	testIcon.setIcon(NEUTRINO_ICON_BUTTON_RED);
	testIcon.setPosition(Box.iX + 150, Box.iY + 150, testIcon.iWidth, testIcon.iHeight);
	
	windowWidget->addCCItem(&testIcon);
	
	// image
	CCImage testImage;
	testImage.setImage(m_vMovieInfo[0].tfile.c_str());
	testImage.setPosition(Box.iX + Box.iWidth - 210, Box.iY + 50, 200, 350);
	
	windowWidget->addCCItem(&testImage);
	
	// label
	CCLabel testLabel;
	testLabel.setFont(SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	testLabel.setColor(COL_GREEN);
	testLabel.enablePaintBG();
	testLabel.setText("this is a CComponent label test :-)");
	testLabel.setPosition(Box.iX + 20, Box.iY + 50, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	
	windowWidget->addCCItem(&testLabel);
	
	// CButtons
	CCButtons testButton(Box.iX + 10, Box.iY + Box.iHeight - 100, Box.iWidth, 40);
	int icon_w, icon_h;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	//testButton.setPosition(Box.iX + 10, Box.iY + Box.iHeight - 100, Box.iWidth, 40);
	testButton.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	windowWidget->addCCItem(&testButton);
	
	// Hline
	CCHline testHline(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 10, 10);
	//testHline.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 10, 10);
	
	windowWidget->addCCItem(&testHline);
	
	// Vline
	CCVline testVline(Box.iX, Box.iY + 10, 10, Box.iHeight - 20);
	//testVline.setPosition(Box.iX, Box.iY + 10, 10, Box.iHeight - 20);
	
	windowWidget->addCCItem(&testVline);
	
	// CCFrameLine
	CCFrameLine testFrameLine(Box.iX + 10, Box.iY + 140, testIcon.iWidth + 100, testIcon.iHeight + 20);
	//testFrameLine.setPosition(Box.iX + 10, Box.iY + 140, testIcon.iWidth + 100, testIcon.iHeight + 20);
	
	windowWidget->addCCItem(&testFrameLine);
	
	// text
	CCText testText(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, Box.iHeight/4);
	//testText.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, Box.iHeight/4);
	testText.setMode(AUTO_WIDTH);
	testText.setText(buffer.c_str());
	
	windowWidget->addCCItem(&testText);

	// grid
	CCGrid testGrid(Box.iX + 180 + testIcon.iWidth + 100 + 20, Box.iY + 100, 200, 160);
	//testGrid.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20, Box.iY + 100, 200, 160);
	testGrid.setColor(COL_PURPLE_PLUS_0);
	
	windowWidget->addCCItem(&testGrid);
	
	// pig
	CCPig testPig(Box.iX + 180 + testIcon.iWidth + 100 + 20 + 200 + 10, Box.iY + 100, 300, 160);
	//testPig.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20 + 200 + 10, Box.iY + 100, 300, 160);
	
	windowWidget->addCCItem(&testPig);
	
	testWidget = new CWidget(&mainBox);
	
	testWidget->addWidgetItem(headers);
	testWidget->addWidgetItem(footers);
	testWidget->addWidgetItem(leftWidget);
	testWidget->addWidgetItem(windowWidget);
	
	testWidget->exec(NULL, "");
	
	delete testWidget;
	testWidget = NULL;
	
	delete leftWidget;
	leftWidget = NULL;
	
	delete windowWidget;
	windowWidget = NULL;
}

// CIcon
void CTestMenu::testCIcon()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCIcon\n");

	//CIcon testIcon(NEUTRINO_ICON_BUTTON_RED);
	CCIcon testIcon;

	// paint testIcon
	testIcon.setIcon(NEUTRINO_ICON_BUTTON_RED);
	
	dprintf(DEBUG_NORMAL, "\ntestCIcon: icon:%s iw:%d ih:%d\n", testIcon.iconName.c_str(), testIcon.iWidth, testIcon.iHeight);
	
	testIcon.setPosition(150 + BORDER_LEFT, 150, testIcon.iWidth, testIcon.iHeight);

	testIcon.paint();

	CFrameBuffer::getInstance()->blit();

	// loop
	testWidget = new CWidget();
	testWidget->exec(NULL, "");
	delete testWidget;
	testWidget = NULL;

	hide();
}

// CImage
void CTestMenu::testCImage()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCImage\n");

	//
	CCImage testImage;

	// paint testImage
	testImage.setImage(DATADIR "/icons/nopreview.jpg");
	
	dprintf(DEBUG_NORMAL, "\ntestCImahe: image:%s iw:%d ih:%d nbp:%d\n", testImage.imageName.c_str(), testImage.iWidth, testImage.iHeight, testImage.iNbp);
	
	testImage.setPosition(150 + BORDER_LEFT, 150, testImage.iWidth, testImage.iHeight);
	testImage.paint();

	CFrameBuffer::getInstance()->blit();

	// loop
	testWidget = new CWidget();
	testWidget->exec(NULL, "");
	delete testWidget;
	testWidget = NULL;
	
	hide();
}

// CProgressBar
void CTestMenu::testCProgressBar()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCProgressBar\n");
	
	CBox Box;
	
	Box.iX = g_settings.screen_StartX + 10;
	Box.iY = g_settings.screen_StartY + 10 + (g_settings.screen_EndY - g_settings.screen_StartY - 20)/2;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20);
	Box.iHeight = 10;
	
	CBox Box2;
	
	Box2.iX = g_settings.screen_StartX + 10;
	Box2.iY = g_settings.screen_StartY + 50;
	Box2.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20);
	Box2.iHeight = 10;
	
	progressBar = new CProgressBar(&Box);
	progressBar2 = new CProgressBar(&Box2, 40, 100, 70, false);
	
	//
	progressBar->paint(10);
	progressBar2->paint(10);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(20);
	progressBar2->paint(20);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(30);
	progressBar2->paint(30);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(40);
	progressBar2->paint(40);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(50);
	progressBar2->paint(50);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(60);
	progressBar2->paint(60);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(70);
	progressBar2->paint(70);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(80);
	progressBar2->paint(80);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(90);
	progressBar2->paint(90);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressBar->paint(100);
	progressBar2->paint(100);
	CFrameBuffer::getInstance()->blit();
	
	delete progressBar;
	progressBar = NULL;
	
	delete progressBar2;
	progressBar2 = NULL;
	
	//
	hide();
}

// CButtons
void CTestMenu::testCButtons()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCButtons (foot)\n");
	
	CCButtons buttons;

	int icon_w, icon_h;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	
	//
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 250, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	//buttons.setMode(BUTTON_FRAME_COLORED);
	buttons.paint();
	
	//
	buttons.clear();
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 250 + 50, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	buttons.setMode(BUTTON_FRAME_COLORED);
	buttons.paint();
	
	//
	buttons.clear();
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 250 + 100, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	buttons.setMode(BUTTON_FRAME_BORDER);
	buttons.paint();

	CFrameBuffer::getInstance()->blit();

	// loop
	testWidget = new CWidget();
	testWidget->exec(NULL, "");
	delete testWidget;
	testWidget = NULL;
	
	hide();
}

// CButtons
void CTestMenu::testCHButtons()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCButtons (head)\n");
	
	CCButtons buttons;

	int icon_w, icon_h;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	
	//
	buttons.clear();
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 50, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(HeadButtons, HEAD_BUTTONS_COUNT, true);
	//buttons.setMode(BUTTON_FRAME_COLORED);
	buttons.paint();
	
	//
	buttons.clear();
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 50 + 50, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(HeadButtons, HEAD_BUTTONS_COUNT, true);
	buttons.setMode(BUTTON_FRAME_COLORED);
	buttons.paint();
	
	//
	buttons.clear();
	buttons.setPosition(g_settings.screen_StartX + 50 + BORDER_LEFT, g_settings.screen_StartY + 50 + 100, (g_settings.screen_EndX - g_settings.screen_StartX - 100), icon_h);
	buttons.setButtons(HeadButtons, HEAD_BUTTONS_COUNT, true);
	buttons.setMode(BUTTON_FRAME_BORDER);
	buttons.paint();

	CFrameBuffer::getInstance()->blit();

	// loop
	testWidget = new CWidget();
	testWidget->exec(NULL, "");
	delete testWidget;
	testWidget = NULL;
	
	hide();
}

void CTestMenu::testCSpinner()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCCSpinner:");
	
	testWidget = new CWidget(10, 10, 20, 20);
	
	CCSpinner testSpinner(10, 10, 20, 20);
	
	testWidget->addCCItem(&testSpinner);
	
	testWidget->exec(NULL, "");
	
	delete testWidget;
	testWidget = NULL;
}

//
void CTestMenu::testCHeaders()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCHeaders\n");
	
	CBox headBox;
	headBox.iX = g_settings.screen_StartX + 10;
	headBox.iY = g_settings.screen_StartY + 10;
	headBox.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20);
	headBox.iHeight = 40;

	//
	headers = new CHeaders(&headBox);

	headers->setTitle("test CHeaders");
	headers->setIcon(NEUTRINO_ICON_MP3);
	headers->enablePaintDate();
	headers->setFormat("%d.%m.%Y %H:%M:%S");
	headers->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	headers->setHAlign(CC_ALIGN_CENTER);
	//headers->setRadius(4);
	headers->setCorner(RADIUS_VERYLARGE, CORNER_TOP_LEFT|CORNER_BOTTOM_RIGHT);
		
	headers->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = 0;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	headers->setSecTimer(sec_timer_id);
	headers->exec(10);
	headers->hide();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	if(headers)
	{
		delete headers;
		headers = NULL;
	}
}

void CTestMenu::testCFooters()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCFooters\n");

	CBox footBox;
	footBox.iHeight = 40;
	footBox.iX = g_settings.screen_StartX + 10;
	footBox.iY = g_settings.screen_EndY - 10 - footBox.iHeight;
	footBox.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20);

	//
	footers = new CFooters(&footBox);
	
	footers->setCorner(RADIUS_VERYLARGE, CORNER_TOP_RIGHT|CORNER_BOTTOM_LEFT);
	footers->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
		
	footers->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	footers->exec(10);
	footers->hide();

	if(footers)
	{
		delete footers;
		footers = NULL;
	}
}

// Cwindow
void CTestMenu::testCWindow()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCWindow\n");

	CBox Box;
	
	Box.iX = g_settings.screen_StartX + 200;
	Box.iY = g_settings.screen_StartY + 200;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 400);
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 400);

	//
	CWindow* window = new CWindow(&Box);

	window->setColor(COL_MENUCONTENT_PLUS_0);
	window->setCorner(RADIUS_MID, CORNER_ALL);
	window->setGradient(LIGHT2DARK2LIGHT, GRADIENT_HORIZONTAL);

	window->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	window->exec();
	window->hide();
	
	if (window)
	{
		delete window;
		window = NULL;
	}
}

// CWindow
void CTestMenu::testCWindowShadow()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCWindowShadow\n");

	CBox Box;
	
	Box.iX = g_settings.screen_StartX + 200;
	Box.iY = g_settings.screen_StartY + 200;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 400);
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 400);

	//
	CWindow* window = new CWindow(&Box);

	window->setColor(COL_MENUCONTENT_PLUS_0);
	window->setCorner(RADIUS_MID, CORNER_ALL);
	window->setBorderMode(BORDER_ALL);
	window->enableSaveScreen();

	window->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	window->exec();
	window->hide();
	
	if (window)
	{
		delete window;
		window = NULL;
	}
}

// custom Color
void CTestMenu::testCWindowCustomColor()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCWindowCustomColor\n");

	CBox Box;
	
	Box.iX = g_settings.screen_StartX + 200;
	Box.iY = g_settings.screen_StartY + 200;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 400);
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 400);

	//
	CWindow* window = new CWindow(&Box);

	window->setColor(COL_ORANGE_PLUS_0); // or use like make16color(0x76A5AF)
	window->setCorner(RADIUS_MID, CORNER_ALL);
	window->setBorderMode(BORDER_ALL);
	window->enableSaveScreen();

	window->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	window->exec();
	window->hide();
	
	if (window)
	{
		delete window;
		window = NULL;
	}
}

// CWindow (CComponent)
void CTestMenu::testCWindowCComponent()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCWindowCComponent\n");

	// CBox
	CBox Box;
	Box.iX = g_settings.screen_StartX + 50;
	Box.iY = g_settings.screen_StartY + 50;
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 100);
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 100);
	
	int currentPage = 0;
	int NrOfPages = 4;
	int pcr = 25;
	
	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	// CWindow
	windowWidget = new CWindow(&Box);
	windowWidget->setColor(COL_MENUCONTENT_PLUS_0);
	windowWidget->setCorner(RADIUS_MID, CORNER_ALL);
	
	// headleft icon
	CCIcon headLeftIcon;
	headLeftIcon.setIcon(NEUTRINO_ICON_COLORS);
	headLeftIcon.setPosition(Box.iX, Box.iY + (40 - headLeftIcon.iHeight)/2, headLeftIcon.iWidth, headLeftIcon.iHeight);
	windowWidget->addCCItem(&headLeftIcon);
	
	// head label
	CCLabel headLabel;
	headLabel.setFont(SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	headLabel.setColor(COL_ORANGE);
	headLabel.enablePaintBG();
	headLabel.setText("CComponets");
	headLabel.setPosition(Box.iX + headLeftIcon.iWidth + BORDER_LEFT, Box.iY, Box.iWidth - headLeftIcon.iWidth - BORDER_LEFT, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	
	windowWidget->addCCItem(&headLabel);
	
	// head time
	CCTime headTimer;
	headTimer.setFormat("%A %d.%m.%Y %H:%M:%S");
	headTimer.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	std::string timestr = getNowTimeStr("%A %d.%m.%Y %H:%M:%S");
	int timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
	headTimer.setPosition(Box.iX + Box.iWidth - BORDER_RIGHT - timestr_len, Box.iY, timestr_len + 1, 40);
	
	windowWidget->addCCItem(&headTimer);
	
	// image
	CCImage testImage;
	testImage.setImage(m_vMovieInfo[0].tfile.c_str());
	testImage.setPosition(Box.iX, Box.iY + 40, Box.iWidth, Box.iHeight - 80);
	testImage.setScaling(true);
	
	windowWidget->addCCItem(&testImage);
	
	// icon
	CCIcon testIcon;
	testIcon.setIcon(NEUTRINO_ICON_BUTTON_RED);
	testIcon.setPosition(Box.iX + 150, Box.iY + 150, testIcon.iWidth, testIcon.iHeight);
	
	windowWidget->addCCItem(&testIcon);
	
	// label
	CCLabel testLabel;
	testLabel.setFont(SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	testLabel.setColor(COL_GREEN);
	testLabel.enablePaintBG();
	testLabel.setText("this is a CComponet label test :-)");
	testLabel.setPosition(Box.iX + 10, Box.iY + 50, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight());
	
	windowWidget->addCCItem(&testLabel);
	
	// CButtons
	CCButtons testButton;
	int icon_w, icon_h;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	testButton.setPosition(Box.iX + 10, Box.iY + Box.iHeight - 40, Box.iWidth, 40);
	testButton.setButtons(WFootButtons, WFOOT_BUTTONS_COUNT);
	
	windowWidget->addCCItem(&testButton);
	
	// Hline
	CCHline testHline;
	testHline.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 10, 10);
	
	windowWidget->addCCItem(&testHline);
	
	// Vline
	CCVline testVline;
	testVline.setPosition(Box.iX + Box.iWidth - 20 - 15, Box.iY + 10, 10, Box.iHeight - 20);
	
	windowWidget->addCCItem(&testVline);
	
	// CCFrameLine
	CCFrameLine testFrameLine;
	testFrameLine.setPosition(Box.iX + 10, Box.iY + 140, testIcon.iWidth + 100, testIcon.iHeight + 20);
	
	windowWidget->addCCItem(&testFrameLine);
	
	// DL
	//CItems2DetailsLine testDline;
	//testDline.setMode(DL_HINT);
	//testDline.setHint(buffer.c_str());
	//testDline.setIcon(m_vMovieInfo[0].tfile.c_str());
	
	// pb
	//CProgressBar testPB(Box.iX + Box.iWidth/2 - Box.iWidth/4, Box.iY + Box.iHeight - 150, Box.iWidth /3, 10, 40, 100, 70, true);
	//testPB.setPosition(Box.iX + Box.iWidth/2 - Box.iWidth/4, Box.iY + Box.iHeight - 150, Box.iWidth /3, 10);
	
	// sb
	//CScrollBar testSB;
	
	// text
	CCText testText;
	testText.setPosition(Box.iX + 10, Box.iY + Box.iHeight/2, Box.iWidth - 20, Box.iHeight/4);
	testText.setMode(AUTO_WIDTH);
	testText.setText(buffer.c_str());
	
	windowWidget->addCCItem(&testText);

	// grid
	CCGrid testGrid;
	testGrid.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20, Box.iY + 100, 200, 160);
	testGrid.setColor(COL_PURPLE_PLUS_0);
	
	windowWidget->addCCItem(&testGrid);
	
	// pig
	CCPig testPig;
	testPig.setPosition(Box.iX + 180 + testIcon.iWidth + 100 + 20 + 200 + 10, Box.iY + 100, 300, 160);
	
	windowWidget->addCCItem(&testPig);
	
	//
	windowWidget->addKey(RC_red, this, "wplay");
	windowWidget->addKey(RC_yellow, this, "winfo");
	
	//
	windowWidget->paint();
	//testDline.paint(Box.iX, Box.iY, Box.iWidth, Box.iHeight, 70, 35, Box.iY + 2*35);
	//testPB.paint(/*Box.iX + Box.iWidth/2 - Box.iWidth/4, Box.iY + Box.iHeight - 150,*/pcr);
	//testSB.paint(Box.iX + Box.iWidth - 10, Box.iY + 40, Box.iHeight - 80, NrOfPages, currentPage);
	CFrameBuffer::getInstance()->blit();
	
	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	windowWidget->setSecTimer(sec_timer_id);
	windowWidget->exec(10);	
	windowWidget->hide();	
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (windowWidget)
	{
		delete windowWidget;
		windowWidget = NULL;
	}
}

// CTextBox
void CTestMenu::testCTextBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCTextBox\n");

	CBox Box;
	
	Box.iX = g_settings.screen_StartX + 10;
	Box.iY = g_settings.screen_StartY + 10;
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 20);
	
	textBoxWidget = new CTextBox();

	textBoxWidget->setPosition(&Box);
	
	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	// scale pic
	int p_w = 0;
	int p_h = 0;

	scaleImage(m_vMovieInfo[0].tfile, &p_w, &p_h);
	
	textBoxWidget->setText(buffer.c_str(), m_vMovieInfo[0].tfile.c_str(), p_w, p_h);
	
	textBoxWidget->addKey(RC_ok, this, "winfo");
	
	//	
	textBoxWidget->paint();
	CFrameBuffer::getInstance()->blit();
	
	// loop
	textBoxWidget->exec();
	textBoxWidget->hide();
	
	if (textBoxWidget)
	{
		delete textBoxWidget;
		textBoxWidget = NULL;
	}
}

// ClistFrame
void CTestMenu::testCListFrame()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistFrame\n");

	CBox listFrameBox;
	LF_LINES listFrameLines;
	int selected = 0;
	
	//
	listFrameBox.iWidth = CFrameBuffer::getInstance()->getScreenWidth() / 20 * 17;
	listFrameBox.iHeight = CFrameBuffer::getInstance()->getScreenHeight() / 20 * 18;

	// recalculate x and y
	listFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - (listFrameBox.iWidth)) / 2);
	listFrameBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - listFrameBox.iHeight) / 2);

	//
#define MAX_ROWS 		LF_MAX_ROWS //6

	// init
	listFrameLines.rows = 6;

	for(int row = 0; row < 6; row++)
	{
		listFrameLines.lineArray[row].clear();
	}

	// rowwidth
	listFrameLines.rowWidth[0] = MAX_WINDOW_WIDTH / 20;
	listFrameLines.rowWidth[1] = MAX_WINDOW_WIDTH / 4;
	listFrameLines.rowWidth[2] = MAX_WINDOW_WIDTH / 12;
	listFrameLines.rowWidth[3] = MAX_WINDOW_WIDTH / 8;
	listFrameLines.rowWidth[4] = MAX_WINDOW_WIDTH / 5;
	listFrameLines.rowWidth[5] = MAX_WINDOW_WIDTH / 10;

	// headertitle
	listFrameLines.lineHeader[0] = "Nr";
	listFrameLines.lineHeader[1] = "title";
	listFrameLines.lineHeader[2] = "duration";
	listFrameLines.lineHeader[3] = "genre";
	listFrameLines.lineHeader[4] = "artist";
	listFrameLines.lineHeader[5] = "date";
	

	listFrame = new CListFrame(&listFrameLines, NULL, CListFrame::TITLE | CListFrame::HEADER_LINE | CListFrame::SCROLL, &listFrameBox);

	// title
	listFrame->setTitle("listFrame (AudioPlayer)", NEUTRINO_ICON_MP3);
	
	// fill lineArrays list
	CHintBox loadBox("listFrame", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadAudioPlaylist();
	
	loadBox.hide();

	//
	int count = 0;
	for (unsigned int i = 0; i < AudioPlaylist.size(); i++)
	{
		std::string title;
		std::string artist;
		std::string genre;
		std::string date;
		char duration[9] = "";

		title = AudioPlaylist[i].MetaData.title;
		artist = AudioPlaylist[i].MetaData.artist;
		genre = AudioPlaylist[i].MetaData.genre;	
		date = AudioPlaylist[i].MetaData.date;

		snprintf(duration, 8, "(%ld:%02ld)", AudioPlaylist[i].MetaData.total_time / 60, AudioPlaylist[i].MetaData.total_time % 60);

		listFrameLines.lineArray[0].push_back(to_string(i + 1));
		listFrameLines.lineArray[1].push_back(title);
		listFrameLines.lineArray[2].push_back(duration);
		listFrameLines.lineArray[3].push_back(genre);
		listFrameLines.lineArray[4].push_back(artist);
		listFrameLines.lineArray[5].push_back(date);
	}
	
	//
	listFrame->setLines(&listFrameLines);
	
	//
	listFrame->addKey(RC_ok, this, "aok");
	listFrame->addKey(RC_info, this, "ainfo");
	
	// paint
	listFrame->paint();
	CFrameBuffer::getInstance()->blit();

	CAudioPlayer::getInstance()->init();
	
	// loop
	listFrame->exec();		
	
	//
	for(int i = 0; i < 6; i++)
	{
		listFrameLines.lineArray[i].clear();
	}

	listFrame->hide();
	
	delete listFrame;
	listFrame = NULL;
}

// ClistBox(standard)
void CTestMenu::testClistBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(standard)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);
	
	CHintBox loadBox("testClistBox(standard)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();
	
	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str());

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->addWidgetType(TYPE_STANDARD);
	//rightWidget->enableShrinkMenu();
	//rightWidget->paintMainFrame(true);

	// head
	rightWidget->enablePaintHead();
	rightWidget->setTitle("ClistBox (standard)", NEUTRINO_ICON_MOVIE);
	rightWidget->setTitleHAlign(CC_ALIGN_CENTER);
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setFormat("%d.%m.%Y %H:%M:%S");
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// itemInfo
	//rightWidget->enablePaintItemInfo(70);
	//rightWidget->setItemInfoMode(ITEMINFO_HINTITEM_MODE);
	//rightWidget->setItemInfoPos(Box.iX + Box.iWidth + 150, Box.iY + 100, 400, 400);
	//rightWidget->paintItemInfoBorder(BORDER_ALL);
	//rightWidget->paintItemInfoFrame(true);
	//rightWidget->enableItemInfoSaveScreen();
	//rightWidget->setItemInfoFont(SNeutrinoSettings::FONT_TYPE_PLUGINLIST_ITEMLARGE);
	
	//
	//rightWidget->paintScrollBar(false);
	
	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	//rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();
	rightWidget->hide();		
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(classic)
void CTestMenu::testClistBox2()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(classic)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(classic)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str());

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());

		rightWidget->addItem(item);
	}

	// widgettype
	rightWidget->setWidgetType(TYPE_CLASSIC);
	rightWidget->enableShrinkMenu();

	// head
	rightWidget->setTitle("ClistBox(classic)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setFormat("%d.%m.%Y %H:%M:%S");
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	//rightWidget->enablePaintItemInfo(70);
	//rightWidget->setItemInfoMode(ITEMINFO_HINT_MODE);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(extended)
void CTestMenu::testClistBox3()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(extended)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(extended)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		//item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());

		rightWidget->addItem(item);
	}

	// widgettype
	rightWidget->setWidgetType(TYPE_EXTENDED);
	rightWidget->enableShrinkMenu();

	// head
	rightWidget->setTitle("ClistBox(extended)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	rightWidget->enablePaintItemInfo(80);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(frame)
void CTestMenu::testClistBox4()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(frame)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(frame)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new CMenuForwarder(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());

		rightWidget->addItem(item);
	}

	// widgettype
	rightWidget->setWidgetType(TYPE_FRAME);
	rightWidget->setItemsPerPage(5,2);
	rightWidget->enableShrinkMenu();
	
	rightWidget->paintMainFrame(false);
	
	// head
	//rightWidget->enablePaintHead();
	rightWidget->setTitle("ClistBox (FRAME)", NEUTRINO_ICON_MOVIE);
	rightWidget->setTitleHAlign(CC_ALIGN_CENTER);
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);
	
	// foot
	//rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(DL_INFO)
void CTestMenu::testClistBox5()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(DL_INFO)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(DL_INFO)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->setWidgetType(TYPE_STANDARD);
	rightWidget->enableShrinkMenu();

	//
	rightWidget->addWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	
	rightWidget->addWidgetType(TYPE_FRAME);
	rightWidget->setItemsPerPage(5, 2);

	// head
	rightWidget->setTitle("ClistBox(DL_INFO)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	rightWidget->enablePaintItemInfo(80);
	rightWidget->setItemInfoMode(ITEMINFO_INFO_MODE);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(listBox mode)
void CTestMenu::testClistBox6()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox(DL_HINT)\n");

	CBox Box;
	
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 20;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(DL_HINT)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		//item->setWidgetMode(MODE_MENU);
		item->setBorderMode(BORDER_TOPBOTTOM);
		//item->setGradient(LIGHT2DARK2LIGHT);
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->setWidgetType(TYPE_STANDARD);
	rightWidget->enableShrinkMenu();

	//
	rightWidget->addWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	rightWidget->addWidgetType(TYPE_FRAME);

	rightWidget->setItemsPerPage(5, 2);

	// head
	rightWidget->setTitle("ClistBox(DL_HINT)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	rightWidget->enablePaintItemInfo(80);
	rightWidget->setItemInfoMode(ITEMINFO_HINT_MODE);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(listBox mode)
void CTestMenu::testClistBox7()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox7(DL_HINTITEM)\n");

	CBox Box;
	
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20)/2;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = 50; //frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(DL_HINT)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		//item->setWidgetMode(MODE_MENU);
		item->setBorderMode(BORDER_TOPBOTTOM);
		//item->setGradient(LIGHT2DARK2LIGHT);
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->setWidgetType(TYPE_STANDARD);
	rightWidget->enableShrinkMenu();

	//
	rightWidget->addWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	rightWidget->addWidgetType(TYPE_FRAME);

	rightWidget->setItemsPerPage(5, 2);

	// head
	rightWidget->setTitle("ClistBox(DL_HINT)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	rightWidget->enablePaintItemInfo(80);
	rightWidget->setItemInfoMode(ITEMINFO_HINTITEM_MODE);
	rightWidget->setItemInfoPos(Box.iX + Box.iWidth + 150, Box.iY + 100, 400, 400);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox(listBox mode)
void CTestMenu::testClistBox8()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox8(DL_HINTICON)\n");

	CBox Box;
	
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20)/2;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = 50; //frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);

	CHintBox loadBox("ClistBox(DL_HINT)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();

	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "wplay");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		//item->setOptionInfo("OptionInfo");

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		//item->setOptionInfo1("OptionInfo1");

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());
		//item->setOptionInfo2("OptionInfo2");

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		//item->setWidgetMode(MODE_MENU);
		item->setBorderMode(BORDER_TOPBOTTOM);
		//item->setGradient(LIGHT2DARK2LIGHT);
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->setWidgetType(TYPE_STANDARD);
	rightWidget->enableShrinkMenu();

	//
	rightWidget->addWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	rightWidget->addWidgetType(TYPE_FRAME);

	rightWidget->setItemsPerPage(5, 2);

	// head
	rightWidget->setTitle("ClistBox(DL_HINT)", NEUTRINO_ICON_MOVIE);
	rightWidget->enablePaintHead();
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// footinfo
	rightWidget->enablePaintItemInfo(80);
	rightWidget->setItemInfoMode(ITEMINFO_HINTICON_MODE);
	rightWidget->setItemInfoPos(Box.iX + Box.iWidth + 150, Box.iY + 100, 400, 400);

	//rightWidget->setSelected(selected);
	
	//
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();		
	rightWidget->hide();
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}

// ClistBox9
void CTestMenu::testClistBox9()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testClistBox9 (hinthint\n");

	CBox Box;
	
	Box.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20)/2;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 20;

	Box.iX = 50; //frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - Box.iWidth ) >> 1 );
	Box.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - Box.iHeight) >> 1 );
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}

	rightWidget = new ClistBox(&Box);
	
	CHintBox loadBox("testClistBox(standard)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();
	
	// load items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str());

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());

		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = m_vMovieInfo[i].epgInfo1;
		tmp += "\n";
		tmp += m_vMovieInfo[i].epgInfo2;

		item->setHint(tmp.c_str());
		
		rightWidget->addItem(item);
	}

	// mode
	rightWidget->setWidgetType(TYPE_CLASSIC);
	rightWidget->addWidgetType(TYPE_STANDARD);
	rightWidget->addWidgetType(TYPE_EXTENDED);
	rightWidget->addWidgetType(TYPE_FRAME);
	rightWidget->setWidgetMode(MODE_MENU);
	rightWidget->enableShrinkMenu();
	rightWidget->paintMainFrame(true);

	// head
	rightWidget->enablePaintHead();
	rightWidget->setTitle("ClistBox (standard)", NEUTRINO_ICON_MOVIE);
	rightWidget->setTitleHAlign(CC_ALIGN_CENTER);
	rightWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	rightWidget->enablePaintDate();
	rightWidget->setFormat("%d.%m.%Y %H:%M:%S");
	rightWidget->setHeadLine(true, true);

	// footer
	rightWidget->enablePaintFoot();
	rightWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	rightWidget->setFootLine(true, true);

	// itemInfo
	rightWidget->enablePaintItemInfo(70);
	rightWidget->setItemInfoMode(ITEMINFO_HINTHINT_MODE);
	rightWidget->setItemInfoPos(Box.iX + Box.iWidth + 150, Box.iY + 100, 400, 400);
	rightWidget->paintItemInfoBorder(BORDER_ALL);
	rightWidget->paintItemInfoFrame(true);
	rightWidget->enableItemInfoSaveScreen();
	rightWidget->setItemInfoFont(SNeutrinoSettings::FONT_TYPE_PLUGINLIST_ITEMLARGE);
	
	//
	//rightWidget->paintScrollBar(false);
	
	//
	rightWidget->addKey(RC_ok, this, "wplay");
	rightWidget->addKey(RC_info, this, "linfo");
	rightWidget->addKey(RC_setup, this, "lsetup");
	
	rightWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	rightWidget->setSecTimer(sec_timer_id);
	rightWidget->exec();
	rightWidget->hide();		
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (rightWidget)
	{
		delete rightWidget;
		rightWidget = NULL;
	}
}


void CTestMenu::testCFrameBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::CTestMenu::testCFrameBox\n");

	// frameBox
	CBox topBox;
	
	topBox.iWidth = (g_settings.screen_EndX - g_settings.screen_StartX - 20)/4;
	topBox.iHeight = (g_settings.screen_EndY - g_settings.screen_StartY - 20);
	topBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - 20) - topBox.iWidth )>> 1;
	topBox.iY = g_settings.screen_StartY + 10;

	frameBoxWidget = new CFrameBox(&topBox);
	frameBoxWidget->enableSaveScreen();
	frameBoxWidget->setColor(0);

	CFrame * frame = NULL;

	frame = new CFrame();
	frame->setPosition(topBox.iX + 10, topBox.iY + 10, topBox.iWidth - 20, 60);
	frame->setTitle("Neu Filme");
	frame->setIconName(NEUTRINO_ICON_MOVIE);
	frame->setOption("in allen Kinos");
	frame->setActionKey(this, "help");
	frame->setColor(COL_RED_PLUS_0);
	frame->setGradient(DARK2LIGHT2DARK);
	frame->setHAlign(CC_ALIGN_CENTER);
	frameBoxWidget->addFrame(frame);
	
	frame = new CFrame();
	frame->setPosition(topBox.iX + 10, topBox.iY + 10 + 60 + 10, topBox.iWidth - 20, 60);
	frame->setTitle("Im Kino");
	frame->setActionKey(this, "help");
	frame->setColor(COL_GREEN_PLUS_0);
	frame->setGradient(LIGHT2DARK);
	frame->setHAlign(CC_ALIGN_CENTER);
	frameBoxWidget->addFrame(frame);

	frame = new CFrame();
	frame->setPosition(topBox.iX + 10, topBox.iY + 2*(10 + 60) +10, topBox.iWidth - 20, 60);
	frame->setTitle("Am populärsten");
	frame->setOption("(2019)");
	frame->setActionKey(this, "help");
	frame->setColor(COL_NOBEL_PLUS_0);
	frame->setHAlign(CC_ALIGN_CENTER);
	frameBoxWidget->addFrame(frame);
	
	// pic
	frame = new CFrame();
	frame->setMode(FRAME_PICTURE);
	frame->setPosition(topBox.iX + 10, topBox.iY + 3*(10 + 60) +50, topBox.iWidth - 20, 200);
	//frame->setActive(false);
	frame->setIconName(DATADIR "/icons/nopreview.jpg");
	frameBoxWidget->addFrame(frame);

	frame = new CFrame();
	frame->setPosition(topBox.iX + 10, topBox.iY + topBox.iHeight - 60 - 10, topBox.iWidth - 20, 60);
	frame->setTitle("Exit");
	frame->setActionKey(this, "exit");
	frame->setGradient(LIGHT2DARK);
	frame->setColor(COL_BLUE_PLUS_0);
	frame->setHAlign(CC_ALIGN_CENTER);
	frameBoxWidget->addFrame(frame);

	frameBoxWidget->setSelected(selected);

	frameBoxWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	frameBoxWidget->setSecTimer(sec_timer_id);
	frameBoxWidget->exec();
	frameBoxWidget->hide();		
	
	if (sec_timer_id)
	{
		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (frameBoxWidget)
	{
		delete frameBoxWidget;
		frameBoxWidget = NULL;
	}
}

void CTestMenu::testCFrameBox1()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu:testCFrameBox1:\n");
	
	// mainBox
	CBox box;
	box.iX = CFrameBuffer::getInstance()->getScreenX() + 40;
	box.iY = CFrameBuffer::getInstance()->getScreenY() + 40;
	box.iWidth = CFrameBuffer::getInstance()->getScreenWidth() - 80;
	box.iHeight = CFrameBuffer::getInstance()->getScreenHeight() - 80;

	frameBoxWidget = new CFrameBox(&box);

	CHintBox loadBox("CFrameBox", _("Scan for Movies ..."));
	loadBox.paint();
	
	loadMoviePlaylist();
	
	loadBox.hide();

	// titleBox
	CBox titleBox;
	titleBox.iX = box.iX + 10;
	titleBox.iY = box.iY + 40 + 10;
	titleBox.iWidth = box.iWidth;
	titleBox.iHeight = 40;

	// starBox
	CBox starBox;
	starBox.iX = box.iX +10;
	starBox.iY = box.iY + titleBox.iHeight + 40 + 10;
	starBox.iWidth = 25;
	starBox.iHeight = 25;

	// playBox
	CBox playBox;
	playBox.iWidth = 300;
	playBox.iHeight = 60;
	playBox.iX = box.iX + 10;
	playBox.iY = box.iY + box.iHeight - 10 - 40 - 60;

	// textBox
	CBox textBox;
	textBox.iWidth = box.iWidth/2 - 20;
	textBox.iHeight = box.iHeight - playBox.iHeight - 80 - titleBox.iHeight - starBox.iHeight - 4*10 - 100;
	textBox.iX = box.iX + 10 + 40;
	textBox.iY = starBox.iY + 20;

	// head
	frameBoxWidget->enablePaintHead();
	frameBoxWidget->setTitle("CFrameBox", NEUTRINO_ICON_MOVIE);
	frameBoxWidget->enablePaintDate();
	frameBoxWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);

	// artFrame
	CFrame * artFrame = new CFrame();
	artFrame->setMode(FRAME_PICTURE);
	artFrame->setPosition(box.iX + box.iWidth/2, box.iY + 40, box.iWidth/2, box.iHeight - 2*40);
	artFrame->setIconName(m_vMovieInfo[0].tfile.c_str());
	artFrame->setActive(false);

	frameBoxWidget->addFrame(artFrame);

	// title
	CFrame *titleFrame = new CFrame();
	titleFrame->setMode(FRAME_LABEL);
	titleFrame->setPosition(&titleBox);
	titleFrame->paintMainFrame(false);
	titleFrame->setTitle(m_vMovieInfo[0].epgTitle.c_str());
	titleFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	titleFrame->setActive(false);

	frameBoxWidget->addFrame(titleFrame);

	// star1
	CFrame *star1Frame = new CFrame();
	star1Frame->setMode(FRAME_ICON);
	star1Frame->setPosition(&starBox);
	star1Frame->setIconName(NEUTRINO_ICON_STAR_ON);
	star1Frame->paintMainFrame(false);
	star1Frame->setActive(false);

	frameBoxWidget->addFrame(star1Frame);

	// star2
	CFrame *star2Frame = new CFrame();
	star2Frame->setMode(FRAME_ICON);
	star2Frame->setPosition(starBox.iX + 25, starBox.iY, starBox.iWidth, starBox.iHeight);
	star2Frame->setIconName(NEUTRINO_ICON_STAR_ON);
	star2Frame->paintMainFrame(false);
	star2Frame->setActive(false);

	frameBoxWidget->addFrame(star2Frame);

	// star3
	CFrame *star3Frame = new CFrame();
	star3Frame->setMode(FRAME_ICON);
	star3Frame->setPosition(starBox.iX + 2*25, starBox.iY, starBox.iWidth, starBox.iHeight);
	star3Frame->setIconName(NEUTRINO_ICON_STAR_ON);
	star3Frame->paintMainFrame(false);
	star3Frame->setActive(false);

	frameBoxWidget->addFrame(star3Frame);

	// star4
	CFrame *star4Frame = new CFrame();
	star4Frame->setMode(FRAME_ICON);
	star4Frame->setPosition(starBox.iX + 3*25, starBox.iY, starBox.iWidth, starBox.iHeight);
	star4Frame->setIconName(NEUTRINO_ICON_STAR_OFF);
	star4Frame->paintMainFrame(false);
	star4Frame->setActive(false);

	frameBoxWidget->addFrame(star4Frame);

	// text
	CFrame *textFrame = new CFrame();
	textFrame->setMode(FRAME_TEXT);
	textFrame->setPosition(&textBox);
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;

	textFrame->setTitle(buffer.c_str());
	textFrame->paintMainFrame(false);
	textFrame->setActive(false);
	
	frameBoxWidget->addFrame(textFrame);

	// infoFrame
	CFrame * infoFrame = new CFrame();
	infoFrame->setPosition(playBox.iX + 300 + 10, playBox.iY, 300, 60);
	infoFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	infoFrame->setTitle("Movie Details");
	infoFrame->setIconName(NEUTRINO_ICON_INFO);
	infoFrame->setActionKey(this, "winfo");
	infoFrame->enableBorder();

	frameBoxWidget->addFrame(infoFrame);

	// play
	CFrame *playFrame = new CFrame();
	playFrame->setPosition(&playBox);
	playFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	playFrame->setTitle("Movie abspielen");
	playFrame->setIconName(NEUTRINO_ICON_PLAY);
	playFrame->setActionKey(this, "wplay");
	playFrame->enableBorder();

	frameBoxWidget->addFrame(playFrame);

	// foot
	frameBoxWidget->enablePaintFoot();
	frameBoxWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	//
	frameBoxWidget->paint();
	CFrameBuffer::getInstance()->blit();

	// loop
	uint32_t sec_timer_id = 0;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	frameBoxWidget->setSecTimer(sec_timer_id);
	frameBoxWidget->exec();
	frameBoxWidget->hide();
	
	if (sec_timer_id)
	{
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	if (frameBoxWidget)
	{
		delete frameBoxWidget;
		frameBoxWidget = NULL;
	}
}


// CMenuWidget (listBox mode)
void CTestMenu::testCMenuWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCMenuWidget (listBox mode)\n");
	
	// our listBox
	menuWidget = new CMenuWidget("CMenuWidget(listBox Mode)", NEUTRINO_ICON_MOVIE, CFrameBuffer::getInstance()->getScreenWidth() / 20 * 17, CFrameBuffer::getInstance()->getScreenHeight() / 20 * 18);

	//
	CHintBox loadBox("CMenuWidget(listBox mode)", _("Scan for Movies ..."));
	loadBox.paint();
	loadMoviePlaylist();
	loadBox.hide();
	
	// add items
	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "mplay", RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->setOption(m_vMovieInfo[i].epgChannel.c_str());
		item->set2lines();

		item->setHint(m_vMovieInfo[i].epgInfo2.c_str());

		// standard | classic
		item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());
		item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

		menuWidget->addItem(item);
	}

	menuWidget->setWidgetMode(MODE_LISTBOX);
	menuWidget->setWidgetType(TYPE_STANDARD);
	menuWidget->addWidgetType(TYPE_CLASSIC);
	menuWidget->addWidgetType(TYPE_EXTENDED);
	menuWidget->addWidgetType(TYPE_FRAME);
	menuWidget->setItemsPerPage(6, 2);
	menuWidget->enableShrinkMenu();

	// head
	menuWidget->enablePaintDate();
	menuWidget->setFormat("%d.%m.%Y %H:%M:%S");
	menuWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);

	// foot
	menuWidget->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	
	// footInfo
	menuWidget->enablePaintItemInfo(80);
	menuWidget->setItemInfoMode(ITEMINFO_HINT_MODE);

	menuWidget->addKey(RC_info, this, "minfo");
	menuWidget->addKey(RC_setup, this, "lsetup");

	menuWidget->exec(NULL, "");

	delete menuWidget;
	menuWidget = NULL;
}

// CMenuWidget (menu mode)
void CTestMenu::testCMenuWidget1()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCMenuWidget (menu mode)\n");
	
	//
	menuWidget = new CMenuWidget(_("CMenuWidget(Menu Mode)"), NEUTRINO_ICON_MAINMENU);
	
	item = new CMenuForwarder(_("TV / Radio"), true, NULL, CNeutrinoApp::getInstance(), "tvradioswitch");
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_TV);
	item->setIconName(NEUTRINO_ICON_BUTTON_RED);
	item->setDirectKey(RC_red);
	menuWidget->addItem(item);

	item = new CMenuForwarder(_("Timer / EPG"), true, NULL, new CEPGMenuHandler());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_SLEEPTIMER);
	item->setIconName(NEUTRINO_ICON_BUTTON_GREEN);
	item->setDirectKey(RC_green);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("Features"), true, NULL, CNeutrinoApp::getInstance(), "features");
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_FEATURES);
	item->setIconName(NEUTRINO_ICON_BUTTON_YELLOW);
	item->setDirectKey(RC_yellow);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("Service"), true, NULL, new CServiceMenu());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_SERVICE);
	item->setIconName(NEUTRINO_ICON_BUTTON_BLUE);
	item->setDirectKey(RC_blue);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("Settings"), true, NULL, new CMainSettingsMenu());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_OSDSETTINGS);
	item->setIconName(NEUTRINO_ICON_BUTTON_SETUP_SMALL);
	item->setDirectKey(RC_setup);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("OSD"), true, NULL, new COSDSettings());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_SETTINGS);
	item->setIconName(NEUTRINO_ICON_BUTTON_SETUP_SMALL);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("Information"), true, NULL, new CInfoMenu());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_BOXINFO);
	item->setIconName(NEUTRINO_ICON_BUTTON_INFO_SMALL);
	item->setDirectKey(RC_info);
	menuWidget->addItem(item);

	item = new CMenuForwarder(_("Power Menu"), true, NULL, new CPowerMenu());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_POWERMENU);
	item->setIconName(NEUTRINO_ICON_BUTTON_POWER);
	item->setDirectKey(RC_standby);
	menuWidget->addItem(item);
	
	item = new CMenuForwarder(_("Media Player"), true, NULL, new CMediaPlayerMenu());
	item->setHintIcon(NEUTRINO_ICON_MENUITEM_MEDIAPLAYER);
	item->setIconName(NEUTRINO_ICON_VIDEO);
	item->setDirectKey(RC_video);
	menuWidget->addItem(item);
	
	menuWidget->setWidgetMode(MODE_MENU);
	menuWidget->setWidgetType(TYPE_STANDARD);
	menuWidget->addWidgetType(TYPE_CLASSIC);
	menuWidget->addWidgetType(TYPE_EXTENDED);
	menuWidget->addWidgetType(TYPE_FRAME);
	menuWidget->setItemsPerPage(6, 2);
	menuWidget->enableShrinkMenu();

	// head
	menuWidget->enablePaintDate();
	menuWidget->setFormat("%d.%m.%Y %H:%M:%S");
	menuWidget->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);


	//
	menuWidget->addKey(RC_setup, this, "lsetup");

	menuWidget->exec(NULL, "");

	delete menuWidget;
	menuWidget = NULL;
}

// CMenuWidget (setup mode)
void CTestMenu::testCMenuWidget2()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCMenuWidget (setup mode)\n");
	
	CAudioPlayerSettings * audioPlayerSettingsMenu = new CAudioPlayerSettings();

	audioPlayerSettingsMenu->exec(this, "");
	delete audioPlayerSettingsMenu;
	audioPlayerSettingsMenu = NULL;	
}


// CStringInput
void CTestMenu::testCStringInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCStringInput\n");

	std::string value;
	CStringInput * stringInput = new CStringInput("CStringInput", value.c_str());
	
	stringInput->exec(NULL, "");
	stringInput->hide();
	delete stringInput;
	stringInput = NULL;
	value.clear();
}

// CStringInputSMS
void CTestMenu::testCStringInputSMS()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCStringInputSMS\n");

	std::string value;
	CStringInputSMS * stringInputSMS = new CStringInputSMS("CStringInputSMS", value.c_str());
	
	stringInputSMS->exec(NULL, "");
	stringInputSMS->hide();
	delete stringInputSMS;
	value.clear();
}

// CPINInput
void CTestMenu::testCPINInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCPINInput\n");

	std::string value;
	CPINInput * pinInput = new CPINInput("CPINInput", value.c_str());
	
	pinInput->exec(NULL, "");
	pinInput->hide();
	delete pinInput;
	pinInput = NULL;
	value.clear();
}

// CPLPINInput
void CTestMenu::testCPLPINInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCPLPINInput\n");

	std::string value;
	CPLPINInput * pinInput = new CPLPINInput("CPLPINInput", value.c_str());
	
	pinInput->exec(NULL, "");
	pinInput->hide();
	delete pinInput;
	pinInput = NULL;
	value.clear();
}

// CPINChangeWidget
void CTestMenu::testCPINChangeWidget()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCPINChangeWidget\n");

	std::string value;
	CPINChangeWidget * pinInput = new CPINChangeWidget("CPINChangeWidget", value.c_str());
	
	pinInput->exec(NULL, "");
	pinInput->hide();
	delete pinInput;
	pinInput = NULL;
	value.clear();
}

// CIPInput
void CTestMenu::testCIPInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCIPInput\n");

	std::string value;
	CIPInput * ipInput = new CIPInput(_("IP:"), value);
	
	ipInput->exec(NULL, "");
	ipInput->hide();
	delete ipInput;
	value.clear();
}

// CMACInput
void CTestMenu::testCMACInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCMACInput\n");

	std::string value;
	CMACInput * macInput = new CMACInput(_("MAC address:"), (char *)value.c_str());
	
	macInput->exec(NULL, "");
	macInput->hide();
	delete macInput;
	macInput = NULL;
	value.clear();
}

// CDateInput
void CTestMenu::testCDateInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCDateInput\n");

	time_t value;
	CDateInput * dateInput = new CDateInput(_("Date:"), &value);
	
	dateInput->exec(NULL, "");
	dateInput->hide();
	delete dateInput;
}

// CTimeInput
void CTestMenu::testCTimeInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCTimeInput\n");

	std::string value;
	CTimeInput * timeInput = new CTimeInput(_("Time:"), (char *)value.c_str());
	
	timeInput->exec(NULL, "");
	timeInput->hide();
	delete timeInput;
	timeInput = NULL;
	value.clear();
}

// CIntInput
void CTestMenu::testCIntInput()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCIntInput\n");

	int value;
	CIntInput * intInput = new CIntInput(_("Test"), value);
	
	intInput->exec(NULL, "");
	intInput->hide();
	delete intInput;
	intInput = NULL;	
}

// CInfoBox
void CTestMenu::testCInfoBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCInfoBox\n");

	loadMoviePlaylist();
	
	std::string buffer;
	buffer = m_vMovieInfo[0].epgInfo1;
	buffer += "\n";
	buffer += m_vMovieInfo[0].epgInfo2;
	
	// scale pic
	int p_w = 0;
	int p_h = 0;

	scaleImage(m_vMovieInfo[0].tfile, &p_w, &p_h);
	
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, "CInfoBox", NEUTRINO_ICON_INFO);	
	
	infoBox->setBorderMode(BORDER_ALL);
	infoBox->setBackgroundColor(/*make16color(0xBEBEBE)*/COL_SILVER_PLUS_0);
	infoBox->setText(buffer.c_str(), m_vMovieInfo[0].tfile.c_str(), p_w, p_h, PIC_RIGHT, true, true);
	infoBox->setTextColor(COL_LIME);
	
	infoBox->setHeadColor(COL_NOBEL_PLUS_0);
	infoBox->setHeadCorner(NO_RADIUS, CORNER_NONE);
	infoBox->setHeadGradient(NOGRADIENT);
	
	infoBox->setFootColor(COL_NOBEL_PLUS_0);
	infoBox->setFootCorner(NO_RADIUS, CORNER_NONE);
	infoBox->setFootGradient(NOGRADIENT);
	
	infoBox->exec();
	
	delete infoBox;
	infoBox = NULL;
}

// CMessageBox
void CTestMenu::testCMessageBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCMessageBox\n");

	CMessageBox * messageBox = new CMessageBox(_("Information"), "testing CMessageBox"/*, 600, NEUTRINO_ICON_INFO, mbrYes, mbNone*/);
	
	int res = messageBox->exec();

	printf("res:%d messageBox->result:%d\n", res, messageBox->result);

	delete messageBox;
	messageBox = NULL;
}

// MessageBox
void CTestMenu::testCMessageBoxInfoMsg()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCMessageBox\n");

	MessageBox(_("Information"), "testing CMessageBoxInfoMsg", mbrBack, mbBack, NEUTRINO_ICON_INFO);
}

// MessageBox
void CTestMenu::testCMessageBoxErrorMsg()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCMessageBox\n");

	MessageBox(_("Error"), "testing CMessageBoxErrorMsg", mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
}

// CHintBox
void CTestMenu::testCHintBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCHintBox\n");

	CHintBox * hintBox = new CHintBox(_("Information"), "testing CHintBox");
	
	hintBox->exec(10);

	delete hintBox;
	hintBox = NULL;
}

// HintBox
void CTestMenu::testCHintBoxInfo()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCHintBox\n");

	HintBox(_("Information"), "testCHintBoxInfo", HINTBOX_WIDTH, 10, NEUTRINO_ICON_INFO);
}

// CHelpBox
void CTestMenu::testCHelpBox()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCHelpBox\n");

	CHelpBox * helpBox = new CHelpBox();
	
	// text
	helpBox->addLine("helpBox");

	// icon|text
	helpBox->addLine(NEUTRINO_ICON_BUTTON_RED, "Huhu :-P", g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], 
	COL_RED, true);

	//
	helpBox->addLine(NEUTRINO_ICON_BUTTON_GREEN, "Huhu :-)", g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], COL_BLUE, true);

	helpBox->addSeparator();

	//
	helpBox->addLine("neutrinoHD2 the best GUI :-P", g_Font[SNeutrinoSettings::FONT_TYPE_MENU], COL_MENUCONTENTINACTIVE, true);
	

	//
	helpBox->addSeparator();
	
	helpBox->add2Line("Gui: ", "neutrinoHD2 the best GUI :-P", g_Font[SNeutrinoSettings::FONT_TYPE_MENU], COL_MENUCONTENTINACTIVE, true, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], COL_BLUE, true);

	// icon
	helpBox->addLine(NEUTRINO_ICON_BUTTON_YELLOW, "Huhu :-P", g_Font[SNeutrinoSettings::FONT_TYPE_MENU], COL_GREEN, true);

	helpBox->addSeparator();

	//
	helpBox->addLine("Huhu :-)");

	//
	helpBox->addLine(NEUTRINO_ICON_BUTTON_BLUE, "neutrinoHD2 the best GUI :-P", g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], COL_YELLOW, true);

	helpBox->addSeparator();
		
	helpBox->addLine(NEUTRINO_ICON_MENUITEM_HDDSETTINGS, "neutrinoHD2 the best GUI :-P", g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], COL_YELLOW, true);
		
	helpBox->addLine(NEUTRINO_ICON_MENUITEM_SCANSETTINGS, "neutrinoHD2 the best GUI :-P", g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1], COL_YELLOW, true);

	helpBox->show(_("Information"), HELPBOX_WIDTH, -1, mbrBack, mbNone);
	
	delete helpBox;
	helpBox = NULL;
}

// CProgressWindow
void CTestMenu::testCProgressWindow()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::testCProgressWindow\n");

	progressWindow = new CProgressWindow();
	progressWindow->setTitle("CProgressWindow");
	progressWindow->paint();
	
	progressWindow->showStatusMessageUTF("testing CProgressWindow:0");
	progressWindow->showGlobalStatus(0);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(10);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(20);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showStatusMessageUTF("testing CProgressWindow:30");
	progressWindow->showGlobalStatus(30);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(40);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(50);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showStatusMessageUTF("testing CProgressWindow:60");
	progressWindow->showGlobalStatus(60);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(70);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showStatusMessageUTF("testing CProgressWindow:80");
	progressWindow->showGlobalStatus(80);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showGlobalStatus(90);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	progressWindow->showStatusMessageUTF("testing CProgressWindow:100");
	progressWindow->showGlobalStatus(100);
	CFrameBuffer::getInstance()->blit();
	usleep(1000000);
	
	progressWindow->hide();
	delete progressWindow;
	progressWindow = NULL;
}

// play Movie Url
void CTestMenu::testPlayMovieURL()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayMovieURL\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
		
	CFileBrowser * fileBrowser;
	
	fileBrowser = new CFileBrowser();

	fileFilter.clear();
	
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

	fileBrowser->Multi_Select    = false;
	fileBrowser->Dirs_Selectable = false;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_moviedir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();
		
		CFile * file;
		
		if ((file = fileBrowser->getSelectedFile()) != NULL) 
		{		
			tmpMoviePlayerGui.addToPlaylist(*file);
			tmpMoviePlayerGui.exec(NULL, "");
		}

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

// play audio Url
void CTestMenu::testPlayAudioURL()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayAudioURL\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
		
	CFileBrowser * fileBrowser;
	
	fileBrowser = new CFileBrowser();

	fileFilter.clear();
	
	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
	fileFilter.addFilter("ogg");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");
	
	fileBrowser->Multi_Select = false;
	fileBrowser->Dirs_Selectable = false;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_audioplayerdir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();
		
		CFile * file;
		
		if ((file = fileBrowser->getSelectedFile()) != NULL) 
		{	
			if (file->getType() == CFile::FILE_AUDIO)
			{
				tmpAudioPlayerGui.addToPlaylist(*file);
				tmpAudioPlayerGui.exec(NULL, "urlplayback");
			}
		}

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

// show pic url
void CTestMenu::testShowPictureURL()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testShowPictureURL\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	CFileBrowser * fileBrowser;
	
	fileBrowser = new CFileBrowser();

	fileFilter.clear();
	
	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");
	
	fileBrowser->Multi_Select    = false;
	fileBrowser->Dirs_Selectable = false;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_picturedir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();
		
		CFile * file;
		
		if ((file = fileBrowser->getSelectedFile()) != NULL) 
		{					
			tmpPictureViewerGui.addToPlaylist(*file);
			tmpPictureViewerGui.exec(NULL, "");
		}

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

// play movie folder
void CTestMenu::testPlayMovieFolder()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayMovieFolder\n");
	
	CFileBrowser * fileBrowser;
	
	fileBrowser = new CFileBrowser();

	fileFilter.clear();
	
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

	fileBrowser->Multi_Select = true;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_moviedir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();
		
		CFileList::const_iterator files = fileBrowser->getSelectedFiles().begin();
		for(; files != fileBrowser->getSelectedFiles().end(); files++)
		{		
			tmpMoviePlayerGui.addToPlaylist(*files);
		}
		
		tmpMoviePlayerGui.exec(NULL, "urlplayback");
		
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

// play audio folder
void CTestMenu::testPlayAudioFolder()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayAudioFolder\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
		
	CFileBrowser * fileBrowser;

	fileFilter.clear();
	
	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("m2a");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
	fileFilter.addFilter("ogg");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");
	
	fileBrowser = new CFileBrowser();
	fileBrowser->Multi_Select = true;
	fileBrowser->Dirs_Selectable = false;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_audioplayerdir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();

		CFileList::const_iterator files = fileBrowser->getSelectedFiles().begin();
		
		for(; files != fileBrowser->getSelectedFiles().end(); files++)
		{

			if ( (files->getExtension() == CFile::EXTENSION_CDR)
					||  (files->getExtension() == CFile::EXTENSION_MP3)
					||  (files->getExtension() == CFile::EXTENSION_WAV)
					||  (files->getExtension() == CFile::EXTENSION_FLAC)
			)
			{
				tmpAudioPlayerGui.addToPlaylist(*files);
			}
		}
		
		tmpAudioPlayerGui.exec(NULL, "urlplayback");

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

// show pic folder
void CTestMenu::testShowPictureFolder()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testShowPictureFolder\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	CFileBrowser * fileBrowser;
	
	fileFilter.clear();
	
	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");
	
	fileBrowser = new CFileBrowser();
	fileBrowser->Multi_Select    = true;
	fileBrowser->Dirs_Selectable = true;
	fileBrowser->Filter = &fileFilter;
	
	std::string Path_local = g_settings.network_nfs_picturedir;

BROWSER:
	if (fileBrowser->exec(Path_local.c_str()))
	{
		Path_local = fileBrowser->getCurrentDir();
				
		CFileList::const_iterator files = fileBrowser->getSelectedFiles().begin();
		
		for(; files != fileBrowser->getSelectedFiles().end(); files++)
		{

			if (files->getType() == CFile::FILE_PICTURE)
			{
				tmpPictureViewerGui.addToPlaylist(*files);
			}
		}
		
		tmpPictureViewerGui.exec(NULL, "urlplayback");

		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if (msg != RC_home) 
		{
			goto BROWSER;
		}
	}
	
	delete fileBrowser;
}

// start plugin
void CTestMenu::testStartPlugin()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testStartPlugin\n");
	
	g_PluginList->startPlugin("youtube");
}

// show actuell epg
void CTestMenu::testShowActuellEPG()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testShowActuelEPG\n");
	
	std::string title = "testShowActuellEPG:";
	std::string buffer;

	// get EPG
	CEPGData epgData;
	event_id_t epgid = 0;
			
	if(CSectionsd::getInstance()->getActualEPGServiceKey(CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID() & 0xFFFFFFFFFFFFULL, &epgData))
		epgid = epgData.eventID;

	if(epgid != 0) 
	{
		CShortEPGData epgdata;
				
		if(CSectionsd::getInstance()->getEPGidShort(epgid, &epgdata)) 
		{
			title += CNeutrinoApp::getInstance()->channelList->getActiveChannelName();
			title += ":";
			title += epgdata.title;

			buffer = epgdata.info1;
			buffer += "\n";
			buffer += epgdata.info2;	
		}
	}

	title += getNowTimeStr("%d.%m.%Y %H:%M");
	//
	
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, title.c_str(), NULL);

	infoBox->setText(buffer.c_str());
	
	infoBox->exec();
	delete infoBox;
	infoBox = NULL;	
}

// CChannelSelect
void CTestMenu::testChannelSelectWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testChannelSelectWidget\n");
	
	CSelectChannelWidget * CSelectChannelWidgetHandler = new CSelectChannelWidget();

	std::string mode = "tv";

	if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv)
		mode = "tv";
	else if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio)
		mode = "radio";
	//else if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_webtv)
	//	mode = "webtv";

	CSelectChannelWidgetHandler->exec(NULL, mode);
		
	//CSelectChannelWidget_TVChanID;
	//CSelectChannelWidget_TVChanName.c_str();

	printf("testChannelSelectWidget: chan_id:%llx chan_name:%s\n", CSelectChannelWidgetHandler->getChannelID(), CZapit::getInstance()->getChannelName(CSelectChannelWidgetHandler->getChannelID()).c_str());
		
	delete CSelectChannelWidgetHandler;
	CSelectChannelWidgetHandler = NULL;
}

// Bedit
void CTestMenu::testBEWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testBEWidget\n");
		
	CBEBouquetWidget* BEWidget = new CBEBouquetWidget();

	BEWidget->exec(NULL, "");

	delete BEWidget;
	BEWidget = NULL;
}

// AVSelect
void CTestMenu::testAVSelectWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testAVSelectWidget\n");
	
	CAVPIDSelectWidget * AVSelectHandler = new CAVPIDSelectWidget();
	AVSelectHandler->exec(NULL, "");
		
	delete AVSelectHandler;
	AVSelectHandler = NULL;
}

// CAudioSelect
void CTestMenu::testAudioSelectWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testAudioSelectWidget\n");
	
	CAudioSelectMenuHandler * ASelectHandler = new CAudioSelectMenuHandler();
	ASelectHandler->exec(NULL, "");
	delete ASelectHandler;
	ASelectHandler = NULL;
}

// DVBSubSelect
void CTestMenu::testDVBSubSelectWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testDVBSelectWidget\n");
	
	CDVBSubSelectMenuHandler * dvbSubSelectHandler = new CDVBSubSelectMenuHandler();
	dvbSubSelectHandler->exec(NULL, "");
	delete dvbSubSelectHandler;
	dvbSubSelectHandler = NULL;
}

void CTestMenu::testAlphaSetupWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testAlphaSetup\n");
	
	CAlphaSetup * alphaSetup = new CAlphaSetup(_("Alpha Setup"), &g_settings.gtx_alpha);
	alphaSetup->exec(NULL, "");
	delete alphaSetup;
	alphaSetup = NULL;
}

void CTestMenu::testPSISetup()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPSISetup\n");
	
	CPSISetup * psiSetup = new CPSISetup(_("PSI Setup"), &g_settings.contrast, &g_settings.saturation, &g_settings.brightness, &g_settings.tint);
	psiSetup->exec(NULL, "");
	delete psiSetup;
	psiSetup = NULL;
}

void CTestMenu::testRCLock()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testRCLock\n");
	
	CRCLock * rcLock = new CRCLock();
	rcLock->exec(NULL, CRCLock::NO_USER_INPUT);
	delete rcLock;
	rcLock = NULL;
}

void CTestMenu::testSleepTimerWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testSleepTimerWidget\n");
	
	//FIXME:TEST
	/*
	CSleepTimerWidget * sleepTimerHandler = new CSleepTimerWidget();
	sleepTimerHandler->exec(NULL, "");
	delete sleepTimerHandler;
	sleepTimerHandler = NULL;
	*/
}

void CTestMenu::testMountGUI()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testMountGUI\n");
	
	CNFSMountGui * mountGUI = new CNFSMountGui();
	mountGUI->exec(NULL, "");
	delete mountGUI;
	mountGUI = NULL;
}

void CTestMenu::testUmountGUI()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testUmountGUI\n");
	
	CNFSUmountGui * umountGUI = new CNFSUmountGui();
	umountGUI->exec(NULL, "");
	delete umountGUI;
	umountGUI = NULL;
}

void CTestMenu::testMountSmallMenu()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testMountSmallMenu\n");
	
	CNFSSmallMenu * mountSmallMenu = new CNFSSmallMenu();
	mountSmallMenu->exec(NULL, "");
	delete mountSmallMenu;
	mountSmallMenu = NULL;
}

void CTestMenu::testVFDController()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testVFDController\n");
	
	CVfdControler * vfdControllerHandler = new CVfdControler(_("Display settings"), NULL);
	vfdControllerHandler->exec(NULL, "");
	delete vfdControllerHandler;
	vfdControllerHandler = NULL;
}

void CTestMenu::testColorChooser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testColorChooser\n");
	
	CColorChooser * colorChooserHandler = new CColorChooser(_("Background"), &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue, &g_settings.menu_Head_alpha, CNeutrinoApp::getInstance()->colorSetupNotifier);

	colorChooserHandler->exec(NULL, "");
	//delete colorChooserHandler;
	//colorChooserHandler = NULL;
}

void CTestMenu::testKeyChooser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testKeyChooser\n");
	
	CKeyChooserItem * keyChooser = new CKeyChooserItem("testing CKeyChooser: key_screenshot:", &g_settings.key_screenshot);

	keyChooser->exec(NULL, "");
	delete keyChooser;
	keyChooser = NULL;
}

void CTestMenu::testMountChooser()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testMountChooser\n");
	
	CMountChooser * mountChooser = new CMountChooser("testing CMountChooser", NEUTRINO_ICON_SETTINGS, NULL, g_settings.network_nfs_moviedir, g_settings.network_nfs_recordingdir);

	mountChooser->exec(NULL, "");
	delete mountChooser;
	mountChooser = NULL;
}

void CTestMenu::testPluginsList()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPluginlist\n");
	
	CPluginList * pluginList = new CPluginList();
	pluginList->exec(NULL, "");
	delete pluginList;
	pluginList = NULL;
}

void CTestMenu::testPlayMovieDir()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayMovieDi\n");
	
	filelist.clear();
	fileFilter.clear();
	
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
	
	std::string Path_local = g_settings.network_nfs_moviedir;

	if(CFileHelpers::getInstance()->readDir(Path_local, &filelist, &fileFilter))
	{
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			tmpMoviePlayerGui.addToPlaylist(*files);
		}
		
		tmpMoviePlayerGui.exec(NULL, "urlplayback");
	}
}

void CTestMenu::testPlayAudioDir()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testPlayAudioDir\n");
	
	filelist.clear();
	fileFilter.clear();
	
	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("m2a");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
	fileFilter.addFilter("ogg");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");
	
	std::string Path_local = g_settings.network_nfs_audioplayerdir;

	if(CFileHelpers::getInstance()->readDir(Path_local, &filelist, &fileFilter))
	{
		// filter them
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			if ( (files->getExtension() == CFile::EXTENSION_CDR)
					||  (files->getExtension() == CFile::EXTENSION_MP3)
					||  (files->getExtension() == CFile::EXTENSION_WAV)
					||  (files->getExtension() == CFile::EXTENSION_FLAC)
			)
			{
				tmpAudioPlayerGui.addToPlaylist(*files);
			}
		}
		
		tmpAudioPlayerGui.exec(NULL, "");
	}
}

void CTestMenu::testShowPictureDir()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testShowPictureDir\n");
	
	filelist.clear();
	fileFilter.clear();

	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");
	
	std::string Path_local = g_settings.network_nfs_picturedir;

	if(CFileHelpers::getInstance()->readDir(Path_local, &filelist, &fileFilter))
	{		
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			if (files->getType() == CFile::FILE_PICTURE)
			{
				tmpPictureViewerGui.addToPlaylist(*files);
			}
		}
		
		tmpPictureViewerGui.exec(NULL, "urlplayback");
	}
}

// CChannellist
void CTestMenu::testCChannellist()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCChannellist\n");
	
	webTVchannelList = new CChannelList("CTestMenu::testWebTVChannellist:");

	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		if (it->second.isWebTV) 
		{
			webTVchannelList->addChannel(&(it->second));
		}
	}

	webTVchannelList->exec(); // with zap
}


// CBouquetlist
void CTestMenu::testCBouquetlist()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testCBouquetlist\n");
	
	webTVBouquetList = new CBouquetList("CTestMenu::testWebTVBouquetlist");

	for (int i = 0; i < g_bouquetManager->Bouquets.size(); i++) 
	{
		if (g_bouquetManager->Bouquets[i]->bWebTV && !g_bouquetManager->Bouquets[i]->tvChannels.empty())
		{
			CBouquet *ltmp = webTVBouquetList->addBouquet(g_bouquetManager->Bouquets[i]);

			ZapitChannelList *channels = &(g_bouquetManager->Bouquets[i]->tvChannels);
	
			ltmp->channelList->setSize(channels->size());

			for(int j = 0; j < (int) channels->size(); j++) 
			{
				ltmp->channelList->addChannel((*channels)[j]);
			}
		}
	}

	webTVBouquetList->exec(true); // with zap
}

// skin
void CTestMenu::testSkinWidget()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testSkinWidget\n");
	
	CNeutrinoApp::getInstance()->eraseWidget("testmenu");
	
	//
	std::string skin = "\n<skin>\n\t<WIDGET name=\"testmenu\" posx=\"0\" posy=\"0\" width=\"700\" height=\"720\" paintframe=\"1\">\n\t\t<LISTBOX posx=\"30\" posy=\"100\" width=\"640\" height=\"520\" paintframe=\"1\" mode=\"MODE_MENU\" type=\"TYPE_STANDARD\" scrollbar=\"1\"/>\n\t\t<HEAD posx=\"30\" posy=\"50\" width=\"640\" height=\"40\" paintframe=\"1\" gradient=\"DARK2LIGHT2DARK\" corner=\"CORNER_ALL\" radius=\"RADIUS_MID\" title=\"Test Skin\" icon=\"multimedia\" paintdate=\"1\" format=\"%d.%m.%Y %H:%M:%S\"/>\n\t\t<FOOT posx=\"30\" posy=\"630\" width=\"640\" height=\"40\" paintframe=\"1\" gradient=\"DARK2LIGHT2DARK\" corner=\"CORNER_ALL\" radius=\"RADIUS_MID\">\n\t\t\t<BUTTON_LABEL name=\"info\"/>\n\t\t</FOOT>\n\t</WIDGET>\n</skin>\n";

	CNeutrinoApp::getInstance()->parseSkin(skin.c_str(), true);
	
	CNeutrinoApp::getInstance()->execSkinWidget("testmenu", NULL, "");
}

// skin2
void CTestMenu::testSkinWidget2()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testSkinWidget2\n");
	
	CNeutrinoApp::getInstance()->eraseWidget("testmenu");
	
	//
	std::string skin = PLUGINDIR "/test/skin.xml";
	CNeutrinoApp::getInstance()->parseSkin(skin.c_str());
	
	CNeutrinoApp::getInstance()->execSkinWidget("testmenu", NULL, "");
}

// skin3
void CTestMenu::testSkinWidget3()
{
	dprintf(DEBUG_NORMAL, "\nCTestMenu::testSkinWidget3\n");
	
	CNeutrinoApp::getInstance()->eraseWidget("testmenu");
	
	//
	std::string skin = PLUGINDIR "/test/test.xml";
	CNeutrinoApp::getInstance()->parseSkin(skin.c_str());
	
	CNeutrinoApp::getInstance()->execSkinWidget("testmenu", NULL, "");
}

// exec
int CTestMenu::exec(CMenuTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CTestMenu::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();
	
	if(actionKey == "widget")
	{
		testCWidget();

		return RETURN_REPAINT;
	}
	else if (actionKey == "ccwindow")
	{
		testCWindowWidget();
		
		return RETURN_REPAINT;
	}
	else if (actionKey == "textboxwidget")
	{
		testCTextBoxWidget();
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "menuforwarder")
	{
		MessageBox(_("Information"), "testing CMenuForwarder", mbrBack, mbBack, NEUTRINO_ICON_INFO);

		return RETURN_REPAINT;
	}
	else if(actionKey == "listboxitem")
	{
		MessageBox(_("Information"), "testing ClistBoxItem", mbrBack, mbBack, NEUTRINO_ICON_INFO);

		return RETURN_REPAINT;
	}
	else if(actionKey == "icon")
	{
		testCIcon();

		return RETURN_REPAINT;
	}
	else if(actionKey == "image")
	{
		testCImage();

		return RETURN_REPAINT;
	}
	else if(actionKey == "window")
	{
		testCWindow();

		return RETURN_REPAINT;
	}
	else if(actionKey == "windowshadow")
	{
		testCWindowShadow();

		return RETURN_REPAINT;
	}
	else if(actionKey == "windowcustomcolor")
	{
		testCWindowCustomColor();
		return RETURN_REPAINT;
	}
	else if(actionKey == "windowcomponent")
	{
		testCWindowCComponent();
		return RETURN_REPAINT;
	}
	else if(actionKey == "headers")
	{
		testCHeaders();
		return RETURN_REPAINT;
	}
	else if(actionKey == "footers")
	{
		testCFooters();
		return RETURN_REPAINT;
	}
	else if(actionKey == "stringinput")
	{
		testCStringInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "stringinputsms")
	{
		testCStringInputSMS();

		return RETURN_REPAINT;
	}
	else if(actionKey == "pininput")
	{
		testCPINInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "plpininput")
	{
		testCPLPINInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "pinchangewidget")
	{
		testCPINChangeWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "ipinput")
	{
		testCIPInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "macinput")
	{
		testCMACInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "dateinput")
	{
		testCDateInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "timeinput")
	{
		testCTimeInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "intinput")
	{
		testCIntInput();

		return RETURN_REPAINT;
	}
	else if(actionKey == "infobox")
	{
		testCInfoBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "messagebox")
	{
		testCMessageBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "messageboxinfomsg")
	{
		testCMessageBoxInfoMsg();

		return RETURN_REPAINT;
	}
	else if(actionKey == "messageboxerrormsg")
	{
		testCMessageBoxErrorMsg();

		return RETURN_REPAINT;
	}
	else if(actionKey == "hintbox")
	{
		testCHintBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "hintboxinfo")
	{
		testCHintBoxInfo();

		return RETURN_REPAINT;
	}
	else if(actionKey == "helpbox")
	{
		testCHelpBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "textbox")
	{
		testCTextBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listframe")
	{
		testCListFrame();

		return RETURN_REPAINT;
	}
	else if(actionKey == "progressbar")
	{
		testCProgressBar();

		return RETURN_REPAINT;
	}
	else if(actionKey == "progresswindow")
	{
		testCProgressWindow();

		return RETURN_REPAINT;
	}
	else if(actionKey == "buttons")
	{
		testCButtons();

		return RETURN_REPAINT;
	}
	else if(actionKey == "hbuttons")
	{
		testCHButtons();

		return RETURN_REPAINT;
	}
	else if(actionKey == "spinner")
	{
		testCSpinner();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox")
	{
		testClistBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox2")
	{
		testClistBox2();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox3")
	{
		testClistBox3();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox4")
	{
		testClistBox4();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox5")
	{
		testClistBox5();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox6")
	{
		testClistBox6();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox7")
	{
		testClistBox7();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox8")
	{
		testClistBox8();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listbox9")
	{
		testClistBox9();

		return RETURN_REPAINT;
	}
	else if(actionKey == "framebox")
	{
		testCFrameBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playmovieurl")
	{
		testPlayMovieURL();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playaudiourl")
	{
		testPlayAudioURL();

		return RETURN_REPAINT;
	}
	else if(actionKey == "showpictureurl")
	{
		testShowPictureURL();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playmoviefolder")
	{
		testPlayMovieFolder();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playaudiofolder")
	{
		testPlayAudioFolder();

		return RETURN_REPAINT;
	}
	else if(actionKey == "showpicturefolder")
	{
		testShowPictureFolder();

		return RETURN_REPAINT;
	}
	else if(actionKey == "startplugin")
	{
		testStartPlugin();

		return RETURN_REPAINT;
	}
	else if(actionKey == "showepg")
	{
		testShowActuellEPG();

		return RETURN_REPAINT;
	}
	else if(actionKey == "channelselect")
	{
		testChannelSelectWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "bewidget")
	{
		testBEWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "avselect")
	{
		testAVSelectWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "aselect")
	{
		testAudioSelectWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "dvbsubselect")
	{
		testDVBSubSelectWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "alphasetup")
	{
		testAlphaSetupWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "psisetup")
	{
		testPSISetup();

		return RETURN_REPAINT;
	}
	else if(actionKey == "rclock")
	{
		testRCLock();

		return RETURN_REPAINT;
	}
	else if(actionKey == "sleeptimer")
	{
		testSleepTimerWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "mountgui")
	{
		testMountGUI();

		return RETURN_REPAINT;
	}
	else if(actionKey == "umountgui")
	{
		testUmountGUI();

		return RETURN_REPAINT;
	}
	else if(actionKey == "mountsmallmenu")
	{
		testMountSmallMenu();

		return RETURN_REPAINT;
	}
	else if(actionKey == "vfdcontroller")
	{
		testVFDController();

		return RETURN_REPAINT;
	}
	else if(actionKey == "colorchooser")
	{
		testColorChooser();

		return RETURN_REPAINT;
	}
	else if(actionKey == "keychooser")
	{
		testKeyChooser();

		return RETURN_REPAINT;
	}
	else if(actionKey == "mountchooser")
	{
		testMountChooser();

		return RETURN_REPAINT;
	}
	else if(actionKey == "menuwidget")
	{
		testCMenuWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "pluginslist")
	{
		testPluginsList();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playmoviedir")
	{
		testPlayMovieDir();

		return RETURN_REPAINT;
	}
	else if(actionKey == "playaudiodir")
	{
		testPlayAudioDir();

		return RETURN_REPAINT;
	}
	else if(actionKey == "showpicturedir")
	{
		testShowPictureDir();

		return RETURN_REPAINT;
	}
	else if(actionKey == "menuwidget1")
	{
		testCMenuWidget1();

		return RETURN_REPAINT;
	}
	else if(actionKey == "menuwidget2")
	{
		testCMenuWidget2();

		return RETURN_REPAINT;
	}
	else if(actionKey == "mplay")
	{
		if (menuWidget)
			selected = menuWidget->getSelected();
		else
			selected = 0;

		if (&m_vMovieInfo[selected].file != NULL) 
		{
			CMovieInfoWidget movieInfoWidget;
			movieInfoWidget.setMovie(m_vMovieInfo[selected]);
		
			movieInfoWidget.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "minfo")
	{
		if (menuWidget)
			selected = menuWidget->getSelected();
		else
			selected = 0;

		m_movieInfo.showMovieInfo(m_vMovieInfo[selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "channellist")
	{
		testCChannellist();

		return RETURN_REPAINT;
	}
	else if(actionKey == "bouquetlist")
	{
		testCBouquetlist();

		return RETURN_REPAINT;
	}
	else if(actionKey == "winfo")
	{
		if (rightWidget)
		{
			if (rightWidget->inFocus)
			{
				right_selected = rightWidget->getSelected();
				m_movieInfo.showMovieInfo(m_vMovieInfo[right_selected]);
			}
		}
		else if (windowWidget)
		{
			windowWidget->hide();
			m_movieInfo.showMovieInfo(m_vMovieInfo[0]);
		}
		else if (textBoxWidget)
		{
			textBoxWidget->hide();
			m_movieInfo.showMovieInfo(m_vMovieInfo[0]);
		}
		else if (frameBoxWidget)
		{
			frameBoxWidget->hide();
			m_movieInfo.showMovieInfo(m_vMovieInfo[0]);
		}

		return RETURN_REPAINT;
	}
	else if (actionKey == "wplay")
	{
		if (rightWidget)
		{
			rightWidget->hide();
			
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[rightWidget->getSelected()]);
			tmpMoviePlayerGui.exec(NULL, "");
		}
		else if (windowWidget)
		{
			windowWidget->hide();
			
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[0]);
			tmpMoviePlayerGui.exec(NULL, "");
		}
		else if (textBoxWidget)
		{
			textBoxWidget->hide();
			
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[0]);
			tmpMoviePlayerGui.exec(NULL, "");
		}
		else if (frameBoxWidget)
		{
			frameBoxWidget->hide();
			
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[0]);
			tmpMoviePlayerGui.exec(NULL, "");
		}
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "listframewidget")
	{
		testCListFrameWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "listboxmwidget")
	{
		testClistBoxWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "aok")
	{
		hide();
		
		if(AudioPlaylist.size() > 0)
		{
			for (unsigned int i = 0; i < AudioPlaylist.size(); i++)
			{
				tmpAudioPlayerGui.addToPlaylist(AudioPlaylist[i]);
			}

			tmpAudioPlayerGui.setCurrent(listFrame->getSelectedLine());
			tmpAudioPlayerGui.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if (actionKey == "ainfo")
	{
		hide();
		
		selected = 0;
		
		if (listFrame)
		{
			selected = listFrame->getSelectedLine();
		}
		
		std::string title;
		std::string artist;
		std::string genre;
		std::string date;
		std::string cover;
		char duration[9] = "";

		title = AudioPlaylist[selected].MetaData.title;
		artist = AudioPlaylist[selected].MetaData.artist;
		genre = AudioPlaylist[selected].MetaData.genre;	
		date = AudioPlaylist[selected].MetaData.date;
		cover = AudioPlaylist[selected].MetaData.cover.empty()? DATADIR "/icons/no_coverArt.png" : AudioPlaylist[selected].MetaData.cover;

		snprintf(duration, 8, "(%ld:%02ld)", AudioPlaylist[selected].MetaData.total_time / 60, AudioPlaylist[selected].MetaData.total_time % 60);
		
		std::string buffer;
		
		// title
		if (!title.empty())
		{
			buffer = _("Title: ");
			buffer += title.c_str();
		}
		
		// artist
		if (!artist.empty())
		{
			buffer += "\n\n";
			buffer += _("Artist: ");
			buffer += artist.c_str();
		}
		
		// genre
		if (!genre.empty())
		{
			buffer += "\n\n";
			buffer += _("Genre: ");
			buffer += genre.c_str();
		}
		
		// date
		if (!date.empty())
		{
			buffer += "\n\n";
			buffer += _("Date: ");
			buffer += date.c_str();
		}
		
		// duration
		if (duration)
		{
			buffer += "\n\n";
			buffer += _("Length (Min)");
			buffer += duration;
		}
		
		//
		// infoBox
		CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
		
		CInfoBox * infoBox = new CInfoBox(&position, _("Track Infos"), NEUTRINO_ICON_MP3);

		// scale pic
		int p_w = 0;
		int p_h = 0;

		::scaleImage(cover, &p_w, &p_h);

		infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
		infoBox->setMode(SCROLL);
		infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
		infoBox->setText(buffer.c_str(), cover.c_str(), p_w, p_h);
		infoBox->exec();
		delete infoBox;
	
		return RETURN_REPAINT;
	}
	else if(actionKey == "linfo")
	{
		hide();
		
		selected = rightWidget->getSelected();
		m_movieInfo.showMovieInfo(m_vMovieInfo[selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "lsetup")
	{
		if (rightWidget)
			rightWidget->changeWidgetType();
		else if (menuWidget)
			menuWidget->changeWidgetType();

		return RETURN_NONE;
	}
	else if(actionKey == "exit")
	{
		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "movie_in_cinema")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "movie";
		plist = "now_playing";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_popular")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "movie";
		plist = "popular";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_top_rated")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "movie";
		plist = "top_rated";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie_new")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "movie";
		plist = "upcoming";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "tv_today")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "tv";
		plist = "airing_today";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();
		
		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "tv_on_air")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "tv";
		plist = "on_the_air";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "tv_popular")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "tv";
		plist = "popular";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "tv_top_rated")
	{
		right_selected = 0;
		rightWidget->clearItems();
		//loadTMDBPlaylist("movie", "now_playing", 1);
		TVShows = "tv";
		plist = "top_rated";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "yplay")
	{
		right_selected = rightWidget->getSelected();

		///
		ytparser.Cleanup();

		// setregion
		ytparser.SetRegion("DE");

		// set max result
		ytparser.SetMaxResults(1);
			
		// parse feed
		if (ytparser.ParseFeed(cYTFeedParser::SEARCH_BY_ID, m_vMovieInfo[right_selected].vname, m_vMovieInfo[right_selected].vkey))
		{
			yt_video_list_t &ylist = ytparser.GetVideoList();
	
			for (unsigned int j = 0; j < ylist.size(); j++) 
			{
				m_vMovieInfo[right_selected].ytid = ylist[j].id;
				m_vMovieInfo[right_selected].file.Name = ylist[j].GetUrl();
			}
		} 
			///

		if (&m_vMovieInfo[right_selected].file != NULL) 
		{
			//tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[right_selected]);
			//tmpMoviePlayerGui.exec(NULL, "");
			CMovieInfoWidget movieInfoWidget;
			movieInfoWidget.setMovie(m_vMovieInfo[right_selected]);
		
			movieInfoWidget.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "movie")
	{
		top_selected = frameBoxWidget->getSelected();

		right_selected = 0;
		left_selected = 0;

		rightWidget->clearItems();
		leftWidget->clearItems();

		ClistBoxItem *item1 = new ClistBoxItem("In den Kinos", true, NULL, this, "movie_in_cinema");
		ClistBoxItem *item2 = new ClistBoxItem("Am", true, NULL, this, "movie_popular");
		item2->setOption("populärsten");
		item2->set2lines();
		ClistBoxItem *item3 = new ClistBoxItem("Am besten", true, NULL, this, "movie_top_rated");
		item3->setOption("bewertet");
		item3->set2lines();
		ClistBoxItem *item4 = new ClistBoxItem("Neue Filme", true, NULL, this, "movie_new");
		CMenuSeparator *item5 = new CMenuSeparator();
		CMenuSeparator *item6 = new CMenuSeparator();
		CMenuSeparator *item7 = new CMenuSeparator();
		CMenuSeparator *item8 = new CMenuSeparator();
		ClistBoxItem *item9 = new ClistBoxItem("Beenden", true, NULL, this, "exit");

		leftWidget->addItem(item1);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item2);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item3);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item4);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item5);
		leftWidget->addItem(item6);
		leftWidget->addItem(item7);
		leftWidget->addItem(item8);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item9);
		leftWidget->addItem(new CMenuSeparator(LINE));

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist();
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		leftWidget->setSelected(0);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "tv")
	{
		right_selected = 0;
		left_selected = 0;

		rightWidget->clearItems();
				//
		leftWidget->clearItems();

		ClistBoxItem *item1 = new ClistBoxItem("Heute auf", true, NULL, this, "tv_today");
		item1->setOption("Sendung");
		item1->set2lines();
		ClistBoxItem *item2 = new ClistBoxItem("Auf Sendung", true, NULL, this, "tv_on_air");
		ClistBoxItem *item3 = new ClistBoxItem("Am", true, NULL, this, "tv_popular");
		item3->setOption("populärsten");
		item3->set2lines();
		ClistBoxItem *item4 = new ClistBoxItem("am", true, NULL, this, "tv_top_rated");
		item4->setOption("besten bewertet");
		item4->set2lines();
		CMenuSeparator *item5 = new CMenuSeparator();
		CMenuSeparator *item6 = new CMenuSeparator();
		CMenuSeparator *item7 = new CMenuSeparator();
		CMenuSeparator *item8 = new CMenuSeparator();
		ClistBoxItem *item9 = new ClistBoxItem("Beenden", true, NULL, this, "exit");

		leftWidget->addItem(item1);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item2);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item3);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item4);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item5);
		leftWidget->addItem(item6);
		leftWidget->addItem(item7);
		leftWidget->addItem(item8);
		leftWidget->addItem(new CMenuSeparator(LINE));
		leftWidget->addItem(item9);
		leftWidget->addItem(new CMenuSeparator(LINE));

		//loadTMDBPlaylist("tv", "airing_today", 1);
		TVShows = "tv";
		plist = "airing_today";
		page = 1;

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		leftWidget->setSelected(0);
		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "nextPage")
	{
		top_selected = frameBoxWidget->getSelected();
		page++;
		right_selected = 0;
		rightWidget->clearItems();

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		if(top_selected == 2) // search
		{
			loadTMDBPlaylist("transformers", "", page, true);
		}
		else
			loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
			
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "prevPage")
	{
		top_selected = frameBoxWidget->getSelected();

		page--;
		if(page <= 1)
			page = 1;
		right_selected = 0;
		rightWidget->clearItems();

		CHintBox loadBox("CWidget", _("Scan for Movies ..."));
		loadBox.paint();
		
		if(top_selected == 2) // search
		{
			loadTMDBPlaylist("transformers", "", page, true);
		}
		else
			loadTMDBPlaylist(TVShows.c_str(), plist.c_str(), page);
			
		loadBox.hide();

		// load items
		for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
		{
			item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

			item->setOption(m_vMovieInfo[i].epgChannel.c_str());

			item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

			item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

			item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

			item->set2lines();

			//std::string tmp = m_vMovieInfo[i].epgTitle;
			//tmp += "\n";
			std::string tmp = m_vMovieInfo[i].epgInfo1;
			tmp += "\n";
			tmp += m_vMovieInfo[i].epgInfo2;

			item->setHint(tmp.c_str());

			rightWidget->addItem(item);
		}

		rightWidget->setSelected(0);

		return RETURN_REPAINT;
	}
	else if(actionKey == "search")
	{
		leftWidget->clearItems();

		tmdbsearch.clear();

		CStringInputSMS stringInput(_("Search"), tmdbsearch.c_str());
		stringInput.exec(NULL, "");

		if(!stringInput.getExitPressed())
		{
			rightWidget->clearItems();

			CHintBox loadBox("CWidget", _("Scan for Movies ..."));
			loadBox.paint();
			loadTMDBPlaylist(tmdbsearch.c_str(), "", 1, true);
			loadBox.hide();

			// load items
			for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
			{
				item = new ClistBoxItem(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "yplay");

				item->setOption(m_vMovieInfo[i].epgChannel.c_str());

				item->setInfo1(m_vMovieInfo[i].epgInfo1.c_str());

				item->setInfo2(m_vMovieInfo[i].epgInfo2.c_str());

				item->setHintIcon(file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

				item->set2lines();

				//std::string tmp = m_vMovieInfo[i].epgTitle;
				//tmp += "\n";
				std::string tmp = m_vMovieInfo[i].epgInfo1;
				tmp += "\n";
				tmp += m_vMovieInfo[i].epgInfo2;

				item->setHint(tmp.c_str());

				rightWidget->addItem(item);
			}

			rightWidget->setSelected(0);

			tmdbsearch.clear();
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "multiwidget")
	{
		testMultiWidget();

		return RETURN_REPAINT;
	}	
	else if(actionKey == "singleWidget")
	{
		testCFrameBox1();

		return RETURN_REPAINT;
	}
	else if(actionKey == "firetv")
	{
		testCFrameBoxWidget();

		return RETURN_REPAINT;
	}
	else if(actionKey == "home")
	{
		return RETURN_REPAINT;
	}
	else if(actionKey == "setup")
	{
		CAudioPlayerSettings * audioPlayerSettingsMenu = new CAudioPlayerSettings();

		audioPlayerSettingsMenu->exec(this, "");
		delete audioPlayerSettingsMenu;
		audioPlayerSettingsMenu = NULL;	

		return RETURN_REPAINT;
	}
	else if(actionKey == "help")
	{
		testCHelpBox();

		return RETURN_REPAINT;
	}
	else if(actionKey == "fireplay")
	{
		if (frameBoxWidget)
			selected = frameBoxWidget->getSelected() - 6;
		else
			selected = 0;

		if (&m_vMovieInfo[selected].file != NULL) 
		{
			CMovieInfoWidget movieInfoWidget;
			movieInfoWidget.setMovie(m_vMovieInfo[selected]);
		
			movieInfoWidget.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "fire1play")
	{
		selected = 0;

		if (&m_vMovieInfo[selected].file != NULL) 
		{
			CMovieInfoWidget movieInfoWidget;
			movieInfoWidget.setMovie(m_vMovieInfo[selected]);
		
			movieInfoWidget.exec(NULL, "");
		}

		return RETURN_REPAINT;
	}
	else if(actionKey == "tinfo")
	{
		textBoxWidget->setBigFonts();
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "skin")
	{
		testSkinWidget();
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "skin2")
	{
		testSkinWidget2();
		
		return RETURN_REPAINT;
	}
	else if(actionKey == "skin3")
	{
		testSkinWidget3();
		
		return RETURN_REPAINT;
	}

	showMenu();
	
	return RETURN_REPAINT;
}

//
void CTestMenu::showMenu()
{
	dprintf(DEBUG_NORMAL, "CTestMenu::showMenu:\n");

	CWidget* mWidget = NULL;
	ClistBox* mainMenu = NULL;
	
	mWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
	mWidget->setMenuPosition(MENU_POSITION_CENTER);
		
	mWidget->name = "Test Menu";
		
	mainMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

	mainMenu->enablePaintHead();
	mainMenu->setTitle(_("Test Menu"), NEUTRINO_ICON_BUTTON_SETUP);
	mainMenu->setWidgetMode(MODE_MENU);
	mainMenu->enableShrinkMenu(),
	mainMenu->enablePaintDate();
	mainMenu->enablePaintFoot();
		
	const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
		
	mainMenu->setFootButtons(&btn);
		
	mWidget->addWidgetItem(mainMenu);
	
	mainMenu->clear();
	
	//
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "CComponent"));
	mainMenu->addItem(new CMenuForwarder("CIcon", true, NULL, this, "icon"));
	mainMenu->addItem(new CMenuForwarder("CImage", true, NULL, this, "image"));
	mainMenu->addItem(new CMenuForwarder("CButtons (foot)", true, NULL, this, "buttons"));
	mainMenu->addItem(new CMenuForwarder("CButtons (head)", true, NULL, this, "hbuttons"));
	mainMenu->addItem(new CMenuForwarder("CSpinner", true, NULL, this, "spinner"));
	//mainMenu->addItem(new CMenuForwarder("CProgressBar", true, NULL, this, "progressbar"));
	
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "CWidgetItems"));
	mainMenu->addItem(new CMenuForwarder("CHeaders", true, NULL, this, "headers"));
	mainMenu->addItem(new CMenuForwarder("CFooters", true, NULL, this, "footers"));
	mainMenu->addItem(new CMenuForwarder("CTextBox", true, NULL, this, "textbox"));
	mainMenu->addItem(new CMenuForwarder("CListFrame", true, NULL, this, "listframe"));
	mainMenu->addItem(new CMenuForwarder("CWindow", true, NULL, this, "window"));
	mainMenu->addItem(new CMenuForwarder("CWindow(with shadow)", true, NULL, this, "windowshadow"));
	mainMenu->addItem(new CMenuForwarder("CWindow(shadow|customColor)", true, NULL, this, "windowcustomcolor"));
	mainMenu->addItem(new CMenuForwarder("CWindow (CComponent)", true, NULL, this, "windowcomponent"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(standard)", true, NULL, this, "listbox"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(classic)", true, NULL, this, "listbox2"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(extended)", true, NULL, this, "listbox3"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(Frame)", true, NULL, this, "listbox4"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(DL_INFO)", true, NULL, this, "listbox5"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(DL_HINT)", true, NULL, this, "listbox6"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(DL_HINTITEM)", true, NULL, this, "listbox7"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(DL_HINTICON)", true, NULL, this, "listbox8"));
	mainMenu->addItem(new CMenuForwarder("ClistBox(DL_HINTHINT)", true, NULL, this, "listbox9"));
	mainMenu->addItem(new CMenuForwarder("CFrameBox", true, NULL, this, "framebox"));
	mainMenu->addItem(new CMenuForwarder("CFrameBox", true, NULL, this, "singleWidget"));
	
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "CWidget"));
	mainMenu->addItem(new CMenuForwarder("CWidget(ClistFrame|CHead|CFoot)", true, NULL, this, "listframewidget"));
	mainMenu->addItem(new CMenuForwarder("CWidget(CWindow|CHead|CFoot)", true, NULL, this, "ccwindow"));
	mainMenu->addItem(new CMenuForwarder("CWidget(CTextBox)", true, NULL, this, "textboxwidget"));
	mainMenu->addItem(new CMenuForwarder("CWidget(ClistBox)", true, NULL, this, "listboxmwidget"));
	mainMenu->addItem(new CMenuForwarder("CWidget(CFrameBox)", true, NULL, this, "firetv"));
	mainMenu->addItem(new CMenuForwarder("CWidget(ClistBox|CWindow|CHead|CFoot)", true, NULL, this, "multiwidget"));
	mainMenu->addItem(new CMenuForwarder("CWidget(ClistBox|CFrameBox|CHead|CFoot)", true, NULL, this, "widget"));
	
	// CMenuWidhet
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "CMenuWidget"));
	mainMenu->addItem(new CMenuForwarder("CMenuWidget(MODE_LISTBOX)", true, NULL, this, "menuwidget"));
	mainMenu->addItem(new CMenuForwarder("CMenuWidget(MODE_MENU)", true, NULL, this, "menuwidget1"));
	mainMenu->addItem(new CMenuForwarder("CMenuWidget(MODE_SETUP)", true, NULL, this, "menuwidget2"));

	// other widget
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "other Widget"));
	mainMenu->addItem(new CMenuForwarder("CStringInput", true, NULL, this, "stringinput"));
	mainMenu->addItem(new CMenuForwarder("CStringInputSMS", true, NULL, this, "stringinputsms"));
	mainMenu->addItem(new CMenuForwarder("CPINInput", true, NULL, this, "pininput"));
	mainMenu->addItem(new CMenuForwarder("CPLPINInput", true, NULL, this, "plpininput"));
	mainMenu->addItem(new CMenuForwarder("CPINChangeWidget", true, NULL, this, "pinchangewidget"));
	mainMenu->addItem(new CMenuForwarder("CIPInput", true, NULL, this, "ipinput"));
	mainMenu->addItem(new CMenuForwarder("CMACInput", true, NULL, this, "macinput"));
	//mainMenu->addItem(new CMenuForwarder("CDateInput", true, NULL, this, "dateinput"));
	//mainMenu->addItem(new CMenuForwarder("CTimeInput", true, NULL, this, "timeinput"));
	//mainMenu->addItem(new CMenuForwarder("CIntInput", true, NULL, this, "intinput"));
	//
	mainMenu->addItem(new CMenuSeparator(LINE));
	mainMenu->addItem(new CMenuForwarder("ColorChooser", true, NULL, this, "colorchooser"));
	mainMenu->addItem(new CMenuForwarder("KeyChooserItem", true, NULL, this, "keychooser"));
	mainMenu->addItem(new CMenuForwarder("VFDController", true, NULL, this, "vfdcontroller"));
	mainMenu->addItem(new CMenuForwarder("MountChooser", true, NULL, this, "mountchooser"));
	
	//
	mainMenu->addItem(new CMenuSeparator(LINE));
	mainMenu->addItem(new CMenuForwarder("ChannelSelectWidget", true, NULL, this, "channelselect"));
	mainMenu->addItem(new CMenuForwarder("BEWidget", true, NULL, this, "bewidget"));
	mainMenu->addItem(new CMenuForwarder("AudioVideoSelectWidget", true, NULL, this, "avselect"));
	mainMenu->addItem(new CMenuForwarder("AudioSelectWidget", true, NULL, this, "aselect"));
	mainMenu->addItem(new CMenuForwarder("DVBSubSelectWidget", true, NULL, this, "dvbsubselect"));
	mainMenu->addItem(new CMenuForwarder("AlphaSetup", true, NULL, this, "alphasetup"));
	mainMenu->addItem(new CMenuForwarder("PSISetup", true, NULL, this, "psisetup"));
	mainMenu->addItem(new CMenuForwarder("RCLock", true, NULL, this, "rclock"));
	mainMenu->addItem(new CMenuForwarder("SleepTimerWidget", true, NULL, this, "sleeptimer"));
	mainMenu->addItem(new CMenuForwarder("MountGUI", true, NULL, this, "mountgui"));
	//mainMenu->addItem(new CMenuForwarder("UmountGUI", true, NULL, this, "umountgui"));
	mainMenu->addItem(new CMenuForwarder("MountSmallMenu", true, NULL, this, "mountsmallmenu"));
	mainMenu->addItem(new CMenuForwarder("PluginsList", true, NULL, this, "pluginslist"));
	
	//
	mainMenu->addItem(new CMenuSeparator(LINE));
	mainMenu->addItem(new CMenuForwarder("CInfoBox", true, NULL, this, "infobox"));
	mainMenu->addItem(new CMenuForwarder("CMessageBox", true, NULL, this, "messagebox"));
	mainMenu->addItem(new CMenuForwarder("CMessageBoxInfoMsg", true, NULL, this, "messageboxinfomsg"));
	mainMenu->addItem(new CMenuForwarder("CMessageBoxErrorMsg", true, NULL, this, "messageboxerrormsg"));
	mainMenu->addItem(new CMenuForwarder("CHintBox", true, NULL, this, "hintbox"));
	mainMenu->addItem(new CMenuForwarder("CHintBoxInfo", true, NULL, this, "hintboxinfo"));
	mainMenu->addItem(new CMenuForwarder("CHelpBox", true, NULL, this, "helpbox"));
	mainMenu->addItem(new CMenuForwarder("CProgressWindow", true, NULL, this, "progresswindow"));

	//
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "Players"));
	mainMenu->addItem(new CMenuForwarder("PlayMovieURL", true, NULL, this, "playmovieurl"));
	mainMenu->addItem(new CMenuForwarder("PlayAudioURL", true, NULL, this, "playaudiourl"));
	mainMenu->addItem(new CMenuForwarder("ShowPictureURL", true, NULL, this, "showpictureurl"));

	//
	mainMenu->addItem(new CMenuSeparator(LINE));
	mainMenu->addItem(new CMenuForwarder("PlayMovieFolder", true, NULL, this, "playmoviefolder"));
	mainMenu->addItem(new CMenuForwarder("PlayAudioFolder", true, NULL, this, "playaudiofolder"));
	mainMenu->addItem(new CMenuForwarder("ShowPictureFolder", true, NULL, this, "showpicturefolder"));

	//
	mainMenu->addItem(new CMenuSeparator(LINE));
	mainMenu->addItem(new CMenuForwarder("PlayMovieDir(without Browser)", true, NULL, this, "playmoviedir"));
	mainMenu->addItem(new CMenuForwarder("PlayAudioDir(without Browser)", true, NULL, this, "playaudiodir"));
	mainMenu->addItem(new CMenuForwarder("ShowPictureDir(without Browser)", true, NULL, this, "showpicturedir"));

	//
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "Plugins") );
	mainMenu->addItem(new CMenuForwarder("StartPlugin(e.g: youtube)", true, NULL, this, "startplugin"));

	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "EPG") );
	mainMenu->addItem(new CMenuForwarder("ShowActuellEPG", true, NULL, this, "showepg"));

	//
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "Channellist") );
	mainMenu->addItem(new CMenuForwarder("CChannelList:", true, NULL, this, "channellist"));
	mainMenu->addItem(new CMenuForwarder("CBouquetList:", true, NULL, this, "bouquetlist"));
	
	//
	mainMenu->addItem(new CMenuSeparator(LINE | STRING, "SKIN") );		
	mainMenu->addItem(new CMenuForwarder("SKIN-WIDGET", true, NULL, this, "skin"));
	mainMenu->addItem(new CMenuForwarder("SKIN-WIDGET2", true, NULL, this, "skin2"));
	mainMenu->addItem(new CMenuForwarder("SKIN-WIDGET3", true, NULL, this, "skin3"));
	
	mWidget->exec(NULL, "");
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CTestMenu *testMenu = new CTestMenu();
	
	testMenu->exec(NULL, "");
	
	delete testMenu;
	testMenu = NULL;
}


