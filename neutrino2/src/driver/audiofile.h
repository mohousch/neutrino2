/*
  Neutrino-GUI  -   DBoxII-Project

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

#ifndef __AUDIOFILE_H__
#define __AUDIOFILE_H__

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/version.h>
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,0,100)
#include <libavcodec/avcodec.h>
#endif
}
#include <OpenThreads/Thread>
#include <OpenThreads/Condition>

#include <driver/file.h>


////
class CAudioMetaData
{
	public:
		// constructor
		CAudioMetaData();
		// copy constructor
		CAudioMetaData( const CAudioMetaData& src );
		// assignment operator
		void operator=( const CAudioMetaData& src );
		void clear();

		//
		std::string type_info;
		long filesize; 			// filesize in bits (for mp3: without leading id3 tag)
		unsigned int bitrate; 		// overall bitrate, vbr file: current bitrate
		unsigned int avg_bitrate; 	// average bitrate in case of vbr file 
		unsigned int samplerate;
		time_t total_time;
		long audio_start_pos; 		// position of first audio frame
		std::string artist;
		std::string title;
		std::string album;
		std::string sc_station;
		std::string date;
		std::string genre;
		std::string track;
		std::string cover;
		bool changed;
};

////
class CAudiofile
{
	public:
		/* constructors */
		CAudiofile();
		CAudiofile( std::string name, CFile::FileExtension extension );
		CAudiofile( const CAudiofile& src );

		void operator=( const CAudiofile& src );
		void clear();

		CAudioMetaData MetaData;
		std::string Filename;
		CFile::FileExtension FileExtension;
};

typedef std::vector<CAudiofile> CAudioPlayList;

////
class CFfmpegDec
{
	private:
		bool meta_data_valid;
		bool is_stream;
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
		bool init(const char *_in);
		void deInit(void);
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
		static CFfmpegDec *getInstance();
		bool GetMetaData(const char *in, CAudioMetaData *m);
		CFfmpegDec();
		~CFfmpegDec();
};

#endif /* __AUDIOFILE_H__ */

