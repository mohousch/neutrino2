/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: hdd_menu.h 2013/10/12 mohousch Exp $

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

#ifndef __hdd_menu__
#define __hdd_menu__

#include <gui/widget/icons.h>
#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <gui/filebrowser.h>


//// HDD menu handler
class CHDDMenuHandler : public CTarget
{
	private:
		int HDDMenu();
			
	public:
		int  exec( CTarget * parent,  const std::string &actionKey);
};

//// HDDdest
class CHDDDestExec : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string&);
};

//// HDDinit
class CHDDInit : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string& actionKey);
};

//// HDDformat
class CHDDFmtExec : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string&);
};

//// HDDcheckfs
class CHDDChkExec : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string&);
};

//// HDDmountMSG
class CHDDMountMSGExec : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string&);
};

//// HDDumount
class CHDDuMountMSGExec : public CTarget
{
	public:
		int exec(CTarget * parent, const std::string&);
};

//// HDDBrowser
class CHDDBrowser : public CTarget
{	
	public:
		int exec(CTarget * parent, const std::string& actionKey);
};

#endif	//hdd_menu_h

