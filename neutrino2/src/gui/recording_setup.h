/*
	Neutrino-GUI  -   DBoxII-Project

	$id: recording_setup.h 2016.01.02 21:42:28 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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

#ifndef __recording_setup__
#define __recording_setup__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


class CRecordingSettings : public CMenuTarget, CChangeObserver
{
	private:
		void showMenu();
		
		bool changeNotify(const std::string& OptionName, void *);
		
	public:
		CRecordingSettings();
		~CRecordingSettings();
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// recording safety notifier
class CRecordingSafetyNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const std::string&, void *);
};

// rec apids notifier
class CRecAPIDSettingsNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const std::string& OptionName, void*);
};

#endif // __recording_setup__
