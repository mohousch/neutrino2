/*
 * common.h
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
 
 #ifndef COMMON_H_
#define COMMON_H_

#include "container.h"
#include "output.h"
#include "manager.h"
#include "playback.h"
#include <pthread.h>

#include <config.h>


typedef struct Context_s 
{
	PlaybackHandler_t* playback;
	ContainerHandler_t* container;
	OutputHandler_t* output;
	ManagerHandler_t* manager;
} Context_t;

#ifdef USE_OPENGL
typedef struct Data_s
{
	uint8_t* buffer;
	int size;
	uint32_t width;
	uint32_t height;
	uint64_t vpts;
	float rate;
}Data_t;
#endif

#endif

