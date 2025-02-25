//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: messagebox.cpp 21122024 mohousch Exp $
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

#include <system/debug.h>
#include <system/settings.h>

#include <gui/widget/messagebox.h>


CMessageBox::CMessageBox(const char* const Caption, const char * const Text, const int Width, const char * const Icon, const result_ Default, const uint32_t ShowButtons)
{
	dprintf(DEBUG_INFO, "CMessageBox::CMessageBox\n");
	
	widget = NULL;
	headers = NULL;
	m_lines.clear();
	
	pages = 1;
	
	m_message = strdup(Text);
	
	//
	showbuttons = ShowButtons;
	
	//
	init(Caption, Width, Icon);

	//
	returnDefaultOnTimeout = false;

	result = Default;

	int MaxButtonTextWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_("Cancel"), true) + 5; // UTF-8

	int ih = 0;
	int iw = 0;
	
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_HOME, &iw, &ih);
	
	int ButtonWidth = BORDER_LEFT + BORDER_RIGHT + iw + MaxButtonTextWidth + ICON_OFFSET;
	
	int num = 0;

	if (showbuttons & mbNone)
		num = 0;
	
	if (showbuttons & mbYes)
		num++;
	
	if (showbuttons & mbNo)
		num++;
	
	if (showbuttons & (mbCancel | mbBack | mbOk))
		num++;
	
	int new_width = BORDER_LEFT + BORDER_RIGHT + num*ButtonWidth;
	if(new_width > m_width)
		m_width = new_width;
		
	//
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
		
	// initFrames
	initFrames();
}

CMessageBox::~CMessageBox(void)
{
	dprintf(DEBUG_INFO, "CMessageBox::del:\n");

	if (m_message != NULL) 
	{
		free(m_message);
	}
	
	m_lines.clear();
	
	hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CMessageBox::init(const char * const Caption, const int Width, const char * const Icon)
{
	dprintf(DEBUG_INFO, "CMessageBox::init\n");
	
	m_width = Width;
	int nw = m_width;
	m_theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6; 	// title
	m_fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6; 	// foot
	m_iheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();		// item
	m_height  = m_theight + m_fheight + m_iheight;

	m_caption = Caption? Caption : "";

	char * begin;
	char * pos;
	
	begin = m_message;
	
	// recalculate height
	while (true)
	{
		m_height += m_iheight;
		
		if (m_height > MESSAGEBOX_MAX_HEIGHT)
		{
			m_height -= m_iheight;
		}
		
		m_lines.push_back(begin);
		
		pos = strchr(begin, '\n');
		
		if (pos != NULL)
		{
			*pos = 0;
			begin = pos + 1;
		}
		else
			break;
	}
			
	//		
	pages = 1;
	entries_per_page = ((m_height - m_theight - m_fheight ) / m_iheight);
	current_page = 0;
	pages = (m_lines.size() + entries_per_page - 1) / entries_per_page;
	
	//
	unsigned int additional_width = 0;

	if (entries_per_page < m_lines.size())
		additional_width = BORDER_LEFT + BORDER_RIGHT + SCROLLBAR_WIDTH;
	else
		additional_width = BORDER_LEFT + BORDER_RIGHT;

	if (Icon != NULL)
	{
		m_iconfile = Icon;

		int iw, ih;
		CFrameBuffer::getInstance()->getIconSize(m_iconfile.c_str(), &iw, &ih);
		additional_width += iw + ICON_OFFSET; 
	}
	else
		m_iconfile = "";

	nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(m_caption); // UTF-8

	if (nw > m_width)
		m_width = nw;
		
	//
	for (std::vector<char *>::const_iterator it = m_lines.begin(); it != m_lines.end(); it++)
	{
		nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(*it, true); // UTF-8
		
		if (nw > m_width)
		{
			m_width = nw;

			if(m_width > HINTBOX_MAX_WIDTH)
				m_width = HINTBOX_MAX_WIDTH;		
		}
	}
}

void CMessageBox::initFrames(void)
{
	dprintf(DEBUG_INFO, "CMessageBox::initFrames\n");
	
	cFrameBox.iWidth = m_width;
	cFrameBox.iHeight = m_height;
	cFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - cFrameBox.iWidth ) >> 1);
	cFrameBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - cFrameBox.iHeight) >> 2);
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("messagebox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		widget->name = "messagebox";
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
		
		headers = new CCHeaders();
		
		widget->addCCItem(headers);
	}

	widget->paintMainFrame(true);
}

void CMessageBox::paint(void)
{
	dprintf(DEBUG_INFO, "CMessageBox::paint\n");
	
	// 
	widget->setPosition(&cFrameBox);
	widget->setBorderMode(borderMode);
	widget->setBorderColor(borderColor);
	widget->enableSaveScreen();

	// title
	if (headers)
	{
		headers->setPosition(borderMode? CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1) + 2 : CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1), borderMode? CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + 2 : CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2), borderMode? m_width - 4 : m_width, m_theight);
	
		headers->setTitle(m_caption.c_str(), m_iconfile.c_str());
	}

	refreshPage();
}

void CMessageBox::refreshPage()
{
	dprintf(DEBUG_INFO, "CMessageBox::refreshPage\n");
	
	//
	widget->paint();

	//
	int count = entries_per_page;
	int ypos  = cFrameBox.iY + m_theight;
	
	for (std::vector<char *>::const_iterator it = m_lines.begin() + (entries_per_page * current_page); ((it != m_lines.end()) && (count > 0)); it++, count--)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(cFrameBox.iX + BORDER_LEFT, (ypos += g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight()), m_width - 20, *it, COL_MENUCONTENT_TEXT_PLUS_0, 0, true); 
	}

	// scrollBar #TODO
	if (entries_per_page < m_lines.size())
	{
		int ypos  = cFrameBox.iY + m_theight;
		scrollBar.paint(cFrameBox.iX + cFrameBox.iWidth - SCROLLBAR_WIDTH, ypos, m_height - m_theight - m_fheight, pages, current_page);
	}
}

bool CMessageBox::has_scrollbar(void)
{
	return (entries_per_page < m_lines.size());
}

void CMessageBox::scroll_up(void)
{
	if (current_page > 0)
	{
		current_page--;
		refreshPage();
	}
}

void CMessageBox::scroll_down(void)
{
	if ((entries_per_page * (current_page + 1)) <= m_lines.size())
	{
		current_page++;
		refreshPage();
	}
}

void CMessageBox::hide(void)
{
	dprintf(DEBUG_INFO, "CMessageBox::hide:\n");

	widget->hide();
}

void CMessageBox::returnDefaultValueOnTimeout(bool returnDefault)
{
	returnDefaultOnTimeout = returnDefault;
}

void CMessageBox::paintButtons()
{
	dprintf(DEBUG_INFO, "CMessageBox::paintButtons\n");
	
	//
	uint32_t color;
	fb_pixel_t bgcolor;

	if (showbuttons & mbNone)
		return;
	
	// horizontal line separator	
	CFrameBuffer::getInstance()->paintHLineRel(CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1) + BORDER_LEFT, m_width - BORDER_LEFT - BORDER_RIGHT, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight, COL_MENUCONTENT_PLUS_5);
		
	//
	int MaxButtonTextWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(_("Cancel"), true) + 5; // UTF-8

	int iw, ih;
	CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_HOME, &iw, &ih);
	
	if (ih >= m_fheight - 10)
		ih = m_fheight - 12;
		
	int ButtonWidth = BORDER_LEFT + BORDER_RIGHT + iw + ICON_OFFSET + MaxButtonTextWidth;

	int ButtonSpacing = (m_width - BORDER_LEFT - BORDER_RIGHT - (ButtonWidth * 3)) / 2;
	if(ButtonSpacing <= ICON_OFFSET) 
		ButtonSpacing = ICON_OFFSET;

	int xpos = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_width ) >> 1) + BORDER_LEFT;
	int fh = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight();
	
	// yes
	if (showbuttons & mbYes)
	{
		if (result == mbrYes)
		{
			color   = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_TEXT_PLUS_0;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}
		

		CFrameBuffer::getInstance()->paintBoxRel(xpos, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, ButtonWidth, m_fheight - 10, bgcolor);

		CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_RED, &iw, &ih);
		if (ih >= m_fheight - 10)
			ih = m_fheight - 12;
		
		CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_RED, xpos + BORDER_LEFT + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, m_fheight - 10, iw, ih);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(xpos + BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + (m_height) - (m_fheight - fh)/2, ButtonWidth - (BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET + BORDER_LEFT), _("Yes"), color, 0, true); // UTF-8
		
		xpos += ButtonWidth + ButtonSpacing;
	}

	// no
	if (showbuttons & mbNo)
	{
		if (result == mbrNo)
		{
			color   = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_TEXT_PLUS_0;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}

		CFrameBuffer::getInstance()->paintBoxRel(xpos, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, ButtonWidth, m_fheight - 10, bgcolor);

		CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_GREEN, &iw, &ih);
		if (ih >= m_fheight - 10)
			ih = m_fheight - 12;
			
		CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, xpos + BORDER_LEFT + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, m_fheight - 10, iw, ih);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(xpos + BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + (m_height) - (m_fheight - fh)/2, ButtonWidth - (BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET + BORDER_LEFT), _("No"), color, 0, true); // UTF-8		
	
		xpos += ButtonWidth + ButtonSpacing;
	}

	// cancel|back|ok
	if (showbuttons & (mbCancel | mbBack | mbOk))
	{
		if (result >= mbrCancel)
		{
			color   = COL_MENUCONTENTSELECTED_TEXT_PLUS_0;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		}
		else
		{
			color   = COL_INFOBAR_TEXT_PLUS_0;
			bgcolor = COL_INFOBAR_SHADOW_PLUS_0;
		}

		CFrameBuffer::getInstance()->paintBoxRel(xpos, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, ButtonWidth, m_fheight - 10, bgcolor);

		CFrameBuffer::getInstance()->getIconSize(NEUTRINO_ICON_BUTTON_HOME, &iw, &ih);
		if (ih >= m_fheight - 10)
			ih = m_fheight - 12;
			
		CFrameBuffer::getInstance()->paintIcon(NEUTRINO_ICON_BUTTON_HOME, xpos + BORDER_LEFT + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + m_height - m_fheight + 5, m_fheight - 10, iw, ih);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(xpos + BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET, CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_height) >> 2) + (m_height) - (m_fheight - fh)/2, ButtonWidth - (BORDER_LEFT + ICON_OFFSET + iw + ICON_OFFSET + BORDER_LEFT), (showbuttons & mbCancel) ? _("Cancel") : (showbuttons & mbOk) ? _("OK") : _("Back"), color, 0, true); // UTF-8	
	}	
}

int CMessageBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CMessageBox::exec: timeout:%d\n", timeout);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	// paint
	paint();
	paintButtons();
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg;
		
	dprintf(DEBUG_NORMAL, "CMessageBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (msg == CRCInput::RC_timeout && returnDefaultOnTimeout)
		{
			// return default
			loop = false;
		}
		else if (((msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home)) && (showbuttons & (mbCancel | mbBack | mbOk)))
		{
			result = (showbuttons & mbCancel) ? mbrCancel : (showbuttons & mbOk) ? mbrOk: mbrBack;
			loop   = false;
		}
		else if ((msg == CRCInput::RC_green) && (showbuttons & mbNo))
		{
			result = mbrNo;
			loop   = false;
		}
		else if ((msg == CRCInput::RC_red) && (showbuttons & mbYes))
		{
			result = mbrYes;
			loop   = false;
		}
		else if(msg == CRCInput::RC_right)
		{
			bool ok = false;
			while (!ok)
			{
				result = (result_)((result + 1) & 3);
				ok = showbuttons & (1 << result);
			}

			paintButtons();
		}
		else if (has_scrollbar() && ((msg == CRCInput::RC_up) || (msg == CRCInput::RC_down) || (msg == CRCInput::RC_page_up) || (msg == CRCInput::RC_page_down)))
		{
			if ( (msg == CRCInput::RC_up) || (msg == CRCInput::RC_page_up))
				scroll_up();
			else
				scroll_down();
			
			paintButtons();
		}
		else if(msg == CRCInput::RC_left)
		{
			bool ok = false;
			while (!ok)
			{
				result = (result_)((result - 1) & 3);
				ok = showbuttons & (1 << result);
			}

			paintButtons();

		}
		else if(msg == CRCInput::RC_ok || (msg == CRCInput::RC_home && (showbuttons & mbNone)))
		{
			loop = false;
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			loop = false;
		}

		CFrameBuffer::getInstance()->blit();
	}

	// hide
	hide();
	
	return result;
}

////
int MessageBox(const char * const Caption, const char * const Text, const CMessageBox::result_ Default, const uint32_t ShowButtons, const char * const Icon, const int Width, const int timeout, bool returnDefaultOnTimeout, const int border)
{
   	CMessageBox * messageBox = new CMessageBox(Caption, Text, Width, Icon, Default, ShowButtons);
	messageBox->returnDefaultValueOnTimeout(returnDefaultOnTimeout);
	messageBox->setBorderMode(border);
//	messageBox->setBorderColor(bcol);
	messageBox->exec(timeout);
	
	int res = messageBox->result;
	
	delete messageBox;

	return res;
}

