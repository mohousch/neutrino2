/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: audioplay.cpp 24.11.2023 mohousch Exp $

	audioplayer
	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	Copyright (C) 2002,2003 Dirch
	Copyright (C) 2002,2003,2004 Zwen
	Homepage: http://www.dbox2.info/

	Kommentar:

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

#include "global.h"
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>

#include <neutrino2.h>

#include <driver/audioplay.h>
#include <system/debug.h>
#include <sectionsd/edvbstring.h> // UTF8
#include <playback_cs.h>
#include <driver/netfile.h>


void ShoutcastCallback(void *arg)
{
	dprintf(DEBUG_NORMAL, "%s\n", __FUNCTION__);
	
	CAudioPlayer::getInstance()->sc_callback(arg);
}

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
	// shoutcast
	FILE* fp = NULL;
	
	if(getInstance()->m_Audiofile.FileExtension == CFile::EXTENSION_URL)
	{
		fp = ::f_open( getInstance()->m_Audiofile.Filename.c_str(), "rc" );

		if ( fp == NULL )
		{
			dprintf(DEBUG_INFO, "CAudioPlayer::PlayThread: Error opening file %s.\n", getInstance()->m_Audiofile.Filename.c_str() );
			
			return NULL;
		}
		else
		{
			if ( ::fstatus(fp, ShoutcastCallback) < 0 )
				fprintf( stderr, "CAudioPlayer::PlayThread: Error adding shoutcast callback: %s\n", err_txt );
		}

		if ( ::f_close( fp ) == EOF )
		{
			fprintf( stderr, "CAudioPlayer::PlayThread: Could not close file %s.\n", getInstance()->m_Audiofile.Filename.c_str() );
		}
	}

	//stop playing if already playing (multiselect)
	if(playback->playing)
		playback->Close();
		
	if(! playback->Open())	  
		return NULL;
			
	if(!playback->Start( (char*)getInstance()->m_Audiofile.Filename.c_str() ))
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

bool CAudioPlayer::play(const CAudiofile *file, const bool highPrio)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	if (state != STOP)
		stop();

	getInstance()->clearFileData();

	m_Audiofile = *file;
	
	state = PLAY;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(highPrio)
	{
		struct sched_param param;
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		param.sched_priority = 1;
		pthread_attr_setschedparam(&attr, &param);
		usleep(100000); // give the event thread some time to handle his stuff without this sleep there were duplicated events...
	}

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
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	state = STOP;	
	thrPlay = 0;

	CFileHelpers::getInstance()->createDir("/tmp/audioplayer", 0755);	
}

void CAudioPlayer::sc_callback(void *arg)
{
	dprintf(DEBUG_NORMAL, "CAudioPlayer::%s\n", __FUNCTION__);
	
	bool changed = false;
	CSTATE *stat = (CSTATE*)arg;
	
	const std::string artist = isUTF8(stat->artist)	? stat->artist : convertLatin1UTF8(stat->artist);
	const std::string title	= isUTF8(stat->title) ? stat->title : convertLatin1UTF8(stat->title);
	const std::string station = isUTF8(stat->station) ? stat->station : convertLatin1UTF8(stat->station);
	const std::string genre	= isUTF8(stat->genre) ? stat->artist : convertLatin1UTF8(stat->genre);

	if (m_Audiofile.MetaData.artist != artist)
	{
		m_Audiofile.MetaData.artist = artist;
		changed = true;
	}
	
	if (m_Audiofile.MetaData.title != title)
	{
		m_Audiofile.MetaData.title = title;
		changed = true;
	}
	
	if (m_Audiofile.MetaData.sc_station != station)
	{
		m_Audiofile.MetaData.sc_station = station;
		changed = true;
	}
	
	if (m_Audiofile.MetaData.genre != genre)
	{
		m_Audiofile.MetaData.genre = genre;
		changed = true;
	}
	
	if(changed)
	{
		m_played_time = 0;
	}
	
	m_sc_buffered = stat->buffered;
	m_Audiofile.MetaData.changed = changed;
}

void CAudioPlayer::clearFileData()
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::%s\n", __FUNCTION__);
	
	m_Audiofile.clear();
	m_played_time = 0;
	m_sc_buffered = 0;
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

bool CAudioPlayer::readMetaData(CAudiofile* const file, const bool nice)
{
	dprintf(DEBUG_DEBUG, "CAudioPlayer::%s\n", __FUNCTION__);
	
	bool Status = true;
	FILE* fp = NULL;

	fp = fopen( file->Filename.c_str(), "r" );

	if ( fp == NULL )
	{
		dprintf(DEBUG_DEBUG, "CAudioPlayer::readMetaData: Error opening file %s for meta data reading.\n", file->Filename.c_str() );
		Status = false;
	}
	else
	{
		Status = CFfmpegDec::getInstance()->GetMetaData(file->Filename.c_str(), &file->MetaData);
			
		if ( fclose( fp ) == EOF )
		{
			dprintf(DEBUG_NORMAL, "Could not close file %s.\n", file->Filename.c_str() );
		}
	}

	return Status;
}

