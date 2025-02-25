//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: remotecontrol.h 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifndef __remotecontrol__
#define __remotecontrol__

#include <vector>
#include <string>

#include <driver/rcinput.h> 			/* neutrino_msg_t, neutrino_msg_data_t */

//
#include <zapit/zapit.h>


//// CZapProtection
class CZapProtection
{
	protected:
		char * validPIN;
	public:
		int fsk;

		CZapProtection(char * validpin, int FSK){ validPIN = validpin; fsk = FSK; };
		~CZapProtection(){};
		bool check();
};

////
class CSubService
{
	private:
		struct CZapit::subServices service;
	
	public:
		time_t      starttime;
		unsigned    duration;
		std::string subservice_name;
	
		CSubService(const t_original_network_id, const t_service_id, const t_transport_stream_id, const std::string &asubservice_name);
		CSubService(const t_original_network_id, const t_service_id, const t_transport_stream_id, const time_t astarttime, const unsigned aduration);
	
		t_channel_id getChannelID(void) const;
		inline const struct CZapit::subServices getAsZapitSubService(void) const { return service;}
};

typedef std::vector<CSubService> CSubServiceListSorted;

//// CRemoteControl
class CRemoteControl
{
	unsigned long long      zap_completion_timeout;
	t_channel_id            current_sub_channel_id;

	void getNVODs();
	void getSubChannels();
	void copySubChannelsToZapit(void);

	public:
		CRemoteControl();
		virtual ~CRemoteControl(){};
		
		////
		t_channel_id current_channel_id;
		unsigned long long current_EPGid;
		unsigned long long next_EPGid;
		CZapit::PIDs current_PIDs;
	
		// APID - Details
		bool has_ac3;
		bool has_unresolved_ctags;
	
		// SubChannel/NVOD - Details
		CSubServiceListSorted subChannels;
		int selected_subchannel;
		bool are_subchannels;
		bool needs_nvods;
		int director_mode;
	
		// Video / Parental-Lock
		CZapProtection *zapProtection;
		bool is_video_started;
	
		////
		void zapToChannelID(const t_channel_id channel_id, const bool start_video = true); // UTF-8
		void startvideo(const t_channel_id channel_id);
		void stopvideo();
		void setAPID(uint32_t APID);
		void processAPIDnames();
		void processZapProtection(const neutrino_msg_t msg, const neutrino_msg_data_t data);
		const std::string & setSubChannel(const int numSub, const bool force_zap = false);
		const std::string & subChannelUp(void);
		const std::string & subChannelDown(void);
		void radioMode();
		void tvMode();
		////
		int handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);
};

#endif

