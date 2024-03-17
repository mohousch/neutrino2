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

#include <ass/ass.h>

#include "common.h"
#include "output.h"
#include "subtitle.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define SUBTITLE_DEBUG

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

/* Error Constants */
#define cERR_SUBTITLE_NO_ERROR         0
#define cERR_SUBTITLE_ERROR            -1

static const char FILENAME[] = "subtitle.c";

/*
Number, Style, Name,, MarginL, MarginR, MarginV, Effect,, Text

1038,0,tdk,,0000,0000,0000,,That's not good.
1037,0,tdk,,0000,0000,0000,,{\i1}Rack them up, rack them up,{\i0}\N{\i1}rack them up.{\i0} [90]
1036,0,tdk,,0000,0000,0000,,Okay, rack them up.
*/

#define PUFFERSIZE 20

/* ***************************** */
/* Types                         */
/* ***************************** */

// ass
typedef struct ass_s 
{
	unsigned char* data;
	int            len;
	unsigned char* extradata;
	int            extralen;
    
	long long int  pts;
	float          duration;
} ass_t;

typedef struct region_s
{
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	time_t       undisplay;
    
	struct region_s* next;
} region_t;

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static pthread_mutex_t mutex;
static int isSubtitleOpened = 0;

// ass
static region_t* firstRegion = NULL;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

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

void replace_all(char ** string, char * search, char * replace) 
{
    int len = 0;
    char * ptr = NULL;
    char tempString[512];
    char newString[512];

    newString[0] = '\0';

    if ((string == NULL) || (*string == NULL) || (search == NULL) || (replace == NULL))
    {
        subtitle_err("null pointer passed\n");
        return;
    }
    
    strncpy(tempString, *string, 511);
    tempString[511] = '\0';

    free(*string);

    while ((ptr = strstr(tempString, search)) != NULL) 
    {
        len  = ptr - tempString;
        strncpy(newString, tempString, len);
        newString[len] = '\0';
        strcat(newString, replace);

        len += strlen(search);
        strcat(newString, tempString+len);

        strcpy(tempString, newString);
    }

    subtitle_printf(20, "strdup in line %d\n", __LINE__);

    if(newString[0] != '\0')
        *string = strdup(newString);
    else
        *string = strdup(tempString);
}

// ass
#define ASS_FONT "/usr/share/fonts/FreeSans.ttf"

static ASS_Library *ass_library;
static ASS_Renderer *ass_renderer;

static float ass_font_scale = 0.7;
static float ass_line_spacing = 0.7;

static ASS_Track* ass_track = NULL;

int ass_init(Context_t *context)
{
    int modefd;
    char buf[16];
    
    subtitle_printf(10, ">\n");

    ass_library = ass_library_init();

    if (!ass_library) 
    {
        subtitle_err("ass_library_init failed!\n");
        return cERR_SUBTITLE_ERROR;
    }   
    
    ass_set_extract_fonts( ass_library, 1 );
    ass_set_style_overrides( ass_library, NULL );
    
    ass_renderer = ass_renderer_init(ass_library);
    
    if (!ass_renderer) 
    {
        subtitle_err("ass_renderer_init failed!\n");

        if (ass_library)
            ass_library_done(ass_library);
        ass_library = NULL;

        return cERR_SUBTITLE_ERROR;
    }

    modefd = open("/proc/stb/video/3d_mode", O_RDWR);
    
    if(modefd > 0)
    {
        read(modefd, buf, 15);
        buf[15]='\0';
	close(modefd);
    }
    else 
    	threeDMode = 0;

    if(strncmp(buf,"sbs",3)==0)
    	threeDMode = 1;
    else if(strncmp(buf,"tab",3)==0)
    	threeDMode = 2;
    else 
    	threeDMode = 0;
    
    subtitle_printf(10, "width %d, height %d, share %d, fd %d, 3D %d\n", screen_width, screen_height, shareFramebuffer, framebufferFD, threeDMode);

    if(threeDMode == 0)
    {
        ass_set_frame_size(ass_renderer, screen_width, screen_height);
        ass_set_margins(ass_renderer, (int)(0.03 * screen_height), (int)(0.03 * screen_height) ,
                                      (int)(0.03 * screen_width ), (int)(0.03 * screen_width )  );
    }
    else if(threeDMode == 1)
    {
        ass_set_frame_size(ass_renderer, screen_width/2, screen_height);
        ass_set_margins(ass_renderer, (int)(0.03 * screen_height), (int)(0.03 * screen_height) ,
                                      (int)(0.03 * screen_width/2 ), (int)(0.03 * screen_width/2 )  );
    }
    else if(threeDMode == 2)
    {
        ass_set_frame_size(ass_renderer, screen_width, screen_height/2);
        ass_set_margins(ass_renderer, (int)(0.03 * screen_height/2), (int)(0.03 * screen_height/2) ,
                                      (int)(0.03 * screen_width ), (int)(0.03 * screen_width )  );
    }
    
    ass_set_use_margins(ass_renderer, 0 );
    ass_set_font_scale(ass_renderer, ass_font_scale);

    ass_set_hinting(ass_renderer, ASS_HINTING_LIGHT);
    ass_set_line_spacing(ass_renderer, ass_line_spacing);
    ass_set_fonts(ass_renderer, ASS_FONT, "Arial", 0, NULL, 1);

    if(threeDMode == 0)
    {
        ass_set_aspect_ratio( ass_renderer, 1.0, 1.0);
    }
    else if(threeDMode == 1)
    {
        ass_set_aspect_ratio( ass_renderer, 0.5, 1.0);
    }
    else if(threeDMode == 2)
    {
        ass_set_aspect_ratio( ass_renderer, 1.0, 0.5);
    }

    return cERR_SUBTITLE_NO_ERROR;
}

//
int process_ass_data(Context_t *context, SubtitleData_t* data)
{
    int first_kiss;
    
    subtitle_printf(20, ">\n");

    if (ass_track == NULL)
    {
        first_kiss = 1;
        ass_track = ass_new_track(ass_library);

        if (ass_track == NULL)
        {
            subtitle_err("error creating ass_track\n");
            return cERR_SUBTITLE_ERROR;
        }
    }

    if ((data->extradata) && (first_kiss))
    {
        subtitle_printf(20,"processing private %d bytes\n",data->extralen);
        ass_process_codec_private(ass_track, (char*) data->extradata, data->extralen);
        subtitle_printf(20,"processing private done\n");
    } 

    if (data->data)
    {
        subtitle_printf(20,"processing data %d bytes\n",data->len);
        ass_process_data(ass_track, (char*) data->data, data->len);
        subtitle_printf(20,"processing data done\n");
    }
    
    return cERR_SUBTITLE_NO_ERROR;
}

/* ********************************* */
/* Region Undisplay handling         */
/* ********************************* */

/* release and undisplay all saved regions
 */
void releaseRegions()
{
    region_t* next, *old;
    Writer_t* writer;
    
    if (firstRegion == NULL)
        return;

    writer = getDefaultFramebufferWriter();

    if (writer == NULL)
    {
        subtitle_err("no framebuffer writer found!\n");
    }

    next = firstRegion;
    while (next != NULL)
    {
        if (writer)
        {
             WriterFBCallData_t out;
                    
             subtitle_printf(100, "release: w %d h %d x %d y %d\n", next->w, next->h, next->x, next->y);

             out.fd            = framebufferFD;
             out.data          = NULL;
             out.Width         = next->w;
             out.Height        = next->h;
             out.x             = next->x;
             out.y             = next->y;

             out.Screen_Width  = screen_width; 
             out.Screen_Height = screen_height; 
             out.destination   = destination;
             out.destStride    = destStride;

             writer->writeData(&out);
             if(threeDMode == 1)
             {
                 out.x = screen_width/2 + next->x;
                 writer->writeData(&out);
             }
             else if(threeDMode == 2)
             {
                 out.y = screen_height/2 + next->y;
                 writer->writeData(&out);
             }
        }
        old  = next;         
        next = next->next;
        free(old);
    }
    
    firstRegion = NULL;  
}

/* check for regions which should be undisplayed. 
 * we are very tolerant on time here, because
 * regions are also released when new regions are
 * detected (see ETSI EN 300 743 Chapter Page Composition)
 */
void checkRegions()
{
#define cDeltaTime 2
    region_t* next, *old, *prev;
    Writer_t* writer;
    time_t now = time(NULL);
    
    if (firstRegion == NULL)
        return;

    writer = getDefaultFramebufferWriter();

    if (writer == NULL)
    {
        subtitle_err("no framebuffer writer found!\n");
    }

    prev = next = firstRegion;
    
    while (next != NULL)
    {
        if (now > next->undisplay + cDeltaTime)
        {
           subtitle_printf(100, "undisplay: %ld > %ld\n", now, next->undisplay + cDeltaTime);

           if (writer)
           {
                WriterFBCallData_t out;

                subtitle_printf(100, "release: w %d h %d x %d y %d\n", next->w, next->h, next->x, next->y);

                out.fd            = framebufferFD;
                out.data          = NULL;
                out.Width         = next->w;
                out.Height        = next->h;
                out.x             = next->x;
                out.y             = next->y;
                
                out.Screen_Width  = screen_width; 
                out.Screen_Height = screen_height; 
                out.destination   = destination;
                out.destStride    = destStride;

                writer->writeData(&out);
                if(threeDMode == 1)
                {
                    out.x = screen_width/2 + next->x;
                    writer->writeData(&out);
                }
                else if(threeDMode == 2)
                {
                    out.y = screen_height/2 + next->y;
                    writer->writeData(&out);
               }
           }
           
           old = next;
           next = prev->next = next->next;

           if (old == firstRegion)
               firstRegion = next;
           free(old);
        } 
        else
        {
            prev = next;
            next = next->next;
        }
    }
}

/* store a display region for later release */
void storeRegion(unsigned int x, unsigned int y, unsigned int w, unsigned int h, time_t undisplay)
{
    region_t* new;
    
    subtitle_printf(100, "%d %d %d %d %ld\n", x, y, w, h, undisplay);
    
    if (firstRegion == NULL)
    {
        firstRegion = malloc(sizeof(region_t));
        new = firstRegion;
    } 
    else
    {
        new = firstRegion;
        while (new->next != NULL)
            new = new->next;
    
        new->next = malloc(sizeof(region_t));
        new = new->next;
    }

    new->next      = NULL;
    new->x         = x;
    new->y         = y;
    new->w         = w;
    new->h         = h;
    new->undisplay = undisplay;
}

// ass_write
static void ass_write(Context_t *context) 
{
    	Writer_t* writer;
    
    	subtitle_printf(10, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}

	if (ass_track)
    	{
		ASS_Image *       img   = NULL;
            	int               change = 0;
            	unsigned long int playPts;

            	getMutex(__LINE__);
            
            	//FIXME: durch den sleep bleibt die cpu usage zw. 5 und 13%, ohne
            	//       steigt sie bei Verwendung von subtiteln bis auf 95%.
            	//       ich hoffe dadurch gehen keine subtitle verloren, wenn die playPts
            	//       durch den sleep verschlafen wird. Besser w�re es den n�chsten
            	//       subtitel zeitpunkt zu bestimmen und solange zu schlafen.
            	usleep(1000);

            	img = ass_render_frame(ass_renderer, ass_track, playPts / 90.0, &change);

            	subtitle_printf(10, "img %p pts %lu %f\n", img, playPts, playPts / 90.0);

            	if(img != NULL && ass_renderer && ass_track)
            	{
                	/* the spec says, that if a new set of regions is present
                 	* the complete display switches to the new state. So lets
                 	* release the old regions on display.
                 	*/
                	if (change != 0)
                    		releaseRegions();

                	while (context && context->playback && context->playback->isPlaying && (img) && (change != 0))
                	{
                    		WriterFBCallData_t out;
                    		time_t now = time(NULL);
                    		time_t undisplay = now + 10;

                    		if (ass_track && ass_track->events)
                    		{
                        		undisplay = now + ass_track->events->Duration / 1000 + 0.5;
                    		}

                    		subtitle_printf(10, "w %d h %d s %d x %d y %d c %d chg %d now %ld und %ld\n", 
                                 img->w, img->h, img->stride, 
                                 img->dst_x, img->dst_y, img->color, 
                                 change, now, undisplay);

                    		/* api docu said w and h can be zero which
                     		* means image should not be rendered
                     		*/
                    		if ((img->w != 0) && (img->h != 0) && (writer)) 
                    		{
                        		out.fd            = framebufferFD;
                        		out.data          = img->bitmap;
                        		out.Width         = img->w;
                        		out.Height        = img->h;
                        		out.Stride        = img->stride;
                        		out.x             = img->dst_x;
                        		out.y             = img->dst_y;
                        		out.color         = img->color;
                        
                        		out.Screen_Width  = screen_width; 
                        		out.Screen_Height = screen_height; 
                        		out.destination   = destination;
                        		out.destStride    = destStride;

                        		storeRegion(img->dst_x, img->dst_y, img->w, img->h, undisplay);
                                    
                        		if (shareFramebuffer)
                        		{
                            			if(context && context->playback && context->playback->isPlaying && writer)
                            			{
                                			writer->writeData(&out);
                                
                                			if(threeDMode == 1)
                                			{
			            				out.x = screen_width/2 + img->dst_x;
			            				writer->writeData(&out);
                                			}
                                			else if(threeDMode == 2)
                                			{
                                    				out.y = screen_height/2 + img->dst_y;
                                    				writer->writeData(&out);
                                			}
                            			}
                        		}
                    		}

                    		/* Next image */
                    		img = img->next;
                	}
            	}
            	else
            	{
               		/* noop */
            	}

            	releaseMutex(__LINE__);
        } 
        else
        {
            usleep(1000);
        }
        
        /* cleanup no longer used but not overwritten regions */
        checkRegions();

    	subtitle_printf(10, "terminating\n");
}

// 
static int ass_stop(Context_t *context) 
{
    int ret = cERR_SUBTITLE_NO_ERROR;
    int wait_time = 20;
    Writer_t* writer;

    subtitle_printf(10, "\n");

    getMutex(__LINE__);

    releaseRegions();

    if (ass_track)
        ass_free_track(ass_track);

    ass_track = NULL;

    if (ass_renderer)
        ass_renderer_done(ass_renderer);
    ass_renderer = NULL;

    if (ass_library)
        ass_library_done(ass_library);
    ass_library = NULL;

    writer = getDefaultFramebufferWriter();

    if (writer != NULL)
    {
        writer->reset();
    }

    releaseMutex(__LINE__);

    subtitle_printf(10, "ret %d\n", ret);
    
    return ret;
}

//
static void dvbsub_write(Context_t *context, SubtitleData_t* out) 
{
    	Writer_t* writer;
    
    	subtitle_printf(20, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}

	//
    	AVSubtitle sub;
    	memset(&sub, 0, sizeof(sub));

 //   	AVPacket packet;
 //   	av_init_packet(&packet);
    	
    	out->packet->data = out->data;
    	out->packet->size = out->len;
    	out->packet->pts  = out->pts;
    		
    	//
    	const AVCodec *codec;
    		
    	codec = avcodec_find_decoder(out->avCodecId);
 //   	AVCodecContext *c = avcodec_alloc_context3(codec);
    		
    	int got_sub_ptr = 0;
    		
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)			   
	if (out->stream->codec && avcodec_decode_subtitle2(out->stream->codec, &sub, &got_sub_ptr, out->packet) < 0)
#else
	if (out->stream->codec && avcodec_decode_subtitle(out->stream->codec, &sub, &got_sub_ptr, out->packet->data, out->packet->size ) < 0)
#endif
	{
		subtitle_err("error decoding subtitle\n");
	} 
	else
	{
		int i;

		subtitle_printf(10, "format %d\n", sub.format);
		subtitle_printf(10, "start_display_time %d\n", sub.start_display_time);
		subtitle_printf(10, "end_display_time %d\n", sub.end_display_time);
		subtitle_printf(10, "num_rects %d\n", sub.num_rects);
								
		for (i = 0; i < sub.num_rects; i++)
		{

			subtitle_printf(10, "x %d\n", sub.rects[i]->x);
			subtitle_printf(10, "y %d\n", sub.rects[i]->y);
			subtitle_printf(10, "w %d\n", sub.rects[i]->w);
			subtitle_printf(10, "h %d\n", sub.rects[i]->h);
			subtitle_printf(10, "nb_colors %d\n", sub.rects[i]->nb_colors);
			subtitle_printf(10, "type %d\n", sub.rects[i]->type);
			subtitle_printf(10, "text %s\n", sub.rects[i]->text);
			subtitle_printf(10, "ass %s\n", sub.rects[i]->ass);
											
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,0,100)
			uint32_t *colors = (uint32_t *) sub.rects[i]->pict.data[1];
#else
			uint32_t *colors = (uint32_t *) sub.rects[i]->data[1];
#endif
			int width = sub.rects[i]->w;
			int height = sub.rects[i]->h;

			int h2 = (width == 1280) ? 720 : 576;
				
			int xoff = sub.rects[i]->x * 1280 / width;
			int yoff = sub.rects[i]->y * 720 / h2;
			int nw = width * 1280 / width;
			int nh = height * 720 / h2;

			subtitle_printf(10, "#%d at %d,%d size %dx%d colors %d (x=%d y=%d w=%d h=%d) \n", i+1, sub.rects[i]->x, sub.rects[i]->y, sub.rects[i]->w, sub.rects[i]->h, sub.rects[i]->nb_colors, xoff, yoff, nw, nh);

			// resize color to 32 bit
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 5, 0)
			uint32_t *newdata = simple_resize32(sub.rects[i]->pict.data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#else
			uint32_t *newdata = simple_resize32(sub.rects[i]->data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#endif
				
			// blit2fb
			blit2FB(newdata, nw, nh, xoff, yoff, 0, 0, true);

			blit(framebufferFD);

			free(newdata);
		} //for
	}

    	subtitle_printf(10, "terminating\n");
}

/* ***************************** */
/* Functions                     */
/* ***************************** */

static int Write(void* _context, void *data) 
{
    Context_t * context = (Context_t  *) _context;
    SubtitleData_t * out;
    
    subtitle_printf(10, "\n");

    if (data == NULL)
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    out = (SubtitleData_t*) data;
    
    switch (out->avCodecId)
    {
    	case AV_CODEC_ID_SSA:
    	case AV_CODEC_ID_ASS:
    	case AV_CODEC_ID_SUBRIP: // FIXME:
    	{
    		subtitle_printf(10, "Write: S_TEXT/ASS: %s\n", (char *)out->data);
    	
    		//
    		process_ass_data(context, data);
    	
    		//
    		ass_write(context);
    		break;
    	}
    		
    	case AV_CODEC_ID_DVB_SUBTITLE:
    	{
    		subtitle_printf(10, "Write: S_GRAPHIC/DVB: %s\n", (char *)out->data);
    		
    		dvbsub_write(context, out); //FIXME: segfault
    		break;
    	}
    	
    	case AV_CODEC_ID_DVB_TELETEXT:
    	{
    		subtitle_printf(10, "Write: S_TEXT/TELETEXT: %s\n", (char *)out->data);
    		break;
    	}
    	
    	default:
    		break;
    }

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Open(context) 
{
    int i;

    subtitle_printf(10, "\n");

    if (isSubtitleOpened == 1)
    {
        subtitle_err("already opened! ignoring\n");
        return cERR_SUBTITLE_ERROR;
    }

    getMutex(__LINE__);
    
    // ass
    ass_init(context);

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
    
    //
     if (ass_track)
        ass_free_track(ass_track);

    ass_track = NULL;

    isSubtitleOpened = 0;

    releaseMutex(__LINE__);

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Stop(context) 
{
    int wait_time = 20;
    int i;
    Writer_t* writer;
    
    subtitle_printf(10, "\n");

    getMutex(__LINE__);
    
    // ass
    releaseRegions();

    if (ass_track)
        ass_free_track(ass_track);

    ass_track = NULL;

    if (ass_renderer)
        ass_renderer_done(ass_renderer);
    ass_renderer = NULL;

    if (ass_library)
        ass_library_done(ass_library);
    ass_library = NULL;
    
    writer = getDefaultFramebufferWriter();

    if (writer != NULL)
    {
        writer->reset();
    }

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
		ret = subtitle_Stop(context);
		break;
	    }
	    
	    case OUTPUT_SWITCH: 
	    {
		ret = subtitle_Stop(context);
		break;
	    }
	    
	    case OUTPUT_SET_SUBTITLE_OUTPUT: 
	    {
		SubtitleOutputDef_t* out = (SubtitleOutputDef_t*)argument;
		
		screen_width = out->screen_width;
		screen_height = out->screen_height;
		shareFramebuffer = out->shareFramebuffer;
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

