/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#include <gui/update.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/filebrowser.h>
#include <system/fsmounter.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <system/flashtool.h>
#include <system/httptool.h>
#include <system/helpers.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/vfs.h>

#include <fstream>

#include <system/debug.h>


#define gTmpPath 					"/var/tmp/"
#define gUserAgent 					"neutrino/softupdater 1.0"

#define LIST_OF_UPDATES_LOCAL_FILENAME 			"update.list"
#define RELEASE_CYCLE                  			PACKAGE_VERSION
#define RELEASE_TYPE					"Snapshot" // FIXME:
#define FILEBROWSER_UPDATE_FILTER      			"img"

#define MTD_OF_WHOLE_IMAGE             			0

//FIXME: add the right mtd part (meaned is roofs, on some boxes this contains also kernel) for your boxtype bevor u use this
//NOTE: be carefull with this
#if defined (PLATFORM_DGS)	
#define MTD_DEVICE_OF_UPDATE_PART      			"/dev/mtd5"
#elif defined (PLATFORM_GIGABLUE_800SE)
#define MTD_DEVICE_OF_UPDATE_PART      			"/dev/mtd0"
#else
#define MTD_DEVICE_OF_UPDATE_PART      			"/dev/mtd5"
#endif


CFlashUpdate::CFlashUpdate(int uMode)
{
	progressWindow = new CProgressWindow();

	updateMode = uMode;
	
	progressWindow->setTitle(("Software update"));
	
	// check rootfs, allow flashing only when rootfs is jffs2/yaffs2/squashfs
	struct statfs s;
	
	if (::statfs("/", &s) == 0) 
	{
		if (s.f_type == 0xEF53L /*ext2*/|| s.f_type == 0x6969L/*nfs*/)
			allow_flash = false;
		else if (s.f_type == 0x72b6L/*jffs2*/ || s.f_type == 0x5941ff53L /*yaffs2*/ || s.f_type == 0x73717368L /*squashfs*/ || s.f_type == 0x24051905L/*ubifs*/)
			allow_flash = true;
	}
	else
		//printf("cant check rootfs\n");
		allow_flash = false;
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

			return CMenuTarget::RETURN_EXIT_ALL;
		}
};

bool CFlashUpdate::selectHttpImage(void)
{
	CHTTPTool httpTool;
	std::string url;
	std::string name;
	std::string version;
	std::string md5;
	std::vector<std::string> updates_lists, urls, names, versions, descriptions, md5s;
	char fileTypes[128];
	int selected = -1;

	//
	httpTool.setTitle(_("Software update"));
	progressWindow->showStatusMessageUTF(_("getting update list")); // UTF-8

	// NOTE: remember me : i dont like this menu GUI
	CWidget* widget = NULL;
	ClistBox* SelectionWidget = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("selecthttpimage");
	
	if (widget)
	{
		SelectionWidget = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, 600, MENU_HEIGHT);
		widget->name = "selecthttpimage";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		SelectionWidget = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		SelectionWidget->setWidgetMode(ClistBox::MODE_SETUP);
		SelectionWidget->enableShrinkMenu();
		
		//
		SelectionWidget->enablePaintHead();
		SelectionWidget->setTitle(_("Available Images/Packages"), NEUTRINO_ICON_UPDATE);
//		SelectionWidget->setHeadLine(true, true);

		//
		SelectionWidget->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		SelectionWidget->setFootButtons(&btn);
//		SelectionWidget->setFootLine(true, true);
		
		//
		widget->addCCItem(SelectionWidget);
	}
	
	// intros
	SelectionWidget->addItem(new CMenuForwarder(_("back")));

	std::ifstream urlFile(g_settings.softupdate_url_file);

	dprintf(DEBUG_NORMAL, "[update] file %s\n", g_settings.softupdate_url_file);

	unsigned int i = 0;
	while (urlFile >> url)
	{
		std::string::size_type startpos, endpos;

		dprintf(DEBUG_NORMAL, "[update] url %s\n", url.c_str());

		// extract domain name 
		startpos = url.find("//");
		if (startpos == std::string::npos)
		{
			startpos = 0;
			endpos   = std::string::npos;
			updates_lists.push_back(url.substr(startpos, endpos - startpos));
		}
		else
		{
			startpos = url.find('/', startpos + 2) + 1;
			endpos   = std::string::npos;
			updates_lists.push_back(url.substr(startpos, endpos - startpos));
		}

		//seperator???
		
		if (httpTool.downloadFile(url, gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME, 20))
		{
			std::ifstream in(gTmpPath LIST_OF_UPDATES_LOCAL_FILENAME);
			
			bool enabled = true; 

			while (in >> url >> version >> md5 >> std::ws)
			{
				urls.push_back(url);
				versions.push_back(version);
				std::getline(in, name);
				names.push_back(name);
				//std::getline(in, md5);
				md5s.push_back(md5);

				dprintf(DEBUG_NORMAL, "[update] url %s version %s md5 %s name %s\n", url.c_str(), version.c_str(), md5.c_str(), name.c_str());

				CFlashVersionInfo versionInfo(versions[i]);

				fileTypes[i] = versionInfo.snapshot;
				std::string description = versionInfo.getType();
				description += ' ';
				description += versionInfo.getDate();
				description += ' ';
				description += versionInfo.getTime();
				
				descriptions.push_back(description);
				
				//if( versionInfo.getType() < '3' && !allow_flash)
				if(!allow_flash && (versionInfo.snapshot < '3'))
					enabled = false;

				SelectionWidget->addItem(new CMenuForwarder(names[i].c_str(), enabled, descriptions[i].c_str(), new CUpdateMenuTarget(i, &selected), NULL, CRCInput::RC_nokey, NEUTRINO_ICON_UPDATE_SMALL ));
				i++;
			}
		}
	}

	progressWindow->hide();

	if (urls.empty())
	{
		HintBox(_("Error"), _("can't get update list")); // UTF-8
		return false;
	}
		
	widget->exec(NULL, "");
	
	if (SelectionWidget)
	{
		delete SelectionWidget;
		SelectionWidget = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}

	if (selected == -1)
		return false;

	filename = urls[selected];
	newVersion = versions[selected];
	file_md5 = md5s[selected];
	fileType = fileTypes[selected];

	dprintf(DEBUG_NORMAL, "[update] filename %s type %c newVersion %s md5 %s\n", filename.c_str(), fileType, newVersion.c_str(), file_md5.c_str());

	return true;
}

bool CFlashUpdate::getUpdateImage(const std::string & version)
{
	CHTTPTool httpTool;
	char * fname, dest_name[100];
	httpTool.setTitle(_("Software update"));

	fname = rindex(const_cast<char *>(filename.c_str()), '/');
	if(fname != NULL) 
		fname++;
	else 
		return false;

	sprintf(dest_name, "%s/%s", g_settings.update_dir, fname);
	progressWindow->showStatusMessageUTF(std::string(_("getting update list")) + ' ' + version); // UTF-8

	dprintf(DEBUG_NORMAL, "get update (url): %s - %s\n", filename.c_str(), dest_name);
	
	return httpTool.downloadFile(filename, dest_name, 40 );
}

bool CFlashUpdate::checkVersion4Update()
{
	char msg[400];
	CFlashVersionInfo * versionInfo;
	const char* msg_body;

	dprintf(DEBUG_NORMAL, "[update] mode is %d\n", updateMode);

	if(updateMode == UPDATEMODE_INTERNET) //internet-update
	{
		// get image/package
		if(!selectHttpImage())
			return false;

		progressWindow->showGlobalStatus(20);
		progressWindow->showStatusMessageUTF(_("checking version")); // UTF-8

		dprintf(DEBUG_NORMAL, "internet version: %s\n", newVersion.c_str());

		progressWindow->showGlobalStatus(20);
		progressWindow->hide();
		
		msg_body = (fileType < '3')? _("Found the following new image:\nDate: %s, %s\nBaseImage: %s\nImageType: %s\n\nDo you want to install this version now?") : _("Found the following new package:\nPackage: %s\nDate: %s, %s\nBaseImage: %s\nType: %s\n\nDo you want to download and install this version now?");
		
		versionInfo = new CFlashVersionInfo(newVersion);	//NOTE: Memory leak: versionInfo
		sprintf(msg, msg_body, filename.c_str(), versionInfo->getDate(), versionInfo->getTime(), versionInfo->getReleaseCycle(), versionInfo->getType());

		// flash
		if(fileType < '3') 
		{
			// check release cycle
			if ((strncmp(RELEASE_CYCLE, versionInfo->getReleaseCycle(), 2) != 0) &&
			(MessageBox(_("Information"), _("Your Release cycle differs.\nTo continue?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes))
			{
				delete versionInfo;
				return false;
			}

			// check if not release ask to install (beta + snapshot)
			if ((strcmp(RELEASE_TYPE, versionInfo->getType()) != 0) &&
		    	    (MessageBox(_("Information"), _("The image you have selected is an untested version, this means\nyour box maybe fail to boot after update.\n\nDo you really want to update to this version?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes))
			{
				delete versionInfo;
				return false;
			}
		}

		delete versionInfo;
	}
	else if(updateMode == UPDATEMODE_MANUAL)// manual update (ftp)
	{
		CFileBrowser UpdatesBrowser;
		CFileFilter UpdatesFilter; 
		
		if(allow_flash) 
			UpdatesFilter.addFilter(FILEBROWSER_UPDATE_FILTER);

		UpdatesFilter.addFilter("bin");
		UpdatesFilter.addFilter("tar");
		UpdatesFilter.addFilter("gz");
		UpdatesFilter.addFilter("txt");

		UpdatesBrowser.Filter = &UpdatesFilter;

		CFile * CFileSelected = NULL;
		
		if (!(UpdatesBrowser.exec(g_settings.update_dir)))
			return false;

		CFileSelected = UpdatesBrowser.getSelectedFile();

		if (CFileSelected == NULL)
			return false;

		filename = CFileSelected->Name;

		FILE * fd = fopen(filename.c_str(), "r");
		if(fd)
			fclose(fd);
		else 
		{
			hide();
			
			dprintf(DEBUG_NORMAL, "flash-file not found: %s\n", filename.c_str());
			
			HintBox(_("Error"), _("can't open file")); // UTF-8
			return false;
		}
		
		progressWindow->hide();
		
		// just only for correct msg
		char * ptr = rindex(const_cast<char *>(filename.c_str()), '.');
		if(ptr) 
		{
			ptr++;

			if( (!strcmp(ptr, "bin")) || (!strcmp(ptr, "tar")) || (!strcmp(ptr, "gz"))) 
				fileType = 'A';
			else if(!strcmp(ptr, "txt")) 
				fileType = 'T';
			else if(!allow_flash) 
				return false;
			else 
				fileType = 0;

			dprintf(DEBUG_NORMAL, "[update] manual file type: %s %c\n", ptr, fileType);
		}

		strcpy(msg, (fileType < '3')? _("manual flash update version checks are currently not supported.\nAre you sure that you wish to install this image?") : _("by manual package update version checks are currently not supported.\nAre you sure that you wish to install this package?") );
		msg_body = (fileType < '3')? _("Found the following new image:\nDate: %s, %s\nBaseImage: %s\nImageType: %s\n\nDo you want to install this version now?") : _("Found the following new package:\nPackage: %s\nDate: %s, %s\nBaseImage: %s\nType: %s\n\nDo you want to download and install this version now?");
	}
	
	return ( (fileType == 'T')? true : MessageBox(_("Information"), msg, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_UPDATE) == CMessageBox::mbrYes); // UTF-8
}

int CFlashUpdate::exec(CMenuTarget * parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CFlashUpdate::exec\n");

	if(parent)
		parent->hide();

	progressWindow->paint();

	if(!checkVersion4Update()) 
	{
		hide();

		return CMenuTarget::RETURN_REPAINT;
	}

	// install
#ifdef ENABLE_LCD
	CVFD::getInstance()->showProgressBar2(0, "checking", 0, "Update Neutrino");
	CVFD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);
#endif

	progressWindow->showGlobalStatus(19);
	progressWindow->paint();
	progressWindow->showGlobalStatus(20);

	// check image version
	if(updateMode == UPDATEMODE_INTERNET) //internet-update
	{
		char * fname = rindex(const_cast<char *>(filename.c_str()), '/') +1;
		char fullname[255];

		if(!getUpdateImage(newVersion)) 
		{
			hide();
			HintBox(_("Error"), _("can't get update list")); // UTF-8
			return CMenuTarget::RETURN_REPAINT;
		}
		
		sprintf(fullname, "%s/%s", g_settings.update_dir, fname);
		filename = std::string(fullname);
	}

	progressWindow->showGlobalStatus(40);

	CFlashTool ft;
	
	// flash image
	if(fileType < '3') 
	{
		ft.setMTDDevice(MTD_DEVICE_OF_UPDATE_PART);
		ft.setStatusViewer(progressWindow);
	}

	// MD5summ check
	progressWindow->showStatusMessageUTF(_("MD5 checking")); // UTF-8
	
	if((updateMode == UPDATEMODE_INTERNET) && !ft.check_md5(filename, file_md5)) 
	{
		// remove flash/package
		remove(filename.c_str());
		progressWindow->hide();
		HintBox(_("Error"), (fileType < '3')? _("image has errors") : _("package has errors")); // UTF-8

		return CMenuTarget::RETURN_REPAINT;
	}
	
	// download or not???
	if(updateMode == UPDATEMODE_INTERNET) 
	{ 
		//internet-update
		if ( MessageBox(_("Information"), (fileType < '3')? _("image has errors") : _("package has errors"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes) // UTF-8
		{
			// remove flash/package
			remove(filename.c_str());
			progressWindow->hide();

			return CMenuTarget::RETURN_REPAINT;
		}
	}

	progressWindow->showGlobalStatus(60);

	// flash/install
	dprintf(DEBUG_NORMAL, "[update] filename %s type %c\n", filename.c_str(), fileType);

	// flash image
	if(fileType < '3') 
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		sleep(2);
		
		// flash it...
		if(!ft.program(filename, 80, 100))
		{
			// remove flash if flashing failed
			remove(filename.c_str());
			progressWindow->hide();
			HintBox(_("Error"), _(ft.getErrorMessage().c_str())); // UTF-8

			return CMenuTarget::RETURN_REPAINT;
		}

		//status anzeigen
		progressWindow->showGlobalStatus(100);
		progressWindow->showStatusMessageUTF(_("Package successfully installed")); // UTF-8

		progressWindow->hide();

		// Unmount all NFS & CIFS volumes
		nfs_mounted_once = false; /* needed by update.cpp to prevent removal of modules after flashing a new cramfs, since rmmod (busybox) might no longer be available */
		CFSMounter::umount();

		HintBox(_("Information"), _("The image was successfully flashed.\nThe box will be rebooted now.")); // UTF-8
		
		ft.reboot();
		sleep(20000);
	}
	else if(fileType == 'T') // display file contents
	{
		FILE* fd = fopen(filename.c_str(), "r");
		if(fd) 
		{
			char * buffer;
			off_t filesize = lseek(fileno(fd), 0, SEEK_END);
			lseek(fileno(fd), 0, SEEK_SET);
			buffer =(char *) malloc(filesize+1);
			fread(buffer, filesize, 1, fd);
			fclose(fd);
			buffer[filesize] = 0;
			MessageBox(_("Information"), buffer, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
			free(buffer);
		}
	}
	else // package 
	{
		char cmd[100];
		const char install_sh[] = "install.sh";
		
		sprintf(cmd, "%s %s %s", install_sh, g_settings.update_dir, filename.c_str());

		if( system(cmd) )
		{
			progressWindow->hide();
			HintBox(_("Error"), _("package install failed")); // UTF-8

			return CMenuTarget::RETURN_REPAINT;
		}
		
		// 100% status
		progressWindow->showGlobalStatus(100);
		
		// show successfull msg :-)
		HintBox(_("Information"), _("Package successfully installed")); // UTF-8
	}
	
	progressWindow->hide();
	
	return CMenuTarget::RETURN_REPAINT;
}

//
CFlashExpert::CFlashExpert()
{
	progressWindow = new CProgressWindow();
	selectedMTD = -1;
}

void CFlashExpert::readmtd(int _readmtd)
{
	char tmp[10];
	sprintf(tmp, "%d", _readmtd);
	std::string filename = "/tmp/mtd";
	filename += tmp;
	filename += ".img"; // US-ASCII (subset of UTF-8 and ISO8859-1)

	if (_readmtd == -1) 
	{
		filename = "/tmp/flashimage.img"; // US-ASCII (subset of UTF-8 and ISO8859-1)
		_readmtd = MTD_OF_WHOLE_IMAGE;
	}
	
	progressWindow->setTitle(_("Reading Flash"));
	progressWindow->paint();
	progressWindow->showGlobalStatus(0);
	progressWindow->showStatusMessageUTF((std::string(_("Read whole image")) + " (" + CMTDInfo::getInstance()->getMTDName(_readmtd) + ')')); // UTF-8
	CFlashTool ft;
	ft.setStatusViewer(progressWindow);
	ft.setMTDDevice(CMTDInfo::getInstance()->getMTDFileName(_readmtd));

	if(!ft.readFromMTD(filename, 100)) 
	{
		progressWindow->showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	} 
	else 
	{
		progressWindow->showGlobalStatus(100);
		progressWindow->showStatusMessageUTF(_("Package successfully installed")); // UTF-8
		char message[500];
		sprintf(message, _("The image was successfully saved \nunder %s."), filename.c_str());
		sleep(1);
		progressWindow->hide();
		HintBox(_("Information"), _(message));
	}
}

void CFlashExpert::writemtd(const std::string & filename, int mtdNumber)
{
	char message[500];

	sprintf(message, _("Do you really want to flash?\n\nIf a error occurs or the image is not\nvalid, the box will not boot after flashing.\n\nImagename: %s\nTarget: %s"), FILESYSTEM_ENCODING_TO_UTF8(std::string(filename).c_str()), CMTDInfo::getInstance()->getMTDName(mtdNumber).c_str());

	if (MessageBox(_("Information"), message, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_UPDATE) != CMessageBox::mbrYes) // UTF-8
		return;

#ifdef ENABLE_LCD
        CVFD::getInstance()->showProgressBar2(0, "checking", 0, "Update Neutrino");
        CVFD::getInstance()->setMode(CLCD::MODE_PROGRESSBAR2);	
#endif // VFD_UPDATE

	progressWindow->setTitle(_("Writing Flash"));
	progressWindow->paint();
	progressWindow->showGlobalStatus(0);
	CFlashTool ft;
	ft.setStatusViewer(progressWindow);
	ft.setMTDDevice( CMTDInfo::getInstance()->getMTDFileName(mtdNumber) );

	if(!ft.program( "/tmp/" + filename, 50, 100)) 
	{
		progressWindow->showStatusMessageUTF(ft.getErrorMessage()); // UTF-8
		sleep(10);
	} 
	else 
	{
		progressWindow->showGlobalStatus(100);
		progressWindow->showStatusMessageUTF(_("Package successfully installed")); // UTF-8
		sleep(1);
		progressWindow->hide();
		HintBox(_("Information"), _("The image was successfully flashed.\nThe box will be rebooted now.")); // UTF-8
		ft.reboot();
	}
}

void CFlashExpert::showMTDSelector(const std::string & actionkey)
{
	//
	CWidget* widget = NULL;
	ClistBox* mtdselector = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("mtdselector");
	
	if (widget)
	{
		mtdselector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "mtdselector";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		mtdselector = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		mtdselector->setWidgetMode(ClistBox::MODE_SETUP);
		mtdselector->enableShrinkMenu();
		
		//
		mtdselector->enablePaintHead();
		mtdselector->setTitle(_("Partition selector"), NEUTRINO_ICON_UPDATE);
//		mtdselector->setHeadLine(true, true);

		//
		mtdselector->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mtdselector->setFootButtons(&btn);
//		mtdselector->setFootLine(true, true);
		
		//
		widget->addCCItem(mtdselector);
	}
	
	// intros
	mtdselector->addItem(new CMenuForwarder(_("Cancel")));
	mtdselector->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	CMTDInfo* mtdInfo = CMTDInfo::getInstance();

	for(int x1 = 0; x1 < (int) mtdInfo->getMTDCount(); x1++) 
	{
		char sActionKey[20];
		sprintf(sActionKey, "%s%d", actionkey.c_str(), x1);

		/* for Cuberevo family boxes */
		/*
		* dev:    size   erasesize  name
		* mtd0: 00040000 00020000 "nor.boot"
		* mtd1: 00020000 00020000 "nor.config_welcome"
		* mtd2: 00200000 00020000 "nor.kernel"
		* mtd3: 013a0000 00020000 "nor.root"
		* mtd4: 00a00000 00020000 "nor.db"
		* mtd5: 015a0000 00020000 "nor.kernel_root"
		* mtd6: 01fa0000 00020000 "nor.kernel_root_db"
		* mtd7: 01fc0000 00020000 "nor.all_noboot"
		* mtd8: 02000000 00020000 "nor.all"
		*/
		/* mtd0 to mtd4 are R_ONLY */
		/* mtd5-mtd8 are RW */
		/* we excluse mtd0(nor.boot) and mtd8(not.all) */
		
		/* giga */
		/*
		root@Giga:~# cat /proc/mtd
		dev:    size   erasesize  name
		mtd0: 07800000 00020000 "rootfs"
		mtd1: 07f00000 00020000 "all"
		mtd2: 00400000 00020000 "kernel"

		*/
		
		if(actionkey == "writemtd")
		{			  
			mtdselector->addItem(new CMenuForwarder(mtdInfo->getMTDName(x1).c_str(), true, NULL, this, sActionKey));
		}
		else if(actionkey == "readmtd")
		{
			mtdselector->addItem(new CMenuForwarder(mtdInfo->getMTDName(x1).c_str(), true, NULL, this, sActionKey));
		}
	}
	
	//
	widget->exec(NULL, "");
	
	if (mtdselector)
	{
		delete mtdselector;
		mtdselector = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CFlashExpert::showFileSelector(const std::string & actionkey)
{
	//
	CWidget* widget = NULL;
	ClistBox* fileselector = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("fileselector");
	
	if (widget)
	{
		fileselector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "fileselector";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		fileselector = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		fileselector->setWidgetMode(ClistBox::MODE_SETUP);
		fileselector->enableShrinkMenu();
		
		//
		fileselector->enablePaintHead();
		fileselector->setTitle(_("File selector"), NEUTRINO_ICON_UPDATE);
//		fileselector->setHeadLine(true, true);

		//
		fileselector->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		fileselector->setFootButtons(&btn);
//		fileselector->setFootLine(true, true);
		
		//
		widget->addCCItem(fileselector);
	}
	
	// intros
	fileselector->addItem(new CMenuForwarder(_("Cancel")));
	fileselector->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	struct dirent **namelist;
	int n = scandir("/tmp", &namelist, 0, alphasort);
	
	if (n < 0)
	{
		perror("no flashimages available");
		//should be available...
	}
	else
	{
		for(int count = 0; count < n; count++)
		{
			std::string filen = namelist[count]->d_name;
			int pos = filen.find(".img");
			
			if(pos != -1)
			{
				fileselector->addItem(new CMenuForwarder(filen.c_str(), true, NULL, this, (actionkey + filen).c_str()));
//#warning TODO: make sure file is UTF-8 encoded
			}
			free(namelist[count]);
		}
		free(namelist);
	}

	//
	widget->exec(NULL, "");
	
	if (fileselector)
	{
		delete fileselector;
		fileselector = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CFlashExpert::exec(CMenuTarget* parent, const std::string & actionKey)
{
	if(parent)
		parent->hide();

	if(actionKey=="readflash") 
	{
		readmtd(-1);
	}
	else if(actionKey=="writeflash") 
	{
		showFileSelector("");
	}
	else if(actionKey=="readflashmtd") 
	{
		showMTDSelector("readmtd");
	}
	else if(actionKey=="writeflashmtd") 
	{
		showMTDSelector("writemtd");
	}
	else 
	{
		int iReadmtd = -1;
		int iWritemtd = -1;
		sscanf(actionKey.c_str(), "readmtd%d", &iReadmtd);
		sscanf(actionKey.c_str(), "writemtd%d", &iWritemtd);
		
		if(iReadmtd != -1) 
		{
			readmtd(iReadmtd);
		}
		else if(iWritemtd != -1) 
		{
			selectedMTD = iWritemtd;
			showFileSelector("");
		} 
		else 
		{
			if(selectedMTD == -1) 
			{
				writemtd(actionKey, MTD_OF_WHOLE_IMAGE);
			} 
			else 
			{
				writemtd(actionKey, selectedMTD);
				selectedMTD=-1;
			}
		}
		hide();

		return CMenuTarget::RETURN_EXIT_ALL;
	}

	hide();
	
	return CMenuTarget::RETURN_REPAINT;
}

//
int CUpdateSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CUpdateSettings::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if (actionKey == "update_dir")
	{
		if(parent)
			parent->hide();
		
		CFileBrowser fileBrowser;
		fileBrowser.Dir_Mode = true;
		
		if (fileBrowser.exec(g_settings.update_dir) == true) 
		{
			const char * newdir = fileBrowser.getSelectedFile()->Name.c_str();
			if(check_dir(newdir))
				printf("CUpdateSettings::exec: Wrong/unsupported update dir %s\n", newdir);
			else
			{
				strcpy(g_settings.update_dir, fileBrowser.getSelectedFile()->Name.c_str());
				
				dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: new update dir %s\n", fileBrowser.getSelectedFile()->Name.c_str());
			}
		}
		
		return ret;
	}
	
	ret = showMenu();
	
	return ret;
}

int CUpdateSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CUpdateSettings::showMenu\n");
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	//
	CWidget* widget = NULL;
	ClistBox* updateSettings = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("updatesetup");
	
	if (widget)
	{
		updateSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "updatesetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		updateSettings = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		updateSettings->setWidgetMode(ClistBox::MODE_SETUP);
		updateSettings->enableShrinkMenu();
		
		//
		updateSettings->enablePaintHead();
		updateSettings->setTitle(_("Software update"), NEUTRINO_ICON_UPDATE);
//		updateSettings->setHeadLine(true, true);

		//
		updateSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		updateSettings->setFootButtons(&btn);
//		updateSettings->setFootLine(true, true);
		
		//
		widget->addCCItem(updateSettings);
	}
		
	// intros
	updateSettings->addItem(new CMenuForwarder(_("back")));
	updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// save settings
	updateSettings->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// 
	CWidget* mtdexpertWidget = NULL;
	ClistBox* mtdexpert = NULL; 
	
	mtdexpertWidget = CNeutrinoApp::getInstance()->getWidget("flashexpert");
	
	if (mtdexpertWidget)
	{
		mtdexpert = (ClistBox*)mtdexpertWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		mtdexpertWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		mtdexpertWidget->name = "flashexpert";
		mtdexpertWidget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		mtdexpert = new ClistBox(mtdexpertWidget->getWindowsPos().iX, mtdexpertWidget->getWindowsPos().iY, mtdexpertWidget->getWindowsPos().iWidth, mtdexpertWidget->getWindowsPos().iHeight);

		mtdexpert->setWidgetMode(ClistBox::MODE_SETUP);
		mtdexpert->enableShrinkMenu();
		
		//
		mtdexpert->enablePaintHead();
		mtdexpert->setTitle(_("Expert functions"), NEUTRINO_ICON_UPDATE);
//		mtdexpert->setHeadLine(true, true);

		//
		mtdexpert->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mtdexpert->setFootButtons(&btn);
//		mtdexpert->setFootLine(true, true);
		
		//
		mtdexpertWidget->addCCItem(mtdexpert);
	}
		
	// intros
	mtdexpert->addItem(new CMenuForwarder(_("back")));
	mtdexpert->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	CFlashExpert * fe = new CFlashExpert();

	// read mtd 
	mtdexpert->addItem(new CMenuForwarder(_("Read one partition"), true, NULL, fe, "readflashmtd"));

	// write mtd
	mtdexpert->addItem(new CMenuForwarder(_("Write one partition"), true, NULL, fe, "writeflashmtd"));

	// experten function
	//FIXME: allow update only when the rootfs is jffs2/squashfs
	updateSettings->addItem(new CMenuForwarder(_("Expert-functions"), true, NULL, mtdexpertWidget));
	updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// update dir
	updateSettings->addItem( new CMenuForwarder(_("Directory for updates"), true, g_settings.update_dir , this, "update_dir") );
	
	// url
	CStringInputSMS * updateSettings_url_file = new CStringInputSMS(_("Software update URL"), g_settings.softupdate_url_file);
	updateSettings->addItem(new CMenuForwarder(_("Software update URL"), true, g_settings.softupdate_url_file, updateSettings_url_file));

	// show current version
	updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Current version")));

	// get current version SBBB YYYY MM TT HH MM -- formatsting
	//CConfigFile lconfigfile('\t');

	//const char * versionString = (lconfigfile.loadConfig("/etc/.version")) ? (lconfigfile.getString( "version", "1201201205091849").c_str()) : "1201201602031021";

	//dprintf(DEBUG_INFO, "CNeutrinoApp::InitServiceSettings: current flash-version: %s\n", versionString);

	//static CFlashVersionInfo versionInfo(versionString);

	// release cycle
	updateSettings->addItem(new CMenuForwarder(_("Release cycle"), false, /*versionInfo.getReleaseCycle()*/ RELEASE_CYCLE));
		
	// date
	updateSettings->addItem(new CMenuForwarder(_("Date"), false, /*versionInfo.getDate()*/ __DATE__ ));
		
	// time
	updateSettings->addItem(new CMenuForwarder(_("Time"), false, /*versionInfo.getTime()*/ __TIME__));
		
	// type
	// versionInfo.getType() returns const char * which is never deallocated
	updateSettings->addItem(new CMenuForwarder(_("ImageType"), false, /*versionInfo.getType()*/ "Snapshot" ));

	// check update
	//FIXME: allow update only when the rootfs is jffs2/squashfs
	updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// offline
	updateSettings->addItem(new CMenuForwarder(_("Manuell(ftp) Software Manager"), true, NULL, new CFlashUpdate(CFlashUpdate::UPDATEMODE_MANUAL)));

	// online
	updateSettings->addItem(new CMenuForwarder(_("Online Software Manager"), true, NULL, new CFlashUpdate(CFlashUpdate::UPDATEMODE_INTERNET)));
	
	//
	res = widget->exec(NULL, "");
	
	if (mtdexpert)
	{
		delete mtdexpert;
		mtdexpert = NULL;
	}
	
	if (mtdexpertWidget)
	{
		delete mtdexpertWidget;
		mtdexpertWidget = NULL;
	}
	
	if (updateSettings)
	{
		delete updateSettings;
		updateSettings = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	delete fe;
	
	return res;
}

