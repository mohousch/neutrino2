/*
 * pes helper
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
 
 #ifndef pes_123
#define pes_123


////
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

int InsertPesHeader(unsigned char *data, int size, unsigned char stream_id, unsigned long long int pts, int pic_start_code);
int InsertVideoPrivateDataHeader(unsigned char *data, int payload_size);
void UpdatePesHeaderPayloadSize(uint8_t *data, int32_t size);

#endif

