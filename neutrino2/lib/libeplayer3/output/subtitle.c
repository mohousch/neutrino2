/*
 * Subtitle output to one registered client.
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
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>

#include "common.h"
#include "output.h"
#include "writer.h"

#include <config.h>


/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define SUBTITLE_DEBUG
#define SUBTITLE_SILENT

#ifdef SUBTITLE_DEBUG

static short debug_level = 10;

#define subtitle_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define subtitle_printf(level, fmt, x...)
#endif

#ifndef SUBTITLE_SILENT
#define subtitle_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define subtitle_err(fmt, x...)
#endif

//// Error Constants
#define cERR_SUBTITLE_NO_ERROR         0
#define cERR_SUBTITLE_ERROR            -1

static const char FILENAME[] = "subtitle.c";

////
int min_x = 0;
int min_y = 0;
int max_x = 0;
int max_y = 0;

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static pthread_mutex_t mutex;
static int isSubtitleOpened = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */
extern uint32_t * simple_resize32(uint8_t * origin, uint32_t * colors, int nb_colors, int ox, int oy, int dx, int dy);
extern void teletext_write(int pid, uint8_t *data, int size);
extern void clearFrameBuffer(void);
extern void writeText(uint8_t* text, int x, int y, int w, int h);

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
static void getMutex(int line) 
{
    subtitle_printf(100, "%d requesting mutex\n", line);

    pthread_mutex_lock(&mutex);

    subtitle_printf(100, "%d received mutex\n", line);
}

static void releaseMutex(int line) 
{
    pthread_mutex_unlock(&mutex);

    subtitle_printf(100, "%d released mutex\n", line);
}

//
static char * ass_get_text(char *str)
{
    	// Events are stored in the Block in this order:
    	// ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    	// 91,0,Default,,0,0,0,,maar hij smaakt vast tof.
    	int i = 0;
    	char *p_str = str;
    	
    	while(i < 8 && *p_str != '\0')
    	{
        	if (*p_str == ',')
            		i++;
        	p_str++;
    	}
    	
    	// standardize hard break: '\N' -> '\n'
    	// http://docs.aegisub.org/3.2/ASS_Tags/
    	char *p_newline = NULL;
    	
    	while((p_newline = strstr(p_str, "\\N")) != NULL)
        	*(p_newline + 1) = 'n';
        	
    	return p_str;
}

/* ***************************** */
/* Functions                     */
/* ***************************** */
static int Write(void* _context, void *data) 
{
    	Context_t * context = (Context_t  *) _context;
    	SubtitleData_t * out;
    
    	subtitle_printf(100, "\n");

	if (data == NULL)
	{
		subtitle_err("null pointer passed\n");
		return cERR_SUBTITLE_ERROR;
	}

    	out = (SubtitleData_t*) data;
    	
	//
    	Writer_t* writer = NULL;
    	WriterFBCallData_t fb;

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
        	return cERR_SUBTITLE_ERROR;
    	}
    	
    	if (out->stream->codec->codec_id == AV_CODEC_ID_DVB_TELETEXT)
    	{
    		if (out->data && out->len > 1)
    			teletext_write(0, out->data + 1, out->len + 1);
    	}
    	else
    	{
		//
	    	AVSubtitle sub;
	    	AVCodecContext* ctx = out->stream->codec;
	    	
	    	memset(&sub, 0, sizeof(sub));
	    		
	    	int got_sub_ptr = 0;
	    	
	    	// decode 	
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)			   
		if (ctx && avcodec_decode_subtitle2(ctx, &sub, &got_sub_ptr, out->packet) < 0)
	#else
		if (ctx && avcodec_decode_subtitle(ctx, &sub, &got_sub_ptr, out->packet->data, out->packet->size ) < 0)
	#endif
		{
			subtitle_err("error decoding subtitle\n");
			return cERR_SUBTITLE_ERROR;
		} 
		else
		{
			int i;

			subtitle_printf(100, "format %d\n", sub.format);
			subtitle_printf(100, "start_display_time %d\n", sub.start_display_time);
			subtitle_printf(100, "end_display_time %d\n", sub.end_display_time);
			subtitle_printf(100, "num_rects %d\n", sub.num_rects);
			
			if (got_sub_ptr && sub.num_rects > 0)
			{
				subtitle_printf(100, "type: %d\n", sub.rects[0]->type);
				
				clearFrameBuffer();
							
				switch (sub.rects[0]->type)
				{
					case SUBTITLE_TEXT: // 2
					{
						for (i = 0; i < sub.num_rects; i++)
						{
							subtitle_printf(100, "x %d\n", sub.rects[i]->x);
							subtitle_printf(100, "y %d\n", sub.rects[i]->y);
							subtitle_printf(100, "w %d\n", sub.rects[i]->w);
							subtitle_printf(100, "h %d\n", sub.rects[i]->h);
							subtitle_printf(100, "nb_colors %d\n", sub.rects[i]->nb_colors);
							subtitle_printf(100, "type %d\n", sub.rects[i]->type);
							subtitle_printf(100, "text %s\n", sub.rects[i]->text);
							subtitle_printf(100, "ass %s\n", sub.rects[i]->ass);
							
		     					writeText((uint8_t*)sub.rects[i]->text, screen_x + 40, screen_y + screen_height - 80, screen_width - 80, 60);
						}
						break;
					}
					
					case SUBTITLE_ASS: // 3
					{
						for (i = 0; i < sub.num_rects; i++)
						{
							subtitle_printf(100, "x %d\n", sub.rects[i]->x);
							subtitle_printf(100, "y %d\n", sub.rects[i]->y);
							subtitle_printf(100, "w %d\n", sub.rects[i]->w);
							subtitle_printf(100, "h %d\n", sub.rects[i]->h);
							subtitle_printf(100, "nb_colors %d\n", sub.rects[i]->nb_colors);
							subtitle_printf(100, "type %d\n", sub.rects[i]->type);
							subtitle_printf(100, "text %s\n", sub.rects[i]->text);
							subtitle_printf(100, "ass %s\n", sub.rects[i]->ass);
							
		     					writeText((uint8_t*)ass_get_text(sub.rects[i]->ass), screen_x + 40, screen_y + screen_height - 80, screen_width - 80, 60);
						}
						break;
					}
					
					case SUBTITLE_BITMAP: // 1: DVB / TELETEXT / PGS
					{
						for (i = 0; i < sub.num_rects; i++)
						{
							subtitle_printf(100, "i %d\n", i);
							subtitle_printf(100, "x %d\n", sub.rects[i]->x);
							subtitle_printf(100, "y %d\n", sub.rects[i]->y);
							subtitle_printf(100, "w %d\n", sub.rects[i]->w);
							subtitle_printf(100, "h %d\n", sub.rects[i]->h);
							subtitle_printf(100, "nb_colors %d\n", sub.rects[i]->nb_colors);
							subtitle_printf(100, "type %d\n", sub.rects[i]->type);
							subtitle_printf(100, "text %s\n", sub.rects[i]->text);
							subtitle_printf(100, "ass %s\n", sub.rects[i]->ass);
							subtitle_printf(100, "pic %p\n", sub.rects[i]->data[0]);
							subtitle_printf(100, "colors %p\n\n", sub.rects[i]->data[1]);
							
							if (sub.rects[i]->nb_colors == 16) // DVB
							{
								int width = sub.rects[i]->w;
								int height = sub.rects[i]->h;

								int h2 = screen_height;
										
								int xoff = sub.rects[i]->x * screen_width / width;
								int yoff = sub.rects[i]->y * screen_height / h2;
								int nw = width * screen_width / width;
								int nh = height * screen_height / h2;

								// resize color to 32 bit
	#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 5, 0)
								uint32_t* newdata = simple_resize32(sub.rects[i]->pict.data[0], (uint32_t*)sub.rects[i]->pict.data[1], sub.rects[i]->nb_colors, width, height, nw, nh);
	#else
								uint32_t* newdata = simple_resize32(sub.rects[i]->data[0], (uint32_t*)sub.rects[i]->data[1], sub.rects[i]->nb_colors, width, height, nw, nh);
	#endif
										
								// writeData
				     				fb.fd            = framebufferFD;
				     				fb.data          = (uint8_t*)newdata;
				     				fb.Width         = nw;
				     				fb.Height        = nh;
				     				fb.x             = xoff;
				     				fb.y             = yoff;
				     					
				     				fb.color	  = 0;

				     				fb.Screen_Width  = screen_width; 
				     				fb.Screen_Height = screen_height; 
				     				fb.destination   = destination;
				     				fb.destStride    = destStride;

				     				writer->writeData(&fb);

								free(newdata);
							}
							/*
							else if (sub.rects[i]->nb_colors == 40)// TELETXT
							{
								teletext_write(0, out->data + 1, out->len + 1);
							}
							*/
							else // PGS (256 nb_colors)
							{
								int width = sub.rects[i]->w;
								int height = sub.rects[i]->h;

								int h2 = height;
										
								int xoff = screen_x; //sub.rects[i]->x * 1280 / width;
								int yoff = screen_y + screen_height - 80; //sub.rects[i]->y * 720 / h2;
								int nw = width * screen_width / width;
								int nh = 60; //height * screen_height / h2;

								// resize color to 32 bit
	#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 5, 0)
								uint32_t* newdata = simple_resize32(sub.rects[i]->pict.data[0], (uint32_t*)sub.rects[i]->pict.data[1], sub.rects[i]->nb_colors, width, height, nw, nh);
	#else
								uint32_t* newdata = simple_resize32(sub.rects[i]->data[0], (uint32_t*)sub.rects[i]->data[1], sub.rects[i]->nb_colors, width, height, nw, nh);
	#endif
										
								// writeData
				     				fb.fd            = framebufferFD;
				     				fb.data          = (uint8_t*)newdata;
				     				fb.Width         = nw;
				     				fb.Height        = nh;
				     				fb.x             = xoff;
				     				fb.y             = yoff;
				     					
				     				fb.color	  = 0;

				     				fb.Screen_Width  = screen_width; 
				     				fb.Screen_Height = screen_height; 
				     				fb.destination   = destination;
				     				fb.destStride    = destStride;

				     				writer->writeData(&fb);

								free(newdata);
							}
						} 
						break;
					}
					
					default:
						break;
				}
			}
		}
	}

    	subtitle_printf(100, "terminating\n");
    	
    	return cERR_SUBTITLE_NO_ERROR;
}

//
static int subtitle_Open(Context_t* context) 
{
    	subtitle_printf(10, "\n");

    	if (isSubtitleOpened == 1)
    	{
        	subtitle_err("already opened! ignoring\n");
        	return cERR_SUBTITLE_ERROR;
    	}

    	getMutex(__LINE__);

    	isSubtitleOpened = 1;

    	releaseMutex(__LINE__);

    	subtitle_printf(10, "<\n");

    	return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Close(Context_t* context) 
{
    	int i;

    	subtitle_printf(10, "\n");

    	getMutex(__LINE__);

    	isSubtitleOpened = 0;

    	releaseMutex(__LINE__);

    	subtitle_printf(10, "<\n");

    	return cERR_SUBTITLE_NO_ERROR;
}

static int Command(void  *_context, OutputCmd_t command, void * argument) 
{
    Context_t  *context = (Context_t*) _context;
    int ret = cERR_SUBTITLE_NO_ERROR;

    subtitle_printf(50, "%d\n", command);

    switch(command) 
    {	
	    case OUTPUT_OPEN: 
	    {
		ret = subtitle_Open(context);
		break;
	    }
	    
	    case OUTPUT_CLOSE: 
	    {
		ret = subtitle_Close(context);
		break;
	    }
	    
	    case OUTPUT_PLAY: 
	    {
		break;
	    }
	    
	    case OUTPUT_STOP: 
	    {
		break;
	    }
	    
	    case OUTPUT_SWITCH: 
	    {
		break;
	    }
	    
	    case OUTPUT_SET_SUBTITLE_OUTPUT: 
	    {
		SubtitleOutputDef_t* out = (SubtitleOutputDef_t*)argument;
		
		screen_x = out->screen_x;
		screen_y = out->screen_y;
		screen_width = out->screen_width;
		screen_height = out->screen_height;
		framebufferFD = out->framebufferFD;
		destination = out->destination;
		destStride = out->destStride;
		break;
	    }
	    
	    case OUTPUT_FLUSH: 
	    {
		subtitle_err("Subtitle Flush not implemented\n");
		ret = cERR_SUBTITLE_ERROR;
		break;
	    }
	    
	    case OUTPUT_CLEAR: 
	    {
		subtitle_err("Subtitle Clear not implemented\n");
		ret = cERR_SUBTITLE_ERROR;
		break;
	    }
	    
	    case OUTPUT_PAUSE: 
	    {
		subtitle_err("Subtitle Pause not implemented\n");
		ret = cERR_SUBTITLE_ERROR;
	    	break;
	    }
	    
	    case OUTPUT_CONTINUE: 
	    {
		subtitle_err("Subtitle Continue not implemented\n");
		ret = cERR_SUBTITLE_ERROR;
	    	break;
	    }

	    default:
		subtitle_err("OutputCmd %d not supported!\n", command);
		ret = cERR_SUBTITLE_ERROR;
		break;
    }

    subtitle_printf(50, "exiting with value %d\n", ret);

    return ret;
}


static char *SubtitleCapabilitis[] = 
{ 
	"subtitle", 
	NULL 
};

struct Output_s SubtitleOutput = {
    "Subtitle",
    &Command,
    &Write,
    SubtitleCapabilitis,

};

