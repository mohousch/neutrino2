/*
 * misc.h
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
 
 #ifndef misc_123
#define misc_123

#include <dirent.h>
#include <libavcodec/avcodec.h>

/* some useful things needed by many files ... */

/* ***************************** */
/* Types                         */
/* ***************************** */
typedef struct BitPacker_s
{
	unsigned char*      Ptr;                                    /* write pointer */
	unsigned int        BitBuffer;                              /* bitreader shifter */
	int                 Remaining;                              /* number of remaining in the shifter */
} BitPacker_t;

typedef struct pcmPrivateData_s
{
	int uNoOfChannels;
	int uSampleRate;
	int uBitsPerSample;
	int bLittleEndian;
	
	//
	uint8_t bResampling;
	enum AVCodecID avCodecId;
	uint8_t *private_data;
    	uint32_t private_size;
    	int32_t block_align;
    	int32_t frame_size;
    	int32_t bits_per_coded_sample;
    	int32_t bit_rate;
} pcmPrivateData_t;

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define INVALID_PTS_VALUE                       0x200000000ull

//#define BIG_READS
#if defined (BIG_READS)
#define BLOCK_COUNT                             8
#else
#define BLOCK_COUNT                             1
#endif
#define TP_PACKET_SIZE                          188
#define BD_TP_PACKET_SIZE                       192
#define NUMBER_PACKETS                          (199*BLOCK_COUNT)
#define BUFFER_SIZE                             (TP_PACKET_SIZE*NUMBER_PACKETS)
#define PADDING_LENGTH                          (1024*BLOCK_COUNT)

//// aac
#define AAC_HEADER_LENGTH       		7

//// pes
#define PES_HEADER_SIZE				9
#define PES_MAX_HEADER_SIZE                     (PES_HEADER_SIZE + 256) // 64
#define PES_PRIVATE_DATA_FLAG                   0x80
#define PES_PRIVATE_DATA_LENGTH                 8
#define PES_LENGTH_BYTE_0                       5
#define PES_LENGTH_BYTE_1                       4
#define PES_FLAGS_BYTE                          7
#define PES_EXTENSION_DATA_PRESENT              0x01
#define PES_HEADER_DATA_LENGTH_BYTE             8
#define PES_START_CODE_RESERVED_4               0xfd
#define PES_VERSION_FAKE_START_CODE             0x31

////
#define MAX_PES_PACKET_SIZE                     (65535) //65400

//// start codes
#define PCM_AUDIO_PES_START_CODE        	0xbd
#define H263_VIDEO_PES_START_CODE		0xfe
#define H264_VIDEO_PES_START_CODE               0xe2
#define MPEG_VIDEO_PES_START_CODE               0xe0
#define MPEG_AUDIO_PES_START_CODE           	0xc0
#define VC1_VIDEO_PES_START_CODE	    	0xfd
#define AAC_AUDIO_PES_START_CODE            	0xcf

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

void PutBits(BitPacker_t * ld, unsigned int code, unsigned int length);
void FlushBits(BitPacker_t * ld);
////
int32_t InsertPesHeader(uint8_t *data, int32_t size, unsigned char stream_id, uint64_t pts, int32_t pic_start_code);
int InsertVideoPrivateDataHeader(uint8_t *data, int32_t payload_size);
void UpdatePesHeaderPayloadSize(uint8_t *data, int32_t size);

//// aac
static inline int aac_get_sample_rate_index (uint32_t sample_rate)
{
	if (96000 <= sample_rate)
		return 0;
	else if (88200 <= sample_rate)
		return 1;
	else if (64000 <= sample_rate)
		return 2;
	else if (48000 <= sample_rate)
		return 3;
	else if (44100 <= sample_rate)
		return 4;
	else if (32000 <= sample_rate)
		return 5;
	else if (24000 <= sample_rate)
		return 6;
	else if (22050 <= sample_rate)
		return 7;
	else if (16000 <= sample_rate)
		return 8;
	else if (12000 <= sample_rate)
		return 9;
	else if (11025 <= sample_rate)
		return 10;
	else if (8000 <= sample_rate)
		return 11;
	else if (7350 <= sample_rate)
		return 12;
	else
	      return 13;
}

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

static inline void getExtension(char * FILENAMEname, char ** extension) 
{
	int i = 0;
	int stringlength;

	if (extension == NULL)
		return;
	  
	*extension = NULL;
	
	if (FILENAMEname == NULL)
		return;

	stringlength = (int) strlen(FILENAMEname);

	for (i = 0; stringlength - i > 0; i++) 
	{
		if (FILENAMEname[stringlength - i - 1] == '.') 
		{
			*extension = strdup(FILENAMEname+(stringlength - i));
			break;
		}
	}
}

static inline void getUPNPExtension(char * FILENAMEname, char ** extension) 
{
	char* str;

	if (extension == NULL)
		return;
	  
	*extension = NULL;
	
	if (FILENAMEname == NULL)
		return;

	str = strstr(FILENAMEname, "ext=");

	if (str != NULL)
	{
		*extension = strdup(str + strlen("ext=") + 1);
		return;
	}
	*extension = NULL;
}

/* the function returns the base name */
static inline char * basename(char * name)
{
	int i = 0;
	int pos = 0;

	while(name[i] != 0)
	{
		if(name[i] == '/')
			pos = i;
		i++;
	}

	if(name[pos] == '/')
		pos++;

	return name + pos;
}

/* the function returns the directry name */
static inline char * dirname(char * name)
{
	static char path[100];
	int i = 0;
	int pos = 0;

	while((name[i] != 0) && (i < sizeof(path)))
	{
		if(name[i] == '/')
			pos = i;
		path[i] = name[i];
		i++;
	}

	path[i] = 0;
	path[pos] = 0;

	return path;
}

static inline uint32_t ReadUint32(uint8_t *buffer)
{
	uint32_t num = (uint32_t)buffer[0] << 24 |
	               (uint32_t)buffer[1] << 16 |
	               (uint32_t)buffer[2] << 8  |
	               (uint32_t)buffer[3];
	return num;
}

static inline uint16_t ReadUInt16(uint8_t *buffer)
{
	uint16_t num = (uint16_t)buffer[0] << 8 |
	               (uint16_t)buffer[1];
	return num;
}

static inline void Hexdump(unsigned char *Data, int length)
{
	int k;
	for (k = 0; k < length; k++)
	{
		printf("%02x ", Data[k]);
		if (((k + 1) & 31) == 0)
			printf("\n");
	}
	printf("\n");
}

#endif

