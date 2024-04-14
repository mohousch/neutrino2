/*
 * manager handling.
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

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define MANAGER_DEBUG

#ifdef MANAGER_DEBUG

static short debug_level = 10;

#define manager_printf(level, x...) do { \
if (debug_level >= level) printf(x); } while (0)
#else
#define manager_printf(level, x...)
#endif

#ifndef MANAGER_SILENT
#define manager_err(x...) do { printf(x); } while (0)
#else
#define manager_err(x...)
#endif

/* Error Constants */
#define cERR_MANAGER_NO_ERROR         0
#define cERR_MANAGER_INTERNAL_ERROR   -1

static const char* FILENAME = __FILE__;

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

extern Manager_t AudioManager;
extern Manager_t VideoManager;
extern Manager_t SubtitleManager;
extern Manager_t ExtSubtitleManager;

ManagerHandler_t ManagerHandler = {
    	"ManagerHandler",
    	&AudioManager,
    	&VideoManager,    
    	&SubtitleManager,
    	&ExtSubtitleManager,    
};

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* Functions                     */
/* ***************************** */
void copyTrack(Track_t* to, Track_t* from)
{
    	*to = *from;

    	if (from->Name != NULL)
        	to->Name = strdup(from->Name);
    	else
        	to->Name = strdup("Unknown");

    	if (from->Encoding != NULL)
        	to->Encoding = strdup(from->Encoding);
    	else
        	to->Encoding = strdup("Unknown");

    	if (from->language != NULL)
        	to->language = strdup(from->language);
    	else
        	to->language = strdup("Unknown");
}

void freeTrack(Track_t* track)
{
    	if (track->Name != NULL)
        	free(track->Name);

    	if (track->Encoding != NULL)
        	free(track->Encoding);

    	if (track->language != NULL)
        	free(track->language);

    	if (track->aacbuf != NULL)
        	free(track->aacbuf);
}

