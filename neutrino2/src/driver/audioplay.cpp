//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: audioplay.cpp 27042026 mohousch Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>

extern "C" {
#include <libavcodec/version.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,0,100)
#include <libavcodec/avcodec.h>
#endif
}

#include <neutrino2.h>

#include <driver/audioplay.h>
#include <system/debug.h>
#include <sectionsd/edvbstring.h> // UTF8

#include <playback_cs.h>


static OpenThreads::Mutex mutex;

void CAudioPlayer::stop()
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	state = STOP_REQ;

	//
	if(thrPlay)
		pthread_join(thrPlay, NULL);
	
	thrPlay = 0;

	if(playback->playing)
		playback->Close();
	
	state = STOP;
}

void CAudioPlayer::pause()
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	if(state == PLAY || state == FF || state == REV)
	{
		state = PAUSE;

		playback->SetSpeed(0);
	}
	else if(state == PAUSE)
	{
		state = PLAY;

		playback->SetSpeed(1);		
	}
}

void CAudioPlayer::ff(unsigned int seconds)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	m_SecondsToSkip = seconds;

	if(state == PLAY || state == PAUSE || state == REV)
	{
		state = FF;

		playback->SetSpeed(2);
	}
	else if(state == FF)
	{
		state = PLAY;
		
		playback->SetSpeed(1);
	}
}

void CAudioPlayer::rev(unsigned int seconds)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	m_SecondsToSkip = seconds;

	if(state == PLAY || state == PAUSE || state == FF)
	{
		state = REV;

		playback->SetSpeed(-2);
	}
	else if(state == REV)
	{
		state = PLAY;
		
		playback->SetSpeed(1);
	}
}

CAudioPlayer * CAudioPlayer::getInstance()
{
	static CAudioPlayer * AudioPlayer = NULL;
	
	if(AudioPlayer == NULL)
	{
		AudioPlayer = new CAudioPlayer();
	}
	
	return AudioPlayer;
}

void * CAudioPlayer::PlayThread( void * /*arg*/)
{
	//stop playing if already playing (multiselect)
	if (playback->playing)
		playback->Close();
		
	if (!playback->Open())	  
		return NULL;
			
	if (!playback->Start( (char*)getInstance()->m_Audiofile.Filename.c_str() ))
	{
		playback->Close();
		return NULL;
	}
		
	getInstance()->state = PLAY;
	
	int position = 0;
	int duration = 0;
	
	do {
		if(!playback->GetPosition(position, duration))	
		{
			getInstance()->state = STOP;
			
			if(playback->playing)
				playback->Close();
				
			break;	
		}
		
		if (getInstance()->m_Audiofile.MetaData.total_time == 0) 
			getInstance()->m_Audiofile.MetaData.total_time = duration / 1000; // in sec

		getInstance()->m_played_time = position/1000;	// in sec
	}while(getInstance()->state != STOP_REQ);
	
	if(playback->playing)
		playback->Close();

	getInstance()->state = STOP;
	
	pthread_exit(0);

	return NULL;
}

bool CAudioPlayer::play(const CAudiofile *file)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::play:\n");
	
	if (state != STOP)
		stop();

	getInstance()->clearFileData();

	m_Audiofile = *file;
	
	state = PLAY;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	bool ret = true;

	// play thread (retrieve position/duration etc...)
	if (pthread_create (&thrPlay, &attr, PlayThread, (void*)&ret) != 0 )
	{
		perror("audioplay: pthread_create(PlayThread)");
		ret = false;
	}	

	pthread_attr_destroy(&attr);

	return ret;
}

CAudioPlayer::CAudioPlayer()
{
	init();
}

CAudioPlayer::~CAudioPlayer()
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	CFileHelpers::getInstance()->removeDir("/tmp/audioplayer");
}

void CAudioPlayer::init()
{
	dprintf(DEBUG_INFO, "CAudioPlayer::%s\n", __FUNCTION__);
	
	state = STOP;	
	thrPlay = 0;

	CFileHelpers::getInstance()->createDir("/tmp/audioplayer", 0755);
	
	//
	meta_data_valid = false;
	avc = NULL;
	codec = NULL;
	
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	avcodec_register_all();
	av_register_all();
#endif	
}

void CAudioPlayer::deinit()
{
	dprintf(DEBUG_INFO, "CAudioPlayer::%s\n", __FUNCTION__);
	
	if (avc)
	{
		avformat_close_input(&avc);
		avformat_free_context(avc);
		avc = NULL;
	}
	
	codec = NULL;
	
	CFileHelpers::getInstance()->removeDir("/tmp/audioplayer");
}

void CAudioPlayer::clearFileData()
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::%s\n", __FUNCTION__);
	
	m_Audiofile.clear();
	m_played_time = 0;
}

CAudioMetaData CAudioPlayer::getMetaData()
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::%s\n", __FUNCTION__);
	
	CAudioMetaData m = m_Audiofile.MetaData;
	m_Audiofile.MetaData.changed = false;
	
	return m;
}

bool CAudioPlayer::hasMetaDataChanged()
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::%s\n", __FUNCTION__);
	
	return m_Audiofile.MetaData.changed;
}

bool CAudioPlayer::readMetaData(CAudiofile *const file)
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::readMetaData\n");
	
	//
	title = "";
	artist = "";
	date = "";
	album = "";
	genre = "";
	type_info = "";
	total_time = 0;
	bitrate = 0;
	meta_data_valid = false;

//FIXME:
#if 0
	int r = avformat_open_input(&avc, file->Filename.c_str(), NULL, NULL);
	if (r)
	{
		if (avc)
		{
			avformat_close_input(&avc);
			avformat_free_context(avc);
			avc = NULL;
		}
		
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

	//	
	if (!meta_data_valid)
	{
		mutex.lock();
		int ret = avformat_find_stream_info(avc, NULL);
		if (ret < 0)
		{
			mutex.unlock();

			if (avc)
			{
				avformat_close_input(&avc);
				avformat_free_context(avc);
				avc = NULL;
			}
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
			if (avc)
			{
				avformat_close_input(&avc);
				avformat_free_context(avc);
				avc = NULL;
			}

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
				cover += file->MetaData.title + ".jpg";
				
				FILE *f = fopen(cover.c_str(), "wb");
				
				if (f)
				{
					AVPacket *pkt = &avc->streams[i]->attached_pic;
					fwrite(pkt->data, pkt->size, 1, f);
					fclose(f);
					file->MetaData.cover = cover;
				}
			}
		}
		
		// total_time
		if (!total_time && file->MetaData.filesize && bitrate)
			total_time = 8 * file->MetaData.filesize / bitrate;

		meta_data_valid = true;
		file->MetaData.changed = true;
	}
	
	file->MetaData.title = title;
	file->MetaData.artist = artist;
	file->MetaData.date = date;
	file->MetaData.album = album;
	file->MetaData.genre = genre;
	file->MetaData.total_time = total_time;
	file->MetaData.type_info = type_info;
	// make sure bitrate is set to prevent refresh metadata from gui, its a flag
	file->MetaData.bitrate = bitrate ? bitrate : 1;
	file->MetaData.samplerate = samplerate;
	
	//
	if (avc)
	{
		avformat_close_input(&avc);
		avformat_free_context(avc);
		avc = NULL;
	}
	
	codec = NULL;
#endif	

	return true;
}

void CAudioPlayer::GetMeta(AVDictionary *metadata)
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::GetMeta\n");
	
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

