/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vcrcontrol.h 31.03.2024 mohousch Exp $

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

#include <driver/movieinfo.h>

#include <zapit/zapit.h>

#include <driver/genpsi.h>

#include <OpenThreads/Mutex>
#include <OpenThreads/Thread>

extern "C" {
#include <libavcodec/version.h>
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,0,100)
#include <libavcodec/bsf.h>
#endif
}

#if (LIBAVCODEC_VERSION_MAJOR > 55)
#define	av_free_packet av_packet_unref
#else
#define av_packet_unref	av_free_packet
#endif


enum stream2file_error_msg_t
{
	STREAM2FILE_OK                        	=  0,
	STREAM2FILE_IDLE			=  1,
	STREAM2FILE_BUSY                      	= -1,
	STREAM2FILE_INVALID_DIRECTORY         	= -2,
	STREAM2FILE_INVALID_PID              	= -3,
	STREAM2FILE_RECORDING_THREADS_FAILED	= -4,
};

typedef struct pvr_file_info
{
	uint32_t  uDuration;      /* Time duration in Ms */
	uint32_t  uTSPacketSize;
} PVR_FILE_INFO;

class CVCRControl : public OpenThreads::Thread
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
		t_channel_id channel_id;
		CMovieInfo * g_cMovieInfo;
		MI_MOVIE_INFO * g_movieInfo;
		std::string ext_channel_name;
				
		typedef struct {
			unsigned short apid;
			unsigned int index;
			bool ac3;
			std::string language;
		} APIDDesc;
		typedef std::list<APIDDesc> APIDList;
				
		virtual void getAPIDs(const t_channel_id channel_id, const unsigned char apids, APIDList & apid_list);
		
		bool  SwitchToScart;
		unsigned long long record_EPGid;
		CZapit::responseGetPIDs pids;
		
		//
		CGenPsi psi;
		
	private:
		//
		void processAPIDnames();

		bool doRecord(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0, const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time = 0);

		stream2file_error_msg_t startRecording(const char * const filename,
					const char * const info,
					unsigned short vpid,
					unsigned short * apids,
					int numpids);			
		void stopRecording();

	protected:
		void RestoreNeutrino(void);
		void CutBackNeutrino(const t_channel_id channel_id, const int mode);
		std::string getMovieInfoString(const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, APIDList apid_list, const time_t epg_time);
	
	public:
		CVCRControl();
		virtual ~CVCRControl();
		static CVCRControl * getInstance();

		inline CVCRStates getDeviceState(void) const { return deviceState; };
		bool Record(const CTimerd::RecordingInfo * const eventinfo);
		bool Screenshot(const t_channel_id channel_id, char * fname = NULL);
		void Stop();
		
	//
	private:
		AVFormatContext *ifcx;
		AVFormatContext *ofcx;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,48,100)
		AVBitStreamFilterContext *bsfc;
#else
		AVBSFContext *bsfc;
#endif
		//
		bool stopped;
		time_t time_started;
		int  stream_index;

		//
		bool saveXML(const char* const filename, const char* const info);
		void FillMovieInfo(CZapitChannel* channel, APIDList& apid_list);
		bool Start();

		void Close();
		bool Open(CZapitChannel* channel, const char* const filename);
		void run();
		void WriteHeader(uint32_t duration);
		
		////
		stream2file_error_msg_t startWebTVRecording(const char* const filename, const event_id_t epgid, const std::string& epgTitle, const time_t epg_time);
		void stopWebTVRecording();
};

#endif
