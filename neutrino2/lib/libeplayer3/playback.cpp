/*
 * playback.cpp 12.12.2023 mohousch.
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
 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <time.h>

#define __STDC_CONSTANT_MACROS

////
extern "C" {
#include <libavutil/avutil.h>
//#if LIBAVCODEC_VERSION_MAJOR > 54
#include <libavutil/time.h>
//#endif
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_MAJOR > 54
#include <libavutil/opt.h>

#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <ao/ao.h>

#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#endif
}

#include <playback.h>
#include "misc.h"
//#include "aac.h"
//#include "pcm.h"


#define PLAYBACK_DEBUG
#define PLAYBACK_SILENT

static short debug_level = 10;

#ifdef PLAYBACK_DEBUG
#define playback_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define playback_printf(level, fmt, x...)
#endif

#ifndef PLAYBACK_SILENT
#define playback_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define playback_err(fmt, x...)
#endif

#define cERR_PLAYBACK_NO_ERROR      	0
#define cERR_PLAYBACK_ERROR        	-1

#define cMaxSpeed_ff   			128
#define cMaxSpeed_fr   			-320

//// ffmpeg
#if LIBAVCODEC_VERSION_MAJOR > 54
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif

// Error Constants
#define cERR_CONTAINER_FFMPEG_NO_ERROR        0
#define cERR_CONTAINER_FFMPEG_INIT           -1
#define cERR_CONTAINER_FFMPEG_NOT_SUPPORTED  -2
#define cERR_CONTAINER_FFMPEG_INVALID_FILE   -3
#define cERR_CONTAINER_FFMPEG_RUNNING        -4
#define cERR_CONTAINER_FFMPEG_NOMEM          -5
#define cERR_CONTAINER_FFMPEG_OPEN           -6
#define cERR_CONTAINER_FFMPEG_STREAM         -7
#define cERR_CONTAINER_FFMPEG_NULL           -8
#define cERR_CONTAINER_playback_err            -9
#define cERR_CONTAINER_FFMPEG_END_OF_FILE    -10

////
#ifdef USE_OPENGL
static ao_device *adevice = NULL;
//static ao_sample_format sformat;
//static AVCodecContext *c = NULL;
//static AVCodecParameters *p = NULL;
#endif

//
int CPlayBack::aac_get_sample_rate_index (uint32_t sample_rate)
{
	if (96000 <= sample_rate)
		return 0;
	else if (88200 <= sample_rate)
		return 1;
	else if (64000 <= sample_rate)
		return 2;
	else if (48000 <= sample_rate)
		return 3;
	else if (44100 <= sample_rate)
		return 4;
	else if (32000 <= sample_rate)
		return 5;
	else if (24000 <= sample_rate)
		return 6;
	else if (22050 <= sample_rate)
		return 7;
	else if (16000 <= sample_rate)
		return 8;
	else if (12000 <= sample_rate)
		return 9;
	else if (11025 <= sample_rate)
		return 10;
	else if (8000 <= sample_rate)
		return 11;
	else if (7350 <= sample_rate)
		return 12;
	else
	      return 13;
}

//
char* CPlayBack::Codec2Encoding(AVCodecContext *codec, int* version)
{
	switch (codec->codec_id)
	{
		case AV_CODEC_ID_MPEG1VIDEO:
			return "V_MPEG1";
			
		case AV_CODEC_ID_MPEG2VIDEO:
			return "V_MPEG2";
			
		case AV_CODEC_ID_H263:
		case AV_CODEC_ID_H263P:
		case AV_CODEC_ID_H263I:
			return "V_H263";
			
		case AV_CODEC_ID_FLV1:
			return "V_FLV";
			
		case AV_CODEC_ID_VP5:
    		case AV_CODEC_ID_VP6:
    		case AV_CODEC_ID_VP6F:
        		return "V_VP6";
        		
    		case AV_CODEC_ID_VP8:
        		return "V_VP8";
        
#if LIBAVCODEC_VERSION_MAJOR > 54
    		case AV_CODEC_ID_VP9:
        		return "V_VP9";
#endif
		
		case AV_CODEC_ID_MJPEG:
        		return "V_MJPEG";
        	
		case AV_CODEC_ID_RV30:
        		return "V_RV30";
        		
    		case AV_CODEC_ID_RV40:
        		return "V_RV40";
        		
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 21, 100)
    		case AV_CODEC_ID_AVS2:
        		return "V_AVS2";
#endif
			
		case AV_CODEC_ID_MPEG4:
			return "V_MPEG4";
			
#if LIBAVCODEC_VERSION_MAJOR < 53
		case CODEC_ID_XVID:
#endif
		case AV_CODEC_ID_MSMPEG4V1:
		case AV_CODEC_ID_MSMPEG4V2:
		case AV_CODEC_ID_MSMPEG4V3:
			return "V_DIVX3";
			
		case AV_CODEC_ID_WMV1:
			*version = 1;
			return "V_WMV";
			
		case AV_CODEC_ID_WMV2:
			*version = 2;
			return "V_WMV";
			
		case AV_CODEC_ID_WMV3:
			*version = 3;
			return "V_WMV";
			
		case AV_CODEC_ID_VC1:
			return "V_VC1";
			
		case AV_CODEC_ID_H264:
#if LIBAVCODEC_VERSION_MAJOR < 54
		case CODEC_ID_FFH264:
#endif
			return "V_MPEG4/ISO/AVC";
			
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(55, 92, 100)
    		case AV_CODEC_ID_HEVC:
        		return "V_HEVC";
#endif

		case AV_CODEC_ID_THEORA:
			return "V_THEORA";
		
		// audio
		case AV_CODEC_ID_MP2:
			return "A_MPEG/L3";
			
		case AV_CODEC_ID_MP3:
			return "A_MP3";
			
		case AV_CODEC_ID_AAC:
			return "A_AAC";
			
		case AV_CODEC_ID_AC3:
			return "A_AC3";
			
		case AV_CODEC_ID_EAC3:
			return "A_EAC3";
			
		case AV_CODEC_ID_DTS:
			return "A_DTS";			
			
		case AV_CODEC_ID_WMAV1:
		case AV_CODEC_ID_WMAV2:
		case AV_CODEC_ID_WMAPRO:
			return "A_WMA";
			
		case AV_CODEC_ID_VORBIS:
			//return "A_VORBIS"; //FIXME:
			return "A_IPCM";
			
		case AV_CODEC_ID_FLAC: //86030
			//return "A_FLAC"; //FIXME:
			return "A_IPCM";
			
		case AV_CODEC_ID_PCM_S8:
		case AV_CODEC_ID_PCM_U8:
		case AV_CODEC_ID_PCM_S16LE:
		case AV_CODEC_ID_PCM_S16BE:
		case AV_CODEC_ID_PCM_U16LE:
		case AV_CODEC_ID_PCM_U16BE:
		case AV_CODEC_ID_PCM_S24LE:
		case AV_CODEC_ID_PCM_S24BE:
		case AV_CODEC_ID_PCM_U24LE:
		case AV_CODEC_ID_PCM_U24BE:
		case AV_CODEC_ID_PCM_S32LE:
		case AV_CODEC_ID_PCM_S32BE:
		case AV_CODEC_ID_PCM_U32LE:
		case AV_CODEC_ID_PCM_U32BE:
			return	"A_IPCM"; 
			
		case AV_CODEC_ID_AMR_NB:
    		case AV_CODEC_ID_AMR_WB:
        		return "A_AMR";
			
		// subtitle
		case AV_CODEC_ID_SSA:
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 3, 100)
    		case AV_CODEC_ID_ASS:
#endif
        		return "S_TEXT/ASS";
        		
    		case AV_CODEC_ID_DVD_SUBTITLE:
    		case AV_CODEC_ID_MOV_TEXT:
    		case AV_CODEC_ID_TEXT: ///< raw UTF-8 text
        		return "S_TEXT/UTF-8";
        		
    		case AV_CODEC_ID_SRT:
        		return "S_TEXT/SRT";
        		
    		case AV_CODEC_ID_SUBRIP:
        		return "S_TEXT/SUBRIP"; 
        		
    		case AV_CODEC_ID_WEBVTT:
        		return "S_TEXT/WEBVTT";
        		
        	case AV_CODEC_ID_DVB_TELETEXT:
        		return "S_GRAPHIC/TELETEXT";
        		
    		case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        		return "S_GRAPHIC/PGS";
        		
    		case AV_CODEC_ID_DVB_SUBTITLE:
        		return "S_GRAPHIC/DVB";
        		
    		case AV_CODEC_ID_XSUB:
        		return "S_GRAPHIC/XSUB";

		default:
			if (codec->codec_type == AVMEDIA_TYPE_AUDIO)
				return "A_IPCM";
			else
				playback_err("ERROR! CODEC NOT FOUND -> %d\n",codec->codec_id);
	}
	
	return NULL;
}

int64_t CPlayBack::calcPts(AVStream* stream, AVPacket* packet)
{
	int64_t pts;

	if ((stream == NULL) || (packet == NULL))
	{
		playback_err("stream / packet null\n");
		return INVALID_PTS_VALUE;
	}

	if(packet->pts == AV_NOPTS_VALUE)
		pts = INVALID_PTS_VALUE;
	else if (avContext->start_time == AV_NOPTS_VALUE)
		pts = 90000.0 * (double)packet->pts * av_q2d(stream->time_base);
	else
		pts = 90000.0 * (((double)(packet->pts) * av_q2d(stream->time_base)) - (avContext->start_time / AV_TIME_BASE));

	if (pts & 0x8000000000000000ull)
		pts = INVALID_PTS_VALUE;

	return pts;
}

//
float CPlayBack::getDurationFromSSALine(unsigned char* line)
{
	int i,h,m,s,ms;
	char* Text = strdup((char*) line);
	char* ptr1;
	char* ptr[10];
	long int msec;

	ptr1 = Text;
	ptr[0] = Text;
	
	for (i = 0; i < 3 && *ptr1 != '\0'; ptr1++) 
	{
		if (*ptr1 == ',') 
		{
			ptr[++i] = ptr1+1;
			*ptr1 = '\0';
		}
	}

	sscanf(ptr[2],"%d:%d:%d.%d", &h, &m, &s, &ms);
	msec = (ms*10) + (s*1000) + (m*60*1000) + (h*24*60*1000);
	sscanf(ptr[1],"%d:%d:%d.%d", &h, &m, &s, &ms);
	msec -= (ms*10) + (s*1000) + (m*60*1000) + (h*24*60*1000);

	playback_printf(10, "%s %s %f\n", ptr[2], ptr[1], (float) msec / 1000.0);

	free(Text);
	
	return (float)msec/1000.0;
}

CPlayBack::Track_t *CPlayBack::getTrack(eTrackType_t _type, int pid)
{
	playback_printf(100, "CPlayBack::getTrack: type:%d index:%d\n", _type, pid);
	
	Track_t *track = NULL;
	
	for (int i = 0; i < (int)Tracks.size(); i++)
	{
		if (Tracks[i].type == _type /*&& Tracks[i].Index == pid*/)
		{
			track = new Track_t;
			*track = Tracks[i];
			break;
		}
	}
}

CPlayBack::Track_t *CPlayBack::getAudioTrack(int pid)
{
	playback_printf(100, "CPlayBack::getTrack: index:%d\n", pid);
	
	Track_t *track = NULL;
	
	for (int i = 0; i < (int)audioTracks.size(); i++)
	{
		if (audioTracks[i].Index == pid)
		{
			track = new Track_t;
			*track = Tracks[i];
			break;
		}
	}
			
	return track;
}

CPlayBack::Track_t *CPlayBack::getVideoTrack(int pid)
{
	playback_printf(100, "CPlayBack::getTrack: index:%d\n", pid);
	
	Track_t *track = NULL;
	
	for (int i = 0; i < (int)videoTracks.size(); i++)
	{
		if (videoTracks[i].Index == pid)
		{
			track = new Track_t;
			*track = Tracks[i];
			break;
		}
	}
			
	return track;
}

CPlayBack::Track_t *CPlayBack::getSubtitleTrack(int pid)
{
	playback_printf(100, "CPlayBack::getTrack: index:%d\n", pid);
	
	Track_t *track = NULL;
	
	for (int i = 0; i < (int)subtitleTracks.size(); i++)
	{
		if (subtitleTracks[i].Index == pid)
		{
			track = new Track_t;
			*track = Tracks[i];
			break;
		}
	}
			
	return track;
}

void CPlayBack::addTrack(Track_t &track)
{
	Tracks.push_back(track);
	
	if (track.type == eTypeAudio)
		audioTracks.push_back(track);
	else if (track.type == eTypeVideo)
		videoTracks.push_back(track);
	else if (track.type == eTypeSubtitle)
		subtitleTracks.push_back(track);
}

////
CPlayBack::CPlayBack()
{
	isFile = false;
	isHttp = false;
	isUPNP = false;
	isPlaying = false;
	isPaused = false;
	isForwarding = false;
	isSeeking = false;
	isCreationPhase = false;
	hasPlayThreadStarted = false;
	ffmpegInited = false;

	BackWard = 0;
	SlowMotion = 0;
	Speed = 0;
	AVSync = 0;

	isVideo = false;
	isAudio = false;    
	isSubtitle = false;
	
	uri = "";
	size = 0;
	
	Tracks.clear();
	audioTracks.clear();
	videoTracks.clear();
	subtitleTracks.clear();
	TrackCount = 0;
	CurrentTrack = 0;
	
	////
	avContext = NULL;
	latestPts = 0;
	
	////
	dec_w = 0;
	dec_h = 0;
	dec_r = 0;
	w_h_changed = false;
	curr_pts = 0;
}

////
int CPlayBack::playbackOpen(char *filename)
{
	playback_printf(10, "URI=%s\n", filename);

	uri = strdup(filename);

	if (!isPlaying) 
	{
		//
		if (!strncmp("file://", uri, 7)) 
		{
			char * extension = NULL;
			isFile = true;
			isHttp = false;
			isUPNP = false;

			getExtension(uri + 7, &extension);

			if(!extension)
				return cERR_PLAYBACK_ERROR;

			// init ffmpeg
			if (init(uri) < 0)
				return cERR_PLAYBACK_ERROR;

			free(extension);
		} 
		else if( (!strncmp("http://", uri, 7)) || (!strncmp("https://", uri, 8)) )
		{
			isFile = false;
			isHttp = true;
			isUPNP = false;

			if (init(uri) < 0)
				return cERR_PLAYBACK_ERROR;
		}
		else if (!strncmp("mms://", uri, 6) || !strncmp("rtsp://", uri, 7) || !strncmp("rtmp://", uri, 7)) 
		{
			isFile = false;
			isHttp = true;
			isUPNP = false;

			if (!strncmp("mms://", uri, 6)) 
			{
				// mms is in reality called rtsp, and ffmpeg expects this
				char * tUri = (char*)malloc(strlen(uri) + 2);
				strncpy(tUri+1, uri, strlen(uri)+1);
				strncpy(tUri, "rtsp", 4);
				free(uri);
				uri = strdup(tUri);
				
				free(tUri);
			}

			if (init(uri) < 0)
				return cERR_PLAYBACK_ERROR;
		}
		else if (!strncmp("upnp://", uri, 7)) 
		{
			char * extension = NULL;
			
			isFile = false;
			isHttp = false;
			isUPNP = true;

			uri += 7; // jump over upnp://

			getUPNPExtension(uri + 7, &extension);

			if(!extension)
				return cERR_PLAYBACK_ERROR;

			if (init(uri) < 0)
				return cERR_PLAYBACK_ERROR;

			free(extension);
		}
		else 
		{
			playback_err("Unknown stream!\n");
			return cERR_PLAYBACK_ERROR;
		}
	}
	else
	{
	    	playback_err("playback alread running\n");
	    	return cERR_PLAYBACK_ERROR;
	}
	
	dec_w = 0;
	dec_h = 0;
	dec_r = 0;
	w_h_changed = false;
	curr_pts = 0;

	playback_printf(10, "exiting with value 0\n");

	return 1;
}

int CPlayBack::playbackClose()
{
	int ret = cERR_PLAYBACK_NO_ERROR;

	playback_printf(10, "\n");

	Tracks.clear();
	audioTracks.clear();
	videoTracks.clear();
	subtitleTracks.clear();   

	isPaused     = false;
	isPlaying    = false;
	isForwarding = false;
	BackWard     = 0;
	SlowMotion   = 0;
	Speed        = 0;

	playback_printf(10, "exiting with value %d\n", ret);

	return ret;
}

int CPlayBack::playbackPlay()
{
	int ret = 1;

	playback_printf(10, "\n");
	
	isPlaying = true;

	if (!hasPlayThreadStarted) 
	{	
		hasPlayThreadStarted = (OpenThreads::Thread::start() == 0);

		if (!hasPlayThreadStarted) 
		{
			ret = cERR_CONTAINER_playback_err;
		}
		else 
		{
			isPlaying = true;
			playback_printf(10, "Created thread\n");
		}
	}
	else 
	{
		playback_printf(10, "A thread already exists!\n");

		ret = cERR_CONTAINER_playback_err;
	}

	playback_printf(10, "exiting with value %d\n", ret);

	return ret;
}

int CPlayBack::playbackStop()
{
	int ret = cERR_CONTAINER_FFMPEG_NO_ERROR;
	
	int wait_time = 20;

	playback_printf(10, "\n");
	
	isPlaying = false;

	while ( (hasPlayThreadStarted) && (--wait_time) > 0 ) 
	{
		playback_printf(10, "Waiting for ffmpeg thread to terminate itself, will try another %d times\n", wait_time);

		usleep(100000);
	}

	if (wait_time == 0) 
	{
		playback_err( "Timeout waiting for thread!\n");

		ret = cERR_CONTAINER_playback_err;

		usleep(100000);
	}

	mutex.lock();
	
	//
	if (hasPlayThreadStarted)
	{
		hasPlayThreadStarted = false;
		OpenThreads::Thread::join();
	}

	if (avContext != NULL) 
	{
#if LIBAVFORMAT_VERSION_MAJOR < 54
 		av_close_input_file(avContext);
 		avContext = NULL;
#else
		avformat_close_input(&avContext);
#endif		
	}

	mutex.unlock();

	playback_printf(10, "ret %d\n", ret);

	return ret;
}

//// init
int CPlayBack::init(char * filename)
{
	int n, err;

	playback_printf(10, ">\n");

	if (filename == NULL) 
	{
		playback_err("filename NULL\n");

		return cERR_CONTAINER_FFMPEG_NULL;
	}

	playback_printf(10, "filename %s\n", filename);

	mutex.lock();
	
	Tracks.clear();

	// initialize ffmpeg 
	avcodec_register_all();
	av_register_all();
	avformat_network_init();

#if LIBAVCODEC_VERSION_MAJOR < 54
	if ((err = av_open_input_file(&avContext, filename, NULL, 0, NULL)) != 0) 
#else
	if ((err = avformat_open_input(&avContext, filename, NULL, 0)) != 0)
#endif
	{
		playback_err("avformat_open_input failed %d (%s)\n", err, filename);

		mutex.unlock();
		
		return cERR_CONTAINER_FFMPEG_OPEN;
	}
	
	avContext->flags |= AVFMT_FLAG_GENPTS;

	if (strstr(filename, ":31339") || strstr(filename, ":8001/") || strstr(filename, ".ts"))
		avContext->max_analyze_duration = 5;

	// find stream info
#if LIBAVCODEC_VERSION_MAJOR < 54
	if (av_find_stream_info(avContext) < 0)
#else
	if (avformat_find_stream_info(avContext, NULL) < 0) 
#endif
	{
		playback_err("Error avformat_find_stream_info\n");
	}

	// dump format
#if LIBAVCODEC_VERSION_MAJOR < 54
	dump_format(avContext, 0, filename, 0);
#else
	av_dump_format(avContext, 0, filename, 0);
#endif

	playback_printf(10, "number streams %d\n", avContext->nb_streams);

	for ( n = 0; n < avContext->nb_streams; n++) 
	{
		Track_t track;
		AVStream * stream = avContext->streams[n];
		int version = 0;

		char * encoding = Codec2Encoding(stream->codec, &version);

		if (encoding != NULL)
			playback_printf(10, "%d. encoding = %s - version %d\n", n, encoding, version);

		//
		memset(&track, 0, sizeof(track));

		switch (stream->codec->codec_type) 
		{
			// video
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)	  
			case AVMEDIA_TYPE_VIDEO:
#else
			case CODEC_TYPE_VIDEO:
#endif        
			playback_printf(10, "CODEC_TYPE_VIDEO %d\n",stream->codec->codec_type);

			if (encoding != NULL) 
			{
				track.type           = eTypeVideo;
				track.version        = version;

				track.width          = stream->codec->width;
				track.height         = stream->codec->height;

				track.extraData      = stream->codec->extradata;
				track.extraSize      = stream->codec->extradata_size;

				track.frame_rate     = stream->r_frame_rate.num;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				double frame_rate = av_q2d(stream->r_frame_rate); // rational to double

				playback_printf(10, "frame_rate = %f\n", frame_rate);

				track.frame_rate = frame_rate * 1000.0;

				//FIXME: revise this
				if (track.frame_rate < 23970)
					track.TimeScale = 1001;
				else
					track.TimeScale = 1000;

				playback_printf(10, "bit_rate = %d\n",stream->codec->bit_rate);
				playback_printf(10, "flags = %d\n",stream->codec->flags);
				playback_printf(10, "frame_bits = %d\n",stream->codec->frame_bits);
				playback_printf(10, "time_base.den %d\n",stream->time_base.den);
				playback_printf(10, "time_base.num %d\n",stream->time_base.num);
				playback_printf(10, "frame_rate %d\n",stream->r_frame_rate.num);
				playback_printf(10, "TimeScale %d\n",stream->r_frame_rate.den);
				playback_printf(10, "frame_rate %d\n", track.frame_rate);
				playback_printf(10, "TimeScale %d\n", track.TimeScale);

				track.Name      = "und";
				track.Encoding  = encoding;
				track.Index     = n;

				track.stream    = stream;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					playback_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				// add track
				addTrack(track);
			}
			else 
			{
				playback_err("codec type video but codec unknown %d\n", stream->codec->codec_id);
			}
			break;
	    
			// audio codec
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)	  
			case AVMEDIA_TYPE_AUDIO:
#else
			case CODEC_TYPE_AUDIO:
#endif        
			playback_printf(10, "CODEC_TYPE_AUDIO %d\n",stream->codec->codec_type);

			if (encoding != NULL) 
			{
				track.type = eTypeAudio;
				
				// language description
#if LIBAVCODEC_VERSION_MAJOR < 54
				AVMetadataTag *lang;
#else
				AVDictionaryEntry *lang;
#endif

#if LIBAVCODEC_VERSION_MAJOR < 54
				lang = av_metadata_get(stream->metadata, "language", NULL, 0);
#else
				lang = av_dict_get(stream->metadata, "language", NULL, 0);
#endif

				if (lang)
					track.Name = strdup(lang->value);
				else
					track.Name = strdup("Stream");

				playback_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Index          = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					playback_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				// pcm
				if(!strncmp(encoding, "A_IPCM", 6))
				{
					track.inject_as_pcm = 1;
					playback_printf(10, " Handle inject_as_pcm = %d\n", track.inject_as_pcm);

					//
					AVCodec *codec = avcodec_find_decoder(stream->codec->codec_id);

#if LIBAVCODEC_VERSION_MAJOR < 54
					if(codec != NULL && !avcodec_open(stream->codec, codec))
#else
					if(codec != NULL && !avcodec_open2(stream->codec, codec, NULL))
#endif					  
						printf("AVCODEC__INIT__SUCCESS\n");
					else
						printf("AVCODEC__INIT__FAILED\n");
				}
				// aac
				else if(stream->codec->codec_id == AV_CODEC_ID_AAC) 
				{
					playback_printf(10,"Create AAC ExtraData\n");
					playback_printf(10,"stream->codec->extradata_size %d\n", stream->codec->extradata_size);

					 /* extradata
					13 10 56 e5 9d 48 00 (anderen cops)
						object_type: 00010 2 = LC
						sample_rate: 011 0 6 = 24000
						chan_config: 0010 2 = Stereo
						000 0
						1010110 111 = 0x2b7
						00101 = SBR
						1
						0011 = 48000
						101 01001000 = 0x548
						ps = 0
						0000000
					*/

					unsigned int object_type = 2; // LC
					unsigned int sample_index = aac_get_sample_rate_index(stream->codec->sample_rate);
					unsigned int chan_config = stream->codec->channels;
					
					if(stream->codec->extradata_size >= 2) 
					{
						object_type = stream->codec->extradata[0] >> 3;
						sample_index = ((stream->codec->extradata[0] & 0x7) << 1) + (stream->codec->extradata[1] >> 7);
						chan_config = (stream->codec->extradata[1] >> 3) && 0xf;
					}

					playback_printf(10,"aac object_type %d\n", object_type);
					playback_printf(10,"aac sample_index %d\n", sample_index);
					playback_printf(10,"aac chan_config %d\n", chan_config);

					object_type -= 1; // Cause of ADTS

					track.aacbuflen = AAC_HEADER_LENGTH;
					track.aacbuf = (uint8_t *)malloc(8);
					track.aacbuf[0] = 0xFF;
					track.aacbuf[1] = 0xF1;
					track.aacbuf[2] = ((object_type & 0x03) << 6)  | (sample_index << 2) | ((chan_config >> 2) & 0x01);
					track.aacbuf[3] = (chan_config & 0x03) << 6;
					track.aacbuf[4] = 0x00;
					track.aacbuf[5] = 0x1F;
					track.aacbuf[6] = 0xFC;

					track.have_aacheader = 1;
				} 
				// wma
				else if(stream->codec->codec_id == AV_CODEC_ID_WMAV1 || stream->codec->codec_id == AV_CODEC_ID_WMAV2 || stream->codec->codec_id == AV_CODEC_ID_WMAPRO ) //if (stream->codec->extradata_size > 0)
				{
					playback_printf(10,"Create WMA ExtraData\n");
					
					track.aacbuflen = 104 + stream->codec->extradata_size;
					track.aacbuf = (uint8_t *)malloc(track.aacbuflen);
					memset (track.aacbuf, 0, track.aacbuflen);
					
					unsigned char ASF_Stream_Properties_Object[16] = {0x91,0x07,0xDC,0xB7,0xB7,0xA9,0xCF,0x11,0x8E,0xE6,0x00,0xC0,0x0C,0x20,0x53,0x65};
					
					memcpy(track.aacbuf + 0, ASF_Stream_Properties_Object, 16); // ASF_Stream_Properties_Object
					memcpy(track.aacbuf + 16, &track.aacbuflen, 4); //FrameDateLength

					unsigned int sizehi = 0;
					memcpy(track.aacbuf + 20, &sizehi, 4); // sizehi (not used)

					unsigned char ASF_Audio_Media[16] = {0x40,0x9E,0x69,0xF8,0x4D,0x5B,0xCF,0x11,0xA8,0xFD,0x00,0x80,0x5F,0x5C,0x44,0x2B};
					memcpy(track.aacbuf + 24, ASF_Audio_Media, 16); //ASF_Audio_Media

					unsigned char ASF_Audio_Spread[16] = {0x50,0xCD,0xC3,0xBF,0x8F,0x61,0xCF,0x11,0x8B,0xB2,0x00,0xAA,0x00,0xB4,0xE2,0x20};
					memcpy(track.aacbuf + 40, ASF_Audio_Spread, 16); //ASF_Audio_Spread

					memset(track.aacbuf + 56, 0, 4); // time_offset (not used)
					memset(track.aacbuf + 60, 0, 4); // time_offset_hi (not used)

					unsigned int type_specific_data_length = 18 + stream->codec->extradata_size;
					memcpy(track.aacbuf + 64, &type_specific_data_length, 4); //type_specific_data_length

					unsigned int error_correction_data_length = 8;
					memcpy(track.aacbuf + 68, &error_correction_data_length, 4); //error_correction_data_length

					unsigned short flags = 1; // stream_number
					memcpy(track.aacbuf + 72, &flags, 2); //flags

					unsigned int reserved = 0;
					memcpy(track.aacbuf + 74, &reserved, 4); // reserved

// type_specific_data
#define WMA_VERSION_1           0x160
#define WMA_VERSION_2_9         0x161
#define WMA_VERSION_9_PRO       0x162
#define WMA_LOSSLESS            0x163
					unsigned short codec_id = 0;
					switch(stream->codec->codec_id) 
					{
						//TODO: What code for lossless ?
						case AV_CODEC_ID_WMAPRO:
							codec_id = WMA_VERSION_9_PRO;
							break;
						case AV_CODEC_ID_WMAV2:
							codec_id = WMA_VERSION_2_9 ;
							break;
						case AV_CODEC_ID_WMAV1:
						default:
							codec_id = WMA_VERSION_1;
							break;
					}
					
					memcpy(track.aacbuf + 78, &codec_id, 2); //codec_id

					unsigned short number_of_channels = stream->codec->channels;
					memcpy(track.aacbuf + 80, &number_of_channels, 2); //number_of_channels

					unsigned int samples_per_second = stream->codec->sample_rate;
					playback_printf(10, "samples_per_second = %d\n", samples_per_second);
					memcpy(track.aacbuf + 82, &samples_per_second, 4); //samples_per_second

					unsigned int average_number_of_bytes_per_second = stream->codec->bit_rate / 8;
					playback_printf(10, "average_number_of_bytes_per_second = %d\n", average_number_of_bytes_per_second);
					memcpy(track.aacbuf + 86, &average_number_of_bytes_per_second, 4); //average_number_of_bytes_per_second

					unsigned short block_alignment = stream->codec->block_align;
					playback_printf(10, "block_alignment = %d\n", block_alignment);
					memcpy(track.aacbuf + 90, &block_alignment, 2); //block_alignment

					unsigned short bits_per_sample = stream->codec->sample_fmt>=0?(stream->codec->sample_fmt+1)*8:8;
					playback_printf(10, "bits_per_sample = %d (%d)\n", bits_per_sample, stream->codec->sample_fmt);
					memcpy(track.aacbuf + 92, &bits_per_sample, 2); //bits_per_sample

					memcpy(track.aacbuf + 94, &stream->codec->extradata_size, 2); //bits_per_sample

					memcpy(track.aacbuf + 96, stream->codec->extradata, stream->codec->extradata_size);
		    
					playback_printf(10, "aacbuf:\n");

					track.have_aacheader = 1;
				}

				//
				//Tracks.push_back(&track);
				addTrack(track);
			}
			else 
			{
				playback_err("codec type audio but codec unknown %d\n", stream->codec->codec_id);
			}
			break;
	    
			// subtitle
			case AVMEDIA_TYPE_SUBTITLE:
			{
				track.type = eTypeSubtitle;
				
#if LIBAVCODEC_VERSION_MAJOR < 54
				AVMetadataTag * lang;
#else
				AVDictionaryEntry * lang;
#endif
				playback_printf(10, "CODEC_TYPE_SUBTITLE %d\n",stream->codec->codec_type);

#if LIBAVCODEC_VERSION_MAJOR < 54
				lang = av_metadata_get(stream->metadata, "language", NULL, 0);
#else
				lang = av_dict_get(stream->metadata, "language", NULL, 0);
#endif	     

				if (lang)
					track.Name        = strdup(lang->value);
				else
					track.Name        = strdup("und");

				playback_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Index          = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				track.width          = -1; /* will be filled online from videotrack */
				track.height         = -1; /* will be filled online from videotrack */

				track.extraData      = stream->codec->extradata;
				track.extraSize      = stream->codec->extradata_size;

				playback_printf(10, "subtitle codec %d\n", stream->codec->codec_id);
				playback_printf(10, "subtitle width %d\n", stream->codec->width);
				playback_printf(10, "subtitle height %d\n", stream->codec->height);
				playback_printf(10, "subtitle stream %p\n", stream);

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					playback_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				if (track.Name)
				{
					playback_printf(10, "FOUND SUBTITLE %s\n", track.Name);
				}

				//
				//Tracks.push_back(&track);
				addTrack(track);
				
				break;
			} 
        
			// all others
			case AVMEDIA_TYPE_UNKNOWN:
			case AVMEDIA_TYPE_DATA:
			case AVMEDIA_TYPE_ATTACHMENT:
			case AVMEDIA_TYPE_NB:
			default:
				playback_err("not handled or unknown codec_type %d\n", stream->codec->codec_type);
				break;	 	 
		} // switch (stream->codec->codec_type)

	} // for

	// init
	latestPts = 0;
	ffmpegInited = true;
	
	mutex.unlock();

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

//// play thread
void CPlayBack::run()
{
	AVPacket   packet;
	off_t currentReadPosition = 0; // last read position
	off_t lastReverseSeek = 0;     // max address to read before seek again in reverse play 
	off_t lastSeek = -1;
	long long int lastPts = -1, currentVideoPts = -1, currentAudioPts = -1, showtime = 0, bofcount = 0;
	int err = 0, gotlastPts = 0, audioMute = 0;
	AudioVideoOut_t avOut;

	// Softdecoding buffer
	AVFrame *samples = NULL;
	
#ifdef USE_OPENGL
	//// video
	AVFrame *frame = NULL;
	AVFrame *rgbframe = NULL;
	struct SwsContext *convert = NULL;
	
	time_t warn_r = 0;
	time_t warn_d = 0;
	
	int av_ret = 0;
	
	frame = av_frame_alloc();
	rgbframe = av_frame_alloc();
	
	//av_init_packet(&packet);
	
	//// audio
	/*
	AVCodec *codec;
	AVFrame *aframe;
	AVCodecParameters *p = NULL;
	int ret, driver;
	//int av_ret = 0;
	// libao
	ao_info *ai;
	// resample
	SwrContext *swr = NULL;
	uint8_t *obuf = NULL;
	int obuf_sz = 0; 	// in samples
	int obuf_sz_max = 0;
	int o_ch, o_sr; 	// output channels and sample rate
	uint64_t o_layout; 	// output channels layout
	char tmp[64] = "unknown";
	
	//
	av_init_packet(&packet);
	
	//
	//p = avContext->streams[0]->codecpar;
	
	aframe = av_frame_alloc();
	
	//
	p = avContext->streams[0]->codecpar;
	
	o_ch = p->channels;     	// 2
	o_sr = p->sample_rate;      	// 48000
	o_layout = p->channel_layout;   // AV_CH_LAYOUT_STEREO
	
	//
	driver = ao_default_driver_id();
	sformat.bits = 16;
	sformat.channels = o_ch;
	sformat.rate = o_sr;
	sformat.byte_format = AO_FMT_NATIVE;
	sformat.matrix = 0;
		
	if (adevice)
		ao_close(adevice);
			
	adevice = ao_open_live(driver, &sformat, NULL);
	ai = ao_driver_info(driver);
	
	playback_printf(10, "changed params ch %d srate %d bits %d adevice %p\n", o_ch, o_sr, 16, adevice);
	
	swr = swr_alloc_set_opts(swr,
	        o_layout, AV_SAMPLE_FMT_S16, o_sr,         		// output
	        p->channel_layout, 0, p->sample_rate,  	// input
	        0, NULL);
	        
	if (! swr)
	{
		playback_printf(10, "cAudio::run: could not alloc resample context\n");
		//goto out3;
	}
	swr_init(swr);
	*/
#endif

	playback_printf(10, "\n");

	while ( isCreationPhase )
	{
		playback_err("Thread waiting for end of init phase...\n");
		usleep(1000);
	}
	
	playback_printf(10, "Running!\n");

	while ( isPlaying  && ffmpegInited) 
	{
		//
		if (isPaused) 
		{
			playback_printf(20, "paused\n");

			usleep(100000);
			continue;
		}

		if (isSeeking) 
		{
			playback_printf(10, "seeking\n");

			usleep(100000);
			continue;
		}

		if (BackWard && av_gettime() >= showtime)
		{
			audioMute = 1;
			
			// clear dvb
			//context->output->Command(context, OUTPUT_CLEAR, "v");

			if(bofcount == 1)
			{
				showtime = av_gettime();
				usleep(100000);
				continue;
			}

			if(lastPts == -1)
			{
				if(currentVideoPts != -1)
					lastPts = currentVideoPts;
				else
					lastPts = currentAudioPts;
			}

			// seek
/*
			if((err = container_ffmpeg_seek_rel(context, lastSeek, lastPts, (float) context->playback->Speed)) < 0)
			{
				playback_err( "Error seeking\n");

				if (err == cERR_CONTAINER_FFMPEG_END_OF_FILE)
				{
					bofcount = 1;
				}
			}
*/

			lastPts = lastPts + (Speed * 90000);
			showtime = av_gettime() + 300000; //jump back all 300ms
		}

		if(!BackWard && audioMute)
		{
			lastPts = -1;
			bofcount = 0;
			showtime = 0;
			audioMute = 0;
			
			// mute
			//context->output->Command(context, OUTPUT_AUDIOMUTE, "0");
		}

		//
		mutex.lock();

		if (av_read_frame(avContext, &packet) == 0 )
		{
			int64_t pts;
			Track_t * videoTrack = NULL;
			Track_t * audioTrack = NULL;	    
			Track_t * subtitleTrack = NULL;    

			int index = packet.stream_index;

#if LIBAVCODEC_VERSION_MAJOR < 54
			currentReadPosition = url_ftell(avContext->pb);
#else
			currentReadPosition = avio_tell(avContext->pb);
#endif

			//
			videoTrack = getTrack(eTypeVideo);
			audioTrack = getTrack(eTypeAudio);
			subtitleTrack = getTrack(eTypeSubtitle); 

			playback_printf(100, "packet.size %d - index %d track.size:%d\n", packet.size, index, Tracks.size());

			// video
			if (videoTrack != NULL) 
			{
				playback_printf(100, "videoTrack: index:%d\n", index);
				
				if (videoTrack->Index == index) 
				{
					currentVideoPts = videoTrack->pts = pts = calcPts(videoTrack->stream, &packet);

					if ((currentVideoPts > latestPts) && (currentVideoPts != INVALID_PTS_VALUE))
						latestPts = currentVideoPts;

					playback_printf(100, "VideoTrack index = %d %lld\n",index, currentVideoPts);

					avOut.data       = packet.data;
					avOut.len        = packet.size;
					avOut.pts        = pts;
					avOut.extradata  = videoTrack->extraData;
					avOut.extralen   = videoTrack->extraSize;
					avOut.frameRate  = videoTrack->frame_rate;
					avOut.timeScale  = videoTrack->TimeScale;
					avOut.width      = videoTrack->width;
					avOut.height     = videoTrack->height;
					avOut.type       = "video";

#ifdef USE_OPENGL
					if (writeVideoData(videoTrack, &packet) != 0)
					{
						playback_printf(100, "writing data to video data failed\n");
					}					
#endif
				}
			}

			// audio
			if (audioTrack != NULL) 
			{
				if (audioTrack->Index == index) 
				{
					currentAudioPts = audioTrack->pts = pts = calcPts(audioTrack->stream, &packet);

					if ((currentAudioPts > latestPts) && (!videoTrack))
						latestPts = currentAudioPts;
					
					////	
					curr_pts = pts;

					playback_printf(100, "AudioTrack index = %d\n", index);
					
#ifdef USE_OPENGL
					if (writeAudioData(audioTrack, &packet) != 0)
					{
						playback_printf(100, "writing data to audio data failed\n");
					}	
#else
					//
					if (audioTrack->inject_as_pcm == 1)
					{
						AVCodecContext *c = (audioTrack->stream)->codec;
						int bytesDone = 0;
						
						
						if(samples == NULL)
							samples = av_frame_alloc();
						else
							av_frame_unref(samples);

						while(packet.size > 0)
						{
							//
							int decoded_data_size = 0; //AVCODEC_MAX_AUDIO_FRAME_SIZE;
							
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 37, 100)
							bytesDone = avcodec_decode_audio4(c, samples, &decoded_data_size, &packet);
#else
							bytesDone = avcodec_send_packet(c, &packet);
							
             						if (bytesDone < 0 && bytesDone != AVERROR(EAGAIN) && bytesDone != AVERROR_EOF) 
             						{
            						} 
            						else 
            						{
             							bytesDone = avcodec_receive_frame(c, samples);
             						}
#endif

							if(bytesDone < 0) // Error Happend
							    break;

							packet.data += bytesDone;
							packet.size -= bytesDone;

							if(decoded_data_size <= 0)
							    continue;

							pcmPrivateData_t extradata;
							extradata.uNoOfChannels = ((AVStream*) audioTrack->stream)->codec->channels;
							extradata.uSampleRate = ((AVStream*) audioTrack->stream)->codec->sample_rate;
							extradata.uBitsPerSample = 16;
							extradata.bLittleEndian = 1;
							
							extradata.avCodecId = ((AVStream*) audioTrack->stream)->codec->codec_id;

							avOut.data       = (uint8_t *)samples;
							avOut.len        = decoded_data_size;
							avOut.pts        = pts;
							avOut.extradata  = (uint8_t *) &extradata;
							avOut.extralen   = sizeof(extradata);
							avOut.frameRate  = 0;
							avOut.timeScale  = 0;
							avOut.width      = 0;
							avOut.height     = 0;
							avOut.type       = "audio";

							if (!CPlayBack::BackWard)
							{
								playback_err("writing data to audio device failed\n");
							}
						}
					}
					else if (audioTrack->have_aacheader == 1)
					{
						playback_printf(200,"write audio aac\n");

						avOut.data       = packet.data;
						avOut.len        = packet.size;
						avOut.pts        = pts;
						avOut.extradata  = audioTrack->aacbuf;
						avOut.extralen   = audioTrack->aacbuflen;
						avOut.frameRate  = 0;
						avOut.timeScale  = 0;
						avOut.width      = 0;
						avOut.height     = 0;
						avOut.type       = "audio";

						if (!CPlayBack::BackWard)
						{
							playback_err("(aac) writing data to audio device failed\n");
						}
					}
					else
					{
						avOut.data       = packet.data;
						avOut.len        = packet.size;
						avOut.pts        = pts;
						avOut.extradata  = NULL;
						avOut.extralen   = 0;
						avOut.frameRate  = 0;
						avOut.timeScale  = 0;
						avOut.width      = 0;
						avOut.height     = 0;
						avOut.type       = "audio";

						if (!CPlayBack::BackWard)
						{
							playback_err("writing data to audio device failed\n");
						}
					}
#endif
				}
			}

			// subtitle
			if (subtitleTrack != NULL) 
			{
				if (subtitleTrack->Index == index) 
				{
					float duration = 3.0;
					playback_printf(10, "subtitleTrack->stream %p \n", subtitleTrack->stream);

					pts = calcPts((AVStream *)subtitleTrack->stream, &packet);

					if ((pts > latestPts) && (!videoTrack) && (!audioTrack))
						latestPts = pts;

					playback_printf(20, "Packet duration %d\n", packet.duration);
					playback_printf(20, "Packet convergence_duration %lld\n", packet.convergence_duration);

					if(packet.duration != 0 && packet.duration != AV_NOPTS_VALUE )
						duration=((float)packet.duration)/1000.0;
					else if(packet.convergence_duration != 0 && packet.convergence_duration != AV_NOPTS_VALUE )
						duration=((float)packet.convergence_duration)/1000.0;		    
					else if(subtitleTrack->stream->codec->codec_id == AV_CODEC_ID_SSA)
					{
						duration = getDurationFromSSALine(packet.data);
					} 

					//
					if (duration > 0 || duration == -1)
					{
						SubtitleData_t data;
						
						playback_printf(100, "videoPts %lld\n", currentVideoPts);
						
						data.avCodecId = subtitleTrack->stream->codec->codec_id;
						data.data      = packet.data;
						data.len       = packet.size;
						data.extradata = subtitleTrack->extraData;
						data.extralen  = subtitleTrack->extraSize;
						data.pts       = pts;
						data.duration  = duration;
						data.width     = subtitleTrack->width;
		           			data.height    = subtitleTrack->height;
						
						playback_err("writing data to subtitle failed\n");
					}
				}
			}            

			if (packet.data)
				av_free_packet(&packet);
		}
		else  
		{
		    	playback_err("no data ->end of file reached ? \n");
		    	
		    	mutex.unlock();
		    	break;
		}
				
		mutex.unlock();
	} // while

	//
	if (samples != NULL) 
	{
		av_frame_free(&samples);
	}
	
#ifdef USE_OPENGL
	if (convert)
	{
		sws_freeContext(convert);
		convert = NULL;
	}
	
	if (frame)
	{
		av_frame_free(&frame);
		frame = NULL;
	}
	
	if (rgbframe)
	{
		av_frame_free(&rgbframe);
		rgbframe = NULL;
	}
	
	////
	//if (adevice)
	//	ao_close(adevice);
		
	//adevice = NULL;
	//ao_shutdown();
#endif

	hasPlayThreadStarted = false;
	isPlaying = false;

	playback_printf(10, "terminating\n");
}

#ifdef USE_OPENGL
CPlayBack::SWFramebuffer *CPlayBack::getDecBuf(void)
{
	buf_m.lock();
	
	SWFramebuffer *p = &buffers[0];

	buf_m.unlock();
	
	return p;
}

//
void CPlayBack::getRate(int &rate) 
{	
	switch (dec_r)
	{
		case 23://23.976fps
			rate = FRAME_RATE_23_976;
			break;
		case 24:
			rate = FRAME_RATE_24;
			break;
		case 25:
			rate = FRAME_RATE_25;
			break;
		case 29://29,976fps
			rate = FRAME_RATE_29_97;
			break;
		case 30:
			rate = FRAME_RATE_30;
			break;
		case 50:
			rate = FRAME_RATE_50;
			break;
		case 60:
			rate = FRAME_RATE_60;
			break;
		default:
			rate = dec_r;
			break;
	}
}

int CPlayBack::writeVideoData(Track_t *track, AVPacket *packet)
{
	int av_ret = -1;
	
	AVFrame *frame = NULL;
	AVFrame *rgbframe = NULL;
	struct SwsContext *convert = NULL;
	AVCodec *codec = NULL;
	
	time_t warn_r = 0;
	time_t warn_d = 0;
	
	frame = av_frame_alloc();
	rgbframe = av_frame_alloc();
	
	codec = avcodec_find_decoder(track->stream->codec->codec_id);
					
	// init avcontext
	av_ret = avcodec_open2(track->stream->codec, codec, NULL);
		
	//
	int got_frame = 0;
	
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
	av_ret = avcodec_decode_video2(track->stream->codec, frame, &got_frame, packet);
		
	if (av_ret < 0)
	{
		if (warn_d - time(NULL) > 4)
		{
			warn_d = time(NULL);
		}
	}
#else
	av_ret = avcodec_send_packet(track->stream->codec, packet);
		
	if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
	{
		if (warn_d - time(NULL) > 4)
		{
			warn_d = time(NULL);
		}
	}
		
	av_ret = avcodec_receive_frame(track->stream->codec, frame);
	
	if (av_ret == 0)
		got_frame = 1;
#endif
					
	// setup swsscaler
	if (got_frame)
	{
		unsigned int need = av_image_get_buffer_size(AV_PIX_FMT_RGB32, track->stream->codec->width, track->stream->codec->height, 1);
						
		convert = sws_getContext(track->stream->codec->width, track->stream->codec->height, track->stream->codec->pix_fmt, track->stream->codec->width, track->stream->codec->height, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
							
		if (convert)
		{
			buf_m.lock();
				
			SWFramebuffer *f = &buffers[0];
				
			if (f->size() < need)
				f->resize(need);
								
			av_image_fill_arrays(rgbframe->data, rgbframe->linesize, &(*f)[0], AV_PIX_FMT_RGB32, track->stream->codec->width, track->stream->codec->height, 1);

			sws_scale(convert, frame->data, frame->linesize, 0, track->stream->codec->height, rgbframe->data, rgbframe->linesize);
			
			//
			if (dec_w != track->stream->codec->width || dec_h != track->stream->codec->height)
			{
				playback_printf(10, "CPlayBack::run: pic changed %dx%d -> %dx%d\n", dec_w, dec_h, track->stream->codec->width, track->stream->codec->height);
				dec_w = track->stream->codec->width;
				dec_h = track->stream->codec->height;
				w_h_changed = true;
			}
							
			f->width(track->stream->codec->width);
			f->height(track->stream->codec->height);	
			
			//
#if (LIBAVUTIL_VERSION_MAJOR < 54)
			track->pts = av_frame_get_best_effort_timestamp(frame);
#else
			track->pts = frame->best_effort_timestamp;
#endif

			// a/v delay determined experimentally :-)
			if (track->stream->codec->codec_id == AV_CODEC_ID_MPEG2VIDEO)
				track->pts += 90000 * 4 / 10; // 400ms
			else
				track->pts += 90000 * 3 / 10; // 300ms

			f->pts(track->pts);
							
			AVRational a = av_guess_sample_aspect_ratio(avContext, avContext->streams[0], frame);
							
			f->AR(a);
							
			//dec_r = track->stream->codec->time_base.den / (track->stream->codec->time_base.num * track->stream->codec->ticks_per_frame);
			dec_r = track->frame_rate/1000;
							
			buf_m.unlock();
		}
	}
					
	//av_packet_unref(packet);
	
	if (frame)
	{
		av_frame_free(&frame);
		frame = NULL;
	}
	
	if (rgbframe)
	{
		av_frame_free(&rgbframe);
		rgbframe = NULL;
	}
	
	if (convert)
	{
		sws_freeContext(convert);
		convert = NULL;
	}
	
	return av_ret;	
}

int CPlayBack::writeAudioData(Track_t *track, AVPacket* packet)
{
	int av_ret = -1;
	
	//curr_pts = track->pts;
	
	#if 0
	AVFrame * aframe = NULL;
	AVCodecContext *ctx = audioTrack->stream->codec;
	int got_frame = 0;
					
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
	avcodec_decode_audio4(ctx, aframe, &got_frame, &packet);
#else
	av_ret = avcodec_send_packet(ctx, &packet);
		
	if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
	{
		playback_printf(200, "%s: avcodec_send_packet %d\n", __func__, av_ret);
	}
	else
	{
		av_ret = avcodec_receive_frame(ctx, aframe);
						
		if (av_ret != 0 && av_ret != AVERROR(EAGAIN))
		{
			playback_printf(200,"%s: avcodec_send_packet %d\n", __func__, av_ret);
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
		obuf_sz = av_rescale_rnd(swr_get_delay(swr, p->sample_rate) + aframe->nb_samples, o_sr, p->sample_rate, AV_ROUND_UP);

		if (obuf_sz > obuf_sz_max)
		{
			av_free(obuf);
							
			if (av_samples_alloc(&obuf, &out_linesize, o_ch, aframe->nb_samples, AV_SAMPLE_FMT_S16, 1) < 0)
			{
				//av_packet_unref(&packet);
				//break; // while (thread_started)
				return -1;
			}
							
			obuf_sz_max = obuf_sz;
		}
						
		obuf_sz = swr_convert(swr, &obuf, obuf_sz, (const uint8_t **)aframe->extended_data, frame->nb_samples);
						
#if (LIBAVUTIL_VERSION_MAJOR < 54)
		curr_pts = av_frame_get_best_effort_timestamp(frame);
#else
		curr_pts = frame->best_effort_timestamp;
#endif
		int o_buf_sz = av_samples_get_buffer_size(&out_linesize, o_ch, obuf_sz, AV_SAMPLE_FMT_S16, 1);
						
		if (o_buf_sz > 0)
			ao_play(adevice, (char *)obuf, o_buf_sz);
	}
	#endif
	
	return av_ret;
}
#endif

