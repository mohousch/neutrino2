/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: hdd_menu.cpp 2013/10/12 mohousch Exp $

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mount.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/progresswindow.h>
#include <gui/widget/infobox.h>

#include <system/settings.h>
#include <system/debug.h>
#include <system/helpers.h>

#include <pictureviewer.h>
#include <movieplayer.h>

#include <gui/audioplayer.h>
#include <gui/hdd_menu.h>
#include <gui/movieinfo.h>

#include <blkid/blkid.h>
#include <mntent.h>

#include <system/tmdbparser.h>


#define HDD_NOISE_OPTION_COUNT 4
const keyval HDD_NOISE_OPTIONS[HDD_NOISE_OPTION_COUNT] =
{
	{ 0,   _("off") },
	{ 128, _("slow") },
	{ 190, _("middle") },
	{ 254, _("fast") }
};

#define HDD_SLEEP_OPTION_COUNT 7
const keyval HDD_SLEEP_OPTIONS[HDD_SLEEP_OPTION_COUNT] =
{
	{0, _("off")},
	{12, "1 min."},
	{60, "5 min."},
	{120, "10 min."},
	{240, "20 min."},
	{241, "30 min."},
	{242, "60 min."}
};

static int my_filter(const struct dirent * dent)
{
	if(dent->d_name[0] == 's' && dent->d_name[1] == 'd')
		return 1;
	
	return 0;
}

/* return 1 if mounted and 0 if not mounted */
static int check_if_mounted(char * dev)
{
	char buffer[255];
	FILE *f = fopen("/proc/mounts", "r");
	
	if(f != NULL)
	{
		while (fgets (buffer, 255, f) != NULL) 
		{
			if(strstr(buffer, dev)) 
			{
				fclose(f);
				return 1;
			}
		}
		fclose(f);
	}

	return 0;
}

int CHDDMenuHandler::exec(CWidgetTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDMenuHandler::exec:\n");

	if (parent)
		parent->hide();
	
	if(actionKey == "activateNow")
	{
		CHintBox * hintBox = new CHintBox(_("Information"), _("Save settings now")); // UTF-8
		hintBox->paint();
		
		CHDDDestExec * hddActiv = new CHDDDestExec();
		hddActiv->exec(NULL, "");
	
		delete hddActiv;
		
		hintBox->hide();
		delete hintBox;
		
		return CWidgetTarget::RETURN_REPAINT;
	}
	
	HDDMenu();
	
	return CWidgetTarget::RETURN_REPAINT;
}

int CHDDMenuHandler::HDDMenu()
{
	FILE * f;
	int fd;
	struct dirent **namelist;
	int ret;
	
	struct stat s;
	int root_dev = -1;
	bool hdd_found = 0;

	int n = scandir("/sys/block", &namelist, my_filter, alphasort);

	if (n < 0) 
	{
                perror("CHDDMenuHandler::doMenu: scandir(\"/sys/block\") failed");

                return CWidgetTarget::RETURN_REPAINT;
        }
	
	//
	CWidget* widget = NULL;
	ClistBox* hddmenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("hddsetup");
	
	if (widget)
	{
		hddmenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "hddsetup";
		
		//
		hddmenu = new ClistBox(&box);

		hddmenu->setWidgetMode(ClistBox::MODE_SETUP);
		hddmenu->enableShrinkMenu();
		
		hddmenu->enablePaintHead();
		hddmenu->setTitle(_("HDD settings"), NEUTRINO_ICON_HDD);

		hddmenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		hddmenu->setFootButtons(&btn);
		
		//
		widget->addCCItem(hddmenu);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("HDD settings"));
	
	hddmenu->addItem(new CMenuForwarder(_("back")));
	hddmenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// save settings
	hddmenu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	hddmenu->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// activate settings
	hddmenu->addItem(new CMenuForwarder(_("Activate settings"), true, NULL, this, "activateNow"));

	// sleep time
	hddmenu->addItem( new CMenuOptionChooser(_("Sleep time"), &g_settings.hdd_sleep, HDD_SLEEP_OPTIONS, HDD_SLEEP_OPTION_COUNT, true));
	
	// noise
	hddmenu->addItem( new CMenuOptionChooser(_("Noise"), &g_settings.hdd_noise, HDD_NOISE_OPTIONS, HDD_NOISE_OPTION_COUNT, true));
	
	// HDDs
	hddmenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	ret = stat("/", &s);
	int drive_mask = 0xfff0;
	
	if (ret != -1) 
	{
		if ((s.st_dev & drive_mask) == 0x0300) 	/* hda, hdb,... has max 63 partitions */
			drive_mask = 0xffc0; 		/* hda: 0x0300, hdb: 0x0340, sda: 0x0800, sdb: 0x0810 */
			
		root_dev = (s.st_dev & drive_mask);
	}
	
	//hdd manage
	CWidget* tempMenuWidget[n];
	ClistBox* tempMenu[n];

	for(int i = 0; i < n; i++) 
	{
		char str[256];
		char vendor[128];
		char model[128];
		int64_t bytes;
		int64_t megabytes;
		int removable = 0;
		bool isroot = false;

		//printf("[neutrino] HDD: checking /sys/block/%s\n", namelist[i]->d_name);
		
		sprintf(str, "/dev/%s", namelist[i]->d_name);
		fd = open(str, O_RDONLY);

		if(fd < 0) 
		{
			printf("[neutrino] HDD: Cant open (1) %s\n", str);
			continue;
		}
		
		if (ioctl(fd, BLKGETSIZE64, &bytes))
			perror("BLKGETSIZE64");
		
		//
		ret = fstat(fd, &s);
		if (ret != -1) 
		{
			if ((int)(s.st_rdev & drive_mask) == root_dev) 
			{
				isroot = true;
			}
		}
                
                close(fd);

		megabytes = bytes/1000000;

		// vendor
		sprintf(str, "/sys/block/%s/device/vendor", namelist[i]->d_name);
		f = fopen(str, "r");

		if(!f) 
		{
			printf("[neutrino] HDD: Cant open (2) %s\n", str);
			continue;
		}
		fscanf(f, "%s", vendor);
		fclose(f);

		// model
		sprintf(str, "/sys/block/%s/device/model", namelist[i]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			printf("[neutrino] HDD: Cant open (3) %s\n", str);
			continue;
		}
		fscanf(f, "%s", model);
		fclose(f);

		// removable
		sprintf(str, "/sys/block/%s/removable", namelist[i]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			printf("[neutrino] HDD: Cant open (4) %s\n", str);
			continue;
		}
		fscanf(f, "%d", &removable);
		fclose(f);

		sprintf(str, "%s (%s-%s %lld %s)", namelist[i]->d_name, vendor, model, megabytes < 10000 ? megabytes : megabytes/1000, megabytes < 10000 ? "MB" : "GB");
		
		bool enabled = !CNeutrinoApp::getInstance()->recordingstatus && !isroot;

		// hdd menu
		tempMenuWidget[i] = CNeutrinoApp::getInstance()->getWidget("temphdd2");
		
		if (tempMenuWidget[i] != NULL)
		{
			tempMenu[i] = (ClistBox*)tempMenuWidget[i]->getCCItem(CComponent::CC_LISTBOX);
			
			tempMenuWidget[i]->setTitle(str, NEUTRINO_ICON_HDD);
		}
		else
		{
			//
			CBox box;
			box.iWidth = MENU_WIDTH;
			box.iHeight = MENU_HEIGHT;
			box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
			box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
			tempMenuWidget[i] = new CWidget(&box);
			
			//
			tempMenu[i] = new ClistBox(tempMenuWidget[i]->getWindowsPos().iX, tempMenuWidget[i]->getWindowsPos().iY, tempMenuWidget[i]->getWindowsPos().iWidth, tempMenuWidget[i]->getWindowsPos().iHeight);

			tempMenu[i]->setWidgetMode(ClistBox::MODE_SETUP);
			tempMenu[i]->enableShrinkMenu();
			
			//
			tempMenu[i]->enablePaintHead();
			tempMenu[i]->setTitle(str, NEUTRINO_ICON_HDD);
			
			//
			tempMenu[i]->enablePaintFoot();		
			const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };		
			tempMenu[i]->setFootButtons(&btn);
			
			//
			tempMenuWidget[i]->addCCItem(tempMenu[i]);
		}
		
		//
		tempMenu[i]->clear();

		
		tempMenu[i]->addItem(new CMenuForwarder(_("back")));
		tempMenu[i]->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
		//init hdd	
		tempMenu[i]->addItem(new CMenuForwarder(_("HDD Init"), enabled, "", new CHDDInit(), namelist[i]->d_name));
		tempMenu[i]->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
		// check for parts
		#define MAX_PARTS 4
		char DEVICE[256];
		char PART[256];
		
		//
		CWidget* PartMenuWidget[MAX_PARTS];
		ClistBox* PartMenu[MAX_PARTS];
		
		for (int j = 1; j <= MAX_PARTS; j++)
		{
			bool mounted = false;
			
			memset(PART, 0, sizeof(PART));
			memset(DEVICE, 0, sizeof(DEVICE));
			
			sprintf(PART, "%s%d", namelist[i]->d_name, j);
			sprintf(DEVICE, "/dev/%s%d", namelist[i]->d_name, j);
			
			fd = open(DEVICE, O_RDONLY);

			if( fd < 0) 
			{
				//printf("[neutrino] HDD: %s not exist\n", DEVICE);
				close(fd);
				continue;
			}
			
			// check if mounted
			mounted = check_if_mounted(DEVICE);
			
			// part submenu
			PartMenuWidget[j] = CNeutrinoApp::getInstance()->getWidget("temphdd");
			
			if (PartMenuWidget[j] != NULL)
			{
				PartMenu[j] = (ClistBox*)PartMenuWidget[j]->getCCItem(CComponent::CC_LISTBOX);
				
				PartMenuWidget[j]->setTitle(PART, NEUTRINO_ICON_HDD);
			}
			else
			{
				//
				CBox box;
				box.iWidth = MENU_WIDTH;
				box.iHeight = MENU_HEIGHT;
				box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
				box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
				PartMenuWidget[j] = new CWidget(&box);
				
				//
				PartMenu[j] = new ClistBox(PartMenuWidget[j]->getWindowsPos().iX, PartMenuWidget[j]->getWindowsPos().iY, PartMenuWidget[j]->getWindowsPos().iWidth, PartMenuWidget[j]->getWindowsPos().iHeight);

				PartMenu[j]->setWidgetMode(ClistBox::MODE_SETUP);
				PartMenu[j]->enableShrinkMenu();
				
				//
				PartMenu[j]->enablePaintHead();
				PartMenu[j]->setTitle(PART, NEUTRINO_ICON_HDD);
				
				//
				PartMenu[j]->enablePaintFoot();		
				const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };		
				PartMenu[j]->setFootButtons(&btn);
				
				//
				PartMenuWidget[j]->addCCItem(PartMenu[j]);
			}
			
			//
			PartMenu[j]->clear();

			//
			PartMenu[j]->addItem(new CMenuForwarder(_("back")));
			PartMenu[j]->addItem(new CMenuSeparator(CMenuSeparator::LINE));
			
			// format part
			PartMenu[j]->addItem(new CMenuForwarder(_("HDD Format"), true, NULL, new CHDDFmtExec(), PART));
			
			// fs check
			PartMenu[j]->addItem(new CMenuForwarder(_("Check filesystem"), true, NULL, new CHDDChkExec(), PART));
			
			// mount part
			PartMenu[j]->addItem(new CMenuForwarder(_("HDD Mount"), true, NULL, new CHDDMountMSGExec(), PART));

			// umount part
			PartMenu[j]->addItem(new CMenuForwarder(_("HDD Umount"), true, NULL, new CHDDuMountMSGExec(), PART));
			
			// hdd explorer
			PartMenu[j]->addItem(new CMenuSeparator(CMenuSeparator::LINE));
			PartMenu[j]->addItem(new CMenuForwarder(_("HDD Filexplorer"), mounted, NULL, new CHDDBrowser(), DEVICE));
			
			// part
			tempMenu[i]->addItem(new CMenuForwarder(PART, true, mounted? _("HDD mounted") : _("HDD umounted"), PartMenuWidget[j]));
			
			close(fd);
		}
		
		hddmenu->addItem(new CMenuForwarder(str, enabled, NULL, tempMenuWidget[i]));

		// result
		hdd_found = 1;
		
		// free
		free(namelist[i]);
	}
	
	if (n >= 0)
                free(namelist);
	
	ret = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	 //
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());

	return ret;
}

//// HDDDestExec
int CHDDDestExec::exec(CWidgetTarget *, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CHDDDestExec::exec:\n");

        char cmd[100];
	char str[256];
	FILE * f;
	int removable = 0;
        struct dirent **namelist;

        int n = scandir("/sys/block", &namelist, my_filter, alphasort);

        if (n < 0)
                return 0;
	
	const char hdparm[] = "/sbin/hdparm";
	bool hdparm_link = false;
	struct stat stat_buf;
	
	if(::lstat(hdparm, &stat_buf) == 0)
		if( S_ISLNK(stat_buf.st_mode) )
			hdparm_link = true;

        for (int i = 0; i < n; i++) 
	{
                removable = 0;
//		printf("CHDDDestExec: checking /sys/block/%s\n", namelist[i]->d_name);
 
		sprintf(str, "/sys/block/%s/removable", namelist[i]->d_name);
		f = fopen(str, "r");
		if (!f)
		{
			printf("Can't open %s\n", str);
			continue;
		}
		fscanf(f, "%d", &removable);
		fclose(f);
		
		if (removable) 
		{
			printf("CHDDDestExec: /dev/%s is not a hdd, no sleep needed\n", namelist[i]->d_name);
		} 
		else 
		{
			//set hdparm for all hdd's
	                printf("CHDDDestExec: noise %d sleep %d /dev/%s\n", g_settings.hdd_noise, g_settings.hdd_sleep, namelist[i]->d_name);

			if(hdparm_link)
			{
				//hdparm -M is not included in busybox hdparm!
	        	        snprintf(cmd, sizeof(cmd), "%s -S%d /dev/%s >/dev/null 2>/dev/null &", hdparm, g_settings.hdd_sleep, namelist[i]->d_name);
			}
			else
			{
	        	        snprintf(cmd, sizeof(cmd), "%s -M%d -S%d /dev/%s >/dev/null 2>/dev/null &", hdparm, g_settings.hdd_noise, g_settings.hdd_sleep, namelist[i]->d_name);
			}
			
        	        system(cmd);
		}

                free(namelist[i]);
        }

        free(namelist);

        return 1;
}


//// hdd init
int CHDDInit::exec(CWidgetTarget * parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDInit::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		parent->hide();

	char cmd[100];
	CHintBox * hintbox;
	int res;
	FILE * f;
	const char *dst = NULL;
	char src[128];
	CProgressWindow * progress;
	bool idone;
	
	sprintf(src, "/dev/%s", actionKey.c_str());

	res = MessageBox(_("HDD Init"), _("Are you sure to init ?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );

	if(res != CMessageBox::mbrYes)
		return RETURN_REPAINT;
	
	// find mount point
	FILE * fstab = setmntent("/etc/mtab", "r");
	struct mntent *e;

	while ((e = getmntent(fstab))) 
	{
		if (strcmp(src, e->mnt_fsname) == 0) 
		{
			dst = e->mnt_dir;
			break;
		}
	}
		
//	printf("mount point is %s\n", dst);
	endmntent(fstab);
		
	// umount /media/sda%
	res = umount(dst);
	printf("CHDDuMountExec: umount res %d\n", res);
	
	progress = new CProgressWindow();
	progress->setTitle(_("HDD Init"));
	progress->paint();
	progress->showStatusMessageUTF(_("HDD Init"));
	progress->showGlobalStatus(0);
	
	const char init[] = "init_hdd.sh";
	sprintf(cmd, "%s /dev/%s > /tmp/%s.txt", init, actionKey.c_str(), init);
//	printf("CHDDInit: executing %s\n", cmd);
	
	system(cmd);
	
	// check if cmd executed
	char buffer[255];
	sprintf(buffer, "/tmp/%s.txt", init);
	
	FILE * output = fopen(buffer, "r");

	if (!output) 
	{
		hintbox = new CHintBox(_("HDD Init"), _("HDD init failed !"));
		hintbox->exec();
		delete hintbox;
		return CWidgetTarget::RETURN_REPAINT;
	}
	
	char buf[256];
	idone = false;
	while(fgets(buf, sizeof(buf), output) != NULL)
	{
		printf("%s", buf);
                if(!idone && strncmp(buf, "Building a new DOS disklabel.", 29)) 
		{
			idone = true;
			buf[21] = 0;
			progress->showGlobalStatus(20);
                        progress->showStatusMessageUTF(buf);
                } 
		else if(strncmp(buf, "Command (m for help): Command action", 36)) 
		{
			progress->showGlobalStatus(40);
                        progress->showStatusMessageUTF(buf);
		}
		else if(strncmp(buf, "The partition table", 19)) 
		{
			progress->showGlobalStatus(60);
                        progress->showStatusMessageUTF(buf);
		}
	}
	
	fclose(output);

	progress->showGlobalStatus(100);
	sleep(2);
	
	unlink(buffer);
	
	progress->hide();
	delete progress;

	return CWidgetTarget::RETURN_REPAINT;
}

//// format
int CHDDFmtExec::exec(CWidgetTarget *parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDFmtExec::exec: actionKey:%s\n", actionKey.c_str());

	if(parent)
		hide();
	
	char cmd[100];
	CHintBox * hintbox;
	int res;
	FILE * f;
	char src[128];
	const char * dst = NULL;
	bool mountPoint = false;
	
	CProgressWindow *progress;
	bool idone;

	sprintf(src, "/dev/%s", actionKey.c_str());

	res = MessageBox(_("HDD Format"), _("Are you sure to format ? You will lost all data "), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );

	if(res != CMessageBox::mbrYes)
		return RETURN_REPAINT;

	bool srun = system("killall -9 smbd");

	// check if mounted then umount
	if(check_if_mounted(src) == 1)
	{
		// find mount point
		FILE * fstab = setmntent("/etc/mtab", "r");
		struct mntent *e;

		while ((e = getmntent(fstab))) 
		{
			if (strcmp(src, e->mnt_fsname) == 0) 
			{
				dst = e->mnt_dir;
				mountPoint = true;
				break;
			}
		}
//		printf("mount point is %s\n", dst);
		endmntent(fstab);
		
		// can not parse mtab, fallback to /media/sda%
		if(!mountPoint)
			sprintf((char *)dst, "/media/%s", actionKey.c_str());
		
		// umount /media/sda%n
		res = umount(dst);
		
		if(res == -1) 
		{
			// not mounted to /media/sda%n, fallback to /hdd
			strcpy((char *)dst, "/hdd");
			res = umount(dst);
			
			if(res == -1)
			{
				hintbox = new CHintBox(_("HDD Format"), _("HDD unmount failed !"));
				hintbox->exec();
				delete hintbox;
				return CWidgetTarget::RETURN_REPAINT;
			}
		}
	}

	progress = new CProgressWindow();
	progress->setTitle(_("HDD Format"));
	progress->paint();
	progress->showStatusMessageUTF("Executing fdisk");
	progress->showGlobalStatus(0);
	
	//format part ext3
	const char fmt[] = "mkfs.ext3";
	//NOTE: on some arch popen is not working, so dump output of system to /tmp
	sprintf(cmd, "%s -T largefile -m0 %s > /tmp/%s.txt", fmt, src, fmt);

	printf("CHDDFmtExec: executing %s\n", cmd);
	
	system(cmd);
	
	// check if cmd executed
	char buffer[255];
	sprintf(buffer, "/tmp/%s.txt", fmt);
	
	FILE * output = fopen(buffer, "r");
	
	if (!output) 
	{
		hintbox = new CHintBox(_("HDD Format"), _("HDD format failed !"));
		hintbox->exec();
		delete hintbox;
		return CWidgetTarget::RETURN_REPAINT;
	}

	char buf[256];
	idone = false;
	while(fgets(buf, sizeof(buf), output) != NULL)
	{
		printf("%s", buf);
                if(!idone && strncmp(buf, "Writing inode", 13)) 
		{
			idone = true;
			buf[21] = 0;
			progress->showGlobalStatus(20);
                        progress->showStatusMessageUTF(buf);
                } 
		else if(strncmp(buf, "Creating", 8)) 
		{
			progress->showGlobalStatus(40);
                        progress->showStatusMessageUTF(buf);
		}
		else if(strncmp(buf, "Writing superblocks", 19)) 
		{
			progress->showGlobalStatus(60);
                        progress->showStatusMessageUTF(buf);
		}
	}
	
	fclose(output);

	progress->showGlobalStatus(100);
	sleep(2);

	sprintf(cmd, "tune2fs -r 0 -c 0 -i 0 %s", src);
	printf("CHDDFmtExec: executing %s\n", cmd);
	system(cmd);
	
	unlink(buffer);

//_remount:
	progress->hide();
	delete progress;

	// mount
	res = mount(src, dst, "ext3", 0, NULL);

	// create directories
	if(!res) 
	{
		sprintf(cmd, "%s/record", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/movie", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/picture", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/epg", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/music", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/backup", dst);
		safe_mkdir((char *) cmd);
		sprintf(cmd, "%s/logos", dst);
		safe_mkdir((char *) cmd);
		sync();
	}
	
//_return:
	if(!srun) 
		system("smbd");

	return CWidgetTarget::RETURN_REPAINT;
}

//// HDD checkfs
int CHDDChkExec::exec(CWidgetTarget *parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDChkExec: actionKey %s\n", actionKey.c_str());

	if(parent)
		hide();
	
	char cmd[100];
	CHintBox * hintbox;
	int res;
	char src[128];
	const char * dst = NULL;
	const char * fstype = NULL;
	
	CProgressWindow *progress;
	int oldpass = 0, pass, step, total;
	int percent = 0, opercent = 0;

	sprintf(src, "/dev/%s", actionKey.c_str());

	bool srun = system("killall -9 smbd");

	// check if mounted
	if(check_if_mounted(src) == 1)
	{
		// find mount point
		FILE * fstab = setmntent("/etc/mtab", "r");
		struct mntent *e;

		while ((e = getmntent(fstab))) 
		{
			if (strcmp(src, e->mnt_fsname) == 0) 
			{
				dst = e->mnt_dir;
				fstype = e->mnt_type;
				//mountPoint = true;
				break;
			}
		}
		
//		printf("mount point is %s type %s\n", dst, fstype);
		endmntent(fstab);
		
		// unmout /media/sda%
		res = umount(dst);
		
		// not mounted to /media/sda% fallback to /hhd
		if(res == -1)
		{
			dprintf(DEBUG_NORMAL, "CHDDChkExec::exec: can not umount %s\n", dst);
				
			hintbox = new CHintBox(_("Check filesystem"), _("HDD check failed !"));
			hintbox->exec();
			delete hintbox;
			return CWidgetTarget::RETURN_REPAINT;
		}
	}

	// not mounted process directly check
	const char fsck[] = "fsck.ext3";
	sprintf(cmd, "%s -C 1 -f -y %s > /tmp/%s.txt", fsck, src, fsck);

	dprintf(DEBUG_NORMAL, "CHDDChkExec: Executing %s\n", cmd);
	
	my_system(cmd);
	
	// check if cmd executed
	char buffer[255];
	sprintf(buffer, "/tmp/%s.txt", fsck);
	
	FILE * output = fopen(buffer, "r");
	
	if (!output) 
	{
		hintbox = new CHintBox(_("Check filesystem"), _("HDD check failed !"));
		hintbox->exec();
		delete hintbox;
		return CWidgetTarget::RETURN_REPAINT;
	}

	progress = new CProgressWindow();
	progress->setTitle(_("Check filesystem"));
	progress->paint();
	progress->showGlobalStatus(20);

	char buf[256];
	while(fgets(buf, sizeof(buf), output) != NULL)
	{
		if(isdigit(buf[0])) 
		{
			sscanf(buf, "%d %d %d\n", &pass, &step, &total);
			if(total == 0)
				total = 1;
			if(oldpass != pass) 
			{
				oldpass = pass;
				progress->showGlobalStatus(pass > 0 ? (pass-1)*20: 0);
			}
			percent = (step * 100) / total;
			if(opercent != percent) 
			{
				opercent = percent;
				//printf("CHDDChkExec: pass %d : %d\n", pass, percent);
				//progress->showLocalStatus(percent);
			}
		}
		else if(!strncmp(buf, "Pass", 4))
			progress->showStatusMessageUTF(buf);
	}
	
	fclose(output);

	progress->showGlobalStatus(100);
	progress->showStatusMessageUTF(buf);
	
	sleep(2);
	progress->hide();
	delete progress;
	
	unlink(buffer);

	// mount
	res = mount(src, dst, fstype, 0, NULL);

	dprintf(DEBUG_NORMAL, "CHDDChkExec: mount res %d\n", res);
	
	if(res < 0)
	{
		hintbox = new CHintBox(_("Check filesystem"), _("HDD Mount Failed"));
		hintbox->exec();
		delete hintbox;
	}
	
	if(!srun) 
		system("smbd");
	
	return CWidgetTarget::RETURN_REPAINT;
}

//// HDD mount
int CHDDMountMSGExec::exec(CWidgetTarget *parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDMountMSGExec: %s\n", (char *)actionKey.c_str());

	if(parent)
		hide();
	
	CHintBox * hintbox;
	int res;
	char src[128];
	char dst[128];
	const char * fstype = NULL;

	sprintf(src, "/dev/%s", actionKey.c_str());
	sprintf(dst, "/media/%s", actionKey.c_str());

	res = check_if_mounted((char *)actionKey.c_str());

	// if mounted 
	if(res == 1) 
	{
		hintbox = new CHintBox(_("HDD Mount"), _("HDD Mounted"));
		hintbox->exec();
		delete hintbox;
		return CWidgetTarget::RETURN_REPAINT;
	}
	else
	{
		// get fs type
		fstype = blkid_get_tag_value(NULL, "TYPE", src);
		
		dprintf(DEBUG_NORMAL, "fstype: %s\n", fstype);

		if(fstype != NULL)
		{
			//mount to /hdd
			res = mount(src, "/hdd", fstype, 0, NULL);
	
			dprintf(DEBUG_NORMAL, "CHDDMountMSGExec::exec: mount1 to /hdd res %d\n", res);

			if(res == 0) 
			{
				hintbox = new CHintBox(_("HDD Mount"), _("HDD Mounted"));
				hintbox->exec();
				delete hintbox;
				
				return CWidgetTarget::RETURN_REPAINT;
			}
			else
			{
				// mount to /media/sda%
				res = mount(src, dst, fstype, 0, NULL);
	
				dprintf(DEBUG_NORMAL, "CHDDMountMSGExec::exec: mount2 to %s res %d\n", dst, res);
				
				if(res == 0)
				{
					hintbox = new CHintBox(_("HDD Mount"), _("HDD Mounted"));
					hintbox->exec();
					delete hintbox;
					
					return CWidgetTarget::RETURN_REPAINT;
				}
				else //fallback to /tmp/hdd
				{
					// create /tmp/hdd
					safe_mkdir((char *)"/tmp/hdd");
					// mount to /tmp/hdd
					res = mount(src, "/tmp/hdd", fstype, 0, NULL);
					
					dprintf(DEBUG_NORMAL, "CHDDMountMSGExec::exec: mount3 to /tmp/hdd res %d\n", res);
					
					if(res == 0)
					{
						hintbox = new CHintBox(_("HDD Mount"), _("HDD Mounted"));
						hintbox->exec();
						delete hintbox;
						
						return CWidgetTarget::RETURN_REPAINT;
					}
					else
					{
						hintbox = new CHintBox(_("HDD Mount"), _("HDD Mount failed !"));
						hintbox->exec();
						delete hintbox;
						return CWidgetTarget::RETURN_REPAINT;
					}
				}
			}
		}
		else // can not get fstype
		{
			hintbox = new CHintBox(_("HDD Mount"), _("HDD Mount failed !"));
			hintbox->exec();
			delete hintbox;
			return CWidgetTarget::RETURN_REPAINT;
		}
	}
	
	return CWidgetTarget::RETURN_REPAINT;
}

//// umount
int CHDDuMountMSGExec::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDuMountExec: actionKey:%s\n", actionKey.c_str());

	if(parent)
		hide();
	
	CHintBox * hintbox;
	int res;
	char src[128];
	const char * dst = NULL;

	sprintf(src, "/dev/%s", actionKey.c_str());

	// umount
	if(check_if_mounted(src) == 1)
	{
		// find mount point
		FILE * fstab = setmntent("/etc/mtab", "r");
		struct mntent *e;

		while ((e = getmntent(fstab))) 
		{
			if (strcmp(src, e->mnt_fsname) == 0) 
			{
				dst = e->mnt_dir;
				break;
			}
		}
		
		dprintf(DEBUG_NORMAL, "mount point is %s\n", dst);
		endmntent(fstab);
		
		// umount /media/sda%
		res = umount(dst);
		dprintf(DEBUG_NORMAL, "CHDDuMountExec: umount res %d\n", res);

		if(res == 0)
		{
			hintbox = new CHintBox(_("HDD Mount"), _("HDD umount"));
			hintbox->exec();
			delete hintbox;
			
			return CWidgetTarget::RETURN_REPAINT;
		}
		else
		{
			hintbox = new CHintBox(_("HDD Mount"), _("HDD unmount failed !"));
			hintbox->exec();
			delete hintbox;
			return CWidgetTarget::RETURN_REPAINT;

		}
	}

	// not mounted
	hintbox = new CHintBox(_("HDD Mount"), _("HDD umounted"));
	hintbox->exec();
	delete hintbox;
	
	return CWidgetTarget::RETURN_REPAINT;
}

//// hdd browser
int CHDDBrowser::exec(CWidgetTarget * parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CHDDBrowser::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		parent->hide();
	
	CFileBrowser filebrowser;
	CHintBox * hintbox;
	const char * dst = NULL;

	// find mount point
	FILE * fstab = setmntent("/etc/mtab", "r");
	struct mntent * e;

	while ((e = getmntent(fstab))) 
	{
		if (strcmp(actionKey.c_str(), e->mnt_fsname) == 0) 
		{
			dst = e->mnt_dir;
			break;
		}
	}
		
	endmntent(fstab);
	
	if(dst != NULL)
	{
REPEAT:	  
		if( filebrowser.exec(dst))
		{
			dst = filebrowser.getCurrentDir().c_str(); // remark path
			neutrino_msg_t msg;
			neutrino_msg_data_t data;
			
			// get the current file name
			CFile * file;

			if ((file = filebrowser.getSelectedFile()) != NULL) 
			{
				// parse file extension
				if(file->getType() == CFile::FILE_PICTURE)
				{
					CPictureViewerGui tmpPictureViewerGui;
			
					tmpPictureViewerGui.addToPlaylist(*file);
					tmpPictureViewerGui.exec(NULL, "");	
				}
				else if(file->getType() == CFile::FILE_VIDEO)
				{
					MI_MOVIE_INFO movieInfo;
					CMovieInfo m_movieInfo;
					
					m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
						
					movieInfo.file.Name = file->Name;
					
					// load movie infos (from xml file)
					m_movieInfo.loadMovieInfo(&movieInfo);
					
					CMovieInfoWidget movieInfoWidget;
					movieInfoWidget.setMovie(movieInfo);
			
					movieInfoWidget.exec(NULL, "");
				}
				else if(file->getType() == CFile::FILE_AUDIO)
				{
					CAudioPlayerGui tmpAudioPlayerGui;
			
					tmpAudioPlayerGui.addToPlaylist(*file);
					tmpAudioPlayerGui.exec(NULL, "");
				}
				else
				{
					std::string buffer;
					buffer.clear();
				
					char buf[6000];

					int fd = open(file->Name.c_str(), O_RDONLY);
					int bytes = read(fd, buf, 6000 - 1);
					close(fd);
					buf[bytes] = 0;
					buffer = buf;

					CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
					
					CInfoBox * infoBox = new CInfoBox(&position, file->getFileName().c_str(), NEUTRINO_ICON_FILE);
					infoBox->setText(buffer.c_str());
					infoBox->exec();
					delete infoBox;
				}
			}
			
			g_RCInput->getMsg_ms(&msg, &data, 10);
			
			if (msg != CRCInput::RC_home) 
			{
				goto REPEAT;
			}
		}
	}
	else
	{
		hintbox = new CHintBox(_("HDD Mount"), _("HDD umounted"));
		hintbox->exec();
		delete hintbox;
	}
	
	return CWidgetTarget::RETURN_REPAINT;
}

