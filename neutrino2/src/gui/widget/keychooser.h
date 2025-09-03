//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: keychooser.h 31012025 mohousch Exp $
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef __keychooser__
#define __keychooser__

#include <string>

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/fontrenderer.h>

#include <driver/rcinput.h>

#include <system/localize.h>

#include <gui/widget/listbox.h>
#include <gui/widget/component.h>
#include <gui/widget/widget.h>


class CKeyChooserItem;
class CKeyChooserItemNoKey;

class CKeyChooser : public CWidgetTarget
{
	private:
		CFrameBuffer* frameBuffer;
		
		std::string title;
		long * key;
		CKeyChooserItem* keyChooser;
		CKeyChooserItemNoKey* keyDeleter;
		
		//
		CBox cFrameBox;
		CWidget* widget;
		ClistBox* menu;
		int selected;
		
		int paint();

	public:
		//
		CKeyChooser(long * const Key, const char* const Title);
		virtual ~CKeyChooser();
		
		//
		void hide();
		int exec(CWidgetTarget* parent, const std::string & actionKey);
};

class CKeyChooserItem : public CWidgetTarget
{
	private:
		CBox m_cBox;
		CBox m_cTitle;
		int hheight;
		int mheight;

		CCWindow m_cBoxWindow;

		std::string name;
		long * key;

		void paint();

	public:
		CKeyChooserItem(const char* const Name, long *Key);

		void hide();
		int exec(CWidgetTarget* parent, const std::string& actionKey);

};

class CKeyChooserItemNoKey : public CWidgetTarget
{
		long *key;

	public:
		CKeyChooserItemNoKey(long * Key)
		{
			key = Key;
		};

		int exec(CWidgetTarget */*parent*/, const std::string &/*actionKey*/)
		{
			*key = CRCInput::RC_nokey;
			return RETURN_REPAINT;
		}
};

#endif

