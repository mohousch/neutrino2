/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: imageinfo.cpp 2013/10/12 mohousch Exp $
	
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

#include <gui/imageinfo.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <daemonc/remotecontrol.h>

#include <system/flashtool.h>
#include <system/debug.h>

#include <gui/widget/widget_helpers.h>


CImageInfo::CImageInfo()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	widget = NULL;
	head = NULL;
	sec_timer_id = 0;

	font_info   = SNeutrinoSettings::FONT_TYPE_MENU;
	iheight     = g_Font[font_info]->getHeight();

	//
	width = frameBuffer->getScreenWidth() - 10;
	height = frameBuffer->getScreenHeight() - 10;

	x = (((g_settings.screen_EndX - g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y = (((g_settings.screen_EndY - g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CImageInfo::~CImageInfo()
{ 	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CImageInfo::exec(CMenuTarget *parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CImageInfo::exec:\n");
	
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	if (parent)
 		parent->hide();
 		
	//
	paint();

	frameBuffer->blit();	

	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	bool loop = true;
	
	while(loop)
	{
		g_RCInput->getMsg_ms(&msg, &data, 10); // 1 sec
		
		if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			widget->refresh();
		} 
		else if (msg == CRCInput::RC_home) 
		{
			loop = false;
		}
		
		frameBuffer->blit();
	}

	hide();
	
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	return CMenuTarget::RETURN_REPAINT;
}

void CImageInfo::hide()
{
	widget->hide();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	char buf[100];
	sprintf((char*) buf, "%s", text);
	
	g_Font[font]->RenderString(xpos, ypos, width - BORDER_RIGHT, buf, COL_INFOBAR_TEXT_PLUS_0, 0, true);
}

void CImageInfo::paint()
{
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
 	widget = CNeutrinoApp::getInstance()->getWidget("imageinfo");
 	
	//
	if (widget)
	{
		x = widget->getWindowsPos().iX;
		y = widget->getWindowsPos().iY;
		width = widget->getWindowsPos().iWidth;
		height = widget->getWindowsPos().iHeight;
		
		head = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		widget = new CWidget(x, y, width, height);
		
		widget->name = "imageinfo";
		widget->paintMainFrame(true);
		
		// head
		head = new CCHeaders(x, y, width, 40, _("Image Info"), NEUTRINO_ICON_INFO);
		head->enablePaintDate();
		head->setFormat("%d.%m.%Y %H:%M:%S");
		
		widget->addCCItem(head);

	}
	
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, _("Image info"));
	
 	int  xpos = x + 10;
	int x_offset = g_Font[font_info]->getRenderWidth(_("Home page:")) + 10;

	ypos = head? y + head->getWindowsPos().iHeight + 10 : 60;

	//
	widget->paint();
	
	//
	ypos += (iheight >>1);

	const char * imagename = PACKAGE_NAME;
	const char * homepage = "https://github.com/mohousch/neutrino2";
	const char * docs = "https://github.com/mohousch/neutrino2";
	const char * forum = "https://forum.mbremer.de/viewforum.php?f=86";
	const char * releaseCycle = PACKAGE_VERSION;
	const char * imageType = "Snapshot"; //FIXME:

	// image name
	ypos += iheight;
	paintLine(xpos, font_info, _("Image:"));
	paintLine(xpos + x_offset, font_info, imagename);

	// release cycle
	ypos += iheight;
	paintLine(xpos, font_info, _("Version:"));
	paintLine(xpos + x_offset, font_info, releaseCycle);
	
	// git built date
	ypos += iheight;
	paintLine(xpos, font_info, _("Built date: "));
	paintLine(xpos + x_offset, font_info, _(__DATE__));
	paintLine(xpos + x_offset + x_offset + 10, font_info, _(__TIME__));
	
	// image type
	ypos += iheight;
	paintLine(xpos, font_info, _("Type:"));
	paintLine(xpos + x_offset, font_info, imageType);

	// 
	ypos += iheight;

	// doko
	ypos += iheight;
	paintLine(xpos, font_info, _("Docs:"));
	paintLine(xpos + x_offset, font_info, docs);

	// forum
	ypos += iheight;
	paintLine(xpos, font_info, _("Forum:"));
	paintLine(xpos + x_offset, font_info, forum);

	// homepage
	ypos += iheight;
	paintLine(xpos, font_info, _("Home page:"));
	paintLine(xpos + x_offset, font_info, homepage);

	// license
	ypos += 2*iheight;
	paintLine(xpos, font_info, _("License:"));
	
	std::string Text = "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\nSee the GNU General Public License for more details.\nYou should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.";
	
	CCText licence;
	licence.setPosition(xpos + x_offset, ypos, width - x_offset - 20, height - 2*iheight);
	licence.setText(Text.c_str());
	
	licence.paint();
}

