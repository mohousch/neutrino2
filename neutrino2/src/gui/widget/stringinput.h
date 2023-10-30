/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: stringinput.h 2013/10/12 mohousch Exp $

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


#ifndef __stringinput__
#define __stringinput__

#include <string>

#include <driver/framebuffer.h>

#include <system/localize.h>

#include <gui/widget/icons.h>
#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>

#include <system/settings.h>


class CStringInput : public CMenuTarget
{
	protected:
		CFrameBuffer* frameBuffer;

		int x;
		int y;
		int width;
		int height;
		int hheight; // head font height
		int mheight; // menu font height
		int iheight; // itemheight

		CCWindow m_cBoxWindow;
		CCHeaders headers;
		
		uint32_t smstimer;

		std::string name;
		std::string hint_1;
		std::string hint_2;
		std::string iconfile;
		const char* validchars;
		char* value;
		std::string valueString;
		int size;

		int selected;
		bool exit_pressed;
		CChangeObserver * observ;

		CCButtons buttons;

		virtual void init();
		virtual const char * getHint1(void);

		virtual void paint();
		virtual void paintChar(int pos, char c);
		virtual void paintChar(int pos);

		virtual void NormalKeyPressed(const neutrino_msg_t key);
		virtual void keyRedPressed();
		virtual void keyYellowPressed();
		virtual void keyBluePressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();
		virtual void keyPlusPressed();
		virtual void keyMinusPressed();

		virtual int handleOthers(const neutrino_msg_t msg, const neutrino_msg_data_t data);

	public:	
		CStringInput(const char * const Head, const char * const Value, int Size = 10, const char* const Hint_1 = NULL, const char* const Hint_2 = NULL, const char * const Valid_Chars = NULL, CChangeObserver* Observ = NULL, const char * const Icon = NEUTRINO_ICON_KEYBINDING);
		
		virtual ~CStringInput();

		void hide();
		int exec(CMenuTarget* parent, const std::string &actionKey);

		//
		bool getExitPressed(){return exit_pressed;};
		virtual std::string& getValueString(void) { return valueString; };
};

class CStringInputSMS : public CStringInput
{
		bool capsMode;
		int arraySizes[10];
		char Chars[10][10];					// maximal 9 character in one CharList entry!

		int keyCounter;
		int last_digit;

		virtual void NormalKeyPressed(const neutrino_msg_t key);
		virtual void keyRedPressed();
		virtual void keyYellowPressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();

		virtual void paint();
		void initSMS(const char * const Valid_Chars);

	public:
		CStringInputSMS(const char * const Head, const char * const Value, int Size = MAX_INPUT_CHARS, const char* const Hint_1 = NULL, const char* const Hint_2 = NULL, const char* const Valid_Chars = NULL, CChangeObserver* Observ = NULL, const char * const Icon = NEUTRINO_ICON_KEYBINDING);
};

class CPINInput : public CStringInput
{
	protected:
		virtual void paintChar(int pos);

	public:
		CPINInput(const char * const Head, const char * const Value, int Size = 10, const char* const Hint_1 = NULL, const char* const Hint_2 = NULL, const char * const Valid_Chars = (const char *)"0123456789", CChangeObserver* Observ = NULL) : CStringInput(Head, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ, (char *)NEUTRINO_ICON_LOCK){};

		int exec(CMenuTarget* parent, const std::string& actionKey);
};

class CPLPINInput : public CPINInput
{
	protected:
		int  fsk;
		char hint[100];

		virtual int handleOthers(const neutrino_msg_t msg, const neutrino_msg_data_t data);

		virtual const char * getHint1(void);

	public:
		CPLPINInput(const char * const Name, const char * const Value, int Size = 10, const char* const Hint_2 = NULL, int FSK = 0x100) : CPINInput(Name, Value, Size, NULL, Hint_2) { fsk = FSK; };

		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CPINChangeWidget : public CStringInput
{
	public:
		CPINChangeWidget(const char * const Name, const char * const Value, int Size = 10, const char* const Hint_1 = NULL, const char * const Valid_Chars = (const char *) "0123456789", CChangeObserver* Observ = NULL) : CStringInput(Name, Value, Size, Hint_1, NULL, Valid_Chars, Observ){};
};

#endif


