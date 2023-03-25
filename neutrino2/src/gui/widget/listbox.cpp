/*
	$Id: listbox.cpp 14.10.2022 mohousch Exp $


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

#include <unistd.h> //acces
#include <cctype>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/listbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/textbox.h>
#include <gui/widget/stringinput.h> // locked menu

#include <driver/color.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <system/debug.h>


extern CPlugins * g_PluginList;    // defined in neutrino.cpp

// CMenuItem
CMenuItem::CMenuItem()
{
	x = -1;
	directKey = RC_nokey;
	iconName = "";
	can_arrow = false;
	itemIcon = "";
	itemName = "";
	option = "";
	optionInfo = "";
	itemHint = "";
	info1 = "";
	option_info1 = "";
	info2 = "";
	option_info2 = "";

	icon1 = "";
	icon2 = "";

	number = 0;
	runningPercent = 0;
	pb = false;

	nameFont = SNeutrinoSettings::FONT_TYPE_MENU;
	optionFont = SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER;
	optionFontColor = COL_MENUCONTENT;
			
	//
	nLinesItem = false;
	widgetType = TYPE_STANDARD;
	widgetMode = MODE_LISTBOX;
	isPlugin = false;

	active = true;
	marked = false;

	jumpTarget = NULL;
	actionKey = "";
	
	paintFrame = true;
	borderMode = BORDER_NO;
	itemGradient = NOGRADIENT;
	
	//
	parent = NULL;
	
	//
	background = NULL;
	
	//
	observ = NULL;
	pulldown = false;
}

void CMenuItem::init(const int X, const int Y, const int DX, const int OFFX)
{
	x    = X;
	y    = Y;
	dx   = DX;
	offx = OFFX;
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

// CMenuOptionChooser
CMenuOptionChooser::CMenuOptionChooser(const char * const OptionName, int* const OptionValue, const struct keyval *const Options, const unsigned Number_Of_Options, const bool Active, CChangeObserver* const Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	optionNameString = OptionName;
	
	options = Options;
	active = Active;
	optionValue = OptionValue;
	number_of_options = Number_Of_Options;
	observ = Observ;
	directKey = DirectKey;
	iconName = IconName;
	can_arrow = true;
	pulldown = Pulldown;

	menuItem_type = MENUITEM_OPTION_CHOOSER;
}

void CMenuOptionChooser::setOptionValue(const int newvalue)
{
	*optionValue = newvalue;
}

int CMenuOptionChooser::getOptionValue(void) const
{
	return *optionValue;
}

int CMenuOptionChooser::exec(CMenuTarget*)
{
	dprintf(DEBUG_NORMAL, "CMenuOptionChooser::exec: (%s)\n", optionNameString.c_str());

	bool wantsRepaint = false;
	int ret = RETURN_REPAINT;

	// pulldown
	if( (msg == RC_ok) && pulldown ) 
	{
		if (parent)
			parent->hide();
			
		int select = -1;

		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionchooser");
		
		if (widget)
		{
			menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
			// head title
			if (menu->hasHead())
			{
				menu->setTitle(optionNameString.c_str());
			}
		}
		else
		{
			menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

			menu->setWidgetMode(MODE_SETUP);
			menu->enableShrinkMenu();
			
			menu->enablePaintHead();
			menu->setTitle(optionNameString.c_str());
			
			//
			menu->enablePaintFoot();		
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
			menu->setFootButtons(&btn);
			
			//
			widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			widget->name = "optionchooser";
			widget->setMenuPosition(MENU_POSITION_CENTER);
			widget->enableSaveScreen();
			widget->addWidgetItem(menu);
		}
		
		menu->clear();

		//
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			bool selected = false;
			const char *l_option;
			
			if (options[count].key == (*optionValue))
				selected = true;

			if(options[count].valname != 0)
				l_option = options[count].valname;
			
			menu->addItem(new ClistBoxItem(_(l_option)), selected);
		}
		
		widget->exec(NULL, "");
		ret = RETURN_REPAINT;

		select = menu->getSelected();
		
		if(select >= 0) 
			*optionValue = options[select].key;
	} 
	else 
	{
		for(unsigned int count = 0; count < number_of_options; count++) 
		{
			if (options[count].key == (*optionValue)) 
			{
				if( msg == RC_left ) 
				{
					if(count > 0)
						*optionValue = options[(count-1) % number_of_options].key;
					else
						*optionValue = options[number_of_options-1].key;
				} 
				else
					*optionValue = options[(count+1) % number_of_options].key;
				
				break;
			}
		}
	}
	
	paint(true);
	
	if(observ)
		wantsRepaint = observ->changeNotify(optionNameString, optionValue);

	if ( wantsRepaint )
		ret = RETURN_REPAINT;

	return ret;
}

int CMenuOptionChooser::paint(bool selected, bool AfterPulldown)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionChooser::paint\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint8_t color = COL_MENUCONTENT;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color = COL_MENUCONTENTSELECTED;
		
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
		color = COL_MENUCONTENTINACTIVE;
		//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		if (!paintFrame)
		{
			if (background)
			{
				delete [] background;
				background = NULL;
			}
									
			background = new fb_pixel_t[dx*height];
						
			if (background)
			{
				frameBuffer->saveScreen(x, y, dx, height, background);
			}
		}	
				
		// border
		frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_6);
				
		// itemBox
		if (borderMode == BORDER_NO)
		{
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient); 
		}
		else if (borderMode == BORDER_ALL)
		{
			frameBuffer->paintBoxRel(x + 2, y + 2, dx - 4, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			frameBuffer->paintBoxRel(x + 2, y, dx - 4, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			frameBuffer->paintBoxRel(x, y + 2, dx, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		} 
	}
	else
	{
		if (paintFrame)
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);
		else
		{
			if (background)
			{
				frameBuffer->restoreScreen(x, y, dx, height, background);
							
				delete [] background;
				background = NULL;
			}
		}
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

	if(l_option.empty()) 
	{
		*optionValue = options[0].key;

		if(options[0].valname != 0)
			l_option = options[0].valname;
	}

	// paint icon (left)
	int icon_w = 0;
	int icon_h = 0;
	
	if (!(iconName.empty()))
	{
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
			icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, true, icon_w, icon_h);	
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
			
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, true, icon_w, icon_h);
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + BORDER_LEFT, y + height, height, CRCInput::getKeyName(directKey), color, height);
        }

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_option.c_str()), true); // FIXME: i18n
	int stringstartposName = x + BORDER_LEFT + icon_w + ICON_OFFSET;
	int stringstartposOption = x + dx - (stringwidth + BORDER_RIGHT); //

	// locale
	const char * l_name = optionNameString.c_str();
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x), l_name, color, 0, true); // UTF-8
	
	// option
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_LEFT - (stringstartposOption - x), _(l_option.c_str()), color, 0, true); // FIXME: i18n

	// vfd
	if (selected && !AfterPulldown)
	{ 
		char str[256];
		snprintf(str, 255, "%s %s", l_name, l_option.c_str());

		CVFD::getInstance()->showMenuText(0, str, -1, true);
	}

	return y + height;
}

//CMenuOptionNumberChooser
CMenuOptionNumberChooser::CMenuOptionNumberChooser(const char * const Name, int * const OptionValue, const bool Active, const int min_value, const int max_value, CChangeObserver * const Observ, const int print_offset, const int special_value, const char * non_localized_name)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	nameString  = Name;
	
	active = Active;
	optionValue = OptionValue;

	lower_bound = min_value;
	upper_bound = max_value;

	display_offset = print_offset;

	localized_value = special_value;

	optionString = non_localized_name;
	
	can_arrow = true;
	observ = Observ;

	menuItem_type = MENUITEM_OPTION_NUMBER_CHOOSER;
}

int CMenuOptionNumberChooser::exec(CMenuTarget*)
{
	dprintf(DEBUG_NORMAL, "CMenuOptionNumberChooser::exec: (%s)\n", nameString.c_str());

	if( msg == RC_left ) 
	{
		if (((*optionValue) > upper_bound) || ((*optionValue) <= lower_bound))
			*optionValue = upper_bound;
		else
			(*optionValue)--;
	} 
	else 
	{
		if (((*optionValue) >= upper_bound) || ((*optionValue) < lower_bound))
			*optionValue = lower_bound;
		else
			(*optionValue)++;
	}
	
	paint(true);
	
	if(observ)
		observ->changeNotify(nameString, optionValue);

	return RETURN_REPAINT;
}

int CMenuOptionNumberChooser::paint(bool selected, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionNumberChooser::paint\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint8_t color = COL_MENUCONTENT;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		color = COL_MENUCONTENTSELECTED;
		
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
		color = COL_MENUCONTENTINACTIVE;
		//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		if (!paintFrame)
		{
			if (background)
			{
				delete [] background;
				background = NULL;
			}
									
			background = new fb_pixel_t[dx*height];
						
			if (background)
			{
				frameBuffer->saveScreen(x, y, dx, height, background);
			}
		}	
				
		// border
		frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_6);
				
		// itemBox
		if (borderMode == BORDER_NO)
		{
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient); 
		}
		else if (borderMode == BORDER_ALL)
		{
			frameBuffer->paintBoxRel(x + 2, y + 2, dx - 4, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			frameBuffer->paintBoxRel(x + 2, y, dx - 4, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			frameBuffer->paintBoxRel(x, y + 2, dx, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		} 
	}
	else
	{
		if (paintFrame)
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);
		else
		{
			if (background)
			{
				frameBuffer->restoreScreen(x, y, dx, height, background);
							
				delete [] background;
				background = NULL;
			}
		}
	}

	// option
	std::string l_option = " ";
	char option_value[11];

	if ( ((*optionValue) != localized_value) )
	{
		sprintf(option_value, "%d", ((*optionValue) + display_offset));
		l_option = option_value;
		
	}

	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(_(l_option.c_str()), true); // UTF-8
	int stringstartposName = x + BORDER_LEFT + ICON_OFFSET;
	int stringstartposOption = x + dx - stringwidth - BORDER_RIGHT; //

	// locale
	const char * l_name;
	
	l_name = nameString.c_str();
	
	l_name = (optionString != NULL) ? optionString : l_name;

	// locale
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x), l_name, color, 0, true); // UTF-8
	
	// option value
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_LEFT - (stringstartposOption - x), _(l_option.c_str()), color, 0, true); // UTF-8
	
	// vfd
	if(selected)
	{ 
		char str[256];
		snprintf(str, 255, "%s %s", l_name, option_value);

		CVFD::getInstance()->showMenuText(0, str, -1, true); 
	}

	return y + height;
}

// CMenuOptionStringChooser
CMenuOptionStringChooser::CMenuOptionStringChooser(const char * const Name, char * OptionValue, bool Active, CChangeObserver* Observ, const neutrino_msg_t DirectKey, const std::string & IconName, bool Pulldown)
{
	height = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;

	nameString = Name;
	active = Active;
	optionValue = OptionValue;
	observ = Observ;

	directKey = DirectKey;
	iconName = IconName;
	can_arrow = true;
	
	pulldown = Pulldown;

	menuItem_type = MENUITEM_OPTION_STRING_CHOOSER;
}

CMenuOptionStringChooser::~CMenuOptionStringChooser()
{
	options.clear();
}

void CMenuOptionStringChooser::addOption(const char * const value)
{
	options.push_back(std::string(value));
}

int CMenuOptionStringChooser::exec(CMenuTarget *)
{
	dprintf(DEBUG_NORMAL, "CMenuOptionStringChooser::exec: (%s)\n", nameString.c_str());

	bool wantsRepaint = false;
	int ret = RETURN_REPAINT; 

	// pulldown
	if( (msg == RC_ok) && pulldown) 
	{
		int select = -1;
		
		if (parent)
			parent->hide();
		
		//
		CWidget* widget = NULL;
		ClistBox* menu = NULL;
		
		widget = CNeutrinoApp::getInstance()->getWidget("optionstringchooser");
		
		if (widget)
		{
			menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
			
			// title
			if (menu->hasHead())
				menu->setTitle(nameString.c_str());
		}
		else
		{
			menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

			menu->setWidgetMode(MODE_SETUP);
			menu->enableShrinkMenu();
			
			//
			menu->enablePaintHead();
			menu->setTitle(nameString.c_str());
			
			//
			menu->enablePaintFoot();		
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
			menu->setFootButtons(&btn);
			
			//
			widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			widget->name = "optionstringchooser";
			widget->setMenuPosition(MENU_POSITION_CENTER);
			widget->enableSaveScreen();
			widget->addWidgetItem(menu);
		}
		
		menu->clear();
		
		//
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			bool selected = false;
			if (strcmp(options[count].c_str(), optionValue) == 0)
				selected = true;

			menu->addItem(new ClistBoxItem(options[count].c_str()), selected);
		}
		
		widget->exec(NULL, "");
		ret = RETURN_REPAINT;

		select = menu->getSelected();
		
		if(select >= 0)
			strcpy(optionValue, options[select].c_str());
	} 
	else 
	{
		//select next value
		for(unsigned int count = 0; count < options.size(); count++) 
		{
			if (strcmp(options[count].c_str(), optionValue) == 0) 
			{
				if( msg == RC_left ) 
				{
					if(count > 0)
						strcpy(optionValue, options[(count - 1) % options.size()].c_str());
					else
						strcpy(optionValue, options[options.size() - 1].c_str());
				} 
				else
					strcpy(optionValue, options[(count + 1) % options.size()].c_str());
				
				break;
			}
		}
	}

	paint(true, true);
	
	if(observ) 
		wantsRepaint = observ->changeNotify(nameString.c_str(), optionValue);
	
	if (wantsRepaint)
		ret = RETURN_REPAINT;

	return ret;
}

int CMenuOptionStringChooser::paint( bool selected, bool afterPulldown)
{
	dprintf(DEBUG_DEBUG, "CMenuOptionStringChooser::paint\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	uint8_t color = COL_MENUCONTENT;
	fb_pixel_t bgcolor = COL_MENUCONTENT_PLUS_0;
	
	if (selected) 
	{
		color = COL_MENUCONTENTSELECTED;
		
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
		color = COL_MENUCONTENTINACTIVE;
		//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}
	
	// paint item
	if (selected)
	{
		if (!paintFrame)
		{
			if (background)
			{
				delete [] background;
				background = NULL;
			}
									
			background = new fb_pixel_t[dx*height];
						
			if (background)
			{
				frameBuffer->saveScreen(x, y, dx, height, background);
			}
		}	
				
		// border
		frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_6);
				
		// itemBox
		if (borderMode == BORDER_NO)
		{
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient); 
		}
		else if (borderMode == BORDER_ALL)
		{
			frameBuffer->paintBoxRel(x + 2, y + 2, dx - 4, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			frameBuffer->paintBoxRel(x + 2, y, dx - 4, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			frameBuffer->paintBoxRel(x, y + 2, dx, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
		} 
	}
	else
	{
		if (paintFrame)
			frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);
		else
		{
			if (background)
			{
				frameBuffer->restoreScreen(x, y, dx, height, background);
							
				delete [] background;
				background = NULL;
			}
		}
	}

	// paint icon
	int icon_w = 0;
	int icon_h = 0;
		
	if (!(iconName.empty()))
	{
		frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
		
		if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4)
			icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
		
		frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, true, icon_w, icon_h);	
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
		
			frameBuffer->paintIcon(iconName, x + BORDER_LEFT, y, height, true, icon_w, icon_h);	
		}
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + BORDER_LEFT, y + height, height, CRCInput::getKeyName(directKey), color, height);
        }
        
        // locale text
	const char * l_name;
	
	l_name = nameString.c_str();
	
	int stringstartposName = x + BORDER_LEFT + icon_w + ICON_OFFSET;
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposName, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposName - x),  l_name, color, 0, true); // UTF-8
	
	// option value
	int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(optionValue, true);
	int stringstartposOption = std::max(stringstartposName + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_name, true) + ICON_OFFSET, x + dx - stringwidth - BORDER_RIGHT); //
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposOption, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight(), dx - BORDER_RIGHT - (stringstartposOption - x),  optionValue, color, 0, true);
	
	if (selected && !afterPulldown)
	{
		char str[256];
		snprintf(str, 255, "%s %s", l_name, optionValue);

		CVFD::getInstance()->showMenuText(0, str, -1, true);
	}

	return y + height;
}

// CMenuSeparator
CMenuSeparator::CMenuSeparator(const int Type, const char * const Text)
{
	directKey = RC_nokey;
	iconName = "";
	type = Type;
	textString = Text;

	menuItem_type = MENUITEM_SEPARATOR;
}

int CMenuSeparator::getHeight(void) const
{
	return g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;	
}

int CMenuSeparator::getWidth(void) const
{
	return 0;
}

const char * CMenuSeparator::getString(void)
{
	return textString;
}

int CMenuSeparator::paint(bool /*selected*/, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "CMenuSeparator::paint:\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	int height = getHeight();

	if(widgetType != TYPE_FRAME)
	{
		if (paintFrame)
			frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_0);

		// string
		if (type & STRING)
		{
			if(textString != NULL)
			{
				int stringstartposX;

				const char * l_text = getString();
				int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

				// if no alignment is specified, align centered
				if (type & ALIGN_LEFT)
					stringstartposX = x + BORDER_LEFT;
				else if (type & ALIGN_RIGHT)
					stringstartposX = x + dx - stringwidth - BORDER_RIGHT;
				else // ALIGN_CENTER
					stringstartposX = x + (dx >> 1) - (stringwidth >> 1);

				if (paintFrame)
					frameBuffer->paintBoxRel(stringstartposX - 5, y, stringwidth + 10, height, COL_MENUCONTENT_PLUS_0);

				g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(stringstartposX, y + height, dx - (stringstartposX - x) , l_text, COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
			}
		}
		
		// line
		if (type & LINE)
		{
			if (type & STRING)
			{
				if(textString != NULL)
				{
					const char * l_text = getString();
					int stringwidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true); // UTF-8

					// if no alignment is specified, align centered
					if (type & ALIGN_LEFT)
					{
						//
						frameBuffer->paintHLineRel(x + BORDER_LEFT + stringwidth + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
						frameBuffer->paintHLineRel(x + BORDER_LEFT + stringwidth + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
					}
					else if (type & ALIGN_RIGHT)
					{
						//
						frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
						frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT - stringwidth, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
					}
					else // ALIGN_CENTER
					{
						// left
						frameBuffer->paintHLineRel(x + BORDER_LEFT, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_LEFT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
						frameBuffer->paintHLineRel(x + BORDER_LEFT, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_LEFT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
						
						// right
						frameBuffer->paintHLineRel(x + (dx + stringwidth)/2 + BORDER_LEFT, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_RIGHT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
						frameBuffer->paintHLineRel(x + (dx + stringwidth)/2 + BORDER_LEFT, (dx - BORDER_LEFT - BORDER_RIGHT - stringwidth)/2 - BORDER_RIGHT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
					}
				}
			}
			else
			{
				frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1), COL_MENUCONTENTDARK_PLUS_0 );
				frameBuffer->paintHLineRel(x + BORDER_LEFT, dx - BORDER_LEFT - BORDER_RIGHT, y + (height >> 1) + 1, COL_MENUCONTENTDARK_PLUS_0 );
			}	
		}
	}

	return y + height;
}

bool CPINProtection::check()
{
	char cPIN[5];
	std::string hint = " ";
	
	do
	{
		cPIN[0] = 0;
		CPINInput * PINInput = new CPINInput(_("Youth protection"), cPIN, 4, hint.c_str());
		PINInput->exec(getParent(), "");
		delete PINInput;
		hint = "PIN-Code was wrong! Try again.";
	} while ((strncmp(cPIN, validPIN, 4) != 0) && (cPIN[0] != 0));
	
	return ( strncmp(cPIN, validPIN, 4) == 0);
}

bool CZapProtection::check()
{
	int res;
	char cPIN[5];
	std::string hint2 = " ";
	
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(_("Youth protection"), cPIN, 4, hint2.c_str(), fsk);

		res = PINInput->exec(getParent(), "");
		delete PINInput;

		hint2 = "PIN-Code was wrong! Try again.";
	} while ( (strncmp(cPIN,validPIN, 4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
		  
	return ( ( strncmp(cPIN, validPIN, 4) == 0 ) ||
			 ( fsk < g_settings.parentallock_lockage ) );
}

//ClistBoxItem
ClistBoxItem::ClistBoxItem(const char * const Text, const bool Active, const char * const Option, CMenuTarget* Target, const char * const ActionKey, const neutrino_msg_t DirectKey, const char * const IconName, const char* const ItemIcon, const char* const Hint)
{
	textString = Text? Text : "";

	option = Option? Option : "";

	active = Active;
	jumpTarget = Target;
	actionKey = ActionKey ? ActionKey : "";

	directKey = DirectKey;

	iconName = IconName ? IconName : "";
	itemIcon = ItemIcon? ItemIcon : "";
	itemName = Text? Text : "";
	itemHint = Hint? Hint : "";
	
	runningPercent = 0;
	
	optionValueString = "";

	menuItem_type = MENUITEM_LISTBOXITEM;
}

int ClistBoxItem::getHeight(void) const
{
	int iw = 0;
	int ih = 0;
	int bpp = 0;

	if(widgetType == TYPE_FRAME)
	{
		ih = item_height;
	}
	else if(widgetType == TYPE_CLASSIC)
	{
		ih = ITEM_ICON_H_MINI;
			
		//if (widgetMode == MODE_LISTBOX)
		if(nLinesItem && widgetMode == MODE_LISTBOX)
		{
			ih = ITEM_ICON_H_MINI*2;
		}
	}
	else // standard|extended
	{
		ih = ITEM_ICON_H_MINI/2;
		
		if (!iconName.empty())
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

	return std::max(ih, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight()) + 6;
}

int ClistBoxItem::getWidth(void) const
{
	if(widgetType == TYPE_FRAME)
	{
		return item_width;
	}
	else
	{
		int tw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(textString); //FIXME:

		return tw;
	}
}

int ClistBoxItem::exec(CMenuTarget* target)
{
	dprintf(DEBUG_NORMAL, "ClistBoxItem::exec: (%s) actionKey: (%s)\n", getName(), actionKey.c_str());

	int ret = RETURN_EXIT;

	if(jumpTarget)
	{
		ret = jumpTarget->exec(target, actionKey);

		if(ret) 
		{
			optionValueString = jumpTarget->getString().c_str();
		}
	}
	else
		ret = RETURN_EXIT;

	return ret;
}

const char * ClistBoxItem::getName(void)
{
	const char * l_name;
	
	l_name = textString.c_str();
	
	return l_name;
}

const char * ClistBoxItem::getOption(void)
{
	if(!optionValueString.empty())
		return optionValueString.c_str();
	else if(!option.empty())
		return option.c_str();
	else
		return NULL;
}

int ClistBoxItem::paint(bool selected, bool /*AfterPulldown*/)
{
	dprintf(DEBUG_DEBUG, "ClistBoxItem::paint:\n");

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	int height = getHeight();
	const char * l_text = getName();
	const char * option_text = getOption();	

	uint8_t color = COL_MENUCONTENT;
	fb_pixel_t bgcolor = marked? COL_MENUCONTENTSELECTED_PLUS_1 : COL_MENUCONTENT_PLUS_0;

	if (selected)
	{
		// FIXME:
		if (parent)
		{
			if (parent->inFocus)
			{
				color = COL_MENUCONTENTSELECTED;
				bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
			}
			else
			{
				color = COL_MENUCONTENT;	
				bgcolor = COL_MENUCONTENT_PLUS_0;
			}
		}
		else
		{
			color = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
	}
	else if (!active)
	{
		color = COL_MENUCONTENTINACTIVE;
		//bgcolor = COL_MENUCONTENTINACTIVE_PLUS_0;
	}

	if(widgetType == TYPE_FRAME)
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
									
				background = new fb_pixel_t[item_width*item_height];
						
				if (background)
				{
					frameBuffer->saveScreen(x, y, item_width, item_height, background);
				}
			}
				
			if (parent)
			{
				if (parent->inFocus)
				{	
					frameBuffer->paintBoxRel(x, y, item_width, item_height, bgcolor);

					if(!itemIcon.empty())
						frameBuffer->paintHintIcon(itemIcon, x + 2, y + 2, item_width - 4, item_height - 4);
				}
				else
				{
					frameBuffer->paintBoxRel(x, y, item_width, item_height, bgcolor); //FIXME:

					if(!itemIcon.empty())
						frameBuffer->paintHintIcon(itemIcon, x + 4*ICON_OFFSET, y + 4*ICON_OFFSET, item_width - 8*ICON_OFFSET, item_height - 8*ICON_OFFSET);
				}
			}
			else
			{
				frameBuffer->paintBoxRel(x, y, item_width, item_height, bgcolor);

				if(!itemIcon.empty())
					frameBuffer->paintHintIcon(itemIcon, x + 2, y + 2, item_width - 4, item_height - 4);
			}
		}
		else
		{
			// refresh
			if (paintFrame)
			{
				frameBuffer->paintBoxRel(x, y, item_width, item_height, bgcolor);
			}
			else
			{
				if (background)
				{
					frameBuffer->restoreScreen(x, y, item_width, item_height, background);
							
					delete [] background;
					background = NULL;
				}
			}
			
			if(!itemIcon.empty())
				frameBuffer->paintHintIcon(itemIcon, x + 4*ICON_OFFSET, y + 4*ICON_OFFSET, item_width - 8*ICON_OFFSET, item_height - 8*ICON_OFFSET);
		}

		// vfd
		if (selected)
		{
			CVFD::getInstance()->showMenuText(0, l_text, -1, true);
		}

		return 0;
	}
	else // standard|classic|extended
	{
		//
		if (selected)
		{
			if (!paintFrame)
			{
				if (background)
				{
					delete [] background;
					background = NULL;
				}
									
				background = new fb_pixel_t[dx*height];
						
				if (background)
				{
					frameBuffer->saveScreen(x, y, dx, height, background);
				}
			}	
				
			// border
			frameBuffer->paintBoxRel(x, y, dx, height, COL_MENUCONTENT_PLUS_6);
				
			// itemBox
			if (borderMode == BORDER_NO)
			{
				frameBuffer->paintBoxRel(x, y, dx, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient); 
			}
			else if (borderMode == BORDER_ALL)
			{
				frameBuffer->paintBoxRel(x + 2, y + 2, dx - 4, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
			}
			else if (borderMode == BORDER_LEFTRIGHT)
			{
				frameBuffer->paintBoxRel(x + 2, y, dx - 4, height, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
			}
			else if (borderMode == BORDER_TOPBOTTOM)
			{
				frameBuffer->paintBoxRel(x, y + 2, dx, height - 4, bgcolor, NO_RADIUS, CORNER_NONE, itemGradient);
			} 
		}
		else
		{
			if (paintFrame)
					frameBuffer->paintBoxRel(x, y, dx, height, bgcolor);
			else
			{
				if (background)
				{
					frameBuffer->restoreScreen(x, y, dx, height, background);
							
					delete [] background;
					background = NULL;
				}
			}
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
			
			if (icon1_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6)
			{
				icon1_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
				icon1_w = 1.63*icon1_h;
			}
		
			//frameBuffer->paintIcon(icon1, x + dx - BORDER_LEFT - icon1_w, y + (height - icon1_h)/2 );
			frameBuffer->paintIcon(icon1, x + dx - BORDER_LEFT - icon1_w, y, height, true, icon1_w, icon1_h);
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
			
			if (icon2_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6)
			{
				icon2_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
				icon2_w = 1.63*icon2_h;
			}
		
			frameBuffer->paintIcon(icon2, x + dx - BORDER_LEFT - (icon1_w? icon1_w + ICON_OFFSET : 0) - icon2_w, y + (height - icon2_h)/2 );
		}

		// optionInfo (right)
		int optionInfo_width = 0;
	
		if(!optionInfo.empty())
		{
			optionInfo_width = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(optionInfo.c_str());

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + dx - BORDER_RIGHT - (icon1_w? icon1_w + ICON_OFFSET : 0) - (icon2_w? icon2_w + ICON_OFFSET : 0) - optionInfo_width, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(), optionInfo_width, optionInfo.c_str(), color, 0, true); // UTF-8
		}

		// number
		int number_width = 0;
		int number_offset = 0;
		
		if(number)
		{
			number_offset = ICON_OFFSET;
			
			char tmp[10];

			sprintf((char*) tmp, "%d", number);

			number_width = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");

			int numPosX = x + BORDER_LEFT + number_width - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp) /*+ icon_w*/;

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numPosX, y + (height - g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(), number_width + 5, tmp, color, 0, true); // UTF-8
		}
		
		int icon_w = 0;
		int icon_h = 0;
		int bpp = 0;
		int icon_offset = 0;

		if(widgetType == TYPE_CLASSIC)
		{
			icon_h = ITEM_ICON_H_MINI;
			icon_w = ITEM_ICON_W_MINI;
			
			if(nLinesItem && widgetMode == MODE_LISTBOX)
			{
				icon_h = ITEM_ICON_H_MINI*2;
				icon_w = ITEM_ICON_W_MINI;
			}
					
			if (!itemIcon.empty())
			{
				frameBuffer->paintHintIcon(itemIcon.c_str(), x + BORDER_LEFT + number_width + number_offset, y + ((height - icon_h)/2), icon_w, icon_h);
			}
		}
		else // standard|extended
		{
			if (!iconName.empty())
			{
				icon_offset = ICON_OFFSET;
				
				//get icon size
				frameBuffer->getIconSize(iconName.c_str(), &icon_w, &icon_h);
				
				if (icon_h >= g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6)
				{
					icon_h = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 4;
					if (icon_w > 1.63*icon_h)
						icon_w = 1.63*icon_h;
				}
		
				frameBuffer->paintIcon(iconName, x + BORDER_LEFT + number_width + number_offset, y, height, true, icon_w, icon_h);
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

			CProgressBar timescale(pbPosX, y + (height - pBarHeight)/2, pb_width, pBarHeight);
			timescale.reset();
			timescale.paint(runningPercent);
		}
	
		// locale / option
		int l_text_width = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(l_text, true);
		int l_startPosX = x + BORDER_LEFT + number_width + number_offset + icon_w + icon_offset + pb_width + pb_offset + ICON_OFFSET;

		// local
		if(l_text_width >= dx - BORDER_LEFT - BORDER_RIGHT)
			l_text_width = dx - BORDER_LEFT - BORDER_RIGHT;
			
		if (widgetType == TYPE_CLASSIC)
		{
			if (nLinesItem)
			{
				// local
				if(l_text != NULL)
				{
						g_Font[nameFont]->RenderString(l_startPosX, y + 3 + g_Font[nameFont]->getHeight(), dx - BORDER_RIGHT - BORDER_LEFT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - ICON_OFFSET, l_text, color, 0, true); // UTF-8
				}
				
				// option
				if(option_text != NULL)
				{
						g_Font[optionFont]->RenderString(l_startPosX, y + height, dx - BORDER_LEFT - BORDER_RIGHT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - ICON_OFFSET, option_text, (selected || !active)? color : optionFontColor, 0, true);
				}
				
				// info1
				if (!info1.empty())
				{
					g_Font[optionFont]->RenderString(l_startPosX, y + height/2, dx - BORDER_LEFT - BORDER_RIGHT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - ICON_OFFSET, info1.c_str(), (selected || !active)? color : optionFontColor, 0, true);
				}
				
				// info 2
				if (!info2.empty())
				{
					g_Font[optionFont]->RenderString(l_startPosX, y + height/2 + height/4, dx - BORDER_LEFT - BORDER_RIGHT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - ICON_OFFSET, info2.c_str(), (selected || !active)? color : optionFontColor, 0, true);
				}
			}
			else
			{
				// local
				if(l_text != NULL)
				{
						g_Font[nameFont]->RenderString(l_startPosX, y + g_Font[nameFont]->getHeight() + (height - g_Font[nameFont]->getHeight())/2, dx - BORDER_RIGHT - BORDER_LEFT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - optionInfo_width - ICON_OFFSET, l_text, color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
						g_Font[optionFont]->RenderString(l_startPosX + l_text_width + ICON_OFFSET, y + g_Font[optionFont]->getHeight() + (height - g_Font[optionFont]->getHeight())/2, dx - BORDER_LEFT - BORDER_RIGHT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - optionInfo_width - ICON_OFFSET, option_text, (selected || !active)? color : optionFontColor, 0, true);
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
						g_Font[nameFont]->RenderString(l_startPosX, y + 3 + g_Font[nameFont]->getHeight(), dx - BORDER_RIGHT - BORDER_LEFT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - optionInfo_width - ICON_OFFSET, l_text, color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
						g_Font[optionFont]->RenderString(l_startPosX, y + height, dx - BORDER_LEFT - BORDER_RIGHT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - optionInfo_width - ICON_OFFSET, option_text, (selected || !active)? color : optionFontColor, 0, true);
				}
			}
			else
			{
				int l_text_width = g_Font[nameFont]->getRenderWidth(l_text, true);
				
				// local
				if(l_text != NULL)
				{
					g_Font[nameFont]->RenderString(l_startPosX, y + g_Font[nameFont]->getHeight() + (height - g_Font[nameFont]->getHeight())/2, dx - BORDER_RIGHT - BORDER_LEFT - number_width - pb_width - 2*ICON_OFFSET - icon_w - icon1_w - icon2_w - optionInfo_width - ICON_OFFSET, l_text, color, 0, true); // UTF-8
				}

				// option
				if(option_text != NULL)
				{
					if (widgetMode != MODE_SETUP)
					{
						int iw, ih;
						//get icon size
						frameBuffer->getIconSize(NEUTRINO_ICON_HD, &iw, &ih);

						g_Font[optionFont]->RenderString(l_startPosX + ICON_OFFSET + l_text_width + ICON_OFFSET, y + g_Font[optionFont]->getHeight() + (height - g_Font[optionFont]->getHeight())/2, dx - BORDER_LEFT - BORDER_RIGHT - number_width - ICON_OFFSET - pb_width - ICON_OFFSET - l_text_width - icon_w - icon1_w - ICON_OFFSET - icon2_w - ICON_OFFSET - 2*iw, option_text, (selected || !active)? color : optionFontColor, 0, true);
					}
					else
					{
						int stringwidth = g_Font[optionFont]->getRenderWidth(option_text, true);
						int stringstartposOption = std::max(x + BORDER_LEFT + icon_w + ICON_OFFSET + g_Font[nameFont]->getRenderWidth(l_text, true) + ICON_OFFSET, x + dx - (stringwidth + BORDER_RIGHT)); //

						g_Font[optionFont]->RenderString(stringstartposOption, y + (height - g_Font[optionFont]->getHeight())/2 + g_Font[optionFont]->getHeight(), dx - BORDER_LEFT - BORDER_RIGHT - ICON_OFFSET - l_text_width - icon_w, option_text, (selected || !active)? color : optionFontColor, 0, true);
					}
				}
			}
		}
		
		// vfd
		if (selected)
		{
			CVFD::getInstance()->showMenuText(0, l_text, -1, true);
		}
		
		return y + height;
	}
}

// CLockedlistBoxItem
int CLockedlistBoxItem::exec(CMenuTarget * target)
{
	dprintf(DEBUG_NORMAL, "CLockedlistBoxItem::exec\n");

	Parent = target;
	
	if( (g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_NEVER) || AlwaysAsk )
	{
		if (!check())
		{
			Parent = NULL;
			return RETURN_REPAINT;
		}
	}

	Parent = NULL;
	
	return ClistBoxItem::exec(target);
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
	
	// sanity check
	if(itemBox.iHeight > ((int)frameBuffer->getScreenHeight(true)))
		itemBox.iHeight = frameBuffer->getScreenHeight(true);

	// sanity check
	if(itemBox.iWidth > (int)frameBuffer->getScreenWidth(true))
		itemBox.iWidth = frameBuffer->getScreenWidth(true);
	
	full_width = itemBox.iWidth;
	full_height = itemBox.iHeight;

	wanted_height = dy;
	wanted_width = dx;
	start_x = x;
	start_y = y;
	
	//
	listmaxshow = 0;

	// head
	paintTitle = false;
	hheight = 0;
	hbutton_count	= 0;
	hbutton_labels.clear();
	paintDate = false;
	l_name = "";
	iconfile = "";
	thalign = CC_ALIGN_LEFT;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = g_settings.Head_corner;
	headGradient = g_settings.Head_gradient;
	head_line = g_settings.Head_line;
	head_line_gradient = false;
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
	foot_line = g_settings.Foot_line;
	foot_line_gradient = false;
	
	// foot info
	paintFootInfo = false;
	footInfoHeight = 0;
	cFrameFootInfoHeight = 0;
	footInfoMode = ITEMINFO_INFO_MODE;
	itemInfoBox.iX = 0;
	itemInfoBox.iY = 0;
	itemInfoBox.iWidth = 0;
	itemInfoBox.iHeight = 0;
	iteminfoborder = false;
	iteminfosavescreen = false;
	iteminfobordermode = BORDER_NO;
	iteminfoframe = true;
	iteminfofont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
	iteminfocolor = COL_MENUCONTENT_PLUS_0;
	iteminfoscale = false;
	
	//
	inFocus = true;
	shrinkMenu = false;

	//
	itemsPerX = 6;
	itemsPerY = 3;
	maxItemsPerPage = itemsPerX*itemsPerY;

	//
	widgetType = TYPE_STANDARD;
	cnt = 0;
	widgetMode = MODE_LISTBOX;

	savescreen = false;
	background = NULL;

	textBox = NULL;

	actionKey = "";
	
	paintFrame = true;
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	scrollbar = true;
	items_background = NULL;
	
	item_height = 0;
	item_width = 0;
	iconOffset = 0;
	items_width = 0;
	items_height = 0;
	
	//
	itemBorderMode = BORDER_NO;
	itemGradient = NOGRADIENT;
	item2Lines = false;
	
	//
	sec_timer_id = 0;
	
	widgetItem_type = WIDGETITEM_LISTBOX;
}

ClistBox::ClistBox(CBox* position)
{
	frameBuffer = CFrameBuffer::getInstance();

	selected = -1;
	current_page = 0;
	pos = 0;

	itemBox = *position;
	
	// sanity check
	if(itemBox.iHeight > ((int)frameBuffer->getScreenHeight(true)))
		itemBox.iHeight = frameBuffer->getScreenHeight(true);

	// sanity check
	if(itemBox.iWidth > (int)frameBuffer->getScreenWidth(true))
		itemBox.iWidth = frameBuffer->getScreenWidth(true);
	
	full_width = itemBox.iWidth;
	full_height = itemBox.iHeight;

	wanted_height = position->iHeight;
	wanted_width = position->iWidth;
	start_x = position->iX;
	start_y = position->iY;
	
	//
	listmaxshow = 0;

	// head
	paintTitle = false;
	hheight = 0;
	hbutton_count	= 0;
	hbutton_labels.clear();
	fbutton_count	= 0;
	fbutton_labels.clear();
	fbutton_width = itemBox.iWidth - 20;
	paintDate = false;
	l_name = "";
	iconfile = "";
	thalign = CC_ALIGN_LEFT;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = g_settings.Head_corner;
	headGradient = g_settings.Head_gradient;
	head_line = g_settings.Head_line;
	head_line_gradient = false;
	format = "%d.%m.%Y %H:%M";
	timer = NULL;

	// foot
	paint_Foot = false;
	fheight = 0;
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = g_settings.Foot_corner;
	footGradient = g_settings.Foot_gradient;
	foot_line = g_settings.Foot_line;
	foot_line_gradient = false;
	
	// footInfo
	paintFootInfo = false;
	footInfoHeight = 0;
	cFrameFootInfoHeight = 0;
	footInfoMode = ITEMINFO_INFO_MODE;
	itemInfoBox.iX = 0;
	itemInfoBox.iY = 0;
	itemInfoBox.iWidth = 0;
	itemInfoBox.iHeight = 0;
	iteminfoborder = false;
	iteminfosavescreen = false;
	iteminfobordermode = BORDER_NO;
	iteminfoframe = true;
	iteminfofont = SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
	iteminfocolor = COL_MENUCONTENT_PLUS_0;
	iteminfoscale = false;

	//
	widgetType = TYPE_STANDARD;
	cnt = 0;
	widgetMode = MODE_LISTBOX;
	
	//
	inFocus = true;
	shrinkMenu = false;

	//
	itemsPerX = 6;
	itemsPerY = 3;
	maxItemsPerPage = itemsPerX*itemsPerY;

	savescreen = false;
	background = NULL;

	textBox = NULL;

	actionKey = "";
	
	paintFrame = true;
	bgcolor = COL_MENUCONTENT_PLUS_0;
	radius = NO_RADIUS;
	corner = CORNER_NONE;
	scrollbar = true;
	items_background = NULL;
	
	//
	item_height = 0;
	item_width = 0;
	iconOffset = 0;
	items_width = 0;
	items_height = 0;
	itemBorderMode = BORDER_NO;
	itemGradient = NOGRADIENT;
	item2Lines = false;
	
	//
	sec_timer_id = 0;
	
	widgetItem_type = WIDGETITEM_LISTBOX;
}

ClistBox::~ClistBox()
{
	dprintf(DEBUG_INFO, "ClistBox:: del\n");

	items.clear();
	
	if (items_background)
	{
		delete [] items_background;
		items_background = NULL;
	}
}

void ClistBox::addItem(CMenuItem * menuItem, const bool defaultselected)
{
	if (defaultselected)
		selected = items.size();
	
	items.push_back(menuItem);
	menuItem->setParent(this);
}

bool ClistBox::hasItem()
{
	return !items.empty();
}

void ClistBox::initFrames()
{
	dprintf(DEBUG_INFO, "ClistBox::initFrames:\n");
	
	// reinit position
	itemBox.iHeight = wanted_height;
	itemBox.iWidth = wanted_width;
	itemBox.iX = start_x;
	itemBox.iY = start_y;
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
		item->paintFrame = paintFrame;
		if (itemBorderMode) item->setBorderMode(itemBorderMode);
		if (itemGradient) item->setGradient(itemGradient);
		if (item2Lines) item->set2lines();
	} 

	// head
	if(paintTitle)
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
		if (paintFrame && paintFootInfo)
		{
			cFrameFootInfoHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
		}
		
		//	
		if(fbutton_count == 0)
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

		// HACK:
		for (unsigned int count = 0; count < items.size(); count++) 
		{
			CMenuItem * item = items[count];

			item->item_width = item_width;
			item->item_height = item_height;
		}		
	}
	else 
	{
		// footInfoBox
		if(paintFootInfo)
		{
			if( (widgetType == TYPE_STANDARD) || (widgetType == TYPE_CLASSIC) )
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
				//heightFirstPage = heightCurrPage - item_height;
				heightFirstPage = std::max(heightCurrPage - item_height, heightFirstPage); //FIXME:
				total_pages++;
				heightCurrPage = item_height;
			}
		}

		heightFirstPage = std::max(heightCurrPage, heightFirstPage); //FIXME:
		page_start.push_back(items.size());

		// icon offset
		iconOffset = 0;

		for (unsigned int i = 0; i < items.size(); i++) 
		{
			if ((!(items[i]->iconName.empty())) || CRCInput::isNumeric(items[i]->directKey))
			{
				iconOffset = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
				break;
			}
		}

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
		}
		
		// sanity check after recalculating height
		if(itemBox.iHeight > (int)frameBuffer->getScreenHeight(true))
			itemBox.iHeight = frameBuffer->getScreenHeight(true);

		// sanity check
		if(itemBox.iWidth > (int)frameBuffer->getScreenWidth(true))
			itemBox.iWidth = frameBuffer->getScreenWidth(true);
		
		//
		full_height = itemBox.iHeight;
		full_width = itemBox.iWidth;
		
		// menu position
		if (parent && parent->getMenuPosition())
		{
			if(parent->getMenuPosition() == MENU_POSITION_CENTER)
			{
				itemBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - full_width ) >> 1 );
				itemBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
			}
			else if(parent->getMenuPosition() == MENU_POSITION_LEFT)
			{
				itemBox.iX = frameBuffer->getScreenX();
				itemBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
			}
			else if(parent->getMenuPosition() == MENU_POSITION_RIGHT)
			{
				itemBox.iX = frameBuffer->getScreenX() + frameBuffer->getScreenWidth() - full_width;
				itemBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - full_height) >> 1 );
			}
		}
	}
}

void ClistBox::paint()
{
	dprintf(DEBUG_INFO, "ClistBox::paint: (%s)\n", l_name.c_str());

	//
	initFrames();
	
	//
	if (!paintFrame)
	{
		if (items_background)
		{
			delete [] items_background;
			items_background = NULL;
		}
		
		if(widgetType == TYPE_FRAME)
		{
			items_background = new fb_pixel_t[itemBox.iWidth*(itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20)];
								
			if (items_background)
			{
				frameBuffer->saveScreen(itemBox.iX, itemBox.iY + hheight + 10, itemBox.iWidth, itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20, items_background);
			}
		}
		else
		{					
			items_background = new fb_pixel_t[itemBox.iWidth*(itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight)];
								
			if (items_background)
			{
				frameBuffer->saveScreen(itemBox.iX, itemBox.iY + hheight, itemBox.iWidth, itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight, items_background);
			}
		}
	}

	//
	paintHead();
	paintFoot();
	paintItems();
	
	painted = true;
}

void ClistBox::paintItems()
{
	dprintf(DEBUG_INFO, "ClistBox::paintItems:\n");

	if(widgetType == TYPE_FRAME)
	{
		item_start_y = itemBox.iY + hheight + 10;
		items_height = itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight - 20;
		items_width = itemBox.iWidth;

		// items background
		if (paintFrame)
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + hheight, itemBox.iWidth, itemBox.iHeight - hheight - fheight, bgcolor, radius, corner);
		else
		{
			if (items_background)
			{
				frameBuffer->restoreScreen(itemBox.iX, itemBox.iY + hheight + 10, itemBox.iWidth, items_height, items_background);
			}
		}

		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}

		for (unsigned int i = 0; i < items.size(); i++) 
		{
			CMenuItem * item = items[i];	
			item->init(-1, 0, 0, 0);
		}

		int count = (int)page_start[current_page];

		if(items.size() > 0)
		{
			for (int _y = 0; _y < itemsPerY; _y++)
			{
				for (int _x = 0; _x < itemsPerX; _x++)
				{
					CMenuItem * item = items[count];
					
					item->init(itemBox.iX + _x*item_width, item_start_y + _y*item_height, items_width, iconOffset);

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
	else
	{
		item_start_y = itemBox.iY + hheight;
		items_height = itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight; 

		sb_width = 0;
	
		if(total_pages > 1)
			sb_width = scrollbar? SCROLLBAR_WIDTH : 0;

		items_width = itemBox.iWidth - sb_width;

		// extended
		if(widgetType == TYPE_EXTENDED)
		{
			items_width = 2*(itemBox.iWidth/3) - sb_width;			
		}

		// item not currently on screen
		if (selected >= 0)
		{
			while(selected < (int)page_start[current_page])
				current_page--;
		
			while(selected >= (int)page_start[current_page + 1])
				current_page++;
		}

		// paint items background
		if (paintFrame) //FIXME:
		{
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + hheight, itemBox.iWidth, items_height, bgcolor, radius, corner);
		}
		else
		{
			if (items_background)
			{
				frameBuffer->restoreScreen(itemBox.iX, itemBox.iY + hheight, itemBox.iWidth, items_height, items_background);
			}
		}
	
		// paint right scrollBar if we have more then one page
		if(total_pages > 1)
		{
			if (scrollbar)
			{
				if(widgetType == TYPE_EXTENDED)
					scrollBar.paint(itemBox.iX + 2*(itemBox.iWidth/3) - SCROLLBAR_WIDTH, itemBox.iY + hheight, itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight, total_pages, current_page);
				else
					scrollBar.paint(itemBox.iX + itemBox.iWidth - SCROLLBAR_WIDTH, itemBox.iY + hheight, itemBox.iHeight - hheight - fheight - cFrameFootInfoHeight, total_pages, current_page);
			}
		}

		// paint items
		int ypos = itemBox.iY + hheight;
		int xpos = itemBox.iX;
	
		for (unsigned int count = 0; count < items.size(); count++) 
		{
			CMenuItem * item = items[count];

			if ((count >= page_start[current_page]) && (count < page_start[current_page + 1])) 
			{
				//
				item->init(xpos, ypos, items_width, iconOffset);
				
			
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
	if(paintTitle)
	{
		dprintf(DEBUG_INFO, "ClistBox::paintHead:\n");
		
		if(widgetType == TYPE_FRAME)
		{
			// box
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, hheight, COL_MENUCONTENT_PLUS_0);

			// head line
			if (head_line)
				//frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + hheight - 2, COL_MENUCONTENT_PLUS_5);
				frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + hheight - 2, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, head_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL);

			// icon
			int i_w = 0;
			int i_h = 0;

			frameBuffer->getIconSize(iconfile.c_str(), &i_w, &i_h);
			
			if(i_h >= hheight)
			{
				i_h = hheight - 4;
				i_w = i_h*1.67;
			}

			CFrameBuffer::getInstance()->paintIcon(iconfile, itemBox.iX + BORDER_LEFT, itemBox.iY + (hheight - i_h)/2, 0, true, i_w, i_h);

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

						CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, itemBox.iY + (hheight - ih[i])/2, 0, true, iw[i], ih[i]);
					}
				}
			}

			// paint time/date
			int timestr_len = 0;
			if(paintDate)
			{
				std::string timestr = getNowTimeStr(format);
				
				timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
			
				timer = new CCTime(xstartPos - timestr_len, itemBox.iY, timestr_len, hheight);

				timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
				timer->setFormat(format);
				timer->paint();
			}

			// title
			int startPosX = itemBox.iX + BORDER_LEFT + i_w + ICON_OFFSET;
			int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name);
			
			if (thalign == CC_ALIGN_CENTER)
				startPosX = itemBox.iX + (itemBox.iWidth >> 1) - (stringWidth >> 1);
		
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, itemBox.iY + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - buttonWidth - (hbutton_count - 1)*ICON_TO_ICON_OFFSET - timestr_len, l_name, COL_MENUHEAD);
		}
		else
		{		
			// paint head
			if (paintFrame)
				frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, hheight, headColor, headRadius, headCorner, headGradient);
			
			// paint horizontal line top
			if (head_line)
				//frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + hheight - 2, COL_MENUCONTENT_PLUS_5);
				frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + hheight - 2, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, head_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL);
		
			//paint icon (left)
			int i_w = 0;
			int i_h = 0;
			
			frameBuffer->getIconSize(iconfile.c_str(), &i_w, &i_h);
			
			// limit icon height
			if(i_h >= hheight)
			{
				i_h = hheight - 4;
				i_w = i_h*1.67;
			}

			CFrameBuffer::getInstance()->paintIcon(iconfile, itemBox.iX + BORDER_LEFT, itemBox.iY + (hheight - i_h)/2, 0, true, i_w, i_h);

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

						CFrameBuffer::getInstance()->paintIcon(hbutton_labels[i].button, xstartPos, itemBox.iY + (hheight - ih[i])/2, 0, true, iw[i], ih[i]);
					}
				}
			}

			// paint time/date
			int timestr_len = 0;
			if(paintDate)
			{
				std::string timestr = getNowTimeStr(format);
				
				timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(timestr.c_str(), true); // UTF-8
			
				timer = new CCTime(xstartPos - timestr_len, itemBox.iY, timestr_len, hheight);

				timer->setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
				timer->setFormat(format);
				timer->paint();
			}
		
			// head title
			int startPosX = itemBox.iX + BORDER_LEFT + i_w + ICON_OFFSET;
			int stringWidth = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(l_name);
			
			if (thalign == CC_ALIGN_CENTER)
				startPosX = itemBox.iX + (itemBox.iWidth >> 1) - (stringWidth >> 1);
				
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(startPosX, itemBox.iY + (hheight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight(), itemBox.iWidth - BORDER_RIGHT - BORDER_RIGHT - i_w - 2*ICON_OFFSET - timestr_len - buttonWidth - (hbutton_count - 1)*ICON_TO_ICON_OFFSET, l_name.c_str(), COL_MENUHEAD, 0, true); // UTF-8
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
			if(fbutton_count)
			{
				// 
				frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + itemBox.iHeight - fheight, itemBox.iWidth, fheight, COL_MENUCONTENT_PLUS_0);

				// foot line
				if (foot_line)
					//frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + itemBox.iHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);
					frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight + 2, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL);

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
		
						CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - fheight + (fheight - ih[i])/2, 0, true, iw[i], ih[i]);

						// FIXME: i18n
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + itemBox.iHeight - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT, 0, true); // UTF-8
					}
				}
			}
		}
		else
		{
			if (paintFrame)
				frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight, itemBox.iWidth, fheight, footColor, footRadius, footCorner, footGradient);
			
			// foot line
			if (foot_line)
				//frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + 2, COL_MENUCONTENT_PLUS_5);
				frameBuffer->paintBoxRel(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight + 2, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, 2, COL_MENUCONTENT_PLUS_5, 0, CORNER_NONE, foot_line_gradient? DARK2LIGHT2DARK : NOGRADIENT, GRADIENT_HORIZONTAL);

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
		
					CFrameBuffer::getInstance()->paintIcon(fbutton_labels[i].button, itemBox.iX + BORDER_LEFT + i*buttonWidth, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + (fheight - ih[i])/2, 0, true, iw[i], ih[i]);
					// FIXME: i18n
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(itemBox.iX + BORDER_LEFT + iw[i] + ICON_OFFSET + i*buttonWidth, itemBox.iY + itemBox.iHeight - cFrameFootInfoHeight - fheight + f_h + (fheight - f_h)/2, buttonWidth - iw[i] - ICON_OFFSET, _(fbutton_labels[i].localename.c_str()), COL_MENUFOOT, 0, true); // UTF-8
				}
			}
		}
	}
}

void ClistBox::setHeadButtons(const struct button_label *_hbutton_labels, const int _hbutton_count)
{
	if(paintTitle)
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
	if( (widgetType == TYPE_STANDARD) || (widgetType == TYPE_CLASSIC) )
	{
		if(paintFootInfo)
		{
			dprintf(DEBUG_INFO, "ClistBox::paintItemInfo:\n"); //FIXME:
			
			if (footInfoMode == ITEMINFO_INFO_MODE)
			{
				CMenuItem* item = items[pos];

				// detailslines
				itemsLine.setMode(DL_INFO);
				itemsLine.setInfo1(item->info1.c_str());
				itemsLine.setOptionInfo1(item->option_info1.c_str());
				itemsLine.setInfo2(item->info2.c_str());
				itemsLine.setOptionInfo2(item->option_info2.c_str());
					
				itemsLine.paint(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
			}
			else if (footInfoMode == ITEMINFO_HINT_MODE)
			{
				CMenuItem* item = items[pos];
	
				// detailslines box
				itemsLine.setMode(DL_HINT);
				itemsLine.setHint(item->itemHint.c_str());
				
				if (widgetType == TYPE_STANDARD)
					itemsLine.setIcon(item->itemIcon.c_str());
					
				itemsLine.paint(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight - cFrameFootInfoHeight, cFrameFootInfoHeight, item->getHeight(), item->getYPosition());
			}
			else if (footInfoMode == ITEMINFO_HINTITEM_MODE)
			{
				CMenuItem* item = items[pos];
	
				// detailslines box
				itemsLine.setMode(DL_HINTITEM);
				itemsLine.setBorderMode(iteminfobordermode);
				if (iteminfosavescreen) itemsLine.enableSaveScreen();
				itemsLine.paintFrame(iteminfoframe);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setFont(iteminfofont);
				itemsLine.setScaling(iteminfoscale);
				itemsLine.setHint(item->itemHint.c_str());
				itemsLine.setIcon(item->itemIcon.c_str());
					
				itemsLine.paint(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
			}
			else if (footInfoMode == ITEMINFO_HINTICON_MODE)
			{
				CMenuItem* item = items[pos];
	
				// detailslines box
				itemsLine.setMode(DL_HINTICON);
				itemsLine.setBorderMode(iteminfobordermode);
				if (iteminfosavescreen) itemsLine.enableSaveScreen();
				itemsLine.paintFrame(iteminfoframe);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setScaling(iteminfoscale);
				//itemsLine.setHint(item->itemHint.c_str());
				itemsLine.setIcon(item->itemIcon.c_str());
					
				itemsLine.paint(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
			}
			else if (footInfoMode == ITEMINFO_HINTHINT_MODE)
			{
				CMenuItem* item = items[pos];
	
				// detailslines box
				itemsLine.setMode(DL_HINTHINT);
				itemsLine.setBorderMode(iteminfobordermode);
				if (iteminfosavescreen) itemsLine.enableSaveScreen();
				itemsLine.paintFrame(iteminfoframe);
				itemsLine.setColor(iteminfocolor);
				itemsLine.setFont(iteminfofont);
				itemsLine.setScaling(iteminfoscale);
				itemsLine.setHint(item->itemHint.c_str());
				//itemsLine.setIcon(item->itemIcon.c_str());
					
				itemsLine.paint(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight);
			}
		}
	}	
	else if(widgetType == TYPE_EXTENDED)
	{
		dprintf(DEBUG_INFO, "ClistBox::paintItemInfo:\n"); 
		
		if (paintFootInfo)
		{
			CMenuItem* item = items[pos];

			// scale pic
			int p_w = 0;
			int p_h = 0;

			std::string fname = item->itemIcon;

			::scaleImage(fname, &p_w, &p_h);

			if(textBox)
			{
				delete textBox;
				textBox = NULL;
			}
							
			textBox = new CTextBox(itemBox.iX + 2*(itemBox.iWidth/3), itemBox.iY + hheight, (itemBox.iWidth/3), items_height);
			//textBox->setMode(AUTO_WIDTH | ~SCROLL);
					
			textBox->setBackgroundColor(COL_MENUCONTENTDARK_PLUS_0);

			// hint
			textBox->setText(item->itemHint.c_str(), item->itemIcon.c_str(), p_w, p_h, PIC_CENTER);
			textBox->paint();
		}
	}
	else if(widgetType == TYPE_FRAME)
	{
		if (paintFrame && paintFootInfo)
		{
			dprintf(DEBUG_INFO, "ClistBox::paintItemInfo:\n"); //FIXME:
			
			// refresh
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight, itemBox.iWidth, cFrameFootInfoHeight, COL_MENUCONTENT_PLUS_0);

			// refresh horizontal line buttom
			frameBuffer->paintHLineRel(itemBox.iX + BORDER_LEFT, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight + 2, COL_MENUCONTENT_PLUS_5);

			if(items.size() > 0)
			{
				CMenuItem* item = items[pos];
			
				// itemName
				if(!item->itemName.empty())
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight - cFrameFootInfoHeight + (cFrameFootInfoHeight - g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE] ->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight(), itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, item->itemName.c_str(), COL_MENUHINT);
				}

				// hint
				if(!item->itemHint.empty())
				{
					g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(itemBox.iX + BORDER_LEFT, itemBox.iY + itemBox.iHeight - fheight, itemBox.iWidth - BORDER_LEFT - BORDER_RIGHT, item->itemHint.c_str(), COL_MENUHINT);
				}
			}
		}
	}	
}

void ClistBox::hideItemInfo()
{
	dprintf(DEBUG_INFO, "ClistBox::hideItemInfo:\n");
	
	if (paintFootInfo)
	{
	    	if(widgetType == TYPE_STANDARD || widgetType == TYPE_CLASSIC)
		{
			if ((footInfoMode == ITEMINFO_INFO_MODE) || (footInfoMode == ITEMINFO_HINT_MODE))
				itemsLine.clear(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight - cFrameFootInfoHeight, cFrameFootInfoHeight);
			else
				itemsLine.clear(itemInfoBox.iX, itemInfoBox.iY, itemInfoBox.iWidth, itemInfoBox.iHeight, 0);
		} 
	}	 
}

void ClistBox::saveScreen()
{
	dprintf(DEBUG_INFO, "ClistBox::saveScreen:\n");
	
	if(!savescreen)
		return;

	if(background)
	{
		delete[] background;
		background = NULL;
	}

	background = new fb_pixel_t[wanted_width*wanted_height];
	
	if(background)
	{
		frameBuffer->saveScreen(itemBox.iX, itemBox.iY, wanted_width, wanted_height, background);
	}
}

void ClistBox::restoreScreen()
{
	dprintf(DEBUG_INFO, "ClistBox::restoreScreen:\n");
	
	if(background) 
	{
		if(savescreen)
			frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, wanted_width, wanted_height, background);
	}
}

void ClistBox::enableSaveScreen()
{
	dprintf(DEBUG_INFO, "ClistBox::enableSaveScreen:\n");
	
	savescreen = true;
	
	if(!savescreen && background) 
	{
		delete[] background;
		background = NULL;
	}
	
	saveScreen();
}

void ClistBox::hide()
{
	dprintf(DEBUG_INFO, "ClistBox::hide: (%s)\n", l_name.c_str());

	if( savescreen && background)
		restoreScreen();
	else
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, wanted_width, wanted_height);
		//frameBuffer->paintBackgroundBoxRel(itemBox.iX - 2, itemBox.iY - 2, full_width + 4, full_height + 4);
		
	//
	hideItemInfo(); 

	//
	if(textBox != NULL)
	{
		delete textBox;
		textBox = NULL;
	}
	
	//
	if (items_background)
	{
		delete [] items_background;
		items_background = NULL;
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
						paintItems();
						//paint();
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
						paintItems();
						//paint();
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
						paintItems();
						//paint();
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
							paintItems();
							//paint();
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
							paintItems();
							//paint();
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
void ClistBox::swipLeft()
{
	dprintf(DEBUG_INFO, "ClistBox::swipLeft:\n");

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
						paintItems();
					}
								
					break;
				}
			}
		}
	}
	else if (widgetType == TYPE_EXTENDED)
	{
		if(textBox)
			textBox->scrollPageUp(1);
	}
	else if(widgetType == TYPE_STANDARD)
	{
		if(widgetMode == MODE_SETUP)
		{
			if(hasItem()) 
			{
				if((items[selected]->can_arrow)) 
				{
					items[selected]->msg = RC_left;
					
					parent->onOKKeyPressed(RC_left);
				}
			} 
		}
	}
}

//
void ClistBox::swipRight()
{
	dprintf(DEBUG_INFO, "ClistBox::swipRight:\n");

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
						paintItems();
					}
								
					break;
				}
			}
		}
	}
	else if (widgetType == TYPE_EXTENDED)
	{
		if(textBox)
			textBox->scrollPageDown(1);
	}
	else if(widgetType == TYPE_STANDARD)
	{
		if(widgetMode == MODE_SETUP)
		{
			if(hasItem()) 
			{
				if((items[selected]->can_arrow)) 
				{
					items[selected]->msg = RC_right;
					
					parent->onOKKeyPressed(RC_right);
				}
			} 
		}
	}
}

//
int ClistBox::oKKeyPressed(CMenuTarget* _parent, neutrino_msg_t _msg)
{
	dprintf(DEBUG_INFO, "ClistBox::okKeyPressed:\n");
	
	int ret = RETURN_EXIT;

	if (hasItem() && selected >= 0 && items[selected]->isSelectable())
	{
		actionKey = items[selected]->actionKey;
		items[selected]->msg = _msg;
	}	

	ret = items[selected]->exec(_parent);
	
	return ret;
}

//
void ClistBox::directKeyPressed(neutrino_msg_t _msg)
{
	dprintf(DEBUG_DEBUG, "ClistBox::directKeyPressed: 0x%x\n", _msg);
	
	// 
	for (unsigned int i = 0; i < items.size(); i++) 
	{
		CMenuItem * titem = items[i];
			
		if ((titem->directKey != RC_nokey) && (titem->directKey == _msg)) 
		{
			if (titem->isSelectable()) 
			{
				items[selected]->paint(false);
				selected = i;

				if (selected > (int)page_start[current_page + 1] || selected < (int)page_start[current_page]) 
				{
					// different page
					paintItems();
				}

				paintItemInfo(selected);
				pos = selected;
				
				titem->paint(true);
				//
				parent->onOKKeyPressed(_msg);
			} 
			break;
		}
	}
}

//
void ClistBox::changeWidgetType()
{
	dprintf(DEBUG_NORMAL, "ClistBox::changeWidgetType:\n");

	if(widget.size())
	{
		hide();

		cnt++;

		if(cnt >= (int)widget.size())
		{
			cnt = 0;
		}
			
		widgetType = widget[cnt];

		paint();
	}
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
			neutrino_msg_t dk = (shortcut != RC_nokey) ? CRCInput::convertDigitToKey(sc++) : RC_nokey;

			//FIXME: iconName
			ClistBoxItem *fw_plugin = new ClistBoxItem(g_PluginList->getName(count), enabled, NULL, CPluginsExec::getInstance(), g_PluginList->getFileName(count), dk, NULL, IconName.c_str());

			fw_plugin->setHint(g_PluginList->getDescription(count).c_str());
			fw_plugin->setWidgetType(itype);
			if (i2lines) fw_plugin->set2lines();
			fw_plugin->setBorderMode(iBorder);
			
			fw_plugin->isPlugin = true;
			
			addItem(fw_plugin);
		}
	}
}

