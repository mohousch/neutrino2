#ifndef __tuxtxt_h__
#define __tuxtxt_h__

/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    continued 2004-2005 by Roland Meier <RolandMeier@Siemens.com>           *
 *                       and DBLuelle <dbluelle@blau-weissoedingen.de>        *
 *                                                                            *
 *              ported 2006 to Dreambox 7025 / 32Bit framebuffer              *
 *                   by Seddi <seddi@i-have-a-dreambox.com>                   *
 *                                                                            *
 ******************************************************************************/

#define TUXTXT_DEBUG 0

#include <config.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>

#include <linux/fb.h>

#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <driver/gdi/framebuffer.h>

#include "tuxtxt_def.h"

#include <driver/rcinput.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

/* variables and functions from libtuxtxt */
extern tuxtxt_cache_struct tuxtxt_cache;
extern int tuxtxt_init();
extern void tuxtxt_close();
extern void tuxtxt_start(int tpid);  // Start caching
extern int  tuxtxt_stop(); // Stop caching
extern void tuxtxt_next_dec(int *i); /* skip to next decimal */
extern void tuxtxt_prev_dec(int *i); /* counting down */
extern int tuxtxt_is_dec(int i);
extern int tuxtxt_next_hex(int i);
extern void tuxtxt_decode_btt();
extern void tuxtxt_decode_adip(); /* additional information table */
extern void tuxtxt_compress_page(int p, int sp, unsigned char* buffer);
extern void tuxtxt_decompress_page(int p, int sp, unsigned char* buffer);
#if TUXTXT_DEBUG
extern int tuxtxt_get_zipsize(int p, int sp);
#endif

#define TUXTXTCONF CONFIGDIR "/tuxtxt/tuxtxt2.conf"

/* fonts */
#define TUXTXTTTF DATADIR "/fonts/tuxtxt.ttf"
#define TUXTXTOTB DATADIR "/fonts/tuxtxt.otb"

//
int TTFWidthFactor16, TTFHeightFactor16, TTFShiftX, TTFShiftY; /* parameters for adapting to various TTF fonts */
int fontheight, fontwidth, fontwidth_normal, fontwidth_small, fontwidth_topmenumain, fontwidth_topmenusmall, ascender;
int ymosaic[4];
int displaywidth;

#define TV43STARTX 		(ex - 146) 		
#define TV169FULLSTARTX 	(sx +(ex +1 - sx)/2)
#define TVENDX 			ex
#define TVENDY 			(StartY + 25*fontheight)
#define TV43WIDTH 		144 			/* 120 */
#define TV43HEIGHT 		116 			/* 96 */
#define TV43STARTY 		(TVENDY - TV43HEIGHT)
#define TV169FULLSTARTY 	(720 - 360)/2 //sy
#define TV169FULLWIDTH  	(ex - sx)/2
#define TV169FULLHEIGHT 	360 //(ey - sy)
//
#define TOPMENUSTARTX 		TV43STARTX+2
#define TOPMENUENDX 		TVENDX
#define TOPMENUSTARTY 		StartY
#define TOPMENUENDY 		TV43STARTY
//
#define TOPMENULINEWIDTH 	((TOPMENUENDX-TOPMENU43STARTX+fontwidth_topmenusmall-1)/fontwidth_topmenusmall)
#define TOPMENUINDENTBLK 	0
#define TOPMENUINDENTGRP 	1
#define TOPMENUINDENTDEF 	2
#define TOPMENUSPC 0
#define TOPMENUCHARS 		(TOPMENUINDENTDEF+12+TOPMENUSPC+4)
//
#define FLOFSIZE 		4
// spacing attributes
#define alpha_black         0x00
#define alpha_red           0x01
#define alpha_green         0x02
#define alpha_yellow        0x03
#define alpha_blue          0x04
#define alpha_magenta       0x05
#define alpha_cyan          0x06
#define alpha_white         0x07
#define flash               0x08
#define steady              0x09
#define end_box             0x0A
#define start_box           0x0B
#define normal_size         0x0C
#define double_height       0x0D
#define double_width        0x0E
#define double_size         0x0F
#define mosaic_black        0x10
#define mosaic_red          0x11
#define mosaic_green        0x12
#define mosaic_yellow       0x13
#define mosaic_blue         0x14
#define mosaic_magenta      0x15
#define mosaic_cyan         0x16
#define mosaic_white        0x17
#define conceal             0x18
#define contiguous_mosaic   0x19
#define separated_mosaic    0x1A
#define esc                 0x1B
#define black_background    0x1C
#define new_background      0x1D
#define hold_mosaic         0x1E
#define release_mosaic      0x1F
// rc codes
#define RC_0        CRCInput::RC_0
#define RC_1        CRCInput::RC_1
#define RC_2        CRCInput::RC_2
#define RC_3        CRCInput::RC_3
#define RC_4        CRCInput::RC_4
#define RC_5        CRCInput::RC_5
#define RC_6        CRCInput::RC_6
#define RC_7        CRCInput::RC_7
#define RC_8        CRCInput::RC_8
#define RC_9        CRCInput::RC_9
#define RC_RIGHT    CRCInput::RC_right
#define RC_LEFT     CRCInput::RC_left
#define RC_UP       CRCInput::RC_up
#define RC_DOWN     CRCInput::RC_down
#define RC_OK       CRCInput::RC_ok
#define RC_MUTE     CRCInput::RC_spkr
#define RC_STANDBY  CRCInput::RC_standby
#define RC_GREEN    CRCInput::RC_green
#define RC_YELLOW   CRCInput::RC_yellow
#define RC_RED      CRCInput::RC_red
#define RC_BLUE     CRCInput::RC_blue
#define RC_PLUS     CRCInput::RC_plus
#define RC_MINUS    CRCInput::RC_minus
#define RC_HELP     CRCInput::RC_info
#define RC_DBOX     CRCInput::RC_setup
#define RC_TEXT     CRCInput::RC_text
#define RC_HOME     CRCInput::RC_home

//
typedef enum
{
	OBJ_PASSIVE,
	OBJ_ACTIVE,
	OBJ_ADAPTIVE
} tObjType;

//
const char *ObjectSource[] =
{
	"(illegal)",
	"Local",
	"POP",
	"GPOP"
};

//
const char *ObjectType[] =
{
	"Passive",
	"Active",
	"Adaptive",
	"Passive"
};

// messages
#define ShowInfoBar     0
#define PageNotFound    1
#define ShowServiceName 2
#define NoServicesFound 3

// framebuffer stuff
static unsigned char * lfb = 0;

// freetype stuff
FT_Library      library;
FTC_Manager     manager;
static FTC_SBitCache   cache;
FTC_SBit        sbit;

#define FONTTYPE FTC_ImageTypeRec

FT_Face			face;
FONTTYPE typettf;

// G2 Set as defined in ETS 300 706
const unsigned short int G2table[4][6*16] =
{
        { 0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0023, 0x00A7, 0x00A4, 0x2018, 0x201C, 0x00AB, 0x2190, 0x2191, 0x2192, 0x2193,
          0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x00B5, 0x00B6, 0x00B7, 0x00F7, 0x2019, 0x201D, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
          0x0020, 0x0300, 0x0301, 0x02C6, 0x0303, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
          0x2014, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x0020, 0x0020, 0x0020, 0x215B, 0x215C, 0x215D, 0x215E,
          0x2126, 0x00C6, 0x00D0, 0x00AA, 0x0126, 0x0020, 0x0132, 0x013F, 0x0141, 0x00D8, 0x0152, 0x00BA, 0x00DE, 0x0166, 0x014A, 0x0149,
          0x0138, 0x00E6, 0x0111, 0x00F0, 0x0127, 0x0131, 0x0133, 0x0140, 0x0142, 0x00F8, 0x0153, 0x00DF, 0x00FE, 0x0167, 0x014B, 0x25A0},

        { 0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0020, 0x00A7, 0x0020, 0x2018, 0x201C, 0x00AB, 0x2190, 0x2191, 0x2192, 0x2193,
          0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x00B5, 0x00B6, 0x00B7, 0x00F7, 0x2019, 0x201D, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
          0x0020, 0x0300, 0x0301, 0x02C6, 0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
          0x2014, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x0141, 0x0142, 0x00DF, 0x215B, 0x215C, 0x215D, 0x215E,
          0x0044, 0x0045, 0x0046, 0x0047, 0x0049, 0x004A, 0x004B, 0x004C, 0x004E, 0x0051, 0x0052, 0x0053, 0x0055, 0x0056, 0x0057, 0x005A,
          0x0064, 0x0065, 0x0066, 0x0067, 0x0069, 0x006A, 0x006B, 0x006C, 0x006E, 0x0071, 0x0072, 0x0073, 0x0075, 0x0076, 0x0077, 0x007A},

        { 0x0020, 0x0061, 0x0062, 0x00A3, 0x0065, 0x0068, 0x0069, 0x00A7, 0x003A, 0x2018, 0x201C, 0x006B, 0x2190, 0x2191, 0x2192, 0x2193,
          0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x006D, 0x006E, 0x0070, 0x00F7, 0x2019, 0x201D, 0x0074, 0x00BC, 0x00BD, 0x00BE, 0x0078,
          0x0020, 0x0300, 0x0301, 0x02C6, 0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
          0x003F, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x038A, 0x038E, 0x038F, 0x215B, 0x215C, 0x215D, 0x215E,
          0x0043, 0x0044, 0x0046, 0x0047, 0x004A, 0x004C, 0x0051, 0x0052, 0x0053, 0x0055, 0x0056, 0x0057, 0x0059, 0x005A, 0x0386, 0x0389,
          0x0063, 0x0064, 0x0066, 0x0067, 0x006A, 0x006C, 0x0071, 0x0072, 0x0073, 0x0075, 0x0076, 0x0077, 0x0079, 0x007A, 0x0388, 0x25A0},

        { 0x0020, 0x0639, 0xFEC9, 0xFE83, 0xFE85, 0xFE87, 0xFE8B, 0xFE89, 0xFB7C, 0xFB7D, 0xFB7A, 0xFB58, 0xFB59, 0xFB56, 0xFB6D, 0xFB8E,
          0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFECE, 0xFECD, 0xFEFC, 0xFEEC, 0xFEEA, 0xFEE9,
          0x00E0, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
          0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00EB, 0x00EA, 0x00F9, 0x00EE, 0xFECA,
          0x00E9, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
          0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00E2, 0x00F4, 0x00FB, 0x00E7, 0x25A0}
};

const unsigned short int G0table[6][6*16] =
{
        { ' ', '!', '\"', '#', '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
          0x0427, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0408, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
          0x041F, 0x040C, 0x0420, 0x0421, 0x0422, 0x0423, 0x0412, 0x0403, 0x0409, 0x040A, 0x0417, 0x040B, 0x0416, 0x0402, 0x0428, 0x040F,
          0x0447, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0458, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
          0x043F, 0x045C, 0x0440, 0x0441, 0x0442, 0x0443, 0x0432, 0x0453, 0x0459, 0x045A, 0x0437, 0x045B, 0x0436, 0x0452, 0x0448, 0x25A0},

        { ' ', '!', '\"', '#', '$', '%', 0x044B, '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
          0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
          0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042A, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042B,
          0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
          0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x044A, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x25A0},

        { ' ', '!', '\"', '#', '$', '%', 0x0457, '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
          0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
          0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x0406, 0x0417, 0x0428, 0x0404, 0x0429, 0x0427, 0x0407,
          0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
          0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x0456, 0x0437, 0x0448, 0x0454, 0x0449, 0x0447, 0x25A0},

        { ' ', '!', '\"', '#', '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', 0x00AB, '=', 0x00BB, '?',
          0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
          0x03A0, 0x03A1, 0x0384, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
          0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
          0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x25A0},

        { ' ', '!', 0x05F2, 0x00A3, '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
          '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
          'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x2190, 0x00BD, 0x2192, 0x2191, '#',
          0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
          0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x20AA, 0x2551, 0x00BE, 0x00F7, 0x25A0},

        { ' ', '!', 0x05F2, 0x00A3, '$', 0x066A, 0xFEF0, 0xFEF2, 0xFD3F, 0xFD3E, '*', '+', ',', '-', '.', '/',
          '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', 0x061B, '>', '=', '<', 0x061F,
          0xFE94, 0x0621, 0xFE92, 0x0628, 0xFE98, 0x062A, 0xFE8E, 0xFE8D, 0xFE91, 0xFE93, 0xFE97, 0xFE9B, 0xFE9F, 0xFEA3, 0xFEA7, 0xFEA9,
          0x0630, 0xFEAD, 0xFEAF, 0xFEB3, 0xFEB7, 0xFEBB, 0xFEBF, 0xFEC1, 0xFEC5, 0xFECB, 0xFECF, 0xFE9C, 0xFEA0, 0xFEA4, 0xFEA8, 0x0023,
          0x0640, 0xFED3, 0xFED7, 0xFEDB, 0xFEDF, 0xFEE3, 0xFEE7, 0xFEEB, 0xFEED, 0xFEEF, 0xFEF3, 0xFE99, 0xFE9D, 0xFEA1, 0xFEA5, 0xFEF4,
          0xFEF0, 0xFECC, 0xFED0, 0xFED4, 0xFED1, 0xFED8, 0xFED5, 0xFED9, 0xFEE0, 0xFEDD, 0xFEE4, 0xFEE1, 0xFEE8, 0xFEE5, 0xFEFB, 0x25A0}
};

const unsigned short int nationaltable23[14][2] =
{
        { '#',    0x00A4 }, /* 0          */
        { '#',    0x016F }, /* 1  CS/SK   */
        { 0x00A3,    '$' }, /* 2    EN    */
        { '#',    0x00F5 }, /* 3    ET    */
        { 0x00E9, 0x0457 }, /* 4    FR    */
        { '#',       '$' }, /* 5    DE    */
        { 0x00A3,    '$' }, /* 6    IT    */
        { '#',       '$' }, /* 7  LV/LT   */
        { '#',    0x0144 }, /* 8    PL    */
        { 0x00E7,    '$' }, /* 9  PT/ES   */
        { '#',    0x00A4 }, /* A    RO    */
        { '#',    0x00CB }, /* B SR/HR/SL */
        { '#',    0x00A4 }, /* C SV/FI/HU */
        { 0x20A4, 0x011F }, /* D    TR    */
};

const unsigned short int nationaltable40[14] =
{
        '@',    /* 0          */
        0x010D, /* 1  CS/SK   */
        '@',    /* 2    EN    */
        0x0161, /* 3    ET    */
        0x00E0, /* 4    FR    */
        0x00A7, /* 5    DE    */
        0x00E9, /* 6    IT    */
        0x0161, /* 7  LV/LT   */
        0x0105, /* 8    PL    */
        0x00A1, /* 9  PT/ES   */
        0x0162, /* A    RO    */
        0x010C, /* B SR/HR/SL */
        0x00C9, /* C SV/FI/HU */
        0x0130, /* D    TR    */
};

const unsigned short int nationaltable5b[14][6] =
{
        {    '[',   '\\',    ']',    '^',    '_',    '`' }, /* 0          */
        { 0x0165, 0x017E, 0x00FD, 0x00ED, 0x0159, 0x00E9 }, /* 1  CS/SK   */
        { 0x2190, 0x00BD, 0x2192, 0x2191,    '#', 0x00AD }, /* 2    EN    */
        { 0x00C4, 0x00D6, 0x017D, 0x00DC, 0x00D5, 0x0161 }, /* 3    ET    */
        { 0x0451, 0x00EA, 0x00F9, 0x00EE,    '#', 0x00E8 }, /* 4    FR    */
        { 0x00C4, 0x00D6, 0x00DC,    '^',    '_', 0x00B0 }, /* 5    DE    */
        { 0x00B0, 0x00E7, 0x2192, 0x2191,    '#', 0x00F9 }, /* 6    IT    */
        { 0x0117, 0x0119, 0x017D, 0x010D, 0x016B, 0x0161 }, /* 7  LV/LT   */
        { 0x017B, 0x015A, 0x0141, 0x0107, 0x00F3, 0x0119 }, /* 8    PL    */
        { 0x00E1, 0x00E9, 0x00ED, 0x00F3, 0x00FA, 0x00BF }, /* 9  PT/ES   */
        { 0x00C2, 0x015E, 0x01CD, 0x01CF, 0x0131, 0x0163 }, /* A    RO    */
        { 0x0106, 0x017D, 0x00D0, 0x0160, 0x0451, 0x010D }, /* B SR/HR/SL */
        { 0x00C4, 0x00D6, 0x00C5, 0x00DC,    '_', 0x00E9 }, /* C SV/FI/HU */
        { 0x015E, 0x00D6, 0x00C7, 0x00DC, 0x011E, 0x0131 }, /* D    TR    */
};

const unsigned short int nationaltable7b[14][4] =
{
        { '{',       '|',    '}',    '~' }, /* 0          */
        { 0x00E1, 0x011B, 0x00FA, 0x0161 }, /* 1  CS/SK   */
        { 0x00BC, 0x2551, 0x00BE, 0x00F7 }, /* 2    EN    */
        { 0x00E4, 0x00F6, 0x017E, 0x00FC }, /* 3    ET    */
        { 0x00E2, 0x00F4, 0x00FB, 0x00E7 }, /* 4    FR    */
        { 0x00E4, 0x00F6, 0x00FC, 0x00DF }, /* 5    DE    */
        { 0x00E0, 0x00F3, 0x00E8, 0x00EC }, /* 6    IT    */
        { 0x0105, 0x0173, 0x017E, 0x012F }, /* 7  LV/LT   */
        { 0x017C, 0x015B, 0x0142, 0x017A }, /* 8    PL    */
        { 0x00FC, 0x00F1, 0x00E8, 0x00E0 }, /* 9  PT/ES   */
        { 0x00E2, 0x015F, 0x01CE, 0x00EE }, /* A    RO    */
        { 0x0107, 0x017E, 0x0111, 0x0161 }, /* B SR/HR/SL */
        { 0x00E4, 0x00F6, 0x00E5, 0x00FC }, /* C SV/FI/HU */
        { 0x015F, 0x00F6, 0x00E7, 0x00FC }, /* D    TR    */
};

const unsigned short int arrowtable[] = {
	8592, 8594, 8593, 8595, 'O', 'K', 8592, 8592
};

// national subsets
const char countrystring[] =
"         Default          "   /*  0 no subset specified */
"       Czech/Slovak       "   /*  1 czech, slovak */
"         English          "   /*  2 english */
"         Estonian         "   /*  3 estonian */
"          French          "   /*  4 french */
"         Deutsch          "   /*  5 german */
"         Italian          "   /*  6 italian */
"    Latvian/Lithuanian    "   /*  7 latvian, lithuanian */
"          Polish          "   /*  8 polish */
"    Portuguese/Spanish    "   /*  9 portuguese, spanish */
"         Romanian         "   /* 10 romanian */
"Serbian/Croatian/Slovenian"   /* 11 serbian, croatian, slovenian */
"Swedish/Finnish/Hungarian "   /* 12 swedish, finnish, hungarian */
"          T~rk}e          "   /* 13 turkish */
"      Srpski/Hrvatski     "   /* 14 serbian, croatian */
"     Russkij/Bylgarski    "   /* 15 russian, bulgarian */
"        Ukra&nsxka        "   /* 16 ukrainian */
"         Ekkgmij\\         "   /* 17 greek */
"          zixar           "   /* 18 hebrew */
"           pHQY           "   /* 19 arabic */
;

#define COUNTRYSTRING_WIDTH 26
#define MAX_NATIONAL_SUBSET (sizeof(countrystring) / COUNTRYSTRING_WIDTH - 1)

enum
{
        NAT_DEFAULT = 0,
        NAT_CZ = 1,
        NAT_UK = 2,
        NAT_ET = 3,
        NAT_FR = 4,
        NAT_DE = 5,
        NAT_IT = 6,
        NAT_LV = 7,
        NAT_PL = 8,
        NAT_SP = 9,
        NAT_RO = 10,
        NAT_SR = 11,
        NAT_SW = 12,
        NAT_TR = 13,
        NAT_MAX_FROM_HEADER = 13,
        NAT_SC = 14,
        NAT_RB = 15,
        NAT_UA = 16,
        NAT_GR = 17,
        NAT_HB = 18,
        NAT_AR = 19
};

const unsigned char countryconversiontable[] = { NAT_UK, NAT_DE, NAT_SW, NAT_IT, NAT_FR, NAT_SP, NAT_CZ, NAT_RO};

// some data
//char versioninfo[16];
int hotlist[10];
int maxhotlist;
//
int pig, fb;
int sx, ex, sy, ey;
int PosX, PosY, StartX, StartY;
int lastpage;
int inputcounter;
int zoommode, screenmode, transpmode, hintmode, boxed, nofirst, savedscreenmode, showflof, show39, showl25, prevscreenmode;
char dumpl25;
int catch_row, catch_col, catched_page, pagecatching;
int prev_100, prev_10, next_10, next_100;
int screen_mode1, screen_mode2, color_mode, trans_mode, national_subset, national_subset_secondary, auto_national, swapupdown, showhex, menulanguage;
int pids_found, current_service, getpidsdone;
int SDT_ready;
int pc_old_row, pc_old_col;     // for page catching
int temp_page;	// for page input
char saveconfig, hotlistchanged;
signed char clearbbcolor = -1;
int usettf;
short pop, gpop, drcs, gdrcs;
unsigned char tAPx, tAPy;	// temporary offset to Active Position for objects
unsigned char axdrcs[12+1+10+1];
#define aydrcs (axdrcs + 12+1)
unsigned char FullRowColor[25];
unsigned char FullScrColor;
tstPageinfo *pageinfo = 0; // pointer to cached info of last decoded page
const char * fncmodes[] = {"12", "6"};
const char * saamodes[] = {"4:3_full_format", "16:9_full_format"};

struct timeval tv_delay;
int  subtitledelay, delaystarted;
FILE *conf;

unsigned short RCCode;

struct _pid_table
{
	int  vtxt_pid;
	int  service_id;
	int  service_name_len;
	char service_name[24];
	int  national_subset;
}pid_table[128];

unsigned char restoreaudio = 0;

// language dependent texts
#define MAXMENULANGUAGE 10
const int menusubset[] = { 
	NAT_DE, 
	NAT_UK, 
	NAT_FR, 
	NAT_UK, 
	NAT_GR, 
	NAT_IT, 
	NAT_PL, 
	NAT_SW, 
	NAT_SW,   
	NAT_SP,      
	NAT_RB
};

#define Menu_StartX 	(StartX + fontwidth*9/2)
#define Menu_StartY 	(StartY + fontheight)
#define Menu_Height 	23
#define Menu_Width 	31

const char MenuLine[] =
{
	3,8,11,12,15,17,19,20,21
};

enum
{
	M_HOT=0,
	M_PID,
	M_SC1,
	M_SC2,
	M_COL,
	M_TRA,
	M_AUN,
	M_NAT,
	M_LNG,
	M_Number
};

#define M_Start M_HOT
#define M_MaxDirect M_AUN

const char hotlistpagecolumn[] =	/* last(!) column of page to show in each language */
{
	22, 26, 28, 27, 28, 27, 28, 21, 20
};

const char hotlisttextcolumn[] =
{
	24, 14, 14, 15, 14, 15, 14, 23, 22
};

const char hotlisttext[][2*6] =
{
	{ "dazu entf." },
	{ " add rem. " },
	{ "ajoutenlev" },
	{ "toev.verw." },
	{ "pq|shava_q" },
	{ "agg. elim." },
	{ "dodajkasuj" },
	{ "ny   bort " },
	{ "lis{{pois " },
};

const char configonoff[][2*4] =
{
	{ "ausein" },
	{ "offon " },
	{ "desact" },
	{ "uitaan" },
	{ "emeape" },
	{ "offon " },
	{ "wy}w} " },
	{ "p} av " },
	{ "EI ON " },
};

const char menuatr[Menu_Height*(Menu_Width + 1)] =
{
	"0000000000000000000000000000002"
	"0111111111111111111111111111102"
	"0000000000000000000000000000002"
	"3144444444444444444444444444432"
	"3556655555555555555555555555532"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"3444444444444444444444444444432"
	"3155555555555555555555555555532"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3334444444444444444444444443332"
	"2222222222222222222222222222222"
};

//const char configmenu[][Menu_Height*(Menu_Width+1)] = {};
//const char catchmenutext[][81] = {};
const char configmenu[][Menu_Height * (Menu_Width + 8)] =
{
	{
		/*     0123456789012345678901234567890 */
		"àááááááááááááááááááááááááááááâè"
		"ã     Konfigurationsmen}     äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favoriten: Seite 111 dazu äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletext-Auswahl      äé"
		"ãí          suchen          îäé"
		"ã                            äé"
		"ã      Bildschirmformat      äé"
		"ã3  Standard-Modus 16:9      äé"
		"ã                            äé"
		"ã4        Helligkeit         äé"
		"ãí                          îäé"
		"ã5       Transparenz         äé"
		"ãí                          îäé"
		"ã6  nationaler Zeichensatz   äé"
		"ãautomatische Erkennung      äé"
		"ãí                          îäé"
		"ãí Sprache/Language deutsch îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã     Configuration menu     äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favorites:  add page 111  äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletext selection    äé"
		"ãí          search          îäé"
		"ã                            äé"
		"ã        Screen format       äé"
		"ã3 Standard mode 16:9        äé"
		"ã                            äé"
		"ã4        Brightness         äé"
		"ãí                          îäé"
		"ã5       Transparency        äé"
		"ãí                          îäé"
		"ã6   national characterset   äé"
		"ã automatic recognition      äé"
		"ãí                          îäé"
		"ãí Sprache/language english îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã    Menu de configuration   äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favorites: ajout. page 111äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2  Selection de teletext    äé"
		"ãí        recherche         îäé"
		"ã                            äé"
		"ã      Format de l'#cran     äé"
		"ã3 Mode standard 16:9        äé"
		"ã                            äé"
		"ã4          Clarte           äé"
		"ãí                          îäé"
		"ã5       Transparence        äé"
		"ãí                          îäé"
		"ã6     police nationale      äé"
		"ãreconn. automatique         äé"
		"ãí                          îäé"
		"ãí Sprache/language francaisîäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã      Configuratiemenu      äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favorieten: toev. pag 111 äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletekst-selectie    äé"
		"ãí          zoeken          îäé"
		"ã                            äé"
		"ã     Beeldschermformaat     äé"
		"ã3   Standaardmode 16:9      äé"
		"ã                            äé"
		"ã4        Helderheid         äé"
		"ãí                          îäé"
		"ã5       Transparantie       äé"
		"ãí                          îäé"
		"ã6    nationale tekenset     äé"
		"ãautomatische herkenning     äé"
		"ãí                          îäé"
		"ãí Sprache/Language nederl. îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã      Lemo} quhl_seym       äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Vaboq_:    pqo_h. sek. 111äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Epikoc^ Teket]nt      äé"
		"ãí        Amaf^tgsg         îäé"
		"ã                            äé"
		"ã       Loqv^ oh|mgr         äé"
		"ã3 Tq|por pq|tupor   16:9    äé"
		"ã                            äé"
		"ã4       Kalpq|tgta          äé"
		"ãí                          îäé"
		"ã5       Diav\\meia           äé"
		"ãí                          îäé"
		"ã6    Ehmij^ tuposeiq\\       äé"
		"ãaut|latg amacm~qisg         äé"
		"ãí                          îäé"
		"ãí Ck~ssa/Language ekkgmij\\ îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã   Menu di configurazione   äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1  Preferiti:  agg. pag.111 äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2   Selezione televideo     äé"
		"ãí         ricerca          îäé"
		"ã                            äé"
		"ã      Formato schermo       äé"
		"ã3  Modo standard 16:9       äé"
		"ã                            äé"
		"ã4        Luminosit{         äé"
		"ãí                          îäé"
		"ã5        Trasparenza        äé"
		"ãí                          îäé"
		"ã6   nazionalita'caratteri   äé"
		"ã riconoscimento automatico  äé"
		"ãí                          îäé"
		"ãí Lingua/Language Italiana îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã        Konfiguracja        äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Ulubione : kasuj  str. 111äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Wyb_r telegazety      äé"
		"ãí          szukaj          îäé"
		"ã                            äé"
		"ã       Format obrazu        äé"
		"ã3 Tryb standard 16:9        äé"
		"ã                            äé"
		"ã4          Jasno|^          äé"
		"ãí                          îäé"
		"ã5      Prze~roczysto|^      äé"
		"ãí                          îäé"
		"ã6 Znaki charakterystyczne   äé"
		"ã automatyczne rozpozn.      äé"
		"ãí                          îäé"
		"ãí  J`zyk/Language   polski îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã     Konfigurationsmeny     äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favoriter: sida 111 ny    äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2      TextTV v{ljaren      äé"
		"ãí            s|k           îäé"
		"ã                            äé"
		"ã        TV- format          äé"
		"ã3 Standard l{ge 16:9        äé"
		"ã                            äé"
		"ã4        Ljusstyrka         äé"
		"ãí                          îäé"
		"ã5     Genomskinlighet       äé"
		"ãí                          îäé"
		"ã6nationell teckenupps{ttningäé"
		"ã automatisk igenk{nning     äé"
		"ãí                          îäé"
		"ãí Sprache/language svenska îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã        Asetusvalikko       äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Suosikit: sivu 111 lis{{  äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2   Tekstikanavan valinta   äé"
		"ãí          search          îäé"
		"ã                            äé"
		"ã         N{ytt|tila         äé"
		"ã3 Vakiotila     16:9        äé"
		"ã                            äé"
		"ã4         Kirkkaus          äé"
		"ãí                          îäé"
		"ã5       L{pin{kyvyys        äé"
		"ãí                          îäé"
		"ã6   kansallinen merkist|    äé"
		"ã automaattinen tunnistus    äé"
		"ãí                          îäé"
		"ãí Kieli            suomi   îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã    Menu de Configuracao    äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Favoritos:  adi pag. 111  äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Seleccao Teletext     äé"
		"ãí         Procurar         îäé"
		"ã                            äé"
		"ã       formato ecran        äé"
		"ã3 Standard mode 16:9        äé"
		"ã                            äé"
		"ã4          Brilho           äé"
		"ãí                          îäé"
		"ã5      Transparencia        äé"
		"ãí                          îäé"
		"ã6  Caracteres nacionaist    äé"
		"ãreconhecimento utomatico    äé"
		"ãí                          îäé"
		"ãí Lingua      Portuguesa   îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	},
	/*     0000000000111111111122222222223 */
	/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã        Konfiguraciq        äé"
		"åææææææææææææææææææææææææææææçé"
		"ã1 Faworit&:   dob str. 111  äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     W&bor teleteksta      äé"
		"ãí           Poisk          îäé"
		"ã                            äé"
		"ã       Format kartinki      äé"
		"ã3 Stand. revim  16:9        äé"
		"ã                            äé"
		"ã4          Qrkostx          äé"
		"ãí                          îäé"
		"ã5       Prozra~nostx        äé"
		"ãí                          îäé"
		"ã6  Ispolxzuem&j alfawit     äé"
		"ã      awtoopredelenie       äé"
		"ãí                          îäé"
		"ãí  Qz&k:         Russkij   îäé"
		"åæ   www.tuxtxt.net  x.xx   æçé"
		"ëìììììììììììììììììììììììììììììê"
	}
};

const char catchmenutext[][87] =
{
	{
		"        íïðî w{hlen   ñò anzeigen       "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî select   ñò show           "
		"0000000011110000000000110000000000000000"
	},
	{
		"  íïðî selectionner   ñò montrer        "
		"0011110000000000000000110000000000000000"
	},
	{
		"        íïðî kiezen   ñò tonen          "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî epikoc^  ñò pqobok^        "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðîseleziona ñò mostra         "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî wybiez   ñò wyswietl       "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî v{lj     ñò visa           "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî valitse  ñò n{yt{          "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî seleccao ñò mostrar        "
		"0000000011110000000000110000000000000000"
	},
	{
		"        íïðî w&bratx  ñò pokazatx       "
		"0000000011110000000000110000000000000000"
	}
};


//const char message_3[][39] = {"Page not found"};
//const char message_3_blank[] = {};
const char message_3[][39] =
{
	{ " suche nach Teletext-Anbietern " },
	{ " searching for teletext Services " },
	{ " recherche des services teletext " },
	{ " zoeken naar teletekst aanbieders " },
	{ " amaf^tgsg voq]ym Teket]nt " },
	{ " attesa opzioni televideo " },
	{ " poszukiwanie sygna}u telegazety " },
	{ " s|ker efter TextTV tj{nster " },
	{ " etsit{{n Teksti-TV -palvelua " },
	{ " Procurar servicos de teletexto " },
	{ " W&polnqetsq poisk teleteksta " }
};
const char message_3_blank[] = "ã                                   äé";
//const char message_7[][39] = {};
//const char message_8[][39] = {};
const char message_7[][39] =
{
	{ "kein Teletext auf dem Transponder" },
	{ "no teletext on the transponder" },
	{ "pas de teletext sur le transponder" },
	{ "geen teletekst op de transponder" },
	{ "jal]la Teket]nt ston amaletadot^" },
	{ "nessun televideo sul trasponder" },
	{ "brak sygna}u na transponderze" },
	{ "ingen TextTV p} denna transponder" },
	{ "Ei Teksti-TV:t{ l{hettimell{" },
	{ "nao ha teletexto no transponder" },
	{ "Na transpondere net teleteksta" }
};
const char message_8[][39] =
{
	{ "warte auf Empfang von Seite 100" },
	{ "waiting for reception of page 100" },
	{ "attentre la réception de page 100" },
	{ "wachten op ontvangst van pagina 100" },
	{ "amal]my k^xg sek_dar 100" },
	{ "attesa ricezione pagina 100" },
	{ "oczekiwanie na stron` 100" },
	{ "v{ntar p} mottagning av sida 100" },
	{ "Odotetaan sivua 100" },
	{ "esperando recepcao na pag 100" },
	{ "Ovidanie priema stranic& 100" }
};

const char message8pagecolumn[] = /* last(!) column of page to show in each language */
{
	33, 34, 34, 35, 29, 30, 30, 34, 34
};

enum /* options for charset */
{
	C_G0P = 0, /* primary G0 */
	C_G0S, /* secondary G0 */
	C_G1C, /* G1 contiguous */
	C_G1S, /* G1 separate */
	C_G2,
	C_G3,
	C_OFFSET_DRCS = 32
	/* 32..47: 32+subpage# GDRCS (offset/20 in page_char) */
	/* 48..63: 48+subpage#  DRCS (offset/20 in page_char) */
};

/* struct for page attributes */
typedef struct
{
	unsigned char fg      :6; /* foreground color */
	unsigned char bg      :6; /* background color */
	unsigned char charset :6; /* see enum above */
	unsigned char doubleh :1; /* double height */
	unsigned char doublew :1; /* double width */
	/* ignore at Black Background Color Substitution */
	/* black background set by New Background ($1d) instead of start-of-row default or Black Backgr. ($1c) */
	/* or black background set by level 2.5 extensions */
	unsigned char IgnoreAtBlackBgSubst:1;
	unsigned char concealed:1; /* concealed information */
	unsigned char inverted :1; /* colors inverted */
	unsigned char flashing :5; /* flash mode */
	unsigned char diacrit  :4; /* diacritical mark */
	unsigned char underline:1; /* Text underlined */
	unsigned char boxwin   :1; /* Text boxed/windowed */
	unsigned char setX26   :1; /* Char is set by packet X/26 (no national subset used) */
	unsigned char setG0G2  :7; /* G0+G2 set designation  */
} tstPageAttr;

enum /* indices in atrtable */
{
	ATR_WB, /* white on black */
	ATR_PassiveDefault, /* Default for passive objects: white on black, ignore at Black Background Color Substitution */
	ATR_L250, /* line25 */
	ATR_L251, /* line25 */
	ATR_L252, /* line25 */
	ATR_L253, /* line25 */
	ATR_TOPMENU0, /* topmenu */
	ATR_TOPMENU1, /* topmenu */
	ATR_TOPMENU2, /* topmenu */
	ATR_TOPMENU3, /* topmenu */
	ATR_MSG0, /* message */
	ATR_MSG1, /* message */
	ATR_MSG2, /* message */
	ATR_MSG3, /* message */
	ATR_MSGDRM0, /* message */
	ATR_MSGDRM1, /* message */
	ATR_MSGDRM2, /* message */
	ATR_MSGDRM3, /* message */
	ATR_MENUHIL0, /* hilit menu line */
	ATR_MENUHIL1, /* hilit menu line */
	ATR_MENUHIL2, /* hilit menu line */
	ATR_MENU0, /* menu line */
	ATR_MENU1, /* menu line */
	ATR_MENU2, /* menu line */
	ATR_MENU3, /* menu line */
	ATR_MENU4, /* menu line */
	ATR_MENU5, /* menu line */
	ATR_MENU6, /* menu line */
	ATR_CATCHMENU0, /* catch menu line */
	ATR_CATCHMENU1 /* catch menu line */
};

/* define color names */
enum
{
	black = 0,
	red, /* 1 */
	green, /* 2 */
	yellow, /* 3 */
	blue,	/* 4 */
	magenta,	/* 5 */
	cyan,	/* 6 */
	white, /* 7 */
	menu1 = (4*8),
	menu2,
	menu3,
	transp,
	transp2,
	SIZECOLTABLE
};

//const (avoid warnings :<)
tstPageAttr atrtable[] =
{
	{ white  , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_WB */
	{ white  , black , C_G0P, 0, 0, 1 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_PassiveDefault */
	{ white  , red   , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L250 */
	{ black  , green , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L251 */
	{ black  , yellow, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L252 */
	{ white  , blue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L253 */
	{ magenta, black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU0 */
	{ green  , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU1 */
	{ yellow , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU2 */
	{ cyan   , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU3 */
	{ menu2  , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG0 */
	{ yellow , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG1 */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG2 */
	{ white  , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG3 */
	{ menu2  , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM0 */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM1 */
	{ menu2  , black , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM2 */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM3 */
	{ menu1  , blue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL0 5a Z */
	{ white  , blue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL1 58 X */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL2 9b \9B */
	{ menu2  , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU0 ab \AB */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU1 a4 \A4 */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU2 9b \9B */
	{ menu2  , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU3 cb \CB */
	{ cyan   , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU4 c7 \C7 */
	{ white  , menu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU5 c8 \C8 */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU6 a8 \A8 */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_CATCHMENU0 a4 \A4 */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}  /* ATR_CATCHMENU1 a8 \A8 */
};

/* buffers */
unsigned char  page_char[40 * 25];
tstPageAttr page_atrb[40 * 25];

/* colormap */
const unsigned short defaultcolors[] =	/* 0x0bgr */
{
	0x000, 0x00f, 0x0f0, 0x0ff, 0xf00, 0xf0f, 0xff0, 0xfff,
	0x000, 0x007, 0x070, 0x077, 0x700, 0x707, 0x770, 0x777,
	0x50f, 0x07f, 0x7f0, 0xbff, 0xac0, 0x005, 0x256, 0x77c,
	0x333, 0x77f, 0x7f7, 0x7ff, 0xf77, 0xf7f, 0xff7, 0xddd,
	0x420, 0x210, 0x420, 0x000, 0x000
};

/* 32bit colortable */
unsigned char bgra[][5] = { 
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xFF",
"\0\0\0\xFF", "\0\0\0\xFF", "\0\0\0\xC0", "\0\0\0\x00",
"\0\0\0\x33" };

/* tables for color table remapping, first entry (no remapping) skipped, offsets for color index */
const unsigned char MapTblFG[] = {  0,  0,  8,  8, 16, 16, 16 };
const unsigned char MapTblBG[] = {  8, 16,  8, 16,  8, 16, 24 };

/* shapes */
enum
{
	S_END = 0,
	S_FHL, /* full horizontal line: y-offset */
	S_FVL, /* full vertical line: x-offset */
	S_BOX, /* rectangle: x-offset, y-offset, width, height */
	S_TRA, /* trapez: x0, y0, l0, x1, y1, l1 */
	S_BTR, /* trapez in bgcolor: x0, y0, l0, x1, y1, l1 */
	S_INV, /* invert */
	S_LNK, /* call other shape: shapenumber */
	S_CHR, /* Character from freetype hibyte, lowbyte */
	S_ADT, /* Character 2F alternating raster */
	S_FLH, /* flip horizontal */
	S_FLV  /* flip vertical */
};

/* shape coordinates */
enum
{
	S_W13 = 5, /* width*1/3 */
	S_W12, /* width*1/2 */
	S_W23, /* width*2/3 */
	S_W11, /* width */
	S_WM3, /* width-3 */
	S_H13, /* height*1/3 */
	S_H12, /* height*1/2 */
	S_H23, /* height*2/3 */
	S_H11, /* height */
	S_NrShCoord
};

/* G3 characters */
unsigned char aG3_20[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_21[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_22[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_23[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_24[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_25[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_26[] = { S_INV, S_LNK, 0x66, S_END };
unsigned char aG3_27[] = { S_INV, S_LNK, 0x67, S_END };
unsigned char aG3_28[] = { S_INV, S_LNK, 0x68, S_END };
unsigned char aG3_29[] = { S_INV, S_LNK, 0x69, S_END };
unsigned char aG3_2a[] = { S_INV, S_LNK, 0x6a, S_END };
unsigned char aG3_2b[] = { S_INV, S_LNK, 0x6b, S_END };
unsigned char aG3_2c[] = { S_INV, S_LNK, 0x6c, S_END };
unsigned char aG3_2d[] = { S_INV, S_LNK, 0x6d, S_END };
unsigned char aG3_2e[] = { S_BOX, 2, 0, 3, S_H11, S_END };
unsigned char aG3_2f[] = { S_ADT };
unsigned char aG3_30[] = { S_LNK, 0x20, S_FLH, S_END };
unsigned char aG3_31[] = { S_LNK, 0x21, S_FLH, S_END };
unsigned char aG3_32[] = { S_LNK, 0x22, S_FLH, S_END };
unsigned char aG3_33[] = { S_LNK, 0x23, S_FLH, S_END };
unsigned char aG3_34[] = { S_LNK, 0x24, S_FLH, S_END };
unsigned char aG3_35[] = { S_LNK, 0x25, S_FLH, S_END };
unsigned char aG3_36[] = { S_INV, S_LNK, 0x76, S_END };
unsigned char aG3_37[] = { S_INV, S_LNK, 0x77, S_END };
unsigned char aG3_38[] = { S_INV, S_LNK, 0x78, S_END };
unsigned char aG3_39[] = { S_INV, S_LNK, 0x79, S_END };
unsigned char aG3_3a[] = { S_INV, S_LNK, 0x7a, S_END };
unsigned char aG3_3b[] = { S_INV, S_LNK, 0x7b, S_END };
unsigned char aG3_3c[] = { S_INV, S_LNK, 0x7c, S_END };
unsigned char aG3_3d[] = { S_INV, S_LNK, 0x7d, S_END };
unsigned char aG3_3e[] = { S_LNK, 0x2e, S_FLH, S_END };
unsigned char aG3_3f[] = { S_BOX, 0, 0, S_W11, S_H11, S_END };
unsigned char aG3_40[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_END };
unsigned char aG3_41[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_FLV, S_END };
unsigned char aG3_42[] = { S_LNK, 0x50, S_BOX, S_W12, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_43[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_44[] = { S_LNK, 0x48, S_FLV, S_LNK, 0x48, S_END };
unsigned char aG3_45[] = { S_LNK, 0x44, S_FLH, S_END };
unsigned char aG3_46[] = { S_LNK, 0x47, S_FLV, S_END };
unsigned char aG3_47[] = { S_LNK, 0x48, S_FLH, S_LNK, 0x48, S_END };
unsigned char aG3_48[] = { S_TRA, 0, 0, S_W23, 0, S_H23, 0, S_BTR, 0, 0, S_W13, 0, S_H13, 0, S_END };
unsigned char aG3_49[] = { S_LNK, 0x48, S_FLH, S_END };
unsigned char aG3_4a[] = { S_LNK, 0x48, S_FLV, S_END };
unsigned char aG3_4b[] = { S_LNK, 0x48, S_FLH, S_FLV, S_END };
unsigned char aG3_4c[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W11, S_H13, S_END };
unsigned char aG3_4d[] = { S_CHR, 0x25, 0xE6 };
unsigned char aG3_4e[] = { S_CHR, 0x25, 0xCF };
unsigned char aG3_4f[] = { S_CHR, 0x25, 0xCB };
unsigned char aG3_50[] = { S_BOX, S_W12, 0, 2, S_H11, S_FLH, S_BOX, S_W12, 0, 2, S_H11,S_END };
unsigned char aG3_51[] = { S_BOX, 0, S_H12, S_W11, 2, S_FLV, S_BOX, 0, S_H12, S_W11, 2,S_END };
unsigned char aG3_52[] = { S_LNK, 0x55, S_FLH, S_FLV, S_END };
unsigned char aG3_53[] = { S_LNK, 0x55, S_FLV, S_END };
unsigned char aG3_54[] = { S_LNK, 0x55, S_FLH, S_END };
unsigned char aG3_55[] = { S_LNK, 0x7e, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_56[] = { S_LNK, 0x57, S_FLH, S_END};
unsigned char aG3_57[] = { S_LNK, 0x55, S_LNK, 0x50 , S_END};
unsigned char aG3_58[] = { S_LNK, 0x59, S_FLV, S_END};
unsigned char aG3_59[] = { S_LNK, 0x7e, S_LNK, 0x51 , S_END};
unsigned char aG3_5a[] = { S_LNK, 0x50, S_LNK, 0x51 , S_END};
unsigned char aG3_5b[] = { S_CHR, 0x21, 0x92};
unsigned char aG3_5c[] = { S_CHR, 0x21, 0x90};
unsigned char aG3_5d[] = { S_CHR, 0x21, 0x91};
unsigned char aG3_5e[] = { S_CHR, 0x21, 0x93};
unsigned char aG3_5f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_60[] = { S_INV, S_LNK, 0x20, S_END };
unsigned char aG3_61[] = { S_INV, S_LNK, 0x21, S_END };
unsigned char aG3_62[] = { S_INV, S_LNK, 0x22, S_END };
unsigned char aG3_63[] = { S_INV, S_LNK, 0x23, S_END };
unsigned char aG3_64[] = { S_INV, S_LNK, 0x24, S_END };
unsigned char aG3_65[] = { S_INV, S_LNK, 0x25, S_END };
unsigned char aG3_66[] = { S_LNK, 0x20, S_FLV, S_END };
unsigned char aG3_67[] = { S_LNK, 0x21, S_FLV, S_END };
unsigned char aG3_68[] = { S_LNK, 0x22, S_FLV, S_END };
unsigned char aG3_69[] = { S_LNK, 0x23, S_FLV, S_END };
unsigned char aG3_6a[] = { S_LNK, 0x24, S_FLV, S_END };
unsigned char aG3_6b[] = { S_BOX, 0, 0, S_W11, S_H13, S_TRA, 0, S_H13, S_W11, 0, S_H23, 1, S_END };
unsigned char aG3_6c[] = { S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_FLV, S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_BOX, 0, S_H12, S_W12,1, S_END };
unsigned char aG3_6d[] = { S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_FLH, S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_END };
unsigned char aG3_6e[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_6f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_70[] = { S_INV, S_LNK, 0x30, S_END };
unsigned char aG3_71[] = { S_INV, S_LNK, 0x31, S_END };
unsigned char aG3_72[] = { S_INV, S_LNK, 0x32, S_END };
unsigned char aG3_73[] = { S_INV, S_LNK, 0x33, S_END };
unsigned char aG3_74[] = { S_INV, S_LNK, 0x34, S_END };
unsigned char aG3_75[] = { S_INV, S_LNK, 0x35, S_END };
unsigned char aG3_76[] = { S_LNK, 0x66, S_FLH, S_END };
unsigned char aG3_77[] = { S_LNK, 0x67, S_FLH, S_END };
unsigned char aG3_78[] = { S_LNK, 0x68, S_FLH, S_END };
unsigned char aG3_79[] = { S_LNK, 0x69, S_FLH, S_END };
unsigned char aG3_7a[] = { S_LNK, 0x6a, S_FLH, S_END };
unsigned char aG3_7b[] = { S_LNK, 0x6b, S_FLH, S_END };
unsigned char aG3_7c[] = { S_LNK, 0x6c, S_FLH, S_END };
unsigned char aG3_7d[] = { S_LNK, 0x6d, S_FLV, S_END };
unsigned char aG3_7e[] = { S_BOX, S_W12, 0, 2, S_H12, S_FLH, S_BOX, S_W12, 0, 2, S_H12, S_END };// help char, not printed directly (only by S_LNK)

unsigned char *aShapes[] =
{
	aG3_20, aG3_21, aG3_22, aG3_23, aG3_24, aG3_25, aG3_26, aG3_27, aG3_28, aG3_29, aG3_2a, aG3_2b, aG3_2c, aG3_2d, aG3_2e, aG3_2f,
	aG3_30, aG3_31, aG3_32, aG3_33, aG3_34, aG3_35, aG3_36, aG3_37, aG3_38, aG3_39, aG3_3a, aG3_3b, aG3_3c, aG3_3d, aG3_3e, aG3_3f,
	aG3_40, aG3_41, aG3_42, aG3_43, aG3_44, aG3_45, aG3_46, aG3_47, aG3_48, aG3_49, aG3_4a, aG3_4b, aG3_4c, aG3_4d, aG3_4e, aG3_4f,
	aG3_50, aG3_51, aG3_52, aG3_53, aG3_54, aG3_55, aG3_56, aG3_57, aG3_58, aG3_59, aG3_5a, aG3_5b, aG3_5c, aG3_5d, aG3_5e, aG3_5f,
	aG3_60, aG3_61, aG3_62, aG3_63, aG3_64, aG3_65, aG3_66, aG3_67, aG3_68, aG3_69, aG3_6a, aG3_6b, aG3_6c, aG3_6d, aG3_6e, aG3_6f,
	aG3_70, aG3_71, aG3_72, aG3_73, aG3_74, aG3_75, aG3_76, aG3_77, aG3_78, aG3_79, aG3_7a, aG3_7b, aG3_7c, aG3_7d, aG3_7e
};

//// functions
void ConfigMenu(int Init);
void CleanUp();
void PageInput(int Number);
void ColorKey(int);
void PageCatching();
void CatchNextPage(int, int);
void GetNextPageOne(int up);
void GetNextSubPage(int offset);
void SwitchZoomMode();
void SwitchScreenMode(int newscreenmode);
void SwitchTranspMode();
void SwitchHintMode();
void CreateLine25();
void CopyBB2FB();
void RenderCatchedPage();
void RenderCharFB(int Char, tstPageAttr *Attribute);
void RenderCharBB(int Char, tstPageAttr *Attribute);
void RenderMessage(int Message);
void RenderPage();
void DecodePage();
int  Init(bool isEplayer= false, int page = 0);
int  GetNationalSubset(const char* cc);
int  GetTeletextPIDs();
int  GetRCCode();
int  eval_triplet(int iOData, tstCachedPage *pstCachedPage,
						unsigned char *pAPx, unsigned char *pAPy,
						unsigned char *pAPx0, unsigned char *pAPy0,
						unsigned char *drcssubp, unsigned char *gdrcssubp,
						signed char *endcol, tstPageAttr *attrPassive, unsigned char* pagedata);
void eval_object(int iONr, tstCachedPage *pstCachedPage,
					  unsigned char *pAPx, unsigned char *pAPy,
					  unsigned char *pAPx0, unsigned char *pAPy0,
					  tObjType ObjType, unsigned char* pagedata);

#endif

