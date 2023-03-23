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

CSatIPClient::CSatIPClient()
{
	selected = 0;
}

CSatIPClient::~CSatIPClient()
{
}

void CSatIPClient::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

void CSatIPClient::ReadSettings() 
{
	CConfigFile *satipclient_config = new CConfigFile(',');
	satipclient_config->clear();
	satipclient_config->loadConfig(CONFIG_FILE);
	
	// enabled
	SatIPEnabled = satipclient_config->getInt32("ENABLED", 0);
	// satip server ip
	SatIPServerIP = satipclient_config->getString("SATIPSERVER", "192.168.0.12");
	// satip server port default 554
	SatIPServerPort = satipclient_config->getString("SATIPPORT", "554");
	// frontend typ
	SatIPFrontendTyp = satipclient_config->getInt32("FRONTENDTYPE", 1);
	// vtuner device
	SatIPVtunerDevice = satipclient_config->getString("VTUNER", "/dev/vtunerc0");
	// debug
	SatIPDebug = satipclient_config->getInt32("LOGLEVEL", 1);
}

bool CSatIPClient::SaveSettings() 
{
	CConfigFile *satipclient_config = new CConfigFile(',');
	
	// satip disabled
	satipclient_config->setInt32("ENABLED", SatIPEnabled);
	// satipserver ip
	satipclient_config->setString("SATIPSERVER", SatIPServerIP);
	// satipserver port
	//satipclient_config->setString("SATIPPORT", SatIPServerPort);
	// frontend typ
	satipclient_config->setInt32("FRONTENDTYPE", SatIPFrontendTyp);
	// vtuner device
	satipclient_config->setString("VTUNER", SatIPVtunerDevice);
	// debug
	satipclient_config->setInt32("LOGLEVEL", SatIPDebug);
	
	satipclient_config->saveConfig(CONFIG_FILE);
	
	return true;
}

bool CSatIPClient::loadVTuner() 
{
	std::string cmd = "insmod /var/lib/modules/vtunerc.ko devices=1 debug=0";
	system(cmd.c_str());

	return true;
}

bool CSatIPClient::unloadVTuner() 
{
	std::string cmd = "rmmod vtunerc";
	system(cmd.c_str());

	return true;
}

bool CSatIPClient::startSatIPClient() 
{
	std::string cmd = "/var/bin/satip_client";
	cmd += " -s ";
	cmd += SatIPServerIP;
	cmd += " -t ";
	cmd += to_string(SatIPFrontendTyp);
	cmd += " -d ";
	cmd += SatIPVtunerDevice;
	cmd += " -l ";
	cmd += to_string(SatIPDebug);
	cmd += " &";

	system(cmd.c_str());

	return true;
}

bool CSatIPClient::stopSatIPClient() 
{
	std::string cmd = "killall -9 satip_client";
	system(cmd.c_str());

	return true;
}

void CSatIPClient::showMenu()
{
	// read settings
	ReadSettings();
	
	// create menu
	satIPClientMenu = new CMenuWidget("SatIP Client", NEUTRINO_ICON_SETTINGS);

	satIPClientMenu->setWidgetMode(MODE_SETUP);
	satIPClientMenu->enableShrinkMenu();

	satIPClientMenu->setSelected(selected);

	satIPClientMenu->addItem(new ClistBoxItem(_("back"), true, NULL, NULL, NULL, RC_nokey, NEUTRINO_ICON_BUTTON_LEFT));
	satIPClientMenu->addItem(new CMenuSeparator(LINE));
	satIPClientMenu->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save", RC_red, NEUTRINO_ICON_BUTTON_RED));
	satIPClientMenu->addItem(new CMenuSeparator(LINE));

	// enabled
	satIPClientMenu->addItem(new CMenuOptionChooser(_("SatIP Client enabled"), &SatIPEnabled, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL));
	
	// satipserver ip
	CIPInput * SATIPSERVER_IP = new CIPInput("SatIP Server IP", SatIPServerIP);
	satIPClientMenu->addItem(new ClistBoxItem(_("SatIP Server IP"), true, SatIPServerIP.c_str(), SATIPSERVER_IP, NULL));
	
	// satipserver port
	CStringInput * SATIPSERVER_PORT = new CStringInput("SatIP Server Port", SatIPServerPort.c_str());
	satIPClientMenu->addItem(new ClistBoxItem(_("SatIP Server Port"), false, SatIPServerPort.c_str(), SATIPSERVER_PORT, NULL));
	
	// frontendtyp
	satIPClientMenu->addItem(new CMenuOptionChooser(_("Tuner typ"), &SatIPFrontendTyp, SATIP_FRONTEND_TYPE_OPTIONS, SATIP_FRONTEND_TYPE_OPTION_COUNT, true, NULL));

	// vtuner device
	CStringInputSMS * VTUNER_DEVICE = new CStringInputSMS((char *)"VTuner Device", (char *)SatIPVtunerDevice.c_str());
	satIPClientMenu->addItem(new ClistBoxItem(_("VTuner Device"), true, SatIPVtunerDevice.c_str(), VTUNER_DEVICE, NULL));

	// debug
	satIPClientMenu->addItem(new CMenuOptionChooser(_("SatIP Client Debug"), &SatIPDebug, SATIP_DEBUG_LEVEL_OPTIONS, SATIP_DEBUG_LEVEL_OPTION_COUNT, true, NULL));

	satIPClientMenu->addItem(new CMenuSeparator(LINE));

	// start satipclient
	satIPClientMenu->addItem(new ClistBoxItem(_("Start SatIP Client"), SatIPEnabled, NULL, this, "start", RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	// stop satipclient
	satIPClientMenu->addItem(new ClistBoxItem(_("Stop SatIP Client"), SatIPEnabled, NULL, this, "stop", RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	satIPClientMenu->exec(NULL, "");
	satIPClientMenu->hide();
	
	delete satIPClientMenu;
	satIPClientMenu = NULL;
}

int CSatIPClient::exec(CMenuTarget* parent, const std::string &actionKey)
{
	if(parent)
		parent->hide();
	
	if(actionKey == "save")
	{
		//SaveSettings();
		if(this->SaveSettings())
		 	HintBox(_("Information"), _("Save settings now"));

		return RETURN_REPAINT;
	}
	else if(actionKey == "start")
	{
		// load vtuner driver
		loadVTuner();

		// start satip_client
		startSatIPClient();
	}
	else if(actionKey == "stop")
	{
		// stop satip_client
		stopSatIPClient();

		// unload vtuner driver
		unloadVTuner();
	}
	
	showMenu();

	return RETURN_EXIT_ALL;
}

void plugin_init(void)
{
	CSatIPClient * SatIPClientHandler = new CSatIPClient();
	SatIPClientHandler->ReadSettings();

	if(SatIPClientHandler->SatIPEnabled == 1)
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

