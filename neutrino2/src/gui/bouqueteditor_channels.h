/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: bouqueteditor_channels.h 2018/08/22 mohousch Exp $

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


#ifndef __bouqueteditor_channels__
#define __bouqueteditor_channels__

#include <driver/gdi/framebuffer.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

// zapit
#include <zapit/zapit.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>

#include <string>
#include <gui/widget/listbox.h>


class CBEChannelWidget : public CWidgetTarget
{
	public:
		ZapitChannelList *Channels;

	private:
		CFrameBuffer *frameBuffer;
		CBox cFrameBox;
		CWidget* widget;
		ClistBox *listBox;
		CMenuItem *item;
	
		enum state_
		{
			beDefault,
			beMoving
		} state;

		unsigned int		selected;
		unsigned int		origPosition;
		unsigned int		newPosition;
		bool			channelsChanged;
		std::string		caption;
		CZapit::channelsMode mode;
		unsigned int bouquet;

		uint32_t sec_timer_id;

		void paint();
		void hide();

		void deleteChannel();
		void addChannel();
		void beginMoveChannel();
		void finishMoveChannel();
		void cancelMoveChannel();
		void internalMoveChannel( unsigned int fromPosition, unsigned int toPosition);

	public:
		CBEChannelWidget( const std::string & Caption, unsigned int Bouquet);
		~CBEChannelWidget();
		int exec(CWidgetTarget* parent, const std::string & actionKey);

		bool hasChanged();
};

#endif
