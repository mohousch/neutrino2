/*
 * Container handling for all stream's handled by ffmpeg
 * konfetti 2010; based on code from crow
 *
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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <pthread.h>

#include <libavutil/avutil.h>
#if LIBAVCODEC_VERSION_MAJOR > 54
#include <libavutil/time.h>
#endif
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_MAJOR > 54
#include <libavutil/opt.h>
#endif

#include "common.h"
#include "misc.h"
#include "debug.h"
#include "aac.h"
#include "pcm.h"
#include "ffmpeg_metadata.h"
#include "subtitle.h"

#include <config.h>

#if defined (USE_OPENGL)
#include <ao/ao.h>

static ao_device *adevice = NULL;
static ao_sample_format sformat;
#endif


#if LIBAVCODEC_VERSION_MAJOR > 54
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define FFMPEG_DEBUG

#ifdef FFMPEG_DEBUG

static short debug_level = 10;

#define ffmpeg_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define ffmpeg_printf(level, fmt, x...)
#endif

#ifndef FFMPEG_SILENT
#define ffmpeg_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define ffmpeg_err(fmt, x...)
#endif

/* Error Constants */
#define cERR_CONTAINER_FFMPEG_NO_ERROR        0
#define cERR_CONTAINER_FFMPEG_INIT           -1
#define cERR_CONTAINER_FFMPEG_NOT_SUPPORTED  -2
#define cERR_CONTAINER_FFMPEG_INVALID_FILE   -3
#define cERR_CONTAINER_FFMPEG_RUNNING        -4
#define cERR_CONTAINER_FFMPEG_NOMEM          -5
#define cERR_CONTAINER_FFMPEG_OPEN           -6
#define cERR_CONTAINER_FFMPEG_STREAM         -7
#define cERR_CONTAINER_FFMPEG_NULL           -8
#define cERR_CONTAINER_FFMPEG_ERR            -9
#define cERR_CONTAINER_FFMPEG_END_OF_FILE    -10

static const char* FILENAME = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static pthread_mutex_t mutex;

static pthread_t PlayThread;
static int hasPlayThreadStarted = 0;

static AVFormatContext *avContext = NULL;

static unsigned char isContainerRunning = 0;

static long long int latestPts = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */
static int container_ffmpeg_seek_bytes(off_t pos);
static int container_ffmpeg_seek(Context_t *context, float sec);
static int container_ffmpeg_seek_rel(Context_t *context, off_t pos, long long int pts, float sec);
static int container_ffmpeg_seek_bytes_rel(off_t start, off_t bytes);

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
void getMutex(const char *filename, const char *function, int line) 
{
	ffmpeg_printf(100, "::%d requesting mutex\n", line);

	pthread_mutex_lock(&mutex);

	ffmpeg_printf(100, "::%d received mutex\n", line);
}

void releaseMutex(const char *filename, const const char *function, int line) 
{
	pthread_mutex_unlock(&mutex);

	ffmpeg_printf(100, "::%d released mutex\n", line);
}

static char* Codec2Encoding(AVCodecContext *codec, int* version)
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
			
		case AV_CODEC_ID_RV10:
		case AV_CODEC_ID_RV20:
			return "V_RMV";
			
		case AV_CODEC_ID_MPEG4:
#if LIBAVCODEC_VERSION_MAJOR < 53
		case CODEC_ID_XVID:
#endif
		case AV_CODEC_ID_MSMPEG4V1:
		case AV_CODEC_ID_MSMPEG4V2:
		case AV_CODEC_ID_MSMPEG4V3:
			return "V_MSCOMP";
			
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
		
		case AV_CODEC_ID_AVS:
			return "V_AVS";
			
		case AV_CODEC_ID_MP2:
			return "A_MPEG/L3";
			
		case AV_CODEC_ID_MP3:
			return "A_MP3";
			
		case AV_CODEC_ID_AAC:
			return "A_AAC";
			
		case AV_CODEC_ID_AC3:
			return "A_AC3";
			
		case AV_CODEC_ID_DTS:
			return "A_DTS";
		
//#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52, 72, 2)			
//		case AV_CODEC_ID_EAC3:
//			return "A_EAC3";
//#endif			
			
		case AV_CODEC_ID_WMAV1:
		case AV_CODEC_ID_WMAV2:
		case 86056: //CODEC_ID_WMAPRO
			return "A_WMA";
			
		case AV_CODEC_ID_MLP:
			return "A_MLP";
			
		case AV_CODEC_ID_RA_144:
			return "A_RMA";
			
		case AV_CODEC_ID_RA_288:
			return "A_RMA";
			
		case AV_CODEC_ID_VORBIS:
			return "A_IPCM"; //return "A_VORBIS";
			
		case AV_CODEC_ID_FLAC: //86030
			return "A_IPCM"; //return "A_FLAC";
			
		/* subtitle */
		case AV_CODEC_ID_SSA:
			return "S_TEXT/ASS"; /* Hellmaster1024: seems to be ASS instead of SSA */
			
		case AV_CODEC_ID_TEXT: /* Hellmaster1024: i dont have most of this, but lets hope it is normal text :-) */
		case AV_CODEC_ID_DVD_SUBTITLE:
		case AV_CODEC_ID_DVB_SUBTITLE:
		case AV_CODEC_ID_XSUB:
		case AV_CODEC_ID_MOV_TEXT:
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(56, 72, 2)      
		case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
#endif

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,38,1)
		case AV_CODEC_ID_DVB_TELETEXT:
#endif      
			return "S_TEXT/SRT"; /* fixme */
		
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52, 72, 2)
		case AV_CODEC_ID_SRT:
			return "S_TEXT/SRT"; /* fixme */
#endif 

		default:
			if (codec->codec_type == AVMEDIA_TYPE_AUDIO)
				return "A_IPCM";
			else
				ffmpeg_err("ERROR! CODEC NOT FOUND -> %d\n",codec->codec_id);
	}
	
	return NULL;
}

long long int calcPts(AVStream* stream, AVPacket* packet)
{
	long long int pts;

	if ((stream == NULL) || (packet == NULL))
	{
		ffmpeg_err("stream / packet null\n");
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

/*Hellmaster1024: get the Duration of the subtitle from the SSA line*/
float getDurationFromSSALine(unsigned char* line)
{
	int i,h,m,s,ms;
	char* Text = strdup((char*) line);
	char* ptr1;
	char* ptr[10];
	long int msec;

	ptr1 = Text;
	ptr[0]=Text;
	for (i=0; i < 3 && *ptr1 != '\0'; ptr1++) 
	{
		if (*ptr1 == ',') 
		{
			ptr[++i]=ptr1+1;
			*ptr1 = '\0';
		}
	}

	sscanf(ptr[2],"%d:%d:%d.%d",&h,&m,&s,&ms);
	msec = (ms*10) + (s*1000) + (m*60*1000) + (h*24*60*1000);
	sscanf(ptr[1],"%d:%d:%d.%d",&h,&m,&s,&ms);
	msec -= (ms*10) + (s*1000) + (m*60*1000) + (h*24*60*1000);

	ffmpeg_printf(10, "%s %s %f\n", ptr[2], ptr[1], (float) msec / 1000.0);

	free(Text);
	return (float)msec/1000.0;
}

/* search for metatdata in context and stream
 * and map it to our metadata.
 */
#if LIBAVCODEC_VERSION_MAJOR < 54
static char* searchMeta(AVMetadata *metadata, char* ourTag)
#else
static char* searchMeta(AVDictionary * metadata, char* ourTag)
#endif
{
#if LIBAVCODEC_VERSION_MAJOR < 54
	AVMetadataTag *tag = NULL;
#else
	AVDictionaryEntry *tag = NULL;
#endif
	int i = 0;

	while (metadata_map[i] != NULL)
	{
		if (strcmp(ourTag, metadata_map[i]) == 0)
		{
#if LIBAVCODEC_VERSION_MAJOR < 54
			while ((tag = av_metadata_get(metadata, "", tag, AV_METADATA_IGNORE_SUFFIX)))
#else
			while ((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
#endif
			{
				if (strcmp(tag->key, metadata_map[ i + 1 ]) == 0)
				{
					return tag->value;
				}
			}
		}
		i++;
	}

	return NULL;
}

/* **************************** */
/* Worker Thread                */
/* **************************** */
static void FFMPEGThread(Context_t *context) 
{
	AVPacket   packet;
	off_t currentReadPosition = 0; /* last read position */
	off_t lastReverseSeek = 0;     /* max address to read before seek again in reverse play */
	off_t lastSeek = -1;
	long long int lastPts = -1, currentVideoPts = -1, currentAudioPts = -1, showtime = 0, bofcount = 0;
	int           err = 0, gotlastPts = 0, audioMute = 0;
	AudioVideoOut_t avOut;

	/* Softdecoding buffer*/
	unsigned char *samples = NULL;

	ffmpeg_printf(10, "\n");

	while ( context->playback->isCreationPhase )
	{
		ffmpeg_err("Thread waiting for end of init phase...\n");
		usleep(1000);
	}
	ffmpeg_printf(10, "Running!\n");

	while ( context && context->playback && context->playback->isPlaying ) 
	{
		//IF MOVIE IS PAUSED, WAIT
		if (context->playback->isPaused) 
		{
			ffmpeg_printf(20, "paused\n");

			usleep(100000);
			continue;
		}

		if (context->playback->isSeeking) 
		{
			ffmpeg_printf(10, "seeking\n");

			usleep(100000);
			continue;
		}

#define reverse_playback_3
#ifdef reverse_playback_3
		if (context->playback->BackWard && av_gettime() >= showtime)
		{
			audioMute = 1;
			context->output->Command(context, OUTPUT_CLEAR, "v");

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


			if((err = container_ffmpeg_seek_rel(context, lastSeek, lastPts, (float) context->playback->Speed)) < 0)
			{
				ffmpeg_err( "Error seeking\n");

				if (err == cERR_CONTAINER_FFMPEG_END_OF_FILE)
				{
					bofcount = 1;
				}
			}

			lastPts = lastPts + (context->playback->Speed * 90000);
			showtime = av_gettime() + 300000; //jump back all 300ms
		}

		if(!context->playback->BackWard && audioMute)
		{
			lastPts = -1;
			bofcount = 0;
			showtime = 0;
			audioMute = 0;
			context->output->Command(context, OUTPUT_AUDIOMUTE, "0");
		}
#endif

#ifdef reverse_playback_2
		/* should we seek back again ?
		* reverse play and currentReadPosition >= end of seek reverse play area ? */
		if ((context->playback->BackWard) && (currentReadPosition >= lastReverseSeek))
		{
			/* fixme: surplus detection */
			int surplus = 1;

			ffmpeg_printf(20, "new seek ->c %lld, l %lld, ls %lld, lp %lld\n", currentReadPosition, lastReverseSeek, lastSeek, lastPts);

			context->output->Command(context, OUTPUT_DISCONTINUITY_REVERSE, &surplus);

			/* save the maximum read position, if we reach this, we must
			* seek back again.
			*/
			if(lastReverseSeek == 0)
				lastReverseSeek = currentReadPosition;
			else
				lastReverseSeek = lastSeek;

#define use_sec_to_seek
#if defined(use_sec_to_seek)
			if ((err = container_ffmpeg_seek_rel(context, lastSeek, lastPts, -5)) < 0)
#else
			if ((err = container_ffmpeg_seek_bytes_rel(lastSeek, /* context->playback->BackWard */ -188 * 200)) < 0)
#endif
			{
				ffmpeg_err( "Error seeking\n");

				if (err == cERR_CONTAINER_FFMPEG_END_OF_FILE)
				{
					break;
				}
			}
			else
			{
#if LIBAVCODEC_VERSION_MAJOR < 54
				lastSeek = currentReadPosition = url_ftell(avContext->pb);
#else
				lastSeek = currentReadPosition = avio_tell(avContext->pb);
#endif
				gotlastPts = 1;

#ifndef use_sec_to_seek
				if (err != lastSeek)
					ffmpeg_err("upssssssssssssssss seek not doing what I want\n");
#endif

				/*
				if (currentVideoPts != -1)
					lastPts = currentVideoPts;
				else
					lastPts = currentAudioPts;
				*/
			}
		} 
		else if (!context->playback->BackWard)
		{
			lastReverseSeek = 0;
			lastSeek = -1;
			lastPts = -1;
			gotlastPts = 0;
		}
#endif
		getMutex(FILENAME, __FUNCTION__,__LINE__);

#define use_read_frame
#ifdef use_read_frame
		if (av_read_frame(avContext, &packet) == 0 )
#else
		if (av_read_packet(avContext, &packet) == 0 )
#endif
		{
			long long int pts;
			Track_t * videoTrack = NULL;
			Track_t * audioTrack = NULL;	    
			Track_t * subtitleTrack = NULL;    

			int index = packet.stream_index;

#if LIBAVCODEC_VERSION_MAJOR < 54
			currentReadPosition = url_ftell(avContext->pb);
#else
			currentReadPosition = avio_tell(avContext->pb);
#endif

			if (context->manager->video->Command(context, MANAGER_GET_TRACK, &videoTrack) < 0)
				ffmpeg_err("error getting video track\n");

			if (context->manager->audio->Command(context, MANAGER_GET_TRACK, &audioTrack) < 0)
				ffmpeg_err("error getting audio track\n");
			
			if (context->manager->subtitle->Command(context, MANAGER_GET_TRACK, &subtitleTrack) < 0)
				ffmpeg_err("error getting subtitle track\n");    

			ffmpeg_printf(200, "packet.size %d - index %d\n", packet.size, index);

			// video
			if (videoTrack != NULL) 
			{
				if (videoTrack->Id == index) 
				{
					currentVideoPts = videoTrack->pts = pts = calcPts(videoTrack->stream, &packet);

					if ((currentVideoPts > latestPts) && (currentVideoPts != INVALID_PTS_VALUE))
						latestPts = currentVideoPts;

#ifdef reverse_playback_2
					if (currentVideoPts != INVALID_PTS_VALUE && gotlastPts == 1)
					{
						lastPts = currentVideoPts;
						gotlastPts = 0;
					}
#endif

					ffmpeg_printf(200, "VideoTrack index = %d %lld\n",index, currentVideoPts);

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

					if (context->output->video->Write(context, &avOut) < 0) 
					{
						ffmpeg_err("writing data to video device failed\n");
					}
				}
			}

			// audio
			if (audioTrack != NULL) 
			{
				if (audioTrack->Id == index) 
				{
					currentAudioPts = audioTrack->pts = pts = calcPts(audioTrack->stream, &packet);

					if ((currentAudioPts > latestPts) && (!videoTrack))
						latestPts = currentAudioPts;

#ifdef reverse_playback_2
					if (currentAudioPts != INVALID_PTS_VALUE && gotlastPts == 1 && (!videoTrack))
					{
						lastPts = currentAudioPts;
						gotlastPts = 0;
					}
#endif

					ffmpeg_printf(200, "AudioTrack index = %d\n",index);

#if defined (USE_OPENGL)
					int driver = ao_default_driver_id();

					sformat.bits = 16;
					sformat.channels = ((AVStream*) audioTrack->stream)->codec->channels;
					sformat.rate = ((AVStream*) audioTrack->stream)->codec->sample_rate;
					sformat.byte_format = AO_FMT_LITTLE;
					sformat.matrix = 0;
					//if (adevice)
					//	ao_close(adevice);
					adevice = ao_open_live(driver, &sformat, NULL);
					ao_info *ai = ao_driver_info(driver);
					
					//ffmpeg_printf(10, "\nbits:%d channels:%d rate:%d\n", sformat.bits, sformat.channels, sformat.rate);
					ffmpeg_printf(10, "libao driver: %d name '%s' short '%s' author '%s'\n", driver, ai->name, ai->short_name, ai->author);

					if (ao_play(adevice, (char *)avOut.data, avOut.len) == 0)
						ffmpeg_err("writing data to audio device failed\n");
#else

					if (audioTrack->inject_as_pcm == 1)
					{
						int      bytesDone = 0;
						unsigned int samples_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
						AVPacket avpkt;
						avpkt = packet;

						// This way the buffer is only allocated if we really need it
						if(samples == NULL)
							samples = (unsigned char *)malloc(samples_size);

						while(avpkt.size > 0)
						{
							int decoded_data_size = samples_size;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 1, 99)
							bytesDone = avcodec_decode_audio4(( (AVStream*) audioTrack->stream)->codec, (short *)(samples), &decoded_data_size, &avpkt);
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
							bytesDone = avcodec_decode_audio3(( (AVStream*) audioTrack->stream)->codec, (short *)(samples), &decoded_data_size, &avpkt);
#else
							bytesDone = avcodec_decode_audio2(( (AVStream*) audioTrack->stream)->codec, (short *)(samples), &decoded_data_size, avpkt.data, avpkt.size);
#endif

							if(bytesDone < 0) // Error Happend
							    break;

							avpkt.data += bytesDone;
							avpkt.size -= bytesDone;

							if(decoded_data_size <= 0)
							    continue;

							pcmPrivateData_t extradata;
							extradata.uNoOfChannels = ((AVStream*) audioTrack->stream)->codec->channels;
							extradata.uSampleRate = ((AVStream*) audioTrack->stream)->codec->sample_rate;
							extradata.uBitsPerSample = 16;
							extradata.bLittleEndian = 1;

							avOut.data       = samples;
							avOut.len        = decoded_data_size;

							avOut.pts        = pts;
							avOut.extradata  = (unsigned char *) &extradata;
							avOut.extralen   = sizeof(extradata);
							avOut.frameRate  = 0;
							avOut.timeScale  = 0;
							avOut.width      = 0;
							avOut.height     = 0;
							avOut.type       = "audio";

#ifdef reverse_playback_3
							if (!context->playback->BackWard)
#endif
							if (context->output->audio->Write(context, &avOut) < 0)
								ffmpeg_err("writing data to audio device failed\n");
						}
					}
					else if (audioTrack->have_aacheader == 1)
					{
						ffmpeg_printf(200,"write audio aac\n");

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

#ifdef reverse_playback_3
						if (!context->playback->BackWard)
#endif
						if (context->output->audio->Write(context, &avOut) < 0)
						{
							ffmpeg_err("(aac) writing data to audio device failed\n");
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

#ifdef reverse_playback_3
						if (!context->playback->BackWard)
#endif
						if (context->output->audio->Write(context, &avOut) < 0)
						{
							ffmpeg_err("writing data to audio device failed\n");
						}
					}
#endif
				}
			}

			// subtitle
			if (subtitleTrack != NULL) 
			{
				if (subtitleTrack->Id == index) 
				{
					float duration=3.0;
					ffmpeg_printf(100, "subtitleTrack->stream %p \n", subtitleTrack->stream);

					pts = calcPts(subtitleTrack->stream, &packet);

					if ((pts > latestPts) && (!videoTrack) && (!audioTrack))
						latestPts = pts;

					/*Hellmaster1024: in mkv the duration for ID_TEXT is stored in convergence_duration */
					ffmpeg_printf(20, "Packet duration %d\n", packet.duration);
					ffmpeg_printf(20, "Packet convergence_duration %lld\n", packet.convergence_duration);

					if(packet.duration != 0 && packet.duration != AV_NOPTS_VALUE )
						duration=((float)packet.duration)/1000.0;
					else if(packet.convergence_duration != 0 && packet.convergence_duration != AV_NOPTS_VALUE )
						duration=((float)packet.convergence_duration)/1000.0;		    
					else if(((AVStream*)subtitleTrack->stream)->codec->codec_id == AV_CODEC_ID_SSA)
					{
						/*Hellmaster1024 if the duration is not stored in packet.duration or
						  packet.convergence_duration we need to calculate it any other way, for SSA it is stored in
						  the Text line*/
						duration = getDurationFromSSALine(packet.data);
					} 
					else 
					{
						/* no clue yet */
					}

					/* konfetti: I've found cases where the duration from getDurationFromSSALine
					* is zero (start end and are really the same in text). I think it make's
					* no sense to pass those.
					*/
					if (duration > 0.0)
					{
						/* is there a decoder ? */
						if (avcodec_find_decoder(((AVStream*) subtitleTrack->stream)->codec->codec_id) != NULL)
						{
							AVSubtitle sub;
							int got_sub_ptr;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)			   
							if (avcodec_decode_subtitle2(((AVStream*) subtitleTrack->stream)->codec, &sub, &got_sub_ptr, &packet) < 0)
#else
							if (avcodec_decode_subtitle( ((AVStream*) subtitleTrack->stream)->codec, &sub, &got_sub_ptr, packet.data, packet.size ) < 0)
#endif
							{
							    ffmpeg_err("error decoding subtitle\n");
							} 
							else
							{
								int i;

								ffmpeg_printf(0, "format %d\n", sub.format);
								ffmpeg_printf(0, "start_display_time %d\n", sub.start_display_time);
								ffmpeg_printf(0, "end_display_time %d\n", sub.end_display_time);
								ffmpeg_printf(0, "num_rects %d\n", sub.num_rects);
								//ffmpeg_printf(0, "pts %lld\n", sub.pts);

								for (i = 0; i < sub.num_rects; i++)
								{

									ffmpeg_printf(0, "x %d\n", sub.rects[i]->x);
									ffmpeg_printf(0, "y %d\n", sub.rects[i]->y);
									ffmpeg_printf(0, "w %d\n", sub.rects[i]->w);
									ffmpeg_printf(0, "h %d\n", sub.rects[i]->h);
									ffmpeg_printf(0, "nb_colors %d\n", sub.rects[i]->nb_colors);
									ffmpeg_printf(0, "type %d\n", sub.rects[i]->type);
									ffmpeg_printf(0, "text %s\n", sub.rects[i]->text);
									ffmpeg_printf(0, "ass %s\n", sub.rects[i]->ass);
									//pict ->AVPicture
								}
							}
						}
						else if(((AVStream*)subtitleTrack->stream)->codec->codec_id == AV_CODEC_ID_SSA)
						{
							SubtitleData_t data;

							ffmpeg_printf(10, "videoPts %lld\n", currentVideoPts);

							data.data      = packet.data;
							data.len       = packet.size;
							data.extradata = subtitleTrack->extraData;
							data.extralen  = subtitleTrack->extraSize;
							data.pts       = pts;
							data.duration  = duration;

							context->container->assContainer->Command(context, CONTAINER_DATA, &data);
						}
						else
						{
							/* hopefully native text ;) */

							unsigned char* line = text_to_ass((char *)packet.data,pts/90,duration);
							ffmpeg_printf(50,"text line is %s\n",(char *)packet.data);
							ffmpeg_printf(50,"Sub line is %s\n",line);
							ffmpeg_printf(20, "videoPts %lld %f\n", currentVideoPts,currentVideoPts/90000.0);
							SubtitleData_t data;
							data.data      = line;
							data.len       = strlen((char*)line);
							data.extradata = (unsigned char *) DEFAULT_ASS_HEAD;
							data.extralen  = strlen(DEFAULT_ASS_HEAD);
							data.pts       = pts;
							data.duration  = duration;

							context->container->assContainer->Command(context, CONTAINER_DATA, &data);
							free(line);
						}
					} /* duration */
				}
			}            

			if (packet.data)
				av_free_packet(&packet);
		}
		else  
		{
		    ffmpeg_err("no data ->end of file reached ? \n");
		    releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		    break;
		}
		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
	} /* while */

	// Freeing the allocated buffer for softdecoding
	if (samples != NULL) 
	{
		free(samples);
		samples = NULL;
	}

	hasPlayThreadStarted = 0;

	ffmpeg_printf(10, "terminating\n");
}

/* **************************** */
/* Container part for ffmpeg    */
/* **************************** */
int container_ffmpeg_init(Context_t *context, char * filename)
{
	int n, err;

	ffmpeg_printf(10, ">\n");

	if (filename == NULL) 
	{
		ffmpeg_err("filename NULL\n");

		return cERR_CONTAINER_FFMPEG_NULL;
	}

	if (context == NULL) 
	{
		ffmpeg_err("context NULL\n");

		return cERR_CONTAINER_FFMPEG_NULL;
	}

	ffmpeg_printf(10, "filename %s\n", filename);

	getMutex(FILENAME, __FUNCTION__,__LINE__);

	if (isContainerRunning) 
	{
		ffmpeg_err("ups already running?\n");
		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		return cERR_CONTAINER_FFMPEG_RUNNING;
	}

	// initialize ffmpeg 
	avcodec_register_all();
	av_register_all();

#if defined (USE_OPENGL)
	ao_initialize();
#endif

#if LIBAVCODEC_VERSION_MAJOR < 54
	if ((err = av_open_input_file(&avContext, filename, NULL, 0, NULL)) != 0) 
#else
	if ((err = avformat_open_input(&avContext, filename, NULL, 0)) != 0)
#endif
	{
		char error[512];

#if LIBAVCODEC_VERSION_MAJOR < 54
		ffmpeg_err("av_open_input_file failed %d (%s)\n", err, filename);
#else
		ffmpeg_err("avformat_open_input failed %d (%s)\n", err, filename);
#endif
		//av_strerror(err, error, 512);
		ffmpeg_err("Cause: %s\n", error);

		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		return cERR_CONTAINER_FFMPEG_OPEN;
	}
	
	avContext->flags |= AVFMT_FLAG_GENPTS;

	if (strstr(filename, ":31339") || strstr(filename, ":8001/"))
		avContext->max_analyze_duration = 5;
	
	if ( strstr(filename, ".ts") )
		avContext->max_analyze_duration = 5;

	ffmpeg_printf(20, "find_streaminfo\n");

#if LIBAVCODEC_VERSION_MAJOR < 54
	if (av_find_stream_info(avContext) < 0) 
	{
		ffmpeg_err("Error av_find_stream_info\n");
#else
	if (avformat_find_stream_info(avContext, NULL) < 0) 
	{
		ffmpeg_err("Error avformat_find_stream_info\n");
#endif

#ifdef this_is_ok
		/* crow reports that sometimes this returns an error
		* but the file is played back well. so remove this
		* until other works are done and we can prove this.
		*/
#if LIBAVFORMAT_VERSION_MAJOR < 54
 		av_close_input_file(avContext);
#else
		avformat_close_input(&avContext);
#endif		
		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		return cERR_CONTAINER_FFMPEG_STREAM;
#endif
	}

	ffmpeg_printf(20, "dump format\n");

#if LIBAVCODEC_VERSION_MAJOR < 54
	dump_format(avContext, 0, filename, 0);
#else
	av_dump_format(avContext, 0, filename, 0);
#endif

	ffmpeg_printf(1, "number streams %d\n", avContext->nb_streams);

	for ( n = 0; n < avContext->nb_streams; n++) 
	{
		Track_t track;
		AVStream * stream = avContext->streams[n];
		int version = 0;

		char * encoding = Codec2Encoding(stream->codec, &version);

		if (encoding != NULL)
			ffmpeg_printf(1, "%d. encoding = %s - version %d\n", n, encoding, version);

		/* 
		some values in track are unset and therefor copyTrack segfaults.
		so set it by default to NULL!
		*/
		memset(&track, 0, sizeof(track));

		switch (stream->codec->codec_type) 
		{
			// video
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)	  
			case AVMEDIA_TYPE_VIDEO:
#else
			case CODEC_TYPE_VIDEO:
#endif        
			ffmpeg_printf(10, "CODEC_TYPE_VIDEO %d\n",stream->codec->codec_type);

			if (encoding != NULL) 
			{
				track.type           = eTypeES;
				track.version        = version;

				track.width          = stream->codec->width;
				track.height         = stream->codec->height;

				track.extraData      = stream->codec->extradata;
				track.extraSize      = stream->codec->extradata_size;

				track.frame_rate     = stream->r_frame_rate.num;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				double frame_rate = av_q2d(stream->r_frame_rate); /* rational to double */

				ffmpeg_printf(10, "frame_rate = %f\n", frame_rate);

				track.frame_rate = frame_rate * 1000.0;

				/* fixme: revise this */
				if (track.frame_rate < 23970)
					track.TimeScale = 1001;
				else
					track.TimeScale = 1000;

				ffmpeg_printf(10, "bit_rate = %d\n",stream->codec->bit_rate);
				ffmpeg_printf(10, "flags = %d\n",stream->codec->flags);
				ffmpeg_printf(10, "frame_bits = %d\n",stream->codec->frame_bits);
				ffmpeg_printf(10, "time_base.den %d\n",stream->time_base.den);
				ffmpeg_printf(10, "time_base.num %d\n",stream->time_base.num);
				ffmpeg_printf(10, "frame_rate %d\n",stream->r_frame_rate.num);
				ffmpeg_printf(10, "TimeScale %d\n",stream->r_frame_rate.den);

				ffmpeg_printf(10, "frame_rate %d\n", track.frame_rate);
				ffmpeg_printf(10, "TimeScale %d\n", track.TimeScale);

				track.Name      = "und";
				track.Encoding  = encoding;
				track.Id        = n;

				track.stream    = stream;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					ffmpeg_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				if (context->manager->video)
				{
					if (context->manager->video->Command(context, MANAGER_ADD, &track) < 0) 
					{
						/* konfetti: fixme: is this a reason to return with error? */
						ffmpeg_err("failed to add track %d\n", n);
					}
				}
			}
			else 
			{
				ffmpeg_err("codec type video but codec unknown %d\n", stream->codec->codec_id);
			}
			break;
	    
			// audio codec
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)	  
			case AVMEDIA_TYPE_AUDIO:
#else
			case CODEC_TYPE_AUDIO:
#endif        
			ffmpeg_printf(10, "CODEC_TYPE_AUDIO %d\n",stream->codec->codec_type);

			if (encoding != NULL) 
			{
				track.type = eTypeES;
				
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
					track.Name        = strdup("Stream");

				ffmpeg_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Id             = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					ffmpeg_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

#if defined (USE_OPENGL)
				track.inject_as_pcm = 1;

				AVCodec *codec = avcodec_find_decoder(stream->codec->codec_id);

#if LIBAVCODEC_VERSION_MAJOR < 54
				if(codec != NULL && !avcodec_open(stream->codec, codec))
#else
				if(codec != NULL && !avcodec_open2(stream->codec, codec, NULL))
#endif					  
					printf("AVCODEC__INIT__SUCCESS\n");
				else
					printf("AVCODEC__INIT__FAILED\n");
#endif

				// pcm
				if(!strncmp(encoding, "A_IPCM", 6))
				{
					track.inject_as_pcm = 1;
					ffmpeg_printf(10, " Handle inject_as_pcm = %d\n", track.inject_as_pcm);

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
					ffmpeg_printf(10,"Create AAC ExtraData\n");
					ffmpeg_printf(10,"stream->codec->extradata_size %d\n", stream->codec->extradata_size);
					//Hexdump(stream->codec->extradata, stream->codec->extradata_size);

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

					ffmpeg_printf(10,"aac object_type %d\n", object_type);
					ffmpeg_printf(10,"aac sample_index %d\n", sample_index);
					ffmpeg_printf(10,"aac chan_config %d\n", chan_config);

					object_type -= 1; // Cause of ADTS

					track.aacbuflen = AAC_HEADER_LENGTH;
					track.aacbuf = malloc(8);
					track.aacbuf[0] = 0xFF;
					track.aacbuf[1] = 0xF1;
					track.aacbuf[2] = ((object_type & 0x03) << 6)  | (sample_index << 2) | ((chan_config >> 2) & 0x01);
					track.aacbuf[3] = (chan_config & 0x03) << 6;
					track.aacbuf[4] = 0x00;
					track.aacbuf[5] = 0x1F;
					track.aacbuf[6] = 0xFC;

					//ffmpeg_printf(10, "AAC_HEADER -> ");
					//Hexdump(track.aacbuf,7);
					track.have_aacheader = 1;
				} 
				// wma
				else if(stream->codec->codec_id == AV_CODEC_ID_WMAV1 || stream->codec->codec_id == AV_CODEC_ID_WMAV2 || 86056 ) //CODEC_ID_WMAPRO) //if (stream->codec->extradata_size > 0)
				{
					ffmpeg_printf(10,"Create WMA ExtraData\n");
					track.aacbuflen = 104 + stream->codec->extradata_size;
					track.aacbuf = malloc(track.aacbuflen);
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
						case 86056/*CODEC_ID_WMAPRO*/:
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
					ffmpeg_printf(1, "samples_per_second = %d\n", samples_per_second);
					memcpy(track.aacbuf + 82, &samples_per_second, 4); //samples_per_second

					unsigned int average_number_of_bytes_per_second = stream->codec->bit_rate / 8;
					ffmpeg_printf(1, "average_number_of_bytes_per_second = %d\n", average_number_of_bytes_per_second);
					memcpy(track.aacbuf + 86, &average_number_of_bytes_per_second, 4); //average_number_of_bytes_per_second

					unsigned short block_alignment = stream->codec->block_align;
					ffmpeg_printf(1, "block_alignment = %d\n", block_alignment);
					memcpy(track.aacbuf + 90, &block_alignment, 2); //block_alignment

					unsigned short bits_per_sample = stream->codec->sample_fmt>=0?(stream->codec->sample_fmt+1)*8:8;
					ffmpeg_printf(1, "bits_per_sample = %d (%d)\n", bits_per_sample, stream->codec->sample_fmt);
					memcpy(track.aacbuf + 92, &bits_per_sample, 2); //bits_per_sample

					memcpy(track.aacbuf + 94, &stream->codec->extradata_size, 2); //bits_per_sample

					memcpy(track.aacbuf + 96, stream->codec->extradata, stream->codec->extradata_size);
		    
					ffmpeg_printf(1, "aacbuf:\n");
					//Hexdump(track.aacbuf, track.aacbuflen);
		    
					//ffmpeg_printf(1, "priv_data:\n");
					//Hexdump(stream->codec->priv_data, track.aacbuflen);

					track.have_aacheader = 1;
				}

				if (context->manager->audio)
				{
					if (context->manager->audio->Command(context, MANAGER_ADD, &track) < 0) 
					{
						/* konfetti: fixme: is this a reason to return with error? */
						ffmpeg_err("failed to add track %d\n", n);
					}
				}
			}
			else 
			{
				ffmpeg_err("codec type audio but codec unknown %d\n", stream->codec->codec_id);
			}
			break;
	    
			// subtitle
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
			case AVMEDIA_TYPE_SUBTITLE:
			{
#if LIBAVCODEC_VERSION_MAJOR < 54
				AVMetadataTag * lang;
#else
				AVDictionaryEntry * lang;
#endif
				ffmpeg_printf(10, "CODEC_TYPE_SUBTITLE %d\n",stream->codec->codec_type);

#if LIBAVCODEC_VERSION_MAJOR < 54
				lang = av_metadata_get(stream->metadata, "language", NULL, 0);
#else
				lang = av_dict_get(stream->metadata, "language", NULL, 0);
#endif	     

				if (lang)
					track.Name        = strdup(lang->value);
				else
					track.Name        = strdup("und");

				ffmpeg_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Id             = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				track.width          = -1; /* will be filled online from videotrack */
				track.height         = -1; /* will be filled online from videotrack */

				track.extraData      = stream->codec->extradata;
				track.extraSize      = stream->codec->extradata_size;

				ffmpeg_printf(1, "subtitle codec %d\n", stream->codec->codec_id);
				ffmpeg_printf(1, "subtitle width %d\n", stream->codec->width);
				ffmpeg_printf(1, "subtitle height %d\n", stream->codec->height);
				ffmpeg_printf(1, "subtitle stream %p\n", stream);

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					ffmpeg_printf(10, "Stream has no duration so we take the duration from context\n");
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				if (track.Name)
					ffmpeg_printf(10, "FOUND SUBTITLE %s\n", track.Name);

				if (context->manager->subtitle)
				{
					if (context->manager->subtitle->Command(context, MANAGER_ADD, &track) < 0) 
					{
						/* konfetti: fixme: is this a reason to return with error? */
						ffmpeg_err("failed to add subtitle track %d\n", n);
					}
				}
				break;
			} 
        
			// all others
			case AVMEDIA_TYPE_UNKNOWN:
			case AVMEDIA_TYPE_DATA:
			case AVMEDIA_TYPE_ATTACHMENT:
			case AVMEDIA_TYPE_NB:
			default:
				ffmpeg_err("not handled or unknown codec_type %d\n", stream->codec->codec_type);
				break;	 
#endif	 
		} /* switch (stream->codec->codec_type) */

	} /* for */

	/* init */
	latestPts = 0;
	isContainerRunning = 1;

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int container_ffmpeg_play(Context_t *context)
{
	int error;
	int ret = 0;
	pthread_attr_t attr;

	ffmpeg_printf(10, "\n");

	if ( context && context->playback && context->playback->isPlaying ) 
	{
		ffmpeg_printf(10, "is Playing\n");
	}
	else 
	{
		ffmpeg_printf(10, "is NOT Playing\n");
	}

	if (hasPlayThreadStarted == 0) 
	{
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		if((error = pthread_create(&PlayThread, &attr, (void *)&FFMPEGThread, context)) != 0) 
		{
			ffmpeg_printf(10, "Error creating thread, error:%d:%s\n", error,strerror(error));

			hasPlayThreadStarted = 0;
			ret = cERR_CONTAINER_FFMPEG_ERR;
		}
		else 
		{
			ffmpeg_printf(10, "Created thread\n");

			hasPlayThreadStarted = 1;
		}
	}
	else 
	{
		ffmpeg_printf(10, "A thread already exists!\n");

		ret = cERR_CONTAINER_FFMPEG_ERR;
	}

	ffmpeg_printf(10, "exiting with value %d\n", ret);

	return ret;
}

static int container_ffmpeg_stop(Context_t *context) 
{
	int ret = cERR_CONTAINER_FFMPEG_NO_ERROR;
	int wait_time = 20;

	ffmpeg_printf(10, "\n");

	if (!isContainerRunning)
	{
		ffmpeg_err("Container not running\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}
	
	if (context->playback)
	{
		context->playback->isPlaying = 0;
	}

	while ( (hasPlayThreadStarted != 0) && (--wait_time) > 0 ) 
	{
		ffmpeg_printf(10, "Waiting for ffmpeg thread to terminate itself, will try another %d times\n", wait_time);

		usleep(100000);
	}

	if (wait_time == 0) 
	{
		ffmpeg_err( "Timeout waiting for thread!\n");

		ret = cERR_CONTAINER_FFMPEG_ERR;

		usleep(100000);
	}

	hasPlayThreadStarted = 0;

	getMutex(FILENAME, __FUNCTION__,__LINE__);

	if (avContext != NULL) 
	{
#if LIBAVFORMAT_VERSION_MAJOR < 54
 		av_close_input_file(avContext);
 		avContext = NULL;
#else
		avformat_close_input(&avContext);
#endif		
	}

	isContainerRunning = 0;

#if defined (USE_OPENGL)
	//if (adevice)
	ao_close(adevice);
	adevice = NULL;
	ao_shutdown();
#endif

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	ffmpeg_printf(10, "ret %d\n", ret);

	return ret;
}

static int container_ffmpeg_seek_bytes(off_t pos) 
{
	int flag = AVSEEK_FLAG_BYTE;
#if LIBAVCODEC_VERSION_MAJOR < 54
	off_t current_pos = url_ftell(avContext->pb);
#else
	off_t current_pos = avio_tell(avContext->pb);
#endif

	ffmpeg_printf(20, "seeking to position %lld (bytes)\n", pos);

	if (current_pos > pos)
		flag |= AVSEEK_FLAG_BACKWARD;

	if (avformat_seek_file(avContext, -1, INT64_MIN, pos, INT64_MAX, flag) < 0)
	{
		ffmpeg_err( "Error seeking\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}    

#if LIBAVCODEC_VERSION_MAJOR < 54
	ffmpeg_printf(30, "current_pos after seek %lld\n", url_ftell(avContext->pb));
#else
	ffmpeg_printf(30, "current_pos after seek %lld\n", avio_tell(avContext->pb));
#endif

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

/* seeking relative to a given byteposition N bytes ->for reverse playback needed */
static int container_ffmpeg_seek_bytes_rel(off_t start, off_t bytes) 
{
	int flag = AVSEEK_FLAG_BYTE;
	off_t newpos;
#if LIBAVCODEC_VERSION_MAJOR < 54
	off_t current_pos = url_ftell(avContext->pb);
#else
	off_t current_pos = avio_tell(avContext->pb);
#endif

	if (start == -1)
		start = current_pos;

	ffmpeg_printf(250, "start:%lld bytes:%lld\n", start, bytes);

	newpos = start + bytes;

	if (current_pos > newpos)
		flag |= AVSEEK_FLAG_BACKWARD;

	if (newpos < 0)
	{
		ffmpeg_err("end of file reached\n");
		return cERR_CONTAINER_FFMPEG_END_OF_FILE;
	}

	ffmpeg_printf(20, "seeking to position %lld (bytes)\n", newpos);

/* fixme: should we adapt INT64_MIN/MAX to some better value?
 * take a loog in ffmpeg to be sure what this paramter are doing
 */
	if (avformat_seek_file(avContext, -1, INT64_MIN, newpos, INT64_MAX, flag) < 0)
	{
		ffmpeg_err( "Error seeking\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}    

#if LIBAVCODEC_VERSION_MAJOR < 54
	ffmpeg_printf(30, "current_pos after seek %lld\n", url_ftell(avContext->pb));
#else
	ffmpeg_printf(30, "current_pos after seek %lld\n", avio_tell(avContext->pb));
#endif

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

/* seeking relative to a given byteposition N seconds ->for reverse playback needed */
static int container_ffmpeg_seek_rel(Context_t *context, off_t pos, long long int pts, float sec) 
{
	Track_t * videoTrack = NULL;
	Track_t * audioTrack = NULL;
	Track_t * current = NULL;
	int flag = 0;

	ffmpeg_printf(10, "seeking %f sec relativ to %lld\n", sec, pos);

	context->manager->video->Command(context, MANAGER_GET_TRACK, &videoTrack);
	context->manager->audio->Command(context, MANAGER_GET_TRACK, &audioTrack);

	if (videoTrack != NULL)
		current = videoTrack;
	else if (audioTrack != NULL)
		current = audioTrack;

	if (current == NULL) 
	{
		ffmpeg_err( "no track avaibale to seek\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

	if (pos == -1)
	{
#if LIBAVCODEC_VERSION_MAJOR < 54
		pos = url_ftell(avContext->pb);
#else
		pos = avio_tell(avContext->pb);
#endif
	}

	if (pts == -1)
		pts = current->pts;

	if (sec < 0)
		flag |= AVSEEK_FLAG_BACKWARD;

	getMutex(FILENAME, __FUNCTION__,__LINE__);

	ffmpeg_printf(10, "iformat->flags %d\n", avContext->iformat->flags);

	if (avContext->iformat->flags & AVFMT_TS_DISCONT)
	{
		if (avContext->bit_rate)
		{
			sec *= avContext->bit_rate / 8.0;
			ffmpeg_printf(10, "bit_rate %d\n", avContext->bit_rate);
		}
		else
		{
			sec *= 180000.0;
		}

		pos += sec;

		if (pos < 0)
		{
			ffmpeg_err("end of file reached\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_END_OF_FILE;
		}

		ffmpeg_printf(10, "1. seeking to position %lld bytes ->sec %f\n", pos, sec);

		if (container_ffmpeg_seek_bytes(pos) < 0)
		{
			ffmpeg_err( "Error seeking\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_ERR;
		}

		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		return pos;
	}
	else
	{
		sec += ((float) pts / 90000.0f);

		if (sec < 0)
			sec = 0;

		ffmpeg_printf(10, "2. seeking to position %f sec ->time base %f %d\n", sec, av_q2d(((AVStream*) current->stream)->time_base), AV_TIME_BASE);

		if (av_seek_frame(avContext, -1 , sec * AV_TIME_BASE, flag) < 0) 
		{
			ffmpeg_err( "Error seeking\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_ERR;
		}

		if (sec <= 0)
		{
			ffmpeg_err("end of file reached\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_END_OF_FILE;
		}
	}

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);
	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int container_ffmpeg_seek(Context_t *context, float sec) 
{
	Track_t * videoTrack = NULL;
	Track_t * audioTrack = NULL;
	Track_t * current = NULL;
	int flag = 0;

#if !defined(VDR1722)
	ffmpeg_printf(10, "seeking %f sec\n", sec);

	if (sec == 0.0)
	{
		ffmpeg_err("sec = 0.0 ignoring\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}
#else
	ffmpeg_printf(10, "goto %f sec\n", sec);

	if (sec < 0.0)
	{
		ffmpeg_err("sec < 0.0 ignoring\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}
#endif

	context->manager->video->Command(context, MANAGER_GET_TRACK, &videoTrack);
	context->manager->audio->Command(context, MANAGER_GET_TRACK, &audioTrack);

	if (videoTrack != NULL)
		current = videoTrack;
	else if (audioTrack != NULL)
		current = audioTrack;

	if (current == NULL) 
	{
		ffmpeg_err( "no track available to seek\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

	if (sec < 0)
		flag |= AVSEEK_FLAG_BACKWARD;

	getMutex(FILENAME, __FUNCTION__,__LINE__);

	ffmpeg_printf(10, "iformat->flags %d\n", avContext->iformat->flags);

	if (avContext->iformat->flags & AVFMT_TS_DISCONT)
	{
/* konfetti: for ts streams seeking frame per seconds does not work (why?).
 * I take this algo partly from ffplay.c.
 *
 * seeking per HTTP does still not work very good. forward seeks everytime
 * about 10 seconds, backward does not work.
 */

#if LIBAVCODEC_VERSION_MAJOR < 54
		off_t pos = url_ftell(avContext->pb);
#else
		off_t pos = avio_tell(avContext->pb);
#endif

		ffmpeg_printf(10, "pos %lld %d\n", pos, avContext->bit_rate);

		if (avContext->bit_rate)
		{
			sec *= avContext->bit_rate / 8.0;
			ffmpeg_printf(10, "bit_rate %d\n", avContext->bit_rate);
		}
		else
		{
			sec *= 180000.0;
		}
#if !defined(VDR1722)
		pos += sec;
#else
		pos = sec;
#endif
		if (pos < 0)
		{
			ffmpeg_err("end of file reached\n");
			return cERR_CONTAINER_FFMPEG_END_OF_FILE;
		}

		ffmpeg_printf(10, "1. seeking to position %lld bytes ->sec %f\n", pos, sec);

		if (container_ffmpeg_seek_bytes(pos) < 0)
		{
			ffmpeg_err( "Error seeking\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_ERR;
		}
	} 
	else
	{
#if !defined(VDR1722)
		sec += ((float) current->pts / 90000.0f);
#endif
		ffmpeg_printf(10, "2. seeking to position %f sec ->time base %f %d\n", sec, av_q2d(((AVStream*) current->stream)->time_base), AV_TIME_BASE);

		if (av_seek_frame(avContext, -1 /* or streamindex */, sec * AV_TIME_BASE, flag) < 0) 
		{
			ffmpeg_err( "Error seeking\n");
			releaseMutex(FILENAME, __FUNCTION__,__LINE__);
			return cERR_CONTAINER_FFMPEG_ERR;
		}
	}

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);
	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int container_ffmpeg_get_length(Context_t *context, double * length) 
{
	ffmpeg_printf(50, "\n");
	Track_t * videoTrack = NULL;
	Track_t * audioTrack = NULL;
	Track_t * subtitleTrack = NULL;
	Track_t * current = NULL;

	if (length == NULL) 
	{
		ffmpeg_err( "null pointer passed\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

	context->manager->video->Command(context, MANAGER_GET_TRACK, &videoTrack);
	context->manager->audio->Command(context, MANAGER_GET_TRACK, &audioTrack);    
	context->manager->subtitle->Command(context, MANAGER_GET_TRACK, &subtitleTrack);    

	if (videoTrack != NULL)
		current = videoTrack;
	else if (audioTrack != NULL)
		current = audioTrack;    
	else if (subtitleTrack != NULL)
		current = subtitleTrack;    

	*length = 0.0;

	if (current != NULL) 
	{
		if (current->duration == 0)
			return cERR_CONTAINER_FFMPEG_ERR;
		else
			*length = (current->duration / 1000.0);
	}
	else 
	{
		if (avContext != NULL)
		{
			*length = (avContext->duration / 1000.0);
		} 
		else
		{
			ffmpeg_err( "no Track not context ->no problem :D\n");
			return cERR_CONTAINER_FFMPEG_ERR;
		}
	}

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int container_ffmpeg_swich_audio(Context_t* context, int* arg)
{
	ffmpeg_printf(10, "track %d\n", *arg);
	/* Hellmaster1024: nothing to do here!*/
	//float sec=-5.0;
	//context->playback->Command(context, PLAYBACK_SEEK, (void*)&sec);
	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int container_ffmpeg_swich_subtitle(Context_t* context, int* arg)
{
	/* Hellmaster1024: nothing to do here!*/
	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

/* konfetti comment: I dont like the mechanism of overwriting
 * the pointer in infostring. This lead in most cases to
 * user errors, like it is in the current version (libeplayer2 <-->e2->servicemp3.cpp)
 * From e2 there is passed a tag=strdup here and we overwrite this
 * strdupped tag. This lead to dangling pointers which are never freed!
 * I do not free the string here because this is the wrong way. The mechanism
 * should be changed, or e2 should pass it in a different way...
 */
static int container_ffmpeg_get_info(Context_t* context, char ** infoString)
{
	Track_t * videoTrack = NULL;
	Track_t * audioTrack = NULL;
	char*     meta = NULL;

	ffmpeg_printf(20, ">\n");

	if (avContext != NULL)
	{
		if ((infoString == NULL) || (*infoString == NULL))
		{
			ffmpeg_err("infostring NULL\n");
			return cERR_CONTAINER_FFMPEG_ERR;
		}

		ffmpeg_printf(20, "%s\n", *infoString);

		context->manager->video->Command(context, MANAGER_GET_TRACK, &videoTrack);
		context->manager->audio->Command(context, MANAGER_GET_TRACK, &audioTrack);

		if ((meta = searchMeta(avContext->metadata, *infoString)) == NULL)
		{
			if (audioTrack != NULL)
			{
				AVStream* stream = audioTrack->stream;

				meta = searchMeta(stream->metadata, *infoString);
			}

			if ((meta == NULL) && (videoTrack != NULL))
			{
				AVStream* stream = videoTrack->stream;

				meta = searchMeta(stream->metadata, *infoString);
			}
		}

		if (meta != NULL)
		{
			*infoString = strdup(meta);
		}
		else
		{
			ffmpeg_printf(1, "no metadata found for \"%s\"\n", *infoString);
			*infoString = strdup("not found");
		}
	} 
	else
	{
		ffmpeg_err("avContext NULL\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

static int Command(void  *_context, ContainerCmd_t command, void * argument)
{
	Context_t  *context = (Context_t*) _context;
	int ret = cERR_CONTAINER_FFMPEG_NO_ERROR;

	ffmpeg_printf(50, "Command %d\n", command);

	switch(command)
	{
		case CONTAINER_INIT:  
		{
			char * FILENAME = (char *)argument;
			ret = container_ffmpeg_init(context, FILENAME);
			break;
		}
		
		case CONTAINER_PLAY:  
		{
			ret = container_ffmpeg_play(context);
			break;
		}
		
		case CONTAINER_STOP:  
		{
			ret = container_ffmpeg_stop(context);
			break;
		}
		
		case CONTAINER_SEEK: 
		{
			ret = container_ffmpeg_seek(context, (float)*((float*)argument));
			break;
		}
		
		case CONTAINER_LENGTH: 
		{
			double length = 0;
			ret = container_ffmpeg_get_length(context, &length);

			*((double*)argument) = (double)length;
			break;
		}
		
		case CONTAINER_SWITCH_AUDIO: 
		{
			ret = container_ffmpeg_swich_audio(context, (int*) argument);
			break;
		}
		
		case CONTAINER_SWITCH_SUBTITLE: 
		{
			ret = container_ffmpeg_swich_subtitle(context, (int*) argument);
			break;
		}
		
		case CONTAINER_INFO: 
		{
			ret = container_ffmpeg_get_info(context, (char **)argument);
			break;
		}
		
		case CONTAINER_STATUS: 
		{
			*((int*)argument) = hasPlayThreadStarted;
			break;
		}
		
		case CONTAINER_LAST_PTS: 
		{
			*((long long int*)argument) = latestPts;
			break;
		}
		
		default:
			ffmpeg_err("ContainerCmd %d not supported!\n", command);
			ret = cERR_CONTAINER_FFMPEG_ERR;
			break;
	}

	ffmpeg_printf(50, "exiting with value %d\n", ret);

	return ret;
}

static char *FFMPEG_Capabilities[] = {
	"avi", 
	"mkv", 
	"mp4", 
	"ts", 
	"mov", 
	"flv", 
	"flac", 
	"mp3", 
	"mpg", 
	"m2ts", 
	"vob",
	"wav",
	"wmv",
	"wma", 
	"asf", 
	"mp2", 
	"m4v", 
	"m4a", 
	"divx", 
	"dat", 
	"mpeg", 
	"trp", 
	"mts", 
	"vdr", 
	"ogg",  
	NULL 
};

Container_t FFMPEGContainer = {
	"FFMPEG",
	&Command,
	FFMPEG_Capabilities
};


