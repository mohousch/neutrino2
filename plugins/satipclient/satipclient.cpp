/*
  $Id: satipclient.cpp 16.08.2016 22.13.30 mohousch Exp $

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

#include <satipclient.h>


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);


// option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

// frontend typ
#define SATIP_FRONTEND_TYPE_OPTION_COUNT 3
const keyval SATIP_FRONTEND_TYPE_OPTIONS[SATIP_FRONTEND_TYPE_OPTION_COUNT] =
{
	{ 0, _("Sat") },
	{ 1, _("Cable") },
	{ 2, _("Terrestrial") }
};

// debug level
#define SATIP_DEBUG_LEVEL_OPTION_COUNT 5
const keyval SATIP_DEBUG_LEVEL_OPTIONS[SATIP_DEBUG_LEVEL_OPTION_COUNT] =
{
	{ 0, _("None") },
	{ 1, _("Error") },
	{ 2, _("Warn") },
	{ 3, _("Info") },
	{ 4, _("Debug") }
};

// workaround
#define SATIP_WORKAROUND_OPTION_COUNT 4
const keyval SATIP_WORKAROUND_OPTIONS[SATIP_WORKAROUND_OPTION_COUNT] =
{
	{ 0, _("Disabled") },
	{ 1, _("ADD-DUMMY-PID") },
	{ 2, _("FORCE-FULL-PID") },
	{ 3, _("FORCE-FE-LOCK") }
};

int SatIPEnabled = 0;

//
void CSatIPClient::readSettings() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::loadSettings\n");
	
	CConfigFile *satipclient_config = new CConfigFile(',');
	satipclient_config->clear();
	satipclient_config->loadConfig(CONFIG_FILE);
	
	// disabled
	SatIPEnabled = satipclient_config->getInt32("ENABLED", 0);
	// satip server ip
	SatIPServerIP = satipclient_config->getString("SATIPSERVER0", "");
	// satip server port default 554
	SatIPServerPort = satipclient_config->getString("SATIPPORT0", "554");
	// frontend typ
	SatIPFrontendTyp = satipclient_config->getInt32("FRONTENDTYPE0", 1);
	// vtuner device
	SatIPVtunerDevice = satipclient_config->getString("VTUNER0", "/dev/vtunerc0");
	// debug
	SatIPDebug = satipclient_config->getInt32("LOGLEVEL0", 1);
	//workaround
	SatIPWorkaround = satipclient_config->getInt32("WORKAROUND0", 0);
}

bool CSatIPClient::saveSettings() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::saveSettings\n");
	
	CConfigFile *satipclient_config = new CConfigFile(',');
	
	// satip disabled
	satipclient_config->setInt32("ENABLED", SatIPEnabled);
	// satipserver ip
	satipclient_config->setString("SATIPSERVER0", SatIPServerIP);
	// satipserver port
	satipclient_config->setString("SATIPPORT0", SatIPServerPort);
	// frontend typ
	satipclient_config->setInt32("FRONTENDTYPE0", SatIPFrontendTyp);
	// vtuner device
	satipclient_config->setString("VTUNER0", SatIPVtunerDevice);
	// debug
	satipclient_config->setInt32("LOGLEVEL0", SatIPDebug);
	// workaround
	satipclient_config->setInt32("WORKAROUND0", SatIPWorkaround);
	
	satipclient_config->saveConfig(CONFIG_FILE);
	
	return true;
}

bool CSatIPClient::loadVTuner() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::loadVTuner\n");
	
	//std::string cmd = "insmod /var/lib/modules/vtunerc.ko devices=1 debug=0";
	std::string cmd = "modprobe vtunerc devices=1 debug=0";
	system(cmd.c_str());

	return true;
}

bool CSatIPClient::unloadVTuner() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::unloadVTuner\n");
	
	std::string cmd = "rmmod vtunerc";
	system(cmd.c_str());

	return true;
}

bool CSatIPClient::startSatIPClient() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::startSatIPClient\n");
/*	
	std::string cmd = "satip_client";
	cmd += " -s ";
	cmd += SatIPServerIP;
	cmd += " -t ";
	cmd += to_string(SatIPFrontendTyp);
	cmd += " -d ";
	cmd += SatIPVtunerDevice;
	cmd += " -l ";
	cmd += to_string(SatIPDebug);
	cmd += " &";
*/
	std::string cmd = "satip-client.sh start";

	system(cmd.c_str());

	return true;
}

bool CSatIPClient::stopSatIPClient() 
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::stopSatIPClient\n");
	
	//std::string cmd = "killall -9 satip_client";
	std::string cmd = "satip-client.sh stop";
	system(cmd.c_str());

	return true;
}

void CSatIPClient::showMenu()
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::showMenu\n");
	
	// read settings
	readSettings();
	
	// create menu
	CMenuWidget * satIPClientMenu = new CMenuWidget("SatIP Client", NEUTRINO_ICON_SETTINGS);

	satIPClientMenu->setWidgetMode(ClistBox::MODE_SETUP);
	//satIPClientMenu->setHeadGradient(LIGHT2DARK);
	//satIPClientMenu->setFootGradient(DARK2LIGHT);
	//satIPClientMenu->setHeadLine(false);
	//satIPClientMenu->setFootLine(false);
	//satIPClientMenu->setHeadCorner(RADIUS_SMALL, CORNER_TOP);
	//satIPClientMenu->setFootCorner(RADIUS_SMALL, CORNER_BOTTOM);

	//
	CMenuForwarder *m1= new CMenuForwarder(_("Start SatIP Client"), true, NULL, this, "start", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);
	m1->setHidden(!SatIPEnabled);
	CMenuForwarder *m2 = new CMenuForwarder(_("Stop SatIP Client"), true, NULL, this, "stop", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
	m2->setHidden(!SatIPEnabled);
	
	//
	satIPClientMenu->addItem(new CMenuForwarder(_("back"), true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_LEFT));
	satIPClientMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	satIPClientMenu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, this, "save", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	satIPClientMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// enabled
	CSatIPClientNotifier satIPNotifier(m1, m2);
	satIPClientMenu->addItem(new CMenuOptionChooser(_("SatIP Client enabled"), &SatIPEnabled, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, &satIPNotifier));
	
	// satipserver ip
	CIPInput * SATIPSERVER_IP = new CIPInput("SatIP Server IP", SatIPServerIP);
	satIPClientMenu->addItem(new CMenuForwarder(_("SatIP Server IP"), true, SatIPServerIP.c_str(), SATIPSERVER_IP, NULL));
	
	// satipserver port
	CStringInput * SATIPSERVER_PORT = new CStringInput("SatIP Server Port", SatIPServerPort.c_str());
	satIPClientMenu->addItem(new CMenuForwarder(_("SatIP Server Port"), false, SatIPServerPort.c_str(), SATIPSERVER_PORT, NULL));
	
	// frontendtyp
	satIPClientMenu->addItem(new CMenuOptionChooser(_("Tuner typ"), &SatIPFrontendTyp, SATIP_FRONTEND_TYPE_OPTIONS, SATIP_FRONTEND_TYPE_OPTION_COUNT, true, NULL));

	// vtuner device
	CStringInputSMS * VTUNER_DEVICE = new CStringInputSMS((char *)"VTuner Device", (char *)SatIPVtunerDevice.c_str());
	satIPClientMenu->addItem(new CMenuForwarder(_("VTuner Device"), true, SatIPVtunerDevice.c_str(), VTUNER_DEVICE, NULL));

	// debug
	satIPClientMenu->addItem(new CMenuOptionChooser(_("SatIP Client Debug"), &SatIPDebug, SATIP_DEBUG_LEVEL_OPTIONS, SATIP_DEBUG_LEVEL_OPTION_COUNT, true, NULL));
	
	// workaround
	satIPClientMenu->addItem(new CMenuOptionChooser(_("SatIP Client Workaround"), &SatIPWorkaround, SATIP_WORKAROUND_OPTIONS, SATIP_WORKAROUND_OPTION_COUNT, true, NULL));

	satIPClientMenu->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// start satipclient
	satIPClientMenu->addItem(m1);

	// stop satipclient
	satIPClientMenu->addItem(m2);

	satIPClientMenu->exec(NULL, "");
	
	delete satIPClientMenu;
	satIPClientMenu = NULL;
}

int CSatIPClient::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CSatIPClient::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "save")
	{
		//SaveSettings();
		if(this->saveSettings())
		 	HintBox(_("Information"), _("Save settings now"));

		return ret;
	}
	else if(actionKey == "start")
	{
		// load vtuner driver
		loadVTuner();

		// start satip_client
		startSatIPClient();
		
		return ret;
	}
	else if(actionKey == "stop")
	{
		// stop satip_client
		stopSatIPClient();

		// unload vtuner driver
		unloadVTuner();
		
		return ret;
	}
	
	showMenu();

	return ret;
}

//
CSatIPClientNotifier::CSatIPClientNotifier(CMenuForwarder *m1, CMenuForwarder *m2)
{
	item1 = m1;
	item2 = m2;
}

bool CSatIPClientNotifier::changeNotify(const std::string& OptionName, void *)
{
	if (OptionName == _("SatIP Client enabled"))
	{
		//item1->setActive(SatIPEnabled);
		//item2->setActive(SatIPEnabled);
		item1->setHidden(!SatIPEnabled);
		item2->setHidden(!SatIPEnabled);
	}

	return true;
}

//
void plugin_init(void)
{
	CSatIPClient * SatIPClientHandler = new CSatIPClient();
	SatIPClientHandler->readSettings();

	if(SatIPEnabled)
	{
		// load vtuner driver
		SatIPClientHandler->loadVTuner();

		// start satip_client
		SatIPClientHandler->startSatIPClient();
	}	
	
	delete SatIPClientHandler;
}

void plugin_del(void)
{
	CSatIPClient * SatIPClientHandler = new CSatIPClient();

	// stop satipclient
	SatIPClientHandler->stopSatIPClient();

	// unload vtuner driver
	SatIPClientHandler->unloadVTuner();

	delete SatIPClientHandler;
}

void plugin_exec(void)
{
	// class handler
	CSatIPClient * SatIPClientHandler = new CSatIPClient();

	SatIPClientHandler->exec(NULL, "");
	
	delete SatIPClientHandler;
}

