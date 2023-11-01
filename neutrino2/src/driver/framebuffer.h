/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: framebuffer.h 30.10.2023 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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


#ifndef __framebuffer__
#define __framebuffer__

#include <config.h>

#include <stdint.h>

#include <linux/fb.h>
#include <linux/vt.h>

#include <string>
#include <map>

// stmfb
#ifdef __sh__
#include <linux/stmfb.h>
#endif


#ifdef USE_OPENGL
class GLThreadObj;
#endif

// 32 bit
#define fb_pixel_t uint32_t		// unsigned int 32 bit data

typedef struct fb_var_screeninfo t_fb_var_screeninfo;

#define CORNER_NONE		0x0
#define CORNER_TOP_LEFT		0x1
#define CORNER_TOP_RIGHT	0x2
#define CORNER_TOP		0x3
#define CORNER_BOTTOM_RIGHT	0x4
#define CORNER_RIGHT		0x6
#define CORNER_BOTTOM_LEFT	0x8
#define CORNER_LEFT		0x9
#define CORNER_BOTTOM		0xC
#define CORNER_ALL		0xF
#define CORNER_BOTH 		CORNER_ALL

// resolution
#define DEFAULT_XRES		1280
#define DEFAULT_YRES		720

// bitmap
#define DEFAULT_BPP		32	// 32 bit

#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1	/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

// gradient mode
enum {
	NOGRADIENT,
	DARK2LIGHT,
	LIGHT2DARK,
	DARK2LIGHT2DARK,
	LIGHT2DARK2LIGHT
};

// gradient intensity
enum {
	INT_LIGHT,
	INT_NORMAL,
	INT_EXTENDED
};

// gradient direction
enum {
	GRADIENT_HORIZONTAL,
	GRADIENT_VERTICAL
};

// gradient type
enum {
	GRADIENT_COLOR2TRANSPARENT,
	GRADIENT_ONECOLOR,
	GRADIENT_COLOR2COLOR
};

// png/jpg/bmp/gif/crw/svg
struct cformathandler 
{
	struct cformathandler * next;
	int (*get_size)(const char *,int *,int*, int, int);
	int (*get_pic)(const char *,unsigned char **,int* ,int*);
	int (*id_pic)(const char *);
};
	
typedef struct cformathandler CFormathandler;

// png/jpg/bmp/gif/crw
CFormathandler* fh_getsize(const char * name, int * x, int * y, int width_wanted, int height_wanted);

class CFrameBuffer
{
	public:
		// transparency
		enum 
		{
			TM_EMPTY  = 0,
			TM_NONE   = 1,
			TM_BLACK  = 2,
			TM_INI    = 3
		};
				
		// scaling mode
		enum ScalingMode
		{
			NONE = 0,
			SIMPLE = 1,
			COLOR = 2
		};

		// three mode
		enum
		{
			THREE_NONE = 0,
			THREE_SIDE_BY_SIDE,
			THREE_TOP_AND_BUTTOM
		};
	
	private:
		CFrameBuffer();

		struct rgbData
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};

		struct rawHeader
		{
			uint8_t width_lo;
			uint8_t width_hi;
			uint8_t height_lo;
			uint8_t height_hi;
			uint8_t transp;
		};

		std::string	iconBasePath;
		std::string	hintBasePath;
		std::string	buttonBasePath;
		std::string	spinnerBasePath;

		int             fd;
		fb_pixel_t *    lfb;
		int		available;
		fb_pixel_t *    background;
		fb_pixel_t *    backupBackground;
		fb_pixel_t      backgroundColor;
		std::string     backgroundFilename;
		bool            useBackgroundPaint;
		unsigned int	xRes, yRes, stride, bpp;

		t_fb_var_screeninfo screeninfo, oldscreen;

		fb_cmap cmap;
		__u16 red[256], green[256], blue[256], trans[256];

		bool active;
		
		int m_number_of_pages;
		int m_manual_blit;
		
#ifdef USE_OPENGL
		GLThreadObj *mpGLThreadObj; // the thread object
#endif	

		//
		int  limitRadius(const int& dx, const int& dy, int& radius);
		void setCornerFlags(const int& type);
		void initQCircle();
		inline int calcCornersOffset(const int& dy, const int& line, const int& radius, const int& type) { int ofs = 0; calcCorners(&ofs, NULL, NULL, dy, line, radius, type); return ofs; }
		bool calcCorners(int *ofs, int *ofl, int *ofr, const int& dy, const int& line, const int& radius, const int& type);
		void paintHLineRelInternal2Buf(const int& x, const int& dx, const int& y, const int& box_dx, const fb_pixel_t& col, fb_pixel_t* buf);
		fb_pixel_t* paintBoxRel2Buf(const int dx, const int dy, const fb_pixel_t col, int radius = 0, int type = CORNER_ALL);	

	public:
		~CFrameBuffer();

		int *q_circle;
		bool corner_tl, corner_tr, corner_bl, corner_br;
		
		// 16/32 bits
		fb_pixel_t realcolor[256];

		static CFrameBuffer * getInstance();

		void init(const char * const fbDevice = "/dev/fb0");		
		void setFrameBufferMode(unsigned int xRes, unsigned int yRes, unsigned int bpp);
		int setMode(unsigned int x = DEFAULT_XRES, unsigned int y = DEFAULT_YRES, unsigned int _bpp = 32);

		int getFileHandle() const; //only used for plugins (games) !!
		t_fb_var_screeninfo *getScreenInfo();

		fb_pixel_t * getFrameBufferPointer() const; // pointer to framebuffer
		unsigned int getStride() const;             // size of a single line in the framebuffer (in bytes)
		unsigned int getScreenWidth(bool real = false);
		unsigned int getScreenHeight(bool real = false); 
		unsigned int getScreenX(bool real = false);
		unsigned int getScreenY(bool real = false);
		unsigned int getAvailableMem() const;             	// size of a available mem occupied by the framebuffer
		
		bool getActive() const;                     		// is framebuffer active?
		void setActive(bool enable);                		// is framebuffer active?

		void setBlendMode(uint8_t mode);
		void setBlendLevel(int blev);

		// Palette stuff
		void paletteFade(int i, __u32 rgb1, __u32 rgb2, int level);
		void paletteGenFade(int in, __u32 rgb1, __u32 rgb2, int num, int tr = 0);
		void paletteSetColor(int i, __u32 rgb, int tr);
		void paletteSet(struct fb_cmap * map = NULL);

		// paint functions
		inline void paintPixel(fb_pixel_t * const dest, const uint8_t color) const
		{			
			// 16/32 bit
			*dest = realcolor[color];
		};

		//
		void paintPixel(const int x, const int y, const fb_pixel_t col);
		
		void paintBoxRel(const int x, const int y, const int dx, const int dy, fb_pixel_t col, int radius = 0, int type = CORNER_NONE, int mode = NOGRADIENT, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int grad_type = GRADIENT_ONECOLOR);

		inline void paintBox(int xa, int ya, int xb, int yb, const fb_pixel_t col, int radius = 0, int type = CORNER_NONE, int mode = NOGRADIENT, int direction = GRADIENT_VERTICAL, int intensity = INT_LIGHT, int grad_type = GRADIENT_ONECOLOR) { paintBoxRel(xa, ya, xb - xa, yb - ya, col, radius, type, mode, direction, intensity, grad_type); }

		void paintLine(int xa, int ya, int xb, int yb, const fb_pixel_t col);

		void paintVLine(int x, int ya, int yb, const fb_pixel_t col);
		void paintVLineRel(int x, int y, int dy, const fb_pixel_t col);

		void paintHLine(int xa, int xb, int y, const fb_pixel_t col);
		void paintHLineRel(int x, int dx, int y, const fb_pixel_t col);

		void paintFrameBox(const int x, const int y, const int dx, const int dy, const fb_pixel_t col);

		//
		void setIconBasePath(const std::string& iconPath);
		void setHintBasePath(const std::string& hintPath);
		void setButtonBasePath(const std::string& buttonPath);
		void setSpinnerBasePath(const std::string& spinnerPath);
		
		//
		std::string getIconBasePath(void){return iconBasePath;};
		std::string getButtonBasePath(void){return buttonBasePath;};
		std::string getHintBasePath(void){return hintBasePath;};
		std::string getSpinnerBasePath(void){return spinnerBasePath;};

		//
		void getIconSize(const char * const filename, int* width, int *height);
		bool paintIcon(const std::string & filename, const int x, const int y, const int h = 0, bool paint = true, int width = 0, int height = 0);
		bool paintHintIcon(const std::string& filename, int posx, int posy, int width , int height);
		bool paintIcon8(const std::string & filename, const int x, const int y, const unsigned char offset = 0);
		bool paintIconRaw(const std::string & filename, const int x, const int y, const int h = 0, const unsigned char offset = 1, bool paint = true);
		void loadPal(const std::string & filename, const unsigned char offset = 0, const unsigned char endidx = 255);
		
		// background
		int getBackgroundColor() { return backgroundColor;}
		void setBackgroundColor(const fb_pixel_t color);
		void useBackground(bool ub);
		bool getuseBackground(void);

		void saveBackgroundImage(void);  // <- implies useBackground(false);
		void restoreBackgroundImage(void);

		void paintBackgroundBoxRel(int x, int y, int dx, int dy);
		inline void paintBackgroundBox(int xa, int ya, int xb, int yb) { paintBackgroundBoxRel(xa, ya, xb - xa, yb - ya); }

		void paintBackground();
		bool loadBackgroundPic(const std::string& filename, bool show = true);

		//
		void saveScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp);
		void restoreScreen(int x, int y, int dx, int dy, fb_pixel_t * const memp);

		void clearFrameBuffer();

		//
		void blitBox2FB(const fb_pixel_t* boxBuf, const uint32_t& width, const uint32_t& height, const uint32_t& xoff, const uint32_t& yoff);
		void blit2FB(void * fbbuff, uint32_t width, uint32_t height, uint32_t xoff, uint32_t yoff, uint32_t xp = 0, uint32_t yp = 0, bool transp = false);

		// blit
		void enableManualBlit();
		void disableManualBlit();
		void blit(int mode3d = THREE_NONE);
		
		//
		void getSize(const std::string &name, int * width, int * height, int * nbpp);
		unsigned char* resize(unsigned char * origin, int ox, int oy, int dx, int dy, ScalingMode type, unsigned char * dst = NULL, bool alpha = false);

		fb_pixel_t * getImage(const std::string& name, int width, int height);
		void * convertRGB2FB(unsigned char * rgbbuff, unsigned long x, unsigned long y, int transp = 0xFF, int m_transparent = TM_BLACK, bool alpha = false);
		void displayRGB(unsigned char * rgbbuff, int x_size, int y_size, int x_pan, int y_pan, int x_offs, int y_offs, bool clearfb = true);
		bool displayImage(const std::string& name, int posx = 0, int posy = 0, int width = DEFAULT_XRES, int height = DEFAULT_YRES, int x_pan = 0, int y_pan = 0);
};

#endif
