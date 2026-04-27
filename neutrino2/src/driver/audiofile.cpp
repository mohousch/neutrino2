//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: audiofile.cpp 21122024 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

extern "C" {
#include <libavcodec/version.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,0,100)
#include <libavcodec/avcodec.h>
#endif
}

#include <OpenThreads/ScopedLock>

#include <system/helpers.h>
#include <system/debug.h>

#include <driver/audiofile.h>
#include <system/helpers.h>


////
CAudioMetaData::CAudioMetaData()
{
	clear();
}

// copy constructor
CAudioMetaData::CAudioMetaData( const CAudioMetaData& src )
  : type_info( src.type_info ),
	filesize( src.filesize ), bitrate( src.bitrate ),
	avg_bitrate( src.avg_bitrate ), samplerate( src.samplerate ),
	total_time( src.total_time ),
	audio_start_pos( src.audio_start_pos ), artist( src.artist ),
	title( src.title ), album( src.album ), sc_station( src.sc_station ),
	date( src.date ), genre( src.genre ), track( src.track ),cover(src.cover),
	changed( src.changed )
{
}

// assignment operator
void CAudioMetaData::operator=( const CAudioMetaData& src )
{
	// self assignment check
	if ( &src == this )
		return;

	type_info = src.type_info;
	filesize = src.filesize;
	bitrate = src.bitrate;
	avg_bitrate = src.avg_bitrate;
	samplerate = src.samplerate;
	total_time = src.total_time;
	audio_start_pos = src.audio_start_pos;
	artist = src.artist;
	title = src.title;
	album = src.album;
	date = src.date;
	genre = src.genre;
	track = src.track;
	cover = src.cover;
	sc_station = src.sc_station;
	changed = src.changed;
}

void CAudioMetaData::clear()
{
	type_info.clear();
	filesize = 0;
	bitrate = 0;
	avg_bitrate = 0;
	samplerate = 0;
	total_time = 0;
	audio_start_pos = 0;
	artist.clear();
	title.clear();
	album.clear();
	sc_station.clear();
	date.clear();
	genre.clear();
	track.clear();
	cover.clear();
	changed = false;
}

////
CAudiofile::CAudiofile()
  : MetaData(), Filename(), FileExtension( CFile::EXTENSION_UNKNOWN )
{
}

// constructor
CAudiofile::CAudiofile( std::string name, CFile::FileExtension extension )
	: MetaData(), Filename( name ), FileExtension( extension )
{
}

// copy constructor
CAudiofile::CAudiofile( const CAudiofile& src )
  : MetaData( src.MetaData ), Filename( src.Filename ),
	FileExtension( src.FileExtension )
{
}

// assignment operator 
void CAudiofile::operator=( const CAudiofile& src )
{
	MetaData = src.MetaData;
	Filename = src.Filename;
	FileExtension = src.FileExtension;
}

void CAudiofile::clear()
{
	MetaData.clear();
	Filename.clear();
	FileExtension = CFile::EXTENSION_UNKNOWN;
}

