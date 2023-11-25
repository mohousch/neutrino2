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

#include <driver/netfile.h>

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

////
static OpenThreads::Mutex mutex;

CFfmpegDec::CFfmpegDec(void)
{
	meta_data_valid = false;
	avc = NULL;		// avcontext
	
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	avcodec_register_all();
	av_register_all();
#endif
}

CFfmpegDec::~CFfmpegDec(void)
{
	deInit();
}

bool CFfmpegDec::init(const char *_in)
{
	dprintf(DEBUG_DEBUG, "CFfmpegDec::init\n");
	
	title = "";
	artist = "";
	date = "";
	album = "";
	genre = "";
	type_info = "";
	total_time = 0;
	bitrate = 0;
	meta_data_valid = false;

	int r = avformat_open_input(&avc, _in, NULL, NULL);
	if (r)
	{
		deInit();
		
		return false;
	}
	
	//
	avc->probesize = 128 * 1024;
	av_opt_set_int(avc, "analyzeduration", 1 * AV_TIME_BASE, 0);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,0,100)
	avc->flags |= AVFMT_FLAG_CUSTOM_IO | AVFMT_FLAG_KEEP_SIDE_DATA;
#else
	avc->flags |= AVFMT_FLAG_CUSTOM_IO;
#endif
	
	return true;
}

void CFfmpegDec::deInit(void)
{
	dprintf(DEBUG_DEBUG, "CFfmpegDec::deInit\n");
	
	if (avc)
	{
		avformat_close_input(&avc);
		//avformat_free_context(avc);
		avc = NULL;
	}
}

CFfmpegDec *CFfmpegDec::getInstance()
{
	static CFfmpegDec *FfmpegDec = NULL;
	
	if (FfmpegDec == NULL)
	{
		FfmpegDec = new CFfmpegDec();
	}
	
	return FfmpegDec;
}

bool CFfmpegDec::GetMetaData(const char *_in, CAudioMetaData *m)
{
	dprintf(DEBUG_DEBUG, "CFfmpegDec::GetMetaData\n");
	
	if (!init(_in))
	{
		return false;
	}
	
	if (!meta_data_valid)
	{
		mutex.lock();
		int ret = avformat_find_stream_info(avc, NULL);
		if (ret < 0)
		{
			mutex.unlock();
			deInit();
			dprintf(DEBUG_NORMAL, "avformat_find_stream_info error %d\n", ret);
			
			return false;
		}
		mutex.unlock();
		
		//
		GetMeta(avc->metadata);
		
		//	
		for (unsigned int i = 0; i < avc->nb_streams; i++)
		{
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
			if (avc->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
#else
			if (avc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
#endif
				GetMeta(avc->streams[i]->metadata);
		}

		codec = NULL;
		best_stream = av_find_best_stream(avc, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

		if (best_stream < 0)
		{
			deInit();
			return false;
		}
		
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
		if (!codec)
			codec = avcodec_find_decoder(avc->streams[best_stream]->codec->codec_id);
			
		samplerate = avc->streams[best_stream]->codec->sample_rate;
		mChannels = av_get_channel_layout_nb_channels(avc->streams[best_stream]->codec->channel_layout);
#else
		if (!codec)
			codec = avcodec_find_decoder(avc->streams[best_stream]->codecpar->codec_id);
			
		samplerate = avc->streams[best_stream]->codecpar->sample_rate;
		mChannels = av_get_channel_layout_nb_channels(avc->streams[best_stream]->codecpar->channel_layout);
#endif
		std::stringstream ss;

		if (codec && codec->long_name != NULL)
			type_info = codec->long_name;
		else if (codec && codec->name != NULL)
			type_info = codec->name;
		else
			type_info = "unknown";
		ss << " / " << mChannels << " channel" << (mChannels > 1 ? "s " : " ");
		type_info += ss.str();

		// bitrate / total_time
		bitrate = 0;
		total_time = 0;

		if (avc->duration != int64_t(AV_NOPTS_VALUE))
			total_time = avc->duration / int64_t(AV_TIME_BASE);
			
		// bitrate / cover
		for (unsigned int i = 0; i < avc->nb_streams; i++)
		{
			// bitrate
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57,25,101)
			if (avc->streams[i]->codec->bit_rate > 0)
				bitrate += avc->streams[i]->codec->bit_rate;
#else
			if (avc->streams[i]->codecpar->bit_rate > 0)
				bitrate += avc->streams[i]->codecpar->bit_rate;
#endif
			// cover
			if ((avc->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC))
			{
				std::string cover("/tmp/audioplayer/");
				cover += m->title + ".jpg";
				
				FILE *f = fopen(cover.c_str(), "wb");
				
				if (f)
				{
					AVPacket *pkt = &avc->streams[i]->attached_pic;
					fwrite(pkt->data, pkt->size, 1, f);
					fclose(f);
					m->cover = cover;
				}
			}
		}
		
		// total_time
		if (!total_time && m->filesize && bitrate)
			total_time = 8 * m->filesize / bitrate;

		meta_data_valid = true;
		m->changed = true;
	}
	
	m->title = title;
	m->artist = artist;
	m->date = date;
	m->album = album;
	m->genre = genre;
	m->total_time = total_time;
	m->type_info = type_info;
	// make sure bitrate is set to prevent refresh metadata from gui, its a flag
	m->bitrate = bitrate ? bitrate : 1;
	m->samplerate = samplerate;
	
	//
	deInit();

	return true;
}

void CFfmpegDec::GetMeta(AVDictionary *metadata)
{
	dprintf(DEBUG_DEBUG, "CFfmpegDec::GetMeta\n");
	
	AVDictionaryEntry *tag = NULL;
	
	while ((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
	{
		if (!strcasecmp(tag->key, "Title"))
		{
			if (title.empty())
			{
				title = isUTF8(tag->value) ? tag->value : convertLatin1UTF8(tag->value);
				title = trim(title);
			}
			continue;
		}
		
		if (!strcasecmp(tag->key, "Artist"))
		{
			if (artist.empty())
			{
				artist = isUTF8(tag->value) ? tag->value : convertLatin1UTF8(tag->value);
				artist = trim(artist);
			}
			continue;
		}
		
		if (!strcasecmp(tag->key, "Year"))
		{
			if (date.empty())
			{
				date = isUTF8(tag->value) ? tag->value : convertLatin1UTF8(tag->value);
				date = trim(date);
			}
			continue;
		}
		
		if (!strcasecmp(tag->key, "Album"))
		{
			if (album.empty())
			{
				album = isUTF8(tag->value) ? tag->value : convertLatin1UTF8(tag->value);
				album = trim(album);
			}
			continue;
		}
		
		if (!strcasecmp(tag->key, "Genre"))
		{
			if (genre.empty())
			{
				genre = isUTF8(tag->value) ? tag->value : convertLatin1UTF8(tag->value);
				genre = trim(genre);
			}
			continue;
		}
	}
}

