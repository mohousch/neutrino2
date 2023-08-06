#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <stdio.h>

#include <libavcodec/avcodec.h>


typedef enum {
	OUTPUT_INIT,
	OUTPUT_ADD,
	OUTPUT_DEL,
	OUTPUT_CAPABILITIES,
	OUTPUT_PLAY,
	OUTPUT_STOP,
	OUTPUT_PAUSE,
	OUTPUT_OPEN,
	OUTPUT_CLOSE,
	OUTPUT_FLUSH,
	OUTPUT_CONTINUE,
	OUTPUT_FASTFORWARD,
	OUTPUT_AVSYNC,
	OUTPUT_CLEAR,
	OUTPUT_PTS,
	OUTPUT_SWITCH,
	OUTPUT_SLOWMOTION,
	OUTPUT_AUDIOMUTE,
	OUTPUT_REVERSE,
	OUTPUT_DISCONTINUITY_REVERSE,
	OUTPUT_GET_FRAME_COUNT,
	OUTPUT_GET_SUBTITLE_OUTPUT,
	OUTPUT_SET_SUBTITLE_OUTPUT,
	OUTPUT_DATA   
} OutputCmd_t;

typedef struct
{
	//
	enum AVCodecID 	      avCodecId;
	
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

typedef struct Output_s 
{
	char * Name;
	int (* Command) (void  *, OutputCmd_t, void *);
	int (* Write) (void  *, void* privateData);
	char ** Capabilities;
} Output_t;

//
extern Output_t LinuxDvbOutput;
extern Output_t SubtitleOutput;

static Output_t * AvailableOutput[] = 
{
	&LinuxDvbOutput,    
	&SubtitleOutput,    
	NULL
};

typedef struct OutputHandler_s 
{
	char * Name;
	Output_t * audio;
	Output_t * video;    
	Output_t * subtitle;    
	int (* Command) (void  *, OutputCmd_t, void *);
} OutputHandler_t;

#endif


