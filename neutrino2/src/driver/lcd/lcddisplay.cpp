/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: lcddisplay.cpp 31052024 mohousch Exp $

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

#include <driver/lcd/lcddisplay.h>

#include <png.h>
#include <lib/libngpng/libngpng.h>

#include <stdint.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <byteswap.h>
#include <string.h>
#include <sys/mman.h>
#include <memory.h>


#ifndef BYTE_ORDER
#error "no BYTE_ORDER defined!"
#endif

#ifndef max
#define max(a,b)(((a)<(b)) ? (b) : (a))
#endif

#ifndef min
#define min(a,b)(((a)<(b)) ? (a) : (b))
#endif

#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT _IOW('F', 0x21, __u8)
#define FBIO_BLIT 0x22
#endif

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, uint32_t)
#endif

CLCDDisplay::CLCDDisplay()
{
	printf("CLCDDisplay::CLCDDisplay\n");
	
	paused = 0;
	
	raw_buffer_size = 0;
	xres = 132;
	yres = 64; 
	bpp = 8;
	fd = -1;
	
	flipped = false;
	inverted = 0;
	lcd_type = 0;
	last_brightness = 0;
	
	locked = 0;
	
#ifdef ENABLE_TFTLCD
	m_manual_blit = -1;
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
#endif
}

CLCDDisplay::~CLCDDisplay()
{
#ifdef ENABLE_LCD
	if (_buffer)
	{
		delete [] _buffer;
		_buffer = NULL;
	}
#endif

#ifdef ENABLE_TFTLCD
	if (_buffer)
	{
		msync(_buffer, m_available, MS_SYNC);
		munmap(_buffer, m_available);
		_buffer = NULL;
	}
#endif

	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

bool CLCDDisplay::init(const char *fbdevice)
{
#ifdef ENABLE_LCD
	//open device
#ifdef USE_OPENGL
	fd = open("/dev/null", O_RDWR);
#else
	fd = open("/dev/dbox/oled0", O_RDWR);
#endif
	
	if (fd < 0)
	{
		xres = 128;
		if (!access("/proc/stb/lcd/oled_brightness", W_OK) || !access("/proc/stb/fp/oled_brightness", W_OK) )
			lcd_type = 2;
			
		fd = open("/dev/dbox/lcd0", O_RDWR);
	} 
	else
	{
		printf("found OLED display!\n");
		lcd_type = 1;
	}
	
	if (fd < 0)
	{
		printf("CLCDDisplay::CLCDDisplay: couldn't open LCD - load lcd.ko!\n");
		return false;
	}
	else
	{
		int i = LCD_MODE_BIN;
		
		ioctl(fd, LCD_IOCTL_ASC_MODE, &i);
		
		FILE *f = fopen("/proc/stb/lcd/xres", "r");
		
		if (f)
		{
			int tmp;
			if (fscanf(f, "%x", &tmp) == 1)
				xres = tmp;
			fclose(f);
			
			f = fopen("/proc/stb/lcd/yres", "r");
			
			if (f)
			{
				if (fscanf(f, "%x", &tmp) == 1)
					yres = tmp;
				fclose(f);
				
				f = fopen("/proc/stb/lcd/bpp", "r");
				if (f)
				{
					if (fscanf(f, "%x", &tmp) == 1)
						bpp = tmp;
					fclose(f);
				}
			}
			
			lcd_type = 3;
		}
	}
	
#ifdef USE_OPENGL
	setSize(220, 176, 16);
#else
	setSize(xres, yres, bpp);
#endif
	return true;
#endif

#ifdef ENABLE_TFTLCD
	fd = open(fbdevice, O_RDWR);
	
	if (fd < 0)
	{
		printf("CLCDDisplay::init: %s: %m\n", fbdevice);
		goto nolfb;
	}

	if (::ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo) < 0)
	{
		printf("CLCDDisplay::init: FBIOGET_VSCREENINFO: %m\n");
#ifndef USE_OPENGL
		goto nolfb;
#endif
	}

	fb_fix_screeninfo fix;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		printf("CLCDDisplay::init: FBIOGET_FSCREENINFO: %m\n");
#ifndef USE_OPENGL
		goto nolfb;
#endif
	}

	m_available = fix.smem_len;
	m_phys_mem = fix.smem_start;
	
	printf("CLCDDisplay::init: %s %dk video mem\n", fbdevice, m_available / 1024);
	
	_buffer = (uint8_t *)mmap(0, m_available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	
	if (!_buffer)
	{
		printf("CLCDDisplay::init: mmap: %m\n");
#ifndef USE_OPENGL
		goto nolfb;
#endif
	}

	calcRamp();
	getMode();
	setMode(m_xRes, m_yRes, m_bpp);
	enableManualBlit();
	
	lcd_type = 4;

	return true;
	
nolfb:
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
	
	printf("CTFTLCD::init: framebuffer %s not available\n", fbdevice);
	
	return false;
#endif
}

void CLCDDisplay::setSize(int w, int h, int b)
{
	printf("CLCDDisplay::setSize: xres=%d, yres=%d, bpp=%d type=%d\n", w, h, b, lcd_type);
	
	//
	xres = w;
	yres = h;
	bpp = b;
	
	//
	surface_bpp = b;
	
	real_offset = 0;
	real_yres = yres;
	
	if (yres == 32)
		real_offset = 16;
		
	if (yres < 64)
		yres = 48;

	switch (surface_bpp)
	{
		case 8:
			surface_bypp = 1;
			break;
			
		case 15:
		case 16:
			surface_bypp = 2;
			break;
			
		case 24:		// never use 24bit mode
		case 32:
			surface_bypp = 4;
			break;
			
		default:
			surface_bypp = (bpp + 7)/8;
	}

	// surface
	surface_stride = xres*surface_bypp;
	surface_buffer_size = xres * yres * surface_bypp;
	surface_data = new uint8_t[surface_buffer_size];
	memset(surface_data, 0, surface_buffer_size);

	// 
	_stride = xres*sizeof(uint8_t);
	raw_buffer_size = xres * yres * sizeof(uint8_t);
	_buffer = new uint8_t[raw_buffer_size];
	memset(_buffer, 0, raw_buffer_size);
}

#ifdef ENABLE_TFTLCD
int CLCDDisplay::setMode(int nxRes, int nyRes, int nbpp)
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
			printf("CLCDDisplay::setMode: FBIOPUT_VSCREENINFO: %m\m\n");
			return -1;
		}
		printf("CLCDDisplay::setMode: double buffering not available\n");
	}
	else
		printf("CLCDDisplay::setMode: double buffering available\n");

	ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo);

	if ((m_screeninfo.xres != (unsigned int)nxRes) || (m_screeninfo.yres != (unsigned int)nyRes) ||
		(m_screeninfo.bits_per_pixel != (unsigned int)nbpp))
	{
		printf("CLCDDisplay::setMode: failed: wanted: %dx%dx%d, got %dx%dx%d\n",
			nxRes, nyRes, nbpp,
			m_screeninfo.xres, m_screeninfo.yres, m_screeninfo.bits_per_pixel);
	}
	m_xRes = m_screeninfo.xres;
	m_yRes = m_screeninfo.yres;
	m_bpp = m_screeninfo.bits_per_pixel;
	
	fb_fix_screeninfo fix;
	
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		printf("CLCDDisplay::setMode: FBIOGET_FSCREENINFO: %m\n");
	}
	_stride = fix.line_length;
	memset(_buffer, 0, _stride * m_yRes);
	
	update();
	
	return 0;
}

void CLCDDisplay::getMode()
{
	m_xRes = m_screeninfo.xres;
	m_yRes = m_screeninfo.yres;
	m_bpp = m_screeninfo.bits_per_pixel;
}

int CLCDDisplay::waitVSync()
{
	int c = 0;
	return ioctl(fd, FBIO_WAITFORVSYNC, &c);
}

int CLCDDisplay::putCMAP()
{
	return ioctl(fd, FBIOPUTCMAP, &m_cmap);
}

int CLCDDisplay::lock()
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

void CLCDDisplay::unlock()
{
	if (!locked)
		return;
		
	if (locked == 2)  // re-enable manualBlit
		enableManualBlit();
		
	locked = 0;
	setMode(m_xRes, m_yRes, m_bpp);
	putCMAP();
}

void CLCDDisplay::calcRamp()
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

void CLCDDisplay::enableManualBlit()
{
	unsigned char tmp = 1;
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0)
		printf("CLCDDisplay::enableManualBlit: FBIO_SET_MANUAL_BLIT: %m\n");
	else
		m_manual_blit = 1;
}

void CLCDDisplay::disableManualBlit()
{
	unsigned char tmp = 0;
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0)
		printf("CLCDDisplay::disableManualBlit: FBIO_SET_MANUAL_BLIT: %m");
	else
		m_manual_blit = 0;
}
#endif

//
void CLCDDisplay::setInverted(unsigned char inv)
{
	inverted = inv;
	update();
}

void CLCDDisplay::setFlipped(bool onoff)
{
	flipped = onoff;
	update();
}

int CLCDDisplay::setLCDContrast(int contrast)
{
	int fp;

	fp = open("/dev/dbox/fp0", O_RDWR);

	if (fp < 0)
		fp = open("/dev/dbox/lcd0", O_RDWR);
		
	if (fp < 0)
	{
		printf("CLCDDisplay::setLCDContrast: can't open /dev/dbox/fp0(%m)\n");
		return (-1);
	}
	
	if(ioctl(fd, LCD_IOCTL_SRV, &contrast) < 0)
	{
		printf("CLCDDisplay::setLCDContrast: can't set lcd contrast(%m)\n");
	}
	
	close(fp);

	return(0);
}

int CLCDDisplay::setLCDBrightness(int brightness)
{
	printf("CLCDDisplay::setLCDBrightness: %d\n", brightness);
	
#ifdef ENABLE_LCD
	FILE *f = fopen("/proc/stb/lcd/oled_brightness", "w");
	if (!f)
		f = fopen("/proc/stb/fp/oled_brightness", "w");
	if (f)
	{
		if (fprintf(f, "%d", brightness) == 0)
			printf("write /proc/stb/lcd/oled_brightness failed!! (%m)\n");
		fclose(f);
	}
	else
	{

		int fp;
		if ((fp = open("/dev/dbox/fp0", O_RDWR)) < 0)
		{
			printf("CLCDDisplay::setLCDBrightness: can't open /dev/dbox/fp0\n");
			return (-1);
		}


		if(ioctl(fd, FP_IOCTL_LCD_DIMM, &brightness) < 0)
			printf("CLCDDisplay::setLCDBrightness: can't set lcd brightness (%m)\n");			
		close(fp);
	}
	
	if (brightness == 0)
	{
		memset(_buffer, inverted, raw_buffer_size);
		update();
	}

	last_brightness = brightness;
#endif

#ifdef ENABLE_TFTLCD
	FILE *f = fopen("/proc/stb/lcd/oled_brightness", "w");
	
	if (f)
	{
		if (fprintf(f, "%d", brightness) == 0)
			printf("CLCDDisplay::setLCDBrightness: write /proc/stb/lcd/oled_brightness failed: %m\n");
		fclose(f);
	}
#endif

	return(0);
}

int CLCDDisplay::setLED(int value, int option)
{
	switch (option)
	{
		case LED_BRIGHTNESS:
			printf("CLCDDisplay::setLED: NormalState %d\n", value);
			
			if (ioctl(fd, LED_IOCTL_BRIGHTNESS_NORMAL, (unsigned char)value) < 0)
				printf("CLCDDisplay::setLED: can't set led brightness\n");
			break;
			
		case LED_DEEPSTANDBY:
			printf("CLCDDisplay::setLED: linkingTime %d\n", value);
			
			if (ioctl(fd, LED_IOCTL_BRIGHTNESS_DEEPSTANDBY, (unsigned char)value) < 0)
				printf("CLCDDisplay::setLED: can't set led deep standby\n");
			break;
			
		case LED_BLINKINGTIME:
			printf("CLCDDisplay::setLED: BlinkingTime %d\n", value);
			
			if (ioctl(fd, LED_IOCTL_BLINKING_TIME, (unsigned char)value) < 0)
				printf("CLCDDisplay::setLED: can't set led blinking time\n");
			break;
	}
	
	return 0;
}

void CLCDDisplay::pause()
{
	paused = 1;
}

void CLCDDisplay::resume()
{
	//clear the display
	if( ioctl(fd, LCD_IOCTL_CLEAR) < 0 )
		printf("CLCDDisplay::resume: LCD_IOCTL_CLEAR failed (%m)\n");
	
	//graphic (binary) mode 
	int i = LCD_MODE_BIN;
	if( ioctl(fd, LCD_IOCTL_ASC_MODE, &i) < 0 )
		printf("CLCDDisplay::resume: LCD_IOCTL_ASC_MODE failed (%m)\n");
	
	paused = 0;
}

void CLCDDisplay::convert_data()
{
}

void CLCDDisplay::update()
{
#ifdef ENABLE_LCD
	if ((fd >= 0) && (last_brightness > 0))
	{
		// blit2LCD
		for (unsigned int y = 0; y < yres; y++)
		{
			for (unsigned int x = 0; x < xres; x++)
			{
				blit2LCD(x, y, x + 1, y + 1, _buffer[(y * xres + x)]);
			}
		}

		// blit
		if (lcd_type == 0 || lcd_type == 2)
		{
			unsigned int height = yres;
			unsigned int width = xres;

			// hack move last line to top
			uint8_t linebuffer[width];
			memmove(linebuffer, surface_data + surface_buffer_size - width, width);
			memmove(surface_data + width, surface_data, surface_buffer_size - width);
			memmove(surface_data, linebuffer, width);

			uint8_t raw[132*8];
			int x, y, yy;
			
			memset(raw, 0x00, 132*8);
			
			for (y = 0; y < 8; y++)
			{
				// display has only 128 but buffer must be 132
				for (x = 0; x < 128; x++)
				{
					int pix = 0;
					
					for (yy = 0; yy < 8; yy++)
					{
						pix |= (surface_data[(y*8 + yy)*width + x] >= 108)<<yy;
					}
					
					if (flipped)
					{
						/* 8 pixels per byte, swap bits */
#define BIT_SWAP(a) (( ((a << 7)&0x80) + ((a << 5)&0x40) + ((a << 3)&0x20) + ((a << 1)&0x10) + ((a >> 1)&0x08) + ((a >> 3)&0x04) + ((a >> 5)&0x02) + ((a >> 7)&0x01) )&0xff)
						raw[(7 - y) * 132 + (132-1 - x - 2)] = BIT_SWAP(pix ^ inverted);
					}
					else
					{
						raw[y * 132 + x + 2] = pix ^ inverted;
					}
				}
			}
			
			write(fd, raw, 132*8);
			
			// hack move last line back to bottom
			memmove(linebuffer, surface_data, width);
			memmove(surface_data, surface_data + width, surface_buffer_size - width);
			memmove(surface_data + surface_buffer_size - width, linebuffer, width);
		}
		else if (lcd_type == 3)
		{
			// for now, only support flipping / inverting for 8bpp displays
			if ((flipped || inverted) && surface_stride == xres)
			{
				unsigned int height = yres;
				unsigned int width = xres;
				uint8_t raw[surface_stride * height];
				
				for (unsigned int y = 0; y < height; y++)
				{
					for (unsigned int x = 0; x < width; x++)
					{
						if (flipped)
						{
							/* 8bpp, no bit swapping */
							raw[(height - 1 - y) * width + (width - 1 - x)] = surface_data[y * width + x] ^ inverted;
						}
						else
						{
							raw[y * width + x] = surface_data[y * width + x] ^ inverted;
						}
					}
				}
				
				write(fd, raw, surface_stride * height);
			}
			else
			{
				// LCD_COLOR_BITORDER_RGB565
#if defined (PLATFORM_DREAMBOX)		
				// gggrrrrrbbbbbggg bit order from memory
				// gggbbbbbrrrrrggg bit order to LCD
				uint8_t gb_buffer[surface_stride * yres];
				
				if (!(0x03 & (surface_stride * yres)))
				{ 
					// fast
					for (int offset = 0; offset < ((surface_stride * yres) >> 2); offset++)
					{
						unsigned int src = ((unsigned int *)surface_data)[offset];
						((unsigned int *)gb_buffer)[offset] = src & 0xE007E007 | (src & 0x1F001F00) >> 5 | (src & 0x00F800F8) << 5;
					}
				}
				else
				{ 
					// slow
					for (int offset = 0; offset < surface_stride * yres; offset += 2)
					{
						gb_buffer[offset] = (surface_data[offset] & 0x07) | ((surface_data[offset + 1] << 3) & 0xE8);
						gb_buffer[offset + 1] = (surface_data[offset + 1] & 0xE0) | ((surface_data[offset] >> 3) & 0x1F);
					}
				}
				
				write(fd, gb_buffer, surface_stride * yres);
#elif defined (PLATFORM_GIGABLUE)
				uint8_t gb_buffer[surface_stride * yres];
				
				for (int offset = 0; offset < surface_stride * yres; offset += 2)
				{
					gb_buffer[offset] = (surface_data[offset] & 0x1F) | ((surface_data[offset + 1] << 3) & 0xE0);
					gb_buffer[offset + 1] = ((surface_data[offset + 1] >> 5) & 0x03) | ((surface_data[offset] >> 3) & 0x1C) | ((_buffer[offset + 1] << 5) & 0x60);
				}
				
				write(fd, gb_buffer, surface_stride * yres);
#else
				write(fd, surface_data + surface_stride * real_offset, surface_stride * real_yres);
#endif				
			}
		}
		else /* lcd_type == 1 */
		{
			uint8_t raw[64*64];
			int x, y;
			memset(raw, 0, 64*64);
			
			for (y = 0; y < 64; y++)
			{
				int pix = 0;
				for (x = 0; x < 128 / 2; x++)
				{
					pix = (surface_data[y*132 + x * 2 + 2] & 0xF0) |(surface_data[y*132 + x * 2 + 1 + 2] >> 4);
					if (inverted)
						pix = 0xFF - pix;
						
					if (flipped)
					{
						/* device seems to be 4bpp, swap nibbles */
						uint8_t byte;
						byte = (pix >> 4) & 0x0f;
						byte |= (pix << 4) & 0xf0;
						raw[(63 - y) * 64 + (63 - x)] = byte;
					}
					else
					{
						raw[y * 64 + x] = pix;
					}
				}
			}
			
			write(fd, raw, 64*64);
		}
	}
#endif

#ifdef ENABLE_TFTLCD
	if (m_manual_blit == 1)
	{
		if (ioctl(fd, FBIO_BLIT) < 0)
			printf("[eFbLCD] FBIO_BLIT: %m\n");
	}
#endif
}

// blit2lcd
void CLCDDisplay::blit2LCD(int area_left, int area_top, int area_right, int area_bottom, int color) 
{
	int area_width  = area_right - area_left;
	int area_height = area_bottom - area_top;

	if (surface_bpp == 8)
	{
		for (int y = area_top; y < area_bottom; y++)
		 	memset(((uint8_t*)surface_data) + y*surface_stride + area_left, color, area_width);
	} 
	else if (surface_bpp == 16)
	{
		uint32_t icol;

		icol = 0x10101*color;
		
#if BYTE_ORDER == LITTLE_ENDIAN
		uint16_t col = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
		uint16_t col = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
		for (int y = area_top; y < area_bottom; y++)
		{
			uint16_t *dst = (uint16_t*)(((uint8_t*)surface_data) + y*surface_stride + area_left*surface_bypp);
			int x = area_width;
			while (x--)
				*dst++=col;
		}
	} 
	else if (surface_bpp == 32)
	{
		uint32_t col;

		col = 0x10101*color;
			
		col ^= 0xFF000000;

		for (int y = area_top; y < area_bottom; y++)
		{
			uint32_t *dst = (uint32_t*)(((uint8_t*)surface_data) + y*surface_stride + area_left*surface_bypp);
			int x = area_width;
			while (x--)
				*dst++=col;
		}
	}	
}

void CLCDDisplay::draw_point(const int x, const int y, const int state)
{
	if ((x < 0) || (x >= xres) || (y < 0) || (y >= yres))
		return;

	if (state == LCD_PIXEL_INV)
		_buffer[(y * xres + x)] ^= 1;
	else
		_buffer[(y * xres + x)*sizeof(uint8_t)] = state;
}

void CLCDDisplay::draw_line(const int x1, const int y1, const int x2, const int y2, const int state)  
{
	int dx = abs (x1 - x2);
	int dy = abs (y1 - y2);
	int x;
	int y;
	int End;
	int step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( x1 > x2 )
		{
			x = x2;
			y = y2;
			End = x1;
			step = y1 < y2 ? -1 : 1;
		}
		else
		{
			x = x1;
			y = y1;
			End = x2;
			step = y2 < y1 ? -1 : 1;
		}

		draw_point(x, y, state);

		while( x < End )
		{
			x++;
			if ( p < 0 )
				p += twoDy;
			else
			{
				y += step;
				p += twoDyDx;
			}
			draw_point(x, y, state);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( y1 > y2 )
		{
			x = x2;
			y = y2;
			End = y1;
			step = x1 < x2 ? -1 : 1;
		}
		else
		{
			x = x1;
			y = y1;
			End = y2;
			step = x2 < x1 ? -1 : 1;
		}

		draw_point(x, y, state);

		while( y < End )
		{
			y++;
			if ( p < 0 )
				p += twoDx;
			else
			{
				x += step;
				p += twoDxDy;
			}
			draw_point(x, y, state);
		}
	}
}

void CLCDDisplay::draw_fill_rect(int left, int top, int right, int bottom, int state) 
{
	int x, y;
	
	for(x = left + 1; x < right; x++) 
	{  
		for(y = top + 1; y < bottom; y++) 
		{
			draw_point(x, y, state);
		}
	}
}

void CLCDDisplay::draw_rectangle(int left,int top, int right, int bottom, int linestate,int fillstate)
{
	// coordinate checking in draw_pixel (-> you can draw lines only
	// partly on screen)

	draw_line(left, top, right, top, linestate);
	draw_line(left, top, left, bottom, linestate);
	draw_line(right, top, right, bottom, linestate);
	draw_line(left, bottom, right, bottom, linestate);
	draw_fill_rect(left, top, right, bottom, fillstate);  
}  

void CLCDDisplay::draw_polygon(int num_vertices, int *vertices, int state) 
{
	// coordinate checking in draw_pixel (-> you can draw polygons only
	// partly on screen)

	int i;
	for(i = 0; i < num_vertices - 1; i++) 
	{
		draw_line(vertices[(i<<1)+0],
			vertices[(i<<1)+1],
			vertices[(i<<1)+2],
			vertices[(i<<1)+3],
			state);
	}
   
	draw_line(vertices[0],
		vertices[1],
		vertices[(num_vertices<<1)-2],
		vertices[(num_vertices<<1)-1],
		state);
}

void CLCDDisplay::clear_screen() 
{
	memset(_buffer, 0, raw_buffer_size);
}

void CLCDDisplay::dump_screen(uint8_t **screen) 
{
	memmove(*screen, _buffer, raw_buffer_size);
}

void CLCDDisplay::load_screen_element(raw_lcd_element_t * element, int left, int top, int width, int height) 
{
	printf("CLCDDisplay::load_screen_element: %s\n", element->name.c_str());
	
	if ((element->buffer) && (element->height <= yres - top))
	{
		for (unsigned int i = 0; i < min(element->height, yres - top); i++)
		{	
			memmove(_buffer + ((top + i)*xres) + left, element->buffer + (i*element->width), min(element->width, xres - left));
		}
	}
}

void CLCDDisplay::load_screen(uint8_t **const screen) 
{
	raw_lcd_element_t element;
	
	element.buffer = *screen;
	element.width = xres;
	element.height = yres;
	
	load_screen_element(&element, 0, 0);
}

bool CLCDDisplay::load_png_element(const char * const filename, raw_lcd_element_t * element)
{
	bool ret_value = false;

	png_structp  png_ptr;
	png_infop    info_ptr;
	unsigned int i;
	unsigned int pass;
	unsigned int number_passes;
	int          bit_depth;
	int          color_type;
	int          interlace_type;
	png_uint_32  width;
	png_uint_32  height;
	png_byte *   fbptr;
	FILE *       fh;
	int channels;
	int trns;
	
	element->name = filename;

	if ((fh = fopen(filename, "rb")))
	{
		if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
		{
			if (!(info_ptr = png_create_info_struct(png_ptr)))
				png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
			else
			{
#if (PNG_LIBPNG_VER < 10500)
				if (!(setjmp(png_ptr->jmpbuf)))
#else
				if (!setjmp(png_jmpbuf(png_ptr)))
#endif
				{
					unsigned int lcd_height = yres;
					unsigned int lcd_width = xres;

					png_init_io(png_ptr, fh);
					
					png_read_info(png_ptr, info_ptr);
					png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
					channels = png_get_channels(png_ptr, info_ptr);
					trns = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);
					
					/*if (
						((color_type == PNG_COLOR_TYPE_PALETTE) ||
						 ((color_type & PNG_COLOR_MASK_COLOR) == PNG_COLOR_TYPE_GRAY)) &&
						(bit_depth  <= 8                     ) &&
						(width      <= lcd_width             ) &&
						(height     <= lcd_height            )
						)
					*/
					{
						printf("CLCDDisplay::load_png_element: %s %dx%dx%d, type %d channel %d trans %d\n", filename, width, height, bit_depth, color_type, channels, trns);
						
						element->width = width;
						element->height = height;
						element->bpp = bit_depth;
						
						if (!element->buffer)
						{
							element->buffer = new uint8_t[element->width*element->height];
							lcd_width = width;
							lcd_height = height;
						}
						
						//
						memset(element->buffer, 0, element->width*element->height);
	
						#if 0
						if (bit_depth == 16)
							png_set_strip_16(png_ptr);
						if (bit_depth < 8)
							png_set_packing (png_ptr);

						if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
							png_set_expand_gray_1_2_4_to_8(png_ptr);
							
						if (color_type == PNG_COLOR_TYPE_GRAY && trns)
							png_set_tRNS_to_alpha(png_ptr);
							
						if ((color_type == PNG_COLOR_TYPE_GRAY && trns) || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) 
						{
							png_set_gray_to_rgb(png_ptr);
							png_set_bgr(png_ptr);
						}

						if (color_type == PNG_COLOR_TYPE_RGB) 
						{
							if (trns)
								png_set_tRNS_to_alpha(png_ptr);
							else
								png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);
						}

						if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
							png_set_bgr(png_ptr);
						////
						#else
						png_set_packing(png_ptr); /* expand to 1 byte blocks */
						
						if (color_type == PNG_COLOR_TYPE_PALETTE)
						      png_set_expand(png_ptr);

						if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
						      png_set_expand(png_ptr);

						if (color_type & PNG_COLOR_MASK_COLOR)
#if (PNG_LIBPNG_VER < 10200)
							png_set_rgb_to_gray(png_ptr);
#else
							png_set_rgb_to_gray(png_ptr, 1, NULL, NULL);
#endif
						#endif
						
						number_passes = png_set_interlace_handling(png_ptr);
						png_read_update_info(png_ptr,info_ptr);
						
						if (width == png_get_rowbytes(png_ptr, info_ptr))
						{
							ret_value = true;
							
							for (pass = 0; pass < number_passes; pass++)
							{
								fbptr = (png_byte *)element->buffer;
								
								for (i = 0; i < element->height; i++)
								{
									png_read_row(png_ptr, fbptr, NULL);
									/* if the PNG is smaller, than the display width... */
									if (width < lcd_width)
										memset(fbptr + width, 0, lcd_width - width);
									fbptr += lcd_width;
								}
							}
							png_read_end(png_ptr, info_ptr);
						}
					}
				}
				png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
			}
		}
		fclose(fh);
	}
	
	return ret_value;
}

bool CLCDDisplay::load_png(const char * const filename)
{
	raw_lcd_element_t element;
	
	element.buffer = _buffer;
	
	return load_png_element(filename, &element);
}

bool CLCDDisplay::dump_png_element(const char * const filename, raw_lcd_element_t * element)
{
	bool         ret_value = false;
	
	png_structp  png_ptr;
	png_infop    info_ptr;
	unsigned int i;
	png_byte *   fbptr;
	FILE *       fp;
 
        /* create file */
        fp = fopen(filename, "wb");
        if (!fp)
                printf("[CLCDDisplay] File %s could not be opened for writing\n", filename);
	else
	{
	        /* initialize stuff */
        	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	        if (!png_ptr)
        	        printf("[CLCDDisplay] png_create_write_struct failed\n");
		else
		{
		        info_ptr = png_create_info_struct(png_ptr);
		        if (!info_ptr)
                		printf("[CLCDDisplay] png_create_info_struct failed\n");
			else
			{
			        if (setjmp(png_jmpbuf(png_ptr)))
			                printf("[CLCDDisplay] Error during init_io\n");
				else
				{
					unsigned int lcd_height = yres;
					unsigned int lcd_width = xres;

        				png_init_io(png_ptr, fp);

        				/* write header */
        				if (setjmp(png_jmpbuf(png_ptr)))
        				        printf("[CLCDDisplay] Error during writing header\n");

        				png_set_IHDR(png_ptr, info_ptr, element->width, element->height,
        				             element->bpp, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
        				             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        				png_write_info(png_ptr, info_ptr);


        				/* write bytes */
					if (setjmp(png_jmpbuf(png_ptr)))
					{
        				        printf("[CLCDDisplay] Error during writing bytes\n");
						return ret_value;
					}

					ret_value = true;

					fbptr = (png_byte *)element->buffer;
					for (i = 0; i < element->height; i++)
					{
						png_write_row(png_ptr, fbptr);
						fbptr += lcd_width;
					}

        				/* end write */
        				if (setjmp(png_jmpbuf(png_ptr)))
					{
        				        printf("[CLCDDisplay] Error during end of write\n");
						return ret_value;
					}

        				png_write_end(png_ptr, NULL);
				}
			}
		}
        	fclose(fp);
	}

	return ret_value;
}

bool CLCDDisplay::dump_png(const char * const filename)
{
	raw_lcd_element_t element;
	
	element.buffer = _buffer;
	element.width = xres;
	element.height = yres;
	element.bpp = 0;
	
	return dump_png_element(filename, &element);
}

