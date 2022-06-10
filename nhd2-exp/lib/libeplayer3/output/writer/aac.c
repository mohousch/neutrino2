/*
 * linuxdvb output/writer handling.
 *
 * konfetti 2010 based on linuxdvb.c code from libeplayer2
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

/* ***************************** */
/* Includes                      */
/* ***************************** */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/uio.h>

#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "pes.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define AAC_HEADER_LENGTH       7

static inline int HasADTSHeader(uint8_t *data, int size)
{
	if (size >= AAC_HEADER_LENGTH && 0xFF == data[0] && 0xF0 == 0xF0 & data[1] && size == ((data[3] & 0x3) << 11 | data[4] << 3 | data[5] >> 5))
	{
		return 1;
	}
	    
	return 0;
}

//#define AAC_DEBUG

#ifdef AAC_DEBUG

static short debug_level = 10;

#define aac_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define aac_printf(level, fmt, x...)
#endif

#ifndef AAC_SILENT
#define aac_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define aac_err(fmt, x...)
#endif

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

/// ** AAC ADTS format **
///
/// AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM
/// MMMMMMMM MMMNNNNN NNNNNNOO ........
///
/// Sign            Length          Position         Description
///
/// A                12             (31-20)          Sync code
/// B                 1              (19)            ID
/// C                 2             (18-17)          layer
/// D                 1              (16)            protect absent
/// E                 2             (15-14)          profile
/// F                 4             (13-10)          sample freq index
/// G                 1              (9)             private
/// H                 3             (8-6)            channel config
/// I                 1              (5)             original/copy
/// J                 1              (4)             home
/// K                 1              (3)             copyright id
/// L                 1              (2)             copyright start
/// M                 13         (1-0,31-21)         frame length
/// N                 11           (20-10)           adts buffer fullness
/// O                 2             (9-8)            num of raw data blocks in frame

/*
LC: Audio: aac, 44100 Hz, stereo, s16, 192 kb/ ->ff f1 50 80 00 1f fc
HE: Audio: aac, 48000 Hz, stereo, s16, 77 kb/s ->ff f1 4c 80 00 1f fc
*/

/*
ADIF = basic format called Audio Data Interchange Format (ADIF)
       consisting of a single header followed by the raw AAC audio data blocks
ADTS = streaming format called Audio Data Transport Stream (ADTS)
       consisting of a series of frames, each frame having a header followed by the AAC audio data
LOAS = Low Overhead Audio Stream (LOAS), a self-synchronizing streaming format
*/

/*
AvailableBytes = Writen Bytes
Sync = Bits.Get(11);
if (Sync == AAC_AUDIO_LOAS_ASS_SYNC_WORD{0x2b7}) 
   Type = AAC_AUDIO_LOAS_FORMAT;
   FrameSize = Bits.Get(13) + AAC_LOAS_ASS_SYNC_LENGTH_HEADER_SIZE{3};
   if (FrameSize > AAC_LOAS_ASS_MAX_FRAME_SIZE{8192})
      // ERROR
   AvailableBytes = AvailableBytes - AAC_LOAS_ASS_MAX_FRAME_SIZE{8192};
   
   ImplicitSbrExtension = true;
   ExplicitSbrExtension = false;
   
   if (AvailableBytes > 0)
      useSameStreamMux = Bits->Get(1);
   else
      useSameStreamMux = true;
   
   if ( !useSameStreamMux )
      audioMuxVersion = Bits->Get(1); // Has to be 0
      if (!audioMuxVersion)
         // only get program 0 and layer 0 information ...
         Bits->FlushUnseen(1 + 6 + 4 + 3); // allStreamSameTimeFraming, numSubFrames, numProgram, numLayer
         audioObjectType = Bits->Get(5);
         if ((audioObjectType != AAC_AUDIO_PROFILE_LC{2}) && (audioObjectType != AAC_AUDIO_PROFILE_SBR{5}))
            // Error
         
         samplingFrequencyIndex = Bits->Get(4);
         channelConfiguration = Bits->Get(4);
         if (audioObjectType == AAC_AUDIO_PROFILE_SBR{5})
            ImplicitSbrExtension = false;
            ExplicitSbrExtension = true;
            samplingFrequencyIndex = Bits->Get(4);
            audioObjectType = Bits->Get(5);
            if (audioObjectType != AAC_AUDIO_PROFILE_LC{2})
               // Error
      *SampleCount = 1024 * ((ImplicitSbrExtension || ExplicitSbrExtension)?2:1);
      *SamplingFrequency *= (ImplicitSbrExtension?2:1);
else
   Sync |= Bits.Get(1) << 11;
   if (Sync == AAC_AUDIO_ADTS_SYNC_WORD{0xfff})
      Type = AAC_AUDIO_ADTS_FORMAT; // Supports only LC
      ID = Bits.Get(1);
      Layer = Bits.Get(2); // Has to be 0
      protection_absent = Bits.Get(1);
      profile_ObjectType = Bits.Get(2);
      if ((profile_ObjectType+1) != AAC_AUDIO_PROFILE_LC)
         return
      sampling_frequency_index = Bits.Get(4);
      SamplingFrequency   = aac_sample_rates[sampling_frequency_index] * 2;
      Bits.FlushUnseen(1); //private_bit
      channel_configuration = Bits.Get(3);
      Bits.FlushUnseen(1 + 1 + 1 + 1); //original/copy, home, copyright_identification_bit, copyright_identification_start
      FrameSize = Bits.Get(13); // aac_frame_length
      if (FrameSize < AAC_ADTS_MIN_FRAME_SIZE{7})
         // Error 
      Bits.FlushUnseen(11); //adts_buffer_fullness
      no_raw_data_blocks_in_frame = Bits.Get(2);
      // multiple the sample count by two in case a sbr object is present
      SampleCount         = (no_raw_data_blocks_in_frame + 1) * 1024 * 2 ;
   else
      Sync |= Bits.Get(4) << 12;
      if (Sync == AAC_AUDIO_LOAS_EPASS_SYNC_WORD{0x4de1})
         Type = AAC_AUDIO_LOAS_FORMAT;
         ...
      else
         Sync |= Bits.Get(16) << 16;
         if (Sync == AAC_AUDIO_ADIF_SYNC_WORD{0x41444946})
            Type = AAC_AUDIO_ADIF_FORMAT;
            //not supported
      
   
*/

static unsigned char DefaultAACHeader[] =  
{
	0xff,
	0xf1,
	/*0x00, 0x00*/0x50,  //((Profile & 0x03) << 6)  | (SampleIndex << 2) | ((Channels >> 2) & 0x01);s
	0x80,                //(Channels & 0x03) << 6;
	0x00,
	0x1f,
	0xfc
};

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
static int reset()
{
    return 0;
}

static int writeData(void* _call)
{
	WriterAVCallData_t *call = (WriterAVCallData_t *) _call;
	unsigned char PesHeader[PES_MAX_HEADER_SIZE];
	unsigned char ExtraData[AAC_HEADER_LENGTH];

	aac_printf(10, "\n");

	if (call == NULL)
	{
		aac_err("call data is NULL...\n");
		return 0;
	}

	aac_printf(10, "AudioPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		aac_err("parsing NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		aac_err("file pointer < 0. ignoring ...\n");
		return 0;
	}
	
	/*
	if (0xFF != call->data[0] || 0xF0 != (0xF0 & call->data[1]))
	{
		aac_err("parsing Data with missing syncword. ignoring...\n");
		return 0;
	}

	// STB can handle only AAC LC profile
	if (0 == (call->data[2] & 0xC0))
	{
		// change profile AAC Main -> AAC LC (Low Complexity)
		aac_printf(1, "change profile AAC Main -> AAC LC (Low Complexity) in the ADTS header");
		call->data[2] = (call->data[2] & 0x1F) | 0x40;
	}
	*/

	//
#if defined (__sh__)
        if( (call->private_data && 0 == strncmp("ADTS", (const char*)call->private_data, call->private_size)) || HasADTSHeader(call->data, call->len) )
    	{    	
		unsigned int  HeaderLength = InsertPesHeader(PesHeader, call->len, AAC_AUDIO_PES_START_CODE, call->Pts, 0);
		
		struct iovec iov[2];
			
		iov[0].iov_base = PesHeader;
		iov[0].iov_len = HeaderLength;
		iov[1].iov_base = call->data;
		iov[1].iov_len = call->len;
			
		return call->WriteV(call->fd, iov, 2);
	}
	else
	{	
		unsigned int  PacketLength = call->len + AAC_HEADER_LENGTH;
	
		if (call->private_data == NULL)
		{
			aac_printf(10, "private_data = NULL\n");
			
			call->private_data = DefaultAACHeader;
			call->private_size = AAC_HEADER_LENGTH;
		}
			
		memcpy(ExtraData, call->private_data, AAC_HEADER_LENGTH);
		//ExtraData[3]       |= (PacketLength >> 11) & 0x3;
		ExtraData[4]        = (PacketLength >> 3) & 0xff;
		ExtraData[5]       |= (PacketLength << 5) & 0xe0;
			
		unsigned int  HeaderLength = InsertPesHeader(PesHeader, PacketLength, AAC_AUDIO_PES_START_CODE, call->Pts, 0);
		
		struct iovec iov[3];

		iov[0].iov_base = PesHeader;
		iov[0].iov_len = HeaderLength;
		iov[1].iov_base = ExtraData;
		iov[1].iov_len = AAC_HEADER_LENGTH;
		iov[2].iov_base = call->data;
		iov[2].iov_len = call->len;

		return call->WriteV(call->fd, iov, 3);		
	}
#else
	if( (call->private_data && 0 == strncmp("ADTS", (const char*)call->private_data, call->private_size)) || HasADTSHeader(call->data, call->len) )
    	{  
		unsigned int  HeaderLength = InsertPesHeader(PesHeader, call->len, MPEG_AUDIO_PES_START_CODE, call->Pts, 0);

		struct iovec iov[2];
		
		iov[0].iov_base = PesHeader;
		iov[0].iov_len  = HeaderLength;
		iov[1].iov_base = call->data;
		iov[1].iov_len  = call->len;
		
		return call->WriteV(call->fd, iov, 2);
	}
	else
	{	
		unsigned int  PacketLength = call->len + AAC_HEADER_LENGTH;
	
		if (call->private_data == NULL)
		{
			aac_printf(10, "private_data = NULL\n");
			
			call->private_data = DefaultAACHeader;
			call->private_size = AAC_HEADER_LENGTH;
		}
			
		memcpy(ExtraData, call->private_data, AAC_HEADER_LENGTH);
		//ExtraData[3]       |= (PacketLength >> 11) & 0x3;
		ExtraData[4]        = (PacketLength >> 3) & 0xff;
		ExtraData[5]       |= (PacketLength << 5) & 0xe0;
			
		unsigned int  HeaderLength = InsertPesHeader(PesHeader, PacketLength, AAC_AUDIO_PES_START_CODE, call->Pts, 0);
		
		struct iovec iov[3];

		iov[0].iov_base = PesHeader;
		iov[0].iov_len = HeaderLength;
		iov[1].iov_base = ExtraData;
		iov[1].iov_len = AAC_HEADER_LENGTH;
		iov[2].iov_base = call->data;
		iov[2].iov_len = call->len;

		return call->WriteV(call->fd, iov, 3);		
	}
#endif	
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

static WriterCaps_t caps = {
	"aac",
	eAudio,
	"A_AAC",
	AUDIO_STREAMTYPE_AAC
};

struct Writer_s WriterAudioAAC = {
	&reset,
	&writeData,
	NULL,
	&caps
};


