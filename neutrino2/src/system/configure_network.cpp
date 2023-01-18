/*
 * $Header: configure_network.cpp 2016.01.12 12:13:30 mohousch Exp $
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <config.h>
#include <cstdio>               /* perror... */
#include <sys/wait.h>
#include <sys/types.h>          /* u_char */
#include <sys/stat.h>
#include <string.h>
#include "configure_network.h"
#include "libnet.h"             /* netGetNameserver, netSetNameserver   */
#include "network_interfaces.h" /* getInetAttributes, setInetAttributes */
#include <stdlib.h>             /* system                               */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <unistd.h>

#include <system/safe_system.h>
#include <system/debug.h>


CNetworkConfig::CNetworkConfig(void)
{
	char our_nameserver[16];
	netGetNameserver(our_nameserver);
	nameserver = our_nameserver;
	
	ifname = "eth0";
	orig_automatic_start = false;
	orig_inet_static = false;
	automatic_start = false;
	inet_static = false;
	wireless = false;
}

CNetworkConfig *CNetworkConfig::getInstance()
{
	static CNetworkConfig *network_config = NULL;

	if(!network_config)
	{
		network_config = new CNetworkConfig();
		dprintf(DEBUG_NORMAL, "CNetworkConfig::getInstance: Instance created\n");
	}
	return network_config;
}

CNetworkConfig::~CNetworkConfig()
{
}

void CNetworkConfig::readConfig(std::string iname)
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::readConfig: %s\n", iname.c_str());

	ifname = iname;
	inet_static = getInetAttributes(ifname, automatic_start, address, netmask, broadcast, gateway);

	init_vars();
	copy_to_orig();
}

void CNetworkConfig::init_vars(void)
{
	dprintf(DEBUG_INFO, "CNetworkConfig::init_vars\n");

	char mask[16];
	char _broadcast[16];
	char router[16];
	char ip[16];
	unsigned char addr[6];

	hostname = netGetHostname();

	netGetDefaultRoute(router);
	gateway = router;

	/* FIXME its enough to read IP for dhcp only ?
	 * static config should not be different from settings in etc/network/interfaces */
	if(!inet_static) 
	{
		netGetIP((char *) ifname.c_str(), ip, mask, _broadcast);
		netmask = mask;
		broadcast = _broadcast;
		address = ip;
	}

	netGetMacAddr((char *) ifname.c_str(), addr);

	std::stringstream mac_tmp;
	for(int i = 0; i < 6; ++i)
		mac_tmp<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';

	mac_addr = mac_tmp.str().substr(0,17);

	key = "";
	ssid = "";
	encryption = "WPA2";

	wireless = 0;

	std::string tmp = "/sys/class/net/" + ifname + "/wireless";

	if(access(tmp.c_str(), R_OK) == 0)
		wireless = 1;
		
	if(wireless)
		readWpaConfig();

	dprintf(DEBUG_NORMAL, "CNetworkConfig: %s loaded, wireless %s\n", ifname.c_str(), wireless ? "yes" : "no");
}

void CNetworkConfig::copy_to_orig(void)
{
	orig_automatic_start = automatic_start;
	orig_address         = address;
	orig_netmask         = netmask;
	orig_broadcast       = broadcast;
	orig_gateway         = gateway;
	orig_inet_static     = inet_static;
	orig_hostname	     = hostname;
	orig_ifname	     = ifname;
	orig_ssid	     = ssid;
	orig_key	     = key;
	orig_encryption	     = encryption;
}

bool CNetworkConfig::modified_from_orig(void)
{
	if(wireless) 
	{
		if( (ssid != orig_ssid) || (key != orig_key) || (encryption != orig_encryption) )
			return 1;
	}
	
	if (inet_static) 
	{
		if ((orig_address         != address        ) ||
		    (orig_netmask         != netmask        ) ||
		    (orig_broadcast       != broadcast      ) ||
		    (orig_gateway         != gateway        ))
			return 1;
	}
	
	return (
		(orig_automatic_start != automatic_start) ||
		(orig_hostname        != hostname       ) ||
		(orig_inet_static     != inet_static    ) ||
		(orig_ifname	      != ifname)
		);
}

void CNetworkConfig::commitConfig(void)
{
	dprintf(DEBUG_INFO, "CNetworkConfig::commitConfig\n");

	if (modified_from_orig())
	{
		if(orig_hostname != hostname)
			netSetHostname((char *) hostname.c_str());

		if (inet_static)
		{
			addLoopbackDevice("lo", true);
			setStaticAttributes(ifname, automatic_start, address, netmask, broadcast, gateway, wireless);
		}
		else
		{
			addLoopbackDevice("lo", true);
			setDhcpAttributes(ifname, automatic_start, wireless);
		}

		if( wireless && ((key != orig_key) || (ssid != orig_ssid) || (encryption != orig_encryption)) )
			saveWpaConfig();

		copy_to_orig();

	}
	
	if (nameserver != orig_nameserver)
	{
		orig_nameserver = nameserver;
		netSetNameserver(nameserver.c_str());
	}
}

void CNetworkConfig::startNetwork(void)
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::startNetwork\n");

	std::string cmd = "/sbin/ifup " + ifname;

	safe_system(cmd.c_str());

	if (!inet_static) 
	{
		init_vars();
	}
}

void CNetworkConfig::stopNetwork(void)
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::stopNetwork\n");

	std::string cmd = "/sbin/ifdown " + ifname;

	safe_system(cmd.c_str());
}

void CNetworkConfig::readWpaConfig()
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::readWpaConfig\n");

	std::ifstream F("/etc/network/if-pre-up.d/wlan");
	ssid = "";
	key = "";
	encryption = "WPA2";
	
	if(F.is_open()) 
	{
		std::string line;
		std::string authmode = "WPA2PSK";
		
		while (std::getline(F, line)) 
		{
			if (line.length() < 5)
				continue;
			if (!line.compare(0, 3, "E=\""))
				ssid = line.substr(3, line.length() - 4);
			else if (!line.compare(0, 3, "A=\""))
				authmode = line.substr(3, line.length() - 4);
			else if (!line.compare(0, 3, "K=\""))
				key = line.substr(3, line.length() - 4);
		}
		F.close();
		if (authmode == "WPAPSK")
			encryption = "WPA";
	}
}

void CNetworkConfig::saveWpaConfig()
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::saveWpaConfig\n");

	std::ofstream F("/etc/network/if-pre-up.d/wlan");
	
	if(F.is_open()) 
	{
		chmod("/etc/network/if-pre-up.d/wlan", 0755);
		// We don't have this information  --martii

		std::string authmode = "WPA2PSK"; // WPA2
		std::string encryptype = "AES"; // WPA2
		std::string proto = "RSN";
		
		if (encryption == "WPA") 
		{
			proto = "WPA";
			authmode = "WPAPSK";
			encryptype = "TKIP";
		}
		
		F << "#!/bin/sh\n"
                  << "# AUTOMATICALLY GENERATED. DO NOT MODIFY.\n"
		  << "grep $IFACE: /proc/net/wireless >/dev/null 2>&1 || exit 0\n"
		  << "kill -9 $(pidof wpa_supplicant 2>/dev/null) 2>/dev/null\n"
		  << "E=\"" << ssid << "\"\n"
		  << "A=\"" << authmode << "\"\n"
		  << "C=\"" << encryptype << "\"\n"
		  << "K=\"" << key << "\"\n"
		  << "ifconfig $IFACE down\n"
		  << "ifconfig $IFACE up\n"
		  << "iwconfig $IFACE mode managed\n"
		  << "iwconfig $IFACE essid \"$E\"\n"
		  << "iwpriv $IFACE set AuthMode=$A\n"
		  << "iwpriv $IFACE set EncrypType=$C\n"
		  << "if ! iwpriv $IFACE set \"WPAPSK=$K\"\n"
		  << "then\n"
		  << "\t/usr/sbin/wpa_supplicant -B -i$IFACE -c/etc/wpa_supplicant.conf\n"
		  << "\tsleep 3\n"
		  << "fi\n";
		F.close();

		F.open("/etc/wpa_supplicant.conf");
		if(F.is_open()) 
		{
			F << "# AUTOMATICALLY GENERATED. DO NOT MODIFY.\n"
			  << "ctrl_interface=/var/run/wpa_supplicant\n\n"
			  << "network={\n"
			  << "\tscan_ssid=1\n"
			  << "\tssid=\"" << ssid << "\"\n"
			  << "\tkey_mgmt=WPA-PSK\n"
			  << "\tproto=" << proto << "\n"
			  << "\tpairwise=CCMP TKIP\n"
			  << "\tgroup=CCMP TKIP\n"
			  << "\tpsk=\"" << key << "\"\n"
			  << "}\n";
			F.close();
		}
	}
}

