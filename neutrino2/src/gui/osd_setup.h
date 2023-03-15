/*
	Neutrino-GUI  -   DBoxII-Project

	$id: osd_setup.h 21.09.21 mohousch $
	
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

#ifndef __osd_setup__
#define __osd_setup__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


class COSDSettings : public CMenuTarget
{
	private:
		void showMenu(void);
		
	public:
		COSDSettings();
		~COSDSettings();
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// osd menucolor settings
class COSDMenuColorSettings : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		COSDMenuColorSettings();
		~COSDMenuColorSettings();
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// osd infobarcolor settings
class COSDInfoBarColorSettings : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		COSDInfoBarColorSettings();
		~COSDInfoBarColorSettings();
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// osd language settings
class CLanguageSettings : public CMenuTarget, CChangeObserver
{
	private:
		bool fromStartWizzard;
		void showMenu();
		
	public:
		CLanguageSettings(bool wizzard = false);
		~CLanguageSettings();
		
		bool changeNotify(const std::string& OptionName, void *);
		
		int exec(CMenuTarget *parent, const std::string &actionKey);
};

class CFontSettings : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		CFontSettings();
		~CFontSettings();
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// timing setup notifier
class CTimingSettingsNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const std::string& OptionName, void *);
};

// osd timing settings
class COSDTimingSettings : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		COSDTimingSettings(){};
		~COSDTimingSettings(){};
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// diverses
class COSDDiverses : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		COSDDiverses(){};
		~COSDDiverses(){};
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

// skin
class CSkinManager : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		CSkinManager(){};
		~CSkinManager(){};
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

class CSkinSettings : public CMenuTarget
{
	private:
		void showMenu();
		
	public:
		CSkinSettings(){};
		~CSkinSettings(){};
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
};

#endif //__osd_setup__
