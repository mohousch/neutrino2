//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: pictureviewer.h 04092025 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//
//	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
//	Copyright (C) 2002,2003 Dirch
//	Copyright (C) 2002,2003,2004 Zwen
//	Homepage: http://www.dbox2.info/
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

#ifndef __pictureviewer__
#define __pictureviewer__


#include <string>
#include <stdio.h>
#include <sys/time.h>

#include <vector>

#include <driver/gdi/libngpng.h>

#include <driver/gdi/framebuffer.h>


////
class CPicture
{
	public:
		std::string Filename;
		std::string Name;
		std::string Type;
		time_t Date;
};

typedef std::vector<CPicture> CPicturePlayList;

////
class CPictureViewer
{
	private:
		ScalingMode m_scaling;
		float m_aspect;
		
		std::string m_Pic_Name;
		int m_Pic_X;
		int m_Pic_Y;
		int m_Pic_XPos;
		int m_Pic_YPos;
		int m_Pic_XPan;
		int m_Pic_YPan;
		
		int m_busy_x;
		int m_busy_y;
		int m_busy_width;
		int m_busy_height;

		int m_startx;
		int m_starty;
		int m_endx;
		int m_endy;
		
	public:
		CPictureViewer();
		~CPictureViewer(){};
		
		////
		void setScaling(ScalingMode s){m_scaling = s;}
		void setAspectRatio(float aspect_ratio) {m_aspect = aspect_ratio;}
		void setVisible(int startx, int endx, int starty, int endy);
		static double m_aspect_ratio_correction;
		////
		void showBusy(fb_pixel_t col);
		void hideBusy();
		////
		bool showImage(const std::string & filename);
		bool decodeImage(const std::string & name, bool showBusySign = false);
		void zoom(float factor);
		void move(int dx, int dy);
};

#endif

