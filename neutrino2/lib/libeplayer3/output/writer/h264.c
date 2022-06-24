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
#include <assert.h>

#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "pes.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */


//#define H264_DEBUG

#ifdef H264_DEBUG

static short debug_level = 10;

#define h264_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define h264_printf(level, fmt, x...)
#endif

#ifndef H264_SILENT
#define h264_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define h264_err(fmt, x...)
#endif

#define NALU_TYPE_PLAYER2_CONTAINER_PARAMETERS          24
#define CONTAINER_PARAMETERS_VERSION                    0x00

#define IOVEC_SIZE                              128

/* ***************************** */
/* Types                         */
/* ***************************** */
typedef struct avcC_s
{
	unsigned char       Version;                /* configurationVersion        */
	unsigned char       Profile;                /* AVCProfileIndication        */
	unsigned char       Compatibility;          /* profile_compatibility       */
	unsigned char       Level;                  /* AVCLevelIndication          */
	unsigned char       NalLengthMinusOne;      /* held in bottom two bits     */
	unsigned char       NumParamSets;           /* held in bottom 5 bits       */
	unsigned char       Params[1];              /* {length,params}{length,params}...sequence then picture*/
} avcC_t;


/* ***************************** */
/* Varaibles                     */
/* ***************************** */
const unsigned char Head[] = {0, 0, 0, 1};
static int initialHeader = 1;
static unsigned int NalLengthBytes = 1;

#if !defined __sh__
static unsigned char* CodecData = NULL;
static unsigned int CodecDataLen = 0;
static int avc3 = 0;
static int sps_pps_in_stream = 0;
#endif

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

#if !defined __sh__
static int32_t UpdateExtraData(uint8_t **ppExtraData, uint32_t *pExtraDataSize, uint8_t *pData, uint32_t dataSize)
{
	uint8_t *aExtraData = *ppExtraData;

	if (aExtraData[0] != 1 || !pData)
	{
		// Not AVCC or nothing to update with.
		return -1;
	}

	int32_t nalsize = (aExtraData[4] & 3) + 1;

	uint8_t sps[256];
	uint8_t spsIdx = 0;

	uint8_t numSps = 0;

	uint8_t pps[256];
	uint8_t ppsIdx = 0;

	uint8_t numPps = 0;

	if (nalsize != 4)
	{
		return -1;
	}

	// Find SPS and PPS NALUs in AVCC data
	uint8_t *d = pData;

	while (d + 4 < pData + dataSize)
	{
		uint32_t nalLen = ReadUint32(d);

		uint8_t nalType = d[4] & 0x1f;

		if (nalType == 7)
		{
			/* SPS */

			// 16 bits size
			sps[spsIdx++] = (uint8_t)(0xFF & (nalLen >> 8));
			sps[spsIdx++] = (uint8_t)(0xFF & nalLen);

			if (spsIdx + nalLen >= sizeof(sps))
			{
				h264_err("SPS no free space to copy...\n");
				return -1;
			}
			memcpy(&(sps[spsIdx]), d + 4, nalLen);
			spsIdx += nalLen;

			numSps += 1;

			h264_printf(10, "SPS len[%u]...\n", nalLen);
		}
		else if (nalType == 8)
		{
			/* PPS */

			// 16 bits size
			pps[ppsIdx++] = (uint8_t)(0xFF & (nalLen >> 8));
			pps[ppsIdx++] = (uint8_t)(0xFF & nalLen);

			if (ppsIdx + nalLen >= sizeof(sps))
			{
				h264_err("PPS not free space to copy...\n");
				return -1;
			}
			memcpy(&(pps[ppsIdx]), d + 4, nalLen);
			ppsIdx += nalLen;

			numPps += 1;

			h264_printf(10, "PPS len[%u]...\n", nalLen);
		}
		d += 4 + nalLen;
	}

	uint32_t idx = 0;
	*ppExtraData = malloc(7 + spsIdx + ppsIdx);
	aExtraData = *ppExtraData;
	aExtraData[idx++] = 0x1;            // version
	aExtraData[idx++] = sps[3];         // profile
	aExtraData[idx++] = sps[4];         // profile compat
	aExtraData[idx++] = sps[5];         // level
	aExtraData[idx++] = 0xff;           // nal size - 1

	aExtraData[idx++] = 0xe0 | numSps;

	if (numSps)
	{
		memcpy(&(aExtraData[idx]), sps, spsIdx);
		idx += spsIdx;
	}

	aExtraData[idx++] = numPps;
	if (numPps)
	{
		memcpy(&(aExtraData[idx]), pps, ppsIdx);
		idx += ppsIdx;
	}

	h264_printf(10, "aExtraData len[%u]...\n", idx);
	*pExtraDataSize = idx;

	return 0;
}

static int32_t PreparCodecData(unsigned char *data, unsigned int cd_len, unsigned int *NalLength)
{
	h264_printf(10, "H264 check codec data..!\n");
	int32_t ret = -100;

	if (data)
	{
		unsigned char tmp[2048];
		unsigned int tmp_len = 0;

		unsigned int cd_pos = 0;
		h264_printf(10, "H264 have codec data..!\n");

		if (cd_len > 7 && data[0] == 1)
		{
			unsigned short len = (data[6] << 8) | data[7];

			if (cd_len >= (uint32_t)(len + 8))
			{
				unsigned int i = 0;
				uint8_t profile_num[] = { 66, 77, 88, 100 };
				uint8_t profile_cmp[2] = { 0x67, 0x00 };
				const char *profile_str[] = { "baseline", "main", "extended", "high" };
				/* avoid compiler warning */
				if (*profile_str) {}
				memcpy(tmp, Head, sizeof(Head));
				tmp_len += 4;
				memcpy(tmp + tmp_len, data + 8, len);

				for (i = 0; i < 4; ++i)
				{
					profile_cmp[1] = profile_num[i];
					if (!memcmp(tmp + tmp_len, profile_cmp, 2))
					{
						uint8_t level_org = tmp[tmp_len + 3];
						if (level_org > 0x29)
						{
							h264_printf(10, "H264 %s profile@%d.%d patched down to 4.1!", profile_str[i], level_org / 10, level_org % 10);
							tmp[tmp_len + 3] = 0x29; // level 4.1
						}
						else
						{
							h264_printf(10, "H264 %s profile@%d.%d", profile_str[i], level_org / 10, level_org % 10);
						}
						break;
					}
				}
				tmp_len += len;
				cd_pos = 8 + len;

				if (cd_len > (cd_pos + 2))
				{
					len = (data[cd_pos + 1] << 8) | data[cd_pos + 2];
					cd_pos += 3;

					if (cd_len >= (cd_pos + len))
					{
						memcpy(tmp + tmp_len, "\x00\x00\x00\x01", 4);
						tmp_len += 4;
						memcpy(tmp + tmp_len, data + cd_pos, len);
						tmp_len += len;

						CodecData = malloc(tmp_len);
						memcpy(CodecData, tmp, tmp_len);
						CodecDataLen = tmp_len;

						*NalLength = (data[4] & 0x03) + 1;
						ret = 0;
					}
					else
					{
						h264_printf(10, "codec_data too short(4)");
						ret = -4;
					}
				}
				else
				{
					h264_printf(10,  "codec_data too short(3)");
					ret = -3;
				}
			}
			else
			{
				h264_printf(10, "codec_data too short(2)");
				ret = -2;
			}
		}
		else if (cd_len <= 7)
		{
			h264_printf(10, "codec_data too short(1)");
			ret = -1;
		}
		else
		{
			h264_printf(10, "wrong avcC version %d!", data[0]);
		}
	}
	else
	{
		*NalLength = 0;
	}

	return ret;
}
#endif

static int reset()
{
	initialHeader = 1;
	
#if !defined __sh__
	avc3 = 0;
	sps_pps_in_stream = 0;
#endif
	return 0;
}

static int writeData(void* _call)
{
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

	unsigned char*          PacketStart = NULL;
	unsigned int            PacketStartSIZE = 0;
	unsigned int            HeaderLength;
	unsigned char           PesHeader[PES_MAX_HEADER_SIZE];
	unsigned long long int  VideoPts;
	unsigned int            TimeDelta;
	unsigned int            TimeScale;
	int                     len = 0;

	int ic = 0;
	struct iovec iov[128];

	//static int              NoOtherBeginningFound = 1;
	h264_printf(10, "\n");

	if (call == NULL)
	{
		h264_err("call data is NULL...\n");
		return 0;
	}

	TimeDelta = call->FrameRate;
	TimeScale = call->FrameScale;
	/* avoid compiler warnings */
	if (TimeDelta) {}
	if (TimeScale) {}
	VideoPts  = call->Pts;

	h264_printf(10, "VideoPts %lld - %d %d\n", call->Pts, TimeDelta, TimeScale);

	if ((call->data == NULL) || (call->len <= 0))
	{
		h264_err("NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		h264_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

#if 1 //defined __sh__
	if ((call->len > 3) && ((call->data[0] == 0x00 && call->data[1] == 0x00 && call->data[2] == 0x00 && call->data[3] == 0x01) || (call->data[0] == 0xff && call->data[1] == 0xff && call->data[2] == 0xff && call->data[3] == 0xff)))
	{
		unsigned int PacketLength = 0;
		unsigned int FakeStartCode = (call->Version << 8) | PES_VERSION_FAKE_START_CODE;
		iov[ic++].iov_base = PesHeader;
		if (initialHeader)
		{
			initialHeader = 0;
			iov[ic].iov_base = call->private_data;
			iov[ic++].iov_len = call->private_size;
			PacketLength += call->private_size;
		}
		iov[ic].iov_base = call->data;
		iov[ic++].iov_len = call->len;
		PacketLength += call->len;
		/*Hellmaster1024: some packets will only be accepted by the player if we send one byte more than
		                  data is available. The content of this byte does not matter. It will be ignored
		                  by the player */
		iov[ic].iov_base = "";
		iov[ic++].iov_len = 1;
		iov[0].iov_len = InsertPesHeader(PesHeader, PacketLength, MPEG_VIDEO_PES_START_CODE, call->Pts, FakeStartCode);
		return call->WriteV(call->fd, iov, ic);
	}

	if (initialHeader)
	{
		avcC_t         *avcCHeader          = (avcC_t *)call->private_data;
		unsigned int    i;
		unsigned int    ParamSets;
		unsigned int    ParamOffset;
		unsigned int    InitialHeaderLength     = 0;
		unsigned int    ParametersLength;

		if (avcCHeader == NULL)
		{
			h264_err("private_data NULL\n");
			return -1;
		}

		if (avcCHeader->Version != 1)
			h264_err("Error unknown avcC version (%x). Expect problems.\n", avcCHeader->Version);
		ParametersLength                      = 0;
		unsigned char  HeaderData[19];
		HeaderData[ParametersLength++]        = 0x00;                                         // Start code
		HeaderData[ParametersLength++]        = 0x00;
		HeaderData[ParametersLength++]        = 0x01;
		HeaderData[ParametersLength++]        = NALU_TYPE_PLAYER2_CONTAINER_PARAMETERS;
		// Container message version - changes when/if we vary the format of the message
		HeaderData[ParametersLength++]        = CONTAINER_PARAMETERS_VERSION;
		HeaderData[ParametersLength++]        = 0xff;                                         // Field separator
		if (TimeDelta == 0xffffffff)
			TimeDelta       = (TimeScale > 1000) ? 1001 : 1;
		HeaderData[ParametersLength++]        = (TimeScale >> 24) & 0xff;         // Output the timescale
		HeaderData[ParametersLength++]        = (TimeScale >> 16) & 0xff;
		HeaderData[ParametersLength++]        = 0xff;
		HeaderData[ParametersLength++]        = (TimeScale >> 8) & 0xff;
		HeaderData[ParametersLength++]        = TimeScale & 0xff;
		HeaderData[ParametersLength++]        = 0xff;
		HeaderData[ParametersLength++]        = (TimeDelta >> 24) & 0xff;         // Output frame period
		HeaderData[ParametersLength++]        = (TimeDelta >> 16) & 0xff;
		HeaderData[ParametersLength++]        = 0xff;
		HeaderData[ParametersLength++]        = (TimeDelta >> 8) & 0xff;
		HeaderData[ParametersLength++]        = TimeDelta & 0xff;
		HeaderData[ParametersLength++]        = 0xff;
		HeaderData[ParametersLength++]        = 0x80;                                         // Rsbp trailing bits
		assert(ParametersLength <= sizeof(HeaderData));
		ic = 0;
		iov[ic].iov_base = PesHeader;
		iov[ic++].iov_len = InsertPesHeader(PesHeader, ParametersLength, MPEG_VIDEO_PES_START_CODE, INVALID_PTS_VALUE, 0);
		iov[ic].iov_base = HeaderData;
		iov[ic++].iov_len = ParametersLength;

		len = call->WriteV(call->fd, iov, ic);

		if (len < 0)
			return len;

		NalLengthBytes  = (avcCHeader->NalLengthMinusOne & 0x03) + 1;
		ParamSets       = avcCHeader->NumParamSets & 0x1f;
		h264_printf(20, "avcC contents:\n");
		h264_printf(20, "    version:                       %d\n", avcCHeader->Version);
		h264_printf(20, "    profile:                       %d\n", avcCHeader->Profile);
		h264_printf(20, "    profile compatibility:         %d\n", avcCHeader->Compatibility);
		h264_printf(20, "    level:                         %d\n", avcCHeader->Level);
		h264_printf(20, "    nal length bytes:              %d\n", NalLengthBytes);
		h264_printf(20, "    number of sequence param sets: %d\n", ParamSets);
		ParamOffset     = 0;
		ic = 0;
		iov[ic++].iov_base = PesHeader;
		for (i = 0; i < ParamSets; i++)
		{
			unsigned int  PsLength = (avcCHeader->Params[ParamOffset] << 8) + avcCHeader->Params[ParamOffset + 1];
			h264_printf(20, "        sps %d has length           %d\n", i, PsLength);
			iov[ic].iov_base = (char *)Head;
			iov[ic++].iov_len = sizeof(Head);
			InitialHeaderLength        += sizeof(Head);
			iov[ic].iov_base = &avcCHeader->Params[ParamOffset + 2];
			iov[ic++].iov_len = PsLength;
			InitialHeaderLength        += PsLength;
			ParamOffset                += PsLength + 2;
		}
		ParamSets                       = avcCHeader->Params[ParamOffset];
		h264_printf(20,  "    number of picture param sets:  %d\n", ParamSets);
		ParamOffset++;
		for (i = 0; i < ParamSets; i++)
		{
			unsigned int  PsLength      = (avcCHeader->Params[ParamOffset] << 8) + avcCHeader->Params[ParamOffset + 1];
			h264_printf(20, "        pps %d has length           %d\n", i, PsLength);
			iov[ic].iov_base = (char *) Head;
			iov[ic++].iov_len = sizeof(Head);
			InitialHeaderLength        += sizeof(Head);
			iov[ic].iov_base = &avcCHeader->Params[ParamOffset + 2];
			iov[ic++].iov_len = PsLength;
			InitialHeaderLength        += PsLength;
			ParamOffset                += PsLength + 2;
		}
		iov[0].iov_len = InsertPesHeader(PesHeader, InitialHeaderLength, MPEG_VIDEO_PES_START_CODE, INVALID_PTS_VALUE, 0);
		ssize_t l = call->WriteV(call->fd, iov, ic);
		if (l < 0)
			return l;
		len += l;
		initialHeader           = 0;
	}

	unsigned int SampleSize    = call->len;
	unsigned int NalStart      = 0;
	unsigned int VideoPosition = 0;

	do
	{
		unsigned int   NalLength;
		unsigned char  NalData[4];
		int NalPresent = 1;

		memcpy(NalData, call->data + VideoPosition, NalLengthBytes);
		VideoPosition += NalLengthBytes;
		NalStart += NalLengthBytes;
		switch (NalLengthBytes)
		{
			case 1:
				NalLength = (NalData[0]);
				break;
			case 2:
				NalLength = (NalData[0] <<  8) | (NalData[1]);
				break;
			case 3:
				NalLength = (NalData[0] << 16) | (NalData[1] <<  8) | (NalData[2]);
				break;
			default:
				NalLength = (NalData[0] << 24) | (NalData[1] << 16) | (NalData[2] << 8) | (NalData[3]);
				break;
		}
		
		h264_printf(20, "NalStart = %u + NalLength = %u > SampleSize = %u\n", NalStart, NalLength, SampleSize);
		
		if (NalStart + NalLength > SampleSize)
		{
			h264_printf(20, "nal length past end of buffer - size %u frame offset %u left %u\n",
				    NalLength, NalStart , SampleSize - NalStart);
			NalStart = SampleSize;
		}
		else
		{
			NalStart += NalLength;
			ic = 0;
			iov[ic++].iov_base = PesHeader;
			if (NalPresent)
			{
				NalPresent = 0;
				iov[ic].iov_base = (char *)Head;
				iov[ic++].iov_len = sizeof(Head);
			}
			iov[ic].iov_base = call->data + VideoPosition;
			iov[ic++].iov_len = NalLength;
			VideoPosition += NalLength;
			h264_printf(20, "  pts=%llu\n", VideoPts);
			iov[0].iov_len = InsertPesHeader(PesHeader, NalLength, MPEG_VIDEO_PES_START_CODE, VideoPts, 0);
			ssize_t l = call->WriteV(call->fd, iov, ic);
			if (l < 0)
				return l;
			len += l;
			VideoPts = INVALID_PTS_VALUE;
		}
	}
	while (NalStart < SampleSize);

	if (len < 0)
	{
		h264_err("error writing data errno = %d\n", errno);
		h264_err("%s\n", strerror(errno));
	}
#else
	/* AnnexA */
	if (!avc3 && ((1 < call->private_size && 0 == call->private_data[0]) ||
	             ((call->len > 3) && ((call->data[0] == 0x00 && call->data[1] == 0x00 && call->data[2] == 0x00 && call->data[3] == 0x01) ||
	             (call->data[0] == 0xff && call->data[1] == 0xff && call->data[2] == 0xff && call->data[3] == 0xff)))))
	{
		uint32_t i = 0;
		uint8_t InsertPrivData = !sps_pps_in_stream;
		uint32_t PacketLength = 0;
		uint32_t FakeStartCode = (call->Version << 8) | PES_VERSION_FAKE_START_CODE;
		iov[ic++].iov_base = PesHeader;

		while (InsertPrivData && i < 36 && (call->len - i) > 5)
		{
			if ((call->data[i] == 0x00 && call->data[i + 1] == 0x00 && call->data[i + 2] == 0x00 && call->data[i + 3] == 0x01 && (call->data[i + 4] == 0x67 || call->data[i + 4] == 0x68)))
			{
				InsertPrivData = 0;
				sps_pps_in_stream = 1;
			}
			i += 1;
		}

		if (InsertPrivData && call->private_size > 0 /*&& initialHeader*/) // some rtsp streams can update codec data at runtime
		{
			initialHeader = 0;
			iov[ic].iov_base  = call->private_data;
			iov[ic++].iov_len = call->private_size;
			PacketLength     += call->private_size;
		}

		iov[ic].iov_base  = call->data;
		iov[ic++].iov_len = call->len;
		PacketLength     += call->len;

		iov[0].iov_len = InsertPesHeader(PesHeader, -1, MPEG_VIDEO_PES_START_CODE, VideoPts, FakeStartCode);

		return call->WriteV(call->fd, iov, ic);
	}
	else if (!call->private_data || call->private_size < 7 || 1 != call->private_data[0])
	{
		h264_err("No valid private data available! [%d]\n", (int)call->private_size);
		return 0;
	}

	uint32_t PacketLength = 0;

	ic = 0;
	iov[ic++].iov_base = PesHeader;

	if (!avc3)
	{
		if (CodecData)
		{
			free(CodecData);
			CodecData = NULL;
		}

		uint8_t  *private_data = call->private_data;
		uint32_t  private_size = call->private_size;

		if (PreparCodecData(private_data, private_size, &NalLengthBytes))
		{
			UpdateExtraData(&private_data, &private_size, call->data, call->len);
			PreparCodecData(private_data, private_size, &NalLengthBytes);
		}

		if (private_data != call->private_data)
		{
			avc3 = 1;
			free(private_data);
			private_data = NULL;
		}

		if (CodecData != NULL)
		{
			iov[ic].iov_base  = CodecData;
			iov[ic++].iov_len = CodecDataLen;
			PacketLength     += CodecDataLen;
			initialHeader = 0;
		}
	}

	if (CodecData != NULL)
	{
		uint32_t pos = 0;
		do
		{
			if (ic >= 128)
			{
				h264_err(">> Drop data due to ic overflow\n");
				break;
			}

			uint32_t pack_len = 0;
			uint32_t i = 0;

			for (i = 0; i < NalLengthBytes; i++, pos++)
			{
				pack_len <<= 8;
				pack_len += call->data[pos];
			}

			if ((pos + pack_len) > call->len)
			{
				pack_len = call->len - pos;
			}

			iov[ic].iov_base  = Head;
			iov[ic++].iov_len = sizeof(Head);
			PacketLength += sizeof(Head);

			iov[ic].iov_base  = call->data + pos;
			iov[ic++].iov_len = pack_len;
			PacketLength     += pack_len;

			pos += pack_len;

		}
		while ((pos + NalLengthBytes) < call->len);

		h264_printf(10, "<<<< PacketLength [%d]\n", PacketLength);

		iov[0].iov_len = InsertPesHeader(PesHeader, -1, MPEG_VIDEO_PES_START_CODE, VideoPts, 0);

		len = call->WriteV(call->fd, iov, ic);
		PacketLength += iov[0].iov_len;

		if (PacketLength != len)
		{
			h264_err("<<<< not all data have been written [%d/%d]\n", len, PacketLength);
		}
	}

#endif

	h264_printf (10, "< len %d\n", len);
	return len;
}

/*
static int writeReverseData(void* _call)
{
	WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

#ifndef old_reverse_playback
	h264_printf(10, "\n");

	if (call == NULL)
	{
		h264_err("call data is NULL...\n");
		return 0;
	}

	h264_printf(10, "VideoPts %lld\n", call->Pts);

	if ((call->data == NULL) || (call->len <= 0))
	{
		h264_err("NULL Data. ignoring...\n");
		return 0;
	}

	if (call->fd < 0)
	{
		h264_err("file pointer < 0. ignoring ...\n");
		return 0;
	}

	return 0;

#else
	return 0;
#endif
}
*/

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

static WriterCaps_t caps = {
	"h264",
	eVideo,
	"V_MPEG4/ISO/AVC",
	VIDEO_STREAMTYPE_MPEG4_H264
};

struct Writer_s WriterVideoH264 = {
	&reset,
	&writeData,
	//&writeReverseData,
	NULL,
	&caps,
};


