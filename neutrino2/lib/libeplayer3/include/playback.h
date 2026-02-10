/*
 * playback.h
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
 
 #ifndef PLAYBACK_H_
#define PLAYBACK_H_
#include <sys/types.h>
#include <stdbool.h>


typedef void(* PlaybackDieNowCallback)();
bool PlaybackDieNowRegisterCallback(PlaybackDieNowCallback callback);

typedef enum 
{
	PLAYBACK_INIT,
	PLAYBACK_OPEN,
	PLAYBACK_OPEN_SUB,
	PLAYBACK_CLOSE, 
	PLAYBACK_PLAY,
	PLAYBACK_PLAY_SUB,
	PLAYBACK_STOP, 
	PLAYBACK_PAUSE, 
	PLAYBACK_CONTINUE, 
	PLAYBACK_FLUSH, 
	PLAYBACK_TERM, 
	PLAYBACK_FASTFORWARD, 
	PLAYBACK_SEEK, 
	PLAYBACK_PTS, 
	PLAYBACK_LENGTH, 
	PLAYBACK_SWITCH_AUDIO, 	
	PLAYBACK_SWITCH_SUBTITLE,
	PLAYBACK_SWITCH_EXTSUBTITLE,
	PLAYBACK_INFO, 
	PLAYBACK_SLOWMOTION, 
	PLAYBACK_FASTBACKWARD, 
	PLAYBACK_GET_FRAME_COUNT
} PlaybackCmd_t;

typedef struct PlaybackHandler_s 
{
	char * Name;

	int fd;

	unsigned char isFile;
	unsigned char isHttp;
	unsigned char isUPNP;

	unsigned char isPlaying;
	unsigned char isPaused;
	unsigned char isForwarding;
	unsigned char isSeeking;
	unsigned char isCreationPhase;

	float BackWard;
	int SlowMotion;
	int Speed;
	int AVSync;

	unsigned char isVideo;
	unsigned char isAudio;    
	unsigned char isSubtitle;    

	int (* Command) (void  *, PlaybackCmd_t, void *);
	char * uri;
	char* suburi;
	off_t size;
} PlaybackHandler_t;

#endif

