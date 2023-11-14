/*
  Neutrino-GUI  -   DBoxII-Project

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


#ifndef __screensetup__
#define __screensetup__

#include <gui/widget/widget.h>

#include <driver/gfx/framebuffer.h>

#include <string>


class CScreenSetup : public CMenuTarget
{
	private:
		CFrameBuffer * frameBuffer;
		int	selected;
		int x_coord[2];
		int y_coord[2];
		
		int icon_w;
		int icon_h;
		
		int x;
		int y;
		int BoxHeight;
		int BoxWidth;

		void paint();
		void paintBorderUL();
		void paintBorderLR();
		void paintCoords();
		void paintBorder(int selected);
		void paintIcons();
		
	public:
		CScreenSetup();
		void hide();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};


#endif
