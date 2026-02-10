/*
 * manager.h
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
 
 #ifndef MANAGER_H_
#define MANAGER_H_

#include <stdio.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <config.h>

 
typedef enum 
{
	MANAGER_ADD,
	MANAGER_LIST,
	MANAGER_GET,
	MANAGER_GETNAME,
	MANAGER_SET,
	MANAGER_GETENCODING,
	MANAGER_DEL,
	MANAGER_GET_TRACK,
	MANAGER_GET_TRACKCOUNT
} ManagerCmd_t;

typedef enum 
{
	eTypeES,
	eTypePES
} eTrackTypeEplayer;

typedef struct Track_s 
{
	char*                 Name;
	char*                 Encoding;
	int                   Index;

	//
	char*                 language;

	// length of track
	long long int         duration;
	unsigned int          frame_rate;
	unsigned int          TimeScale;
	int                   version;
	long long int         pts;

	// for later use:
	eTrackTypeEplayer     type;
	int                   width;
	int                   height;

	// stream from ffmpeg
	AVStream* 	      stream;
	AVCodecContext 		*ctx;

	// codec extra data (header or some other stuff)
	void* 		      extraData;
	int		      extraSize;

	// aac header
	uint8_t*              aacbuf;
	unsigned int          aacbuflen;
	int                   have_aacheader;

	//
	int                   inject_as_pcm;
} Track_t;

typedef struct Manager_s 
{
	char * Name;
	int (* Command) (void  *, ManagerCmd_t, void *);
	char ** Capabilities;
} Manager_t;

typedef struct ManagerHandler_s 
{
	char * Name;
	Manager_t * audio;
	Manager_t * video;    
	Manager_t * subtitle;
	Manager_t * extsubtitle; 
} ManagerHandler_t;

void freeTrack(Track_t* track);
void copyTrack(Track_t* to, Track_t* from);

#endif

