/*
 *
 * $Id: playback_cs.cpp,v 1.0 2013/08/18 11:23:30 mohousch Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>

#include <pthread.h>
#include <syscall.h>

#include <unistd.h>

#include "playback_cs.h"

#include <driver/gdi/framebuffer.h>

#include <system/helpers.h>


//// global
#include <common.h>

extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t		ManagerHandler;

static Context_t * player = NULL;

#ifdef HAVE_NO_AV_DECODER
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

OpenThreads::Mutex buf_m;

extern Data_t data[64];
extern uint64_t sCURRENT_APTS;

int buf_in = 0;
int buf_out = 0;
int buf_num = 0;
#endif

////
cPlayback::cPlayback(int)
{
}

bool cPlayback::Open()
{
	printf("cPlayback::Open\n");
	
	mAudioStream = 0;
	mSubStream = -1;
	mExtSubStream = -1;
	mSpeed = 0;
	playing = false;
	
	//
	player = (Context_t*)malloc(sizeof(Context_t));

	//init player
	if(player) 
	{
		player->playback	= &PlaybackHandler;
		player->output		= &OutputHandler;
		player->container	= &ContainerHandler;
		player->manager		= &ManagerHandler;
	}
	
	//
	SubtitleOutputDef_t out;

	out.screen_x = CFrameBuffer::getInstance()->getScreenX(true);
	out.screen_y = CFrameBuffer::getInstance()->getScreenY(true);
	out.screen_width = CFrameBuffer::getInstance()->getScreenWidth(true);
	out.screen_height = CFrameBuffer::getInstance()->getScreenHeight(true);

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_INIT, (void*)&out);
		
#ifdef HAVE_NO_AV_DECODER
	buf_num = 0;
	buf_out = 0;
	buf_in = 0;
#endif

	return true;
}

void cPlayback::Close(void)
{  
	printf("cPlayback::Close\n");
	
	Stop();
	
	//
	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_CLOSE, NULL);

	if(player)
		free(player);

	if(player != NULL)
		player = NULL;
		
#ifdef HAVE_NO_AV_DECODER
	buf_num = 0;
	buf_out = 0;
	buf_in = 0;
#endif
}

// start
bool cPlayback::Start(char *filename)
{
	printf("cPlayback::Start: filename=%s\n", filename);
	
	// check for suburi
	if (filename == NULL)
	{
		playing = false;
		return false;
	}
	
	//
	std::string file("");
	bool isHTTP = false;

	if(!strncmp("http://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("https://", filename, 8))
	{
		isHTTP = true;
	}
	else if(!strncmp("upnp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("rtmp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("rtsp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("mms://", filename, 6))
	{
		isHTTP = true;
	}
	else if(!strncmp("file://", filename, 7))
	{
		isHTTP = false;
	}
	else
		file = "file://";
	
	file.append(filename);

	//open file
	if(player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, (char *)file.c_str()) >= 0) 
	{
		// open suburi
		std::string suburi = filename;
		changeFileNameExt(suburi, ".srt");
		
		if ( !file_exists(suburi.c_str()) )
			changeFileNameExt(suburi, ".ass");
		else if ( !file_exists(suburi.c_str()) )
			changeFileNameExt(suburi, ".ssa");
			
		if ( file_exists(suburi.c_str()) )
		{
			printf("cPlayback::Open suburi:%s\n", suburi.c_str());
			player->playback->Command(player, PLAYBACK_OPEN_SUB, (char*)suburi.c_str());
		}
		
		// start playing
		if(player && player->output && player->playback) 
		{	
			if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) 
			{
				playing = true;
				mSpeed = 1;
			}
			else
			{
				printf("cPlayback::Start: failed to start playing file, sorry we can not play\n");
				playing = false;
			}
		}		
	}
	else
	{
		printf("cPlayback::Start: failed to start playing file, sorry we can not play\n");
		playing = false;
	}

	printf("cPlayback::Start: (playing %d)\n", playing);	

	return playing;
}

bool cPlayback::Play(void)
{
	printf("cPlayback::Play: (playing %d)\n", playing);	

	if(playing == true) 
		return true;
	
	if(player && player->output && player->playback) 
	{		
		if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) // playback.c uses "int = 0" for "true"
		{
			playing = true;
		}
	}

	printf("cPlayback::Play: (playing %d)\n", playing);

	return playing;
}

bool cPlayback::Stop(void)
{ 
	if(playing == false) 
		return false;
	
	printf("cPlayback::Stop: (playing %d)\n", playing);

	if(player && player->playback && player->output) 
		player->playback->Command(player, PLAYBACK_STOP, NULL);

	playing = false;
	
	printf("cPlayback::Stop: (playing %d)\n", playing);

	return true;
}

bool cPlayback::SetAPid(unsigned short pid)
{
	printf("cPlayback::SetAPid: curpid:%d nextpid:%d\n", mAudioStream, pid);
	
	int track = pid;

	if(pid != mAudioStream)
	{
		if(player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&track);

		mAudioStream = pid;
	}

	return true;
}

//
bool cPlayback::SetSubPid(short pid)
{
	printf("cPlayback::SetSubPid: curpid:%d nextpid:%d\n", mSubStream, pid);
	
	int track = pid;

	if(pid != mSubStream)
	{
		if(player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&track);

		mSubStream = pid;
		mExtSubStream = -1;
	}	

	return true;
}

bool cPlayback::SetExtSubPid(short pid)
{
	printf("cPlayback::SetExtSubPid: curpid:%d nextpid:%d\n", mSubStream, pid);
	
	int track = pid;

	if(pid != mExtSubStream)
	{
		if(player && player->playback)
		{
			player->playback->Command(player, PLAYBACK_SWITCH_EXTSUBTITLE, (void*)&track);
			player->playback->Command(player, PLAYBACK_PLAY_SUB, NULL);
		}

		mExtSubStream = pid;
		mSubStream = -1;
	}	

	return true;
}

bool cPlayback::SetSpeed(int speed)
{  
	printf("cPlayback::SetSpeed: speed %d\n", speed);	

	if(playing == false) 
		return false;

	int speedmap = 0;
	
	if(player && player->playback) 
	{
		if(speed > 1) 		//forwarding
		{
			//
			if (speed > 7) 
				speed = 7;
			
			switch(speed)
			{
				case 2: speedmap = 3; break;
				case 3: speedmap = 7; break;
				case 4: speedmap = 15; break;
				case 5: speedmap = 31; break;
				case 6: speedmap = 63; break;
				case 7: speedmap = 127; break;
			}

			player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speedmap);
		}
		else if(speed == 0)	//pausing
		{
			player->playback->Command(player, PLAYBACK_PAUSE, NULL);
		}
		else if (speed < 0)	//backwarding
		{
			//
			if (speed > -1) 
				speed = -1;
			
			if (speed < -7) 
				speed = -7;
			
			switch(speed)
			{
				case -1: speedmap = -5; break;
				case -2: speedmap = -10; break;
				case -3: speedmap = -20; break;
				case -4: speedmap = -40; break;
				case -5: speedmap = -80; break;
				case -6: speedmap = -160; break;
				case -7: speedmap = -320; break;
			}
			
			player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speedmap);

			// trickseek
			//player->playback->Command(player, PLAYBACK_SEEK, (void*)&speedmap);
		}
		else if(speed == 1) 	//continue
		{
			player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
		}
	}

	mSpeed = speed;

	return true;
}

bool cPlayback::SetSlow(int slow)
{  
	printf("cPlayback::SetSlow: (playing %d)\n", playing);	

	if(playing == false) 
		return false;

	if(player && player->playback) 
	{
		player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&slow);
	}

	mSpeed = slow;

	return true;
}

void cPlayback::GetPts(uint64_t &pts)
{
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, (void *)&pts);
}

bool cPlayback::GetSpeed(int &speed) const
{
	speed = mSpeed;

	return true;
}

// in milliseconds
bool cPlayback::GetPosition(int &position, int &duration)
{
	if(playing == false) 
		return false;

	if (player && player->playback && !player->playback->isPlaying) 
	{	  
		printf("cPlayback::%s !!!!EOF!!!! < -1\n", __func__);
		
		playing = false;
	
		return false;
	} 

	// position
	unsigned long long int vpts = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	position = vpts/90;
	
//	printf("%s: position: %d ms ", __FUNCTION__, position);
	
	// duration
	double length = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);
	
	if(length < 0) 
		length = 0;

	duration = (int)(length*1000);
	
//	printf("(duration: %d ms)\n", duration);
	
	return true;
}

bool cPlayback::SetPosition(int position)
{
	if(playing == false) 
		return false;
	
	printf("cPlayback::SetPosition: position: %d msec\n", position);
	
	float pos = (position/1000); // in sec

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);

	return true;
}

void cPlayback::FindAllPids(uint16_t *apids, bool *ac3flags, uint16_t *numpida, std::string *language)
{ 
	printf("cPlayback::FindAllPids\n");
	
	*ac3flags = false;

	char ** TrackList = NULL;
	
	// audio pids
	if(player && player->manager && player->manager->audio) 
	{
		player->manager->audio->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0,j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;
				
				//
				if(!strncmp("A_AC3", TrackList[i + 1], 5))
					ac3flags[j] = true;
				
				//
				language[j] = "Stream";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i+1]);
			}
			free(TrackList);
			*numpida = j;
		}
	}
}

// subs pids
void cPlayback::FindAllSubPids(uint16_t *apids, uint16_t *numpida, std::string *language)
{
	printf("cPlayback::FindAllSubPids:\n");

	char ** TrackList = NULL;
	
	if(player && player->manager && player->manager->subtitle) 
	{
		player->manager->subtitle->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0, j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;

				language[j] = "Sub";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i + 1]);
			}

			free(TrackList);
			*numpida = j;
		}
	}
}

// subs pids
void cPlayback::FindAllExtSubPids(uint16_t *apids, uint16_t *numpida, std::string *language)
{
	printf("cPlayback::FindAllExtSubPids:\n");

	char ** TrackList = NULL;
	
	if(player && player->manager && player->manager->extsubtitle) 
	{
		player->manager->extsubtitle->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0, j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;

				language[j] = "Sub";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i + 1]);
			}

			free(TrackList);
			*numpida = j;
		}
	}
}

//
void cPlayback::AddSubtitleFile(const char* const file)
{
	printf("cPlayback::AddSubtitleFile: %s\n", file? file : "null");
	
	if (file != NULL)
	{
		player->playback->Command(player, PLAYBACK_OPEN_SUB, (char *)file);
	}
}

////
#ifdef HAVE_NO_AV_DECODER
cPlayback::SWFramebuffer* cPlayback::getDecBuf(void)
{
	buf_m.lock();
	
	if (buf_num == 0)
	{
		buf_m.unlock();
		return NULL;
	}
								
	SWFramebuffer *p = &buffers[buf_out];
	
	p->resize(8294400); //FIXME:free(): invalid next size
	p->width(data[buf_out].width);
	p->height(data[buf_out].height);
	p->rate(data[buf_out].rate);
	p->vpts(data[buf_out].vpts);
	p->apts(sCURRENT_APTS);
	
	av_image_fill_arrays(&data[buf_out].buffer, &data[buf_out].size, &(*p)[0], AV_PIX_FMT_RGB32, data[buf_out].width, data[buf_out].height, 1);
	
	buf_out++;
	buf_num--;
	buf_out %= 64;
	
	buf_m.unlock();

	return p;
}
#endif

