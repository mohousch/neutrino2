/*
 * $Id: video_cs.h 2016.06.22 11:40:30 mohousch Exp $
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

#ifndef _VIDEO_CS_H
#define _VIDEO_CS_H

#include <sys/types.h>
#include <vector>

#include <linux/dvb/video.h>

#include <driver/gfx/framebuffer.h>

#include <zapit/frontend_c.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

#ifdef USE_OPENGL
extern "C" {
#include <libavutil/rational.h>
}
#endif


#ifndef VIDEO_SOURCE_HDMI
#define VIDEO_SOURCE_HDMI 2
#endif

// video stream type 
typedef enum {
	VIDEO_STREAMTYPE_MPEG2 		= 0,
	VIDEO_STREAMTYPE_MPEG4_H264 	= 1,
	VIDEO_STREAMTYPE_MPEG4_H263 	= 2,
	VIDEO_STREAMTYPE_VC1 		= 3,
	VIDEO_STREAMTYPE_MPEG4_Part2 	= 4,
	VIDEO_STREAMTYPE_VC1_SM 	= 5,
	VIDEO_STREAMTYPE_MPEG1 		= 6,
	VIDEO_STREAMTYPE_H265_HEVC 	= 7,
	VIDEO_STREAMTYPE_VB8 		= 8,
	VIDEO_STREAMTYPE_VB9 		= 9,
	VIDEO_STREAMTYPE_XVID 		= 10,
	VIDEO_STREAMTYPE_DIVX311 	= 13,
	VIDEO_STREAMTYPE_DIVX4 		= 14,
	VIDEO_STREAMTYPE_DIVX5 		= 15,
	VIDEO_STREAMTYPE_AVS 		= 16,
	VIDEO_STREAMTYPE_VB6 		= 18,
	VIDEO_STREAMTYPE_SPARK 		= 21
}VIDEO_FORMAT;

// video mode
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
PC
//spark7162
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
1080p59 
1080p60 
PC
*/
enum {
	VIDEO_STD_PAL,
	VIDEO_STD_1080I50,
	VIDEO_STD_720P50,
	VIDEO_STD_576P50,
	VIDEO_STD_576I50,
	VIDEO_STD_1080I60,
	VIDEO_STD_720P60,
	VIDEO_STD_1080P24,
	VIDEO_STD_1080P25,
	VIDEO_STD_1080P30,
	VIDEO_STD_1080P50,
	VIDEO_STD_PC
};
#else
// bcm
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
enum {
	VIDEO_STD_PAL,
	VIDEO_STD_NTSC,
	VIDEO_STD_480I60,
	VIDEO_STD_576I50,
	VIDEO_STD_480P60,
	VIDEO_STD_576P50,
	VIDEO_STD_720P50,
	VIDEO_STD_720P60,
	VIDEO_STD_1080I50,
	VIDEO_STD_1080I60
};
#endif

// aspect ratio
enum {
	ASPECTRATIO_43,
	ASPECTRATIO_169,
	ASPECTRATIO_AUTO
};

// policy
#if defined (__sh__)
enum {
	VIDEOFORMAT_LETTERBOX,
	VIDEOFORMAT_PANSCAN,
	VIDEOFORMAT_FULLSCREEN,
	VIDEOFORMAT_PANSCAN2
};
#else
enum {
	VIDEOFORMAT_LETTERBOX,
	VIDEOFORMAT_PANSCAN,
	VIDEOFORMAT_PANSCAN2,
	VIDEOFORMAT_FULLSCREEN
};
#endif

// input
enum {
	INPUT_ENCODER,
	INPUT_SCART,
	INPUT_AUX
};

// standby
enum {
	STANDBY_OFF,
	STANDBY_ON
};

// analoge mode
#if defined (__sh__)
enum {
	ANALOG_RGB,
	ANALOG_CVBS,
	ANALOG_SVIDEO,
	ANALOG_YUV
};
#else
enum {
	ANALOG_RGB,
	ANALOG_CVBS,
	ANALOG_YUV
};
#endif

// color space
#if defined (__sh__)
enum {
	HDMI_RGB,
	HDMI_YUV,
	HDMI_422
};
#else
enum {
	HDMI_AUTO,
	HDMI_RGB,
	HDMI_ITU_R_BT_709,
	HDMI_UNKNOW
};
#endif

// wss
/*
off 
auto 
auto(4:3_off) 
4:3_full_format 
16:9_full_format 
14:9_letterbox_center 
14:9_letterbox_top 
16:9_letterbox_center 
16:9_letterbox_top 
>16:9_letterbox_center 
14:9_full_format
*/
enum {
	WSS_OFF,
	WSS_AUTO,
	WSS_43_OFF,
	WSS_43_FULL,
	WSS_169_FULL,
	WSS_149_LETTERBOX_CENTER,
	WSS_149_LETTERBOX_TOP,
	WSS_169_LETTERBOX_CENTER,
	WSS_169_LETTERBOX_TOP,
	WSS_169_LETTERBOX_CENTER_RIGHT,
	WSS_149_FULL
};

#ifdef USE_OPENGL
typedef enum
{
	VIDEO_FRAME_RATE_23_976 = 0,
	VIDEO_FRAME_RATE_24,
	VIDEO_FRAME_RATE_25,
	VIDEO_FRAME_RATE_29_97,
	VIDEO_FRAME_RATE_30,
	VIDEO_FRAME_RATE_50,
	VIDEO_FRAME_RATE_59_94,
	VIDEO_FRAME_RATE_60
} VIDEO_FRAME_RATE;

#define VDEC_MAXBUFS 0x40
#endif
class cVideo 
#ifdef USE_OPENGL
: public OpenThreads::Thread
#endif
{
	friend class GLThreadObj;
	
	private:
		int video_fd;
		int video_num;
		int video_adapter;

		video_play_state_t playstate;
		VIDEO_FORMAT StreamType;
		
#ifdef USE_OPENGL
		class SWFramebuffer : public std::vector<unsigned char>
		{
			public:
				SWFramebuffer() : mWidth(0), mHeight(0) {}
				void width(int w)
				{
					mWidth = w;
				}
				void height(int h)
				{
					mHeight = h;
				}
				void pts(uint64_t p)
				{
					mPts = p;
				}
				void AR(AVRational a)
				{
					mAR = a;
				}
				int width() const
				{
					return mWidth;
				}
				int height() const
				{
					return mHeight;
				}
				int64_t pts() const
				{
					return mPts;
				}
				AVRational AR() const
				{
					return mAR;
				}
			private:
				int mWidth;
				int mHeight;
				int64_t mPts;
				AVRational mAR;
		};
		int buf_in, buf_out, buf_num;
		
		void run();
		SWFramebuffer buffers[VDEC_MAXBUFS];
		int dec_w, dec_h;
		int dec_r;
		bool thread_running;
		OpenThreads::Mutex buf_m;
		OpenThreads::Mutex still_m;
		bool stillpicture;
		bool w_h_changed;
#endif	

	public:
		/* constructor & destructor */
		cVideo(int num = 0);
		~cVideo(void);

		/* aspect ratio */
		int getAspectRatio(void);
		void getPictureInfo(int &width, int &height, int &rate);
		int setAspectRatio(int ratio, int format);

		/* blank on freeze */
		int getBlank(void);
		int setBlank(int enable);

		/* get play state */
		int getPlayState(void);

		/* video stream source */
		int setSource(int source = VIDEO_SOURCE_DEMUX);

		//
		int Start(void);
		int Stop(bool blank = true);
		bool Pause(void);
		bool Resume(void);
				
		int Flush(void);
		int setSlowMotion(int repeat);
		int setFastForward(int skip);

		bool Open(CFrontend * fe = NULL);
		bool Close();
		
		/* set video_system */
		int SetVideoSystem(int video_system);

		int SetSpaceColour(int space_colour);
		
		void SetStreamType(VIDEO_FORMAT type);
		VIDEO_FORMAT GetStreamType(void) { return StreamType; };

		void SetSyncMode(int mode);

		void SetInput(int val);
		void SetStandby(int val);
		
		void Pig(int x, int y, int w, int h, int osd_w = DEFAULT_XRES, int osd_h = DEFAULT_YRES, int num = 0);

		void SetWideScreen(int val);
		void SetAnalogMode(int mode); //analog		
		
		int64_t GetPTS(void);
		
		// single pic
		int showSinglePic(const char *filename);
		void finishShowSinglePic();
		
		// psi
		void setContrast(int contrast);
		void setSaturation(int saturation);
		void setBrightness(int brightness);
		void setTint(int tint);
		
#ifdef USE_OPENGL
		SWFramebuffer *getDecBuf(void);
#endif
};

#endif

