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
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_MAJOR > 54
#include <libavutil/opt.h>
#endif

#include "common.h"
#include "misc.h"
#include "aac.h"
#include "pcm.h"
#include "ffmpeg_metadata.h"
#include "writer.h"

#include <config.h>


#if LIBAVCODEC_VERSION_MAJOR > 54
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define FFMPEG_DEBUG
#define FFMPEG_SILENT

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
static AVCodecContext *actx = NULL;
static AVCodecContext *vctx = NULL;
////
static pthread_t PlaySubThread;
static int hasPlaySubThreadStarted = 0;
static AVFormatContext *subavContext = NULL;
static AVCodecContext* subctx = NULL;

static unsigned char isContainerRunning = 0;

static long long int latestPts = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */
static int container_ffmpeg_seek_rel(Context_t *context, off_t pos, long long int pts, float sec);

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
void getMutex(const char *filename, const char *function, int line) 
{
	ffmpeg_printf(100, "::%d requesting mutex\n", line);

	pthread_mutex_lock(&mutex);

	ffmpeg_printf(100, "::%d received mutex\n", line);
}

void releaseMutex(const char *filename, const char *function, int line) 
{
	pthread_mutex_unlock(&mutex);

	ffmpeg_printf(100, "::%d released mutex\n", line);
}

static char* Codec2Encoding(uint32_t codec_id, int* version)
{
	switch (codec_id)
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
			return "V_FLV1";
			
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
			return "V_H264";
			
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
			
		case AV_CODEC_ID_TRUEHD:
			return "A_TRUEHD";
			
		case AV_CODEC_ID_DTS:
			return "A_DTS";			
			
		case AV_CODEC_ID_WMAV1:
		case AV_CODEC_ID_WMAV2:
		case AV_CODEC_ID_WMAPRO:
			return "A_WMA";
			
		case AV_CODEC_ID_VORBIS:
			return "A_VORBIS"; 	//FIXME:
			
		case AV_CODEC_ID_OPUS:
			return "A_OPUS";	//FIXME:
			
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
			return	"A_PCM"; 
			
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
			ffmpeg_printf(10, "CODEC FOUND -> %d\n", codec_id);
	}
	
	return NULL;
}

long long int calcPts(AVStream *stream, AVPacket *packet, AVFormatContext *_avContext)
{
	long long int pts;

	if ((stream == NULL) || (packet == NULL) || (_avContext == NULL))
	{
		ffmpeg_err("stream / packet null\n");
		return INVALID_PTS_VALUE;
	}

	if(packet->pts == AV_NOPTS_VALUE)
		pts = INVALID_PTS_VALUE;
	else if (_avContext->start_time == AV_NOPTS_VALUE)
		pts = 90000.0 * (double)packet->pts * av_q2d(stream->time_base);
	else
		pts = 90000.0 * (((double)(packet->pts) * av_q2d(stream->time_base)) - (_avContext->start_time / AV_TIME_BASE));

	if (pts & 0x8000000000000000ull)
		pts = INVALID_PTS_VALUE;

	return pts;
}

//
float getDurationFromSSALine(unsigned char *line)
{
	int i,h,m,s,ms;
	char* Text = strdup((char*) line);
	char* ptr1;
	char* ptr[10];
	long int msec;

	ptr1 = Text;
	ptr[0] = Text;
	
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

//
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

//// play thread
static void FFMPEGThread(Context_t* context) 
{
	AVPacket   packet;
	off_t lastSeek = -1;
	long long int lastPts = -1;
	long long int currentVideoPts = -1;
	long long int currentAudioPts = -1;
	long long int showtime = 0;
	long long int bofcount = 0;
	int err = 0;
	int audioMute = 0;
	AudioVideoOut_t avOut;
#ifdef USE_OPENGL
	AVFrame *frame = NULL;
	AVFrame* aframe = NULL;
	
	frame = av_frame_alloc();
	aframe = av_frame_alloc();
#endif	

	// Softdecoding buffer
	AVFrame *samples = NULL;

	ffmpeg_printf(10, "\n");
	
	av_init_packet(&packet);

	while ( context->playback->isCreationPhase )
	{
		ffmpeg_err("Thread waiting for end of init phase...\n");
		usleep(1000);
	}
	
	ffmpeg_printf(10, "Running!\n");

	while ( context && context->playback && context->playback->isPlaying ) 
	{
		// paused
		if (context->playback->isPaused) 
		{
			ffmpeg_printf(20, "paused\n");

			usleep(100000);
			continue;
		}

		// seeking
		if (context->playback->isSeeking) 
		{
			ffmpeg_printf(10, "seeking\n");

			usleep(100000);
			continue;
		}

		// backward
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

		//
		if(!context->playback->BackWard && audioMute)
		{
			lastPts = -1;
			bofcount = 0;
			showtime = 0;
			audioMute = 0;
			context->output->Command(context, OUTPUT_AUDIOMUTE, "0");
		}
		
		getMutex(FILENAME, __FUNCTION__,__LINE__);

		if ( (av_read_frame(avContext, &packet) == 0) )
		{
			Track_t * videoTrack = NULL;
			Track_t * audioTrack = NULL;	    
			Track_t * subtitleTrack = NULL;    

			int index = packet.stream_index;

			// 
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
				if (videoTrack->Index == index) 
				{
					currentVideoPts = videoTrack->pts = calcPts(videoTrack->stream, &packet, avContext);

					if ((currentVideoPts > latestPts) && (currentVideoPts != INVALID_PTS_VALUE))
						latestPts = currentVideoPts;

					ffmpeg_printf(200, "VideoTrack index = %d %lld\n",index, currentVideoPts);

					avOut.data       = packet.data;
					avOut.len        = packet.size;
					avOut.pts        = videoTrack->pts;
					avOut.extradata  = videoTrack->extraData;
					avOut.extralen   = videoTrack->extraSize;
					avOut.frameRate  = videoTrack->frame_rate;
					avOut.timeScale  = videoTrack->TimeScale;
					avOut.width      = videoTrack->width;
					avOut.height     = videoTrack->height;
					avOut.type       = "video";
					
#ifdef USE_OPENGL
					avOut.stream 	 = videoTrack->stream;
					avOut.ctx 	 = videoTrack->ctx;
					avOut.frame 	 = frame;
					avOut.aframe 	 = NULL;
#endif

					if (context->output->video->Write(context, &avOut) < 0) 
					{
						ffmpeg_err("writing data to video device failed\n");
					}
				}
			}

			// audio
			if (audioTrack != NULL) 
			{
				if (audioTrack->Index == index) 
				{
					currentAudioPts = audioTrack->pts = calcPts(audioTrack->stream, &packet, avContext);

					if ((currentAudioPts > latestPts) && (!videoTrack))
						latestPts = currentAudioPts;

					ffmpeg_printf(200, "AudioTrack index = %d\n",index);
								
#ifdef USE_OPENGL
					avOut.data       = packet.data;
					avOut.len        = packet.size;
					avOut.pts        = audioTrack->pts;
					avOut.extradata  = NULL;
					avOut.extralen   = 0;
					avOut.frameRate  = 0;
					avOut.timeScale  = 0;
					avOut.width      = 0;
					avOut.height     = 0;
					avOut.type       = "audio";
					avOut.stream 	 = audioTrack->stream;
					avOut.ctx 	 = audioTrack->ctx;
					avOut.frame 	 = NULL;
					avOut.aframe 	 = aframe;

					if (!context->playback->BackWard)
					{
						if (context->output->audio->Write(context, &avOut) < 0)
						{
							ffmpeg_err("writing data to audio device failed\n");
						}
					}
#else
					// pcm extradata
					pcmPrivateData_t extradata;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
					extradata.uNoOfChannels = audioTrack->stream->codecpar->ch_layout.nb_channels;
#else
					extradata.uNoOfChannels = audioTrack->stream->codecpar->channels;
#endif
					extradata.uSampleRate = audioTrack->stream->codecpar->sample_rate;
					extradata.uBitsPerSample = 16;
					extradata.bLittleEndian = 1;
					extradata.avCodecId = audioTrack->stream->codecpar->codec_id;
					extradata.bits_per_coded_sample = (int)&audioTrack->stream->codecpar->bits_per_coded_sample;
					extradata.bit_rate = audioTrack->stream->codecpar->bit_rate;
                			extradata.block_align = audioTrack->stream->codecpar->block_align;
                			extradata.frame_size = audioTrack->stream->codecpar->frame_size;
                			extradata.bResampling  = 1;
                			
					//
					if(!strncmp(audioTrack->Encoding, "A_PCM", 5))
					{
						ffmpeg_printf(200,"write audio raw pcm\n");

						avOut.data       = packet.data;
						avOut.len        = packet.size;
						avOut.pts        = audioTrack->pts;
						avOut.extradata  = (uint8_t *) &extradata;
						avOut.extralen   = sizeof(extradata);
						avOut.frameRate  = 0;
						avOut.timeScale  = 0;
						avOut.width      = 0;
						avOut.height     = 0;
						avOut.type       = "audio";

						if (!context->playback->BackWard)
						{
							if (context->output->audio->Write(context, &avOut) < 0)
							{
								ffmpeg_err("(raw pcm) writing data to audio device failed\n");
							}
						}
					}
					else if (audioTrack->inject_as_pcm == 1)
					{
						// FIXME:
						int      bytesDone = 0;
						unsigned int samples_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;

						if(samples == NULL)
							samples = av_frame_alloc();
						else
							av_frame_unref(samples);

						while(packet.size > 0)
						{
							int got_frame = 0;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,37,100)
							bytesDone = avcodec_decode_audio4(audioTrack->stream->codec, samples, &got_frame, &packet);
#else
							bytesDone = avcodec_send_packet(actx, &packet);
							
             						if (bytesDone < 0 && bytesDone != AVERROR(EAGAIN) && bytesDone != AVERROR_EOF) 
             						{
            						} 
            						else 
            						{
             							bytesDone = avcodec_receive_frame(actx, samples);
             						}
#endif
							//
							if(bytesDone < 0) // Error Happend
							    break;

							packet.data += bytesDone;
							packet.size -= bytesDone;
							
							//
							samples_size = av_rescale_rnd(samples->nb_samples, audioTrack->stream->codecpar->sample_rate, audioTrack->stream->codecpar->sample_rate, AV_ROUND_UP);

							avOut.data       = (uint8_t *)samples;
							avOut.len        = samples_size;

							avOut.pts        = audioTrack->pts;
							avOut.extradata  = (unsigned char *) &extradata;
							avOut.extralen   = sizeof(extradata);
							avOut.frameRate  = 0;
							avOut.timeScale  = 0;
							avOut.width      = 0;
							avOut.height     = 0;
							avOut.type       = "audio";

							if (!context->playback->BackWard)
							{
								if (context->output->audio->Write(context, &avOut) < 0)
								{
									ffmpeg_err("writing data to audio device failed\n");
								}
							}
						}
					}
					else if (audioTrack->have_aacheader == 1)
					{
						ffmpeg_printf(200,"write audio aac\n");

						avOut.data       = packet.data;
						avOut.len        = packet.size;
						avOut.pts        = audioTrack->pts;
						avOut.extradata  = audioTrack->aacbuf;
						avOut.extralen   = audioTrack->aacbuflen;
						avOut.frameRate  = 0;
						avOut.timeScale  = 0;
						avOut.width      = 0;
						avOut.height     = 0;
						avOut.type       = "audio";

						if (!context->playback->BackWard)
						{
							if (context->output->audio->Write(context, &avOut) < 0)
							{
								ffmpeg_err("(aac) writing data to audio device failed\n");
							}
						}
					}
					else
					{
						avOut.data       = packet.data;
						avOut.len        = packet.size;
						avOut.pts        = audioTrack->pts;
						avOut.extradata  = NULL;
						avOut.extralen   = 0;
						avOut.frameRate  = 0;
						avOut.timeScale  = 0;
						avOut.width      = 0;
						avOut.height     = 0;
						avOut.type       = "audio";

						if (!context->playback->BackWard)
						{
							if (context->output->audio->Write(context, &avOut) < 0)
							{
								ffmpeg_err("writing data to audio device failed\n");
							}
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
					ffmpeg_printf(100, "subtitleTrack->stream %p \n", subtitleTrack->stream);

					subtitleTrack->pts = calcPts(subtitleTrack->stream, &packet, avContext);

					if ((subtitleTrack->pts > latestPts) && (!videoTrack) && (!audioTrack))
						latestPts = subtitleTrack->pts;

#if LIBAVFORMAT_VERSION_MAJOR > 56
					ffmpeg_printf(20, "Packet duration %d\n", packet.duration);
#else
					ffmpeg_printf(20, "Packet convergence_duration %lld\n", packet.convergence_duration);
#endif

					if(packet.duration != 0 && packet.duration != AV_NOPTS_VALUE )
						duration = ((float)packet.duration)/1000.0;
#if LIBAVFORMAT_VERSION_MAJOR > 56
					else if(packet.duration != 0 && packet.duration != AV_NOPTS_VALUE )
						duration = ((float)packet.duration)/1000.0;
#else
					else if(packet.convergence_duration != 0 && packet.convergence_duration != AV_NOPTS_VALUE )
						duration = ((float)packet.convergence_duration)/1000.0;	
#endif
					else if(subtitleTrack->stream->codecpar->codec_id == AV_CODEC_ID_SSA)
					{
						duration = getDurationFromSSALine(packet.data);
					} 
					else 
					{
						// no clue yet
					}

					//
					if (duration > 0 || duration == -1)
					{
						SubtitleData_t data;
						
						data.data      = packet.data;
						data.len       = packet.size;
						data.extradata = subtitleTrack->extraData;
						data.extralen  = subtitleTrack->extraSize;
						data.pts       = subtitleTrack->pts;
						data.duration  = duration;
						data.width     = subtitleTrack->width;
		           			data.height    = subtitleTrack->height;
		           			//
		           			data.stream    = subtitleTrack->stream;
		           			data.ctx 	= subtitleTrack->ctx;
		           			
						if (context->output->subtitle->Write(context, &data) < 0) 
						{
							ffmpeg_err("writing data to subtitle failed\n");
						}
					}
				}
			}            

			if (packet.data)
#if (LIBAVCODEC_VERSION_MAJOR > 55)
				av_packet_unref(&packet);
#else
				av_free_packet(&packet);
			av_packet_unref(&packet);
#endif
		}
		else  
		{
		    ffmpeg_err("no data ->end of file reached ? \n");
		    releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		    break;
		}
		
		releaseMutex(FILENAME, __FUNCTION__,__LINE__);		
	} // while

	//
	if (samples != NULL) 
	{
		av_frame_free(&samples);
		samples = NULL;
	}
	
	//
#ifdef USE_OPENGL
	if (frame)
	{
		av_frame_free(&frame);
		frame = NULL;
	}
	
	if (aframe)
	{
		av_frame_free(&aframe);
		aframe = NULL;
	}
#endif
	
	//
	av_packet_unref(&packet);

	hasPlayThreadStarted = 0;

	ffmpeg_printf(10, "terminating\n");
}

//// init
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
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	avcodec_register_all();
	av_register_all();
#endif
	avformat_network_init();
	
	//
	AVDictionary *options = NULL;
	av_dict_set(&options, "auth_type", "basic", 0);
	
	if (strncmp(filename, "http://", 7) == 0 || strncmp(filename, "https://", 8) == 0)
	{
		av_dict_set(&options, "timeout", "20000000", 0); //20sec
		av_dict_set(&options, "reconnect", "1", 0);
		av_dict_set(&options, "reconnect_delay_max", "7", 0);
		
		av_dict_set(&options, "seekable", "0", 0);
		av_dict_set(&options, "reconnect_streamed", "1", 0);
	}

#if LIBAVCODEC_VERSION_MAJOR < 54
	if ((err = av_open_input_file(&avContext, filename, NULL, 0, NULL)) != 0) 
#else
	if ((err = avformat_open_input(&avContext, filename, NULL, &options)) != 0)
#endif
	{
		ffmpeg_err("avformat_open_input failed %d (%s)\n", err, filename);
		
		//
		av_dict_free(&options);

		releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		
		return cERR_CONTAINER_FFMPEG_OPEN;
	}
	
	//
	av_dict_free(&options);
	
	avContext->flags |= AVFMT_FLAG_GENPTS;
#if (LIBAVFORMAT_VERSION_MAJOR > 55) && (LIBAVFORMAT_VERSION_MAJOR < 56)
	avContext->max_analyze_duration2 = 1;
#else
	avContext->max_analyze_duration = 1;
#endif
	avContext->probesize = 131072;

	// find stream info
#if LIBAVCODEC_VERSION_MAJOR < 54
	if (av_find_stream_info(avContext) < 0)
#else
	if (avformat_find_stream_info(avContext, NULL) < 0) 
#endif
	{
		ffmpeg_err("Error avformat_find_stream_info\n");
	}

	ffmpeg_printf(10, "number streams %d\n", avContext->nb_streams);

	for ( n = 0; n < avContext->nb_streams; n++) 
	{
		Track_t track;
		AVStream *stream = avContext->streams[n];
		int version = 0;
		char *encoding = NULL;

		encoding = Codec2Encoding(stream->codecpar->codec_id, &version);

		ffmpeg_printf(10, "%d. encoding = %s - version %d CODEC_TYPE:%d codec_id 0x%x\n", n, encoding? encoding : "NULL", version, stream->codecpar->codec_type, stream->codecpar->codec_id);

		//
		memset(&track, 0, sizeof(track));

		switch (stream->codecpar->codec_type)
		{
			// video
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)	  
			case AVMEDIA_TYPE_VIDEO:
#else
			case CODEC_TYPE_VIDEO:
#endif        

			if (encoding != NULL) 
			{
				track.type           = eTypeES;
				track.version        = version;

				track.width          = stream->codecpar->width;
				track.height         = stream->codecpar->height;
				track.extraData      = stream->codecpar->extradata;
				track.extraSize      = stream->codecpar->extradata_size;

				track.frame_rate     = stream->r_frame_rate.num;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				double frame_rate = av_q2d(stream->r_frame_rate); // rational to double

				ffmpeg_printf(10, "frame_rate = %f\n", frame_rate);

				track.frame_rate = frame_rate * 1000.0;

				// fixme: revise this
				if (track.frame_rate < 23970)
					track.TimeScale = 1001;
				else
					track.TimeScale = 1000;

				track.Name      = "und";
				track.Encoding  = encoding;
				track.Index     = n;

				track.stream    = stream;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}
				
				//
				ffmpeg_printf(10, "bit_rate = %d\n",stream->codecpar->bit_rate);
				ffmpeg_printf(10, "time_base.den %d\n",stream->time_base.den);
				ffmpeg_printf(10, "time_base.num %d\n",stream->time_base.num);
				ffmpeg_printf(10, "frame_rate %d\n",stream->r_frame_rate.num);
				ffmpeg_printf(10, "TimeScale %d\n",stream->r_frame_rate.den);
				ffmpeg_printf(10, "frame_rate %d\n", track.frame_rate);
				ffmpeg_printf(10, "TimeScale %d\n", track.TimeScale);
				ffmpeg_printf(10, "width %d\n", track.width);
				ffmpeg_printf(10, "height %d\n", track.height);
				ffmpeg_printf(10, "\n");
				
				// init codec
				vctx = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));
				
				avcodec_open2(vctx, avcodec_find_decoder(stream->codecpar->codec_id), NULL);
				
#ifdef USE_OPENGL
				track.ctx = vctx;
#endif

				if (context->manager->video)
				{
					if (context->manager->video->Command(context, MANAGER_ADD, &track) < 0) 
					{
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
					track.Name = strdup("Stream");

				ffmpeg_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Index          = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}

				// init codec
				actx = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));

				avcodec_open2(actx, avcodec_find_decoder(stream->codecpar->codec_id), NULL);
				
#ifdef USE_OPENGL
				track.ctx = actx;
#else
				// ipcm
				if(!strncmp(encoding, "A_IPCM", 6))
				{
					track.inject_as_pcm = 1;
					
					ffmpeg_printf(10, " Handle inject_as_pcm = %d\n", track.inject_as_pcm);

					// init codec
					AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

#if LIBAVCODEC_VERSION_MAJOR < 54
					if(codec != NULL && !avcodec_open(stream->codec, codec))
#else
					if(codec != NULL && !avcodec_open2(actx, codec, NULL))
#endif					  
						printf("AVCODEC__INIT__SUCCESS\n");
					else
						printf("AVCODEC__INIT__FAILED\n");
				}
				// aac
				else if(stream->codecpar->codec_id == AV_CODEC_ID_AAC) 
				{
					ffmpeg_printf(10,"Create AAC ExtraData\n");
					ffmpeg_printf(10,"stream->codec->extradata_size %d\n", stream->codecpar->extradata_size);
					
					unsigned int object_type = 2; // LC
					unsigned int sample_index = aac_get_sample_rate_index(stream->codecpar->sample_rate);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
					unsigned int chan_config = stream->codecpar->ch_layout.nb_channels;
#else
					unsigned int chan_config = stream->codecpar->channels;
#endif
					
					if(stream->codecpar->extradata_size >= 2) 
					{
						object_type = stream->codecpar->extradata[0] >> 3;
						sample_index = ((stream->codecpar->extradata[0] & 0x7) << 1) + (stream->codecpar->extradata[1] >> 7);
						chan_config = (stream->codecpar->extradata[1] >> 3) && 0xf;
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

					track.have_aacheader = 1;
				} 
				// wma
				else if(stream->codecpar->codec_id == AV_CODEC_ID_WMAV1 || stream->codecpar->codec_id == AV_CODEC_ID_WMAV2 || stream->codecpar->codec_id == AV_CODEC_ID_WMAPRO ) //if (stream->codecpar->extradata_size > 0)
				{
					ffmpeg_printf(10,"Create WMA ExtraData\n");
					ffmpeg_printf(10,"stream->codec->extradata_size %d\n", stream->codecpar->extradata_size);
					
					track.aacbuflen = 104 + stream->codecpar->extradata_size;
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

					unsigned int type_specific_data_length = 18 + stream->codecpar->extradata_size;
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
					switch(stream->codecpar->codec_id) 
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

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
					unsigned short number_of_channels = stream->codecpar->ch_layout.nb_channels;
#else
					unsigned short number_of_channels = stream->codecpar->channels;
#endif
					memcpy(track.aacbuf + 80, &number_of_channels, 2); //number_of_channels

					unsigned int samples_per_second = stream->codecpar->sample_rate;
					ffmpeg_printf(10, "samples_per_second = %d\n", samples_per_second);
					memcpy(track.aacbuf + 82, &samples_per_second, 4); //samples_per_second

					unsigned int average_number_of_bytes_per_second = stream->codecpar->bit_rate / 8;
					ffmpeg_printf(10, "average_number_of_bytes_per_second = %d\n", average_number_of_bytes_per_second);
					memcpy(track.aacbuf + 86, &average_number_of_bytes_per_second, 4); //average_number_of_bytes_per_second

					unsigned short block_alignment = stream->codecpar->block_align;
					ffmpeg_printf(10, "block_alignment = %d\n", block_alignment);
					memcpy(track.aacbuf + 90, &block_alignment, 2); //block_alignment

					unsigned short bits_per_sample = stream->codecpar->sample_rate>=0?(stream->codecpar->sample_rate+1)*8:8;
					ffmpeg_printf(10, "bits_per_sample = %d (%d)\n", bits_per_sample, stream->codecpar->sample_rate);
					memcpy(track.aacbuf + 92, &bits_per_sample, 2); //bits_per_sample

					memcpy(track.aacbuf + 94, &stream->codecpar->extradata_size, 2); //bits_per_sample

					memcpy(track.aacbuf + 96, stream->codecpar->extradata, stream->codecpar->extradata_size);
		    
					ffmpeg_printf(10, "aacbuf:\n");

					track.have_aacheader = 1;
				}
#endif

				ffmpeg_printf(10, "\n");

				if (context->manager->audio)
				{
					if (context->manager->audio->Command(context, MANAGER_ADD, &track) < 0) 
					{
						ffmpeg_err("failed to add track %d\n", n);
					}
				}
			}
			else 
			{
				ffmpeg_err("codec type audio but codec unknown %d\n", stream->codecpar->codec_id);
			}
			break;
	    
			// subtitle
			case AVMEDIA_TYPE_SUBTITLE:
			{	
#if LIBAVCODEC_VERSION_MAJOR < 54
				AVMetadataTag * lang;
#else
				AVDictionaryEntry * lang;
#endif

#if LIBAVCODEC_VERSION_MAJOR < 54
				lang = av_metadata_get(stream->metadata, "language", NULL, 0);
#else
				lang = av_dict_get(stream->metadata, "language", NULL, 0);
#endif	     

				if (lang)
					track.Name = strdup(lang->value);
				else
					track.Name = strdup("Sub");

				ffmpeg_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Index          = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				track.width          = -1; /* will be filled online from videotrack */
				track.height         = -1; /* will be filled online from videotrack */

				track.extraData      = stream->codecpar->extradata;
				track.extraSize      = stream->codecpar->extradata_size;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					track.duration = (double) avContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}
				
				ffmpeg_printf(10, "\n", track.height);
				
				// init codec
				subctx = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));

				avcodec_open2(subctx, avcodec_find_decoder(stream->codecpar->codec_id), NULL);
				track.ctx = subctx;

				if (context->manager->subtitle)
				{
					if (context->manager->subtitle->Command(context, MANAGER_ADD, &track) < 0) 
					{
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
				ffmpeg_err("not handled or unknown codec_type %d\n", stream->codecpar->codec_type);
				break;	 	 
		} // switch (stream->codecpar->codec_type)

	} // for

	// init
	latestPts = 0;
	isContainerRunning = 1;

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

//
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

		if((error = pthread_create(&PlayThread, &attr, (void*)&FFMPEGThread, context)) != 0) 
		{
			ffmpeg_printf(10, "Error creating thread, error:%d:%s\n", error, strerror(error));

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

////
static void FFMPEGSubThread(Context_t* context) 
{
	AVPacket   subpacket;
	off_t lastSeek = -1;
	long long int lastPts = -1;
	long long int showtime = 0;
	long long int bofcount = 0;
	int err = 0;
	int audioMute = 0;	

	ffmpeg_printf(10, "\n");

	while ( context && context->playback && context->playback->isPlaying ) 
	{
		// paused
		if (context->playback->isPaused) 
		{
			ffmpeg_printf(20, "paused\n");

			usleep(100000);
			continue;
		}

		// seeking
		if (context->playback->isSeeking) 
		{
			ffmpeg_printf(10, "seeking\n");

			usleep(100000);
			continue;
		}

		// backward
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

			lastPts = lastPts + (context->playback->Speed * 90000);
			showtime = av_gettime() + 300000; //jump back all 300ms
		}

		//
		if(!context->playback->BackWard && audioMute)
		{
			lastPts = -1;
			bofcount = 0;
			showtime = 0;
			audioMute = 0;
			context->output->Command(context, OUTPUT_AUDIOMUTE, "0");
		}
		
		getMutex(FILENAME, __FUNCTION__,__LINE__);

		if ( (av_read_frame(subavContext, &subpacket) == 0) )
		{    
			Track_t * subtitleTrack = NULL;    

			int index = subpacket.stream_index;

			// 
			if (context->manager->extsubtitle->Command(context, MANAGER_GET_TRACK, &subtitleTrack) < 0)
				ffmpeg_err("error getting subtitle track\n");   

			ffmpeg_printf(200, "packet.size %d - index %d\n", subpacket.size, index);

			// subtitle
			if (subtitleTrack != NULL) 
			{
				if (subtitleTrack->Index == index) 
				{
					float duration = 3.0;
					ffmpeg_printf(100, "subtitleTrack->stream %p \n", subtitleTrack->stream);

					subtitleTrack->pts = calcPts(subtitleTrack->stream, &subpacket, subavContext);

					if (subtitleTrack->pts > latestPts)
						latestPts = subtitleTrack->pts;

#if (LIBAVFORMAT_VERSION_MAJOR > 57) || ((LIBAVFORMAT_VERSION_MAJOR == 57) && (LIBAVFORMAT_VERSION_MINOR > 32))
					ffmpeg_printf(20, "Packet duration %d\n", subpacket.duration);
#else
					ffmpeg_printf(20, "Packet convergence_duration %lld\n", subpacket.convergence_duration);
#endif

					if(subpacket.duration != 0 && subpacket.duration != AV_NOPTS_VALUE )
						duration = ((float)subpacket.duration)/1000.0;
#if (LIBAVFORMAT_VERSION_MAJOR > 57) || ((LIBAVFORMAT_VERSION_MAJOR == 57) && (LIBAVFORMAT_VERSION_MINOR > 32))
					else if(subpacket.duration != 0 && subpacket.duration != AV_NOPTS_VALUE )
						duration = ((float)subpacket.duration)/1000.0;
#else
					else if(subpacket.convergence_duration != 0 && subpacket.convergence_duration != AV_NOPTS_VALUE )
						duration = ((float)subpacket.convergence_duration)/1000.0;	
#endif	
					else if(subtitleTrack->stream->codecpar->codec_id == AV_CODEC_ID_SSA)
					{
						duration = getDurationFromSSALine(subpacket.data);
					} 
					else 
					{
						// no clue yet
					}

					//
					if (duration > 0 || duration == -1)
					{
						SubtitleData_t data;
						
						data.data      = subpacket.data;
						data.len       = subpacket.size;
						data.extradata = subtitleTrack->extraData;
						data.extralen  = subtitleTrack->extraSize;
						data.pts       = subtitleTrack->pts;
						data.duration  = duration;
						data.width     = subtitleTrack->width;
		           			data.height    = subtitleTrack->height;
		           			//
		           			data.stream    = subtitleTrack->stream;
		           			data.ctx	= subtitleTrack->ctx;
		           			
						if (context->output->subtitle->Write(context, &data) < 0) 
						{
							ffmpeg_err("writing data to subtitle failed\n");
						}
					}
				}
			}            

			if (subpacket.data)
#if (LIBAVCODEC_VERSION_MAJOR > 55)
				av_packet_unref(&subpacket);
#else
				av_free_packet(&subpacket);
			av_packet_unref(&subpacket);
#endif
		}
		else  
		{
		    ffmpeg_err("no data ->end of file reached ? \n");
		    releaseMutex(FILENAME, __FUNCTION__,__LINE__);
		    break;
		}
		
		releaseMutex(FILENAME, __FUNCTION__,__LINE__);		
	} // while
	
	////
	av_packet_unref(&subpacket);

	hasPlaySubThreadStarted = 0;

	ffmpeg_printf(10, "terminating\n");
}

//
int container_ffmpeg_init_sub(Context_t *context, char * filename)
{
	int err;
	int n = 0;

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

	//
	subavContext = avformat_alloc_context();
	
#if LIBAVCODEC_VERSION_MAJOR < 54
	if ( (err = av_open_input_file(&subavContext, filename, NULL, 0, NULL)) != 0 ) 
#else
	if ( (err = avformat_open_input(&subavContext, filename, NULL, 0)) != 0 )
#endif
	{
		avformat_free_context(subavContext);
		
		return cERR_CONTAINER_FFMPEG_OPEN;
	}
	
	//
	subavContext->flags |= AVFMT_FLAG_GENPTS;
#if (LIBAVFORMAT_VERSION_MAJOR > 55) && (LIBAVFORMAT_VERSION_MAJOR < 56)
	subavContext->max_analyze_duration2 = 1;
#else
	subavContext->max_analyze_duration = 1;
#endif
	subavContext->probesize = 131072;

	// find stream info
#if LIBAVCODEC_VERSION_MAJOR < 54
	if (av_find_stream_info(subavContext) < 0)
#else
	if (avformat_find_stream_info(subavContext, NULL) < 0) 
#endif
	{
		ffmpeg_err("Error avformat_find_stream_info\n");
	}

	for ( n = 0; n < subavContext->nb_streams; n++) 
	{
		Track_t track;
		AVStream* stream = subavContext->streams[n];
		int version = 0;
		char *encoding = NULL;
		
		encoding = Codec2Encoding(stream->codecpar->codec_id, &version);
		
		//
		memset(&track, 0, sizeof(track));

		switch (stream->codecpar->codec_type) 
		{
			case AVMEDIA_TYPE_SUBTITLE:
			{
				ffmpeg_printf(10, "%d. encoding = %s - version %d CODEC_TYPE:%d codec_id 0x%x\n", n, encoding? encoding : "NULL", version, stream->codecpar->codec_type, stream->codecpar->codec_id);
					
#if LIBAVCODEC_VERSION_MAJOR < 54
				AVMetadataTag * lang;
#else
				AVDictionaryEntry * lang;
#endif

#if LIBAVCODEC_VERSION_MAJOR < 54
				lang = av_metadata_get(stream->metadata, "language", NULL, 0);
#else
				lang = av_dict_get(stream->metadata, "language", NULL, 0);
#endif	     

				if (lang)
					track.Name = strdup(lang->value);
				else
					track.Name = strdup("Sub");

				ffmpeg_printf(10, "Language %s\n", track.Name);

				track.Encoding       = encoding;
				track.Index          = n;

				track.stream         = stream;
				track.duration       = (double)stream->duration * av_q2d(stream->time_base) * 1000.0;

				track.aacbuf         = 0;
				track.have_aacheader = -1;

				track.width          = -1; /* will be filled online from videotrack */
				track.height         = -1; /* will be filled online from videotrack */

				track.extraData      = stream->codecpar->extradata;
				track.extraSize      = stream->codecpar->extradata_size;

				if(stream->duration == AV_NOPTS_VALUE) 
				{
					track.duration = (double) subavContext->duration / 1000.0;
				}
				else 
				{
					track.duration = (double) stream->duration * av_q2d(stream->time_base) * 1000.0;
				}
				
				ffmpeg_printf(10, "\n", track.height);
				
				// init codec
				subctx = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));

				avcodec_open2(subctx, avcodec_find_decoder(stream->codecpar->codec_id), NULL);
				track.ctx = subctx;

				if (context->manager->subtitle)
				{
					if (context->manager->extsubtitle->Command(context, MANAGER_ADD, &track) < 0) 
					{
						ffmpeg_err("failed to add subtitle track %d\n", n);
					}
				}
				break;
			} 
			
			default:
				break;
		}
	}
	
	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	return cERR_CONTAINER_FFMPEG_NO_ERROR;
}

//
static int container_ffmpeg_play_sub(Context_t *context)
{
	int error;
	int ret = 0;
	
	pthread_attr_t attr;

	ffmpeg_printf(10, "\n");

	if (hasPlaySubThreadStarted == 0) 
	{
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		if((error = pthread_create(&PlaySubThread, &attr, (void*)&FFMPEGSubThread, context)) != 0) 
		{
			ffmpeg_printf(10, "Error creating sub thread, error:%d:%s\n", error, strerror(error));

			hasPlaySubThreadStarted = 0;
			ret = cERR_CONTAINER_FFMPEG_ERR;
		}
		else 
		{
			ffmpeg_printf(10, "Created sub thread\n");

			hasPlaySubThreadStarted = 1;
		}
	}
	else 
	{
		ffmpeg_printf(10, "A Sub thread already exists!\n");

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
	hasPlaySubThreadStarted = 0;

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
	
	//
	if (subavContext != NULL)
	{
#if LIBAVFORMAT_VERSION_MAJOR < 54
 		av_close_input_file(subavContext);
 		subavContext = NULL;
#else
		avformat_close_input(&subavContext);
#endif		
	}

	isContainerRunning = 0;

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	ffmpeg_printf(10, "ret %d\n", ret);

	return ret;
}

//// FIXME:
static int container_ffmpeg_stop_sub(Context_t *context) 
{
	int ret = cERR_CONTAINER_FFMPEG_NO_ERROR;

	ffmpeg_printf(10, "\n");
	
	int wait_time = 20;

	while ( (hasPlaySubThreadStarted != 0) && (--wait_time) > 0 ) 
	{
		ffmpeg_printf(10, "Waiting for ffmpeg sub thread to terminate itself, will try another %d times\n", wait_time);

		usleep(100000);
	}

	if (wait_time == 0) 
	{
		ffmpeg_err( "Timeout waiting for sub thread!\n");

		ret = cERR_CONTAINER_FFMPEG_ERR;

		usleep(100000);
	}

	hasPlaySubThreadStarted = 0;

	getMutex(FILENAME, __FUNCTION__,__LINE__);
	
	//
	if (subavContext != NULL)
	{
#if LIBAVFORMAT_VERSION_MAJOR < 54
 		av_close_input_file(subavContext);
 		subavContext = NULL;
#else
		avformat_close_input(&subavContext);
#endif		
	}

	releaseMutex(FILENAME, __FUNCTION__,__LINE__);

	ffmpeg_printf(10, "ret %d\n", ret);

	return ret;
}

////
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

// seeking relative to a given byteposition N seconds ->for reverse playback needed
static int container_ffmpeg_seek_rel(Context_t *context, off_t pos, long long int pts, float sec) 
{
	Track_t * videoTrack = NULL;
	Track_t * audioTrack = NULL;
	Track_t * current = NULL;
	int flag = 0;
	
	////
	if (sec == 0.0)
	{
		ffmpeg_err("sec = 0.0 ignoring\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

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

	ffmpeg_printf(10, "seeking %f sec\n", sec);

	if (sec == 0.0)
	{
		ffmpeg_err("sec = 0.0 ignoring\n");
		return cERR_CONTAINER_FFMPEG_ERR;
	}

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

		pos += sec;

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
		sec += ((float) current->pts / 90000.0f);

		ffmpeg_printf(10, "2. seeking to position %f sec ->time base %f %d\n", sec, av_q2d(((AVStream*) current->stream)->time_base), AV_TIME_BASE);

		if (av_seek_frame(avContext, -1, sec * AV_TIME_BASE, flag) < 0) 
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

static int Command(void* _context, ContainerCmd_t command, void* argument)
{
	Context_t* context = (Context_t*) _context;
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
		
		case CONTAINER_INIT_SUB:
		{
			char* FILENAME = (char*)argument;
			ret = container_ffmpeg_init_sub(context, FILENAME);
			break;
		}
		
		case CONTAINER_PLAY:  
		{
			ret = container_ffmpeg_play(context);
			break;
		}
		
		case CONTAINER_PLAY_SUB:  
		{
			ret = container_ffmpeg_play_sub(context);
			break;
		}
		
		case CONTAINER_STOP:  
		{
			ret = container_ffmpeg_stop(context);
			break;
		}
		
		case CONTAINER_STOP_SUB:  
		{
			ret = container_ffmpeg_stop_sub(context);
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

//
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
	"m3u8",
	"amr", 
	"webm",
	"srt",
	"ass",
	"ssa",
	NULL 
};

//
Container_t FFMPEGContainer = {
	"FFMPEG",
	&Command,
	FFMPEG_Capabilities
};

