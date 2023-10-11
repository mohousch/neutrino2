/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: alphasetup.h 2013/10/12 mohousch Exp $
 
	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
 
	Kommentar:
 
	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.
	
 
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


#ifndef __alphasetup__
#define __alphasetup__


#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/listbox.h>

#include <driver/framebuffer.h>
#include <system/localize.h>

#include <string>


class CAlphaSetup : public CMenuTarget
{
	private:
		CFrameBuffer *frameBuffer;

		CBox mainBox;

		int hheight;		// head menu font height
		int mheight; 		// menu font height

		CCWindow *mainWindow;

		unsigned char *alpha;
		std::string name;
		CChangeObserver *observer;

		void paint();
		void setAlpha();
		void paintSlider(const int x, const int y, const unsigned char * const spos, const char* const text, const char * const iconname, const bool selected);

	public:
		CAlphaSetup(const char* const Name, unsigned char * Alpha, CChangeObserver * Observer = NULL);
		~CAlphaSetup();

		void hide();
		int exec(CMenuTarget *parent, const std::string &actionKey);
};

#endif

