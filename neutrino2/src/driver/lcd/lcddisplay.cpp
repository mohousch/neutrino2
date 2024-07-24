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

#include <global.h>
#include <neutrino2.h>

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
#include <zlib.h>
#include <math.h>

extern "C" {
#include <jpeglib.h>
}

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/version.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <system/settings.h>


//#define LCDDISPLAY_DEBUG

static short debug_level = 10;

#ifdef LCDDISPLAY_DEBUG
#define lcddisplay_printf(level, fmt, x...) do { \
if (debug_level >= level) printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)
#else
#define lcddisplay_printf(level, fmt, x...)
#endif

#define lcddisplay_err(fmt, x...) do { printf("[%s:%s] " fmt, __FILE__, __FUNCTION__, ## x); } while (0)

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

static const char *kDefaultConfigFile = "/etc/graphlcd.conf";

CLCDDisplay::CLCDDisplay()
{
	fd = -1;
	
	//
	xres = 320;
	yres = 240;
	
	paused = 0;
	flipped = false;
	inverted = 0;
	lcd_type = 0;
	last_brightness = 0;
	locked = 0;
	
	//
	raw_buffer_size = 0;
	raw_bypp = sizeof(lcd_pixel_t);
	raw_bpp = 8*sizeof(lcd_pixel_t);
	raw_buffer = NULL;
	raw_stride = 0;
	raw_clut.colors = 0;
	raw_clut.data = 0;
	
	// surface
#ifdef ENABLE_LCD
	surface_data = NULL;
	surface_stride = 0;
	surface_bpp = 0;
	surface_bypp = 0;
	surface_buffer_size = 0;
	////
	real_offset = 0;
	real_yres = 0;
#endif
	
#ifdef ENABLE_TFTLCD
	tftbuffer = NULL;
	tftstride = 0;
	tftbpp = 32;
	tftbypp = 4;
	tft_buffer_size = 0;
	
	m_manual_blit = -1;
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

#ifdef ENABLE_GRAPHLCD
	lcd = NULL;
	bitmap = NULL;
	ngbuffer = NULL;
	ngstride = 0;
	ngbpp = 16;
	ngbypp = 2;
	ng_buffer_size = 0;
	ngxres = 420;
	ngyres = 380;
#endif
}

CLCDDisplay::~CLCDDisplay()
{
#ifdef ENABLE_LCD
	if (surface_data)
	{
		delete [] surface_data;
		surface_data = NULL;
	}
#endif

#ifdef ENABLE_TFTLCD
	if (tftbuffer)
	{
		msync(tftbuffer, m_available, MS_SYNC);
		munmap(tftbuffer, m_available);
		tftbuffer = NULL;
	}
#endif
	
#ifdef ENABLE_GRAPHLCD
	if (lcd)
	{
		lcd->DeInit();
		
		delete lcd;
		lcd = NULL;
		
		delete bitmap;
		bitmap = NULL;
	}
	
	if (ngbuffer)
	{
		delete [] ngbuffer;
		ngbuffer = NULL;
	}
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (raw_buffer)
	{
		delete [] raw_buffer;
		raw_buffer = NULL;
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
	int _bpp = 16; 
	
	//open device
#ifdef USE_OPENGL
	fd = open("/dev/null", O_RDWR);
#else
	fd = open("/dev/dbox/oled0", O_RDWR);
	
	if (fd < 0)
	{
		xres = 128;
		if (!access("/proc/stb/lcd/oled_brightness", W_OK) || !access("/proc/stb/fp/oled_brightness", W_OK) )
			lcd_type = 2;
			
		fd = open("/dev/dbox/lcd0", O_RDWR);
	} 
	else
	{
		lcddisplay_printf(10, "found OLED display!\n");
		lcd_type = 1;
	}
	
	if (fd < 0)
	{
		lcddisplay_err("couldn't open LCD - load lcd.ko!\n");
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
						_bpp = tmp;
					fclose(f);
				}
			}
			
			lcd_type = 3;
		}
	}
#endif
	
	setSize(xres, yres, _bpp);

	return true;
#elif defined (ENABLE_TFTLCD)
	fd = open(fbdevice, O_RDWR);
	
	if (fd < 0)
	{
		lcddisplay_err("%s: %m\n", fbdevice);
		goto nolfb;
	}

#ifndef USE_OPENGL
	if (::ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo) < 0)
	{
		lcddisplay_err("FBIOGET_VSCREENINFO: %m\n");

		goto nolfb;
	}

	fb_fix_screeninfo fix;
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		lcddisplay_err("FBIOGET_FSCREENINFO: %m\n");

		goto nolfb;
	}

	m_available = fix.smem_len;
	m_phys_mem = fix.smem_start;
	
	lcddisplay_printf(0, "%s %dk video mem\n", fbdevice, m_available / 1024);
	
	tftbuffer = (uint32_t *)mmap(0, m_available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	
	if (!tftbuffer)
	{
		lcddisplay_err("mmap: %m\n");

		goto nolfb;
	}

	calcRamp();
	getMode();
	setMode(xres, yres, tftbpp);
	enableManualBlit();
#endif
	
	lcd_type = 4;

	return true;
	
nolfb:
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
	
	lcddisplay_err("framebuffer %s not available\n", fbdevice);
	
	return false;
#endif
}

bool CLCDDisplay::initGLCD()
{
#ifdef ENABLE_GRAPHLCD	
	// configfile
	if (GLCD::Config.Load(kDefaultConfigFile) == false)
	{
		lcddisplay_err("Error loading config file!\n");
		return false;
	}
	
	// driver config
	if ((GLCD::Config.driverConfigs.size() < 1))
	{
		lcddisplay_err("No driver config found!\n");
		return false;
	}
	
	// create driver
	lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[g_settings.glcd_selected_config].id, &GLCD::Config.driverConfigs[g_settings.glcd_selected_config]);
	
	if (!lcd)
	{
		lcddisplay_err("CreateDriver failed.\n");
		return false;
	}
	
	lcddisplay_printf(10, "CreateDriver succeeded.\n");
	
	//
	if (lcd->Init())
	{
		delete lcd;
		lcd = NULL;

		lcddisplay_err("init failed.\n");
		
		return false;
	}

	lcddisplay_printf(10, "init succeeded.\n");
	
	ngxres = lcd->Width();
	ngyres = lcd->Height();
	ngstride = ngxres * ngbypp;
	ng_buffer_size = ngxres * ngyres * ngbypp;
	ngbuffer = new uint32_t[ng_buffer_size];
	memset(ngbuffer, 0, ng_buffer_size);
	
	lcddisplay_printf(10, "%d %d\n", ngxres, ngyres);
	return true;
#endif
}

void CLCDDisplay::initBuffer()
{
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	raw_stride = xres*raw_bypp;
	raw_buffer_size = xres * yres * raw_bypp;
	raw_buffer = new lcd_pixel_t[raw_buffer_size];
	memset(raw_buffer, 0, raw_buffer_size);
	
	lcddisplay_printf(10, "%d %d %d %dk video mem\n", xres, yres, raw_bypp, raw_buffer_size);
#endif
}

#ifdef ENABLE_LCD
void CLCDDisplay::setSize(int w, int h, int b)
{
	lcddisplay_printf(10, "xres=%d, yres=%d, bpp=%d type=%d\n", w, h, b, lcd_type);
	
	//
	xres = w;
	yres = h;
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
			surface_bypp = (surface_bpp + 7)/8;
	}

	// surface
	surface_stride = xres*surface_bypp;
	surface_buffer_size = xres * yres * surface_bypp;
	surface_data = new uint8_t[surface_buffer_size];
	memset(surface_data, 0, surface_buffer_size);
}
#endif

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
			lcddisplay_printf(0, "FBIOPUT_VSCREENINFO: %m\m\n");
			return -1;
		}
		lcddisplay_printf(0, "double buffering not available\n");
	}
	else
		lcddisplay_err("double buffering available\n");

	ioctl(fd, FBIOGET_VSCREENINFO, &m_screeninfo);

	if ((m_screeninfo.xres != (unsigned int)nxRes) || (m_screeninfo.yres != (unsigned int)nyRes) ||
		(m_screeninfo.bits_per_pixel != (unsigned int)nbpp))
	{
		lcddisplay_err("failed: wanted: %dx%dx%d, got %dx%dx%d\n",
			nxRes, nyRes, nbpp,
			m_screeninfo.xres, m_screeninfo.yres, m_screeninfo.bits_per_pixel);
	}
	
	xres = m_screeninfo.xres;
	yres = m_screeninfo.yres;
	tftbpp = m_screeninfo.bits_per_pixel;
	
	fb_fix_screeninfo fix;
	
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
	{
		lcddisplay_err("FBIOGET_FSCREENINFO: %m\n");
	}
	
	tftstride = fix.line_length;
	tft_buffer_size = tftstride * yres;
	
	memset(tftbuffer, 0, tftstride * yres);
	
	update();
	
	return 0;
}

void CLCDDisplay::getMode()
{
	xres = m_screeninfo.xres;
	yres = m_screeninfo.yres;
	tftbpp = m_screeninfo.bits_per_pixel;
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
	setMode(xres, yres, tftbpp);
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

#ifdef ENABLE_GRAPHLCD
int CLCDDisplay::GetConfigSize()
{
	return (int) GLCD::Config.driverConfigs.size();
}

std::string CLCDDisplay::GetConfigName(int driver)
{
	if ((driver < 0) || (driver > GetConfigSize() - 1))
		driver = 0;
		
	return GLCD::Config.driverConfigs[driver].name;
}
#endif

////
void CLCDDisplay::setInverted(uint32_t inv)
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
		lcddisplay_err("can't open /dev/dbox/fp0(%m)\n");
		return (-1);
	}
	
	if(ioctl(fd, LCD_IOCTL_SRV, &contrast) < 0)
	{
		lcddisplay_err("can't set lcd contrast(%m)\n");
	}
	
	close(fp);

	return(0);
}

int CLCDDisplay::setLCDBrightness(int brightness)
{
	lcddisplay_printf(10, "%d\n", brightness);
	
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
		memset(raw_buffer, inverted, raw_buffer_size);
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

//// 
void CLCDDisplay::blit(void)
{
#ifdef ENABLE_LCD
	if ((fd >= 0) && (last_brightness > 0))
	{
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
#if defined (PLATFORM_DREAMBOX)	|| defined (PLATFORM_GIGABLUE)
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
			lcddisplay_err("FBIO_BLIT: %m\n");
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (lcd)
	{
		lcd->SetScreen(ngbuffer, ngxres, ngyres);	
		lcd->Refresh(false);
	}
#endif
}

static inline void blit_8i_to_16(uint16_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
        	*dst++=pal[*src++] & 0xFFFF;
}

static inline void blit_8i_to_16_at(uint16_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
    	{
        	if (!(pal[*src]&0x80000000))
        	{
            		src++;
            		dst++;
        	} 
        	else
            		*dst++=pal[*src++] & 0xFFFF;
    	}
}

static inline void blit_8i_to_32(uint32_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
        	*dst++=pal[*src++];
}

static inline void blit_8i_to_32_at(uint32_t *dst, const uint8_t *src, const uint32_t *pal, int width)
{
    	while (width--)
    	{
        	if (!(pal[*src]&0x80000000))
        	{
            		src++;
            		dst++;
        	} 
        	else
            		*dst++=pal[*src++];
   	 }
}

static void blit_8i_to_32_ab(gRGB *dst, const uint8_t *src, const gRGB *pal, int width)
{
    	while (width--)
    	{
        	dst->alpha_blend(pal[*src++]);
        	++dst;
    	}
}

static void convert_palette(uint32_t* pal, const gPalette& clut)
{
    	int i = 0;
    	
    	if (clut.data)
    	{
        	while (i < clut.colors)
        	{
            		pal[i] = clut.data[i].argb() ^ 0xFF000000;
            		++i;
        	}
    	}
    	
    	for(; i != 256; ++i)
    	{
        	pal[i] = (0x010101*i) | 0xFF000000;
    	}
}

static bool swscale(unsigned char *src, unsigned char *dst, int sw, int sh, int dw, int dh, AVPixelFormat sfmt, AVPixelFormat dfmt)
{
	bool ret = false;
	int len = 0;
	struct SwsContext *scale = NULL;
	
	scale = sws_getCachedContext(scale, sw, sh, sfmt, dw, dh, dfmt, SWS_BICUBIC, 0, 0, 0); //nglcd need rgb32
	
	if (!scale)
	{
		lcddisplay_err("ERROR setting up SWS context\n");
		return ret;
	}
	
	AVFrame *sframe = av_frame_alloc();
	AVFrame *dframe = av_frame_alloc();
	
	if (sframe && dframe)
	{
		len = av_image_fill_arrays(sframe->data, sframe->linesize, &(src)[0], sfmt, sw, sh, 1);
		
		if (len > -1)
			ret = true;

		if (ret && (len = av_image_fill_arrays(dframe->data, dframe->linesize, &(dst)[0], sfmt, dw, dh, 1) < 0))
			ret = false;

		if (ret && (len = sws_scale(scale, sframe->data, sframe->linesize, 0, sh, dframe->data, dframe->linesize) < 0))
			ret = false;
		else
			ret = true;
	}
	else
	{
		lcddisplay_err("could not alloc sframe (%p) or dframe (%p)\n", sframe, dframe);
		ret = false;
	}

	if (sframe)
	{
		av_frame_free(&sframe);
		sframe = NULL;
	}
	
	if (dframe)
	{
		av_frame_free(&dframe);
		dframe = NULL;
	}
	
	if (scale)
	{
		sws_freeContext(scale);
		scale = NULL;
	}
	
	lcddisplay_err("Error scale %ix%i to %ix%i ,len %i\n", sw, sh, dw, dh, len);

	return ret;
}


// blit2lcd
void CLCDDisplay::blitBox2LCD(int flag) 
{
	int area_left = 0;
	int area_top = 0;
	int area_right = xres;
	int area_bottom = yres;
	int area_width  = area_right - area_left;
	int area_height = area_bottom - area_top;
	
#ifdef ENABLE_LCD
	area_left = 0;
	area_top = 0;
	area_right = xres;
	area_bottom = yres;
	area_width  = area_right - area_left;
	area_height = area_bottom - area_top;
	
	if (surface_bpp == 8 && raw_bpp == 8)
	{
		uint8_t *srcptr = (uint8_t*)raw_buffer;
            	uint8_t *dstptr = (uint8_t*)surface_data;

            	srcptr += area_left*raw_bypp + area_top*raw_stride;
            	dstptr += area_left*surface_bypp + area_top*surface_stride;
            
            	if ( (flag & blitAlphaTest) || (flag & blitAlphaBlend) )
            	{
                	for (int y = area_height; y != 0; --y)
                	{
                    		// no real alphatest yet
                    		int width = area_width;
                    		unsigned char *s = (unsigned char*)srcptr;
                    		unsigned char *d = (unsigned char*)dstptr;
                    			
                    		// use duff's device here!
                    		while (width--)
                    		{
                        		if (!*s)
                        		{
                            			s++;
                            			d++;
                        		}
                        		else
                        		{
                            			*d++ = *s++;
                        		}
                    		}
                    		srcptr += raw_stride;
                    		dstptr += surface_stride;
                	}
            	}
            	else
            	{
                	int linesize = area_width*surface_bypp;
                	for (int y = area_height; y != 0; --y)
                	{
                    		memcpy(dstptr, srcptr, linesize);
                    		srcptr += raw_stride;
                    		dstptr += surface_stride;
                	}
            	}
	} 
	else if (surface_bpp == 16 && raw_bpp == 8)
	{
		uint8_t *srcptr = (uint8_t*)raw_buffer;
            	uint8_t *dstptr = (uint8_t*)surface_data;
            	uint32_t pal[256];

            	for (int i = 0; i != 256; ++i)
            	{
                	uint32_t icol;
                	icol = 0x010101*i;
#if BYTE_ORDER == LITTLE_ENDIAN
                	pal[i] = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                	pal[i] = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                	pal[i] ^= 0xFF000000;
            	}

            	srcptr += area_left*raw_bypp + area_top*raw_stride;
            	dstptr += area_left*surface_bypp + area_top*surface_stride;

            	for (int y = 0; y < area_height; y++)
            	{
                	int width = area_width;
                	uint8_t *psrc = (uint8_t *)srcptr;
                	uint16_t *dst=(uint16_t*)dstptr;
                		
                	if (flag & blitAlphaTest)
                    		blit_8i_to_16_at(dst, psrc, pal, width);
                	else
                    		blit_8i_to_16(dst, psrc, pal, width);
                    			
                	srcptr += raw_stride;
                	dstptr += surface_stride;
            	}
        }
	else if (surface_bpp == 16 && raw_bpp == 32)
	{
            	uint8_t *srcptr = (uint8_t*)raw_buffer;
            	uint8_t *dstptr = (uint8_t*)surface_data;

            	srcptr += area_left*raw_bypp + area_top*raw_stride;
            	dstptr += area_left*surface_bypp + area_top*surface_stride;

            	for (int y = 0; y < area_height; y++)
            	{
                	int width = area_width;
                	uint32_t *srcp = (uint32_t*)srcptr;
                	uint16_t *dstp = (uint16_t*)dstptr;

                	if (flag & blitAlphaBlend)
                	{
                    		while (width--)
                    		{
                        		if (!((*srcp)&0xFF000000))
                        		{
                            			srcp++;
                            			dstp++;
                        		} 
                        		else
                        		{
                            			gRGB icol = *srcp++;
#if BYTE_ORDER == LITTLE_ENDIAN
                            			uint32_t jcol = bswap_16(*dstp);
#else
                            			uint32_t jcol = *dstp;
#endif
                            			int bg_b = (jcol >> 8) & 0xF8;
                            			int bg_g = (jcol >> 3) & 0xFC;
                            			int bg_r = (jcol << 3) & 0xF8;

                            			int a = icol.a;
                            			int r = icol.r;
                            			int g = icol.g;
                            			int b = icol.b;

                            			r = ((r - bg_r)*a)/255 + bg_r;
                            			g = ((g - bg_g)*a)/255 + bg_g;
                            			b = ((b - bg_b)*a)/255 + bg_b;

#if BYTE_ORDER == LITTLE_ENDIAN
                            			*dstp++ = bswap_16( (b >> 3) << 11 | (g >> 2) << 5 | r  >> 3 );
#else
                            			*dstp++ = (b >> 3) << 11 | (g >> 2) << 5 | r  >> 3 ;
#endif
                        		}
                    		}
                	}
                	else if (flag & blitAlphaTest)
                	{
                    		while (width--)
                    		{
                        		if (!((*srcp)&0xFF000000))
                        		{
                            			srcp++;
                            			dstp++;
                        		} 
                        		else
                        		{
                            			uint32_t icol = *srcp++;
#if BYTE_ORDER == LITTLE_ENDIAN
                            			*dstp++ = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                            			*dstp++ = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                        		}
                    		}
                	} 
                	else
                	{
                    		while (width--)
                    		{
                        		uint32_t icol = *srcp++;
                        			
#if BYTE_ORDER == LITTLE_ENDIAN
                        		*dstp++ = bswap_16(((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19);
#else
                        		*dstp++ = ((icol & 0xFF) >> 3) << 11 | ((icol & 0xFF00) >> 10) << 5 | (icol & 0xFF0000) >> 19;
#endif
                    		}
                	}
                	srcptr += raw_stride;
                	dstptr += surface_stride;
            	}
	} 
	else if (surface_bpp == 32 && raw_bpp == 8)
	{
		const uint8_t *srcptr = (uint8_t*)raw_buffer;
            	uint8_t *dstptr = (uint8_t*)surface_data;
            	uint32_t pal[256];
            	convert_palette(pal, raw_clut);

            	srcptr += area_left*raw_bypp + area_top*raw_stride;
            	dstptr += area_left*surface_bypp + area_top*surface_stride;
            	const int width = area_width;
            		
            	for (int y = area_height; y != 0; --y)
            	{
                	if (flag & blitAlphaTest)
                    		blit_8i_to_32_at((uint32_t*)dstptr, srcptr, pal, width);
                	else if (flag & blitAlphaBlend)
                    		blit_8i_to_32_ab((gRGB*)dstptr, srcptr, (const gRGB*)pal, width);
                	else
                    			blit_8i_to_32((uint32_t*)dstptr, srcptr, pal, width);
                	srcptr += raw_stride;
                	dstptr += surface_stride;
            	}
	}
	else if (surface_bpp == 32 && raw_bpp == 32)
	{
		uint32_t *srcptr = (uint32_t*)raw_buffer;
            	uint32_t *dstptr = (uint32_t*)surface_data;

            	srcptr += area_left + area_top*raw_stride/4;
            	dstptr += area_left + area_top*surface_stride/4;
            		
            	for (int y = area_height; y != 0; --y)
            	{
                	if (flag & blitAlphaTest)
                	{
                    		int width = area_width;
                    		uint32_t *src = srcptr;
                    		uint32_t *dst = dstptr;

                    		while (width--)
                    		{
                        		if (!((*src)&0xFF000000))
                        		{
                            			src++;
                            			dst++;
                        		} 
                        		else
                            			*dst++=*src++;
                    		}
                	} 
                	else if (flag & blitAlphaBlend)
                	{
                    		int width = area_width;
                    		gRGB *src = (gRGB*)srcptr;
                    		gRGB *dst = (gRGB*)dstptr;
                    			
                    		while (width--)
                    		{
                        		dst->alpha_blend(*src++);
                        		++dst;
                    		}
                	}
                	else
                    		memcpy(dstptr, srcptr, area_width*surface_bypp);
                    
                	srcptr = (uint32_t*)((uint8_t*)srcptr + raw_stride);
                	dstptr = (uint32_t*)((uint8_t*)dstptr + surface_stride);
            	}
	}
#endif

#ifdef ENABLE_TFTLCD
	memcpy(tftbuffer, raw_buffer, tftstride * yres);
#endif

#ifdef ENABLE_GRAPHLCD
		//
		swscale((uint8_t *)raw_buffer, (uint8_t *)ngbuffer, xres, yres, ngxres, ngyres, AV_PIX_FMT_BGR32, AV_PIX_FMT_RGB32);	
#endif
}

void CLCDDisplay::update()
{
	//
	blitBox2LCD();
	
	//
	blit();
}

void CLCDDisplay::draw_point(const int x, const int y, const uint32_t color)
{
	if ((x < 0) || (x >= xres) || (y < 0) || (y >= yres))
		return;

	if (color == LCD_PIXEL_INV)
		raw_buffer[(y * xres + x)] ^= 1;
	else
		raw_buffer[(y * xres + x)] = color;
}

void CLCDDisplay::draw_line(const int x1, const int y1, const int x2, const int y2, const uint32_t color)  
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

		draw_point(x, y, color);

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
			draw_point(x, y, color);
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

		draw_point(x, y, color);

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
			draw_point(x, y, color);
		}
	}
}

void CLCDDisplay::draw_fill_rect(int left, int top, int right, int bottom, uint32_t color) 
{
	int x, y;
	
	for(x = left + 1; x < right; x++) 
	{  
		for(y = top + 1; y < bottom; y++) 
		{
			draw_point(x, y, color);
		}
	}
}

void CLCDDisplay::draw_rectangle(int left,int top, int right, int bottom, uint32_t linecolor, uint32_t fillcolor)
{
	draw_line(left, top, right, top, linecolor);
	draw_line(left, top, left, bottom, linecolor);
	draw_line(right, top, right, bottom, linecolor);
	draw_line(left, bottom, right, bottom, linecolor);
	draw_fill_rect(left, top, right, bottom, fillcolor);  
}  

void CLCDDisplay::draw_polygon(int num_vertices, int *vertices, uint32_t color) 
{
	int i;
	for(i = 0; i < num_vertices - 1; i++) 
	{
		draw_line(vertices[(i<<1)+0],
			vertices[(i<<1)+1],
			vertices[(i<<1)+2],
			vertices[(i<<1)+3],
			color);
	}
   
	draw_line(vertices[0],
		vertices[1],
		vertices[(num_vertices<<1)-2],
		vertices[(num_vertices<<1)-1],
		color);
}

void CLCDDisplay::clear_screen() 
{
	memset(raw_buffer, 0, raw_buffer_size);
#ifdef ENABLE_LCD
	memset(surface_data, 0, surface_buffer_size);
#endif
#ifdef ENABLE_TFTLCD
	memset(tftbuffer, 0, tft_buffer_size);
#endif
#ifdef ENABLE_GRAPHLCD
	memset(ngbuffer, 0, ng_buffer_size);
	if (lcd)
		lcd->Clear();
#endif
	blit();
}

void CLCDDisplay::dump_screen(lcd_pixel_t **screen) 
{
	memmove(*screen, (lcd_pixel_t *)raw_buffer, raw_buffer_size);
}

void CLCDDisplay::load_screen_element(raw_lcd_element_t * element, int left, int top) 
{
	lcddisplay_printf(10, "CLCDDisplay::load_screen_element: %s %dx%dx%d (posx: %d posy: %d)\n", element->name.c_str(), element->width, element->height, element->bpp, left, top);
	
	if ((element->buffer) && (element->height <= yres - top))
	{
		for (unsigned int i = 0; i < min(element->height, yres - top); i++)
		{	
			memmove(raw_buffer + ((top + i)*xres) + left, (lcd_pixel_t *)element->buffer + (i*element->width), min(element->width, xres - left)*raw_bypp);
		}
	}
	
	free(element->buffer);
}

void CLCDDisplay::load_screen(lcd_pixel_t **const screen) 
{
	raw_lcd_element_t element;
	
	element.buffer = (lcd_pixel_t *)*screen;
	element.width = xres;
	element.height = yres;
	element.bpp = raw_bpp;
	
	load_screen_element(&element, 0, 0);
}

bool CLCDDisplay::dump_png(const char * const filename)
{
	bool         ret_value = false;
	
	png_structp  png_ptr;
	png_infop    info_ptr;
	unsigned int i;
	png_byte *   fbptr;
	FILE *       fp;
 
        // create file
        fp = fopen(filename, "wb");
        if (!fp)
                printf("[CLCDDisplay] File %s could not be opened for writing\n", filename);
	else
	{
	        // initialize stuff
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
        				png_init_io(png_ptr, fp);

        				// write header
        				if (setjmp(png_jmpbuf(png_ptr)))
        				        printf("[CLCDDisplay] Error during writing header\n");

					//
					png_set_IHDR(png_ptr, info_ptr, xres, yres, 8, (raw_bpp == 32)? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
					
					//
        				png_write_info(png_ptr, info_ptr);
					
        				// write bytes
					if (setjmp(png_jmpbuf(png_ptr)))
					{
        				        printf("[CLCDDisplay] Error during writing bytes\n");
						return ret_value;
					}

					ret_value = true;
					
					fbptr = (png_byte *)raw_buffer;
					
					for (i = 0; i < (unsigned int)yres; i++)
					{
						png_write_row(png_ptr, fbptr);
						fbptr += raw_stride;
					}

        				// end write
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

int CLCDDisplay::dump_jpeg(const char * filename)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile;
	JSAMPROW row_pointer[1];
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL)
	{
		lcddisplay_printf(10, "jpeg can't open %s\n", filename);
		return 1;
	}
	lcddisplay_printf(10, "save Thumbnail... %s\n",filename);

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = xres;
	cinfo.image_height = yres;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 70, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = (xres*3);
	
	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = (uint8_t *)&raw_buffer[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);
	
	return 0;
}

int CLCDDisplay::showPNGImage(const char *filename, int posx, int posy, int width, int height, int flag)
{
	lcddisplay_printf(10, "%s %d %d %d %d (flag: %d)\n", filename, posx, posy, width, height, flag);
	
	raw_lcd_element_t element;
	int p_w, p_h, p_bpp;
	int chans = 1;
	
	::getSize(filename, &p_w, &p_h, &p_bpp, &chans);
	
	lcddisplay_printf(10, "real: %s %d %d %d %d\n", filename, p_w, p_h, p_bpp, chans);

	if (raw_bpp == 32)
		element.buffer = (lcd_pixel_t *)::getBGR32Image(filename, width, height);
	else
		element.buffer = (lcd_pixel_t *)::getBGR8Image(filename, width, height);

	element.x = posx;
	element.y = posy;
	element.width = width;
	element.height = height;
	element.bpp = p_bpp;
	element.bypp = chans;
	element.stride = element.width*element.bypp;
	element.name = filename;
	
	load_screen_element(&element, posx, posy);
	
	return 0;
}

void CLCDDisplay::load_png_element(raw_lcd_element_t *element, int posx, int posy, int width, int height)
{
	lcddisplay_printf(10, "%s %d %d %d %d\n", element->name.c_str(), posx, posy, width, height);
	
	int p_w, p_h, p_bpp;
	int chans = 1;
	
	::getSize(element->name.c_str(), &p_w, &p_h, &p_bpp, &chans);
	
	lcddisplay_printf(10, "real: %s %d %d\n", element->name.c_str(), p_w, p_h);
	
	uint8_t *image = ::getBitmap(element->name.c_str());
	
	if (width != 0 && height != 0)
	{
	 	image = ::resize(image, p_w, p_h, width, height, SCALE_COLOR, (chans == 4)? true : false);
	}
	else
	{
		width = p_w;
		height = p_h;
	}
	
	if (raw_bpp == 32)
		element->buffer = (lcd_pixel_t *)::convertBGR2FB32(image, width, height, (chans == 4)? true : false);
	else
		element->buffer = (lcd_pixel_t *)::convertBGR2FB8(image, width, height, (chans == 4)? true : false);

	element->x = posx;
	element->y = posy;
	element->width = width;
	element->height = height;
	element->bpp = p_bpp;
	element->bypp = chans;
	element->stride = element->width*element->bypp;
}

void CLCDDisplay::show_png_element(raw_lcd_element_t *element, int posx, int posy, int width, int height)
{
	lcddisplay_printf(10, "%s %d %d %d %d\n", element->name.c_str(), posx, posy, width, height);
	
	load_png_element(element, posx, posy, width, height);
	
	//
	load_screen_element(element, posx, posy);
}

void CLCDDisplay::show_analog_clock(int hour, int min, int sec, int posx, int posy, int hour_size, int min_size, int sec_size)
{
	int time_sec, time_min, time_hour, sec_x, sec_y, min_x, min_y, hour_x, hour_y, dia;
	double pi = 3.1415926535897932384626433832795, sAngleInRad, mAngleInRad, mAngleSave, hAngleInRad;

	time_sec = sec;
	time_min = min;
	time_hour = hour;

	dia = 180;

	// sec
	sAngleInRad = ((6 * time_sec) * (2 * pi / 360));
	sAngleInRad -= pi / 2;
	sec_x = int((dia * 0.9 * cos(sAngleInRad)));
	sec_y = int((dia * 0.9 * sin(sAngleInRad)));

	// min
	mAngleInRad = ((6 * time_min) * (2 * pi / 360));
	mAngleSave = mAngleInRad;
	mAngleInRad -= pi/2;
	min_x = int((dia * 0.7 * cos(mAngleInRad)));
	min_y = int((dia * 0.7 * sin(mAngleInRad)));

	// hr
	hAngleInRad = ((30 * time_hour) * (2 * pi / 360));
	hAngleInRad += mAngleSave/12;
	hAngleInRad -= pi/2;
	hour_x = int((dia * 0.5 * cos(hAngleInRad)));
	hour_y = int((dia * 0.5 * sin(hAngleInRad)));

	// hour
	for (int i = 0; i <= hour_size; i++)
	{
		draw_line(posx - i, posy - i, posx + hour_x, posy + hour_y, LCD_PIXEL_WHITE);
		draw_line(posx + i, posy + i, posx + hour_x, posy + hour_y, LCD_PIXEL_WHITE);
	}

	// min
	for (int i = 0; i <= min_size; i++)
	{
		draw_line(posx - i, posy - i, posx + min_x, posy + min_y, LCD_PIXEL_WHITE);
		draw_line(posx + i, posy + i, posx + min_x, posy + min_y, LCD_PIXEL_WHITE);
	}
	
	// sec
	for (int i = 0; i <= min_size; i++)
	{
		draw_line(posx - i, posy - i, posx + sec_x, posy + sec_y, LCD_PIXEL_RED);
		draw_line(posx + i, posy + i, posx + sec_x, posy + sec_y, LCD_PIXEL_RED);
	}
}

