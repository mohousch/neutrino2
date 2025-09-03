/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: scan.cpp 05102024 mohousch Exp $

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gui/scan.h>

#include <driver/rcinput.h>

#include <driver/gdi/color.h>

#include <gui/widget/messagebox.h>

#include <system/settings.h>
#include <system/debug.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/pictureviewer.h>


#define RED_BAR 	40
#define YELLOW_BAR 	70
#define GREEN_BAR 	100
#define BAR_BORDER 	2
#define BAR_WIDTH 	150
#define BAR_HEIGHT 	8

extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern CScanSettings * scanSettings;						// defined in scan_setup.cpp

//
CScanTs::CScanTs(CFrontend *f, CScanSettings *sc)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	radar = 0;
	total = done = 0;
	freqready = 0;
	
	// window size
	int _iw, _ih;
	frameBuffer->getIconSize(NEUTRINO_ICON_SCAN, &_iw, &_ih);
	hheight = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), _ih) + 6;
	
	//
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	
	width = MENU_WIDTH + 100;
	height = hheight + (10 * mheight); //9 lines
	
	x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
	y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - height) / 2;

	sigscale = new CCProgressBar(x + 20, y + height - mheight + (mheight - BAR_HEIGHT)/2, BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);
	
	int sw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth ("100% SNR");
	snrscale = new CCProgressBar(x + width - 20 - sw - ICON_OFFSET - BAR_WIDTH, y + height - mheight + (mheight - BAR_HEIGHT)/2, BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);
	
	fe = f;
	scanSettings = sc;
	
	//
	ypos_cur_satellite = y + hheight + (mheight >> 1);
	xpos1 = x + BORDER_LEFT;
	xpos2 = xpos1 + 10;
	
	dprintf(DEBUG_NORMAL, "CScanTs::CScanTs: fe(%d:%d) forcedDelsys:0x%x\n", fe->feadapter, fe->fenumber, fe->forcedDelSys);
}

int CScanTs::exec(CWidgetTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CScanTs::exec: actionKey: %s\n", actionKey.c_str());

	if(parent)
		parent->hide();
		
	////
	scanSettings->saveSettings(NEUTRINO_SCAN_SETTINGS_FILE);
	CZapit::getInstance()->saveFrontendConfig();
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int scan_mode = scanSettings->scan_mode;
	bool scan_all = actionKey == "all";
	bool test = actionKey == "test";
	bool manual = (actionKey == "manual") || test;
	success = false;
	
	sat_iterator_t sit;
	CZapit::ScanSatelliteList satList;
	CZapit::scanSatelliteList_t sat;
	transponder TP;

	// window size
	int _iw, _ih;
	frameBuffer->getIconSize(NEUTRINO_ICON_SCAN, &_iw, &_ih);
	hheight = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), _ih) + 6;
	
	//
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	
	width = MENU_WIDTH + 100;
	height = hheight + (10 * mheight); //9 lines
	
	x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
	y = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - height) / 2;
	
	xpos_radar = x + width - 90;
	ypos_radar = y + hheight + (mheight >> 1);
	////
	xpos1 = x + BORDER_LEFT;
	xpos2 = xpos1 + 10;

	if(scan_all)
		scan_mode |= 0xFF00;

	sigscale->reset();
	snrscale->reset();

	if (!frameBuffer->getActive())
		return CWidgetTarget::RETURN_EXIT_ALL;

	//
        CZapit::getInstance()->stopPlayBack();
	CSectionsd::getInstance()->pauseScanning(true);

	// refill satlist and set feparams for manuel scan
	satList.clear();

	if(manual) 
	{
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
		
		// freq
		TP.feparams.frequency = atoi(scanSettings->TP_freq);
		
		// delsys
		TP.feparams.delsys = scanSettings->TP_delsys;
		
		// inversion
		TP.feparams.inversion = INVERSION_AUTO;
		
#if HAVE_DVB_API_VERSION >= 5
		if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
		if(fe->getInfo()->type == FE_QPSK)
#endif
		{
			TP.feparams.symbol_rate = atoi(scanSettings->TP_rate);
			TP.feparams.fec_inner = (fe_code_rate_t) scanSettings->TP_fec;
			TP.feparams.polarization = scanSettings->TP_pol;

			dprintf(DEBUG_NORMAL, "CScanTs::exec: fe(%d:%d delsys:0x%x) freq %d rate %d fec %d pol %d\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), TP.feparams.frequency, TP.feparams.symbol_rate, TP.feparams.fec_inner, TP.feparams.polarization);
		} 
#if HAVE_DVB_API_VERSION >= 5 
		else if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
		else if ( fe->getInfo()->type == FE_QAM )
#endif
		{
			TP.feparams.symbol_rate	= atoi(scanSettings->TP_rate);
			TP.feparams.fec_inner	= (fe_code_rate_t)scanSettings->TP_fec;
			TP.feparams.modulation	= (fe_modulation_t) scanSettings->TP_mod;

			dprintf(DEBUG_NORMAL, "CScanTs::exec: fe(%d:%d delsys:0x%x) freq %d rate %d fec %d mod %d\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), TP.feparams.frequency, TP.feparams.symbol_rate, TP.feparams.fec_inner, TP.feparams.modulation);
		}
#if HAVE_DVB_API_VERSION >= 5
		else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_DTMB)
#else
		else if ( fe->getInfo()->type == FE_OFDM) 
#endif
		{
			TP.feparams.bandwidth =  (fe_bandwidth_t)scanSettings->TP_band;
			TP.feparams.code_rate_HP = (fe_code_rate_t)scanSettings->TP_HP; 
			TP.feparams.code_rate_LP = (fe_code_rate_t)scanSettings->TP_LP; 
			TP.feparams.modulation = (fe_modulation_t)scanSettings->TP_mod; 
			TP.feparams.transmission_mode = (fe_transmit_mode_t)scanSettings->TP_trans;
			TP.feparams.guard_interval = (fe_guard_interval_t)scanSettings->TP_guard;
			TP.feparams.hierarchy_information = (fe_hierarchy_t)scanSettings->TP_hierarchy;
			
#if HAVE_DVB_API_VERSION >= 5
			if (fe->getForcedDelSys() == CFrontend::DVB_T2)
				TP.feparams.plp_id = (unsigned int) atoi(scanSettings->TP_plp_id);
#endif

			dprintf(DEBUG_NORMAL, "CScanTs::exec: fe(%d:%d delsys:0x%x) freq %d band %d HP %d LP %d const %d trans %d guard %d hierarchy %d\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), TP.feparams.frequency, TP.feparams.bandwidth, TP.feparams.code_rate_HP, TP.feparams.code_rate_LP, TP.feparams.modulation, TP.feparams.transmission_mode, TP.feparams.guard_interval, TP.feparams.hierarchy_information);
		}
#if HAVE_DVB_API_VERSION >= 5 
		else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
		else if (fe->getInfo()->type == FE_ATSC)
#endif
		{
			TP.feparams.modulation	= (fe_modulation_t) scanSettings->TP_mod;

			dprintf(DEBUG_NORMAL, "CScanTs::exec: fe(%d:%d delsys:0x%x) freq:%d mod %d\n", fe->feadapter, fe->fenumber, fe->getForcedDelSys(), TP.feparams.frequency, TP.feparams.modulation);
		}
	}
	else if (!scan_all) // auto
	{
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
	}
	else if (scan_all) // all
	{
		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			if(sit->second.use_in_scan) 
			{
				// pos
				sat.position = sit->first;
				// name
				strncpy(sat.satName, sit->second.name.c_str(), 50);
				
				satList.push_back(sat);
			}
		}
	}
	
	// send bouquets mode
	CZapit::getInstance()->setScanBouquetMode( (CZapit::bouquetMode) scanSettings->bouquetMode);

	// send satellite list to zapit
	CZapit::getInstance()->setScanSatelliteList(satList);

        // send scantype to zapit
        CZapit::getInstance()->setScanType((CZapit::scanType) scanSettings->scanType );
	
	paint(test);
	
	frameBuffer->blit();	

	// go
	if(test) //test signal
	{
		int w = x + width - xpos2;
		char buffer[128];
		char *f, *s, *m;

#if HAVE_DVB_API_VERSION >= 5
		if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
		if(fe->getInfo()->type == FE_QPSK)
#endif
		{
			fe->getDelSys(scanSettings->TP_fec, dvbs_get_modulation((fe_code_rate_t)scanSettings->TP_fec), f, s, m);

			sprintf(buffer, "%u %c %d %s %s %s", atoi(scanSettings->TP_freq), scanSettings->TP_pol == 0 ? 'H' : 'V', atoi(scanSettings->TP_rate), f, s, m);
		} 
#if HAVE_DVB_API_VERSION >= 5 
		else if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
		else if ( fe->getInfo()->type == FE_QAM )
#endif
		{
			fe->getDelSys(scanSettings->TP_fec, scanSettings->TP_mod, f, s, m);

			sprintf(buffer, "%u %d %s %s %s", atoi(scanSettings->TP_freq), atoi(scanSettings->TP_rate), f, s, m);
		}
#if HAVE_DVB_API_VERSION >= 5
		else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_DTMB)
#else
		else if (fe->getInfo()->type == FE_OFDM) 
#endif
		{
			fe->getDelSys(scanSettings->TP_HP, scanSettings->TP_mod, f, s, m);

			sprintf(buffer, "%u %s %s %s", atoi(scanSettings->TP_freq), f, s, m);
		}
#if HAVE_DVB_API_VERSION >= 5
		else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
		else if(fe->getInfo()->type == FE_ATSC)
#endif
		{
			fe->getDelSys(FEC_NONE, scanSettings->TP_mod, f, s, m);

			sprintf(buffer, "%u %s %s %s", atoi(scanSettings->TP_freq), f, s, m);
		}

		paintLine(xpos2, ypos_cur_satellite, w - 95, scanSettings->satNameNoDiseqc);
		paintLine(xpos2, ypos_frequency, w, buffer);

		success = CZapit::getInstance()->tuneTP(TP, fe);
	} 
	else if(manual) // manual
	{
		CZapit::scanTP_t msg;
	
		msg.TP = TP;
		msg.scanmode = scan_mode;
		msg.fe = fe;
	
		success = CZapit::getInstance()->scanTP(msg);
	}
	else // auto / all
	{
		CZapit::scanProvider_t msg;
		
		msg.scanmode = scan_mode;
		msg.fe = fe;
	
		success = CZapit::getInstance()->startScan(msg);
	}

	// poll for messages
	istheend = !success;
	while (!istheend) 
	{
		paintRadar();
		showSNR();

		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd_MS( 250 );

		do {
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if (test && (msg <= CRCInput::RC_MaxRC)) 
			{
				istheend = true;
				msg = CRCInput::RC_timeout;
			}
			else if(msg == CRCInput::RC_home) 
			{
				if (MessageBox(_("Abortion of channel scan"), _("Should the search really be aborted?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes) 
				{
					msg = CRCInput::RC_timeout;
					istheend = true;
				}
			}
			else
			{
				msg = handleMsg(msg, data);
			}
				
			frameBuffer->blit();
		}
		while (!(msg == CRCInput::RC_timeout));
	}
	
	// to join scan thread
	CZapit::getInstance()->stopScan();

	//
	if(!test) 
	{
		const char * text = success ? _("Transponderscan finished.") : _("Transponderscan failed!");
		
		CCHeaders head(x, y, width, hheight, text, NEUTRINO_ICON_SCAN);
		const struct button_label btn = { NEUTRINO_ICON_BUTTON_HOME, " ", 0 };
			
		head.setButtons(&btn);
		head.paint();
			
		frameBuffer->blit();		
	
		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(0xFFFF);

		do {
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
			
			if ( msg <= CRCInput::RC_MaxRC )
				msg = CRCInput::RC_timeout;
			else
				CNeutrinoApp::getInstance()->handleMsg( msg, data );
				
			frameBuffer->blit();
		} while (!(msg == CRCInput::RC_timeout));
	}

	hide();
	
	//
	CSectionsd::getInstance()->pauseScanning(false);

	// zap
	if (CNeutrinoApp::getInstance()->getChannelList())
	{
		CNeutrinoApp::getInstance()->getChannelList()->zapToChannelID(CNeutrinoApp::getInstance()->getChannelList()->getActiveChannel_ChannelID(), true);
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return CWidgetTarget::RETURN_REPAINT;
}

neutrino_msg_t CScanTs::handleMsg(neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int w = x + width - xpos2;
	
	char buffer[128];
	char str[256];

	switch (msg) 
	{
		case NeutrinoMessages::EVT_SCAN_SATELLITE:
			paintLine(xpos2, ypos_cur_satellite, w - 100, (char *)data);
			break;
			
		case NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS:
			sprintf(buffer, "%d", data);
			paintLine(xpos2, ypos_transponder, w - 100, buffer);
			total = data;
	
			snprintf(str, 255, "scan: %d/%d", done, total);
			CLCD::getInstance()->showMenuText(0, str, -1, true);		
			break;
			
		case NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
			if (total == 0) data = 0;
			done = data;
			sprintf(buffer, "%d/%d", done, total);
			paintLine(xpos2, ypos_transponder, w - 100, buffer);
	
			snprintf(str, 255, "scan %d/%d", done, total);
			CLCD::getInstance()->showMenuText(0, str, -1, true);		
			break;
			
		case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP: 
			{
				FrontendParameters *feparams = (FrontendParameters*) data;
				char *f, *s, *m;
				
#if HAVE_DVB_API_VERSION >= 5
				if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
				if(fe->getInfo()->type == FE_QPSK)
#endif
				{
					fe->getDelSys(feparams->fec_inner, dvbs_get_modulation(feparams->fec_inner),  f, s, m);

					snprintf(buffer, sizeof(buffer), "%d %c %d %s %s %s ", feparams->frequency, feparams->polarization ? 'V' : 'H', feparams->symbol_rate, f, s, m);
				} 
#if HAVE_DVB_API_VERSION >= 5 
				else if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
				else if ( fe->getInfo()->type == FE_QAM )
#endif
				{
					fe->getDelSys(feparams->fec_inner, feparams->modulation, f, s, m);

					snprintf(buffer, sizeof(buffer), "%d %d %s %s %s ", feparams->frequency, feparams->symbol_rate, f, s, m);
				}
#if HAVE_DVB_API_VERSION >= 5
				else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_DTMB)
#else
				else if (fe->getInfo()->type == FE_OFDM) 
#endif
				{
					fe->getDelSys(feparams->code_rate_HP, feparams->modulation, f, s, m);

					snprintf(buffer, sizeof(buffer), "%d %s %s %s ", feparams->frequency, f, s, m);
				}
#if HAVE_DVB_API_VERSION >= 5
				else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
				else if(fe->getInfo()->type == FE_ATSC)
#endif
				{
					fe->getDelSys(FEC_NONE, feparams->modulation, f, s, m);

					snprintf(buffer, sizeof(buffer), "%d %s %s %s ", feparams->frequency, f, s, m);
				}
				
				paintLine(xpos2, ypos_frequency, w - 80, buffer);
			}
			break;
			
		case NeutrinoMessages::EVT_SCAN_PROVIDER:
			paintLine(xpos2, ypos_provider, w, (char *)data); // UTF-8
			break;
			
		case NeutrinoMessages::EVT_SCAN_SERVICENAME:
			paintLine(xpos2, ypos_channel, w, (char *)data); // UTF-8
			break;
			
		case NeutrinoMessages::EVT_SCAN_NUM_CHANNELS:
			sprintf(buffer, " = %d", data);
			paintLine(xpos1 + 3 * 72, ypos_service_numbers + mheight, width - 3 * 72 - 10, buffer);
			break;
			
		case NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1, ypos_service_numbers + mheight, 72, buffer);
			break;
			
		case NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1 + 72, ypos_service_numbers + mheight, 72, buffer);
			break;
			
		case NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1 + 2 * 72, ypos_service_numbers + mheight, 72, buffer);
			break;
			
		case NeutrinoMessages::EVT_SCAN_COMPLETE:
		case NeutrinoMessages::EVT_SCAN_FAILED:
			success = (msg == NeutrinoMessages::EVT_SCAN_COMPLETE);
			istheend = true;
			msg = CRCInput::RC_timeout;
			break;
			
		case CRCInput::RC_plus:
		case CRCInput::RC_minus:
		case CRCInput::RC_left:
		case CRCInput::RC_right:
			CNeutrinoApp::getInstance()->setVolume(msg, true);
			break;
			
		default:
			break;
	}
	
	return msg;
}

void CScanTs::paintRadar(void)
{
	dprintf(DEBUG_INFO, "CScanTs::paintRadar\n");
	
	char filename[30];
	
	sprintf(filename, "radar%d.raw", radar);
	radar = (radar + 1) % 10;
	frameBuffer->paintIcon8(filename, xpos_radar, ypos_radar, RADAR_OFFSET); // offset = 17
	
	frameBuffer->blit();
}

void CScanTs::hide()
{
	frameBuffer->paintBackground();

	frameBuffer->blit();

	freqready = 0;
}

void CScanTs::paintLineLocale(int _x, int * _y, int _width, const char* const l)
{
	frameBuffer->paintBoxRel(_x, *_y, _width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, *_y + mheight, _width, l, COL_MENUCONTENTINACTIVE_TEXT_PLUS_0, 0, true); // UTF-8
	*_y += mheight;
}

void CScanTs::paintLine(int _x, int _y, int width, const char * const txt)
{
	frameBuffer->paintBoxRel(_x, _y, width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + mheight, width, txt, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

void CScanTs::paint(bool fortest)
{
	dprintf(DEBUG_INFO, "CScanTs::paint\n");
	
	int ypos;
	int iw, ih;

	ypos = y;
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, fortest ? _("Test signal") : _("Scan transponder"));
	
	//
	CCHeaders head(x, ypos, width, hheight, fortest ? _("Test signal") : _("Scan transponder"), NEUTRINO_ICON_SCAN);
	const struct button_label btn = { NEUTRINO_ICON_BUTTON_HOME, " ", 0 };
			
	head.setButtons(&btn);
	
	// main box
	frameBuffer->paintBoxRel(x, ypos, width, height, COL_MENUCONTENT_PLUS_0, g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	
	//
	head.paint();
	
	// radar
	frameBuffer->loadPal("radar.pal", RADAR_OFFSET, RADAR_OFFSET + 20);	// 17/37
	
	// satellites
	ypos = y + hheight + (mheight >> 1);
	
	ypos_cur_satellite = ypos;
	
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	if(fe->getInfo()->type == FE_QPSK)
#endif
	{	//sat
		paintLineLocale(xpos1, &ypos, width - xpos1, _("Satellite:"));
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_("Satellite:"), true); // UTF-8
	}
#if HAVE_DVB_API_VERSION >= 5 
	else if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
	else if (fe->getInfo()->type == FE_QAM)
#endif
	{	//cable
		paintLineLocale(xpos1, &ypos, width - xpos1, _("Cable:"));
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_("Cable:"), true); // UTF-8
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{	//terrestrial
		paintLineLocale(xpos1, &ypos, width - xpos1, _("Terrestrial:"));
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_("Terrestrial:"), true); // UTF-8
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
	{	//terrestrial
		paintLineLocale(xpos1, &ypos, width - xpos1, _("Atsc:"));
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_("Atsc:"), true); // UTF-8
	}

	// transponder
	ypos_transponder = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, _("Transponders:"));
	xpos2 = greater_xpos(xpos2, _("Transponders:"));

	// frequency
	ypos_frequency = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, _("Frequency:"));
	xpos2 = greater_xpos(xpos2, _("Frequency:"));

	// provider
	ypos += mheight >> 1; // 1/2 blank line
	
	ypos_provider = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, _("Provider:"));
	xpos2 = greater_xpos(xpos2, _("Provider:"));
	
	// channel
	ypos_channel = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, _("Channel:"));
	xpos2 = greater_xpos(xpos2, _("Channel:"));

	// services (tv/radio/data/summe)
	ypos += mheight >> 1; // 1/2 blank line

	ypos_service_numbers = ypos; paintLineLocale(xpos1         , &ypos, 72                 , _("TV"));
	ypos = ypos_service_numbers; paintLineLocale(xpos1 +     56, &ypos, 72                 , _("Radio"));
	ypos = ypos_service_numbers; paintLineLocale(xpos1 + 2 * 72, &ypos, 72                 , _("Data") );
	ypos = ypos_service_numbers; paintLineLocale(xpos1 + 3 * 72, &ypos, width - 3 * 72 - 10, _("Total"));
}

int CScanTs::greater_xpos(int xpos, const char *const txt)
{
	int txt_xpos = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(txt, true); // UTF-8
	if (txt_xpos > xpos)
		return txt_xpos;
	else 
		return xpos;
}

void CScanTs::showSNR()
{
	dprintf(DEBUG_INFO, "CScanTs::showSNR\n");
	
	char percent[10];
	uint16_t ssig, ssnr;
	int sig, snr;
	int posx, posy;
	int sw;

	ssig = fe->getSignalStrength();
	ssnr = fe->getSignalNoiseRatio();

	snr = (ssnr & 0xFFFF) * 100 / 65535;
	sig = (ssig & 0xFFFF) * 100 / 65535;

	posy = y + height - mheight;

	if(sigscale->getPercent() != sig) 
	{
		sprintf(percent, "%d%% SIG", sig);
		sw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth ("100% SIG");

		sigscale->refresh(sig);

		posx = x + 20 + BAR_WIDTH + ICON_OFFSET;

		frameBuffer->paintBoxRel(posx, posy, sw, mheight, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString (posx, posy + mheight + (mheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2, sw, percent, COL_MENUCONTENT_TEXT_PLUS_0);
	}

	// snr
	if(snrscale->getPercent() != snr) 
	{
		sw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth ("100% SNR");
		posx = x + width - 20 - sw;
		sprintf(percent, "%d%% SNR", snr);
		
		snrscale->refresh(snr);

		frameBuffer->paintBoxRel(posx, posy, sw, mheight, COL_MENUCONTENT_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString (posx, posy + mheight + (mheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2, sw, percent, COL_MENUCONTENT_TEXT_PLUS_0);
	}
}

