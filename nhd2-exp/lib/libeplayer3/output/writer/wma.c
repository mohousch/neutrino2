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
}

/* ***************************** */
/* Writer Definition            */
/* ***************************** */

static WriterCaps_t caps = {
	"wma",
	eAudio,
	"A_WMA",
	AUDIO_STREAMTYPE_MPEG
};

struct Writer_s WriterAudioWMA = {
	&reset,
	&writeData,
	NULL,
	&caps
};
