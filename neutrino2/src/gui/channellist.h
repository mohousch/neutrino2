//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: channellist.h 21122024 mohousch Exp $
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

#ifndef __channellist__
#define __channellist__

#include <driver/gdi/framebuffer.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>
#include <gui/widget/stringinput.h>

#include <system/lastchannel.h>

#include <sectionsd/sectionsd.h>

// zapit includes
#include <zapit/zapit.h>
#include <zapit/channel.h>

#include <string>
#include <vector>

#include <daemonc/remotecontrol.h>


////
class CChannelList
{
	public:
		enum 
		{
			LIST_MODE_FAV,
			LIST_MODE_PROV,
			LIST_MODE_SAT,
			LIST_MODE_ALL
		};
		
		enum
		{
			CHANNEL_NUMBER_NONE,
			CHANNEL_NUMBER_LIST_ORDER,
			CHANNEL_NUMBER_REALNUMBER
		};

	private:
		CFrameBuffer *frameBuffer;

		//
		CBox cFrameBox;
		CBox winTopBox;
		CBox winBottomBox;
		//
		CWidget* widget;
		ClistBox *listBox;
		CMenuItem *item;
		CCHeaders* head;
		CCFooters* foot;
		CCWindow* window;
		CCVline* vline;
		CCHline* hline;
		//
		uint32_t sec_timer_id;
		int selected;
		//
		void paint(bool customMode = false);
		void hide();
		void paintCurrentNextEvent(int _selected);

		//
		t_channel_id tuned_chid;
//		CLastChannel lastChList;
		std::string name;
		ZapitChannelList chanlist;
		CChannelEventList events;

		bool historyMode;
		bool displayNext;

		SMSKeyInput c_SMSKeyInput;

	public:
		CChannelList(const char * const Name, bool _historyMode = false);
		~CChannelList();
		
		void addChannel(CZapitChannel* chan, unsigned int i = 0);
		
		CZapitChannel * getChannel(int number);
		CZapitChannel * getChannel(t_channel_id channel_id);
		CZapitChannel * getChannelFromIndex( uint32_t index) { if (chanlist.size() > index) return chanlist[index]; else return NULL;};
		CZapitChannel * operator[]( uint32_t index) { if (chanlist.size() > index) return chanlist[index]; else return NULL;};
		int getKey(int);

		const char* getName(void) const { return name.c_str(); };
		////
		const std::string&   getActiveChannelName(void) const; // UTF-8
		t_satellite_position getActiveSatellitePosition(void) const;
		int                  getActiveChannelNumber(void) const;
		t_channel_id         getActiveChannel_ChannelID(void) const;
		t_channel_id         getActiveChannel_EPGID(void) const;
		t_channel_id         getActiveChannel_LogoID(void) const;
		////
		void zapTo(int pos, bool rezap = false);
		void virtual_zap_mode(bool up);
		bool zapToChannelID(const t_channel_id channel_id, bool rezap = false);
		bool adjustToChannelID(const t_channel_id channel_id, bool bToo = true);
		bool showInfo(int pos, int epgpos = 0, bool fromNumZap = true);
		void updateEvents(void);
		int numericZap(int key);
		void quickZap(int key, bool cycle = false);
		int hasChannel(int nChannelNr);
		int hasChannelID(t_channel_id channel_id);
		void setSelected( int nChannelNr); 	// for adjusting bouquet's channel list after numzap or quickzap
		int getSize() const;
		int getSelectedChannelIndex() const;
		void setSize(int newsize);
		unsigned int tuned;
		int doChannelMenu(void);
		void SortAlpha(void);
		void SortSat(void);
		void ClearList(void);
		bool canZap(CZapitChannel* channel = NULL);
		int show(bool customMode = false);
		int exec(bool customMode = false);
};

#endif

