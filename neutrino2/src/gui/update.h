/*
	Neutrino-GUI  -   DBoxII-Project
	
	$id: update.h 2015.12.22 11:38:28 mohousch $

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


#ifndef __update__
#define __update__

#include <string>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>
#include <gui/widget/progresswindow.h>

#include <driver/gdi/framebuffer.h>


//// CFlashUpdate
class CFlashUpdate : public CWidgetTarget
{
	public:
		enum 
		{
			UPDATEMODE_MANUAL,
			UPDATEMODE_INTERNET
		};
	
		int updateMode;
	  
	private:
		CProgressWindow *progressWindow;

		bool allow_flash;

		std::string filename;
		std::string file_md5;
		char fileType;
		
		std::string installedVersion;
		std::string newVersion;
		
		bool selectHttpImage(void);
		bool getUpdateImage(const std::string &version);
		bool checkVersion4Update();
		
		////
		int showOfgWriteMenu();
		
	public:
		CFlashUpdate(int uMode);
		virtual ~CFlashUpdate();
		int exec( CWidgetTarget *parent, const std::string &actionKey );
};

//// CFlashExppert
class CFlashExpert : public CWidgetTarget
{
	private:
		CProgressWindow *progressWindow;

		int selectedMTD;

		void showMTDSelector(const std::string & actionkey);
		void showFileSelector(const std::string & actionkey);

		void readmtd(int readmtd);
		void writemtd(const std::string &filename, int mtdNumber);

	public:
		CFlashExpert();
		int exec(CWidgetTarget *parent, const std::string &actionKey);
};

//// update setup
class CUpdateSettings : public CWidgetTarget
{
	private:
		int showMenu();
		int showMTDExpert();
	
	public:
		CUpdateSettings(){};
		~CUpdateSettings(){};
		
		int exec(CWidgetTarget *parent, const std::string &actionKey);
};

#endif

