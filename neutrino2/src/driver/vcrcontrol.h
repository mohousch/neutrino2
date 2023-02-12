/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vcrcontrol.h 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
#ifndef __vcrcontrol__
#define __vcrcontrol__

#include <string>
#include <list>

#include <sectionsd/sectionsdclient.h>
#include <timerdclient/timerdclient.h>

#include <neutrinoMessages.h>

/*zapit includes*/
#include <client/zapitclient.h>


class CVCRControl
{
	public:
		typedef enum CVCRStates 
		{
			CMD_VCR_UNKNOWN =	0,
			CMD_VCR_RECORD	=	1,
			CMD_VCR_STOP	=	2,
			CMD_VCR_PAUSE	=	3,
			CMD_VCR_RESUME	=	4,
			CMD_VCR_AVAILABLE =	5
		} CVCRCommand;
	
		enum CVCRDevices
		{
			DEVICE_VCR,
			DEVICE_SERVER,
			DEVICE_FILE
		};

		class CDevice			// basisklasse f�r die devices
		{
			public:
				//int sock_fd;
				int last_mode;
				time_t start_time;
				virtual CVCRDevices getDeviceType(void) const = 0;
				CVCRStates  deviceState;
				virtual bool Stop() = 0;
				virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0, const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time=0) = 0; // epg_time added for .xml (MovieBrowser)
				virtual bool Pause() = 0;
				virtual bool Resume() = 0;
				virtual bool IsAvailable() = 0;
				CDevice() { deviceState = CMD_VCR_STOP; };
				virtual ~CDevice(){};
				
				typedef struct {
					unsigned short apid;
					unsigned int index;
					bool ac3;
				} APIDDesc;
				typedef std::list<APIDDesc> APIDList;
				
				virtual void getAPIDs(const unsigned char apids, APIDList & apid_list);
		};

		class CVCRDevice : public CDevice		// VCR per IR
		{
			public:
				bool  SwitchToScart;
				
				virtual CVCRDevices getDeviceType(void) const
				{
					return DEVICE_VCR;
				};
				
				virtual bool Stop(); 
				virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0, const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time=0); // epg_time added for .xml (MovieBrowser)
				virtual bool Pause();
				virtual bool Resume();
				virtual bool IsAvailable() { return true; };
				CVCRDevice(bool switchtoscart) { SwitchToScart = switchtoscart; };
				virtual ~CVCRDevice(){};
		};

		class CFileAndServerDevice : public CDevice
		{
			protected:
				void RestoreNeutrino(void);
				void CutBackNeutrino(const t_channel_id channel_id, const int mode);
				std::string getCommandString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, unsigned char apids);
				std::string getMovieInfoString(const CVCRCommand command, const t_channel_id channel_id,const event_id_t epgid, const std::string& epgTitle, APIDList apid_list, const time_t epg_time);

			public:
				virtual bool Pause()
				{
					return false;
				};

				virtual bool Resume()
				{
					return false;
				};

				virtual bool IsAvailable()
				{
					return true;
				};
				
				//
				unsigned short g_vpid;
				unsigned short g_vtype;
				unsigned short g_apids[10];
				unsigned short g_ac3flags[10];
				unsigned short g_numpida;
				unsigned int g_currentapid, g_currentac3;
				//
				
				unsigned long long            record_EPGid;
				unsigned long long            record_next_EPGid;
				CZapitClient::responseGetPIDs pids;
				void processAPIDnames();
		};

		class CFileDevice : public CFileAndServerDevice
		{
			public:
				std::string  Directory;
	
				virtual CVCRDevices getDeviceType(void) const
				{
					return DEVICE_FILE;
				};
					
				virtual bool Stop(); 
				virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0, const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time=0); // epg_time added for .xml (MovieBrowser)
	
				// file
				CFileDevice(const char * const directory )
				{
					Directory          = directory;
				};

				virtual ~CFileDevice()
				{
				};
		};

	public:
		CVCRControl();
		~CVCRControl();
		static CVCRControl * getInstance();

		CDevice * Device;
			
		void registerDevice(CDevice * const device);
		void unregisterDevice();

		inline bool isDeviceRegistered(void) const { return (Device != NULL); };

		inline CVCRStates getDeviceState(void) const { return Device->deviceState; };
		bool Stop(){return Device->Stop();};
		bool Record(const CTimerd::RecordingInfo * const eventinfo);
		bool Pause(){return Device->Pause();};
		bool Resume(){return Device->Resume();};
		bool Screenshot(const t_channel_id channel_id, char * fname = NULL);
};

#endif
