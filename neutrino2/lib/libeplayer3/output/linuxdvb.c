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

#include "common.h"
#include "output.h"

#include "writer.h"
#include "misc.h"
#include "pes.h"


/* ***************************** */
/* Makros/Constants              */
/* ***************************** */
#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#else
#ifndef AUDIO_GET_PTS
#define AUDIO_GET_PTS              _IOR('o', 19, __u64)
#endif
#endif

//#define LINUXDVB_DEBUG

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

unsigned long long int sCURRENT_PTS = 0;


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
	
	return cERR_LINUXDVB_NO_ERROR;
}

// close
int LinuxDvbClose(Context_t  *context, char * type) 
{
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

	/* closing stand alone is not allowed, so prevent
	* user from closing and dont call stop. stop will
	* set default values for us (speed and so on).
	*/
	LinuxDvbStop(context, type);

	getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	
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
	
	if (audio && audiofd != -1) 
	{
		char * Encoding = NULL;
		context->manager->audio->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(20, "0 A %s\n", Encoding);

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

	return ret;
}

// stop
int LinuxDvbStop(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	return ret;
}

// pause
int LinuxDvbPause(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	return ret;
}

// continue
int LinuxDvbContinue(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);
	
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

	linuxdvb_printf(10, "exiting\n");

	return ret;
}

// reverse discontinuity
int LinuxDvbReverseDiscontinuity(Context_t  *context, int* surplus) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	
#if 0
	int dis_type = VIDEO_DISCONTINUITY_CONTINUOUS_REVERSE | *surplus;
    
	linuxdvb_printf(50, "\n");
	
	if (ioctl( videofd, VIDEO_DISCONTINUITY, (void*) dis_type) == -1)
	{
		linuxdvb_err("ioctl failed with errno %d\n", errno);
		linuxdvb_err("VIDEO_DISCONTINUITY: %s\n", strerror(errno));
	}
#endif	

	linuxdvb_printf(50, "exiting\n");

	return ret;
}

// audio mute
int LinuxDvbAudioMute(Context_t  *context, char *flag) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	linuxdvb_printf(10, "\n");

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

	linuxdvb_printf(10, "exiting\n");

	return ret;
}

// flush
int LinuxDvbFlush(Context_t  *context, char * type) 
{
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	linuxdvb_printf(10, "exiting\n");

	return cERR_LINUXDVB_NO_ERROR;
}

// fastforward
int LinuxDvbFastForward(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d speed %d\n", video, audio, context->playback->Speed);

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

	linuxdvb_printf(10, "exiting with value %d\n", ret);

	return ret;
}

// reverse
int LinuxDvbReverse(Context_t  *context, char * type) 
{
#if 0
	int ret = cERR_LINUXDVB_NO_ERROR;
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

		/*
		if (ioctl(audiofd, AUDIO_SET_SPEED, speed) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("AUDIO_SET_SPEED: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
		*/

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}
    
	if (video && videofd != -1) 
	{
		getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

		/*
		if (ioctl(videofd, VIDEO_SET_SPEED, speed) == -1)
		{
			linuxdvb_err("ioctl failed with errno %d\n", errno);
			linuxdvb_err("VIDEO_SET_SPEED: %s\n", strerror(errno));
			ret = cERR_LINUXDVB_ERROR;
		}
		*/

		releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
	}

	linuxdvb_printf(10, "exiting with value %d\n", ret);

	return ret;
#endif
}

// slowmotion
int LinuxDvbSlowMotion(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;

	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	linuxdvb_printf(10, "exiting with value %d\n", ret);

	return ret;
}

int LinuxDvbAVSync(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	/* konfetti: this one is dedicated to audiofd so we
	* are ignoring what is given by type! I think we should
	* remove this param. Therefor we should add a variable
	* setOn or something like that instead, this would remove
	* using a variable inside the structure.
	*/
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

	return ret;
}

// clear
int LinuxDvbClear(Context_t  *context, char * type) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	unsigned char video = !strcmp("video", type);
	unsigned char audio = !strcmp("audio", type);

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	linuxdvb_printf(10, "exiting\n");

	return ret;
}

// pts
int LinuxDvbPts(Context_t  *context, unsigned long long int* pts) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
    
	linuxdvb_printf(50, "\n");

	// pts is a non writting requests and can be done in parallel to other requests
	//getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

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

	*((unsigned long long int *)pts)=(unsigned long long int)sCURRENT_PTS;

	//releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);

	return ret;
}

// framecount
int LinuxDvbGetFrameCount(Context_t  *context, unsigned long long int* frameCount) 
{
	int ret = cERR_LINUXDVB_NO_ERROR;
	
#if 0
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

	return ret;
}

// switch
int LinuxDvbSwitch(Context_t  *context, char * type) 
{
	unsigned char audio = !strcmp("audio", type);
	unsigned char video = !strcmp("video", type);
	Writer_t * writer = NULL;

	linuxdvb_printf(10, "v%d a%d\n", video, audio);

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

	linuxdvb_printf(10, "exiting\n");

	return cERR_LINUXDVB_NO_ERROR;
}

// Write to decoder
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
			//call.WriteV         = writev_with_retry;
#if defined __sh__
			call.WriteV	  = writev;
#else
			call.WriteV       = writev_with_retry;
#endif			

			if (writer->writeData)
				res = writer->writeData(&call);

			if (res <= 0)
			{
				linuxdvb_err("failed to write data %d - %d\n", res, errno);
				linuxdvb_err("%s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		}

		free(Encoding);
	}
	else if (video) 
	{
		char * Encoding = NULL;
		context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(20, "Encoding = %s\n", Encoding);

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
				linuxdvb_err("failed to write data %d - %d\n", res, errno);
				linuxdvb_err("%s\n", strerror(errno));
				ret = cERR_LINUXDVB_ERROR;
			}
		}

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
		{	// 4
			sCURRENT_PTS = 0;
			ret = LinuxDvbPlay(context, (char*)argument);
			break;
		}
		
		case OUTPUT_STOP: 
		{
			reset(context);
			ret = LinuxDvbStop(context, (char*)argument);
			//reset(context);
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

