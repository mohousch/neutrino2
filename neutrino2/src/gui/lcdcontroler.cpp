/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: lcdcontroller.cpp 30052024 mohousch Exp $

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

#include <lcdcontroler.h>

#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/color.h>

#include <driver/rcinput.h>

#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino2.h>

#include <math.h>

#include <system/debug.h>


#define BRIGHTNESSFACTOR 	(MAXBRIGHTNESS/100)
#define CONTRASTFACTOR		(MAXCONTRAST/100)

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
#define NUM_LCD_SLIDERS		3
#else
#define NUM_LCD_SLIDERS		2
#endif


CLCDControler::CLCDControler(const char *const Name)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	mainWindow = NULL;

	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	name = Name? Name : "";

	width = MENU_WIDTH;
	height = hheight+ mheight*4 + mheight/2;
	x = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - width) >> 1);
	y = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - height)>> 1);

	mainWindow = new CCWindow(x, y, width, height + 10);

	contrast = CLCD::getInstance()->getContrast();
	brightness = CLCD::getInstance()->getBrightness();
	brightnessstandby = CLCD::getInstance()->getBrightnessStandby();
}

void CLCDControler::setLCD()
{
	CLCD::getInstance()->setBrightness(brightness);
	CLCD::getInstance()->setBrightnessStandby(brightnessstandby);
	CLCD::getInstance()->setContrast(contrast);
}

int CLCDControler::exec(CMenuTarget *parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CLCDControler::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int selected;
	int res = CMenuTarget::RETURN_REPAINT;
	
	unsigned int contrast_alt;
	unsigned int brightness_alt;
	unsigned int brightnessstandby_alt;

	if (parent)
		parent->hide();
	
	contrast_alt = CLCD::getInstance()->getContrast();
	brightness_alt = CLCD::getInstance()->getBrightness();
	brightnessstandby_alt = CLCD::getInstance()->getBrightnessStandby();
	selected = 0;

	setLCD();
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
			if(selected < NUM_LCD_SLIDERS) // max entries
			{
				paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), false);
				paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)				
				paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), false);
#endif
				selected++;
				
				switch (selected) 
				{
					case 0:
						paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
						break;
						
					case 1:
						paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);

						CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
						break;
					
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)	
					case 2:
						paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), true);
						break;
						
					case 3:
#else
					case 2:
#endif
						frameBuffer->paintBoxRel(x, y + hheight + mheight*3 + mheight/2, width, mheight, COL_MENUCONTENTSELECTED_PLUS_0);

						g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*4 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENTSELECTED_TEXT_PLUS_0, 0, true); // UTF-8
						break;
				}
			}
			break;

			case CRCInput::RC_up:
			if (selected > 0) 
			{
				paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), false);

				paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)
				paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), false);
#endif
				selected--;
				switch (selected) 
				{
					case 0:
						paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);

						CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
						break;
						
					case 1:
						paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);

						CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
					
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)	
					case 2:
						paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), true);

						CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
						break;
						
					case 3:
#else
					case 2:
#endif
						frameBuffer->paintBoxRel(x, y + hheight + mheight*3 + mheight/2, width, mheight, COL_MENUCONTENT_PLUS_0);

						g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*4 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
						break;
				}
			}
			break;

			case CRCInput::RC_right:
				switch (selected) 
				{
					case 0:
						if (brightness < MAXBRIGHTNESS) 
						{
							brightness ++;
							
							paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
							setLCD();
						}
						break;
						
					case 1:
						if (brightnessstandby < MAXBRIGHTNESS) 
						{
							brightnessstandby ++;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);
							setLCD();
						}
						break;
					
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)	
					case 2:
						if (contrast < MAXCONTRAST) 
						{
							contrast ++;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), true);
							setLCD();
						}
						break;
#endif
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
							setLCD();
						}
						break;
						
					case 1:
						if (brightnessstandby > 0) 
						{
							brightnessstandby--;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), true);
							setLCD();
						}
						break;
					
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)	
					case 2:
						if (contrast > 0) 
						{
							contrast--;
							
							paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), true);
							setLCD();
						}
						break;
#endif
				}
				break;

			case CRCInput::RC_home:
				if ( ((brightness != brightness_alt) || (brightnessstandby != brightnessstandby_alt) ) && (MessageBox(name.c_str(), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
					break;

				brightness = brightness_alt;
				brightnessstandby = brightnessstandby_alt;
				contrast = contrast_alt;
				setLCD();
				loop = false;
				break;

			case CRCInput::RC_ok:
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)
				if (selected == 3)
#else
				if (selected == 2)
#endif
				{
					brightness = DEFAULT_LCD_BRIGHTNESS;
					brightnessstandby = DEFAULT_LCD_STANDBYBRIGHTNESS;
					contrast = DEFAULT_LCD_CONTRAST;
					selected = 0;
					setLCD();
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

	return res;
}

void CLCDControler::hide()
{
	mainWindow->hide();

	frameBuffer->blit();
}

void CLCDControler::paint()
{
	dprintf(DEBUG_NORMAL, "CLCDControler::paint\n");

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, name.c_str());
	
	// main window
	mainWindow->setColor(COL_MENUCONTENT_PLUS_0);
	mainWindow->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	mainWindow->paint();

	// head
	CCHeaders headers(x, y, width, hheight, name.c_str(), NEUTRINO_ICON_LCD);
	headers.setCorner(g_settings.Head_radius);
	headers.setGradient(g_settings.Head_gradient);
	headers.setLine(g_settings.Head_line, g_settings.Head_line_gradient);
	headers.paint();

	paintSlider(x + BORDER_LEFT, y + hheight, brightness, BRIGHTNESSFACTOR, _("normal Brightness"), true);
	paintSlider(x + BORDER_LEFT, y + hheight + mheight, brightnessstandby, BRIGHTNESSFACTOR, _("Standby Brightness"), false);
#if defined (ENABLE_LCD) || defined (ENABLE_TFT_LCD)
	paintSlider(x + BORDER_LEFT, y + hheight + mheight*2, contrast, CONTRASTFACTOR, _("Contrast"), false);
#endif

	frameBuffer->paintHLineRel(x + BORDER_LEFT, width - 20, y + hheight + mheight*3 + mheight/4, COL_MENUCONTENT_PLUS_3);
	
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + BORDER_LEFT, y + hheight + mheight*4 + mheight/2, width, _("Reset to defaults"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

void CLCDControler::paintSlider(int _x, int _y, unsigned int spos, float factor, const char* const text, bool selected)
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

