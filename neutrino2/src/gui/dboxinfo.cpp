/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: dboxinfo.cpp 2013/10/12 mohousch Exp $

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/gdi/fontrenderer.h>

#include <driver/rcinput.h>

#include <gui/dboxinfo.h>
#include <gui/imageinfo.h>
#include <gui/streaminfo.h>

// zapit includes
#include <zapit/frontend_c.h>

#include <system/debug.h>


static const int FSHIFT = 16;              	// nr of bits of precision
#define FIXED_1         (1<<FSHIFT)     	// 1.0 as fixed-point
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

// hdd
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

#include <system/settings.h>


static int my_filter(const struct dirent * dent)
{
	if(dent->d_name[0] == 's' && dent->d_name[1] == 'd')
		return 1;
	
	return 0;
}

CDBoxInfoWidget::CDBoxInfoWidget()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	Box.iX = g_settings.screen_StartX + 20;
	Box.iY = g_settings.screen_StartY + 20;
	Box.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - 40;
	Box.iHeight = g_settings.screen_EndY - g_settings.screen_StartY - 40;
	
	//
	dboxInfoWidget = NULL;
	head = NULL;
	
	cpuIcon = NULL;
	cpuLabel = NULL;
	cpuLabel1 = NULL;
	CCLabel* cpuLabel2 = NULL;
	CCHline* hLine = NULL;
	CCLabel* upLabel = NULL;
	CCLabel* memLabel = NULL;
	CCHline* hLine2 = NULL;
	CCIcon* hddIcon = NULL;
	CCLabel* hddLabel = NULL;
	CCLabel* hddLabel1 = NULL;
	CCIcon* tunerIcon = NULL;
	CCLabel* tunerLabel = NULL;
	CCLabel* tunerLabel1 = NULL;
}

int CDBoxInfoWidget::exec(CWidgetTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CDBoxInfoWidget::exec:\n");

	if (parent)
		parent->hide();
	
	showInfo();
	
	if (!parent)
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	}	

	return CWidgetTarget::RETURN_REPAINT;
}

void CDBoxInfoWidget::hide()
{
	dprintf(DEBUG_NORMAL, "CDBoxInfoWidget::hide:\n");
}

int CDBoxInfoWidget::showInfo()
{
	dprintf(DEBUG_NORMAL, "CDBoxInfoWidget::showInfo:\n");
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	dboxInfoWidget = CNeutrinoApp::getInstance()->getWidget("boxinfo");
	
	//
	if (dboxInfoWidget)
	{
		head = (CCHeaders*)dboxInfoWidget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		dboxInfoWidget = new CWidget(&Box);
		
		dboxInfoWidget->name = "boxinfo";
		dboxInfoWidget->paintMainFrame(true);
		dboxInfoWidget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
		
		// head
		head = new CCHeaders(Box.iX, Box.iY, Box.iWidth, 40, _("Box Info"), NEUTRINO_ICON_INFO);
		head->enablePaintDate();
		head->setFormat("%d.%m.%Y %H:%M:%S");
		
		//
		dboxInfoWidget->addCCItem(head);

	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Box Info"));
	
	int yPos = Box.iY + (head? head->getWindowsPos().iHeight : 0);

	int i = 0;

	//cpu
	yPos += 10;
	
	std::string CPU = CFrameBuffer::getInstance()->getHintBasePath();
	CPU += "/";
	CPU += NEUTRINO_ICON_MENUITEM_IMAGEINFO;
	CPU += ".png";
	
	cpuIcon = new CCIcon(Box.iX + 10, yPos, 100, 40);
	cpuIcon->setIcon(CPU.c_str());
	dboxInfoWidget->addCCItem(cpuIcon);
	
	cpuLabel = new CCLabel();
	cpuLabel->setText(_("CPU:"));
	cpuLabel->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getRenderWidth("CPU:");
	cpuLabel->setColor(COL_MENUHEAD_TEXT_PLUS_0);
	cpuLabel->setPosition(Box.iX + 10 + 100 + 10, yPos, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getRenderWidth("CPU:"), 40);
	dboxInfoWidget->addCCItem(cpuLabel);
	
	//
	yPos += cpuIcon->height + 10;
	
	FILE* fd = fopen("/proc/cpuinfo", "rt");

	if (fd == NULL) 
	{
		printf("error while opening proc-cpuinfo\n" );
	} 
	else 
	{
		char *buffer = NULL;
		size_t len = 0;
		ssize_t read;

		while ((read = getline(&buffer, &len, fd)) != -1) 
		{
			if (!(strncmp(const_cast<char *>("Hardware"), buffer, 8))) 
			{
				char *t = rindex(buffer, '\n');
				if (t)
					*t = '\0';

				std::string hw;
				char *p = rindex(buffer, ':');
				if (p)
					hw = ++p;
				hw += " Info";
				
				cpuLabel1 = new CCLabel();
				cpuLabel1->setText(hw.c_str());
				cpuLabel1->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
				cpuLabel1->setColor(COL_MENUCONTENT_TEXT_PLUS_0);
				cpuLabel1->setPosition(Box.iX + 10 + 100, yPos, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight());
				dboxInfoWidget->addCCItem(cpuLabel1);
				
				yPos += cpuLabel1->getWindowsPos().iHeight;
				
				break;
			}
			i++;

			if (i > 4)
				continue;
				
			cpuLabel2 = new CCLabel();
			cpuLabel2->setText(buffer);
			cpuLabel2->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
			cpuLabel2->setColor(COL_MENUCONTENT_TEXT_PLUS_0);
			cpuLabel2->setPosition(Box.iX + 10 + 100, yPos, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight());
			dboxInfoWidget->addCCItem(cpuLabel2);
			
			yPos += cpuLabel2->getWindowsPos().iHeight;
		}
		fclose(fd);

		if (buffer)
			free(buffer);
	}
	
	// separator
	hLine = new CCHline();
	
	hLine->setPosition(Box.iX + 10,  yPos, Box.iWidth - 20, 10);
	hLine->setGradient(3);
	dboxInfoWidget->addCCItem(hLine);

	// up time
	upLabel = new CCLabel();
	
	int updays, uphours, upminutes;
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;
	char ubuf[256], sbuf[256];

	memset(sbuf, 0, 256);
	time(&current_secs);
	current_time = localtime(&current_secs);

	sysinfo(&info);

	sprintf( ubuf, "%2d:%02d%s  up ", 
			current_time->tm_hour%12 ? current_time->tm_hour%12 : 12, 
			current_time->tm_min, current_time->tm_hour > 11 ? "pm" : "am");

	strcat(sbuf, ubuf);
	updays = (int) info.uptime / (60*60*24);
	if (updays) 
	{
		sprintf(ubuf, "%d day%s, ", updays, (updays != 1) ? "s" : "");
		strcat(sbuf, ubuf);
	}
	upminutes = (int) info.uptime / 60;
	uphours = (upminutes / 60) % 24;
	upminutes %= 60;
	if(uphours)
		sprintf(ubuf,"%2d:%02d, ", uphours, upminutes);
	else
		sprintf(ubuf,"%d min, ", upminutes);
	strcat(sbuf, ubuf);

	sprintf(ubuf, "load: %ld.%02ld, %ld.%02ld, %ld.%02ld\n", 
			LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]), 
			LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]), 
			LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));

	strcat(sbuf, ubuf);
	
	upLabel = new CCLabel();
	upLabel->setText(sbuf);
	upLabel->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
	upLabel->setColor(COL_MENUCONTENTINACTIVE_TEXT_PLUS_0);
	upLabel->setPosition(Box.iX + 10, yPos, Box.iWidth, 35);
	dboxInfoWidget->addCCItem(upLabel);
	
	// mem
	sprintf(ubuf, "memory total %dKb, free %dKb", (int) info.totalram/1024, (int) info.freeram/1024);

	yPos += upLabel->getWindowsPos().iHeight + 10;
	
	memLabel = new CCLabel();
	memLabel->setText(ubuf);
	memLabel->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
	memLabel->setColor(COL_MENUCONTENTINACTIVE_TEXT_PLUS_0);
	memLabel->setPosition(Box.iX + 10, yPos, Box.iWidth, 35);
	dboxInfoWidget->addCCItem(memLabel);
	
	// separator
	yPos += memLabel->getWindowsPos().iHeight + 10;
	
	hLine2 = new CCHline();
	hLine2->setPosition(Box.iX + 10,  yPos, Box.iWidth - 20, 10);
	hLine2->setGradient(3);
	dboxInfoWidget->addCCItem(hLine2);
    	
    	//hdd devices
    	hddIcon = new CCIcon();
    	
    	std::string HDD = CFrameBuffer::getInstance()->getHintBasePath();
	HDD += "/";
	HDD += NEUTRINO_ICON_MENUITEM_HDDSETTINGS;
	HDD += ".png";
    	
    	hddIcon->setIcon(HDD.c_str());
    	
	FILE * f;
	int fd_hdd;
	struct dirent **namelist;
	
	int n = scandir("/sys/block", &namelist, my_filter, alphasort);
	
	if (n)
	{
		yPos += 10;
		
		hddIcon->setPosition(Box.iX + 10, yPos, hddIcon->width, hddIcon->height);
		dboxInfoWidget->addCCItem(hddIcon);
		
		hddLabel = new CCLabel();
		hddLabel->setText(_("HDD Devices:"));
		hddLabel->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
		hddLabel->setColor(COL_MENUHEAD_TEXT_PLUS_0);
		hddLabel->setPosition(Box.iX + 10 + hddIcon->width + 10, yPos, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getRenderWidth("HDD Devices:"), 40);
		dboxInfoWidget->addCCItem(hddLabel);
		
		yPos += hddIcon->height + 10;
	}
	
	for(int i1 = 0; i1 < n; i1++) 
	{
		char str[256];
		char vendor[128];
		char model[128];
		int64_t bytes = 0;
		int64_t megabytes = 0;
		int removable = 0;
		
		sprintf(str, "/dev/%s", namelist[i1]->d_name);
		fd_hdd = open(str, O_RDONLY);

		if(fd_hdd < 0) 
		{
			//printf("HDD: Cant open %s\n", str);
			//continue;
		}
		
		if (ioctl(fd_hdd, BLKGETSIZE64, &bytes))
			perror("BLKGETSIZE64");
                
                close(fd_hdd);

		megabytes = bytes/1000000;

		// vendor
		sprintf(str, "/sys/block/%s/device/vendor", namelist[i1]->d_name);
		f = fopen(str, "r");

		if(!f) 
		{
			continue;
		}
		fscanf(f, "%s", vendor);
		fclose(f);

		// model
		sprintf(str, "/sys/block/%s/device/model", namelist[i1]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			continue;
		}
		fscanf(f, "%s", model);
		fclose(f);

		// removable
		sprintf(str, "/sys/block/%s/removable", namelist[i1]->d_name);
		f = fopen(str, "r");
		
		if(!f) 
		{
			continue;
		}
		fscanf(f, "%d", &removable);
		fclose(f);
		
		// free space on hdd
		struct statfs s;
		if (megabytes)
		{
			if (::statfs(g_settings.network_nfs_recordingdir, &s) == 0) 
			{
				sprintf(str, "%s (%s-%s %lld %s), free %ldMB", namelist[i1]->d_name, vendor, model, megabytes < 10000 ? megabytes : megabytes/1000, megabytes < 10000 ? "MB" : "GB", (long)( ((s.f_bfree/1024)/1024))*s.f_bsize);
			}
			else
				sprintf(str, "%s (%s-%s %lld %s)", namelist[i1]->d_name, vendor, model, megabytes < 10000 ? megabytes : megabytes/1000, megabytes < 10000 ? "MB" : "GB");
		}
		else
		{
			if (::statfs(g_settings.network_nfs_recordingdir, &s) == 0) 
			{
				sprintf(str, "%s (%s-%s), free %ldMB", namelist[i1]->d_name, vendor, model, (long)( ((s.f_bfree/1024)/1024))*s.f_bsize);
			}
			else
				sprintf(str, "%s (%s-%s)", namelist[i1]->d_name, vendor, model);
		}
		
		free(namelist[i1]);
		
		hddLabel1 = new CCLabel();
		hddLabel1->setText(str);
		hddLabel1->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
		hddLabel1->setColor(COL_MENUCONTENT_TEXT_PLUS_0);
		hddLabel1->setPosition(Box.iX + 10 + 100, yPos, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight());
		dboxInfoWidget->addCCItem(hddLabel1);
			
		yPos += hddLabel1->getWindowsPos().iHeight;
	}
	
	//frontend
	std::string TUNER = CFrameBuffer::getInstance()->getHintBasePath();
	TUNER += "/";
	TUNER += NEUTRINO_ICON_MENUITEM_SCANSETTINGS;
	TUNER += ".png";
	
	tunerIcon = new CCIcon();
	tunerIcon->setIcon(TUNER.c_str());
	
	if (CZapit::getInstance()->getFrontendCount())
	{
		yPos += 10;
		
		tunerIcon->setPosition(Box.iX + 10, yPos, 100, 40);
		dboxInfoWidget->addCCItem(tunerIcon);
		
		tunerLabel = new CCLabel();
		tunerLabel->setText(_("Frontend:"));
		tunerLabel->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
		tunerLabel->setColor(COL_MENUHEAD_TEXT_PLUS_0);
		tunerLabel->setPosition(Box.iX + 10 + 100 + 10, yPos, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getRenderWidth("Frontend:"), 40);
		dboxInfoWidget->addCCItem(tunerLabel);
		
		yPos += tunerIcon->height + 10;
	}
	
	for(int i2 = 0; i2 < CZapit::getInstance()->getFrontendCount(); i2++)
	{
		CFrontend * fe = CZapit::getInstance()->getFE(i2);
		char tbuf[255];
		
		sprintf(tbuf, "Tuner-%d: %s", i2 + 1, fe->getInfo()->name);
		
		tunerLabel1 = new CCLabel();
		tunerLabel1->setText(tbuf);
		tunerLabel1->setFont(SNeutrinoSettings::FONT_TYPE_MENU);
		tunerLabel1->setColor(COL_MENUCONTENT_TEXT_PLUS_0);
		tunerLabel1->setPosition(Box.iX + 10 + 100, yPos, Box.iWidth, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight());
		dboxInfoWidget->addCCItem(tunerLabel1);
		
		yPos += tunerLabel1->getWindowsPos().iHeight;
		
	}
	
	res = dboxInfoWidget->exec(NULL, "");
	
	if (dboxInfoWidget)
	{
		delete dboxInfoWidget;
		dboxInfoWidget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

//
int CInfoMenu::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CInfoMenu::exec: %s\n", actionKey.c_str());
	
	int ret = CWidgetTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();
		
	ret = showMenu();
	
	return ret;
}

int CInfoMenu::showMenu()
{
	dprintf(DEBUG_NORMAL, "CInfoMenu::showMenu:\n");
	
	int res = CWidgetTarget::RETURN_REPAINT;
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Information"));
	
	//
	widget = NULL; 
	infoMenu = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("information");
	
	if (widget)
	{
		infoMenu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "information";
		
		//
		infoMenu = new ClistBox(&box);
		
		infoMenu->setWidgetMode(ClistBox::MODE_MENU);
		infoMenu->setWidgetType(ClistBox::TYPE_CLASSIC);
		infoMenu->enableShrinkMenu();
		infoMenu->paintMainFrame(true);
		
		//
		infoMenu->enablePaintHead();
		infoMenu->setTitle(_("Information"), NEUTRINO_ICON_INFO);
		infoMenu->enablePaintDate();
		
		//
		infoMenu->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		infoMenu->setFootButtons(&btn);
		
		// iteminfo
		if (g_settings.item_info) infoMenu->enablePaintItemInfo(60);
		
		//
		widget->addCCItem(infoMenu);
	
		//
		infoMenu->addItem( new CMenuForwarder(_("Information"), true, NULL, new CDBoxInfoWidget(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_BOXINFO));
			
		//
		infoMenu->addItem(new CMenuForwarder(_("Image info"),  true, NULL, new CImageInfo(), NULL, CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_IMAGEINFO), false);
			
		//
		infoMenu->addItem(new CMenuForwarder(_("Stream information"), true, NULL, new CStreamInfo(), "", CRCInput::RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_BOXINFO));
		
		//
		infoMenu->integratePlugins(CPlugins::I_TYPE_INFORMATION);
	}
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

