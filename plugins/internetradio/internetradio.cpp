/*
  $Id: internetradio.cpp 2018/07/10 mohousch Exp $

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

#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <iostream>

#include <string>

# include <plugin.h>


//
extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);


//// defines
//FIXME: make this global
#undef _
#define _(string) dgettext("internetradio", string)
//
#define SHOW_FILE_LOAD_LIMIT 50
#define AUDIOPLAYER_CHECK_FOR_DUPLICATES
#define RADIO_STATION_XML_FILE PLUGINDIR "/internetradio/radio-stations.xml"

//// globals
const long int GET_PLAYLIST_TIMEOUT = 10;

class CInternetRadio : public CTarget
{
	private:
		CFrameBuffer* frameBuffer;

		CWidget *widget;
		ClistBox *ilist;
		CMenuItem *item;

		CAudioPlayerGui tmpAudioPlayerGui;
		CFileFilter fileFilter;
		CFileList filelist;
		CAudioPlayList playlist;
		std::string m_Path;
		int selected;

		////
		void loadPlaylist(void);
		bool shufflePlaylist(void);
		void addUrl2Playlist(const char *url, const char *name = NULL, const time_t bitrate = 0);
		void processPlaylistUrl(const char *url, const char *name = NULL, const time_t bitrate = 0);
		void scanXmlFile(std::string filename);
		void scanXmlData(xmlDocPtr answer_parser, const char *nametag, const char *urltag, const char *bitratetag = NULL, bool usechild = false);
		void GetMetaData(CAudiofile& File);
		void getFileInfoToDisplay(std::string& fileInfo, CAudiofile& file);
		void openFileBrowser(void);

		////
		int showMenu();
		
	public:
		CInternetRadio();
		~CInternetRadio();
		int exec(CTarget* parent, const std::string& actionKey);
		void hide();
};

CInternetRadio::CInternetRadio()
{
	frameBuffer = CFrameBuffer::getInstance();

	widget = NULL;
	ilist = NULL;
	item = NULL;

	selected = 0;


	fileFilter.addFilter("url");
	fileFilter.addFilter("xml");
	fileFilter.addFilter("m3u");
	fileFilter.addFilter("m3u8");
	fileFilter.addFilter("pls");

	CAudioPlayer::getInstance()->init();
}

CInternetRadio::~CInternetRadio()
{
	playlist.clear();
	
	delete widget;
	widget = NULL;
}

void CInternetRadio::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

bool CInternetRadio::shufflePlaylist(void)
{
	dprintf(DEBUG_NORMAL, "CInternetRadio::shufflePlaylist\n");
	
	RandomNumber rnd;
	bool result = false;
	
	if (!(playlist.empty()))
	{
		if (selected > 0)
		{
			std::swap(playlist[0], playlist[selected]);
			selected = 0;
		}

		std::random_shuffle((selected != 0) ? playlist.begin() : playlist.begin() + 1, playlist.end(), rnd);

		selected = 0;

		result = true;
	}
	
	return(result);
}

void CInternetRadio::addUrl2Playlist(const char *url, const char *name, const time_t bitrate) 
{
	dprintf(DEBUG_NORMAL, "CInternetRadio::addUrl2Playlist: name = %s, url = %s\n", name, url);
	
	CAudiofile mp3(url, CFile::EXTENSION_URL);
	
	if (name != NULL) 
	{
		mp3.MetaData.title = name;
	} 
	else 
	{
		std::string tmp = mp3.Filename.substr(mp3.Filename.rfind('/')+1);
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

void CInternetRadio::processPlaylistUrl(const char *url, const char *name, const time_t tim) 
{
	dprintf(DEBUG_INFO, "CInternetRadio::processPlaylistUrl\n");
	
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

void CInternetRadio::scanXmlFile(std::string filename)
{
	dprintf(DEBUG_NORMAL, "CInternetRadio::scanXmlFile: %s\n", filename.c_str());

	xmlDocPtr answer_parser = parseXmlFile(filename.c_str());

	scanXmlData(answer_parser, "name", "url");
}

void CInternetRadio::scanXmlData(xmlDocPtr answer_parser, const char *nametag, const char *urltag, const char *bitratetag, bool usechild)
{
	dprintf(DEBUG_NORMAL, "CInternetRadio::scanXmlData\n");
	
#define IC_typetag "server_type"

	if (answer_parser != NULL) 
	{
		xmlNodePtr element = xmlDocGetRootElement(answer_parser);
		element = element->xmlChildrenNode;
		xmlNodePtr element_tmp = element;
		
		if (element == NULL) 
		{
			dprintf(DEBUG_NORMAL, "CInternetRadio::scanXmlData: No valid XML File.\n");
		} 
		else 
		{
			CProgressWindow progress;
			long maxProgress = 1;
			int global = 0;
			
			// count # of all entries
			while (element) 
			{
				maxProgress++;
				element = element->xmlNextNode;
			}
			
			element = element_tmp;
			long listPos = 1;
			
			progress.setTitle(_("Receiving list, please wait"));
			progress.enableCancelIcon();
			progress.paint();
			frameBuffer->blit();
			
			neutrino_msg_t      msg;
			neutrino_msg_data_t data;
			
			g_RCInput->getMsg(&msg, &data, 0);
			
			while (element && msg != CRCInput::RC_home) 
			{
				char *ptr = NULL;
				char *name = NULL;
				char *url = NULL;
				char *type = NULL;
				time_t bitrate = 0;
				bool skip = true;
				listPos++;
				
				// show status
				global = 100*listPos / maxProgress;
				progress.showGlobalStatus(global);

				if (usechild) 
				{
					xmlNodePtr child = element->xmlChildrenNode;
					while (child) 
					{
						if (strcmp(xmlGetName(child), nametag) == 0)
							name = xmlGetData(child);
						else if (strcmp(xmlGetName(child), urltag) == 0)
							url = xmlGetData(child);
						else if (strcmp(xmlGetName(child), IC_typetag) == 0)
							type = xmlGetData(child);
						else if (bitratetag && strcmp(xmlGetName(child), bitratetag) == 0) {
							ptr = xmlGetData(child);
							if (ptr) 
								bitrate = atoi(ptr);
						}
						child = child->xmlNextNode;
					}
					if 	(strcmp("audio/mpeg", type) == 0) 	skip = false;
					else if (strcmp("mp3", type) == 0) 		skip = false;
					else if (strcmp("application/mp3", type) == 0) 	skip = false;
				} 
				else 
				{
					url = xmlGetAttribute(element, (char *) urltag);
					name = xmlGetAttribute(element, (char *) nametag);
					
					if (bitratetag) 
					{
						ptr = xmlGetAttribute(element, (char *) bitratetag);
						if (ptr)
							bitrate = atoi(ptr);
					}
					skip = false;
				}

				if ((url != NULL) && !skip) 
				{
					progress.showStatusMessageUTF(url);
					
					if (strstr(url, ".m3u") || strstr(url, ".pls"))
						processPlaylistUrl(url, name);
					else 
						addUrl2Playlist(url, name, bitrate);
				}
				element = element->xmlNextNode;
				g_RCInput->getMsg(&msg, &data, 0);
				
				frameBuffer->blit();

			}

			usleep(1000000);
			progress.hide();
			frameBuffer->blit();
		}

		xmlFreeDoc(answer_parser);
	}
}

void CInternetRadio::openFileBrowser(void)
{
	dprintf(DEBUG_NORMAL, "CMP3Player::openFileBrowser\n");
	
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR);
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_picturedir : "");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;
	
	m_Path = g_settings.network_nfs_audioplayerdir;

	if (filebrowser.exec(m_Path.c_str()))
	{
		////
		CProgressWindow progress;
		long maxProgress = (filebrowser.getSelectedFiles().size() > 1) ? filebrowser.getSelectedFiles().size() - 1 : 1;
		long currentProgress = -1;
		
		if (maxProgress > SHOW_FILE_LOAD_LIMIT)
		{
			progress.setTitle(_("Receiving list, please wait..."));	
			progress.paint();
		}
		
		m_Path = filebrowser.getCurrentDir();
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
			
			if(files->getType() == CFile::FILE_URL)
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
										GetMetaData(audioFile);
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

void CInternetRadio::GetMetaData(CAudiofile& File)
{
	dprintf(DEBUG_DEBUG, "CInternetRadio::GetMetaData: fileExtension:%d\n", File.FileExtension);
	
	bool ret = 1;

	if (CFile::EXTENSION_URL != File.FileExtension)
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

void CInternetRadio::getFileInfoToDisplay(std::string &info, CAudiofile& file)
{
	std::string fileInfo;
	std::string artist;
	std::string title;

	if (!file.MetaData.bitrate)
	{
		GetMetaData(file);
	}

	if (!file.MetaData.artist.empty())
		artist = file.MetaData.artist;

	if (!file.MetaData.title.empty())
		title = file.MetaData.title;

	fileInfo += title;
	if (!title.empty() && !artist.empty()) 
		fileInfo += ", ";
		
	fileInfo += artist;

	if (!file.MetaData.album.empty())
	{
		fileInfo += " (";
		fileInfo += file.MetaData.album;
		fileInfo += ')';
	} 
	
	if (fileInfo.empty())
	{
		fileInfo += "Unknown";
	}
	
	info += fileInfo;
}

void CInternetRadio::loadPlaylist(void)
{
	playlist.clear();
	
	scanXmlFile(RADIO_STATION_XML_FILE);
}

const struct button_label HeadButtons{ NEUTRINO_ICON_BUTTON_SETUP, "" };

#define FOOT_BUTTONS_COUNT 4
const struct button_label AudioPlayerButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Add") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Delete all") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Reload") }
};

int CInternetRadio::showMenu()
{
	int ret = RETURN_REPAINT;
	
	widget = new CWidget(frameBuffer->getScreenX() + 20, frameBuffer->getScreenY() + 20, frameBuffer->getScreenWidth() - 40, frameBuffer->getScreenHeight() - 40);
	ilist = new ClistBox(frameBuffer->getScreenX() + 20, frameBuffer->getScreenY() + 20, frameBuffer->getScreenWidth() - 40, frameBuffer->getScreenHeight() - 40);

	for(unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
	{
		//
		if (playlist[i].FileExtension != CFile::EXTENSION_URL && !playlist[i].MetaData.bitrate)
		{
			// 
			GetMetaData(playlist[i]);
		}

		std::string tmp;
		
		//
		getFileInfoToDisplay(tmp, playlist[i]);

		char duration[9];
		snprintf(duration, 8, "%ldk", playlist[i].MetaData.total_time);
		std::string cover = playlist[i].MetaData.cover.empty()? DATADIR "/icons/no_coverArt.png" : playlist[i].MetaData.cover;

		//
		item = new CMenuForwarder(tmp.c_str(), true, NULL, this, "iplay");
			
		item->setOptionInfo(duration);
		item->setNumber(i + 1);

		item->setHint(tmp.c_str());
		item->setHintIcon(cover.c_str());

		ilist->addItem(item);
	}
	
	ilist->setMode(ClistBox::MODE_LISTBOX);

	ilist->setSelected(selected);

	ilist->enablePaintHead();
	ilist->setTitle(_("Internet Radio"), NEUTRINO_ICON_MP3);
	ilist->setHeadButtons(&HeadButtons);
	ilist->enablePaintDate();
	
	ilist->enablePaintFoot();
	ilist->setFootButtons(AudioPlayerButtons, FOOT_BUTTONS_COUNT);

	widget->addKey(CRCInput::RC_setup, this, CRCInput::getSpecialKeyName(CRCInput::RC_setup));
	widget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	widget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	widget->addKey(CRCInput::RC_yellow, this, CRCInput::getSpecialKeyName(CRCInput::RC_yellow));
	widget->addKey(CRCInput::RC_blue, this, CRCInput::getSpecialKeyName(CRCInput::RC_blue));

	widget->addCCItem(ilist);
	
	ret = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	return ret;
}

int CInternetRadio::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CInternetRadio::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = RETURN_REPAINT;
	
	if(parent)
		hide();

	if(actionKey == "iplay")
	{
		selected = ilist->getSelected();
		
		for (unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
		{
			tmpAudioPlayerGui.addToPlaylist(playlist[i]);
		}

		tmpAudioPlayerGui.setCurrent(selected);
		tmpAudioPlayerGui.setInetMode();
		
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
	else if(actionKey == "RC_blue")
	{
		shufflePlaylist();
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
	else if(actionKey == "RC_red")
	{
		CAudioPlayList::iterator p = playlist.begin() + ilist->getSelected();
		playlist.erase(p);

		if (selected >= playlist.size())
			selected = playlist.size() - 1;

		showMenu();

		return RETURN_EXIT_ALL;
	}
	
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
	CInternetRadio* internetRadioHandler = new CInternetRadio();
	
	internetRadioHandler->exec(NULL, "");
	
	delete internetRadioHandler;
	internetRadioHandler = NULL;
}


