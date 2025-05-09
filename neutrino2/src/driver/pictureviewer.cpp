//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: pictureviewer.cpp 21122024 mohousch Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <unistd.h>

#include <config.h>

#include <global.h>
#include <neutrino2.h>

#include "pictureviewer.h"

#include <system/debug.h>
#include <system/helpers.h>


double CPictureViewer::m_aspect_ratio_correction;

CPictureViewer::CPictureViewer()
{
	m_scaling = (ScalingMode)g_settings.picviewer_scaling;

	m_Pic_Name = "";
	m_Pic_X = 0;
	m_Pic_Y = 0;
	m_Pic_XPos = 0;
	m_Pic_YPos = 0;
	m_Pic_XPan = 0;
	m_Pic_YPan = 0;
	
	int xs, ys;
	xs = CFrameBuffer::getInstance()->getScreenWidth(true);
	ys = CFrameBuffer::getInstance()->getScreenHeight(true);
	
	m_aspect = 16.0/9;
	
	m_startx = 0;
	m_endx = xs - 1;
	m_starty = 0;
	m_endy = ys - 1;
	m_aspect_ratio_correction = m_aspect / ((double) xs / ys);
	
	m_busy_x = m_startx + 3;
	m_busy_y = m_starty + 3;
	m_busy_width = 10;
	m_busy_height = 10;
}

void CPictureViewer::setVisible (int startx, int endx, int starty, int endy)
{
	m_startx = startx;
	m_endx = endx;
	m_starty = starty;
	m_endy = endy;
}

bool CPictureViewer::decodeImage(const std::string &name, bool showBusySign)
{
	dprintf(DEBUG_INFO, "CPictureViewer::decodeImage: %s\n", name.c_str()); 
	
	int x;
	int y;
	int imx;
	int imy;
	int bpp;
	int nchans = 0;
	
	// Show red block for "next ready" in view state
	if (showBusySign)
	{
		showBusy(COL_RED_PLUS_0);
	}

	getSize(name.c_str(), &x, &y, &bpp, &nchans);
 
	if ((x > (m_endx - m_startx) || y > (m_endy - m_starty))) 
	{
		if ((m_aspect_ratio_correction * y * (m_endx - m_startx) / x) <= (m_endy - m_starty)) 
		{
			imx = (m_endx - m_startx);
			imy = (int) (m_aspect_ratio_correction * y * (m_endx - m_startx) / x);
		} 
		else 
		{
			imx = (int) ((1.0 / m_aspect_ratio_correction) * x * (m_endy - m_starty) / y);
			imy = (m_endy - m_starty);
		}

		x = imx;
		y = imy;
	}
			
	m_Pic_X = x;
	m_Pic_Y = y;

	if (x < (m_endx - m_startx))
		m_Pic_XPos = (m_endx - m_startx - x) / 2 + m_startx;
	else
		m_Pic_XPos = m_startx;

	if (y < (m_endy - m_starty))
		m_Pic_YPos = (m_endy - m_starty - y) / 2 + m_starty;
	else
		m_Pic_YPos = m_starty;

	if (x > (m_endx - m_startx))
		m_Pic_XPan = (x - (m_endx - m_startx)) / 2;
	else
		m_Pic_XPan = 0;
			
	if (y > (m_endy - m_starty))
		m_Pic_YPan = (y - (m_endy - m_starty)) / 2;
	else
		m_Pic_YPan = 0;
	
	hideBusy();

	return true;
}

bool CPictureViewer::showImage(const std::string &filename)
{
	dprintf(DEBUG_NORMAL, "CPictureViewer::showImage: %s\n", filename.c_str());
	
	m_Pic_Name = filename;

	// decode image
  	decodeImage(m_Pic_Name, true);
	
	// display image
	CFrameBuffer::getInstance()->displayImage(m_Pic_Name, m_Pic_XPos, m_Pic_YPos, m_Pic_X, m_Pic_Y, m_Pic_XPan, m_Pic_YPan, m_scaling);
	
  	return true;
}

void CPictureViewer::zoom(float factor)
{
	dprintf(DEBUG_NORMAL, "CPictureViewer::zoom %f\n",factor);
	
	CFrameBuffer::getInstance()->clearFrameBuffer();
	
	showBusy(COL_YELLOW_PLUS_0);
	
	int oldx = m_Pic_X;
	int oldy = m_Pic_Y;
	
	m_Pic_X = (int) (factor * m_Pic_X);
	m_Pic_Y = (int) (factor * m_Pic_Y);
	
	if (m_Pic_X < (m_endx - m_startx))
		m_Pic_XPos = (m_endx - m_startx - m_Pic_X) / 2 + m_startx;
	else
		m_Pic_XPos = m_startx;
	
	if (m_Pic_Y < (m_endy - m_starty))
		m_Pic_YPos = (m_endy - m_starty - m_Pic_Y) / 2 + m_starty;
	else
		m_Pic_YPos = m_starty;
	
	if (m_Pic_X > (m_endx - m_startx))
		m_Pic_XPan = (m_Pic_X - (m_endx - m_startx)) / 2;
	else
		m_Pic_XPan = 0;
	
	if (m_Pic_Y > (m_endy - m_starty))
		m_Pic_YPan = (m_Pic_Y - (m_endy - m_starty)) / 2;
	else
		m_Pic_YPan = 0;

	CFrameBuffer::getInstance()->displayImage(m_Pic_Name, m_Pic_XPos, m_Pic_YPos, m_Pic_X, m_Pic_Y, m_Pic_XPan, m_Pic_YPan, m_scaling);
}

void CPictureViewer::move(int dx, int dy)
{
	dprintf(DEBUG_NORMAL, "CPictureViewer::move %d %d (xpan:%d ypan:%d)\n", dx, dy, m_Pic_XPan, m_Pic_YPan);
	
	CFrameBuffer::getInstance()->clearFrameBuffer();
	
	showBusy(COL_GREEN_PLUS_0);
	
	int xs, ys;
	xs = CFrameBuffer::getInstance()->getScreenWidth(true);
	ys = CFrameBuffer::getInstance()->getScreenHeight(true);
	
	
	m_Pic_XPan += dx;

	if (m_Pic_XPan + xs >= m_Pic_X)
		m_Pic_XPan = m_Pic_X - xs - 1;
	
	if (m_Pic_XPan < 0)
		m_Pic_XPan = 0;
	
	m_Pic_YPan += dy;
	
	if (m_Pic_YPan + ys >= m_Pic_Y)
		m_Pic_YPan = m_Pic_Y - ys - 1;
	
	if (m_Pic_YPan < 0)
		m_Pic_YPan = 0;
	
	if (m_Pic_X < (m_endx - m_startx))
		m_Pic_XPos = (m_endx - m_startx - m_Pic_X) / 2 + m_startx;
	else
		m_Pic_XPos = m_startx;
	
	if (m_Pic_Y < (m_endy - m_starty))
		m_Pic_YPos = (m_endy - m_starty - m_Pic_Y) / 2 + m_starty;
	else
		m_Pic_YPos = m_starty;
	
	CFrameBuffer::getInstance()->displayImage(m_Pic_Name, m_Pic_XPos, m_Pic_YPos, m_Pic_X, m_Pic_Y, m_Pic_XPan, m_Pic_YPan, m_scaling);
}

void CPictureViewer::showBusy(fb_pixel_t col)
{
	dprintf(DEBUG_INFO, "CPictureViewer::Show Busy\n");

	CFrameBuffer::getInstance()->clearFrameBuffer();

	CFrameBuffer::getInstance()->paintBoxRel(m_busy_x, m_busy_y, m_busy_width, m_busy_height, col);
	
	CFrameBuffer::getInstance()->blit();
}

void CPictureViewer::hideBusy()
{
	dprintf(DEBUG_INFO, "CPictureViewer::Hide Busy\n");
	
	CFrameBuffer::getInstance()->paintBackgroundBoxRel(m_busy_x, m_busy_y, m_busy_width, m_busy_height);
	
	CFrameBuffer::getInstance()->blit();	
}

