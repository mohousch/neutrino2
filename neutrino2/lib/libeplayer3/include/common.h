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
	uint64_t apts;
	uint32_t rate;
}Data_t;
#endif

#endif

