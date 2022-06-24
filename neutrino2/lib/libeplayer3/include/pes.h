#ifndef pes_123
#define pes_123

#define PES_MAX_HEADER_SIZE                     64
#define PES_PRIVATE_DATA_FLAG                   0x80
#define PES_PRIVATE_DATA_LENGTH                 8
#define PES_LENGTH_BYTE_0                       5
#define PES_LENGTH_BYTE_1                       4
#define PES_FLAGS_BYTE                          7
#define PES_EXTENSION_DATA_PRESENT              0x01
#define PES_HEADER_DATA_LENGTH_BYTE             8
#define PES_MIN_HEADER_SIZE                     9
#define PES_START_CODE_RESERVED_4               0xfd
#define PES_VERSION_FAKE_START_CODE             0x31


#define MAX_PES_PACKET_SIZE                     65400


/* start codes */
#define PCM_PES_START_CODE        		0xbd
#define PRIVATE_STREAM_1_PES_START_CODE         0xbd
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
