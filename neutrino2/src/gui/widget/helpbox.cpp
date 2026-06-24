//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: helpbox.cpp 24062026 mohousch Exp $
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

#include <gui/widget/icons.h>

#include <system/debug.h>

#include <gui/widget/helpbox.h>


////
CHelpBox::CHelpBox(const char* const Caption, const int Width, const char * const Icon)
{
	dprintf(DEBUG_INFO, "CHelpBox::CMessageBox\n");
	
	widget = NULL;
	listBox = NULL;
	item = NULL;
	
	m_width = Width;
	
	m_iconfile = Icon? Icon : "";
	m_caption = Caption? Caption : "";
		
	//
	borderMode = CComponent::BORDER_NO;
	borderColor = COL_INFOBAR_SHADOW_PLUS_0;
	
	//
	init();
}

CHelpBox::~CHelpBox(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::del:\n");
	
	hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CHelpBox::init()
{
	dprintf(DEBUG_INFO, "CHelpBox::init\n");
	
	int nw = 0;

	m_height  = HELPBOX_HEIGHT;
	
	int maxWidth = HELPBOX_WIDTH;
//	int maxOverallHeight = 0;

	m_width = maxWidth;
	
	////
	initFrames();
}

void CHelpBox::initFrames(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::initFrames\n");
	
	cFrameBox.iWidth = m_width;
	cFrameBox.iHeight = m_height;
	cFrameBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - cFrameBox.iWidth) >> 1);
	cFrameBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - cFrameBox.iHeight) >> 2);
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("helpbox");
	
	if (widget)
	{
		listBox = (ClistBox *)widget->getCCItem(CComponent::CC_LISTBOX);
		
		if (listBox == NULL)
		{
			listBox = new ClistBox(&cFrameBox);
			
			widget->addCCItem(listBox);
		}
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		widget->name = "helpbox";
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
		
		listBox = new ClistBox(&cFrameBox);
		
		widget->addCCItem(listBox);
	}

	listBox->paintMainFrame(false);
	listBox->setInFocus(false);
	listBox->enablePaintHead();
	
	widget->paintMainFrame(true);
}

void CHelpBox::paint(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::paint\n");
	
	// 
	widget->setPosition(&cFrameBox);
	widget->setBorderMode(borderMode);
	widget->setBorderColor(borderColor);
	widget->enableSaveScreen();
	
	// title / body
	if (listBox)
	{
		listBox->setPosition(cFrameBox.iX, cFrameBox.iY, cFrameBox.iWidth, cFrameBox.iHeight - 20);
		listBox->setTitle(m_caption.c_str(), m_iconfile.c_str());
	}

	refreshPage();
}

void CHelpBox::refreshPage()
{
	dprintf(DEBUG_INFO, "CHelpBox::refreshPage\n");
	
	// get maxHeight / maxWidth and resize
	
	//
	widget->paint();
}

void CHelpBox::scroll_up(void)
{
	listBox->scrollPageUp();
}

void CHelpBox::scroll_down(void)
{
	listBox->scrollPageDown();
}

void CHelpBox::hide(void)
{
	dprintf(DEBUG_INFO, "CHelpBox::hide:\n");

	widget->hide();
}

void CHelpBox::addLine(const char* const text)
{
	item = new CMenuForwarder(text);
	
	listBox->addItem(item);
}

void CHelpBox::addLine(const char* const icon, const char* const text)
{
	item = new CMenuForwarder(text);
	item->setIconName(icon);
	
	listBox->addItem(item);
}

void CHelpBox::addSeparator(void)
{
	item = new CMenuSeparator(CMenuSeparator::LINE);
	listBox->addItem(item);
}

void CHelpBox::addPagebreak(void)
{

}

int CHelpBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CHelpBox::exec: timeout:%d\n", timeout);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	
	int result = messages_return::none;

	// paint
	paint();
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg;
		
	dprintf(DEBUG_NORMAL, "CHelpBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(timeout);

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ((msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_ok))
		{
			loop   = false;
		}
		else if (((msg == CRCInput::RC_up) || (msg == CRCInput::RC_down) || (msg == CRCInput::RC_page_up) || (msg == CRCInput::RC_page_down)))
		{
			if ( (msg == CRCInput::RC_up) || (msg == CRCInput::RC_page_up))
				scroll_up();
			else
				scroll_down();
		}
		else if((msg == CRCInput::RC_mode) || (msg == CRCInput::RC_next) || (msg == CRCInput::RC_prev)) 
		{
			loop = false;
			g_RCInput->postMsg(msg, data);
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

