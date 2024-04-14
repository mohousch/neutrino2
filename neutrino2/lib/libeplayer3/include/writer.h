#ifndef WRITER_H_
#define WRITER_H_

#include <stdio.h>
#include <sys/uio.h>
#include <stdint.h>

#include "common.h"


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
	VIDEO_STREAMTYPE_SPARK 		= 21,
	VIDEO_STREAMTYPE_MJPEG 		= 30,
	VIDEO_STREAMTYPE_RV30 		= 31, 
	VIDEO_STREAMTYPE_RV40 		= 32,
	VIDEO_STREAMTYPE_AVS2 		= 40
}VIDEO_FORMAT;

// audio stream type
typedef enum {
	AUDIO_STREAMTYPE_AC3 		= 0,
	AUDIO_STREAMTYPE_MPEG 		= 1,
	AUDIO_STREAMTYPE_DTS 		= 2,
	AUDIO_STREAMTYPE_LPCMDVD 	= 6,
	AUDIO_STREAMTYPE_AAC 		= 8,
	AUDIO_STREAMTYPE_AAC_HE 	= 9,
	AUDIO_STREAMTYPE_MP3 		= 0xa,
	AUDIO_STREAMTYPE_AAC_PLUS 	= 0xb,
	AUDIO_STREAMTYPE_DTSHD 		= 0x10,
	AUDIO_STREAMTYPE_WMA 		= 0x20,
	AUDIO_STREAMTYPE_WMA_PRO 	= 0x21,
#if defined (PLATFORM_DREAMBOX)
	AUDIO_STREAMTYPE_EAC3 		= 7,
#else
	AUDIO_STREAMTYPE_EAC3 		= 0x22,
#endif
	AUDIO_STREAMTYPE_AMR 		= 0x23,
	AUDIO_STREAMTYPE_OPUS 		= 0x24,
	AUDIO_STREAMTYPE_VORBIS 	= 0x25,
	AUDIO_STREAMTYPE_RAW 		= 0x30
}AUDIO_FORMAT;

typedef enum 
{ 
	eNone, 
	eAudio, 
	eVideo, 
	eGfx
} eWriterType_t;

typedef ssize_t (* WriteV_t)(int, const struct iovec *, int);

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
	uint8_t*         	data;
	unsigned int           	Width;
	unsigned int           	Height;
	unsigned int           	Stride;
	unsigned int           	color;

	unsigned int           	x;       /* dst x ->given by ass */
	unsigned int           	y;       /* dst y ->given by ass */

	/* destination values if we use a shared framebuffer */
	int                    	fd;
	unsigned int           	Screen_Width;
	unsigned int           	Screen_Height;
	uint32_t*         	destination;
	unsigned int           	destStride;
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

// audio
extern Writer_t WriterAudioPCM;
extern Writer_t WriterAudioIPCM;
extern Writer_t WriterAudioLPCM;
extern Writer_t WriterAudioMP3;
extern Writer_t WriterAudioMPEGL3;
extern Writer_t WriterAudioAC3;
extern Writer_t WriterAudioEAC3;
extern Writer_t WriterAudioAAC;
extern Writer_t WriterAudioDTS;
extern Writer_t WriterAudioDTSHD;
extern Writer_t WriterAudioWMA;
extern Writer_t WriterAudioWMAPRO;
extern Writer_t WriterAudioVORBIS;
extern Writer_t WriterAudioOPUS;
extern Writer_t WriterAudioAMR;
// video
extern Writer_t WriterVideoMPEG1;
extern Writer_t WriterVideoMPEG2;
extern Writer_t WriterVideoH263;
extern Writer_t WriterVideoFLV1;
extern Writer_t WriterVideoH264;
extern Writer_t WriterVideoH265;
extern Writer_t WriterVideoWMV;
extern Writer_t WriterVideoDIVX;
extern Writer_t WriterVideoDIVX3;
extern Writer_t WriterVideoVC1;
extern Writer_t WriterVideoMJPEG;
extern Writer_t WriterVideoRV30;
extern Writer_t WriterVideoRV40;
extern Writer_t WriterVideoAVS2;
extern Writer_t WriterVideoVP6;
extern Writer_t WriterVideoVP8;
extern Writer_t WriterVideoVP9;
// subtitle
extern Writer_t WriterFramebuffer;

//
static Writer_t * AvailableWriter[] = 
{
	&WriterAudioPCM,
	&WriterAudioIPCM,
	&WriterAudioLPCM,
	&WriterAudioMP3,
	&WriterAudioMPEGL3,
	&WriterAudioAC3,
	&WriterAudioEAC3,
	&WriterAudioAAC,
	&WriterAudioDTS,
	&WriterAudioDTS,
	&WriterAudioWMA,
	&WriterAudioWMAPRO,
	&WriterAudioVORBIS,
	&WriterAudioOPUS,
	//
	&WriterVideoMPEG1,
	&WriterVideoMPEG2,
	&WriterVideoH263,
	&WriterVideoFLV1,
	&WriterVideoH264,
	&WriterVideoH265,
	&WriterVideoWMV,
	&WriterVideoDIVX,
	&WriterVideoDIVX3,
	&WriterVideoVC1,
	&WriterVideoMJPEG,
	&WriterVideoRV30,
	&WriterVideoRV40,
	&WriterVideoAVS2,
	&WriterVideoVP6,
	&WriterVideoVP8,
	&WriterVideoVP9,
	//   
	&WriterFramebuffer,   
	NULL
};

Writer_t* getWriter(char* encoding);
Writer_t* getDefaultVideoWriter();
Writer_t* getDefaultAudioWriter();
Writer_t* getDefaultFramebufferWriter();

//
ssize_t WriteExt(WriteV_t _call, int fd, void *data, size_t size); // used in mpeg2.c
ssize_t write_with_retry(int fd, const void *buf, int size);
ssize_t writev_with_retry(int fd, const struct iovec *iov, int ic);

#endif

