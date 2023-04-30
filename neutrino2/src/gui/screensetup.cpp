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

#include <gui/screensetup.h>

#include <gui/widget/messagebox.h>

#include <driver/color.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <system/settings.h>
#include <system/debug.h>

#include <global.h>
#include <neutrino2.h>


CScreenSetup::CScreenSetup()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	
	//
	x = 15*5;
	y = 15*25;
	BoxHeight = 15*4;
	BoxWidth = 15*27;
}

int CScreenSetup::exec(CMenuTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CScreenSetup::exec\n");
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	x_coord[0] = g_settings.screen_StartX;
	x_coord[1] = g_settings.screen_EndX;
	y_coord[0] = g_settings.screen_StartY;
	y_coord[1] = g_settings.screen_EndY;

	paint();
	
	frameBuffer->blit();

	selected = 0;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

		switch ( msg )
		{
			case RC_ok:
				// save
				g_settings.screen_StartX = x_coord[0];
				g_settings.screen_EndX = x_coord[1];
				g_settings.screen_StartY = y_coord[0];
				g_settings.screen_EndY = y_coord[1];

				CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);

				loop = false;
				break;

			case RC_home:
				if ( ( ( g_settings.screen_StartX != x_coord[0] ) || ( g_settings.screen_EndX != x_coord[1] ) || ( g_settings.screen_StartY != y_coord[0] ) || ( g_settings.screen_EndY != y_coord[1] ) ) &&
						(MessageBox(_("Screen Setup"), _("Discard changes?"), mbrYes, mbYes | mbCancel) == mbrCancel))
					break;

			case RC_timeout:
				loop = false;
				break;

			case RC_red:
			case RC_green:
				{
					selected = ( msg == RC_green ) ? 1 : 0 ;

					frameBuffer->paintBoxRel(x, y, BoxWidth, BoxHeight/2, (selected == 0)? COL_MENUCONTENTSELECTED_PLUS_0 : COL_MENUCONTENT_PLUS_0);
					frameBuffer->paintBoxRel(x, y + BoxHeight/2,  BoxWidth, BoxHeight/2, (selected == 1)? COL_MENUCONTENTSELECTED_PLUS_0 : COL_MENUCONTENT_PLUS_0);

					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 30, y + BoxHeight/2, BoxWidth, _("Upper Left"), (selected == 0)?COL_MENUCONTENTSELECTED : COL_MENUCONTENT, 0, true); // UTF-8
					g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 30, y + BoxHeight, BoxWidth, _("Lower Right"), (selected == 1)?COL_MENUCONTENTSELECTED : COL_MENUCONTENT, 0, true); // UTF-8

					paintIcons();
					break;
				}
				
			case RC_up:
				{
					y_coord[selected]--;

					int min = ( selected == 0 ) ? 0 : 400;
					if ( y_coord[selected] < min )
						y_coord[selected] = min ;
					else
						paintBorder( selected );
					break;
				}
				
			case RC_down:
				{
					y_coord[selected]++;

					int max = ( selected == 0 ) ? 200 : frameBuffer->getScreenHeight(true);
					
					if ( y_coord[selected] > max )
						y_coord[selected] = max ;
					else
						paintBorder( selected );
					break;
				}
				
			case RC_left:
				{
					x_coord[selected]--;

					int min = ( selected == 0 ) ? 0 : 400;
					if ( x_coord[selected] < min )
						x_coord[selected] = min ;
					else
						paintBorder( selected );
					break;
				}
				
			case RC_right:
				{
					x_coord[selected]++;

					int max = ( selected == 0 ) ? 200 : frameBuffer->getScreenWidth(true);
					
					if ( x_coord[selected] > max )
						x_coord[selected] = max ;
					else
						paintBorder( selected );
					break;
				}

			default:
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = RETURN_EXIT_ALL;
				}
		}

		frameBuffer->blit();	
	}

	hide();
	
	return res;
}

void CScreenSetup::hide()
{
	frameBuffer->paintBackgroundBox(0, 0, frameBuffer->getScreenWidth(true), frameBuffer->getScreenHeight(true));

	frameBuffer->blit();
}

void CScreenSetup::paintBorder( int _selected )
{
	if ( _selected == 0 )
		paintBorderUL();
	else
		paintBorderLR();

	paintCoords();
}

void CScreenSetup::paintIcons()
{
	// red
        frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, x + BORDER_LEFT/2, y + (BoxHeight/2 - icon_h)/2);
	
	// green
        frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, x + BORDER_LEFT/2, y + BoxHeight/2 + (BoxHeight/2 - icon_h)/2);
}

void CScreenSetup::paintBorderUL()
{
	frameBuffer->paintIconRaw(NEUTRINO_ICON_BORDER_UL, x_coord[0], y_coord[0] );
}

void CScreenSetup::paintBorderLR()
{
	frameBuffer->paintIconRaw(NEUTRINO_ICON_BORDER_LR, x_coord[1] - 96, y_coord[1] - 96 );
}

void CScreenSetup::paintCoords()
{
	int w = 15*9;
	int h = 15*6;

	int x1 = (frameBuffer->getScreenWidth(true) - w) / 2;
	int y1 = (frameBuffer->getScreenHeight(true) - h) / 2;

	frameBuffer->paintBoxRel(x1, y1, 15*9, 15*6, COL_MENUCONTENT_PLUS_0);

	char xpos[30];
	char ypos[30];
	char xepos[30];
	char yepos[30];

	sprintf((char*) &xpos, "SX: %d", x_coord[0] );
	sprintf((char*) &ypos, "SY: %d", y_coord[0] );
	sprintf((char*) &xepos, "EX: %d", x_coord[1] );
	sprintf((char*) &yepos, "EY: %d", y_coord[1] );

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x1 + BORDER_LEFT, y1 + 30, 200, xpos, COL_MENUCONTENT);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x1 + BORDER_LEFT, y1 + 50, 200, ypos, COL_MENUCONTENT);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x1 + BORDER_LEFT, y1 + 70, 200, xepos, COL_MENUCONTENT);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x1 + BORDER_LEFT, y1 + 90, 200, yepos, COL_MENUCONTENT);
}

void CScreenSetup::paint()
{
	dprintf(DEBUG_DEBUG, "CScreenSetup::paint\n");

	if (!frameBuffer->getActive())
		return;

	int w = (int) frameBuffer->getScreenWidth(true);
	int h = (int) frameBuffer->getScreenHeight(true);

	// box(background)
	frameBuffer->paintBox(0, 0, w, h, /*make16color(0xA0A0A0)*/ COL_NOBEL_PLUS_0 );

	// hlines grid
	for(int count = 0; count < h; count += 15)
		frameBuffer->paintHLine(0, w - 1, count, COL_MATTERHORN_PLUS_0 );

	// vlines grid
	for(int count = 0; count < w; count += 15)
		frameBuffer->paintVLine(count, 0, h - 1, COL_MATTERHORN_PLUS_0 ); //matterhorn

	// box top left
	frameBuffer->paintBox(0, 0, w/3, h/3, COL_NOBEL_PLUS_0 );
	
	// box buttom right
	frameBuffer->paintBox(w - w/3, h - h/3, w - 1, h - 1, COL_NOBEL_PLUS_0 );

	//upper selected box
        frameBuffer->paintBoxRel(x, y, BoxWidth, BoxHeight/2, COL_MENUCONTENTSELECTED_PLUS_0);
        
        //lower selected box
        frameBuffer->paintBoxRel(x, y + BoxHeight/2, BoxWidth, BoxHeight/2, COL_MENUCONTENT_PLUS_0);

	// upper
        g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 30, y+BoxHeight/2, BoxWidth, _("Upper Left"), COL_MENUCONTENTSELECTED , 0, true); // UTF-8
        
        // lower
        g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + 30, y+BoxHeight, BoxWidth, _("Lower Right"), COL_MENUCONTENT, 0, true); // UTF-8

	paintIcons();
	paintBorderUL();
	paintBorderLR();
	paintCoords();
}

