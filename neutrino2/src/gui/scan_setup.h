/*
	$Id: scan_setup.h 05102024 mohousch Exp $

	Copyright (C) 2009 Thilo Graf (dbt)
	http://www.dbox2-tuning.de

	Neutrino-GUI  -   DBoxII-Project

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

#ifndef __scan_setup__
#define __scan_setup__

#include <string>

#include <gui/widget/icons.h>
#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <gui/scan.h>

#include <driver/gdi/framebuffer.h>

#include <zapit/frontend_c.h>


////
class CScanSettings
{
	private:
		CFrontend *fe;
		
		uint32_t getConfigValue(CFrontend* fe, const char * name, uint32_t defval);
		void setConfigValue(CFrontend* fe, const char * name, uint32_t val);
		
	public:
		CConfigFile		configfile;
		
		unsigned int		bouquetMode;
		unsigned int		scanType;
		char            	satNameNoDiseqc[50];
		unsigned int		scan_mode;
		unsigned int		TP_fec;
		unsigned int		TP_pol;
		unsigned int		TP_mod;
		char			TP_freq[10];
		char			TP_rate[9];
		unsigned int		TP_band;
		unsigned int 		TP_HP;
		unsigned int 		TP_LP;
		unsigned int		TP_trans;
		unsigned int		TP_guard;
		unsigned int		TP_hierarchy;
		char			TP_plp_id[4];
		uint32_t		TP_delsys;
	
		CScanSettings(CFrontend *f);
		virtual ~CScanSettings(){fe = NULL;};
	
		bool loadSettings(const char * const fileName);
		bool saveSettings(const char * const fileName);
};

////
class CTPSelectHandler : public CWidgetTarget
{
	private:
		CFrontend * fe;
		CScanSettings * scanSettings;
	
	public:
		CTPSelectHandler(CFrontend* f = NULL, CScanSettings * sc = NULL);
		virtual ~CTPSelectHandler(){};
		
		int exec(CWidgetTarget *parent, const std::string &actionkey);
};

//// satsetupnotifuer
class CSatelliteSetupNotifier : public CChangeObserver
{
	private:
		std::vector<CMenuItem*> items1;
		std::vector<CMenuItem*> items2;
		std::vector<CMenuItem*> items3;
		std::vector<CMenuItem*> items4;
		std::vector<CMenuItem*> items5;
		
		CFrontend * fe;
	public:
		CSatelliteSetupNotifier(CFrontend *f);
		virtual ~CSatelliteSetupNotifier(){items1.clear(); items2.clear(); items3.clear(); items4.clear(); items5.clear();};
		
		void addItem(int list, CMenuItem *item);
		bool changeNotify(const std::string&, void * Data);
};

//// scansetupfemodenotifier
class CScanSetupFEModeNotifier : public CChangeObserver
{
	private:
		std::vector<CMenuItem*> items1;
		std::vector<CMenuItem*> items2;
		std::vector<CMenuItem*> items3;
		std::vector<CMenuItem*> items4;
		std::vector<CMenuItem*> items5;
		
		CFrontend *fe;
	public:
		CScanSetupFEModeNotifier(CFrontend *f);
		virtual ~CScanSetupFEModeNotifier(){items1.clear(); items2.clear(); items3.clear(); items4.clear(); items5.clear();};
		
		void addItem(int list, CMenuItem *item);
		bool changeNotify(const std::string&, void * Data);
};

//// scanSetupDelSysNotifier
class CScanSetupDelSysNotifier : public CChangeObserver
{
	private:
		CFrontend * fe;
		CMenuItem *item;
	public:
		CScanSetupDelSysNotifier(CFrontend* f);
		virtual ~CScanSetupDelSysNotifier(){};
		
		void addItem(CMenuItem *m);
		bool changeNotify(const std::string&, void *Data);
};

//// scansetup
class CScanSetup : public CWidgetTarget
{
	private:
		CFrontend * fe;
		sat_iterator_t sit;
		CScanTs *scanTs;
		CSatelliteSetupNotifier *satNotify;
		CScanSetupFEModeNotifier *feModeNotifier;
		CScanSetupDelSysNotifier *feDelSysNotifier;
		CTPSelectHandler *tpSelect;
		CScanSettings * scanSettings;
		CMenuItem *item_freq;
		CMenuItem *item_sr;
		int freq_length;

		////
		int showScanService();
		int showUnicableSetup();
		int showManualScanSetup();
		int showAutoScanSetup();
		int showAllAutoScanSetup();
		int showMotorSetup();
		int showLNBSetup();
		int showSatOnOffSetup();
		
	public:
		CScanSetup(CFrontend* f = NULL);
		virtual ~CScanSetup();
		
		int exec(CWidgetTarget *parent, const std::string &actionKey);
};

////
class CTunerSetup : public CWidgetTarget
{
	private:
		int showMenu();
		
	public:
		CTunerSetup(){};
		virtual ~CTunerSetup(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

#endif

