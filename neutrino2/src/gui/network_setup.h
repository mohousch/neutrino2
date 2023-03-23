/*
	Neutrino-GUI  -   DBoxII-Project

	$id: network_setup.h 2016.01.02 20:19:28 mohousch $
	
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

#ifndef __network_setup__
#define __network_setup__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>
#include <system/configure_network.h>
#include <system/setting_helpers.h>

#include <string>


// IP change notifier
class CIPChangeNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const std::string& locale, void * Data);
};

class CNetworkSettings : public CMenuTarget, CChangeObserver
{
	private:
		CIPChangeNotifier* MyIPChanger;
		
		void showMenu();
		
	public:
		CNetworkSettings();
		~CNetworkSettings();
		
		CNetworkConfig *networkConfig;
		CMenuItem * wlanEnable[3];
		
		int network_dhcp;
		int network_automatic_start;
		
		std::string network_hostname;
		std::string mac_addr;
		
		std::string network_ssid;
		std::string network_key;
		int network_encryption;
		
		static CNetworkSettings* getInstance();

		void readNetworkSettings(std::string iname);
		void setNetwork(void);
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// dhcp notifier
class CDHCPNotifier : public CChangeObserver
{
	private:
		ClistBoxItem * toDisable[5];
	public:
		CDHCPNotifier(ClistBoxItem*, ClistBoxItem*, ClistBoxItem*, ClistBoxItem*, ClistBoxItem*);
		bool changeNotify(const std::string&, void * data);
};

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool dhcp);
void showCurrentNetworkSettings();

#endif //__network_setup__
