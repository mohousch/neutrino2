//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: framebuffer.cpp 17062024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//                    2003 thegoodguy
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/color.h>

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory.h>
#include <math.h>

#include <linux/kd.h>

#include <stdint.h>
#include <png.h>
#include <zlib.h>	

#include <global.h>
#include <system/debug.h>
#include <system/helpers.h>

// opengl
#ifdef USE_OPENGL
#include <GL/glew.h>
#include <driver/rcinput.h>
#include <driver/gdi/glthread.h>
#endif


#define BACKGROUNDIMAGEWIDTH 	DEFAULT_XRES
#define BACKGROUNDIMAGEHEIGHT	DEFAULT_YRES

////
CFrameBuffer::CFrameBuffer()
: active ( true )
{
#if defined (USE_OPENGL)
	mpGLThreadObj = NULL;
#endif

	iconBasePath = "";
	buttonBasePath = "";
	hintBasePath = "";
	spinnerBasePath = "";
	available  = 0;
	cmap.start = 0;
	cmap.len = 256;
	cmap.red = red;
	cmap.green = green;
	cmap.blue  = blue;
	cmap.transp = trans;
	backgroundColor = 0;
	useBackgroundPaint = false;
	background = NULL;
	backupBackground = NULL;
	backgroundFilename = "";
	fd  = 0;
	m_manual_blit = -1;
	
	// 
	memset(red, 0, 256*sizeof(__u16));
	memset(green, 0, 256*sizeof(__u16));
	memset(blue, 0, 256*sizeof(__u16));
	memset(trans, 0, 256*sizeof(__u16));

	//
	q_circle = NULL;
	initQCircle();
	corner_tl = false;
	corner_tr = false;
	corner_bl = false;
	corner_br = false;
	
	//
	xRes = DEFAULT_XRES;
	yRes = DEFAULT_YRES; 
	stride = xRes * sizeof(fb_pixel_t); 
	bpp = DEFAULT_BPP;
	lfb = NULL;
}

CFrameBuffer* CFrameBuffer::getInstance()
{
	static CFrameBuffer * frameBuffer = NULL;

	if(!frameBuffer) 
	{
		frameBuffer = new CFrameBuffer();
	} 

	return frameBuffer;
}

void CFrameBuffer::init(const char * const fbDevice)
{
	dprintf(DEBUG_NORMAL, "CFrameBuffer::init\n");
	
#if defined (USE_OPENGL)
	fd = -1;
	if(!mpGLThreadObj)
	{
		screeninfo.bits_per_pixel = DEFAULT_BPP;
		screeninfo.xres = DEFAULT_XRES;
		screeninfo.xres_virtual = screeninfo.xres;
		screeninfo.yres = DEFAULT_YRES;
		screeninfo.yres_virtual = screeninfo.yres;
		
		mpGLThreadObj = new GLThreadObj(screeninfo.xres, screeninfo.yres);

		if(mpGLThreadObj)
		{ 
			// kick off the GL thread for the window
			mpGLThreadObj->Start();
			mpGLThreadObj->waitInit();
		}
	}
	
	lfb = reinterpret_cast<fb_pixel_t *>(mpGLThreadObj->getOSDBuffer());
	
	if (!lfb) 
	{
		perror("mmap");
		goto nolfb;
	}
#else	

	fd = open(fbDevice, O_RDWR);

	if(!fd) 
		fd = open(fbDevice, O_RDWR);

	if (fd < 0) 
	{
		perror(fbDevice);
		goto nolfb;
	}

	// get variable screeninfo
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) < 0) 
	{
		perror("FBIOGET_VSCREENINFO");
		goto nolfb;
	}

	// get fix screen info
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0) 
	{
		perror("FBIOGET_FSCREENINFO");
		goto nolfb;
	}

	available = fix.smem_len;
	
	dprintf(DEBUG_NORMAL, "CFrameBuffer::init %dk video mem\n", available/1024);
	
	lfb = (fb_pixel_t *)mmap(0, available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

	if (!lfb) 
	{
		perror("mmap");
		goto nolfb;
	}

#ifdef __sh__ 
	//we add 2MB at the end of the buffer, the rest does the blitter 
	lfb += 1920*1080;

	if (available / (1024 * 1024) < 12)
	{
		printf("CFrameBuffer::init: to less memory for stmfb given, need at least 12mb\n"); 
		goto nolfb;
	}
#else
	enableManualBlit();
#endif /*sh*/ 
#endif /* USE_OPENGL */
	
	// set colors
	paletteSetColor(0x1, 0x010101, 0xFF);			// black
        paletteSetColor(COL_MAROON, 0x800000, 0xFF);		// maroon
        paletteSetColor(COL_GREEN, 0x008000, 0xFF);		// green
	paletteSetColor(COL_OLIVE, 0x808000, 0xFF);		// olive
        paletteSetColor(COL_NAVY, 0x000080, 0xFF);		// navy
        paletteSetColor(COL_PURPLE, 0x800080, 0xFF);		// purple
        paletteSetColor(COL_TEAL, 0x008080, 0xFF);		// teal
        paletteSetColor(COL_NOBEL, 0xA0A0A0, 0xFF);		// nobel
        paletteSetColor(COL_MATTERHORN, 0x505050, 0xFF);	// matterhorn
        paletteSetColor(COL_RED, 0xFF0000, 0xFF);		// red
        paletteSetColor(COL_LIME, 0x00FF00, 0xFF);		// lime
        paletteSetColor(COL_YELLOW, 0xFFFF00, 0xFF);		// yelloow
        paletteSetColor(COL_BLUE, 0x0000FF, 0xFF);		// blue
        paletteSetColor(COL_MAGENTA, 0xFF00FF, 0xFF);		// magenta
        paletteSetColor(COL_AQUA, 0x00FFFF, 0xFF);		// aqua
        paletteSetColor(COL_WHITE, 0xFFFFFF, 0xFF);		// white
        paletteSetColor(COL_BLACK, 0x000000, 0xFF);		// black
        paletteSetColor(COL_ORANGE, 0xFF5500, 0xFF);		// orange
        paletteSetColor(COL_SILVER, 0xBEBEBE, 0xFF);		// silver
        paletteSetColor(COL_BACKGROUND, 0x000000, 0x0);

        paletteSet(&cmap);

        useBackground(false);
        
        // init libngpng
	init_handlers();

	return;

nolfb:
	dprintf(DEBUG_NORMAL, "CFrameBuffer::init: framebuffer not available.\n");
	lfb = 0;
}


CFrameBuffer::~CFrameBuffer()
{
	dprintf(DEBUG_NORMAL, "~CFrameBuffer()\n");
	
	if (background) 
	{
		free(background);
		background = NULL;
	}

	if (backupBackground) 
	{
		delete[] backupBackground;
		backupBackground = NULL;
	}

	if (lfb)
		munmap(lfb, available);
		
	// deinit libngpng
	deinit_handlers();
	
#if defined (USE_OPENGL)
	active = false;
	mpGLThreadObj->shutDown();
	mpGLThreadObj->join();
#else	
	close(fd);
#endif	
}

int CFrameBuffer::getFileHandle() const
{
	return fd;
}

unsigned int CFrameBuffer::getStride() const
{
	return stride;
}

unsigned int CFrameBuffer::getScreenWidth(bool real)
{
	if(real)
		return xRes;
	else
		return g_settings.screen_EndX - g_settings.screen_StartX;
}

unsigned int CFrameBuffer::getScreenHeight(bool real)
{
	if(real)
		return yRes;
	else
		return g_settings.screen_EndY - g_settings.screen_StartY;
}

unsigned int CFrameBuffer::getScreenX(bool real)
{
	if (real)
		return 0;
	else
		return g_settings.screen_StartX;
}

unsigned int CFrameBuffer::getScreenY(bool real)
{
	if (real)
		return 0;
	else
		return g_settings.screen_StartY;
}

fb_pixel_t * CFrameBuffer::getFrameBufferPointer() const
{	  
	if (active)
	{
		return lfb;		
	}	
}

unsigned int CFrameBuffer::getAvailableMem() const
{
	return available;
}

bool CFrameBuffer::getActive() const
{
	return (active);
}

void CFrameBuffer::setActive(bool enable)
{
	active = enable;
		
#if !defined (__sh__)
	if(enable)
	{
		// set manual blit when fb is activ
		if (m_manual_blit == 0) 
		{
			enableManualBlit();
		}
	}
	else
	{
		if (m_manual_blit == 1) 
		{
			disableManualBlit();
		}
	}
#endif
}

t_fb_var_screeninfo *CFrameBuffer::getScreenInfo()
{
	return &screeninfo;
}

void CFrameBuffer::setFrameBufferMode(unsigned int nxRes, unsigned int nyRes, unsigned int nbpp)
{
	screeninfo.xres_virtual = screeninfo.xres = nxRes;
	screeninfo.yres_virtual = (screeninfo.yres = nyRes)*2;
	screeninfo.height = 0;
	screeninfo.width = 0;
	screeninfo.xoffset = screeninfo.yoffset = 0;
	screeninfo.bits_per_pixel = nbpp;

	switch (nbpp) 
	{
		case 16:
			// ARGB 1555
			screeninfo.transp.offset = 15;
			screeninfo.transp.length = 1;
			screeninfo.red.offset = 10;
			screeninfo.red.length = 5;
			screeninfo.green.offset = 5;
			screeninfo.green.length = 5;
			screeninfo.blue.offset = 0;
			screeninfo.blue.length = 5;
			break;
			
		case 32:
			// ARGB 8888
			screeninfo.transp.offset = 24;
			screeninfo.transp.length = 8;
			screeninfo.red.offset = 16;
			screeninfo.red.length = 8;
			screeninfo.green.offset = 8;
			screeninfo.green.length = 8;
			screeninfo.blue.offset = 0;
			screeninfo.blue.length = 8;
			break;
	}
	
	// num of pages
	m_number_of_pages = screeninfo.yres_virtual / nyRes;
	
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo) < 0)
	{
		// try single buffering
		screeninfo.yres_virtual = screeninfo.yres = nyRes;

		if (ioctl(fd, FBIOPUT_VSCREENINFO, &screeninfo) < 0)
		{
			perror("FBIOPUT_VSCREENINFO");
		}
		
		printf("CFrameBuffer::setVideoMode: double buffering not available.\n");
	} 
	else
		printf("CFrameBuffer::setVideoMode: double buffering available!\n");
	
	ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);

	if ((screeninfo.xres != nxRes) && (screeninfo.yres != nyRes) && (screeninfo.bits_per_pixel != nbpp))
	{
		printf("CFrameBuffer::setVideoMode: failed: wanted: %dx%dx%d, got %dx%dx%d\n", nxRes, nyRes, nbpp, screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);
	}
	
	xRes = screeninfo.xres;
	yRes = screeninfo.yres;
	bpp  = screeninfo.bits_per_pixel;
	
	// stride
	fb_fix_screeninfo fix;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
	}

	stride = fix.line_length;
}

int CFrameBuffer::setMode(unsigned int x, unsigned int y, unsigned int _bpp)
{
	if (!available && !active)
		return -1;
	
	dprintf(DEBUG_NORMAL, "CFrameBuffer::setMode: FB: %dx%dx%d\n", x, y, _bpp);

#if defined (__sh__) || defined (USE_OPENGL)
	xRes = x;
	yRes = y;
	bpp = _bpp;
	stride = xRes * sizeof(fb_pixel_t);
#else
	setFrameBufferMode(x, y, _bpp);
#endif	

	// clear frameBuffer
	memset(lfb, 0, screeninfo.xres * screeninfo.yres * sizeof(fb_pixel_t));

	return 0;
}

// blend mode: 0=non-premultiplied alpha | 1=premultiplied alpha
void CFrameBuffer::setBlendMode(uint8_t mode)
{
#ifdef __sh__	
	struct stmfbio_var_screeninfo_ex varEx = {0};
	memset(&varEx, 0, sizeof(varEx));

	varEx.layerid  = 0;
	varEx.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	varEx.caps |= STMFBIO_VAR_CAPS_PREMULTIPLIED;
	varEx.premultiplied_alpha = mode;

	if(ioctl(fd, STMFBIO_SET_VAR_SCREENINFO_EX, &varEx) < 0)
		perror("STMFBIO_SET_VAR_SCREENINFO_EX");
#endif
}

// blendlevel (e.g transparency)
void CFrameBuffer::setBlendLevel(int level)
{
#ifdef __sh__
	struct stmfbio_var_screeninfo_ex varEx = {0};
	memset(&varEx, 0, sizeof(varEx));

	varEx.layerid  = 0;
	varEx.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	varEx.caps = STMFBIO_VAR_CAPS_OPACITY;
	varEx.opacity = level;


	if(ioctl(fd, STMFBIO_SET_VAR_SCREENINFO_EX, &varEx) < 0)
		perror("STMFBIO_SET_VAR_SCREENINFO_EX");
#endif	
}

void CFrameBuffer::paletteFade(int i, __u32 rgb1, __u32 rgb2, int level)
{
	cmap.red[i] = ((rgb2&0xFF0000)>>16)*level + ((rgb1&0xFF0000)>>16)*(255 - level);
	cmap.green[i] = ((rgb2&0x00FF00)>>8 )*level + ((rgb1&0x00FF00)>>8 )*(255 - level);
	cmap.blue[i] = ((rgb2&0x0000FF)    )*level + ((rgb1&0x0000FF))*(255 - level);
}

void CFrameBuffer::paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr)
{
	for (int i = 0; i < num; i++) 
	{
		paletteFade(in + i, rgb1, rgb2, i*(255/(num - 1)));
		cmap.transp[in + i] = tr;
	}
}

void CFrameBuffer::paletteSetColor(int i, __u32 rgb, int tr)
{
	cmap.red[i] = (rgb&0xFF0000)>>8;
	cmap.green[i] = (rgb&0x00FF00)   ;
	cmap.blue[i] = (rgb&0x0000FF)<<8;
	cmap.transp[i] = tr;
}

void CFrameBuffer::paletteSet(struct fb_cmap *map)
{
	if (!active)
		return;
	
	//
	if(map == NULL)
		map = &cmap;

	uint32_t rl, ro, gl, go, bl, bo, tl, to;
	
	rl = screeninfo.red.length;
	ro = screeninfo.red.offset;
	gl = screeninfo.green.length;
	go = screeninfo.green.offset;
	bl = screeninfo.blue.length;
	bo = screeninfo.blue.offset;
	tl = screeninfo.transp.length;
	to = screeninfo.transp.offset;

	for (int i = 0; i < 256; i++)
	{
		realcolor[i] = ::make16color(cmap.red[i], cmap.green[i], cmap.blue[i], cmap.transp[i], rl, ro, gl, go, bl, bo, tl, to);
	}
}

//
int CFrameBuffer::limitRadius(const int& dx, const int& dy, int& radius)
{
	if (radius > dx)
		return dx;
	if (radius > dy)
		return dy;
	if (radius > 540)
		return 540;
	return radius;
}

void CFrameBuffer::setCornerFlags(const int& type)
{
	corner_tl = (type & CORNER_TOP_LEFT)     == CORNER_TOP_LEFT;
	corner_tr = (type & CORNER_TOP_RIGHT)    == CORNER_TOP_RIGHT;
	corner_bl = (type & CORNER_BOTTOM_LEFT)  == CORNER_BOTTOM_LEFT;
	corner_br = (type & CORNER_BOTTOM_RIGHT) == CORNER_BOTTOM_RIGHT;
}

void CFrameBuffer::initQCircle()
{
	/* this table contains the x coordinates for a quarter circle (the bottom right quarter) with fixed
	   radius of 540 px which is the half of the max HD graphics size of 1080 px. So with that table we
	   ca draw boxes with round corners and als circles by just setting dx = dy = radius (max 540). */
	static const int _q_circle[541] = {
		540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540, 540,
		540, 540, 540, 540, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539, 539,
		539, 538, 538, 538, 538, 538, 538, 538, 538, 538, 538, 538, 538, 537, 537, 537, 537, 537, 537, 537,
		537, 537, 536, 536, 536, 536, 536, 536, 536, 536, 535, 535, 535, 535, 535, 535, 535, 535, 534, 534,
		534, 534, 534, 534, 533, 533, 533, 533, 533, 533, 532, 532, 532, 532, 532, 532, 531, 531, 531, 531,
		531, 531, 530, 530, 530, 530, 529, 529, 529, 529, 529, 529, 528, 528, 528, 528, 527, 527, 527, 527,
		527, 526, 526, 526, 526, 525, 525, 525, 525, 524, 524, 524, 524, 523, 523, 523, 523, 522, 522, 522,
		522, 521, 521, 521, 521, 520, 520, 520, 519, 519, 519, 518, 518, 518, 518, 517, 517, 517, 516, 516,
		516, 515, 515, 515, 515, 514, 514, 514, 513, 513, 513, 512, 512, 512, 511, 511, 511, 510, 510, 510,
		509, 509, 508, 508, 508, 507, 507, 507, 506, 506, 506, 505, 505, 504, 504, 504, 503, 503, 502, 502,
		502, 501, 501, 500, 500, 499, 499, 499, 498, 498, 498, 497, 497, 496, 496, 496, 495, 495, 494, 494,
		493, 493, 492, 492, 491, 491, 490, 490, 490, 489, 489, 488, 488, 487, 487, 486, 486, 485, 485, 484,
		484, 483, 483, 482, 482, 481, 481, 480, 480, 479, 479, 478, 478, 477, 477, 476, 476, 475, 475, 474,
		473, 473, 472, 472, 471, 471, 470, 470, 469, 468, 468, 467, 466, 466, 465, 465, 464, 464, 463, 462,
		462, 461, 460, 460, 459, 459, 458, 458, 457, 456, 455, 455, 454, 454, 453, 452, 452, 451, 450, 450,
		449, 449, 448, 447, 446, 446, 445, 445, 444, 443, 442, 441, 441, 440, 440, 439, 438, 437, 436, 436,
		435, 435, 434, 433, 432, 431, 431, 430, 429, 428, 427, 427, 426, 425, 425, 424, 423, 422, 421, 421,
		420, 419, 418, 417, 416, 416, 415, 414, 413, 412, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403,
		403, 402, 401, 400, 399, 398, 397, 397, 395, 394, 393, 393, 392, 391, 390, 389, 388, 387, 386, 385,
		384, 383, 382, 381, 380, 379, 378, 377, 376, 375, 374, 373, 372, 371, 369, 368, 367, 367, 365, 364,
		363, 362, 361, 360, 358, 357, 356, 355, 354, 353, 352, 351, 350, 348, 347, 346, 345, 343, 342, 341,
		340, 339, 337, 336, 335, 334, 332, 331, 329, 328, 327, 326, 324, 323, 322, 321, 319, 317, 316, 315,
		314, 312, 310, 309, 308, 307, 305, 303, 302, 301, 299, 297, 296, 294, 293, 291, 289, 288, 287, 285,
		283, 281, 280, 278, 277, 275, 273, 271, 270, 268, 267, 265, 263, 261, 259, 258, 256, 254, 252, 250,
		248, 246, 244, 242, 240, 238, 236, 234, 232, 230, 228, 225, 223, 221, 219, 217, 215, 212, 210, 207,
		204, 202, 200, 197, 195, 192, 190, 187, 184, 181, 179, 176, 173, 170, 167, 164, 160, 157, 154, 150,
		147, 144, 140, 136, 132, 128, 124, 120, 115, 111, 105, 101,  95,  89,  83,  77,  69,  61,  52,  40,
		 23};
		 
	if (q_circle == NULL)
		q_circle = new int[sizeof(_q_circle) / sizeof(int)];
	memcpy(q_circle, _q_circle, sizeof(_q_circle));
}

bool CFrameBuffer::calcCorners(int *ofs, int *ofl, int *ofr, const int& dy, const int& line, const int& radius, const int& type)
{
// just an multiplicator for all math to reduce rounding errors
#define MUL 32768
	int scl, _ofs = 0;
	bool ret = false;
	
	if (ofl != NULL) 
		*ofl = 0;
	if (ofr != NULL) 
		*ofr = 0;
		
	int scf = (540 * MUL) / ((radius < 1) ? 1 : radius);

	// one of the top corners
	if (line < radius && (type & CORNER_TOP)) 
	{
		// uper round corners
		scl = scf * (radius - line) / MUL;
		if ((scf * (radius - line) % MUL) >= (MUL / 2)) // round up
			scl++;
		_ofs =  radius - (q_circle[scl] * MUL / scf);
		if (ofl != NULL) 
			*ofl = corner_tl ? _ofs : 0;
			
		if (ofr != NULL) 
			*ofr = corner_tr ? _ofs : 0;
	}

	// one of the bottom corners 
	else if ((line >= dy - radius) && (type & CORNER_BOTTOM)) 
	{
		// lower round corners
		scl = scf * (radius - (dy - (line + 1))) / MUL;
		if ((scf * (radius - (dy - (line + 1))) % MUL) >= (MUL / 2)) // round up
			scl++;
		_ofs =  radius - (q_circle[scl] * MUL / scf);
		
		if (ofl != NULL) 
			*ofl = corner_bl ? _ofs : 0;
		if (ofr != NULL) 
			*ofr = corner_br ? _ofs : 0;
	}
	else
		ret = true;

	if (ofs != NULL) 
		*ofs = _ofs;
		
	return ret;
}

void CFrameBuffer::paintHLineRelInternal2Buf(const int &x, const int &dx, const int &y, const int &box_dx, const fb_pixel_t &col, fb_pixel_t *buf)
{
	uint8_t * pos = ((uint8_t *)buf) + x * sizeof(fb_pixel_t) + box_dx * sizeof(fb_pixel_t) * y;
	fb_pixel_t * dest = (fb_pixel_t *)pos;
	
	for (int i = 0; i < dx; i++)
		*(dest++) = col;		
}

fb_pixel_t *CFrameBuffer::paintBoxRel2Buf(const int dx, const int dy, const fb_pixel_t col, int radius, int type)
{
	if (!getActive())
		return NULL;

	if (dx == 0 || dy == 0) 
	{
		return NULL;
	}

	fb_pixel_t *pixBuf = NULL;
	
	pixBuf = (fb_pixel_t *)malloc(dx*dy*sizeof(fb_pixel_t));
	
	if (pixBuf == NULL) 
	{
		dprintf(DEBUG_NORMAL, "CFrameBuffer::paintBoxRel2Buf: malloc error\n");
		return NULL;
	}

	memset((void*)pixBuf, '\0', dx*dy*sizeof(fb_pixel_t));

	if (type && radius) 
	{
		setCornerFlags(type);
		radius = limitRadius(dx, dy, radius);
 
		for (int line = 0; line < dy; line++)
		{
			int ofl, ofr;
			calcCorners(NULL, &ofl, &ofr, dy, line, radius, type);

			if (dx - ofr - ofl < 1) 
			{
				line++;
				continue;
			}

			paintHLineRelInternal2Buf(ofl, dx - ofl - ofr, line, dx, col, pixBuf);
		}
	} 
	else 
	{
		fb_pixel_t *bp = pixBuf;

		for (int line = 0; line < dy; line++)
		{
			for (int pos = 0; pos < dx; pos++)
				*(bp + pos) = col;
			bp += dx;
		}
	}

	return pixBuf;
}

//
void CFrameBuffer::paintBoxRel(const int x, const int y, const int dx, const int dy, fb_pixel_t col, int radius, int type, int mode, int direction, int intensity, int grad_type)
{
	if (!getActive())
		return;

	fb_pixel_t MASK = 0xFFFFFFFF;

	// boxBuf
	fb_pixel_t *boxBuf = paintBoxRel2Buf(dx, dy, (mode > NOGRADIENT)? MASK : col, radius, type);
	
        if (!boxBuf)
               return;

	// gradientBuf
	if(mode > NOGRADIENT)
	{
		// gradient mode / direction / type
		fb_pixel_t *gradientBuf = NULL;
		
		if (grad_type == GRADIENT_ONECOLOR)
			gradientBuf = gradientOneColor(col, (direction == GRADIENT_VERTICAL)? dy : dx, mode, intensity);
		else if (grad_type == GRADIENT_COLOR2TRANSPARENT)
			gradientBuf = gradientColorToTransparent(col, (direction == GRADIENT_VERTICAL)? dy : dx, mode, intensity);
		else if (grad_type == GRADIENT_COLOR2COLOR) // FIXME:
			gradientBuf = gradientColorToColor(COL_SILVER_PLUS_0, col, (direction == GRADIENT_VERTICAL)? dy : dx, mode, intensity);

		fb_pixel_t *bp = boxBuf;
		fb_pixel_t *gra = gradientBuf;

		// vertical
		if (direction == GRADIENT_VERTICAL)
		{
			for (int pos = 0; pos < dx; pos++) 
			{
				for(int count = 0; count < dy; count++) 
				{
					if (*(bp + pos) == MASK)
						*(bp + pos) = (fb_pixel_t)(*(gra + count));
					bp += dx;
				}
				bp = boxBuf;
			}
		}
		else
		{
			// horizontal
			for (int line = 0; line < dy; line++) 
			{
				int gra_pos = 0;
				for (int pos = 0; pos < dx; pos++) 
				{
					if ((*(bp + pos) == MASK) && (pos >= 0) && (gra_pos < dx))
					{
						*(bp + pos) = (fb_pixel_t)(*(gra + gra_pos));
						gra_pos++;
					}
				}
				bp += dx;
			}
		}
		
		free(gradientBuf);
	}

	if(type && radius)
		blitRoundedBox2FB(boxBuf, dx, dy, x, y);
	else
		blitBox2FB(boxBuf, dx, dy, x, y);

	free(boxBuf);
}

void CFrameBuffer::paintVLine(int x, int ya, int yb, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * ya;

	int dy = yb - ya;
	
	for (int count = 0; count < dy; count++) 
	{
		*(fb_pixel_t *)pos = col;
		pos += stride;
	}	
}

void CFrameBuffer::paintVLineRel(int x, int y, int dy, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;

	for(int count = 0; count < dy; count++) 
	{
		*(fb_pixel_t *)pos = col;
		pos += stride;
	}	
}

void CFrameBuffer::paintHLine(int xa, int xb, int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)lfb) + xa * sizeof(fb_pixel_t) + stride * y;

	int dx = xb - xa;
	fb_pixel_t * dest = (fb_pixel_t *)pos;
	
	for (int i = 0; i < dx; i++)
		*(dest++) = col;	
}

void CFrameBuffer::paintHLineRel(int x, int dx, int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;

	fb_pixel_t * dest = (fb_pixel_t *)pos;
	
	for (int i = 0; i < dx; i++)
		*(dest++) = col;	
}

void CFrameBuffer::paintFrameBox(const int x, const int y, const int dx, const int dy, const fb_pixel_t col)
{
	paintVLineRel(x, y, dy, col);
	paintVLineRel(x + dx - 1, y, dy, col);
	paintHLineRel(x, dx, y, col);
	paintHLineRel(x, dx, y + dy - 1, col);
}

void CFrameBuffer::setIconBasePath(const std::string & iconPath)
{
	dprintf(DEBUG_INFO, "CFrameBuffer::setIconBasePath: %s\n", iconPath.c_str());
	
	iconBasePath = iconPath;
}

void CFrameBuffer::setHintBasePath(const std::string & hintPath)
{
	dprintf(DEBUG_INFO, "CFrameBuffer::setHintBasePath: %s\n", hintPath.c_str());
	
	hintBasePath = hintPath;
}

void CFrameBuffer::setButtonBasePath(const std::string & buttonPath)
{
	dprintf(DEBUG_INFO, "CFrameBuffer::setButtonBasePath: %s\n", buttonPath.c_str());
	
	buttonBasePath = buttonPath;
}

void CFrameBuffer::setSpinnerBasePath(const std::string & spinnerPath)
{
	dprintf(DEBUG_INFO, "CFrameBuffer::setSpinnerBasePath: %s\n", spinnerPath.c_str());
	
	spinnerBasePath = spinnerPath;
}

// get icon size
void CFrameBuffer::getIconSize(const char * const filename, int * width, int * height)
{
	if(filename == NULL)
		return;
	
	int icon_fd;
	std::string iconfile = filename;
	
	if( !strstr(iconfile.c_str(), ".png"))
		iconfile = iconBasePath + filename + ".png";
		
	if (!file_exists(iconfile.c_str()))
		iconfile = buttonBasePath + filename + ".png";
		
	if (!file_exists(iconfile.c_str()))
		iconfile = spinnerBasePath + filename + ".png";

	icon_fd = open(iconfile.c_str(), O_RDONLY);

	if (icon_fd == -1)
	{
		// raw check for fullname icon
		std::string iconfile1 = iconBasePath + filename + ".raw";
			
		icon_fd = open(iconfile1.c_str(), O_RDONLY);
		
		if (icon_fd == -1)
		{
			*width = 0;
			*height = 0;

			return;
		}
		else
		{
			// raw
			struct rawHeader header;
			uint16_t x;
			uint16_t y;
				
			read(icon_fd, &header, sizeof(struct rawHeader));
			
			x = (header.width_hi << 8) | header.width_lo;
			y = (header.height_hi << 8) | header.height_lo;
				
			*width = x;
			*height = y;
		}
		close(icon_fd);
	}
	else
	{
		int x = 0;
		int y = 0;
		int bpp = 0;
		int channels = 0;
		
		getSize(iconfile.c_str(), &x, &y, &bpp, &channels);
		
		*width = x;
		*height = y;
	}

	close(icon_fd);
}

//
void CFrameBuffer::paintIcon8(const std::string &filename, const int x, const int y, const unsigned char offset)
{
	if (!getActive())
		return;

	struct rawHeader header;
	uint16_t width, height;
	int _fd;

	_fd = open((iconBasePath + filename).c_str(), O_RDONLY);

	if (_fd == -1) 
	{
		printf("CFrameBuffer::paintIcon8: error while loading icon: %s%s\n", iconBasePath.c_str(), filename.c_str());
		return;
	}

	read(_fd, &header, sizeof(struct rawHeader));

	width  = (header.width_hi  << 8) | header.width_lo;
	height = (header.height_hi << 8) | header.height_lo;

	unsigned char pixbuf[768];

	uint8_t * d = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;
	
	fb_pixel_t * d2;

	for (int count = 0; count < height; count ++ ) 
	{
		read(_fd, &pixbuf[0], width );
		unsigned char *pixpos = &pixbuf[0];
		d2 = (fb_pixel_t *) d;
		
		for (int count2 = 0; count2 < width; count2 ++ ) 
		{
			unsigned char color = *pixpos;
			
			if (color != header.transp) 
			{
				paintPixel(d2, color + offset);
			}
			d2++;
			pixpos++;
		}
		d += stride;
	}
	close(_fd);
}

// paint icon raw
void CFrameBuffer::paintIconRaw(const std::string & filename, const int x, const int y, const int h, const unsigned char offset)
{
	if (!getActive())
		return;
	
	struct rawHeader header;
	
	int width, height;
	int lfd;
	
	fb_pixel_t * data;
	fb_pixel_t * tmp_data;
	int dsize;
	int  yy = y;

	std::string newname = iconBasePath + filename.c_str() + ".raw";

	lfd = open(newname.c_str(), O_RDONLY);

	if (lfd == -1) 
	{
		dprintf(DEBUG_NORMAL, "paintIcon: error while loading icon: %s\n", newname.c_str());
		return;
	}
		
	read(lfd, &header, sizeof(struct rawHeader));

	width  = (header.width_hi  << 8) | header.width_lo;
	height = (header.height_hi << 8) | header.height_lo;
	dsize = width*height*sizeof(fb_pixel_t);
	tmp_data = (fb_pixel_t*)malloc(dsize);
	data = tmp_data;
	
	//	
	unsigned char pixbuf[768];
	
	for (int count = 0; count < height; count ++ ) 
	{
		read(lfd, &pixbuf[0], width >> 1 );
		unsigned char *pixpos = &pixbuf[0];
		
		for (int count2 = 0; count2 < width >> 1; count2 ++ ) 
		{
			unsigned char compressed = *pixpos;
			unsigned char pix1 = (compressed & 0xf0) >> 4;
			unsigned char pix2 = (compressed & 0x0f);
			
			if (pix1 != header.transp)
				*data++ = realcolor[pix1 + offset];
			else
				*data++ = 0;
			if (pix2 != header.transp)
				*data++ = realcolor[pix2 + offset];
			else
				*data++ = 0;

			pixpos++;
		}
	}
		
	close(lfd);
	data = tmp_data;

	if (h != 0)
		yy += (h - height) / 2;	

	blitBox2FB(data, width, height, x, yy);

	free(tmp_data);
}

//
void CFrameBuffer::paintIcon(const std::string &filename, const int x, const int y, const int h, int width, int height)
{
	dprintf(DEBUG_DEBUG, "CFrameBuffer::paintIcon: %s\n", filename.c_str());
	
	if (!getActive())
		return;
	
	fb_pixel_t * data;
	int  yy = y;

	// check into iconBasePath
	std::string newname = iconBasePath + filename.c_str() + ".png";
		
	if(width == 0 || height == 0)	
		getIconSize(newname.c_str(), &width, &height);

	data = (fb_pixel_t *)getARGB32Image(newname, width, height, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha));
	
	// check into buttonBasePath
	if(!data) 
	{
		newname = buttonBasePath +filename.c_str() + ".png";
		
		if(width == 0 || height == 0)	
			getIconSize(newname.c_str(), &width, &height);

		data = (fb_pixel_t *)getARGB32Image(newname, width, height,  convertSetupAlpha2Alpha(g_settings.menu_Content_alpha));
	}
	
	// check into spinnerBasePath	
	if(!data) 
	{
		newname = spinnerBasePath +filename.c_str() + ".png";
		
		if(width == 0 || height == 0)	
			getIconSize(newname.c_str(), &width, &height);

		data = (fb_pixel_t *)getARGB32Image(newname, width, height,  convertSetupAlpha2Alpha(g_settings.menu_Content_alpha));
	}
	
	// full path
	if (!data)
	{
		dprintf(DEBUG_DEBUG, "CFrameBuffer::paintIcon: %s\n", filename.c_str());
		
		newname = filename;
			
		if(width == 0 || height == 0)	
			getIconSize(newname.c_str(), &width, &height);

		data = (fb_pixel_t *)getARGB32Image(newname, width, height,  convertSetupAlpha2Alpha(g_settings.menu_Content_alpha));
	}

	if(data) 
	{	
		if (h != 0)
			yy += (h - height) / 2;	

		blitBox2FB(data, width, height, x, yy, 0, 0, true);
		free(data);
	}
}

// paintHintIcon
void CFrameBuffer::paintHintIcon(const std::string& filename, int posx, int posy, int width, int height)
{
	dprintf(DEBUG_DEBUG, "CFrameBuffer::paintHintIcon: %s\n", filename.c_str());
	
	if (!getActive())
		return;

	if (::file_exists(filename.c_str()))
		displayImage(filename, posx, posy, width, height);
	else
	{
		std::string newname = hintBasePath + filename.c_str() + ".png";		

		displayImage(newname, posx, posy, width, height);
	}
}

void CFrameBuffer::loadPal(const std::string &filename, const unsigned char offset, const unsigned char endidx)
{
	if (!getActive())
		return;

	struct rgbData rgbdata;
	int _fd;

	_fd = open((iconBasePath + filename).c_str(), O_RDONLY);

	if (_fd == -1) 
	{
		printf("CFrameBuffer::loadPal: error while loading palette: %s%s\n", iconBasePath.c_str(), filename.c_str());
		return;
	}

	int pos = 0;
	int readb = read(_fd, &rgbdata,  sizeof(rgbdata) );
	
	while(readb) 
	{
		__u32 rgb = (rgbdata.r<<16) | (rgbdata.g<<8) | (rgbdata.b);
		int colpos = offset + pos;
		
		if( colpos > endidx)
			break;

		paletteSetColor(colpos, rgb, 0xFF);
		readb = read(_fd, &rgbdata,  sizeof(rgbdata) );
		pos++;
	}
	
	paletteSet(&cmap);
	close(_fd);
}

void CFrameBuffer::paintPixel(const int x, const int y, const fb_pixel_t col)
{
	if (!getActive())
		return;

	lfb[y*(stride / sizeof(fb_pixel_t)) + x] = col;
}

void CFrameBuffer::paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col)
{
	if (!getActive())
		return;
	
	int dx = abs (xa - xb);
	int dy = abs (ya - yb);
	int x;
	int y;
	int End;
	int step;

	if ( dx > dy )
	{
		int	p = 2 * dy - dx;
		int	twoDy = 2 * dy;
		int	twoDyDx = 2 * (dy-dx);

		if ( xa > xb )
		{
			x = xb;
			y = yb;
			End = xa;
			step = ya < yb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = xb;
			step = yb < ya ? -1 : 1;
		}

		paintPixel(x, y, col);

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
			paintPixel(x, y, col);
		}
	}
	else
	{
		int	p = 2 * dx - dy;
		int	twoDx = 2 * dx;
		int	twoDxDy = 2 * (dx-dy);

		if ( ya > yb )
		{
			x = xb;
			y = yb;
			End = ya;
			step = xa < xb ? -1 : 1;
		}
		else
		{
			x = xa;
			y = ya;
			End = yb;
			step = xb < xa ? -1 : 1;
		}

		paintPixel(x, y, col);

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
			paintPixel(x, y, col);
		}
	}
}

void CFrameBuffer::setBackgroundColor(const fb_pixel_t color)
{
	backgroundColor = color;
}

bool CFrameBuffer::loadBackgroundPic(const std::string &filename, bool show)
{
	if ((backgroundFilename == filename) && (background))
		return true;
	
	dprintf(DEBUG_INFO, "CFrameBuffer::loadBackgroundPic: %s\n", filename.c_str());	

	if (background)
		free(background);
	
	// get bg image
	background = (fb_pixel_t *)getARGB32Image(iconBasePath + filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT);
	
	if(!background) 
		background = (fb_pixel_t *)getARGB32Image(filename, BACKGROUNDIMAGEWIDTH, BACKGROUNDIMAGEHEIGHT);

	// if not found
	if (background == NULL) 
	{
		background = 0;
		return false;
	}

	backgroundFilename = filename;
	
	if(show) 
	{
		useBackgroundPaint = true;
		paintBackground();
	}
	
	return true;
}

void CFrameBuffer::useBackground(bool ub)
{
	useBackgroundPaint = ub;
	
	if(!useBackgroundPaint) 
	{
		free(background);
		background = 0;
	}
}

bool CFrameBuffer::getuseBackground(void)
{
	return useBackgroundPaint;
}

void CFrameBuffer::saveBackgroundImage(void)
{
	if (backupBackground != NULL)
		delete[] backupBackground;

	backupBackground = background;
	useBackgroundPaint = false;
	background = NULL;
}

void CFrameBuffer::restoreBackgroundImage(void)
{
	fb_pixel_t * tmp = background;

	if (backupBackground != NULL)
	{
		background = backupBackground;
		backupBackground = NULL;
	}
	else
		useBackground(false); 		// <- necessary since no background is available

	if (tmp != NULL)
		delete[] tmp;
}

void CFrameBuffer::paintBackgroundBoxRel(int x, int y, int dx, int dy)
{
	if (!getActive())
		return;

	if(!useBackgroundPaint)
	{		
		paintBoxRel(x, y, dx, dy, backgroundColor);
	}
	else
	{
		uint8_t * fbpos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;

		fb_pixel_t * bkpos = background + x + BACKGROUNDIMAGEWIDTH * y;

		for(int count = 0; count < dy; count++)
		{
			memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
			fbpos += stride;
			bkpos += BACKGROUNDIMAGEWIDTH;
		}
	}
}

void CFrameBuffer::paintBackground()
{
	if (!getActive())
		return;

	if (useBackgroundPaint && (background != NULL))
	{
		for (int i = 0; i < BACKGROUNDIMAGEHEIGHT; i++)
			memcpy(((uint8_t *)lfb) + i * stride, (background + i * BACKGROUNDIMAGEWIDTH), BACKGROUNDIMAGEWIDTH * sizeof(fb_pixel_t));
	}
	else
	{
		paintBoxRel(0, 0, xRes, yRes, backgroundColor);
	}	
}

void CFrameBuffer::saveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	if (!getActive())
		return;

	uint8_t * pos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;

	fb_pixel_t * bkpos = memp;

	for (int count = 0; count < dy; count++) 
	{
		fb_pixel_t * dest = (fb_pixel_t *)pos;
		for (int i = 0; i < dx; i++)
			*(bkpos++) = *(dest++);
		
		pos += stride;
	}
}

void CFrameBuffer::restoreScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp)
{
	if (!getActive())
		return;

	uint8_t * fbpos = ((uint8_t *)lfb) + x * sizeof(fb_pixel_t) + stride * y;
	
	fb_pixel_t * bkpos = memp;

	for (int count = 0; count < dy; count++)
	{
		memcpy(fbpos, bkpos, dx * sizeof(fb_pixel_t));
		fbpos += stride;
		bkpos += dx;
	}
}

void CFrameBuffer::clearFrameBuffer()
{
	paintBackground();
}

// blitRoundedBox2FB
void CFrameBuffer::blitRoundedBox2FB(void *boxBuf, const uint32_t &width, const uint32_t &height, const uint32_t &xoff, const uint32_t &yoff)
{
	uint32_t xc = (width > xRes) ? (uint32_t)xRes : width;
	uint32_t yc = (height > yRes) ? (uint32_t)yRes : height;
	
	uint32_t swidth = stride / sizeof(fb_pixel_t);

	fb_pixel_t *data = (fb_pixel_t *)boxBuf;
	fb_pixel_t *fbp = (fb_pixel_t *)lfb + (swidth * yoff);
	
	if (!data)
		return;
 
 	uint32_t line = 0;
	
	while (line < yc)
	{
		fb_pixel_t *pixpos = &data[(line) * xc];
		
		for (uint32_t pos = xoff; pos < xoff + xc; pos++) 
		{
			if (*pixpos)
				*(fbp + pos) = *pixpos;
			pixpos++;
		}
		fbp += swidth;
		line++;
	}
}

// blitBox2FB
void CFrameBuffer::blitBox2FB(void * fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp, uint32_t yp, bool transp)
{
	int xc = (width > xRes) ? xRes : width;
	int yc = (height > yRes) ? yRes : height;
	
	fb_pixel_t *data = (fb_pixel_t *) fbbuff;
	uint8_t *d = ((uint8_t *)lfb) + xoff * sizeof(fb_pixel_t) + stride * yoff;
	fb_pixel_t *d2;
	
	if (!data)
		return;

	for (int count = 0; count < yc - yp; count++ ) 
	{
		fb_pixel_t * pixpos = &data[(count + yp) * width];
		d2 = (fb_pixel_t *) d;
		
		for (int count2 = 0; count2 < xc - xp; count2++ ) 
		{
			fb_pixel_t pix = *(pixpos + xp);
			
			if (!transp || (pix & 0xff000000) == 0xff000000)
				*d2 = pix;
			else
			{
				uint8_t *in = (uint8_t *)(pixpos + xp);
				uint8_t *out = (uint8_t *)d2;
				
#if __BYTE_ORDER == __LITTLE_ENDIAN
				int a = in[3];
#elif __BYTE_ORDER == __BIG_ENDIAN
				int a = in[0];
				out++; 
				in++;
#endif				
				*out = (*out + ((*in - *out) * a) / 256);
				in++; 
				out++;
				*out = (*out + ((*in - *out) * a) / 256);
				in++; 
				out++;
				*out = (*out + ((*in - *out) * a) / 256);
			}
			
			d2++;
			pixpos++;
		}
		d += stride;
	}	
}

//
#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC 	_IOW('F', 0x20, __u32)
#endif

#ifndef FBIO_BLIT
#define FBIO_SET_MANUAL_BLIT 	_IOW('F', 0x21, __u8)
#define FBIO_BLIT 		0x22
#endif

void CFrameBuffer::enableManualBlit()
{
#if !defined USE_OPENGL  
	unsigned char tmp = 1;
	
	if (ioctl(fd, FBIO_SET_MANUAL_BLIT, &tmp) < 0) 
	{
		perror("FB: FBIO_SET_MANUAL_BLIT");
	}
	else 
	{
		m_manual_blit = 1;
	}
#endif	
}

void CFrameBuffer::disableManualBlit()
{
#if !defined USE_OPENGL  
	unsigned char tmp = 0;
	
	if (ioctl(fd,FBIO_SET_MANUAL_BLIT, &tmp) < 0) 
	{
		perror("FB: FBIO_SET_MANUAL_BLIT");
	}
	else 
	{
		m_manual_blit = 0;
	}
#endif	
}

void CFrameBuffer::blit(int mode3d)
{
#if defined USE_OPENGL  
	mpGLThreadObj->blit();
#elif defined (__sh__)
	STMFBIO_BLT_DATA  bltData; 
	memset(&bltData, 0, sizeof(STMFBIO_BLT_DATA)); 

	bltData.operation  = BLT_OP_COPY;
	bltData.ulFlags |= BLT_OP_FLAGS_BLEND_SRC_ALPHA | BLT_OP_FLAGS_BLEND_DST_COLOR;

	// src
	bltData.srcOffset  = 1920 * 1080 * 4;
	bltData.srcPitch   = xRes * 4; // stride

	bltData.src_left   = 0; 
	bltData.src_top    = 0; 
	bltData.src_right  = xRes; 
	bltData.src_bottom = yRes;

		
	bltData.srcFormat = SURF_ARGB8888;
	bltData.srcMemBase = STMFBGP_FRAMEBUFFER;
	
	// get variable screeninfo
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) == -1)
	{
		perror("frameBuffer <FBIOGET_VSCREENINFO>");
	}

	// dst
	bltData.dstOffset  = 0; 
	bltData.dstPitch   = screeninfo.xres * 4;

	bltData.dst_left   = 0; 
	bltData.dst_top    = 0;
	
	// right
	if(mode3d == THREE_SIDE_BY_SIDE)
		bltData.dst_right  = screeninfo.xres/2; 
	else
		bltData.dst_right  = screeninfo.xres; 
	
	// buttom
	if(mode3d == THREE_TOP_AND_BUTTOM)
		bltData.dst_bottom = screeninfo.yres/2;
	else
		bltData.dst_bottom = screeninfo.yres;

	bltData.dstFormat = SURF_ARGB8888;		
	bltData.dstMemBase = STMFBGP_FRAMEBUFFER;

	if ( (bltData.dst_right > screeninfo.xres) || (bltData.dst_bottom > screeninfo.yres) )
	{
		printf("CFrameBuffer::blit: values out of range desXb:%d desYb:%d\n", bltData.dst_right, bltData.dst_bottom);
	}

	if (ioctl(fd, STMFBIO_BLT, &bltData ) < 0) 
		perror("STMFBIO_BLIT");
	
	// sync bliter
	if(ioctl(fd, STMFBIO_SYNC_BLITTER) < 0)
		perror("ioctl STMFBIO_SYNC_BLITTER");
	
	if(mode3d != 0)
	{
		if(mode3d == THREE_SIDE_BY_SIDE)
			bltData.dst_left = screeninfo.xres/2;
		if(mode3d == THREE_TOP_AND_BUTTOM)
			bltData.dst_top = screeninfo.yres/2;
		
		
		bltData.dst_right  = screeninfo.xres;
		bltData.dst_bottom = screeninfo.yres;

		if (ioctl(fd, STMFBIO_BLT, &bltData) < 0)
			perror("ioctl STMFBIO_BLT");
		
		if(ioctl(fd, STMFBIO_SYNC_BLITTER) < 0)
			perror("ioctl STMFBIO_SYNC_BLITTER");
	}
#else
	// blit
	if (m_manual_blit == 1) 
	{
		if (ioctl(fd, FBIO_BLIT) < 0)
			perror("FBIO_BLIT");		
	}
#endif	
}

//// display image
void CFrameBuffer::displayImage(const std::string &name, int posx, int posy, int width, int height, int x_pan, int y_pan, ScalingMode scaletype)
{
	dprintf(DEBUG_DEBUG, "CFrameBuffer::displayImage %s\n", name.c_str());
	
	if(!getActive())
		return;
		
	int i_w, i_h, i_bpp, i_chans;
	
	::getSize(name.c_str(), &i_w, &i_h, &i_bpp, &i_chans);
	
	if (width == 0 && height == 0)
	{
		width = i_w;
		height = i_h;
	}
	
	bool isPNG = false;
	
	if( name.find(".png") == (name.length() - 4) )
		isPNG = true;
	
	fb_pixel_t *data = (fb_pixel_t *)getARGB32Image(name, width, height,  convertSetupAlpha2Alpha(g_settings.menu_Content_alpha), scaletype);

	if(data) 
	{
		blitBox2FB(data, width, height, posx, posy, x_pan, y_pan, isPNG? true : false);
		free(data);
	}
}

