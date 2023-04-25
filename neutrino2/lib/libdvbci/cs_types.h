/* pretty useless, but we don't need to work around this if
 * we just copy it over... */

#ifndef __CS_TYPES_H__
#define __CS_TYPES_H__

typedef enum
{
	AVSYNC_DISABLED,
	AVSYNC_ENABLED,
	AVSYNC_AUDIO_IS_MASTER
} AVSYNC_TYPE;

typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char        u8;
typedef signed long long    s64;
typedef signed int      s32;
typedef signed short        s16;
typedef signed char      s8;

#endif // __CS_TYPES_H__
