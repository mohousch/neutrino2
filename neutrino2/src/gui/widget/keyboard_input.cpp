/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2014 CoolStream International Ltd

	License: GPLv2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, version 2 of the License.

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

#include <driver/gfx/fontrenderer.h>

#include <driver/rcinput.h>
#include <driver/encoding.h>

#include <driver/gfx/color.h>
#include <driver/gfx/icons.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/messagebox.h>

#include <system/helpers.h>
#include <system/settings.h>

#include <gui/widget/keyboard_input.h>
#include <gui/widget/keyboard_keys.h>


////
CInputString::CInputString(int Size)
{
	len = Size;
	std::string tmp;
	
	for (unsigned i = 0; i < len; i++)
		inputString.push_back(tmp);
}

void CInputString::clear()
{
	for (unsigned i = 0; i < len; i++)
		inputString[i].clear();
}

size_t CInputString::length()
{
	return inputString.size();
}

CInputString &CInputString::operator=(const std::string &str)
{
	clear();
	const char *text = str.c_str();
	
	for (unsigned i = 0; i < inputString.size() && *text; i++, text++)
		inputString[i] = UTF8ToString(text);
		
	return *this;
}

void CInputString::assign(size_t n, char c)
{
	clear();
	
	for (unsigned i = 0; i < n && i < inputString.size(); i++)
		inputString[i] = c;
}

std::string &CInputString::at(size_t pos)
{
	return inputString[pos];
}

std::string &CInputString::getValue()
{
	valueString.clear();
	
	for (unsigned i = 0; i < inputString.size(); i++)
	{
		if (!inputString[i].empty())
			valueString += inputString[i];
	}
	
	valueString = valueString.erase(valueString.find_last_not_of(" ") + 1);
	
	return valueString;
}

const char *CInputString::c_str()
{
	return getValue().c_str();
}

////
CKeyboardInput::CKeyboardInput(const char* const Name, int Size, CChangeObserver *Observ, const char *const Icon, std::string HintText_1, std::string HintText_2)
{
	title = Name? Name : "";
	inputSize = Size;

	iconfile = Icon ? Icon : NEUTRINO_ICON_EDIT;

	observ = Observ;
	hintText_1 = HintText_1;
	hintText_2 = HintText_2;
	inputString = NULL;
	layout = NULL;
	selected = 0;
	caps = 0;
	srow = scol = 0;
	focus = FOCUS_STRING;
	force_saveScreen = false;
	pixBuf = NULL;
	exit_pressed = false;
}

#define BORDER_OFFSET 20
#define INPUT_BORDER 2
#define KEY_FRAME_WIDTH 2 	// keys frame width
#define KEY_BORDER 4 		// spacing between keys

void CKeyboardInput::init()
{
	frameBuffer = CFrameBuffer::getInstance();
	setLayout();

	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	iheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getHeight();
	input_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 2;		// font height + border
	input_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("M") + INPUT_BORDER;// hack font width + border
	offset  = BORDER_OFFSET;
	fheight = hheight;
	iwidth = inputSize*input_w + BORDER_LEFT + BORDER_RIGHT;

	key_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth("M") + 20;
	key_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 10;
	kwidth = key_w * KEY_COLUMNS + (KEY_COLUMNS - 1) * KEY_BORDER + 2 * offset;

	width = std::max(iwidth, kwidth);
	width = std::min(width, (int) frameBuffer->getScreenWidth());

	// auto calc inputSize
	if (!inputSize || (iwidth > width))
	{
		inputSize = (width - 2 * offset) / input_w;
		iwidth = inputSize * input_w + 2 * offset;
	}
	inputString = new CInputString(inputSize);

	int tmp_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(title);
	if (!(iconfile.empty()))
	{
		int icol_w, icol_h;
		frameBuffer->getIconSize(iconfile.c_str(), &icol_w, &icol_h);
		hheight = std::max(hheight, icol_h + (offset / 4));
		tmp_w += icol_w + (offset / 2);
	}
	width = std::max(width, tmp_w + offset);

	bheight = input_h + (key_h + KEY_BORDER) * KEY_ROWS + 3 * offset;

	if (!hintText_1.empty())
	{
		const char *_hint_1 = hintText_1.c_str();
		tmp_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getRenderWidth(_hint_1);
		width = std::max(width, tmp_w + 2 * offset);
		bheight += iheight;
	}
	
	if (!hintText_2.empty())
	{
		const char *_hint_2 = hintText_2.c_str();
		tmp_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->getRenderWidth(_hint_2);
		width = std::max(width, tmp_w + 2 * offset);
		bheight += iheight;
	}
	bheight += offset;

	height = hheight + bheight + fheight;

	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width) >> 1 );
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height) >> 1 );
	
	changed = false;
}

void CKeyboardInput::setLayout()
{
	if (layout != NULL)
		return;

	layout = &keyboards[0];
	keyboard = layout->keys[caps];
	
	for (unsigned i = 0; i < LAYOUT_COUNT; i++)
	{
		if (keyboards[i].locale == g_settings.language)
		{
			layout = &keyboards[i];
			keyboard = layout->keys[caps];
			
			return;
		}
	}
}

void CKeyboardInput::switchLayout()
{
	unsigned i;
	for (i = 0; i < LAYOUT_COUNT; i++)
	{
		if (layout == &keyboards[i])
			break;
	}
	i++;
	if (i >= LAYOUT_COUNT)
		i = 0;
		
	layout = &keyboards[i];
	keyboard = layout->keys[caps];
	
	paintFooter();
	paintKeyboard();
}

void CKeyboardInput::NormalKeyPressed()
{
	if (keyboard[srow][scol].empty())
		return;

	inputString->at(selected) = keyboard[srow][scol];
	
	if (selected < (inputSize - 1))
	{
		selected++;
		paintChar(selected - 1);
	}
	
	paintChar(selected);
	changed = true;
}

void CKeyboardInput::clearString()
{
	selected = 0;
	inputString->assign(inputString->length(), ' ');
	
	for (int i = 0 ; i < inputSize; i++)
		paintChar(i);
		
	changed = true;
}

void CKeyboardInput::switchCaps()
{
	caps = caps ? 0 : 1;
	keyboard = layout->keys[caps];
	
	paintFooter();
	paintKeyboard();
}

void CKeyboardInput::keyUpPressed()
{
	if (focus == FOCUS_KEY)
	{
		int old_row = srow;
		srow--;
		paintKey(old_row, scol);
		
		if (srow < 0)
		{
			focus = FOCUS_STRING;
			paintChar(selected);
		}
		else
		{
			paintKey(srow, scol);
		}
	}
	else
	{
		srow = KEY_ROWS - 1;
		focus = FOCUS_KEY;
		
		paintChar(selected);
		paintKey(srow, scol);
	}
}

void CKeyboardInput::keyDownPressed()
{
	if (focus == FOCUS_KEY)
	{
		int old_row = srow;
		srow++;
		paintKey(old_row, scol);
		
		if (srow >= KEY_ROWS)
		{
			focus = FOCUS_STRING;
			paintChar(selected);
		}
		else
		{
			paintKey(srow, scol);
		}
	}
	else
	{
		srow = 0;
		focus = FOCUS_KEY;
		paintChar(selected);
		paintKey(srow, scol);
	}
}

void CKeyboardInput::keyLeftPressed()
{
	if (focus == FOCUS_KEY)
	{
		int old_col = scol;;
		scol--;
		if (scol < 0)
			scol = KEY_COLUMNS - 1;
		paintKey(srow, old_col);
		paintKey(srow, scol);
	}
	else
	{
		int old = selected;
		if (selected > 0)
			selected--;
		else
			selected = inputSize - 1;

		paintChar(old);
		paintChar(selected);
	}
}

void CKeyboardInput::keyRightPressed()
{
	if (focus == FOCUS_KEY)
	{
		int old_col = scol;;
		scol++;
		if (scol >= KEY_COLUMNS)
			scol = 0;
		paintKey(srow, old_col);
		paintKey(srow, scol);
	}
	else
	{
		int old = selected;
		if (selected < (inputSize - 1))
		{
			selected++;
		}
		else
			selected = 0;

		paintChar(old);
		paintChar(selected);
	}
}

void CKeyboardInput::deleteChar()
{
	int item = selected;
	
	while (item < (inputSize - 1))
	{
		inputString->at(item) = inputString->at(item + 1);
		paintChar(item);
		item++;
	}
	
	inputString->at(item) = ' ';
	paintChar(item);
	changed = true;
}

void CKeyboardInput::keyBackspacePressed(void)
{
	if (selected > 0)
	{
		selected--;
		for (int i = selected; i < inputSize - 1; i++)
		{
			inputString->at(i) = inputString->at(i + 1);
			paintChar(i);
		}
		
		inputString->at(inputSize - 1) = ' ';
		paintChar(inputSize - 1);
		changed = true;
	}
}

void CKeyboardInput::keyDigiPressed(const neutrino_msg_t key)
{
	int old_col = scol;
	int old_srow = srow;
	int digi = CRCInput::getNumericValue(key);
	digi = (digi == 0) ? 10 : digi;
	srow = 0;
	scol = digi;
	
	if (focus == FOCUS_KEY)
		paintKey(old_srow, old_col);

	focus = FOCUS_KEY;
	
	paintKey(srow, scol);
	NormalKeyPressed();
}

void CKeyboardInput::insertChar()
{
	int item = inputSize - 1;
	
	while (item > selected)
	{
		inputString->at(item) = inputString->at(item - 1);
		paintChar(item);
		item--;
	}
	
	inputString->at(item) = ' ';
	paintChar(item);
	changed = true;
}

void CKeyboardInput::enableSaveScreen(bool enable)
{
	force_saveScreen = enable;
	
	if (!enable && pixBuf)
	{
		delete[] pixBuf;
		pixBuf = NULL;
	}
}

int CKeyboardInput::exec(CMenuTarget* parent, const std::string&)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	init();

	std::string oldval = valueString;

	if (pixBuf)
		delete[] pixBuf;
		
	if (!parent || force_saveScreen)
	{
		pixBuf = new fb_pixel_t[(width) * (height)];
		
		if (pixBuf)
			frameBuffer->saveScreen(x, y, width, height, pixBuf);
	}

	paint();
	paintKeyboard();

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu);

	bool loop = true;
	while (loop)
	{
		frameBuffer->blit();
		
		if (changed)
		{
			changed = false;
			CVFD::getInstance()->showMenuText(1, inputString->c_str(), selected + 1);
		}
		
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd, true);

		if (msg <= CRCInput::RC_MaxRC)
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu);

		if (msg == CRCInput::RC_left)
		{
			keyLeftPressed();
		}
		else if (msg == CRCInput::RC_right)
		{
			keyRightPressed();
		}
		else if (msg == CRCInput::RC_up)
		{
			keyUpPressed();
		}
		else if (msg == CRCInput::RC_down)
		{
			keyDownPressed();
		}
		else if (msg == CRCInput::RC_red)
		{
			loop = false;
		}
		else if (msg == CRCInput::RC_green)
		{
			insertChar();
		}
		else if (msg == CRCInput::RC_yellow)
		{
			clearString();
		}
		else if (msg == CRCInput::RC_blue)
		{
			switchCaps();
		}
		else if (msg == CRCInput::RC_rewind)
		{
			keyBackspacePressed();
		}
		else if (msg == CRCInput::RC_ok)
		{
			if (focus == FOCUS_KEY)
				NormalKeyPressed();
		}
		else if (msg == CRCInput::RC_setup)
		{
			switchLayout();
		}
		else if (CRCInput::isNumeric(msg))
		{
			keyDigiPressed(msg);
		}
		else if ((msg == CRCInput::RC_home) || (msg == CRCInput::RC_timeout))
		{
			if ((inputString->getValue() != oldval) && (MessageBox(title.c_str(), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
			{
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu);
				continue;
			}

			*inputString = oldval;
			loop = false;
			exit_pressed = true;
			res = RETURN_REPAINT;
		}
		else
		{
			if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			{
				loop = false;
				res = RETURN_EXIT_ALL;
			}
		}
	}

	if (pixBuf)
	{
		frameBuffer->restoreScreen(x, y, width, height, pixBuf);
		delete[] pixBuf;
		pixBuf = NULL;
		frameBuffer->blit();
	}
	else
		hide();

	valueString = inputString->getValue();

	if (inputString)
	{
		delete inputString;
		inputString = NULL;
	}

	if (msg == CRCInput::RC_red)
	{
		if (observ)
			observ->changeNotify(title, (void *) valueString.c_str());
	}

	return res;
}

void CKeyboardInput::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
	frameBuffer->blit();
}

int CKeyboardInput::paintFooter()
{
	struct button_label footerButtons[] =
	{
		{ NEUTRINO_ICON_BUTTON_RED, _("Save"), COL_RED_PLUS_0 },
		{ NEUTRINO_ICON_BUTTON_GREEN, _("Insert"), COL_GREEN_PLUS_0 },
		{ NEUTRINO_ICON_BUTTON_YELLOW, _("Clear"), COL_YELLOW_PLUS_0 },
		{ NEUTRINO_ICON_BUTTON_BLUE, caps? _("No Caps") : _("Caps"), COL_BLUE_PLUS_0 },
		{ NEUTRINO_ICON_BUTTON_REW_SMALL, _("Space"), COL_RED_PLUS_0 },
		{ NEUTRINO_ICON_BUTTON_SETUP, _(layout->name.c_str()), COL_RED_PLUS_0 }
	};
	
	int cnt = (sizeof(footerButtons) / sizeof(struct button_label));
	
	CCFooters buttons(x, y + hheight + bheight, width, fheight);
		
	buttons.setButtons(footerButtons, cnt);
	buttons.paint();
}

void CKeyboardInput::paint()
{
	frameBuffer->paintBoxRel(x, y + hheight, width, bheight, COL_MENUCONTENT_PLUS_0);

	CCHeaders header(x, y, width, hheight, title.c_str(), iconfile.c_str());
	header.paint();

	key_y = y + hheight + offset + input_h + offset;

	if (!hintText_1.empty())
	{
		key_y += iheight;
		const char *_hint_1 = hintText_1.c_str();
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(x + offset, key_y, width - 2 * offset, _hint_1, COL_MENUCONTENT_TEXT);
	}
	
	if (!hintText_2.empty())
	{
		key_y += iheight;
		const char *_hint_2 = hintText_2.c_str();
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_INFO]->RenderString(x + offset, key_y, width - 2 * offset, _hint_2, COL_MENUCONTENT_TEXT);
	}
	key_y += offset;

	for (int count = 0; count < inputSize; count++)
		paintChar(count);

	paintFooter();
}

void CKeyboardInput::paintChar(int pos)
{
	if (pos < (int) inputString->length())
		paintChar(pos, inputString->at(pos));
}

void CKeyboardInput::paintChar(int pos, std::string &c)
{
	int xpos = x + offset + (width - iwidth) / 2 + pos * input_w;
	int ypos = y + hheight + offset;

	fb_pixel_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (pos == selected)
	{
		color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	frameBuffer->paintBoxRel(xpos, ypos, input_w, input_h, bgcolor);
	frameBuffer->paintFrameBox(xpos, ypos, input_w, input_h, COL_MENUCONTENT_PLUS_2);

	int ch_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(c);
	int ch_x = xpos + std::max(input_w / 2 - ch_w / 2, 0);
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(ch_x, ypos + input_h, ch_w, c, color);
}

void CKeyboardInput::paintKeyboard()
{
	for (int i = 0; i < KEY_ROWS; i++)
	{
		for (int j = 0; j < KEY_COLUMNS; j++)
			paintKey(i, j);
	}
}

void CKeyboardInput::paintKey(int row, int column)
{
	int xpos = x + offset + (width - kwidth) / 2 + (key_w + KEY_BORDER) * column;
	int ypos = key_y + (key_h + KEY_BORDER) * row;

	int i_selected = (focus == FOCUS_KEY && row == srow && column == scol);

	fb_pixel_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;
	
	if (i_selected)
	{
		color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	int radius = RADIUS_SMALL;
	frameBuffer->paintBoxRel(xpos, ypos, key_w, key_h, bgcolor, radius);
	frameBuffer->paintFrameBox(xpos, ypos, key_w, key_h, COL_MENUCONTENT_PLUS_2);

	if (keyboard[row][column].empty())
		return;

	std::string &s = keyboard[row][column];
	int ch_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(s);
	int ch_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int ch_x = xpos + key_w / 2 - ch_w / 2;
	int ch_y = ypos + key_h / 2 + ch_h / 2;
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(ch_x, ch_y, ch_w, s, color);
}

