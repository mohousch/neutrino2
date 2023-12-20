/*
 * playback.h 12.12.2023 mohousch.
 * based on libeplayer3
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
 
#ifndef PLAYBACK_H
#define PLAYBACK_H
 
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
 
#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>
 
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/rational.h>
}

#include <dirent.h>

#include <config.h>


//// 
#ifdef USE_OPENGL
typedef enum
{
	FRAME_RATE_23_976 = 0,
	FRAME_RATE_24,
	FRAME_RATE_25,
	FRAME_RATE_29_97,
	FRAME_RATE_30,
	FRAME_RATE_50,
	FRAME_RATE_59_94,
	FRAME_RATE_60
} FRAME_RATE;

#define VDEC_MAXBUFS 				0x40
#endif

//// defines
#define AAC_HEADER_LENGTH       		7

#define INVALID_PTS_VALUE                       0x200000000ull

//#define BIG_READS
#if defined (BIG_READS)
#define BLOCK_COUNT                             8
#else
#define BLOCK_COUNT                             1
#endif
#define TP_PACKET_SIZE                          188
#define BD_TP_PACKET_SIZE                       192
#define NUMBER_PACKETS                          (199*BLOCK_COUNT)
#define BUFFER_SIZE                             (TP_PACKET_SIZE*NUMBER_PACKETS)
#define PADDING_LENGTH                          (1024*BLOCK_COUNT)

// subtitle hacks ->for file subtitles
#define TEXTSRTOFFSET 				100
#define TEXTSSAOFFSET 				200

typedef ssize_t (* WriteV_t)(int, const struct iovec *, int);
 
class CPlayBack : public OpenThreads::Thread
{
#ifdef USE_OPENGL
	friend class GLThreadObj;
#endif

 	public:
 		typedef enum 
		{
			eTypeAudio,
			eTypeVideo,
			eTypeSubtitle
		} eTrackType_t;

		typedef struct Track_s 
		{
			char *                Name;
			char *                Encoding;
			int                   Index;
			//
			char*                 language;
			int64_t               duration;
			unsigned int          frame_rate;
			unsigned int          TimeScale;
			int                   version;
			int64_t               pts;
			//
			eTrackType_t          type;
			int                   width;
			int                   height;
			//
			AVStream              * stream;
			//
			uint8_t               * extraData;
			int		      extraSize;
			//
			uint8_t               * aacbuf;
			unsigned int          aacbuflen;
			int                   have_aacheader;
			//
			int                   inject_as_pcm;
		} Track_t;
		
		typedef std::vector<Track_t> TrackList;
		
		TrackList Tracks;
		TrackList audioTracks;
		TrackList videoTracks;
		TrackList subtitleTracks;
		
		void addTrack(Track_t &track);
		Track_t *getTrack(eTrackType_t _type, int pid = 0);
		Track_t *getAudioTrack(int pid = 0);
		Track_t *getVideoTrack(int pid = 0);
		Track_t *getSubtitleTrack(int pid = 0);
		
		//
		typedef struct pcmPrivateData_s
		{
			int uNoOfChannels;
			int uSampleRate;
			int uBitsPerSample;
			int bLittleEndian;
			
			//
			uint8_t bResampling;
			enum AVCodecID avCodecId;
			
		} pcmPrivateData_t;
		
		typedef struct BitPacker_s
		{
			unsigned char*      Ptr;
			unsigned int        BitBuffer;
			int                 Remaining;
		} BitPacker_t;
		
		////
		typedef enum 
		{ 
			eNone, 
			eAudio, 
			eVideo, 
			eGfx
		} eWriterType_t;
		
		typedef struct 
		{
			//
			int                    fd;
			unsigned char*         data;
			unsigned int           len;
			unsigned long long int Pts;
			unsigned char*         private_data;
			unsigned int           private_size;
			unsigned int           FrameRate;
			unsigned int           FrameScale;
			unsigned int           Width;
			unsigned int           Height;
			unsigned char          Version;
			WriteV_t               WriteV;
		} WriterAVCallData_t;
		
		typedef struct 
		{
			unsigned char*         data;
			unsigned int           Width;
			unsigned int           Height;
			unsigned int           Stride;
			unsigned int           color;

			unsigned int           x;       /* dst x ->given by ass */
			unsigned int           y;       /* dst y ->given by ass */

			/* destination values if we use a shared framebuffer */
			int                    fd;
			unsigned int           Screen_Width;
			unsigned int           Screen_Height;
			unsigned char*         destination;
			unsigned int           destStride;
		} WriterFBCallData_t;
		
		typedef struct WriterCaps_s 
		{
			char*          name;
			eWriterType_t  type;
			char*          textEncoding;
			int            dvbEncoding;
		} WriterCaps_t;

		typedef struct Writer_s 
		{
			int           (* reset) ();
			int           (* writeData) (void*);
			int           (* writeReverseData) (void*);
			WriterCaps_t *caps;
		} Writer_t;
		
		////
		typedef struct
		{
			//
			unsigned char*         data;
			unsigned int           len;

			unsigned char*         extradata;
			unsigned int           extralen;
			
			unsigned long long int pts;
			
			float                  frameRate;
			unsigned int           timeScale;
			
			unsigned int           width;
			unsigned int           height;
			
			char*                  type;
		} AudioVideoOut_t;
		
		//
		typedef struct
		{
			//
			enum AVCodecID 	avCodecId;
			
			//
			unsigned char* data;
			int            len;

			unsigned char* extradata;
			int            extralen;
			
			long long int  pts;
			float          duration;
			
			int          width;
		    	int          height;
		} SubtitleData_t;

		typedef struct
		{
			unsigned char* destination;
			unsigned int   screen_width;
			unsigned int   screen_height;
			unsigned int   destStride;
			
			int            shareFramebuffer;
			int            framebufferFD;
		} SubtitleOutputDef_t;

 	private:
#ifdef USE_OPENGL
		int64_t curr_pts;
		
		////
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
		//int buf_in, buf_out, buf_num;
		
		SWFramebuffer buffers[VDEC_MAXBUFS];
		int dec_w;
		int dec_h;
		int dec_r;
		bool w_h_changed;
		OpenThreads::Mutex buf_m;
		int writeVideoData(Track_t *track, AVPacket* packet);
		int writeAudioData(Track_t *track, AVPacket* packet);
#endif
 		bool isFile;
		bool isHttp;
		bool isUPNP;

		bool isPlaying;
		bool isPaused;
		bool isForwarding;
		bool isSeeking;
		bool isCreationPhase;
		bool ffmpegInited;

		float BackWard;
		int SlowMotion;
		int Speed;
		int AVSync;

		bool isVideo;
		bool isAudio;    
		bool isSubtitle;
		
		char * uri;
		off_t size;
		
		//
		int TrackCount;
		int CurrentTrack;
		
		////
		AVFormatContext *avContext;
		int64_t latestPts;
		
		bool hasPlayThreadStarted;
		
		////
		void PutBits(BitPacker_t * ld, unsigned int code, unsigned int length);
		void FlushBits(BitPacker_t * ld);
		void getExtension(char * FILENAMEname, char ** extension);
		void getUPNPExtension(char * FILENAMEname, char ** extension) ;
		char * basename(char * name);
		char * dirname(char * name);
		uint32_t ReadUint32(uint8_t *buffer);
		uint16_t ReadUInt16(uint8_t *buffer);
		int aac_get_sample_rate_index (uint32_t sample_rate);
		char* Codec2Encoding(AVCodecContext *codec, int* version);
		int64_t calcPts(AVStream* stream, AVPacket* packet);
		float getDurationFromSSALine(unsigned char* line);
		int init(char * filename);
		
		////
		void run();
		OpenThreads::Mutex mutex;
		
	public:
		CPlayBack();
		virtual ~CPlayBack(){};
		
		////
		int playbackOpen(char *filename); 
		int playbackClose();
		int playbackPlay();
//		int playbackPause();
//		int playbackContinue();
		int playbackStop();
//		int playbackTerminate();
//		int playbackFastForward();
//		int playbackFastBackward(int *speed);
//		int playbackSlowmotion(int *speed);
//		int playbackSeek(float *pos);
//		int playbackGetFrameCount(uint64_t frameCount);
//		int playbackPts(uint64_t *pts);
//		int playbackLength(double length);
//		int playbackSwitchAudio(int *track);
//		int playbackSwitchSubtitle(int *track);
//		int playbackInfo(char **infoString);

#ifdef USE_OPENGL
		SWFramebuffer *getDecBuf(void);
		int64_t getPts(){return curr_pts;};
		void getRate(int &rate);
#endif
 };
 
 #endif 
 
