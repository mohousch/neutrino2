/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: filebrowser.cpp 2018/08/24 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#include <unistd.h>

/* include <config.h> before <gui/filebrowser.h> to enable 64 bit file offsets */
#include <gui/filebrowser.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <gui/widget/hintbox.h>

#include <driver/encoding.h>

#include <algorithm>
#include <iostream>
#include <cctype>

#include <global.h>
#include <neutrino2.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sstream>

#include <sys/stat.h>

#include <driver/encoding.h>

#include <xmlinterface.h>
#include <system/debug.h>


#define FILEBROWSER_SMSKEY_TIMEOUT 2000

bool comparetolower(const char a, const char b)
{
	return tolower(a) < tolower(b);
};

// sort operators
bool sortByName (const CFile& a, const CFile& b)
{
	if (std::lexicographical_compare(a.Name.begin(), a.Name.end(), b.Name.begin(), b.Name.end(), comparetolower))
		return true;

	if (std::lexicographical_compare(b.Name.begin(), b.Name.end(), a.Name.begin(), a.Name.end(), comparetolower))
		return false;

	return a.Mode < b.Mode;
}

// Sorts alphabetically with Directories first
bool sortByNameDirsFirst(const CFile& a, const CFile& b)
{
	int typea, typeb;
	typea = a.getType();
	typeb = b.getType();

	if (typea == CFile::FILE_DIR)
		if (typeb == CFile::FILE_DIR)
			//both directories
			return sortByName(a, b);
		else
			//only a is directory
			return true;
	else if (typeb == CFile::FILE_DIR)
		//only b is directory
		return false;
	else
		//no directory
		return sortByName(a, b);
}

bool sortByType (const CFile& a, const CFile& b)
{
	if(a.Mode == b.Mode)
		return sortByName(a, b);
	else
		return a.Mode < b.Mode;
}

bool sortByDate (const CFile& a, const CFile& b)
{
	if(a.getFileName() == "..")
		return true;
	if(b.getFileName() == "..")
		return false;
	return a.Time < b.Time ;
}

bool sortBySize (const CFile& a, const CFile& b)
{
	if(a.getFileName()=="..")
		return true;
	if(b.getFileName()=="..")
		return false;
	return a.Size < b.Size;
}

bool (* const sortBy[FILEBROWSER_NUMBER_OF_SORT_VARIANTS])(const CFile& a, const CFile& b) =
{
	&sortByName,
	&sortByNameDirsFirst,
	&sortByType,
	&sortByDate,
	&sortBySize
};

const char* const sortByNames[FILEBROWSER_NUMBER_OF_SORT_VARIANTS] =
{
	_("(filename)"),
	_("(filename2)"),
	_("(type)"),
	_("(Date)"),
	_("(Size)")
};

CFileBrowser::CFileBrowser()
{
	commonInit();
	base = "";
}

CFileBrowser::CFileBrowser(const char * const _base)
{
	commonInit();
	base = _base;
}

void CFileBrowser::commonInit()
{
	frameBuffer = CFrameBuffer::getInstance();

	Filter = NULL;
	use_filter = true;
	Multi_Select = false;
	Dirs_Selectable = false;
	Dir_Mode = false;
	
	selected = 0;
	
	selections.clear();
	selected_filelist.clear();
	filelist.clear();

	// sms key input timeout
	m_SMSKeyInput.setTimeout(FILEBROWSER_SMSKEY_TIMEOUT);

	listBox = NULL;
	item = NULL;
	widget = NULL;
	
	sec_timer_id = 0;

	// box	
	cFrameBox.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 40;
	cFrameBox.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 40;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CFileBrowser::~CFileBrowser()
{
}

CFile * CFileBrowser::getSelectedFile()
{
	if(exit_pressed)
		return NULL;
	else
	{
		if ((!(filelist.empty())) && (!(filelist[selected].Name.empty())))
			return &filelist[selected];
		else
			return NULL;
	}
}

void CFileBrowser::ChangeDir(const std::string& filename, int selection)
{
	dprintf(DEBUG_INFO, "CFileBrowser::ChangeDir %s\n", filename.c_str());

	std::string newpath;
	
	if((filename == ".."))
	{
		std::string::size_type pos = Path.substr(0, Path.length()-1).rfind('/');

		if (pos == std::string::npos)
		{
			newpath = Path;
		}
		else
		{
			newpath = Path.substr(0, pos + 1);
		}

		if (strncmp(newpath.c_str(), base.c_str(), base.length()) != 0)
			return;
	}
	else
	{
		newpath = filename;
	}
	
	if((newpath.rfind('/') != newpath.length()- 1 || newpath.length() == 0))
	{
		newpath += '/';
	}
	
	Path = newpath;
	name = newpath;
	
	CFileList allfiles;
	
	readDir(newpath, &allfiles);

	filelist.clear();
	
	// filter
	CFileList::iterator file = allfiles.begin();
	for(; file != allfiles.end() ; file++)
	{
		if(Filter != NULL && (!S_ISDIR(file->Mode)) && use_filter)
		{
			if(!Filter->matchFilter(file->Name))
			{
				continue;
			}
		}
		
		if(Dir_Mode && (!S_ISDIR(file->Mode)))
		{
			continue;
		}
		
		filelist.push_back(*file);
	}
	
	// sort result
	sort(filelist.begin(), filelist.end(), sortBy[g_settings.filebrowser_sortmethod]);

	selected = 0;
	if ((selection != -1) && (selection < (int)filelist.size()))
		selected = selection;
}

bool CFileBrowser::readDir(const std::string& dirname, CFileList* flist)
{
	dprintf(DEBUG_NORMAL, "CFileBrowser::readDir %s\n", dirname.c_str());
	
	struct stat64 statbuf;
	dirent64 **namelist;
	int n;

	n = scandir64(dirname.c_str(), &namelist, 0, alphasort64);

	if (n < 0)
	{
		perror(("Filebrowser scandir: " + dirname).c_str());
		return false;
	}
	
	for(int i = 0; i < n; i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name, ".") != 0)
		{
			// name
			file.Name = dirname + namelist[i]->d_name;

			// stat
			if(stat64((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;
				
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return true;
}

bool CFileBrowser::exec(const char * const dirname)
{
	dprintf(DEBUG_NORMAL, "CFileBrowser::exec:\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	bool res = false;
	exit_pressed = false;

	//
	widget = CNeutrinoApp::getInstance()->getWidget("filebrowser");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		listBox = new ClistBox(&cFrameBox);
		
		widget->name = "filebrowser";
		widget->addWidgetItem(listBox);
	}	

	name = dirname;
	std::replace(name.begin(), name.end(), '\\', '/');
	
	int selection = -1;
	
	if (name == Path)
		selection = selected;
		
	ChangeDir(name, selection);

	paint();
	CFrameBuffer::getInstance()->blit();

	int oldselected = selected;
	
	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER]);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~RC_Repeat;

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_FILEBROWSER]);

		if(!CRCInput::isNumeric(msg))
		{
			m_SMSKeyInput.resetOldKey();
		}

		if (msg == RC_yellow)
		{
			selected = listBox->getSelected();

			if ((Multi_Select) && (selected < filelist.size()))
			{
				if(filelist[selected].getFileName() != "..")
				{
					if( (S_ISDIR(filelist[selected].Mode) && Dirs_Selectable) || !S_ISDIR(filelist[selected].Mode) )
					{
						filelist[selected].Marked = !filelist[selected].Marked;

						paint();
					}
				}
				msg_repeatok = RC_down;	// jump to next item
			}
		}

		if ((msg == RC_red) || msg == RC_page_down)
		{
			listBox->scrollPageDown();
		}
		else if ((msg == RC_green) || (msg == RC_page_up) )
		{
			listBox->scrollPageUp();
		}
		else if (msg_repeatok == RC_up)
		{
			listBox->scrollLineUp();
		}
		else if (msg_repeatok == RC_down)
		{
			listBox->scrollLineDown();
		}
		else if ( ( msg == RC_timeout ) )
		{
			selected = oldselected;
			exit_pressed = true; //FIXME:
			loop = false;
		}
		else if ( msg == RC_right )
		{
			selected = listBox->getSelected();

			if (!(filelist.empty()))
			{
				if (S_ISDIR(filelist[selected].Mode))
				{
	 				if (filelist[selected].getFileName() != "..") 
					{
						selections.push_back(selected);
						ChangeDir(filelist[selected].Name);

						paint();
					}
				}
			}
		}
		else if ( msg == RC_left )
		{
			if (selections.size() > 0)
			{
				ChangeDir("..", selections.back());
				selections.pop_back();
				
				paint();
			} 
			else
			{
				ChangeDir("..");
				
				paint();
			}

			if (!(filelist.empty()))	
			{
				paint();
			}
		}
		else if ( msg == RC_blue )
		{
			if(Filter != NULL)
			{
				use_filter = !use_filter;

				ChangeDir(Path);
				//hide();
				paint();
			}
		}
		else if ( msg == RC_home )
		{
			selected = -1;
			exit_pressed = true;
			loop = false;
		}
		else if ( msg == RC_spkr)
		{
			selected = listBox->getSelected();

			if(".." !=(filelist[selected].getFileName().substr(0, 2))) // do not delete that
			{
				std::stringstream _msg;
				_msg << _("Delete") << " ";
				if (filelist[selected].getFileName().length() > 25)
				{
					_msg << filelist[selected].getFileName().substr(0, 25) << "...";
				}
				else
					_msg << filelist[selected].getFileName();

				_msg << " " << _("?");
				if (MessageBox(_("Delete"), _msg.str().c_str(), mbrNo, mbYes|mbNo) == mbrYes)
				{
					recursiveDelete(filelist[selected].Name.c_str());
					
					if(".ts" ==(filelist[selected].getFileName().substr(filelist[selected].getFileName().length() - 3, filelist[selected].getFileName().length())))//if bla.ts
					{
						recursiveDelete((filelist[selected].Name.substr(0,filelist[selected].Name.length()-7)+".xml").c_str());//remove bla.xml von bla.ts
					}
					ChangeDir(Path);
					//hide();
					paint();
				}
			}
		}
		else if (msg == RC_ok)
		{
			selected = listBox->getSelected();

			if (!(filelist.empty()))
			{
				if (filelist[selected].getFileName() == "..")
				{
					if (selections.size() > 0)
					{
						ChangeDir("..", selections.back());
						selections.pop_back();
						//hide();
						paint();
					} 
					else
					{
						std::string::size_type pos = Path.substr(0, Path.length()-1).rfind('/');
						if (pos != std::string::npos) 
						{
							ChangeDir("..");
							//hide();
							paint();
						}
						else 
						{
							loop = false;
							res = true;
							filelist[selected].Name = "/";
						}
					}
				}
				else
				{
					std::string filename = filelist[selected].Name;
					if ( filename.length() > 1 )
					{
						if((!Multi_Select) && S_ISDIR(filelist[selected].Mode) && !Dir_Mode)
						{
							selections.push_back(selected);
							ChangeDir(filelist[selected].Name);
							//hide();
							paint();
						}
						else
						{
							filelist[selected].Marked = true;
							loop = false;
							res = true;
						}
					}
				}
			}
		}
		else if ( msg == RC_info )
		{
			if (++g_settings.filebrowser_sortmethod >= FILEBROWSER_NUMBER_OF_SORT_VARIANTS)
				g_settings.filebrowser_sortmethod = 0;

			sort(filelist.begin(), filelist.end(), sortBy[g_settings.filebrowser_sortmethod]);

			paint();
		}
		else if (CRCInput::isNumeric(msg_repeatok))
		{
			if (!(filelist.empty()))
				SMSInput(msg_repeatok);
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			//
			widget->refresh();
		} 
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}
	
		frameBuffer->blit();	
	}

	hide();

	selected_filelist.clear();

	if(res && Multi_Select)
	{
		for(unsigned int i = 0; i < filelist.size(); i++)
		{
			if(filelist[i].Marked)
			{
				if(S_ISDIR(filelist[i].Mode)) 
				{
					addRecursiveDir(&selected_filelist, filelist[i].Name, true);
				} 
				else
					selected_filelist.push_back(filelist[i]);
			}
		}
	}

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	return res;
}

void CFileBrowser::addRecursiveDir(CFileList * re_filelist, std::string rpath, bool bRootCall)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int n;

	dprintf(DEBUG_INFO, "CFileBrowser::addRecursiveDir %s\n", rpath.c_str());

	if (bRootCall) 
		bCancel = false;

	g_RCInput->getMsg_us(&msg, &data, 1);

	if (msg == RC_home)
	{
		// home key cancel scan
		bCancel = true;
	}
	else if (msg != RC_timeout)
	{
		// other event, save to low priority queue
		g_RCInput->postMsg( msg, data, false );
	}
	
	if(bCancel)
		return;

	if ( ((rpath.empty()) || ((*rpath.rbegin()) != '/')))
	{
		rpath += '/';
	}

	CFileList tmplist;
	if(!readDir(rpath, &tmplist))
	{
		perror(("Recursive scandir: " + rpath).c_str());
	}
	else
	{
		n = tmplist.size();
		
		for(int i = 0; i < n; i++)
		{
			std::string basename = tmplist[i].Name.substr(tmplist[i].Name.rfind('/')+1);
			
			if( basename != ".." )
			{
				if(Filter != NULL && (!S_ISDIR(tmplist[i].Mode)) && use_filter)
				{
					if(!Filter->matchFilter(tmplist[i].Name))
					{
						continue;
					}
				}
				if(!S_ISDIR(tmplist[i].Mode))
					re_filelist->push_back(tmplist[i]);
				else
					addRecursiveDir(re_filelist,tmplist[i].Name, false);
			}
		}
	}
}

void CFileBrowser::hide()
{
	dprintf(DEBUG_NORMAL, "CFileBrowser::hide:\n");

	frameBuffer->paintBackgroundBoxRel(cFrameBox.iX, cFrameBox.iY, cFrameBox.iWidth, cFrameBox.iHeight);
	//listBox->hide();
	widget->hide();
	
	frameBuffer->blit();
}

const struct button_label FileBrowserButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Next Page") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("Prev Page") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Mark") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Filter off") },
};

const struct button_label HButtons[2] =
{
	{ NEUTRINO_ICON_BUTTON_MUTE_SMALL, "" },
	{ NEUTRINO_ICON_BUTTON_HELP, "" },
};

void CFileBrowser::paint()
{
	dprintf(DEBUG_NORMAL, "CFileBrowser::paint:\n");

	listBox->clear();

	for (unsigned int count = 0; count < filelist.size(); count++)
	{
		item = new ClistBoxItem(FILESYSTEM_ENCODING_TO_UTF8(std::string(filelist[count].getFileName()).c_str()));

		// marked
		if (filelist[count].Marked)
		{
			item->setMarked(true);
		}

		// iconName
		std::string fileicon = "";

		switch(filelist[count].getType())
		{
			case CFile::FILE_AUDIO:
				fileicon = NEUTRINO_ICON_MP3;
				break;

			case CFile::FILE_DIR:
				fileicon = NEUTRINO_ICON_FOLDER;
				break;

			case CFile::FILE_PICTURE:
				fileicon = NEUTRINO_ICON_PICTURE;
				break;
				
			case CFile::FILE_VIDEO:
				fileicon = NEUTRINO_ICON_MOVIE;
				break;
					
			case CFile::FILE_TEXT:
			default:
				fileicon = NEUTRINO_ICON_FILE;
		}

		item->setIconName(fileicon.c_str());

		std::string tmp;

		//
		if( S_ISREG(filelist[count].Mode) )
		{
			if (g_settings.filebrowser_showrights != 0)
			{
				const char * attribute = "xwr";
				char modestring[9 + 1];

				for (int m = 8; m >= 0; m--)
				{
					modestring[8 - m] = (filelist[count].Mode & (1 << m)) ? attribute[m % 3] : '-';
				}

				modestring[9] = 0;

				tmp = modestring;
			}

#define GIGABYTE 1073741824LL
#define MEGABYTE 1048576LL
#define KILOBYTE 1024LL
			char tmpstr[256];
			const char *unit = "";
			long long factor = 0;

			if (filelist[count].Size >= GIGABYTE)
			{
				factor = GIGABYTE;
				unit = "G";
			}
			else if (filelist[count].Size >= MEGABYTE)
			{
				factor = MEGABYTE;
				unit = "M";
			}
			else if (filelist[count].Size >= KILOBYTE)
			{
				factor = KILOBYTE;
				unit = "k";
			}

			if (factor)
			{
				int a = filelist[count].Size / factor;
				int b = (filelist[count].Size - a * factor) * 1000 / factor;
				snprintf(tmpstr, sizeof(tmpstr), "%d.%03d%s", a, b, unit);
			}
			else
				snprintf(tmpstr,sizeof(tmpstr),"%d", (int)filelist[count].Size);

			tmp += " ";
			tmp += tmpstr;

			if( S_ISDIR(filelist[count].Mode) )
			{
				char timestring[18];
				time_t rawtime;

				rawtime = filelist[count].Time;
				strftime(timestring, 18, "%d-%m-%Y %H:%M", gmtime(&rawtime));

				tmp = timestring;
			}
		}

		item->setOptionInfo(tmp.c_str()); 

		listBox->addItem(item);
	}

	// head
	char l_name[100];
	snprintf(l_name, sizeof(l_name), "%s %s", _("Filebrowser"), FILESYSTEM_ENCODING_TO_UTF8(std::string(name).c_str())); // UTF-8

	listBox->enablePaintHead();
	listBox->setTitle(l_name, NEUTRINO_ICON_FILEBROWSER);
	listBox->enablePaintDate();
	listBox->setHeadGradient(g_settings.Head_gradient);
	listBox->setHeadRadius(g_settings.Head_radius);
	listBox->setHeadLine(g_settings.Head_line);
	listBox->setHeadButtons(HButtons, 2);

	// foot
	listBox->enablePaintFoot();
	listBox->setFootGradient(g_settings.Foot_gradient);
	listBox->setFootRadius(g_settings.Foot_radius);
	listBox->setFootLine(g_settings.Foot_line);

	struct button_label Button[4];
	Button[0] = FileBrowserButtons[0];
	Button[1] = FileBrowserButtons[1];

	Button[2].button = NEUTRINO_ICON_BUTTON_YELLOW;
	Button[2].localename = Multi_Select? _("Mark") : " ";

	Button[3].button = NEUTRINO_ICON_BUTTON_BLUE;
	Button[3].localename = (Filter != NULL)? (use_filter)? _("Filter off") : _("Filter on") : " ";

	listBox->setFootButtons(Button, 4);

	//
	listBox->setSelected(selected);
	//listBox->paint();
	widget->paint();
}

void CFileBrowser::SMSInput(const neutrino_msg_t msg)
{
	unsigned char key = m_SMSKeyInput.handleMsg(msg);

	unsigned int i;
	for(i = (selected + 1) % filelist.size(); i != selected ; i= (i + 1) % filelist.size())
	{
		if(tolower(filelist[i].getFileName()[0]) == key)
		{
			break;
		}
	}

	paint();
}

void CFileBrowser::recursiveDelete(const char *file)
{
	struct stat64 statbuf;
	dirent64 **namelist;
	int n;
	
	dprintf(DEBUG_INFO, "CFileBrowser::Delete %s\n", file);
	
	if(lstat64(file, &statbuf) == 0)
	{
		if(S_ISDIR(statbuf.st_mode))
		{
			n = scandir64(file, &namelist, 0, alphasort64);
			printf("CFileBrowser::Delete: n:%d\n", n);

			if(n > 0)
			{
				while(n--)
				{
					if(strcmp(namelist[n]->d_name, ".") != 0 && strcmp(namelist[n]->d_name, "..") != 0)
					{
						std::string fullname = (std::string)file + "/" + namelist[n]->d_name;
						recursiveDelete(fullname.c_str());
					}
					free(namelist[n]);
				}

				free(namelist);

				rmdir(file);
			}
		}
		else
		{
			unlink(file);
		}
	}
	else
		perror(file);
}
