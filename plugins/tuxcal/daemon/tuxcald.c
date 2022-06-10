/******************************************************************************
 *                        <<< TuxCal - Calendar daemon >>>
 *                (c) Robert "robspr1" Spreitzer 2006 (robert.spreitzer@inode.at)
 *  
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ******************************************************************************/
// lots of code is from the tuxmail-project

#include "tuxcald.h"

/******************************************************************************
 * ReadConf
 ******************************************************************************/
/*!
 * read configuration-file 

 \param			: none
 \return 		: none
*/
void ReadConf()
{
	FILE *fd_conf;
	char *ptr;
	char line_buffer[256];

	// open config-file
	if (!(fd_conf = fopen(CFGPATH CFGFILE, "r")))
	{
		printf("TuxCal <Config not found, using defaults>\n");
		return;
	}

	// read config-file line-by-line
	while(fgets(line_buffer, sizeof(line_buffer), fd_conf))
	{
		if ((ptr = strstr(line_buffer, "STARTDELAY=")))
		{
			sscanf(ptr + 11, "%d", &startdelay);
		}
		else if((ptr = strstr(line_buffer, "INTERVALL=")))
		{
			sscanf(ptr + 10, "%d", &intervall);
		}
		else if((ptr = strstr(line_buffer, "LOGGING=")))
		{
			sscanf(ptr + 8, "%c", &logging);
		}
		else if((ptr = strstr(line_buffer, "AUDIO=")))
		{
			sscanf(ptr + 6, "%c", &audio);
		}
		else if((ptr = strstr(line_buffer, "VIDEO=")))
		{
			sscanf(ptr + 6, "%d", &video);
		}
		else if((ptr = strstr(line_buffer, "SIGNAL=")))
		{
			sscanf(ptr + 7, "%d", &sigtype);
		}
		else if((ptr = strstr(line_buffer, "SIGMODE=")))
		{
			sscanf(ptr + 8, "%d", &sigmode);
		}
		else if((ptr = strstr(line_buffer, "SIGTIME=")))
		{
			sscanf(ptr + 8, "%s", &sigtime[0]);
		}
		else if((ptr = strstr(line_buffer, "OSD=")))
		{
			sscanf(ptr + 4, "%c", &osd);
		}
		else if((ptr = strstr(line_buffer, "SKIN=")))
		{
			sscanf(ptr + 5, "%d", &skin);
		}
		else if((ptr = strstr(line_buffer, "WEBPORT=")))
		{
			sscanf(ptr + 8, "%d", &webport);
		}
		else if((ptr = strstr(line_buffer, "WEBUSER=")))
		{
			sscanf(ptr + 8, "%s", &webuser[0]);
		}
		else if((ptr = strstr(line_buffer, "WEBPASS=")))
		{
			sscanf(ptr + 8, "%s", &webpass[0]);
		}
		else if((ptr = strstr(line_buffer, "POS_X=")))
		{
			sscanf(ptr + 6, "%d", &startx);
		}
		else if((ptr = strstr(line_buffer, "POS_Y=")))
		{
			sscanf(ptr + 6, "%d", &starty);
		}
		else if((ptr = strstr(line_buffer, "SHOW=")))
		{
			sscanf(ptr + 5, "%c", &show_clockatstart);
		}
		else if((ptr = strstr(line_buffer, "DATE=")))
		{
			sscanf(ptr + 5, "%c", &disp_date);
		}
		else if((ptr = strstr(line_buffer, "CLOCK=")))
		{
			sscanf(ptr + 6, "%c", &disp_clock);
		}
		else if((ptr = strstr(line_buffer, "SEC=")))
		{
			sscanf(ptr + 4, "%c", &disp_sec);
		}
		else if((ptr = strstr(line_buffer, "SIZE=")))
		{
			sscanf(ptr + 5, "%c", &disp_size);
		}
		else if((ptr = strstr(line_buffer, "COLOR=")))
		{
			sscanf(ptr + 6, "%d", &disp_color);
		}
		else if((ptr = strstr(line_buffer, "BACK=")))
		{
			sscanf(ptr + 5, "%d", &disp_back);
		}
		else if((ptr = strstr(line_buffer, "DETECT=")))
		{
			sscanf(ptr + 7, "%d", &disp_detect);
		}
		else if((ptr = strstr(line_buffer, "MAIL=")))
		{
			sscanf(ptr + 5, "%c", &disp_mail);
		}
	}

	// close config-file
	fclose(fd_conf);

	// ----------------
	// check config
	// on-screen-display german or english
	if (osd != 'G' && osd != 'E')
	{
		printf("TuxCal <OSD=%c invalid, set to \"G\">\n", osd);
		osd = 'G';
	}
	switch (osd)
	{
		case 'G': osdidx=0; break;
		case 'E': osdidx=1; break;
	}

	if (!startdelay) startdelay = 30;												// default 30 seconds delay
	if (!intervall) intervall = 1;													// default check every 1 second

	if ((sigtype<1) || (sigtype>MAXCHECKDAYS)) sigtype=1;		// default only this day
	if ((sigmode<0) || (sigmode>3)) sigmode=0;							// default only events and birthdays
	
	// we have different skins
	if (skin != 1 && skin != 2 && skin != 3)
	{
		printf("TuxCal <SKIN=%d invalid, set to \"1\">\n", skin);
		skin = 1;
	}

	switch (disp_size)
	{
		case 'N': iFontSize=FONTSIZE_NORMAL; iFont=NORMAL; break;
		case 'B': iFontSize=FONTSIZE_BIG; iFont=BIG; break;
		default: iFontSize=FONTSIZE_SMALL; iFont=SMALL;
	}

}

/******************************************************************************
 * WriteConf
 ******************************************************************************/
/*!
 * write the configuration back to the file
 
 \param			: none
 \return 		: 1:OK    0:FAILED    
*/
int WriteConf()
{
	FILE *fd_conf;

	// open config-file
	if (!(fd_conf = fopen(CFGPATH CFGFILE , "w")))
	{
		return 0;
	}

	fprintf(fd_conf, "STARTDELAY=%d\n", startdelay);
	fprintf(fd_conf, "INTERVALL=%d\n", intervall);
	fprintf(fd_conf, "LOGGING=%c\n", logging);
	fprintf(fd_conf, "AUDIO=%c\n", audio);
	fprintf(fd_conf, "VIDEO=%d\n", video);
	fprintf(fd_conf, "SIGNAL=%d\n", sigtype);
	fprintf(fd_conf, "SIGMODE=%d\n", sigmode);
	fprintf(fd_conf, "SIGTIME=%s\n\n", sigtime);
	fprintf(fd_conf, "OSD=%c\n", osd);
	fprintf(fd_conf, "SKIN=%d\n\n", skin);
	fprintf(fd_conf, "WEBPORT=%d\n", webport);
	fprintf(fd_conf, "WEBUSER=%s\n", webuser);
	fprintf(fd_conf, "WEBPASS=%s\n\n", webpass);
	fprintf(fd_conf, "POS_X=%d\n", startx);
	fprintf(fd_conf, "POS_Y=%d\n", starty);
	fprintf(fd_conf, "SHOW=%c\n", show_clockatstart);
	fprintf(fd_conf, "DATE=%c\n", disp_date);
	fprintf(fd_conf, "CLOCK=%c\n", disp_clock);
	fprintf(fd_conf, "SEC=%c\n", disp_sec);
	fprintf(fd_conf, "SIZE=%c\n", disp_size);
	fprintf(fd_conf, "COLOR=%d\n", disp_color);
	fprintf(fd_conf, "BACK=%d\n", disp_back);
	fprintf(fd_conf, "DETECT=%d\n", disp_detect);
	fprintf(fd_conf, "MAIL=%c\n\n", disp_mail);

	fclose(fd_conf);
	return 1;
}

#if 1
/******************************************************************************
 * MyFaceRequester
 ******************************************************************************/
/*!
 * load font
 
 \param	face_id				: FTC_FaceID
 \param library				: FT_Library
 \param request_data	: FT_Pointer
 \param afacs					: FT_Face*
 \return 							: FT_Error
*/
FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

	if (!result) printf("TuxCal <Font \"%s\" loaded>\n", (char*)face_id);
	else printf("TuxCal <Font \"%s\" failed>\n", (char*)face_id);
	
	return result;
}
#endif

/******************************************************************************
 * FindColors
 ******************************************************************************/
/*!
 * find black and white in the colormap 

 \param			: none
 \return 		: none
*/
void FindColors()
{
	int i, l;
	__u16 *pR, *pG, *pB, *pT;
	__u16 iWmax=0;
	
	if (colormap == NULL) return;
		
	pR=colormap->red;
	pG=colormap->green;
	pB=colormap->blue;
	pT=colormap->transp;
	
	iWhite=-1;
	iBlack=-1;
	
	l=colormap->len;
	for (i=0; i<l; i++)
	{
		// find the first black
		if ((iBlack==-1) && (*pR==0) && (*pG==0) && (*pB==0) && (*pT==0)) iBlack=i;

		// find "whitest" color
		if ((*pR==*pG) && (*pR==*pB) && (*pT==0)) 
		{
			if (*pR>iWmax) iWhite=i;
		}
		
//		printf("colormap idx:%d %04x %04x %04x %04x\r\n",i,*pR,*pG,*pB,*pT);
		pR++; pG++; pB++;	pT++;
//		if ((iBlack!=-1) && (iWhite!=-1)) break;
	}
	printf("TuxCalD <found black at %d, white at %d>\r\n",iBlack, iWhite);

  // free colormap
	if (colormap)
	{
		free(colormap->red);
		free(colormap->green);
		free(colormap->blue);
		free(colormap->transp);
		free(colormap);
		colormap=NULL;
	}
}

/******************************************************************************
 * OpenFB
 ******************************************************************************/
/*!
 * opens the connection to the framebuffer

 \param			: none
 \return 		: 1:OK    0:FAILED    
*/
int OpenFB(void)
{
  #if 0
	FT_Error error;

	colormap=NULL;

	// framebuffer stuff
	if ((fbdev = open("/dev/fb/0", O_RDWR))<0)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "open fb failed"): printf("TuxCalD <open fb failed>");
		return 0;
	}

	// init framebuffer
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, &var_screeninfo)<0)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FBIOGET_VSCREENINFO"): printf("TuxCalD <FBIOGET_VSCREENINFO>");
		close(fbdev);
		return 0;
	}

	if (ioctl(fbdev, FBIOGET_FSCREENINFO, &fix_screeninfo)<0)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FBIOGET_FSCREENINFO"): printf("TuxCalD <FBIOGET_FSCREENINFO>");
		close(fbdev);
		return 0;
	}

	if (!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "mapping of Framebuffer failed"): printf("TuxCalD <mapping of Framebuffer failed>\n");
		close(fbdev);
		return 0;
	}

	// init fontlibrary
	if ((error = FT_Init_FreeType(&library)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FT_Init_FreeType failed with Errorcode 0x%.2X", error): printf("TuxCalD <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
		return 0;
	}

	if ((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FTC_Manager_New failed with Errorcode 0x%.2X", error): printf("TuxCalD <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
		return 0;
	}

	if ((error = FTC_SBitCache_New(manager, &cache)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FTC_SBitCache_New failed with Errorcode 0x%.2X", error): printf("TuxCalD <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
		return 0;
	}

	if ((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X", error): printf("TuxCalD <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
		return 0;
	}

	use_kerning = FT_HAS_KERNING(face);

#ifdef FT_NEW_CACHE_API
	desc.face_id = FONT;
#else
	desc.font.face_id = FONT;
#endif

#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
		desc.type = ftc_image_mono;
#else
		desc.flags = FT_LOAD_MONOCHROME;
#endif

	// init backbuffer
	if (!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "allocating of Backbuffer failed"): printf("TuxCalD <allocating of Backbuffer failed>\n");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
		return 0;
	}

	memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);

	// i'm not sure if this is ok
	sx=var_screeninfo.xoffset;
	sy=var_screeninfo.yoffset;
	ex=sx+var_screeninfo.xres;
	ey=sy+var_screeninfo.yres;

	// find blach or white in the colormap
	if (disp_detect) 
	{
  	bps=var_screeninfo.bits_per_pixel;
    if(fix_screeninfo.visual==FB_VISUAL_PSEUDOCOLOR)
    {
      colormap=(struct fb_cmap*)malloc(sizeof(struct fb_cmap));
      colormap->red=(__u16*)malloc(sizeof(__u16)*(1<<bps));
      colormap->green=(__u16*)malloc(sizeof(__u16)*(1<<bps));
      colormap->blue=(__u16*)malloc(sizeof(__u16)*(1<<bps));
      colormap->transp=(__u16*)malloc(sizeof(__u16)*(1<<bps));
      colormap->start=0;
      colormap->len=1<<bps;
      if (ioctl(fbdev, FBIOGETCMAP, colormap))
      {
  			slog ? syslog(LOG_DAEMON | LOG_INFO, "FBIOGETCMAP failed") : printf("TuxCalD <FBIOGETCMAP failed>\n");
				FTC_Manager_Done(manager);
				FT_Done_FreeType(library);
				free(lbb);
				munmap(lfb, fix_screeninfo.smem_len);
				close(fbdev);
  			return 0;
    	}
    }
		FindColors();	
	}
#endif

	return 1;
}

/******************************************************************************
 * ClearScreen
 ******************************************************************************/
/*!
 * clear the framebuffer

 \param			: none
 \return 		: none    
*/
void ClearScreen(void)
{
	if (iFB)
	{
		memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);								// clear buffer for framebuffer-writing to transparent
		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);							// empty framebuffer
	}
}

/******************************************************************************
 * CloseFB
 ******************************************************************************/
/*!
 * close the connection to the framebuffer

 \param			: none
 \return 		: none    
*/
void CloseFB(void)
{
	// clear the framebuffer
	ClearScreen();
	
	if (iFB)
	{
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		free(lbb);
		munmap(lfb, fix_screeninfo.smem_len);
		close(fbdev);
	}
	iFB=0;
}

/******************************************************************************
 * RenderChar
 ******************************************************************************/
/*!
 * render a character to the screen
 
 \param currentchar	: FT_ULong
 \param sx					: startposition x
 \param sy					: startposition y
 \param ex					: endposition x
 \param color				: color
 \return 						: charwidth
*/
int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, int color)
{
	int row, pitch, bit, x = 0, y = 0;
	FT_Error error;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FTC_Node anode;

	//load char
	if (!(glyphindex = FT_Get_Char_Index(face, currentchar)))
	{
		printf("TuxCal <FT_Get_Char_Index for Char \"0x%x\" failed: \"undefined character code\">\n", (int)currentchar);
		return 0;
	}

	if ((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
	{
		printf("TuxCal <FTC_SBitCache_Lookup for Char \"%c\" failed with Errorcode 0x%.2X>\n", (int)currentchar, error);
		return 0;
	}

	if (use_kerning)
	{
		FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);
		prev_glyphindex = glyphindex;
		kerning.x >>= 6;
	}
	else
	{
		kerning.x = 0;
	}

	// render char
	if (color != -1) 																			// don't render char, return charwidth only 
	{
		if (sx + sbit->xadvance >= ex) return -1; 					// limit to maxwidth 

		for (row = 0; row < sbit->height; row++)
		{
			for (pitch = 0; pitch < sbit->pitch; pitch++)
			{
				for (bit = 7; bit >= 0; bit--)
				{
					if (pitch*8 + 7-bit >= sbit->width)
					{
						break; 																			// render needed bits only 
					}

					if ((sbit->buffer[row * sbit->pitch + pitch]) & 1<<bit)
					{
						*(lbb + startx + sx + sbit->left + kerning.x + x + var_screeninfo.xres*(starty + sy - sbit->top + y)) = color;
					}

					x++;
				}
			}
			x = 0;
			y++;
		}
	}

	// return charwidth
	return sbit->xadvance + kerning.x;
}

/******************************************************************************
 * GetStringLen
 ******************************************************************************/
/*!
 * calculate used pixels on screen for output
 
*/
int GetStringLen(unsigned char *string)
{
	int stringlen = 0;

	// reset kerning
	prev_glyphindex = 0;

	// calc len
	while (*string != '\0')
	{
		stringlen += RenderChar(*string, -1, -1, -1, -1);
		string++;
	}

	return stringlen;
}

/******************************************************************************
 * RenderString
 ******************************************************************************/
/*!
 * render a string to the screen
 
*/
void RenderString(unsigned char *string, int sx, int sy, int maxwidth, int layout, int size, int color)
{
	int stringlen, ex, charwidth;

	// set size
	if(size == SMALL)
	{
#ifdef FT_NEW_CACHE_API
		desc.width = desc.height = FONTSIZE_SMALL;
#else
		desc.font.pix_width = desc.font.pix_height = FONTSIZE_SMALL;
#endif
	}
	else if (size == NORMAL)
	{
#ifdef FT_NEW_CACHE_API
		desc.width = desc.height = FONTSIZE_NORMAL;
#else
		desc.font.pix_width = desc.font.pix_height = FONTSIZE_NORMAL;
#endif
	}
	else
	{
#ifdef FT_NEW_CACHE_API
		desc.width = desc.height = FONTSIZE_BIG;
#else
		desc.font.pix_width = desc.font.pix_height = FONTSIZE_BIG;
#endif
	}

	// set alignment
	if ((layout != LEFT) && (layout != FIXEDLEFT))
	{
		stringlen = GetStringLen(string);

		switch(layout)
		{
			case FIXEDCENTER:
#ifdef FT_NEW_CACHE_API
				stringlen = (desc.width/2) * strlen(string);
#else
				stringlen = (desc.font.pix_width/2) * strlen(string);
#endif

			case CENTER:
			{
				if(stringlen < maxwidth)
				{
					sx += (maxwidth - stringlen)/2;
				} 
			} break;

			case FIXEDRIGHT:
#ifdef FT_NEW_CACHE_API
				stringlen = (desc.width/2) * strlen(string);
#else
				stringlen = (desc.font.pix_width/2) * strlen(string);
#endif
			case RIGHT:
			{
				if(stringlen < maxwidth)
				{
					sx += maxwidth - stringlen;
				}
			} break;		
		}
	}

	// reset kerning
	prev_glyphindex = 0;

	// render string
	ex = sx + maxwidth;

	while (*string != '\0')
	{
		if ((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1)  return; // string > maxwidth 

		if ((layout == FIXEDLEFT) || (layout == FIXEDCENTER) || (layout == FIXEDRIGHT))
#ifdef FT_NEW_CACHE_API
			sx += (desc.width/2);
#else
			sx += (desc.font.pix_width/2);
#endif
		else 
			sx += charwidth;
		string++;
	}
}

/******************************************************************************
 * RenderBox
 ******************************************************************************/
/*!
 * render a box to the screen

 \param sx		: startposition x
 \param sy		: startposition y
 \param ex		: endposition x
 \param ey		: endposition y
 \param mode	: mode for painting (FILL or GRID)
 \param color	: color to paint with
 \return      : none
*/
void RenderBox(int sx, int sy, int ex, int ey, int mode, int color)
{
	int loop;

	// paint a box with solid color
	if (mode == FILL)
	{
		// for all lines of the box
		for (; sy <= ey; sy++)
		{
			// fill the line with the color
			memset(lbb + startx + sx + var_screeninfo.xres*(starty + sy), color, ex-sx + 1);
		}
	}
	// paint a grid around the rectangle
	else
	{
		// lines
		for (loop = sx; loop <= ex; loop++)
		{
			*(lbb + startx+loop + var_screeninfo.xres*(sy+starty)) = color;
			*(lbb + startx+loop + var_screeninfo.xres*(sy+1+starty)) = color;
			*(lbb + startx+loop + var_screeninfo.xres*(ey-1+starty)) = color;
			*(lbb + startx+loop + var_screeninfo.xres*(ey+starty)) = color;
		}

		// columns

		for (loop = sy; loop <= ey; loop++)
		{
			*(lbb + startx+sx + var_screeninfo.xres*(loop+starty)) = color;
			*(lbb + startx+sx+1 + var_screeninfo.xres*(loop+starty)) = color;
			*(lbb + startx+ex-1 + var_screeninfo.xres*(loop+starty)) = color;
			*(lbb + startx+ex + var_screeninfo.xres*(loop+starty)) = color;
		}
	}
}

/******************************************************************************
 * RenderInt
 ******************************************************************************/
/*!
 * render a integer to the screen
 
*/
void RenderInt(unsigned char *string, int sx, int sy, int maxwidth, int layout, int size, int color, int colorgrid, int colorfill)
{
	int x,y,cx,cy;
	int sizey=FONTSIZE_NORMAL;
	
	if (size==SMALL) sizey=FONTSIZE_SMALL;
	else if (size==BIG) sizey=FONTSIZE_BIG;
	
	x=sx-5;
	y=sy-sizey+8;
	cx=x+maxwidth+3;
	cy=y+sizey;	
	if (colorfill!=-1) RenderBox(x,y,cx,cy,FILL,colorfill);
	if (colorgrid!=-1) RenderBox(x,y,cx,cy,GRID,colorgrid);

	RenderString(string,sx,sy,maxwidth,layout,size,color);
}

/******************************************************************************
 * RenderSObject
 ******************************************************************************/
/*!
 * render an object to the screen paint only the pixels which are set
 
 \param sx		: startposition x
 \param sy		: startposition y
 \param color	: color to paint with
 \param iType	: index for the object to paint
 \return      : none
*/
void RenderSObject(int sx, int sy, int color, int iType)
{
	int x, y;
  char* pObj=sym_letter;

	// choose the object
/*	
	if (iType == OBJ_CIRCLE) pObj=circle;
	if (iType == OBJ_HEART) pObj=heart;
	if (iType == OBJ_MARKER) pObj=marker;
	if (iType == OBJ_SCROLLUP) pObj=scroll_up;
	if (iType == OBJ_SCROLLDN) pObj=scroll_dn;
	if (iType == OBJ_CLOCK) pObj=symbolclock;
*/	
	if (iType == OBJ_LETTER) pObj=sym_letter;

	// render
	for (y = 0; y < OBJ_SY; y++)					// for all columns
	{	
		for (x = 0; x < OBJ_SX; x++)				// for all lines
		{
			if (*pObj++)											// only paint if mask-value set
				memset(lbb + startx + sx + x + var_screeninfo.xres*(starty + sy + y), color, 1);
		}
	}
}

/******************************************************************************
 * CheckEvent
 ******************************************************************************/
/*!
 * check the entry
 
 \param pEvt	: EVT_DB* pointer to event-object
 \return  		: 1:OK/Yes  0:Exit/No - changes made
*/
/*  --- not used in tuxcal-daemon
int CheckEvent(EVT_DB* pEvt)
{
	EVT_DB evtsic;
	memcpy(&evtsic,pEvt,sizeof(EVT_DB)-MAXINFOLEN);
	
	int iLeapYear=0;
	if (pEvt->month<1) pEvt->month=1;
	if (pEvt->month>12) pEvt->month=12;
	if (pEvt->year!=0)
	{
		if (pEvt->year<1700) pEvt->year=1700;
		if (pEvt->year>2299) pEvt->year=2299;
		iLeapYear=LeapYear(pEvt->year);
	}
	if (pEvt->day<1) pEvt->day=1;
	if (pEvt->day>monthdays[iLeapYear][pEvt->month-1]) pEvt->day=monthdays[iLeapYear][pEvt->month-1];
	if (pEvt->hour>=0)
	{
		if (pEvt->hour>23) pEvt->hour=23;
		if (pEvt->min<0) pEvt->min=0;
		if (pEvt->min>59) pEvt->min=59;
	}
	
	if (pEvt->type==PERIOD)
	{
		iLeapYear=0;
  	if (pEvt->emonth<1) pEvt->emonth=1;
  	if (pEvt->emonth>12) pEvt->emonth=12;
  	if (pEvt->eyear!=0)
  	{
  		if (pEvt->eyear<1700) pEvt->eyear=1700;
  		if (pEvt->eyear>2299) pEvt->eyear=2299;
  		iLeapYear=LeapYear(pEvt->eyear);
  	}
  	if (pEvt->eday<1) pEvt->eday=1;
  	if (pEvt->eday>monthdays[iLeapYear][pEvt->emonth-1]) pEvt->eday=monthdays[iLeapYear][pEvt->emonth-1];
  	if (pEvt->ehour>=0)
  	{
  		if (pEvt->ehour>23) pEvt->ehour=23;
  		if (pEvt->emin<0) pEvt->emin=0;
  		if (pEvt->emin>59) pEvt->emin=59;
  	}
	}
	
	if (memcmp(&evtsic,pEvt,sizeof(EVT_DB)-MAXINFOLEN)) return 0;
	return 1;
}
*/

/******************************************************************************
 * LeapYear
 ******************************************************************************/
/*!
 * calculate if we have a leap-year
 
\return 1 if leap-year
*/
int LeapYear(int year)
{
	if (((year % 4 == 0) && (!( year % 100 == 0))) && (year % 400 != 0))
	{
		return 1;
	}
	return 0;
}

/*
The basic steps for a date in the years 2000-2099 are as follows:

Example date July 13th, 2004

   1. Take the last 2 digits of the year and add a quarter onto itself. (04 + 1 = 5)
   2. Get the corresponding code for the month. (January = 6, February = 2, March = 2, etc. See month codes for details). July = 5
   3. Take the day. (=13)
   4. Add the numbers together (5 + 5 + 13 = 23)
   5. Take away 7 (or multiples of 7) until a number from 1-7 is left. (23 - 21 =2)
   6. This number corresponds to the day of the week. (1 = Monday, 2 = Tuesday, etc.) In this case 2 = Tuesday

Apart from the basic steps, other elements have to be taken into account:

    * When adding a quarter of the year onto itself, If the quarter of the year is not a whole number, simply ignore the decimals. Do not round up. Therefore 27/4 = 6.75 = 6, and 2/4 = 0.5 = 0.
       
    * Leap years: subtract 1 from the total if the month is January or February.
       
    * Negative numbers. During the calculation you get 0 or negative numbers, just add seven until you get a number from 1-7.
       
    * Different "centuries" *.
          o 1700s add 5
          o 1800s add 3
          o 1900s add 1
          o 2100s subtract 2
          o 2200s subtract 4


Jan  	Feb  	Mar  	Apr.  May  	Jun  	Jul  	Ago  	Sept  Oct  	Nov  	Dec
6 	  2 	  2 	  5  	  0 	  3 	  5 	  1 	  4 	  6 	  2 	  4
*/
/******************************************************************************
 * DayOfWeek
 ******************************************************************************/
/*!
 * return the day-of-the-week
 * monday=1, tueday=2, etc. 

*/
int DayOfWeek(int day, int month, int year)
{
	int iCalc;

	// no calculations out of range, return monday :)
	if ((year<1700) || (year>=2300)) return 1;
		
	// Take the last 2 digits of the year and add a quarter onto itself. (04 + 1 = 5)
	iCalc=(year % 100) + (year % 100)/4;
	
	// Get the corresponding code for the month. (January = 6, February = 2, March = 2, etc. See month codes for details). July = 5
	// Take the day. (=13)
	// Add the numbers together (5 + 5 + 13 = 23)
	iCalc+=monthcode[month-1]+day;

	// Leap years: subtract 1 from the total if the month is January or February.
	if ((LeapYear(year)) && (month<3)) iCalc--;

	// Different "centuries" *.
	//           o 1700s add 5
	//           o 1800s add 3
	//           o 1900s add 1
	//           o 2100s subtract 2
	//           o 2200s subtract 4
	if (year<1800)       iCalc+=5;
	else if (year<1900)  iCalc+=3;
	else if (year<2000)  iCalc+=1;
	else if (year>=2200) iCalc-=4;
	else if (year>=2100) iCalc-=2;

	// Negative numbers. During the calculation you get 0 or negative numbers, just add seven until you get a number from 1-7.
	while (iCalc<1) iCalc+=7;
			
	// Take away 7 (or multiples of 7) until a number from 1-7 is left. (23 - 21 =2)
	while (iCalc>7) iCalc-=7;
	
	return iCalc;
}

/******************************************************************************
 * AddDays
 ******************************************************************************/
/*!
 * add days (max. +/- 28) to date

*/
void AddDays(int* pday, int* pmonth, int* pyear, int adddays)
{
	if ((adddays>28) || (adddays<-28)) return;
	
	int day = *pday;
	int month = *pmonth;
	int year = *pyear;
	int iLeapYear;
	
	iLeapYear=LeapYear(year);
	
	// substract days
	if (adddays<0)
	{
		day+=adddays;
		if (day<1)
		{
			month--;
			if (month<1)
			{
				month=12;
				year--;
			}
			day+=monthdays[iLeapYear][month-1];
		}
	}
	
	// add days
	if (adddays>0)
	{
		day+=adddays;
		if (day > monthdays[iLeapYear][month-1])
		{
			day-=monthdays[iLeapYear][month-1];
			month++;
			if (month>12)
			{
				month=1;
				year++;
			}
		}
	}

	*pday=day;
	*pmonth=month;
	*pyear=year;
}

/******************************************************************************
 * CalcEastern
 ******************************************************************************/
/*!
 * 
 - Christi Himmelfahrt (+39), 
 - Pfingsten (+49), 
 - Fronleichnam (+60), 

Der Muttertag ist der zweite Sonntag im Mai, 
das Erntedankfest der erste Sonntag im Oktober (jedoch nicht �berall!). 
Der 1. Advent ist der Sonntag nach dem 26. November; 
der Bu�- und Bettag liegt 11 Tage vor dem 1. Advent. 

 
*/
void CalcEastern(int year, int* month, int* day)
{
	int a,b,c,d,e;
	int dM, dA;
	int M = 24, N=5;
	
	if (year<1800)      	{	M=23; N=3; }
	else if (year<1900)		{	M=23; N=4; }
	else if (year<2100)		{	M=24; N=5; }
	else if (year<2200)		{	M=24; N=6; }
	
	a= year % 19;
	b= year % 4;
	c= year % 7;
	d= (19*a + M) % 30;
	e= (2*b + 4*c + 6*d + N) % 7;
	
	dM = (22 + d + e);									// eastern in march dM >= 22
	dA = (d + e - 9);										// eastern in april dA <= 25

	*month=3; 
	*day=dM; 														// we think eastern is in march
	
	if (dM > 31)	
	{
		if (dA == 26)											// special rule
		{ *month=4; *day=19; }
		else if ((dA == 25) && (d == 28) && (a > 10))	// another special rule
		{ *month=4; *day=18; }
		else
		{ *month=4; *day=dA; }
	}
}

/******************************************************************************
 * FillEasternDays
 ******************************************************************************/
/*!
 * fill the structure for this years for eastern etc.
 - Christi Himmelfahrt (+39), 
 - Pfingsten (+49), 
 - Fronleichnam (+60), 

Der Muttertag ist der zweite Sonntag im Mai, 
das Erntedankfest der erste Sonntag im Oktober (jedoch nicht �berall!). 
Der 1. Advent ist der Sonntag nach dem 26. November; 
der Bu�- und Bettag liegt 11 Tage vor dem 1. Advent. 

 
*/
void FillEasternDays(int year)
{
	int mon, day;
	int mon1, day1;
	
	CalcEastern(year, &mon, &day);
	mon1=mon; day1=day;
	varaibledays[OFFSET_E-1].mon = mon;			// eastern
	varaibledays[OFFSET_E-1].day = day;
	AddDays(&day, &mon, &year, 1);
	varaibledays[OFFSET_EM-1].mon = mon;		// eastern monday
	varaibledays[OFFSET_EM-1].day = day;
	
	AddDays(&day, &mon, &year, 19);					// +39 days from eastern
	AddDays(&day, &mon, &year, 19);
	varaibledays[OFFSET_H-1].mon = mon;			// christi himmelfahrt
	varaibledays[OFFSET_H-1].day = day;
	
	AddDays(&day, &mon, &year, 10);
	varaibledays[OFFSET_P-1].mon = mon;			// pfingsten
	varaibledays[OFFSET_P-1].day = day;
	AddDays(&day, &mon, &year, 1);
	varaibledays[OFFSET_PM-1].mon = mon;		// pfingsten montag
	varaibledays[OFFSET_PM-1].day = day;
	
	AddDays(&day, &mon, &year, 10);
	varaibledays[OFFSET_F-1].mon = mon;			// fronleichnam
	varaibledays[OFFSET_F-1].day = day;
	
	AddDays(&day1, &mon1, &year, -2);
	varaibledays[OFFSET_KF-1].mon = mon1;		// karfreitag
	varaibledays[OFFSET_KF-1].day = day1;
	AddDays(&day1, &mon1, &year, -1);
	varaibledays[OFFSET_GD-1].mon = mon1;		// gruendonnerstag
	varaibledays[OFFSET_GD-1].day = day1;

	AddDays(&day1, &mon1, &year, -23);
	AddDays(&day1, &mon1, &year, -20);
	varaibledays[OFFSET_A-1].mon = mon1;		// aschermittwoch
	varaibledays[OFFSET_A-1].day = day1;
	AddDays(&day1, &mon1, &year, -2);
	varaibledays[OFFSET_RM-1].mon = mon1;		// rosenmontag
	varaibledays[OFFSET_RM-1].day = day1;

	//mothersday is 2. sunday in may
	int iWDay;
	iWDay=DayOfWeek(1,5,year);
	varaibledays[OFFSET_M-1].mon = 5;
	varaibledays[OFFSET_M-1].day = 15-iWDay;
	
	// sommertime
	iWDay=DayOfWeek(31,3,year);
	if (iWDay!=7)	varaibledays[OFFSET_SZ-1].day = 31-iWDay;
	else varaibledays[OFFSET_SZ-1].day = 31;
	varaibledays[OFFSET_SZ-1].mon = 3;

	// wintertime	
	iWDay=DayOfWeek(31,10,year);
	if (iWDay!=7)	varaibledays[OFFSET_WZ-1].day = 31-iWDay;
	else varaibledays[OFFSET_WZ-1].day = 31;
	varaibledays[OFFSET_WZ-1].mon = 10;
	
	varaibledays[OFFSET_W0-1].day=24;					// heiliger abend
	varaibledays[OFFSET_W0-1].mon=12;
	varaibledays[OFFSET_W1-1].day=25;					// 1. weihnachtstag
	varaibledays[OFFSET_W1-1].mon=12;
	varaibledays[OFFSET_W2-1].day=26;					// 2. weihnachtstag
	varaibledays[OFFSET_W2-1].mon=12;
	varaibledays[OFFSET_3K-1].day=6;					// hl. 3 koenige
	varaibledays[OFFSET_3K-1].mon=1;
	varaibledays[OFFSET_N-1].day=1;						// neujahr
	varaibledays[OFFSET_N-1].mon=1;
	varaibledays[OFFSET_S-1].day=31;					// sylvester
	varaibledays[OFFSET_S-1].mon=12;
	varaibledays[OFFSET_V-1].day=14;					// valentinstag
	varaibledays[OFFSET_V-1].mon=2;
	varaibledays[OFFSET_1M-1].day=1;					// 1. may
	varaibledays[OFFSET_1M-1].mon=5;
	varaibledays[OFFSET_MH-1].day=15;					// maria himmelfahrt
	varaibledays[OFFSET_MH-1].mon=8;
	varaibledays[OFFSET_NI-1].day=6;					// nikolaus
	varaibledays[OFFSET_NI-1].mon=12;
	varaibledays[OFFSET_ND-1].day=3;					// tag der deutschen einheit
	varaibledays[OFFSET_ND-1].mon=10;
	varaibledays[OFFSET_NA-1].day=26;					// nationalfeiertag oesterreich
	varaibledays[OFFSET_NA-1].mon=10;
}

/******************************************************************************
 * IsEvent
 ******************************************************************************/
/*!
 * is there an event at this day and the next sigtype days
 * we fill array-index 0 with the actual day and time-info
 * we fill array-index 1 with the actual day without time-info
 * we fill array-index 2 with the next day 
 * we fill array-index 3 with the following day

*/
int IsEvent(int day, int month, int year)
{
	int iEntry=0;
	int iCnt=0;
	int iCntTm=0;
	int iTotalCnt = 0;
	int iFound;
	int days = __mon_yday[LeapYear(year) ? 1 : 0][month-1]+day;
	int i,j;

	// empty all found events
	for (j=0; j<=MAXCHECKDAYS; j++)
		iCntEvents[j]=0;
	
	// check for this, but also for up to sigtype days
	for (j=1; j<=sigtype; j++)
	{
		if ((sigmode == SIGALL) || (sigmode == SIGHOLIDAY))
		{
			// first check for any of the holidays: eastern, etc.
			for (i=1;i<=NOF_VDAYS;i++)
				if ((varaibledays[i-1].mon==month) && (varaibledays[i-1].day==day))
				{
					iEventType[j][iCnt++]=MAXENTRYS+i;
					iCntEvents[j]++;
				}
		}
		
		// start with first index
		iEntry=0;
		
		// now check the database
  	while (iEntry<MAXENTRYS)
  	{
  		// abort if no entry
  		if (eventdb[iEntry].type == FREE) break;
  		
  		iFound=0;
  		// check for holiday or event
  		if ((eventdb[iEntry].type == HOLIDAY) || (eventdb[iEntry].type == EVENT))
  		{
  			if ((eventdb[iEntry].day == day) && (eventdb[iEntry].month == month) && ((eventdb[iEntry].year==0) || (eventdb[iEntry].year == year)))
  				iFound=1;
  		}
  
  		// check for birthday
  		else if ((eventdb[iEntry].type == BIRTHDAY) && (eventdb[iEntry].day == day) && (eventdb[iEntry].month == month))
  		{
  			iFound=1;		
  		}
  	
  		// check for period
  		else if (eventdb[iEntry].type == PERIOD)
  		{		
	  		if ((sigmode == SIGPERIOD) || (sigmode == SIGALL))
  			{
    			// don't check the year
    			if (eventdb[iEntry].year==0)
    			{
    				if (LeapYear(year))
    				{
    					int iTmpDays1=eventdb[iEntry].days;
    					int iTmpDays2=eventdb[iEntry].edays;
    					
    					if (iTmpDays1 >= __mon_yday[0][2]) iTmpDays1++;
    					if (iTmpDays2 >= __mon_yday[0][2]) iTmpDays2++;
    					if ((iTmpDays1 <= days) && (iTmpDays2 >= days))
    						iFound=1;
    				}
    				else
    				{
    					if ((eventdb[iEntry].days <= days) && (eventdb[iEntry].edays >= days))
    						iFound=1;
    				}
    			}
    			else
    			{
    					if (((eventdb[iEntry].year < year) || ((eventdb[iEntry].year == year) && (eventdb[iEntry].days <= days))) && 
    						  ((eventdb[iEntry].eyear > year) || ((eventdb[iEntry].eyear == year) && (eventdb[iEntry].edays >= days))))
    						iFound=1;					 
    			}
  			}
  		}
  
  		// add event-idx to array
  		if (iFound)
  		{
  			if (j==1)		// the first day to check
  			{
  				if (eventdb[iEntry].hour == -1)
  				{
	  				iEventType[j][iCnt]=iEntry;
  					iCnt++;
  					iCntEvents[j]++;
  				}
  				else
  				{
	  				iEventType[0][iCntTm]=iEntry;
  					iCntTm++;
  					iCntEvents[0]++;
  				}
  			}
  			else
	  		{
  				iEventType[j][iCnt]=iEntry;
  				iCnt++;
  				iCntEvents[j]++;
	  		}
  		}
  				  		
  		iEntry++;
  		if ((iCnt+iCntTm)==MAXCHECKS) break;
  	}
  	
  	iTotalCnt += (iCnt+iCntTm);
 	
  	iCnt = 0;
  	iCntTm = 0;
  	
  	AddDays(&day, &month, &year, 1);																					// goto the next day
  	days = __mon_yday[LeapYear(year) ? 1 : 0][month-1]+day;										// count days (for easyier calculations
	}	
	return iTotalCnt;																																// return the number of found events
}

/******************************************************************************
 * LoadDatabase
 ******************************************************************************/
/*!
 * load the database from file

*/
void LoadDatabase(void)
{
	// clear database
	memset(eventdb,0,sizeof(eventdb));
	
	char linebuffer[1024];
	FILE *fd_evt;
	int iEntry=0;
	int iLen;
	char* p1;
	char* p2;
	
	// read the tuxcal-event-file
	if ((fd_evt = fopen(CFGPATH EVTFILE, "r"))!=NULL)
	{
		// read line by line
		while (fgets(linebuffer, sizeof(linebuffer), fd_evt))
		{			
			p1=linebuffer;
			
			// first check for type
			if ((p2=strchr(p1,';')) == NULL) 
			{
				if ((p2=strchr(p1,'#')) != NULL)								// comment ?
				{
					eventdb[iEntry].type = COMMENT;								
					strcpy(eventdb[iEntry].info,linebuffer);
				}				
				else
				{
					eventdb[iEntry].type = SPACE;									// empty line
				}
				iEntry++;
				if (iEntry >= MAXENTRYS) break;		
				continue;			
			}
			
			switch (p1[0])
			{
				case 'Z':
				case 'z': eventdb[iEntry].type = PERIOD; break;
				case 'G':
				case 'g': eventdb[iEntry].type = BIRTHDAY; break;
				case 'T':
				case 't': eventdb[iEntry].type = EVENT; break;				
				case 'F':
				case 'f': eventdb[iEntry].type = HOLIDAY; break;				
				default:  eventdb[iEntry].type = COMMENT;
			}
			
			
			p1=p2;
			*p1=' ';
			//second check for date
			if ((p2=strchr(p1,';')) == NULL) 
				continue;			
			
			// we have 6 different formats, we use the length for recognition
			//    dd.mm.(6) or dd.mm.yyyy(10) or dd.mm.-dd.mm(13) or dd.mm.yyyy-dd.mm.yyyy(21)
			// or dd.mm.yyyy hh:mm(16) or dd.mm.yyyy hh:mm-dd.mm.yy hh:mm(33)
			*p2=0;
			iLen=strlen(p1)-1;		// stringlen without leading space
			switch (iLen)
			{
				// dd.mm.   all day event/holiday
				case 6: 
					sscanf(p1," %02u.%02u.",&eventdb[iEntry].day,&eventdb[iEntry].month); 
					eventdb[iEntry].days=__mon_yday[0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
					eventdb[iEntry].hour=-1; 
					eventdb[iEntry].ehour=-1; 
				break;
				
				// dd.mm.yyyy   all day event
				case 10:
					sscanf(p1," %02u.%02u.%04u",&eventdb[iEntry].day,&eventdb[iEntry].month,&eventdb[iEntry].year); 
					eventdb[iEntry].days=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
					eventdb[iEntry].hour=-1; 
					eventdb[iEntry].ehour=-1; 
				break;
				
				// dd.mm.-dd.mm.   all day event-period
				case 13:
					sscanf(p1," %02u.%02u.-%02u.%02u.",&eventdb[iEntry].day,&eventdb[iEntry].month,&eventdb[iEntry].eday,&eventdb[iEntry].emonth); 
					eventdb[iEntry].days=__mon_yday[0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
					eventdb[iEntry].edays=__mon_yday[0][eventdb[iEntry].emonth-1]+eventdb[iEntry].eday;
					eventdb[iEntry].hour=-1; 
					eventdb[iEntry].ehour=-1; 
				break;

				// dd.mm.yyyy hh:mm   event at certain time
				case 16:
					sscanf(p1," %02u.%02u.%04u %02u:%02u",&eventdb[iEntry].day,&eventdb[iEntry].month,&eventdb[iEntry].year,&eventdb[iEntry].hour,&eventdb[iEntry].min); 
					eventdb[iEntry].days=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
				break;

				// dd.mm.yyyy-dd.mm.yyyy   all day event period
				case 21:
					sscanf(p1," %02u.%02u.%04u-%02u.%02u.%04u",&eventdb[iEntry].day,&eventdb[iEntry].month,&eventdb[iEntry].year,&eventdb[iEntry].eday,&eventdb[iEntry].emonth,&eventdb[iEntry].eyear); 
					eventdb[iEntry].days=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
					eventdb[iEntry].edays=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].emonth-1]+eventdb[iEntry].eday;
					eventdb[iEntry].hour=-1; 
					eventdb[iEntry].ehour=-1; 
				break;

				// dd.mm.yyyy hh:mm-dd.mm.yyyy hh:mm   event-period
				case 33:
					sscanf(p1," %02u.%02u.%04u %02u:%02u-%02u.%02u.%04u %02u:%02u",&eventdb[iEntry].day,&eventdb[iEntry].month,&eventdb[iEntry].year,&eventdb[iEntry].hour,&eventdb[iEntry].min,&eventdb[iEntry].eday,&eventdb[iEntry].emonth,&eventdb[iEntry].eyear,&eventdb[iEntry].ehour,&eventdb[iEntry].emin); 
					eventdb[iEntry].days=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].month-1]+eventdb[iEntry].day;
					eventdb[iEntry].edays=__mon_yday[LeapYear(eventdb[iEntry].year) ? 1 : 0][eventdb[iEntry].emonth-1]+eventdb[iEntry].eday;
				break;
			}

			p1=p2;
			*p2=' ';
			//third check for info
			if ((p2=strchr(p1,';')) == NULL)
				if ((p2=strchr(p1,'\n')) == NULL)
					continue;			
			
			*p2=0;
			strcpy(eventdb[iEntry].info,&p1[1]);
			
			iEntry++;
			if (iEntry >= MAXENTRYS) break;		
		}
		
		iCntEntries=iEntry;

		printf("TuxcalD <read %d entries from database>\r\n",iCntEntries);
				
		fclose(fd_evt);
	}
}

/******************************************************************************
 * SaveDatabase
 ******************************************************************************/
/*!
 * save the database to the file

*/
void SaveDatabase(void)
{
	FILE *fd_evt;
	int iEntry=0;
	char info_tm1[7];
	char info_tm2[7];
	char info_yr1[5];
	char info_yr2[5];
		
	// open the tuxcal-event-file
	if ((fd_evt = fopen(CFGPATH EVTFILE, "w"))!=NULL)
	{
		while (iEntry<MAXENTRYS)
		{
			// abort if no entry
			if (eventdb[iEntry].type == FREE) break;
			
			info_tm1[0]=0;
			info_tm2[0]=0;
			if (eventdb[iEntry].hour!=-1) sprintf(info_tm1," %02u:%02u",eventdb[iEntry].hour,eventdb[iEntry].min);
			if (eventdb[iEntry].ehour!=-1) sprintf(info_tm2," %02u:%02u",eventdb[iEntry].ehour,eventdb[iEntry].emin);
			info_yr1[0]=0;
			info_yr2[0]=0;
			if (eventdb[iEntry].year!=0)	sprintf(info_yr1,"%04u",eventdb[iEntry].year);
			if (eventdb[iEntry].eyear!=0) sprintf(info_yr2,"%04u",eventdb[iEntry].eyear);
			
			// check for holiday
			if (eventdb[iEntry].type == HOLIDAY)
			{
				fprintf(fd_evt,"f;%02u.%02u.%s;%s;\n", eventdb[iEntry].day,eventdb[iEntry].month,info_yr1,eventdb[iEntry].info);
			}
			
			// check for event
 			if (eventdb[iEntry].type == EVENT)
			{
				fprintf(fd_evt,"t;%02u.%02u.%s%s;%s;\n", eventdb[iEntry].day,eventdb[iEntry].month,info_yr1,info_tm1,eventdb[iEntry].info);
			}
 
			// check for birthday
			if (eventdb[iEntry].type == BIRTHDAY)
			{
				fprintf(fd_evt,"g;%02u.%02u.%s;%s;\n", eventdb[iEntry].day,eventdb[iEntry].month,info_yr1,eventdb[iEntry].info);
			}

			// check for period
			else if (eventdb[iEntry].type == PERIOD)
			{
					fprintf(fd_evt,"z;%02u.%02u.%s%s-%02u.%02u.%s%s;%s;\n", eventdb[iEntry].day,eventdb[iEntry].month,info_yr1,info_tm1,eventdb[iEntry].eday,eventdb[iEntry].emonth,info_yr2,info_tm2,eventdb[iEntry].info);
			}				
			
			// check if comment
			else if (eventdb[iEntry].type == COMMENT)
			{
				fprintf(fd_evt,"%s", eventdb[iEntry].info);			
			}

			// check if space
			else if (eventdb[iEntry].type == SPACE)
			{
				fprintf(fd_evt,"\n");
			}

			iEntry++;
		}		
		fclose(fd_evt);
	}
	iCntEntries=iEntry;
}

/******************************************************************************
 * ReadSTimer
 ******************************************************************************/
/*
 * read some times from the tuxcal.conf, at these times the daemon signals the events
 
 \param	
 \return
*/
void ReadSTimer()
{
	int i=0;
	char *ptr;
	
	ptr=sigtime;
	
	// set all s-timers
	for (;i<MAXSTIMER; i++)
	{
		if ((*ptr!=0) && (strlen(ptr)>=6))
		{
			sscanf(ptr,"%02d:%02d;",&tSignal[i].hour,&tSignal[i].min);
			ptr+=6;
		}
		else
		{
			tSignal[i].hour=-1;
			tSignal[i].min=-1;
		}
	}
}

/******************************************************************************
 * InterfaceThread 
 ******************************************************************************/
/*
 * communication-thread
 
 \param		arg	: argument for thread
 \return 			: 1:OK   0:FAILED
*/
void *InterfaceThread(void *arg)
{
	int fd_sock, fd_conn;																					// file for socket and connection
	struct sockaddr_un srvaddr;
	socklen_t addrlen;
	char command;

	// setup connection
	unlink(SCKFILE);

	srvaddr.sun_family = AF_UNIX;
	strcpy(srvaddr.sun_path, SCKFILE);
	addrlen = sizeof(srvaddr.sun_family) + strlen(srvaddr.sun_path);

	// open connection to socket-file
	if ((fd_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Interface: socket() failed") : printf("TuxCalD <Interface: socket() failed>\n");
		return 0;
	}

	// bind to socket
	if (bind(fd_sock, (struct sockaddr*)&srvaddr, addrlen) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Interface: bind() failed") : printf("TuxCalD <Interface: bind() failed>\n");
		return 0;
	}

	// listen to interface
	if (listen(fd_sock, 0) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Interface: listen() failed") : printf("TuxCalD <Interface: listen() failed>\n");
		return 0;
	}

	// communication loop
	while(1)
	{
		// check if interface is ok
		if ((fd_conn = accept(fd_sock, (struct sockaddr*)&srvaddr, &addrlen)) == -1)
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "Interface: accept() failed") : printf("TuxCalD <Interface: accept() failed>\n");
			continue;
		}

		// request one byte from plugin
		recv(fd_conn, &command, 1, 0);

		switch(command)
		{
			// plugin requests status 
			case 'G':
			{
				send(fd_conn, &online, 1, 0);
			}	break;

			// plugin sends new status
			case 'S':
			{
				recv(fd_conn, &online, 1, 0);

				if ( online )	kill(pid, SIGUSR1);
				else kill(pid, SIGUSR2);
			}	break;

			// relead database
			case 'R':
			{
				slog ? syslog(LOG_DAEMON | LOG_INFO, "update") : printf("TuxCalD <update>\n");
				int iOnlineTmp = online;
				online = 0;																															// stop output now
				memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);								// clear buffer for framebuffer-writing to transparent
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);							// empty framebuffer
				ReadConf();																															// load configuration
				LoadDatabase();																													// load database
				online = iOnlineTmp;
				oldyear = 0;																														// initiate no read of database
			} break;

			case 'C':																																	// toggle displaying the clock
			{
				if (show_clock=='Y') unlink(CLKFILE);
				else fclose(fopen(CLKFILE, "w"));
			} break;			
			
			// plugin requests version
			case 'V':
			{
				send(fd_conn, &versioninfo_d, 12, 0);
			} break;

			close(fd_conn);
		}
	}
	
	return 0;
}


/******************************************************************************
 * SwapEndian
 ******************************************************************************/
// from tuxmaild
void SwapEndian(unsigned char *header)
{
	/* wrote the PlaySound() on my pc not in mind that dbox is big endian. so this was the lazy way to make it work, sorry... */

	struct WAVEHEADER *wave = (struct WAVEHEADER*)header;

	wave->ChunkID1 = (wave->ChunkID1 << 24) | ((wave->ChunkID1 & 0x0000ff00) << 8) | ((wave->ChunkID1 & 0x00ff0000) >> 8) | (wave->ChunkID1 >> 24);
//	wave->ChunkSize1 = (wave->ChunkSize1 << 24) | ((wave->ChunkSize1 & 0x0000ff00) << 8) | ((wave->ChunkSize1 & 0x00ff0000) >> 8) | (wave->ChunkSize1 >> 24);
	wave->ChunkType = (wave->ChunkType << 24) | ((wave->ChunkType & 0x0000ff00) << 8) | ((wave->ChunkType & 0x00ff0000) >> 8) | (wave->ChunkType >> 24);

	wave->ChunkID2 = (wave->ChunkID2 << 24) | ((wave->ChunkID2 & 0x0000ff00) << 8) | ((wave->ChunkID2 & 0x00ff0000) >> 8) | (wave->ChunkID2 >> 24);
	wave->ChunkSize2 = (wave->ChunkSize2 << 24) | ((wave->ChunkSize2 & 0x0000ff00) << 8) | ((wave->ChunkSize2 & 0x00ff0000) >> 8) | (wave->ChunkSize2 >> 24);
	wave->Format = (wave->Format >> 8) | (wave->Format << 8);
	wave->Channels = (wave->Channels >> 8) | (wave->Channels << 8);
	wave->SampleRate = (wave->SampleRate << 24) | ((wave->SampleRate & 0x0000ff00) << 8) | ((wave->SampleRate & 0x00ff0000) >> 8) | (wave->SampleRate >> 24);
//	wave->BytesPerSecond = (wave->BytesPerSecond << 24) | ((wave->BytesPerSecond & 0x0000ff00) << 8) | ((wave->BytesPerSecond & 0x00ff0000) >> 8) | (wave->BytesPerSecond >> 24);
//	wave->BlockAlign = (wave->BlockAlign >> 8) | (wave->BlockAlign << 8);
	wave->BitsPerSample = (wave->BitsPerSample >> 8) | (wave->BitsPerSample << 8);

	wave->ChunkID3 = (wave->ChunkID3 << 24) | ((wave->ChunkID3 & 0x0000ff00) << 8) | ((wave->ChunkID3 & 0x00ff0000) >> 8) | (wave->ChunkID3 >> 24);
	wave->ChunkSize3 = (wave->ChunkSize3 << 24) | ((wave->ChunkSize3 & 0x0000ff00) << 8) | ((wave->ChunkSize3 & 0x00ff0000) >> 8) | (wave->ChunkSize3 >> 24);
}

/******************************************************************************
 * SwapEndianChunk
 ******************************************************************************/
// from tuxmaild
void SwapEndianChunk(unsigned char *chunk)
{
	/* wrote the PlaySound() on my pc not in mind that dbox is big endian. so this was the lazy way to make it work, sorry... */

	struct CHUNK *wave = (struct CHUNK*)chunk;

	wave->ChunkID = (wave->ChunkID << 24) | ((wave->ChunkID & 0x0000ff00) << 8) | ((wave->ChunkID & 0x00ff0000) >> 8) | (wave->ChunkID >> 24);
	wave->ChunkSize = (wave->ChunkSize << 24) | ((wave->ChunkSize & 0x0000ff00) << 8) | ((wave->ChunkSize & 0x00ff0000) >> 8) | (wave->ChunkSize >> 24);
}

/******************************************************************************
 * PlaySound
 ******************************************************************************/
// from tuxmaild
void PlaySound(unsigned char *file)
{
	FILE *fd_wav;
	unsigned char header[sizeof(struct WAVEHEADER)];
	int dsp, format, channels, speed, blocksize, readcount, count = 0;
	unsigned char tmp;
	unsigned char *samples;
	struct WAVEHEADER *wave = (struct WAVEHEADER*)header;

	// check for userdefined soundfile
	if ((fd_wav = fopen(file, "rb")))
	{
		// read header and detect format
		fread(header, 1, sizeof(header)-8, fd_wav);
		SwapEndian(header);

		if(wave->ChunkID1 != RIFF || wave->ChunkType != WAVE || wave->ChunkID2 != FMT)
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "unsupported Soundfile (WAVE only)") : printf("TuxCalD <unsupported Soundfile (WAVE only)>\n");
			fclose(fd_wav);
			return;
		}

		if(wave->Format != PCM)
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "unsupported Soundfile (PCM only)") : printf("TuxCalD <unsupported Soundfile (PCM only)>\n");
			fclose(fd_wav);
			return;
		}

		if(wave->SampleRate != 12000 && wave->SampleRate != 24000 && wave->SampleRate != 48000)
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "unsupported Soundfile (12/24/48KHz only)") : printf("TuxCalD <unsupported Soundfile (12/24/48KHz only)>\n");
			fclose(fd_wav);
			return;
		}

		format = (wave->BitsPerSample == 8) ? AFMT_U8 : AFMT_S16_LE;
		channels = wave->Channels;
		speed = wave->SampleRate;

		// step over unsed bytes in fmt-chunk
		count = wave->ChunkSize2 - 16;
		while(count)
		{
			fread(&tmp, 1, 1, fd_wav);
			count--;	
		}
		
		// find data-chunk
		do
		{
		  readcount=fread(&wave->ChunkID3, 1, 8, fd_wav);
		  SwapEndianChunk((char *)&wave->ChunkID3);
			if(readcount < 8)
			{
				slog ? syslog(LOG_DAEMON | LOG_INFO, "could not find Sounddata") : printf("TuxMailD <could not find Sounddata>\n");
				fclose(fd_wav);
				return;
			}
		  if(wave->ChunkID3 != DATA)
		  {
				count = wave->ChunkSize3;
				while(count)
				{
					fread(&tmp, 1, 1, fd_wav);
					count--;	
				}				  	
		  }
		}while(wave->ChunkID3 != DATA);

		// get samples
		if(!(samples = (unsigned char*)malloc(wave->ChunkSize3)))
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "not enough Memory for Sounddata") : printf("TuxCalD <not enough Memory for Sounddata>\n");
			fclose(fd_wav);
			return;
		}

		fread(samples, 1, wave->ChunkSize3, fd_wav);
		fclose(fd_wav);
	}
	else return;

	// play sound
	if((dsp = open(DSP, O_WRONLY)) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not open DSP") : printf("TuxCalD <could not open DSP>\n");
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not set DSP-Format") : printf("TuxCalD <could not set DSP-Format>\n");
		close(dsp);
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not set DSP-Channels") : printf("TuxCalD <could not set DSP-Channels>\n");
		close(dsp);
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not set DSP-Samplerate") : printf("TuxCalD <could not set DSP-Samplerate>\n");
		close(dsp);
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, &blocksize) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not get DSP-Blocksize") : printf("TuxCalD <could not get DSP-Blocksize>\n");
		close(dsp);
		return;
	}

	while(count < wave->ChunkSize3)
	{
		write(dsp, samples + count, (count + blocksize > wave->ChunkSize3) ?  wave->ChunkSize3 - count : blocksize);
		count += blocksize;
	}

	ioctl(dsp, SNDCTL_DSP_SYNC);

	// cleanup
	if (samples != NULL)	free(samples);

	close(dsp);
}

/******************************************************************************
 * EncodeBase64
 ******************************************************************************/
// from tuxmaild
void EncodeBase64(char *decodedstring, int decodedlen)
{
	char encodingtable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int src_index, dst_index;

	memset(encodedstring, 0, sizeof(encodedstring));
	for(src_index = dst_index = 0; src_index < decodedlen; src_index += 3, dst_index += 4)
	{
		encodedstring[0 + dst_index] = encodingtable[decodedstring[src_index] >> 2];
		encodedstring[1 + dst_index] = encodingtable[(decodedstring[src_index] & 3) << 4 | decodedstring[1 + src_index] >> 4];
		encodedstring[2 + dst_index] = encodingtable[(decodedstring[1 + src_index] & 15) << 2 | decodedstring[2 + src_index] >> 6];
		encodedstring[3 + dst_index] = encodingtable[decodedstring[2 + src_index] & 63];
	}

	if(decodedlen % 3)
	{
		switch(3 - decodedlen%3)
		{
			case 2:
				encodedstring[strlen(encodedstring) - 2] = '=';
			case 1:
			encodedstring[strlen(encodedstring) - 1] = '=';
		}
	}
}

/******************************************************************************
 * NotifyUser
 ******************************************************************************/
void NotifyUser()
{
	struct sockaddr_in SockAddr;
	char http_cmd[1024], tmp_buffer[1024],tmp_buffer1[1024];
	char *http_cmd1 = "GET /cgi-bin/startPlugin?name=tuxcal.cfg HTTP/1.1\n\n";
	char *http_cmd2 = "GET /cgi-bin/xmessage?timeout=10&caption=TuxCal%20Information&body=";
	char *http_cmd3 = "GET /control/message?nmsg=";
	char *http_cmd4 = "GET /control/message?popup=";
	int iBirthday = 0;
	
/*
	{
		printf("we found %d event today with time info\r\n",iCntEvents[0]);
		printf("we found %d event today without time info\r\n",iCntEvents[1]);
		printf("we found %d event tomorrow\r\n",iCntEvents[2]);
		printf("we found %d event in two days\r\n",iCntEvents[3]);
		printf("we found %d event at exact this time\r\n",iCntTmEvents);
	}
*/

	// video notify
	if(video != 5)
	{
		switch(video)
		{
			case 4:	strcpy(http_cmd, http_cmd4); break;
			case 3:	strcpy(http_cmd, http_cmd3); break;
			case 2:	strcpy(http_cmd, http_cmd2); break;
			default:	strcpy(http_cmd, http_cmd1);
		}

		if(video > 1)
		{
			int day, month, year;
			int i;
			EVT_DB* pEvt;
			
			day = tCheck_day;
			month = tCheck_mon;
			year = tCheck_year;

			// start with a newline (because of dreambox-httpd)
//			tmp_buffer[0]=0;
			strcpy(tmp_buffer, http_ln);

			// event now
			if (iCntTmEvents)																	
			{
				strcat(tmp_buffer, http_br);
				sprintf(tmp_buffer1,http_lines[MAXCHECKDAYS][osdidx],tCheck_hour,tCheck_min);
				strcat(tmp_buffer, tmp_buffer1);
//				printf("TuxCald <%s>\r\n",tmp_buffer);
				
				for (i=0;i<iCntTmEvents;i++)
				{
					pEvt=&eventdb[iEventType[MAXCHECKDAYS+1][i]];
					sprintf(tmp_buffer1,infotype[1][osdidx],pEvt->info);
					strcat(tmp_buffer, tmp_buffer1);
				}
				strcat(tmp_buffer, http_ln);
//				printf("TuxCald <%s>\r\n",tmp_buffer);
			}
			else
			{			
  			// todays event with time-info
  			if (iCntEvents[0])																	
  			{
  				strcat(tmp_buffer, http_br);
  				if (osdidx == 0)
  					sprintf(tmp_buffer1,http_lines[0][osdidx],day,monthmsg[month-1][osdidx],year);
  				else
  					sprintf(tmp_buffer1,http_lines[0][osdidx],monthmsg[month-1][osdidx],day,year);
  				strcat(tmp_buffer, tmp_buffer1);
  				
  				for (i=0;i<iCntEvents[0];i++)
  				{
  					pEvt=&eventdb[iEventType[0][i]];
  					sprintf(tmp_buffer1,infotype[2][osdidx],pEvt->info,pEvt->hour,pEvt->min);
  					strcat(tmp_buffer, tmp_buffer1);
  				}
  				strcat(tmp_buffer, http_ln);
//					printf("TuxCald <%s>\r\n",tmp_buffer);
  			}
  			
				// check events/birthdays/holidays for max. three days
  			int k;
  			for (k=1;k<=MAXCHECKDAYS;k++)
  			{
  				if (iCntEvents[k])																	
	  			{
  					strcat(tmp_buffer, http_br);
  					if (osdidx == 0)
  						sprintf(tmp_buffer1,http_lines[k-1][osdidx],day,monthmsg[month-1][osdidx],year);
  					else
  						sprintf(tmp_buffer1,http_lines[k-1][osdidx],monthmsg[month-1][osdidx],day,year);
  					strcat(tmp_buffer, tmp_buffer1);
  				
  					for (i=0;i<iCntEvents[k];i++)
  					{
  						if (iEventType[k][i]>MAXENTRYS)
  						{
	  						sprintf(tmp_buffer1,infotype[3][osdidx],vdaysnames[iEventType[k][i]-MAXENTRYS-1][osdidx]);
	  					}
  						else
  						{
	  						pEvt=&eventdb[iEventType[k][i]];
	  						if (pEvt->type == BIRTHDAY)
  							{
  								iBirthday = 1;
  								sprintf(tmp_buffer1,infotype[0][osdidx],pEvt->info,tCheck_year-pEvt->year);
  							}
  							else
	  							sprintf(tmp_buffer1,infotype[1][osdidx],pEvt->info);
	  					}	
  						strcat(tmp_buffer, tmp_buffer1);
  					}
  					strcat(tmp_buffer, http_ln);
//					printf("TuxCald <%s>\r\n",tmp_buffer);
	  			}
					// step to next day
  				AddDays(&day,&month,&year,1);
				}  				
  		}
  		
			// convert spaces and german-characters
			char *src=tmp_buffer;
			char *dest=http_cmd+strlen(http_cmd);
			char c;
			while ((c=*src++)!=0)
			{
				switch (c)
				{
					case ' ': strcat(http_cmd,"%20"); 		dest+=3; break;
					case '�': strcat(http_cmd,"%C3%A4");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%B6");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%BC");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%84");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%96");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%9C");  dest+=6; break;
					case '�': strcat(http_cmd,"%C3%9F");  dest+=6; break;
					default:
						*dest++=c;
						*dest=0;
				}
			}
		}

		strcat(http_cmd, " HTTP/1.1\n");
//		printf("TuxCald <%s>\r\n",http_cmd);

		if(webuser[0])
		{
			strcat(http_cmd, "Authorization: Basic ");

			strcpy(decodedstring, webuser);
			strcat(decodedstring, ":");
			strcat(decodedstring, webpass);
			EncodeBase64(decodedstring, strlen(decodedstring));

			strcat(http_cmd, encodedstring);
			strcat(http_cmd, "\n\n");			
		}
		else
		{
			strcat(http_cmd, "\n");
		}
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "could not create Socket") : printf("TuxCalD <could not create Socket>\n");
	}
	else
	{
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_port = htons(webport);
		inet_aton("127.0.0.1", &SockAddr.sin_addr);
	
		if (connect(sock, (struct sockaddr*)&SockAddr, sizeof(SockAddr)))
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "could not connect to WebServer") : printf("TuxCalD <could not connect to WebServer>\n");
		}
		else
		{
			send(sock, http_cmd, strlen(http_cmd), 0);
		}
		close(sock);
	}
	
	// audio notify
	if (audio == 'Y')
	{
		if (iCntTmEvents) PlaySound(CFGPATH SNDFILE3);
		else if (iBirthday) PlaySound(CFGPATH SNDFILE1);
		else PlaySound(CFGPATH SNDFILE2);
	}
			
	sprintf(tmp_buffer,"%s %d %d",CFGPATH SHELLFILE,iCntTmEvents,iBirthday);
	system(tmp_buffer);
}		


//
// SigHandler
//
void SigHandler(int signal)
{
	switch(signal)
	{
		case SIGTERM:
			slog ? syslog(LOG_DAEMON | LOG_INFO, "shutdown") : printf("TuxCalD <shutdown>\n");
			online = 0;																															// no longer online
			ClearScreen();
			intervall = 0;																													// let the daemon end
			break;

		case SIGHUP:
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "update") : printf("TuxCalD <update>\n");
			int iOnlineTmp = online;
			online = 0;																															// stop output now
			ClearScreen();
			ReadConf();																															// load configuration
			LoadDatabase();																													// load database
			oldyear = 0;																														// initiate no read of database
			online = iOnlineTmp;
		}	break;

		case SIGUSR1:
			online = 1;
			if(slog) syslog(LOG_DAEMON | LOG_INFO, "wakeup");
			else printf("TuxCalD <wakeup>\n");
			break;
			
		case SIGUSR2:
			online = 0;																															// no longer online
			ClearScreen();
			if(slog) syslog(LOG_DAEMON | LOG_INFO, "sleep");
			else printf("TuxCalD <sleep>\n");
			break;

		case SIGALRM:
			ClearScreen();
			if (show_clock=='Y') unlink(CLKFILE);
			else fclose(fopen(CLKFILE, "w"));
			if (slog) syslog(LOG_DAEMON | LOG_INFO, "show/hide the clock");
			else printf("TuxCalD <show/hide the clock>\n");
			break;	
	}
}

//
// MainProgram
//
int main(int argc, char **argv)
{
	char cvs_revision[] = "$Revision: 1.14 $";
	int param, nodelay = 0;
	pthread_t thread_id;
	void *thread_result = 0;
	
	sscanf(cvs_revision, "%*s %s", versioninfo_d);

	// check commandline parameter
	if(argc > 1)
	{
		for(param = 1; param < argc; param++)
		{
			if(!strcmp(argv[param], "-nodelay"))
			{
				nodelay = 1;
			}
			else if(!strcmp(argv[param], "-syslog"))
			{
				slog = 1;
				openlog("TuxCalD", LOG_ODELAY, LOG_DAEMON);
			}
			else if(!strcmp(argv[param], "-play"))
			{
				param++;
				PlaySound(argv[param]);
				return 0;
			}
			else if(!strcmp(argv[param], "-v"))
			{
				printf("%s\r\n",versioninfo_d);
				return 0;
			}
		}
	}

	// create daemon
	time(&tt);
	strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

	switch(fork())
	{
		case 0:
			slog ? syslog(LOG_DAEMON | LOG_INFO, "%s started [%s]", versioninfo_d, timeinfo) : printf("TuxCalD %s started [%s]\n", versioninfo_d, timeinfo);
			setsid();
			chdir("/");
		break;

		case -1:
			slog ? syslog(LOG_DAEMON | LOG_INFO, "%s aborted!", versioninfo_d) : printf("TuxCalD %s aborted!\n", versioninfo_d);
			return -1;
		default:

		exit(0);
	}

	// check for running daemon
	if ((fd_pid = fopen(PIDFILE, "r+")))
	{
		fscanf(fd_pid, "%d", &pid);

		if(kill(pid, 0) == -1 && errno == ESRCH)
		{
			pid = getpid();
			rewind(fd_pid);
			fprintf(fd_pid, "%d", pid);
			fclose(fd_pid);
		}
		else
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "Daemon already running with PID %d", pid) : printf("TuxCalD <Daemon already running with PID %d>\n", pid);
			fclose(fd_pid);
			return -1;
		}
	}
	else
	{
		pid = getpid();
		fd_pid = fopen(PIDFILE, "w");
		fprintf(fd_pid, "%d", pid);
		fclose(fd_pid);
	}

	// install sighandler
	if (signal(SIGTERM, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for TERM failed") : printf("TuxCalD <Installation of Signalhandler for TERM failed>\n");
		return -1;
	}
	if (signal(SIGHUP, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for HUP failed") : printf("TuxCalD <Installation of Signalhandler for HUP failed>\n");
		return -1;
	}

	if (signal(SIGUSR1, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for USR1 failed") : printf("TuxCalD <Installation of Signalhandler for USR1 failed>\n");
		return -1;
	}

	if (signal(SIGUSR2, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for USR2 failed") : printf("TuxCalD <Installation of Signalhandler for USR2 failed>\n");
		return -1;
	}

	if (signal(SIGALRM, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for ALRM failed") : printf("TuxCalD <Installation of Signalhandler for ALRM failed>\n");
		return -1;
	}

	// install communication interface
	if (pthread_create(&thread_id, NULL, InterfaceThread, NULL))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Interface-Thread failed") : printf("TuxCalD <Interface-Thread failed>\n");
		return -1;
	}

	// read, update or create config
	ReadConf();																							// read actual config
	WriteConf();																						// write actual config back to file

	// startdelay
	if (!nodelay)	
		sleep(startdelay);

	// read the dates and times
	LoadDatabase();																					// load database
	ReadSTimer();																						// read the timers for fixed signaling
	
	// we are online now
	online = 1;
	show_clock=show_clockatstart;
	
	unlink(CLKFILE);
	
	if (show_clock == 'Y')
	{
		// open connection to framebuffer
		iFB=OpenFB();
		if (!iFB) show_clock = 'N';
		else fclose(fopen(CLKFILE, "w"));
	}
		
	// definitions for some variables
	char info[MAXCLOCKINFOLEN];														// storage for clock/date output
	int iLen1;																						// used len for this info
	int iLen2;
	int y;																								// framebuffer output linecounter
	unsigned char *pmem1;																	// pointer for framebuffer-output
	unsigned char *pmem2;
	int iNewMails = 0;																		// new mail received by tuxmail	
	int iFG;
	int iBG;
	
	// check events until signal to finish
	do
	{
		// check and display clock only if online
		if (online)
		{		
			time(&tt);																				// read the actual time			
			at = localtime(&tt);

			// correct colors 
			iFG = disp_color;
			iBG = disp_back;
			if (iBlack!=-1)
			{
				if (disp_color==1) iFG=iBlack;
				if (disp_back==1)  iBG=iBlack;
			}
			if (iWhite!=-1)
			{
				if (disp_color==2) iFG=iWhite;
				if (disp_back==2)  iBG=iWhite;
			}
			
			//------------------- part for controlling the clock		
			if ((iFB) && (disp_clock=='Y') && (show_clock=='Y'))					// should we display the clock
			{
				FILE* pipe;
				pipe = fopen(CLKFILE,"r");
				if (pipe == NULL) 
				{
					show_clock='N';
					ClearScreen();
				}
				else fclose(pipe);
			}
			else if (disp_clock=='Y')
			{
				FILE* pipe;
				pipe = fopen(CLKFILE,"r");
				if (pipe != NULL)
				{
					if (!iFB) iFB=OpenFB();
					if (iFB) show_clock = 'Y';
					fclose(pipe);
				}
				else 
				{
					show_clock = 'N';
				}
			}
			
			//------------------- part for showing the clock		
			if ((iFB) && (disp_clock=='Y') && (show_clock=='Y'))					// should we display the clock
			{				
				char line[10];
				if (disp_mail=='Y')
				{
					// check for mail
					FILE* pipe;
					pipe = fopen(NOTIFILE,"r");
					if( pipe != NULL)
					{
						if (fgets( line, 10, pipe ))
							iNewMails=atoi(line);												// number of new mails
						fclose(pipe);
					}
					else
					{
						if (iNewMails)	
						{
							ClearScreen();
						}
						iNewMails=0;
					}
				}
				else
				{
					if (iNewMails)	
					{
						ClearScreen();
					}
					iNewMails=0;
				}
				
				strftime(info,MAXCLOCKINFOLEN,infomsgclock[(disp_sec == 'Y') ? 1 : 0],at);		
				iLen1=(strlen(info)*iFontSize)/2;				
				if (iNewMails)
				{
					RenderBox(0,8,iFontSize*2,iFontSize+2,FILL,iBG);
					if (at->tm_sec & 1)
					{
						sprintf(line,"%d",iNewMails);
						RenderInt(line,0,iFontSize,iFontSize*2, CENTER, iFont, iFG,-1,iBG);
					}
					else RenderSObject((iFontSize*2-OBJ_SX)/2,(iFontSize-OBJ_SY)/2+8,iFG,OBJ_LETTER);
				}
				RenderInt(info,iNewMails?(iFontSize*2+5):0,iFontSize,iLen1, (startx>(ex/2))?FIXEDRIGHT:FIXEDLEFT, iFont, iFG,-1,iBG);
				if (iNewMails) iLen1+=(iFontSize*2+5);
				
				iLen2=0;
				if (disp_date=='Y')															// should we display the date
				{
					strftime(info,MAXCLOCKINFOLEN,infomsg[0][osdidx],at);
					iLen2=(strlen(info)*iFontSize)/2;
					RenderInt(info,0,2*iFontSize,iLen2, (startx>(ex/2))?FIXEDRIGHT:FIXEDLEFT, iFont, iFG,-1,iBG);
				}
				
				int iLen;
				iLen=(iLen1>iLen2)?iLen1:iLen2;
				
				// output to framebuffer	
				pmem1=lfb+starty*var_screeninfo.xres+startx;
				pmem2=lbb+(starty+3)*var_screeninfo.xres+startx;
				if (iNewMails) pmem1-=(OBJ_SX+15);
				for (y=0;y<(iFontSize);y++)
				{			
					memcpy(pmem1,pmem2,iLen);
					pmem1+=var_screeninfo.xres;
					pmem2+=var_screeninfo.xres;
				}
				if (disp_date=='Y')
					for (y=0;y<(iFontSize);y++)
					{			
						memcpy(pmem1,pmem2,iLen);
						pmem1+=var_screeninfo.xres;
						pmem2+=var_screeninfo.xres;
					}
			}
			
			//------------------- part for calculating events
			int bChanged = 0;
			int iFoundEvent = 0;
			iCntTmEvents = 0;
			tCheck_year = at->tm_year+1900;
			if (oldyear!=tCheck_year)
			{
				// calculate the christian and other holidays
				FillEasternDays(tCheck_year);
				bChanged=1;
			}

			tCheck_min = at->tm_min;
			tCheck_hour = at->tm_hour;
			
			// check for signaling at s-times
			if ((tCheck_min != oldmin) || (tCheck_hour != oldhour))
			{
				int i;
				// scan all s-timers
				for (i=0;i<MAXSTIMER; i++)
				{
					if ((tSignal[i].hour==tCheck_hour) && (tSignal[i].min==tCheck_min)) 
					{
						bChanged=1;																												// flag used to enable signal
//						printf("TuxCalD <signal at %02d:%02d>\r\n",tCheck_hour,tCheck_min);
					}
				}				
			}
			
			tCheck_mon = at->tm_mon+1;
			tCheck_day = at->tm_mday;
			
			if ((tCheck_year != oldyear) || (tCheck_mon != oldmonth) || (tCheck_day != oldday) || (bChanged))
			{
				oldyear = tCheck_year;
				oldmonth = tCheck_mon;
				oldday = tCheck_day;
				iFoundEvent = IsEvent(tCheck_day, tCheck_mon, tCheck_year);
			}
			
			if ((tCheck_min != oldmin) || (tCheck_hour != oldhour) || (bChanged))
			{
				oldhour = tCheck_hour;
				oldmin = tCheck_min;

				int i;
				EVT_DB* pEvt;
				// check all events on this day if the event is at exact time
				for (i=0; i<iCntEvents[0]; i++)
				{
					pEvt=&eventdb[iEventType[0][i]];
					if ((pEvt->year == tCheck_year) && (pEvt->month == tCheck_mon) && (pEvt->day == tCheck_day)
					    && (pEvt->hour == tCheck_hour) && (pEvt->min == tCheck_min))
					{
						iFoundEvent=1;
						iEventType[MAXCHECKDAYS+1][iCntTmEvents++]=iEventType[0][i];
					}
				}		
			}	
			
			// we found (min.) one event to signal now
			if (iFoundEvent) NotifyUser();
		}
		sleep(intervall);
	}
	while (intervall);

	// cleanup
	pthread_cancel(thread_id);
	pthread_join(thread_id, thread_result);
	
	// close connection to framebuffer
	CloseFB();	
	
	unlink(PIDFILE);
	unlink(SCKFILE);
	
	time(&tt);
	strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

	slog ? syslog(LOG_DAEMON | LOG_INFO, "%s closed [%s]", versioninfo_d, timeinfo) : printf("TuxCalD %s closed [%s]\n", versioninfo_d, timeinfo);

	closelog();

	return 0;
}
