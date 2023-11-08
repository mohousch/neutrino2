/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: dvbsub_select.h 2011/11/22 11:23:30 mohousch Exp $
	
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


#ifndef __dvbsub_selector__
#define __dvbsub_selector__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>


class CDVBSubSelectMenuHandler : public CMenuTarget
{
	public:
		int  exec( CMenuTarget* parent,  const std::string &actionKey);
		int  doMenu();
};

#endif

