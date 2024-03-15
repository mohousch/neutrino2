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

#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "pes.h"
#include "writer.h"
#include "pcm.h"

#include <libavcodec/avcodec.h>

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */


//#define PCM_DEBUG

#ifdef PCM_DEBUG

static short debug_level = 10;

#define pcm_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define pcm_printf(level, fmt, x...)
#endif

#ifndef PCM_SILENT
#define pcm_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define pcm_err(fmt, x...)
#endif

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static int initialHeader = 1;

//
static uint8_t codec_data[18];
static uint64_t fixed_buffertimestamp;
static uint64_t fixed_bufferduration;
static uint32_t fixed_buffersize;
static uint8_t *fixed_buffer;
static uint32_t fixed_bufferfilled;

//
static unsigned int SubFrameLen = 0;
static unsigned int SubFramesPerPES = 0;

static const unsigned char clpcm_pes[18] = {   0x00, 0x00, 0x01, 0xBD, //start code
					0x07, 0xF1,             //pes length
					0x81, 0x81, 0x09,       //fixed
					0x21, 0x00, 0x01, 0x00, 0x01, //PTS marker bits
					0x1E, 0x60, 0x0A,       //first pes only, 0xFF after
					0xFF
			};
			
static const unsigned char clpcm_prv[14] = {   0xA0,   //sub_stream_id
					0, 0,   //resvd and UPC_EAN_ISRC stuff, unused
					0x0A,   //private header length
					0, 9,   //first_access_unit_pointer
					0x00,   //emph,rsvd,stereo,downmix
					0x0F,   //quantisation word length 1,2
					0x0F,   //audio sampling freqency 1,2
					0,      //resvd, multi channel type
					0,      //bit shift on channel GR2, assignment
					0x80,   //dynamic range control
					0, 0    //resvd for copyright management
			};

static unsigned char lpcm_pes[18];
static unsigned char lpcm_prv[14];

static unsigned char breakBuffer[8192];
static unsigned int breakBufferFillSize = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

static int prepareClipPlay(int uNoOfChannels, int uSampleRate, int uBitsPerSample, int bLittleEndian)
{
	pcm_printf(10, "rate: %d ch: %d bits: %d (%d bps)\n",
		uSampleRate/*Format->dwSamplesPerSec*/,
		uNoOfChannels/*Format->wChannels*/,
		uBitsPerSample/*Format->wBitsPerSample*/,
		(uBitsPerSample/*Format->wBitsPerSample*/ / 8)
	);

	SubFrameLen = 0;
	SubFramesPerPES = 0;
	breakBufferFillSize = 0;

	memcpy(lpcm_pes, clpcm_pes, sizeof(lpcm_pes));
	memcpy(lpcm_prv, clpcm_prv, sizeof(lpcm_prv));

	//figure out size of subframe
	//and set up sample rate
	switch(uSampleRate) 
	{
		case 48000:             
			SubFrameLen = 40;
			break;
		case 96000:             
			lpcm_prv[8] |= 0x10;
			SubFrameLen = 80;
			break;
		case 192000:    
			lpcm_prv[8] |= 0x20;
			SubFrameLen = 160;
			break;
		case 44100:             
			lpcm_prv[8] |= 0x80;
			SubFrameLen = 40;
			break;
		case 88200:             
			lpcm_prv[8] |= 0x90;
			SubFrameLen = 80;
			break;
		case 176400:    
			lpcm_prv[8] |= 0xA0;
			SubFrameLen = 160;
			break;
		default:                
			break;
	}

	SubFrameLen *= uNoOfChannels;
	SubFrameLen *= (uBitsPerSample / 8);

	//rewrite PES size to have as many complete subframes per PES as we can
	SubFramesPerPES = ((2048-sizeof(lpcm_pes))-sizeof(lpcm_prv))/SubFrameLen;
	SubFrameLen *= SubFramesPerPES;

	lpcm_pes[4] = ((SubFrameLen+(sizeof(lpcm_pes)-6)+sizeof(lpcm_prv))>>8) & 0xFF;
	lpcm_pes[5] =  (SubFrameLen+(sizeof(lpcm_pes)-6)+sizeof(lpcm_prv))     & 0xFF;

	//set number of channels
	lpcm_prv[10]  = uNoOfChannels - 1;

	switch(uBitsPerSample) 
	{
		case 16: 
			break;
		case 24: 
			lpcm_prv[7] |= 0x20;
			break;
		default:        
			printf("inappropriate bits per sample (%d) - must be 16 or 24\n",uBitsPerSample);
			return 1;
	}

	return 0;
}

static int reset()
{
	initialHeader = 1;
	return 0;
}

static int writeData(void* _call)
{
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

#if defined (__sh__)
	unsigned char  PesHeader[PES_MAX_HEADER_SIZE];
#else
	static uint8_t  PesHeader[PES_MAX_HEADER_SIZE + 22];
#endif

	pcm_printf(10, "\n");

	if (call == NULL)
	{
	    pcm_err("call data is NULL...\n");
	    return 0;
	}

	pcm_printf(10, "AudioPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
	    pcm_err("parsing NULL Data. ignoring...\n");
	    return 0;
	}

	if (call->fd < 0)
	{
	    pcm_err("file pointer < 0. ignoring ...\n");
	    return 0;
	}

	//
	pcmPrivateData_t* pcmPrivateData = (pcmPrivateData_t*)call->private_data;
	
	uint8_t *buffer = call->data;
	uint32_t size = call->len;

#if defined (__sh__)
	if (initialHeader)
	{
		uint32_t codecID = (uint32_t)pcmPrivateData->avCodecId;
		uint8_t LE = 0;
		
		switch (codecID)
		{
			case AV_CODEC_ID_PCM_S8:
			case AV_CODEC_ID_PCM_U8:
				break;
			case AV_CODEC_ID_PCM_S16LE:
			case AV_CODEC_ID_PCM_U16LE:
				LE = 1;
			case AV_CODEC_ID_PCM_S16BE:
			case AV_CODEC_ID_PCM_U16BE:
				break;
			case AV_CODEC_ID_PCM_S24LE:
			case AV_CODEC_ID_PCM_U24LE:
				LE = 1;
			case AV_CODEC_ID_PCM_S24BE:
			case AV_CODEC_ID_PCM_U24BE:
				break;
			case AV_CODEC_ID_PCM_S32LE:
			case AV_CODEC_ID_PCM_U32LE:
				LE = 1;
			case AV_CODEC_ID_PCM_S32BE:
			case AV_CODEC_ID_PCM_U32BE:
				break;
			default:
				break;
		}
		initialHeader = 0;
		prepareClipPlay(pcmPrivateData->uNoOfChannels, pcmPrivateData->uSampleRate, pcmPrivateData->uBitsPerSample, LE);
	}

	uint32_t n;
	uint8_t *injectBuffer = malloc(SubFrameLen);
	uint32_t pos;

	for (pos = 0; pos < size;)
	{
		//
		if ((size - pos) < SubFrameLen)
		{
			breakBufferFillSize = size - pos;
			memcpy(breakBuffer, &buffer[pos], sizeof(uint8_t) * breakBufferFillSize);
			break;
		}

		//get first PES's worth
		if (breakBufferFillSize > 0)
		{
			memcpy(injectBuffer, breakBuffer, sizeof(uint8_t)*breakBufferFillSize);
			memcpy(&injectBuffer[breakBufferFillSize], &buffer[pos], sizeof(unsigned char) * (SubFrameLen - breakBufferFillSize));
			pos += (SubFrameLen - breakBufferFillSize);
			breakBufferFillSize = 0;
		}
		else
		{
			memcpy(injectBuffer, &buffer[pos], sizeof(uint8_t)*SubFrameLen);
			pos += SubFrameLen;
		}

		struct iovec iov[3];
		iov[0].iov_base = PesHeader;
		iov[1].iov_base = lpcm_prv;
		iov[1].iov_len = sizeof(lpcm_prv);

		iov[2].iov_base = injectBuffer;
		iov[2].iov_len = SubFrameLen;

		//write the PCM data
		if (16 == pcmPrivateData->uBitsPerSample)
		{
			for (n = 0; n < SubFrameLen; n += 2)
			{
				uint8_t tmp;
				tmp = injectBuffer[n];
				injectBuffer[n] = injectBuffer[n + 1];
				injectBuffer[n + 1] = tmp;
			}
		}
		else
		{
			//      0   1   2   3   4   5   6   7   8   9  10  11
			//    A1c A1b A1a-B1c B1b B1a-A2c A2b A2a-B2c B2b B2a
			// to A1a A1b B1a B1b.A2a A2b B2a B2b-A1c B1c A2c B2c
			for (n = 0; n < SubFrameLen; n += 12)
			{
				unsigned char t, *p = &injectBuffer[n];
				t = p[0];
				p[ 0] = p[ 2];
				p[ 2] = p[ 5];
				p[ 5] = p[ 7];
				p[ 7] = p[11];
				p[11] = p[ 9];
				p[ 9] = p[ 3];
				p[ 3] = p[ 4];
				p[ 4] = p[ 8];
				p[ 8] = t;
			}
		}

		//increment err... subframe count?
		lpcm_prv[1] = ((lpcm_prv[1] + SubFramesPerPES) & 0x1F);

		iov[0].iov_len = InsertPesHeader(PesHeader, iov[1].iov_len + iov[2].iov_len, PCM_PES_START_CODE, call->Pts, 0);
		int32_t len = call->WriteV(call->fd, iov, 3);
		
		if (len < 0)
		{
			break;
		}
	}
	
	free(injectBuffer);
#else
	if (NULL == fixed_buffer)
	{
		int32_t width = 0;
		int32_t depth = 0;
		int32_t rate = (uint64_t)pcmPrivateData->uSampleRate;
		int32_t channels = (uint8_t) pcmPrivateData->uNoOfChannels;
		int32_t block_align = 0;
		int32_t byterate = 0;

		uint32_t codecID = (uint32_t)pcmPrivateData->avCodecId;

		//uint8_t dataPrecision = 0;
		uint8_t LE = 0;
		switch (codecID)
		{
			case AV_CODEC_ID_PCM_S8:
			case AV_CODEC_ID_PCM_U8:
				width = depth = 8;
				break;
			case AV_CODEC_ID_PCM_S16LE:
			case AV_CODEC_ID_PCM_U16LE:
				LE = 1;
			// fall through
			case AV_CODEC_ID_PCM_S16BE:
			case AV_CODEC_ID_PCM_U16BE:
				width = depth = 16;
				break;
			case AV_CODEC_ID_PCM_S24LE:
			case AV_CODEC_ID_PCM_U24LE:
				LE = 1;
			// fall through
			case AV_CODEC_ID_PCM_S24BE:
			case AV_CODEC_ID_PCM_U24BE:
				width = depth = 24;
				break;
			case AV_CODEC_ID_PCM_S32LE:
			case AV_CODEC_ID_PCM_U32LE:
				LE = 1;
			// fall through
			case AV_CODEC_ID_PCM_S32BE:
			case AV_CODEC_ID_PCM_U32BE:
				width = depth = 32;
				break;
			default:
				break;
		}

		uint8_t *data = codec_data;
		uint16_t format = LE ? 0x0001 : 0x0100;

		byterate = channels * rate * width / 8;
		block_align = channels * width / 8;
		memset(data, 0, sizeof(codec_data));
		/* format tag */
		*(data++) = format & 0xff;
		*(data++) = (format >> 8) & 0xff;
		/* channels */
		*(data++) = channels & 0xff;
		*(data++) = (channels >> 8) & 0xff;
		/* sample rate */
		*(data++) = rate & 0xff;
		*(data++) = (rate >> 8) & 0xff;
		*(data++) = (rate >> 16) & 0xff;
		*(data++) = (rate >> 24) & 0xff;
		/* byte rate */
		*(data++) = byterate & 0xff;
		*(data++) = (byterate >> 8) & 0xff;
		*(data++) = (byterate >> 16) & 0xff;
		*(data++) = (byterate >> 24) & 0xff;
		/* block align */
		*(data++) = block_align & 0xff;
		*(data++) = (block_align >> 8) & 0xff;
		/* word size */
		*(data++) = depth & 0xff;
		*(data++) = (depth >> 8) & 0xff;

		uint32_t nfixed_buffersize = rate * 30 / 1000;
		nfixed_buffersize *= channels * depth / 8;
		fixed_buffertimestamp = call->Pts;
		fixed_bufferduration = 90000 * nfixed_buffersize /  byterate;

		if (fixed_buffersize != nfixed_buffersize || NULL == fixed_buffer)
		{
			fixed_buffersize = nfixed_buffersize;
			if (NULL != fixed_buffer)
			{
				free(fixed_buffer);
			}
			fixed_buffer = malloc(fixed_buffersize);
		}
		fixed_bufferfilled = 0;
		/* avoid compiler warning */
		if (LE) {}
		pcm_printf(40, "PCM fixed_buffersize [%u] [%s]\n", fixed_buffersize, LE ? "LE" : "BE");
	}
	
	while (size > 0)
	{
		uint32_t cpSize = (fixed_buffersize - fixed_bufferfilled);
		if (cpSize > size)
		{
			memcpy(fixed_buffer + fixed_bufferfilled, buffer, size);
			fixed_bufferfilled += size;
			return size;
		}

		memcpy(fixed_buffer + fixed_bufferfilled, buffer, cpSize);
		fixed_bufferfilled = 0;
		buffer += cpSize;
		size -= cpSize;

		uint32_t addHeaderSize = 0;
		
		//if (STB_DREAMBOX == GetSTBType())
		//{
		//	addHeaderSize = 4;
		//}
		
		uint32_t headerSize = InsertPesHeader(PesHeader, fixed_buffersize + 4 + addHeaderSize + sizeof(codec_data), MPEG_AUDIO_PES_START_CODE, fixed_buffertimestamp, 0);
		
		// dreamboxes
		//if (STB_DREAMBOX == GetSTBType())
		//{
		//	PesHeader[headerSize++] = 0x42; // B
		//	PesHeader[headerSize++] = 0x43; // C
		//	PesHeader[headerSize++] = 0x4D; // M
		//	PesHeader[headerSize++] = 0x41; // A
		//}

		PesHeader[headerSize++] = (fixed_buffersize >> 24) & 0xff;
		PesHeader[headerSize++] = (fixed_buffersize >> 16) & 0xff;
		PesHeader[headerSize++] = (fixed_buffersize >> 8)  & 0xff;
		PesHeader[headerSize++] = fixed_buffersize & 0xff;

		memcpy(PesHeader + headerSize, codec_data, sizeof(codec_data));
		headerSize += sizeof(codec_data);

		PesHeader[6] |= 1;

		struct iovec iov[2];
		iov[0].iov_base = PesHeader;
		iov[0].iov_len  = headerSize;
		iov[1].iov_base = fixed_buffer;
		iov[1].iov_len  = fixed_buffersize;
		
		call->WriteV(call->fd, iov, 2);
		
		fixed_buffertimestamp += fixed_bufferduration;
	}
#endif

	return size;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */
// pcm
static WriterCaps_t caps_pcm = {
	"pcm",
	eAudio,
	"A_PCM",
	AUDIO_STREAMTYPE_LPCMDVD
};

struct Writer_s WriterAudioPCM = {
	&reset,
	&writeData,
	NULL,
	&caps_pcm
};

// ipcm
static WriterCaps_t caps_ipcm = {
	"ipcm",
	eAudio,
	"A_IPCM",
	AUDIO_STREAMTYPE_LPCMDVD
};

struct Writer_s WriterAudioIPCM = {
	&reset,
	&writeData,
	NULL,
	&caps_ipcm
};

