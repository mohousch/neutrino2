/*
  $Id: picviewer.cpp 2018/07/10 mohousch Exp $

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

class CPicViewer : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;

		CWidget* widget;
		ClistBox* plist;
		CMenuItem* item;

		CPictureViewerGui tmpPictureViewerGui;
		CFileFilter fileFilter;
		CFileList filelist;
		CPicture pic;
		CPicturePlayList playlist;
		std::string Path;
		int selected;

		void loadPlaylist();
		void openFileBrowser();

		void showMenu();
		
	public:
		CPicViewer();
		~CPicViewer();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

CPicViewer::CPicViewer()
{
	frameBuffer = CFrameBuffer::getInstance();

	widget = NULL;
	plist = NULL;
	item = NULL;

	selected = 0;

	fileFilter.addFilter("png");
	fileFilter.addFilter("bmp");
	fileFilter.addFilter("jpg");
	fileFilter.addFilter("jpeg");
}

CPicViewer::~CPicViewer()
{
	playlist.clear();

	hide();
}

void CPicViewer::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

void CPicViewer::loadPlaylist()
{
	playlist.clear();

	Path = g_settings.network_nfs_picturedir;

	//if(CFileHelpers::getInstance()->readDir(Path, &filelist, &fileFilter))
	CFileHelpers::getInstance()->addRecursiveDir(&filelist, Path, &fileFilter);

	if(filelist.size() > 0)
	{
		struct stat statbuf;
				
		CFileList::iterator files = filelist.begin();
		for(; files != filelist.end() ; files++)
		{
			if (files->getType() == CFile::FILE_PICTURE)
			{
				pic.Filename = files->Name;
				std::string tmp = files->Name.substr(files->Name.rfind('/') + 1);
				pic.Name = tmp.substr(0, tmp.rfind('.'));
				pic.Type = tmp.substr(tmp.rfind('.') + 1);
			
				if(stat(pic.Filename.c_str(), &statbuf) != 0)
					printf("stat error");
				pic.Date = statbuf.st_mtime;
				
				playlist.push_back(pic);
			}
		}
	}
}

void CPicViewer::openFileBrowser()
{
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_picturedir : "");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

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
				for (unsigned long i = 0; i < (unsigned long)playlist.size(); i++)
				{
					if(playlist[i].Filename == pic.Filename)
						playlist.erase(playlist.begin() + i); 
				}
							
				playlist.push_back(pic);
			}
		}
	}
}

#define HEAD_BUTTONS_COUNT	2
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_SETUP, " " },
	{ NEUTRINO_ICON_BUTTON_HELP, " " }
};

#define FOOT_BUTTONS_COUNT 4
const struct button_label PictureViewerButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN , _("Add") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Delete all") },
	{ NEUTRINO_ICON_BUTTON_BLUE  , _("Slideshow") }
};

void CPicViewer::showMenu()
{
	widget = new CWidget();
	
	plist = new ClistBox(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	for(unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
	{
		std::string tmp = playlist[i].Name;
		tmp += " (";
		tmp += playlist[i].Type;
		tmp += ')';

		char timestring[18];
		strftime(timestring, 18, "%d-%m-%Y %H:%M", gmtime(&playlist[i].Date));

		item = new ClistBoxItem(tmp.c_str(), true, NULL, this, "view");
		
		item->setNumber(i + 1);	
		item->setOptionInfo(timestring);
		
		item->setHintIcon(playlist[i].Filename.c_str());
		item->setHint(timestring);

		plist->addItem(item);
	}

	plist->setWidgetMode(MODE_LISTBOX);
	plist->setWidgetType(WIDGET_TYPE_FRAME);
	plist->setItemsPerPage(10, 6);
	
	//
	plist->enablePaintHead();
	plist->setTitle(_("Pictureviewer"), NEUTRINO_ICON_PICTURE);
	plist->enablePaintDate();
	plist->setHeadButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	
	//
	plist->enablePaintFoot();
	plist->setFootButtons(PictureViewerButtons, FOOT_BUTTONS_COUNT);
	
	//
	plist->enablePaintItemInfo();

	//
	widget->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));
	widget->addKey(RC_setup, this, CRCInput::getSpecialKeyName(RC_setup));
	widget->addKey(RC_red, this, CRCInput::getSpecialKeyName(RC_red));
	widget->addKey(RC_green, this, CRCInput::getSpecialKeyName(RC_green));
	widget->addKey(RC_yellow, this, CRCInput::getSpecialKeyName(RC_yellow));
	widget->addKey(RC_blue, this, CRCInput::getSpecialKeyName(RC_blue));
	
	widget->addItem(plist);

	widget->exec(NULL, "");
	
	delete plist;
	plist = NULL;
	
	delete widget;
	widget = NULL;
}

int CPicViewer::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CPicViewer::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();

	selected = plist? plist->getSelected() : 0;

	if(actionKey == "view")
	{
		tmpPictureViewerGui.addToPlaylist(playlist[selected]);
		tmpPictureViewerGui.exec(NULL, "");

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_setup")
	{
		CPictureViewerSettings * pictureViewerSettingsMenu = new CPictureViewerSettings();
		pictureViewerSettingsMenu->exec(NULL, "");
		delete pictureViewerSettingsMenu;
		pictureViewerSettingsMenu = NULL;

		return RETURN_REPAINT;					
	}
	else if(actionKey == "RC_info")
	{
		tmpPictureViewerGui.showHelp();

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_red")
	{
		CPicturePlayList::iterator p = playlist.begin() + plist->getSelected();
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
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_blue")
	{
		for (unsigned int i = 0; i < (unsigned int)playlist.size(); i++)
		{
			tmpPictureViewerGui.addToPlaylist(playlist[i]);
		}

		tmpPictureViewerGui.setCurrent(selected);
		tmpPictureViewerGui.setState(CPictureViewerGui::SLIDESHOW);		
		tmpPictureViewerGui.exec(NULL, "");

		return RETURN_REPAINT;
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
	CPicViewer* picViewerHandler = new CPicViewer();
	
	picViewerHandler->exec(NULL, "");
	
	delete picViewerHandler;
	picViewerHandler = NULL;
}


