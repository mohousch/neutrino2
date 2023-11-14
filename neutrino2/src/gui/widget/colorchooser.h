/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: colorchooser.h 25.10.2023 mohousch Exp $
 
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


#ifndef __colorchooser__
#define __colorchooser__

#include <string>

#include <driver/gfx/framebuffer.h>

#include <system/localize.h>

#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>


class CColorChooser : public CMenuTarget
{
	private:
		CFrameBuffer* frameBuffer;

		CBox cFrameBox;
		CBox cFrameBoxTitle;
		CBox cFrameBoxBody;
		CBox cFrameBoxItem;
		CBox cFrameBoxColorPreview;

		CCWindow* m_cBoxWindow;

		unsigned char * value[4]; // r, g, b, alpha

		std::string name;

		CChangeObserver* observer;
		CColorSetupNotifier * notifier;

		void paint();
		void setColor();
		void paintSlider(int x, int y, unsigned char *spos, const char* const text, const char * const iconname, const bool selected);

	public:
		//
		CColorChooser(const char * const Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer = NULL); // UTF-8
		virtual ~CColorChooser();

		void hide();
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

#endif

