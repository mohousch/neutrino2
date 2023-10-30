/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouquetlist.h 16.11.2020 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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


#ifndef __bouquetlist__
#define __bouquetlist__

#include <gui/channellist.h>

#include <driver/framebuffer.h>
#include <system/lastchannel.h>

// zapit includes
#include <zapit/bouquets.h>

#include <string>
#include <vector>

#include <gui/widget/listbox.h>


typedef enum bouquetSwitchMode
{
	bsmBouquets,	// pressing OK shows list of all Bouquets
	bsmChannels,	// pressing OK shows list of all channels of active bouquets
	bsmAllChannels	// OK shows list of all channels
} BouquetSwitchMode;

class CBouquet
{
	public:
		int unique_key;
		bool bLocked;
		CChannelList*	channelList;
		CZapitBouquet* zapitBouquet;

		CBouquet(const int Unique_key, const char * const Name, const bool locked)
		{
			zapitBouquet = NULL;
			unique_key = Unique_key;
			bLocked = locked;
			channelList = new CChannelList(Name);
		}

		~CBouquet()
		{
			delete channelList;
		}
};

class CBouquetList
{
	private:
		CFrameBuffer *frameBuffer;

		//
		CBox cFrameBox;
		CWidget* bqWidget;
		ClistBox *listBox;
		CMenuItem *item;
		//
		uint32_t sec_timer_id;
		int selected;
		//
		void paint();
		void hide();

		std::string name;
		
		int doMenu();

	public:
		CBouquetList(const char* const Name = NULL);
		~CBouquetList();

		std::vector<CBouquet*> Bouquets;
		CChannelList* orgChannelList;
		
		CBouquet* addBouquet(const char* const _name, int BouquetKey = -1, bool locked = false );
		CBouquet* addBouquet(CZapitBouquet * zapitBouquet);
		void deleteBouquet(CBouquet* bouquet);
		int getActiveBouquetNumber();
		void activateBouquet(int id);
		int show(bool customMode = false);
		int showChannelList(int nBouquet = -1, bool customMode = false);
		void adjustToChannel(int nChannelNr);
		void adjustToChannelID(t_channel_id channel_id);
		int exec(bool bShowChannelList = true, bool custonMode = false);
};

#endif
