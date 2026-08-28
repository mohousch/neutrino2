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

#if defined (__sh__)
#include <linux/dvb/stm_ioctls.h>
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

#ifdef HAVE_NO_AV_DECODER
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>

extern int buf_num;
extern int buf_in;
extern int buf_out;
bool stillpicture = false;
Data_t data[64] = {0};
uint64_t sCURRENT_APTS = 0;

#ifdef USE_LIBAO
#include <ao/ao.h>

static ao_device *adevice = NULL;
static ao_sample_format sformat;
#endif

#ifdef USE_LIBDRM
#include <libdrm/drm_fourcc.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sys/mman.h>

#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>

#include <gbm.h>

extern int drm_fd;
extern uint8_t *fb_ptr;
extern struct drm_mode_create_dumb creq;
extern uint32_t crtc_id;
extern uint32_t conn_id;
extern uint32_t ov_id;
extern int scr_w;
extern int scr_h;

uint32_t last_fb=0;
AVFrame *nv12= NULL;

int gbm_fd = -1;
struct gbm_device *gbm = NULL;
#endif

#ifdef USE_DIRECTFB
#include <directfb.h>

//extern IDirectFB *dfb;
//extern IDirectFBSurface *primary;
//extern IDirectFBDisplayLayer *layer;
extern IDirectFBSurface *video_surf;
#endif
#endif // HAVE_NO_AV_DECODER

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
	
#ifndef HAVE_NO_AV_DECODER
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
#else
#ifdef USE_LIBDRM
//	AVBufferRef *hw_dev=NULL;
//  	av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_DRM, "/dev/dri/card0", NULL, 0);
    	// fallback to VAAPI if no DRM hwaccel (Intel)
//    	if(!hw_dev) av_hwdevice_ctx_create(&hw_dev, AV_HWDEVICE_TYPE_VAAPI, "/dev/dri/renderD128", NULL, 0);
    	
    	/////
//    	if (drm_fd < 0)
//    	{
//    	drm_fd = open("/dev/dri/card1", O_RDWR);
//    	if(drm_fd<0) drm_fd=open("/dev/dri/card0", O_RDWR);
//    	drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES,1);

    	// find overlay plane
    	ov_id = 0;
    	drmModePlaneRes *pr = drmModeGetPlaneResources(drm_fd);
    	
    	for(uint32_t i = 0; i < pr->count_planes; i++)
    	{
        	drmModePlane *pl = drmModeGetPlane(drm_fd, pr->planes[i]);
        	
        	if(pl->possible_crtcs & (1<<0))
        	{ 
        		ov_id = pl->plane_id; 
        		drmModeFreePlane(pl); 
        		break; 
        	}
        	
        	drmModeFreePlane(pl);
    	}
    	
    	// init gbm
    	if (gbm_fd < 0)
    	{
		gbm_fd = open("/dev/dri/renderD128", O_RDWR); // no permission issue
		gbm = gbm_create_device(gbm_fd);
		
		if (gbm == NULL)
		{
			linuxdvb_printf(10, "failed to create gbm device\n");
		}
		else
			linuxdvb_printf(10, "gbm device created:%p\n", gbm);
	}
#endif
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

#ifdef HAVE_NO_AV_DECODER	
#ifdef USE_LIBAO
	if (adevice)
		ao_close(adevice);
		
	adevice = NULL;
#endif
	
#ifdef USE_LIBDRM
	close(gbm_fd);
	gbm_fd = -1;
#endif	
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
	
#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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
	
#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER	
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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
	
#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER
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

#ifndef HAVE_NO_AV_DECODER	
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

#ifndef HAVE_NO_AV_DECODER
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
					
					if (ioctl(audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT) AUDIO_STREAMTYPE_MPEG) == -1)
					{
						linuxdvb_err("ioctl failed with errno %d\n", errno);
						linuxdvb_err("AUDIO_SET_ENCODING: %s\n", strerror(errno));
					}
				} 
				else
				{
					linuxdvb_printf(10, "found writer %s for encoding %s\n", writer->caps->name, Encoding);
					
					if (ioctl(audiofd, AUDIO_SET_BYPASS_MODE, (AUDIO_FORMAT) writer->caps->dvbEncoding) == -1)
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

					if (ioctl(videofd, VIDEO_SET_STREAMTYPE, (VIDEO_FORMAT) writer->caps->dvbEncoding) == -1)
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

#ifndef HAVE_NO_AV_DECODER
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
		int got_frame = 0;
		SwrContext *swr = NULL;
		uint8_t *obuf = NULL;
		int obuf_size = 0; 				// in samples
		int obuf_size_max = 0;
		int o_ch = 2;
		int o_sr = 48000; 				// output channels and sample rate
		uint64_t o_layout = AV_CH_LAYOUT_STEREO; 	// output channels layout
		
#ifdef USE_LIBAO
		int driver = -1;
		ao_info *ai = NULL;
#endif
		
		//
		AVPacket avpkt;
		av_init_packet(&avpkt);
		
		avpkt.data = out->data;
    		avpkt.size = out->len;
    		avpkt.pts  = out->pts;
		
		// output sample rate, channels, layout could be set here if necessary
		o_ch = out->ctx->channels;     			// 2
		o_sr = out->ctx->sample_rate;      		// 48000
		o_layout = out->ctx->channel_layout;   		// AV_CH_LAYOUT_STEREO
	
#ifdef USE_LIBAO
		if (sformat.channels != o_ch || sformat.rate != o_sr || sformat.byte_format != AO_FMT_NATIVE || sformat.bits != 16)
		{
			sformat.bits = 16;
			sformat.channels = out->ctx->channels;
			sformat.rate = out->ctx->sample_rate;
			sformat.byte_format = AO_FMT_NATIVE;
			sformat.matrix = 0;
			
			if (adevice == NULL)
			{
				driver = ao_default_driver_id();	
				adevice = ao_open_live(driver, &sformat, NULL);
				ai = ao_driver_info(driver);
			}
		}
#endif

		//
		swr = swr_alloc_set_opts(swr, o_layout, AV_SAMPLE_FMT_S16, o_sr, out->ctx->channel_layout, out->ctx->sample_fmt, out->ctx->sample_rate, 0, NULL);
	        
		if (!swr)
		{
			return cERR_LINUXDVB_ERROR;
		}
	
		swr_init(swr);
						
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		res = avcodec_decode_audio4(out->ctx, out->aframe, &got_frame, &avpkt);
#else
		res = avcodec_send_packet(out->ctx, &avpkt);
		
		if (res != 0 && res != AVERROR(EAGAIN))
		{
			linuxdvb_printf(200, "%s: avcodec_send_packet %d\n", __func__, res);
		}
		else
		{
			res = avcodec_receive_frame(out->ctx, out->aframe);
							
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
			obuf_size = av_rescale_rnd(out->aframe->nb_samples, out->ctx->sample_rate, out->ctx->sample_rate, AV_ROUND_UP);

			if (obuf_size > obuf_size_max)
			{
				av_free(obuf);
								
				if (av_samples_alloc(&obuf, &out_linesize, out->ctx->channels, out->aframe->nb_samples, AV_SAMPLE_FMT_S16, 1) < 0)
				{
					av_packet_unref(&avpkt);
					ret = cERR_LINUXDVB_ERROR;
				}
								
				obuf_size_max = obuf_size;
			}
							
			obuf_size = swr_convert(swr, &obuf, obuf_size, (const uint8_t **)out->aframe->extended_data, out->aframe->nb_samples);
							
#if (LIBAVUTIL_VERSION_MAJOR < 54)
			sCURRENT_APTS = sCURRENT_PTS = av_frame_get_best_effort_timestamp(out->aframe);
#else
			sCURRENT_APTS = sCURRENT_PTS = out->aframe->best_effort_timestamp;
#endif

			int o_buf_size = av_samples_get_buffer_size(&out_linesize, out->stream->codecpar->channels, obuf_size, AV_SAMPLE_FMT_S16, 1);
			
			// play
#ifdef USE_LIBAO				
			if (o_buf_size > 0)
				res = ao_play(adevice, (char *)obuf, o_buf_size);
#endif
				
			if (res <= 0)
			{
				ret = cERR_LINUXDVB_ERROR;
			}
		}
		
		//
		av_packet_unref(&avpkt);

		av_free(obuf);
		swr_free(&swr);
		////
		if (out->aframe)
			av_frame_unref(out->aframe);
		
		ret = cERR_LINUXDVB_ERROR;
#endif

		free(Encoding);
	}
	else if (video) 
	{
		char * Encoding = NULL;
		context->manager->video->Command(context, MANAGER_GETENCODING, &Encoding);

		linuxdvb_printf(20, "%s::%s Encoding = %s\n", FILENAME, __FUNCTION__, Encoding);

#ifndef HAVE_NO_AV_DECODER
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
		
		//
		memset(&data[64], 0, sizeof(data[64]));
		
		//
		AVPacket avpkt;
		av_init_packet(&avpkt);
		
		avpkt.data = out->data;
    		avpkt.size = out->len;
    		avpkt.pts  = out->pts;
		
		//
		int got_frame = 0;
		
		if (out->vframe)
			av_frame_unref(out->vframe);
	
		// decode frame
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		res = avcodec_decode_video2(out->ctx, out->vframe, &got_frame, &avpkt);
		
		if (res < 0)
		{
			linuxdvb_printf(200, "%s: avcodec_decode_video2 %d\n", __func__, res);
			
			av_packet_unref(&avpkt);
		}
#else
		res = avcodec_send_packet(out->ctx, &avpkt);
		
		if (res != 0 && res != AVERROR(EAGAIN))
		{
			linuxdvb_printf(200, "%s: avcodec_send_packet %d\n", __func__, res);
			
			av_packet_unref(&avpkt);
		}
		else
		{
			res = avcodec_receive_frame(out->ctx, out->vframe);
							
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
		if (got_frame)
		{
			getLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
			
#ifdef USE_OPENGL				
			convert = sws_getCachedContext(convert, out->ctx->width, out->ctx->height, out->ctx->pix_fmt, out->ctx->width, out->ctx->height, AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
								
			if (convert)
			{
				int need = av_image_get_buffer_size(AV_PIX_FMT_BGRA, out->ctx->width, out->ctx->height, 1);

				if (data[buf_in].size < need)
					data[buf_in].size = need;
					
				// swsscale YUV420 to RGB32:
				uint8_t *dest[4] = { data[buf_in].buffer, NULL, NULL, NULL };
	    			int dest_linesize[4] = { out->ctx->width*4, 0, 0, 0 };
	    			
				sws_scale(convert, out->vframe->data, out->vframe->linesize, 0, out->ctx->height, dest, dest_linesize);
					
				//
				data[buf_in].width = out->ctx->width;
				data[buf_in].height = out->ctx->height;
					
				//
#if (LIBAVUTIL_VERSION_MAJOR < 54)
				data[buf_in].vpts = sCURRENT_PTS = av_frame_get_best_effort_timestamp(out->vframe);
#else
				data[buf_in].vpts = sCURRENT_PTS = out->vframe->best_effort_timestamp;
#endif
					
				//
				if (out->ctx->time_base.num && out->ctx->ticks_per_frame)
					data[buf_in].rate = out->ctx->time_base.den / (out->ctx->time_base.num * out->ctx->ticks_per_frame);

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
			}
#endif
#if defined (USE_LIBDRM)
			////
			#if 0
			convert = sws_getCachedContext(convert, out->ctx->width, out->ctx->height, /*out->ctx->pix_fmt*/AV_PIX_FMT_YUV420P, out->ctx->width, out->ctx->height, AV_PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);
			
			if (convert)
			{
				// create scanout BO
				if (gbm)
				{
					struct gbm_bo *bo = gbm_bo_create(gbm, out->ctx->width, out->ctx->height, GBM_FORMAT_NV12, GBM_BO_USE_SCANOUT | GBM_BO_USE_LINEAR);
					
					uint32_t stride;
					void *map_data;
					
					uint8_t *mapped = gbm_bo_map(bo, 0, 0, out->ctx->width, out->ctx->height, GBM_BO_TRANSFER_WRITE, &stride, &map_data);
					
					// use sws into it
					uint8_t *dest[2] = { mapped, mapped + stride* out->ctx->height };
		    			int dest_linesize[2] = { (int)stride, (int)stride };

		    			sws_scale(convert, (const uint8_t* const*)out->vframe->data, out->vframe->linesize, 0, out->ctx->height, dest, dest_linesize);
		    			
		    			// export to PRIME -> DRM FB
		    			int dma_fd = gbm_bo_get_fd(bo);
		    			uint32_t handle;
		    			
		    			drmPrimeFDToHandle(drm_fd, dma_fd, &handle);
		    			
		    			uint32_t pitches[2] = {stride, stride};
		    			uint32_t offsets[2] = {0, stride*out->ctx->height};
		    			uint32_t fb;
		    			drmModeAddFB2(drm_fd, out->ctx->width, out->ctx->height, DRM_FORMAT_NV12, (uint32_t[]){handle, handle}, pitches, offsets, &fb, 0);
		    			drmModeSetPlane(drm_fd, ov_id, crtc_id, fb, 0, 0, 0, 1920, 1080, 0, 0, out->ctx->width<<16, out->ctx->height<<16);
		    			
		    			// cleanup.
		    			close(dma_fd);
		    			gbm_bo_destroy(bo);
            			}
			}
			#endif
			////
			#if 0
			////
			convert = sws_getCachedContext(convert, out->ctx->width, out->ctx->height, out->ctx->pix_fmt, out->ctx->width, out->ctx->height, AV_PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);
			
			if (convert)
			{
				// create DRM NV12 dumb
            			struct drm_mode_create_dumb cre={0}; 
            			cre.width=out->vframe->width; 
            			cre.height=out->vframe->height*3/2; 
            			cre.bpp=8;
            			
            			drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &cre);
            			struct drm_mode_map_dumb mp={0}; 
            			mp.handle=cre.handle;
            			drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mp);
            			uint8_t *fb_ptr = mmap(0, cre.size, PROT_READ|PROT_WRITE, MAP_SHARED, drm_fd, mp.offset);

            			// wrap ptr as AVFrame for sws
            			uint8_t *dest[4] = { fb_ptr, fb_ptr+out->ctx->width*out->ctx->height, NULL, NULL };
	    			int dest_linesize[4] = { out->ctx->width, out->ctx->height, 0, 0 };

            			sws_scale(convert, (const uint8_t* const*)out->vframe->data, out->vframe->linesize, 0, out->ctx->height, dest, dest_linesize);

            			uint32_t hdl[4]={cre.handle,cre.handle};
            			uint32_t pitch[4]={(uint32_t)cre.pitch,(uint32_t)cre.pitch};
            			uint32_t off[4]={0,(uint32_t)(out->ctx->width*out->ctx->height)};
            			
            			uint32_t fb; 
            			drmModeAddFB2(drm_fd, out->ctx->width, out->ctx->height, DRM_FORMAT_NV12, hdl,pitch,off,&fb,0);

            			drmModeSetPlane(drm_fd, ov_id, crtc_id, fb, 0, 0,0,scr_w,scr_h, 0,0,out->ctx->width<<16,out->ctx->height<<16);
            			
            			if(last_fb)
            			{ 
            				drmModeRmFB(drm_fd, last_fb); 
            			}
            			
            			// free old dumb
            			struct drm_mode_destroy_dumb des={0}; 
            			des.handle=cre.handle; // we keep handle? need keep before rmFB, leak for demo simplicity
            			// keep for 2 frames then unmap
            			munmap(fb_ptr, cre.size);
            			last_fb=fb;
            			//usleep(40000);
			}
			#endif
			////
			////
			#if 1
			// Prüfen, ob der Frame im DRM_PRIME Format vorliegt
                    	if (out->vframe->format == AV_PIX_FMT_DRM_PRIME) 
                    	{
                        	// Das 'data[0]' Array enthält bei DRM_PRIME die AVDRMFrameDescriptor Struktur
                        	AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)out->vframe->data[0];
                        
                        	linuxdvb_printf(10, "[DRM PRIME] Frame dekodiert! Layer-Anzahl: %d, Objekte (Fds): %d\n", desc->nb_layers, desc->nb_objects);

                        	// import dma-buf to GEM
                        	uint32_t handles[4]={0}, pitches[4]={0}, offsets[4]={0};
            			uint64_t mods[4]={0};
            			
            			 for(int i=0;i<desc->nb_layers;i++)
            			 {
            			 	AVDRMLayerDescriptor *layer = &desc->layers[i];
            			 	
		                	for(int j=0;j<layer->nb_planes;j++)
		                	{
		            			int idx = layer->planes[j].object_index;
		            			int fd_prime = desc->objects[idx].fd;
		            			uint32_t gem_handle;
		            			drmPrimeFDToHandle(drm_fd, fd_prime, &gem_handle);
		            			handles[j]=gem_handle;
		            			pitches[j]=layer->planes[j].pitch;
		            			offsets[j]=layer->planes[j].offset;
		            			mods[j]=desc->objects[idx].format_modifier;
		        		}
		        		uint32_t fb_id;
		        		// try with modifiers first (KODI does this)
		        		if(drmModeAddFB2WithModifiers(drm_fd, out->vframe->width, out->vframe->height, layer->format, handles, pitches, offsets, mods, &fb_id, DRM_MODE_FB_MODIFIERS)!=0)
		        		{
		            			drmModeAddFB2(drm_fd, out->vframe->width, out->vframe->height, layer->format, handles, pitches, offsets, &fb_id, 0);
		        		}
		        		
		        		if(last_fb) drmModeRmFB(drm_fd, last_fb);
		        		last_fb=fb_id;

		        		// show fullscreen, HW scaler does it
		        		drmModeSetPlane(drm_fd, ov_id, crtc_id, fb_id, 0,
		            			0,0, scr_w, scr_h,
		            			0,0, out->vframe->width<<16, out->vframe->height<<16);
		        		break; // only first layer for NV12
		        	}
		        	// simple sync - wait 1 vsync
            			drmVBlank vbl={0}; vbl.request.type=DRM_VBLANK_RELATIVE; vbl.request.sequence=1;
            			drmIoctl(drm_fd, DRM_IOCTL_WAIT_VBLANK, &vbl);
                    	} 
                    	else 
                    	{
                        	linuxdvb_printf(10, "[CPU] Frame im Software-Format (%d) dekodiert (Kein DRM_PRIME).\n", out->vframe->format);
                    	}
                    	////
                    	#endif
#elif defined (USE_DIRECTFB)
			convert = sws_getCachedContext(convert, out->ctx->width, out->ctx->height, out->ctx->pix_fmt, out->ctx->width, out->ctx->height, AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
								
			if (convert)
			{
				void *ptr; int pitch;
	   			video_surf->Lock(video_surf, DSLF_WRITE, &ptr, &pitch);

	   			uint8_t *dest[1] = {ptr}; 
	   			int dest_linesize[1] = {pitch};
	   			
	   			//
	   			sws_scale(convert, out->vframe->data, out->vframe->linesize, 0, out->ctx->height, dest, dest_linesize);

	   			video_surf->Unlock(video_surf);
	   			video_surf->Flip(video_surf, NULL, DSFLIP_WAITFORSYNC);
   			}
#endif
			releaseLinuxDVBMutex(FILENAME, __FUNCTION__,__LINE__);
		}
		
		//
		av_packet_unref(&avpkt);
		
		if (convert)
		{
			sws_freeContext(convert);
			convert = NULL;
		}
		
		if (out->vframe)
			av_frame_unref(out->vframe);
		
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

