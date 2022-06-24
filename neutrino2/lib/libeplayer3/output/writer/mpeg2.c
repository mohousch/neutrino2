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

//#define MPEG2_DEBUG

#ifdef MPEG2_DEBUG

static short debug_level = 10;

#define mpeg2_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define mpeg2_printf(level, fmt, x...)
#endif

#ifndef MPEG2_SILENT
#define mpeg2_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define mpeg2_err(fmt, x...)
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

static bool must_send_header = true;
static uint8_t *private_data = NULL;
static uint32_t private_size = 0;

static int reset()
{
	return 0;
}

static int writeData(void* _call)
{
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

	unsigned char PesHeader[PES_MAX_HEADER_SIZE];
	int len = 0;
	int Position = 0;

	mpeg2_printf(10, "\n");

	if (call == NULL)
	{
		mpeg2_err("call data is NULL...\n");
		return 0;
	}

	mpeg2_printf(10, "VideoPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		mpeg2_err("parsing NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		mpeg2_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

#if defined __sh__
	while (Position < call->len)
	{
		int PacketLength = (call->len - Position) <= MAX_PES_PACKET_SIZE ? (call->len - Position) : MAX_PES_PACKET_SIZE;
		int Remaining = call->len - Position - PacketLength;

		mpeg2_printf(20, "PacketLength=%d, Remaining=%d, Position=%d\n", PacketLength, Remaining, Position);
		struct iovec iov[2];

		iov[0].iov_base = PesHeader;
		iov[0].iov_len = InsertPesHeader(PesHeader, PacketLength, 0xe0, call->Pts, 0);
		iov[1].iov_base = call->data + Position;
		iov[1].iov_len = PacketLength;

		ssize_t l = writev(call->fd, iov, 2);

		if (l < 0)
		{
			len = l;
			break;
		}
		len += l;
		Position += PacketLength;
		call->Pts = INVALID_PTS_VALUE;
	}

	mpeg2_printf(10, "< len %d\n", len);
	return len;
#else
	uint8_t *data = call->data;
	uint32_t data_len = call->len;

	if (!private_data && !call->private_data && data_len > 3 && !memcmp(data, "\x00\x00\x01\xb3", 4))
	{
		bool ok = true;
		uint32_t pos = 4;
		uint32_t sheader_data_len = 0;
		while (pos < data_len && ok)
		{
			if (pos >= data_len) break;
			pos += 7;
			if (pos >= data_len) break;
			sheader_data_len = 12;
			if (data[pos] & 2)
			{
				// intra matrix
				pos += 64;
				if (pos >= data_len) break;
				sheader_data_len += 64;
			}
			if (data[pos] & 1)
			{
				// non intra matrix
				pos += 64;
				if (pos >= data_len) break;
				sheader_data_len += 64;
			}
			pos += 1;
			if (pos + 3 >= data_len) break;
			if (!memcmp(&data[pos], "\x00\x00\x01\xb5", 4))
			{
				// extended start code
				pos += 3;
				sheader_data_len += 3;
				do
				{
					pos += 1;
					++sheader_data_len;
					if (pos + 2 > data_len)
					{
						ok = false;
						break;
					}
				}
				while (memcmp(&data[pos], "\x00\x00\x01", 3));
				if (!ok) break;
			}
			if (pos + 3 >= data_len) break;
			if (!memcmp(&data[pos], "\x00\x00\x01\xb2", 4))
			{
				// private data
				pos += 3;
				sheader_data_len += 3;
				do
				{
					pos += 1;
					++sheader_data_len;
					if (pos + 2 > data_len)
					{
						ok = false;
						break;
					}
				}
				while (memcmp(&data[pos], "\x00\x00\x01", 3));
				if (!ok) break;
			}

			free(private_data);
			private_data = malloc(sheader_data_len);
			if (private_data)
			{
				private_size = sheader_data_len;
				memcpy(private_data, data + pos - sheader_data_len, sheader_data_len);
			}
			must_send_header = false;
			break;
		}
	}
	else if ((private_data || call->private_data) && must_send_header)
	{
		uint8_t *codec_data = NULL;
		uint32_t codec_data_size = 0;
		int pos = 0;

		if (private_data)
		{
			codec_data = private_data;
			codec_data_size = private_size;
		}
		else
		{
			codec_data = call->private_data;
			codec_data_size = call->private_size;
		}

		while ((unsigned)pos <= data_len - 4)
		{
			if (memcmp(&data[pos], "\x00\x00\x01\xb8", 4)) /* find group start code */
			{
				pos++;
				continue;
			}

			struct iovec iov[4];
			iov[0].iov_base = PesHeader;
			iov[0].iov_len = InsertPesHeader(PesHeader, call->len + codec_data_size, MPEG_VIDEO_PES_START_CODE, call->Pts, 0);

			iov[1].iov_base = data;
			iov[1].iov_len = pos;

			iov[2].iov_base = codec_data;
			iov[2].iov_len = codec_data_size;

			iov[3].iov_base = data + pos;
			iov[3].iov_len = data_len - pos;

			must_send_header = false;
			return call->WriteV(call->fd, iov, 4);
		}
	}

	struct iovec iov[2];

	iov[0].iov_base = PesHeader;
	iov[0].iov_len = InsertPesHeader(PesHeader, call->len, MPEG_VIDEO_PES_START_CODE, call->Pts, 0);

	iov[1].iov_base = data;
	iov[1].iov_len = data_len;

	PesHeader[6] = 0x81;

	UpdatePesHeaderPayloadSize(PesHeader, data_len + iov[0].iov_len - 6);

	if (iov[0].iov_len != (unsigned)WriteExt(call->WriteV, call->fd, iov[0].iov_base, iov[0].iov_len)) 			return -1;
	if (iov[1].iov_len != (unsigned)WriteExt(call->WriteV, call->fd, iov[1].iov_base, iov[1].iov_len)) 			return -1;
#endif
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */
static WriterCaps_t caps = {
	"mpeg2",
	eVideo,
	"V_MPEG2",
	VIDEO_STREAMTYPE_MPEG2,
};

struct Writer_s WriterVideoMPEG2 = {
	&reset,
	&writeData,
	NULL,
	&caps
};

static WriterCaps_t h264_caps = {
	"mpges_h264",
	eVideo,
	"V_MPEG2/H264",
	VIDEO_STREAMTYPE_MPEG4_H264
};

struct Writer_s WriterVideoMPEGH264 = {
	&reset,
	&writeData,
	NULL,
	&h264_caps
};
