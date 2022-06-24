#ifndef _subtitle_123
#define _subtitle_123

/*
 * Interface File for subtitle handling (container input and output).
 *
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

#define DEFAULT_ASS_HEAD "[Script Info]\n\
Original Script: (c) 2008\n\
ScriptType: v4.00\n\
Synch Point: Side 1 0m00s\n\
Collisions: Normal\n\
Timer: 100.0000\n\n\
[V4 Styles]\n\
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding\n\
Style: Default,Arial,26,16777215,0,16777215,0,0,0,2,2,2,2,20,20,10,0\n\n\
[Events]\n\
Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n\n\n"

static inline unsigned char* text_to_ass(char *text, long long int pts, double duration)
{
	char buf[1024];
	int x,pos=0;
	for(x=0;x<strlen(text);x++)
	{
		if(text[x]=='\n')
		{
		    buf[pos++]='\\';
		    buf[pos++]='N';
		}
		else if(text[x]!='\r')
			buf[pos++]=text[x];
	}
	
	buf[pos++]='\0';
	int len = 80 + strlen(buf);
	long long int end_pts = pts + (duration * 1000.0);
	char* line = (char*)malloc( sizeof(char) * len );
	int sc = pts / 10;
	int ec = end_pts  / 10;
	int sh, sm, ss, eh, em, es;
	sh = sc/360000;  sc -= 360000*sh;
	sm = sc/  6000;  sc -=   6000*sm;
	ss = sc/   100;  sc -=    100*ss;
	eh = ec/360000;  ec -= 360000*eh;
	em = ec/  6000;  ec -=   6000*em;
	es = ec/   100;  ec -=    100*es;
	snprintf(line,len,"Dialogue: Marked=0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,Default,NTP,0000,0000,0000,!Effect,%s\n", sh, sm, ss, sc, eh, em, es, ec, buf);

	return (unsigned char*)line;

}

typedef enum
{
	eSub_Gfx,
	eSub_Txt
} SubType_t;


typedef struct
{
	unsigned char* data;
	int            len;
} SubText_t;

typedef struct
{
	unsigned char* data;
	unsigned int   Width;
	unsigned int   Height;
	unsigned int   Stride;
	unsigned int   x;
	unsigned int   y;
	unsigned int   color;
} SubGfx_t;

typedef struct
{
	SubType_t      type;
	long long int  pts;
	float          duration;
	
	union
	{
		SubText_t text;
		SubGfx_t  gfx;
	} u;
} SubtitleOut_t;

typedef struct
{
	unsigned char* data;
	int            len;

	unsigned char* extradata;
	int            extralen;
	
	long long int  pts;
	float          duration;
} SubtitleData_t;

typedef struct
{
	unsigned char* destination;
	unsigned int   screen_width;
	unsigned int   screen_height;
	unsigned int   destStride;
	
	int            shareFramebuffer;
	int            framebufferFD;
} SubtitleOutputDef_t;

#endif
