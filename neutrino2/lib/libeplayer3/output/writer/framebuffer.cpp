/*
 * framebuffer output/writer handling.
 *
 * This is a hacky implementation of a framebuffer output for the subtitling.
 * This is ment as a POV, later this should be implemented in enigma2 and
 * neutrino.
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
#include <sys/ioctl.h>
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "writer.h"

#include <config.h>
#include <src/driver/gfx/framebuffer.h>

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

#define FB_DEBUG

#ifdef FB_DEBUG

static short debug_level = 10;

#define fb_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define fb_printf(level, fmt, x...)
#endif

#ifndef FB_SILENT
#define fb_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define fb_err(fmt, x...)
#endif

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


/* ***************************** */
/* Writer Functions              */
/* ***************************** */
// stmfb
#ifdef __sh__
#include <linux/stmfb.h>
#endif

#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT _IOW('F', 0x21, __u8)
#define FBIO_BLIT 0x22
#endif

void blit()
{	
	CFrameBuffer::getInstance()->blit();
}

//
void blit2FB(void* fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp, uint32_t yp, bool transp )
{
	CFrameBuffer::getInstance()->blit2FB(fbbuff, width, height, xoff, yoff, xp, yp, transp);
}

//
static int reset()
{
    return 0;
}

static int writeData(void* _call)
{
	WriterFBCallData_t * call = (WriterFBCallData_t*) _call;
	
	fb_printf(100, "\n");

	if (call == NULL)
	{
		fb_err("call data is NULL...\n");
		return 0;
	}

	if (call->destination == NULL)
	{
		fb_err("file pointer < 0. ignoring ...\n");
		return 0;
	}
	
	if (call->data != NULL)
	{
		//
		blit2FB(call->data, call->Width, call->Height, call->x, call->y, 0, 0, false);
	}
	else
	{
		//CFrameBuffer::getInstance()->paintBackgroundBoxRel (call->x, call->y, call->Width, call->Height);
		CFrameBuffer::getInstance()->clearFrameBuffer();
	}
	
	blit();
    
	fb_printf(100, "<\n");
	
	return 0;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */
// framebuffer
static WriterCaps_t caps = {
	"framebuffer",
	eGfx,
	"framebuffer",
	0
};

struct Writer_s WriterFramebuffer = {
	&reset,
	&writeData,
	NULL,
	&caps,
};

