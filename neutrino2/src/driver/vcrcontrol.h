/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vcrcontrol.h 30.10.2023 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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
#ifndef __vcrcontrol__
#define __vcrcontrol__

#include <string>
#include <list>

#include <sectionsd/sectionsd.h>
#include <timerd/timerd.h>

#include <neutrinoMessages.h>

#include <gui/movieinfo.h>

/*zapit includes*/
#include <zapit/zapit.h>


enum stream2file_error_msg_t
{
	STREAM2FILE_OK                        =  0,
	STREAM2FILE_BUSY                      = -1,
	STREAM2FILE_INVALID_DIRECTORY         = -2,
	STREAM2FILE_INVALID_PID               = -3,
	STREAM2FILE_PES_FILTER_FAILURE        = -4,
	STREAM2FILE_DVR_OPEN_FAILURE          = -5,
	STREAM2FILE_RECORDING_THREADS_FAILED  = -6,
};

enum stream2file_status_t
{
	STREAM2FILE_STATUS_RUNNING            =  0,
	STREAM2FILE_STATUS_IDLE               =  1,
	STREAM2FILE_STATUS_BUFFER_OVERFLOW    = -1,
	STREAM2FILE_STATUS_WRITE_OPEN_FAILURE = -2,
	STREAM2FILE_STATUS_WRITE_FAILURE      = -3,
	STREAM2FILE_STATUS_READ_FAILURE       = -4
};

class CVCRControl
{
	public:
		typedef enum CVCRStates 
		{
			CMD_VCR_UNKNOWN =	0,
			CMD_VCR_RECORD	=	1,
			CMD_VCR_STOP	=	2
		} CVCRCommand;
	
		int last_mode;
		time_t start_time;
		CVCRStates  deviceState;
		
		std::string  Directory;
		
		stream2file_status_t exit_flag;
		t_channel_id channel_id;
		
		////
		CMovieInfo * g_cMovieInfo;
		MI_MOVIE_INFO * g_movieInfo;
		
		std::string ext_channel_name;
				
		typedef struct {
			unsigned short apid;
			unsigned int index;
			bool ac3;
		} APIDDesc;
		typedef std::list<APIDDesc> APIDList;
				
		virtual void getAPIDs(const t_channel_id channel_id, const unsigned char apids, APIDList & apid_list);
		
		bool  SwitchToScart;
		
		//
		unsigned short g_vpid;
		unsigned short g_vtype;
		unsigned short g_apids[10];
		unsigned short g_ac3flags[10];
		unsigned short g_numpida;
		unsigned int g_currentapid, g_currentac3;		
		unsigned long long            record_EPGid;
		unsigned long long            record_next_EPGid;
		CZapit::responseGetPIDs pids;
		
		//
		void processAPIDnames();

		bool doRecord(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0, const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time=0); // epg_time added for .xml (MovieBrowser)
		
		//
		struct stream2file_status2_t
		{
			stream2file_status_t status;
			char dir[100];
		};

		stream2file_error_msg_t startRecording(const char * const filename,
					const char * const info,
					unsigned short vpid,
					unsigned short * apids,
					int numpids);			
		stream2file_error_msg_t stopRecording();

	protected:
		void RestoreNeutrino(void);
		void CutBackNeutrino(const t_channel_id channel_id, const int mode);
		std::string getMovieInfoString(const t_channel_id channel_id,const event_id_t epgid, const std::string& epgTitle, APIDList apid_list, const time_t epg_time);
	
	public:
		CVCRControl();
		virtual ~CVCRControl();
		static CVCRControl * getInstance();

		inline CVCRStates getDeviceState(void) const { return deviceState; };
		bool Record(const CTimerd::RecordingInfo * const eventinfo);
		bool Screenshot(const t_channel_id channel_id, char * fname = NULL);
		bool Stop(); 
};

#endif
