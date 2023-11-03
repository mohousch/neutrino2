/*
 * 	video_cs.cpp
 *
 * 	$Id: video.cpp 2016.06.22 11:40:30 mohousch Exp $

 *
 * 	Copyright (C) 2011 duckbox project
 *
 *  	This program is free software; you can redistribute it and/or modify
 *  	it under the terms of the GNU General Public License as published by
 *  	the Free Software Foundation; either version 2 of the License, or
 *  	(at your option) any later version.
 *
 *  	This program is distributed in the hope that it will be useful,
 *  	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  	GNU General Public License for more details.
 *
 *  	You should have received a copy of the GNU General Public License
 *  	along with this program; if not, write to the Free Software
 *  	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <config.h>

#include "video_cs.h"

#include <linux/fb.h>

#include <driver/framebuffer.h>
#include <system/debug.h>


#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#endif

#ifdef USE_OPENGL
#include "dmx_cs.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/* ffmpeg buf 32k */
#define INBUF_SIZE 0x8000
/* my own buf 256k */
#define DMX_BUF_SZ 0x20000

#define VDEC_PIXFMT AV_PIX_FMT_RGB32

extern cVideo *videoDecoder;
extern cDemux *videoDemux;
//
static uint8_t *dmxbuf;
static int bufpos;
#endif

static const char * FILENAME = "[video_cs.cpp]";

////
cVideo::cVideo(int num)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	video_fd = -1;
	video_adapter = 0;
	video_num = num;

	playstate = VIDEO_STOPPED;
	
	StreamType = VIDEO_STREAMTYPE_MPEG2;
	
#ifdef USE_OPENGL
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif
	dmxbuf = (uint8_t *)malloc(DMX_BUF_SZ);
	bufpos = 0;
	thread_running = false;
	buf_num = 0;
	buf_in = 0;
	buf_out = 0;
	dec_w = 0;
	dec_h = 0;
	stillpicture = false;
	w_h_changed = false;
#endif
}

cVideo::~cVideo(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	Close();
}

bool cVideo::Open(CFrontend * fe)
{ 
	if(fe)
		video_adapter = fe->feadapter;
	
	char devname[32];

	// Open video device
	sprintf(devname, "/dev/dvb/adapter%d/video%d", video_adapter, video_num);
	
	if(video_fd > 0)
	{
		printf("%s %s already opened\n", __FUNCTION__, devname);
		return true;
	}

	video_fd = ::open(devname, O_RDWR);

	if(video_fd > 0)
	{
		dprintf(DEBUG_NORMAL, "cVideo::Open %s\n", devname);
		return true;
	}
	
	dprintf(DEBUG_INFO, "%s %s failed\n", __FUNCTION__, devname);

	return false;
}

bool cVideo::Close()
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_NORMAL, "%s:%s\n", FILENAME, __FUNCTION__);	
	
	if(video_fd >= 0)
	{
		::close(video_fd);
		video_fd = -1;
	}	

	return true;
}

int cVideo::getAspectRatio(void) 
{  
	int ratio = ASPECTRATIO_43; // 0 = 4:3, 1 = 16:9

	dprintf(DEBUG_NORMAL, "%s:%s\n", FILENAME, __FUNCTION__);	
	 
#if !defined (USE_OPENGL)	 
	unsigned char buffer[2];
	int n, fd;

	fd = ::open("/proc/stb/vmpeg/0/aspect", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 2);
		::close(fd);
		
		if (n > 0) 
		{
			ratio = atoi((const char*) buffer);
		}
		
		char buf[100];
		
		switch (ratio) 
		{
			case ASPECTRATIO_43:
				sprintf ((char *) buf, "4:3");
				break;
			
			case ASPECTRATIO_169:
				sprintf ((char *) buf, "16:9");
				break;
				
			case ASPECTRATIO_AUTO:
				sprintf ((char *) buf, "auto");
				break;
				
			//case 3:
			//	sprintf ((char *) buf, "14:9");
			//	break;
		
			//case 4:
			//	sprintf ((char *) buf, "20:9");
			//	break;
				
			default:
				strncpy (buf, "unknow", sizeof (buf));
				break;
		}
	}
	
	dprintf(DEBUG_INFO, "%s:%s (ratio=%d)\n", FILENAME, __FUNCTION__, ratio);
#endif	
	
	return ratio;
}

/*
letterbox 
panscan 
non 
bestfit
*/
/* set aspect ratio */
int cVideo::setAspectRatio(int ratio, int format) 
{ 
	dprintf(DEBUG_NORMAL, "%s:%s\n", FILENAME, __FUNCTION__);	

	const char * sRatio[] =
	{
	   	"4:3",
	   	"16:9",
	   	"any" 
        }; 
	
#if defined (__sh__)
	const char* sFormat[]=
	{
		"letterbox",
		"panscan",
		"non",
		"bestfit" 
	};
#else
	const char* sFormat[]=
	{
		"letterbox",
		"panscan", 
		"bestfit",
		"nonlinear"
	};
#endif

#if !defined (USE_OPENGL)  
	int fd;

	// aspectratio	
        fd = ::open("/proc/stb/video/aspect", O_WRONLY);
	
	if(fd > 0)
	{
		write(fd, sRatio[ratio], strlen(sRatio[ratio]));
		::close(fd);
	}

	// policy
	fd = ::open("/proc/stb/video/policy", O_WRONLY);
	
	if(fd > 0)
	{
		write(fd, sFormat[format], strlen((const char*) sFormat[format]));
		::close(fd);
	}
	
	dprintf(DEBUG_INFO, "%s %s (aspect=%d format=%d) set %s %s\n", FILENAME, __FUNCTION__, ratio, format, sRatio[ratio], sFormat[format]);
#endif	

    	return 0; 
}

void cVideo::getPictureInfo(int &width, int &height, int &rate) 
{
	rate = 25;
	height = 576;
	width = 720;
#if !defined (USE_OPENGL)	  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__); 

  	unsigned char buffer[10];
	int n, fd;	

	// framerate
	fd = ::open("/proc/stb/vmpeg/0/framerate", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		::close(fd);

		if (n > 0) 
		{
#if defined (__sh__)
			sscanf((const char*) buffer, "%X", &rate);
#else
			sscanf((const char*) buffer, "%d", &rate);
#endif		
			rate = rate/1000;
		}
	}

	// width (xres)
	fd = ::open("/proc/stb/vmpeg/0/xres", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		::close(fd);

		if (n > 0) 
		{
			sscanf((const char*) buffer, "%X", &width);
		}
	}

	// height  (yres)
	fd = ::open("/proc/stb/vmpeg/0/yres", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		::close(fd);

		if (n > 0) 
		{
			sscanf((const char*) buffer, "%X", &height);
		}
	}
	
	dprintf(DEBUG_INFO, "%s:%s < w %d, h %d, r %d\n", FILENAME, __FUNCTION__, width, height, rate);
#endif	
}

int cVideo::Start()
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
#ifdef USE_OPENGL
	if (!thread_running)
		OpenThreads::Thread::start();
		
	return 0;
#else
	if(video_fd < 0)
		return false;

	if (playstate == VIDEO_PLAYING)
		return 0;

	playstate = VIDEO_PLAYING;
	
	// Video Play
	if(::ioctl(video_fd, VIDEO_PLAY) < 0)
		perror("VIDEO_PLAY");	
		
	return true;
#endif
}

int cVideo::Stop(bool blank)
{ 
	dprintf(DEBUG_INFO, "%s:%s blank:%d\n", FILENAME, __FUNCTION__, blank);	
	
#ifdef USE_OPENGL
	if (thread_running)
	{
		thread_running = false;
		OpenThreads::Thread::join();
	}
	
	return 0;
#else
	if(video_fd < 0)
		return false;
		
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
	
	if( ::ioctl(video_fd, VIDEO_STOP, blank ? 1 : 0) < 0 )  
		perror("VIDEO_STOP");	
	
	return true;
#endif
}

bool cVideo::Pause(void)
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
		
	if (::ioctl(video_fd, VIDEO_FREEZE) < 0)
		perror("VIDEO_FREEZE");
	
	playstate = VIDEO_FREEZED;	
		
	return true;
}

bool cVideo::Resume(void)
{
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
		
	if (::ioctl(video_fd, VIDEO_CONTINUE) < 0)
		perror("VIDEO_CONTINUE");
	
	playstate = VIDEO_PLAYING;	
		
	return true;
}

int cVideo::Flush(void)
{  
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	int ret = -1;

#if defined (__sh__)
	ret = ::ioctl(video_fd, VIDEO_FLUSH);
#else
	ret = ::ioctl(video_fd, VIDEO_CLEAR_BUFFER);
#endif

	if(ret < 0)
		perror("VIDEO_FLUSH");		
	
	return ret;
}

int cVideo::setSlowMotion(int repeat)
{
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "VIDEO_SLOWMOTION(%d) - \n", repeat);
	
	int ret = -1;
		
	ret = ::ioctl(video_fd, VIDEO_SLOWMOTION, repeat);
	if (ret < 0)
		perror("VIDEO_SLOWMOTION");	
	
	return ret;
}

int cVideo::setFastForward(int skip)
{
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "VIDEO_FAST_FORWARD(%d) - \n", skip);
	
	int ret = -1;
		
	ret = ::ioctl(video_fd, VIDEO_FAST_FORWARD, skip);
	if (ret < 0)
		perror("VIDEO_FAST_FORWARD");	

	return ret;
}

/* set video_system */
int cVideo::SetVideoSystem(int video_system)
{	
	
#if defined (__sh__)
/*
pal 
1080i50 
720p50 
576p50 
576i50 
1080i60 
720p60 
1080p24 
1080p25 
1080p30 
1080p50
PC
*/
	const char *aVideoSystems[][2] = {
		{"VIDEO_STD_PAL", "pal"},
		{"VIDEO_STD_1080I50", "1080i50"},
		{"VIDEO_STD_720P50", "720p50"},
		{"VIDEO_STD_576P", "576p50"},
		{"VIDEO_STD_576I", "576i50"},
		{"VIDEO_STD_1080I60", "1080i60"},
		{"VIDEO_STD_720P60", "720p60"},
		{"VIDEO_STD_1080P24", "1080p24"},
		{"VIDEO_STD_1080P25", "1080p25"},
		{"VIDEO_STD_1080P30", "1080p30"},
		{"VIDEO_STD_1080P50", "1080p50"},
		{"VIDEO_STD_PC", "PC"},
	};
#else
// giga
/*
pal 
ntsc 
480i 
576i 
480p 
576p 
720p50 
720p 
1080i50 
1080i
*/
	const char *aVideoSystems[][2] = {
		{"VIDEO_STD_PAL", "pal"},
		{"VIDEO_STD_NTSC", "ntsc"},
		{"VIDEO_STD_480I60", "480i"},
		{"VIDEO_STD_576I50", "576i"},
		{"VIDEO_STD_480P60", "480p"},
		{"VIDEO_STD_576P50", "576p"},
		{"VIDEO_STD_720P50", "720p50"},
		{"VIDEO_STD_720P60", "720p"},
		{"VIDEO_STD_1080P50", "1080i50"},
		{"VIDEO_STD_1080P60", "1080i"},
	};
#endif

	dprintf(DEBUG_INFO, "%s:%s - video_system=%s\n", FILENAME, __FUNCTION__, aVideoSystems[video_system][0]);	

#if !defined (USE_OPENGL)	
	int fd = ::open("/proc/stb/video/videomode", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aVideoSystems[video_system][1], strlen(aVideoSystems[video_system][1]));
		::close(fd);
	}
#endif	

	return 0;
}

/* set hdmi space colour */
int cVideo::SetSpaceColour(int colour_space)
{
#if defined (__sh__)
	const char *aCOLORSPACE[] = {
		"hdmi_rgb",
		"hdmi_yuv",
		"hdmi_422"
	};
#else
	const char *aCOLORSPACE[] = {
		"Edid(Auto)",
		"Hdmi_Rgb",
		"Itu_R_BT_709",
		"unknow"
	};
	
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aCOLORSPACE[colour_space]);	

#if !defined (USE_OPENGL)
#if defined (__sh__)
	int fd = ::open("/proc/stb/avs/0/colorformat", O_RDWR);
#else
	int fd = ::open("/proc/stb/video/hdmi_colorspace", O_RDWR);
#endif	
	if(fd > 0)
	{
		write(fd, aCOLORSPACE[colour_space], strlen(aCOLORSPACE[colour_space]));
		::close(fd);
	}
#endif	

	return 0;
}

void cVideo::SetStreamType(VIDEO_FORMAT type) 
{
#if !defined USE_OPENGL
	if(video_fd < 0)
		return;
	
	const char *aVIDEOFORMAT[] = {
		"VIDEO_STREAMTYPE_MPEG2",
		"VIDEO_STREAMTYPE_MPEG4_H264",
		"VIDEO_STREAMTYPE_MPEG4_H263",
		"VIDEO_STREAMTYPE_VC1",
		"VIDEO_STREAMTYPE_MPEG4_Part2",
		"VIDEO_STREAMTYPE_VC1_SM",
		"VIDEO_STREAMTYPE_MPEG1",
		//"VIDEO_STREAMTYPE_DIVX311"
		"VIDEO_STREAMTYPE_H265_HEVC",
		"VIDEO_STREAMTYPE_AVS"
	};

	dprintf(DEBUG_INFO, "%s:%s - type=%s\n", FILENAME, __FUNCTION__, aVIDEOFORMAT[type]);

	if (ioctl( video_fd, VIDEO_SET_STREAMTYPE, type) < 0)
		perror("VIDEO_SET_STREAMTYPE");
#endif
	
	StreamType = type;
}

/* set sync mode */
void cVideo::SetSyncMode(int mode)
{
	dprintf(DEBUG_NORMAL, "%s:%s\n", FILENAME, __FUNCTION__);	

#if defined (__sh__)
        int clock = 0;
	
	const char *aAVSYNCTYPE[] = {
		"AVSYNC_DISABLED",
		"AVSYNC_ENABLED",
		"AVSYNC_AUDIO_IS_MASTER"
	};

        const char* av_modes[] = {
		"disapply",
		"apply"
	};

        const char* master_clock[] = {
		"video",
		"audio"
	};
      	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aAVSYNCTYPE[mode]);	

	int fd = ::open("/proc/stb/stream/policy/AV_SYNC", O_RDWR);

        if (fd > 0)  
        {
           	if ((mode == 0) || (mode == 1))
	   	{
	      		write(fd, av_modes[mode], strlen(av_modes[mode]));
	      		clock = 0;
	   	} 
		else
           	{
	      		write(fd, av_modes[1], strlen(av_modes[1]));
	      		clock = 1;
	   	}
	   	::close(fd);
        }
		
        dprintf(DEBUG_INFO, "%s:%s - set master clock = %s\n", FILENAME, __FUNCTION__, master_clock[clock]);	

	fd = ::open("/proc/stb/stream/policy/MASTER_CLOCK", O_RDWR);
        if (fd > 0)  
        {
	   	write(fd, master_clock[clock], strlen(master_clock[clock]));
	   	::close(fd);
        }
#endif	
}

// setInput
void cVideo::SetInput(int val)
{ 
	const char *input[] = {"encoder", "scart", "aux"};
	
	printf("cVideo::SetInput: %s\n", input[val]);	

#if !defined (USE_OPENGL)
	// avs input
	int fd_avs_input = ::open("/proc/stb/avs/0/input", O_RDWR);

	if(fd_avs_input > 0)
	{
		write(fd_avs_input, input[val], strlen(input[val]));
		::close(fd_avs_input);
	}
#endif	
}

// setInput
void cVideo::SetStandby(int val)
{ 
	const char *sb[] = {"off", "on"};
	
	dprintf(DEBUG_INFO, "cVideo::SetStandby: %s\n", sb[val]);	

#if !defined (USE_OPENGL)
	// standby
	int fd_sb = ::open("/proc/stb/avs/0/standby", O_RDWR);
	
	if(fd_sb > 0)
	{
		write(fd_sb, sb[val], strlen(sb[val]));
		::close(fd_sb);
	}
	
	// FIXME:hdmi output
#endif	
}

/* Pig */
void cVideo::Pig(int x, int y, int w, int h, int osd_w, int osd_h, int num)
{ 
	//ugly we just resize the video display
	dprintf(DEBUG_INFO, "%s:%s - x=%d y=%d w=%d h=%d (video_num=%d)\n", FILENAME, __FUNCTION__, x, y, w, h, num);
	
	int _x, _y, _w, _h;
	/* the target "coordinates" seem to be in a PAL sized plane
	 * TODO: check this in the driver sources */
	int xres = 720;
	int yres = 576;
	
	if (x == -1 && y == -1 && w == -1 && h == -1)
	{
		_w = xres;
		_h = yres;
		_x = 0;
		_y = 0;
	}
	else
	{
		_x = x * xres / osd_w;
		_w = w * xres / osd_w;
		_y = y * yres / osd_h;
		_h = h * yres / osd_h;
	}
	
#if !defined (USE_OPENGL)	
	FILE* fd;
	char vmpeg_left[100];
	char vmpeg_top[100];
	char vmpeg_width[100];
	char vmpeg_height[100];
	
	// left
	sprintf(vmpeg_left, "/proc/stb/vmpeg/%d/dst_left", num);
	
	fd = fopen(vmpeg_left, "w");
	
	if(fd)
	{
		fprintf(fd, "%x", _x);
		fclose(fd);
	}

	// top
	sprintf(vmpeg_top, "/proc/stb/vmpeg/%d/dst_top", num);
	
	fd = fopen(vmpeg_top, "w");
	
	if(fd)
	{
		fprintf(fd, "%x", _y);
		fclose(fd);
	}

	// width
	sprintf(vmpeg_width, "/proc/stb/vmpeg/%d/dst_width", num);
	
	fd = fopen(vmpeg_width, "w");
	
	if(fd)
	{
		fprintf(fd, "%x", _w);
		fclose(fd);
	}

	// height
	sprintf(vmpeg_height, "/proc/stb/vmpeg/%d/dst_height", num);
	
	fd = fopen(vmpeg_height, "w");
	
	if(fd)
	{
		fprintf(fd, "%x", _h);
		fclose(fd);
	}
#endif	
}

/* set wss */
/*
0: off
1: auto
2: auto(4:3_off)
3: 4:3_full_format
4: 16:9_full_format
5: 14:9_letterbox_center
6: 14:9_letterbox_top
7: 16:9_letterbox_center
8: 16:9_letterbox_top
9: >16:9_letterbox_center
10: 14:9_full_format
*/
//void cVideo::SetWideScreen(bool onoff)
void cVideo::SetWideScreen(int val) // 0 = auto, 1 = auto(4:3_off)
{
#if defined (__sh__)
	const char * wss[] = {
		"off", 
		"auto", 
		"auto(4:3_off)", 
	};
#else
	const char * wss[] = {
		"off", 
		"auto", 
		"auto(4:3_off)", 
		"4:3_full_format", 		//not used
		"16:9_full_format",		//not used
		"14:9_letterbox_center",	//not used 
		"14:9_letterbox_top", 	//not used
		"16:9_letterbox_center",	//not used
		"16:9_letterbox_top", 	//not used
		">16:9_letterbox_center", 	//not used
		"14:9_full_format"		//not used
	};
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, wss[val]);

#if !defined (USE_OPENGL)	
	int fd = ::open("/proc/stb/denc/0/wss", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, wss[val], strlen(wss[val]));
		::close(fd);
	}
#endif	
}

/* set video mode */
void cVideo::SetAnalogMode(int mode)
{
  /*
  rgb 
  cvbs 
  yuv 
  */
#if defined (__sh__)
  	const char *aANALOGMODE[] = {
		"rgb",
		"cvbs",
		"svideo"
		"yuv"
	};
#else
	const char *aANALOGMODE[] = {
		"cvbs",
		"rgb",
		"yuv"
	};
#endif
	
	dprintf(DEBUG_INFO, "%s:%s - mode=%s\n", FILENAME, __FUNCTION__, aANALOGMODE[mode]);	

#if !defined (USE_OPENGL)	
	int fd = ::open("/proc/stb/avs/0/colorformat", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aANALOGMODE[mode], strlen(aANALOGMODE[mode]));
		::close(fd);
	}
#endif	
}

/* blank on freeze */
int cVideo::getBlank(void) 
{ 
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	struct video_status status;

	if( ::ioctl(video_fd, VIDEO_GET_STATUS, &status) < 0)
		perror("VIDEO_GET_STATUS");

	return status.video_blank;
}

/* set blank */
int cVideo::setBlank(int enable) 
{  
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return ::ioctl(video_fd, VIDEO_SET_BLANK, enable);
}

/* get play state */
int cVideo::getPlayState(void) 
{ 
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return playstate; 
}

/* set source */
int cVideo::setSource(int source)
{
	if(video_fd < 0)
		return -1;
	
	const char *aVIDEOSTREAMSOURCE[] = {
		"VIDEO_SOURCE_DEMUX",
		"VIDEO_SOURCE_MEMORY",
		"VIDEO_SOURCE_HDMI"
	};
		
	dprintf(DEBUG_INFO, "%s:%s - source=%s\n", FILENAME, __FUNCTION__, aVIDEOSTREAMSOURCE[source]);	
	
	return ::ioctl(video_fd, VIDEO_SELECT_SOURCE, source);
}

int64_t cVideo::GetPTS(void)
{
#ifdef USE_OPENGL
	int64_t pts = 0;
	buf_m.lock();
	if (buf_num != 0)
		pts = buffers[buf_out].pts();
	buf_m.unlock();
	return pts;
#else
	if(video_fd < 0)
		return -1;
	
	int64_t pts = 0;
	if (::ioctl(video_fd, VIDEO_GET_PTS, &pts) < 0)
		perror("GET_PTS failed");
	
	return pts;
#endif
}

// show mpeg still (used by RASS)
int cVideo::showSinglePic(const char *filename)
{
	
	dprintf(DEBUG_NORMAL, "showSinglePic %s\n", filename);
	
	int f = ::open(filename, O_RDONLY);
	
	if (f >= 0)
	{
		struct stat s;
		fstat(f, &s);
		
		// open video device
		Open();
		
		if (video_fd >= 0)
		{
			bool seq_end_avail = false;
			size_t pos = 0;
			unsigned char pes_header[] = { 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x80, 0x00, 0x00 };
			unsigned char seq_end[] = { 0x00, 0x00, 0x01, 0xB7 };
			unsigned char iframe[s.st_size];
			unsigned char stuffing[8192];
			
			memset(stuffing, 0, 8192);
			read(f, iframe, s.st_size);
			
			// setsource
			setSource(VIDEO_SOURCE_MEMORY);			  
			
			// play
			Start();
			
			// unfreeze
			Resume();
			
			// clear buffer
			Flush();
			
			while(pos <= (s.st_size-4) && !(seq_end_avail = (!iframe[pos] && !iframe[pos+1] && iframe[pos+2] == 1 && iframe[pos+3] == 0xB7)))
				++pos;
			if ((iframe[3] >> 4) != 0xE) // no pes header
				write(video_fd, pes_header, sizeof(pes_header));
			else
				iframe[4] = iframe[5] = 0x00;
			write(video_fd, iframe, s.st_size);
			if (!seq_end_avail)
				write(video_fd, seq_end, sizeof(seq_end));
			write(video_fd, stuffing, 8192);
		}
		else
		{
			dprintf(DEBUG_NORMAL, "video device not open\n");
			return -1;
		}
		
		::close(f);
	}
	else
	{
		dprintf(DEBUG_NORMAL, "couldnt open %s\n", filename);
		return -1;
	}
	
	return 0;
}

void cVideo::finishShowSinglePic()
{
	if (video_fd >= 0)
	{
		// stop playing
		Stop();
		
		// set source to demux
		setSource(VIDEO_SOURCE_DEMUX);
	}
}

void cVideo::setContrast(int Contrast)
{
	dprintf(DEBUG_NORMAL, "%s %s (%d)\n", __FILE__, __FUNCTION__, Contrast);
	
	FILE *fd;
#if defined (__sh__)
	fd = fopen("/proc/stb/video/plane/psi_contrast", "w");
#else
	fd = fopen("/proc/stb/vmpeg/0/pep_contrast", "w");
#endif
	if(fd)
	{
		fprintf(fd, "%d", Contrast);
		fclose(fd);
	}
}

void cVideo::setSaturation(int Saturation)
{
	dprintf(DEBUG_NORMAL, "%s %s (%d)\n", __FILE__, __FUNCTION__, Saturation);
	
	FILE *fd;
#if defined (__sh__)
	fd = fopen("/proc/stb/video/plane/psi_saturation", "w");
#else
	fd = fopen("/proc/stb/vmpeg/0/pep_saturation", "w");
#endif
	if(fd)
	{
		fprintf(fd, "%d", Saturation);
		fclose(fd);
	}
}

void cVideo::setBrightness(int Brightness)
{
	dprintf(DEBUG_NORMAL, "%s %s (%d)\n", __FILE__, __FUNCTION__, Brightness);
	
	FILE *fd;
#if defined (__sh__)
	fd = fopen("/proc/stb/video/plane/psi_brightness", "w");
#else
	fd = fopen("/proc/stb/vmpeg/0/pep_brightness", "w");
#endif
	if(fd)
	{
		fprintf(fd, "%d", Brightness);
		fclose(fd);
	}
}

void cVideo::setTint(int Tint)
{
	dprintf(DEBUG_NORMAL, "%s %s (%d)\n", __FILE__, __FUNCTION__, Tint);
	
	FILE *fd;
#if defined (__sh__)
	fd = fopen("/proc/stb/video/plane/psi_tint", "w");
#else
	fd = fopen("/proc/stb/vmpeg/0/pep_hue", "w");
#endif
	if(fd)
	{
		fprintf(fd, "%d", Tint);
		fclose(fd);
	}
}

#ifdef USE_OPENGL
cVideo::SWFramebuffer *cVideo::getDecBuf(void)
{
	buf_m.lock();
	if (buf_num == 0)
	{
		buf_m.unlock();
		return NULL;
	}
	SWFramebuffer *p = &buffers[buf_out];
	buf_out++;
	buf_num--;
	buf_out %= VDEC_MAXBUFS;
	buf_m.unlock();
	return p;
}

static int my_read(void *, uint8_t *buf, int buf_size)
{
	int tmp = 0;
	
	if (videoDecoder && videoDemux && bufpos < DMX_BUF_SZ - 4096)
	{
		while (bufpos < buf_size && ++tmp < 20)   /* retry max 20 times */
		{
			int ret = videoDemux->Read(dmxbuf + bufpos, DMX_BUF_SZ - bufpos, 20);
			
			if (ret > 0)
				bufpos += ret;
		}
	}
	
	if (bufpos == 0)
		return 0;
		
	if (bufpos > buf_size)
	{
		memcpy(buf, dmxbuf, buf_size);
		memmove(dmxbuf, dmxbuf + buf_size, bufpos - buf_size);
		bufpos -= buf_size;
		return buf_size;
	}
	
	memcpy(buf, dmxbuf, bufpos);
	tmp = bufpos;
	bufpos = 0;
	
	return tmp;
}

void cVideo::run(void)
{
	dprintf(DEBUG_NORMAL, "cVideo::run\n");
	
	AVCodec *codec;
	AVCodecParameters *p = NULL;
	AVCodecContext *c = NULL;
	AVFormatContext *avfc = NULL;
	AVInputFormat *inp;
	AVFrame *frame, *rgbframe;
	uint8_t *inbuf = (uint8_t *)av_malloc(INBUF_SIZE);
	AVPacket avpkt;
	struct SwsContext *convert = NULL;

	time_t warn_r = 0; /* last read error */
	time_t warn_d = 0; /* last decode error */
	int av_ret = 0;

	bufpos = 0;
	buf_num = 0;
	buf_in = 0;
	buf_out = 0;
	dec_r = 0;

	av_init_packet(&avpkt);
	inp = av_find_input_format("mpegts");
	AVIOContext *pIOCtx = avio_alloc_context(inbuf, INBUF_SIZE, // internal Buffer and its size
	        0,      // bWriteable (1=true,0=false)
	        NULL,       // user data; will be passed to our callback functions
	        my_read,    // read callback
	        NULL,       // write callback
	        NULL);      // seek callback
	        
	avfc = avformat_alloc_context();
	avfc->pb = pIOCtx;
	avfc->iformat = inp;
	avfc->probesize = 188 * 5;

	thread_running = true;
	
	if (avformat_open_input(&avfc, NULL, inp, NULL) < 0)
	{
		//hal_info("%s: Could not open input\n", __func__);
		goto out;
	}
	
	while (avfc->nb_streams < 1)
	{
		printf("%s: nb_streams %d, should be 1 => retry\n", __func__, avfc->nb_streams);
		
		if (av_read_frame(avfc, &avpkt) < 0)
			printf("%s: av_read_frame < 0\n", __func__);
			
		av_packet_unref(&avpkt);
		if (! thread_running)
			goto out;
	}

	p = avfc->streams[0]->codecpar;
	
	if (p->codec_type != AVMEDIA_TYPE_VIDEO)
		printf("%s: no video codec? 0x%x\n", __func__, p->codec_type);

	codec = avcodec_find_decoder(p->codec_id);
	
	if (!codec)
	{
		printf("%s: Codec for %s not found\n", __func__, avcodec_get_name(p->codec_id));
		goto out;
	}
	c = avcodec_alloc_context3(codec);
	if (avcodec_open2(c, codec, NULL) < 0)
	{
		printf("%s: Could not open codec\n", __func__);
		goto out;
	}
	
	frame = av_frame_alloc();
	rgbframe = av_frame_alloc();
	
	if (!frame || !rgbframe)
	{
		printf("%s: Could not allocate video frame\n", __func__);
		goto out2;
	}
	
	printf("decoding %s\n", avcodec_get_name(c->codec_id));
	
	while (thread_running)
	{
		if (av_read_frame(avfc, &avpkt) < 0)
		{
			if (warn_r - time(NULL) > 4)
			{
				printf("%s: av_read_frame < 0\n", __func__);
				warn_r = time(NULL);
			}
			
			usleep(10000);
			continue;
		}
		
		int got_frame = 0;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		av_ret = avcodec_decode_video2(c, frame, &got_frame, &avpkt);
		
		if (av_ret < 0)
		{
			if (warn_d - time(NULL) > 4)
			{
				printf("%s: avcodec_decode_video2 %d\n", __func__, av_ret);
				warn_d = time(NULL);
			}
			
			av_packet_unref(&avpkt);
			continue;
		}
		
		if (avpkt.size > av_ret)
			hal_info("%s: WARN: pkt->size %d != len %d\n", __func__, avpkt.size, av_ret);
#else
		av_ret = avcodec_send_packet(c, &avpkt);
		
		if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
		{
			if (warn_d - time(NULL) > 4)
			{
				printf("%s: avcodec_send_packet %d\n", __func__, av_ret);
				warn_d = time(NULL);
			}
			av_packet_unref(&avpkt);
			continue;
		}
		
		av_ret = avcodec_receive_frame(c, frame);
		if (!av_ret)
			got_frame = 1;
#endif
		still_m.lock();
		if (got_frame && ! stillpicture)
		{
			unsigned int need = av_image_get_buffer_size(VDEC_PIXFMT, c->width, c->height, 1);
			
			convert = sws_getCachedContext(convert,
			        c->width, c->height, c->pix_fmt,
			        c->width, c->height, VDEC_PIXFMT,
			        SWS_BICUBIC, 0, 0, 0);
			        
			if (!convert)
				printf("%s: ERROR setting up SWS context\n", __func__);
			else
			{
				buf_m.lock();
				
				SWFramebuffer *f = &buffers[buf_in];
				
				if (f->size() < need)
					f->resize(need);
					
				av_image_fill_arrays(rgbframe->data, rgbframe->linesize, &(*f)[0], VDEC_PIXFMT, c->width, c->height, 1);
				sws_scale(convert, frame->data, frame->linesize, 0, c->height, rgbframe->data, rgbframe->linesize);
				if (dec_w != c->width || dec_h != c->height)
				{
					printf("%s: pic changed %dx%d -> %dx%d\n", __func__, dec_w, dec_h, c->width, c->height);
					dec_w = c->width;
					dec_h = c->height;
					w_h_changed = true;
				}
				
				f->width(c->width);
				f->height(c->height);
				
#if (LIBAVUTIL_VERSION_MAJOR < 54)
				int64_t vpts = av_frame_get_best_effort_timestamp(frame);
#else
				int64_t vpts = frame->best_effort_timestamp;
#endif
				/* a/v delay determined experimentally :-) */
				if (StreamType == VIDEO_STREAMTYPE_MPEG2)
					vpts += 90000 * 4 / 10; /* 400ms */
				else
					vpts += 90000 * 3 / 10; /* 300ms */

				f->pts(vpts);
				AVRational a = av_guess_sample_aspect_ratio(avfc, avfc->streams[0], frame);
				f->AR(a);
				buf_in++;
				buf_in %= VDEC_MAXBUFS;
				buf_num++;
				
				if (buf_num > (VDEC_MAXBUFS - 1))
				{
					//printf("%s: buf_num overflow\n", __func__);
					buf_out++;
					buf_out %= VDEC_MAXBUFS;
					buf_num--;
				}
				dec_r = c->time_base.den / (c->time_base.num * c->ticks_per_frame);
				buf_m.unlock();
			}
			
			//printf("%s: time_base: %d/%d, ticks: %d rate: %d pts 0x%" PRIx64 "\n", __func__, c->time_base.num, c->time_base.den, c->ticks_per_frame, dec_r,
#if (LIBAVUTIL_VERSION_MAJOR < 54)
			 //   av_frame_get_best_effort_timestamp(frame));
#else
			//    frame->best_effort_timestamp);
#endif
		}
		else
			printf("%s: got_frame: %d stillpicture: %d\n", __func__, got_frame, stillpicture);
		still_m.unlock();
		av_packet_unref(&avpkt);
	}
	sws_freeContext(convert);
out2:
	avcodec_close(c);
	av_free(c);
	av_frame_free(&frame);
	av_frame_free(&rgbframe);
out:
	avformat_close_input(&avfc);
	av_free(pIOCtx->buffer);
	av_free(pIOCtx);
	/* reset output buffers */
	bufpos = 0;
	still_m.lock();
	if (!stillpicture)
	{
		buf_num = 0;
		buf_in = 0;
		buf_out = 0;
	}
	still_m.unlock();
}
#endif

