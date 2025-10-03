//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: colorchooser.cpp 21122024 mohousch Exp $
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

#include <global.h>
#include <neutrino2.h>

#include <driver/gdi/fontrenderer.h>
#include <driver/gdi/color.h>

#include <driver/rcinput.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/colorchooser.h>


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

CColorChooser::CColorChooser(const char * const Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha) // UTF-8
{
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
	m_cBoxWindow = new CCWindow(&cFrameBox);
}

CColorChooser::~CColorChooser()
{
	if (m_cBoxWindow)
	{
		delete m_cBoxWindow;
		m_cBoxWindow = NULL;
	}
}

void CColorChooser::setColor()
{
	dprintf(DEBUG_NORMAL, "CColorChooser::setColor:\n");
	
	uint32_t color = convertSetupColor2RGB(*(value[VALUE_R]), *(value[VALUE_G]), *(value[VALUE_B]));
	uint8_t tAlpha = (value[VALUE_ALPHA]) ? *(value[VALUE_ALPHA]) : 0xFF;

	fb_pixel_t col = ((tAlpha << 24) & 0xFF000000) | color;
	
	frameBuffer->paintBoxRel(cFrameBoxColorPreview.iX + 2, cFrameBoxColorPreview.iY + 2,  cFrameBoxColorPreview.iWidth - 4, cFrameBoxColorPreview.iHeight - 4, col);
}

int CColorChooser::exec(CWidgetTarget* parent, const std::string&)
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
	unsigned char a_alt = (value[VALUE_ALPHA]) ? (*(value[VALUE_ALPHA])) : 0xFF;

	paint();
	setColor();
	CFrameBuffer::getInstance()->blit();

	int selected = 0;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd, true);

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu == 0 ? 0xFFFF : g_settings.timing_menu);

		switch ( msg ) 
		{
			case CRCInput::RC_down:
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
				
			case CRCInput::RC_up:
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
				
			case CRCInput::RC_right:
			{
				if ((*value[selected]) < 255)
				{
					if ((*value[selected]) < 250)
						(*value[selected]) += 5;
					else
						(*value[selected]) = 255;

					paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					
					setColor();
				}
				break;
			}
				
			case CRCInput::RC_left:
			{
				if ((*value[selected]) > 0)
				{
					if ((*value[selected]) > 5)
						(*value[selected]) -= 5;
					else
						(*value[selected]) = 0;

					paintSlider(cFrameBox.iX + BORDER_LEFT, cFrameBox.iY + cFrameBoxTitle.iHeight + cFrameBoxItem.iHeight*selected, value[selected], colorchooser_names[selected], iconnames[selected], true);
					
					setColor();
				}
				break;
			}
				
			case CRCInput::RC_home:
			{
				if (((*value[VALUE_R] != r_alt) || (*value[VALUE_G] != g_alt) || (*value[VALUE_B] != b_alt) || ((value[VALUE_ALPHA]) && (*(value[VALUE_ALPHA]) != a_alt)) ) && (MessageBox(name.c_str(), _("Discard changes?"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
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
					
				loop = false;
				break;
			}
	
			case CRCInput::RC_timeout:
			case CRCInput::RC_ok:
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

	//
	CNeutrinoApp::getInstance()->setupColor();

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
	m_cBoxWindow->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	m_cBoxWindow->paint();

	// Head
	cFrameBoxTitle.iX = cFrameBox.iX;
	cFrameBoxTitle.iY = cFrameBox.iY;
	cFrameBoxTitle.iWidth = cFrameBox.iWidth;

	CCHeaders headers(&cFrameBoxTitle, name.c_str(), NEUTRINO_ICON_COLORS);

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
	
	// color preview frame
	frameBuffer->paintFrameBox(cFrameBoxColorPreview.iX, cFrameBoxColorPreview.iY, cFrameBoxColorPreview.iWidth, cFrameBoxColorPreview.iHeight, COL_SILVER_PLUS_0);
}

void CColorChooser::paintSlider(int _x, int _y, unsigned char *spos, const char* const text, const char * const iconname, const bool selected)
{
	dprintf(DEBUG_NORMAL, "CColorChooser::paintSlider:\n");
	
	if (!spos)
		return;

	//
	int a_w = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(colorchooser_names[3]);
	
	// volumebox box
	frameBuffer->paintBoxRel(_x + a_w + 2*ICON_OFFSET, _y, 150, cFrameBoxItem.iHeight, COL_MENUCONTENT_PLUS_0);

	// volumebody icon
	frameBuffer->paintIcon(NEUTRINO_ICON_VOLUMEBODY, _x + a_w + 2*ICON_OFFSET, _y + 2 + cFrameBoxItem.iHeight/ITEMS_COUNT);

	// slider icon
	frameBuffer->paintIcon(selected ? iconname : NEUTRINO_ICON_VOLUMESLIDER2, _x + a_w + 2*ICON_OFFSET + 3 + (*spos*100)/255, _y + cFrameBoxItem.iHeight/ITEMS_COUNT);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + cFrameBoxItem.iHeight, cFrameBox.iWidth - BORDER_LEFT - BORDER_RIGHT - ICON_OFFSET - 150 - cFrameBoxColorPreview.iWidth, text, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

