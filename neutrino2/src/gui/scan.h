/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: scan.h 05102024 mohousch Exp $

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

#ifndef __scants__
#define __scants__

#include <string>

#include <gui/widget/widget.h>

#include <driver/gdi/framebuffer.h>

#include <system/localize.h>
#include <gui/widget/component.h>

#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>


#define NEUTRINO_SCAN_SETTINGS_FILE	CONFIGDIR "/scan.conf"

class CScanSettings;

class CScanTs : public CTarget
{
	private:
		CFrameBuffer *frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight, mheight; 	// head/menu font height
		int xpos1; 		// x position for first column
		int xpos2; 		// x position for second column
		int radar; 
		int xpos_radar;
		int ypos_radar;
		int ypos_cur_satellite;
		int ypos_transponder;
		int ypos_frequency;
		int ypos_provider;
		int ypos_channel;
		int ypos_service_numbers;
		bool success;
		bool canceled;
		bool istheend;
		uint32_t total;
		uint32_t done;
		
		CCProgressBar * snrscale, * sigscale;

		void paint(bool fortest = false);
		void paintLineLocale(int _x, int * _y, int _width, const char* const l);
		void paintLine(int _x, int _y, int width, const char * const txt);
		void paintRadar(void);
		neutrino_msg_t handleMsg(neutrino_msg_t msg, neutrino_msg_data_t data);
		int greater_xpos(int xpos, const char* const txt);
		bool freqready;
		void showSNR();
		
		//
		CFrontend *fe;
		CScanSettings * scanSettings;

	public:
		CScanTs(CFrontend *f, CScanSettings * sc);
		void hide();
		int exec(CTarget* parent, const std::string & actionKey);
};

#endif

