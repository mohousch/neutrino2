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
	if (_buffer)
	{
		msync(_buffer, m_available, MS_SYNC);
		munmap(_buffer, m_available);
		_buffer = 0;
	}
	
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

	calcRamp();
	getMode();
	setMode(m_xRes, m_yRes, m_bpp);
	enableManualBlit();

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

int CTFTLCD::setMode(int nxRes, int nyRes, int nbpp)
{
	m_screeninfo.xres_virtual = m_screeninfo.xres = nxRes;
	m_screeninfo.yres_virtual = (m_screeninfo.yres = nyRes) * 2;
	m_screeninfo.height = 0;
	m_screeninfo.width = 0;
	m_screeninfo.xoffset = m_screeninfo.yoffset = 0;
	m_screeninfo.bits_per_pixel = nbpp;

	switch (nbpp) 
	{
		case 16:
			// ARGB 1555
			m_screeninfo.transp.offset = 15;
			m_screeninfo.transp.length = 1;
			m_screeninfo.red.offset = 10;
			m_screeninfo.red.length = 5;
			m_screeninfo.green.offset = 5;
			m_screeninfo.green.length = 5;
			m_screeninfo.blue.offset = 0;
			m_screeninfo.blue.length = 5;
			break;
		case 32:
			// ARGB 8888
			m_screeninfo.transp.offset = 24;
			m_screeninfo.transp.length = 8;
			m_screeninfo.red.offset = 16;
			m_screeninfo.red.length = 8;
			m_screeninfo.green.offset = 8;
			m_screeninfo.green.length = 8;
			m_screeninfo.blue.offset = 0;
			m_screeninfo.blue.length = 8;
			break;
	}

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &m_screeninfo) < 0)
	{
		// try single buffering
		m_screeninfo.yres_virtual = m_screeninfo.yres=nyRes;

		if (ioctl(fd, FBIOPUT_VSCREENINFO, &m_screeninfo) < 0)
		{
			printf("CTFTLCD::setMode: FBIOPUT_VSCREENINFO: %m\m\n");
			return -1;
		}
		printf("CTFTLCD::setMode: double buffering not available\n");
	}
	else
		printf("CTFTLCD::setMode: double buffering available\n");

	ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo);

	if ((m_screeninfo.xres != (unsigned int)nxRes) || (m_screeninfo.yres != (unsigned int)nyRes) ||
		(m_screeninfo.bits_per_pixel != (unsigned int)nbpp))
	{
		printf("CTFTLCD::setMode: failed: wanted: %dx%dx%d, got %dx%dx%d\n",
			nxRes, nyRes, nbpp,
			m_screeninfo.xres, m_screeninfo.yres, m_screeninfo.bits_per_pixel);
	}
	m_xRes = m_screeninfo.xres;
	m_yRes = m_screeninfo.yres;
	m_bpp = m_screeninfo.bits_per_pixel;
	
	fb_fix_screeninfo fix;
	
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		printf("[eFbLCD] FBIOGET_FSCREENINFO: %m\n");
	}
	_stride = fix.line_length;
	memset(_buffer, 0, _stride * m_yRes);
	
	update();
	
	return 0;
}

void CTFTLCD::getMode()
{
	m_xRes = m_screeninfo.xres;
	m_yRes = m_screeninfo.yres;
	m_bpp = m_screeninfo.bits_per_pixel;
}

int CTFTLCD::waitVSync()
{
	int c = 0;
	return ioctl(fd, FBIO_WAITFORVSYNC, &c);
}

void CTFTLCD::update() // blit
{
	if (m_manual_blit == 1)
	{
		if (ioctl(fd, FBIO_BLIT) < 0)
			printf("[eFbLCD] FBIO_BLIT: %m\n");
	}
}

int CTFTLCD::putCMAP()
{
	return ioctl(fd, FBIOPUTCMAP, &m_cmap);
}

int CTFTLCD::lock()
{
	if (locked)
		return -1;
	if (m_manual_blit == 1)
	{
		locked = 2;
		disableManualBlit();
	}
	else
		locked = 1;
		
	return fd;
}

void CTFTLCD::unlock()
{
	if (!locked)
		return;
		
	if (locked == 2)  // re-enable manualBlit
		enableManualBlit();
		
	locked = 0;
	setMode(m_xRes, m_yRes, m_bpp);
//	putCMAP();
}

void CTFTLCD::calcRamp()
{
	for (int i = 0; i < 256; i++)
	{
		int d;
		d = i;
		d = (d-128)*(m_gamma + 64)/(128 + 64) + 128;
		d += m_brightness - 128; // brightness correction
		if (d < 0)
			d = 0;
		if (d > 255)
			d = 255;
		m_ramp[i] = d;

		m_rampalpha[i] = i*m_alpha/256;
	}

	m_rampalpha[255] = 255; // transparent BLEIBT bitte so.
}

void CTFTLCD::enableManualBlit()
{
	unsigned char tmp = 1;
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0)
		printf("CTFTLCD::enableManualBlit: FBIO_SET_MANUAL_BLIT: %m\n");
	else
		m_manual_blit = 1;
}

void CTFTLCD::disableManualBlit()
{
	unsigned char tmp = 0;
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0)
		printf("CTFTLCD::disableManualBlit: FBIO_SET_MANUAL_BLIT: %m");
	else
		m_manual_blit = 0;
}

int CTFTLCD::setLCDBrightness(int brightness)
{
	FILE *f = fopen("/proc/stb/lcd/oled_brightness", "w");
	if (f)
	{
		if (fprintf(f, "%d", brightness) == 0)
			printf("CTFTLCD::setLCDBrightness: write /proc/stb/lcd/oled_brightness failed: %m\n");
		fclose(f);
	}
	return 0;
}

