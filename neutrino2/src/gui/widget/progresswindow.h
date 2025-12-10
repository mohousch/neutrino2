//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: progresswindow.h 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
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

#ifndef __progresswindow__
#define __progresswindow__

#include <string>

#include <driver/gdi/framebuffer.h>

#include <system/localize.h>

#include <gui/widget/widget.h>
#include <gui/widget/component.h>


class CProgressWindow
{
	protected:

		CFrameBuffer* frameBuffer;
		
		//
		int x;
		int y;
		int width;
		int height;
		int hheight; // head font height
		int mheight; // menu font height

		CCWindow m_cBoxWindow;

		std::string captionString;

		//
		int global_progress;
		int globalstatusX;
		int globalstatusY;
		int statusTextY;
		std::string statusText;
		
		CCProgressBar *progressBar;

		bool paintHead;
		bool paintCancelIcon;
		
		void initFrames(int _x = 0, int _y = 0, int _width = 0, int _height = 0);

	public:

		CProgressWindow(int _x = 0, int _y = 0, int _width = 0, int _height = 0);
		virtual ~CProgressWindow(){ delete progressBar; progressBar = NULL;};

		void paint(bool _selected = false);
		void hide();

		//
		void setTitle(const char* const title);
		void showGlobalStatus(const unsigned int prog);
		void showStatusMessageUTF(const std::string & text); // UTF-8
		
		//
		void enableCancelIcon(){paintCancelIcon = true;};

		unsigned int getGlobalStatus(void){return global_progress;};
};

#endif

