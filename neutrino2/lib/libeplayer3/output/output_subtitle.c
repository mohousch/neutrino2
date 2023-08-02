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

struct sub_t 
{
    char *                 text;
    unsigned long long int pts;
    unsigned long int      milliDuration;
};

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

static pthread_t thread_sub;

void* clientData = NULL;
void  (*clientFunction) (long int, size_t, char *, void *);

static struct sub_t subPuffer[PUFFERSIZE];
static int readPointer = 0;
static int writePointer = 0;
static int hasThreadStarted = 0;
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

                subtitle_printf(100, "release: w %d h %d x %d y %d\n", 
                                    next->w, next->h, next->x, next->y);

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

    while ( context && context->playback && context->playback->isPlaying ) 
    {
        //IF MOVIE IS PAUSED, WAIT
        if (context->playback->isPaused) 
        {
            subtitle_printf(20, "paused\n");

            usleep(100000);
            continue;
        }

        if (context->playback->isSeeking) 
        {
            subtitle_printf(10, "seeking\n");

            usleep(100000);
            continue;
        }

        if (ass_track)
        {
            ASS_Image *       img   = NULL;
            int               change = 0;
            unsigned long int playPts;
            
            if (context && context->playback)
            {
                if (context->playback->Command(context, PLAYBACK_PTS, &playPts) < 0)
                    continue;
            }

            getMutex(__LINE__);
            
            //FIXME: durch den sleep bleibt die cpu usage zw. 5 und 13%, ohne
            //       steigt sie bei Verwendung von subtiteln bis auf 95%.
            //       ich hoffe dadurch gehen keine subtitle verloren, wenn die playPts
            //       durch den sleep verschlafen wird. Besser w�re es den n�chsten
            //       subtitel zeitpunkt zu bestimmen und solange zu schlafen.
            usleep(1000);

            img = ass_render_frame(ass_renderer, ass_track, playPts / 90.0, &change);

            subtitle_printf(150, "img %p pts %lu %f\n", img, playPts, playPts / 90.0);

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

                    subtitle_printf(100, "w %d h %d s %d x %d y %d c %d chg %d now %ld und %ld\n", 
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
    } /* while */

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

    //releaseRegions();

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
int subtitle_ParseASS (char **Line) 
{
    char* Text;
    int   i;
    char* ptr1;

    if ((Line == NULL) || (*Line == NULL))
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }
    
    Text = strdup(*Line);

    subtitle_printf(10, "-> Text = %s\n", *Line);

    ptr1 = Text;
    
    for (i=0; i < 9 && *ptr1 != '\0'; ptr1++) 
    {
        subtitle_printf(20, "%s",ptr1);

        if (*ptr1 == ',')
            i++;
    }

    free(*Line);

    *Line = strdup(ptr1);
    free(Text);

    replace_all(Line, "\\N", "\n");

    replace_all(Line, "{\\i1}", "<i>");
    replace_all(Line, "{\\i0}", "</i>");

    subtitle_printf(10, "<- Text=%s\n", *Line);

    return cERR_SUBTITLE_NO_ERROR;
}

int subtitle_ParseSRT (char **Line) 
{
    if ((Line == NULL) || (*Line == NULL))
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    subtitle_printf(20, "-> Text=%s\n", *Line);

    replace_all(Line, "\x0d", "");
    replace_all(Line, "\n\n", "\\N");
    replace_all(Line, "\n", "");
    replace_all(Line, "\\N", "\n");
    replace_all(Line, "�", "oe");
    replace_all(Line, "�", "ae");
    replace_all(Line, "�", "ue");
    replace_all(Line, "�", "Oe");
    replace_all(Line, "�", "Ae");
    replace_all(Line, "�", "Ue");
    replace_all(Line, "�", "ss");

    subtitle_printf(10, "<- Text=%s\n", *Line);

    return cERR_SUBTITLE_NO_ERROR;
}

int subtitle_ParseSSA (char **Line) 
{
    if ((Line == NULL) || (*Line == NULL))
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    subtitle_printf(20, "-> Text=%s\n", *Line);

    replace_all(Line, "\x0d", "");
    replace_all(Line, "\n\n", "\\N");
    replace_all(Line, "\n", "");
    replace_all(Line, "\\N", "\n");
    replace_all(Line, "�", "oe");
    replace_all(Line, "�", "ae");
    replace_all(Line, "�", "ue");
    replace_all(Line, "�", "Oe");
    replace_all(Line, "�", "Ae");
    replace_all(Line, "�", "Ue");
    replace_all(Line, "�", "ss");

    subtitle_printf(10, "<- Text=%s\n", *Line);

    return cERR_SUBTITLE_NO_ERROR;
}

void addSub(Context_t *context, char * text, unsigned long long int pts, unsigned long int milliDuration) 
{
    int count = 20;
    
    subtitle_printf(50, "index %d\n", writePointer);

    if(context && context->playback && !context->playback->isPlaying)
    {
        subtitle_err("1. aborting ->no playback\n");
        return;
    }
    
    if (text == NULL)
    {
        subtitle_err("null pointer passed\n");
        return;
    }

    if (pts == 0)
    {
        subtitle_err("pts 0\n");
        return;
    }

    if (milliDuration == 0)
    {
        subtitle_err("duration 0\n");
        return;
    }
    
    while (subPuffer[writePointer].text != NULL) 
    {
        //List is full, wait till we got some free space
        if(context && context->playback && !context->playback->isPlaying)
        {
            subtitle_err("2. aborting ->no playback\n");
            return;
        }

/* konfetti: we dont want to block forever here. if no buffer
 * is available we start ring from the beginning and loose some stuff
 * which is acceptable!
 */
        subtitle_printf(10, "waiting on free buffer %d - %d (%d) ...\n", writePointer, readPointer, count);
        usleep(10000);
        count--;
        
        if (count == 0)
        {
            subtitle_err("abort waiting on buffer...\n");
            break;
        }
    }
    
    subtitle_printf(20, "from mkv: %s pts:%lld milliDuration:%lud\n",text,pts,milliDuration);

    getMutex(__LINE__);

    if (count == 0)
    {
        int i;
        subtitle_err("freeing not delivered data\n");
        
        //Reset all
        readPointer = 0;
        writePointer = 0;

        for (i = 0; i < PUFFERSIZE; i++) 
        {
            if (subPuffer[i].text != NULL)
               free(subPuffer[i].text);
               
            subPuffer[i].text          = NULL;
            subPuffer[i].pts           = 0;
            subPuffer[i].milliDuration = 0;
        }
    }

    subPuffer[writePointer].text = strdup(text);
    subPuffer[writePointer].pts = pts;
    subPuffer[writePointer].milliDuration = milliDuration;

    writePointer++;
    
    if (writePointer == PUFFERSIZE)
        writePointer = 0;

    if (writePointer == readPointer)
    {
        /* this should not happen, and means that there is nor reader or
         * the reader has performance probs ;)
         * the recovery is done at startup of this function - but next time
         */
        subtitle_err("ups something went wrong. no more readers? \n");
    }

    releaseMutex(__LINE__);

    subtitle_printf(10, "<\n");
}

int getNextSub(char ** text, unsigned long long int * pts, long int * milliDuration) 
{
    subtitle_printf(50, "index %d\n", readPointer);

    if (text == NULL)
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    getMutex(__LINE__);

    if (subPuffer[readPointer].text == NULL)
    {
        /* this is acutally not an error, because it may happen
         * that there is no subtitle for a while
         */
        subtitle_printf(200, "null in subPuffer\n");
        releaseMutex(__LINE__);
        return cERR_SUBTITLE_ERROR;
    }
    
    *text = strdup(subPuffer[readPointer].text);
    free(subPuffer[readPointer].text);
    subPuffer[readPointer].text = NULL;

    *pts = subPuffer[readPointer].pts;
    subPuffer[readPointer].pts = 0;

    *milliDuration = subPuffer[readPointer].milliDuration;
    subPuffer[readPointer].milliDuration = 0;

    readPointer++;

    if (readPointer == PUFFERSIZE)
        readPointer = 0;

    if (writePointer == readPointer)
    {
        /* this may happen, in normal case the reader is ones ahead the 
         * writer. So this is the normal case that we eat the data
         * and have the reader reached.
         */
        subtitle_printf(20, "ups something went wrong. no more writers? \n");
    }

    releaseMutex(__LINE__);

    subtitle_printf(20, "readPointer %d\n",readPointer);
    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

/* **************************** */
/* Worker Thread                */
/* **************************** */

static void* SubtitleThread(void* data) 
{
    Context_t *context = (Context_t*) data;
    char *                  subText             = NULL;
    long int                subMilliDuration    = 0;
    unsigned long long int  subPts              = 0;
    unsigned long long int  Pts                 = 0;
    
    Writer_t* writer = NULL;

    subtitle_printf(10, "\n");

    while ( context->playback->isCreationPhase ) 
    {
        subtitle_err("Thread waiting for end of init phase...\n");
        usleep(1000);
    }

    subtitle_printf(10, "done\n");
    
    //
    writer = getDefaultFramebufferWriter();

    if (writer == NULL)
    {
        subtitle_err("no framebuffer writer found!\n");
    }

    while ( context && context->playback && context->playback->isPlaying) 
    {
	#if 0
        int curtrackid = -1;
        
        if (context && context->manager && context->manager->subtitle)
            context->manager->subtitle->Command(context, MANAGER_GET, &curtrackid);

        subtitle_printf(50, "curtrackid %d\n", curtrackid);

        if (curtrackid >= 0) 
        {
            if (getNextSub(&subText, &subPts, &subMilliDuration) != 0) 
            {
                usleep(500000);
                continue;
            }

            if (context && context->playback)
                context->playback->Command(context, PLAYBACK_PTS, &Pts);
            else 
            	return NULL;

            if(Pts > subPts) 
            {
                subtitle_printf(10,"subtitle is to late, ignoring\n");
                
                if(subText != NULL)
                    free(subText);
                    
                continue;
            }

            subtitle_printf(20, "Pts:%llu < subPts%llu duration %ld\n", Pts, subPts, subMilliDuration);

            while ( context && context->playback && context->playback->isPlaying && Pts < subPts) 
            {
                unsigned long int diff = subPts - Pts;
                diff = (diff*1000)/90.0;

                subtitle_printf(50, "DIFF: %lud\n", diff);

                if(diff > 100)
                    usleep(diff);

                if (context && context->playback)
                    context->playback->Command(context, PLAYBACK_PTS, &Pts);
                else
                { 
                   subtitle_err("no playback ? terminated?\n");
                   break;
                }
                subtitle_printf(20, "cur: %llu wanted: %llu\n", Pts, subPts);
            }
            
            // ass
            if (context && context->playback && context->playback->isPlaying && ass_track != NULL)
            {
            	// write
    		ass_write(context);
            }

	    //
            if (context && context->playback && context->playback->isPlaying && subText != NULL) 
            {
                if(clientFunction != NULL)
                    clientFunction(subMilliDuration, strlen(subText), subText, clientData);
                else
                    subtitle_printf(10, "writing Sub failed (%ld) (%d) \"%s\"\n", subMilliDuration, strlen(subText), subText);

                free(subText);
            }

        } /* trackID >= 0 */
        else //Wait
            usleep(500000);
        #endif
        
        ////
        //IF MOVIE IS PAUSED, WAIT
        if (context->playback->isPaused) 
        {
            subtitle_printf(20, "paused\n");

            usleep(100000);
            continue;
        }

        if (context->playback->isSeeking) 
        {
            subtitle_printf(10, "seeking\n");

            usleep(100000);
            continue;
        }

	// ass
        if (ass_track)
        {
            ASS_Image *       img   = NULL;
            int               change = 0;
            unsigned long int playPts;
            
            if (context && context->playback)
            {
                if (context->playback->Command(context, PLAYBACK_PTS, &playPts) < 0)
                    continue;
            }

            getMutex(__LINE__);
            
            //FIXME: durch den sleep bleibt die cpu usage zw. 5 und 13%, ohne
            //       steigt sie bei Verwendung von subtiteln bis auf 95%.
            //       ich hoffe dadurch gehen keine subtitle verloren, wenn die playPts
            //       durch den sleep verschlafen wird. Besser w�re es den n�chsten
            //       subtitel zeitpunkt zu bestimmen und solange zu schlafen.
            usleep(1000);

            img = ass_render_frame(ass_renderer, ass_track, playPts / 90.0, &change);

            subtitle_printf(150, "img %p pts %lu %f\n", img, playPts, playPts / 90.0);

            if(img != NULL && ass_renderer && ass_track)
            {
                /* the spec says, that if a new set of regions is present
                 * the complete display switches to the new state. So lets
                 * release the old regions on display.
                 */
                if (change != 0)
                    releaseRegions();

                while (context && context->playback && context->playback->isPlaying &&
                       (img) && (change != 0))
                {
                    WriterFBCallData_t out;
                    time_t now = time(NULL);
                    time_t undisplay = now + 10;

                    if (ass_track && ass_track->events)
                    {
                        undisplay = now + ass_track->events->Duration / 1000 + 0.5;
                    }

                    subtitle_printf(100, "w %d h %d s %d x %d y %d c %d chg %d now %ld und %ld\n", 
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
        ////

    } /* outer while */
    
    subtitle_printf(0, "has ended\n");
 
    hasThreadStarted = 0;
   
    return NULL;
}

/* ***************************** */
/* Functions                     */
/* ***************************** */

static int Write(void* _context, void *data) 
{
    Context_t * context = (Context_t  *) _context;
    char * Encoding = NULL;
    char * Text;
    SubtitleData_t * out;
    int DataLength;
    unsigned long long int Pts;
    float Duration;
    
    subtitle_printf(20, "\n");

    if (data == NULL)
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    out = (SubtitleData_t*) data;
    
    /*
    if (out->type == eSub_Txt)
    {
        Text = strdup((const char*) out->u.text.data);
    } 
    else
    {    
        subtitle_err("subtitle gfx currently not handled\n");
        return cERR_SUBTITLE_ERROR;
    }

    DataLength = out->u.text.len;
    Pts = out->pts;
    Duration = out->duration;
    */
    
    context->manager->subtitle->Command(context, MANAGER_GETENCODING, &Encoding);

    if (Encoding == NULL)
    {
       subtitle_err("encoding unknown\n");
       //free(Text);
       
       return cERR_SUBTITLE_ERROR;
    }
    
    subtitle_printf(20, "Encoding:%s\n", Encoding);
    
/*
    if (!strncmp("S_TEXT/SSA", Encoding, 10) || !strncmp("S_SSA", Encoding, 5))
        subtitle_ParseSSA(&Text);
    else if(!strncmp("S_TEXT/ASS", Encoding, 10) || !strncmp("S_AAS", Encoding, 5))
        subtitle_ParseASS(&Text);
    else if(!strncmp("S_TEXT/SRT", Encoding, 10) || !strncmp("S_SRT", Encoding, 5))
        subtitle_ParseSRT(&Text);
    else
    {
        subtitle_err("unknown encoding %s\n", Encoding);
        return  cERR_SUBTITLE_ERROR;
    }

    subtitle_printf(10, "Text:%s Duration:%f\n", Text,Duration);

    addSub(context, Text, Pts, Duration * 1000);
    
    free(Text);
*/
    
    //
    if(!strncmp("S_TEXT/SUBRIP", Encoding, 13))
    {
    	subtitle_printf(20, "FIXME: S_TEXT/SUBRIP\n");
    }
    else if (!strncmp("S_TEXT/ASS", Encoding, 10))
    {
    	subtitle_printf(20, "Write: S_TEXT/ASS\n");
    	
    	//
    	process_ass_data(context, data);
    }
    else if (!strncmp("S_TEXT/WEBVTT", Encoding, 18))
    {
    	subtitle_printf(20, "FIXME: S_TEXT/WEBVTT\n");
    }
    else if (!strncmp("S_GRAPHIC/PGS", Encoding, 13))
    {
    	subtitle_printf(20, "FIXME: S_GRAPHIC/PGS\n");
    }
    else if (!strncmp("S_GRAPHIC/DVB", Encoding, 13))
    {
    	subtitle_printf(20, "FIXME: S_GRAPHIC/DVB\n");
    }
    else if (!strncmp("S_GRAPHIC/XSUB", Encoding, 14))
    {
    	subtitle_printf(20, "FIXME: S_GRAPHIC/XSUB\n");
    }
    else if (!strncmp("S_TEXT/UTF-8", Encoding, 12))
    {
    	subtitle_printf(20, "FIXME: S_TEXT/UTF-8\n");
    }
    
    free(Encoding);

    subtitle_printf(20, "<\n");

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

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) 
    {
        subPuffer[i].text          = NULL;
        subPuffer[i].pts           = 0;
        subPuffer[i].milliDuration = 0;
    }

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

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) 
    {
        if (subPuffer[i].text != NULL)
           free(subPuffer[i].text);

        subPuffer[i].text          = NULL;
        subPuffer[i].pts           = 0;
        subPuffer[i].milliDuration = 0;
    }

    isSubtitleOpened = 0;

    releaseMutex(__LINE__);

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Play(Context_t* context) 
{
	 pthread_attr_t attr;
	 
    subtitle_printf(10, "\n");

    if (hasThreadStarted == 0)
    {  
        pthread_attr_init(&attr);
        
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create (&thread_sub, &attr, &SubtitleThread, (void*) context) != 0)
        {
           subtitle_err("Error creating thread\n");
           hasThreadStarted = 0;
        } 
        else
        {
           subtitle_printf(10, "Created thread\n");
           hasThreadStarted = 1;
        }
    }
    else
    {
        subtitle_err("thread already created.\n");
        return cERR_SUBTITLE_ERROR;
    }
    
    // ass
    getMutex(__LINE__);

    releaseRegions();

    /* free the track so extradata will be written next time
     * process_data is called.
     */
    if (ass_track)
        ass_free_track(ass_track);

    ass_track = NULL;

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

    while ( (hasThreadStarted != 0) && (--wait_time) > 0 ) 
    {
        subtitle_printf(10, "Waiting for subtitle thread to terminate itself, will try another %d times\n", wait_time);
        usleep(100000);
    }

    if (wait_time == 0) 
    {
        subtitle_err("Timeout waiting for thread!\n");

        return cERR_SUBTITLE_ERROR;
    }
    
    hasThreadStarted = 0;

    /* konfetti: thread has ended, so nobody will eat the date... 
     * free the data...
     */

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

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) 
    {
        if (subPuffer[i].text != NULL)
           free(subPuffer[i].text);

        subPuffer[i].text          = NULL;
        subPuffer[i].pts           = 0;
        subPuffer[i].milliDuration = 0;
    }

    releaseMutex(__LINE__);

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

void subtitle_SignalConnect(void (*fkt) (long int, size_t, char *, void *))
{
    subtitle_printf(10, "%p\n", fkt);

    clientFunction = fkt;
}

void subtitle_SignalConnectBuffer(void* data)
{
    subtitle_printf(10, "%p\n", data);

    clientData = data;
}

static int Command(void  *_context, OutputCmd_t command, void * argument) 
{
    Context_t  *context = (Context_t*) _context;
    int ret = cERR_SUBTITLE_NO_ERROR;

    subtitle_printf(50, "%d\n", command);

    switch(command) 
    {
    	    case OUTPUT_INIT:
    	    {
    		ret = ass_init(context);
    		break;
    	    }
    		
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
		//ret = subtitle_Play(context);
		break;
	    }
	    
	    case OUTPUT_STOP: 
	    {
		ret = subtitle_Stop(context);
		break;
	    }
	    
	    case OUTPUT_SWITCH: 
	    {
		ret = subtitle_Play(context);
		break;
	    }
	    
	    case OUTPUT_GET_SUBTITLE_OUTPUT: 
	    {
		SubtitleOutputDef_t* out = (SubtitleOutputDef_t*)argument;
		out->screen_width = screen_width;
		out->screen_height = screen_height;
		out->shareFramebuffer = shareFramebuffer;
		out->framebufferFD = framebufferFD;
		out->destination = destination;
		out->destStride = destStride;
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
	    
	    case OUTPUT_SUBTITLE_REGISTER_FUNCTION: 
	    {
		subtitle_SignalConnect(argument);
		break;
	    }
	    
	    case OUTPUT_SUBTITLE_REGISTER_BUFFER: 
	    {
		subtitle_SignalConnectBuffer(argument);
		break;
	    }
	    
	    case OUTPUT_FLUSH: 
	    {
		subtitle_err("Subtitle Flush not implemented\n");
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
	    
	    case OUTPUT_DATA: 
	    {
	    	ret = cERR_SUBTITLE_ERROR;
	    	
		SubtitleData_t* data = (SubtitleData_t*) argument;
		
		if (data != NULL)
			ret = cERR_SUBTITLE_NO_ERROR;
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

