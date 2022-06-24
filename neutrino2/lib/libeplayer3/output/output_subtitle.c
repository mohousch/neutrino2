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
#include "subtitle.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define SUBTITLE_DEBUG

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

struct sub_t {
    char *                 text;
    unsigned long long int pts;
    unsigned long int      milliDuration;
};


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

static int            screen_width     = 0;
static int            screen_height    = 0;
static int            destStride       = 0;
static int            shareFramebuffer = 0;
static int            framebufferFD    = -1;
static unsigned char* destination      = NULL;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
static void getMutex(int line) {
    subtitle_printf(100, "%d requesting mutex\n", line);

    pthread_mutex_lock(&mutex);

    subtitle_printf(100, "%d received mutex\n", line);
}

static void releaseMutex(int line) {
    pthread_mutex_unlock(&mutex);

    subtitle_printf(100, "%d released mutex\n", line);
}

void replace_all(char ** string, char * search, char * replace) {
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

    while ((ptr = strstr(tempString, search)) != NULL) {
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

int subtitle_ParseASS (char **Line) {
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
    
    for (i=0; i < 9 && *ptr1 != '\0'; ptr1++) {

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

int subtitle_ParseSRT (char **Line) {

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

int subtitle_ParseSSA (char **Line) {

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

void addSub(Context_t  *context, char * text, unsigned long long int pts, unsigned long int milliDuration) {
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
    
    while (subPuffer[writePointer].text != NULL) {
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

        for (i = 0; i < PUFFERSIZE; i++) {
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

int getNextSub(char ** text, unsigned long long int * pts, long int * milliDuration) {

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

static void* SubtitleThread(void* data) {
    Context_t *context = (Context_t*) data;
    char *                  subText             = NULL;
    long int                subMilliDuration    = 0;
    unsigned long long int  subPts              = 0;
    unsigned long long int  Pts                 = 0;

    subtitle_printf(10, "\n");

    while ( context->playback->isCreationPhase ) {
        subtitle_err("Thread waiting for end of init phase...\n");
        usleep(1000);
    }

    subtitle_printf(10, "done\n");

    while ( context &&
            context->playback &&
            context->playback->isPlaying) {

        int curtrackid = -1;
        
        if (context && context->manager && context->manager->subtitle)
            context->manager->subtitle->Command(context, MANAGER_GET, &curtrackid);

        subtitle_printf(50, "curtrackid %d\n", curtrackid);

        if (curtrackid >= 0) {
            if (getNextSub(&subText, &subPts, &subMilliDuration) != 0) {
                usleep(500000);
                continue;
            }

            if (context && context->playback)
                context->playback->Command(context, PLAYBACK_PTS, &Pts);
            else return NULL;

            if(Pts > subPts) {
                subtitle_printf(10,"subtitle is to late, ignoring\n");
                if(subText != NULL)
                    free(subText);
                continue;
            }

            subtitle_printf(20, "Pts:%llu < subPts%llu duration %ld\n", Pts, subPts,subMilliDuration);

            while ( context &&
                    context->playback &&
                    context->playback->isPlaying &&
                    Pts < subPts) {

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

            if (    context &&
                    context->playback &&
                    context->playback->isPlaying &&
                    subText != NULL ) {

                if(clientFunction != NULL)
                    clientFunction(subMilliDuration, strlen(subText), subText, clientData);
                else
                    subtitle_printf(10, "writing Sub failed (%ld) (%d) \"%s\"\n", subMilliDuration, strlen(subText), subText);

                free(subText);
            }

        } /* trackID >= 0 */
        else //Wait
            usleep(500000);

    } /* outer while */
    
    subtitle_printf(0, "has ended\n");
 
    hasThreadStarted = 0;
   
    return NULL;
}

/* ***************************** */
/* Functions                     */
/* ***************************** */

static int Write(void* _context, void *data) {
    Context_t  * context = (Context_t  *) _context;
    char * Encoding = NULL;
    char * Text;
    SubtitleOut_t * out;
    int DataLength;
    unsigned long long int Pts;
    float Duration;
    
    subtitle_printf(10, "\n");

    if (data == NULL)
    {
        subtitle_err("null pointer passed\n");
        return cERR_SUBTITLE_ERROR;
    }

    out = (SubtitleOut_t*) data;
    
    if (out->type == eSub_Txt)
    {
        Text = strdup((const char*) out->u.text.data);
    } else
    {
/* fixme handle gfx subs from container_ass and send it to
 * the callback. this must be implemented also in e2/neutrino
 * then.
 */    
        subtitle_err("subtitle gfx currently not handled\n");
        return cERR_SUBTITLE_ERROR;
    } 

    DataLength = out->u.text.len;
    Pts = out->pts;
    Duration = out->duration;
    
    context->manager->subtitle->Command(context, MANAGER_GETENCODING, &Encoding);

    if (Encoding == NULL)
    {
       subtitle_err("encoding unknown\n");
       free(Text);
       return cERR_SUBTITLE_ERROR;
    }
    
    subtitle_printf(20, "Encoding:%s Text:%s Len:%d\n", Encoding,Text, DataLength);

    if (    !strncmp("S_TEXT/SSA",  Encoding, 10) ||
            !strncmp("S_SSA",       Encoding, 5))
        subtitle_ParseSSA(&Text);
    
    else if(!strncmp("S_TEXT/ASS",  Encoding, 10) ||
            !strncmp("S_AAS",       Encoding, 5))
        subtitle_ParseASS(&Text);
    
    else if(!strncmp("S_TEXT/SRT",  Encoding, 10) ||
            !strncmp("S_SRT",       Encoding, 5))
        subtitle_ParseSRT(&Text);
    else
    {
        subtitle_err("unknown encoding %s\n", Encoding);
        return  cERR_SUBTITLE_ERROR;
    }
    
    subtitle_printf(10, "Text:%s Duration:%f\n", Text,Duration);

    addSub(context, Text, Pts, Duration * 1000);
    
    free(Text);
    free(Encoding);

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Open(context) {
    int i;

    subtitle_printf(10, "\n");

    if (isSubtitleOpened == 1)
    {
        subtitle_err("already opened! ignoring\n");
        return cERR_SUBTITLE_ERROR;
    }

    getMutex(__LINE__);

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) {
        subPuffer[i].text          = NULL;
        subPuffer[i].pts           = 0;
        subPuffer[i].milliDuration = 0;
    }

    isSubtitleOpened = 1;

    releaseMutex(__LINE__);

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Close(Context_t* context) {
    int i;

    subtitle_printf(10, "\n");

    getMutex(__LINE__);

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) {
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

static int subtitle_Play(Context_t* context) {
    subtitle_printf(10, "\n");

    if (hasThreadStarted == 0)
    {
        pthread_attr_t attr;
        
        pthread_attr_init(&attr);
        
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create (&thread_sub, &attr, &SubtitleThread, (void*) context) != 0)
        {
           subtitle_err("Error creating thread\n");
           hasThreadStarted = 0;
        } else
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

    subtitle_printf(10, "<\n");

    return cERR_SUBTITLE_NO_ERROR;
}

static int subtitle_Stop(context) {
    int wait_time = 20;
    int i;
    
    subtitle_printf(10, "\n");

    while ( (hasThreadStarted != 0) && (--wait_time) > 0 ) {
        subtitle_printf(10, "Waiting for subtitle thread to terminate itself, will try another %d times\n", wait_time);
        usleep(100000);
    }

    if (wait_time == 0) {
        subtitle_err("Timeout waiting for thread!\n");

        return cERR_SUBTITLE_ERROR;
    }
    
    hasThreadStarted = 0;

    /* konfetti: thread has ended, so nobody will eat the date... 
     * free the data...
     */

    getMutex(__LINE__);

    //Reset all
    readPointer = 0;
    writePointer = 0;

    for (i = 0; i < PUFFERSIZE; i++) {
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

static int Command(void  *_context, OutputCmd_t command, void * argument) {
    Context_t  *context = (Context_t*) _context;
    int ret = cERR_SUBTITLE_NO_ERROR;

    subtitle_printf(50, "%d\n", command);

    switch(command) {
    case OUTPUT_OPEN: {
        ret = subtitle_Open(context);
        break;
    }
    case OUTPUT_CLOSE: {
        ret = subtitle_Close(context);
        break;
    }
    case OUTPUT_PLAY: {
        ret = subtitle_Play(context);
        break;
    }
    case OUTPUT_STOP: {
        ret = subtitle_Stop(context);
        break;
    }
    case OUTPUT_SWITCH: {
        subtitle_err("Subtitle Switch not implemented\n");
        ret = cERR_SUBTITLE_ERROR;
        break;
    }
    case OUTPUT_GET_SUBTITLE_OUTPUT: {
        SubtitleOutputDef_t* out = (SubtitleOutputDef_t*)argument;
        out->screen_width = screen_width;
        out->screen_height = screen_height;
        out->shareFramebuffer = shareFramebuffer;
        out->framebufferFD = framebufferFD;
        out->destination = destination;
        out->destStride = destStride;
        break;
    }
    case OUTPUT_SET_SUBTITLE_OUTPUT: {
        SubtitleOutputDef_t* out = (SubtitleOutputDef_t*)argument;
        screen_width = out->screen_width;
        screen_height = out->screen_height;
        shareFramebuffer = out->shareFramebuffer;
        framebufferFD = out->framebufferFD;
        destination = out->destination;
        destStride = out->destStride;
        break;
    }
    case OUTPUT_SUBTITLE_REGISTER_FUNCTION: {
        subtitle_SignalConnect(argument);
        break;
    }
    case OUTPUT_SUBTITLE_REGISTER_BUFFER: {
        subtitle_SignalConnectBuffer(argument);
        break;
    }
    case OUTPUT_FLUSH: {
        subtitle_err("Subtitle Flush not implemented\n");
        ret = cERR_SUBTITLE_ERROR;
        break;
    }
    case OUTPUT_PAUSE: {
        subtitle_err("Subtitle Pause not implemented\n");
        ret = cERR_SUBTITLE_ERROR;
    	break;
    }
    case OUTPUT_CONTINUE: {
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


static char *SubtitleCapabilitis[] = { "subtitle", NULL };

struct Output_s SubtitleOutput = {
    "Subtitle",
    &Command,
    &Write,
    SubtitleCapabilitis,

};

