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

#include <driver/gfx/framebuffer.h>


#if defined (__sh__)
#define VIDEO_FLUSH                     _IO('o',  82)
#define AUDIO_FLUSH                     _IO('o',  71)
#endif

#ifdef USE_OPENGL
#include "dmx_cs.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/* ffmpeg buf 32k */
#define INBUF_SIZE 0x8000
/* my own buf 256k */
#define DMX_BUF_SZ 0x20000

extern cVideo *videoDecoder;
extern cDemux *videoDemux;
//
static uint8_t *dmxbuf;
static int bufpos;
#endif

////
cVideo::cVideo(int num)
{ 
	printf("cVideo::cVideo: num:%d\n", num);
	
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
	dec_r = 0;
	dec_vpts = 0;
	stillpicture = false;
	w_h_changed = false;
	////
	pig_x = pig_y = pig_w = pig_h = 0;
	pig_changed = false;
#endif
}

cVideo::~cVideo(void)
{  
	printf("cVideo::~cVideo\n");	

	Close();
}

bool cVideo::Open(CFrontend * fe)
{
	printf("cVideo::Open\n");
	
#ifndef USE_OPENGL
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
		printf("cVideo::Open %s\n", devname);
		return true;
	}
	
	printf("%s %s failed\n", __FUNCTION__, devname);
#endif

	return false;
}

bool cVideo::Close()
{ 
	printf("cVideo::Close\n");
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return false;	
	
	if(video_fd >= 0)
	{
		::close(video_fd);
		video_fd = -1;
	}
#endif	

	return true;
}

int cVideo::getAspectRatio(void) 
{  
	int ratio = ASPECTRATIO_43; // 0 = 4:3, 1 = 16:9

	printf("cVideo::getAspectRatio\n");	
	 
#ifndef USE_OPENGL
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
	
	printf("cVideo::getAspectRatio (ratio=%d)\n", ratio);
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
	printf("cVideo::setAspectRatio\n");
	
#ifndef USE_OPENGL
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
	
	printf("cVideo::setAspectRatio: (aspect=%d format=%d) set %s %s\n", ratio, format, sRatio[ratio], sFormat[format]);
#endif	

    	return 0; 
}

void cVideo::getPictureInfo(int &width, int &height, int &rate) 
{
#ifdef USE_OPENGL
	width = dec_w;
	height = dec_h;
	
	switch (dec_r)
	{
		case 23://23.976fps
			rate = VIDEO_FRAME_RATE_23_976;
			break;
		case 24:
			rate = VIDEO_FRAME_RATE_24;
			break;
		case 25:
			rate = VIDEO_FRAME_RATE_25;
			break;
		case 29://29,976fps
			rate = VIDEO_FRAME_RATE_29_97;
			break;
		case 30:
			rate = VIDEO_FRAME_RATE_30;
			break;
		case 50:
			rate = VIDEO_FRAME_RATE_50;
			break;
		case 60:
			rate = VIDEO_FRAME_RATE_60;
			break;
		default:
			rate = dec_r;
			break;
	}
#else
	rate = 25;
	height = 576;
	width = 720;
	  
	printf("getPictureInfo\n"); 

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
	
	printf("getPictureInfo < w %d, h %d, r %d\n", width, height, rate);
#endif	
}

int cVideo::Start(void)
{ 
	printf("cVideo::Start\n");
	
	int ret = -1;
	
#ifdef USE_OPENGL
	if (!thread_running)
	{
		ret = OpenThreads::Thread::start();
	}
		
	playstate = VIDEO_PLAYING;
#else
	if(video_fd < 0)
		return -1;

	if (playstate == VIDEO_PLAYING)
		return 0;

	playstate = VIDEO_PLAYING;
	
	// Video Play
	ret = ::ioctl(video_fd, VIDEO_PLAY);
	
	if (ret < 0)
		perror("VIDEO_PLAY");	
#endif

	return ret;
}

int cVideo::Stop(bool blank)
{ 
	printf("cVideo::Stop: blank:%d\n", blank);	
	
	int ret = -1;
	
#ifdef USE_OPENGL
	if (thread_running)
	{
		thread_running = false;
		ret = OpenThreads::Thread::join();
	}
	
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
#else
	if(video_fd < 0)
		return -1;
		
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
	
	ret = ::ioctl(video_fd, VIDEO_STOP, blank ? 1 : 0);
	
	if (ret < 0) 
		perror("VIDEO_STOP");	
#endif

	return ret;
}

bool cVideo::Pause(void)
{ 
	printf("cVideo::Pause\n");
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return false;
		
	if (::ioctl(video_fd, VIDEO_FREEZE) < 0)
		perror("VIDEO_FREEZE");
#endif
	
	playstate = VIDEO_FREEZED;	
	
	return true;
}

bool cVideo::Resume(void)
{
	printf("cVideo::Resume\n");	
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return false;
		
	if (::ioctl(video_fd, VIDEO_CONTINUE) < 0)
		perror("VIDEO_CONTINUE");
#endif
	
	playstate = VIDEO_PLAYING;	
		
	return true;
}

int cVideo::Flush(void)
{ 
	printf("cVideo::Flush\n");
	
	int ret = -1;
	
#ifndef USE_OPENGL 
	if(video_fd < 0)
		return -1;

#if defined (__sh__)
	ret = ::ioctl(video_fd, VIDEO_FLUSH);
#else
	ret = ::ioctl(video_fd, VIDEO_CLEAR_BUFFER);
#endif

	if(ret < 0)
		perror("VIDEO_FLUSH");
#endif		
	
	return ret;
}

int cVideo::setSlowMotion(int repeat)
{
	printf("cVideo::setSlowMotion: (%d)\n", repeat);
	
	int ret = -1;
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;
		
	ret = ::ioctl(video_fd, VIDEO_SLOWMOTION, repeat);
	if (ret < 0)
		perror("VIDEO_SLOWMOTION");
#endif	
	
	return ret;
}

int cVideo::setFastForward(int skip)
{
	printf("cVideo::setFastForward: (%d)\n", skip);
	
	int ret = -1;
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;
		
	ret = ::ioctl(video_fd, VIDEO_FAST_FORWARD, skip);
	
	if (ret < 0)
		perror("VIDEO_FAST_FORWARD");
#endif	

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

	printf("cVideo::setVideoSystem: video_system=%s\n", aVideoSystems[video_system][0]);	

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
	
	printf("cVideo::SetSpaceColour: mode=%s\n", aCOLORSPACE[colour_space]);	

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
	const char *aVIDEOFORMAT[] = {
		"VIDEO_STREAMTYPE_MPEG2",
		"VIDEO_STREAMTYPE_MPEG4_H264",
		"VIDEO_STREAMTYPE_MPEG4_H263",
		"VIDEO_STREAMTYPE_VC1",
		"VIDEO_STREAMTYPE_MPEG4_Part2",
		"VIDEO_STREAMTYPE_VC1_SM",
		"VIDEO_STREAMTYPE_MPEG1",
		"VIDEO_STREAMTYPE_H265_HEVC",
		"VIDEO_STREAMTYPE_VB8",
		"VIDEO_STREAMTYPE_VB9",
		"VIDEO_STREAMTYPE_XVID",
		"VIDEO_STREAMTYPE_DIVX311",
		"VIDEO_STREAMTYPE_DIVX4",
		"VIDEO_STREAMTYPE_DIVX5",
		"VIDEO_STREAMTYPE_AVS",
		"VIDEO_STREAMTYPE_VB6",
		"VIDEO_STREAMTYPE_SPARK"
	};

	printf("cVideo::SetStreamType: type=%s\n", aVIDEOFORMAT[type]);
	
#if !defined USE_OPENGL
	if(video_fd < 0)
		return;

	if (ioctl( video_fd, VIDEO_SET_STREAMTYPE, type) < 0)
		perror("VIDEO_SET_STREAMTYPE");
#endif
	
	StreamType = type;
}

/* set sync mode */
void cVideo::SetSyncMode(int mode)
{
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
      	
	printf("cVideo::setSyncMode: mode=%s\n", aAVSYNCTYPE[mode]);	

#ifndef USE_OPENGL
#if defined (__sh__)
        int clock = 0;	

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
		
        printf("cVideo::SetSyncMode: set master clock = %s\n", master_clock[clock]);	

	fd = ::open("/proc/stb/stream/policy/MASTER_CLOCK", O_RDWR);
        if (fd > 0)  
        {
	   	write(fd, master_clock[clock], strlen(master_clock[clock]));
	   	::close(fd);
        }
#endif
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
	
	printf("cVideo::SetStandby: %s\n", sb[val]);	

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
	printf("cVideo::Pig: - x=%d y=%d w=%d h=%d (video_num=%d)\n", x, y, w, h, num);
	
#ifdef USE_OPENGL
	pig_x = x;
	pig_y = y;
	pig_w = w;
	pig_h = h;
	pig_changed = true;
#else
	
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
		
		_x /= 1280;
		_y /= 720;
		_w /= 1280;
		_h /= 720;
	}
		
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
	
	printf("cVideo::SetWideScreen: mode=%s\n", wss[val]);

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
	
	printf("cVideo::SetAnalogMode: mode=%s\n", aANALOGMODE[mode]);	
	
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
	printf("cVideo::getBlank\n");	
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;

	struct video_status status;

	if( ::ioctl(video_fd, VIDEO_GET_STATUS, &status) < 0)
		perror("VIDEO_GET_STATUS");

	return status.video_blank;
#else
	return 1;
#endif
}

/* set blank */
int cVideo::setBlank(int enable) 
{ 
	printf("cVideo::setBlank\n");	
	 
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;

	return ::ioctl(video_fd, VIDEO_SET_BLANK, enable);
#else
	return enable;
#endif
}

/* get play state */
int cVideo::getPlayState(void) 
{ 
	printf("cVideo::getPlayState:\n");	
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;
#endif

	return playstate; 
}

/* set source */
int cVideo::setSource(int source)
{
	const char *aVIDEOSTREAMSOURCE[] = {
		"VIDEO_SOURCE_DEMUX",
		"VIDEO_SOURCE_MEMORY",
		"VIDEO_SOURCE_HDMI"
	};
		
	printf("cVideo::setSource: source=%s\n", aVIDEOSTREAMSOURCE[source]);	
	
#ifndef USE_OPENGL
	if(video_fd < 0)
		return -1;
	
	return ::ioctl(video_fd, VIDEO_SELECT_SOURCE, source);
#else
	return 0;
#endif
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
	
	printf("cVideo::showSinglePic %s\n", filename);
	
#ifndef USE_OPENGL
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
			printf("video device not open\n");
			return -1;
		}
		
		::close(f);
	}
	else
	{
		printf("couldnt open %s\n", filename);
		return -1;
	}
#endif
	
	return 0;
}

void cVideo::finishShowSinglePic()
{
	printf("cVideo::finishShowSinglePic:\n");
	
#ifndef USE_OPENGL
	if (video_fd >= 0)
	{
		// stop playing
		Stop();
		
		// set source to demux
		setSource(VIDEO_SOURCE_DEMUX);
	}
#endif
}

void cVideo::setContrast(int Contrast)
{
	printf("cVideo::setContrast: (%d)\n", Contrast);
	
#ifndef USE_OPENGL
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
#endif
}

void cVideo::setSaturation(int Saturation)
{
	printf("cVideo::setSaturation: (%d)\n", Saturation);
	
#ifndef USE_OPENGL
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
#endif
}

void cVideo::setBrightness(int Brightness)
{
	printf("cVideo::setBrightness: (%d)\n", Brightness);
	
#ifndef USE_OPENGL
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
#endif
}

void cVideo::setTint(int Tint)
{
	printf("cVideo::setTint: (%d)\n", Tint);
	
#ifndef USE_OPENGL
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
#endif
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
	
	//
	if (videoDecoder && videoDemux && bufpos < DMX_BUF_SZ - 4096)
	{
		while (bufpos < buf_size && ++tmp < 20)   // retry max 20 times
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
	printf("cVideo::run: START\n");
	
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
	const AVCodec *codec;
	const AVInputFormat *inp;
#else
	AVCodec *codec;
	AVInputFormat *inp;
#endif
	AVCodecParameters *p = NULL;
	AVCodecContext *c = NULL;
	AVFormatContext *avfc = NULL;
	AVFrame *frame, *rgbframe;
	uint8_t *inbuf = (uint8_t *)av_malloc(INBUF_SIZE);
	AVPacket avpkt;
	struct SwsContext* convert = NULL;

	time_t warn_r = 0; // last read error
	time_t warn_d = 0; // last decode error
	int av_ret = 0;

	bufpos = 0;
	buf_num = 0;
	buf_in = 0;
	buf_out = 0;
	dec_r = 0;

	//
	av_init_packet(&avpkt);
	
	//
	inp = av_find_input_format("mpegts");
	AVIOContext *pIOCtx = avio_alloc_context(inbuf, INBUF_SIZE, // internal Buffer and its size
	        0,      	// bWriteable (1=true,0=false)
	        NULL,       	// user data; will be passed to our callback functions
	        my_read,    	// read callback
	        NULL,       	// write callback
	        NULL);      	// seek callback
	
	//       
	avfc = avformat_alloc_context();
	avfc->pb = pIOCtx;
	avfc->iformat = inp;
	avfc->probesize = 188 * 5;
	
	thread_running = true;
	
	if (avformat_open_input(&avfc, NULL, inp, NULL) < 0)
	{
		printf("cVideo::run: Could not open input\n");
		goto out;
	}
	
	while (avfc->nb_streams < 1)
	{
		printf("cVideo::run: nb_streams %d, should be 1 => retry\n", avfc->nb_streams);
		
		if (av_read_frame(avfc, &avpkt) < 0)
			printf("cVideo::run: av_read_frame < 0\n");
			
		av_packet_unref(&avpkt);
		
		if (! thread_running)
			goto out;
	}

	p = avfc->streams[0]->codecpar;
	
	if (p->codec_type != AVMEDIA_TYPE_VIDEO)
		printf("cVideo::run: no video codec? 0x%x\n", p->codec_type);

	codec = avcodec_find_decoder(p->codec_id);
	
	if (!codec)
	{
		printf("cVideo::run: Codec for %s not found\n", avcodec_get_name(p->codec_id));
		goto out;
	}
	
	// setup codec context for decoder
	c = avcodec_alloc_context3(codec);
	
	if (avcodec_open2(c, codec, NULL) < 0)
	{
		printf("cVideo::run: Could not open codec\n");
		goto out;
	}
	
	//
	frame = av_frame_alloc();
	rgbframe = av_frame_alloc();
	
	if (!frame || !rgbframe)
	{
		printf("cVideo::run: Could not allocate video frame\n");
		goto out2;
	}
	
	printf("cVideo::run: decoding %s\n", avcodec_get_name(c->codec_id));
	
	while (thread_running)
	{
		if (playstate == VIDEO_FREEZED) 
		{
			usleep(100000);
			continue;
		}
		
		//
		if (av_read_frame(avfc, &avpkt) < 0)
		{
			if (warn_r - time(NULL) > 4)
			{
				warn_r = time(NULL);
			}
			
			usleep(10000);
			continue;
		}
		
		//
		int got_frame = 0;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
		av_ret = avcodec_decode_video2(c, frame, &got_frame, &avpkt);
		
		if (av_ret < 0)
		{
			if (warn_d - time(NULL) > 4)
			{
				warn_d = time(NULL);
			}
			
			av_packet_unref(&avpkt);
			continue;
		}
#else
		av_ret = avcodec_send_packet(c, &avpkt);
		
		if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
		{
			if (warn_d - time(NULL) > 4)
			{
				warn_d = time(NULL);
			}
			av_packet_unref(&avpkt);
			continue;
		}
		
		av_ret = avcodec_receive_frame(c, frame);
		if (!av_ret)
			got_frame = 1;
#endif
		// setup sws scaler
		still_m.lock();
		if (got_frame && ! stillpicture)
		{
			int need = av_image_get_buffer_size(AV_PIX_FMT_RGB32, c->width, c->height, 1);

			convert = sws_getCachedContext(convert, c->width, c->height, c->pix_fmt, c->width, c->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, 0, 0, 0);
			        
			if (convert)
			{
				buf_m.lock();
				
				SWFramebuffer *f = &buffers[buf_in];
				
				if (f->size() < need)
					f->resize(need);
				
				// fill	
				av_image_fill_arrays(rgbframe->data, rgbframe->linesize, &(*f)[0], AV_PIX_FMT_RGB32, c->width, c->height, 1);

				// scale
				sws_scale(convert, frame->data, frame->linesize, 0, c->height, rgbframe->data, rgbframe->linesize);
				
				//
				if (dec_w != c->width || dec_h != c->height)
				{
					printf("cVideo::run: pic changed %dx%d -> %dx%d\n", dec_w, dec_h, c->width, c->height);
					dec_w = c->width;
					dec_h = c->height;
					w_h_changed = true;
				}
				
				f->width(c->width);
				f->height(c->height);
				
#if (LIBAVUTIL_VERSION_MAJOR < 54)
				dec_vpts = av_frame_get_best_effort_timestamp(frame);
#else
				dec_vpts = frame->best_effort_timestamp;
#endif
				// a/v delay determined experimentally :-)
				//if (p->codec_id == AV_CODEC_ID_MPEG2VIDEO)
				//	dec_vpts += 90000 * 4 / 10; // 400ms
				//else
				//	dec_vpts += 90000 * 3 / 10; // 300ms

				f->pts(dec_vpts);
				
				AVRational a = av_guess_sample_aspect_ratio(avfc, avfc->streams[0], frame);
				
				f->AR(a);
				
				//
				buf_in++;
				buf_in %= VDEC_MAXBUFS;
				buf_num++;
				
				if (buf_num > (VDEC_MAXBUFS - 1))
				{
					buf_out++;
					buf_out %= VDEC_MAXBUFS;
					buf_num--;
				}
				
				dec_r = c->time_base.den / (c->time_base.num * c->ticks_per_frame);
				
				buf_m.unlock();
			}
		}
		
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
	
	// reset output buffers
	bufpos = 0;
	
	still_m.lock();
	if (!stillpicture)
	{
		buf_num = 0;
		buf_in = 0;
		buf_out = 0;
	}
	still_m.unlock();
	
	printf("cVideo::run: END\n");
}
#endif

