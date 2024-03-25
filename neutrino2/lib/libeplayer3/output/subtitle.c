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

/* Error Constants */
#define cERR_SUBTITLE_NO_ERROR         0
#define cERR_SUBTITLE_ERROR            -1

static const char FILENAME[] = "subtitle.c";

////
int min_x = 20;
int min_y = 700;
int max_x = 1240;
int max_y = 28;

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

//
uint32_t * resize32(uint8_t * origin, uint32_t * colors, int nb_colors, int ox, int oy, int dx, int dy)
{
	uint32_t  *cr, *l;
	int i, j, k, ip;

	cr = (uint32_t *) malloc(dx * dy * sizeof(uint32_t));

	if(cr == NULL) 
	{
		printf("Error: malloc\n");
		return NULL;
	}
	
	l = cr;

	for(j = 0; j < dy; j++, l += dx)
	{
		uint8_t * p = origin + (j*oy/dy*ox);
		
		for(i = 0, k = 0; i < dx; i++, k++) 
		{
			ip = i*ox/dx;
			
			int idx = p[ip];
			
			if(idx < nb_colors)
				l[k] = colors[idx];
		}
	}
	
	return(cr);
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
#define ASS_FONT DATADIR "/fonts/arial.ttf"

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

//// ass_write
static void ass_write(Context_t *context, SubtitleData_t* data) 
{
    	Writer_t* writer;
    
    	subtitle_printf(10, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}
    	
    	//
    	int first_kiss;

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

	//
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
                        		out.data          = (uint32_t*)img->bitmap;
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

/*
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
*/

//// bitmap_write
static void dvbsub_write(Context_t *context, SubtitleData_t* out) 
{
    	Writer_t* writer;
    
    	subtitle_printf(100, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}

	//
    	AVSubtitle sub;
    	memset(&sub, 0, sizeof(sub));
    	
    	out->packet->data = out->data;
    	out->packet->size = out->len;
    	out->packet->pts  = out->pts;
    		
    	//
    	const AVCodec *codec  = avcodec_find_decoder(out->avCodecId);
    		
    	int got_sub_ptr = 0;
    	
    	if (avcodec_open2(out->stream->codec, codec, NULL) != 0)
    	{
    		subtitle_err("error decoding subtitle\n");
	}
    		
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

		subtitle_printf(100, "format %d\n", sub.format);
		subtitle_printf(100, "start_display_time %d\n", sub.start_display_time);
		subtitle_printf(100, "end_display_time %d\n", sub.end_display_time);
		subtitle_printf(100, "num_rects %d\n", sub.num_rects);
		
		if (got_sub_ptr && sub.num_rects > 0)
		{			
			switch (sub.rects[0]->type)
			{
				case SUBTITLE_TEXT:
				case SUBTITLE_ASS:
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
					}
					break;
				}
				
				case SUBTITLE_BITMAP:
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

						subtitle_printf(100, "#%d at %d,%d size %dx%d colors %d (x=%d y=%d w=%d h=%d) \n", i+1, sub.rects[i]->x, sub.rects[i]->y, sub.rects[i]->w, sub.rects[i]->h, sub.rects[i]->nb_colors, xoff, yoff, nw, nh);

						// resize color to 32 bit
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 5, 0)
						uint32_t *newdata = resize32(sub.rects[i]->pict.data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#else
						uint32_t *newdata = resize32(sub.rects[i]->data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#endif
						WriterFBCallData_t out;
						
						// clear
						out.fd            = framebufferFD;
             					out.data          = NULL;
             					out.Width         = max_x;
             					out.Height        = max_y;
             					out.x             = min_x;
             					out.y             = min_y;
             					
             					out.color	  = 0;

             					out.Screen_Width  = screen_width; 
             					out.Screen_Height = screen_height; 
             					out.destination   = destination;
             					out.destStride    = destStride;

             					//writer->writeData(&out); //FIXME:
							
						// writeData
             					out.fd            = framebufferFD;
             					out.data          = newdata;
             					out.Width         = nw;
             					out.Height        = nh;
             					out.x             = xoff;
             					out.y             = yoff;
             					
             					out.color	  = 0;

             					out.Screen_Width  = screen_width; 
             					out.Screen_Height = screen_height; 
             					out.destination   = destination;
             					out.destStride    = destStride;

             					writer->writeData(&out);

						free(newdata);
						
						//
						if(min_x > xoff)
							min_x = xoff;

						if(min_y > yoff)
							min_y = yoff;

						if(max_x < nw)
							max_x = nw;

						if(max_y < nh)
							max_y = nh;
					} //for
					break;
				}
				
				default:
					break;
			}
		}
	}

    	subtitle_printf(100, "terminating\n");
}

static void teletext_write(Context_t *context, SubtitleData_t* out) 
{
    	Writer_t* writer;
    
    	subtitle_printf(10, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}
    	
    	// FIXME:
    	subtitle_printf(10, "%s\n", (char *)out->data);
 
 	subtitle_printf(10, "terminating\n");   	
}

static void text_write(Context_t *context, SubtitleData_t* out) 
{
    	Writer_t* writer;
    
    	subtitle_printf(100, "\n");

    	writer = getDefaultFramebufferWriter();

    	if (writer == NULL)
    	{
        	subtitle_err("no framebuffer writer found!\n");
    	}
    	
    	subtitle_printf(100, "%s\n", (char *)out->data);
    	
    	WriterFBCallData_t fb;

        fb.fd            = framebufferFD;
        fb.data          = (uint32_t*)out->data;
        fb.Width         = 1200;
        fb.Height        = 60;
        fb.x             = 40;
        fb.y             = 600;
             					
        fb.color	  = 0;

        fb.Screen_Width  = screen_width; 
        fb.Screen_Height = screen_height; 
        fb.destination   = destination;
        fb.destStride    = destStride;

        writer->writeReverseData(&fb);
    	
 
 	subtitle_printf(100, "terminating\n");   	
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
    	switch (out->avCodecId)
    	{
    		case AV_CODEC_ID_SSA:
    		case AV_CODEC_ID_ASS:
    		{
    			//
    			ass_write(context, out);
    			break;
    		}
    		
    		case AV_CODEC_ID_DVB_SUBTITLE:
    		{    		
    			dvbsub_write(context, out); 
    			break;
    		}
    	
    		case AV_CODEC_ID_DVB_TELETEXT:
    		{
    			teletext_write(context, out); 
    		
    			break;
    		}
    		
    		case AV_CODEC_ID_SUBRIP:
    		{
    			text_write(context, out);
    		}
    	
    		default:
    			break;
    	}

    	subtitle_printf(100, "<\n");

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

