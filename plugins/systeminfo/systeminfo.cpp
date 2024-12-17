/*
  $Id: systeminfo.cpp 24112024 mohousch Exp $

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

#include <systeminfo.h>


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

//// defines
//FIXME: make this global
#define __(string) dgettext("systeminfo", string)

bool refreshIt = true;

// construktor
CSysInfoWidget::CSysInfoWidget(int m)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	// windows size
	cFrameBox.iWidth  = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	//head height
	cFrameBoxTitle.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight() + 6;
       
	//foot height
	cFrameBoxFoot.iHeight = cFrameBoxTitle.iHeight;
	
	// coordinate
	cFrameBox.iX = (((g_settings.screen_EndX - g_settings.screen_StartX) - cFrameBox.iWidth) / 2) + g_settings.screen_StartX;
	cFrameBox.iY = (((g_settings.screen_EndY - g_settings.screen_StartY) - cFrameBox.iHeight) / 2) + g_settings.screen_StartY;
	
	mode = m;

	// textBox
	cFrameBoxText.iX = cFrameBox.iX;
	cFrameBoxText.iY = cFrameBox.iY + cFrameBoxTitle.iHeight;
	cFrameBoxText.iWidth = cFrameBox.iWidth;
	cFrameBoxText.iHeight = cFrameBox.iHeight - cFrameBoxTitle.iHeight - cFrameBoxFoot.iHeight;

	textBox = new CTextBox(&cFrameBoxText);

	textBox->setBackgroundColor(COL_BLACK_PLUS_0);
	
	widget = new CWidget(&cFrameBox);
	widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	widget->paintMainFrame(true);
}

CSysInfoWidget::~CSysInfoWidget()
{
	if(textBox != NULL)
	{
		delete textBox;
		textBox = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

// paintlistbox
void CSysInfoWidget::paint()
{
	widget->paint();
	
	paintHead();
	
	// paint
	textBox->paint();

	// settext
	textBox->setText(buffer.c_str());
	
	textBox->refresh();
	
	paintFoot();
}

// paint head
void CSysInfoWidget::paintHead()
{
	char buf[100];
	
	// Title
	cFrameBoxTitle.iX = cFrameBox.iX;
	cFrameBoxTitle.iY = cFrameBox.iY;
	cFrameBoxTitle.iWidth = cFrameBox.iWidth;
	
	if(mode == SYSINFO)
		sprintf((char *) buf, "%s", __("System-Info:"));
	
	if(mode == DMESGINFO)
		sprintf((char *) buf, "%s", __("System-Messages:"));
	
	if(mode == CPUINFO)
		sprintf((char *) buf, "%s", __("CPU/File-Info:"));
	
	if(mode == PSINFO)
		sprintf((char *) buf, "%s", __("Process-List:"));
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();	
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, buf);
	
	// title
	CCHeaders headers(&cFrameBoxTitle, buf, NEUTRINO_ICON_INFO);
	headers.enablePaintDate();

	headers.paint();
}

// paint foot
const struct button_label Buttons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED, __("info") },
	{ NEUTRINO_ICON_BUTTON_GREEN, __("dmesg") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, __("cpu/file") },
	{ NEUTRINO_ICON_BUTTON_BLUE, __("ps") },
	
};

void CSysInfoWidget::paintFoot()
{
	// Foot
	cFrameBoxFoot.iX = cFrameBox.iX;
	cFrameBoxFoot.iY = cFrameBox.iY + cFrameBox.iHeight - cFrameBoxFoot.iHeight;
	cFrameBoxFoot.iWidth = cFrameBox.iWidth;

	CCFooters footers(&cFrameBoxFoot);

	footers.setButtons(Buttons, 4);
	
	footers.paint();
}

// hide
void CSysInfoWidget::hide()
{
//	frameBuffer->paintBackgroundBoxRel(cFrameBox.iX, cFrameBox.iY, cFrameBox.iWidth, cFrameBox.iHeight);
	widget->hide();
	
	frameBuffer->blit();

	//if(textBox != NULL)
	{
	//	delete textBox;
	//	textBox = NULL;
	}
	
	//
	CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
}

// main
int CSysInfoWidget::exec(CMenuTarget* parent, const std::string& /*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CsysInfoWidget::exec:\n");

	int res = CMenuTarget::RETURN_REPAINT;

	if(mode == SYSINFO)
	{
		sysinfo();
	}
	else if(mode == DMESGINFO)
	{
		dmesg();
	}
	else if(mode == CPUINFO)
	{
		cpuinfo();
	}
	else if(mode == PSINFO)
	{
		ps();
	}

	if (parent)
		parent->hide();

	//paintHead();
	paint();
	//paintHead();
	//paintFoot();
	
	frameBuffer->blit();

	neutrino_msg_t msg; 
	neutrino_msg_data_t data;
	int timercount = 0;
	uint64_t timeoutEnd = g_RCInput->calcTimeoutEnd(5);

	while (msg != CRCInput::RC_home)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (msg <= CRCInput::RC_MaxRC  ) 
			timeoutEnd = g_RCInput->calcTimeoutEnd(5);
		
		// dont cancel by timeout
		if (msg == CRCInput::RC_timeout)
		{
			if (mode == SYSINFO)
			{
				timercount = 0;
				sysinfo();

				//paintHead();
				paint();
				//paintFoot();
			}
			
			if ((mode == DMESGINFO) && (++timercount>11))
			{
				timercount = 0;
				dmesg();

				//paintHead();
				paint();
				//paintFoot();
			}
			
			if ((mode == PSINFO)&&(refreshIt == true))
			{
				timercount = 0;
				ps();

				//paintHead();
				paint();
				//paintHead();
				//paintFoot();
			}

			timeoutEnd = g_RCInput->calcTimeoutEnd(5);
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
			
		}

		if ((msg == CRCInput::RC_up || msg == CRCInput::RC_page_up) && (mode != SYSINFO))
		{
			textBox->scrollPageUp(1);
		}
		else if ((msg == CRCInput::RC_down || msg == CRCInput::RC_page_down) && (mode != SYSINFO))
		{
			textBox->scrollPageDown(1);
		}
		else if ((msg == CRCInput::RC_red) && (mode != SYSINFO))
		{
			mode = SYSINFO;
			sysinfo();
			
			if (textBox)
				textBox->refresh();

			//paintHead();
			paint();
			//paintHead();
			//paintFoot();

		}
		else if ((msg == CRCInput::RC_green) && (mode != DMESGINFO))
		{
			mode = DMESGINFO;
			timercount = 0;
			dmesg();

			//paintHead();
			paint();
			//paintHead();
			//paintFoot();
		}
		else if ((msg == CRCInput::RC_yellow) && (mode != CPUINFO))
		{
			mode = CPUINFO;
			cpuinfo();

			//paintHead();
			paint();
			//paintHead();
			//paintFoot();
		}
		else if (msg == CRCInput::RC_blue)
		{
			mode = PSINFO;
			ps();

			//paintHead();
			paint();
			//paintHead();
			//paintFoot();
		}
		else
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}

		frameBuffer->blit();	
	}
	
	hide();
	
	return res;
}

void CSysInfoWidget::sysinfo()
{
	static long curCPU[5] = {0, 0, 0, 0, 0};
	static long prevCPU[5] = {0, 0, 0, 0, 0};
	double value[5] = {0, 0, 0, 0, 0};
	float faktor;
	int i, j = 0;
	char strhelp[6];
	FILE *f;
	char line[256];
	const char *fmt = " %a %d %b %Y %H:%M";
	long t;

	// Get and Format the SystemTime
	t = time(NULL);
	struct tm *tp;
	tp = localtime(&t);
	strftime(line, sizeof(line), fmt, tp);

	// Create tmpfile with date /tmp/sysinfo
	system("echo 'DATUM:' > /tmp/sysinfo");
	
	f = fopen("/tmp/sysinfo", "a");
	if(f)
		fprintf(f,"%s\n", line);
	fclose(f);

	// Get the statistics from /proc/stat
	if(prevCPU[0] == 0)
	{
		f = fopen("/proc/stat","r");
		if(f)
		{
			fgets(line, 256, f); /* cpu */
			sscanf(line,"cpu %lu %lu %lu %lu", &prevCPU[1], &prevCPU[2], &prevCPU[3], &prevCPU[4]);
			for(i = 1; i < 5; i++)
				prevCPU[0] += prevCPU[i];
		}
		fclose(f);
		sleep(1);
	}
	else
	{
		for(i = 0; i < 5; i++)
			prevCPU[i]=curCPU[i];
	}

	while(((curCPU[0] - prevCPU[0]) < 100) || (curCPU[0] == 0))
	{
		f = fopen("/proc/stat","r");
		if(f)
		{
			curCPU[0] = 0;
			fgets(line, 256, f); /* cpu */
			sscanf(line, "cpu %lu %lu %lu %lu", &curCPU[1], &curCPU[2], &curCPU[3], &curCPU[4]);
			for(i = 1; i < 5; i++)
				curCPU[0] += curCPU[i];
		}
		fclose(f);
		if((curCPU[0] - prevCPU[0]) < 100)
			sleep(1);
	}
	
	// some calculations
	if(!(curCPU[0] - prevCPU[0]) == 0)
	{
		faktor = 100.0/(curCPU[0] - prevCPU[0]);
		for(i = 0; i < 4; i++)
			value[i] = (curCPU[i] - prevCPU[i])*faktor;

		value[4] = 100.0 - value[1] - value[2] - value[3];

		f = fopen("/tmp/sysinfo", "a");
		if(f)
		{
			memset(line, 0x20, sizeof(line));
			for(i = 1, j = 0; i < 5; i++)
			{
				memset(strhelp, 0, sizeof(strhelp));
				sprintf(strhelp, "%.1f", value[i]);
				memcpy(&line[(++j*7) - 2 - strlen(strhelp)], &strhelp[0], strlen(strhelp));
				memcpy(&line[(j*7) - 2], "%", 1);
			}
			line[(j*7) - 1] = '\0';
			fprintf(f,"\nPERFORMANCE:\n USER:  NICE:   SYS:  IDLE:\n%s\n", line);
		}
		fclose(f);
	}

	// Get kernel-info from /proc/version
	f = fopen("/proc/version", "r");
	if(f)
	{
		char* token;
		fgets(line, 256, f); // version
		token = strstr(line,") (");
		if(token != NULL)
			*++token = 0x0;
		fclose(f);
		f = fopen("/tmp/sysinfo", "a");
		fprintf(f, "\nKERNEL:\n %s\n %s\n", line, ++token);
	}
	fclose(f);

	// Get uptime-info from /proc/uptime
	f = fopen("/proc/uptime", "r");
	if(f)
	{
		fgets(line, 256, f);
		float ret[4];
		const char* strTage[2] = {__("Days"), __("Day")};
		const char* strStunden[2] = {__("Hours"), __("Hour")};
		const char* strMinuten[2] = {__("Minutes"), __("Minute")};
		sscanf(line, "%f", &ret[0]);
		ret[0] /= 60;
		ret[1] = long(ret[0])/60/24; // Tage
		ret[2] = long(ret[0])/60-long(ret[1])*24; // Stunden
		ret[3] = long(ret[0])-long(ret[2])*60-long(ret[1])*60*24; // Minuten
		fclose(f);

		f = fopen("/tmp/sysinfo", "a");
		if(f)
			fprintf(f, "UPTIME:\n %s %.0f %s %.0f %s %.0f %s\n", __("System running:"), ret[1], strTage[int(ret[1]) == 1], ret[2], strStunden[int(ret[2]) == 1], ret[3], strMinuten[int(ret[3]) == 1]);
	}
	fclose(f);

	//
	readList("/tmp/sysinfo");
}

void CSysInfoWidget::cpuinfo()
{
	char Wert1[30];
	char Wert2[10];
	char Wert3[10];
	char Wert4[10];
	char Wert5[6];
	char Wert6[30];

	FILE *f,*w;
	char line[256];
	int i = 0;
	
	// Get file-info from /proc/cpuinfo
	system("df > /tmp/systmp");
	
	f = fopen("/tmp/systmp", "r");
	if(f)
	{
		w = fopen("/tmp/sysinfo","w");
		if(w)
		{
			while((fgets(line,256, f)!=NULL))
			{
				sscanf(line,"%s %s %s %s %s %s ", &Wert1, &Wert2, &Wert3, &Wert4, &Wert5, &Wert6);
				if(i++)
					fprintf(w,"\nFilesystem: %s\n  1-KBlocks: %s\n  Used: %s\n  Free: %s\n  Use%%: %s\nMounted on: %s\n",Wert1,Wert2,Wert3,Wert4,Wert5,Wert6);
			}
			fprintf(w, "\nCPU:\n\n");
			fclose(w);
		}
	}
	fclose(f);

	// cpuinfo from /proc/cpuinfo
	system("cat /proc/cpuinfo >> /tmp/sysinfo");
	unlink("/tmp/systmp");
	
	readList("/tmp/sysinfo");
}

void CSysInfoWidget::dmesg()
{
	system("dmesg > /tmp/sysinfo");

	readList("/tmp/sysinfo");
}

void CSysInfoWidget::ps()
{
#ifdef USE_OPENGL
	system("ps -A > /tmp/sysinfo");
#else
	system("ps > /tmp/sysinfo");
#endif

	readList("/tmp/sysinfo");
}

// read infos
void CSysInfoWidget::readList(const char * const filename)
{
	buffer.clear();
	buffer = "";
	
	buffer = ::readFile(filename).c_str();
	
	system("rm -rf /tmp/sysinfo");
}

//
void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CSysInfoWidget * SysInfoWidget = new CSysInfoWidget();
	
	SysInfoWidget->exec(NULL, "");
	SysInfoWidget->hide();
	
	delete SysInfoWidget;
	SysInfoWidget = NULL;
}
