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


CImageInfo::CImageInfo()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	widget = NULL;
	window = NULL;

	font_head   = SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME;
	font_small  = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;
	font_info   = SNeutrinoSettings::FONT_TYPE_MENU;

	hheight     = g_Font[font_head]->getHeight();
	iheight     = g_Font[font_info]->getHeight();
	sheight     = g_Font[font_small]->getHeight();

	//
	width = frameBuffer->getScreenWidth() - 10;
	height = frameBuffer->getScreenHeight() - 10;

	x = (((g_settings.screen_EndX - g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y = (((g_settings.screen_EndY - g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CImageInfo::~CImageInfo()
{ 
}

int CImageInfo::exec(CMenuTarget *parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CImageInfo::exec:\n");

	if (parent)
 		parent->hide();
 		
 	//
 	widget = CNeutrinoApp::getInstance()->getWidget("imageinfo");
 	
	if (widget == NULL)
	{
		widget = new CWidget(x, y, width, height);
		
		widget->name = "imageinfo";
	}
	
	window = new CWindow(x, y, width, height);
	
	// recalculate
	if (widget)
	{
		x = widget->getWindowsPos().iX;
		y = widget->getWindowsPos().iY;
		width = widget->getWindowsPos().iWidth;
		height = widget->getWindowsPos().iHeight;
		
		window->setPosition(x, y, width, height);
	}

	paint();

	frameBuffer->blit();	

	widget->exec(NULL, "");

	hide();

	return RETURN_REPAINT;
}

void CImageInfo::hide()
{
	widget->hide();
}

void CImageInfo::paintLine(int xpos, int font, const char* text)
{
	char buf[100];
	sprintf((char*) buf, "%s", text);
	
	g_Font[font]->RenderString(xpos, ypos, width - BORDER_RIGHT, buf, COL_INFOBAR, 0, true);
}

void CImageInfo::paint()
{
	const char * head_string;
 	int  xpos = x + 10;
	int x_offset = g_Font[font_info]->getRenderWidth(_("Home page:")) + 10;

	ypos = y + 5;

	head_string = _("Image info:");

	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8, head_string);

	//
	window->paint();
	
	// title
	g_Font[font_head]->RenderString(xpos, ypos + hheight + 10, width, head_string, COL_MENUHEAD, 0, true);

	ypos += hheight;
	ypos += (iheight >>1);


	//CConfigFile config('\t');
	//config.loadConfig("/etc/.version");

	const char * imagename = PACKAGE_NAME;
	//imagename = config.getString("imagename", "neutrino-HD2").c_str();
	const char * homepage = "https://github.com/mohousch/neutrino2";
	//homepage = config.getString("homepage", "http://gitorious.org/open-duckbox-project-sh4").c_str();
	const char * docs = "https://github.com/mohousch/neutrino2";
	//docs = config.getString("docs", "http://wiki.neutrino-hd.de").c_str();
	const char * forum = "https://forum.mbremer.de/viewforum.php?f=86";
	//forum = config.getString("forum", "http://gitorious.org/open-duckbox-project-sh4").c_str();
	//const char * version = config.getString("version", "1202201602031021").c_str();
	
	//static CFlashVersionInfo versionInfo(version);
	const char * releaseCycle = PACKAGE_VERSION;
	//releaseCycle = versionInfo.getReleaseCycle();
	const char * imageType = "Snapshot"; //FIXME:
	//imageType = versionInfo.getType();

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
	paintLine(xpos + x_offset + x_offset, font_info, _(__TIME__));

	// git rev
	//ypos += iheight;
	//paintLine(xpos, font_info, GIT_REV);
	//paintLine(xpos + x_offset, font_info, gitrev );	
	
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
	ypos += 5*iheight;
	paintLine(xpos, font_info, _("License:"));
	paintLine(xpos + x_offset, font_small, "This program is free software; you can redistribute it and/or modify");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "it under the terms of the GNU General Public License as published by");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "the Free Software Foundation; either version 2 of the License, or");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "(at your option) any later version.");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "This program is distributed in the hope that it will be useful,");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "but WITHOUT ANY WARRANTY; without even the implied warranty of");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "See the GNU General Public License for more details.");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "You should have received a copy of the GNU General Public License");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "along with this program; if not, write to the Free Software");

	ypos += sheight;
	paintLine(xpos + x_offset, font_small, "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.");	
}


