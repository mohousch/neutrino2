/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: keychooser.h 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#ifndef __keychooser__
#define __keychooser__

#include <string>

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <system/localize.h>

#include <gui/widget/listbox.h>
#include <gui/widget/window.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/widget.h>
#include <gui/widget/menue.h>


class CKeyChooserItem;
class CKeyChooserItemNoKey;

class CKeyChooser : public CMenuWidget/*CMenuTarget*/
{
	private:
		CFrameBuffer* frameBuffer;
		
		std::string title;
		int* key;
		CKeyChooserItem* keyChooser;
		CKeyChooserItemNoKey* keyDeleter;
		
		//
		//CWidget* widget;
		//ClistBox* menu;
		
		void paint();

	public:
		//
		CKeyChooser(int* const Key, const char* const Title, const std::string& Icon = "");
		~CKeyChooser();
		
		//
		//int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CKeyChooserItem : public CMenuTarget
{
	private:
		CBox m_cBox;
		CBox m_cTitle;

		CWindow m_cBoxWindow;

		std::string name;
		int* key;

		void paint();

	public:
		CKeyChooserItem(const char* const Name, int *Key);

		void hide();
		int exec(CMenuTarget* parent, const std::string& actionKey);

};

class CKeyChooserItemNoKey : public CMenuTarget
{
		int *key;

	public:
		CKeyChooserItemNoKey(int* Key)
		{
			key = Key;
		};

		int exec(CMenuTarget */*parent*/, const std::string &/*actionKey*/)
		{
			*key = RC_nokey;
			return RETURN_REPAINT;
		}
};

#endif
