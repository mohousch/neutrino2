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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <gui/widget/drawable.h>

#include <global.h>
#include <neutrino.h>


Drawable::Drawable()
{	
}

Drawable::~Drawable()
{
}

int Drawable::getWidth(void)
{
	return m_width;
}

int Drawable::getHeight(void)
{
	return m_height;
}

Drawable::DType Drawable::getType(void)
{
	return Drawable::DTYPE_DRAWABLE;
}

// DText
DText::DText(std::string& text, CFont* font, uint8_t col, const bool bg) 
{
	m_text = text;
	m_font = font;
	m_color = col;
	m_background = bg;

	init();
}

DText::DText(const char *text, CFont* font, uint8_t col, const bool bg)
{
	m_text = std::string(text);
	m_font = font;
	m_color = col;
	m_background = bg;

	init();
}

void DText::init()
{
	m_width = m_font->getRenderWidth(m_text, true); // UTF-8
	m_height = m_font->getHeight() + 4;	
}

void DText::draw(int x, int y, int width)
{
	m_font->RenderString(x, y + m_font->getHeight() + 2, width, m_text.c_str(), m_color, 0, true, m_background); // UTF-8	
}

Drawable::DType DText::getType(void)
{
	return Drawable::DTYPE_TEXT;
}

// DIcon
DIcon::DIcon(std::string& icon)
{
	m_icon = icon;
	init();
}

DIcon::DIcon(const char *icon)
{
	m_icon = std::string(icon);
	init();
}

void DIcon::init()
{
	CFrameBuffer::getInstance()->getIconSize(m_icon.c_str(), &m_width, &m_height);

	m_height = m_height + 4;
}

void DIcon::draw(int x, int y, int)
{
	CFrameBuffer::getInstance()->paintIcon(m_icon.c_str(), x, y + 2);	
}

Drawable::DType DIcon::getType(void)
{
	return Drawable::DTYPE_ICON;
}

// DSeparator
DSeparator::DSeparator()
{
	m_height = 10;
	m_width = 0;
}

void DSeparator::draw(int x, int y, int width)
{
	int height = 10;

	CFrameBuffer::getInstance()->paintBoxRel(x, y, width, height, COL_MENUCONTENT_PLUS_0);

	// line
	CFrameBuffer::getInstance()->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );

	CFrameBuffer::getInstance()->paintHLineRel(x + BORDER_LEFT, width - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );	
}

Drawable::DType DSeparator::getType(void)
{
	return Drawable::DTYPE_SEPARATOR;
}

// DPageBreak
DPagebreak::DPagebreak()
{
	m_height = 0;
	m_width = 0;
}

void DPagebreak::draw(int, int, int)
{
}

Drawable::DType DPagebreak::getType(void)
{
	return Drawable::DTYPE_PAGEBREAK;
}


