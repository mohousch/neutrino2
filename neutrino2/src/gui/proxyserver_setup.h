//
//	$Id: proxyserver_setup.h 25022025 mohousch Exp $
//
//	proxyserver_setup menue - Neutrino-GUI
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2010 T. Graf 'dbt'
//	Homepage: http://www.dbox2-tuning.net/
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
//

#ifndef __proxyserver_setup__
#define __proxyserver_setup__

#include <gui/widget/icons.h>
#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


class CProxySetup : public CWidgetTarget
{
	private:
		std::string menue_title;
		std::string menue_icon;

		int showProxySetup();

	public:	
		CProxySetup(){};
		~CProxySetup(){};
		
		int exec(CWidgetTarget * parent, const std::string &actionKey);
};

#endif
