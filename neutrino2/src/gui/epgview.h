/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: epgview.h 26.09.2021 mohousch Exp $

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


#ifndef __epgview__
#define __epgview__

#include <vector>
#include <string>

#include <driver/framebuffer.h>
#include <driver/rcinput.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>

#include <driver/color.h>

#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>
#include <gui/widget/textbox.h>

#include <sectionsd/sectionsd.h>


class CEpgData
{
	private:
		CFrameBuffer* frameBuffer;

		//
		CBox cFrameBox;
		CBox cFollowScreeningBox;
		CBox cHeadBox;
		CBox cFootBox;
		CBox cTextBox;

		//
		CCHeaders* headers;
		CCFooters* footers;
		CTextBox *textBox;
		CCWindow *cFollowScreeningWindow;
		CWidget *widget;

		CChannelEventList evtlist;
		CEPGData epgData;

		std::string epg_date;
		std::string epg_start;
		std::string epg_end;
		int epg_done;
		uint64_t prev_id;
		time_t prev_zeit;
		uint64_t next_id;
		time_t next_zeit;

		std::string epgBuffer;
		
		void initFrames();

		void GetEPGData(const t_channel_id channel_id, uint64_t id, time_t* startzeit, bool clear = true);
		void GetPrevNextEPGData( uint64_t id, time_t* startzeit );
		bool hasFollowScreenings(const t_channel_id channel_id, const std::string & title);
		int FollowScreenings(const t_channel_id channel_id, const std::string & title);
		void showTimerEventBar(bool show);
		void showHead(const t_channel_id channel_id);

		//
		uint32_t sec_timer_id;
	public:

		CEpgData();
		~CEpgData();
		int show(const t_channel_id channel_id, uint64_t id = 0, time_t * startzeit = NULL, bool doLoop = true );
		void hide();
};

////
class CEPGDataHandler : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent,  const std::string& actionKey);
};

#endif

