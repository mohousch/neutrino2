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
#include "misc.h"
#include "pes.h"
#include "pcm.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define WMA_DEBUG

#ifdef WMA_DEBUG

static short debug_level = 10;

#define wma_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define wma_printf(level, fmt, x...)
#endif

#ifndef WMA_SILENT
#define wma_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define wma_err(fmt, x...)
#endif

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static int initialHeader = 1;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

static int reset()
{
	initialHeader = 1;
	
	return 0;
}

static int writeData(void* _call)
{
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

	int len = 0;

	wma_printf(10, "\n");

	if (call == NULL)
	{
		wma_err("call data is NULL...\n");
		return 0;
	}

	wma_printf(10, "AudioPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		wma_err("parsing NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		wma_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

#ifdef __sh__
	if (initialHeader) 
	{

		unsigned char  PesHeader[PES_MAX_HEADER_SIZE];
		int HeaderLength;

		if ((call->private_size <= 0) || (call->private_data == NULL))
		{
			wma_err("private NULL.\n");
			return -1;
		}

		HeaderLength = InsertPesHeader (PesHeader, call->private_size, MPEG_AUDIO_PES_START_CODE, 0, 0);

		unsigned char* PacketStart = malloc(call->private_size + HeaderLength);
		memcpy (PacketStart, PesHeader, HeaderLength);
		memcpy (PacketStart + HeaderLength, call->private_data, call->private_size);

		len = write(call->fd, PacketStart, call->private_size + HeaderLength);

		free(PacketStart);

		initialHeader = 0;
	}

	if (call->len > 0 && call->data)
	{
		unsigned char  PesHeader[PES_MAX_HEADER_SIZE];

		int HeaderLength = InsertPesHeader (PesHeader, call->len, MPEG_AUDIO_PES_START_CODE, call->Pts, 0);

		unsigned char* PacketStart = malloc(call->len + HeaderLength);
		memcpy (PacketStart, PesHeader, HeaderLength);
		memcpy (PacketStart + HeaderLength, call->data, call->len);

		len = write(call->fd, PacketStart, call->len + HeaderLength);

		free(PacketStart);
	}

	wma_printf(10, "wma < %d\n", len);

	return len;
#else
	if (call == NULL || call->data == NULL || call->len <= 0 || call->fd < 0 || \
        !call->private_data || call->private_size != sizeof(pcmPrivateData_t)) 
        {
        	wma_err("Wrong input call: %p, data: %p, len: %d, fd: %d\n", call, call->data, call->len, call->fd);
        	return 0;
    	}

    	wma_printf(10, "AudioPts %lld\n", call->Pts);
    	uint8_t PesHeader[PES_MAX_HEADER_SIZE + 22];
    	uint32_t i;
    	uint32_t private_size = 0;
    	const uint8_t *vorbis_header_start[3];
    	int vorbis_header_len[3];
    	uint8_t vorbis_header_len_raw[3][2];
    	pcmPrivateData_t *pcmPrivateData  = (pcmPrivateData_t*)call->private_data;

    	uint32_t headerSize = InsertPesHeader(PesHeader, call->len, MPEG_AUDIO_PES_START_CODE, call->Pts, 0);
    
    	if (pcmPrivateData->avCodecId == AV_CODEC_ID_VORBIS) 
    	{
        	if (avpriv_split_xiph_headers(pcmPrivateData->private_data, pcmPrivateData->private_size, 30, vorbis_header_start, vorbis_header_len) < 0) 
        	{
            		wma_err("Wrong VORBIS codec data : %p, len: %d\n", pcmPrivateData->private_data, pcmPrivateData->private_size);
            		return -1;
        	}

        	for (i=0; i<3; ++i) 
        	{
            		vorbis_header_len_raw[i][0] = (vorbis_header_len[i] >> 8) & 0xff;
            		vorbis_header_len_raw[i][1] = vorbis_header_len[i] & 0xff;
            		private_size += 2 + vorbis_header_len[i];
        	}
    	}
    	else 
    	{
        	private_size = pcmPrivateData->private_size;
    	}

/*
    	if( STB_DREAMBOX == GetSTBType() ) 
    	{
        	PesHeader[headerSize++] = 'B';
        	PesHeader[headerSize++] = 'C';
        	PesHeader[headerSize++] = 'M';
        	PesHeader[headerSize++] = 'A';
    	}
*/

    	if (pcmPrivateData->avCodecId != AV_CODEC_ID_VORBIS || pcmPrivateData->avCodecId != AV_CODEC_ID_OPUS /*|| STB_HISILICON != GetSTBType()*/) 
    	{
        	uint32_t payloadSize = call->len;
        	PesHeader[headerSize++] = (payloadSize >> 24) & 0xFF;
        	PesHeader[headerSize++] = (payloadSize >> 16) & 0xFF;
        	PesHeader[headerSize++] = (payloadSize >> 8) & 0xFF;
        	PesHeader[headerSize++] = payloadSize & 0xFF;

        	int32_t channels        = pcmPrivateData->uNoOfChannels;
        	uint32_t sample_rate    = pcmPrivateData->uSampleRate;

        	int32_t bits_per_sample = pcmPrivateData->uBitsPerSample;
        	uint32_t byte_rate      = pcmPrivateData->uBitsPerSample / 8;
        	uint32_t block_align    = pcmPrivateData->block_align;

	        int32_t format_tag;
		switch(pcmPrivateData->avCodecId)
		{
			case AV_CODEC_ID_WMAV1:
			    format_tag = 0x160;
			    break;
			case AV_CODEC_ID_WMAV2:
			    format_tag = 0x161;
			    break;
			case AV_CODEC_ID_WMAPRO:
			    format_tag = 0x162;
			    break;
			case AV_CODEC_ID_WMALOSSLESS:
			    format_tag = 0x163;
			    break;
			case AV_CODEC_ID_VORBIS:
			    bits_per_sample = 8;
			    byte_rate = 32000;
			    block_align = 1;
			default:
			    format_tag = 0xFFFF;
			    break;
		}

		/* format tag */
		PesHeader[headerSize++] = format_tag & 0xff;
		PesHeader[headerSize++] = (format_tag >> 8) & 0xff;

		/* channels */
		PesHeader[headerSize++] = channels & 0xff;
		PesHeader[headerSize++] = (channels >> 8) & 0xff;

		/* sample rate */
		PesHeader[headerSize++] = sample_rate & 0xff;
		PesHeader[headerSize++] = (sample_rate >> 8) & 0xff;
		PesHeader[headerSize++] = (sample_rate >> 16) & 0xff;
		PesHeader[headerSize++] = (sample_rate >> 24) & 0xff;

		/* byte rate */
		PesHeader[headerSize++] = byte_rate & 0xff;
		PesHeader[headerSize++] = (byte_rate >> 8) & 0xff;
		PesHeader[headerSize++] = (byte_rate >> 16) & 0xff;
		PesHeader[headerSize++] = (byte_rate >> 24) & 0xff;

		/* block align */
		PesHeader[headerSize++] = block_align & 0xff;
		PesHeader[headerSize++] = (block_align >> 8) & 0xff;

		/* bits per sample */
		PesHeader[headerSize++] = bits_per_sample & 0xff;
		PesHeader[headerSize++] = (bits_per_sample >> 8) & 0xff;

		/* Codec Specific Data Size */
		PesHeader[headerSize++] = private_size & 0xff;
		PesHeader[headerSize++] = (private_size >> 8) & 0xff;
    	}

    	PesHeader[6] |= 1;
    	UpdatePesHeaderPayloadSize(PesHeader, headerSize - 6 + private_size + call->len);

    	struct iovec iov[5];

    	i = 0;
    	iov[i].iov_base = PesHeader;
    	iov[i++].iov_len  = headerSize;

    	if (private_size > 0) 
    	{
        	if (pcmPrivateData->avCodecId == AV_CODEC_ID_VORBIS) 
        	{
            		for (i=0; i<3; ++i) 
            		{
                		iov[i].iov_base = vorbis_header_len_raw[i];
                		iov[i++].iov_len  = 2;

                		iov[i].iov_base = vorbis_header_start;
                		iov[i++].iov_len  = vorbis_header_len[i];
            		}
        	}
        	else 
        	{
            		iov[i].iov_base = pcmPrivateData->private_data;
            		iov[i++].iov_len  = private_size;
        	}
    	}

    	iov[i].iov_base = call->data;
    	iov[i++].iov_len  = call->len;

    	return call->WriteV(call->fd, iov, i);
#endif
}

/* ***************************** */
/* Writer Definition            */
/* ***************************** */
// wma
static WriterCaps_t caps = {
	"wma",
	eAudio,
	"A_WMA",
	AUDIO_STREAMTYPE_WMA
};

struct Writer_s WriterAudioWMA = {
	&reset,
	&writeData,
	NULL,
	&caps
};

static WriterCaps_t capsWMAPRO = {
    "wma/pro",
    eAudio,
    "A_WMA/PRO",
    -1,
    AUDIO_STREAMTYPE_WMA_PRO,
    -1
};

struct Writer_s WriterAudioWMAPRO = {
    &reset,
    &writeData,
    NULL,
    &capsWMAPRO
};

static WriterCaps_t capsVORBIS = {
    "vorbis",
    eAudio,
    "A_VORBIS",
    -1,
    AUDIO_STREAMTYPE_VORBIS,
    -1
};

struct Writer_s WriterAudioVORBIS = {
    &reset,
    &writeData,
    NULL,
    &capsVORBIS
};

static WriterCaps_t capsOPUS = {
    "opus",
    eAudio,
    "A_OPUS",
    -1,
    AUDIO_STREAMTYPE_OPUS,
    -1
};

struct Writer_s WriterAudioOPUS = {
    &reset,
    &writeData,
    NULL,
    &capsOPUS
};


