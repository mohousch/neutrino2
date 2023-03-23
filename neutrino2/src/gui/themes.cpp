/*
	$port: themes.cpp 21.09.21 tuxbox-cvs Exp $
	
	Neutrino-GUI  -   DBoxII-Project

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

	Copyright (C) 2007, 2008, 2009 (flasher) Frank Liebelt

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#include <global.h>
#include <neutrino2.h>

#include <system/setting_helpers.h>
#include <system/debug.h>

#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <sys/stat.h>
#include <sys/time.h>

#include <gui/themes.h>


#define THEMEDIR 		DATADIR "/themes/"
#define USERDIR 		CONFIGDIR "/themes/"
#define FILE_PREFIX 		".theme"

CThemes::CThemes()
: themefile('\t')
{
	notifier = NULL;
}

int CThemes::exec(CMenuTarget * parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CThemes::exec: actionKey:%s\n", actionKey.c_str());

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	if( !actionKey.empty() )
	{
		if(actionKey == "saveCurrentTheme")
		{
			if (MessageBox(_("Information"), _("Save current theme"), mbrNo, mbYes | mbNo, NULL, 600, 30, true) == mbrYes) 
			{
				std::string file_name = "";
				CStringInputSMS * nameInput = new CStringInputSMS(_("Theme name"), file_name.c_str());

				nameInput->exec(NULL, "");
				
				//
				if (!nameInput->getExitPressed())
				{
					HintBox(_("Save current theme"), _("Saving current theme!"));
					
					saveFile((char*)((std::string)USERDIR + nameInput->getString().c_str() + FILE_PREFIX).c_str());
				}

				file_name.clear();

				delete nameInput;
				nameInput = NULL;
			}

			return res;
		}
		else if (actionKey == "theme_default")
		{
			setupDefaultColors();
			
			CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
			CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
			
			//
			if (g_settings.preferred_skin != "neutrino2")
			{
				if (MessageBox(_("Information"), _("this need Neutrino restart\ndo you really want to restart?"), mbrNo, mbYes | mbNo, NULL, 600, 30, true) == mbrYes) 
				{
					CNeutrinoApp::getInstance()->exec(NULL, "restart");
				}
			}

			return res;
		}
		else
		{
			std::string themeFile = actionKey;
			
			if ( strstr(themeFile.c_str(), "{U}") != 0 ) 
			{
				themeFile.erase(0, 3);
				readFile((char*)((std::string)USERDIR + themeFile + FILE_PREFIX).c_str());
			} 
			else
				readFile((char*)((std::string)THEMEDIR + themeFile + FILE_PREFIX).c_str());
				
			CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
			CNeutrinoApp::getInstance()->exec(NULL, "saveskinsettings");
			
			//
			if (g_settings.preferred_skin != "neutrino2")
			{
				if (MessageBox(_("Information"), _("this need Neutrino restart\ndo you really want to restart?"), mbrNo, mbYes | mbNo, NULL, 600, 30, true) == mbrYes) 
				{
					CNeutrinoApp::getInstance()->exec(NULL, "restart");
				}
			}

			return res;
		}
	}

	return Show();
}

void CThemes::readThemes(ClistBox* themes)
{
	struct dirent **themelist;
	int n;
	const char *pfade[] = {THEMEDIR, USERDIR};
	bool hasCVSThemes, hasUserThemes;
	hasCVSThemes = hasUserThemes = false;
	std::string userThemeFile = "";
	ClistBoxItem* oj;

	for(int p = 0; p < 2; p++)
	{
		n = scandir(pfade[p], &themelist, 0, alphasort);
		if(n < 0)
			perror("loading themes: scandir");
		else
		{
			for(int count = 0; count < n; count++)
			{
				char *file = themelist[count]->d_name;
				char *pos = strstr(file, ".theme");

				if(pos != NULL)
				{
					if ( p == 0 && hasCVSThemes == false ) 
					{
						//themes->addItem(new CMenuSeparator(LINE | STRING, _("Select theme")));
						hasCVSThemes = true;
					} 
					else if ( p == 1 && hasUserThemes == false ) 
					{
						//themes->addItem(new CMenuSeparator(LINE | STRING, _("Select theme")));
						hasUserThemes = true;
					}
					
					*pos = '\0';
					if ( p == 1 ) 
					{
						userThemeFile = "{U}" + (std::string)file;
						oj = new ClistBoxItem((char*)file, true, "", this, userThemeFile.c_str());
					} 
					else
						oj = new ClistBoxItem((char*)file, true, "", this, file);
					
					themes->addItem( oj );
				}
				free(themelist[count]);
			}
			free(themelist);
		}
	}
}

int CThemes::Show()
{
	dprintf(DEBUG_NORMAL, "CThemes::Show:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* themes = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("themesetup");
	
	if (widget)
	{
		themes = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		themes = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		themes->setWidgetMode(MODE_SETUP);
		themes->enableShrinkMenu();
		
		themes->enablePaintHead();
		themes->setTitle(_("Themes"), NEUTRINO_ICON_COLORS);

		themes->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		themes->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "themessetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(themes);
	}
	
	themes->clearItems();

	// intros
	themes->addItem(new ClistBoxItem(_("back")));
	themes->addItem( new CMenuSeparator(LINE) );

	// save current theme
	themes->addItem(new ClistBoxItem(_("Save current theme"), true , NULL, this, "saveCurrentTheme", RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	//set default theme
	themes->addItem( new CMenuSeparator(LINE) );
	themes->addItem(new ClistBoxItem(_("Neutrino2"), true, NULL, this, "theme_default"));

	//	
	readThemes(themes);

	int res = widget->exec(NULL, "");
	
	return res;
}

void CThemes::readFile(const char* themename)
{
	dprintf(DEBUG_NORMAL, "CThemes::readFile: %s\n", themename);
	
	if(themefile.loadConfig(themename))
	{
		g_settings.menu_Head_alpha = themefile.getInt32( "menu_Head_alpha", 15);
		g_settings.menu_Head_red = themefile.getInt32( "menu_Head_red", 15);
		g_settings.menu_Head_green = themefile.getInt32( "menu_Head_green", 15);
		g_settings.menu_Head_blue = themefile.getInt32( "menu_Head_blue", 15);

		g_settings.menu_Head_Text_alpha = themefile.getInt32( "menu_Head_Text_alpha", 0);
		g_settings.menu_Head_Text_red = themefile.getInt32( "menu_Head_Text_red", 100 );
		g_settings.menu_Head_Text_green = themefile.getInt32( "menu_Head_Text_green", 100 );
		g_settings.menu_Head_Text_blue = themefile.getInt32( "menu_Head_Text_blue", 100 );
	
		g_settings.menu_Content_alpha = themefile.getInt32( "menu_Content_alpha", 20);
		g_settings.menu_Content_red = themefile.getInt32( "menu_Content_red", 25);
		g_settings.menu_Content_green = themefile.getInt32( "menu_Content_green", 25);
		g_settings.menu_Content_blue = themefile.getInt32( "menu_Content_blue", 25);
		
		g_settings.menu_Content_Text_alpha = themefile.getInt32( "menu_Content_Text_alpha", 0);
		g_settings.menu_Content_Text_red = themefile.getInt32( "menu_Content_Text_red", 85 );
		g_settings.menu_Content_Text_green = themefile.getInt32( "menu_Content_Text_green", 85 );
		g_settings.menu_Content_Text_blue = themefile.getInt32( "menu_Content_Text_blue", 85 );
	
		g_settings.menu_Content_Selected_alpha = themefile.getInt32( "menu_Content_Selected_alpha", 20);
		g_settings.menu_Content_Selected_red = themefile.getInt32( "menu_Content_Selected_red", 75);
		g_settings.menu_Content_Selected_green = themefile.getInt32( "menu_Content_Selected_green", 75);
		g_settings.menu_Content_Selected_blue = themefile.getInt32( "menu_Content_Selected_blue", 75);
		
		g_settings.menu_Content_Selected_Text_alpha = themefile.getInt32( "menu_Content_Selected_Text_alpha", 0);
		g_settings.menu_Content_Selected_Text_red = themefile.getInt32( "menu_Content_Selected_Text_red", 25 );
		g_settings.menu_Content_Selected_Text_green = themefile.getInt32( "menu_Content_Selected_Text_green", 25 );
		g_settings.menu_Content_Selected_Text_blue = themefile.getInt32( "menu_Content_Selected_Text_blue", 25 );
	
		g_settings.menu_Content_inactive_alpha = themefile.getInt32( "menu_Content_inactive_alpha", 20);
		g_settings.menu_Content_inactive_red = themefile.getInt32( "menu_Content_inactive_red", 25);
		g_settings.menu_Content_inactive_green = themefile.getInt32( "menu_Content_inactive_green", 25);
		g_settings.menu_Content_inactive_blue = themefile.getInt32( "menu_Content_inactive_blue", 25);
		
		g_settings.menu_Content_inactive_Text_alpha = themefile.getInt32( "menu_Content_inactive_Text_alpha", 0);
		g_settings.menu_Content_inactive_Text_red = themefile.getInt32( "menu_Content_inactive_Text_red", 55);
		g_settings.menu_Content_inactive_Text_green = themefile.getInt32( "menu_Content_inactive_Text_green", 55);
		g_settings.menu_Content_inactive_Text_blue = themefile.getInt32( "menu_Content_inactive_Text_blue", 55);

		g_settings.infobar_alpha = themefile.getInt32( "infobar_alpha", 20);
		g_settings.infobar_red = themefile.getInt32( "infobar_red", 25);
		g_settings.infobar_green = themefile.getInt32( "infobar_green", 25);
		g_settings.infobar_blue = themefile.getInt32( "infobar_blue", 25);
		
		g_settings.infobar_Text_alpha = themefile.getInt32( "infobar_Text_alpha", 0);
		g_settings.infobar_Text_red = themefile.getInt32( "infobar_Text_red", 100);
		g_settings.infobar_Text_green = themefile.getInt32( "infobar_Text_green", 100);
		g_settings.infobar_Text_blue = themefile.getInt32( "infobar_Text_blue", 100);
		
		g_settings.infobar_colored_events_alpha = themefile.getInt32( "infobar_colored_events_alpha", 0);
		g_settings.infobar_colored_events_red = themefile.getInt32( "infobar_colored_events_red", 95);
		g_settings.infobar_colored_events_green = themefile.getInt32( "infobar_colored_events_green", 70);
		g_settings.infobar_colored_events_blue = themefile.getInt32( "infobar_colored_events_blue", 0);
	
		g_settings.menu_Foot_alpha = themefile.getInt32( "menu_Foot_alpha", 15);
		g_settings.menu_Foot_red = themefile.getInt32( "menu_Foot_red", 15);
		g_settings.menu_Foot_green = themefile.getInt32( "menu_Foot_green", 15);
		g_settings.menu_Foot_blue = themefile.getInt32( "menu_Foot_blue", 15);
		
		g_settings.menu_Foot_Text_alpha = themefile.getInt32( "menu_Foot_Text_alpha", 0);
		g_settings.menu_Foot_Text_red = themefile.getInt32( "menu_Foot_Text_red", 100);
		g_settings.menu_Foot_Text_green = themefile.getInt32( "menu_Foot_Text_green", 100);
		g_settings.menu_Foot_Text_blue = themefile.getInt32( "menu_Foot_Text_blue", 100);

		g_settings.menu_Hint_alpha = themefile.getInt32( "menu_Hint_alpha", 20);
		g_settings.menu_Hint_red = themefile.getInt32( "menu_Hint_red", 25);
		g_settings.menu_Hint_green = themefile.getInt32( "menu_Hint_green", 25);
		g_settings.menu_Hint_blue = themefile.getInt32( "menu_Hint_blue", 25);
		
		g_settings.menu_Hint_Text_alpha = themefile.getInt32( "menu_Hint_Text_alpha", 0);
		g_settings.menu_Hint_Text_red = themefile.getInt32( "menu_Hint_Text_red", 85);
		g_settings.menu_Hint_Text_green = themefile.getInt32( "menu_Hint_Text_green", 85);
		g_settings.menu_Hint_Text_blue = themefile.getInt32( "menu_Hint_Text_blue", 85);

		notifier = new CColorSetupNotifier;
		notifier->changeNotify("", NULL);
		
		delete notifier;
		notifier = NULL;
	}
	else
		printf("[neutrino theme] %s not found\n", themename);
}

void CThemes::saveFile(const char * themename)
{
	dprintf(DEBUG_NORMAL, "CThemes::saveFile: %s\n", themename);
	
	themefile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	themefile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	themefile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	themefile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );
	themefile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	themefile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	themefile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	themefile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	themefile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	themefile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	themefile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	themefile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );
	themefile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	themefile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	themefile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	themefile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	themefile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	themefile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	themefile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	themefile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );
	themefile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	themefile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	themefile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	themefile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	themefile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	themefile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	themefile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	themefile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );
	themefile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	themefile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	themefile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	themefile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	themefile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	themefile.setInt32( "infobar_red", g_settings.infobar_red );
	themefile.setInt32( "infobar_green", g_settings.infobar_green );
	themefile.setInt32( "infobar_blue", g_settings.infobar_blue );
	themefile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	themefile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	themefile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	themefile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );
	
	themefile.setInt32( "infobar_colored_events_alpha", g_settings.infobar_colored_events_alpha );
	themefile.setInt32( "infobar_colored_events_red", g_settings.infobar_colored_events_red );
	themefile.setInt32( "infobar_colored_events_green", g_settings.infobar_colored_events_green );
	themefile.setInt32( "infobar_colored_events_blue", g_settings.infobar_colored_events_blue );
	
	themefile.setInt32( "menu_Foot_alpha", g_settings.menu_Foot_alpha );
	themefile.setInt32( "menu_Foot_red", g_settings.menu_Foot_red );
	themefile.setInt32( "menu_Foot_green", g_settings.menu_Foot_green );
	themefile.setInt32( "menu_Foot_blue", g_settings.menu_Foot_blue );
	themefile.setInt32( "menu_Foot_Text_alpha", g_settings.menu_Foot_Text_alpha );
	themefile.setInt32( "menu_Foot_Text_red", g_settings.menu_Foot_Text_red );
	themefile.setInt32( "menu_Foot_Text_green", g_settings.menu_Foot_Text_green );
	themefile.setInt32( "menu_Foot_Text_blue", g_settings.menu_Foot_Text_blue );

	themefile.setInt32( "menu_Hint_alpha", g_settings.menu_Hint_alpha );
	themefile.setInt32( "menu_Hint_red", g_settings.menu_Hint_red );
	themefile.setInt32( "menu_Hint_green", g_settings.menu_Hint_green );
	themefile.setInt32( "menu_Hint_blue", g_settings.menu_Hint_blue );
	themefile.setInt32( "menu_Hint_Text_alpha", g_settings.menu_Hint_Text_alpha );
	themefile.setInt32( "menu_Hint_Text_red", g_settings.menu_Hint_Text_red );
	themefile.setInt32( "menu_Hint_Text_green", g_settings.menu_Hint_Text_green );
	themefile.setInt32( "menu_Hint_Text_blue", g_settings.menu_Hint_Text_blue );

	if (!themefile.saveConfig(themename))
		printf("[neutrino theme] %s write error\n", themename);
}



// setup default Colors
void CThemes::setupDefaultColors()
{
	dprintf(DEBUG_NORMAL, "CThemes::setupDefaultColors\n");
	
	g_settings.menu_Head_alpha = 15;
	g_settings.menu_Head_red = 15;
	g_settings.menu_Head_green = 15;
	g_settings.menu_Head_blue = 15;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue = 100;
	
	g_settings.menu_Content_alpha = 0;
	g_settings.menu_Content_red = 15;
	g_settings.menu_Content_green = 15;
	g_settings.menu_Content_blue = 15;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red = 85;
	g_settings.menu_Content_Text_green = 85;
	g_settings.menu_Content_Text_blue = 85;
	
	g_settings.menu_Content_Selected_alpha = 20;
	g_settings.menu_Content_Selected_red = 75;
	g_settings.menu_Content_Selected_green = 75;
	g_settings.menu_Content_Selected_blue = 75;

	g_settings.menu_Content_Selected_Text_alpha = 0;
	g_settings.menu_Content_Selected_Text_red = 25;
	g_settings.menu_Content_Selected_Text_green = 25;
	g_settings.menu_Content_Selected_Text_blue = 25;
	
	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red = 25;
	g_settings.menu_Content_inactive_green = 25;
	g_settings.menu_Content_inactive_blue = 25;

	g_settings.menu_Content_inactive_Text_alpha = 0;
	g_settings.menu_Content_inactive_Text_red = 55;
	g_settings.menu_Content_inactive_Text_green = 55;
	g_settings.menu_Content_inactive_Text_blue = 55;

	g_settings.infobar_alpha = 15;
	g_settings.infobar_red = 15;
	g_settings.infobar_green = 15;
	g_settings.infobar_blue = 15;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red = 85;
	g_settings.infobar_Text_green = 85;
	g_settings.infobar_Text_blue = 85;
		
	g_settings.infobar_colored_events_alpha = 0;
	g_settings.infobar_colored_events_red = 95;
	g_settings.infobar_colored_events_green = 70;
	g_settings.infobar_colored_events_blue = 0;
	
	g_settings.menu_Foot_alpha = 15;
	g_settings.menu_Foot_red = 15;
	g_settings.menu_Foot_green = 15;
	g_settings.menu_Foot_blue = 15;
		
	g_settings.menu_Foot_Text_alpha = 0;
	g_settings.menu_Foot_Text_red = 100;
	g_settings.menu_Foot_Text_green = 100;
	g_settings.menu_Foot_Text_blue = 100;

	g_settings.menu_Hint_alpha = 0;
	g_settings.menu_Hint_red = 15;
	g_settings.menu_Hint_green = 15;
	g_settings.menu_Hint_blue = 15;
		
	g_settings.menu_Hint_Text_alpha = 0;
	g_settings.menu_Hint_Text_red = 85;
	g_settings.menu_Hint_Text_green = 85;
	g_settings.menu_Hint_Text_blue = 85;
	
	notifier = new CColorSetupNotifier();
	notifier->changeNotify("", NULL);
	delete notifier;
	notifier = NULL;
}

