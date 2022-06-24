/*
 * subtitle handling for srt files.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <limits.h>

#include "common.h"
#include "misc.h"
#include "subtitle.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define SRT_DEBUG

#ifdef SRT_DEBUG

static short debug_level = 10;

#define srt_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define srt_printf(level, fmt, x...)
#endif

#ifndef SRT_SILENT
#define srt_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define srt_err(fmt, x...)
#endif

/* Error Constants */
#define cERR_SRT_NO_ERROR        0
#define cERR_SRT_ERROR          -1

#define TRACKWRAP 20
#define MAXLINELENGTH 80

static const char FILENAME[] = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

typedef struct {
    char * File;
    int Id;
} SrtTrack_t;

static pthread_t thread_sub;

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static SrtTrack_t * Tracks;
static int TrackCount = 0;
static int CurrentTrack = -1; //no as default.

FILE * fsub = NULL;

static int hasThreadStarted = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

void data_to_manager(Context_t *context, char* Text, unsigned long long int Pts, double Duration)
{
    srt_printf(20, "--> Text= \"%s\"\n", Text);

    if( context &&
        context->playback &&
        context->playback->isPlaying){
            int sl = strlen(Text)-1;
            while(sl && (Text[sl]=='\n' || Text[sl]=='\r')) Text[sl--]='\0'; /*Delete last \n or \r */
            unsigned char* line = text_to_ass(Text, Pts, Duration);
            srt_printf(50,"Sub text is %s\n",Text);
            srt_printf(50,"Sub line is %s\n",line);
            SubtitleData_t data;
            data.data      = line;
            data.len       = strlen((char*)line);
            data.extradata = DEFAULT_ASS_HEAD;
            data.extralen  = strlen(DEFAULT_ASS_HEAD);
            data.pts       = Pts*90;
            data.duration  = Duration;

            context->container->assContainer->Command(context, CONTAINER_DATA, &data);
            free(line);
    }

    srt_printf(20, "<-- Text= \"%s\"\n", Text);
}

/* ***************************** */
/* Worker Thread                 */
/* ***************************** */

static void* SrtSubtitleThread(void *data) {
    int pos = 0;
    char  Data[MAXLINELENGTH];
    unsigned long long int Pts = 0;
    double Duration = 0;
    char * Text = NULL;
     
    Context_t *context = (Context_t*) data;

    srt_printf(10, "\n");

    while(context && context->playback && context->playback->isPlaying && fsub && fgets(Data, MAXLINELENGTH, fsub)) {
        srt_printf(20, "pos=%d\n", pos);

        if(pos == 0) 
        {
            if(Data[0] == '\n' || Data[0] == '\0' || Data[0] == 13 /* ^M */) 
                continue; /* Empty line not allowed here */
            pos++;
        } else if(pos == 1) 
        {
            int ret, horIni, minIni, secIni, milIni, horFim, minFim, secFim, milFim;

            ret = sscanf(Data, "%d:%d:%d,%d --> %d:%d:%d,%d", &horIni, &minIni, &secIni, &milIni, &horFim, &minFim, &secFim, &milFim);
            if (ret!=8) continue; /* Data is not in correct format */

            Pts = (horIni*3600 + minIni*60 + secIni)*1000 + milIni;
            Duration = ((horFim*3600 + minFim*60 + secFim) * 1000  + milFim - Pts) / 1000.0;

            pos++;

        } else if(pos == 2) {
            srt_printf(20, "Data[0] = %d \'%c\'\n", Data[0], Data[0]);

            if(Data[0] == '\n' || Data[0] == '\0' || Data[0] == 13 /* ^M */) {
                if(Text == NULL)
                    Text = strdup(" \n"); /* better to display at least one character */

                /*Hellmaster 1024 since we have waited, we have to check if we are still paying */
                data_to_manager(context, Text, Pts, Duration);
                free(Text);
                Text = NULL;
                pos = 0;
                continue;
            }

            if(!Text) {
                Text = strdup(Data);
            } else {
                int length = strlen(Text) /* \0 -> \n */ + strlen(Data) + 2 /* \0 */;
                char * tmpText = strdup(Text);

                free(Text);

                Text = (char*)malloc(length);

                strcpy(Text, tmpText);
                strcat(Text, Data);
                free(tmpText);
            }
        }
    } /* while */

    hasThreadStarted = 0;

    if(Text) {
        data_to_manager(context, Text, Pts, Duration);
        free(Text);
        Text = NULL;
    }

    srt_printf(0, "thread has ended\n");

    return NULL;
}

/* ***************************** */
/* Functions                     */
/* ***************************** */

static void SrtManagerAdd(Context_t  *context, SrtTrack_t track) {
    srt_printf(10, "%s %d\n",track.File, track.Id);

    if (Tracks == NULL) {
        Tracks = malloc(sizeof(SrtTrack_t) * TRACKWRAP);
    }

    if (TrackCount < TRACKWRAP) {
        Tracks[TrackCount].File       = strdup(track.File);
        Tracks[TrackCount].Id         = track.Id;
        TrackCount++;
    }
}

static char ** SrtManagerList(Context_t  *context) {
    char ** tracklist = NULL;

    srt_printf(10, "\n");

    if (Tracks != NULL) {
        char help[256];
        int i = 0, j = 0;

        tracklist = malloc(sizeof(char *) * ((TrackCount*2) + 1));

        for (i = 0, j = 0; i < TrackCount; i++, j+=2) {

            sprintf(help, "%d", Tracks[i].Id);
            tracklist[j]    = strdup(help);
            tracklist[j+1]  = strdup(Tracks[i].File);
        }
        tracklist[j] = NULL;
    }

    return tracklist;
}

static void SrtManagerDel(Context_t * context) {
    int i = 0;

    srt_printf(10, "\n");

    if(Tracks != NULL) {
        for (i = 0; i < TrackCount; i++) {
            if (Tracks[i].File != NULL)
                free(Tracks[i].File);
            Tracks[i].File = NULL;
        }
        free(Tracks);
        Tracks = NULL;
    }

    TrackCount = 0;
    CurrentTrack = -1;
}


static int SrtGetSubtitle(Context_t  *context, char * Filename) {
    struct dirent *dirzeiger;
    DIR  *  dir;
    int     i                    = TEXTSRTOFFSET;
    char *  copyFilename         = NULL;
    char *  FilenameExtension    = NULL;
    char *  FilenameFolder       = NULL;
    char *  FilenameShort        = NULL;

    srt_printf(10, "\n");

    if (Filename == NULL)
    {
       srt_err("Filename NULL\n");
       return cERR_SRT_ERROR;
    }
    
    srt_printf(10, "file: %s\n", Filename);

    copyFilename = strdup(Filename);

    FilenameFolder = dirname(copyFilename);
    
    if (FilenameFolder == NULL)
    {
       srt_err("FilenameFolder NULL\n");
       return cERR_SRT_ERROR;
    }
    
    srt_printf(10, "folder: %s\n", FilenameFolder);

    getExtension(copyFilename, &FilenameExtension);

    if (FilenameExtension == NULL)
    {
       srt_err("FilenameExtension NULL\n");
       free(FilenameFolder);
       return cERR_SRT_ERROR;
    }

    srt_printf(10, "ext: %s\n", FilenameExtension);

    FilenameShort = basename(copyFilename);
    
    /* cut extension */
    FilenameShort[strlen(FilenameShort) - strlen(FilenameExtension) - 1] = '\0';
    
    srt_printf(10, "basename: %s\n", FilenameShort);
    srt_printf(10, "%s\n%s | %s | %s\n", copyFilename, FilenameFolder, FilenameShort, FilenameExtension);

    if((dir = opendir(FilenameFolder)) != NULL) {
        while((dirzeiger = readdir(dir)) != NULL) {
            char subtitleFilename[PATH_MAX];
            char *subtitleExtension = NULL;

            srt_printf(20, "%s\n",(*dirzeiger).d_name);

            strcpy(subtitleFilename, (*dirzeiger).d_name);

            // Extension of Relativ Subtitle File Name
            getExtension(subtitleFilename, &subtitleExtension);

            if (subtitleExtension == NULL)
                continue;

            if (strcmp(subtitleExtension, "srt") != 0)
            {
                free(subtitleExtension);
                continue;
            }

            /* cut extension */
            subtitleFilename[strlen(subtitleFilename) - strlen(subtitleExtension) - 1] = '\0';

            srt_printf(10, "%s %s\n", FilenameShort, subtitleFilename);

            if (strncmp(FilenameShort, subtitleFilename,strlen(FilenameShort)) == 0)
            {
                char absSubtitleFileName[PATH_MAX];
                /* found something of interest, so now make an absolut path name */
                
                sprintf(absSubtitleFileName, "%s/%s.%s", FilenameFolder, subtitleFilename, subtitleExtension);
                
                srt_printf(10, "SRT: %s [%s]\n", subtitleExtension, subtitleFilename);
                srt_printf(10, "\t->%s\n", absSubtitleFileName);

                SrtTrack_t SrtSubtitle = {
                        absSubtitleFileName,
                        i,
                };
                
                SrtManagerAdd(context, SrtSubtitle);

                Track_t Subtitle = {
                        subtitleExtension,
                        "S_TEXT/SRT",
                        i++,
                };
                context->manager->subtitle->Command(context, MANAGER_ADD, &Subtitle);
            }

            free(subtitleExtension);
        } /* while */
	closedir(dir);
    } /* if dir */

    free(FilenameExtension);
    free(copyFilename);

    srt_printf(10, "<\n");
    return cERR_SRT_NO_ERROR;
}

static int SrtOpenSubtitle(Context_t *context, int trackid) {
    srt_printf(10, "\n");

    if(trackid < TEXTSRTOFFSET || (trackid % TEXTSRTOFFSET) >= TrackCount) {
        srt_err("trackid not for us\n");
        return cERR_SRT_ERROR;
    }

    trackid %= TEXTSRTOFFSET;

    srt_printf(10, "%s\n", Tracks[trackid].File);

    fsub = fopen(Tracks[trackid].File, "rb");

    srt_printf(10, "%s\n", fsub ? "fsub!=NULL" : "fsub==NULL");

    if(!fsub)
    {
        srt_err("cannot open file %s\n", Tracks[trackid].File);
        return cERR_SRT_ERROR;
    }
    return cERR_SRT_NO_ERROR;
}

static int SrtCloseSubtitle(Context_t *context) {
    srt_printf(10, "\n");

    if(fsub)
        fclose(fsub);

    /* this closes the thread! */
    fsub = NULL;

    hasThreadStarted = 0;

    return cERR_SRT_NO_ERROR;
}


static int SrtSwitchSubtitle(Context_t *context, int* arg) {
    int ret = cERR_SRT_NO_ERROR;
    
    srt_printf(10, "arg:%d\n", *arg);

    ret = SrtCloseSubtitle(context);

    if (( (ret |= SrtOpenSubtitle(context, *arg)) == cERR_SRT_NO_ERROR) && (!hasThreadStarted))
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create (&thread_sub, &attr, &SrtSubtitleThread, context);

        hasThreadStarted = 1;
    }
    return ret;
}

static int SrtDel(Context_t *context) {
    int ret = cERR_SRT_NO_ERROR;
  
    srt_printf(10, "\n");

    ret = SrtCloseSubtitle(context);
    SrtManagerDel(context);

    return ret;
}

static int Command(void  *_context, ContainerCmd_t command, void * argument) {
    Context_t  *context = (Context_t*) _context;
    int ret = cERR_SRT_NO_ERROR;
    
    srt_printf(10, "\n");

    switch(command) {
    case CONTAINER_INIT: {
        char * filename = (char *)argument;
        ret = SrtGetSubtitle(context, filename);
        break;
    }
    case CONTAINER_DEL: {
        ret = SrtDel(context);
        break;
    }
    case CONTAINER_SWITCH_SUBTITLE: {
        ret = SrtSwitchSubtitle(context, (int*) argument);
        break;
    }
    default:
        srt_err("ConatinerCmd not supported! %d\n", command);
        break;
    }

    srt_printf(10, "ret = %d\n", ret);

    return 0;
}

static char *SrtCapabilities[] = { "srt", NULL };

Container_t SrtContainer = {
    "SRT",
    &Command,
    SrtCapabilities,
};
