/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: stringinput.cpp 2013/10/12 mohousch Exp $

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

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <driver/color.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <system/debug.h>

#include <gui/widget/stringinput.h>


#define borderwidth 4

CStringInput::CStringInput(const char * const Head, const char * const Value, int Size, const char* const Hint_1, const char* const Hint_2, const char * const Valid_Chars, CChangeObserver* Observ, const char* const Icon)
{
        frameBuffer = CFrameBuffer::getInstance();
        
        name = Head? Head : "";
        value = (char *)Value;
        valueString.clear();
        size =  Size;
        hint_1 = Hint_1? Hint_1 : "";
        hint_2 = Hint_2? Hint_2 : "";
        validchars = Valid_Chars? Valid_Chars : "0123456789. ";
        iconfile = Icon? Icon : "";
        observ = Observ;

        init();
}

CStringInput::~CStringInput() 
{
	if (smstimer)
	{
		g_RCInput->killTimer(smstimer);
		smstimer = 0;
	}
	
	valueString.clear();
}

void CStringInput::init() 
{
	width = (size*20) + 40;

	if (width < 420)
		width = 420;

	int neededWidth;
	neededWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(name); // UTF-8

	if (!(iconfile.empty()))
		neededWidth += 28;
	if (neededWidth + 20 > width)
		width = neededWidth + 20;

	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	
	if ( !(iconfile.empty()))
	{
		int icol_w = 28;
		int icol_h = 16;
		frameBuffer->getIconSize(iconfile.c_str(), &icol_w, &icol_h);
		hheight = std::max(hheight, icol_h + 6);
	}
	
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	iheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getHeight();

	height = hheight + mheight + 50 + 2*iheight;

	// coordinate
	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width) >> 1 );
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height) >> 1 );
	
	smstimer = 0;
	
	selected = 0;
	exit_pressed = false;

	m_cBoxWindow.setPosition(x, y + hheight, width, height - hheight);
	
	// head
	headers.setPosition(x, y, width, hheight);
}

void CStringInput::NormalKeyPressed(const neutrino_msg_t key)
{
	if (CRCInput::isNumeric(key))
	{
		value[selected] = validchars[CRCInput::getNumericValue(key)];

		if (selected < (size - 1))
		{
			selected++;
			paintChar(selected - 1);
		}
		
		paintChar(selected);
	}
}

void CStringInput::keyRedPressed()
{
	if (index(validchars, ' ') != NULL)
	{
		value[selected] = ' ';

		if (selected < (size - 1))
		{
			selected++;
			paintChar(selected - 1);
		}
  
		paintChar(selected);
	}
}

void CStringInput::keyYellowPressed()
{
	selected = 0;

	for(int i = 0 ; i < size ; i++)
	{
		value[i] = ' ';
		paintChar(i);
	}
}

void CStringInput::keyBluePressed()
{
	if (((value[selected] | 32) >= 'a') && ((value[selected] | 32) <= 'z'))
	{
		char newValue = value[selected] ^ 32;
		
		if (index(validchars, newValue) != NULL) 
		{
			value[selected] = newValue;
			paintChar(selected);
		}
	}
}

void CStringInput::keyUpPressed()
{
	int npos = 0;

	for(int count = 0; count < (int)strlen(validchars);count++)
		if(value[selected] == validchars[count])
			npos = count;
	npos++;
	if(npos >= (int)strlen(validchars))
		npos = 0;

	value[selected] = validchars[npos];

	paintChar(selected);
}

void CStringInput::keyDownPressed()
{
	int npos = 0;
	for(int count = 0; count < (int)strlen(validchars); count++)
		if(value[selected] == validchars[count])
			npos = count;

	npos--;
	if(npos<0)
		npos = strlen(validchars)-1;

	value[selected] = validchars[npos];
	paintChar(selected);
}

void CStringInput::keyLeftPressed()
{
	int old = selected;
	if(selected > 0) 
	{
		selected--;
	} 
	else 
	{
		selected = size - 1;
	}
	
	paintChar(old);
	paintChar(selected);
}

void CStringInput::keyRightPressed()
{
	int old = selected;
	
	if (selected < (size - 1)) 
	{
		selected++;
	} 
	else
		selected = 0;

	paintChar(old);
	paintChar(selected);
}

void CStringInput::keyMinusPressed()
{
	int item = selected;
	
	while (item < (size -1))
	{
		value[item] = value[item+1];
		paintChar(item);
		item++;
	}
	
	value[item] = ' ';
	paintChar(item);
}

void CStringInput::keyPlusPressed()
{
	int item = size -1;
	
	while (item > selected)
	{
		value[item] = value[item - 1];
		paintChar(item);
		item--;
	}
	value[item] = ' ';
	paintChar(item);
}

int CStringInput::exec(CMenuTarget* parent, const std::string& )
{
	dprintf(DEBUG_NORMAL, "CStringInput::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = RETURN_REPAINT;

        char oldval[size + 1], dispval[size + 1];
        oldval[size] = 0;
        dispval[size] = 0;

	if (parent)
		parent->hide();

	for(int count = strlen(value) - 1; count < size - 1; count++)
		strcat(value, " ");
	
	strncpy(oldval, value, size);

	paint();
	CFrameBuffer::getInstance()->blit();

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

	bool loop = true;
	while (loop)
	{
		if ( strncmp(value, dispval, size) != 0)
		{
			CVFD::getInstance()->showMenuText(1, value, selected+1);

			strncpy(dispval, value, size);
		}

		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd, true );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

		if (msg == CRCInput::RC_left)
		{
			keyLeftPressed();
		}
		else if (msg == CRCInput::RC_right)
		{
			keyRightPressed();
		}
		else if (CRCInput::getUnicodeValue(msg) != -1)
		{
			NormalKeyPressed(msg);
		}
		else if (msg == CRCInput::RC_red)
		{
			keyRedPressed();
		}
		else if (msg == CRCInput::RC_yellow)
		{
			keyYellowPressed();
		}
		else if ( (msg == CRCInput::RC_green) && (index(validchars, '.') != NULL))
		{
			value[selected] = '.';

			if (selected < (size - 1))
			{
				selected++;
				paintChar(selected - 1);
			}
  
			paintChar(selected);
		}
		else if (msg == CRCInput::RC_blue)
		{
			keyBluePressed();
		}
		else if (msg == CRCInput::RC_up)
		{
			keyUpPressed();
		}
		else if (msg == CRCInput::RC_down)
		{
			keyDownPressed();
		} 
		else if (msg == CRCInput::RC_plus)
		{
			keyPlusPressed();
		} 
		else if (msg == CRCInput::RC_minus)
		{
			keyMinusPressed();
		}
		else if ((msg == NeutrinoMessages::EVT_TIMER) && (data == smstimer))
		{
			keyRightPressed();
		}
		else if (msg == CRCInput::RC_ok)
		{
			loop = false;
		}
		else if ( (msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout) )
		{
			if ( ( strcmp(value, oldval) != 0) && (MessageBox(name.c_str(), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
				continue;

			strncpy(value, oldval, size);
			loop = false;
			exit_pressed = true;
			dprintf(DEBUG_NORMAL, "CStringInput::exec: exit_pressed\n");
		}
		else
		{
			int r = handleOthers( msg, data );
			if (r & (messages_return::cancel_all | messages_return::cancel_info))
			{
				res = (r & messages_return::cancel_all) ? RETURN_EXIT_ALL : RETURN_EXIT;
				loop = false;
			}
			else if ( r & messages_return::unhandled )
			{
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = RETURN_EXIT_ALL;
				}
			}
		}

		frameBuffer->blit();	
	}

	hide();

	for(int count = size - 1; count >= 0; count--)
	{
		if((value[count] == ' ') || (value[count] == 0))
		{
			value[count] = 0;
		}
		else
			break;
	}

	value[size] = 0;

	if (value != NULL)
        {
                valueString = value;
        }

        if ( (observ) && (msg == CRCInput::RC_ok) )
        {
                observ->changeNotify(name, value);
        }

	return res;
}

int CStringInput::handleOthers(const neutrino_msg_t /*msg*/, const neutrino_msg_data_t /*data*/)
{
	return messages_return::unhandled;
}

void CStringInput::hide()
{
	dprintf(DEBUG_NORMAL, "CStringInput::hide\n");

	m_cBoxWindow.hide();
	headers.hide();
	
	frameBuffer->blit();
}

const char * CStringInput::getHint1(void)
{
	return hint_1.c_str();
}

void CStringInput::paint()
{
	dprintf(DEBUG_NORMAL, "CStringInput::paint\n");

	// reinit
	m_cBoxWindow.setPosition(x, y + hheight, width, height - hheight);

	//box
	//m_cBoxWindow.enableSaveScreen();
	m_cBoxWindow.setColor(COL_MENUCONTENT_PLUS_0);
	m_cBoxWindow.setCorner(g_settings.Foot_radius, g_settings.Foot_corner);
	m_cBoxWindow.paint();

	// head
	headers.setPosition(x, y, width, hheight);
	headers.setTitle(name.c_str());
	headers.setIcon(iconfile.c_str());
	headers.setCorner(RADIUS_SMALL);
	headers.setGradient(LIGHT2DARK);
//	headers.setLine(false);
	headers.paint();

	if (!hint_1.empty())
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(x + 20, y + hheight + mheight + iheight + 40, width - 20, hint_1, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
		
		if (!hint_2.empty())
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(x + 20, y + hheight + mheight + iheight*2 + 40, width - 20, hint_2, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
	}

	// chars grid
	for (int count = 0; count < size; count++)
		paintChar(count);
}

void CStringInput::paintChar(int pos, const char c)
{
	const int xs = 20;
	int ys = mheight;
	int xpos = x + 20 + pos*xs;
	int ypos = y + hheight + 25;

	char ch[2] = {c, 0};

	uint32_t    color;
	fb_pixel_t bgcolor;
	
	if (pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else
	{
		color   = COL_MENUCONTENT_TEXT_PLUS_0;
		bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	frameBuffer->paintBoxRel(xpos, ypos, xs, ys, COL_MENUCONTENT_PLUS_4);
	frameBuffer->paintBoxRel(xpos + 1, ypos + 1, xs - 2, ys - 2, bgcolor);

	int xfpos = xpos + ((xs- g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(ch))>>1);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(xfpos,ypos+ys, width, ch, color);
}

void CStringInput::paintChar(int pos)
{
	if(pos < (int)strlen(value))
		paintChar(pos, value[pos]);
}

// CStringInputSMS
CStringInputSMS::CStringInputSMS(const char * const Head, const char * const Value, int Size, const char* const Hint_1, const char* const Hint_2, const char * const Valid_Chars, CChangeObserver* Observ, const char * const Icon)
   		: CStringInput(Head, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ, Icon)
{
	initSMS(Valid_Chars);
}

void CStringInputSMS::initSMS(const char * const Valid_Chars)
{
	last_digit = -1;				// no key pressed yet
	const char CharList[10][11] = { "0 -_/()<>=",	// 10 characters
					"1+.,:!?%\\",
					"abc2@ä",
					"def3",
					"ghi4",
					"jkl5",
					"mno6ö",
					"pqrs7ß",
					"tuv8ü",
					"wxyz9" };

	for (int i = 0; i < 10; i++)
	{
		int j = 0;
		
		if (Valid_Chars != NULL)
		{
			for (int k = 0; k < (int) strlen(CharList[i]); k++)
			{
				if (strchr(Valid_Chars, CharList[i][k]) != NULL)
					Chars[i][j++] = CharList[i][k];
			}
		}
		else
		{
			for (int k = 0; k < (int) strlen(CharList[i]); k++)
			{
				Chars[i][j++] = CharList[i][k];
			}
		}
				
		if (j == 0)
			Chars[i][j++] = ' ';	// prevent empty char lists 
		arraySizes[i] = j;
	}

	height += 260;
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height) >> 1 );
}

void CStringInputSMS::NormalKeyPressed(const neutrino_msg_t key)
{
	if (CRCInput::isNumeric(key))
	{
		int numericvalue = CRCInput::getNumericValue(key);
		
		if (last_digit != numericvalue)
		{
			// there is a last key we can shift the cursor one field to the right
			if ((last_digit != -1) && (selected < (size- 1)))
			{
				selected++;
				paintChar(selected - 1);
			}
			keyCounter = 0;
		}
		else
			keyCounter = (keyCounter + 1) % arraySizes[numericvalue];
		
		value[selected] = Chars[numericvalue][keyCounter];
		last_digit = numericvalue;
		paintChar(selected);
		//
		g_RCInput->killTimer(smstimer);
		smstimer = g_RCInput->addTimer(2*1000*1000);
	}
	else
	{
		value[selected] = (char)CRCInput::getUnicodeValue(key);
		keyRedPressed();   // to lower, paintChar
		keyRightPressed(); // last_digit = -1, move to next position
	}
}

void CStringInputSMS::keyRedPressed()		// switch between lower & uppercase
{
	if (((value[selected] | 32) >= 'a') && ((value[selected] | 32) <= 'z'))
	{
		value[selected] ^= 32;
		//
		paintChar(selected);
	}

	//paintChar(selected);
	smstimer = g_RCInput->addTimer(2*1000*1000);
}

void CStringInputSMS::keyYellowPressed()		// clear all
{
	last_digit = -1;
	CStringInput::keyYellowPressed();
}

void CStringInputSMS::keyUpPressed()
{
	last_digit = -1;

	if (selected > 0)
	{
		int lastselected = selected;
		selected = 0;
		paintChar(lastselected);
		paintChar(selected);
	}
}

void CStringInputSMS::keyDownPressed()
{
	last_digit = -1;

	int lastselected = selected;
	
	selected = size - 1;

	while (value[selected] == ' ')
	{
		selected--;
		if (selected < 0)
			break;
	}

	if (selected < (size - 1))
		selected++;
	
	paintChar(lastselected);
	paintChar(selected);
}

void CStringInputSMS::keyLeftPressed()
{
	last_digit = -1;
	CStringInput::keyLeftPressed();
}

void CStringInputSMS::keyRightPressed()
{
	last_digit = -1;
	CStringInput::keyRightPressed();
}

const struct button_label CStringInputSMSButtons[2] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , _("caps / no caps")  },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("clear all") }
};

void CStringInputSMS::paint()
{
	dprintf(DEBUG_NORMAL, "CStringInputSMS::paint\n");

	CStringInput::paint();

	//numeric pad
	int icol_w, icol_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_NUMERIC_PAD, &icol_w, &icol_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_NUMERIC_PAD, x + (width - icol_w)/2, y + hheight + mheight + iheight*3 + 20, COL_MENUCONTENT);

	// foot
	int icon_w, icon_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	int ButtonHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;

	frameBuffer->paintBoxRel(x, y + height - ButtonHeight, width, ButtonHeight, COL_MENUFOOT_PLUS_0, RADIUS_SMALL, CORNER_BOTTOM, DARK2LIGHT);

	// foot buttons
	buttons.setPosition(x, y + height - ButtonHeight, width, ButtonHeight);
	buttons.setButtons(CStringInputSMSButtons, 2);
	buttons.paint();
}

// CPINInput
void CPINInput::paintChar(int pos)
{
	CStringInput::paintChar(pos, (value[pos] == ' ') ? ' ' : '*');
}

int CPINInput::exec( CMenuTarget* parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CPINInput::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	for(int count = strlen(value) - 1; count < size - 1; count++)
		strcat(value, " ");

	paint();
	CFrameBuffer::getInstance()->blit();

	bool loop = true;

	while(loop)
	{
		g_RCInput->getMsg( &msg, &data, 300 );

		if (msg == CRCInput::RC_left)
		{
			keyLeftPressed();
		}
		else if (msg == CRCInput::RC_right)
		{
			keyRightPressed();
		}
		else if (CRCInput::isNumeric(msg))
		{
			int old_selected = selected;
			NormalKeyPressed(msg);
			if ( old_selected == ( size- 1 ) )
				loop = false;
		}
		else if ( (msg == CRCInput::RC_up) || (msg == CRCInput::RC_down) )
		{
			g_RCInput->postMsg( msg, data );
			res = RETURN_EXIT;
			loop = false;
		}
		else if ( (msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_ok) )
		{
			loop = false;
		}
		else
		{
			int r = handleOthers(msg, data);
			if (r & (messages_return::cancel_all | messages_return::cancel_info))
			{
				res = (r & messages_return::cancel_all) ? RETURN_EXIT_ALL : RETURN_EXIT;
				loop = false;
			}
			else if ( r & messages_return::unhandled )
			{
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & ( messages_return::cancel_all | messages_return::cancel_info ) )
				{
					loop = false;
					res = RETURN_EXIT_ALL;
				}
			}
		}

		frameBuffer->blit();
	}

	hide();

	for(int count = size - 1; count >= 0; count--)
	{
		if((value[count] == ' ') || (value[count] == 0))
		{
			value[count] = 0;
		}
		else
			break;
	}

	value[size] = 0;

	if ( (observ) && (msg == CRCInput::RC_ok) )
	{
		observ->changeNotify(name, value);
	}

	return res;
}

//PLPINInput
int CPLPINInput::handleOthers(neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int res = messages_return::unhandled;

	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS )
	{
		// trotzdem handlen
		CNeutrinoApp::getInstance()->handleMsg(msg, data);

		if (data != (neutrino_msg_data_t) fsk)
			res = messages_return::cancel_info;
	}

	return res;
}

const char * CPLPINInput::getHint1(void)
{
	if (fsk == 0x100)
	{
		hint_1 = _("Locked Channel");
		return CStringInput::getHint1();
	}
	else
	{
		sprintf(hint, _("Locked program (from %d years up)"), fsk);
		return hint;
	}
}

int CPLPINInput::exec( CMenuTarget* parent, const std::string & )
{
	dprintf(DEBUG_NORMAL, "CPLPINInput::exec\n");

	fb_pixel_t * pixbuf = new fb_pixel_t[(width + 2 * borderwidth) * (height + 2 * borderwidth)];

	if (pixbuf != NULL)
	{
		frameBuffer->saveScreen(x- borderwidth, y- borderwidth, width+ 2*borderwidth, height + 2*borderwidth, pixbuf);
		
		frameBuffer->blit();
	}

	// clear border
	frameBuffer->paintBackgroundBoxRel(x- borderwidth, y - borderwidth, width + 2*borderwidth, borderwidth);
	frameBuffer->paintBackgroundBoxRel(x - borderwidth, y + height, width + 2*borderwidth, borderwidth);
	frameBuffer->paintBackgroundBoxRel(x - borderwidth, y, borderwidth, height);
	frameBuffer->paintBackgroundBoxRel(x + width, y, borderwidth, height);
	
	frameBuffer->blit();

	int res = CPINInput::exec (parent, "");

	if (pixbuf != NULL)
	{
		frameBuffer->restoreScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);

		frameBuffer->blit();
		
		delete[] pixbuf;//Mismatching allocation and deallocation: pixbuf
	}

	return ( res );
}


