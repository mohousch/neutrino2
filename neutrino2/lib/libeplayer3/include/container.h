/*
 * container.h
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
 
 #ifndef CONTAINER_H_
#define CONTAINER_H_

#include <stdio.h>


typedef enum 
{ 
	CONTAINER_INIT,
	CONTAINER_INIT_SUB,
	CONTAINER_ADD, 
	CONTAINER_CAPABILITIES, 
	CONTAINER_PLAY,
	CONTAINER_PLAY_SUB,
	CONTAINER_STOP,
	CONTAINER_STOP_SUB, 
	CONTAINER_SEEK, 
	CONTAINER_LENGTH, 
	CONTAINER_DEL,  
	CONTAINER_INFO, 
	CONTAINER_STATUS, 
	CONTAINER_LAST_PTS, 
	CONTAINER_DATA
} ContainerCmd_t;

typedef struct Container_s 
{
	char * Name;
	int (* Command) (void  *, ContainerCmd_t, void *);
	char ** Capabilities;

} Container_t;

//
extern Container_t FFMPEGContainer;

static Container_t * AvailableContainer[] = 
{
	&FFMPEGContainer,
	NULL
};

typedef struct ContainerHandler_s 
{
	char * Name;
    	Container_t * selectedContainer;

    	int (* Command) (void  *, ContainerCmd_t, void *);
} ContainerHandler_t;

#endif

