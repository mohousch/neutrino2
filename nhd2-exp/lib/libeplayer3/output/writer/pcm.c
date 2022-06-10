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
		case 48000:             SubFrameLen = 40;
				                break;
		case 96000:             lpcm_prv[8] |= 0x10;
				                SubFrameLen = 80;
				                break;
		case 192000:    lpcm_prv[8] |= 0x20;
				                SubFrameLen = 160;
				                break;
		case 44100:             lpcm_prv[8] |= 0x80;
				                SubFrameLen = 40;
				                break;
		case 88200:             lpcm_prv[8] |= 0x90;
				                SubFrameLen = 80;
				                break;
		case 176400:    lpcm_prv[8] |= 0xA0;
				                SubFrameLen = 160;
				                break;
		default:                break;
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
		case    16: break;
		case    24: lpcm_prv[7] |= 0x20;
				        break;
		default:        printf("inappropriate bits per sample (%d) - must be 16 or 24\n",uBitsPerSample);
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

	unsigned char  PesHeader[PES_MAX_HEADER_SIZE];

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

	pcmPrivateData_t*         pcmPrivateData          = (pcmPrivateData_t*)call->private_data;

	if (initialHeader) 
	{
		initialHeader = 0;
		prepareClipPlay(pcmPrivateData->uNoOfChannels, pcmPrivateData->uSampleRate, pcmPrivateData->uBitsPerSample, pcmPrivateData->bLittleEndian);
	}

	unsigned char * buffer = call->data;
	int size = call->len;
	//pcm_printf(10, "PCM %d size SubFrameLen=%d\n", size, SubFrameLen);

	unsigned int qty;
	unsigned int n;
	unsigned int injectBufferSize = sizeof(lpcm_pes) + sizeof(lpcm_prv) + SubFrameLen;
	unsigned char * injectBuffer = (unsigned char *)malloc(sizeof(unsigned char)*injectBufferSize);
	unsigned char * injectBufferDataPointer = &injectBuffer[sizeof(lpcm_pes)+sizeof(lpcm_prv)];
	int pos;

	for(pos = 0; pos < size; )
	{
		//printf("PCM %s - Position=%d\n", __FUNCTION__, pos);
		if((size - pos) < SubFrameLen)
		{
			breakBufferFillSize = size - pos;
			memcpy(breakBuffer, &buffer[pos], sizeof(unsigned char) * breakBufferFillSize);
			//printf("PCM %s - Unplayed=%d\n", __FUNCTION__, breakBufferFillSize);
			break;
		}

                //get first PES's worth
		if(breakBufferFillSize > 0)
		{
			memcpy(injectBufferDataPointer, breakBuffer, sizeof(unsigned char)*breakBufferFillSize);
			memcpy(&injectBufferDataPointer[breakBufferFillSize], &buffer[pos], sizeof(unsigned char)*(SubFrameLen - breakBufferFillSize));
			pos += (SubFrameLen - breakBufferFillSize);
			breakBufferFillSize = 0;
		} 
		else
		{
		        memcpy(injectBufferDataPointer, &buffer[pos], sizeof(unsigned char)*SubFrameLen);
			pos += SubFrameLen;
		}

		//write the PES header
		memcpy(injectBuffer, lpcm_pes, sizeof(lpcm_pes));

		//write the private data area
		memcpy(&injectBuffer[sizeof(lpcm_pes)], lpcm_prv, sizeof(lpcm_prv));

		//write the PCM data
		if(pcmPrivateData->uBitsPerSample == 16) 
		{
			for(n = 0; n < SubFrameLen; n += 2) 
			{
				unsigned char tmp;
				tmp = injectBufferDataPointer[n];
				injectBufferDataPointer[n] = injectBufferDataPointer[n + 1];
				injectBufferDataPointer[n + 1] = tmp;
			}
		} 
		else 
		{
			//A1cA1bA1a-B1cB1bB1a-A2cA2bA2a-B2cB2bB2a to A1aA1bB1aB1b.A2aA2bB2aB2b-A1cB1cA2cB2c
			for(n = 0; n < SubFrameLen; n += 12) 
			{
				unsigned char tmp[12];
				tmp[ 0]=injectBufferDataPointer[n+2];
				tmp[ 1]=injectBufferDataPointer[n+1];
				tmp[ 8]=injectBufferDataPointer[n+0];
				tmp[ 2]=injectBufferDataPointer[n+5];
				tmp[ 3]=injectBufferDataPointer[n+4];
				tmp[ 9]=injectBufferDataPointer[n+3];
				tmp[ 4]=injectBufferDataPointer[n+8];
				tmp[ 5]=injectBufferDataPointer[n+7];
				tmp[10]=injectBufferDataPointer[n+6];
				tmp[ 7]=injectBufferDataPointer[n+11];
				tmp[ 8]=injectBufferDataPointer[n+10];
				tmp[11]=injectBufferDataPointer[n+9];
				memcpy(&injectBufferDataPointer[n],tmp,12);
			}
		}

		//increment err... subframe count?
		lpcm_prv[1] = ((lpcm_prv[1]+SubFramesPerPES) & 0x1F);

	        //disable PES to save calculating correct values
	        lpcm_pes[7] = 0x01;

	        //kill off first A_PKT only fields in PES header
	        lpcm_pes[14] = 0xFF;
	        lpcm_pes[15] = 0xFF;
	        lpcm_pes[16] = 0xFF;

		write(call->fd, injectBuffer, injectBufferSize);

		//printf("PCM %d bytes injected\n", injectBufferSize);
		//Hexdump(injectBuffer, 126);
	}
	
	free(injectBuffer);

	return size;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

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

