//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: zapit_setup.h 24022025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
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

#ifndef __ZAPIT_SETUP__
#define __ZAPIT_SETUP__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


class CZapitSetup : public CMenuTarget
{
	private:
		int selected;
		CWidget* widget;
		ClistBox* zapit;
		CMenuForwarder * m3, *m4;
		
		void showMenu();

	public:
		CZapitSetup();
		~CZapitSetup();
		
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CZapitSetupNotifier : public CChangeObserver
{
	private:
		CMenuOptionChooser * zapit1;
		CMenuForwarder * zapit2, * zapit3;
	public:
		CZapitSetupNotifier(CMenuOptionChooser* m1, CMenuForwarder* m2, CMenuForwarder* m3);
		bool changeNotify(const std::string&, void * data);
};

class CZapitSetupModeNotifier : public CChangeObserver
{
	private:
		CMenuItem *item1, *item2;
		int *mode;
	public:
		CZapitSetupModeNotifier(int *zMode, CMenuItem *m1, CMenuItem *m2);
		bool changeNotify(const std::string&, void *);
};

#endif

