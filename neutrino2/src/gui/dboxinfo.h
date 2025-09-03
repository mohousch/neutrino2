/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: dboxinfo.h 2013/10/12 mohousch Exp $

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


#ifndef __dboxinfo__
#define __dboxinfo__

#include <string>

#include <driver/gdi/framebuffer.h>

#include <gui/widget/widget.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/widget.h>


class CDBoxInfoWidget : public CWidgetTarget
{
	private:
		CFrameBuffer* frameBuffer;
		
		CBox Box;
		
		CWidget* dboxInfoWidget;
		CCHeaders* head;
		
		CCIcon* cpuIcon;
		CCLabel* cpuLabel;
		CCLabel* cpuLabel1;
		CCLabel* cpuLabel2;
		CCHline* hLine;
		CCLabel* upLabel;
		CCLabel* memLabel;
		CCHline* hLine2;
		CCIcon* hddIcon;
		CCLabel* hddLabel;
		CCLabel* hddLabel1;
		CCIcon* tunerIcon;
		CCLabel* tunerLabel;
		CCLabel* tunerLabel1;
		
		//
		int showInfo();

	public:

		CDBoxInfoWidget();
		~CDBoxInfoWidget(){};

		void hide();
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

class CInfoMenu : public CWidgetTarget
{
	private:
		CWidget* widget;
		ClistBox* infoMenu;
	
		int showMenu();
		
	public:
		CInfoMenu(){};
		~CInfoMenu(){};
		
		void hide(){};
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

#endif

