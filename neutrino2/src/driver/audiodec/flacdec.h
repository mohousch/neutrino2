/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: flacdec.h 2013/10/12 mohousch Exp $

	Copyright (C) 2002,2003,2004 Sania,Zwen
	
	flacc audio decoder
	Homepage: http://www.dbox2.info/

	Kommentar:

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


#ifndef __FLAC_DEC__
#define __FLAC_DEC__

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <basedec.h>
#include <driver/audiometadata.h>
#include <FLAC/all.h>

#define DECODE_SLOTS 30

class CFlacDec : public CBaseDec
{
	public:
		static CFlacDec *getInstance();
		bool GetMetaData(FILE *in, const bool nice, CAudioMetaData* m);
		CFlacDec(){ mMetadata = NULL; };

		int mWriteSlot;
		int mReadSlot;
		int mSlotSize;
		unsigned long mFrameCount;
		unsigned int mChannels;
		unsigned int mSampleRate;
		unsigned long mBytes;
		unsigned int mBps;
		unsigned int mSamplesProcessed;
		unsigned int mBuffersize;
		unsigned long mTotalSamples;
		unsigned int mLengthInMsec;
		FLAC__StreamDecoder *mFlacDec;
		FILE* mIn;
		char* mPcmSlots[DECODE_SLOTS];
		FLAC__uint64 mSlotTime[DECODE_SLOTS];
		int mOutputFd;
		RetCode Status;
		FLAC__StreamMetadata *mMetadata;

	private:
		void ParseUserComments(FLAC__StreamMetadata_VorbisComment*, CAudioMetaData*);
		bool Open(FILE*, FLAC__StreamDecoder*);
		void SetMetaData(FLAC__StreamMetadata*, CAudioMetaData*);
		State* mState;
		bool mSeekable;
		time_t* mTimePlayed;
};

#endif

 
