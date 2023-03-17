/*
	Neutrino-GUI  -   DBoxII-Project

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

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/motorcontrol.h>

#include <gui/widget/messagebox.h>

#include <system/settings.h>
#include <system/debug.h>

// zapit includes
#include <zapit/satconfig.h>
#include <zapit/frontend_c.h>


static int g_sig;
static int g_snr;
static int last_snr = 0;
static int moving = 0;

#define RED_BAR 40
#define YELLOW_BAR 70
#define GREEN_BAR 100

#define BAR_BORDER 2
#define BAR_WIDTH 100
#define BAR_HEIGHT 16 //(13 + BAR_BORDER*2)

extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern CScanSettings * scanSettings;

//
CMotorControl::CMotorControl(int num)
{
	Init();
	
	feindex = num;
}

void CMotorControl::Init(void)
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	satfindpid = -1;
	
	width = MENU_WIDTH;
	mheight = mheight - 2;
	height = hheight + (20 * mheight) - 5;
	height = height;

	x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
	y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - height) / 2;

	stepSize = 1; //default: 1 step
	stepMode = STEP_MODE_ON;
	installerMenue = false;
	motorPosition = 1;
	satellitePosition = 0;
	stepDelay = 10;
	
	//
	sigscale = new CProgressBar(x + 10 - 1, y + height - mheight - 5, BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);
	snrscale = new CProgressBar(x + 10 + 210 - 1, y + height - mheight - 5, BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);
}

int CMotorControl::exec(CMenuTarget* parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CMotorControl::exec:\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	TP_params TP;
	int wasgrow = 0;
	last_snr = 0;
	moving = 0;

        CZapit::ScanSatelliteList satList;
        CZapit::commandSetScanSatelliteList sat;
	sat_iterator_t sit;

	sigscale->reset();
	snrscale->reset();

	bool istheend = false;
	int lim_cmd;
	if (!frameBuffer->getActive())
		return RETURN_EXIT_ALL;
	
	if (parent)
		parent->hide();
		
        x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
        y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - height) / 2;

       	/* send satellite list to zapit */
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		if(!strcmp(sit->second.name.c_str(), scanSettings->satNameNoDiseqc)) 
		{
			sat.position = sit->first;
			strncpy(sat.satName, scanSettings->satNameNoDiseqc, 50);
			satList.push_back(sat);
			break;
		}
	}

       	CZapit::getInstance()->setScanSatelliteList( satList);

	TP.feparams.frequency = atoi( scanSettings->TP_freq);
	TP.feparams.u.qpsk.symbol_rate = atoi( scanSettings->TP_rate);
	TP.feparams.u.qpsk.fec_inner = (fe_code_rate_t) scanSettings->TP_fec;
	TP.polarization = scanSettings->TP_pol;

	CZapit::getInstance()->stopPlayBack();
	CZapit::getInstance()->tuneTP(TP, feindex);

	paint();
	paintMenu();
	paintStatus();
	
	frameBuffer->blit();

	while (!istheend)
	{
		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd_MS(250);
		msg = RC_nokey;

		while (!(msg == RC_timeout) && (!(msg == RC_home)))
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
			showSNR();
			//printf("SIG: %d SNR %d last %d\n", g_sig, g_snr, last_snr);
			if(moving && (stepMode == STEP_MODE_AUTO)) 
			{
				if(last_snr < g_snr) 
				{
					wasgrow = 1;
				}
				//if((last_snr > g_snr) && last_snr > 37000) {
				if(wasgrow && (last_snr > g_snr) && last_snr > 50) 
				{
					//printf("Must stop rotor!!!\n");
					CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0, feindex);
					moving = 0;
					paintStatus();
					last_snr = 0;
				} 
				else
					last_snr = g_snr;
			} 
			else
				wasgrow = 0;

			if (installerMenue)
			{
				switch(msg)
				{
					case RC_ok:
					case RC_0:
						printf("[motorcontrol] 0 key received... goto userMenue\n");
						installerMenue = false;
						paintMenu();
						paintStatus();
						break;
						
					case RC_1:
					case RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case RC_red:
					case RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0, feindex);
						moving = 0;
						paintStatus();
						break;

					case RC_3:
					case RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
						
					case RC_4:
						printf("[motorcontrol] 4 key received... set west (soft) limit\n");
						if(g_settings.rotor_swap) 
							lim_cmd = 0x66;
						else 
							lim_cmd = 0x67;
						
						CZapit::getInstance()->sendMotorCommand(0xE1, 0x31, lim_cmd, 0, 0, 0, feindex);
						break;
						
					case RC_5:
						printf("[motorcontrol] 5 key received... disable (soft) limits\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x63, 0, 0, 0, feindex);
						break;
					
					case RC_6:
						printf("[motorcontrol] 6 key received... set east (soft) limit\n");
						if(g_settings.rotor_swap) 
							lim_cmd = 0x67;
						else 
							lim_cmd = 0x66;
						
						CZapit::getInstance()->sendMotorCommand(0xE1, 0x31, lim_cmd, 0, 0, 0, feindex);
						break;
					
					case RC_7:
						printf("[motorcontrol] 7 key received... goto reference position\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x6B, 1, 0, 0, feindex);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_8:
						printf("[motorcontrol] 8 key received... enable (soft) limits\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x6A, 1, 0, 0, feindex);
						break;
					
					case RC_9:
						printf("[motorcontrol] 9 key received... (re)-calculate positions\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x6F, 1, 0, 0, feindex);
						break;
					
					case RC_plus:
					case RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_minus:
					case RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_blue:
						if (++stepMode > 3) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
							satellitePosition = 0;
						last_snr = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
						if ((msg >= RC_WithData) && (msg < RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
			else
			{
				switch(msg)
				{
					case RC_ok:
					case RC_0:
						printf("[motorcontrol] 0 key received... goto installerMenue\n");
						installerMenue = true;
						paintMenu();
						paintStatus();
						break;
						
					case RC_1:
					case RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case RC_red:
					case RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0, feindex);
						break;

					case RC_3:
					case RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
					
					case RC_green:
					case RC_5:
						printf("[motorcontrol] 5 key received... store present satellite number: %d\n", motorPosition);
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x6A, 1, motorPosition, 0, feindex);
						break;
					
					case RC_6:
						if (stepSize < 0x7F) stepSize++;
						printf("[motorcontrol] 6 key received... increase Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case RC_yellow:
					case RC_7:
						printf("[motorcontrol] 7 key received... goto satellite number: %d\n", motorPosition);
						CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x6B, 1, motorPosition, 0, feindex);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_9:
						if (stepSize > 1) stepSize--;
						printf("[motorcontrol] 9 key received... decrease Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case RC_plus:
					case RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_minus:
					case RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case RC_blue:
						if (++stepMode > 2) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
							satellitePosition = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
						if ((msg >= RC_WithData) && (msg < RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
		}
		
		istheend = (msg == RC_home);
		
		//NOTE: think about multi tuner, this zap back to the last shown channel (live_channel_id will not change)
		if(istheend)
		{
			//g_Zapit->Rezap();
			// zap
			if (CNeutrinoApp::getInstance()->channelList)
			{
				CNeutrinoApp::getInstance()->channelList->zapTo_ChannelID(CNeutrinoApp::getInstance()->channelList->getActiveChannel_ChannelID(), true);
			}
		}
		
		frameBuffer->blit();	
	}
	
	hide();

	return RETURN_REPAINT;
}

void CMotorControl::motorStepWest(void)
{
	int cmd;
	printf("[motorcontrol] motorStepWest\n");
	if(g_settings.rotor_swap) 
		cmd = 0x68;
	else 
		cmd = 0x69;
	
	switch(stepMode)
	{
		case STEP_MODE_ON:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, (-1 * stepSize), 0, feindex);
			satellitePosition += stepSize;
			break;
			
		case STEP_MODE_TIMED:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, 40, 0, feindex);
			usleep(stepSize * stepDelay * 1000);
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0, feindex); //halt motor
			satellitePosition += stepSize;
			break;
			
		case STEP_MODE_AUTO:
			moving = 1;
			paintStatus();
		default:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, 40, 0, feindex);
	}
}	

void CMotorControl::motorStepEast(void)
{
	int cmd;
	if(g_settings.rotor_swap) 
		cmd = 0x69;
	else 
		cmd = 0x68;
	
	printf("[motorcontrol] motorStepEast\n");
	
	switch(stepMode)
	{
		case STEP_MODE_ON:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, (-1 * stepSize), 0, feindex);
			satellitePosition -= stepSize;
			break;
			
		case STEP_MODE_TIMED:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, 40, 0, feindex);
			usleep(stepSize * stepDelay * 1000);
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0, feindex); //halt motor
			satellitePosition -= stepSize;
			break;
			
		case STEP_MODE_AUTO:
			moving = 1;
		default:
			CZapit::getInstance()->sendMotorCommand(0xE0, 0x31, cmd, 1, 40, 0, feindex);
	}
}

void CMotorControl::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height + 20); //20:???

	frameBuffer->blit();

	stopSatFind();
}

void CMotorControl::paintLine(int _x, int * _y, int _width, char * txt)
{
	*_y += mheight;
	frameBuffer->paintBoxRel(_x, *_y - mheight, _width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(_x, *_y, _width, txt, COL_MENUCONTENT, 0, true);
}

void CMotorControl::paintLine(int _x, int _y, int _width, char * txt)
{
	//frameBuffer->paintBoxRel(_x, _y - mheight, _width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(_x, _y, _width, txt, COL_MENUCONTENT, 0, true);
}

void CMotorControl::paintSeparator(int xpos, int * _ypos, int _width, char * /*txt*/)
{
	//int stringwidth = 0;
	//int stringstartposX = 0;
	int th = 10;
	//*ypos += mheight;
	*_ypos += th;
	frameBuffer->paintHLineRel(xpos, _width - 20, *_ypos - (th >> 1), COL_MENUCONTENT_PLUS_3);
	frameBuffer->paintHLineRel(xpos, _width - 20, *_ypos - (th >> 1) + 1, COL_MENUCONTENT_PLUS_1);
	
#if 0
	stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(txt);
	stringstartposX = 0;
	stringstartposX = (xpos + (width >> 1)) - (stringwidth >> 1);
	frameBuffer->paintBoxRel(stringstartposX - 5, *ypos - mheight, stringwidth + 10, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, *ypos, stringwidth, txt, COL_MENUCONTENT);
#endif
}

void CMotorControl::paintStatus()
{
	char buf[256];
	char buf2[256];
	
	int xpos1 = x + 10;
	int xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getRenderWidth(_("(a) Motor Position:"));
	int width2 = width - (xpos2 - xpos1) - 20;
	int width1 = width - 20;
	
	ypos = ypos_status;
	paintSeparator(xpos1, &ypos, width, _("Motor Setup"));
	
	paintLine(xpos1, &ypos, width1, _("(a) Motor Position:"));
	sprintf(buf, "%d", motorPosition);
	paintLine(xpos2, ypos, width2 , buf);
	
	paintLine(xpos1, &ypos, width1, _("(b) Movement:"));
	
	switch(stepMode)
	{
		case STEP_MODE_ON:
			strcpy(buf, _("Step Mode"));
			break;
			
		case STEP_MODE_OFF:
			strcpy(buf, _("Drive Mode"));
			break;
			
		case STEP_MODE_AUTO:
			strcpy(buf, _("Drive Mode/auto stop"));
			break;
			
		case STEP_MODE_TIMED:
			strcpy(buf, _("Timed Step Mode"));
			break;
	}
	
	paintLine(xpos2, ypos, width2, buf);
	
	paintLine(xpos1, &ypos, width1, _("(c) Step Size:"));
	
	switch(stepMode)
	{
		case STEP_MODE_ON:
			sprintf(buf, "%d", stepSize);
			break;
			
		case STEP_MODE_AUTO:
			if(moving)
				strcpy(buf, _("stop on good signal(moving)"));
			else
				strcpy(buf, _("stop on good signal(stopped)"));
			break;
			
		case STEP_MODE_OFF:
			strcpy(buf, _("don't care"));
			break;
			
		case STEP_MODE_TIMED:
			sprintf(buf, "%d ", stepSize * stepDelay);
			strcat(buf, _("milliseconds"));
			break;
	}
	paintLine(xpos2, ypos, width2, buf);
	
	paintSeparator(xpos1, &ypos, width, _("Status"));
	strcpy(buf, _("Satellite Position (Step Mode):"));
	sprintf(buf2, "%d", satellitePosition);
	strcat(buf, buf2);
	paintLine(xpos1, &ypos, width1, buf);
	paintSeparator(xpos1, &ypos, width, _("Motor Control Settings"));
}

void CMotorControl::paint()
{
	ypos = y;
	
	// headBox
	CHeaders head(x, ypos, width, hheight, _("Motor Setup"), NEUTRINO_ICON_SCAN);
	head.paint();
	
	// footer
	CFooters foot(x, ypos + hheight, width, height - hheight);
	foot.paint();

	ypos += hheight + (mheight >> 1) - 10;
	ypos_menue = ypos;
}

void CMotorControl::paintMenu()
{
	ypos = ypos_menue;

	int xpos1 = x + 10;
	int xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("(7/yellow)");
	int width2 = width - (xpos2 - xpos1) - 20;
	int width1 = width - 20;

	paintLine(xpos1, &ypos, width1, (char *) "(0/OK)");
	if(installerMenue)
		paintLine(xpos2, ypos, width2, _("User menu"));
	else
		paintLine(xpos2, ypos, width2, _("Install menu"));

	paintLine(xpos1, &ypos, width1, (char *) "(1/right)");
	paintLine(xpos2, ypos, width2, _("Step/Drive Motor West (b,c)"));
	paintLine(xpos1, &ypos, width1, (char *) "(2/red)");
	paintLine(xpos2, ypos, width2, _("Halt Motor"));
	paintLine(xpos1, &ypos, width1, (char *) "(3/left)");
	paintLine(xpos2, ypos, width2, _("Step/Drive Motor East (b,c)"));

	if (installerMenue)
	{
		paintLine(xpos1, &ypos, width1,(char *)  "(4)");
		paintLine(xpos2, ypos, width2, _("Set West (soft) Limit"));
		paintLine(xpos1, &ypos, width1, (char *) "(5)");
		paintLine(xpos2, ypos, width2, _("Disable (soft) Limits"));
		paintLine(xpos1, &ypos, width1, (char *) "(6)");
		paintLine(xpos2, ypos, width2, _("Set East (soft) Limit"));
		paintLine(xpos1, &ypos, width1, (char *) "(7)");
		paintLine(xpos2, ypos, width2, _("Goto Reference Position"));
		paintLine(xpos1, &ypos, width1, (char *) "(8)");
		paintLine(xpos2, ypos, width2, _("Enable (soft) Limits"));
		paintLine(xpos1, &ypos, width1, (char *) "(9)");
		paintLine(xpos2, ypos, width2, _("(Re)-Calculate Positions"));
		paintLine(xpos1, &ypos, width1, (char *) "(+/up)");
		paintLine(xpos2, ypos, width2, _("Increase Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(-/down)");
		paintLine(xpos2, ypos, width2, _("Decrease Motor Position (a)"));
		paintLine(xpos1, &ypos, width1,(char *)  "(blue)");
		paintLine(xpos2, ypos, width2, _("Switch Step/Drive Mode (b)"));
	}
	else
	{
		paintLine(xpos1, &ypos, width1, (char *) "(4)");
		paintLine(xpos2, ypos, width2, _("Not used"));
		paintLine(xpos1, &ypos, width1, (char *) "(5/green)");
		paintLine(xpos2, ypos, width2, _("Store Motor Position (a)"));
		paintLine(xpos1, &ypos, width1,(char *)  "(6)");
		paintLine(xpos2, ypos, width2, _("Increase Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(7/yellow)");
		paintLine(xpos2, ypos, width2, _("Goto Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(8)");
		paintLine(xpos2, ypos, width2, _("Not used"));
		paintLine(xpos1, &ypos, width1, (char *) "(9)");
		paintLine(xpos2, ypos, width2, _("Decrease Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(+/up)");
		paintLine(xpos2, ypos, width2, _("Increase Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(-/down)");
		paintLine(xpos2, ypos, width2, _("Decrease Motor Position (a)"));
		paintLine(xpos1, &ypos, width1, (char *) "(blue)");
		paintLine(xpos2, ypos, width2, _("Switch Step/Drive Mode (b)"));
	}
	
	ypos_status = ypos;
}

void CMotorControl::startSatFind(void)
{
#if 0
	if (satfindpid != -1) 
	{
		kill(satfindpid, SIGKILL);
		waitpid(satfindpid, 0, 0);
		satfindpid = -1;
	}
		
	switch ((satfindpid = fork())) 
	{
		case -1:
			printf("[motorcontrol] fork");
			break;
		case 0:
			printf("[motorcontrol] starting satfind...\n");
#if HAVE_DVB_API_VERSION >= 3
			if (execlp("/bin/satfind", "satfind", NULL) < 0)
#else
			//if (execlp("/bin/satfind", "satfind", "--tune", NULL) < 0)
			if (execlp("/bin/satfind", "satfind", NULL) < 0)
#endif
				printf("[motorcontrol] execlp satfind failed.\n");		
			break;
	} /* switch */
#endif
}

void CMotorControl::stopSatFind(void)
{
	
	if (satfindpid != -1) 
	{
		printf("[motorcontrol] killing satfind...\n");
		kill(satfindpid, SIGKILL);
		waitpid(satfindpid, 0, 0);
		satfindpid = -1;
	}
}

#define BARWT 10 
#define BAR_BL 2
#define BARW (BARWT - BAR_BL)
#define BARWW (BARWT - BARW)

void CMotorControl::showSNR()
{
	char percent[10];
	//char ber[20];
	int barwidth = 100;
	uint16_t ssig, ssnr;
	int sig, snr;
	int bheight, posx, posy;

	int sw;
	
	ssig = CZapit::getInstance()->getFE(feindex)->getSignalStrength();
	ssnr = CZapit::getInstance()->getFE(feindex)->getSignalNoiseRatio();

	snr = (ssnr & 0xFFFF) * 100 / 65535;
	sig = (ssig & 0xFFFF) * 100 / 65535;
	
	if(sig < 5)
		return;
	
	g_sig = ssig & 0xFFFF;
	g_snr = snr;

	bheight = mheight - 5;
	posy = y + height - mheight - 5;

	if(sigscale->getPercent() != sig) 
	{
		posx = x + 10;
		sprintf(percent, "%d%% SIG", sig);
		sw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getRenderWidth ("100% SIG");

		//sigscale->setPosition(posx - 1, posy, BAR_WIDTH, BAR_HEIGHT);
		sigscale->paint(/*x + 10 - 1, y + height - mheight - 5,*/ sig);

		posx = posx + barwidth + 3;
		frameBuffer->paintBoxRel(posx, posy - 2, sw+4, mheight, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString (posx+2, posy + mheight, sw, percent, COL_MENUCONTENT);
	}

	if(snrscale->getPercent() != snr) 
	{
		posx = x + 10 + 210;
		sprintf(percent, "%d%% SNR", snr);
		sw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getRenderWidth ("100% SNR");
		
		//snrscale->setPosition(posx - 1, posy, BAR_WIDTH, BAR_HEIGHT);
		snrscale->paint(/*x + 10 + 210 - 1, y + height - mheight - 5,*/ snr);

		posx = posx + barwidth + 3;
		frameBuffer->paintBoxRel(posx, posy - 2, sw+4, mheight, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString (posx+2, posy + mheight, sw, percent, COL_MENUCONTENT);
	}
}
