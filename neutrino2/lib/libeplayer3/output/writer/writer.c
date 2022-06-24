/*
 * linuxdvb output/writer handling.
 *
 * konfetti 2010
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
#include <errno.h>

#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */

//#define WRITER_DEBUG

#ifdef WRITER_DEBUG

static short debug_level = 10;

#define writer_printf(level, x...) do { \
if (debug_level >= level) printf(x); } while (0)
#else
#define writer_printf(level, x...)
#endif

#ifndef WRITER_SILENT
#define writer_err(x...) do { printf(x); } while (0)
#else
#define writer_err(x...)
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
/*  Functions                    */
/* ***************************** */

Writer_t* getWriter(char* encoding)
{
	int i;

	for (i = 0; AvailableWriter[i] != NULL; i++)
	{
		if (strcmp(AvailableWriter[i]->caps->textEncoding, encoding) == 0)
		{
			writer_printf(50, "%s: found writer \"%s\" for \"%s\"\n", __func__, AvailableWriter[i]->caps->name, encoding);
			return AvailableWriter[i];
		}
	}

	writer_printf(1, "%s: no writer found for \"%s\"\n", __func__, encoding);

	return NULL;
}

Writer_t* getDefaultVideoWriter()
{
	int i;

	for (i = 0; AvailableWriter[i] != NULL; i++)
	{
		if (strcmp(AvailableWriter[i]->caps->textEncoding, "V_MPEG2") == 0)
		{
			writer_printf(50, "%s: found writer \"%s\"\n", __func__, AvailableWriter[i]->caps->name);
			return AvailableWriter[i];
		}
	}

	writer_printf(1, "%s: no writer found\n", __func__);

	return NULL;
}

Writer_t* getDefaultAudioWriter()
{
	int i;

	for (i = 0; AvailableWriter[i] != NULL; i++)
	{
		if (strcmp(AvailableWriter[i]->caps->textEncoding, "A_MP3") == 0)
		{
			writer_printf(50, "%s: found writer \"%s\"\n", __func__, AvailableWriter[i]->caps->name);
			return AvailableWriter[i];
		}
	}

	writer_printf(1, "%s: no writer found\n", __func__);

	return NULL;
}

Writer_t* getDefaultFramebufferWriter()
{
	int i;

	for (i = 0; AvailableWriter[i] != NULL; i++)
	{
		writer_printf(10, "%s\n", AvailableWriter[i]->caps->textEncoding);
		if (strcmp(AvailableWriter[i]->caps->textEncoding, "framebuffer") == 0)
		{
			writer_printf(50, "%s: found writer \"%s\"\n", __func__, AvailableWriter[i]->caps->name);
			return AvailableWriter[i];
		}
	}

	writer_printf(1, "%s: no writer found\n", __func__);

	return NULL;
}

// used in mpeg2.c
ssize_t WriteExt(WriteV_t _call, int fd, void *data, size_t size)
{
	struct iovec iov[1];
	iov[0].iov_base = data;
	iov[0].iov_len = size;
	
	return _call(fd, iov, 1);
}

//
ssize_t write_with_retry(int fd, const void *buf, int size)
{
	ssize_t ret;
	int retval = 0;
	
	while (size > 0 && 0 == PlaybackDieNow(0))
	{
		ret = write(fd, buf, size);
		
		if (ret < 0)
		{
			switch (errno)
			{
				case EINTR:
				case EAGAIN:
					usleep(1000);
					continue;
				default:
					retval = -3;
					break;
			}
			if (retval < 0)
			{
				break;
			}
		}

		if (ret < 0)
		{
			return ret;
		}

		size -= ret;
		buf += ret;

		if (size > 0)
		{
			if (usleep(1000))
			{
				writer_err("usleep error \n");
			}
		}
	}
	
	return 0;
}

//
ssize_t writev_with_retry(int fd, const struct iovec *iov, int ic)
{
	ssize_t len = 0;
	int i = 0;
	
	for (i = 0; i < ic; ++i)
	{
		write_with_retry(fd, iov[i].iov_base, iov[i].iov_len);
		len += iov[i].iov_len;
		if (PlaybackDieNow(0))
		{
			return -1;
		}
	}
	
	return len;
}



