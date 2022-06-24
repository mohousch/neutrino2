/*
 * video manager handling.
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
#include <string.h>

#include "manager.h"
#include "common.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */
#define TRACKWRAP 4

//#define VIDEO_MGR_DEBUG

#ifdef VIDEO_MGR_DEBUG

static short debug_level = 0;

#define video_mgr_printf(level, x...) do { \
if (debug_level >= level) printf(x); } while (0)
#else
#define video_mgr_printf(level, x...)
#endif

#ifndef VIDEO_MGR_SILENT
#define video_mgr_err(x...) do { printf(x); } while (0)
#else
#define video_mgr_err(x...)
#endif

/* Error Constants */
#define cERR_VIDEO_MGR_NO_ERROR        0
#define cERR_VIDEO_MGR_ERROR          -1

static const char FILENAME[] = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

static Track_t * Tracks;
static int TrackCount = 0;
static int CurrentTrack = 0; //TRACK[0] as default.

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* Functions                     */
/* ***************************** */

static int ManagerAdd(Context_t  *context, Track_t track) 
{
    	video_mgr_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

    	if (Tracks == NULL) 
	{
        	Tracks = malloc(sizeof(Track_t) * TRACKWRAP);
    	}

    	if (Tracks == NULL)
    	{
        	video_mgr_err("%s:%s malloc failed\n", FILENAME, __FUNCTION__);
        	return cERR_VIDEO_MGR_ERROR;
    	}

    	if (TrackCount < TRACKWRAP) 
	{
        	copyTrack(&Tracks[TrackCount], &track);

        	TrackCount++;
    	} 
	else 
	{
        	video_mgr_err("%s:%s TrackCount out if range %d - %d\n", FILENAME, __FUNCTION__, TrackCount, TRACKWRAP);
        	return cERR_VIDEO_MGR_ERROR;
    	}

    	if (TrackCount > 0)
        	context->playback->isVideo = 1;

    	video_mgr_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

    	return cERR_VIDEO_MGR_NO_ERROR;
}

static char ** ManagerList(Context_t  *context) 
{
    	int i = 0, j = 0;
    	char ** tracklist = NULL;

    	video_mgr_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

    	if (Tracks != NULL) 
	{
        	tracklist = malloc(sizeof(char *) * ((TrackCount*2) + 1));

        	if (tracklist == NULL)
        	{
            		video_mgr_err("%s:%s malloc failed\n", FILENAME, __FUNCTION__);
            		return NULL;
        	}

        	for (i = 0, j = 0; i < TrackCount; i++, j+=2) 
		{
            		tracklist[j]    = strdup(Tracks[i].Name);
            		tracklist[j+1]  = strdup(Tracks[i].Encoding);
        	}
        	tracklist[j] = NULL;
    	}

    	video_mgr_printf(10, "%s::%s return %p (%d - %d)\n", FILENAME, __FUNCTION__, tracklist, j, TrackCount);

    	return tracklist;
}

static int ManagerDel(Context_t * context) 
{
    	int i = 0;

    	video_mgr_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

    	if(Tracks != NULL) 
	{
        	for (i = 0; i < TrackCount; i++) 
		{
            		freeTrack(&Tracks[i]);
        	}
        	free(Tracks);
        	Tracks = NULL;
    	} 
	else
    	{
        	video_mgr_err("%s::%s nothing to delete!\n", FILENAME, __FUNCTION__);
        	return cERR_VIDEO_MGR_ERROR;
    	}

    	TrackCount = 0;
    	CurrentTrack = 0;
    	context->playback->isVideo = 0;

    	video_mgr_printf(10, "%s::%s return no error\n", FILENAME, __FUNCTION__);

    	return cERR_VIDEO_MGR_NO_ERROR;
}

static int Command(void  *_context, ManagerCmd_t command, void * argument) 
{
    	Context_t  *context = (Context_t*) _context;
    	int ret = cERR_VIDEO_MGR_NO_ERROR;

    	video_mgr_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

    	switch(command) 
	{
    		case MANAGER_ADD: 
		{
			Track_t * track = argument;
			ret = ManagerAdd(context, *track);
			break;
    		}

		case MANAGER_LIST: {
			*((char***)argument) = (char **)ManagerList(context);
			break;
		}

	    	case MANAGER_GET: 
		{
			if ((TrackCount > 0) && (CurrentTrack >=0))
		    		*((int*)argument) = (int)Tracks[CurrentTrack].Id;
			else
		    		*((int*)argument) = (int)-1;
			break;
	    	}

	    	case MANAGER_GET_TRACK: 
	    	{
			video_mgr_printf(20, "%s::%s MANAGER_GET_TRACK\n", FILENAME, __FUNCTION__);

			if ((TrackCount > 0) && (CurrentTrack >=0))
		    		*((Track_t**)argument) = (Track_t*) &Tracks[CurrentTrack];
			else
		    		*((Track_t**)argument) = NULL;
			break;
	    	}

   		case MANAGER_GETENCODING: 
		{
        		if ((TrackCount > 0) && (CurrentTrack >=0))
            			*((char**)argument) = (char *)strdup(Tracks[CurrentTrack].Encoding);
        		else
            			*((char**)argument) = (char *)strdup("");
        		break;
    		}

    		case MANAGER_GETNAME: 
		{
			if ((TrackCount > 0) && (CurrentTrack >=0))
			    *((char**)argument) = (char *)strdup(Tracks[CurrentTrack].Name);
			else
			    *((char**)argument) = (char *)strdup("");
			break;
    		}

	    	case MANAGER_SET: 
		{
			int id = (int) argument;

			if (id < TrackCount)
			    	CurrentTrack = id;
			else
			{
			    video_mgr_err("%s::%s track id out of range (%d - %d)\n", FILENAME, __FUNCTION__, id, TrackCount);
			    ret = cERR_VIDEO_MGR_ERROR;
			}
			break;
	    	}

    		case MANAGER_DEL: 
		{
        		ret = ManagerDel(context);
        		break;
    		}

    		default:
        		video_mgr_err("%s::%s ContainerCmd %d not supported!\n", FILENAME, __FUNCTION__, command);
        		ret = cERR_VIDEO_MGR_ERROR;
        		break;
    	}

    	video_mgr_printf(10, "%s:%s: returning %d\n", FILENAME, __FUNCTION__,ret);

    	return ret;
}


struct Manager_s VideoManager = {
    	"Video",
    	&Command,
    	NULL
};

