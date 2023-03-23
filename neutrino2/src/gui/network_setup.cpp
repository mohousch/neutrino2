/*
	Neutrino-GUI  -   DBoxII-Project

	$id: network_setup.cpp 2016.01.02 20:19:30 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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

#include <stdio.h> 
#include <sys/stat.h>
#include <dirent.h>

#include <libnet.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/messagebox.h>

#include <gui/network_setup.h>


#include <gui/proxyserver_setup.h>
#include <gui/nfs.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>

extern "C" int pinghost( const char *hostname );


#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, "off" },
        { 1, "on" }
};

#define OPTIONS_NTPENABLE_OPTION_COUNT 2
const keyval OPTIONS_NTPENABLE_OPTIONS[OPTIONS_NTPENABLE_OPTION_COUNT] =
{
	{ 0, "DVB" },
	{ 1, "NTP" }
};

#define OPTIONS_WLAN_SECURITY_OPTION_COUNT 2
const keyval OPTIONS_WLAN_SECURITY_OPTIONS[OPTIONS_WLAN_SECURITY_OPTION_COUNT] =
{
        { 0, "WPA" },
        { 1, "WPA2"  }
};

static int my_filter(const struct dirent * dent)
{
	if(dent->d_name[0] == 'l' && dent->d_name[1] == 'o')
		return 0;
	if(dent->d_name[0] == '.')
		return 0;
	
	return 1;
}

CNetworkSettings::CNetworkSettings()
{
	networkConfig = CNetworkConfig::getInstance();
}

CNetworkSettings *CNetworkSettings::getInstance()
{
	static CNetworkSettings *networkSettings = NULL;

	if(!networkSettings)
	{
		networkSettings = new CNetworkSettings();
		dprintf(DEBUG_NORMAL, "CNetworkSettings::getInstance: Instance created\n");
	}
	
	return networkSettings;
}

CNetworkSettings::~CNetworkSettings()
{
}

void CNetworkSettings::readNetworkSettings(std::string iname)
{
	networkConfig->readConfig(iname);

	network_automatic_start = networkConfig->automatic_start ? 1 : 0;
	network_dhcp = networkConfig->inet_static ? 0 : 1;
	network_hostname = networkConfig->hostname;
	mac_addr = networkConfig->mac_addr;

	//
	network_ssid = networkConfig->ssid;
	network_key = networkConfig->key;
	network_encryption = (networkConfig->encryption == "WPA") ? 0 : 1;
}

void CNetworkSettings::setNetwork()
{
	networkConfig->commitConfig();
	networkConfig->startNetwork();
}

int CNetworkSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CNetworkSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		networkConfig->automatic_start = (network_automatic_start == 1);
		networkConfig->commitConfig();

		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "network") 
	{
		networkConfig->automatic_start = (network_automatic_start == 1);

		networkConfig->stopNetwork();
		networkConfig->commitConfig();
		networkConfig->startNetwork();

		HintBox(_("Information"), _("Setup network now, please wait..."));
		
		return ret;
	}
	else if(actionKey == "networktest") 
	{
		dprintf(DEBUG_INFO, "CNeutrinoApp::exec: doing network test...\n");

		testNetworkSettings(networkConfig->address.c_str(), networkConfig->netmask.c_str(), networkConfig->broadcast.c_str(), networkConfig->gateway.c_str(), networkConfig->nameserver.c_str(), networkConfig->inet_static);
		
		return ret;
	}
	else if(actionKey == "networkshow") 
	{
		dprintf(DEBUG_INFO, "CNeutrinoApp::exec: showing current network settings...\n");
		showCurrentNetworkSettings();
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void CNetworkSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CNetworkSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* networkSettings = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("networksetup");
	
	if (widget)
	{
		networkSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		networkSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		networkSettings->setWidgetMode(MODE_SETUP);
		networkSettings->enableShrinkMenu();
		
		networkSettings->enablePaintHead();
		networkSettings->setTitle(_("Network settings"), NEUTRINO_ICON_NETWORK);

		networkSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		networkSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "networksetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(networkSettings);
	}
	
	networkSettings->clearItems();
	
	//
	struct dirent **namelist;

	//interface
	int ifcount = scandir("/sys/class/net", &namelist, my_filter, alphasort);

	CMenuOptionStringChooser * ifSelect = new CMenuOptionStringChooser(_("Interface"), g_settings.ifname, ifcount > 1, this);

	bool found = false;

	for(int i = 0; i < ifcount; i++) 
	{
		ifSelect->addOption(namelist[i]->d_name);
		
		if(strcmp(g_settings.ifname, namelist[i]->d_name) == 0)
			found = true;
		free(namelist[i]);
	}

	if (ifcount >= 0)
		free(namelist);

	if(!found)
		strcpy(g_settings.ifname, "eth0");
	
	// read network settings
	readNetworkSettings(g_settings.ifname);

	// init IP changer
	MyIPChanger = new CIPChangeNotifier;
	
	//eth id
	ClistBoxItem* mac = new ClistBoxItem("MAC address", false, mac_addr.c_str());
	
	CIPInput * networkSettings_NetworkIP = new CIPInput(_("IP address"), networkConfig->address, _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"), MyIPChanger);

	CIPInput * networkSettings_NetMask = new CIPInput(_("Netmask"), networkConfig->netmask, _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));

	CIPInput * networkSettings_Broadcast = new CIPInput(_("Broadcast"), networkConfig->broadcast, _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));

	CIPInput * networkSettings_Gateway = new CIPInput(_("Default gateway"), networkConfig->gateway, _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));

	CIPInput * networkSettings_NameServer = new CIPInput(_("Name server"), networkConfig->nameserver, _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	
	//hostname
	CStringInputSMS * networkSettings_Hostname = new CStringInputSMS(_("Hostname"), network_hostname.c_str());

        CSectionsdConfigNotifier * sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	// ntp server
        CStringInputSMS * networkSettings_NtpServer = new CStringInputSMS(_("NTP-Server"), g_settings.network_ntpserver.c_str(), MAX_INPUT_CHARS, _("NTP-Server example: ntp1.ptb.de"), _("need reboot or epg-reset"), "abcdefghijklmnopqrstuvwxyz0123456789-. ", sectionsdConfigNotifier);
        CStringInput * networkSettings_NtpRefresh = new CStringInput(_("NTP/DVB-Refresh"), g_settings.network_ntprefresh.c_str(), 3, _("NTP/DVB-Time-Sync in minutes"), _("need reboot or epg-reset"), "0123456789 ", sectionsdConfigNotifier);

	ClistBoxItem * m0 = new ClistBoxItem(_("Setup network now"), true, NULL, this, "network");

	ClistBoxItem * m1 = new ClistBoxItem(_("IP address"), networkConfig->inet_static, networkConfig->address.c_str(), networkSettings_NetworkIP);

	ClistBoxItem * m2 = new ClistBoxItem(_("Netmask"), networkConfig->inet_static, networkConfig->netmask.c_str(), networkSettings_NetMask);

	ClistBoxItem * m3 = new ClistBoxItem(_("Broadcast"), networkConfig->inet_static, networkConfig->broadcast.c_str(), networkSettings_Broadcast );

	ClistBoxItem * m4 = new ClistBoxItem(_("Default gateway"), networkConfig->inet_static, networkConfig->gateway.c_str(), networkSettings_Gateway   );

	ClistBoxItem * m5 = new ClistBoxItem(_("Name server"), networkConfig->inet_static, networkConfig->nameserver.c_str(), networkSettings_NameServer);

        ClistBoxItem * m6 = new ClistBoxItem(_("NTP-Server"), true, g_settings.network_ntpserver.c_str(), networkSettings_NtpServer );

        ClistBoxItem * m7 = new ClistBoxItem(_("NTP/DVB-Refresh"), true, g_settings.network_ntprefresh.c_str(), networkSettings_NtpRefresh );
	
	ClistBoxItem * m8 = new ClistBoxItem(_("Hostname"), true, network_hostname.c_str(), networkSettings_Hostname);

	CDHCPNotifier * dhcpNotifier = new CDHCPNotifier(m1, m2, m3, m4, m5);

	// setup network on startup
	CMenuOptionChooser * oj = new CMenuOptionChooser(_("Setup network on startup"), &network_automatic_start, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	// intros
	networkSettings->addItem(new ClistBoxItem(_("back"), true));
	networkSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	networkSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	networkSettings->addItem( new CMenuSeparator(LINE) );
	
	// setup network on start
	networkSettings->addItem( oj );

	// test network now
	networkSettings->addItem(new ClistBoxItem(_("Test network now"), true, NULL, this, "networktest"));

	// show active network settings
	networkSettings->addItem(new ClistBoxItem(_("Show active network settings"), true, NULL, this, "networkshow"));
	
	// setup network now
	networkSettings->addItem( m0 );
	
	// mac id
	networkSettings->addItem(new CMenuSeparator(LINE));
	networkSettings->addItem(mac);	//eth id
	
	// interface
	if(ifcount)
		networkSettings->addItem(ifSelect);	//if select
	else
		delete ifSelect;

	networkSettings->addItem(new CMenuSeparator(LINE));

	// dhcp on/off
	oj = new CMenuOptionChooser(_("DHCP"), &network_dhcp, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, dhcpNotifier);
	networkSettings->addItem(oj);

	// hostname
	networkSettings->addItem( m8);

	// ip
	networkSettings->addItem(new CMenuSeparator(LINE));
	networkSettings->addItem( m1);

	// netmask
	networkSettings->addItem( m2);

	// broadcast
	networkSettings->addItem( m3);

	// default gateway
	networkSettings->addItem(new CMenuSeparator(LINE));
	networkSettings->addItem( m4);

	// nameserver
	networkSettings->addItem( m5);
	
	//
	if(ifcount > 1) // if there is only one, its probably wired
	{
		//ssid
		CStringInputSMS * networkSettings_ssid = new CStringInputSMS(_("Network Name"), network_ssid.c_str());
		ClistBoxItem * m9 = new ClistBoxItem(_("Network Name"), networkConfig->wireless, network_ssid.c_str(), networkSettings_ssid );

		//key
		CStringInputSMS *networkSettings_key = new CStringInputSMS(_("Key"), network_key.c_str());
		ClistBoxItem *m10 = new ClistBoxItem(_("Key"), networkConfig->wireless, network_key.c_str(), networkSettings_key );

		wlanEnable[0] = m9;
		wlanEnable[1] = m10;
		
		// ssid
		networkSettings->addItem(new CMenuSeparator(LINE));
		networkSettings->addItem( m9);

		// key
		networkSettings->addItem( m10);

		//encryption
		CMenuOptionChooser * m11 = new CMenuOptionChooser(_("Security"), &network_encryption, OPTIONS_WLAN_SECURITY_OPTIONS, OPTIONS_WLAN_SECURITY_OPTION_COUNT, true);
		wlanEnable[2] = m11;
		networkSettings->addItem( m11); //encryption
	}
	
	// ntp
	networkSettings->addItem(new CMenuSeparator(LINE | STRING, _("Time-Syncronisation")));

	networkSettings->addItem(new CMenuOptionChooser(_("Syncronisation via"), &g_settings.network_ntpenable, OPTIONS_NTPENABLE_OPTIONS, OPTIONS_NTPENABLE_OPTION_COUNT, true, sectionsdConfigNotifier));

	// ntp server
        networkSettings->addItem( m6);

	// ntp refresh
        networkSettings->addItem( m7);
	
	//proxyserver submenu
	networkSettings->addItem(new CMenuSeparator(LINE));
	networkSettings->addItem(new ClistBoxItem(_("Proxyserver"), true, NULL, new CProxySetup()));

	// mount manager
	networkSettings->addItem(new CMenuSeparator(LINE | STRING, _("Network Mount Manager")));

	networkSettings->addItem(new ClistBoxItem(_("Mount network volume"), true, NULL, new CNFSMountGui()));

	networkSettings->addItem(new ClistBoxItem(_("Umount network volume"), true, NULL, new CNFSUmountGui()));
	
	//
	widget->exec(NULL, "");

	delete MyIPChanger;
	delete dhcpNotifier;
	delete sectionsdConfigNotifier;
}

// IP notifier
bool CIPChangeNotifier::changeNotify(const std::string& locale, void * Data)
{
	if(locale == _("IP address")) 
	{
		char ip[16];
		unsigned char _ip[4];
		sscanf((char*) Data, "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);

		sprintf(ip, "%hhu.%hhu.%hhu.255", _ip[0], _ip[1], _ip[2]);
		CNetworkSettings::getInstance()->networkConfig->broadcast = ip;

		CNetworkSettings::getInstance()->networkConfig->netmask = (_ip[0] == 10) ? "255.0.0.0" : "255.255.255.0";
	}
	else if(locale == _("Interface")) 
	{
		CNetworkSettings::getInstance()->networkConfig->readConfig(g_settings.ifname);
		//readNetworkSettings(); //???
		
		dprintf(DEBUG_NORMAL, "CNetworkSetup::changeNotify: using %s, static %d\n", g_settings.ifname, CNetworkSettings::getInstance()->networkConfig->inet_static);

		changeNotify(_("DHCP"), &CNetworkSettings::getInstance()->networkConfig->inet_static);

		int ecnt = sizeof(CNetworkSettings::getInstance()->wlanEnable) / sizeof(CMenuItem*);

		for(int i = 0; i < ecnt; i++)
			CNetworkSettings::getInstance()->wlanEnable[i]->setActive(CNetworkSettings::getInstance()->networkConfig->wireless);

	}

	return true;
}

// dhcp notifier
CDHCPNotifier::CDHCPNotifier( ClistBoxItem* a1, ClistBoxItem* a2, ClistBoxItem* a3, ClistBoxItem* a4, ClistBoxItem* a5)
{
	toDisable[0] = a1;
	toDisable[1] = a2;
	toDisable[2] = a3;
	toDisable[3] = a4;
	toDisable[4] = a5;
}


bool CDHCPNotifier::changeNotify(const std::string&, void * data)
{
	CNetworkSettings::getInstance()->networkConfig->inet_static = ((*(int*)(data)) == 0);
	
	for(int x = 0; x < 5; x++)
		toDisable[x]->setActive(CNetworkSettings::getInstance()->networkConfig->inet_static);
	
	return true;
}

//
const char * mypinghost(const char * const host)
{
	int retvalue = pinghost(host);
	switch (retvalue)
	{
		case 1: return _("is reachable (ping)");
		case 0: return _("is unreachable");
		case -1: return _("is unreachable (host or protocol error)");
		case -2: return _("is unreachable (socket error)");
	}
	return "";
}

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool ip_static)
{
	char our_ip[16];
	char our_mask[16];
	char our_broadcast[16];
	char our_gateway[16];
	char our_nameserver[16];
	std::string text;

	if (ip_static) 
	{
		strcpy(our_ip,ip);
		strcpy(our_mask,netmask);
		strcpy(our_broadcast,broadcast);
		strcpy(our_gateway,gateway);
		strcpy(our_nameserver,nameserver);
	}
	else 
	{
		netGetIP((char *) "eth0",our_ip,our_mask,our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}

	dprintf(DEBUG_NORMAL, "testNw IP       : %s\n", our_ip);
	dprintf(DEBUG_NORMAL, "testNw Netmask  : %s\n", our_mask);
	dprintf(DEBUG_NORMAL, "testNw Broadcast: %s\n", our_broadcast);
	dprintf(DEBUG_NORMAL, "testNw Gateway: %s\n", our_gateway);
	dprintf(DEBUG_NORMAL, "testNw Nameserver: %s\n", our_nameserver);

	text = our_ip;
	text += ": ";
	text += mypinghost(our_ip);
	text += '\n';
	text += _("Default gateway");
	text += ": ";
	text += our_gateway;
	text += ' ';
	text += mypinghost(our_gateway);
	text += '\n';
	text += _("Name server");
	text += ": ";
	text += our_nameserver;
	text += ' ';
	text += mypinghost(our_nameserver);
	text += "\nwww.google.de: ";
	text += mypinghost("173.194.35.152");

	MessageBox(_("Test network now"), text.c_str(), mbrBack, mbBack, NEUTRINO_ICON_INFO); // UTF-8
}

void showCurrentNetworkSettings()
{
	char ip[16];
	char mask[16];
	char broadcast[16];
	char router[16];
	char nameserver[16];
	std::string mac;
	std::string text;

	//netGetIP((char *) "eth0",ip,mask,broadcast);
	netGetIP(g_settings.ifname, ip, mask, broadcast);
	
	if (ip[0] == 0) 
	{
		text = "Network inactive\n";
	}
	else 
	{
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);

		//netGetMacAddr(g_settings.ifname, (unsigned char *)mac.c_str());
		//text = "Box: " + mac + "\n    ";
		
		text  = _("IP address");
		text += ": ";
		text += ip;
		text += '\n';
		text += _("Netmask");
		text += ": ";
		text += mask;
		text += '\n';
		text += _("Broadcast");
		text += ": ";
		text += broadcast;
		text += '\n';
		text += _("Name server");
		text += ": ";
		text += nameserver;
		text += '\n';
		text += _("Default gateway");
		text += ": ";
		text += router;
	}
	
	MessageBox(_("Show active network settings"), text.c_str(), mbrBack, mbBack, NEUTRINO_ICON_INFO); // UTF-8
}



