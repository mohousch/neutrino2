/*
 * subtitle handling for ssa files.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include "misc.h"
#include "subtitle.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define SSA_DEBUG

#ifdef SSA_DEBUG

static short debug_level = 10;

#define ssa_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define ssa_printf(level, fmt, x...)
#endif

#ifndef SSA_SILENT
#define ssa_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define ssa_err(fmt, x...)
#endif

/* Error Constants */
#define cERR_SSA_NO_ERROR        0
#define cERR_SSA_ERROR          -1

#define TRACKWRAP 20
#define MAXLINELENGTH 1000

//Buffer size used in getLine function. Do not set to value less than 1 !!!
#define SSA_BUFFER_SIZE 14

static const char FILENAME[] = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

typedef struct {
    char * File;
    int Id;
} SsaTrack_t;

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static pthread_t thread_sub;

static SsaTrack_t * Tracks;
static int TrackCount = 0;
FILE * fssa = NULL;

static int hasThreadStarted = 0;

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
char *SSAgetLine()
{
    char *strAux = NULL, *strInput;
    char c[SSA_BUFFER_SIZE], ch;
    int k, tam, tamAux;

    k = tamAux = 0;

    if(SSA_BUFFER_SIZE>0)
    {

        strInput = (char*)malloc(1*sizeof(char));
        strInput[0]='\0';

        while(tamAux!=1)
        {

            if((ch = fgetc(fssa))!=EOF)
            {
                ungetc(ch , fssa);
                fgets(c, SSA_BUFFER_SIZE, fssa);
                strAux = (char*)strchr(c,'\n');
                tam = strlen(c);
                if(strAux != NULL)
                {
                    tamAux = strlen(strAux);
                    tam--;
                }

                k = k + tam;
                strInput = (char*)realloc(strInput, (k+1)*sizeof(char));

                if(k!=tam)
                    strncat(strInput, c, tam);
                else
                    strncpy(strInput, c, tam);

                strInput[k] = '\0';

            }
            else { 
                tamAux = 1;
                fclose(fssa);
                fssa = NULL;
            }
        }

    }

    return strInput;

}

/* ***************************** */
/* Worker Thread                 */
/* ***************************** */
static void* SsaSubtitleThread(void *data) {
    Context_t *context = (Context_t*) data;
    char *                 head =malloc(sizeof(char)*1);

    ssa_printf(10, "\n");
    head[0]='\0';


    while ( context && context->playback && context->playback->isPlaying && fssa ) {
        char *line = NULL;

        do 
        {
            line = SSAgetLine();
            if(strncmp(line,"Dialogue: ",10)) {
                int head_len = strlen(head);
                int line_len = strlen(line);
                head = realloc(head, line_len + head_len +2);
                memcpy(head + head_len, line, sizeof(char)*line_len+1);
                head[head_len + line_len] = '\n';
                head[head_len + line_len + 1] = '\0';
            }
        } while (strncmp(line,"Dialogue: ",10)!=0 && fssa);

        /*Hellmaster 1024 since we have waited, we have to check if we are still paying */
        if( context &&
            context->playback &&
            context->playback->isPlaying) {
                SubtitleData_t data;
                          
                data.data      = (unsigned char*) line;
                data.len       = strlen(line);
                data.extradata = (unsigned char*) head;
                data.extralen  = strlen(head);
                data.pts       = 0;
                data.duration  = 0.0;
                context->container->assContainer->Command(context, CONTAINER_DATA, &data);
        }
        free(line);
        line = NULL;
        continue;
    }

    hasThreadStarted = 0;
    
    if(head) {
        free(head);
        head = NULL;
    }
    ssa_printf(0, "thread has ended\n");

    return NULL;
}


/* ***************************** */
/* Functions                     */
/* ***************************** */

static void SsaManagerAdd(Context_t  *context, SsaTrack_t track) {
    ssa_printf(10, "%s %d\n", track.File, track.Id);

    if (Tracks == NULL) {
        Tracks = malloc(sizeof(SsaTrack_t) * TRACKWRAP);
    }

    if (TrackCount < TRACKWRAP) {
        Tracks[TrackCount].File       = strdup(track.File);
        Tracks[TrackCount].Id         = track.Id;
        TrackCount++;
    }
}

static char ** SsaManagerList(Context_t  *context) {
    char ** tracklist = NULL;

    ssa_printf(10, "\n");

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

static void SsaManagerDel(Context_t * context) {
    int i = 0;

    ssa_printf(10, "\n");

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
}

static int SsaGetSubtitle(Context_t  *context, char * Filename) {
    struct dirent *dirzeiger;
    DIR  *  dir;
    int     i                    = TEXTSSAOFFSET;
    char *  copyFilename         = NULL;
    char *  FilenameExtension    = NULL;
    char *  FilenameFolder       = NULL;
    char *  FilenameShort        = NULL;

    ssa_printf(10, "\n");

    if (Filename == NULL)
    {
       ssa_err("Filename NULL\n");
       return cERR_SSA_ERROR;
    }
    
    ssa_printf(10, "file: %s\n", Filename);

    copyFilename = strdup(Filename);

    FilenameFolder = dirname(copyFilename);
    
    if (FilenameFolder == NULL)
    {
       ssa_err("FilenameFolder NULL\n");
       return cERR_SSA_ERROR;
    }
    
    ssa_printf(10, "folder: %s\n", FilenameFolder);

    getExtension(copyFilename, &FilenameExtension);

    if (FilenameExtension == NULL)
    {
       ssa_err("FilenameExtension NULL\n");
       free(FilenameFolder);
       return cERR_SSA_ERROR;
    }

    ssa_printf(10, "ext: %s\n", FilenameExtension);

    FilenameShort = basename(copyFilename);

    /* cut extension */
    FilenameShort[strlen(FilenameShort) - strlen(FilenameExtension) - 1] = '\0';

    ssa_printf(10, "basename: %s\n", FilenameShort);
    ssa_printf(10, "%s\n%s | %s | %s\n", copyFilename, FilenameFolder, FilenameShort, FilenameExtension);

    if((dir = opendir(FilenameFolder)) != NULL) {
        while((dirzeiger = readdir(dir)) != NULL) {
            char subtitleFilename[PATH_MAX];
            char *subtitleExtension = NULL;

            ssa_printf(20, "%s\n",(*dirzeiger).d_name);

            strcpy(subtitleFilename, (*dirzeiger).d_name);

            // Extension of Relativ Subtitle File Name
            getExtension(subtitleFilename, &subtitleExtension);

            if (subtitleExtension == NULL)
                continue;

            if ( strcmp(subtitleExtension, "ssa") != 0 && strcmp(subtitleExtension, "ass") != 0 )
            {
                free(subtitleExtension);
                continue;
            }

            /* cut extension */
            subtitleFilename[strlen(subtitleFilename) - strlen(subtitleExtension) - 1] = '\0';

            ssa_printf(10, "%s %s\n", FilenameShort, subtitleFilename);

            if (strncmp(FilenameShort, subtitleFilename,strlen(FilenameShort)) == 0)
            {
                char absSubtitleFileName[PATH_MAX];
                /* found something of interest, so now make an absolut path name */
                
                sprintf(absSubtitleFileName, "%s/%s.%s", FilenameFolder, subtitleFilename, subtitleExtension);
                
                ssa_printf(10, "SSA: %s [%s]\n", subtitleExtension, subtitleFilename);
                ssa_printf(10, "\t->%s\n", absSubtitleFileName);

                SsaTrack_t SsaSubtitle = {
                        absSubtitleFileName,
                        i,
                };
                
                SsaManagerAdd(context, SsaSubtitle);

                Track_t Subtitle = {
                        subtitleExtension,
                        "S_TEXT/SSA",
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

    ssa_printf(10, "<\n");

    return cERR_SSA_NO_ERROR;
}
static int SsaOpenSubtitle(Context_t *context, int trackid) {
    ssa_printf(10, "\n");

    if(trackid < TEXTSSAOFFSET || (trackid % TEXTSSAOFFSET) >= TrackCount ) {
        ssa_err("trackid not for us\n");
        return cERR_SSA_ERROR;
    }

    trackid %= TEXTSSAOFFSET;

    ssa_printf(10, "%s\n", Tracks[trackid].File);

    fssa = fopen(Tracks[trackid].File, "rb");

    ssa_printf(10, "%s\n", fssa ? "fssa!=NULL" : "fssa==NULL");

    if (!fssa)
    {
        ssa_err("cannot open file %s\n", Tracks[trackid].File);
        return cERR_SSA_ERROR;
    }
    return cERR_SSA_NO_ERROR;
}

static int SsaCloseSubtitle(Context_t *context) {
    ssa_printf(10, "\n");

    if(fssa)
        fclose(fssa);

    /* this closes the thread! */
    fssa = NULL;
    
    hasThreadStarted = 0;

    return cERR_SSA_NO_ERROR;
}

static int SsaSwitchSubtitle(Context_t *context, int* arg) {
    int ret = cERR_SSA_NO_ERROR;
    
    ssa_printf(10, "\n");

    ret = SsaCloseSubtitle(context);

    if (((ret |= SsaOpenSubtitle(context, *arg)) == cERR_SSA_NO_ERROR) && (!hasThreadStarted))
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create (&thread_sub, &attr, &SsaSubtitleThread, context);
        
        hasThreadStarted = 1;
    }

    return ret;
}

static int SsaDel(Context_t *context) {
    int ret = cERR_SSA_NO_ERROR;
  
    ssa_printf(10, "\n");

    ret = SsaCloseSubtitle(context);
    
    SsaManagerDel(context);

    return ret;
}

static int Command(void  *_context, ContainerCmd_t command, void * argument) {
    Context_t  *context = (Context_t*) _context;
    int ret = cERR_SSA_NO_ERROR;
    
    ssa_printf(10, "\n");

    switch(command) {
    case CONTAINER_INIT: {
        char * filename = (char *)argument;
        ret = SsaGetSubtitle(context, filename);
        break;
    }
    case CONTAINER_DEL: {
        ret = SsaDel(context);
        break;
    }
    case CONTAINER_SWITCH_SUBTITLE: {
        ret = SsaSwitchSubtitle(context, (int*) argument);
        break;
    }
    default:
        ssa_err("ConatinerCmd not supported! %d\n", command);
        break;
    }

    ssa_printf(10, "ret = %d\n", ret);

    return 0;
}

static char *SsaCapabilities[] = { "ssa", NULL };

Container_t SsaContainer = {
    "SSA",
    &Command,
    SsaCapabilities,
};
