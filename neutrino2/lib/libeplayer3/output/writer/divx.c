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
#define B_GET_BITS(w,e,b)  (((w)>>(b))&(((unsigned)(-1))>>((sizeof(unsigned))*8-(e+1-b))))
#define B_SET_BITS(name,v,e,b)  (((unsigned)(v))<<(b))

//#define DIVX_DEBUG

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

static uint8_t brcm_divx311_sequence_header[] =
{
	0x00, 0x00, 0x01, 0xE0, 0x00, 0x34, 0x80, 0x80, // PES HEADER
	0x05, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x20, /* 0 .. 7 */
	0x08, 0xC8, 0x0D, 0x40, 0x00, 0x53, 0x88, 0x40, /* 8 .. 15 */
	0x0C, 0x40, 0x01, 0x90, 0x00, 0x97, 0x53, 0x0A, /* 16 .. 24 */
	0x00, 0x00, 0x00, 0x00,
	0x30, 0x7F, 0x00, 0x00, 0x01, 0xB2, 0x44, 0x69, /* 0 .. 7 */
	0x76, 0x58, 0x33, 0x31, 0x31, 0x41, 0x4E, 0x44  /* 8 .. 15 */
};

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

#if defined __sh__
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

#if defined __sh__
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

/*
	FakeHeaderLength    = (ld.Ptr - (FakeHeaders));

	if (initialHeader) ExtraLength = call->private_size;

	HeaderLength        = InsertPesHeader (PesHeader, call->len, MPEG_VIDEO_PES_START_CODE, call->Pts, FakeStartCode);
	unsigned char* PacketStart = malloc(call->len + HeaderLength + FakeHeaderLength + ExtraLength);
	memcpy (PacketStart, PesHeader, HeaderLength);
	memcpy (PacketStart + HeaderLength, FakeHeaders, FakeHeaderLength);
	
	if (initialHeader) 
	{
		memcpy (PacketStart + HeaderLength + FakeHeaderLength, call->private_data, call->private_size);
		initialHeader = 0;
	}
	memcpy (PacketStart + HeaderLength + FakeHeaderLength + ExtraLength, call->data, call->len);

	int len = write(call->fd, PacketStart ,call->len + HeaderLength + FakeHeaderLength + ExtraLength);

	free(PacketStart);

	divx_printf(10, "xvid_Write < len=%d\n", len);

	return len;
*/
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

	divx_printf(10, "xvid_Write < len=%d\n", len);

	return len;
#else
	struct iovec iov[8];
	int ic = 0;

	if (initialHeader)
	{
		initialHeader = 0;
		uint8_t *data = brcm_divx311_sequence_header;
		int32_t height = call->Height;
		int32_t width  = call->Width;
		data += 38;
		data[0] = B_GET_BITS(width, 11, 4);
		data[1] = B_SET_BITS("width [3..0]", B_GET_BITS(width, 3, 0), 7, 4) |
		          B_SET_BITS("'10'", 0x02, 3, 2) |
		          B_SET_BITS("height [11..10]", B_GET_BITS(height, 11, 10), 1, 0);
		data[2] = B_GET_BITS(height, 9, 2);
		data[3] = B_SET_BITS("height [1.0]", B_GET_BITS(height, 1, 0), 7, 6) |
		          B_SET_BITS("'100000'", 0x20, 5, 0);

		iov[ic].iov_base = brcm_divx311_sequence_header;
		iov[ic++].iov_len = sizeof(brcm_divx311_sequence_header);
	}

	iov[ic].iov_base = PesHeader;
	uint32_t headerSize = 0;

	if (memcmp(call->data, "\x00\x00\x01\xb6", 4))
	{
		headerSize = InsertPesHeader(PesHeader, call->len + 4, MPEG_VIDEO_PES_START_CODE, call->Pts, 0);
		memcpy(PesHeader + headerSize, "\x00\x00\x01\xb6", 4);
		headerSize += 4;
	}
	else
	{
		headerSize = InsertPesHeader(PesHeader, call->len, MPEG_VIDEO_PES_START_CODE, call->Pts, 0);
	}
	iov[ic++].iov_len = headerSize;

	iov[ic].iov_base = call->data;
	iov[ic++].iov_len = call->len;

	int len = writev(call->fd, iov, ic);

	divx_printf(10, "xvid_Write < len=%d\n", len);

	return len;
#endif
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

static WriterCaps_t mpeg4p2_caps = {
	"mscomp",
	eVideo,
	"V_MSCOMP",
	VIDEO_STREAMTYPE_MPEG4_Part2
};

struct Writer_s WriterVideoMSCOMP = {
	&reset,
	&writeData,
	NULL,
	&mpeg4p2_caps,
};

static WriterCaps_t fourcc_caps = {
    "fourcc",
    eVideo,
    "V_MS/VFW/FOURCC",
    VIDEO_STREAMTYPE_MPEG4_Part2    
};

struct Writer_s WriterVideoFOURCC = {
    &reset,
    &writeData,
    NULL,
    &fourcc_caps,
};

static WriterCaps_t divx_caps = {
    "divx",
    eVideo,
    "V_MKV/XVID",
    VIDEO_STREAMTYPE_MPEG4_Part2   
};

struct Writer_s WriterVideoDIVX = {
    &reset,
    &writeData,
    NULL,
    &divx_caps,
};


