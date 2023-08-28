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

#define DIVX_DEBUG

#ifdef DIVX_DEBUG

static short debug_level = 10;

#define divx_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define divx_printf(level, fmt, x...)
#endif

#ifndef DIVX_SILENT
#define divx_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define divx_err(fmt, x...)
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

	unsigned char  PesHeader[PES_MAX_HEADER_SIZE];

#if defined (__sh__)
	unsigned char  FakeHeaders[64]; // 64bytes should be enough to make the fake headers
	unsigned int   FakeHeaderLength;
	unsigned int   ExtraLength = 0;
	unsigned char  Version             = 5;
	unsigned int   FakeStartCode       = (Version << 8) | PES_VERSION_FAKE_START_CODE;
	unsigned int   HeaderLength = 0;
	unsigned int   usecPerFrame = 41708; /* Hellmaster1024: default value */
	BitPacker_t ld = {FakeHeaders, 0, 32};
#endif

	divx_printf(10, "\n");

	if (call == NULL)
	{
		divx_err("call data is NULL...\n");
		return 0;
	}

	divx_printf(10, "AudioPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		divx_err("parsing NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		divx_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

#if defined (__sh__)
	usecPerFrame = 1000000000 / call->FrameRate;
	divx_printf(10, "Microsecends per frame = %d\n", usecPerFrame);

	memset(FakeHeaders, 0, sizeof(FakeHeaders));

	/* Create info record for frame parser */
	/* divx4 & 5
	  VOS
	  PutBits(&ld, 0x0, 8);
	  PutBits(&ld, 0x0, 8);
	*/
	PutBits(&ld, 0x1b0, 32);      // startcode
	PutBits(&ld, 0, 8);           // profile = reserved
	PutBits(&ld, 0x1b2, 32);      // startcode (user data)
	PutBits(&ld, 0x53545443, 32); // STTC - an embedded ST timecode from an avi file
	PutBits(&ld, usecPerFrame , 32);
	// microseconds per frame
	FlushBits(&ld);

	//
	struct iovec iov[4];

	int ic = 0;
	iov[ic].iov_base = PesHeader;
	iov[ic++].iov_len = InsertPesHeader(PesHeader, call->len, MPEG_VIDEO_PES_START_CODE, call->Pts, FakeStartCode);
	iov[ic].iov_base = FakeHeaders;
	iov[ic++].iov_len = FakeHeaderLength;

	if (initialHeader)
	{
		iov[ic].iov_base = call->private_data;
		iov[ic++].iov_len = call->private_size;
		initialHeader = 0;
	}

	iov[ic].iov_base = call->data;
	iov[ic++].iov_len = call->len;
	int len = writev(call->fd, iov, ic);
#else
	unsigned int PacketLength = call->len;
    	if (initialHeader && call->private_size && call->private_data != NULL)
    	{
        	PacketLength += call->private_size;
   	}

   	struct iovec iov[3];
    	int ic = 0;
    	iov[ic].iov_base = PesHeader;
    	iov[ic++].iov_len = InsertPesHeader (PesHeader, PacketLength, MPEG_VIDEO_PES_START_CODE, call->Pts, 0);

    	if (initialHeader && call->private_size && call->private_data != NULL)
    	{
        	initialHeader = 0;
        	iov[ic].iov_base = call->private_data;
        	iov[ic++].iov_len = call->private_size;
    	}
    	iov[ic].iov_base = call->data;
    	iov[ic++].iov_len = call->len;

    	int len = call->WriteV(call->fd, iov, ic);
#endif

	divx_printf(10, "xvid_Write < len=%d\n", len);

	return len;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */
// divx
static WriterCaps_t divx_caps = {
	"divx",
    	eVideo,
    	"V_MPEG4",
    	VIDEO_STREAMTYPE_MPEG4_Part2   
};

struct Writer_s WriterVideoDIVX = {
    	&reset,
    	&writeData,
    	NULL,
    	&divx_caps,
};

