//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: audioplay.h 27042026 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//
//	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
//	Copyright (C) 2002,2003 Dirch
//	Copyright (C) 2002,2003,2004 Zwen
//	Homepage: http://www.dbox2.info/
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

#ifndef __AUDIO_PLAY__
#define __AUDIO_PLAY__

#include <unistd.h>
#include <pthread.h>

#include <driver/audiofile.h>
#include <string>

extern "C" {
#include <libavcodec/version.h>
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,0,100)
#include <libavcodec/avcodec.h>
#endif
}
#include <OpenThreads/Thread>
#include <OpenThreads/Condition>
#include <OpenThreads/ScopedLock>


////
class CAudioPlayer
{
	public:
		enum State {
			STOP = 0, 
			STOP_REQ, 
			PLAY, 
			PAUSE, 
			FF, 
			REV
		};
		
		CAudiofile m_Audiofile;
		
	private:		
		time_t m_played_time;	
		pthread_t thrPlay;
		State state;
		static void *PlayThread(void*);
		void clearFileData();
		unsigned int m_SecondsToSkip;
		
		////
		bool meta_data_valid;
		int mChannels;
		//
		AVFormatContext *avc;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,0,100)
		AVCodec *codec;
#else
		const AVCodec *codec;
#endif
		//
		int best_stream;
		void GetMeta(AVDictionary *metadata);
		//
		std::string title;
		std::string artist;
		std::string date;
		std::string album;
		std::string genre;
		std::string type_info;
		time_t total_time;
		int bitrate;
		int samplerate;

	public:
		////
		static CAudioPlayer* getInstance();
		
		////
		bool play(const CAudiofile*);
		void stop();
		void pause();
		void init();
		void deinit();
		void ff(unsigned int seconds = 0);
		void rev(unsigned int seconds = 0);
		CAudioMetaData getMetaData();
		bool hasMetaDataChanged();
		bool readMetaData(CAudiofile* const);
		time_t getTimePlayed(){return m_played_time;}
		time_t getTimeTotal(){return m_Audiofile.MetaData.total_time;}
		State getState(){return state;}

		////
		CAudioPlayer();
		~CAudioPlayer();
};

#endif

