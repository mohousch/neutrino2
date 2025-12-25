/*
	$Id: scan_setup.cpp 10082024 mohousch Exp $

	Neutrino-GUI  -   DBoxII-Project

	scan setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#include <gui/scan_setup.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/scan.h>
#include <gui/motorcontrol.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/hintbox.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <system/debug.h>


#ifndef TRANSMISSION_MODE_C1
#define TRANSMISSION_MODE_C1		7
#endif

#ifndef TRANSMISSION_MODE_C3780
#define TRANSMISSION_MODE_C3780		8
#endif

#ifndef GUARD_INTERVAL_PN420
#define GUARD_INTERVAL_PN420		8
#endif

#ifndef GUARD_INTERVAL_PN595 
#define GUARD_INTERVAL_PN595		9
#endif

#ifndef GUARD_INTERVAL_PN945
#define GUARD_INTERVAL_PN945		10
#endif

//// globals
static int dmode = CFrontend::NO_DISEQC;
//
char zapit_lat[20];						
char zapit_long[20];
////
extern satellite_map_t satellitePositions;					// defined in zapit.cpp
extern std::map<transponder_id_t, transponder> select_transponders;		// defined in zapit.cpp

// option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

//
#define SCANTS_SCANMODE_OPTION_COUNT 2
const keyval SCANTS_SCANMODE_OPTIONS[SCANTS_SCANMODE_OPTION_COUNT] =
{
	{ CZapit::SM_NIT, _("Network") },
	{ CZapit::SM_FAST, _("Fast") }
};

#define SCANTS_BOUQUET_OPTION_COUNT 2
const keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	{ CZapit::BM_DELETEBOUQUETS, _("create new") },
	{ CZapit::BM_UPDATEBOUQUETS, _("update") }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapit::ST_TVRADIO, _("TV & Radio") },
	{  CZapit::ST_TV, _("only tv") },
	{  CZapit::ST_RADIO, _("only radio") },
	{  CZapit::ST_ALL, _("all services") }
};

#define SATSETUP_DISEQC_OPTION_COUNT 8
const keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ CFrontend::NO_DISEQC          , _("no DiSEqC") },
	{ CFrontend::MINI_DISEQC        , _("Mini-DiSEqC") },
	{ CFrontend::DISEQC_1_0         , _("DiSEqC 1.0") },
	{ CFrontend::DISEQC_1_1         , _("DiSEqC 1.1") },
	{ CFrontend::DISEQC_ADVANCED    , _("Advanced") },
	{ CFrontend::DISEQC_UNICABLE    , _("Unicable") },
	{ CFrontend::DISEQC_UNICABLE2   , _("Unicable2 (JESS)") },
	{ CFrontend::SMATV_REMOTE_TUNING, _("SMATV Remote Tuning") }
};

#define SATSETUP_SCANTP_FEC_COUNT 28
#define CABLESETUP_SCANTP_FEC_COUNT 10
const keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
	{ FEC_NONE, _("FEC_NONE") },
	
        { FEC_1_2, "1/2" },
        { FEC_2_3, "2/3" },
        { FEC_3_4, "3/4" },
        { FEC_4_5, "4/5" },
        { FEC_5_6, "5/6" },
        { FEC_6_7, "6/7" },
        { FEC_7_8, "7/8" },
        { FEC_8_9, "8/9" },
        { FEC_AUTO, "AUTO" },

        { FEC_S2_QPSK_1_2, "1/2 s2 qpsk" },
        { FEC_S2_QPSK_2_3, "3/2 s2 qpsk" },
        { FEC_S2_QPSK_3_4, "3/4 s2 qpsk" },
        { FEC_S2_QPSK_5_6, "5/6 s2 qpsk" },
        { FEC_S2_QPSK_7_8, "7/8 s2 qpsk" },
        { FEC_S2_QPSK_8_9, "8/9 s2 qpsk" },
        { FEC_S2_QPSK_3_5, "3/5 s2 qpsk" },
        { FEC_S2_QPSK_4_5, "4/5 s2 qpsk" },
        { FEC_S2_QPSK_9_10, "9/10 s2 qpsk" },

        { FEC_S2_8PSK_1_2, "1/2 s2 8psk" },
        { FEC_S2_8PSK_2_3, "3/2 s2 8psk" },
        { FEC_S2_8PSK_3_4, "3/4 s2 8psk" },
        { FEC_S2_8PSK_5_6, "5/6 s2 8psk" },
        { FEC_S2_8PSK_7_8, "7/8 s2 8psk" },
        { FEC_S2_8PSK_8_9, "8/9 s2 8psk" },
        { FEC_S2_8PSK_3_5, "3/5 s2 8psk" },
        { FEC_S2_8PSK_4_5, "4/5 s2 8psk" },
        { FEC_S2_8PSK_9_10, "9/10 s2 8psk" }
};

#define CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT 6
const keyval CABLETERRESTRIALSETUP_SCANTP_MOD[CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT] =
{
	{ QAM_16, "QAM/16" },
	{ QAM_32, "QAM/32" },
	{ QAM_64, "QAM/64" },
	{ QAM_128, "QAM/128" },
	{ QAM_256, "QAM/256" },
	{ QAM_AUTO, "QAM_AUTO" }
};

#define SATSETUP_SCANTP_MOD_COUNT 2
const keyval SATSETUP_SCANTP_MOD[SATSETUP_SCANTP_MOD_COUNT] =
{
	// sat
	{ QPSK, "QPSK" },
#if HAVE_DVB_API_VERSION >= 5
	{ PSK_8, "PSK_8" }
#else
	{ PSK8, "PSK8" }
#endif
};

#define SATSETUP_SCANTP_BAND_COUNT 7
const keyval SATSETUP_SCANTP_BAND[SATSETUP_SCANTP_BAND_COUNT] =
{
	{ BANDWIDTH_8_MHZ, "BAND_8" },
	{ BANDWIDTH_7_MHZ, "BAND_7" },
	{ BANDWIDTH_6_MHZ, "BAND_6" },
	{ BANDWIDTH_AUTO, "BAND_AUTO" },
	{ BANDWIDTH_5_MHZ, "BAND_5" },
	{ BANDWIDTH_10_MHZ, "BAND_10" },
	{ BANDWIDTH_1_712_MHZ, "BAND_712" }
};

// transmition mode
#define TERRESTRIALSETUP_TRANSMIT_MODE_COUNT 9
const keyval TERRESTRIALSETUP_TRANSMIT_MODE[TERRESTRIALSETUP_TRANSMIT_MODE_COUNT] =
{
	{ TRANSMISSION_MODE_2K, "2K" },
	{ TRANSMISSION_MODE_8K, "8K" },
	{ TRANSMISSION_MODE_AUTO, "AUTO" },
	{ TRANSMISSION_MODE_4K, "4K" },
	{ TRANSMISSION_MODE_1K, "1K" },
	{ TRANSMISSION_MODE_16K, "16K" },
	{ TRANSMISSION_MODE_32K, "32K" },
	{ TRANSMISSION_MODE_C1, "C1" },
	{ TRANSMISSION_MODE_C3780, "C3780" }
};

// guard interval
#define TERRESTRIALSETUP_GUARD_INTERVAL_COUNT 11
const keyval TERRESTRIALSETUP_GUARD_INTERVAL[TERRESTRIALSETUP_GUARD_INTERVAL_COUNT] =
{
	{ GUARD_INTERVAL_1_32, "1_32" },
	{ GUARD_INTERVAL_1_16, "1_16" },
	{ GUARD_INTERVAL_1_8, "1_8" },
	{ GUARD_INTERVAL_1_4, "1_4"},
	{ GUARD_INTERVAL_AUTO, "AUTO"},
	{ GUARD_INTERVAL_1_128, "1_128" },
	{ GUARD_INTERVAL_19_128, "19_128" },
	{ GUARD_INTERVAL_19_256, "19_256" },
	{ GUARD_INTERVAL_PN420, "PN420" },
	{ GUARD_INTERVAL_PN595, "PN595" },
	{ GUARD_INTERVAL_PN945, "PN945" }
};

// hierarchy
#define TERRESTRIALSETUP_HIERARCHY_COUNT 5
const keyval TERRESTRIALSETUP_HIERARCHY[TERRESTRIALSETUP_HIERARCHY_COUNT] =
{
	{ HIERARCHY_NONE, "NONE" },
	{ HIERARCHY_1, "1" },
	{ HIERARCHY_2, "2" },
	{ HIERARCHY_4, "4"},
	{ HIERARCHY_AUTO, "AUTO"},
};

#define SATSETUP_SCANTP_POL_COUNT 2
const keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, "H" },
	{ 1, "V" }
};

#define DISEQC_ORDER_OPTION_COUNT 2
const keyval DISEQC_ORDER_OPTIONS[DISEQC_ORDER_OPTION_COUNT] =
{
	{ CFrontend::COMMITED_FIRST, _("Commited/Uncommited") },
	{ CFrontend::UNCOMMITED_FIRST, _("Uncommited/Commited") }
};

#define OPTIONS_SOUTH0_NORTH1_OPTION_COUNT 2
const keyval OPTIONS_SOUTH0_NORTH1_OPTIONS[OPTIONS_SOUTH0_NORTH1_OPTION_COUNT] =
{
	{0, _("South") },
	{1, _("North") }
};

#define OPTIONS_EAST0_WEST1_OPTION_COUNT 2
const keyval OPTIONS_EAST0_WEST1_OPTIONS[OPTIONS_EAST0_WEST1_OPTION_COUNT] =
{
	{0, _("East") },
	{1, _("West") }
};

// 
#define FRONTEND_MODE_SINGLE_OPTION_COUNT 2
#define FRONTEND_MODE_TWIN_OPTION_COUNT 3
const keyval FRONTEND_MODE_OPTIONS[FRONTEND_MODE_TWIN_OPTION_COUNT] =
{
	{ CFrontend::FE_SINGLE, _("direct connected") },
	{ CFrontend::FE_NOTCONNECTED, _("not connected") },
	{ CFrontend::FE_LOOP, _("loop") },
};

//
CScanSetup::CScanSetup(CFrontend *f)
{
	fe = f;
	
	dprintf(DEBUG_NORMAL, "CScanSetup::CScanSetup: fe(%d:%d) delsys:0x%x\n", fe->feadapter, fe->fenumber, fe->forcedDelSys);
	
	scanSettings = new CScanSettings(fe);
	scanTs = new CScanTs(fe, scanSettings);
	satNotify = new CSatelliteSetupNotifier(fe);
	feModeNotifier = new CScanSetupFEModeNotifier(fe);
	feDelSysNotifier = new CScanSetupDelSysNotifier(fe);
	item_freq = NULL;
	item_sr = NULL;
	freq_length = 8;
}

CScanSetup::~CScanSetup()
{
	if (scanTs)
	{
		delete scanTs;
		scanTs = NULL;
	}
	
	if (scanSettings)
	{
		delete scanSettings;
		scanSettings = NULL;
	}
	
	if (satNotify)
	{
		delete satNotify;
		satNotify = NULL;
	}
	
	if (feModeNotifier)
	{
		delete feModeNotifier;
		feModeNotifier = NULL;
	}
	
	if (feDelSysNotifier)
	{
		delete feDelSysNotifier;
		feDelSysNotifier = NULL;
	}
}

int CScanSetup::exec(CTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "CScanSetup::exec: actionKey: %s\n", actionKey.c_str());
	
	int res = CTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();

	if(actionKey == "save_scansettings") 
	{
		// hint box
		CHintBox * hintBox = new CHintBox(_("Information"), _("Save settings now")); // UTF-8
		hintBox->paint();
		
		// save scanSettings (scan.conf)
		if(!scanSettings->saveSettings(NEUTRINO_SCAN_SETTINGS_FILE))
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: error while saving scan-settings!\n");
		
		// send directly diseqc
#if HAVE_DVB_API_VERSION >= 5
		if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
		if(fe->getInfo()->type == FE_QPSK)
#endif
		{
			CZapit::getInstance()->saveMotorPositions();
		
			//gotoxx
			fe->gotoXXLatitude = strtod(zapit_lat, NULL);
			fe->gotoXXLongitude = strtod(zapit_long, NULL);
		}
		
		// set fe mode
		CZapit::getInstance()->setFEMode(fe->mode, fe);
		
		// save frontend.conf
		CZapit::getInstance()->saveFrontendConfig();
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
		
		return CTarget::RETURN_REPAINT;
	}
	else if(actionKey == "unisetup") 
	{
		return showUnicableSetup();
	}
	else if (actionKey == "allautoscansetup")
	{
		return showAllAutoScanSetup();
	}
	else if (actionKey == "autoscan")
	{
		return showAutoScanSetup();
	}
	else if (actionKey == "manualscan")
	{
		return showManualScanSetup();
	}
	else if (actionKey == "motorsetup")
	{
		return showMotorSetup();
	}
	else if (actionKey == "lnbsetup")
	{
		return showLNBSetup();
	}
	else if (actionKey == "satonoffsetup")
	{
		return showSatOnOffSetup();
	}
	else if (actionKey == "freq")
	{
		CStringInput * freq = new CStringInput(_("Frequency"), (char *) scanSettings->TP_freq, freq_length, NULL, NULL, "0123456789");
		freq->exec(NULL, "");
		item_freq->addOption((char *) scanSettings->TP_freq);
		
		delete freq;
		freq = NULL;
		
		return CTarget::RETURN_REPAINT;
	}
	else if (actionKey == "srate")
	{
		CStringInput * rate = new CStringInput(_("Symbol rate"), (char *) scanSettings->TP_rate, 8, NULL, NULL, "0123456789");
		rate->exec(NULL, "");
		item_sr->addOption((char *) scanSettings->TP_rate);
		
		delete rate;
		rate = NULL;
		
		return CTarget::RETURN_REPAINT;
	}
	else if (actionKey == "tpselect")
	{
		showTPSelect();
		
		showManualScanSetup();
		
		return RETURN_EXIT;
	}
	
	res = showScanService();
	
	return res;
}

int CScanSetup::showScanService()
{
	dprintf(DEBUG_NORMAL, "Tuner: %d:%d\n", fe->feadapter, fe->fenumber);
	
	int res = CTarget::RETURN_REPAINT;
	
	if(!fe)
		return res;
	
	// load scansettings 
	if( !scanSettings->loadSettings(NEUTRINO_SCAN_SETTINGS_FILE) )
		dprintf(DEBUG_NORMAL, "CScanSetup::CScanSetup: Loading of scan settings failed. Using defaults.\n");
		
	// 
	dmode = fe->diseqcType;
	
	// load motorposition
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		CZapit::getInstance()->loadMotorPositions();
	}
	
	//
	CWidget* widget = NULL;
	ClistBox* scansetup = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("scansetup");
	
	if (widget)
	{
		scansetup = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "scansetup";
		
		//
		scansetup = new ClistBox(&box);

		scansetup->setWidgetMode(ClistBox::MODE_SETUP);
		
		// head
		scansetup->enablePaintHead();
		scansetup->setTitle(_("Scan transponder"), NEUTRINO_ICON_SCAN);

		// foot
		scansetup->enablePaintFoot();	
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };	
		scansetup->setFootButtons(&btn);
		
		//
		widget->addCCItem(scansetup);
	}
	
	//
	oldLcdMode = CLCD::getInstance()->getMode();
	oldLcdMenutitle = CLCD::getInstance()->getMenutitle();
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, _("Scan transponder"));
	
	// intros
	scansetup->addItem(new CMenuForwarder(_("back")));
	scansetup->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	//save settings
	scansetup->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	scansetup->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// tuner mode
	bool have_twin = false;
	
	if(fe->getInfo()->type == FE_QPSK || fe->getInfo()->type == FE_OFDM)
	{
		have_twin = CZapit::getInstance()->FrontendIsTwin(fe);
	}
	
	scansetup->addItem(new CMenuOptionChooser(_("Tuner mode"),  (int *)&fe->mode, FRONTEND_MODE_OPTIONS, have_twin? FRONTEND_MODE_TWIN_OPTION_COUNT:FRONTEND_MODE_SINGLE_OPTION_COUNT, true, feModeNotifier));
	
	// tunertype (forceddelsys)
	if (fe->isHybrid())
	{
		// 
		if (fe->getDeliverySystem() & CFrontend::DVB_C)
			fe->forcedDelSys = CFrontend::DVB_C;
		else if (fe->getDeliverySystem() & CFrontend::DVB_T)
			fe->forcedDelSys = CFrontend::DVB_T;
		else if (fe->getDeliverySystem() & CFrontend::DVB_T2)
			fe->forcedDelSys = CFrontend::DVB_T2;
		//
		CMenuOptionChooser *tunerType = new CMenuOptionChooser(_("Tuner type"),  (int *)&fe->forcedDelSys);
		
		if (fe->getDeliverySystem() & CFrontend::DVB_C)
			tunerType->addOption("DVBC", CFrontend::DVB_C);
		if (fe->getDeliverySystem() & CFrontend::DVB_T)
			tunerType->addOption("DVBT", CFrontend::DVB_T);
		if (fe->getDeliverySystem() & CFrontend::DVB_T2)
			tunerType->addOption("DVBT2", CFrontend::DVB_T2);
			
		tunerType->setChangeObserver(feDelSysNotifier);
		tunerType->setActive(true);
		
		feModeNotifier->addItem(0, tunerType);
		
		scansetup->addItem(tunerType);
	}
	
	// voltage
	bool hidden = true;
	
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	if(fe->getInfo()->type == FE_OFDM)
#endif
	{
		hidden = false;
	}
	
	CMenuOptionChooser *ojVoltage = new CMenuOptionChooser(_("5 Volt"), (int *)&fe->powered, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
	ojVoltage->setHidden(hidden);
	feDelSysNotifier->addItem(ojVoltage);
	scansetup->addItem(ojVoltage);
	
	// separartor
	if (fe->mode == CFrontend::FE_NOTCONNECTED)
		hidden = true;
	else
		hidden = false;
		
	CMenuItem *item = new CMenuSeparator(CMenuSeparator::LINE);
	item->setHidden(hidden);
	feModeNotifier->addItem(0, item);
	scansetup->addItem(item);

	// scantype
	CMenuOptionChooser * ojScantype = new CMenuOptionChooser(_("Scan for services"), (int *)&scanSettings->scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, ((fe->mode != CFrontend::FE_NOTCONNECTED) && (fe->mode != CFrontend::FE_LOOP)));
	feModeNotifier->addItem(0, ojScantype);
	scansetup->addItem(ojScantype);
		
	// bqtsmode
	CMenuOptionChooser * ojBouquets = new CMenuOptionChooser(_("Bouquet"), (int *)&scanSettings->bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, ((fe->mode != CFrontend::FE_NOTCONNECTED) && (fe->mode != CFrontend::FE_LOOP)));
	feModeNotifier->addItem(0, ojBouquets);
	scansetup->addItem(ojBouquets);
	
	// scanmode
	CMenuOptionChooser * useNit = new CMenuOptionChooser(_("Scan Mode"), (int *)&scanSettings->scan_mode, SCANTS_SCANMODE_OPTIONS, SCANTS_SCANMODE_OPTION_COUNT, ( (fe->mode != CFrontend::FE_NOTCONNECTED) && (fe->mode != CFrontend::FE_LOOP) ));
	feModeNotifier->addItem(0, useNit);
	scansetup->addItem(useNit);
		
	// separator
	item = new CMenuSeparator(CMenuSeparator::LINE);
	item->setHidden(hidden);
	feModeNotifier->addItem(0, item);
	scansetup->addItem(item);
	
	// DVB_S / DVB_S2
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		// diseqc
		CMenuOptionChooser *ojDiseqc = new CMenuOptionChooser(_("DiSEqC"), (int *)&fe->diseqcType, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, satNotify, CRCInput::RC_nokey, "", true);
		feModeNotifier->addItem(1, ojDiseqc);
		
		// diseqc repeat
		CMenuOptionNumberChooser *ojDiseqcRepeats = new CMenuOptionNumberChooser(_("DiSEqC-repeats"), &fe->diseqcRepeats, true, 0, 2, NULL);
		ojDiseqcRepeats->setHidden((dmode == CFrontend::NO_DISEQC) || (dmode > CFrontend::DISEQC_ADVANCED) || (fe->mode == CFrontend::FE_NOTCONNECTED) || (fe->mode == CFrontend::FE_LOOP));
		satNotify->addItem(4, ojDiseqcRepeats);
		feModeNotifier->addItem(4, ojDiseqcRepeats);

		// unicablesetup
		CMenuForwarder *uniSetup = new CMenuForwarder(_("Unicable Setup"), true, NULL, this, "unisetup");
		uniSetup->setHidden((dmode > CFrontend::DISEQC_ADVANCED ? false : true));
		satNotify->addItem(3, uniSetup);
		feModeNotifier->addItem(3, uniSetup);

		// lnbsetup
		CMenuForwarder *lnbSetup = new CMenuForwarder(_("Setup satellites input / LNB"), true, NULL, this, "lnbsetup");
		lnbSetup->setHidden((fe->mode == CFrontend::FE_NOTCONNECTED) || (fe->mode == CFrontend::FE_LOOP));
		feModeNotifier->addItem(1, lnbSetup);
		
		// motorsetup
		CMenuForwarder *motorSetup = new CMenuForwarder(_("Motor settings"), true, NULL, this, "motorsetup");
		motorSetup->setHidden((fe->mode == CFrontend::FE_NOTCONNECTED) || (fe->mode == CFrontend::FE_LOOP));
		feModeNotifier->addItem(1, motorSetup);
		
		scansetup->addItem(ojDiseqc);		// diseqc
		scansetup->addItem(ojDiseqcRepeats);	// diseqcrepeat
		scansetup->addItem(uniSetup);		// unicablesetup
		scansetup->addItem(lnbSetup);		// lnbsetup
		scansetup->addItem(motorSetup); 	// motorsetup
	}
	
	// manual scan	
	CMenuForwarder * manScan = new CMenuForwarder(_("Manual frequency scan / Test signal"), (fe->mode != CFrontend::FE_NOTCONNECTED) && (fe->mode != CFrontend::FE_LOOP), NULL, this, "manualscan", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);
	feModeNotifier->addItem(0, manScan);
	
	scansetup->addItem(manScan);
		
	// autoscan
	CMenuForwarder * auScan = new CMenuForwarder(_("Auto-Scan"), (fe->mode != CFrontend::FE_NOTCONNECTED) && (fe->mode != CFrontend::FE_LOOP), NULL, this, "autoscan", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
	feModeNotifier->addItem(0, auScan);
	
	scansetup->addItem(auScan);

	// allautoscan ( DVB_S / DVB_S2)
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		CMenuForwarder *fautoScanAll = new CMenuForwarder(_("Auto-Scan multiple Satellites"), true, NULL, this, "allautoscansetup", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE);
		fautoScanAll->setHidden(( (dmode == CFrontend::NO_DISEQC) || (fe->mode == CFrontend::FE_NOTCONNECTED) || (fe->mode == CFrontend::FE_LOOP)));
		satNotify->addItem(2, fautoScanAll);
		feModeNotifier->addItem(2, fautoScanAll);
		
		scansetup->addItem(fautoScanAll);
	}

	res = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        CLCD::getInstance()->setMode(oldLcdMode, oldLcdMenutitle.c_str());
	
	return res;
}

// showMotorSetup
int CScanSetup::showMotorSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showMotorSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	CWidget* motorMenuWidget = NULL;
	ClistBox* motorMenu = NULL;
		
	//
	motorMenuWidget = CNeutrinoApp::getInstance()->getWidget("motorsetup");
		
	if (motorMenuWidget)
	{
		motorMenu = (ClistBox*)motorMenuWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		motorMenuWidget = new CWidget(&box);
		motorMenuWidget->name = "motorsetup";
		
		//
		motorMenu = new ClistBox(&box);

		motorMenu->setWidgetMode(ClistBox::MODE_SETUP);
			
		motorMenu->enablePaintHead();
		motorMenu->setTitle(_("Motor settings"), NEUTRINO_ICON_SCAN);

		motorMenu->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
				
		motorMenu->setFootButtons(&btn);
			
		//
		motorMenuWidget->addCCItem(motorMenu);
	}
		
	// intros
	motorMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	motorMenu->addItem(new CMenuForwarder(_("back")));

	// save settings
	motorMenu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	motorMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// speed
	motorMenu->addItem(new CMenuOptionNumberChooser(_("moving speed (10 = 1deg/sec)"), (int *)&fe->motorRotationSpeed, true, 0, 64, NULL) );

	// gotoxx
	motorMenu->addItem(new CMenuOptionChooser(_("Use gotoXX"),  (int *)&fe->useGotoXX, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	CStringInput * toff;
	CStringInput * taff;
	sprintf(zapit_lat, "%3.6f", fe->gotoXXLatitude);
	sprintf(zapit_long, "%3.6f", fe->gotoXXLongitude);

	// gotoxxladirection
	motorMenu->addItem(new CMenuOptionChooser(_("LaDirection"),  (int *)&fe->gotoXXLaDirection, OPTIONS_SOUTH0_NORTH1_OPTIONS, OPTIONS_SOUTH0_NORTH1_OPTION_COUNT, true));

	// latitude
	toff = new CStringInput(_("Latitude"), (char *) zapit_lat, 10, NULL, NULL, "0123456789.");
	motorMenu->addItem(new CMenuForwarder(_("Latitude"), true, zapit_lat, toff));

	// gotoxx lodirection
	motorMenu->addItem(new CMenuOptionChooser(_("LoDirection"),  (int *)&fe->gotoXXLoDirection, OPTIONS_EAST0_WEST1_OPTIONS, OPTIONS_EAST0_WEST1_OPTION_COUNT, true));

	// longitude
	taff = new CStringInput(_("Longitude"), (char *) zapit_long, 10, NULL, NULL, "0123456789.");
	motorMenu->addItem(new CMenuForwarder(_("Longitude"), true, zapit_long, taff));
		
	// usals repeat
	motorMenu->addItem(new CMenuOptionNumberChooser(_("USALS command repeat"), (int *)&fe->repeatUsals, true, 0, 10) );
		
	// rotor swap east/west
	motorMenu->addItem( new CMenuOptionChooser(_("Swap rotor east/west"), &g_settings.rotor_swap, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	//
	motorMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	motorMenu->addItem(new CMenuForwarder(_("Motor settings"), true, NULL, new CMotorControl(fe, scanSettings)));
	
	ret = motorMenuWidget->exec(this, "");
	
	if (motorMenuWidget)
	{
		delete motorMenuWidget;
		motorMenuWidget = NULL;
	}
	
	return ret;
}

// showUnicableSetup
int CScanSetup::showUnicableSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showUnicableSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	//
	CWidget* uniWidget = NULL;
	ClistBox* uni_setup = NULL;
	
	uniWidget = CNeutrinoApp::getInstance()->getWidget("unicablesetup");
	
	if (uniWidget)
	{
		uni_setup = (ClistBox*)uniWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		uniWidget = new CWidget(&box);
		uniWidget->name = "unicablesetup";
		
		//
		uni_setup = new ClistBox(&box);

		uni_setup->setWidgetMode(ClistBox::MODE_SETUP);
		
		uni_setup->enablePaintHead();
		uni_setup->setTitle(_("Unicable settings"), NEUTRINO_ICON_SCAN);

		uni_setup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		uni_setup->setFootButtons(&btn);
		
		//
		uniWidget->addCCItem(uni_setup);
	}

	uni_setup->addItem(new CMenuForwarder(_("back")));
	uni_setup->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// uni_scr
	CMenuOptionNumberChooser * uniscr = new CMenuOptionNumberChooser(_("Unicable SCR address"), &fe->uni_scr, true, -1, dmode == CFrontend::DISEQC_UNICABLE ? 7 : 31);
	uni_setup->addItem(uniscr);

	// uni_qrg
	CIntInput *uni_qrg = new CIntInput(_("Unicable frequency"), (int&)fe->uni_qrg, 4);
	CMenuForwarder * uniqrg = new CMenuForwarder(_("Unicable frequency"), true, uni_qrg->getValue(), uni_qrg);
	uni_setup->addItem(uniqrg);

	//
	ret = uniWidget->exec(this, "");
	
	if (uniWidget)
	{
		delete uniWidget;
		uniWidget = NULL;
	}

	return ret;
}

// showLNBSetup
int CScanSetup::showLNBSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showLNBSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	//
	CWidget* satSetupWidget = NULL;
	ClistBox* satSetup = NULL;
		
	satSetupWidget = CNeutrinoApp::getInstance()->getWidget("satsetup");
		
	if (satSetupWidget)
	{
		satSetup = (ClistBox*)satSetupWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		satSetupWidget = new CWidget(&box);
		satSetupWidget->name = "satsetup";
		
		//
		satSetup = new ClistBox(&box);

		satSetup->setWidgetMode(ClistBox::MODE_SETUP);
			
		satSetup->enablePaintHead();
		satSetup->setTitle(_("Sat settings"), NEUTRINO_ICON_SCAN);

		satSetup->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
				
		satSetup->setFootButtons(&btn);
			
		//
		satSetupWidget->addCCItem(satSetup);
	}
		
	satSetup->addItem(new CMenuForwarder(_("back")));
	satSetup->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// tmpSat
	CWidget* tempsatWidget = NULL;
	ClistBox* tempsatlistBox = NULL;
		
	tempsatWidget = CNeutrinoApp::getInstance()->getWidget("tempsat");
				
	if (tempsatWidget)
	{
		tempsatlistBox = (ClistBox*)tempsatWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		tempsatWidget = new CWidget(&box);
		tempsatWidget->name = "tempsat";
		tempsatWidget->enableSaveScreen();
		
		//
		tempsatlistBox = new ClistBox(&box);

		tempsatlistBox->setWidgetMode(ClistBox::MODE_SETUP);
					
		//
		tempsatlistBox->enablePaintHead();
					
		//
		tempsatlistBox->enablePaintFoot();		
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };		
		tempsatlistBox->setFootButtons(&btn);
					
		//
		tempsatWidget->addCCItem(tempsatlistBox);
	}

	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		// satname
		if(sit->second.system == CFrontend::DVB_S)
		{	
			//
//			if (tempsatlistBox->hasTitle())
			tempsatWidget->setTitle(sit->second.name.c_str(), NEUTRINO_ICON_SCAN);
			tempsatlistBox->setWidgetMode(ClistBox::MODE_SETUP);
				
			tempsatlistBox->clear();
				
			//
			tempsatlistBox->addItem(new CMenuForwarder(_("back")));
			tempsatlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
				
			// savesettings
			tempsatlistBox->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
			tempsatlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));

			// diseqc
			CMenuOptionNumberChooser * diseqc = new CMenuOptionNumberChooser(_("Diseqc input"), &sit->second.diseqc, ((dmode != CFrontend::NO_DISEQC) && (dmode != CFrontend::DISEQC_ADVANCED)), -1, 15, NULL, 1, -1);

			// commited input
			CMenuOptionNumberChooser * comm = new CMenuOptionNumberChooser(_("Commited/Uncommited"), &sit->second.commited, dmode == CFrontend::DISEQC_ADVANCED, -1, 15, NULL, 1, -1);

			// uncommited input
			CMenuOptionNumberChooser * uncomm = new CMenuOptionNumberChooser(_("Uncommited/Commited"), &sit->second.uncommited, dmode == CFrontend::DISEQC_ADVANCED, -1, 15, NULL, 1, -1);

			// motor position
			CMenuOptionNumberChooser * motor = new CMenuOptionNumberChooser(_("Rotor position"), &sit->second.motor_position, true, 0, 64);

			// usals
			CMenuOptionChooser * usals = new CMenuOptionChooser(_("Use gotoXX"),  &sit->second.use_usals, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

			satNotify->addItem(1, diseqc);
			satNotify->addItem(0, comm);
			satNotify->addItem(0, uncomm);

			CIntInput* lofL = new CIntInput(_("LNB Low Offset"), (int&) sit->second.lnbOffsetLow, 5);
			CIntInput* lofH = new CIntInput(_("LNB High Offset"), (int&) sit->second.lnbOffsetHigh, 5);
			CIntInput* lofS = new CIntInput(_("LNB switch Offset"), (int&) sit->second.lnbSwitch, 5);
					
			tempsatlistBox->addItem(diseqc);
			tempsatlistBox->addItem(comm);
			tempsatlistBox->addItem(uncomm);
			tempsatlistBox->addItem(motor);
			tempsatlistBox->addItem(usals);
			tempsatlistBox->addItem(new CMenuForwarder(_("LNB Low Offset"), true, lofL->getValue(), lofL ));
			tempsatlistBox->addItem(new CMenuForwarder(_("LNB High Offset"), true, lofH->getValue(), lofH ));
			tempsatlistBox->addItem(new CMenuForwarder(_("LNB switch Offset"), true, lofS->getValue(), lofS));
		}
		
		// sat setup
		satSetup->addItem(new CMenuForwarder(sit->second.name.c_str(), true, NULL, tempsatWidget));
	}
	
	ret = satSetupWidget->exec(this, "");
		
	if (tempsatWidget)
	{
		delete tempsatWidget;
		tempsatWidget = NULL;
	}
	
	if (satSetupWidget)
	{
		delete satSetupWidget;
		satSetupWidget = NULL;
	}
	
	return ret;
}

// showSatOnOffSetup
int CScanSetup::showSatOnOffSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showSatOnOffSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	//
	CWidget *satOnOffWidget = NULL;
	ClistBox *satOnOfflistBox = NULL;
			
	//
	satOnOffWidget = CNeutrinoApp::getInstance()->getWidget("satOnOff");
		
	if (satOnOffWidget)
	{
		satOnOfflistBox = (ClistBox*)satOnOffWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		satOnOffWidget = new CWidget(&box);
		satOnOffWidget->name = "satOnOff";
		
		//
		satOnOfflistBox = new ClistBox(&box);

		satOnOfflistBox->setWidgetMode(ClistBox::MODE_SETUP);
			
		satOnOfflistBox->enablePaintHead();
		satOnOfflistBox->setTitle(_("Satellite"), NEUTRINO_ICON_SCAN);

		satOnOfflistBox->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
				
		satOnOfflistBox->setFootButtons(&btn);
			
		//
		satOnOffWidget->addCCItem(satOnOfflistBox);
	}
	
	// intros
	satOnOfflistBox->addItem(new CMenuForwarder(_("back")));
	satOnOfflistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	//
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		// satname
		if(sit->second.system == CFrontend::DVB_S)
		{
			// inuse
			CMenuOptionChooser * inuse = new CMenuOptionChooser(sit->second.name.c_str(),  &sit->second.use_in_scan, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);
			
			satOnOfflistBox->addItem(inuse);
		}
	}
	
	ret = satOnOffWidget->exec(this, "");
	
	if (satOnOffWidget)
	{
		delete satOnOffWidget;
		satOnOffWidget = NULL;
	}
	
	return ret;
}

// showManualScanSetup
int CScanSetup::showManualScanSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showManualScanSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	// load scansettings 
	if( !scanSettings->loadSettings(NEUTRINO_SCAN_SETTINGS_FILE) )
		dprintf(DEBUG_NORMAL, "CScanSetup::CScanSetup: Loading of scan settings failed. Using defaults.\n");
	
	CWidget* manualScanWidget = NULL;
	ClistBox* manualScanlistBox = NULL;
	
	manualScanWidget = CNeutrinoApp::getInstance()->getWidget("manualscan");
	
	if (manualScanWidget)
	{
		manualScanlistBox = (ClistBox*)manualScanWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		manualScanWidget = new CWidget(&box);
		manualScanWidget->name = "manualscan";
		
		//
		manualScanlistBox = new ClistBox(&box);

		manualScanlistBox->setWidgetMode(ClistBox::MODE_SETUP);
		
		manualScanlistBox->enablePaintHead();
		manualScanlistBox->setTitle(_("Manual frequency scan / Test signal"), NEUTRINO_ICON_SCAN);

		manualScanlistBox->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		manualScanlistBox->setFootButtons(&btn);
		
		//
		manualScanWidget->addCCItem(manualScanlistBox);
	}

	// intros
	manualScanlistBox->addItem(new CMenuForwarder(_("back")));
	manualScanlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// save settings
	manualScanlistBox->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	manualScanlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// sat select
	CMenuOptionStringChooser * satSelect = NULL;
	
#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Cable"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			if(sit->second.system == CFrontend::DVB_C)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == CFrontend::DVB_T)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == CFrontend::DVB_A)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif 
	{
		satSelect = new CMenuOptionStringChooser(_("Satellite"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);
		
		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			// satname
			if(sit->second.system == CFrontend::DVB_S)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
	
	manualScanlistBox->addItem(satSelect);
		
	// TP select
	manualScanlistBox->addItem(new CMenuForwarder(_("Select transponder"), true, NULL, this, "tpselect"));
		
	// frequency
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
		freq_length = 8;
	else if (fe->getForcedDelSys() == CFrontend::DVB_C)
		freq_length = 6;
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_A)
		freq_length = 9;
#else
	switch (fe->getInfo()->type)
	{
		case FE_QPSK:
			freq_length = 8;
			break;
		
		case FE_QAM:
			freq_length = 6;
			break;
		
		case FE_OFDM:
		case FE_ATSC:
			freq_length = 9;
			break;
		
		default:
			freq_length = 8;
			break;
	}
#endif

	item_freq = new CMenuForwarder(_("Frequency"), true, scanSettings->TP_freq, this, "freq");
		
	manualScanlistBox->addItem(item_freq);
		
	// modulation(t/c)/polarisation(sat)
	CMenuOptionChooser * mod_pol = NULL;

#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	if(fe->getInfo()->type == FE_QPSK)
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Polarization"), (int *)&scanSettings->TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5 
	else if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
	else if (fe->getInfo()->type == FE_QAM)
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_mod, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_mod, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_mod, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}

	manualScanlistBox->addItem(mod_pol);

	// symbol rate
	item_sr = new CMenuForwarder(_("Symbol rate"), true, scanSettings->TP_rate, this, "srate");

	// fec
#if HAVE_DVB_API_VERSION >= 5
	int fec_count = (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X) ? SATSETUP_SCANTP_FEC_COUNT : CABLESETUP_SCANTP_FEC_COUNT;
#else
	int fec_count = (fe->getInfo()->type == FE_QPSK) ? SATSETUP_SCANTP_FEC_COUNT : CABLESETUP_SCANTP_FEC_COUNT;
#endif
	CMenuOptionChooser * fec = new CMenuOptionChooser(_("FEC"), (int *)&scanSettings->TP_fec, SATSETUP_SCANTP_FEC, fec_count, true);
	
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() != CFrontend::DVB_T && fe->getForcedDelSys() != CFrontend::DVB_T2 && fe->getForcedDelSys() != CFrontend::DVB_A)
#else	
	if(fe->getInfo()->type != FE_OFDM && fe->getInfo()->type != FE_ATSC)
#endif
	{
		// Rate
		manualScanlistBox->addItem(item_sr);
			
		// fec
		manualScanlistBox->addItem(fec);
	}

	// band/hp/lp/transmode/guard/hierarchy/plp_id
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		// Band
		CMenuOptionChooser * Band = new CMenuOptionChooser(_("Bandwidth"), (int *)&scanSettings->TP_band, SATSETUP_SCANTP_BAND, SATSETUP_SCANTP_BAND_COUNT, true);
		manualScanlistBox->addItem(Band);

		// HP
		CMenuOptionChooser * HP = new CMenuOptionChooser(_("Code Rate HP"), (int *)&scanSettings->TP_HP, SATSETUP_SCANTP_FEC, fec_count, true);
		manualScanlistBox->addItem(HP);

		// LP
		CMenuOptionChooser * LP = new CMenuOptionChooser(_("Code Rate LP"), (int *)&scanSettings->TP_LP, SATSETUP_SCANTP_FEC, fec_count, true);
		manualScanlistBox->addItem(LP);
		
		// transmition mode
		CMenuOptionChooser * TM = new CMenuOptionChooser(_("Transmission mode"), (int *)&scanSettings->TP_trans, TERRESTRIALSETUP_TRANSMIT_MODE, TERRESTRIALSETUP_TRANSMIT_MODE_COUNT, true);
		manualScanlistBox->addItem(TM);
		
		// guard intervall
		CMenuOptionChooser * GI = new CMenuOptionChooser(_("Guard Interval"), (int *)&scanSettings->TP_guard, TERRESTRIALSETUP_GUARD_INTERVAL, TERRESTRIALSETUP_GUARD_INTERVAL_COUNT, true);
		manualScanlistBox->addItem(GI);
		
		// hierarchy
		CMenuOptionChooser * HR = new CMenuOptionChooser(_("Hierarchy"), (int *)&scanSettings->TP_hierarchy, TERRESTRIALSETUP_HIERARCHY, TERRESTRIALSETUP_HIERARCHY_COUNT, true);
		manualScanlistBox->addItem(HR);
		
		// plp_id
#if HAVE_DVB_API_VERSION >= 5
		if (fe->getForcedDelSys() == CFrontend::DVB_T2)
		{
			CStringInput * plp = new CStringInput(_("PLP ID"), (char *) scanSettings->TP_plp_id, 3);
			CMenuForwarder * plp_id = new CMenuForwarder(_("PLP ID"), true, scanSettings->TP_plp_id, plp);
			manualScanlistBox->addItem(plp_id);
		}
#endif
	}	

	manualScanlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// testsignal
	manualScanlistBox->addItem(new CMenuForwarder(_("Test signal"), true, NULL, scanTs, "test", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW) );
		
	// scan
	manualScanlistBox->addItem(new CMenuForwarder(_("Start scan"), true, NULL, scanTs, "manual", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE) );
	
	ret = manualScanWidget->exec(this, "");
	
	if (manualScanWidget)
	{
		delete manualScanWidget;
		manualScanWidget = NULL;
	}

	return ret;
}

// showAutoScanSetup
int CScanSetup::showAutoScanSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showAutoScanSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	CWidget* autoScanWidget = NULL;
	ClistBox* autoScanlistBox = NULL;
	
	autoScanWidget = CNeutrinoApp::getInstance()->getWidget("autoscan");
	
	if (autoScanWidget)
	{
		autoScanlistBox = (ClistBox*)autoScanWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		autoScanWidget = new CWidget(&box);
		autoScanWidget->name = "autoscan";
		
		//
		autoScanlistBox = new ClistBox(&box);

		autoScanlistBox->setWidgetMode(ClistBox::MODE_SETUP);
		
		autoScanlistBox->enablePaintHead();
		autoScanlistBox->setTitle(_("Auto-Scan"), NEUTRINO_ICON_SCAN);

		autoScanlistBox->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		autoScanlistBox->setFootButtons(&btn);
		
		//
		autoScanWidget->addCCItem(autoScanlistBox);
	}
	
	// intros
	autoScanlistBox->addItem(new CMenuForwarder(_("back")));
	autoScanlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	// savesettings
	autoScanlistBox->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	autoScanlistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// satselect
	CMenuOptionStringChooser * satSelect = NULL;
#if HAVE_DVB_API_VERSION >= 5 
	if (fe->getForcedDelSys() == CFrontend::DVB_C)
#else
	if (fe->getInfo()->type == FE_QAM)
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Cable"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			if(sit->second.system == CFrontend::DVB_C)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2)
#else
	else if (fe->getInfo()->type == FE_OFDM) 
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == CFrontend::DVB_T)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
#else
	else if (fe->getInfo()->type == FE_ATSC)
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == CFrontend::DVB_A)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
#else
	else if(fe->getInfo()->type == FE_QPSK)
#endif 
	{
		satSelect = new CMenuOptionStringChooser(_("Satellite"), (char*)scanSettings->satNameNoDiseqc, true, NULL, CRCInput::RC_nokey, "", true);
		
		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			// satname
			if(sit->second.system == CFrontend::DVB_S)
			{
				satSelect->addOption(sit->second.name.c_str());
			}
		}
	}
	
	autoScanlistBox->addItem(satSelect);
		
	// scan
	autoScanlistBox->addItem(new CMenuForwarder(_("Start scan"), true, NULL, scanTs, "auto", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE) );
	
	ret = autoScanWidget->exec(this, "");
	
	if (autoScanWidget)
	{
		delete autoScanWidget;
		autoScanWidget = NULL;
	}

	return ret;
}

// showAllAutoScanSetup
int CScanSetup::showAllAutoScanSetup()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showAllAutoScanSetup\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	//
	CWidget* autoScanAllWidget = NULL;
	ClistBox* autoScanAlllistBox = NULL;
		
	autoScanAllWidget = CNeutrinoApp::getInstance()->getWidget("autoscanall");
		
	if (autoScanAllWidget)
	{
		autoScanAlllistBox = (ClistBox*)autoScanAllWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		autoScanAllWidget = new CWidget(&box);
		autoScanAllWidget->name = "autoscanall";
		
		//
		autoScanAlllistBox = new ClistBox(&box);

		autoScanAlllistBox->setWidgetMode(ClistBox::MODE_SETUP);
			
		autoScanAlllistBox->enablePaintHead();
		autoScanAlllistBox->setTitle(_("Auto-Scan multiple Satellites"), NEUTRINO_ICON_SCAN);

		autoScanAlllistBox->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
				
		autoScanAlllistBox->setFootButtons(&btn);
			
		//
		autoScanAllWidget->addCCItem(autoScanAlllistBox);
	}
			
	// intros
	autoScanAlllistBox->addItem(new CMenuForwarder(_("back")));
	autoScanAlllistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// save settings
	autoScanAlllistBox->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save_scansettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	autoScanAlllistBox->addItem(new CMenuSeparator(CMenuSeparator::LINE));
		
	// satonoff
	autoScanAlllistBox->addItem(new CMenuForwarder(_("Satellite"), true, NULL, this, "satonoffsetup"));
			
	// scants
	autoScanAlllistBox->addItem(new CMenuForwarder(_("Start scan"), true, NULL, scanTs, "all", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE) );
	
	ret = autoScanAllWidget->exec(this, "");

	if (autoScanAllWidget)
	{	
		delete autoScanAllWidget;
		autoScanAllWidget = NULL;
	}

	return ret;
}

int CScanSetup::showTPSelect()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showTPSelect\n");
	
	transponder_list_t::iterator tI;
	sat_iterator_t sit;
	t_satellite_position position = 0;
	std::map<int, transponder> tmplist;
	std::map<int, transponder>::iterator tmpI;
	int i;
	int select = -1;
	static int old_selected = 0;
	static t_satellite_position old_position = 0;

	// get satposition (loop throught satpos)
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		if(!strcmp(sit->second.name.c_str(), scanSettings->satNameNoDiseqc)) 
		{
			position = sit->first;
			break;
		}
	}

	if(old_position != position) 
	{
		old_selected = 0;
		old_position = position;
	}
	
	//
	CWidget* tpWidget = NULL;
	ClistBox* menu = NULL;
	
	tpWidget = CNeutrinoApp::getInstance()->getWidget("transponder");
	
	if (tpWidget)
	{
		menu = (ClistBox*)tpWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		tpWidget = new CWidget(&box);
		tpWidget->name = "transponder";
		
		//
		menu = new ClistBox(&box);

		menu->setWidgetMode(ClistBox::MODE_SETUP);
		
		//
		menu->enablePaintHead();
		menu->setTitle(_("Select transponder"), NEUTRINO_ICON_SCAN);

		//
		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		menu->setFootButtons(&btn);
		
		//
		tpWidget->addCCItem(menu);
	}
	
	//
	i = 0;

	for(tI = select_transponders.begin(); tI != select_transponders.end(); tI++) 
	{
		t_satellite_position satpos = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;
		if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
			satpos = -satpos;
		
		if(satpos != position)
			continue;

		char buf[128];
		char * f, *s, *m;
		
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
	{
		fe->getDelSys(tI->second.feparams.fec_inner, dvbs_get_modulation(tI->second.feparams.fec_inner),  f, s, m);

		snprintf(buf, sizeof(buf), "%d %c %d %s %s %s ", tI->second.feparams.frequency, tI->second.feparams.polarization ? 'V' : 'H', tI->second.feparams.symbol_rate, f, s, m);
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_C)
	{
		fe->getDelSys(tI->second.feparams.fec_inner, tI->second.feparams.modulation, f, s, m);

		snprintf(buf, sizeof(buf), "%d %d %s %s %s ", tI->second.feparams.frequency, tI->second.feparams.symbol_rate, f, s, m);
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_DTMB)
	{
		fe->getDelSys(tI->second.feparams.code_rate_HP, tI->second.feparams.modulation, f, s, m);

		snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency, f, s, m);
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
	{
		fe->getDelSys(FEC_NONE, tI->second.feparams.modulation, f, s, m);

		snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency, f, s, m);
	}
#else
		switch(fe->getInfo()->type) 
		{
			case FE_QPSK:
			{
				fe->getDelSys(tI->second.feparams.fec_inner, dvbs_get_modulation(tI->second.feparams.fec_inner),  f, s, m);

				snprintf(buf, sizeof(buf), "%d %c %d %s %s %s ", tI->second.feparams.frequency, tI->second.polarization ? 'V' : 'H', tI->second.feparams.symbol_rate, f, s, m);
			}
			break;

			case FE_QAM:
			{
				fe->getDelSys(tI->second.feparams.fec_inner, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %d %s %s %s ", tI->second.feparams.frequency, tI->second.feparams.symbol_rate, f, s, m);
			}
			break;

			case FE_OFDM:
			{
				fe->getDelSys(tI->second.feparams.code_rate_HP, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency, f, s, m);
			}
			break;
				
			case FE_ATSC:
            		{
				fe->getDelSys(FEC_NONE, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency, f, s, m);
			}
			break;
		}
#endif
		
		menu->addItem(new CMenuForwarder(buf), old_selected == i);
		tmplist.insert(std::pair <int, transponder>(i, tI->second));
		i++;
	}

	int retval = tpWidget->exec(this, "");
	
	select = menu->getSelected();
	
	if (tpWidget)
	{
		delete tpWidget;
		tpWidget = NULL;
	}

	if(select >= 0) 
	{
		old_selected = select;

		tmpI = tmplist.find(select);

		//
		sprintf(scanSettings->TP_freq, "%d", tmpI->second.feparams.frequency);
		scanSettings->TP_delsys = tmpI->second.feparams.delsys;
		
#if HAVE_DVB_API_VERSION >= 5
	if (fe->getForcedDelSys() & CFrontend::DVB_S || fe->getForcedDelSys() & CFrontend::DVB_S2 || fe->getForcedDelSys() & CFrontend::DVB_S2X)
	{
		sprintf(scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
		scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
		scanSettings->TP_pol = tmpI->second.feparams.polarization;
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_C)
	{
		sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
		scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
		scanSettings->TP_mod = tmpI->second.feparams.modulation;
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_T || fe->getForcedDelSys() == CFrontend::DVB_T2 || fe->getForcedDelSys() == CFrontend::DVB_DTMB)
	{
		scanSettings->TP_band = tmpI->second.feparams.bandwidth;
		scanSettings->TP_HP = tmpI->second.feparams.code_rate_HP;
		scanSettings->TP_LP = tmpI->second.feparams.code_rate_LP;
		scanSettings->TP_mod = tmpI->second.feparams.modulation;
		scanSettings->TP_trans = tmpI->second.feparams.transmission_mode;
		scanSettings->TP_guard = tmpI->second.feparams.guard_interval;
		scanSettings->TP_hierarchy = tmpI->second.feparams.hierarchy_information;
				
		if (fe->getForcedDelSys() == CFrontend::DVB_T2)
			sprintf( scanSettings->TP_plp_id, "%d", tmpI->second.feparams.plp_id);
	}
	else if (fe->getForcedDelSys() == CFrontend::DVB_A)
	{
		//sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
		//scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
		scanSettings->TP_mod = tmpI->second.feparams.modulation;
	}
#else
		switch(fe->getInfo()->type) 
		{
			case FE_QPSK:	
				sprintf(scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_pol = tmpI->second.feparams.polarization;
				break;

			case FE_QAM:	
				sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
				break;

			case FE_OFDM:
			{	
				scanSettings->TP_band = tmpI->second.feparams.bandwidth;
				scanSettings->TP_HP = tmpI->second.feparams.code_rate_HP;
				scanSettings->TP_LP = tmpI->second.feparams.code_rate_LP;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
				scanSettings->TP_trans = tmpI->second.feparams.transmission_mode;
				scanSettings->TP_guard = tmpI->second.feparams.guard_interval;
				scanSettings->TP_hierarchy = tmpI->second.feparams.hierarchy_information;
			}
			break;

			case FE_ATSC:
            		{	
				//sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				//scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
			}
			break;
		}
#endif	
	}
	
	// save scanSettings (scan.conf)
	if(!scanSettings->saveSettings(NEUTRINO_SCAN_SETTINGS_FILE))
		dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: error while saving scan-settings!\n");

	return retval;
}

//// CScanSettings
CScanSettings::CScanSettings(CFrontend * f)
	: configfile('\t')
{
	fe = f;
	
	dprintf(DEBUG_NORMAL, "CScanSettings::CScanSettings: fe(%d:%d) delsys:0x%x\n", fe->feadapter, fe->fenumber, fe->forcedDelSys);
	
	//
	//satNameNoDiseqc[0] = 0;
	strcpy(satNameNoDiseqc, "none");
	bouquetMode = CZapit::BM_UPDATEBOUQUETS;
	scanType = CZapit::ST_TVRADIO;
	scan_mode = CZapit::SM_FAST;
}

uint32_t CScanSettings::getConfigValue(CFrontend *fe, const char * name, uint32_t defval)
{
	char cfg_key[81];
	sprintf(cfg_key, "fe%d%d_%s", fe->feadapter, fe->fenumber, name);
	
	return configfile.getInt32(cfg_key, defval);
}

void CScanSettings::setConfigValue(CFrontend* fe, const char * name, uint32_t val)
{
	char cfg_key[81];
	
	sprintf(cfg_key, "fe%d%d_%s", fe->feadapter, fe->fenumber, name);
	configfile.setInt32(cfg_key, val);
}

bool CScanSettings::loadSettings(const char * const fileName)
{
	dprintf(DEBUG_NORMAL, "fe(%d:%d)\n", fe->feadapter, fe->fenumber);
	
	// if scan.conf not exists load default
	if(!configfile.loadConfig(fileName))
		printf("%s not found\n", fileName);
	
	if( !fe )
		return false;
		
	char cfg_key[81];
	
	// common
	scanType = (CZapit::scanType) getConfigValue(fe, "scanType", CZapit::ST_ALL);
	bouquetMode = (CZapit::bouquetMode) getConfigValue(fe, "bouquetMode", CZapit::BM_UPDATEBOUQUETS);
	scan_mode = getConfigValue(fe, "scan_mode", CZapit::SM_FAST); // NIT (0) or fast (1)
	
	// satname
	sprintf(cfg_key, "fe%d%d_satNameNoDiseqc", fe->feadapter, fe->fenumber);
	strcpy(satNameNoDiseqc, configfile.getString(cfg_key, "none").c_str());
	
	// freq
	sprintf(cfg_key, "fe%d%d_TP_freq", fe->feadapter, fe->fenumber);
	strcpy(TP_freq, configfile.getString(cfg_key, "10100000").c_str());
	
	// rate
	sprintf(cfg_key, "fe%d%d_TP_rate", fe->feadapter, fe->fenumber);
	strcpy(TP_rate, configfile.getString(cfg_key, "27500000").c_str());
	
	// modulation
	TP_mod = (fe_code_rate_t)getConfigValue(fe, "TP_mod", (fe_modulation_t) QAM_AUTO);

	// fec
	TP_fec = (fe_code_rate_t)getConfigValue(fe, "TP_fec", (fe_code_rate_t)FEC_AUTO);
		
	// pol
	TP_pol = getConfigValue(fe, "TP_pol", 0);
	
#if HAVE_DVB_API_VERSION >= 3
	if(TP_fec == 4) 
		TP_fec = 5;
#endif

	// band
	TP_band = (fe_bandwidth_t)getConfigValue(fe, "TP_band", (fe_bandwidth_t)BANDWIDTH_AUTO);
		
	// HP
	TP_HP = (fe_code_rate_t)getConfigValue(fe, "TP_HP", (fe_code_rate_t)FEC_AUTO);
		
	// LP
	TP_LP = (fe_code_rate_t)getConfigValue(fe, "TP_LP", (fe_code_rate_t)FEC_AUTO);
		
	// trams
	TP_trans = (fe_transmit_mode_t)getConfigValue(fe, "TP_trans", (fe_transmit_mode_t)TRANSMISSION_MODE_AUTO);
		
	// guard
	TP_guard = (fe_guard_interval_t)getConfigValue(fe, "TP_guard", (fe_guard_interval_t)GUARD_INTERVAL_AUTO);
		
	// hierarchy
	TP_hierarchy = (fe_hierarchy_t)getConfigValue(fe, "TP_hierarchy", (fe_hierarchy_t)HIERARCHY_AUTO);
		
	// plp_id
	sprintf(cfg_key, "fe%d%d_TP_plp_id", fe->feadapter, fe->fenumber);
	strcpy(TP_plp_id, configfile.getString(cfg_key, "000").c_str());
	
	// delsys
	TP_delsys = (uint32_t)getConfigValue(fe, "TP_delsys", CFrontend::UNDEFINED);

	return true;
}

bool CScanSettings::saveSettings(const char * const fileName)
{
	dprintf(DEBUG_NORMAL, "CScanSettings::saveSettings: fe(%d:%d)\n", fe->feadapter, fe->fenumber);
	
	char cfg_key[81];
	
	// common
	setConfigValue(fe, "scanType", scanType );
	setConfigValue(fe, "bouquetMode", bouquetMode );
	setConfigValue(fe, "scan_mode", scan_mode);
	
	//
	sprintf(cfg_key, "fe%d%d_satNameNoDiseqc", fe->feadapter, fe->fenumber);
	configfile.setString(cfg_key, satNameNoDiseqc );
	
	// freq
	sprintf(cfg_key, "fe%d%d_TP_freq", fe->feadapter, fe->fenumber);
	configfile.setString(cfg_key, TP_freq);
	
	// rate
	sprintf(cfg_key, "fe%d%d_TP_rate", fe->feadapter, fe->fenumber);
	configfile.setString(cfg_key, TP_rate);
	
	// modulation
	setConfigValue(fe, "TP_mod", TP_mod);

	// pol
	setConfigValue(fe, "TP_pol", TP_pol);
		
	// fec
	setConfigValue(fe, "TP_fec", TP_fec);

	// band
	setConfigValue(fe, "TP_band", TP_band);
		
	// HP
	setConfigValue(fe, "TP_HP", TP_HP);
		
	// LP
	setConfigValue(fe, "TP_LP", TP_LP);
		
	// trans
	setConfigValue(fe, "TP_trans", TP_trans);
		
	// guard
	setConfigValue(fe, "TP_guard", TP_guard);
		
	// hierarchy
	setConfigValue(fe, "TP_hierarchy", TP_hierarchy);
		
	// plp
	sprintf(cfg_key, "fe%d%d_TP_plp_id", fe->feadapter, fe->fenumber);
	configfile.setString(cfg_key, TP_plp_id);
	
	// delsys
	setConfigValue(fe, "TP_delsys", TP_delsys);

	//if(configfile.getModifiedFlag())
	{
		// save neu configuration
		configfile.saveConfig(fileName);
	}

	return true;
}

//// CSatelliteSetupNotifier
CSatelliteSetupNotifier::CSatelliteSetupNotifier(CFrontend *f)
{
	fe = f;
	
	dprintf(DEBUG_NORMAL, "CSatelliteSetupNotifier: fe(%d:%d)\n", fe->feadapter, fe->fenumber);
}

// list 0 =
// list 1 =
// list 2 =
// list 3 =
// list 4 =
void CSatelliteSetupNotifier::addItem(int list, CMenuItem* item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		case 3:
			items4.push_back(item);
			break;
		case 4:
			items5.push_back(item);
			break;
		default:
			break;
	}
}

// item1: comm uncomm
// item2: lnb diseqc input
// item3: auto scan all
// item4: unicable
// item5: diseqc repeats
bool CSatelliteSetupNotifier::changeNotify(const std::string&, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int type = *((int*) Data);

	if (type == CFrontend::NO_DISEQC) 
	{
		for(it = items1.begin(); it != items1.end(); it++)
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items2.begin(); it != items2.end(); it++)
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items3.begin(); it != items3.end(); it++)
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items4.begin(); it != items4.end(); it++)
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items5.begin(); it != items5.end(); it++)
		{
			if (*it) (*it)->setHidden(true);
		}
	}
	else if(type < CFrontend::DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}
	}
	else if(type == CFrontend::DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}
	}
	else if(type > CFrontend::DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}
	}

	dmode = type;

	return true;
}

//// CScanSetupFEModeNotifier
CScanSetupFEModeNotifier::CScanSetupFEModeNotifier(CFrontend* f)
{
	fe = f;
	
	dprintf(DEBUG_NORMAL, "fe(%d:%d)\n", fe->feadapter, fe->fenumber);
}

// list 0: main
// list 1: diseqc
// list 2: unisetup
// list 3: diseqc rep
// list 4: ?
void CScanSetupFEModeNotifier::addItem(int list, CMenuItem *item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;	
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		case 3:
			items4.push_back(item);
			break;
		case 4:
			items5.push_back(item);
			break;
		default:
			break;
	}
}

// items1: enabled for advanced diseqc settings
// items2: for diseqc != NO_DISEQC
// items3: disabled for NO_DISEQC
// items4:
// items5:
bool CScanSetupFEModeNotifier::changeNotify(const std::string&, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int FeMode = *((int*) Data);
	
	dprintf(DEBUG_NORMAL, "Femode:%d\n", FeMode);

	if ( (FeMode == CFrontend::FE_NOTCONNECTED) || (FeMode == CFrontend::FE_LOOP) ) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (*it) (*it)->setHidden(true);
		}
	}
	else
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			if (*it) (*it)->setHidden(false);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (dmode != CFrontend::NO_DISEQC)
			{
				if (*it) (*it)->setHidden(false);
			}
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (dmode > CFrontend::DISEQC_ADVANCED)
			{
				if (*it) (*it)->setHidden(false);
			}
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (dmode != CFrontend::NO_DISEQC && dmode < CFrontend::DISEQC_ADVANCED)
			{
				if (*it) (*it)->setHidden(false);
			}
		}
	}

	return true;
}

//// CScanSetupDelSysNotifier
CScanSetupDelSysNotifier::CScanSetupDelSysNotifier(CFrontend *f)
{
	fe = f;
	item = NULL;
	
	dprintf(DEBUG_NORMAL, "CScanSetupDelSysNotifier::CScanSetupDelSysNotifier: fe(%d:%d)\n", fe->feadapter, fe->fenumber);
}

void CScanSetupDelSysNotifier::addItem(CMenuItem *m)
{
	item = m;
}

bool CScanSetupDelSysNotifier::changeNotify(const std::string&, void *Data)
{
	uint32_t delsys = *((uint32_t*) Data);
	
	if (delsys == CFrontend::DVB_T || delsys == CFrontend::DVB_T2)
	{
		if (item)
			item->setHidden(false);
	}
	else
	{
		if (item)
			item->setHidden(true);
	}
	
	return true;
}

//// TunerSetup
int CTunerSetup::exec(CTarget *parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CTunerSetup::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = CTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	ret = showMenu();
	
	return ret;
}

int CTunerSetup::showMenu()
{
	dprintf(DEBUG_NORMAL, "CTunerSetup::showMenu\n");
	
	int ret = CTarget::RETURN_REPAINT;
	
	CWidget* widget = NULL;
	ClistBox* TunerSetup = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("tunersetup");
	
	if (widget)
	{
		TunerSetup = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
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
		widget->name = "tunersetup";
		
		//
		TunerSetup = new ClistBox(&box);
		
		TunerSetup->setWidgetMode(ClistBox::MODE_MENU);
		
		//
		TunerSetup->enablePaintHead();
		TunerSetup->setTitle(_("Servicescan"), NEUTRINO_ICON_SCAN);
		TunerSetup->enablePaintDate();
		
		//
		TunerSetup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		TunerSetup->setFootButtons(&btn);
		
		//		
		widget->addCCItem(TunerSetup);
	}
	
	TunerSetup->clearItems();
		
	// intros
	TunerSetup->addItem(new CMenuForwarder(_("back")));
	TunerSetup->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	//			
	for(int i = 0; i < CZapit::getInstance()->getFrontendCount(); i++)
	{
		CFrontend * fe = CZapit::getInstance()->getFE(i);
		char tbuf[255];
				
		sprintf(tbuf, "Tuner-%d: %s", i + 1, fe->getInfo()->name);
		TunerSetup->addItem(new CMenuForwarder(tbuf, true, NULL, new CScanSetup(fe) ));
	}
	
	ret = widget->exec(this, "");
	
	delete widget;
	widget = NULL;
	
	return ret;
}

