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


//#define FLAC_DEBUG

#ifdef FLAC_DEBUG

static short debug_level = 10;

#define flac_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define flac_printf(level, fmt, x...)
#endif

#ifndef FLAC_SILENT
#define flac_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define flac_err(fmt, x...)
#endif

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

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
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

	unsigned char  PesHeader[PES_MAX_HEADER_SIZE];

	flac_printf(10, "\n");

	if (call == NULL)
	{
		flac_err("call data is NULL...\n");
		return 0;
	}

	flac_printf(10, "AudioPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		flac_err("parsing NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		flac_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

	int HeaderLength = InsertPesHeader (PesHeader, call->len , MPEG_AUDIO_PES_START_CODE, call->Pts, 0);

	unsigned char* PacketStart = malloc(call->len + HeaderLength);

	memcpy (PacketStart, PesHeader, HeaderLength);
	memcpy (PacketStart + HeaderLength, call->data, call->len);

	int len = write(call->fd, PacketStart, call->len + HeaderLength);

	free(PacketStart);

	flac_printf(10, "flac_Write-< len=%d\n", len);
	return len;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

static WriterCaps_t caps_flac = {
	"flac",
	eAudio,
	"A_FLAC",
	AUDIO_STREAMTYPE_LPCMDVD
};

struct Writer_s WriterAudioFLAC = {
	&reset,
	&writeData,
	NULL,
	&caps_flac,
};

