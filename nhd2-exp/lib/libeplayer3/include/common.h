#ifndef COMMON_H_
#define COMMON_H_

#include "container.h"
#include "output.h"
#include "manager.h"
#include "playback.h"
#include <pthread.h>


typedef struct Context_s 
{
	PlaybackHandler_t* playback;
	ContainerHandler_t* container;
	OutputHandler_t* output;
	ManagerHandler_t* manager;
} Context_t;

// video stream type 
typedef enum {
	VIDEO_STREAMTYPE_MPEG2,
	VIDEO_STREAMTYPE_MPEG4_H264,
	VIDEO_STREAMTYPE_MPEG4_H263,
	VIDEO_STREAMTYPE_VC1,
	VIDEO_STREAMTYPE_MPEG4_Part2,
	VIDEO_STREAMTYPE_VC1_SM,
	VIDEO_STREAMTYPE_MPEG1,
	VIDEO_STREAMTYPE_H265_HEVC,
	VIDEO_STREAMTYPE_AVS = 16
}VIDEO_FORMAT;

// audio stream type
typedef enum {
	AUDIO_STREAMTYPE_AC3 = 0,
	AUDIO_STREAMTYPE_MPEG,
	AUDIO_STREAMTYPE_DTS,
	AUDIO_STREAMTYPE_LPCMDVD = 6,
	AUDIO_STREAMTYPE_AAC = 8,
	AUDIO_STREAMTYPE_AACPLUS,
	AUDIO_STREAMTYPE_MP3,
	AUDIO_STREAMTYPE_DTSHD = 0x10,
#if defined (PLATFORM_DREAMBOX)
	AUDIO_STREAMTYPE_EAC3 = 7
#else
	AUDIO_STREAMTYPE_EAC3 = 0x22
#endif
}AUDIO_FORMAT;

#endif


