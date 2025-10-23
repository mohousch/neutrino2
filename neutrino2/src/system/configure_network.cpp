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

	// get hostname
	hostname = netGetHostname();

	netGetDefaultRoute(router);
	gateway = router;

	// get ip / mask/ broadcast
	if(!inet_static) 
	{
		netGetIP((char *) ifname.c_str(), ip, mask, _broadcast);
		netmask = mask;
		broadcast = _broadcast;
		address = ip;
	}

	// get mac
	netGetMacAddr((char *) ifname.c_str(), addr);

	std::stringstream mac_tmp;
	for(int i = 0; i < 6; ++i)
		mac_tmp<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';

	mac_addr = mac_tmp.str().substr(0,17);

	// wireless
	key = "";
	ssid = "";
	encryption = "WPA2";

	wireless = false;

	std::string tmp = "/sys/class/net/" + ifname + "/wireless";

	if(access(tmp.c_str(), R_OK) == 0)
		wireless = true;
		
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
	if (wireless)
	{
		if ((ssid != orig_ssid) || (key != orig_key))
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
	dprintf(DEBUG_NORMAL, "CNetworkConfig::commitConfig: automatic_start:%d inet_static:%d hostname:%s ssid:%s key:%s encryption:%s\n", automatic_start, inet_static, hostname.c_str(), ssid.c_str(), key.c_str(), encryption.c_str());

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

		if( wireless /*&& ((key != orig_key) || (ssid != orig_ssid) || (encryption != orig_encryption))*/ )
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

	std::string   s;
	std::ifstream in("/etc/wpa_supplicant.conf");

	ssid = "";
	key = "";
	
	if (!in.is_open())
	{
		perror("/etc/wpa_supplicant.conf read error");
		return;
	}
	
	while (getline(in, s))
	{
		if (s[0] == '#')
			continue;
			
		std::string::size_type i = s.find('=');
		
		if (i != std::string::npos)
		{
			std::string n = s.substr(0, i);
			std::string val = s.substr(i + 1, s.length() - (i + 1));

			while ((i = n.find(' ')) != std::string::npos)
				n.erase(i, 1);
			while ((i = n.find('\t')) != std::string::npos)
				n.erase(i, 1);

			if ((i = val.find('"')) != std::string::npos)
				val.erase(i, 1);
			if ((i = val.rfind('"')) != std::string::npos)
				val.erase(i, 1);

			if (n == "ssid")
				ssid = val;
			else if (n == "psk")
				key = val;
		}
	}

	printf("CNetworkConfig::readWpaConfig: ssid %s key %s\n", ssid.c_str(), key.c_str());
}

void CNetworkConfig::saveWpaConfig()
{
	dprintf(DEBUG_NORMAL, "CNetworkConfig::saveWpaConfig\n");

	std::ofstream out("/etc/wpa_supplicant.conf");
	
	if (out.is_open())
	{
		out << "# generated by Neutrino2\n";
		out << "ctrl_interface=/var/run/wpa_supplicant\n";
		out << "network={\n";
		out << "	ssid=\"" + ssid + "\"\n";
		
		if (!key.empty())
		{
			out << "	psk=\"" + key + "\"\n";;
			out << "	proto=WPA WPA2\n";
			out << "	key_mgmt=WPA-PSK\n";
			out << "	pairwise=CCMP TKIP\n";
			out << "	group=CCMP TKIP\n";
		}
		else
		{
			out << "	key_mgmt=NONE\n";
		}
		out << "}\n";
		out.close();
	}
}

