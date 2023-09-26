/*
	$Id: audio_cs.cpp 2016.06.22 11:57:30 mohousch Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>

#include <config.h>

#include <linux/dvb/audio.h>

#include "audio_cs.h"

#include <system/debug.h>


#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#endif


static const char * FILENAME = "[audio_cs.cpp]";

////
cAudio::cAudio(int num)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	Muted = false;
	
	audio_fd = -1;	
	audio_adapter = 0;
	audio_num = num;
		
	StreamType = AUDIO_STREAMTYPE_MPEG;

	volume = 0;
	
	m_pcm_delay = -1,
	m_ac3_delay = -1;

#if not defined (__sh__) // dont reset volume on start
	setVolume(100, 100);
#endif
}

cAudio::~cAudio(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	Close();
}

bool cAudio::Open(CFrontend * fe)
{ 
	if(fe)
		audio_adapter = fe->feadapter;
	
	char devname[32];

	// Open audio device	
	sprintf(devname, "/dev/dvb/adapter%d/audio%d", audio_adapter, audio_num);
	
	if(audio_fd > 0)
	{
		printf("%s %s already opened\n", __FUNCTION__, devname);
		return true;
	}

	audio_fd = ::open(devname, O_RDWR);

	if(audio_fd > 0)
	{
		dprintf(DEBUG_NORMAL, "cAudio::Open %s\n", devname);
		
		return true;
	}	

	return false;
}

bool cAudio::Close()
{ 
	if (audio_fd < 0)
		return false;
	  
	dprintf(DEBUG_NORMAL, "%s:%s\n", FILENAME, __FUNCTION__);	

	if (audio_fd >= 0)
	{
		::close(audio_fd);
		audio_fd = -1;	
	}
	
	return true;
}

// shut up
int cAudio::SetMute(int enable)
{ 
	dprintf(DEBUG_NORMAL, "%s:%s (%d)\n", FILENAME, __FUNCTION__, enable);	
	
	Muted = enable?true:false;
	
	int ret = 0;	
	
#if !defined (__sh__)
	if (audio_fd > 0)
	{
		ret = ::ioctl(audio_fd, AUDIO_SET_MUTE, enable);
	
		if(ret < 0)
			perror("AUDIO_SET_MUTE"); 
	}
#endif

#if !defined (USE_OPENGL)
	char sMuted[4];
	sprintf(sMuted, "%d", Muted);

	int fd = ::open("/proc/stb/audio/j1_mute", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, sMuted, strlen(sMuted));
		::close(fd);
	}
#endif

	return ret;
}

/* volume, min = 0, max = 100 */
int cAudio::setVolume(unsigned int left, unsigned int right)
{ 
	dprintf(DEBUG_INFO, "%s:%s volume: %d\n", FILENAME, __FUNCTION__, left);
	
	int ret = -1;
	
	volume = (left + right)/2;
	
	// map volume
	if (volume < 0)
		volume = 0;
	else if (volume > 100)
		volume = 100;

	// convert to -1dB steps
	int _left = 63 - volume * 0.63;
	int _right = 63 - volume * 0.63;
	//now range is 63..0, where 0 is loudest
	
#if !defined (__sh__)
	audio_mixer_t mixer;

	mixer.volume_left = _left;
	mixer.volume_right = _right;
	
	if (audio_fd > 0)
	{
		ret = ::ioctl(audio_fd, AUDIO_SET_MIXER, &mixer);
	
		if(ret < 0)
			perror("AUDIO_SET_MIXER");
	}
#endif	

#if !defined (USE_OPENGL) && !defined (PLATFORM_HYPERCUBE)
	char sVolume[4];
	
#if defined (__sh__)
	sprintf(sVolume, "%d", _left);
#else
	sprintf(sVolume, "%d", volume);
#endif

	int fd = ::open("/proc/stb/avs/0/volume", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, sVolume, strlen(sVolume));
		::close(fd);
	}
#endif
	
	return ret;
}

/* start audio */
int cAudio::Start(void)
{ 
	if (audio_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	int ret = -1;
	
	ret = ::ioctl(audio_fd, AUDIO_PLAY);
	
	if(ret < 0)
		perror("AUDIO_PLAY");	

	return ret;
}

int cAudio::Stop(void)
{ 
	if (audio_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
	
	int ret = -1;
		
	ret = ::ioctl(audio_fd, AUDIO_STOP);
	
	if(ret < 0)
		perror("AUDIO_STOP");	

	return ret;
}

bool cAudio::Pause()
{ 
	if (audio_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	if (::ioctl(audio_fd, AUDIO_PAUSE) < 0)
	{
		perror("AUDIO_PAUSE");
		return false;
	}	

	return true;
}

bool cAudio::Resume()
{ 
	if (audio_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	if(::ioctl(audio_fd, AUDIO_CONTINUE) < 0)
	{
		perror("AUDIO_CONTINUE");
		return false;
	}	
	
	return true;
}

void cAudio::SetStreamType(AUDIO_FORMAT type)
{
	if (audio_fd < 0)
		return;
	
	const char* aAUDIOFORMAT[] = {
		"AUDIO_STREAMTYPE_AC3",
		"AUDIO_STREAMTYPE_MPEG",
		"AUDIO_STREAMTYPE_DTS"
		"AUDIO_STREAMTYPE_AAC",
		"AUDIO_STREAMTYPE_AACPLUS",
		"AUDIO_STREAMTYPE_LPCMDVD",
		"AUDIO_STREAMTYPE_MP3",
		"AUDIO_STREAMTYPE_DTSHD",
		"AUDIO_STREAMTYPE_EAC3",
	};

	dprintf(DEBUG_INFO, "%s:%s - type=%s\n", FILENAME, __FUNCTION__, aAUDIOFORMAT[type]);
	
	if (::ioctl(audio_fd, AUDIO_SET_BYPASS_MODE, type) < 0)
	{
		perror("AUDIO_SET_BYPASS_MODE");
		return;
	}	

	StreamType = type;
}

void cAudio::SetSyncMode(int Mode)
{
	if (audio_fd < 0)
		return;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
	
	if (::ioctl(audio_fd, AUDIO_SET_AV_SYNC, Mode) < 0)
	{
		perror("AUDIO_SET_AV_SYNC");
		return;
	}	
}

int cAudio::Flush(void)
{  
	if (audio_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	int ret = -1;

#if defined (__sh__)
	ret = (::ioctl(audio_fd, AUDIO_FLUSH) < 0);
#else
	ret = ::ioctl(audio_fd, AUDIO_CLEAR_BUFFER);
#endif

	if(ret < 0)
		perror("AUDIO_FLUSH");	
	
	return ret;
}

/* select channels */
int cAudio::setChannel(int channel)
{
	const char * aAUDIOCHANNEL[] = {
		"STEREO",
		"MONOLEFT",
		"MONORIGHT",
	};
	 
	dprintf(DEBUG_INFO, "%s:%s %s\n", FILENAME, __FUNCTION__, aAUDIOCHANNEL[channel]);

	if (audio_fd < 0)
		return -1;
	
	int ret = -1;

	ret = ::ioctl(audio_fd, AUDIO_CHANNEL_SELECT, (audio_channel_select_t)channel);
		perror("AUDIO_CHANNEL_SELECT");	
	
	return ret;
}

void cAudio::SetHdmiDD(int ac3)
{
	const char *aHDMIDD[] = {
		"downmix",
		"passthrough"
	};
	
	dprintf(DEBUG_NORMAL, "%s:%s %s\n", FILENAME, __FUNCTION__, aHDMIDD[ac3]);	

#if defined (__sh__)
	const char *aHDMIDDSOURCE[] = {
		"pcm",
		"spdif",
		"8ch",
		"none"
	};
	
	int fd = ::open("/proc/stb/hdmi/audio_source", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aHDMIDDSOURCE[ac3], strlen(aHDMIDDSOURCE[ac3]));
		::close(fd);
	}
#endif

#if !defined (USE_OPENGL)
	int fd_ac3 = ::open("/proc/stb/audio/ac3", O_RDWR);
	
	if(fd_ac3 > 0)
	{
		write(fd_ac3, aHDMIDD[ac3], strlen(aHDMIDD[ac3]));
		::close(fd_ac3);
	}
#endif	
}

/* set source */
int cAudio::setSource(int source)
{ 
	if (audio_fd < 0)
		return -1;
	
	const char *aAUDIOSTREAMSOURCE[] = {
		"AUDIO_SOURCE_DEMUX",
		"AUDIO_SOURCE_MEMORY",
		"AUDIO_SOURCE_HDMI"
	};
		
	dprintf(DEBUG_INFO, "%s:%s - source=%s\n", FILENAME, __FUNCTION__, aAUDIOSTREAMSOURCE[source]);
	
	int ret = -1;

	ret = ::ioctl(audio_fd, AUDIO_SELECT_SOURCE, source);
	
	return ret;
}

int cAudio::setHwPCMDelay(int delay)
{  
	dprintf(DEBUG_INFO, "%s:%s - delay=%d\n", FILENAME, __FUNCTION__, delay);
	
#if !defined (USE_OPENGL)	
	if (delay != m_pcm_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_pcm", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_pcm_delay = delay;
			return 0;
		}
	}
#endif	
	
	return -1;
}

int cAudio::setHwAC3Delay(int delay)
{
	dprintf(DEBUG_INFO, "%s:%s - delay=%d\n", FILENAME, __FUNCTION__, delay);
	
#if !defined (USE_OPENGL)	
	if ( delay != m_ac3_delay )
	{
		FILE *fp = fopen("/proc/stb/audio/audio_delay_bitstream", "w");
		if (fp)
		{
			fprintf(fp, "%x", delay*90);
			fclose(fp);
			m_ac3_delay = delay;
			return 0;
		}
	}
#endif	
	
	return -1;
}

