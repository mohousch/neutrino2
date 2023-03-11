/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: imageinfo.h 2013/10/12 mohousch Exp $

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


#ifndef __imageinfo__
#define __imageinfo__

#include <configfile.h>

#include <gui/widget/widget.h>

#include <driver/framebuffer.h>


class CImageInfo : public CMenuTarget
{
	private:
		//CConfigFile     * configfile;
		CFrameBuffer	*frameBuffer;
		
		//
		CWidget *widget;
		CWindow *window;
		
		//
		int x;
		int y;
		int width;
		int height;

		int ypos;
		int hheight, iheight, sheight; 	// head/info/small font height

		int font_head;
		int font_info;
		int font_small;

		void paint();
		void paintLine(int xpos, int font, const char* text);

	public:

		CImageInfo();
		~CImageInfo();

		void hide();
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

#endif

