/*
 * LinuxDVB Output handling.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* ***************************** */
/* Includes                      */
/* ***************************** */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>

#include <config.h>

#include "common.h"
#include "output.h"

#include "writer.h"
#include "misc.h"
#include "pes.h"

#if defined (__sh__)
#include <linux/dvb/stm_ioctls.h>
#endif

//// opengl
#ifdef USE_OPENGL
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <ao/ao.h>
#endif


/* ***************************** */
/* Makros/Constants              */
/* ***************************** */
#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#else
#ifndef AUDIO_GET_PTS
#define AUDIO_GET_PTS              	_IOR('o', 19, __u64)
#endif
#endif

//#define LINUXDVB_DEBUG
#define LINUXDVB_SILENT

static short debug_level = 10;

static const char FILENAME[] = __FILE__;

#ifdef LINUXDVB_DEBUG
#define linuxdvb_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x ); } while (0)
#else
#define linuxdvb_printf(x...)
#endif

#ifndef LINUXDVB_SILENT
#define linuxdvb_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define linuxdvb_err(x...)
#endif

#define cERR_LINUXDVB_NO_ERROR      0
#define cERR_LINUXDVB_ERROR        -1

static const char VIDEODEV[] 	= "/dev/dvb/adapter0/video0";
static const char AUDIODEV[] 	= "/dev/dvb/adapter0/audio0";

static int videofd 	= -1;
static int audiofd 	= -1;

uint64_t sCURRENT_PTS = 0;

#ifdef USE_OPENGL
static ao_device *adevice = NULL;
static ao_sample_format sformat;
int buf_num = 0;
int buf_in = 0;
int buf_out = 0;
bool stillpicture = false;
Data_t data[64];
uint64_t sCURRENT_APTS = 0;
#endif

//
pthread_mutex_t LinuxDVBmutex;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */
int LinuxDvbStop(Context_t  *context, char * type);

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

void getLinuxDVBMutex(const char *filename, const char *function, int line) 
{
	linuxdvb_printf(250, "requesting mutex\n");

	pthread_mutex_lock(&LinuxDVBmutex);

	linuxdvb_printf(250, "received mutex\n");
}

void releaseLinuxDVBMutex(const char *filename, const char *function, int line) 
{
	pthread_mutex_unlock(&LinuxDVBmutex);

	linuxdvb_printf(250, "released mutex\n");
}

// open
int LinuxDvbOpen(Context_t  *context, char * type) 
{
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);
	
#ifndef USE_OPENGL
	if (audio && audiofd == -1) 
	{
		audiofd = open(AUDIODEV, O_RDWR);

		if (audiofd <= 0)
		{
			// fallback
			sleep(1);
			
			audiofd = open(AUDIODEV, O_RDWR);
	    
			if (audiofd <= 0)
			{
				linuxdvb_err("failed to open %s - errno %d\n", AUDIODEV, errno);
				linuxdvb_err("%s\n", strerror(errno));
			}

			if (videofd != -1)
				close(videofd);
			
			return cERR_LINUXDVB_ERROR;
		}

#if defined (__sh__)
		if (ioctl( audiofd, AUDIO_FLUSH) == -1)
#else
		if (ioctl( audiofd, AUDIO_CLEAR_BUFFER, NULL) == -1)
#endif
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_CLEAR_BUFFER: %s\n", strerror(errno));
		}

		if (ioctl( audiofd, AUDIO_SELECT_SOURCE, (void*)AUDIO_SOURCE_MEMORY) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_SELECT_SOURCE: %s\n", strerror(errno));
		}		
	}

	if (video && videofd == -1) 
	{
		videofd = open(VIDEODEV, O_RDWR);

		if (videofd <= 0)
		{
			// fallback
			sleep(1);
	     
			videofd = open(VIDEODEV, O_RDWR);
	    
			if (videofd <= 0)
			{
				linuxdvb_err("failed to open %s - errno %d\n", VIDEODEV, errno);
				linuxdvb_err("%s\n", strerror(errno));
				return cERR_LINUXDVB_ERROR;
			}
		}

#if defined (__sh__)
		if (ioctl( videofd, VIDEO_FLUSH) == -1)
#else
		if (ioctl( videofd, VIDEO_CLEAR_BUFFER, NULL) == -1)
#endif
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_CLEAR_BUFFER: %s\n", strerror(errno));
		}

		if (ioctl( videofd, VIDEO_SELECT_SOURCE, (void*)VIDEO_SOURCE_MEMORY) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_SELECT_SOURCE: %s\n", strerror(errno));
		}        
	}
#endif
	
	return cERR_LINUXDVB_NO_ERROR;
}

// close
int LinuxDvbClose(Context_t  *context, char * type) 
{
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

	LinuxDvbStop(context, type);

	getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	
#ifdef USE_OPENGL
	if (adevice)
		ao_close(adevice);
		
	adevice = NULL;
	
#else	
	if (audio && audiofd != -1) 
	{
		close(audiofd);
		audiofd = -1;
	}

	if (video && videofd != -1) 
	{
		close(videofd);
		videofd = -1;
	}
#endif

	releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	
	return cERR_LINUXDVB_NO_ERROR;
}

// play
int LinuxDvbPlay(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	Writer_t * writer = NULL;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);
	
#ifndef USE_OPENGL
	if (audio && audiofd != -1) 
	{
		char * Encoding = NULL;
		context->manager->audio->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(10, "0 A %s\n", Encoding);

		writer = getWriter(Encoding);

		if (writer == NULL)
		{
			linuxdvb_err("cannot found writer for encoding %s using default\n", Encoding);
			
			if (ioctl( audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT)AUDIO_STREAMTYPE_MPEG) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("AUDIO_SET_ENCODING: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		} 
		else
		{
			linuxdvb_printf(20, "found writer %s for encoding %s\n", writer->caps->name, Encoding);
			
			if (ioctl( audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT) writer->caps->dvbEncoding) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("AUDIO_SET_ENCODING: %s\n", strerror(errno));
				ret = -1;
			}		
		}

		if (ioctl(audiofd, AUDIO_PLAY, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_PLAY: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		free(Encoding);
	}

	if (video && videofd != -1) 
	{
		char * Encoding = NULL;
		context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(10, "V %s\n", Encoding);

		writer = getWriter(Encoding);

		if (writer == NULL)
		{
			linuxdvb_err("cannot found writer for encoding %s using default\n", Encoding);

			if (ioctl( videofd, VIDEO_SET_STREAMTYPE, (VIDEO_FORMAT) VIDEO_STREAMTYPE_MPEG2 ) == -1)			  
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_SET_ENCODING: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		} 
		else
		{
			linuxdvb_printf(20, "found writer %s for encoding %s\n", writer->caps->name, Encoding);
			
			if (ioctl( videofd, VIDEO_SET_STREAMTYPE, (VIDEO_FORMAT) writer->caps->dvbEncoding) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_SET_ENCODING: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
			
		}

		if (ioctl(videofd, VIDEO_PLAY, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_PLAY: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		free(Encoding);
	}
#endif

	return ret;
}

// stop
int LinuxDvbStop(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	
	if (audio && audiofd != -1) 
	{
#if defined (__sh__)
		if (ioctl( audiofd, AUDIO_FLUSH) == -1)
#else
		if (ioctl(audiofd, AUDIO_CLEAR_BUFFER, NULL) == -1)
#endif
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_CLEAR_BUFFER: %s\n", strerror(errno));
		}		

		if (ioctl(audiofd, AUDIO_STOP, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_STOP: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}

	if (video && videofd != -1) 
	{
#if defined (__sh__)
		if (ioctl(videofd, VIDEO_FLUSH) == -1)
#else
		if (ioctl(videofd, VIDEO_CLEAR_BUFFER, NULL) == -1)
#endif
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_CLEAR_BUFFER: %s\n", strerror(errno));
		}		

		if (ioctl(videofd, VIDEO_STOP, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_STOP: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}

	releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
#endif

	return ret;
}

// pause
int LinuxDvbPause(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	
	if (audio && audiofd != -1) 
	{
		if (ioctl(audiofd, AUDIO_PAUSE, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_PAUSE: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}

	if (video && videofd != -1) 
	{
		if (ioctl(videofd, VIDEO_FREEZE, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_FREEZE: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}

	releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
#endif

	return ret;
}

// continue
int LinuxDvbContinue(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);
	
#ifndef USE_OPENGL
	if (audio && audiofd != -1) 
	{
		if (ioctl(audiofd, AUDIO_CONTINUE, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_CONTINUE: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}

	if (video && videofd != -1) 
	{
		if (ioctl(videofd, VIDEO_CONTINUE, NULL) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_CONTINUE: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}
#endif

	return ret;
}

// reverse discontinuity
int LinuxDvbReverseDiscontinuity(Context_t  *context, int* surplus) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

#ifndef USE_OPENGL	
#if defined (__sh__)
	int dis_type = VIDEO_DISCONTINUITY_CONTINUOUS_REVERSE | *surplus;
    
	linuxdvb_printf(50, "\n");
	
	if (ioctl( videofd, VIDEO_DISCONTINUITY, (void*) dis_type) == -1)
	{
		linuxdvb_err("ioctl failed with errno %d\n", errno);
		linuxdvb_err("VIDEO_DISCONTINUITY: %s\n", strerror(errno));
	}
#endif	
#endif

	return ret;
}

// audio mute
int LinuxDvbAudioMute(Context_t* context, char* flag) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	linuxdvb_printf(10, "\n");

#ifndef USE_OPENGL
	if (audiofd != -1) 
	{
		if(*flag == '1')
		{
			//AUDIO_SET_MUTE has no effect with new player
#if defined (__sh__)
			if (ioctl(audiofd, AUDIO_STOP, NULL) == -1)
#else
			if (ioctl(audiofd, AUDIO_SET_MUTE, 1) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
#if defined (__sh__)
				linuxdvb_err("AUDIO_STOP: %s\n", strerror(errno));
#else
				linuxdvb_err("AUDIO_SET_MUTE: %s\n", strerror(errno));
#endif
				ret = cERR_LINUXDVB_ERROR;
			}
		}
		else
		{
			//AUDIO_SET_MUTE has no effect with new player
#if defined (__sh__)
			if (ioctl(audiofd, AUDIO_PLAY, NULL) == -1)
#else
			if (ioctl(audiofd, AUDIO_SET_MUTE, 0) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
#if defined (__sh__)
				linuxdvb_err("AUDIO_PLAY: %s\n", strerror(errno));
#else
				linuxdvb_err("AUDIO_SET_MUTE: %s\n", strerror(errno));
#endif
				ret = cERR_LINUXDVB_ERROR;
			}
		}
	}
#endif

	return ret;
}

// flush
int LinuxDvbFlush(Context_t  *context, char * type) 
{
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	if ( (video && videofd != -1) || (audio && audiofd != -1) ) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		if (video && videofd != -1) 
		{
#if defined (__sh__)
			if (ioctl(videofd, VIDEO_FLUSH) == -1)
#else
			if (ioctl(videofd, VIDEO_CLEAR_BUFFER,NULL) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_FLUSH: %s\n", strerror(errno));
			}

			if (ioctl(videofd, VIDEO_STOP, NULL) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_STOP: %s\n", strerror(errno));
			}			
		}

		if (audio && audiofd != -1) 
		{
#if defined (__sh__)
			if (ioctl( audiofd, AUDIO_FLUSH) == -1)
#else
			if (ioctl(audiofd, AUDIO_CLEAR_BUFFER ,NULL) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("AUDIO_FLUSH: %s\n", strerror(errno));
			}
						
			if (ioctl(audiofd, AUDIO_STOP, NULL) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("AUDIO_STOP: %s\n", strerror(errno));
			}
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return cERR_LINUXDVB_NO_ERROR;
}

// fastforward
int LinuxDvbFastForward(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d speed %d\n", video, audio, context->playback->Speed);

#ifndef USE_OPENGL
	if (video && videofd != -1) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		/* konfetti comment: speed is a value given in skipped frames */
		if (ioctl(videofd, VIDEO_FAST_FORWARD, context->playback->Speed) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_FAST_FORWARD: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return ret;
}

// reverse
int LinuxDvbReverse(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	
#if defined (__sh__)
	int speed;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

	if (context->playback->Speed >= 0)
	{
		linuxdvb_err("error speed is greater 0, but should be a neg value in skipped frames (or zero)\n");
		return cERR_LINUXDVB_ERROR;
	}

	/* speed == 0 indicates end of trick mode, otherwise negative value of skipped frames
	* multiplicated with DVB_SPEED_NORMAL_PLAY (currently 1000)
	*/
	speed = (context->playback->Speed == 0) ? DVB_SPEED_REVERSE_STOPPED : context->playback->Speed * DVB_SPEED_NORMAL_PLAY;

	linuxdvb_printf(10, "speed %d - %d\n", speed, context->playback->Speed);
	
	if (audio && audiofd != -1) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		//
		if (ioctl(audiofd, AUDIO_SET_SPEED, speed) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_SET_SPEED: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
    
	if (video && videofd != -1) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		//
		if (ioctl(videofd, VIDEO_SET_SPEED, speed) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_SET_SPEED: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return ret;
}

// slowmotion
int LinuxDvbSlowMotion(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	if ( (video && videofd != -1) || (audio && audiofd != -1) ) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		if (video && videofd != -1) 
		{
			if (ioctl(videofd, VIDEO_SLOWMOTION, context->playback->SlowMotion) == -1)
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_SLOWMOTION: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return ret;
}

int LinuxDvbAVSync(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	
#ifndef USE_OPENGL
	if (audiofd != -1) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		if (ioctl(audiofd, AUDIO_SET_AV_SYNC, (void *)context->playback->AVSync) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_SET_AV_SYNC: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return ret;
}

// clear
int LinuxDvbClear(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	if ( (video && videofd != -1) || (audio && audiofd != -1) ) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		if (video && videofd != -1) 
		{
#if defined (__sh__)
			if (ioctl(videofd, VIDEO_FLUSH) == -1)
#else
			if (ioctl(videofd, VIDEO_CLEAR_BUFFER, NULL) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("VIDEO_CLEAR_BUFFER: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		}
		
		if (audio && audiofd != -1) 
		{
#if defined (__sh__)
			if (ioctl( audiofd, AUDIO_FLUSH) == -1)
#else
			if (ioctl(audiofd, AUDIO_CLEAR_BUFFER, NULL) == -1)
#endif
			{
				linuxdvb_err("ioctl failed with errno %d\n", errno);
				linuxdvb_err("AUDIO_CLEAR_BUFFER: %s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return ret;
}

// pts
int LinuxDvbPts(Context_t  *context, unsigned long long int* pts) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
    
	linuxdvb_printf(50, "\n");

#ifndef USE_OPENGL
	if (videofd != -1)
	{
		if (ioctl(videofd, VIDEO_GET_PTS, (void*)&sCURRENT_PTS) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_GET_PTS: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}
	else if (audiofd != -1)
	{
		if (ioctl(audiofd, AUDIO_GET_PTS, (void*)&sCURRENT_PTS) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_GET_PTS: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
	}
	else 
	{
		sCURRENT_PTS = 0;
		ret = cERR_LINUXDVB_ERROR;
	}
#endif

	*((unsigned long long int *)pts) = (unsigned long long int)sCURRENT_PTS;

	return ret;
}

// framecount
int LinuxDvbGetFrameCount(Context_t  *context, unsigned long long int* frameCount) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

#ifndef USE_OPENGL	
#if defined (__sh__)
	dvb_play_info_t playInfo;  //???

	linuxdvb_printf(50, "\n");

	getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

	if (videofd != -1)
	{
		if (ioctl(videofd, VIDEO_GET_PLAY_INFO, (void*)&playInfo) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_GET_PLAY_INFO: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
		else 
			linuxdvb_err("V: %ull\n", playInfo.frame_count);
	}
	else if (audiofd != -1)
	{
		if (ioctl(audiofd, AUDIO_GET_PLAY_INFO, (void*)&playInfo) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_GET_PLAY_INFO: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
		else 
			linuxdvb_err("A: %ull\n", playInfo.frame_count);
	}
	else 
	{
		ret = cERR_LINUXDVB_ERROR;
	}

	if(ret == cERR_LINUXDVB_NO_ERROR)
		*((unsigned long long int *)frameCount) = playInfo.frame_count;

	releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
#endif
#endif	

	return ret;
}

// switch
int LinuxDvbSwitch(Context_t  *context, char * type) 
{
	unsigned char audio = !strcmp("audio", type);
	unsigned char video = !strcmp("video", type);
	Writer_t * writer = NULL;

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

#ifndef USE_OPENGL
	if ( (video && videofd != -1) || (audio && audiofd != -1) ) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		if (audio && audiofd != -1) 
		{
			char * Encoding = NULL;
			if (context && context->manager && context->manager->audio) 
			{
				context->manager->audio->Command(context, MANAGER_GETENCODING, &Encoding);

				linuxdvb_printf(10, "A %s\n", Encoding);

				writer = getWriter(Encoding);

				if (ioctl(audiofd, AUDIO_STOP ,NULL) == -1)
				{
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("AUDIO_STOP: %s\n", strerror(errno));
				}
				
#if defined (__sh__)
				if (ioctl( audiofd, AUDIO_FLUSH) == -1)
#else
				if (ioctl(audiofd, AUDIO_CLEAR_BUFFER ,NULL) == -1)
#endif
				{
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("AUDIO_CLEAR_BUFFER: %s\n", strerror(errno));
				}
				
				if (writer == NULL)
				{
					linuxdvb_err("cannot found writer for encoding %s using default\n", Encoding);
					
					if (ioctl( audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT) AUDIO_STREAMTYPE_MPEG) == -1)
					{
						linuxdvb_err("ioctl failed with errno %d\n", errno);
						linuxdvb_err("AUDIO_SET_ENCODING: %s\n", strerror(errno));
					}
				} 
				else
				{
					linuxdvb_printf(10, "found writer %s for encoding %s\n", writer->caps->name, Encoding);
					
					if (ioctl( audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT) writer->caps->dvbEncoding) == -1)
					{
						linuxdvb_err("ioctl failed with errno %d\n", errno);
						linuxdvb_err("AUDIO_SET_ENCODING: %s\n", strerror(errno));
					}				
				}

				if (ioctl(audiofd, AUDIO_PLAY, NULL) == -1)
				{
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("AUDIO_PLAY: %s\n", strerror(errno));
				}

				free(Encoding);
			}
			else
				linuxdvb_printf(20, "no context for Audio\n");
		}

		if (video && videofd != -1) 
		{
			char * Encoding = NULL;
			if (context && context->manager && context->manager->video) 
			{
				context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

				if (ioctl(videofd, VIDEO_STOP ,NULL) == -1)
				{
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("VIDEO_STOP: %s\n", strerror(errno));
				}

#if defined (__sh__)
				if (ioctl(videofd, VIDEO_FLUSH) == -1)
#else
				if (ioctl(videofd, VIDEO_CLEAR_BUFFER , NULL) == -1)
#endif
				{
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("VIDEO_CLEAR_BUFFER: %s\n", strerror(errno));
				}

				linuxdvb_printf(10, "V %s\n", Encoding);

				writer = getWriter(Encoding);

				if (writer == NULL)
				{
					linuxdvb_err("cannot found writer for encoding %s using default\n", Encoding);
					
					if (ioctl( videofd, VIDEO_SET_STREAMTYPE, (VIDEO_FORMAT)VIDEO_STREAMTYPE_MPEG2 ) == -1)
					{
						linuxdvb_err("ioctl failed with errno %d\n", errno);
						linuxdvb_err("VIDEO_SET_ENCODING: %s\n", strerror(errno));
					}
				} 
				else
				{
					linuxdvb_printf(10, "found writer %s for encoding %s\n", writer->caps->name, Encoding);

					if (ioctl( videofd, VIDEO_SET_STREAMTYPE, (VIDEO_FORMAT) writer->caps->dvbEncoding) == -1)
					{
						linuxdvb_err("ioctl failed with errno %d\n", errno);
						linuxdvb_err("VIDEO_SET_ENCODING: %s\n", strerror(errno));
					}					
				}

				if (ioctl(videofd, VIDEO_PLAY, NULL) == -1)
				{
					/* konfetti: fixme: think on this, I think we should
					* return an error here and stop the playback mode
					*/
					linuxdvb_err("ioctl failed with errno %d\n", errno);
					linuxdvb_err("VIDEO_PLAY: %s\n", strerror(errno));
				}

				free(Encoding);
			}
			else
				linuxdvb_printf(20, "no context for Video\n");
		}

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
#endif

	return cERR_LINUXDVB_NO_ERROR;
}

//// Write to decoder
static int Write(void* _context, void* _out)
{
	Context_t          *context  = (Context_t  *) _context;
	AudioVideoOut_t    *out      = (AudioVideoOut_t*) _out;
	int                ret       = cERR_LINUXDVB_NO_ERROR;
	int                res       = 0;
	unsigned char      video     = 0;
	unsigned char      audio     = 0;
	Writer_t *         writer = NULL;
	WriterAVCallData_t call;

	if (out == NULL)
	{
		linuxdvb_err("null pointer passed\n");
		return cERR_LINUXDVB_ERROR;
	}
    
	video = !strcmp("video", out->type);
	audio = !strcmp("audio", out->type);
  
	linuxdvb_printf(20, "DataLength=%u PrivateLength=%u Pts=%llu FrameRate=%f\n", out->len, out->extralen, out->pts, out->frameRate);
	linuxdvb_printf(20, "v%d a%d\n", video, audio);

	if (audio) 
	{
		char * Encoding = NULL;
		context->manager->audio->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(20, "%s::%s Encoding = %s\n", FILENAME, __FUNCTION__, Encoding);

#ifndef USE_OPENGL
		writer = getWriter(Encoding);

		if (writer == NULL)
		{
			linuxdvb_printf(20, "searching default writer ... %s\n", Encoding);
			writer = getDefaultAudioWriter();
		}

		if (writer == NULL)
		{
			linuxdvb_err("unknown audio codec %s and no default writer\n",Encoding);
			ret = cERR_LINUXDVB_ERROR;
		} 
		else
		{
			call.fd             = audiofd;
			call.data           = out->data;
			call.len            = out->len;
			call.Pts            = out->pts;
			call.private_data   = out->extradata;
			call.private_size   = out->extralen;
			call.FrameRate      = out->frameRate;
			call.FrameScale     = out->timeScale;
			call.Version        = 0; /* -1; unsigned char cannot be negative */
#if defined __sh__
			call.WriteV	  = writev;
#else
			call.WriteV       = writev_with_retry;
#endif			

			if (writer->writeData)
				res = writer->writeData(&call);
				
			if (res <= 0)
			{
				ret = cERR_LINUXDVB_ERROR;
			}
		}
#else
		AVCodecContext* ctx = out->stream->codec;
		int got_frame = 0;
		// resample
		SwrContext *swr = NULL;
		uint8_t *obuf = NULL;
		int obuf_size = 0; 	// in samples
		int obuf_size_max = 0;
		int o_ch, o_sr; 	// output channels and sample rate
		uint64_t o_layout; 	// output channels layout
		int driver;
		ao_info *ai;
		
		//
		AVPacket avpkt;
		av_init_packet(&avpkt);
		
		avpkt.data = out->data;
    		avpkt.size = out->len;
    		avpkt.pts  = out->pts;
		
		// output sample rate, channels, layout could be set here if necessary
		o_ch = ctx->channels;     		// 2
		o_sr = ctx->sample_rate;      		// 48000
		o_layout = ctx->channel_layout;   	// AV_CH_LAYOUT_STEREO
	
		if (sformat.channels != o_ch || sformat.rate != o_sr || sformat.byte_format != AO_FMT_NATIVE || sformat.bits != 16 || adevice == NULL)
		{
			driver = ao_default_driver_id();
			sformat.bits = 16;
			sformat.channels = ctx->channels;
			sformat.rate = ctx->sample_rate;
			sformat.byte_format = AO_FMT_NATIVE;
			sformat.matrix = 0;
				
			adevice = ao_open_live(driver, &sformat, NULL);
			ai = ao_driver_info(driver);
		}

		//
		swr = swr_alloc_set_opts(swr, o_layout, AV_SAMPLE_FMT_S16, o_sr, ctx->channel_layout, ctx->sample_fmt, ctx->sample_rate, 0, NULL);
	        
		if (!swr)
		{
			return cERR_LINUXDVB_ERROR;
		}
	
		swr_init(swr);
						
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		res = avcodec_decode_audio4(ctx, out->aframe, &got_frame, &avpkt);
#else
		res = avcodec_send_packet(ctx, &avpkt);
		
		if (res != 0 && res != AVERROR(EAGAIN))
		{
			linuxdvb_printf(200, "%s: avcodec_send_packet %d\n", __func__, res);
		}
		else
		{
			res = avcodec_receive_frame(ctx, out->aframe);
							
			if (res != 0 && res != AVERROR(EAGAIN))
			{
				linuxdvb_printf(200,"%s: avcodec_send_packet %d\n", __func__, res);
			}
			else
			{
				got_frame = 1;
			}
		}
#endif

		if (got_frame)
		{
			int out_linesize;
			
			//
			obuf_size = av_rescale_rnd(out->aframe->nb_samples, ctx->sample_rate, ctx->sample_rate, AV_ROUND_UP);

			if (obuf_size > obuf_size_max)
			{
				av_free(obuf);
								
				if (av_samples_alloc(&obuf, &out_linesize, ctx->channels, out->aframe->nb_samples, AV_SAMPLE_FMT_S16, 1) < 0)
				{
					av_packet_unref(&avpkt);
					ret = cERR_LINUXDVB_ERROR;
				}
								
				obuf_size_max = obuf_size;
			}
							
			obuf_size = swr_convert(swr, &obuf, obuf_size, (const uint8_t **)out->aframe->extended_data, out->aframe->nb_samples);
							
#if (LIBAVUTIL_VERSION_MAJOR < 54)
			data[buf_in].apts = sCURRENT_PTS = av_frame_get_best_effort_timestamp(out->aframe);
#else
			data[buf_in].apts = sCURRENT_PTS = out->aframe->best_effort_timestamp;
#endif
			int o_buf_size = av_samples_get_buffer_size(&out_linesize, out->stream->codec->channels, obuf_size, AV_SAMPLE_FMT_S16, 1);
							
			if (o_buf_size > 0)
				res = ao_play(adevice, (char *)obuf, o_buf_size);
				
			if (res <= 0)
			{
				ret = cERR_LINUXDVB_ERROR;
			}
		}
		
		//
		av_packet_unref(&avpkt);

		av_free(obuf);
		swr_free(&swr);
		
		ret = cERR_LINUXDVB_ERROR;
#endif

		free(Encoding);
	}
	else if (video) 
	{
		char * Encoding = NULL;
		context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(20, "%s::%s Encoding = %s\n", FILENAME, __FUNCTION__, Encoding);

#ifndef USE_OPENGL
		writer = getWriter(Encoding);

		if (writer == NULL)
		{
			linuxdvb_printf(20, "searching default writer ... %s\n", Encoding);
			writer = getDefaultVideoWriter();
		}

		if (writer == NULL)
		{
			linuxdvb_err("unknown video codec and no default writer %s\n",Encoding);
			ret = cERR_LINUXDVB_ERROR;
		} 
		else
		{
			call.fd           = videofd;
			call.data         = out->data;
			call.len          = out->len;
			call.Pts          = out->pts;
			call.private_data = out->extradata;
			call.private_size = out->extralen;
			call.FrameRate    = out->frameRate;
			call.FrameScale   = out->timeScale;
			call.Width        = out->width;
			call.Height       = out->height;
			call.Version      = 0; // is unsingned char
#if defined __sh__
			call.WriteV	  = writev;
#else
			call.WriteV       = writev_with_retry;
#endif

			if (writer->writeData)
				res = writer->writeData(&call);

			if (res <= 0)
			{
				ret = cERR_LINUXDVB_ERROR;
			}
		}
#else
		struct SwsContext *convert = NULL;
		AVCodecContext* ctx = out->stream->codec;
		
		//
		AVPacket avpkt;
		av_init_packet(&avpkt);
		
		avpkt.data = out->data;
    		avpkt.size = out->len;
    		avpkt.pts  = out->pts;
		
		//
		int got_frame = 0;
		
		if (out->frame)
			av_frame_unref(out->frame);
			
		if (out->rgbframe)
			av_frame_unref(out->rgbframe);
	
		// decode frame
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		res = avcodec_decode_video2(ctx, out->frame, &got_frame, &avpkt);
		
		if (res < 0)
		{
			linuxdvb_printf(200, "%s: avcodec_decode_video2 %d\n", __func__, res);
			
			av_packet_unref(&avpkt);
		}
#else
		res = avcodec_send_packet(ctx, &avpkt);
		
		if (res != 0 && res != AVERROR(EAGAIN))
		{
			linuxdvb_printf(200, "%s: avcodec_send_packet %d\n", __func__, res);
			
			av_packet_unref(&avpkt);
		}
		else
		{
			res = avcodec_receive_frame(ctx, out->frame);
							
			if (res != 0 && res != AVERROR(EAGAIN))
			{
				linuxdvb_printf(200,"%s: avcodec_send_packet %d\n", __func__, res);
			}
			else
			{
				got_frame = 1;
			}
		}
#endif
					
		// setup swsscaler
		if (got_frame && !stillpicture)
		{
			int need = av_image_get_buffer_size(AV_PIX_FMT_RGB32, ctx->width, ctx->height, 1);
							
			convert = sws_getCachedContext(convert, ctx->width, ctx->height, ctx->pix_fmt, ctx->width, ctx->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
								
			if (convert)
			{
				//
				getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
				
				// fill				
				av_image_fill_arrays(out->rgbframe->data, out->rgbframe->linesize, data[buf_in].buffer, AV_PIX_FMT_RGB32, ctx->width, ctx->height, 1);

				// scale
				sws_scale(convert, out->frame->data, out->frame->linesize, 0, ctx->height, out->rgbframe->data, out->rgbframe->linesize);
				
				// fill our struct	
				data[buf_in].width = ctx->width;
				data[buf_in].height = ctx->height;
				
				//
#if (LIBAVUTIL_VERSION_MAJOR < 54)
				data[buf_in].vpts = sCURRENT_PTS = av_frame_get_best_effort_timestamp(out->frame);
#else
				data[buf_in].vpts = sCURRENT_PTS = out->frame->best_effort_timestamp;
#endif

				// a/v delay determined experimentally :-)
				if (ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
					data[buf_in].vpts += 90000 * 4 / 10; // 400ms
				else
					data[buf_in].vpts += 90000 * 3 / 10; // 300ms
				
				//
				int framerate = ctx->time_base.den / (ctx->time_base.num * ctx->ticks_per_frame);
				
				switch (framerate)
				{
					case 23://23.976fps
						data[buf_in].rate = 0;
						break;
					case 24:
						data[buf_in].rate = 1;
						break;
					case 25:
						data[buf_in].rate = 2;
						break;
					case 29://29,976fps
						data[buf_in].rate = 3;
						break;
					case 30:
						data[buf_in].rate = 4;
						break;
					case 50:
						data[buf_in].rate = 5;
						break;
					case 60:
						data[buf_in].rate = 6;
						break;
					default:
						data[buf_in].rate = framerate;
						break;
				}
				
				//
				data[buf_in].size = need;

				//
				buf_in++;
				buf_in %= 64;
				buf_num++;
				
				if (buf_num > (64 - 1))
				{
					buf_out++;
					buf_out %= 64;
					buf_num--;
				}
				
				releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
			}
		}
		
		////
		av_packet_unref(&avpkt);
		
		if (convert)
		{
			sws_freeContext(convert);
			convert = NULL;
		}
		
		if (out->frame)
			av_frame_unref(out->frame);
			
		if (out->rgbframe)
			av_frame_unref(out->rgbframe);
		
		ret = cERR_LINUXDVB_ERROR;
#endif

		free(Encoding);
	} 

	return ret;
}

static int reset(Context_t  *context)
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	Writer_t*   writer = NULL;
	char * Encoding = NULL;

	// video writer
	context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

	writer = getWriter(Encoding);

	if (writer == NULL)
	{
		linuxdvb_err("unknown video codec %s\n",Encoding);
		ret = cERR_LINUXDVB_ERROR;
	} 
	else
	{
		writer->reset();
	}

	free(Encoding);

	// audio writer
	context->manager->audio->Command(context, MANAGER_GETENCODING, &Encoding);

	writer = getWriter(Encoding);

	if (writer == NULL)
	{
		linuxdvb_err("unknown audio codec %s\n",Encoding);
		ret = cERR_LINUXDVB_ERROR;
	} 
	else
	{
		writer->reset();
	}

	free(Encoding);

	return ret;
}

static int Command(void  *_context, OutputCmd_t command, void * argument) 
{
	Context_t* context = (Context_t*) _context;
	int ret = cERR_LINUXDVB_NO_ERROR;
	
	linuxdvb_printf(50, "Command %d\n", command);

	switch(command) 
	{
		case OUTPUT_OPEN: 
		{
			ret = LinuxDvbOpen(context, (char*)argument);
			break;
		}
		
		case OUTPUT_CLOSE: 
		{
			ret = LinuxDvbClose(context, (char*)argument);
			reset(context);
			sCURRENT_PTS = 0;
			break;
		}
		
		case OUTPUT_PLAY: 
		{
			sCURRENT_PTS = 0;
			ret = LinuxDvbPlay(context, (char*)argument);
			break;
		}
		
		case OUTPUT_STOP: 
		{
			reset(context);
			ret = LinuxDvbStop(context, (char*)argument);
			sCURRENT_PTS = 0;
			break;
		}
		
		case OUTPUT_FLUSH: 
		{
			ret = LinuxDvbFlush(context, (char*)argument);
			reset(context);
			sCURRENT_PTS = 0;
			break;
		}
		
		case OUTPUT_PAUSE: 
		{
			ret = LinuxDvbPause(context, (char*)argument);
			break;
		}
		
		case OUTPUT_CONTINUE: 
		{
			ret = LinuxDvbContinue(context, (char*)argument);
			break;
		}
		
		case OUTPUT_FASTFORWARD: 
		{
			return LinuxDvbFastForward(context, (char*)argument);
			break;
		}
		
		case OUTPUT_REVERSE: 
		{
			return LinuxDvbReverse(context, (char*)argument);
			break;
		}
		
		case OUTPUT_AVSYNC: 
		{
			ret = LinuxDvbAVSync(context, (char*)argument);
			break;
		}
		
		case OUTPUT_CLEAR: 
		{
			ret = LinuxDvbClear(context, (char*)argument);
			break;
		}
		
		case OUTPUT_PTS: 
		{
			unsigned long long int pts = 0;
			ret = LinuxDvbPts(context, &pts);
			*((unsigned long long int*)argument) = (unsigned long long int)pts;
			break;
		}
		
		case OUTPUT_SWITCH: 
		{
			ret = LinuxDvbSwitch(context, (char*)argument);
			break;
		}
		
		case OUTPUT_SLOWMOTION: 
		{
			return LinuxDvbSlowMotion(context, (char*)argument);
			break;
		}
		
		case OUTPUT_AUDIOMUTE: 
		{
			return LinuxDvbAudioMute(context, (char*)argument);
			break;
		}
		
		case OUTPUT_DISCONTINUITY_REVERSE: 
		{
			return LinuxDvbReverseDiscontinuity(context, (int*)argument);
			break;
		}
		
		case OUTPUT_GET_FRAME_COUNT: 
		{
			unsigned long long int frameCount = 0;
			ret = LinuxDvbGetFrameCount(context, &frameCount);
			*((unsigned long long int*)argument) = (unsigned long long int)frameCount;
			break;
		}
		
		default:
			linuxdvb_err("ContainerCmd %d not supported!\n", command);
			ret = cERR_LINUXDVB_ERROR;
			break;
	}

	linuxdvb_printf(50, "exiting with value %d\n", ret);

	return ret;
}

static char *LinuxDvbCapabilities[] = { "audio", "video", NULL };

struct Output_s LinuxDvbOutput = {
	"LinuxDvb",
	&Command,
	&Write,
	LinuxDvbCapabilities
};

