/*
  $Id: systeminfo.h 2014/01/22 mohousch Exp $

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

#ifndef __sysinfo__
#define __sysinfo__

#include <plugin.h>


class CSysInfoWidget : public CMenuTarget
{
	private:
		enum {
			SYSINFO = 1,
			DMESGINFO,
			CPUINFO,
			PSINFO
		};
		
		CFrameBuffer *frameBuffer;

		CBox cFrameBox;
		CBox cFrameBoxTitle;
		CBox cFrameBoxText;
		CBox cFrameBoxFoot;

		CCIcon titleIcon;
		CTextBox* textBox;

		std::string buffer;

		bool syslistChanged;

		int mode;

		void paint();
		void paintHead();
		void paintFoot();

		void sysinfo();
		void dmesg();
		void cpuinfo();
		void ps();

		void readList();
	public:
		CSysInfoWidget(int m = SYSINFO);
		~CSysInfoWidget();
		int exec(CMenuTarget *parent, const std::string &actionKey);
		void hide();
};

#endif

