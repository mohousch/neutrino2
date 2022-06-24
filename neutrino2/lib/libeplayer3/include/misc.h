#ifndef misc_123
#define misc_123

#include <dirent.h>

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

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define INVALID_PTS_VALUE                       0x200000000ull

/*#define BIG_READS*/
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

/* subtitle hacks ->for file subtitles */
#define TEXTSRTOFFSET 100
#define TEXTSSAOFFSET 200

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

void PutBits(BitPacker_t * ld, unsigned int code, unsigned int length);
void FlushBits(BitPacker_t * ld);

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

#endif


