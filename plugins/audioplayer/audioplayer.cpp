/*
  $Id: audioplayer.cpp 2018/07/10 mohousch Exp $

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
#include <fstream>


//
extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

//// defines
//FIXME: make this global
#define __(string) dgettext("audioplayer", string)

//
#define SHOW_FILE_LOAD_LIMIT 50

//// globals
const long int GET_PLAYLIST_TIMEOUT = 10;

class CMP3Player : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;

		CWidget *widget;
		ClistBox *alist;
		CMenuItem *item;

		CAudioPlayerGui tmpAudioPlayerGui;
		CFileFilter fileFilter;
		CFileList filelist;
		CAudioPlayList playlist;
		std::string Path;
		int selected;

		void loadPlaylist();
		void openFileBrowser();
		void showTrackInfo(CAudiofile& file);
		
		//
		void addUrl2Playlist(const char *url, const char *name = NULL, const time_t bitrate = 0);
		void processPlaylistUrl(const char *url, const char *name = NULL, const time_t bitrate = 0);
		void getMetaData(CAudiofile& File);

		int showMenu();
		
	public:
		CMP3Player();
		~CMP3Player();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CMP3Player::CMP3Player()
{
	frameBuffer = CFrameBuffer::getInstance();

	widget = NULL;
	alist = NULL;
	item = NULL;

	selected = 0;

	fileFilter.addFilter("cdr");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("m2a");
	fileFilter.addFilter("mpa");
	fileFilter.addFilter("mp2");
//	fileFilter.addFilter("wav"); // player segfault
	fileFilter.addFilter("flac");
	fileFilter.addFilter("aac");
	fileFilter.addFilter("dts");
	fileFilter.addFilter("m4a");
	
	//
	fileFilter.addFilter("url");
	fileFilter.addFilter("m3u");
	fileFilter.addFilter("m3u8");
	fileFilter.addFilter("pls");

	CAudioPlayer::getInstance()->init();
}

CMP3Player::~CMP3Player()
{
	playlist.clear();

	if(CAudioPlayer::getInstance()->getState() != CAudioPlayer::STOP)
		CAudioPlayer::getInstance()->stop();
		
	delete widget;
	widget = NULL;
}

void CMP3Player::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CMP3Player::loadPlaylist()
{
	dprintf(DEBUG_NORMAL, "CMP3Player::loadPlaylist\n");
	
	playlist.clear();

	Path = g_settings.network_nfs_audioplayerdir;

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

				getMetaData(audiofile);
				
				playlist.push_back(audiofile);
			}
		}
	}
}

//
void CMP3Player::addUrl2Playlist(const char *url, const char *name, const time_t bitrate) 
{
	dprintf(DEBUG_NORMAL, "CMP3Player::addUrl2Playlist: name = %s, url = %s\n", name, url);
	
	CAudiofile mp3(url, CFile::EXTENSION_URL);
	
	getMetaData(mp3);
	
	if (name != NULL) 
	{
		mp3.MetaData.title = name;
	} 
	else 
	{
		std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/') + 1);
		mp3.MetaData.title = tmp;
	}
	
	if (bitrate)
		mp3.MetaData.total_time = bitrate;
	else
		mp3.MetaData.total_time = 0;

	if (url[0] != '#') 
	{
		playlist.push_back(mp3);
	}
}

void CMP3Player::processPlaylistUrl(const char *url, const char *name, const time_t tim) 
{
	dprintf(DEBUG_NORMAL, "CMP3Player::processPlaylistUrl\n");
	
	CURL *curl_handle;
	struct MemoryStruct chunk;
	
	chunk.memory = NULL; 	// we expect realloc(NULL, size) to work
	chunk.size = 0;    	// no data at this point

	curl_global_init(CURL_GLOBAL_ALL);

	// init the curl session
	curl_handle = curl_easy_init();

	// specify URL to get
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	// send all data to this function
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	// we pass our 'chunk' struct to the callback function
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	// some servers don't like requests that are made without a user-agent field, so we provide one
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	// don't use signal for timeout
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, (long)1);

	// set timeout to 10 seconds
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, GET_PLAYLIST_TIMEOUT);
	
	if(strcmp(g_settings.softupdate_proxyserver, "") != 0)
	{
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, g_settings.softupdate_proxyserver);
		
		if(strcmp(g_settings.softupdate_proxyusername, "") != 0)
		{
			char tmp[200];
			strcpy(tmp, g_settings.softupdate_proxyusername);
			strcat(tmp, ":");
			strcat(tmp, g_settings.softupdate_proxypassword);
			curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, tmp);
		}
	}

	// get it! 
	curl_easy_perform(curl_handle);

	// cleanup curl stuff
	curl_easy_cleanup(curl_handle);

	long res_code;
	if (curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &res_code ) ==  CURLE_OK) 
	{
		if (200 == res_code) 
		{
			//printf("\nchunk = %s\n", chunk.memory);
			std::istringstream iss;
			iss.str (std::string(chunk.memory, chunk.size));
			char line[512];
			char *ptr;
			
			while (iss.rdstate() == std::ifstream::goodbit) 
			{
				iss.getline(line, 512);
				if (line[0] != '#') 
				{
					//printf("chunk: line = %s\n", line);
					ptr = strstr(line, "http://");
					if (ptr != NULL) 
					{
						char *tmp;
						// strip \n and \r characters from url
						tmp = strchr(line, '\r');
						if (tmp != NULL)
							*tmp = '\0';
						tmp = strchr(line, '\n');
						if (tmp != NULL)
							*tmp = '\0';
						
						addUrl2Playlist(ptr, name, tim);
					}
				}
			}
		}
	}

	if(chunk.memory)
		free(chunk.memory);
 
	// we're done with libcurl, so clean it up
	curl_global_cleanup();
}

void CMP3Player::getMetaData(CAudiofile &File)
{
	dprintf(DEBUG_INFO, "CMP3Player::GetMetaData:\n");
	
	bool ret = 1;

	ret = CAudioPlayer::getInstance()->readMetaData(&File, true);

	if (!ret || (File.MetaData.artist.empty() && File.MetaData.title.empty() ))
	{
		//Set from Filename
		std::string tmp = File.Filename.substr(File.Filename.rfind('/') + 1);
		tmp = tmp.substr(0, tmp.length() - 4);	//remove extension (.mp3)
		std::string::size_type i = tmp.rfind(" - ");
		
		if(i != std::string::npos)
		{ 
			// Trennzeichen " - " gefunden
			File.MetaData.artist = tmp.substr(0, i);
			File.MetaData.title = tmp.substr(i + 3);
		}
		else
		{
			i = tmp.rfind('-');
			if(i != std::string::npos)
			{ //Trennzeichen "-"
				File.MetaData.artist = tmp.substr(0, i);
				File.MetaData.title = tmp.substr(i + 1);
			}
			else
				File.MetaData.title = tmp;
		}
		
		File.MetaData.artist = FILESYSTEM_ENCODING_TO_UTF8(std::string(File.MetaData.artist).c_str());
		File.MetaData.title  = FILESYSTEM_ENCODING_TO_UTF8(std::string(File.MetaData.title).c_str());
	}
}

void CMP3Player::openFileBrowser()
{
	dprintf(DEBUG_NORMAL, "CMP3Player::openFileBrowser\n");
	
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR);
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_picturedir : "");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	if (filebrowser.exec(Path.c_str()))
	{
		////
		CProgressWindow progress;
		long maxProgress = (filebrowser.getSelectedFiles().size() > 1) ? filebrowser.getSelectedFiles().size() - 1 : 1;
		long currentProgress = -1;
		
		if (maxProgress > SHOW_FILE_LOAD_LIMIT)
		{
			progress.setTitle(__("Receiving list, please wait..."));	
			progress.paint();
		}
		
		Path = filebrowser.getCurrentDir();
		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for(; files != filebrowser.getSelectedFiles().end(); files++)
		{
			////
			if (maxProgress > SHOW_FILE_LOAD_LIMIT)
			{
				currentProgress++;
				// show status
				int global = 100*currentProgress/maxProgress;
				progress.showGlobalStatus(global);
				progress.showStatusMessageUTF(files->Name);
				
				CLCD::getInstance()->showProgressBar(global, "Receiving list, please wait...");
			}
			
			if ( (files->getExtension() == CFile::EXTENSION_CDR)
					||  (files->getExtension() == CFile::EXTENSION_MP3)
					||  (files->getExtension() == CFile::EXTENSION_WAV)
					||  (files->getExtension() == CFile::EXTENSION_FLAC))
			{
				CAudiofile audiofile(files->Name, files->getExtension());

				// skip duplicate
				for (unsigned long i = 0; i < (unsigned long)playlist.size(); i++)
				{
					if(playlist[i].Filename == audiofile.Filename)
						playlist.erase(playlist.begin() + i); 
				}

				//
				getMetaData(audiofile);
		
				playlist.push_back(audiofile);
			}
			else if(files->getType() == CFile::FILE_URL)
			{
				std::string filename = files->Name;
				FILE *fd = fopen(filename.c_str(), "r");
				
				if(fd)
				{
					char buf[512];
					unsigned int len = fread(buf, sizeof(char), 512, fd);
					fclose(fd);

					if (len && (strstr(buf, ".m3u") || strstr(buf, ".m3u8") || strstr(buf, ".pls")))
					{
						dprintf(DEBUG_NORMAL, "CMP3Player::openFilebrowser: m3u/pls Playlist found: %s\n", buf);
						
						filename = buf;
						processPlaylistUrl(files->Name.c_str());
					}
					else
					{
						addUrl2Playlist(filename.c_str());
					}
				}
			}
			else if(files->getType() == CFile::FILE_PLAYLIST)
			{
				std::string sPath = files->Name.substr(0, files->Name.rfind('/'));
				std::ifstream infile;
				char cLine[1024];
				char name[1024] = { 0 };
				int duration;
				
				infile.open(files->Name.c_str(), std::ifstream::in);

				while (infile.good())
				{
					infile.getline(cLine, sizeof(cLine));
					
					// remove CR
					if(cLine[strlen(cLine) - 1] == '\r')
						cLine[strlen(cLine) - 1] = 0;
					
					sscanf(cLine, "#EXTINF:%d,%[^\n]\n", &duration, name);
					
					if(strlen(cLine) > 0 && cLine[0] != '#')
					{
						char *url = strstr(cLine, "http://");
						
						//
						if (url != NULL) 
						{
							if (strstr(url, ".m3u") || strstr(url, ".m3u8") || strstr(url, ".pls"))
								processPlaylistUrl(url);
							else
								addUrl2Playlist(url, name, duration);
						} 
						else if ((url = strstr(cLine, "icy://")) != NULL) 
						{
							addUrl2Playlist(url);
						} 
						else if ((url = strstr(cLine, "scast:://")) != NULL) 
						{
							addUrl2Playlist(url);
						}
						else
						{
							std::string filename = sPath + '/' + cLine;

							std::string::size_type pos;
							while((pos = filename.find('\\')) != std::string::npos)
								filename[pos] = '/';

							std::ifstream testfile;
							testfile.open(filename.c_str(), std::ifstream::in);
							
							if(testfile.good())
							{
								//
								if(strcasecmp(filename.substr(filename.length() - 3, 3).c_str(), "url") == 0)
								{
									addUrl2Playlist(filename.c_str());
								}
								else
								{
									CFile playlistItem;
									playlistItem.Name = filename;
									CFile::FileExtension fileExtension = playlistItem.getExtension();
									
									if (fileExtension == CFile::EXTENSION_CDR
											|| fileExtension == CFile::EXTENSION_MP3
											|| fileExtension == CFile::EXTENSION_WAV
											|| fileExtension == CFile::EXTENSION_FLAC
									)
									{
										CAudiofile audioFile(filename, fileExtension);
										getMetaData(audioFile);
										playlist.push_back(audioFile);
									} 
									else
									{
										dprintf(DEBUG_NORMAL, "CMP3Player::openFilebrowser: file type (%d) is *not* supported in playlists\n(%s)\n", fileExtension, filename.c_str());
									}
								}
							}
							testfile.close();
						}
					}
				}
				infile.close();
			}
		}
		
		usleep(1000000);
		progress.hide();
	}
	
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

//
void CMP3Player::showTrackInfo(CAudiofile& file)
{
	dprintf(DEBUG_NORMAL, "CMP3Player::showTrackInfo\n");
	
	if (playlist.empty())
		return;
	
	std::string title;
	std::string artist;
	std::string genre;
	std::string date;
	std::string cover;
	char duration[9] = "";

	title = file.MetaData.title;
	artist = file.MetaData.artist;
	genre = file.MetaData.genre;	
	date = file.MetaData.date;
	cover = file.MetaData.cover.empty()? DATADIR "/icons/no_coverArt.png" : file.MetaData.cover;

	snprintf(duration, 8, "%ld:%02ld", file.MetaData.total_time / 60, file.MetaData.total_time % 60);
	
	std::string buffer;
	
	// title
	if (!title.empty())
	{
		buffer = __("Title: ");
		buffer += title.c_str();
	}
	
	// artist
	if (!artist.empty())
	{
		buffer += "\n\n";
		buffer += __("Artist: ");
		buffer += artist.c_str();
	}
	
	// genre
	if (!genre.empty())
	{
		buffer += "\n\n";
		buffer += __("Genre: ");
		buffer += genre.c_str();
	}
	
	// date
	if (!date.empty())
	{
		buffer += "\n\n";
		buffer += __("Date: ");
		buffer += date.c_str();
	}
	
	// duration
	if (duration)
	{
		buffer += "\n\n";
		buffer += __("Length (Min): ");
		buffer += duration;
	}
	
	// infoBox
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, __("Track Infos"), NEUTRINO_ICON_MP3);

	// scale pic
	int p_w = 0;
	int p_h = 0;

	::scaleImage(cover, &p_w, &p_h);

	infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	infoBox->setText(buffer.c_str(), cover.c_str(), p_w, p_h);
	infoBox->exec();
	delete infoBox;
}

#define HEAD_BUTTONS_COUNT 2
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_SETUP, "" },
	{ NEUTRINO_ICON_BUTTON_HELP, "" }
};

#define FOOT_BUTTONS_COUNT 4
const struct button_label AudioPlayerButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, __("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN, __("Add") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, __("Delete all") },
	{ NEUTRINO_ICON_BUTTON_BLUE, __("Reload") }
};

int CMP3Player::showMenu()
{
	int res = RETURN_REPAINT;
		
	widget = new CWidget(frameBuffer->getScreenX() + 20, frameBuffer->getScreenY() + 20, frameBuffer->getScreenWidth() - 40, frameBuffer->getScreenHeight() - 40);
	alist = new ClistBox(frameBuffer->getScreenX() + 20, frameBuffer->getScreenY() + 20, frameBuffer->getScreenWidth() - 40, frameBuffer->getScreenHeight() - 40);

	for(unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
	{
		//
		std::string title;
		std::string artist;
		std::string genre;
		std::string date;
		char duration[9] = "";
		
		std::string desc;

		title = playlist[i].MetaData.title;
		artist = playlist[i].MetaData.artist;
		genre = playlist[i].MetaData.genre;	
		date = playlist[i].MetaData.date;
		std::string cover = playlist[i].MetaData.cover.empty()? DATADIR "/icons/no_coverArt.png" : playlist[i].MetaData.cover;

		snprintf(duration, 8, "(%ld:%02ld)", playlist[i].MetaData.total_time / 60, playlist[i].MetaData.total_time % 60);
			
		desc = artist.c_str();
			
		if (!genre.empty())
		{
			desc += "   ";
			desc += genre.c_str();
		}
			
		if (!date.empty())
		{
			desc += "   (";
			desc += date.c_str();
			desc += ")";
		}

		//
		item = new CMenuForwarder(title.c_str(), true, desc.c_str(), this, "aplay");
			
		item->setOptionInfo(duration);
		item->setNumber(i + 1);
		
		//
		item->setBorderMode();
		item->set2lines(true);

		alist->addItem(item);
	}
	
	alist->setWidgetMode(ClistBox::MODE_LISTBOX);

	//
	alist->enablePaintHead();
	alist->setTitle(__("Audio Player"), NEUTRINO_ICON_MP3);
	alist->enablePaintDate();
	alist->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	
	//
	alist->enablePaintFoot();
	alist->setFootButtons(AudioPlayerButtons, FOOT_BUTTONS_COUNT);

	//
	widget->addKey(CRCInput::RC_setup, this, CRCInput::getSpecialKeyName(CRCInput::RC_setup));
	widget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	widget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	widget->addKey(CRCInput::RC_yellow, this, CRCInput::getSpecialKeyName(CRCInput::RC_yellow));
	widget->addKey(CRCInput::RC_blue, this, CRCInput::getSpecialKeyName(CRCInput::RC_blue));
	widget->addKey(CRCInput::RC_info, this, CRCInput::getSpecialKeyName(CRCInput::RC_info));
	widget->addKey(CRCInput::RC_help, this, CRCInput::getSpecialKeyName(CRCInput::RC_help));

	widget->addCCItem(alist);
	
	res = widget->exec(NULL, "");
	
	return res;
}

int CMP3Player::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMP3Player::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = RETURN_REPAINT;
	
	if(parent)
		hide();

	if(alist != NULL)
		selected = alist->getSelected();

	if(actionKey == "aplay")
	{
		for (unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
		{
			tmpAudioPlayerGui.addToPlaylist(playlist[i]);
		}

		tmpAudioPlayerGui.setCurrent(selected);
		tmpAudioPlayerGui.exec(NULL, "");

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_setup")
	{
		CAudioPlayerSettings * audioPlayerSettingsMenu = new CAudioPlayerSettings();
		audioPlayerSettingsMenu->exec(this, "");
		delete audioPlayerSettingsMenu;
		audioPlayerSettingsMenu = NULL;	

		return RETURN_REPAINT;					
	}
	else if(actionKey == "RC_red")
	{
		CAudioPlayList::iterator p = playlist.begin() + alist->getSelected();
		playlist.erase(p);

		if (selected >= (int)playlist.size())
			selected = playlist.size() - 1;

		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		openFileBrowser();
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_yellow")
	{
		playlist.clear();
		selected = 0;
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_blue")
	{
		playlist.clear();
		loadPlaylist();
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_info")
	{
		selected = alist? alist->getSelected() : 0;
		showTrackInfo(playlist[selected]);

		return RETURN_REPAINT;
	}
	else if (actionKey == "RC_help")
	{
		tmpAudioPlayerGui.showHelp();
		
		return RETURN_REPAINT;
	}

	//
	loadPlaylist();
	res = showMenu();
	
	return res;
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CMP3Player* audioPlayerHandler = new CMP3Player();
	
	audioPlayerHandler->exec(NULL, "");
	
	delete audioPlayerHandler;
	audioPlayerHandler = NULL;
}


