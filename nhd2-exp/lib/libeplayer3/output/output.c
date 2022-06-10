/*
 * Output handling.
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
#include "common.h"
#include "output.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define OUTPUT_DEBUG

#ifdef OUTPUT_DEBUG

static short debug_level = 10;

#define output_printf(level, x...) do { \
if (debug_level >= level) printf(x); } while (0)
#else
#define output_printf(level, x...)
#endif

#ifndef OUTPUT_SILENT
#define output_err(x...) do { printf(x); } while (0)
#else
#define output_err(x...)
#endif

/* Error Constants */
#define cERR_OUTPUT_NO_ERROR         0
#define cERR_OUTPUT_INTERNAL_ERROR   -1

static const char* FILENAME = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */

static void printOutputCapabilities() 
{
	int i, j;

	output_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);
	output_printf(10, "Capabilities:\n");

	for (i = 0; AvailableOutput[i] != NULL; i++)	
	{
		output_printf(10, "\t%s : ", AvailableOutput[i]->Name);

		for (j = 0; AvailableOutput[i]->Capabilities[j] != NULL; j++)
			output_printf(10, "%s ", AvailableOutput[i]->Capabilities[j]);
		output_printf(10, "\n");
	}
}

/* ***************************** */
/* Output Functions              */
/* ***************************** */

static void OutputAdd(Context_t  *context, char * port) 
{
	int i, j;

	output_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

	for (i = 0; AvailableOutput[i] != NULL; i++)
	{
		for (j = 0; AvailableOutput[i]->Capabilities[j] != NULL; j++)
		{
			if (!strcmp(AvailableOutput[i]->Capabilities[j], port)) 
			{
				if (!strcmp("audio", port))
					context->output->audio = AvailableOutput[i];
				else if (!strcmp("video", port))
					context->output->video = AvailableOutput[i];	
				else if (!strcmp("subtitle", port))
					context->output->subtitle = AvailableOutput[i];	
				break;
			}
		}
	}
}

static void OutputDel(Context_t  *context, char * port) 
{
	output_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

	if (!strcmp("audio", port))
		context->output->audio = NULL;
	else if (!strcmp("video", port))
		context->output->video = NULL;   
	else if (!strcmp("subtitle", port))
		context->output->subtitle = NULL;
}

static int Command(void  *_context, OutputCmd_t command, void * argument) 
{
	Context_t  *context = (Context_t*) _context;
	int ret = cERR_OUTPUT_NO_ERROR;

	output_printf(10, "%s::%s Command %d\n", FILENAME, __FUNCTION__, command);

	switch(command) 
	{
		case OUTPUT_OPEN: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_OPEN, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_OPEN, "audio");	    
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_OPEN, "subtitle");    
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_CLOSE: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_CLOSE, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_CLOSE, "audio");	    
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_CLOSE, "subtitle");	    
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_ADD: 
		{
			OutputAdd(context, (char*) argument);
			break;
		}
		
		case OUTPUT_DEL: 
		{
			OutputDel(context, (char*) argument);
			break;
		}
		
		case OUTPUT_CAPABILITIES: 
		{
			printOutputCapabilities();
			break;
		}
		
		case OUTPUT_PLAY: 
		{ // 4
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret = context->output->video->Command(context, OUTPUT_PLAY, "video");

				if (!ret) 
				{	// success or not executed, dunn care
					if (context->playback->isAudio)
						ret = context->output->audio->Command(context, OUTPUT_PLAY, "audio");
					//if (!ret) 
					//{	// success or not executed, dunn care
					//	if (context->playback->isSubtitle)
					//		ret = context->output->subtitle->Command(context, OUTPUT_PLAY, "subtitle");
					//}                
				}
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_STOP: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_STOP, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_STOP, "audio");	    
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_STOP, "subtitle");    
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_FLUSH: {
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_FLUSH, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_FLUSH, "audio");
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_FLUSH, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_PAUSE: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_PAUSE, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_PAUSE, "audio");
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_PAUSE, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_FASTFORWARD: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_FASTFORWARD, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_FASTFORWARD, "audio");
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_PAUSE, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_REVERSE: {
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_REVERSE, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_REVERSE, "audio");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_CONTINUE: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_CONTINUE, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_CONTINUE, "audio");
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_CONTINUE, "subtitle");
			} else
			    ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_AVSYNC: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo && context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_AVSYNC, "audio");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_CLEAR: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo && (argument == NULL || *(char *) argument == 'v'))
					ret |= context->output->video->Command(context, OUTPUT_CLEAR, "video");
				if (context->playback->isAudio && (argument == NULL || *(char *) argument == 'a'))
					ret |= context->output->audio->Command(context, OUTPUT_CLEAR, "audio");
				//if (context->playback->isSubtitle && (argument == NULL || *(char *) argument == 's'))
				//	ret |= context->output->subtitle->Command(context, OUTPUT_CLEAR, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_PTS: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					return context->output->video->Command(context, OUTPUT_PTS, argument);
				if (context->playback->isAudio)
					return context->output->audio->Command(context, OUTPUT_PTS, argument);
				//if (context->playback->isSubtitle)
				//	return context->output->subtitle->Command(context, OUTPUT_PTS, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_SWITCH: 
		{
			if (context && context->playback ) 
			{
			    if (context->playback->isAudio)
				    return context->output->audio->Command(context, OUTPUT_SWITCH, "audio");
			    if (context->playback->isVideo)
				    return context->output->video->Command(context, OUTPUT_SWITCH, "video");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_SLOWMOTION: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_SLOWMOTION, "video");
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_SLOWMOTION, "audio");
				//if (context->playback->isSubtitle)
				//	ret |= context->output->subtitle->Command(context, OUTPUT_PAUSE, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_AUDIOMUTE: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isAudio)
					ret |= context->output->audio->Command(context, OUTPUT_AUDIOMUTE, (char*) argument);
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_DISCONTINUITY_REVERSE: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					ret |= context->output->video->Command(context, OUTPUT_DISCONTINUITY_REVERSE, (void*) argument);
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		case OUTPUT_GET_FRAME_COUNT: 
		{
			if (context && context->playback ) 
			{
				if (context->playback->isVideo)
					return context->output->video->Command(context, OUTPUT_GET_FRAME_COUNT, argument);
				if (context->playback->isAudio)
					return context->output->audio->Command(context, OUTPUT_GET_FRAME_COUNT, argument);
				//if (context->playback->isSubtitle)
				//	return context->output->subtitle->Command(context, OUTPUT_GET_FRAME_COUNT, "subtitle");
			} 
			else
				ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
		}
		
		default:
			output_err("%s::%s OutputCmd %d not supported!\n", FILENAME, __FUNCTION__, command);
			ret = cERR_OUTPUT_INTERNAL_ERROR;
			break;
	}

	output_printf(10, "%s::%s exiting with value %d\n", FILENAME, __FUNCTION__, ret);

	return ret;
}

OutputHandler_t OutputHandler = {
	"Output",
	NULL,
	NULL,    
	NULL,   
	&Command
};
