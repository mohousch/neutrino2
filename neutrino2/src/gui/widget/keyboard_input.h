//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: keyboard_input.h 24022025 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Copyright (C) 2014 CoolStream International Ltd
//
//	License: GPLv2
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

#ifndef __keyboard_input__
#define __keyboard_input__

#include <system/localize.h>

#include <string>


#define KEY_ROWS 4
#define KEY_COLUMNS 14

struct keyboard_layout
{
	std::string name;
	std::string locale;
	std::string(*keys)[KEY_ROWS][KEY_COLUMNS];
};

class CInputString
{
	private:
		size_t len;
		std::vector<std::string> inputString;
		std::string valueString;

	public:
		CInputString(int Size);
		size_t length();
		void clear();
		std::string &at(size_t pos);
		void assign(size_t n, char c);
		const char *c_str();
		CInputString &operator=(const std::string &str);
		std::string &getValue();
};

////
class CKeyboardInput : public CWidgetTarget
{
	protected:
		CFrameBuffer	*frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight; // head font height
		int iheight; // input string height
		int bheight; // body height
		int fheight; // footer height
		int input_h; // input field height
		int input_w; // input field width
		int iwidth;  // input width
		int offset;
		int key_y;

		// keyboard
		int key_w;   // keyboard key width
		int key_h;   // keyboard key height
		int kwidth;  // keyboard width
		int srow, scol;
		
		enum
		{
			FOCUS_KEY,
			FOCUS_STRING
		};
		int focus;
		int caps;
		struct keyboard_layout *layout;
		std::string(*keyboard)[KEY_COLUMNS];
		CInputString *inputString;
		char *value;

		std::string  title;
		std::string hintText_1, hintText_2;
		std::string iconfile;
		int          inputSize;
		int          selected;
		bool	     changed;
		CChangeObserver *observ;
		bool force_saveScreen;
		fb_pixel_t *pixBuf;
		bool exit_pressed;

		void keyDigiPressed(const neutrino_msg_t key);

		void init();

		void paint();
		void paintFooter();
		void paintChar(int pos, std::string &c);
		void paintChar(int pos);
		void paintKeyboard();
		void paintKey(int row, int column);

		void NormalKeyPressed();
		void clearString();
		void switchCaps();
		void keyUpPressed();
		void keyDownPressed();
		void keyLeftPressed();
		void keyRightPressed();
		void insertChar();
		void deleteChar();
		void keyBackspacePressed();
		void switchLayout();
		void setLayout();

	public:
		CKeyboardInput(const char *const Name, const char *Value, int Size = MAX_INPUT_CHARS, std::string HintText_1 = "", std::string HintText_2 = "", CChangeObserver *Observ = NULL, const char *const Icon = NEUTRINO_ICON_EDIT);
		virtual ~CKeyboardInput(){valueString.clear();};

		void hide();
		int exec(CWidgetTarget *parent, const std::string &actionKey);

		void enableSaveScreen(bool enable);
};

#endif

