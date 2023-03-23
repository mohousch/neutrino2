/*
	Neutrino-GUI  -   DBoxII-Project

	NFSMount/Umount GUI by Zwen

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

#include <global.h>
#include <neutrino2.h>

#include <gui/nfs.h>

#include <gui/filebrowser.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <system/debug.h>

#include <fstream>

#include <errno.h>
#include <pthread.h>
#include <sys/mount.h>
#include <unistd.h>

#include <driver/encoding.h>


class CNFSMountGuiNotifier : public CChangeObserver
{
	private:
		ClistBoxItem *m_opt1, *m_opt2, *m_user, *m_pass;
		int *m_type;
		
	public:
		CNFSMountGuiNotifier(ClistBoxItem *a3, ClistBoxItem *a4 , int *type)
		{
			m_user = a3;
			m_pass = a4;
			m_type = type;
		}
		
		bool changeNotify(const std::string& /*OptionName*/, void *)
		{
			if(*m_type == (int)CFSMounter::NFS)
			{
				m_user->setActive(false);
				m_pass->setActive(false);
			}
			else
			{
				m_user->setActive(true);
				m_pass->setActive(true);
			}
			
			return true;
		}
};

CNFSMountGui::CNFSMountGui()
{
      //#warning move probing from exec() to fsmounter
	m_nfs_sup = CFSMounter::FS_UNPROBED;
	m_cifs_sup = CFSMounter::FS_UNPROBED;
	m_lufs_sup = CFSMounter::FS_UNPROBED;
	m_smbfs_sup = CFSMounter::FS_UNPROBED;
}

const char * nfs_entry_printf_string[4] =
{
	"NFS %s:%s -> %s auto: %4s",
	"CIFS //%s/%s -> %s auto: %4s",
	"FTPFS %s/%s -> %s auto: %4s",
	"SMBFS //%s%s -> %s auto: %4s"
};

int CNFSMountGui::exec( CMenuTarget *parent, const std::string &actionKey )
{
	dprintf(DEBUG_NORMAL, "CNFSMountGui::exec: actionKey:%s\n", actionKey.c_str());
	
	int returnval = RETURN_REPAINT;
	
	if (m_nfs_sup == CFSMounter::FS_UNPROBED)
		m_nfs_sup = CFSMounter::fsSupported(CFSMounter::NFS);

	if (m_cifs_sup == CFSMounter::FS_UNPROBED)
		m_cifs_sup = CFSMounter::fsSupported(CFSMounter::CIFS);

	if (m_lufs_sup == CFSMounter::FS_UNPROBED)
		m_lufs_sup = CFSMounter::fsSupported(CFSMounter::LUFS);
	
	if (m_smbfs_sup == CFSMounter::FS_UNPROBED)
		m_smbfs_sup = CFSMounter::fsSupported(CFSMounter::SMBFS);

	dprintf(DEBUG_NORMAL, "SUPPORT: NFS: %d, CIFS: %d, LUFS: %d, SMBFS: %d\n", m_nfs_sup, m_cifs_sup, m_lufs_sup, m_smbfs_sup);

	if (actionKey.empty())
	{
		if(parent)
			parent->hide();
		
		for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) CFSMounter::NFS) ? 0 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::CIFS) ? 1 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::SMBFS) ? 3 : 2))],
				g_settings.network_nfs_ip[i].c_str(),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_dir[i]),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_local_dir[i]),
				g_settings.network_nfs_automount[i] ? _("Yes") : _("No"));
		}
		returnval = menu();
	}
	else if(actionKey.substr(0,10) == "mountentry")
	{
		if(parent)
			parent->hide();
		
		returnval = menuEntry(actionKey[10]-'0');
		
		for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			sprintf(m_entry[i],
				nfs_entry_printf_string[(g_settings.network_nfs_type[i] == (int) CFSMounter::NFS) ? 0 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::CIFS) ? 1 : ((g_settings.network_nfs_type[i] == (int) CFSMounter::SMBFS) ? 3 : 2))],
				g_settings.network_nfs_ip[i].c_str(),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_dir[i]),
				FILESYSTEM_ENCODING_TO_UTF8(g_settings.network_nfs_local_dir[i]),
				g_settings.network_nfs_automount[i] ? _("Yes") : _("No"));
			sprintf(ISO_8859_1_entry[i], "%s", UTF8_to_Latin1(m_entry[i]).c_str());
		}
	}
	else if(actionKey.substr(0, 7) == "domount")
	{
		int nr = atoi(actionKey.substr(7,1).c_str());
		CFSMounter::mount(g_settings.network_nfs_ip[nr].c_str(), g_settings.network_nfs_dir[nr], 
				  g_settings.network_nfs_local_dir[nr], (CFSMounter::FSType) g_settings.network_nfs_type[nr],
				  g_settings.network_nfs_username[nr], g_settings.network_nfs_password[nr],
				  g_settings.network_nfs_mount_options1[nr], g_settings.network_nfs_mount_options2[nr]);
		// TODO show msg in case of error
		returnval = RETURN_EXIT;
	}
	else if(actionKey.substr(0, 3) == "dir")
	{
		if(parent)
			parent->hide();
		
		int nr = atoi(actionKey.substr(3,1).c_str());
		CFileBrowser b;
		b.Dir_Mode = true;

		if (b.exec(g_settings.network_nfs_local_dir[nr]))
			strcpy(g_settings.network_nfs_local_dir[nr], b.getSelectedFile()->Name.c_str());

		returnval = RETURN_REPAINT;
	}
	
	return returnval;
}

int CNFSMountGui::menu()
{
	//
	CWidget* widget = NULL;
	ClistBox* mountMenuW = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("nfs");
	
	if (widget)
	{
		mountMenuW = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		mountMenuW = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		mountMenuW->setWidgetMode(MODE_SETUP);
		mountMenuW->enableShrinkMenu();
		
		mountMenuW->enablePaintHead();
		mountMenuW->setTitle(_("Network settings"), NEUTRINO_ICON_NETWORK);

		mountMenuW->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mountMenuW->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "nfs";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(mountMenuW);
	}
	
	mountMenuW->clearItems();
	
	// intros
	mountMenuW->addItem(new ClistBoxItem(_("back")));
	mountMenuW->addItem(new CMenuSeparator(LINE));
	char s2[12];

	for(int i = 0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		sprintf(s2, "mountentry%d", i);
		sprintf(ISO_8859_1_entry[i], "%s", UTF8_to_Latin1(m_entry[i]).c_str());
		ClistBoxItem *forwarder = new ClistBoxItem("", true, ISO_8859_1_entry[i], this, s2);
		
		if (CFSMounter::isMounted(g_settings.network_nfs_local_dir[i]))
		{
			forwarder->iconName = NEUTRINO_ICON_MOUNTED;
		} 
		else
		{
			forwarder->iconName = NEUTRINO_ICON_NOT_MOUNTED;
		}
		mountMenuW->addItem(forwarder);
	}
	
	int ret = widget->exec(this, "");

	return ret;
}

//#warning MESSAGEBOX_NO_YES_XXX is defined in neutrino.cpp, too!
#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("No")  },
	{ 1, _("Yes") }
};

#define NFS_TYPE_OPTION_COUNT 4
const keyval NFS_TYPE_OPTIONS[NFS_TYPE_OPTION_COUNT] =
{
	{ CFSMounter::NFS , _("NFS")  },
	{ CFSMounter::CIFS, _("CIFS") },
	{ CFSMounter::LUFS, _("LUFS") },
	{ CFSMounter::SMBFS, _("SMBFS") }
};

int CNFSMountGui::menuEntry(int nr)
{
	char *dir, *local_dir, *username, *password, *options1, *options2, *mac;
	int *automount;
	int *type;
	char cmd[9];
	char cmd2[9];

	dir = g_settings.network_nfs_dir[nr];
	local_dir = g_settings.network_nfs_local_dir[nr];
	username = g_settings.network_nfs_username[nr];
	password = g_settings.network_nfs_password[nr];
	automount = &g_settings.network_nfs_automount[nr];
	type = &g_settings.network_nfs_type[nr];
	options1 = g_settings.network_nfs_mount_options1[nr];
	options2 = g_settings.network_nfs_mount_options2[nr];
	mac = g_settings.network_nfs_mac[nr];

	sprintf(cmd, "domount%d", nr);
	sprintf(cmd2, "dir%d", nr);
	
	/* rewrite fstype in new entries */
	if(strlen(local_dir) == 0)
	{
		if(m_cifs_sup != CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_lufs_sup == CFSMounter::FS_UNSUPPORTED && m_smbfs_sup == CFSMounter::FS_UNSUPPORTED)
			*type = (int) CFSMounter::CIFS;

		else if(m_lufs_sup != CFSMounter::FS_UNSUPPORTED && m_cifs_sup == CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_smbfs_sup == CFSMounter::FS_UNSUPPORTED)
			*type = (int) CFSMounter::LUFS;

		else if(m_smbfs_sup != CFSMounter::FS_UNSUPPORTED && m_cifs_sup == CFSMounter::FS_UNSUPPORTED && m_nfs_sup == CFSMounter::FS_UNSUPPORTED && m_lufs_sup == CFSMounter::FS_UNSUPPORTED)
			*type = (int) CFSMounter::SMBFS;
	}
	
	bool typeEnabled = (m_cifs_sup != CFSMounter::FS_UNSUPPORTED && m_nfs_sup != CFSMounter::FS_UNSUPPORTED && m_lufs_sup != CFSMounter::FS_UNSUPPORTED && m_smbfs_sup != CFSMounter::FS_UNSUPPORTED) ||
	   (m_cifs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::CIFS) ||
	   (m_nfs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::NFS) ||
	   (m_lufs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::LUFS) ||
	   (m_smbfs_sup != CFSMounter::FS_UNSUPPORTED && *type != (int)CFSMounter::SMBFS);
	
	//
	CWidget* widget = NULL;
	ClistBox* mountMenuEntryW = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("mountvolume");
	
	if (widget)
	{
		mountMenuEntryW = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		mountMenuEntryW = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		//mountMenuEntryW->setMenuPosition(MENU_POSITION_CENTER);
		mountMenuEntryW->setWidgetMode(MODE_SETUP);
		mountMenuEntryW->enableShrinkMenu();
		
		mountMenuEntryW->enablePaintHead();
		mountMenuEntryW->setTitle(_("Mount network volume"), NEUTRINO_ICON_NETWORK);

		mountMenuEntryW->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mountMenuEntryW->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "mountvolume";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(mountMenuEntryW);
	}
	
	mountMenuEntryW->clearItems();
	
	// intros
	mountMenuEntryW->addItem(new ClistBoxItem(_("back")));
	mountMenuEntryW->addItem(new CMenuSeparator(LINE));
	
	// ip
	CIPInput ipInput(_("Server IP"), g_settings.network_nfs_ip[nr], _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	
	// dir
	CStringInputSMS dirInput(_("directory/share"), dir);
	
	// automount
	CMenuOptionChooser *automountInput= new CMenuOptionChooser(_("mount on startup"), automount, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);
	
	// option1
	CStringInputSMS options1Input(_("Mount options"), options1);
	ClistBoxItem *options1_fwd = new ClistBoxItem(_("Mount options"), true, options1, &options1Input);
	
	// option2
	CStringInputSMS options2Input(_("Mount options"), options2);
	ClistBoxItem *options2_fwd = new ClistBoxItem(_("Mount options"), true, options2, &options2Input);
	
	// username
	CStringInputSMS userInput(_("username"), username);
	ClistBoxItem *username_fwd = new ClistBoxItem(_("username"), (*type==CFSMounter::CIFS || CFSMounter::LUFS), username, &userInput);
	
	// password
	CStringInputSMS passInput(_("password"), password);
	ClistBoxItem *password_fwd = new ClistBoxItem(_("password"), (*type==CFSMounter::CIFS || CFSMounter::LUFS), NULL, &passInput);
	
	// mac
	CMACInput * macInput = new CMACInput(_("MAC address"),  g_settings.network_nfs_mac[nr], _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	ClistBoxItem * macInput_fwd = new ClistBoxItem(_("MAC address"), true, g_settings.network_nfs_mac[nr], macInput);

	CNFSMountGuiNotifier notifier(username_fwd, password_fwd, type);

	mountMenuEntryW->addItem(new CMenuOptionChooser(_("type"), type, NFS_TYPE_OPTIONS, NFS_TYPE_OPTION_COUNT, typeEnabled, &notifier));
	mountMenuEntryW->addItem(new ClistBoxItem(_("Server IP"), true, g_settings.network_nfs_ip[nr].c_str(), &ipInput));
	mountMenuEntryW->addItem(new ClistBoxItem(_("Dir"), true, dir, &dirInput));
	mountMenuEntryW->addItem(new ClistBoxItem(_("local dir"), true, local_dir, this, cmd2));
	mountMenuEntryW->addItem(automountInput);
	mountMenuEntryW->addItem(options1_fwd);
	mountMenuEntryW->addItem(options2_fwd);
	mountMenuEntryW->addItem(username_fwd);
	mountMenuEntryW->addItem(password_fwd);
	mountMenuEntryW->addItem(macInput_fwd);
	mountMenuEntryW->addItem(new ClistBoxItem(_("mount now"), true, NULL, this, cmd ));

	int ret = widget->exec(this, "");
	return ret;
}

int CNFSUmountGui::exec( CMenuTarget *parent, const std::string &actionKey )
{
	dprintf(DEBUG_NORMAL, "CNFSUmountGui::exec: actionKey:%s\n", actionKey.c_str());

	int returnval;

	if (actionKey.empty())
	{
		if(parent)
			parent->hide();
		
		returnval = menu();
	}
	else if(actionKey.substr(0,8) == "doumount")
	{
		CFSMounter::umount((actionKey.substr(9)).c_str());
		returnval = RETURN_EXIT;
	}
	else
		returnval = RETURN_REPAINT;

	return returnval;
}

int CNFSUmountGui::menu()
{
	int count = 0;
	CFSMounter::MountInfos infos;
	
	//
	CWidget* widget = NULL;
	ClistBox* umountMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("umountvolume");
	
	if (widget)
	{
		umountMenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		umountMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		//umountMenu->setMenuPosition(MENU_POSITION_CENTER);
		umountMenu->setWidgetMode(MODE_SETUP);
		umountMenu->enableShrinkMenu();
		
		umountMenu->enablePaintHead();
		umountMenu->setTitle(_("Umount network volume"), NEUTRINO_ICON_NETWORK);

		umountMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		umountMenu->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "umountvolume";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(umountMenu);
	}
	
	umountMenu->clearItems();
	
	// intros
	umountMenu->addItem(new ClistBoxItem(_("back")));
	umountMenu->addItem(new CMenuSeparator(LINE));
	
	CFSMounter::getMountedFS(infos);
	for (CFSMounter::MountInfos::const_iterator it = infos.begin(); it != infos.end();it++)
	{
		if(it->type == "nfs" || it->type == "cifs" || it->type == "lufs" || it->type == "smbfs")
		{
			count++;
			std::string s1 = it->device;
			s1 += " -> ";
			s1 += it->mountPoint;
			std::string s2 = "doumount ";
			s2 += it->mountPoint;
			ClistBoxItem *forwarder = new ClistBoxItem(s1.c_str(), true, NULL, this, s2.c_str());
			forwarder->iconName = NEUTRINO_ICON_MOUNTED;
			umountMenu->addItem(forwarder);
		}
	}
	if(infos.size() > 0)
		return widget->exec(this, "");
	else
		return RETURN_REPAINT;
}

// CNFSSmallMenu
CNFSSmallMenu::CNFSSmallMenu()
{
}

CNFSSmallMenu::~CNFSSmallMenu()
{
}

int CNFSSmallMenu::exec( CMenuTarget* parent, const std::string & actionKey )
{
	dprintf(DEBUG_NORMAL, "CNFSSmallMenu::exec: actionKey:%s\n", actionKey.c_str());

	if (actionKey.empty())
	{
		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("nfssmall");
		
		if (widget)
		{
			menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
		}
		else
		{
			menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
			//menu->setMenuPosition(MENU_POSITION_CENTER);
			menu->setWidgetMode(MODE_SETUP);
			menu->enableShrinkMenu();
			
			menu->enablePaintHead();
			menu->setTitle(_("Network Mount Manager"), NEUTRINO_ICON_NETWORK);

			menu->enablePaintFoot();
				
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
			menu->setFootButtons(&btn);
			
			//
			widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			widget->name = "nfssmall";
			widget->setMenuPosition(MENU_POSITION_CENTER);
			widget->addWidgetItem(menu);
		}
		
		menu->clearItems();

		CNFSMountGui mountGui;
		CNFSUmountGui umountGui;
		
		// intros
		menu->addItem(new ClistBoxItem(_("back")));
		menu->addItem(new CMenuSeparator(LINE));
		
		menu->addItem(new ClistBoxItem(_("remount"), true, NULL, this, "remount"));
		menu->addItem(new ClistBoxItem(_("mount"), true, NULL, &mountGui));
		menu->addItem(new ClistBoxItem(_("umount"), true, NULL, &umountGui));

		return widget->exec(parent, actionKey);
	}
	else if(actionKey.substr(0, 7) == "remount")
	{
		//umount automount dirs
		for(int i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++)
		{
			if(g_settings.network_nfs_automount[i])
				umount2(g_settings.network_nfs_local_dir[i],MNT_FORCE);
		}

		CFSMounter::automount();
		return RETURN_REPAINT;
	}

	return RETURN_REPAINT;
}

const char * mntRes2Str(CFSMounter::MountRes res)
{
	switch(res)
	{
		case CFSMounter::MRES_FS_NOT_SUPPORTED:
			return _("filesystem type not supported");
			break;
			
		case CFSMounter::MRES_FS_ALREADY_MOUNTED:
			return _("directory already mounted");
			break;
			
		case CFSMounter::MRES_TIMEOUT:
			return _("mount error: timeout");
			break;
			
		case CFSMounter::MRES_UNKNOWN:
			return _("mount error");
			break;
			
		case CFSMounter::MRES_OK:
			return _("mount successful");
			break;
			
		default:
			return "";
			break;
	}
}

const char * mntRes2Str(CFSMounter::UMountRes res)
{
	switch(res)
	{
		case CFSMounter::UMRES_ERR:
			return _("error umounting volume");
			break;
			
		case CFSMounter::UMRES_OK:
			return "";
			break;
			
		default:
			return "";
			break;
	}
}


