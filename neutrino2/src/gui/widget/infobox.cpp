/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: infobox.cpp 2016.12.02 12:25:30 mohousch Exp $

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

#include <infobox.h>

#include <gui/widget/icons.h>
#include <neutrino.h>

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
CInfoBox::CInfoBox(/*CFont *fontText, const int _mode,*/ const CBox* position, const char * title, const char * icon)
{
	initVar();

	if(title != NULL)		
		m_cTitle = title;

	//if(fontText != NULL)
	//	m_pcFontText = fontText;
	
	if(icon != NULL)		
		m_cIcon = icon;
	
	if(position != NULL)	
		m_cBoxFrame = *position;
	
	//m_nMode = _mode;

	// initialise the window frames first
	initFramesRel();

	m_pcTextBox = new CTextBox(&m_cBoxFrameText);
	
	//m_pcTextBox->setFont(fontText);
	//m_pcTextBox->setMode(m_nMode);
	//m_pcTextBox->setBorderMode(borderMode);

	if(m_nMode & AUTO_WIDTH || m_nMode & AUTO_HIGH)
	{
		// window might changed in size
		m_cBoxFrameText = m_pcTextBox->getWindowsPos();

		m_cBoxFrame.iWidth = m_cBoxFrameText.iWidth;
		m_cBoxFrame.iHeight = m_cBoxFrameText.iHeight +  m_cBoxFrameTitleRel.iHeight;

		initFramesRel();
	}

	//
	m_cBoxFrame.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - m_cBoxFrame.iWidth) >>1);
	m_cBoxFrame.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - m_cBoxFrame.iHeight) >>1);
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
	initVar();

	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 

	m_cBoxFrame = position;

	// initialise the window frames first
	initFramesRel();

	m_pcTextBox = new CTextBox();

	m_pcTextBox->setPosition(&m_cBoxFrameText);

	if(m_nMode & AUTO_WIDTH || m_nMode & AUTO_HIGH)
	{
		// window might changed in size
		m_cBoxFrameText = m_pcTextBox->getWindowsPos();

		m_cBoxFrame.iWidth = m_cBoxFrameText.iWidth;
		m_cBoxFrame.iHeight = m_cBoxFrameText.iHeight +  m_cBoxFrameTitleRel.iHeight;

		initFramesRel();
	}

	//if(m_nMode & CENTER)
	{
		m_cBoxFrame.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - m_cBoxFrame.iWidth) >>1);
		m_cBoxFrame.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - m_cBoxFrame.iHeight) >>1);
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
	if (m_pcTextBox != NULL)
	{
		delete m_pcTextBox;
		m_pcTextBox = NULL;
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
	m_nMode = SCROLL;
	m_pcFontText = SNeutrinoSettings::FONT_TYPE_EPG_INFO1;
	
	// foot
	footColor = COL_MENUFOOT_PLUS_0;
	footRadius = g_settings.Foot_radius;
	footCorner = CORNER_BOTTOM;
	footGradient = g_settings.Foot_gradient;

	// set the main frame to default
	m_cBoxFrame.iX = g_settings.screen_StartX + ((g_settings.screen_EndX - g_settings.screen_StartX - MIN_WINDOW_WIDTH) >>1);
	m_cBoxFrame.iWidth = MIN_WINDOW_WIDTH;
	m_cBoxFrame.iY = g_settings.screen_StartY + ((g_settings.screen_EndY - g_settings.screen_StartY - MIN_WINDOW_HEIGHT) >>1);
	m_cBoxFrame.iHeight = MIN_WINDOW_HEIGHT;

	frameBuffer = CFrameBuffer::getInstance();
	
	borderMode = BORDER_NO;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	InitFramesRel	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::initFramesRel(void)
{
	// init the title frame
	m_cBoxFrameTitleRel.iX		= m_cBoxFrame.iX;
	m_cBoxFrameTitleRel.iY		= m_cBoxFrame.iY;
	m_cBoxFrameTitleRel.iWidth	= m_cBoxFrame.iWidth;
	m_cBoxFrameTitleRel.iHeight	= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;

	// init the foot frame
	m_cBoxFrameFootRel.iX		= m_cBoxFrame.iX;
	m_cBoxFrameFootRel.iWidth	= m_cBoxFrame.iWidth;
	m_cBoxFrameFootRel.iHeight	= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight() + 6;
	m_cBoxFrameFootRel.iY		= m_cBoxFrame.iY + m_cBoxFrame.iHeight - m_cBoxFrameFootRel.iHeight;

	// init the text frame
	m_cBoxFrameText.iY		= m_cBoxFrame.iY + m_cBoxFrameTitleRel.iHeight;
	m_cBoxFrameText.iX		= m_cBoxFrame.iX;
	m_cBoxFrameText.iHeight	= m_cBoxFrame.iHeight - m_cBoxFrameTitleRel.iHeight - m_cBoxFrameFootRel.iHeight;
	m_cBoxFrameText.iWidth		= m_cBoxFrame.iWidth;	
}

//////////////////////////////////////////////////////////////////////
// Function Name:	RefreshTitle	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
const struct button_label HButton = { NEUTRINO_ICON_BUTTON_HELP, " "};

void CInfoBox::refreshTitle(void)
{
	CHeaders headers(&m_cBoxFrameTitleRel, m_cTitle.c_str(), m_cIcon.c_str());

	headers.setColor(headColor);
	headers.setCorner(headCorner);
	headers.setRadius(headRadius);
	headers.setGradient(headGradient);
	headers.setButtons(&HButton, 1);
	
	headers.paint();
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
	CFooters footers(&m_cBoxFrameFootRel);
	struct button_label Button = { NEUTRINO_ICON_INFO, " " };
	
	footers.setColor(footColor);
	footers.setRadius(footRadius);
	footers.setCorner(footCorner);
	footers.setGradient(footGradient);
	footers.setButtons(&Button);
	
	footers.paint();
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
bool CInfoBox::hide(void)
{
/*
	if(m_pcTextBox != NULL)
	{
		m_pcTextBox->hide();
	}
*/	

	// hide
	frameBuffer->paintBackgroundBoxRel(m_cBoxFrame.iX, m_cBoxFrame.iY, m_cBoxFrame.iWidth, m_cBoxFrame.iHeight);
	
	frameBuffer->blit();
	
	return (true);
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
	if(m_pcTextBox != NULL)
	{
		m_pcTextBox->scrollPageDown(pages);
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
	if(m_pcTextBox != NULL)
	{
		m_pcTextBox->scrollPageUp(pages);
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
	if(m_pcTextBox != NULL)
	{
		m_pcTextBox->setBigFonts();
	}
}

//
void CInfoBox::setBackgroundColor(fb_pixel_t col)
{
	if(m_pcTextBox != NULL)
		m_pcTextBox->setBackgroundColor(col);
}

//
void CInfoBox::setTextColor(uint8_t col)
{
	if(m_pcTextBox != NULL)
		m_pcTextBox->setTextColor(col);
}

//
void CInfoBox::setFont(unsigned int font_text)
{
	//if(m_pcTextBox != NULL)
		m_pcTextBox->setFont(font_text);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	Paint	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CInfoBox::paint(void)
{
	dprintf(DEBUG_DEBUG, "CInfoBox::paint\n");

	// title
	refreshTitle();

	// textBox
	if(m_pcTextBox != NULL)
	{	
		// paint
		m_pcTextBox->paint();
	}

	// foot
	refreshFoot();
	
	return (true);
}

//////////////////////////////////////////////////////////////////////
// Function Name:	Refresh	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
void CInfoBox::refresh(void)
{
	//refresh title
	refreshTitle();

	// rep-draw textbox if there is one
	if(m_pcTextBox != NULL)
	{
		m_pcTextBox->refresh();
	}

	refreshFoot();
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
	int res = RETURN_REPAINT;

	// show infobox
	paint();
	CFrameBuffer::getInstance()->blit();

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_EPG];
		
	dprintf(DEBUG_NORMAL, "CInfoBox::exec: timeout:%d\n", timeout);

	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	bool loop = true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (((msg == RC_timeout) || (msg == RC_home)))
		{
			loop = false;
		}
		else if (msg == RC_up || msg == RC_page_up)
		{
			scrollPageUp(1);
		}
		else if (msg == RC_down || msg == RC_page_down)
		{
			scrollPageDown(1);
		}
		else if(msg == RC_ok)
		{
			loop = false;
		}
		else if(msg == RC_info)
		{
			setBigFonts();
		}
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		{
			res  = RETURN_EXIT_ALL;
			loop = false;
		}

		frameBuffer->blit();
	}

	hide();
	
	return res;
}

//////////////////////////////////////////////////////////////////////
// Function Name:	SetText	
// Description:		
// Parameters:		
// Data IN/OUT:		
// Return:		
// Notes:		
//////////////////////////////////////////////////////////////////////
bool CInfoBox::setText(const char * const newText, const char * const _thumbnail, int _tw, int _th, int tmode, bool enable_frame, const bool useBackground)
{
	bool _result = false;
	
	// update text in textbox if there is one
	if(m_pcTextBox != NULL && newText != NULL)
	{
		_result = m_pcTextBox->setText(newText, _thumbnail, _tw, _th, tmode, enable_frame, useBackground);
	}
	
	return(_result);
}

//
void InfoBox(const char * const text, const char * const title, const char * const icon, const char * const thumbnail, int tw, int th, int tmode)
{
	CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
	CInfoBox * infoBox = new CInfoBox(&position, title, icon);

	//
	infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	infoBox->setMode(SCROLL);
	infoBox->setText(text, thumbnail, tw, th, tmode);
	infoBox->exec();
	delete infoBox;
	infoBox = NULL;
}




