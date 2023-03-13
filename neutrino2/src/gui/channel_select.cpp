/*
	* $Id: channel_select.cpp 16.11.2020 mohousch Exp $
	
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

// zapit includes
#include <zapit/bouquets.h>

#include "gui/channel_select.h"

#include <global.h>
#include <neutrino2.h>

#include <gui/bouquetlist.h>

#include <system/debug.h>


extern CBouquetList * bouquetList;

//select menu
CSelectChannelWidget::CSelectChannelWidget()
{
	ChannelID = 0;
}

CSelectChannelWidget::~CSelectChannelWidget()
{
	ChannelID = 0;
}

int CSelectChannelWidget::exec(CMenuTarget *parent, const std::string &actionKey)
{
	int   res = RETURN_REPAINT;
	
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

	// save channel mode
	int channelMode = g_settings.channel_mode;
	int nNewChannel = -1;
	int nActivBouquet = -1;
	int activBouquet = 0;
	int activChannel = 0;
	int nMode = CNeutrinoApp::getInstance()->getMode();

	// set mode
	if(mode == CZapit::MODE_TV)
	{
		CNeutrinoApp::getInstance()->setChannelMode(g_settings.channel_mode, NeutrinoMessages::mode_tv);
	}
	else if(mode == CZapit::MODE_RADIO)
	{
		CNeutrinoApp::getInstance()->setChannelMode(g_settings.channel_mode, NeutrinoMessages::mode_radio);
	}
	
_repeat:	
	// get activ bouquet and channel number
	if(bouquetList->Bouquets.size())
	{ 
		activBouquet = bouquetList->getActiveBouquetNumber();
		activChannel = bouquetList->Bouquets[activBouquet]->channelList->getActiveChannelNumber();
	}

	dprintf(DEBUG_NORMAL, "CChannellistWidget: activChannel: %d activBouquet:%d\n", activChannel, activBouquet);

	// show channel list
	if(bouquetList->Bouquets.size() && bouquetList->Bouquets[activBouquet]->channelList->getSize() > 0)
		nNewChannel = bouquetList->Bouquets[activBouquet]->channelList->show(false, true);
	else
		nNewChannel = bouquetList->show(true, true);

	if(bouquetList->Bouquets.size())
	{ 
		nActivBouquet = bouquetList->getActiveBouquetNumber();
	}

	dprintf(DEBUG_NORMAL, "CChannellistWidget: nNewChannel: %d nActivBouquet:%d\n", nNewChannel, nActivBouquet);

	// handle list mode changed
	if(nNewChannel == -3) // channel mode changed
	{ 
		goto _repeat;
	}

	// get our channel
	if (nNewChannel == -1) // by exit
	{
		// do not thing
	}
	else
	{
		ChannelID = bouquetList->Bouquets[nActivBouquet]->channelList->getActiveChannel_ChannelID();
	}
	
	// set last channel mode
	CNeutrinoApp::getInstance()->setChannelMode(channelMode, nMode);

	// set last activ bouquet and channel
	if(bouquetList->Bouquets.size()) 
	{
		bouquetList->activateBouquet(activBouquet, false, false);
		bouquetList->Bouquets[activBouquet]->channelList->setSelected(activChannel - 1);
	}
}


