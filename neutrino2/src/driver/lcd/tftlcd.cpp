/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: tftlcd.cpp 01062024 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
        baseroutines by Shadow_
	Homepage: http://dbox.cyberphoria.org/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <linux/kd.h>

#include <driver/lcd/tftlcd.h>


#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT _IOW('F', 0x21, __u8)
#define FBIO_BLIT 0x22
#endif

CTFTLCD::CTFTLCD()
{
	fd = -1;
	m_manual_blit = -1;
	locked = 0;
	_buffer = 0;
	m_available = 0;
	m_cmap.start = 0;
	m_cmap.len = 256;
	m_cmap.red = m_red;
	m_cmap.green = m_green;
	m_cmap.blue = m_blue;
	m_cmap.transp = m_trans;
	m_alpha = 255;
	m_gamma = 128;
	m_brightness = 128;
}

CTFTLCD::~CTFTLCD()
{
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

bool CTFTLCD::init(const char *fbdevice)
{
	fd = open(fbdevice, O_RDWR);
	
	if (fd < 0)
	{
		printf("CTFTLCD::init: %s: %m\n", fbdevice);
		goto nolfb;
	}

	if (::ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo) < 0)
	{
		printf("CTFTLCD::init: FBIOGET_VSCREENINFO: %m\n");
		goto nolfb;
	}

	fb_fix_screeninfo fix;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		printf("CTFTLCD::init: FBIOGET_FSCREENINFO: %m\n");
		goto nolfb;
	}

	m_available = fix.smem_len;
	m_phys_mem = fix.smem_start;
	
	printf("CTFTLCD::init: %s %dk video mem\n", fbdevice, m_available / 1024);
	
	_buffer = (unsigned char*)mmap(0, m_available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	
	if (!_buffer)
	{
		printf("CTFTLCD::init: mmap: %m\n");
		goto nolfb;
	}

//	calcRamp();
//	getMode();
//	setMode(m_xRes, m_yRes, m_bpp);
//	enableManualBlit();

	return true;
nolfb:
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
	
	printf("CTFTLCD::init: framebuffer %s not available\n", fbdevice);
	
	return false;
}


