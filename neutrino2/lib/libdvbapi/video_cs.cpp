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


static const char * FILENAME = "[video_cs.cpp]";

cVideo * videoDecoder = NULL;

//ugly most functions are done in proc
/* constructor & destructor */
cVideo::cVideo(int num)
{ 
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
	
	video_fd = -1;
	video_adapter = 0;
	video_num = num;

	playstate = VIDEO_STOPPED;
	
	StreamType = VIDEO_STREAMTYPE_MPEG2;
}

cVideo::~cVideo(void)
{  
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	Close();
}

bool cVideo::Open(CFrontend * fe)
{ 
	if(fe)
		video_adapter = fe->fe_adapter;
	
	char devname[32];

	// Open video device
	sprintf(devname, "/dev/dvb/adapter%d/video%d", video_adapter, video_num);
	
	if(video_fd > 0)
	{
		printf("%s %s already opened\n", __FUNCTION__, devname);
		return true;
	}

	video_fd = open(devname, O_RDWR);

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
		close(video_fd);
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

	fd = open("/proc/stb/vmpeg/0/aspect", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 2);
		close(fd);
		
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
        fd = open("/proc/stb/video/aspect", O_WRONLY);
	
	if(fd > 0)
	{
		write(fd, sRatio[ratio], strlen(sRatio[ratio]));
		close(fd);
	}

	// policy
	fd = open("/proc/stb/video/policy", O_WRONLY);
	
	if(fd > 0)
	{
		write(fd, sFormat[format], strlen((const char*) sFormat[format]));
		close(fd);
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
	fd = open("/proc/stb/vmpeg/0/framerate", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		close(fd);

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
	fd = open("/proc/stb/vmpeg/0/xres", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		close(fd);

		if (n > 0) 
		{
			sscanf((const char*) buffer, "%X", &width);
		}
	}

	// height  (yres)
	fd = open("/proc/stb/vmpeg/0/yres", O_RDONLY);
	
	if(fd > 0)
	{
		n = read(fd, buffer, 10);
		close(fd);

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
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);

	if (playstate == VIDEO_PLAYING)
		return 0;

	playstate = VIDEO_PLAYING;
	
	// Video Play
	if(ioctl(video_fd, VIDEO_PLAY) < 0)
		perror("VIDEO_PLAY");	
		
	return true;
}

int cVideo::Stop(bool blank)
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s blank:%d\n", FILENAME, __FUNCTION__, blank);	
		
	playstate = blank ? VIDEO_STOPPED : VIDEO_FREEZED;
	
	if( ioctl(video_fd, VIDEO_STOP, blank ? 1 : 0) < 0 )  
		perror("VIDEO_STOP");	
	
	return true;
}

bool cVideo::Pause(void)
{ 
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);
		
	if (ioctl(video_fd, VIDEO_FREEZE) < 0)
		perror("VIDEO_FREEZE");
	
	playstate = VIDEO_FREEZED;	
		
	return true;
}

bool cVideo::Resume(void)
{
	if(video_fd < 0)
		return false;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	
		
	if (ioctl(video_fd, VIDEO_CONTINUE) < 0)
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
	ret = ioctl(video_fd, VIDEO_FLUSH);
#else
	ret = ioctl(video_fd, VIDEO_CLEAR_BUFFER);
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
	int fd = open("/proc/stb/video/videomode", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aVideoSystems[video_system][1], strlen(aVideoSystems[video_system][1]));
		close(fd);
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
	int fd = open("/proc/stb/avs/0/colorformat", O_RDWR);
#else
	int fd = open("/proc/stb/video/hdmi_colorspace", O_RDWR);
#endif	
	if(fd > 0)
	{
		write(fd, aCOLORSPACE[colour_space], strlen(aCOLORSPACE[colour_space]));
		close(fd);
	}
#endif	

	return 0;
}

void cVideo::SetStreamType(VIDEO_FORMAT type) 
{
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

	int fd = open("/proc/stb/stream/policy/AV_SYNC", O_RDWR);

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
	   	close(fd);
        }
		
        dprintf(DEBUG_INFO, "%s:%s - set master clock = %s\n", FILENAME, __FUNCTION__, master_clock[clock]);	

	fd = open("/proc/stb/stream/policy/MASTER_CLOCK", O_RDWR);
        if (fd > 0)  
        {
	   	write(fd, master_clock[clock], strlen(master_clock[clock]));
	   	close(fd);
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
	int fd_avs_input = open("/proc/stb/avs/0/input", O_RDWR);

	if(fd_avs_input > 0)
	{
		write(fd_avs_input, input[val], strlen(input[val]));
		close(fd_avs_input);
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
	int fd_sb = open("/proc/stb/avs/0/standby", O_RDWR);
	
	if(fd_sb > 0)
	{
		write(fd_sb, sb[val], strlen(sb[val]));
		close(fd_sb);
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
	
	if(fd > 0)
	{
		fprintf(fd, "%x", _x);
		fclose(fd);
	}

	// top
	sprintf(vmpeg_top, "/proc/stb/vmpeg/%d/dst_top", num);
	
	fd = fopen(vmpeg_top, "w");
	
	if(fd > 0)
	{
		fprintf(fd, "%x", _y);
		fclose(fd);
	}

	// width
	sprintf(vmpeg_width, "/proc/stb/vmpeg/%d/dst_width", num);
	
	fd = fopen(vmpeg_width, "w");
	
	if(fd > 0)
	{
		fprintf(fd, "%x", _w);
		fclose(fd);
	}

	// height
	sprintf(vmpeg_height, "/proc/stb/vmpeg/%d/dst_height", num);
	
	fd = fopen(vmpeg_height, "w");
	
	if(fd > 0)
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
	int fd = open("/proc/stb/denc/0/wss", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, wss[val], strlen(wss[val]));
		close(fd);
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
	int fd = open("/proc/stb/avs/0/colorformat", O_RDWR);
	
	if(fd > 0)
	{
		write(fd, aANALOGMODE[mode], strlen(aANALOGMODE[mode]));
		close(fd);
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

	if( ioctl(video_fd, VIDEO_GET_STATUS, &status) < 0)
		perror("VIDEO_GET_STATUS");

	return status.video_blank;
}

/* set blank */
int cVideo::setBlank(int enable) 
{  
	if(video_fd < 0)
		return -1;
	
	dprintf(DEBUG_INFO, "%s:%s\n", FILENAME, __FUNCTION__);	

	return ioctl(video_fd, VIDEO_SET_BLANK, enable);
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
	
	return ioctl(video_fd, VIDEO_SELECT_SOURCE, source);
}

int64_t cVideo::GetPTS(void)
{
	if(video_fd < 0)
		return -1;
	
	int64_t pts = 0;
	if (ioctl(video_fd, VIDEO_GET_PTS, &pts) < 0)
		perror("GET_PTS failed");
	
	return pts;
}

// show mpeg still (used by RASS)
int cVideo::showSinglePic(const char *filename)
{
	
	dprintf(DEBUG_NORMAL, "showSinglePic %s\n", filename);
	
	int f = open(filename, O_RDONLY);
	
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
		
		close(f);
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

