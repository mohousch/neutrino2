//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: audio_select.h 21122024 mohousch Exp $
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifndef __audio_selector__
#define __audio_selector__

#include <gui/widget/icons.h>
#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>


class CAudioSelectMenuHandler : public CTarget, CChangeObserver
{
	public:
		CAudioSelectMenuHandler(){};
		~CAudioSelectMenuHandler(){};
		
		int exec( CTarget * parent, const std::string &actionkey);
		int doMenu();
		bool changeNotify(const std::string& OptionName, void *);
};

//// apid
class CAPIDChangeExec : public CTarget
{
	public:
		int exec(CTarget* parent, const std::string & actionKey);
};

//// subtitle
class CSubtitleChangeExec : public CTarget
{
	public:
		CSubtitleChangeExec(){};
		~CSubtitleChangeExec(){};
		
		int exec(CTarget* parent, const std::string& actionKey);
};

//// tuxtxt
class CTuxtxtChangeExec : public CTarget
{
	public:
		CTuxtxtChangeExec(){};
		~CTuxtxtChangeExec(){};
		
		int exec(CTarget* parent, const std::string& actionKey);
};

#endif

