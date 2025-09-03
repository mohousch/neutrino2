//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: infobox.cpp 10022025 mohousch Exp $
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

#include <infobox.h>

#include <neutrino2.h>

#include <system/debug.h>
#include <system/settings.h>
	

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Function Name:	CInfoBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
///////////////////
const struct button_label HButton = { NEUTRINO_ICON_BUTTON_HELP, " ", 0 };
const struct button_label FButton = { NEUTRINO_ICON_INFO, " ", 0 };

CInfoBox::CInfoBox(const int x, const int y, const int dx, const int dy, const char * title, const char * icon)
{
	//
	widget = NULL;
	textBox = NULL;
	headers = NULL;
	footers = NULL;
	
	//
	initVar();

	if(title != NULL)		
		m_cTitle = title;
	
	if(icon != NULL)		
		m_cIcon = icon;

	CBox position(x, y, dx, dy); 

	cFrameBox = position;

	// initialise the window frames first
//	initFrames();

	cFrameBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - cFrameBox.iWidth) >>1);
	cFrameBox.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - cFrameBox.iHeight) >>1);
	
	initFrames();
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("infobox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
		footers = (CCFooters*)widget->getCCItem(CComponent::CC_FOOT);
		textBox = (CTextBox*)widget->getCCItem(CComponent::CC_TEXTBOX);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		headers = new CCHeaders();
		footers = new CCFooters();
		textBox = new CTextBox(&cFrameBoxText);
		
		widget->addCCItem(textBox);
		widget->addCCItem(headers);
		widget->addCCItem(footers);
		
		widget->paintMainFrame(true);
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	}
	
	if (textBox)
	{
		textBox->setPosition(&cFrameBoxText);
	}
	
	if (headers)
	{
		headers->setButtons(&HButton, 1);
	}
	
	if (footers)
	{
		footers->setButtons(&FButton, 1);
	}
}

CInfoBox::CInfoBox(const CBox* position, const char * title, const char * icon)
{
	//
	widget = NULL;
	textBox = NULL;
	headers = NULL;
	footers = NULL;
	
	//
	initVar();

	if(title != NULL)		
		m_cTitle = title;
	
	if(icon != NULL)		
		m_cIcon = icon;
	
	if(position != NULL)	
		cFrameBox = *position;

	// initialise the window frames first
//	initFrames();

	//
	cFrameBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - cFrameBox.iWidth) >>1);
	cFrameBox.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - cFrameBox.iHeight) >>1);
	
	initFrames();
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("infobox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
		footers = (CCFooters*)widget->getCCItem(CComponent::CC_FOOT);
		textBox = (CTextBox*)widget->getCCItem(CComponent::CC_TEXTBOX);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		headers = new CCHeaders();
		footers = new CCFooters();
		textBox = new CTextBox(&cFrameBoxText);
		
		widget->addCCItem(textBox);
		widget->addCCItem(headers);
		widget->addCCItem(footers);
		
		widget->paintMainFrame(true);
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	}
	
	if (textBox)
	{
		textBox->setPosition(&cFrameBoxText);
	}
	
	if (headers)
	{
		headers->setButtons(&HButton, 1);
	}
	
	if (footers)
	{
		footers->setButtons(&FButton, 1);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	CInfoBox()	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CInfoBox::CInfoBox()
{
	//
	widget = NULL;
	textBox = NULL;
	headers = NULL;
	footers = NULL;
	
	//
	initVar();

	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 

	cFrameBox = position;

	// initialise the window frames first
//	initFrames();

	//
	cFrameBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - cFrameBox.iWidth) >>1);
	cFrameBox.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - cFrameBox.iHeight) >>1);
	
	initFrames();
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("infobox");
	
	if (widget)
	{
		headers = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
		footers = (CCFooters*)widget->getCCItem(CComponent::CC_FOOT);
		textBox = (CTextBox*)widget->getCCItem(CComponent::CC_TEXTBOX);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		
		headers = new CCHeaders();
		footers = new CCFooters();
		textBox = new CTextBox(&cFrameBoxText);
		
		widget->addCCItem(textBox);
		widget->addCCItem(headers);
		widget->addCCItem(footers);
		
		widget->paintMainFrame(true);
		widget->setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	}
	
	if (textBox)
	{
		textBox->setPosition(&cFrameBoxText);
	}
	
	if (headers)
	{
		headers->setButtons(&HButton, 1);
	}
	
	if (footers)
	{
		footers->setButtons(&FButton, 1);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	~CInfoBox	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
CInfoBox::~CInfoBox()
{
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	InitVar	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::initVar(void)
{
	// head
	m_cTitle = _("Information");
	m_cIcon = NEUTRINO_ICON_INFO;
	headColor = COL_MENUHEAD_PLUS_0;
	headRadius = g_settings.Head_radius;
	headCorner = CORNER_TOP;
	headGradient = g_settings.Head_gradient;
	
	// text
	m_pcFontText = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	
	// foot
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = CORNER_BOTTOM;
	footGradient = g_settings.Foot_gradient;

	// set the main frame to default
	cFrameBox.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - MIN_WINDOW_WIDTH) >>1);
	cFrameBox.iWidth = MIN_WINDOW_WIDTH;
	cFrameBox.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - MIN_WINDOW_HEIGHT) >>1);
	cFrameBox.iHeight = MIN_WINDOW_HEIGHT;

	frameBuffer = CFrameBuffer::getInstance();
	
	borderMode = CComponent::BORDER_NO;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	InitFrames
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::initFrames(void)
{
	// init the title frame
	cFrameBoxTitleRel.iX		= cFrameBox.iX;
	cFrameBoxTitleRel.iY		= cFrameBox.iY;
	cFrameBoxTitleRel.iWidth	= cFrameBox.iWidth;
	cFrameBoxTitleRel.iHeight	= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;

	// init the foot frame
	cFrameBoxFootRel.iX		= cFrameBox.iX;
	cFrameBoxFootRel.iWidth		= cFrameBox.iWidth;
	cFrameBoxFootRel.iHeight	= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	cFrameBoxFootRel.iY		= cFrameBox.iY + cFrameBox.iHeight - cFrameBoxFootRel.iHeight;

	// init the text frame
	cFrameBoxText.iY		= cFrameBox.iY + cFrameBoxTitleRel.iHeight;
	cFrameBoxText.iX		= cFrameBox.iX;
	cFrameBoxText.iHeight		= cFrameBox.iHeight - cFrameBoxTitleRel.iHeight - cFrameBoxFootRel.iHeight;
	cFrameBoxText.iWidth		= cFrameBox.iWidth;	
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshTitle	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::refreshTitle(void)
{
	if (headers)
	{
		headers->setPosition(&cFrameBoxTitleRel);
		headers->setTitle(m_cTitle.c_str(), m_cIcon.c_str());
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshFoot	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::refreshFoot(void)
{
	if (footers)
	{
		footers->setPosition(&cFrameBoxFootRel);
	}
}

//////////////////////////////////////////////////////////////////////
// global Functions
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Function Name:	Hide	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::hide(void)
{
	widget->hide();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageDown	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::scrollPageDown(const int pages)
{
	// send scroll up event to text box if there is one
	if(textBox != NULL)
	{
		textBox->scrollPageDown(pages);
	}

}

//////////////////////////////////////////////////////////////////////
// Function Name:	ScrollPageUp	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::scrollPageUp(const int pages)
{
	// send scroll up event to text box if there is one
	if(textBox != NULL)
	{
		textBox->scrollPageUp(pages);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	setBigFonts	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::setBigFonts()
{
	dprintf(DEBUG_NORMAL, "CInfoBox::setBigFonts\n");

	// send setBigFonts event to textbox if there is one
	if(textBox != NULL)
	{
		textBox->setBigFonts();
	}
}

//
void CInfoBox::setBackgroundColor(fb_pixel_t col)
{
	if(textBox != NULL)
		textBox->setBackgroundColor(col);
}

//
void CInfoBox::setTextColor(uint32_t col)
{
	if(textBox != NULL)
		textBox->setTextColor(col);
}

//
void CInfoBox::setFont(unsigned int font_text)
{
	m_pcFontText = font_text;
	
	if(textBox != NULL)
		textBox->setFont(m_pcFontText);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	SetText	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::setText(const char * const newText, const char * const _thumbnail, int _tw, int _th, int tmode, bool enable_frame)
{
	// update text in textbox if there is one
	if(textBox != NULL && newText != NULL)
	{
		textBox->setText(newText, _thumbnail, _tw, _th, tmode, enable_frame);
	}
}

//////////////////////////////////////////////////////////////////////
// Function Name:	Paint	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::paint(void)
{
	dprintf(DEBUG_DEBUG, "CInfoBox::paint\n");

	// title
	refreshTitle();

	// foot
	refreshFoot();
	
	widget->paint();
}

//////////////////////////////////////////////////////////////////////
// Function Name:	Exec	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
int CInfoBox::exec(int timeout)
{
	dprintf(DEBUG_NORMAL, "CInfoBox::exec: timeout:%d\n", timeout);

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = CWidgetTarget::RETURN_REPAINT;

	// show infobox
	paint();
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg;

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( (msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_info))
		{
			loop = false;
		}
		else if (msg == CRCInput::RC_up || msg == CRCInput::RC_page_up)
		{
			scrollPageUp(1);
		}
		else if (msg == CRCInput::RC_down || msg == CRCInput::RC_page_down)
		{
			scrollPageDown(1);
		}
		else if(msg == CRCInput::RC_ok)
		{
			loop = false;
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			res  = CWidgetTarget::RETURN_EXIT_ALL;
			loop = false;
		}

		frameBuffer->blit();
	}

	hide();
	
	return res;
}

//
void InfoBox(const char * const title, const char * const text, const char * const icon, const char * const thumbnail, int tw, int th, int tmode)
{
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, title, icon);

	//
	infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	infoBox->setText(text, thumbnail, tw, th, tmode);
	infoBox->exec();
	delete infoBox;
	infoBox = NULL;
}

