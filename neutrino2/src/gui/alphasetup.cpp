/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: alphasetup.cpp 2013/10/12 mohousch Exp $

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

#include <gui/alphasetup.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/widget/messagebox.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>


CAlphaSetup::CAlphaSetup(const char* const Name, unsigned char * Alpha, CChangeObserver * Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	mainWindow = NULL;

	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	mainBox.iWidth = MENU_WIDTH;
	mainBox.iHeight = hheight + mheight*2;

	mainBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - mainBox.iWidth) >> 1);
	mainBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - mainBox.iHeight) >> 1);

	//mainWindow.setPosition(&mainBox);
	mainWindow = new CCWindow(&mainBox);

	observer = Observer;
	name = Name? Name : "";

	alpha = Alpha;

	frameBuffer->setBlendLevel(*alpha);
}

CAlphaSetup::~CAlphaSetup()
{
}

int CAlphaSetup::exec(CMenuTarget * parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CAlphaSetup::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = CMenuTarget::RETURN_REPAINT;
	
	if (parent)
		parent->hide();

	unsigned char alpha_alt = *alpha;

	frameBuffer->setBlendLevel(*alpha);
	
	paint();

	int selected = 0;
	int max = 0;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

		switch ( msg )
		{
			case CRCInput::RC_down:
				{
					if(selected < max)
					{
						paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2, false);
						
						selected++;

						if(selected == 0)
							paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2RED, true );
					}
					break;
				}
				
			case CRCInput::RC_up:
				{
					if (selected > 0)
					{
						paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2, false);

						selected--;

						if(selected == 0)
							paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2RED, true );
					}
					break;
				}

			case CRCInput::RC_right:
				{
					if(selected == 0)
					{
						if (*alpha < 255) 
						{
							if ( *alpha < 250 )
								*alpha += 5;
							else
								*alpha = 255;
								
							paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2RED, true );
							frameBuffer->setBlendLevel(*alpha);
						}
					
					}
					break;
				}
				
			case CRCInput::RC_left:
				{
					if(selected == 0)
					{
						if (*alpha > 0) 
						{
							if (* alpha > 5)
								*alpha -= 5;
							else
								*alpha = 0;
								
							paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2RED, true );
							frameBuffer->setBlendLevel(*alpha);
						}
					}
					break;
				}
					
			case CRCInput::RC_home:
				if ( *alpha != alpha_alt)
				{
					if (MessageBox(_("Alpha Setup"), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel)
					{
						// FIXME:
						break;
					}
				}

				// sonst abbruch...
				*alpha = alpha_alt;

			case CRCInput::RC_timeout:
			case CRCInput::RC_ok:
				loop = false;
				break;

			default:
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = CMenuTarget::RETURN_EXIT_ALL;
				}
		}
		
		// blit
		frameBuffer->blit();
	}

	hide();

	if(observer)
		observer->changeNotify(name, NULL);

	return res;
}

void CAlphaSetup::hide()
{
	mainWindow->hide();
	
	// blit
	frameBuffer->blit();
}

void CAlphaSetup::paint()
{
	dprintf(DEBUG_NORMAL, "CAlphaSetup::paint\n");

	// main window
	mainWindow->setColor(COL_MENUCONTENT_PLUS_0);
	mainWindow->setCorner(RADIUS_MID, CORNER_BOTTOM);
	mainWindow->paint();

	// head
	CCHeaders headers(mainBox.iX, mainBox.iY, mainBox.iWidth, hheight, name.c_str(), NEUTRINO_ICON_COLORS);
	headers.setCorner(g_settings.Head_radius);
	headers.setGradient(g_settings.Head_gradient);
	headers.setLine(g_settings.Head_line, g_settings.Head_line_gradient);
	headers.paint();

	// slider
	paintSlider(mainBox.iX + BORDER_LEFT, mainBox.iY + hheight, alpha, _("alpha"), NEUTRINO_ICON_VOLUMESLIDER2RED, true );
}

void CAlphaSetup::paintSlider(const int _x, const int _y, const unsigned char * const spos, const char* const text, const char * const iconname, const bool /*selected*/) // UTF-8
{
	if (!spos)
		return;

	int sspos = (*spos)*100/255;
	char wert[5];
	
	int icon_w = 120;
	int icon_h = 11;
	
	frameBuffer->getIconSize(NEUTRINO_ICON_VOLUMEBODY, &icon_w, &icon_h);
	
	int slider_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("100", true); //UTF-8
	
	int startx = mainBox.iWidth - icon_w - slider_w - 50;

	frameBuffer->paintBoxRel(_x + startx, _y, 120, mheight, COL_MENUCONTENT_PLUS_0);

	frameBuffer->paintIcon(NEUTRINO_ICON_VOLUMEBODY, _x + startx, _y + 2 + mheight / 4);
	frameBuffer->paintIcon(iconname, _x + startx + 3 + sspos, _y + mheight / 4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + mheight, mainBox.iWidth, text, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
	
	sprintf(wert, "%3d", sspos); // UTF-8 encoded

	frameBuffer->paintBoxRel(_x + startx + 120 + 10, _y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x + startx + 120 + 10, _y + mheight, mainBox.iWidth, wert, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

