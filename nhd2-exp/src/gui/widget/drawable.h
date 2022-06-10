/*
	Neutrino-GUI  -   DBoxII-Project

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


#ifndef __drawable__
#define __drawable__

#include <string>
#include <vector>

#include <global.h>

#include <system/localize.h>


class Drawable;

typedef std::vector<std::vector<Drawable*> > ContentLines;


/**
 * The base class for items which can be drawn on a CFBWindow.
 */
class Drawable
{
	public:

		enum DType {
			DTYPE_DRAWABLE,
			DTYPE_TEXT,
			DTYPE_ICON,
			DTYPE_SEPARATOR,
			DTYPE_PAGEBREAK
		};

		virtual ~Drawable();
		
		/**
		* Overwrite this method in subclasses to draw on the window
		*
		* @param window the window to draw on
		* @param x x component of the top left corner
		* @param y y component of the top left corner
		*/
		virtual void draw(int x, int y, int width) = 0;
		virtual int getWidth(void);
		virtual int getHeight(void);

		/**
		* Returns the type of this drawable. Used to distinguish between
		* drawing objects and control objects like pagebreaks.
		* @return the type of this drawable.
		*/
		virtual DType getType();

	protected:

		Drawable();

		int m_height;
		int m_width;

	private:		
};

/**
 * This class draws a given string.
 */
class DText : public Drawable
{
	public:
		DText(std::string& text, CFont* font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU], uint8_t col = COL_MENUCONTENT, const bool bg = false);
		DText(const char *text, CFont* font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU], uint8_t col = COL_MENUCONTENT, const bool bg = false);
		void init();
		void draw(int x, int y, int width);
		DType getType();

	protected:
		std::string m_text;
		CFont* m_font;
		uint8_t m_color;
		bool m_background;
};


/**
 * This class draws a given icon.
 */
class DIcon : public Drawable
{
	public:
		DIcon(std::string& icon);
		DIcon(const char  *icon);
		void init();
		void draw(int x, int y, int width);
		DType getType();

	protected:
		std::string m_icon;
};

/**
 * This class draws separator line.
 */
class DSeparator : public Drawable
{
	public:
		DSeparator();

		void draw(int x, int y, int width);
		DType getType();
};

/**
 * This class is used as a control object and forces a new page
 * in scrollable windows.
 */
class DPagebreak : public Drawable
{
	public:
		DPagebreak();

		void draw(int x, int y, int width);
		DType getType();	
};

#endif
