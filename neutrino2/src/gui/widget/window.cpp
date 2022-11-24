/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: window.cpp 2016.12.12 11:43:30 mohousch Exp $

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

#include <gui/widget/window.h>
#include <system/debug.h>


CWindow::CWindow(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;

	init();
}

CWindow::CWindow(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	frameBuffer = CFrameBuffer::getInstance(); 
	
	itemBox = *position;

	init();
}

CWindow::~CWindow()
{
	if(background)
	{
		delete[] background;
		background = NULL;
	}
	
	if (hasCCItem())
	{
		CCItems.clear();
	}
}

void CWindow::init()
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);

	radius = NO_RADIUS;
	corner = CORNER_NONE;
	bgcolor = COL_MENUCONTENT_PLUS_0;
	gradient = NOGRADIENT;
	grad_direction = GRADIENT_VERTICAL;
	grad_intensity = INT_LIGHT;
	grad_type = GRADIENT_ONECOLOR;

	//
	borderMode = BORDER_NO;
	paintFrame = true;

	savescreen = false;
	background = NULL;

	// sanity check
	if(itemBox.iHeight > ((int)frameBuffer->getScreenHeight(true)))
		itemBox.iHeight = frameBuffer->getScreenHeight(true);

	// sanity check
	if(itemBox.iWidth > (int)frameBuffer->getScreenWidth(true))
		itemBox.iWidth = frameBuffer->getScreenWidth(true);
	
	//
	widgetItem_type = WIDGETITEM_WINDOW;
}

void CWindow::saveScreen()
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);

	if (savescreen)
	{
		if(background)
		{
			delete[] background;
			background = NULL;
		}

		background = new fb_pixel_t[itemBox.iWidth*itemBox.iHeight];
		
		if(background)
		{
			frameBuffer->saveScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
		}
	}
}

void CWindow::restoreScreen()
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	if(savescreen && background) 
	{
		frameBuffer->restoreScreen(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, background);
	}
	
	CFrameBuffer::getInstance()->blit();
}

void CWindow::enableSaveScreen()
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	savescreen = true;
	
	saveScreen();
}

void CWindow::setPosition(const int x, const int y, const int dx, const int dy)
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	itemBox.iX = x;
	itemBox.iY = y;
	itemBox.iWidth = dx;
	itemBox.iHeight = dy;
	
	init();
}

void CWindow::setPosition(CBox* position)
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	itemBox = *position;
	
	init();
}

void CWindow::paint()
{
	dprintf(DEBUG_INFO, "CWindow::%s\n", __FUNCTION__);

	if (paintFrame)
	{
		if (borderMode == BORDER_NO)
		{
			// frame
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_ALL)
		{
			// border
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_6, radius, corner);
			
			// frame
			frameBuffer->paintBoxRel(itemBox.iX + 2, itemBox.iY + 2, itemBox.iWidth - 4, itemBox.iHeight - 4, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_LEFTRIGHT)
		{
			// border
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_6, radius, corner);
			
			// frame
			frameBuffer->paintBoxRel(itemBox.iX + 2, itemBox.iY, itemBox.iWidth - 4, itemBox.iHeight, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
		else if (borderMode == BORDER_TOPBOTTOM)
		{
			// border
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight, COL_MENUCONTENT_PLUS_6, radius, corner);
			
			// frame
			frameBuffer->paintBoxRel(itemBox.iX, itemBox.iY + 2, itemBox.iWidth, itemBox.iHeight - 4, bgcolor, radius, corner, gradient, grad_direction, grad_intensity, grad_type);
		}
	}
		
	//CCItems
	if (hasCCItem())
	{
		paintCCItems();
	}
}

void CWindow::hide()
{
	dprintf(DEBUG_DEBUG, "CWindow::%s\n", __FUNCTION__);
	
	if( savescreen)
		restoreScreen();
	else
		frameBuffer->paintBackgroundBoxRel(itemBox.iX, itemBox.iY, itemBox.iWidth, itemBox.iHeight);
		
	// ??? CCPIG
	if (hasCCItem())
	{
		for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
		{
			if (CCItems[count]->getCCType() == CC_PIG)
			{
				CCItems[count]->hide();
				break;
			}
		}
	}
		
	CFrameBuffer::getInstance()->blit();
}

//
void CWindow::addCCItem(CComponent* CCItem)
{
	CCItems.push_back(CCItem);
}

//
void CWindow::paintCCItems()
{
	dprintf(DEBUG_INFO, "CWindow::paintCCItems:\n");

	for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
	{
		CCItems[count]->paint();
	}
}

//
void CWindow::refresh(void)
{
	for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
	{
		if (CCItems[count]->update())
		{
			CCItems[count]->refresh();
		}
	}
}

//
bool CWindow::update() const
{
	bool ret = false;
	
	for (unsigned int count = 0; count < (unsigned int)CCItems.size(); count++) 
	{
		if (CCItems[count]->update())
		{
			ret = true;
		}
	}
	
	return ret;
}



