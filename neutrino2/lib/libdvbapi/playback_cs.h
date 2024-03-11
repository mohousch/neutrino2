/*
 * $Id: playback_cs.h,v 1.0 2013/08/18 11:23:30 mohousch Exp $
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __PLAYBACK_CS_H
#define __PLAYBACK_CS_H

#include <stdint.h>
#include <string>

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

#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#endif
}

#include <config.h>


class cPlayback
{
	private:
		int mSpeed;
		int mAudioStream;
		int mSubStream;
		
	public:
		bool playing;

		bool Open();
		void Close(void);
		bool Start(char * filename, const char *const suburi = NULL);
		
		bool Play(void);
		bool Stop(void);
		bool SetAPid(unsigned short pid, int _ac = 0);
		bool SetSubPid(unsigned short pid);

#if ENABLE_GSTREAMER
		void trickSeek(double ratio);
#endif		
		bool SetSpeed(int speed);
		bool SetSlow(int slow);
		bool GetSpeed(int &speed) const;
		bool GetPosition(int &position, int &duration);
		bool SetPosition(int position);
		void FindAllPids(uint16_t *apids, unsigned short *ac3flags, uint16_t *numpida, std::string *language);
		void FindAllSubPids(uint16_t *apids, uint16_t *numpida, std::string *language);

		cPlayback(int num = 0);
		~cPlayback(){};	
		
#ifdef USE_OPENGL
		void getDecBuf(uint8_t** buffer, int* width, int* height, int* rate, uint64_t* pts, AVRational* a);
#endif
};

#endif

