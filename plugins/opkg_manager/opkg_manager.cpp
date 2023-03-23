/*
	Experimental OPKG-Manager - Neutrino-GUI

	Based upon Neutrino-GUI 
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Implementation: 
	Copyright (C) 2012 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

        License: GPL

        This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.

		
	NOTE for ignorant distributors:
	It's not allowed to distribute any compiled parts of this code, if you don't accept the terms of GPL.
	Please read it and understand it right!
	This means for you: Hold it, if not, leave it! You could face legal action! 
	Otherwise ask the copyright owners, anything else would be theft!
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "opkg_manager.h"

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include "gui/widget/progresswindow.h"
#include <gui/filebrowser.h>

#include <system/debug.h>


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

COPKGManager::COPKGManager()
{
	vp_pkg_menu = NULL;
	v_pkg_list.clear();
	v_pkg_installed.clear();
	v_pkg_upgradable.clear();
}


COPKGManager::~COPKGManager()
{
	
}

const opkg_cmd_struct_t pkg_types[OM_MAX] =
{
	{OM_LIST, 		"opkg list"},
	{OM_LIST_INSTALLED, 	"opkg list-installed"},
	{OM_LIST_UPGRADEABLE,	"opkg list-upgradable"},
	{OM_UPDATE,		"opkg update"},
	{OM_UPGRADE,		"opkg upgrade"},
};

const char *pkg_menu_names[] = {
	"List All",
	"List Installed",
	"List Upgradable",
	"Update Package List",
	"Upgrade System",
};

const char * cmd_names[] = {
	"list",
	"list-installed",
	"list-upgradable",
	"update",
	"upgrade",
};

int COPKGManager::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "COPKGManager::exec: actionKey:%s\n", actionKey.c_str());

	int   res = RETURN_REPAINT;

	if (parent)
		parent->hide();
	
	// internet
	if(actionKey == "internet")
	{
		if(!showPkgMenu(OM_LIST)) 
		{
			return RETURN_REPAINT;
		}
	}
	
	// manual
	if(actionKey == "manual")
	{
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("ipk");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec(g_settings.update_dir) == true)
		{
			filename = fileBrowser.getSelectedFile()->Name;
		}
		else
			return RETURN_REPAINT;
	}
	
	// install
	if(!filename.empty())
	{
		std::string success;
		std::string action_name = "opkg -V 1 install " + getBlankPkgName(filename);
		
		if( !system(action_name.c_str()))
		{
				success = getBlankPkgName(filename) + " successfull installed";
				
				MessageBox(_("Information"), _(success.c_str()), mbrBack, mbBack, NEUTRINO_ICON_INFO);
		}
		else
		{
				success = getBlankPkgName(filename) + " install failed";
				MessageBox(_("Error"), _(success.c_str()), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
		}
		
		// remove filename.ipk
		if(actionKey == "manual")
			remove(getBlankPkgName(filename).c_str());
	}
	
	res = showMenu(); 
	
	return res;
}

class CUpdateMenuTarget : public CMenuTarget
{
	int    myID;
	int *  myselectedID;

	public:
		CUpdateMenuTarget(const int id, int * const selectedID)
		{
			myID = id;
			myselectedID = selectedID;
		}

		virtual int exec(CMenuTarget *, const std::string &)
		{
			*myselectedID = myID;
			return RETURN_EXIT_ALL;
		}
};

//show items
bool COPKGManager::showPkgMenu(const int pkg_content_id)
{
	int selected = -1;
	std::vector<std::string> urls;
	CHintBox * loadingBox;

	loadingBox = new CHintBox(_("Information"), _("Loading package list"));	// UTF-8	
	loadingBox->paint();

	// update list
	if(!execCmd(pkg_types[OM_UPDATE].cmdstr))
	{
		loadingBox->hide();
		delete loadingBox;
		MessageBox(_("Error"), _("Update failed"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);

		return false;
	}
		
	CMenuWidget menu(_("OPKG-Manager"), NEUTRINO_ICON_UPDATE, MENU_WIDTH + 50);
	
	if(!getPkgData(pkg_content_id))
	{
		loadingBox->hide();
		delete loadingBox;
		return false;
	}
	
	if (vp_pkg_menu)
	{
		for(uint i = 0; i < vp_pkg_menu->size(); i++)
		{
			printf("Update to %s\n", vp_pkg_menu->at(i).c_str());
			urls.push_back(vp_pkg_menu->at(i));

			menu.addItem( new ClistBoxItem(vp_pkg_menu->at(i).c_str(), true, NULL, new CUpdateMenuTarget(i, &selected), NULL, NULL, NEUTRINO_ICON_UPDATE_SMALL));
		}
	}

	loadingBox->hide();
	
	if (urls.empty())
	{
		HintBox(_("Error"), _("can't get update list")); // UTF-8
		delete loadingBox;
		return false;
	}
	
	menu.exec(NULL, "");
	if (selected == -1)
	{
		delete loadingBox;
		return false;
	}
	
	delete loadingBox;
	
	filename = urls[selected];
	
	printf("%s: %s\n", __FUNCTION__, filename.c_str());
	
	return true;
}

int COPKGManager::showMenu()
{
	dprintf(DEBUG_NORMAL, "COPKGManager::showMenu\n");

	CMenuWidget * menu = new CMenuWidget("OPKG-Manager", NEUTRINO_ICON_UPDATE);

	menu->setWidgetMode(MODE_SETUP);
	menu->enableShrinkMenu();

	menu->addItem(new ClistBoxItem(_("Online Software Manager"), true, NULL, this, "internet" ));
	menu->addItem(new ClistBoxItem(_("Manuell(ftp) Software Manager"), true, NULL, this, "manual" ));

	int res = menu->exec (NULL, "");
	menu->hide ();
	delete menu;
	
	return res;
}

//returns true if opkg support is available
bool COPKGManager::hasOpkgSupport()
{
	std::string deps[] = {"/bin/opkg-cl", "/bin/opkg-key", "/etc/opkg/opkg.conf", "/var/lib/opkg"};
	bool ret = true;
	
	for (uint i = 0; i < (sizeof(deps) / sizeof(deps[0])); i++)
	{
		if(access(deps[i].c_str(), R_OK) !=0)
		{
			printf("[neutrino opkg] %s not found\n", deps[i].c_str());
			ret = false;
		}
	}
	
	return ret;
}

bool COPKGManager::getPkgData(const int pkg_content_id)
{
	char cmd[100];
	FILE * output;
	char buf[OM_MAX_LINE_LENGTH];
	//setbuf(output, NULL);
	int in, pos;
	bool is_pkgline;
	pos = 0;
	
	switch (pkg_content_id) 
	{
		case OM_LIST: //list of pkgs
		{
			v_pkg_list.clear();
			//vp_pkg_menu = &v_pkg_list;
			break;
		}
		
		case OM_LIST_INSTALLED: //installed pkgs
		{
			v_pkg_installed.clear();
			//vp_pkg_menu = &v_pkg_installed;
			break;
		}
		
		case OM_LIST_UPGRADEABLE:
		{
			v_pkg_upgradable.clear();
			//vp_pkg_menu = &v_pkg_upgradable;
			break;
		}
		
		default:
			//vp_pkg_menu = NULL;
			printf("unknown content id! \n\t");
			break;
	}
	
	// dump output to /tmp
	sprintf(cmd, "%s > /tmp/%s.list", pkg_types[pkg_content_id].cmdstr, cmd_names[pkg_content_id]);
	
	printf("COPKGManager: executing >> %s\n", cmd);
	
	system(cmd);
	
	// check if cmd executed
	char buffer[255];
	sprintf(buffer, "/tmp/%s.list", cmd_names[pkg_content_id]);
	
	output = fopen(buffer, "r");

	if(output != NULL)
	{
		while (true)
		{
			in = fgetc(output);
			if (in == EOF)
				break;

			buf[pos] = (char)in;
			if (pos == 0)
				is_pkgline = ((in != ' ') && (in != '\t'));
			
			// avoid buffer overflow
			if (pos + 1 > OM_MAX_LINE_LENGTH)
				in = '\n';
			else
				pos++;
			buf[pos] = 0;
			
			if (in == '\b' || in == '\n')
			{
				// start a new line
				pos = 0;
				if ((in == '\n') && is_pkgline)
				{
					//clean up string
					int ipos = -1;
					std::string line = buf;
					while( (ipos = line.find('\n')) != -1 )
						line = line.erase(ipos, 1);
									
					//add to lists
					switch (pkg_content_id) 
					{
						case OM_LIST: //list of pkgs
						{
							v_pkg_list.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						case OM_LIST_INSTALLED: //installed pkgs
						{
							v_pkg_installed.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						case OM_LIST_UPGRADEABLE:
						{
							v_pkg_upgradable.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						default:
							printf("unknown output! \n\t");
							printf("%s\n", buf);
							break;
					}
				}
			}
		}

		fclose(output);
	}
	else
		return false;
	
	unlink(buffer);
	
	switch (pkg_content_id) 
	{
		case OM_LIST: //list of pkgs
		{
			//v_pkg_list.clear();
			vp_pkg_menu = &v_pkg_list;
			break;
		}
		
		case OM_LIST_INSTALLED: //installed pkgs
		{
			//v_pkg_installed.clear();
			vp_pkg_menu = &v_pkg_installed;
			break;
		}
		
		case OM_LIST_UPGRADEABLE:
		{
			//v_pkg_upgradable.clear();
			vp_pkg_menu = &v_pkg_upgradable;
			break;
		}
		
		default:
			//vp_pkg_menu = NULL;
			printf("unknown content id! \n\t");
			break;
	}
	
	return true;
}

std::string COPKGManager::getBlankPkgName(const std::string& line)
{
	int l_pos = line.find(" ");
	std::string name = line.substr(0, l_pos);
	
	return name;
}

bool COPKGManager::execCmd(const char * cmdstr)
{
	char cmd[100];

	snprintf(cmd, sizeof(cmd),"%s", cmdstr);
	
	printf("COPKGManager: executing %s\n", cmd);
	
	if(!system(cmd))
	{
		MessageBox(_("Error"), _("Command failed"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
		sleep(2);
		return false;
	}

	return true;
}

void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	// class handler
	COPKGManager * opkgHandler = new COPKGManager;
	
	opkgHandler->exec(NULL, "");
	
	delete opkgHandler;
	opkgHandler = NULL;
}


