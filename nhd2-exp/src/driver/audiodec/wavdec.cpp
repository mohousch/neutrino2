/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: wavdec.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2004 Zwen
	
	Homepage: http://www.dbox2.info/

	Kommentar:

	wav audio decoder
	
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <wavdec.h>
#include <sstream>

#include <linux/soundcard.h>
#include <driver/audioplay.h>
#include <system/debug.h>


#define ProgName "WavDec"

struct WavHeader
{
	char  ChunkID[4];
	int   ChunkSize;
	char  Format[4];
	char  Subchunk1ID[4];
	int   Subchunk1Size;
	short AudioFormat;
	short NumChannels;
	int   SampleRate;
	int   ByteRate;
	short BlockAlign;
	short BitsPerSample;
	char  Subchunk2ID[4];
	int   Subchunk2Size;
} __attribute__ ((packed));

int endianTest = 1;

#define Swap32IfBE(l) \
    (*(char *)&endianTest ? (l) : \
                             ((((l) & 0xff000000) >> 24) | \
                             (((l) & 0x00ff0000) >> 8)  | \
                             (((l) & 0x0000ff00) << 8)  | \
                             (((l) & 0x000000ff) << 24)))
#define Swap16IfBE(l) \
    (*(char *)&endianTest ? (l) : \
                             ((((l) & 0xff00) >> 8) | \
                             (((l) &  0x00ff) << 8)))

//#define MAX_OUTPUT_SAMPLES 1022 /* AVIA_GT_PCM_MAX_SAMPLES-1 */

bool CWavDec::GetMetaData(FILE *in, const bool nice, CAudioMetaData* m)
{
	return SetMetaData(in, m);
}

CWavDec* CWavDec::getInstance()
{
	static CWavDec* WavDec = NULL;
	if(WavDec == NULL)
	{
		WavDec = new CWavDec();
	}
	return WavDec;
}

bool CWavDec::SetMetaData(FILE* in, CAudioMetaData* m)
{
	/* Set Metadata */
	struct WavHeader wh;

	header_size = 44;
	
	fseek(in, 0, SEEK_END);
	int filesize = ftell(in);
	fseek(in, 0, SEEK_SET);
	if(fread(&wh, sizeof(wh), 1, in)!=1)
		return false;
	if(memcmp(wh.ChunkID, "RIFF", 4)!=0 ||
		memcmp(wh.Format, "WAVE", 4)!=0 ||
		Swap16IfBE(wh.AudioFormat) != 1)
	{
		dprintf(DEBUG_NORMAL, "%s: wrong format (header)\n", ProgName);
		return false;
	}
	m->type = CAudioMetaData::WAV;
	m->bitrate = Swap32IfBE(wh.ByteRate)*8;
	m->samplerate = Swap32IfBE(wh.SampleRate);
	mBitsPerSample = Swap16IfBE(wh.BitsPerSample);
	mChannels = Swap16IfBE(wh.NumChannels);
	m->total_time = (m->bitrate!=0) ? (filesize-header_size)*8 / m->bitrate : 0;
	std::stringstream ss;
	ss << "Riff/Wave / " << mChannels << "channel(s) / " << mBitsPerSample << "bit";
	m->type_info = ss.str();
	m->changed = true;
	
	return true;
}
 
