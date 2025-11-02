//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: channel_select.h 21122024 mohousch Exp $
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

#ifndef __CHANNEL_SELECT__
#define __CHANNEL_SELECT__

#include <string>

// zapit includes
#include <zapit/zapit.h>

#include <gui/widget/component.h>
#include <gui/channellist.h>

		
class CSelectChannelWidget : public CTarget
{	
	private:
		CChannelList *tmpChannelList;
		t_channel_id ChannelID;

		void InitZapitChannelHelper(CZapit::channelsMode mode);

	public:
		CSelectChannelWidget();
		~CSelectChannelWidget();
		int exec(CTarget *parent, const std::string & actionKey);

		t_channel_id getChannelID(){return ChannelID;};
};

#endif

