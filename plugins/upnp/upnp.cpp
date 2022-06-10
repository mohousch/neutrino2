/*
  Neutrino-GUI  -   DBoxII-Project

  UPnP Browser (c) 2007 by Jochen Friedrich

  Homepage: http://dbox.cyberphoria.org/

  Kommentar:

  Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
  Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
  auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
  Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <stdexcept>

#include <global.h>
#include <neutrino.h>
#include <xmltree.h>
#include <upnpclient.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/audioplay.h>
#include <driver/audiofile.h>
#include <driver/audiometadata.h>
#include <driver/color.h>

#include <daemonc/remotecontrol.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <system/settings.h>
#include <system/debug.h>
#include <gui/pictureviewer.h>
#include <gui/movieplayer.h>
#include <gui/audioplayer.h>

#include <upnp.h>

#define UPNP_ICON_SMALL PLUGINDIR "/upnp/upnp_small.png"


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);


const struct button_label RescanButton[4] = 
{
	{ "", "" },
	{ "", "" },
	{ "", "" },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Scan again") }
};

CUpnpBrowserGui::CUpnpBrowserGui(UPNP_GUI g)
{
	m_socket = new CUPnPSocket();
	m_frameBuffer = CFrameBuffer::getInstance();

	thumbnail_dir = "/tmp/upnpbrowser";

	listBox = NULL;
	item = NULL;

	selected = 0;
	gui = g;

	fileHelper.createDir(thumbnail_dir.c_str(), 0755);

	// box	
	cFrameBox.iWidth = m_frameBuffer->getScreenWidth() / 20 * 17;
	cFrameBox.iHeight = m_frameBuffer->getScreenHeight() / 20 * 18;
	
	cFrameBox.iX = m_frameBuffer->getScreenX() + (m_frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = m_frameBuffer->getScreenY() + (m_frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CUpnpBrowserGui::~CUpnpBrowserGui()
{
	if(m_socket)
		delete m_socket;

	fileHelper.removeDir(thumbnail_dir.c_str());
}

void CUpnpBrowserGui::hide()
{
	listBox->hide();
}

void CUpnpBrowserGui::splitProtocol(std::string &protocol, std::string &prot, std::string &network, std::string &mime, std::string &additional)
{
	std::string::size_type pos;
	std::string::size_type startpos = 0;

	pos = protocol.find(":", startpos);
	if (pos != std::string::npos)
	{
		prot = protocol.substr(startpos, pos-startpos);
		startpos = pos + 1;

		pos = protocol.find(":", startpos);
		if (pos != std::string::npos)
		{
			network = protocol.substr(startpos, pos-startpos);
			startpos = pos + 1;

			pos = protocol.find(":", startpos);
			if (pos != std::string::npos)
			{
				mime = protocol.substr(startpos, pos-startpos);
				startpos = pos + 1;

				pos = protocol.find(":", startpos);
				if (pos != std::string::npos)
				{
					additional = protocol.substr(startpos, pos-startpos);
				}
			}
		}
	}
	
	dprintf(DEBUG_DEBUG, "%s -> %s - %s - %s - %s\n", protocol.c_str(), prot.c_str(), network.c_str(), mime.c_str(), additional.c_str());
}

std::vector<UPnPEntry> *CUpnpBrowserGui::decodeResult(std::string result)
{
	XMLTreeParser * parser;
	XMLTreeNode   * root, * node, * snode;

	parser = new XMLTreeParser("UTF-8");
	parser->Parse(result.c_str(), result.size(), 1);
	root = parser->RootNode();
	
	if (!root)
	{
		delete parser;
		return NULL;
	}
	entries = new std::vector<UPnPEntry>;

	for (node = root->GetChild(); node; node = node->GetNext())
	{
		bool isdir;
		std::string title, artist = "", album = "", id, children;
		char *type, *p;

		if (!strcmp(node->GetType(), "container"))
		{
			std::vector<UPnPResource> resources;
			isdir = true;
			for (snode = node->GetChild(); snode; snode = snode->GetNext())
			{
				type=snode->GetType();
				p = strchr(type,':');
				if (p)
					type = p + 1;
				
				if (!strcmp(type,"title"))
				{
					p = snode->GetData();
					if (!p)
						p = (char *) "";
					title = std::string(p);
				}
			}
			p = node->GetAttributeValue((char *) "id");
			if (!p)
				p = (char *) "";
			id = std::string(p);

			p = node->GetAttributeValue((char *) "childCount");
			if (!p)
				p = (char *) "";
			children = std::string(p);

			UPnPEntry entry = {id, isdir, title, artist, album, children, resources, -1};
			entries->push_back(entry);
		}
		
		if (!strcmp(node->GetType(), "item"))
		{
			std::vector<UPnPResource> resources;
			int preferred = -1;
			std::string protocol, prot, network, mime, additional;
			isdir = false;
			for (snode = node->GetChild(); snode; snode = snode->GetNext())
			{
				std::string duration, url, size;
				unsigned int i;
				type = snode->GetType();
				p = strchr(type,':');
				if (p)
					type = p + 1;

				if (!strcmp(type,"title"))
				{
					p = snode->GetData();
					if (!p)
						p = (char *) "";
					title = std::string(p);
				}
				else if (!strcmp(type,"artist"))
				{
					p = snode->GetData();
					if (!p)
						p = (char *) "";
					artist = std::string(p);
				}
				else if (!strcmp(type,"album"))
				{
					p = snode->GetData();
					if (!p)
						p = (char *) "";
					album = std::string(p);
				}
				else if (!strcmp(type,"res"))
				{
					p = snode->GetData();
					if (!p)
						p = (char *) "";
					url = std::string(p);
					p = snode->GetAttributeValue((char *) "size");
					if (!p)
						p = (char *) "0";
					size = std::string(p);
					p = snode->GetAttributeValue((char *) "duration");
					if (!p)
						p = (char *) "";
					duration = std::string(p);
					p = snode->GetAttributeValue((char *) "protocolInfo");
					if (!p)
						p = (char *) "";
					protocol = std::string(p);
					UPnPResource resource = {url, protocol, size, duration};
					resources.push_back(resource);
				}
				int pref = 0;
				preferred = -1;
				
				for (i = 0; i < resources.size(); i++)
				{
					protocol = resources[i].protocol;
					splitProtocol(protocol, prot, network, mime, additional);
					if (prot != "http-get")
						continue;
					
					if (mime == "image/jpeg" && pref < 1)
					{
						preferred = i;
						pref = 1;
					}
					if (mime == "image/gif" && pref < 2)
					{
						preferred = i;
						pref = 2;
					}

					if (mime == "audio/mpeg" && pref < 3)
					{
						preferred = i;
						pref = 3;
					}
					
					if (mime == "audio/x-vorbis+ogg" && pref < 4)
					{
						preferred=i;
						pref = 4;
					}
					
					//
					if (mime.substr(0, 6) == "video/" && pref < 5)
					{
						preferred = i;
						pref = 5;
					}
					
					if (mime == "video/x-flv" && pref < 6)
					{
						preferred = i;
						pref = 6;
					}
					
					if (mime == "video/mp4" && pref < 7)
					{
						preferred = i;
						pref = 7;
					}
				}
			}
			p = node->GetAttributeValue((char *) "id");
			if (!p)
				p = (char *) "";
			id = std::string(p);

			p = node->GetAttributeValue((char *) "childCount");
			if (!p)
				p=(char *) "";
			children = std::string(p);

			UPnPEntry entry = {id, isdir, title, artist, album, children, resources, preferred};

			entries->push_back(entry);
		}
	}

	delete parser;
	return entries;
}

bool CUpnpBrowserGui::loadItem(std::string id, int _selected)
{
	dprintf(DEBUG_NORMAL, "CUpnpBrowserGui::loadItem: %s\n", id.c_str());

	bool changed = true;
	bool rchanged = true;

	unsigned int index, dirnum;

	index = 0;
	dirnum = 0;
	entries = NULL;

	if (entries)
		delete entries;
			
	entries = NULL;

	std::list<UPnPAttribute>attribs;
	std::list<UPnPAttribute>results;
	std::list<UPnPAttribute>::iterator i;
	std::stringstream sindex;
	std::stringstream scount;
	unsigned int returned = 0;

	bool rfound = false;
	bool nfound = false;
	bool tfound = false;

	sindex << index;
	scount << /*ulist->getListMaxShow()*/100;

	attribs.push_back(UPnPAttribute("ObjectID", id));
	attribs.push_back(UPnPAttribute("BrowseFlag", "BrowseDirectChildren"));
	attribs.push_back(UPnPAttribute("Filter", "*"));
	attribs.push_back(UPnPAttribute("StartingIndex", sindex.str()));
	attribs.push_back(UPnPAttribute("RequestedCount", /*scount.str()*/"100"));
	attribs.push_back(UPnPAttribute("SortCriteria", ""));

	results = m_devices[_selected].SendSOAP("urn:schemas-upnp-org:service:ContentDirectory:1", "Browse", attribs);

	for (i = results.begin(); i != results.end(); i++)
	{
		if (i->first == "NumberReturned")
		{
			returned = atoi(i->second.c_str());
			nfound = true;
		}

		if (i->first == "TotalMatches")
		{
			dirnum = atoi(i->second.c_str());
			tfound = true;
		}

		if (i->first == "Result")
		{
			entries = decodeResult(i->second);
			rfound = true;
		}
	}

	if (!entries)
		return false;

	if (!nfound || !tfound || !rfound)
	{
		delete entries;
		return false;
	}

	if (returned != entries->size())
	{
		delete entries;
		return false;
	}

	if (returned == 0)
	{
		delete entries;
		return false;
	}

	//
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	bool loop = true;

	while (loop)
	{
		if(changed)
		{
			listBox->clearItems();
			selected = 0;
			showMenuEntry();

			changed = false;
		}

		g_RCInput->getMsg(&msg, &data, 10); // 1 sec timeout to update play/stop state display
		neutrino_msg_t msg_repeatok = msg & ~RC_Repeat;

		if( msg == RC_timeout)
		{
			// nothing
		}
		else if(msg == RC_home)
		{
			loop = false;
			//endall = true;
		}
		else if(msg == RC_left)
		{
			loop = false;
			changed = true;
		}
		else if (msg_repeatok == RC_up /*&& selected > 0*/)
		{
			listBox->scrollLineUp();
			//rchanged = true;
			//changed = true;
		}

		else if( (msg == RC_yellow || (int) msg == RC_page_up))
		{
			listBox->scrollPageUp();
			rchanged = true;
			changed = true;
		}
		else if (msg_repeatok == RC_down)
		{
			listBox->scrollLineDown();
			//rchanged = true;
			//changed = true;
		}
		else if( (msg == RC_green || (int) msg == RC_page_down))
		{
			listBox->scrollPageDown();
			rchanged = true;
		}
		else if(msg == RC_right)
		{
			selected = listBox->getSelected();

			if ((*entries)[selected - index].isdir)
			{
				bool endall = loadItem((*entries)[selected - index].id, _selected);
				if (endall)
					loop = false;
			}
			changed = true;
		}
		else if(msg == RC_red || msg == RC_ok)
		{
			selected = listBox->getSelected();

			if (!(*entries)[selected - index].isdir)
			{
				int preferred = (*entries)[selected - index].preferred;
				if (preferred != -1)
				{
					std::string protocol, prot, network, mime, additional;
					protocol=(*entries)[selected - index].resources[preferred].protocol;
					splitProtocol(protocol, prot, network, mime, additional);
					
					if (mime == "audio/mpeg")
					{
						CAudiofile audiofile((*entries)[selected - index].resources[preferred].url, CFile::EXTENSION_MP3);
						tmpAudioPlayerGui.addToPlaylist(audiofile);
						tmpAudioPlayerGui.exec(this, "urlplayback");
					}
					else if ((mime == "image/gif") || (mime == "image/jpeg"))
					{
						std::string fname;

						//DownloadImage
						if(!((*entries)[selected - index].resources[preferred].url).empty())
						{
							fname = thumbnail_dir;

							fname += (*entries)[selected - index].resources[preferred].url.substr((*entries)[selected - index].resources[preferred].url.find_last_of("/"));

							int ext_pos = 0;
							ext_pos = fname.rfind('?');
	
							if( ext_pos > 0)
							{
								std::string extension;
								extension = fname.substr(ext_pos + 1, fname.length() - ext_pos);

								fname = fname.substr(0, fname.length() - (extension.length() + 1));
							}

							::downloadUrl((*entries)[selected - index].resources[preferred].url, fname);
						}
					
						//
						CPicture pic;
						struct stat statbuf;
				
						pic.Filename = fname;
						std::string tmp = fname.substr(fname.rfind('/') + 1);
						pic.Name = tmp.substr(0, tmp.rfind('.'));
						pic.Type = tmp.substr(tmp.rfind('.') + 1);
			
						if(stat(pic.Filename.c_str(), &statbuf) != 0)
							printf("stat error");
						pic.Date = statbuf.st_mtime;
				
						tmpPictureViewerGui.addToPlaylist(pic);
						tmpPictureViewerGui.exec(this, "urlplayback");

						changed = true;
					}

					else if (mime.substr(0,6) == "video/")
					{
						MI_MOVIE_INFO mfile;
						mfile.file.Name = (*entries)[selected - index].resources[preferred].url.c_str(); 
						mfile.epgTitle = (*entries)[selected - index].title;
						mfile.ytid = "upnp"; 
						
						tmpMoviePlayerGui.addToPlaylist(mfile);
						tmpMoviePlayerGui.exec(this, "urlplayback");
						
						changed = true;
					}
				}

			}
			
			changed = true;
		}
		else if(msg == NeutrinoMessages::RECORD_START ||
			msg == NeutrinoMessages::ZAPTO ||
			msg == NeutrinoMessages::STANDBY_ON ||
			msg == NeutrinoMessages::SHUTDOWN ||
			msg == NeutrinoMessages::SLEEPTIMER)
		{
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else if(msg == NeutrinoMessages::EVT_TIMER)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				loop = false;
			changed = true;
		}
								
		m_frameBuffer->blit();	
	}
	
	hide();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
	
	//return endall;
	//
	
	return true;
}

void CUpnpBrowserGui::loadDevices(bool hint)
{
	m_devices.clear();

	CHintBox *scanBox = new CHintBox(_("Information"), _("Scanning for UPnP servers")); // UTF-8

	if(hint)
		scanBox->paint();

	m_devices = m_socket->Discover("urn:schemas-upnp-org:service:ContentDirectory:1");

	if(hint)
		scanBox->hide();

	if (!m_devices.size())
	{
		MessageBox(_("Information"), _("No UPnP servers found"), mbrBack, mbBack, NEUTRINO_ICON_UPDATE);
		delete scanBox;
		return;
	}

	delete scanBox;
	scanBox = NULL;
}

void CUpnpBrowserGui::showMenuDevice()
{
	dprintf(DEBUG_NORMAL, "CUpnpBrowserGui::showMenuDevice:\n");

	//selected = 0;
	listBox->clearAll();

	// add items
	for(unsigned int i = 0; i < m_devices.size(); i++)
	{
		char sNr[20];
		sprintf(sNr, "%2d", i + 1);

		item = new ClistBoxItem(m_devices[i].modelname.c_str());

		item->setOptionInfo(m_devices[i].friendlyname.c_str());
		item->setNumber(i + 1);

		// details Box
		item->setInfo1(m_devices[i].manufacturer.c_str());
		item->setOptionInfo1(m_devices[i].manufacturerurl.c_str());
		item->setInfo2(m_devices[i].modeldescription.c_str());
		item->setOptionInfo2(m_devices[i].modelnumber.c_str());

		listBox->addItem(item);
	}

	// head
	listBox->setTitle(_("UPnP Browser"), UPNP_ICON_SMALL);
	listBox->enablePaintHead();
	listBox->enablePaintDate();

	// foot
	listBox->enablePaintFoot();
	listBox->setFootButtons(RescanButton, 4);

	// foot info
	listBox->enablePaintItemInfo(70);

	//
	listBox->setSelected(selected);
	listBox->paint();
}

void CUpnpBrowserGui::showMenuEntry()
{
	dprintf(DEBUG_NORMAL, "CUpnpBrowserGui::showMenuEntry:\n");
	
	listBox->clearAll();
	
	for(unsigned int i = 0; i < entries->size(); i++)
	{
		//
		int preferred = (*entries)[i].preferred;
		std::string info;
		std::string fileicon;
	
		if ((*entries)[i].isdir)
		{
			info = "<DIR>";
			fileicon = NEUTRINO_ICON_FOLDER;
		}
		else
		{
			//FIXME: revise this
			if (preferred != -1)
			{
				info = (*entries)[i].resources[preferred].duration;
			
				// icon
				std::string protocol, prot, network, mime, additional;

				protocol = (*entries)[i].resources[preferred].protocol;
				splitProtocol(protocol, prot, network, mime, additional);
					
				if (mime.substr(0, 6)  == "audio/")
				{
					fileicon = NEUTRINO_ICON_MP3;
				}
				else if (mime.substr(0, 6) == "image/")
				{
					fileicon = NEUTRINO_ICON_PICTURE;
				}
				else if (mime.substr(0, 6) == "video/")
				{
					fileicon = NEUTRINO_ICON_MOVIE;
				}
			}
			else
			{
				info = "(none)";
				fileicon = NEUTRINO_ICON_FILE;
			}
		}
		//

		item = new ClistBoxItem((*entries)[i].title.c_str());

		item->setIconName(fileicon.c_str());
		item->setOptionInfo(info.c_str());

		//
		std::string tmp;
		std::stringstream ts;

		ts << "Resources: " << (*entries)[i].resources.size() << " Selected: " << preferred + 1 << " ";
		tmp = ts.str();

		if (preferred != -1)
			tmp = tmp + "Duration: " + (*entries)[i].resources[preferred].duration;
		else
			tmp = tmp + "No resource for Item";

		item->setInfo1(tmp.c_str());

		if ((*entries)[i].isdir)
			tmp = "Directory";
		else
		{
			tmp = "";
			if (preferred != -1)
			{
				std::string proto, network, mime, _info;
				splitProtocol((*entries)[i].resources[preferred].protocol, proto, network, mime, _info);
				tmp = "Protocol: " + proto + ", MIME-Type: " + mime;
			}

		}

		item->setInfo2(tmp.c_str());

		listBox->addItem(item);
	}

	// head
	listBox->setTitle(_("UPnP Browser"), UPNP_ICON_SMALL);
	listBox->enablePaintHead();
	listBox->enablePaintDate();

	// foot
	listBox->enablePaintFoot();
	listBox->setFootButtons(RescanButton, 4);

	// foot info
	listBox->enablePaintItemInfo(70);

	//
	listBox->setSelected(selected);
	listBox->paint();
}

int CUpnpBrowserGui::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CUpnpBrowserGui::exec: %s\n", actionKey.c_str());

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	bool changed = false;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	listBox = new ClistBox(&cFrameBox);
	loadDevices();
	showMenuDevice();
	m_frameBuffer->blit();

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	bool loop = true;

	while(loop)
	{
		if (changed)
		{
			listBox->clearItems();

			showMenuDevice();
			changed = false;
		}

		g_RCInput->getMsg(&msg, &data, 10); // 1 sec timeout to update play/stop state display
		neutrino_msg_t msg_repeatok = msg & ~RC_Repeat;

		if( msg == RC_timeout)
		{
			//loop = false;
		}
		else if( msg == RC_home)
		{
			loop = false;
		}
		else if (msg_repeatok == RC_page_up)
		{
			listBox->scrollPageUp();
		} 
		else if (msg_repeatok == RC_page_down)
		{
			listBox->scrollPageDown();
		}
		else if (msg_repeatok == RC_up)
		{
			listBox->scrollLineUp();
		} 
		else if (msg_repeatok == RC_down)
		{
			listBox->scrollLineDown();
		}
		else if (msg == RC_right)
		{
			loadItem("0", listBox->getSelected());
			changed = true;
			listBox->clearItems();
			showMenuEntry();
		}
		else if (msg == RC_left)
		{
			loop = false;
		}
		else if (msg == RC_ok)
		{
			//listBox->clearItems();
			loadItem("0", listBox->getSelected());
			changed = true;
		} 
		else if (msg == RC_blue)
		{
			hide();
			listBox->clearItems();
			loadDevices();
			showMenuDevice();
			//changed = true;
		}       
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			listBox->paintHead();
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				loop = false;
			//changed = true;
		}

		m_frameBuffer->blit();	
	}

	hide();

	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	delete listBox;
	listBox = NULL;
	
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
	CUpnpBrowserGui * upnpHandler = new CUpnpBrowserGui();
	
	upnpHandler->exec(NULL, "");
	
	delete upnpHandler;
	upnpHandler = NULL;
}



