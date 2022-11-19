/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: colorchooser.cpp 2013/10/12 mohousch Exp $

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

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <driver/color.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/colorchooser.h>
#include <gui/widget/icons.h>


#define VALUE_R     0
#define VALUE_G     1
#define VALUE_B     2
#define VALUE_ALPHA 3
#define ITEMS_COUNT 4

static const char * const iconnames[4] = {
	NEUTRINO_ICON_VOLUMESLIDER2RED,
	NEUTRINO_ICON_VOLUMESLIDER2GREEN,
	NEUTRINO_ICON_VOLUMESLIDER2BLUE,
	NEUTRINO_ICON_VOLUMESLIDER2ALPHA
};

static const char* const colorchooser_names[4] =
{
	_("red"),
	_("green"),
	_("blue"),
	_("alpha")
};

CColorChooser::CColorChooser(const char * const Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer) // UTF-8
{
	dprintf(DEBUG_NORMAL, "CColorChooser::CColorChooser:\n");
	
	observer = Observer;
	notifier = NULL;

	name = Name? Name : "";
	
	frameBuffer = CFrameBuffer::getInstance();

	// Head
	cFrameBoxTitle.iHeight= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;

	// Item
	cFrameBoxItem.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	
	cFrameBox.iWidth = MENU_WIDTH;
	cFrameBox.iHeight = cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*ITEMS_COUNT;

	cFrameBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - cFrameBox.iWidth) >> 1);
	cFrameBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - cFrameBox.iHeight) >>1);

	value[VALUE_R]     = R;
	value[VALUE_G]     = G;
	value[VALUE_B]     = B;
	value[VALUE_ALPHA] = Alpha;
	
	// ColorPreview Box
	int a_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[3]);

	cFrameBoxColorPreview.iHeight = cFrameBox.iHeight - cFrameBoxTitle.iHeight - cFrameBoxItem.iHeight;
	cFrameBoxColorPreview.iWidth = cFrameBox.iWidth - BORDER_LEFT - BORDER_RIGHT - 2*ICON_OFFSET - 150 - 2*ICON_OFFSET - a_w;
	cFrameBoxColorPreview.iX = cFrameBox.iX + cFrameBox.iWidth - BORDER_RIGHT - cFrameBoxColorPreview.iWidth;
	cFrameBoxColorPreview.iY = cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight/2;

	//
	//m_cBoxWindow.setPosition(&cFrameBox);
	m_cBoxWindow = new CWindow(&cFrameBox);
}

CColorChooser::~CColorChooser()
{
	dprintf(DEBUG_NORMAL, "CColorChooser::del:\n");
	
	if (m_cBoxWindow)
	{
		delete m_cBoxWindow;
		m_cBoxWindow = NULL;
	}
}

void CColorChooser::setColor()
{
	dprintf(DEBUG_NORMAL, "CColorChooser::setColor:\n");
	
	int color = convertSetupColor2RGB(*(value[VALUE_R]), *(value[VALUE_G]), *(value[VALUE_B]));
	int tAlpha = (value[VALUE_ALPHA]) ? (convertSetupAlpha2Alpha(*(value[VALUE_ALPHA]))) : 0;

	if(!value[VALUE_ALPHA]) 
		tAlpha = 0xFF;

	fb_pixel_t col = ((tAlpha << 24) & 0xFF000000) | color;
	
	frameBuffer->paintBoxRel(cFrameBoxColorPreview.iX + 2, cFrameBoxColorPreview.iY + 2,  cFrameBoxColorPreview.iWidth - 4, cFrameBoxColorPreview.iHeight - 4, col);
}

int CColorChooser::exec(CMenuTarget* parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CColorChooser::exec:\n");
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = RETURN_REPAINT;
	
	if (parent)
		parent->hide();

	unsigned char r_alt= *value[VALUE_R];
	unsigned char g_alt= *value[VALUE_G];
	unsigned char b_alt= *value[VALUE_B];
	unsigned char a_alt = (value[VALUE_ALPHA]) ? (*(value[VALUE_ALPHA])) : 0;
	
	if(!value[VALUE_ALPHA]) 
		a_alt = 0xFF;

	paint();
	setColor();
	CFrameBuffer::getInstance()->blit();

	int selected = 0;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd, true);

		if ( msg <= RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		switch ( msg ) 
		{
			case RC_down:
				{
					if (selected < ((value[VALUE_ALPHA]) ? 3 : 2))
					{
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], false);
						selected++;
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					} 
					else 
					{
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], false);
						selected = 0;
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					}
					break;
				}
				
			case RC_up:
				{
					if (selected > 0)
					{
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], false);
						selected--;
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					} 
					else 
					{
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], false);
						selected = ((value[VALUE_ALPHA]) ? 3 : 2);
						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					}
					break;
				}
				
			case RC_right:
				{
					if ((*value[selected]) < 100)
					{
						if ((*value[selected]) < 98)
							(*value[selected]) += 2;
						else
							(*value[selected]) = 100;

						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
						setColor();
					}
					break;
				}
				
			case RC_left:
				{
					if ((*value[selected]) > 0)
					{
						if ((*value[selected]) > 2)
							(*value[selected]) -= 2;
						else
							(*value[selected]) = 0;

						paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
						setColor();
					}
					break;
				}
				
			case RC_home:
				if (((*value[VALUE_R] != r_alt) || (*value[VALUE_G] != g_alt) || (*value[VALUE_B] != b_alt) || ((value[VALUE_ALPHA]) && (*(value[VALUE_ALPHA]) != a_alt)) ) &&
						(MessageBox(name.c_str(), _("Discard changes?"), mbrYes, mbYes | mbCancel) == mbrCancel))
					break;
		
					// cancel
					if (value[VALUE_R])
						*value[VALUE_R] = r_alt;

					if (value[VALUE_G])
						*value[VALUE_G] = g_alt;

					if (value[VALUE_B])
						*value[VALUE_B] = b_alt;

					if (value[VALUE_ALPHA])
						*value[VALUE_ALPHA] = a_alt;
	
			case RC_timeout:
			case RC_ok:
				loop = false;
				break;

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

	//if(observer)
	//	observer->changeNotify(name, NULL);
	notifier = new CColorSetupNotifier();
	notifier->changeNotify("", NULL);
	delete notifier;
	notifier = NULL;

	return res;
}

void CColorChooser::hide()
{
	dprintf(DEBUG_NORMAL, "CColorChooser::hide:\n");
	
	m_cBoxWindow->hide();
	frameBuffer->blit();
}

void CColorChooser::paint()
{
	dprintf(DEBUG_NORMAL, "CColorChooser::paint:\n");
	
	// box
	m_cBoxWindow->setColor(COL_MENUCONTENT_PLUS_0);
	m_cBoxWindow->setCorner(RADIUS_MID, CORNER_ALL);
	m_cBoxWindow->paint();

	// Head
	cFrameBoxTitle.iX = cFrameBox.iX;
	cFrameBoxTitle.iY = cFrameBox.iY;
	cFrameBoxTitle.iWidth = cFrameBox.iWidth;

	CHeaders headers(&cFrameBoxTitle, name.c_str(), NEUTRINO_ICON_COLORS);

	headers.paint();

	// slider
	for (int i = 0; i < ITEMS_COUNT; i++)
		paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*i, value[i], colorchooser_names[i], iconnames[i], (i == 0));

	//color preview
	int a_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[3]);

	cFrameBoxColorPreview.iHeight = cFrameBox.iHeight - cFrameBoxTitle.iHeight - cFrameBoxItem.iHeight;
	cFrameBoxColorPreview.iWidth = cFrameBox.iWidth - BORDER_LEFT - BORDER_RIGHT - 2*ICON_OFFSET - 150 - 2*ICON_OFFSET - a_w;
	cFrameBoxColorPreview.iX = cFrameBox.iX + cFrameBox.iWidth - BORDER_RIGHT - cFrameBoxColorPreview.iWidth;
	cFrameBoxColorPreview.iY = cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight/2;

	frameBuffer->paintBoxRel(cFrameBoxColorPreview.iX, cFrameBoxColorPreview.iY, cFrameBoxColorPreview.iWidth, cFrameBoxColorPreview.iHeight, COL_MENUHEAD_PLUS_0);

	frameBuffer->paintBoxRel(cFrameBoxColorPreview.iX + 2, cFrameBoxColorPreview.iY + 2, cFrameBoxColorPreview.iWidth - 4, cFrameBoxColorPreview.iHeight - 4 - BORDER_LEFT, 254);
}

void CColorChooser::paintSlider(int _x, int _y, unsigned char *spos, const char* const text, const char * const iconname, const bool selected)
{
	dprintf(DEBUG_NORMAL, "CColorChooser::paintSlider:\n");
	
	if (!spos)
		return;

	//
	//int r_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[0]);
	//int g_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[1]);
	//int b_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[2]);
	int a_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[3]);
	
	// volumebox box
	frameBuffer->paintBoxRel(_x + a_w + 2*ICON_OFFSET, _y, 150, cFrameBoxItem.iHeight, COL_MENUCONTENT_PLUS_0);

	// volumebody icon
	frameBuffer->paintIcon(NEUTRINO_ICON_VOLUMEBODY, _x + a_w + 2*ICON_OFFSET, _y + 2 + cFrameBoxItem.iHeight/ITEMS_COUNT);

	// slider icon
	frameBuffer->paintIcon(selected ? iconname : NEUTRINO_ICON_VOLUMESLIDER2, _x + a_w + 2*ICON_OFFSET + 3 + (*spos), _y + cFrameBoxItem.iHeight/ITEMS_COUNT);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + cFrameBoxItem.iHeight, cFrameBox.iWidth - BORDER_LEFT - BORDER_RIGHT - ICON_OFFSET - 150 - cFrameBoxColorPreview.iWidth, text, COL_MENUCONTENT, 0, true); // UTF-8
}

