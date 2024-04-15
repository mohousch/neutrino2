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
#include <vector>

#include <config.h>

#ifdef USE_OPENGL
extern "C" {
#include <libavutil/rational.h>
}
#endif


class cPlayback
{
	private:
		int mSpeed;
		int mAudioStream;
		int mSubStream;
		int mExtSubStream;
		
#ifdef USE_OPENGL
	public:
		////
		class SWFramebuffer : public std::vector<unsigned char>
		{
			public:
				SWFramebuffer() : mWidth(0), mHeight(0) {};
				
				void width(int w)
				{
					mWidth = w;
				}
				
				void height(int h)
				{
					mHeight = h;
				}
				
				void vpts(uint64_t p)
				{
					mVPts = p;
				}
				
				void apts(uint64_t p)
				{
					mAPts = p;
				}
				
				void AR(AVRational a)
				{
					mAR = a;
				}
				
				void rate(int r)
				{
					mRate = r;
				}
				
				int width() const
				{
					return mWidth;
				}
				
				int height() const
				{
					return mHeight;
				}
				
				int64_t vpts() const
				{
					return mVPts;
				}
				
				int64_t apts() const
				{
					return mAPts;
				}
				
				AVRational AR() const
				{
					return mAR;
				}
				
				int rate() const
				{
					return mRate;
				}
				
			private:
				int mWidth;
				int mHeight;
				int64_t mVPts;
				int64_t mAPts;
				AVRational mAR;
				int mRate;
		};
		
		SWFramebuffer buffers[64];
#endif
		
	public:
		bool playing;

		bool Open();
		void Close(void);
		bool Start(char * filename);
		
		bool Play(void);
		bool Stop(void);
		bool SetAPid(unsigned short pid);
		bool SetSubPid(short pid);
		bool SetExtSubPid(short pid);

#if ENABLE_GSTREAMER
		void trickSeek(double ratio);
#endif		
		bool SetSpeed(int speed);
		bool SetSlow(int slow);
		bool GetSpeed(int &speed) const;
		bool GetPosition(int &position, int &duration);
		void GetPts(uint64_t &pts);
		bool SetPosition(int position);
		void FindAllPids(uint16_t *apids, bool *ac3flags, uint16_t *numpida, std::string *language);
		void FindAllSubPids(uint16_t *apids, uint16_t *numpida, std::string *language);
		void FindAllExtSubPids(uint16_t *apids, uint16_t *numpida, std::string *language);
		void AddSubtitleFile(const char* const file);

		cPlayback(int num = 0);
		~cPlayback(){};	
		
#ifdef USE_OPENGL
#ifndef ENABLE_GSTREAMER
		cPlayback::SWFramebuffer* getDecBuf(void);
#endif
#endif
};

#endif

