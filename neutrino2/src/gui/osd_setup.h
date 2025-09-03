//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: osd_setup.h 24022025 mohousch $
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

#ifndef __osd_setup__
#define __osd_setup__

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <string>


////
class COSDSettings : public CWidgetTarget
{
	private:
		int showMenu(void);
		
	public:
		COSDSettings(){};
		virtual ~COSDSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

//// osd menucolor settings
class COSDMenuColorSettings : public CWidgetTarget
{
	private:
		void showMenu();
		
	public:
		COSDMenuColorSettings(){};
		virtual ~COSDMenuColorSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

//// osd infobarcolor settings
class COSDInfoBarColorSettings : public CWidgetTarget
{
	private:
		void showMenu();
		
	public:
		COSDInfoBarColorSettings(){};
		virtual ~COSDInfoBarColorSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

// osd language settings
class CLanguageSettings : public CWidgetTarget
{
	private:
		bool fromStartWizzard;
		int showMenu();
		
	public:
		CLanguageSettings(bool wizzard = false);
		virtual ~CLanguageSettings(){};
		
		int exec(CWidgetTarget *parent, const std::string &actionKey);
};

class CFontSettings : public CWidgetTarget
{
	private:
		void showMenu();
		
	public:
		CFontSettings(){};
		virtual ~CFontSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

// osd timing settings
class COSDTimingSettings : public CWidgetTarget
{
	private:
		void showMenu();
		
	public:
		COSDTimingSettings(){};
		virtual ~COSDTimingSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

// diverses
class COSDDiverses : public CWidgetTarget
{
	private:
		void showMenu();
		
	public:
		COSDDiverses(){};
		virtual ~COSDDiverses(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

// skin
class CSkinManager : public CWidgetTarget
{
	private:
		int showMenu();
		
	public:
		CSkinManager(){};
		virtual ~CSkinManager(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

class CSkinSettings : public CWidgetTarget
{
	private:
		ClistBox *skinSettings;
		
		int showMenu();
		
	public:
		CSkinSettings(){};
		virtual ~CSkinSettings(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

class CPersonalisation : public CWidgetTarget
{
	private:
		int showMenu(void);
		
	public:
		CPersonalisation(){};
		virtual ~CPersonalisation(){};
		
		int exec(CWidgetTarget* parent, const std::string& actionKey);
};

#endif //__osd_setup__

