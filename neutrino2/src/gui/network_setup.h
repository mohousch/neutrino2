//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: network_setup.h 26022025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 

#ifndef __network_setup__
#define __network_setup__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>
#include <system/configure_network.h>

#include <string>


class CIPChangeNotifier;
////
class CNetworkSettings : public CWidgetTarget, CChangeObserver
{
	private:
		CWidget* widget;
		ClistBox* networkSettings;
		CMenuItem *m10;
	
		CIPChangeNotifier* MyIPChanger;
		
		void showMenu();
		
	public:
		CNetworkSettings();
		~CNetworkSettings(){};
		
		CNetworkConfig *networkConfig;
		CMenuItem *wlanEnable[4];
		
		////
		int network_dhcp;
		int network_automatic_start;
		std::string network_hostname;
		std::string mac_addr;
		std::string network_ssid;
		std::string network_key;
		int network_encryption;
		
		static CNetworkSettings* getInstance();

		void readNetworkSettings(std::string iname);
		void commitNetworkSettings();
		void setNetwork(void);
		void getWlanList(void);
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

// IP notifier
class CIPChangeNotifier : public CChangeObserver
{
	private:
		CMenuItem *menuItem[4];
		
	public:
		CIPChangeNotifier(CMenuItem *m[4]);
		bool changeNotify(const std::string& locale, void * Data);
};

// dhcp notifier
class CDHCPNotifier : public CChangeObserver
{
	private:
		CMenuForwarder * toDisable[5];
	public:
		CDHCPNotifier(CMenuForwarder*, CMenuForwarder*, CMenuForwarder*, CMenuForwarder*, CMenuForwarder*);
		bool changeNotify(const std::string&, void * data);
};

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool dhcp);
void showCurrentNetworkSettings();

#endif //__network_setup__

