/*
	$Id: audio_cs.h 2016.06.22 11:48:30 mohousch Exp $
	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _AUDIO_CS_H_
#define _AUDIO_CS_H_

#include <linux/dvb/audio.h>

#include <config.h>
#include <string>

#include <OpenThreads/Thread>

#include <zapit/frontend_c.h>


#ifndef AUDIO_SOURCE_HDMI
#define AUDIO_SOURCE_HDMI 2
#endif

// audio stream type
typedef enum {
	AUDIO_STREAMTYPE_AC3 		= 0,
	AUDIO_STREAMTYPE_MPEG 		= 1,
	AUDIO_STREAMTYPE_DTS 		= 2,
	AUDIO_STREAMTYPE_LPCMDVD 	= 6,
	AUDIO_STREAMTYPE_AAC 		= 8,
	AUDIO_STREAMTYPE_AACPLUS 	= 9,
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
	AUDIO_STREAMTYPE_RAW 		= 0x30
}AUDIO_FORMAT;

// av sync
enum {
	AVSYNC_OFF,
	AVSYNC_ON,
	AVSYNC_AM
};

// ac3
enum {
	AC3_DOWNMIX,
	AC3_PASSTHROUGH
};


class cAudio : public OpenThreads::Thread
{
	private:
		int audio_fd;
		int audio_num;
		int audio_adapter;	
		
		bool Muted;
		AUDIO_FORMAT StreamType;
	
		int volume;
		
		int m_pcm_delay;
		int m_ac3_delay;
		
#ifdef USE_OPENGL
		bool started;
		bool thread_started;
		int64_t curr_pts;
		void run();
#endif		
		
	public:
		// construct & destruct
		cAudio(int num = 0);
		~cAudio(void);
		
		// shut up
		int SetMute(int enable);

		// volume, min = 0, max = 255
		int setVolume(unsigned int left, unsigned int right);
		int getVolume(void) { return volume;}

		// start and stop audio
		int Start(void);
		int Stop(void);
		bool Pause();
		bool Resume();

		void SetStreamType(AUDIO_FORMAT type);
		AUDIO_FORMAT GetStreamType(void) { return StreamType; }
		
		void SetSyncMode(int Mode);

		bool Open(CFrontend * fe = NULL);
		bool Close();

		// flush	
		int Flush(void);	

		// select channels
		int setChannel(int channel);
		
		void SetHdmiDD(int ac3);
		
		// audio stream source		
		int setSource(int source = AUDIO_SOURCE_DEMUX);
		
		int setHwPCMDelay(int delay);
		int setHwAC3Delay(int delay);
		
#ifdef USE_OPENGL
		int my_read(uint8_t *buf, int buf_size);
		int64_t getPts()
		{
			return curr_pts;
		}
#endif
};

#endif

