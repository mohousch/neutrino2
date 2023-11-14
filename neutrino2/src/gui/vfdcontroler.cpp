/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: vfdcontroller.cpp 2013/10/12 mohousch Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vfdcontroler.h>

#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/color.h>

#include <driver/rcinput.h>

#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino2.h>

#include <math.h>

#include <system/debug.h>

#if defined(ENABLE_LCD) && (defined(PLATFORM_DREAMBOX) || defined(PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS))
#define BRIGHTNESSFACTOR 2.55 // 0 - 255
#else
#define BRIGHTNESSFACTOR 0.07 // 0 - 7
#endif


CVfdControler::CVfdControler(const char* const Name, CChangeObserver* Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	mainWindow = NULL;

	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	observer = Observer;

	name = Name? Name : "";

	width = MENU_WIDTH;
	height = hheight+ mheight*3 + mheight/2;
	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width) >> 1);
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height)>> 1);

	mainWindow = new CCWindow(x, y, width, height);

	brightness = CVFD::getInstance()->getBrightness();
	brightnessstandby = CVFD::getInstance()->getBrightnessStandby();
}

void CVfdControler::setVfd()
{
	CVFD::getInstance()->setBrightness(brightness);
	CVFD::getInstance()->setBrightnessStandby(brightnessstandby);
}

int CVfdControler::exec(CMenuTarget* parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CVfdControler::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int selected, res = CMenuTarget::RETURN_REPAINT;
	unsigned int brightness_alt, brightnessstandby_alt;

	if (parent)
		parent->hide();
	
	brightness_alt = CVFD::getInstance()->getBrightness();
	brightnessstandby_alt = CVFD::getInstance()->getBrightnessStandby();
	selected = 0;

	setVfd();
	paint();
	frameBuffer->blit();

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

		switch ( msg )
		{
			case CRCInput::RC_down:
			if(selected < 2) // max entries
			{
				paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), false);
				paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);
				selected++;
				
				switch (selected) 
				{
					case 0:
						paintSlider(x + BORDER_LEFT, y+ hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
						break;
						
					case 1:
						paintSlider(x + BORDER_LEFT, y+ hheight+ mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);

						CVFD::getInstance()->setMode(CVFD::MODE_STANDBY);
						break;
						
					case 2:
						frameBuffer->paintBoxRel(x, y + hheight + mheight*2 + mheight/2, width, mheight, COL_MENUCONTENTSELECTED_PLUS_0);

						g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*3 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENTSELECTED_TEXT_PLUS_0, 0, true); // UTF-8
						break;
				}
			}
			break;

			case CRCInput::RC_up:
			if (selected > 0) 
			{
				paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), false);

				paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);
				selected--;
				switch (selected) 
				{
					case 0:
						paintSlider(x + BORDER_LEFT, y+ hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);

						CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
						break;
						
					case 1:
						paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);

						CVFD::getInstance()->setMode(CVFD::MODE_STANDBY);

						frameBuffer->paintBoxRel(x, y + hheight + mheight*2 + mheight/2, width, mheight, COL_MENUCONTENT_PLUS_0);

						g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*3 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
						break;
						
					case 2:
						break;
				}
			}
			break;

			case CRCInput::RC_right:
				switch (selected) 
				{
					case 0:
						if (brightness < DEFAULT_LCD_BRIGHTNESS) 
						{
							brightness ++;
							
							paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
							setVfd();
						}
						break;
						
					case 1:
						if (brightnessstandby < DEFAULT_LCD_STANDBYBRIGHTNESS) 
						{
							brightnessstandby ++;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);
							setVfd();
						}
						break;
				}
				break;

			case CRCInput::RC_left:
				switch (selected) 
				{
					case 0:
						if (brightness > 0) 
						{
							brightness--;
							
							paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
							setVfd();
						}
						break;
						
					case 1:
						if (brightnessstandby > 0) 
						{
							brightnessstandby--;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);
							setVfd();
						}
						break;
				}
				break;

			case CRCInput::RC_home:
				if ( ((brightness != brightness_alt) || (brightnessstandby != brightnessstandby_alt) ) && (MessageBox(name.c_str(), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
					break;

				brightness = brightness_alt;
				brightnessstandby = brightnessstandby_alt;
				setVfd();
				loop = false;
				break;

			case CRCInput::RC_ok:
				if (selected == 2) 
				{
					brightness = DEFAULT_LCD_BRIGHTNESS;
					brightnessstandby = DEFAULT_LCD_STANDBYBRIGHTNESS;
					selected = 0;
					setVfd();
					paint();
					break;
				}

			case CRCInput::RC_timeout:
				loop = false;
				break;
	
			default:
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = CMenuTarget::RETURN_EXIT_ALL;
				}
		}

		frameBuffer->blit();	
	}

	hide();

	if(observer)
		observer->changeNotify(name, NULL);

	return res;
}

void CVfdControler::hide()
{
	mainWindow->hide();

	frameBuffer->blit();
}

void CVfdControler::paint()
{
	dprintf(DEBUG_NORMAL, "CVfdControler::paint\n");

	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);
	
	// main window
	mainWindow->setColor(COL_MENUCONTENT_PLUS_0);
	mainWindow->setCorner(RADIUS_MID, CORNER_BOTTOM);
	mainWindow->paint();

	// head
	CCHeaders headers(x, y, width, hheight, name.c_str(), NEUTRINO_ICON_LCD);
	headers.setCorner(g_settings.Head_radius);
	headers.setGradient(g_settings.Head_gradient);
	headers.setLine(g_settings.Head_line, g_settings.Head_line_gradient);
	headers.paint();

	paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
	paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);

	frameBuffer->paintHLineRel(x + BORDER_LEFT, width - 20, y + hheight + mheight*2 + mheight/4, COL_MENUCONTENT_PLUS_3);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*3 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

void CVfdControler::paintSlider(int _x, int _y, unsigned int spos, float factor, const char* const text, bool selected)
{
	char wert[5];
	
	int icon_w = 120;
	int icon_h = 11;
	
	frameBuffer->getIconSize(NEUTRINO_ICON_VOLUMEBODY, &icon_w, &icon_h);
	
	int slider_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth("100", true); //UTF-8
	
	int startx = width - icon_w - slider_w - 50;

	frameBuffer->paintBoxRel(_x + startx, _y, 120, mheight, COL_MENUCONTENT_PLUS_0);
	frameBuffer->paintIcon(NEUTRINO_ICON_VOLUMEBODY, _x + startx, _y + 2 + mheight/4);
	frameBuffer->paintIcon(selected ? NEUTRINO_ICON_VOLUMESLIDER2BLUE : NEUTRINO_ICON_VOLUMESLIDER2, (int)(_x + (startx + 3) + (spos / factor)), _y + mheight/4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + mheight, width, text, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
	
	// wert //FIXME
	sprintf(wert, "%3d", spos ); // UTF-8 encoded

	frameBuffer->paintBoxRel(_x + startx + 120 + 10, _y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x + startx + 120 + 10, _y + mheight, width, wert, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

