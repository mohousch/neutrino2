//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: listbox.cpp 07022025 mohousch Exp $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h> //acces
#include <cctype>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>
#include <gui/widget/listbox.h>
#include <gui/widget/textbox.h>
#include <gui/widget/stringinput.h>

#include <driver/gdi/color.h>
#include <driver/gdi/fontrenderer.h>

#include <driver/rcinput.h>

#include <system/debug.h>


//// CMenuItem
CMenuItem::CMenuItem()
{
	x = -1;
	y = 0;
	dx = 0;
	dy = 0;
	menuItem_type = -1;
	directKey = CRCInput::RC_nokey;
	msg = 0;
	iconName = "";
	can_arrow = false;
	itemIcon = "";
	itemName = "";
	option = "";
	optionInfo = "";
	itemHint = " ";
	icon1 = "";
	icon2 = "";

	number = 0;
	runningPercent = 0;
	pb = false;

	nameFont = SNeutrinoSettings::FONT_TYPE_MENU;
	optionFont = SNeutrinoSettings::FONT_TYPE_MENU; //SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER;
	optionFontColor = COL_MENUCONTENT_TEXT_PLUS_0;
	optionHAlign = CComponent::CC_ALIGN_LEFT;
			
	//
	nLinesItem = false;
	widgetType = ClistBox::TYPE_STANDARD;
	widgetMode = ClistBox::MODE_LISTBOX;
	isPlugin = false;
	paintIconName = true;

	//
	active = true;
	marked = false;
	hidden = false;
	locked = false;
	state = ITEM_ACTIVE;
	
	//
	AlwaysAsk = false;

	jumpTarget = NULL;
	actionKey = "";
	
	paintFrame = true;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_MENUCONTENT_PLUS_6;
	itemGradient = NOGRADIENT;
	
	//
	parent = NULL;
	
	//
	background = NULL;
	
	//
	observ = NULL;
	pulldown = false;
}

CMenuItem::~CMenuItem()
{
	if (background)
	{
		delete [] background;
		background = NULL;
	}
			
	option.clear();
	itemHint.clear();
	itemIcon.clear();
}

void CMenuItem::init(const int X, const int Y, const int DX, const int DY)
{
	x = X;
	y = Y;
	dx = DX;
	dy = DY;
}

void CMenuItem::setActive(const bool Active)
{
	active = Active;
	
	if (x != -1)
		paint();
}

void CMenuItem::setMarked(const bool Marked)
{
	marked = Marked;
	
	if (x != -1)
		paint();
}

void CMenuItem::setHidden(const bool Hidden)
{
	hidden = Hidden;
	
	if (x != -1)
	{
		if (parent)
			parent->initFrames();
			
		paint();
	}
}

void CMenuItem::setLocked(const bool Locked)
{ 
	locked = Locked; 
	AlwaysAsk = true; 
	validPIN = g_settings.parentallock_pincode? g_settings.parentallock_pincode : (char *)"";
}

void CMenuItem::setState(int st)
{
	state = st;
	
	switch (st)
	{
		case ITEM_LOCKED:
			setLocked(true);
			break;
			
		case ITEM_HIDDEN:
			setHidden(true);
			break;
			
		case ITEM_MARKED:
			setMarked(true);
			break;
			
		case ITEM_INACTIVE:
			setActive(false);
			break;
			
		case ITEM_ACTIVE:
		default:
			setActive(true);
			break;
	}
}

// locked
bool CMenuItem::check()
{
	char cPIN[5];
	std::string hint = " ";
	
	do
	{
		cPIN[0] = 0;
		CPINInput * PINInput = new CPINInput(_("Youth protection"), cPIN, 4, hint.c_str());
		PINInput->exec(parent->parent, "");
		delete PINInput;
		hint = _("PIN-Code was wrong! Try again.");
	} while ((strncmp(cPIN, validPIN, 4) != 0) && (cPIN[0] != 0));
	
	return ( strncmp(cPIN, validPIN, 4) == 0);
}

void CMenuItem::saveScreen()
{
	if (background)
	{
			delete [] background;
			background = NULL;
	}
								
	background = new fb_pixel_t[dx*dy];
						
	if (background)
	{
		CFrameBuffer::getInstance()->saveScreen(x, y, dx, dy, background);
	}
}

void CMenuItem::restoreScreen()
{
	if (background)
	{
		CFrameBuffer::getInstance()->restoreScreen(x, y, dx, dy, background);
		
		// 
		delete [] background;
		background = NULL;
	}
}

void CMenuItem::paintItemBox(fb_pixel_t col)
{
	if (!paintFrame)
	{
		saveScreen();
	}	
	
	if (parent && parent->inFocus)
	{	
		// border
		CFrameBuffer::getInstance()->paintBoxRel(x, y, dx, dy, borderColor);
					
		// itemBox
		if (borderMode == CComponent::BORDER_NO)
		{
			CFrameBuffer::getInstance()->paintBoxRel(x, y, dx, dy, col, NO_RADIUS, CORNER_NONE, itemGradient); 
		}
		else if (borderMode == CComponent::BORDER_ALL)
		{
			CFrameBuffer::getInstance()->paintBoxRel(x + 2, y + 2, dx - 4, dy - 4, col, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == CComponent::BORDER_LEFTRIGHT)
		{
			CFrameBuffer::getInstance()->paintBoxRel(x + 2, y, dx - 4, dy, col, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == CComponent::BORDER_TOPBOTTOM)
		{
			CFrameBuffer::getInstance()->paintBoxRel(x, y + 2, dx, dy - 4, col, NO_RADIUS, CORNER_NONE, itemGradient);
		}
	}
}

void CMenuItem::refreshItemBox(fb_pixel_t col)
{
	if (paintFrame || marked)
		CFrameBuffer::getInstance()->paintBoxRel(x, y, dx, dy, col);
	else
	{
		restoreScreen();
	}
}

void CMenuItem::paintItemSlider(const bool select_mode, const int &item_height, const int &optionvalue, const int &factor, const char *left_text, const char *right_text)
{
	// bar
	int bar_width = 120, bar_height = 12;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_VOLUMEBODY, &bar_width, &bar_height);
	
	// get sliderbody width
	int slider_width = 16, slider_height = 16;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_VOLUMESLIDER2ALPHA, &slider_width, &slider_height);

	// avoid division by zero
	if (factor < optionvalue || factor < 1)
		return;

	int right_needed = 0;
	
	if (right_text != NULL)
	{
		right_needed = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("U999");
	}
	
	int left_needed = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(left_text));

	int space = dx - right_needed - ICON_OFFSET - left_needed - ICON_OFFSET;
	
	if (space < bar_width)
		return ;

	int start_x = x + dx - right_needed - bar_width - 2*ICON_OFFSET;

	// FIXME: negative optionvalues falsifies the slider on the right side
	int optionV = (optionvalue < 0) ? 0 : optionvalue;
	
	// paint sliderbody
	CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_VOLUMEBODY, start_x, y, item_height, bar_width + slider_width, bar_height);

	// paint slider
	CFrameBuffer::getInstance()->paintIcon(select_mode ? NEUTRINO_ICON_VOLUMESLIDER2ALPHA : NEUTRINO_ICON_VOLUMESLIDER2, start_x + (optionV * bar_width / factor), y, item_height);
}

//// CMenuOptionChooser
CMenuOptionChooser::CMenuOptionChooser(const char * const Name, int* const OptionValue, const struct keyval *const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver* const Observ, const neutrino_msg_t DirectKey, const char *const IconName, bool Pulldown, const bool useOnOffIcon)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	itemName = Name? Name : "";
	
	//
	if (Number_Of_Options)
	{
		for (unsigned int i = 0; i < (unsigned int)Number_Of_Options; i++)
		{
			options.push_back(Options[i]);
		}
	}
	
	active = Active;
	optionValue = OptionValue;
	number_of_options = Number_Of_Options;
	observ = Observ;
	directKey = DirectKey;
	iconName = IconName ? IconName : "";
	can_arrow = true;
	pulldown = Pulldown;
	onofficon = useOnOffIcon;

	menuItem_type = MENUITEM_OPTIONCHOOSER;
}

void CMenuOptionChooser::addOption(const char *optionname, const int optionvalue)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionChooser::addOption: %s %d\n", optionname, optionvalue);
	
	keyval_struct option;
	
	option.valname = optionname;
	option.key = optionvalue;
	
	options.push_back(option);
	
	number_of_options++;
}

int CMenuOptionChooser::exec(CMenuTarget*)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionChooser::exec: (%s)\n", itemName.c_str());
	
	int ret = CMenuTarget::RETURN_NONE; // FIXME
	bool wantsRepaint = false;
	
	//
	if (locked)
	{
		if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		{
			if (!check())
			{
				return CMenuTarget::RETURN_REPAINT;
			}
		}
		
		locked = false;
	}

	// pulldown
	if( (msg == CRCInput::RC_ok) && pulldown) 
	{	
		int select = -1;

		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;

		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0};	
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionchooser");
		
		if (widget)
		{
			widget->move(parent->getWindowsPos().iX + parent->getWindowsPos().iWidth/2, parent->getWindowsPos().iY + 50);
			widget->enableSaveScreen();
			
			menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
			
			widget->setTitle(itemName.c_str());
		}
		else
		{
			if (parent)
				widget = new CWidget(parent->getWindowsPos().iX + parent->getWindowsPos().iWidth/2, parent->getWindowsPos().iY + 50, 450, 400);
			else
				widget = new CWidget(340, 60, 600,600);
			widget->name = "optionchooser";
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			menu->paintMainFrame(true);
			menu->enablePaintHead();
			if (parent) menu->setHeadColor(COL_MENUCONTENT_PLUS_0);
			if (parent) menu->setHeadGradient(NOGRADIENT);
			menu->setTitle(itemName.c_str());
			menu->enablePaintFoot();
			if (parent) menu->setFootColor(COL_MENUCONTENT_PLUS_0);
			if (parent) menu->setFootGradient(NOGRADIENT);
			menu->setFootButtons(&btn);
			if (parent) menu->setBorderMode();
			
			//
			widget->addCCItem(menu);
		}
		
		//
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			bool selected = false;
			const char *l_option;
			
			if (options[count].key == (*optionValue))
				selected = true;

			if(options[count].valname != 0)
				l_option = options[count].valname;
			
			menu->addItem(new CMenuForwarder(_(l_option)), selected);
		}
		
		ret = widget->exec(NULL, "");

		select = menu->getSelected();
		
		if(select >= 0) 
			*optionValue = options[select].key;
			
		if (widget)
		{
			delete widget;
			widget = NULL;
		}
	}
	else if(msg == CRCInput::RC_ok) 
	{
		if (parent && parent->parent)
			parent->parent->hide();
			
		int select = -1;

		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionchooser");
		
		if (widget)
		{
			menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
			
			widget->setTitle(itemName.c_str());
		}
		else
		{
			const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0};	
			
			//
			CBox box;
			box.iWidth = MENU_WIDTH;
			box.iHeight = MENU_HEIGHT;
			box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
			box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
			widget = new CWidget(&box);
			widget->name = "optionchooser";
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(&box);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			menu->paintMainFrame(true);
			menu->enablePaintHead();
			menu->setTitle(itemName.c_str());
			menu->enablePaintFoot();
			menu->setFootButtons(&btn);
			
			//
			widget->addCCItem(menu);
		}

		//
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			bool selected = false;
			const char *l_option;
			
			if (options[count].key == (*optionValue))
				selected = true;

			if(options[count].valname != 0)
				l_option = options[count].valname;
			
			menu->addItem(new CMenuForwarder(_(l_option)), selected);
		}
		
		ret = widget->exec(NULL, "");

		select = menu->getSelected();
		
		if(select >= 0) 
			*optionValue = options[select].key;
			
		if (widget)
		{
			delete widget;
			widget = NULL;
		}
	} 
	else 
	{
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			if ( options[count].key == (*optionValue) ) 
			{
				if( msg == CRCInput::RC_left ) 
				{
					if(count > 0)
						*optionValue = options[(count - 1) % number_of_options].key;
					else
						*optionValue = options[number_of_options - 1].key;
				} 
				else // RC_right
					*optionValue = options[(count + 1) % number_of_options].key;
				
				break;
			}
		}
	}
	
	if(observ)
		wantsRepaint = observ->changeNotify(itemName, optionValue);
		
	//
	if (wantsRepaint || !paintFrame)
		ret = CMenuTarget::RETURN_REPAINT;
		
	paint(true, true);

	return ret;
}

int CMenuOptionChooser::paint(bool selected, bool AfterPulldown)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionChooser::paint\n");
	
	if (hidden)
		return y;

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		
		//FIXME:
		if (parent)
		{
			bgcolor = parent->inFocus? COL_MENUCONTENTSELECTED_PLUS_0: COL_MENUCONTENTINACTIVE_PLUS_0;
		}
		else
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active)
	{
		color = COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		paintItemBox(bgcolor);
	}
	else
	{
		refreshItemBox(bgcolor);
	}

	//
	std::string l_option = " ";

	for(unsigned int count = 0 ; count < number_of_options; count++) 
	{
		if (options[count].key == *optionValue) 
		{
			if(options[count].valname != 0)
				l_option = options[count].valname;
			break;
		}
	}
	

	// paint icon (left)
	int icon_w = 0;
	int icon_h = 0;
	
	if (!(iconName.empty()))
	{
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
			icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, icon_w, icon_h);	
	}
	else if (CRCInput::isNumeric(directKey))
	{
		// define icon name depends of numeric value
		char i_name[6]; // X +'\0'
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; // even if snprintf truncated the string, ensure termination
		iconName = i_name;
		
		if (!iconName.empty())
		{
			frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
				icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
			
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, icon_w, icon_h);
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + BORDER_LEFT, y + height, height, _(CRCInput::getKeyName(directKey).c_str()), color, height);
        }

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_option.c_str()), true); // FIXME: i18n
	int stringstartposName = x + BORDER_LEFT + icon_w + ICON_OFFSET;
	int stringstartposOption = x + dx - (stringwidth + BORDER_RIGHT); //

	// locale
	const char * l_name = itemName.c_str();
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x), _(l_name), color, 0, true); // UTF-8
	
	// option
	if (onofficon && number_of_options == 2)
	{
		if ( l_option == "on" || l_option == "yes" )
		{
			setIcon1(NEUTRINO_ICON_BUTTON_OPTION_ON_ACTIVE);
		}
		else
		{
			setIcon1(NEUTRINO_ICON_BUTTON_OPTION_OFF_ACTIVE);
		}
		
		int icon1_w, icon1_h;
		
		frameBuffer->getIconSize(icon1.c_str(), &icon1_w, &icon1_h);
			
		//
		frameBuffer->paintIcon(icon1, x + dx - BORDER_LEFT - icon1_w, y, height, icon1_w, icon1_h);
	}
	else
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_LEFT - (stringstartposOption - x), _(l_option.c_str()), color, 0, true); // FIXME: i18n

	// vfd
	if (selected && !AfterPulldown)
	{ 
		char str[256];
		snprintf(str, 255, "%s %s", l_name, l_option.c_str());

		CLCD::getInstance()->showMenuText(0, str, -1, true);
	}

	return y + height;
}

//// CMenuOptionNumberChooser
CMenuOptionNumberChooser::CMenuOptionNumberChooser(const char * const Name, int * const OptionValue, const bool Active, const int min_value, const int max_value, CChangeObserver * const Observ, const int print_offset, const int special_value, bool paintSlider)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	itemName  = Name;
	
	active = Active;
	optionValue = OptionValue;

	lower_bound = min_value;
	upper_bound = max_value;

	display_offset = print_offset;
	unwanted_value = special_value;
	
	can_arrow = true;
	observ = Observ;
	
	paint_slider = paintSlider;

	menuItem_type = MENUITEM_OPTIONNUMBERCHOOSER;
}

int CMenuOptionNumberChooser::exec(CMenuTarget*)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionNumberChooser::exec: (%s)\n", itemName.c_str());
	
	int ret = CMenuTarget::RETURN_NONE; // FIXME
	bool wantsRepaint = false;
	
	//
	if (locked)
	{
		if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		{
			if (!check())
			{
				return CMenuTarget::RETURN_REPAINT;
			}
		}
		
		locked = false;
	}

	//
	if( msg == CRCInput::RC_left ) 
	{
		(*optionValue)--;
		
		if ((*optionValue) < lower_bound)
			*optionValue = lower_bound;
	} 
	else if (msg == CRCInput::RC_right)
	{
		(*optionValue)++;
		
		if ((*optionValue) > upper_bound)
			*optionValue = upper_bound;
	}
	
	if(observ)
		wantsRepaint = observ->changeNotify(itemName, optionValue);
		
	if (wantsRepaint || !paintFrame)
		ret = CMenuTarget::RETURN_REPAINT;
		
	paint(true);

	return ret;
}

int CMenuOptionNumberChooser::paint(bool selected, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionNumberChooser::paint\n");
	
	if (hidden)
		return y;

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		
		//FIXME:
		if (parent)
		{
			bgcolor = parent->inFocus? COL_MENUCONTENTSELECTED_PLUS_0: COL_MENUCONTENTINACTIVE_PLUS_0;
		}
		else
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active)
	{
		color = COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		paintItemBox(bgcolor);
	}
	else
	{
		refreshItemBox(bgcolor);
	}

	// option
	std::string l_option = " ";
	char option_value[11];

	if ( ((*optionValue) != unwanted_value) )
	{
		sprintf(option_value, "%d", ((*optionValue) + display_offset));
		l_option = option_value;
		
	}

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_option.c_str()), true); // UTF-8
	int stringstartposName = x + BORDER_LEFT + ICON_OFFSET;
	int stringstartposOption = x + dx - stringwidth - BORDER_RIGHT; //

	// locale
	const char * l_name;
	
	l_name = itemName.c_str();

	// locale
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x), _(l_name), color, 0, true); // UTF-8
	
	// option value
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_LEFT - (stringstartposOption - x), _(l_option.c_str()), color, 0, true); // UTF-8

	// slider
	if (paint_slider)
		paintItemSlider(selected, height, *optionValue, (upper_bound - lower_bound), l_name, l_option.c_str());
	
	// vfd
	if(selected)
	{ 
		char str[256];
		snprintf(str, 255, "%s %s", l_name, option_value);

		CLCD::getInstance()->showMenuText(0, str, -1, true); 
	}

	return y + height;
}

//// CMenuOptionStringChooser
CMenuOptionStringChooser::CMenuOptionStringChooser(const char * const Name, char * OptionValue, bool Active, CChangeObserver* Observ, const neutrino_msg_t DirectKey, const char *const IconName, bool Pulldown)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	itemName = Name? Name : "";
	active = Active;
	optionStringValue = OptionValue;
	observ = Observ;

	directKey = DirectKey;
	iconName = IconName ? IconName : "";
	can_arrow = true;
	
	pulldown = Pulldown;

	menuItem_type = MENUITEM_OPTIONSTRINGCHOOSER;
}

CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	options.clear();
}

void CMenuOptionStringChooser::addOption(const char * optionname, const int optionvalue)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionStringChooser::addOption: %s\n", optionname? optionname : "null");
	
	options.push_back(std::string(optionname));
}

int CMenuOptionStringChooser::exec(CMenuTarget *)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionStringChooser::exec: (%s) options:%d\n", itemName.c_str(), (int)options.size());
	
	int ret = CMenuTarget::RETURN_NONE; // FIXME
	bool wantsRepaint = false;
	
	//
	if (locked)
	{
		if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		{
			if (!check())
			{
				return CMenuTarget::RETURN_REPAINT;
			}
		}
		
		locked = false;
	}

	// pulldown
	if( (msg == CRCInput::RC_ok) && pulldown) 
	{
		int select = -1;
		
		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0};	

		
		widget = CNeutrinoApp::getInstance()->getWidget("optionstringchooser");
		
		if (widget)
		{
			menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
			
			//
			widget->move(parent->getWindowsPos().iX + parent->getWindowsPos().iWidth/2, parent->getWindowsPos().iY + 50);
			widget->enableSaveScreen();
			
			widget->setTitle(itemName.c_str());
			
			
		}
		else
		{
			if (parent)
				widget = new CWidget(parent->getWindowsPos().iX + parent->getWindowsPos().iWidth/2, parent->getWindowsPos().iY + 50, 500, 450);
			else
				widget = new CWidget(340, 60, 600,600);
			widget->name = "optionstringchooser";
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			menu->paintMainFrame(true);
			menu->enablePaintHead();
			if (parent) menu->setHeadColor(COL_MENUCONTENT_PLUS_0);
			if (parent) menu->setHeadGradient(NOGRADIENT);
			menu->setTitle(itemName.c_str());
			menu->enablePaintFoot();
			if (parent) menu->setFootColor(COL_MENUCONTENT_PLUS_0);
			if (parent) menu->setFootGradient(NOGRADIENT);
			menu->setFootButtons(&btn);
			if (parent) menu->setBorderMode();
			
			//
			widget->addCCItem(menu);
		}
		
		//
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			bool selected = false;
			if (strcmp(options[count].c_str(), optionStringValue) == 0)
				selected = true;

			menu->addItem(new CMenuForwarder(_(options[count].c_str())), selected);
		}
		
		ret = widget->exec(NULL, "");

		select = menu->getSelected();
		
		if(select >= 0)
			strcpy(optionStringValue, options[select].c_str());
			
		if (widget)
		{
			delete widget;
			widget = NULL;
		}
	}
	else if(msg == CRCInput::RC_ok) 
	{
		int select = -1;
		
		if (parent && parent->parent)
			parent->parent->hide();
		
		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionstringchooser");
		
		if (widget)
		{
			menu = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
			
			widget->setTitle(itemName.c_str());
		}
		else
		{
			const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0};	
			
			//
			CBox box;
			box.iWidth = MENU_WIDTH;
			box.iHeight = MENU_HEIGHT;
			box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
			box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
			widget = new CWidget(&box);
			widget->name = "optionstringchooser";
			widget->enableSaveScreen();
			
			//
			menu = new ClistBox(&box);

			menu->setWidgetMode(ClistBox::MODE_SETUP);
			menu->paintMainFrame(true);
			menu->enablePaintHead();
			menu->setTitle(itemName.c_str());
			menu->enablePaintFoot();
			menu->setFootButtons(&btn);
			menu->setBorderMode();
			
			//
			widget->addCCItem(menu);
		}
		
		//
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			bool selected = false;
			if (strcmp(options[count].c_str(), optionStringValue) == 0)
				selected = true;

			menu->addItem(new CMenuForwarder(_(options[count].c_str())), selected);
		}
		
		ret = widget->exec(NULL, "");

		select = menu->getSelected();
		
		if(select >= 0)
			strcpy(optionStringValue, options[select].c_str());
			
		if (widget)
		{
			delete widget;
			widget = NULL;
		}
	} 
	else 
	{
		//select next value
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			if (strcmp(options[count].c_str(), optionStringValue) == 0) //FIXME
			{
				if( msg == CRCInput::RC_left ) 
				{
					if(count > 0)
						strcpy(optionStringValue, options[(count - 1) % options.size()].c_str());
					else
						strcpy(optionStringValue, options[options.size() - 1].c_str());
				} 
				else
					strcpy(optionStringValue, options[(count + 1) % options.size()].c_str());
				
				break;
			}
		}
	}
	
	if(observ) 
		wantsRepaint = observ->changeNotify(itemName, optionStringValue);
		
	//
	if (wantsRepaint || !paintFrame)
		ret = CMenuTarget::RETURN_REPAINT;
		
	paint(true, true);

	return ret;
}

int CMenuOptionStringChooser::paint( bool selected, bool afterPulldown)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionStringChooser::paint\n");
	
	if (hidden)
		return y;

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;
	
	if (selected) 
	{
		color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
		
		//FIXME:
		if (parent)
		{
			bgcolor = parent->inFocus? COL_MENUCONTENTSELECTED_PLUS_0: COL_MENUCONTENTINACTIVE_PLUS_0;
		}
		else
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}
	else if (!active) 
	{
		color = COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		paintItemBox(bgcolor);
	}
	else
	{
		refreshItemBox(bgcolor);
	}

	// paint icon
	int icon_w = 0;
	int icon_h = 0;
		
	if (!(iconName.empty()))
	{
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
			icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, icon_w, icon_h);	
	}
	else if (CRCInput::isNumeric(directKey))
	{
		// define icon name depends of numeric value
		char i_name[6]; // X +'\0'
		snprintf(i_name, 6, "%d", CRCInput::getNumericValue(directKey));
		i_name[5] = '\0'; // even if snprintf truncated the string, ensure termination
		iconName = i_name;
		
		if (!iconName.empty())
		{
			frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
			
			if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
				icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
		
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, icon_w, icon_h);	
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + BORDER_LEFT, y + height, height, _(CRCInput::getKeyName(directKey).c_str()), color, height);
        }
        
        // locale text
	const char * l_name;
	
	l_name = itemName.c_str();
	
	int stringstartposName = x + BORDER_LEFT + icon_w + ICON_OFFSET;
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x), _(l_name), color, 0, true); // UTF-8
	
	// option value
	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(optionStringValue), true);
	int stringstartposOption = std::max(stringstartposName + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_name), true) + ICON_OFFSET, x + dx - stringwidth - BORDER_RIGHT); //
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposOption - x), _(optionStringValue), color, 0, true);
	
	if (selected && !afterPulldown)
	{
		char str[256];
		snprintf(str, 255, "%s %s", l_name, optionStringValue);

		CLCD::getInstance()->showMenuText(0, str, -1, true);
	}

	return y + height;
}

//// CMenuSeparator
CMenuSeparator::CMenuSeparator(const int Type, const char * const Text, const bool Gradient)
{
	//
	type = Type;
	itemName = Text? Text : "";
	
	//
	color = COL_MENUCONTENT_PLUS_5;
	gradient = g_settings.sep_gradient;

	menuItem_type = MENUITEM_SEPARATOR;
}

int CMenuSeparator::getHeight(void) const
{
	if (hidden)
		return 0;
	else
		return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;	
}

int CMenuSeparator::getWidth(void) const
{
	return 0;
}

const char * CMenuSeparator::getString(void)
{
	return itemName.c_str();
}

int CMenuSeparator::paint(bool /*selected*/, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "CMenuSeparator::paint:\n");
	
	if (hidden)
		return y;

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	int height = getHeight();

	if(widgetType != ClistBox::TYPE_FRAME)
	{
		// paint item
		if (paintFrame)
			frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_0);

		// string
		if (type & STRING)
		{
			if(!itemName.empty())
			{
				int stringstartposX;

				const char * l_text = getString();
				int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_text), true); // UTF-8

				// 
				if (type & ALIGN_LEFT)
					stringstartposX = x + BORDER_LEFT;
				else if (type & ALIGN_RIGHT)
					stringstartposX = x + dx - stringwidth - BORDER_RIGHT;
				else // ALIGN_CENTER
					stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

				if (paintFrame)
					frameBuffer->paintBoxRel(stringstartposX - 5, y, stringwidth + 10, height, COL_MENUCONTENT_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - (stringstartposX - x) , _(l_text), COL_MENUCONTENTINACTIVE_TEXT_PLUS_0, 0, true); // UTF-8
			}
		}
		
		// line
		if (type & LINE)
		{
			if (type & STRING)
			{
				if(!itemName.empty())
				{
					const char * l_text = getString();
					int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_text), true); // UTF-8

					// 
					if (type & ALIGN_LEFT)
					{
						//
						frameBuffer->paintBoxRel(x + BORDER_LEFT + stringwidth + BORDER_LEFT, y + (height - 2)/2, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, 2, color, 0, CORNER_NONE, gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
					}
					else if (type & ALIGN_RIGHT)
					{
						//
						frameBuffer->paintBoxRel(x + BORDER_LEFT, y + (height - 2)/2, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, 2, color, 0, CORNER_NONE, gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
					}
					else // ALIGN_CENTER
					{
						// left
						frameBuffer->paintBoxRel(x + BORDER_LEFT, y + (height - 2)/2, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_LEFT, 2, color, 0, CORNER_NONE, gradient? DARK2LIGHT : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
						
						// right
						frameBuffer->paintBoxRel(x + (dx + stringwidth)/2 + BORDER_LEFT, y + (height - 2)/2, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_RIGHT, 2, color, 0, CORNER_NONE, gradient? LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
					}
				}
			}
			else
			{
				frameBuffer->paintBoxRel(x + BORDER_LEFT, y + (height - 2)/2, dx - BORDER_LEFT - BORDER_RIGHT, 2, color, 0, CORNER_NONE, gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
			}	
		}
	}

	return y + height;
}

//// CMenuForwarder
CMenuForwarder::CMenuForwarder(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, const neutrino_msg_t DirectKey, const char * const IconName, const char* const ItemIcon, const char* const Hint)
{
	itemName = Text? Text : "";

	option = Option? Option : "";

	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";

	directKey = DirectKey;

	iconName = IconName ? IconName : "";
	itemIcon = ItemIcon? ItemIcon : "";
	itemHint = Hint? Hint : "";
	
	runningPercent = 0;

	menuItem_type = MENUITEM_FORWARDER;
}

CMenuForwarder::~CMenuForwarder()
{
	dprintf(DEBUG_DEBUG, "CMenuForwarder::del (%s)\n", itemName.c_str());
			
	option.clear();
}

int CMenuForwarder::getHeight(void) const
{
	int iw = 0;
	int ih = 0;
	int bpp = 0;
	
	if (hidden)
		return 0;

	if(widgetType == ClistBox::TYPE_FRAME)
	{
		ih = dy;
	}
	else if(widgetType == ClistBox::TYPE_CLASSIC)
	{
		ih = ITEM_ICON_H_MINI;
			
		//
		if(nLinesItem && widgetMode == ClistBox::MODE_LISTBOX)
		{
			ih = ITEM_ICON_H_MINI*2;
		}
	}
	else // standard|extended
	{
		ih = paintIconName? ITEM_ICON_H_MINI/2 : 0;
		
		if (paintIconName && !iconName.empty())
		{
			CFrameBuffer::getInstance()->getIconSize(iconName.c_str(), &iw, &ih);
		
			if ( ih > ITEM_ICON_H_MINI/2)
				ih = ITEM_ICON_H_MINI/2;	
		}
			
		if(nLinesItem)
		{
			ih = ITEM_ICON_H_MINI;
		}
	}

	return std::max(ih, g_Font[nameFont]->getHeight()) + 6;
}

int CMenuForwarder::getWidth(void) const
{
	if(widgetType == ClistBox::TYPE_FRAME)
	{
		return dx;
	}
	else
	{
		int tw = g_Font[nameFont]->getRenderWidth(_(itemName.c_str())); //FIXME:

		return tw;
	}
}

int CMenuForwarder::exec(CMenuTarget *target)
{
	dprintf(DEBUG_DEBUG, "CMenuForwarder::exec: (%s) actionKey: (%s)\n", getName(), actionKey.c_str());

	int ret = CMenuTarget::RETURN_EXIT;
	
	// locked
	if (locked)
	{
		if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
		{
			if (!check())
			{
				return CMenuTarget::RETURN_REPAINT;
			}
		}
		
		locked = false;
	}

	// exec
	if(jumpTarget)
	{
		ret = jumpTarget->exec(target, actionKey);
		
		setOption(jumpTarget->getValueString().c_str());
		
		// 
		if (ret == CMenuTarget::RETURN_REPAINT)
			paint(true);
	}

	return ret;
}

const char * CMenuForwarder::getName(void)
{
	const char * l_name;
	
	l_name = itemName.c_str();
	
	return l_name;
}

const char * CMenuForwarder::getOption(void)
{
	if(!option.empty())
		return option.c_str();
	else
		return NULL;
}

int CMenuForwarder::paint(bool selected, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "CMenuForwarder::paint:\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	int height = getHeight();
	const char * l_text = getName();
	const char * option_text = getOption();	

	uint32_t color = COL_MENUCONTENT_TEXT_PLUS_0;
	fb_pixel_t bgcolor = marked? COL_MENUCONTENTSELECTED_PLUS_1 : COL_MENUCONTENT_PLUS_0;
	
	if (hidden)
		return y;

	if (selected)
	{
		// FIXME:
		if (parent)
		{
			if (parent->inFocus)
			{
				color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
				bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
			}
			else
			{
				color = COL_MENUCONTENT_TEXT_PLUS_0;	
				bgcolor = COL_MENUCONTENT_PLUS_0;
			}
		}
		else
		{
			color = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
	}
	else if (!active)
	{
		color = COL_MENUCONTENTINACTIVE_TEXT_PLUS_0;
	}

	if(widgetType == ClistBox::TYPE_FRAME)
	{		
		//
		if(selected)
		{
			if (!paintFrame)
			{
				if (background)
				{
					delete [] background;
					background = NULL;
				}
									
				background = new fb_pixel_t[dx*dy];
						
				if (background)
				{
					frameBuffer->saveScreen(x, y, dx, dy, background);
				}
			}
				
			if (parent)
			{
				if (parent->inFocus)
				{	
					frameBuffer->paintBoxRel(x, y, dx, dy, bgcolor);

					if(!itemIcon.empty())
						frameBuffer->paintHintIcon(itemIcon, x + 2, y + 2, dx - 4, dy - 4);
				}
				else
				{
					frameBuffer->paintBoxRel(x, y, dx, dy, bgcolor); //FIXME:

					if(!itemIcon.empty())
						frameBuffer->paintHintIcon(itemIcon, x + 4*ICON_OFFSET, y + 4*ICON_OFFSET, dx - 8*ICON_OFFSET, dy - 8*ICON_OFFSET);
				}
			}
			else
			{
				frameBuffer->paintBoxRel(x, y, dx, dy, bgcolor);

				if(!itemIcon.empty())
					frameBuffer->paintHintIcon(itemIcon, x + 2, y + 2, dx - 4, dy - 4);
			}
		}
		else
		{
			// refresh
			if (paintFrame)
			{
				frameBuffer->paintBoxRel(x, y, dx, dy, bgcolor);
			}
			else
			{
				if (background)
				{
					frameBuffer->restoreScreen(x, y, dx, dy, background);
							
					delete [] background;
					background = NULL;
				}
			}
			
			if(!itemIcon.empty())
				frameBuffer->paintHintIcon(itemIcon, x + 4*ICON_OFFSET, y + 4*ICON_OFFSET, dx - 8*ICON_OFFSET, dy - 8*ICON_OFFSET);
		}

		// vfd
		if (selected)
		{
			CLCD::getInstance()->showMenuText(0, l_text, -1, true);
		}

		return 0;
	}
	else // standard|classic|extended
	{
		//
		if (selected)
		{
			paintItemBox(bgcolor);
		}
		else
		{
			refreshItemBox(bgcolor);
		}

		// icon1 (right)
		int icon1_w = 0;
		int icon1_h = 0;
		int icon1_offset = 0;
	
		if (!icon1.empty())
		{
			icon1_offset = ICON_OFFSET; 
			
			//get icon size
			frameBuffer->getIconSize(icon1.c_str(), &icon1_w, &icon1_h);
			
			if (icon1_h >= g_Font[nameFont]->getHeight() + 6)
			{
				icon1_h = g_Font[nameFont]->getHeight() + 4;
				icon1_w = 1.63*icon1_h;
			}
		
			//
			frameBuffer->paintIcon(icon1, x + dx - BORDER_LEFT - icon1_w, y, height, icon1_w, icon1_h);
		}

		// icon2 (right)
		int icon2_w = 0;
		int icon2_h = 0;
		int icon2_offset = 0;
	
		if (!icon2.empty())
		{
			icon2_offset = ICON_OFFSET;
			
			//get icon size
			frameBuffer->getIconSize(icon2.c_str(), &icon2_w, &icon2_h);
			
			if (icon2_h >= g_Font[nameFont]->getHeight() + 6)
			{
				icon2_h = g_Font[nameFont]->getHeight() + 4;
				icon2_w = 1.63*icon2_h;
			}
		
			frameBuffer->paintIcon(icon2, x + dx - BORDER_LEFT - icon1_w -icon1_offset - icon2_w, y , height, icon2_w, icon2_h);
		}

		// optionInfo (right)
		int optionInfo_width = 0;
		int option_info_max = (dx - BORDER_LEFT - BORDER_RIGHT)/3;
	
		if(!optionInfo.empty())
		{
			optionInfo_width = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(_(optionInfo.c_str()));
			
			if (optionInfo_width > option_info_max)
				optionInfo_width = option_info_max;

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + dx - BORDER_RIGHT - icon1_w - icon1_offset - icon2_w -icon2_offset - optionInfo_width, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(), optionInfo_width, _(optionInfo.c_str()), color, 0, true); // UTF-8
		}

		// number
		int number_width = 0;
		int number_offset = 0;
		
		if(number)
		{
			number_offset = 5;
			
			char tmp[10];

			sprintf((char*) tmp, "%d", number);

			number_width = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");

			int numPosX = x + BORDER_LEFT + number_width - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(_(tmp));

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numPosX, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(), number_width + 1, _(tmp), color, 0, true); // UTF-8
		}
		
		// hintIcon / iconName
		int icon_w = 0;
		int icon_h = 0;
		int bpp = 0;
		int icon_offset = 0;

		if(widgetType == ClistBox::TYPE_CLASSIC)
		{
			if (paintIconName && !itemIcon.empty())
			{
				icon_offset = ICON_OFFSET;
				icon_h = ITEM_ICON_H_MINI;
				icon_w = ITEM_ICON_W_MINI;
				
				if(nLinesItem && widgetMode == ClistBox::MODE_LISTBOX)
				{
					icon_h = ITEM_ICON_H_MINI*2;
					icon_w = ITEM_ICON_W_MINI;
				}
			
				frameBuffer->paintHintIcon(itemIcon.c_str(), x + BORDER_LEFT + number_width + number_offset, y + ((height - icon_h)/2), icon_w, icon_h);
			}
		}
		else // standard|extended
		{
			if (paintIconName && !iconName.empty())
			{
				icon_offset = ICON_OFFSET;
				icon_h = g_Font[nameFont]->getHeight() + 4;
				icon_w = 1.63*icon_h;
				
				//get icon size
				frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
					
				// scale iconName
				if (icon_h >= getHeight())
				{
					icon_h = g_Font[nameFont]->getHeight() + 4;
					if (icon_w > 1.63*icon_h)
						icon_w = 1.63*icon_h;
				}
			
				frameBuffer->paintIcon(iconName, x + BORDER_LEFT + number_width + number_offset, y, height, icon_w, icon_h);
			}
		}

		// ProgressBar
		int pb_width = 0;
		int pb_offset = 0;
		
		if(pb)
		{
			pb_offset = ICON_OFFSET;
			
			pb_width = 35;
			int pBarHeight = 4;
			
			int pbPosX = x + BORDER_LEFT + number_width + number_offset + icon_w + icon_offset;

			CCProgressBar timescale(pbPosX, y + (height - pBarHeight)/2, pb_width, pBarHeight);
			timescale.reset();
			timescale.refresh(runningPercent);
		}
	
		// locale / option / option_info
		int l_text_width = l_text != NULL? g_Font[nameFont]->getRenderWidth(_(l_text), true) : 0;
		int l_startPosX = x + BORDER_LEFT + number_width + number_offset + icon_w + icon_offset + pb_width + pb_offset;
		int option_width = option_text != NULL? g_Font[optionFont]->getRenderWidth(_(option_text), true) : 0;
		int option_startPosX = l_startPosX;

		// local
		if(l_text_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width)
			l_text_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width;
			
		if(option_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width)
			option_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width;
			
		if (widgetType == ClistBox::TYPE_CLASSIC)
		{
			if (nLinesItem)
			{
				// local
				if(l_text != NULL)
				{
					g_Font[nameFont]->RenderString(l_startPosX, y + 3 + g_Font[nameFont]->getHeight(), l_text_width, _(l_text), color, 0, true); // UTF-8
				}
				
				// option
				if(option_text != NULL)
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(option_startPosX, y + height, option_width, _(option_text), (selected || !active)? color : optionFontColor, 0, true);
				}
				
				// hint
				if (!itemHint.empty())
				{
					int hint_width = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(_(itemHint.c_str()), true);
					
					if(hint_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width)
						hint_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width;
					
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(option_startPosX, y + height/2, hint_width, _(itemHint.c_str()), (selected || !active)? color : optionFontColor, 0, true);
				}
			}
			else
			{
				// local
				if(l_text != NULL)
				{
					g_Font[nameFont]->RenderString(l_startPosX, y + g_Font[nameFont]->getHeight() + (height - g_Font[nameFont]->getHeight())/2, l_text_width, _(l_text), color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
					if (option_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - l_text_width)
						option_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - l_text_width;
						
					if (optionHAlign == CComponent::CC_ALIGN_LEFT)
						option_startPosX = l_startPosX + l_text_width + ICON_OFFSET;
					else
						option_startPosX = dx - BORDER_RIGHT - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - option_width;
						
					g_Font[optionFont]->RenderString(option_startPosX, y + g_Font[optionFont]->getHeight() + (height - g_Font[optionFont]->getHeight())/2, option_width, _(option_text), (selected || !active)? color : optionFontColor, 0, true);
				}
			}
		}
		else // standard / extended
		{
			if(nLinesItem)
			{
				// local
				if(l_text != NULL)
				{
					g_Font[nameFont]->RenderString(l_startPosX, y + 3 + g_Font[nameFont]->getHeight(), l_text_width, _(l_text), color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
					if(option_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width)
						option_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width;
			
					option_startPosX = l_startPosX;
						
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(option_startPosX, y + height, option_width, _(option_text), (selected || !active)? color : optionFontColor, 0, true);
				}
			}
			else
			{
				// local
				if(l_text != NULL)
				{
					g_Font[nameFont]->RenderString(l_startPosX, y + g_Font[nameFont]->getHeight() + (height - g_Font[nameFont]->getHeight())/2, l_text_width, _(l_text), color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
					if (option_width > dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - l_text_width)
						option_width = dx - BORDER_RIGHT - BORDER_LEFT - number_width - number_offset - pb_width - pb_offset - icon_w - icon_offset - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - l_text_width;
						
					if (optionHAlign == CComponent::CC_ALIGN_LEFT)
						option_startPosX = l_startPosX + l_text_width + ICON_OFFSET;
					else
						option_startPosX = dx - BORDER_RIGHT - icon1_w - icon1_offset - icon2_w - icon2_offset - optionInfo_width - option_width;
						
					if (widgetMode != ClistBox::MODE_SETUP)
					{
						g_Font[optionFont]->RenderString(option_startPosX, y + g_Font[optionFont]->getHeight() + (height - g_Font[optionFont]->getHeight())/2, option_width, _(option_text), (selected || !active)? color : optionFontColor, 0, true);
					}
					else
					{
						int stringwidth = g_Font[optionFont]->getRenderWidth(_(option_text), true);
						
						int stringstartposOption = std::max(x + BORDER_LEFT + icon_w + icon_offset + g_Font[nameFont]->getRenderWidth(_(l_text), true) + ICON_OFFSET, x + dx - (stringwidth + BORDER_RIGHT));

						g_Font[optionFont]->RenderString(stringstartposOption, y + (height - g_Font[optionFont]->getHeight())/2 + g_Font[optionFont]->getHeight(), dx - BORDER_LEFT - BORDER_RIGHT - icon_w - icon_offset - l_text_width, _(option_text), (selected || !active)? color : optionFontColor, 0, true);
					}
				}
			}
		}
		
		// vfd
		if (selected)
		{
			CLCD::getInstance()->showMenuText(0, l_text, -1, true);
		}
		
		return y + height;
	}
}

//// ClistBox
ClistBox::ClistBox(const int x, const int y, const int dx, const int dy)
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = -1;
	current_page = 0;
	pos = 0;

	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	oldPosition = itemBox;
	
	//
	listmaxshow = 0;

	// head
	paint_Head = false;
	has_Title = false;
	hheight = 0;
	hbutton_count	= 0;
	hbutton_labels.clear();
	paintDate = false;
//	l_name = "";
//	iconfile = "";
	thalign = CC_ALIGN_LEFT;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = g_settings.Head_corner;
	headGradient = g_settings.Head_gradient;
	headGradient_direction = GRADIENT_VERTICAL;
	headGradient_intensity = INT_LIGHT;
	headGradient_type = g_settings.Head_gradient_type;
	head_line = g_settings.Head_line;
	head_line_gradient = g_settings.Head_line_gradient;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;

	// foot
	paint_Foot = false;
	fheight = 0;
	fbutton_count	= 0;
	fbutton_labels.clear();
	fbutton_width = itemBox.iWidth - 20;
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = g_settings.Foot_corner;
	footGradient = g_settings.Foot_gradient;
	footGradient_direction = GRADIENT_VERTICAL;
	footGradient_intensity = INT_LIGHT;
	footGradient_type = g_settings.Foot_gradient_type;
	foot_line = g_settings.Foot_line;
	foot_line_gradient = g_settings.Foot_line_gradient;
	
	// iteminfo
	paint_ItemInfo = false;
	footInfoHeight = 0;
	cFrameFootInfoHeight = 0;
	footInfoMode = CCItemInfo::ITEMINFO_HINTITEM;
	itemInfoBox.iX = 0;
	itemInfoBox.iY = 0;
	itemInfoBox.iWidth = 0;
	itemInfoBox.iHeight = 0;
	iteminfoborder = false;
	iteminfosavescreen = false;
	iteminfobordermode = CComponent::BORDER_NO;
	iteminfoframe = true;
	iteminfofont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
	iteminfocolor = COL_MENUCONTENT_PLUS_0;
	iteminfoscale = false;
	itemInfoBox2.iX = 0;
	itemInfoBox2.iY = 0;
	itemInfoBox2.iWidth = 0;
	itemInfoBox2.iHeight = 0;
	
	//
	inFocus = true;
	shrinkMenu = false;
	paintframe = true;

	//
	itemsPerX = 6;
	itemsPerY = 3;
	maxItemsPerPage = itemsPerX*itemsPerY;

	//
	widgetType = TYPE_STANDARD;
	widgetMode = MODE_LISTBOX;

	background = NULL;
	
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	scrollbar = true;
	gradient = NOGRADIENT;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_MENUCONTENT_PLUS_6;
	
	//
	item_height = 0;
	item_width = 0;
	items_width = 0;
	items_height = 0;
	itemBorderMode = CComponent::BORDER_NO;
	itemBorderColor = COL_MENUCONTENT_PLUS_6;
	itemGradient = NOGRADIENT;
	item2Lines = false;
	paintIconName = true;
	
	//
	sec_timer_id = 0;
	adjustToParent = false;
	
	cc_type = CC_LISTBOX;
}

ClistBox::ClistBox(CBox* position)
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = -1;
	current_page = 0;
	pos = 0;

	itemBox = *position;
	oldPosition = itemBox;
	
	//
	listmaxshow = 0;

	// head
	paint_Head = false;
	has_Title = false;
	hheight = 0;
	hbutton_count	= 0;
	hbutton_labels.clear();
	fbutton_count	= 0;
	fbutton_labels.clear();
	fbutton_width = itemBox.iWidth - 20;
	paintDate = false;
//	l_name = "";
//	iconfile = "";
	thalign = CC_ALIGN_LEFT;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = g_settings.Head_corner;
	headGradient = g_settings.Head_gradient;
	headGradient_direction = GRADIENT_VERTICAL;
	headGradient_intensity = INT_LIGHT;
	headGradient_type = g_settings.Head_gradient_type;
	head_line = g_settings.Head_line;
	head_line_gradient = g_settings.Head_line_gradient;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;

	// foot
	paint_Foot = false;
	fheight = 0;
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = g_settings.Foot_corner;
	footGradient = g_settings.Foot_gradient;
	footGradient_direction = GRADIENT_VERTICAL;
	footGradient_intensity = INT_LIGHT;
	footGradient_type = g_settings.Foot_gradient_type;
	foot_line = g_settings.Foot_line;
	foot_line_gradient = g_settings.Foot_line_gradient;
	
	// itemInfo
	paint_ItemInfo = false;
	footInfoHeight = 0;
	cFrameFootInfoHeight = 0;
	footInfoMode = CCItemInfo::ITEMINFO_HINTITEM;
	itemInfoBox.iX = 0;
	itemInfoBox.iY = 0;
	itemInfoBox.iWidth = 0;
	itemInfoBox.iHeight = 0;
	iteminfoborder = false;
	iteminfosavescreen = false;
	iteminfobordermode = CComponent::BORDER_NO;
	iteminfoframe = true;
	iteminfofont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
	iteminfocolor = COL_MENUCONTENT_PLUS_0;
	iteminfoscale = false;
	itemInfoBox2.iX = 0;
	itemInfoBox2.iY = 0;
	itemInfoBox2.iWidth = 0;
	itemInfoBox2.iHeight = 0;

	//
	widgetType = TYPE_STANDARD;
	widgetMode = MODE_LISTBOX;
	
	//
	inFocus = true;
	shrinkMenu = false;
	paintframe = true;

	//
	itemsPerX = 6;
	itemsPerY = 3;
	maxItemsPerPage = itemsPerX*itemsPerY;
	//
	background = NULL;
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	scrollbar = true;
	gradient = NOGRADIENT;
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_MENUCONTENT_PLUS_0;
	
	//
	item_height = 0;
	item_width = 0;
	items_width = 0;
	items_height = 0;
	itemBorderMode = CComponent::BORDER_NO;
	itemBorderColor = COL_MENUCONTENT_PLUS_6;
	itemGradient = NOGRADIENT;
	item2Lines = false;
	paintIconName = true;
	
	//
	sec_timer_id = 0;
	adjustToParent = false;
	
	cc_type = CC_LISTBOX;
}

ClistBox::~ClistBox()
{
	dprintf(DEBUG_INFO, "ClistBox:: del (%s)\n", htitle.c_str());

	//
	if (background)
	{
		delete [] background;
		background = NULL;
	}
	
	//
	if (timer)
	{
		timer->hide();
		delete timer;
		timer = NULL;
	}
	
	//
	hbutton_labels.clear();
	fbutton_labels.clear();
	
	//
	items.clear();
	page_start.clear();
}

void ClistBox::addItem(CMenuItem *menuItem, const bool defaultselected)
{
	if (menuItem != NULL)
	{
		if (defaultselected && (!menuItem->hidden || !menuItem->active))
			selected = items.size();
		
		items.push_back(menuItem);
		menuItem->setParent(this);
	}
}

bool ClistBox::hasItem()
{
	return !items.empty();
}

void ClistBox::initFrames()
{
	dprintf(DEBUG_INFO, "ClistBox::initFrames:\n");
	
	// reinit position
	cFrameFootInfoHeight = 0;
	
	// sanity check
	if(itemBox.iHeight > ((int)frameBuffer->getScreenHeight(true)))
		itemBox.iHeight = frameBuffer->getScreenHeight(true);

	// sanity check
	if(itemBox.iWidth > (int)frameBuffer->getScreenWidth(true))
		itemBox.iWidth = frameBuffer->getScreenWidth(true);

	// widgettype forwarded to item 
	for (unsigned int count = 0; count < items.size(); count++) 
	{
		CMenuItem * item = items[count];

		item->widgetType = widgetType;
		item->widgetMode = widgetMode;
		item->paintFrame = paintframe;
		item->paintIconName = paintIconName;
		////
		if (itemBorderMode) item->setBorderMode(itemBorderMode);
		if (itemBorderColor) item->setBorderColor(itemBorderColor);
		if (itemGradient) item->setGradient(itemGradient);
		if (item2Lines) item->set2lines(item2Lines);

	} 

	// head
	if(paint_Head)
	{
		hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	}
	
	// foot height
	if(paint_Foot)
	{
		fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	}

	// init frames
	if(widgetType == TYPE_FRAME)
	{
		//
		if (paint_ItemInfo)
		{
			cFrameFootInfoHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
		}
		
		//	
		if(fbutton_count == 0) // ???
		{
			fheight = 0;
		}

		//
		page_start.clear();
		page_start.push_back(0);
		total_pages = 1;

		for (unsigned int i = 0; i < items.size(); i++) 
		{
			if(i == maxItemsPerPage*total_pages)
			{
				page_start.push_back(i);
				total_pages++;
			}
		}

		page_start.push_back(items.size());

		//
		item_width = itemBox.iWidth/itemsPerX;
		item_height = (itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20)/itemsPerY; //
		
		//
		items_height = itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20;
		items_width = itemBox.iWidth;	
	}
	else 
	{
		// footInfoBox
		if(paint_ItemInfo)
		{
			if(widgetType == TYPE_STANDARD)
			{
				cFrameFootInfoHeight = footInfoHeight;
			}
		}

		// calculate some values
		int itemHeightTotal = 0;
		item_height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 3;
		int heightCurrPage = 0;
		page_start.clear();
		page_start.push_back(0);
		total_pages = 1;
		int heightFirstPage = 0;
	
		for (unsigned int i = 0; i < items.size(); i++) 
		{
			item_height = items[i]->getHeight();
			itemHeightTotal += item_height;
			heightCurrPage += item_height;

			if((heightCurrPage + hheight + fheight + cFrameFootInfoHeight) > itemBox.iHeight)
			{
				page_start.push_back(i);
				heightFirstPage = std::max(heightCurrPage - item_height, heightFirstPage);
				total_pages++;
				heightCurrPage = item_height;
			}
		}

		heightFirstPage = std::max(heightCurrPage, heightFirstPage);
		page_start.push_back(items.size());

		// recalculate height
		if(shrinkMenu && hasItem())
		{
			if (widgetMode == MODE_LISTBOX)
			{
				listmaxshow = (itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight)/item_height;
				itemBox.iHeight = hheight + listmaxshow*item_height + fheight + cFrameFootInfoHeight;
			}
			else if ( (widgetMode == MODE_MENU) || (widgetMode == MODE_SETUP) )
			{
				itemBox.iHeight = std::min(itemBox.iHeight, hheight + heightFirstPage + fheight + cFrameFootInfoHeight);
			}
			
			//
			itemBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - itemBox.iHeight) >> 1 );
		}
	}
}

void ClistBox::paint(bool _selected)
{
	dprintf(DEBUG_INFO, "ClistBox::paint: (%s)\n", htitle.c_str());

	//
	initFrames();
	
	// calculate items_width / items_height
	if(widgetType == TYPE_FRAME)
	{
		items_height = itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20 - (borderMode? 4 : 0);
		items_width = itemBox.iWidth - (borderMode? 4 : 0);
	}
	else
	{
		items_height = itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - (borderMode? 4 : 0); 

		sb_width = 0;
		
		if(total_pages > 1)
			sb_width = scrollbar? SCROLLBAR_WIDTH : 0;

		items_width = itemBox.iWidth - sb_width - (borderMode? 4 : 0);

		// extended
		if(widgetType == TYPE_EXTENDED)
		{
			items_width = 2*(itemBox.iWidth/3) - sb_width - (borderMode? 4 : 0);			
		}
	}
	
	// paint mainFrame
	if (paintframe)
	{
		if (paint_Head && widgetType != TYPE_FRAME)
		{
			radius |= g_settings.Head_radius;
			corner |= g_settings.Head_corner;
		}
		
		if (paint_Foot && widgetType != TYPE_FRAME)
		{
			radius |= g_settings.Foot_radius;
			corner |= g_settings.Foot_corner;
		}
		
		// border
		if (borderMode != CComponent::BORDER_NO)
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, borderColor, radius, corner);
		
		// mainframe
		frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), itemBox.iHeight - (borderMode? 4 : 0), bgcolor, radius, corner, gradient);
	}
	else
	{
		saveScreen();
	}
	
	//
	if (widgetType == TYPE_EXTENDED)
	{
		itemsLine.setPosition(itemBox.iX + items_width + (itemBox.iWidth - items_width - ITEM_ICON_W)/2, itemBox.iY + (itemBox.iHeight - ITEM_ICON_H)/2, ITEM_ICON_W, ITEM_ICON_H);
		itemsLine.enableSaveScreen();
	}
	else if(itemInfoBox.iWidth != 0)
	{
		itemsLine.setPosition(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
		if (iteminfosavescreen) itemsLine.enableSaveScreen();
	}
	
	// FIXME:						
	if (paint_ItemInfo && itemInfoBox2.iWidth != 0)
	{
		label.setPosition(itemInfoBox2.iX, itemInfoBox2.iY, itemInfoBox2.iWidth, itemInfoBox2.iHeight);
		label.enableSaveScreen();
	}

	//
	paintHead();
	paintFoot();
	
	if (paint_ItemInfo && paint_Foot)
	{
		int iw, ih;
		frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
			
		label.setPosition(itemBox.iX + BORDER_LEFT + iw + ICON_OFFSET, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + 2, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - iw, fheight - 2);
			
		label.enableSaveScreen();
	}
	
	paintItems();
	
	//
	painted = true;
}

void ClistBox::paintItems()
{
	dprintf(DEBUG_INFO, "ClistBox::paintItems:\n");

	if(widgetType == TYPE_FRAME)
	{
		item_start_y = itemBox.iY + hheight + 10;
		
		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}

		// items background
		if (paintframe)
			frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + hheight, items_width, items_height + cFrameFootInfoHeight + 20, bgcolor, radius, corner, gradient);
		else
		{
			restoreScreen();
		}

		// init items
		for (unsigned int i = 0; i < items.size(); i++) 
		{
			CMenuItem * item = items[i];	
			item->init(-1, 0, 0, 0);
		}

		// paint
		int count = (int)page_start[current_page];

		if(items.size() > 0)
		{
			for (int _y = 0; _y < itemsPerY; _y++)
			{
				for (int _x = 0; _x < itemsPerX; _x++)
				{
					CMenuItem * item = items[count];
					
					item->init(itemBox.iX + _x*item_width, item_start_y + _y*item_height, item_width, item_height);

					if((item->isSelectable()) && (selected == -1)) 
					{
						selected = count;
					} 

					if (selected == (signed int)count) 
					{
						paintItemInfo(count);
					}

					item->paint( selected == ((signed int) count));

					count++;

					if ( (count == (int)page_start[current_page + 1]) || (count == (int)items.size()))
					{
						break;
					}
				}

				if ( (count == (int)page_start[current_page + 1]) || (count == (int)items.size()))
				{
					break;
				}		
			}
		}
	}
	else //
	{
		item_start_y = itemBox.iY + hheight + (borderMode? 2 : 0);

		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}

		// paint items background
		if (paintframe)
		{
			frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + hheight + (borderMode? 2 : 0), items_width, items_height, bgcolor, radius, corner, gradient);
		}
		else
		{
			restoreScreen();
		}
		
		//
		if(widgetType == TYPE_EXTENDED)
		{
			CCVline line(itemBox.iX + items_width + sb_width + (borderMode? 2 : 0), item_start_y + 2, 2, items_height - 4);
				
			line.setColor(COL_MENUCONTENT_PLUS_6);
			line.paint(); 

		}
	
		// paint scrollbar
		if(total_pages > 1)
		{
			if (scrollbar)
			{
				if(widgetType == TYPE_EXTENDED)
					scrollBar.paint(itemBox.iX + items_width + (borderMode? 2 : 0), itemBox.iY + hheight + (borderMode? 2 : 0), items_height, total_pages, current_page);
				else
					scrollBar.paint(itemBox.iX + (borderMode? 2 : 0) + items_width, itemBox.iY + hheight + (borderMode? 2 : 0), items_height, total_pages, current_page);
			}
		}

		// paint items
		int ypos = itemBox.iY + hheight + (borderMode? 2 : 0);
		int xpos = itemBox.iX + (borderMode? 2 : 0);
	
		for (unsigned int count = 0; count < items.size(); count++) 
		{
			CMenuItem * item = items[count];

			if ((count >= page_start[current_page]) && (count < page_start[current_page + 1])) 
			{
				//
				item->init(xpos, ypos, items_width, item->getHeight());
				
				//
				if((item->isSelectable()) && (selected == -1)) 
				{
					selected = count;
				} 

				// paint itemInfo
				if (selected == (signed int)count) 
				{
					paintItemInfo(count);
				}

				// paint item
				ypos = item->paint(selected == ((signed int) count));
			} 
			else 
			{
				// x = -1 is a marker which prevents the item from being painted on setActive changes
				item->init(-1, 0, 0, 0);
			}	
		} 
	}
}

void ClistBox::paintHead()
{
	if(paint_Head)
	{
		dprintf(DEBUG_INFO, "ClistBox::paintHead:\n");
		
		if(widgetType == TYPE_FRAME)
		{
			// box
			if (paintframe)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), hheight, COL_MENUCONTENT_PLUS_0);

			// line
			if (head_line)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + hheight - 2 + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, head_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

			// icon
			int i_w = 0;
			int i_h = 0;

			frameBuffer->getIconSize(hicon.c_str(), &i_w, &i_h);
			
			if(i_h >= hheight)
			{
				i_h = hheight - 4;
				i_w = i_h*1.67;
			}

			CFrameBuffer::getInstance()->paintIcon(hicon, itemBox.iX + BORDER_LEFT, itemBox.iY + (hheight - i_h)/2, 0, i_w, i_h);

			// Buttons
			int iw[hbutton_count], ih[hbutton_count];
			int xstartPos = itemBox.iX + itemBox.iWidth - BORDER_RIGHT;
			int buttonWidth = 0; //FIXME

			if (hbutton_count)
			{
				for (int i = 0; i < (int)hbutton_count; i++)
				{
					if (!hbutton_labels[i].button.empty())
					{
						frameBuffer->getIconSize(hbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
						
						if(ih[i] >= hheight)
						{
							ih[i] = hheight - 4;
							iw[i] = ih[i]*1.67;
						}

						xstartPos -= (iw[i] + ICON_TO_ICON_OFFSET);
						buttonWidth += iw[i];

						CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, itemBox.iY + (hheight - ih[i])/2, 0, iw[i], ih[i]);
					}
				}
			}

			// paint time/date
			int timestr_len = 0;
			if(paintDate)
			{
				std::string timestr = getNowTimeStr(format);
				
				timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(_(timestr.c_str()), true); // UTF-8
				
				if (timer)
				{
					timer->hide();
					delete timer;
					timer = NULL;
				}
			
				timer = new CCTime(xstartPos - timestr_len, itemBox.iY, timestr_len, hheight);

				timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
				timer->setFormat(format);
				
				timer->paint();
			}

			// title
			int startPosX = itemBox.iX + BORDER_LEFT + i_w + ICON_OFFSET;
			int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(_(htitle.c_str()));
			
			if (thalign == CC_ALIGN_CENTER)
				startPosX = itemBox.iX + (itemBox.iWidth >> 1) - (stringWidth >> 1);
		
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, itemBox.iY + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - buttonWidth - (hbutton_count - 1)*ICON_TO_ICON_OFFSET - timestr_len, _(htitle.c_str()), COL_MENUHEAD_TEXT_PLUS_0);
		}
		else
		{		
			// box
			if (paintframe)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), hheight, headColor, headRadius, headCorner, headGradient, headGradient_direction, headGradient_intensity, headGradient_type);
			
			// line
			if (head_line)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + hheight - 2 + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, head_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
		
			//paint icon (left)
			int i_w = 0;
			int i_h = 0;
			
			frameBuffer->getIconSize(hicon.c_str(), &i_w, &i_h);
			
			// limit icon height
			if(i_h >= hheight)
			{
				i_h = hheight - 4;
				i_w = i_h*1.67;
			}

			CFrameBuffer::getInstance()->paintIcon(hicon, itemBox.iX + BORDER_LEFT + (borderMode? 2 : 0), itemBox.iY + (hheight - i_h)/2 + (borderMode? 2 : 0), 0, i_w, i_h);

			// Buttons
			int iw[hbutton_count], ih[hbutton_count];
			int xstartPos = itemBox.iX + itemBox.iWidth - BORDER_RIGHT - (borderMode? 4 : 0);
			int buttonWidth = 0; //FIXME

			if (hbutton_count)
			{
				for (int i = 0; i < (int)hbutton_count; i++)
				{
					if (!hbutton_labels[i].button.empty())
					{
						frameBuffer->getIconSize(hbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
						
						if(ih[i] >= hheight)
						{
							ih[i] = hheight - 4;
							iw[i] = ih[i]*1.67;
						}
					
						xstartPos -= (iw[i] + ICON_TO_ICON_OFFSET);
						buttonWidth += iw[i];

						CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, itemBox.iY + (hheight - ih[i])/2, 0, iw[i], ih[i]);
					}
				}
			}

			// paint time/date
			int timestr_len = 0;
			if(paintDate)
			{
				std::string timestr = getNowTimeStr(format);
				
				timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(_(timestr.c_str()), true); // UTF-8
				
				if (timer)
				{
					timer->hide();
					delete timer;
					timer = NULL;
				}
			
				timer = new CCTime(xstartPos - timestr_len, itemBox.iY, timestr_len, hheight);

				timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
				timer->setFormat(format);
				timer->paint();
			}
		
			// head title
			int startPosX = itemBox.iX + BORDER_LEFT + i_w + ICON_OFFSET;
			int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(_(htitle.c_str()));
			
			if (thalign == CC_ALIGN_CENTER)
				startPosX = itemBox.iX + (itemBox.iWidth >> 1) - (stringWidth >> 1);
				
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, itemBox.iY + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), itemBox.iWidth - BORDER_RIGHT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - timestr_len - buttonWidth - (hbutton_count - 1)*ICON_TO_ICON_OFFSET, _(htitle.c_str()), COL_MENUHEAD_TEXT_PLUS_0, 0, true); // UTF-8
		}		
	}	
}

void ClistBox::paintFoot()
{
	if(paint_Foot)
	{
		dprintf(DEBUG_INFO, "ClistBox::paintFoot:\n");
		
		if(widgetType == TYPE_FRAME)
		{
			// box
			if (paintframe)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + itemBox.iHeight - fheight + (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), fheight, COL_MENUCONTENT_PLUS_0);

			// line
			if (foot_line)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + itemBox.iHeight - fheight - (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

			// buttons
			int buttonWidth = 0;
			int iw[fbutton_count];
			int ih[fbutton_count];

			buttonWidth = (fbutton_width)/fbutton_count;
	
			for (int i = 0; i < (int)fbutton_count; i++)
			{
				if (!fbutton_labels[i].button.empty())
				{
					iw[i] = 0;
					ih[i] = 0;

					CFrameBuffer::getInstance()->getIconSize(fbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
						
					if(ih[i] >= fheight)
					{
							ih[i] = fheight - 4;
					}
						
					int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
		
					CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - fheight + (fheight - ih[i])/2, 0, iw[i], ih[i]);

					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + itemBox.iHeight - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT_TEXT_PLUS_0, 0, true); // UTF-8
				}
			}
		}
		else
		{
			// box
			if (paintframe)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight - (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), fheight, footColor, footRadius, footCorner, footGradient, footGradient_direction, footGradient_intensity, footGradient_type);
			
			// line
			if (foot_line)
				frameBuffer->paintBoxRel(itemBox.iX + (borderMode? 2 : 0), itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight - (borderMode? 2 : 0), itemBox.iWidth - (borderMode? 4 : 0), 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

			// buttons
			int buttonWidth = 0;
			int iw[fbutton_count];
			int ih[fbutton_count];

			if (fbutton_count)
				buttonWidth = (fbutton_width)/fbutton_count;
	
			for (int i = 0; i < (int)fbutton_count; i++)
			{
				if (!fbutton_labels[i].button.empty())
				{
					iw[i] = 0;
					ih[i] = 0;

					CFrameBuffer::getInstance()->getIconSize(fbutton_labels[i].button.c_str(), &iw[i], &ih[i]);
					
					if(ih[i] >= fheight)
					{
						ih[i] = fheight - 4;
					}
						
					int f_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
		
					CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + (fheight - ih[i])/2, 0, iw[i], ih[i]);
					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT_TEXT_PLUS_0, 0, true); // UTF-8
				}
			}
		}
	}
}

void ClistBox::setHeadButtons(const struct button_label *_hbutton_labels, const int _hbutton_count)
{
	if(paint_Head)
	{
		if (_hbutton_count)
		{
			for (int i = 0; i < (int)_hbutton_count; i++)
			{
				hbutton_labels.push_back(_hbutton_labels[i]);
			}
		}

		hbutton_count = hbutton_labels.size();
	}
}

void ClistBox::setFootButtons(const struct button_label* _fbutton_labels, const int _fbutton_count, const int _fbutton_width)
{
	if(paint_Foot)
	{
		if (_fbutton_count)
		{
			for (int i = 0; i < (int)_fbutton_count; i++)
			{
				fbutton_labels.push_back(_fbutton_labels[i]);
			}
		}

		fbutton_count = fbutton_labels.size();	
		fbutton_width = (_fbutton_width == 0)? itemBox.iWidth - 20: _fbutton_width;
	}
}

void ClistBox::paintItemInfo(int pos)
{
	dprintf(DEBUG_INFO, "ClistBox::paintItemInfo: %d\n", pos); 
	
	if( (widgetType == TYPE_STANDARD) || (widgetType == TYPE_CLASSIC) )
	{
		if(paint_ItemInfo)
		{
			CMenuItem* item = items[pos];
			
			////
			if (paint_Foot && (widgetMode == MODE_MENU || (widgetMode == MODE_LISTBOX && fbutton_count == 0)))
			{	
				// info icon
				CCIcon infoIcon;
				infoIcon.setIcon(NEUTRINO_ICON_INFO);
				int iw, ih;
				frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
								
				// paint horizontal line buttom
				if (g_settings.Foot_line)
					frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, g_settings.Foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);
				
				// limit icon height
				if(ih >= fheight)
					ih = fheight;
						
				infoIcon.setPosition(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + (fheight - ih)/2, iw, ih);
				infoIcon.paint();

				// Hint
				label.setText(_(item->itemHint.c_str()));
				label.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
				label.setColor(COL_MENUFOOT_TEXT_PLUS_0);
				label.paint();
			}
			
			//// label InfoBox2: FIXME
			if (widgetMode == MODE_MENU && itemInfoBox2.iWidth != 0)
			{
				label.setText(_(item->itemHint.c_str()));
				label.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
				label.setColor(COL_MENUFOOT_TEXT_PLUS_0);
				label.paint();
			}
			
			////
			if (footInfoMode == CCItemInfo::ITEMINFO_HINTITEM)
			{
				// detailslines box
				itemsLine.setPosition(itemBox.iX, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight + 2, itemBox.iWidth, cFrameFootInfoHeight);
				itemsLine.setMode(CCItemInfo::ITEMINFO_HINTITEM);
				itemsLine.setHint(_(item->itemHint.c_str()));
					
				if (widgetType == TYPE_STANDARD)
				{
					std::string fname = item->itemIcon;
						
					if (widgetMode == MODE_MENU)
					{
						if (item->isPlugin)
							fname = item->itemIcon;
						else				
							fname = CFrameBuffer::getInstance()->getHintBasePath() + item->itemIcon.c_str() + ".png";
					}
				
					itemsLine.setIcon(fname.c_str());
				}
				
				itemsLine.paintMainFrame(true);		
				itemsLine.paint();
			}
			else if (footInfoMode == CCItemInfo::ITEMINFO_HINTICON)
			{
				// detailslines box
				itemsLine.setPosition(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
				itemsLine.setMode(CCItemInfo::ITEMINFO_HINTICON);
				itemsLine.setBorderMode(iteminfobordermode);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setFont(iteminfofont);
				itemsLine.setScaling(iteminfoscale);
				itemsLine.setHint(item->itemHint.c_str());
				//
				std::string fname = item->itemIcon;
						
				if (widgetMode == MODE_MENU)
				{
					if (item->isPlugin)
						fname = item->itemIcon;
					else				
						fname = CFrameBuffer::getInstance()->getHintBasePath() + item->itemIcon.c_str() + ".png";
				}
				
				itemsLine.setIcon(fname.c_str());
				itemsLine.paintMainFrame(true);
						
				itemsLine.paint();
			}
			else if (footInfoMode == CCItemInfo::ITEMINFO_ICON)
			{
				// detailslines box
				itemsLine.setPosition(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
				itemsLine.setMode(CCItemInfo::ITEMINFO_ICON);
				itemsLine.setBorderMode(iteminfobordermode);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setScaling(iteminfoscale);
				//
				std::string fname = item->itemIcon;
						
				if (widgetMode == MODE_MENU)
				{
					if (item->isPlugin)
						fname = item->itemIcon;
					else				
						fname = CFrameBuffer::getInstance()->getHintBasePath() + item->itemIcon.c_str() + ".png";
				}
				
				itemsLine.setIcon(fname.c_str());
				itemsLine.paintMainFrame(true); // FIXME
						
				itemsLine.paint();
			}
			else if (footInfoMode == CCItemInfo::ITEMINFO_HINT)
			{
				// detailslines box
				itemsLine.setPosition(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
				itemsLine.setMode(CCItemInfo::ITEMINFO_HINT);
				itemsLine.setBorderMode(iteminfobordermode);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setFont(iteminfofont);
				itemsLine.setScaling(iteminfoscale);
				itemsLine.setHint(_(item->itemHint.c_str()));
				itemsLine.paintMainFrame(true); //FIXME:
						
				itemsLine.paint();
			}
		}
	}	
	else if(widgetType == TYPE_EXTENDED)
	{
		CMenuItem* item = items[pos];
		
		if (widgetMode == MODE_LISTBOX)
		{
			// scale pic
			int p_w = 0;
			int p_h = 0;

			std::string fname = item->itemIcon;

			::scaleImage(fname, &p_w, &p_h);
					
			//
			itemsLine.setPosition(itemBox.iX + 2*(itemBox.iWidth/3) + 2, itemBox.iY + hheight, (itemBox.iWidth/3) - 2, items_height);
			itemsLine.setCorner(NO_RADIUS, CORNER_ALL);
			itemsLine.setGradient(NOGRADIENT);
					
			if (paintframe)
			{
				itemsLine.paintMainFrame(true);
				itemsLine.setBorderMode(CComponent::BORDER_NO);
			}
					
			itemsLine.setMode(CCItemInfo::ITEMINFO_HINTICON);		
			itemsLine.setHint(_(item->itemHint.c_str()));
			itemsLine.setIcon(fname.c_str());
					
			itemsLine.paint();
		}
		else //if (widgetMode == MODE_MENU)
		{
			itemsLine.setPosition(itemBox.iX + items_width + (itemBox.iWidth - items_width - ITEM_ICON_W)/2, itemBox.iY + (itemBox.iHeight - ITEM_ICON_H)/2, ITEM_ICON_W, ITEM_ICON_H);
			
			// item icon (right) check for minimum hight
			if(itemBox.iHeight - hheight - fheight >= ITEM_ICON_H)
			{ 
				itemsLine.setMode(CCItemInfo::ITEMINFO_ICON);
					
				std::string fname = item->itemIcon;
						
				if (item->isPlugin)
					fname = item->itemIcon;
				
				if (!::file_exists(fname.c_str()))
					fname = CFrameBuffer::getInstance()->getHintBasePath() + item->itemIcon.c_str() + ".png";
					
				itemsLine.setIcon(fname.c_str());
				itemsLine.setScaling(true);
				itemsLine.paint();
			}
		}
		
		//// itemHint on FootBar
		if (paint_ItemInfo) // MODE_LISTBOX | MODE_MENU
		{
			if ( paint_Foot && fbutton_count == 0)
			{
				int iw, ih;
				frameBuffer->getIconSize(NEUTRINO_ICON_INFO, &iw, &ih);
							
				// paint horizontal line buttom
				if (g_settings.Foot_line)
					frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, g_settings.Foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL, INT_LIGHT, GRADIENT_ONECOLOR);

				// info icon
				CCIcon infoIcon;
				infoIcon.setIcon(NEUTRINO_ICON_INFO);
							
				// limit icon height
				if(ih >= fheight)
					ih = fheight;
					
				infoIcon.setPosition(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + (fheight - ih)/2, iw, ih);
				infoIcon.paint();

				// Hint
				label.setText(_(item->itemHint.c_str()));
				label.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
				label.setColor(COL_MENUFOOT_TEXT_PLUS_0);
				label.paint();
			}
		}
	}
	else if(widgetType == TYPE_FRAME)
	{
		if (paint_ItemInfo)
		{	
			if (paintframe)
			{
				// refresh
				frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight, itemBox.iWidth, cFrameFootInfoHeight, COL_MENUCONTENT_PLUS_0);

				// refresh horizontal line buttom
				frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight + 2, COL_MENUCONTENT_PLUS_5);

				//
				if(items.size() > 0)
				{
					CMenuItem* item = items[pos];
				
					// itemName
					if(!item->itemName.empty())
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight + (cFrameFootInfoHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE] ->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight(), itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, _(item->itemName.c_str()), COL_MENUHINT_TEXT_PLUS_0);
					}

					// itemHint
					if(!item->itemHint.empty())
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, _(item->itemHint.c_str()), COL_MENUHINT_TEXT_PLUS_0);
					}
				}
			}
		}
	}	
}

void ClistBox::hideItemInfo()
{
	dprintf(DEBUG_INFO, "ClistBox::hideItemInfo:\n");
	
	if (paint_ItemInfo)
	{
	    	if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC)
		{
			itemsLine.hide();
		} 
	}	 
}

void ClistBox::saveScreen()
{
	dprintf(DEBUG_INFO, "ClistBox::saveScreen:\n");

	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[items_width*items_height];
	
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY + hheight, items_width, items_height, background);
	}
}

void ClistBox::restoreScreen()
{
	dprintf(DEBUG_INFO, "ClistBox::restoreScreen:\n");
	
	if(background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY + hheight, items_width, items_height, background);
	}
}

void ClistBox::hide()
{
	dprintf(DEBUG_INFO, "ClistBox::hide: (%s)\n", htitle.c_str());

	if (paintframe)
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
	else
		restoreScreen();
		
	//
	hideItemInfo(); 
	
	//
	if (timer)
	{
		timer->hide();
		delete timer;
		timer = NULL;
	}
	
	CFrameBuffer::getInstance()->blit();
	
	painted = false;
}

//
void ClistBox::scrollLineDown(const int)
{
	dprintf(DEBUG_INFO, "ClistBox::scrollLineDown:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			pos = selected + itemsPerX;

			//FIXME:
			if (pos >= (int)items.size())
				pos -= itemsPerX;

			CMenuItem * item = items[pos];

			if ( item->isSelectable() ) 
			{
				if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
				{ 
					// Item is currently on screen
					//clear prev. selected
					items[selected]->paint(false);
					//select new
					item->paint(true);
					paintItemInfo(pos);
					selected = pos;
				} 
			}
		}
	}
	else
	{
		if(items.size())
		{
			//search next / prev selectable item
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = (selected + count)%items.size();

				CMenuItem * item = items[pos];

				if ( item->isSelectable() ) 
				{
					if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
					{	
						// Item is currently on screen
						//clear prev. selected
						items[selected]->paint(false);
						//select new
						paintItemInfo(pos);
						item->paint(true);
						selected = pos;
					} 
					else 
					{
						selected = pos;
						
						paintHead();
						paintFoot();
						paintItems();
					}
					break;
				}
			}
		}
	}
}

//
void ClistBox::scrollLineUp(const int)
{
	dprintf(DEBUG_INFO, "ClistBox::scrollLineUp:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			pos = selected - itemsPerX;

			if(pos < 0)
				pos = selected;

			CMenuItem * item = items[pos];

			if ( item->isSelectable() ) 
			{
				if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
				{ 
					// Item is currently on screen
					//clear prev. selected
					items[selected]->paint(false);
					//select new
					item->paint(true);
					paintItemInfo(pos);
					selected = pos;
				}
			}
		}
	}
	else
	{
		if(items.size())
		{
			//search next / prev selectable item
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = selected - count;
				if ( pos < 0 )
					pos += items.size();

				CMenuItem * item = items[pos];

				if ( item->isSelectable() ) 
				{
					if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
					{ 
						// Item is currently on screen
						//clear prev. selected
						items[selected]->paint(false);
						//select new
						paintItemInfo(pos);
						item->paint(true);
						selected = pos;
					} 
					else 
					{
						selected = pos;
						
						paintHead();
						paintFoot();
						paintItems();
					}
					break;
				}
			}
		}
	}
}

//
void ClistBox::scrollPageDown(const int)
{
	dprintf(DEBUG_INFO, "ClistBox::scrollPageDown:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			if(current_page) 
			{
				pos = (int) page_start[current_page] - 1;

				selected = pos;
				
				paintHead();
				paintFoot();
				paintItems();
			}
		}
	}
	else
	{
		if(items.size())
		{
			pos = (int) page_start[current_page + 1];

			// check pos
			if(pos >= (int) items.size()) 
				pos = items.size() - 1;

			for (unsigned int count = pos ; count < items.size(); count++) 
			{
				CMenuItem * item = items[pos];
				if (item->isSelectable()) 
				{
					if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
					{
						items[selected]->paint(false);

						// paint new item
						paintItemInfo(pos);
						item->paint(true);
						selected = pos;
					} 
					else 
					{
						selected = pos;
						
						paintHead();
						paintFoot();
						paintItems();
					}
					break;
				}
				pos++;
			}
		}
	}
}

//
void ClistBox::scrollPageUp(const int)
{
	dprintf(DEBUG_INFO, "ClistBox::scrollPageUp:\n");
	
	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			pos = (int) page_start[current_page + 1];
			if(pos >= (int) items.size()) 
				pos = items.size() - 1;

			selected = pos;
			
			paintHead();
			paintFoot();
			paintItems();
		}
	}
	else
	{
		if(items.size())
		{
			if(current_page) 
			{
				pos = (int) page_start[current_page] - 1;
				for (unsigned int count = pos; count > 0; count--) 
				{
					CMenuItem * item = items[pos];
					if ( item->isSelectable() ) 
					{
						if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
						{
							// prev item
							items[selected]->paint(false);

							// new item
							paintItemInfo(pos);
							item->paint(true);
							selected = pos;
						} 
						else 
						{
							selected = pos;
							
							paintHead();
							paintFoot();
							paintItems();
						}
						break;
					}
					pos--;
				}
			} 
			else 
			{
				pos = 0;
				for (unsigned int count = 0; count < items.size(); count++) 
				{
					CMenuItem * item = items[pos];
					if (item->isSelectable()) 
					{
						if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page])) 
						{
							// prev item
							items[selected]->paint(false);

							// new item
							paintItemInfo(pos);
							item->paint(true);
							selected = pos;
						} 
						else 
						{
							selected = pos;
							
							paintHead();
							paintFoot();
							paintItems();
						}
						break;
					}
					pos++;
				}
			}
		}
	}
}

//
int ClistBox::swipLeft()
{
	dprintf(DEBUG_NORMAL, "ClistBox::swipLeft:\n");
	
	int ret = CMenuTarget::RETURN_NONE;

	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			//search next / prev selectable item
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = selected - count;
				if ( pos < 0 )
					pos += items.size();

				CMenuItem * item = items[pos];

				if ( item->isSelectable() ) 
				{
					if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
					{ 
						// Item is currently on screen
						//clear prev. selected
						items[selected]->paint(false);
						//select new
						item->paint(true);
						paintItemInfo(pos);
						selected = pos;
					}
					else 
					{
						selected = pos;
						
						paintHead();
						paintFoot();
						paintItems();
					}
								
					break;
				}
			}
		}
	}
	else if(widgetType == TYPE_STANDARD)
	{
		if(widgetMode == MODE_SETUP)
		{
			if(hasItem()) 
			{
				if((items[selected]->can_arrow)) 
				{
					CMenuItem * item = items[selected];
					item->msg = CRCInput::RC_left;
					actionKey = item->actionKey;
					
					ret = item->exec(parent);
				}
			} 
		}
	}
	
	return ret;
}

//
int ClistBox::swipRight()
{
	dprintf(DEBUG_NORMAL, "ClistBox::swipRight:\n");
	
	int ret = CMenuTarget::RETURN_NONE;

	if(widgetType == TYPE_FRAME)
	{
		if(items.size())
		{
			for (unsigned int count = 1; count < items.size(); count++) 
			{
				pos = (selected + count)%items.size();

				CMenuItem * item = items[pos];

				if ( item->isSelectable() ) 
				{
					if ((pos < (int)page_start[current_page + 1]) && (pos >= (int)page_start[current_page]))
					{ 
						// Item is currently on screen
						//clear prev. selected
						items[selected]->paint(false);
						//select new
						item->paint(true);
						paintItemInfo(pos);
						selected = pos;
					}
					else 
					{
						selected = pos;
						
						paintHead();
						paintFoot();
						paintItems();
					}
								
					break;
				}
			}
		}
	}
	else if(widgetType == TYPE_STANDARD)
	{
		if(widgetMode == MODE_SETUP)
		{
			if(hasItem()) 
			{
				if((items[selected]->can_arrow)) 
				{
					CMenuItem * item = items[selected];
					item->msg = CRCInput::RC_right;
					actionKey = item->actionKey;
					
					//
					ret = item->exec(parent);
				}
			} 
		}
	}
	
	return ret;
}

//
int ClistBox::oKKeyPressed(CMenuTarget *target, neutrino_msg_t _msg)
{
	dprintf(DEBUG_NORMAL, "ClistBox::okKeyPressed: msg:0x%x\n", _msg);
	
	int ret = CMenuTarget::RETURN_EXIT;

	if (hasItem() && selected >= 0 && items[selected]->isSelectable())
	{
		CMenuItem * item = items[selected];
		item->msg = _msg;
		actionKey = item->actionKey;
		
		ret = item->exec(target);
	}
	
	return ret;
}

//
int ClistBox::directKeyPressed(neutrino_msg_t _msg)
{
	dprintf(DEBUG_DEBUG, "ClistBox::directKeyPressed: msg:0x%x\n", _msg);
	
	int ret = CMenuTarget::RETURN_NONE;
	
	// 
	for (unsigned int i = 0; i < items.size(); i++) 
	{
		CMenuItem * titem = items[i];
			
		if ((titem->directKey != CRCInput::RC_nokey) && (titem->directKey == _msg)) 
		{
			if (titem->isSelectable()) 
			{
				items[selected]->paint(false);
				selected = i;

				if (selected > (int)page_start[current_page + 1] || selected < (int)page_start[current_page]) 
				{
					// different page
					paintHead();
					paintFoot();
					paintItems();
				}

				paintItemInfo(selected);
				pos = selected;
				
				titem->paint(true);

				//
				ret = titem->exec(parent);
			} 
			break;
		}
	}
	
	return ret;
}

//
void ClistBox::integratePlugins(CPlugins::i_type_t integration, const unsigned int shortcut, bool enabled, int imode, int itype, bool i2lines, int iBorder)
{
	unsigned int number_of_plugins = (unsigned int) g_PluginList->getNumberOfPlugins();

	std::string IconName;
	unsigned int sc = shortcut;

	for (unsigned int count = 0; count < number_of_plugins; count++)
	{
		if ((g_PluginList->getIntegration(count) == integration) && !g_PluginList->isHidden(count))
		{
			//
			IconName = NEUTRINO_ICON_MENUITEM_PLUGIN;

			std::string icon("");
			icon = g_PluginList->getIcon(count);

			if(!icon.empty())
			{
				IconName = PLUGINDIR;
				IconName += "/";
				IconName += g_PluginList->getFileName(count);
				IconName += "/";
				IconName += g_PluginList->getIcon(count);
			}

			//
			neutrino_msg_t dk = (shortcut != CRCInput::RC_nokey) ? CRCInput::convertDigitToKey(sc++) : CRCInput::RC_nokey;

			//FIXME: iconName
			CMenuForwarder *fw_plugin = new CMenuForwarder(_(g_PluginList->getName(count)), enabled, NULL, CPluginsExec::getInstance(), g_PluginList->getFileName(count), dk, NULL, IconName.c_str());

			fw_plugin->setHint(_(g_PluginList->getDescription(count).c_str()));
			fw_plugin->setWidgetType(itype);
			fw_plugin->set2lines(i2lines);
			fw_plugin->setBorderMode(iBorder);
			
			fw_plugin->isPlugin = true;
			
			addItem(fw_plugin);
		}
	}
}

void ClistBox::selectItemByName(const char *name)
{
	dprintf(DEBUG_NORMAL, "ClistBox::selectItemByName: %s\n", (name != NULL)? name : "NULL");
	
	if (hasItem())
	{
		for (int i = 0; i < items.size(); i++)
		{
			if (items[i]->itemName == name)
				setSelected(i);
		}
	}
}

