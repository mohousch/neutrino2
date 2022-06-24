/*
 * Main Container Handling.
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

#include <stdio.h>
#include <string.h>

#include "common.h"

#define CONTAINER_DEBUG

#ifdef CONTAINER_DEBUG

static short debug_level = 10;

#define container_printf(level, x...) do { \
if (debug_level >= level) printf(x); } while (0)
#else
#define container_printf(level, x...)
#endif

#ifndef CONTAINER_SILENT
#define container_err(x...) do { printf(x); } while (0)
#else
#define container_err(x...)
#endif


static const char FILENAME[] = __FILE__;

static void printContainerCapabilities() 
{
	int i, j;

	container_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);
	container_printf(10, "Capabilities: ");

	for (i = 0; AvailableContainer[i] != NULL; i++)
		for (j = 0; AvailableContainer[i]->Capabilities[j] != NULL; j++)
			container_printf(10, "%s ", AvailableContainer[i]->Capabilities[j]);
		
	 container_printf(10, "\n");
}

static int selectContainer(Context_t  *context, char * extension) 
{
	int i, j;
	int ret = -1;

	container_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

	for (i = 0; AvailableContainer[i] != NULL; i++)
	{
		for (j = 0; AvailableContainer[i]->Capabilities[j] != NULL; j++)
			if (!strcasecmp(AvailableContainer[i]->Capabilities[j], extension)) 
			{
				context->container->selectedContainer = AvailableContainer[i];

				container_printf(10, "Selected Container: %s\n", context->container->selectedContainer->Name);
				ret = 0;
				break;
			}
			
		if (ret == 0)
			break;
	}

	if (ret != 0) 
	{
		container_err("No Container found :-(\n");
	}

	return ret;
}


static int Command(void  *_context, ContainerCmd_t command, void * argument) 
{
	Context_t* context = (Context_t*) _context;
	int ret = 0;

	container_printf(10, "%s::%s\n", FILENAME, __FUNCTION__);

	switch(command) 
	{
		case CONTAINER_ADD: 
		{
			ret = selectContainer(context, (char*) argument);
			break;
		}
		
		case CONTAINER_CAPABILITIES: 
		{
			printContainerCapabilities();
			break;
		}
		
		case CONTAINER_DEL: 
		{
			context->container->selectedContainer = NULL;
			break;
		}
		
		default:
			container_err("%s::%s ContainerCmd %d not supported!\n", FILENAME, __FUNCTION__, command);
			break;
	}

	return ret;
}

extern Container_t SrtContainer;
extern Container_t SsaContainer;
//extern Container_t ASSContainer;

ContainerHandler_t ContainerHandler = {
	"Output",
	NULL,   
	&SrtContainer,
	&SsaContainer,
	/*&ASSContainer*/NULL,    
	Command,
};
