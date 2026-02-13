/*
 * output.h
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
 
 #ifndef OUTPUT_H_
#define OUTPUT_H_

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <config.h>


typedef enum {
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
	OUTPUT_GET_FRAME_COUNT
} OutputCmd_t;

// audio / video
typedef struct
{
	//
	unsigned char*         	data;
	unsigned int           	len;

	uint8_t         	*extradata;
	unsigned int           	extralen;
	
	uint64_t         	pts;
	
	float                  	frameRate;
	unsigned int           	timeScale;
	
	unsigned int           	width;
	unsigned int           	height;
	
	char*                  	type;
	
	//
#ifdef USE_OPENGL	
	AVStream* 		stream;
	AVCodecContext 		*ctx;
	AVFrame*		vframe;
	AVFrame* 		aframe;
#endif
} AudioVideoOut_t;

// subtitle
typedef struct
{
	//
	uint8_t 		*data;
	int            		len;

	uint8_t 		*extradata;
	int            		extralen;
	
	int64_t  		pts;
	float          		duration;
	
	int          		width;
    	int          		height;
    	
    	//
	AVStream* 		stream;
	AVCodecContext 		*ctx;
} SubtitleData_t;

typedef struct
{
	unsigned int 	screen_x;
	unsigned int 	screen_y;
	unsigned int   	screen_width;
	unsigned int   	screen_height;
} SubtitleOutputDef_t;

typedef struct Output_s 
{
	char * Name;
	int (* Command) (void*, OutputCmd_t, void*);
	int (* Write) (void*, void* privateData);
	char** Capabilities;
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
	int (* Command) (void*, OutputCmd_t, void*);
} OutputHandler_t;

#endif

