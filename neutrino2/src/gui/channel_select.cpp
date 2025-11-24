//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: channel_select.cpp 21122024 mohousch Exp $
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// zapit includes
#include <zapit/bouquets.h>

#include "gui/channel_select.h"

#include <global.h>
#include <neutrino2.h>

#include <gui/bouquetlist.h>

#include <system/debug.h>


////
extern CBouquetList * bouquetList;
extern tallchans allchans;

CSelectChannelWidget::CSelectChannelWidget()
{
	tmpChannelList = NULL;
	ChannelID = 0;
}

CSelectChannelWidget::~CSelectChannelWidget()
{
	ChannelID = 0;
	
	if (tmpChannelList)
	{
		delete tmpChannelList;
		tmpChannelList = NULL;
	}
}

int CSelectChannelWidget::exec(CTarget *parent, const std::string &actionKey)
{
	int res = RETURN_REPAINT;
	
	dprintf(DEBUG_NORMAL, "CSelectChannelWidget::exec: actionKey:%s\n", actionKey.c_str());

	if (parent)
		parent->hide();

	if(actionKey == "tv")
	{
		InitZapitChannelHelper(CZapit::MODE_TV);
		return res;
	}
	else if(actionKey == "radio")
	{
		InitZapitChannelHelper(CZapit::MODE_RADIO);
		return res;
	}

	return res;
}

void CSelectChannelWidget::InitZapitChannelHelper(CZapit::channelsMode mode)
{
	dprintf(DEBUG_NORMAL, "CSelectChannelWidget::InitZapitChannelHelper:\n");
	
	tmpChannelList = new CChannelList(_("Select channel"));
	
	if(mode == CZapit::MODE_TV)
	{
		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
		{
			if ((it->second.getServiceType() == ST_DIGITAL_TELEVISION_SERVICE)) 
			{
				tmpChannelList->addChannel(&(it->second));
			}
		}
	}
	else if(mode == CZapit::MODE_RADIO)
	{
		for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
		{
			if ((it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)) 
			{
				tmpChannelList->addChannel(&(it->second));
			}
		}
	}
	
	tmpChannelList->SortAlpha();
	tmpChannelList->exec(true); // without zap
	
	ChannelID = tmpChannelList->getActiveChannel_ChannelID();
}

