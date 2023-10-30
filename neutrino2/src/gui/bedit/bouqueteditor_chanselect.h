/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_chanselect.h 2018/08/21 mohousch Exp $

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

#ifndef __bouqueteditor_chanselect__
#define __bouqueteditor_chanselect__

#include <string>

#include <driver/framebuffer.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

// zapit includes
#include <zapit/zapit.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>


class CBEChannelSelectWidget : public CMenuTarget
{
	public:
		ZapitChannelList Channels;
		ZapitChannelList *bouquetChannels;

	private:
		CFrameBuffer *frameBuffer;
		CBox cFrameBox;
		CWidget* widget;
		ClistBox *listBox;
		CMenuItem *item;

		uint32_t sec_timer_id;

		//
		unsigned int bouquet;
		CZapit::channelsMode mode;
		bool isChannelInBouquet(int index);

		bool modified;
		std::string caption;

		//
		unsigned int selected;

		void setModified(){modified = true;};

		//
		void paint();

	public:
		CBEChannelSelectWidget(const std::string& Caption, unsigned int Bouquet, CZapit::channelsMode Mode);
		~CBEChannelSelectWidget();
		int exec(CMenuTarget *parent, const std::string &actionKey);
		void hide();
		
		bool hasChanged();
};

#endif

