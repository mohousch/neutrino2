/*
        Neutrino-GUI  -   DBoxII-Project
        
        $Id: epgplus.h 2013/10/12 mohousch Exp $

        Copyright (C) 2001 Steffen Hehn 'McClean'
        Copyright (C) 2004 Martin Griep 'vivamiga'

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


#ifndef __EPGPLUS_HPP__
#define __EPGPLUS_HPP__

#include <driver/gdi/framebuffer.h>
#include <driver/gdi/fontrenderer.h>
#include <driver/gdi/color.h>

#include <driver/rcinput.h>
#include <system/settings.h>

#include <gui/channellist.h>
#include <gui/infoviewer.h>
#include <gui/filebrowser.h>

#include <gui/widget/widget.h>
#include <gui/widget/component.h>

#include <string>


class EpgPlus
{
	//// types, inner classes
	public:
		enum SizeSettingID
		{                                                 
			EPGPlus_channelentry_width = 0,                    
			EPGPlus_channelentry_separationlineheight,     
			EPGPlus_slider_width,                          
			EPGPlus_horgap1_height,                        
			EPGPlus_horgap2_height,                        
			EPGPlus_vergap1_width,                         
			EPGPlus_vergap2_width,                         
			NumberOfSizeSettings
		};

		struct SizeSetting
		{
			SizeSettingID     settingID;
			int               size;           
		};

		class Footer;

		//// Header
		class Header
		{
			public:
				//// construction / destruction
				Header( CFrameBuffer* _frameBuffer , int _x , int _y , int _width);
				~Header();

				//// methods
				static void init();
				void paint();
				void refresh();
				static int getUsedHeight();

				//// attributes
				CFrameBuffer* frameBuffer;

				int x;
				int y;
				int width;
				
				static CFont *font;
				static CCHeaders *head;
		};

		//// timeline
		class TimeLine
		{
			public:
				//// construction / destruction
				TimeLine( CFrameBuffer* _frameBuffer , int _x , int _y , int _width , int _startX , int _durationX);
				~TimeLine();

				//// methods
				static void init();
				void paint ( time_t startTime, int _duration);
				void paintMark ( time_t startTime, int _duration , int _x , int _width);
				void paintGrid();
				void clearMark();
				static int getUsedHeight();

				//// attributes
				CFrameBuffer* frameBuffer;

				int currentDuration;

				int x;
				int y;
				int width;
			      
				static CFont *fontTime;
				static CFont *fontDate;

				int startX;
				int durationX;
		};

		//// channel event entry
		class ChannelEventEntry
		{
			public:
				//// construction / destruction
				ChannelEventEntry( const CChannelEvent* _channelEvent, CFrameBuffer* _frameBuffer, TimeLine* _timeLine, Footer* _footer, int _x, int _y, int _width);
				~ChannelEventEntry();

				//// methods
				static void init();

				bool isSelected( time_t selectedTime) const;

				void paint( bool isSelected, bool toggleColor);

				static int getUsedHeight();

				//// attributes
				CChannelEvent channelEvent;

				CFrameBuffer* frameBuffer;
				TimeLine* timeLine;
				Footer* footer;

				int x;
				int y;
				int width;
				static int separationLineHeight;

				static CFont *font;
		};

		typedef std::vector<ChannelEventEntry*> TCChannelEventEntries;

		//// channel entry
		class ChannelEntry
		{
			public:
				//// construction / destruction
				ChannelEntry
				  ( const CZapitChannel* channel, int index, CFrameBuffer* frameBuffer, Footer* footer, int x, int y, int width);

				~ChannelEntry();

				//// methods
				static void init();

				void paint( bool   isSelected, time_t selectedTime);

				static int getUsedHeight();

				//// attributes
				const CZapitChannel * channel;
				std::string displayName;
				int index;

				CFrameBuffer* frameBuffer;
				Footer* footer;

				int x;
				int y;
				int width;
				static int separationLineHeight;

				static CFont *font;

				TCChannelEventEntries      channelEventEntries;
		};

		typedef std::vector<ChannelEntry*> TChannelEntries;

		//// footer
		class Footer
		{
			public:
				//// construction / destruction
				Footer( CFrameBuffer* _frameBuffer, int _x, int _y, int _width);

				~Footer();

				//// methods
				static void init();

				void setBouquetChannelName(const std::string& newChannelName);
				void paintEventDetails( const std::string& description, const std::string& shortDescription);
				void paintButtons( button_label* _buttonLabels, int numberOfButtons);
				static int getUsedHeight();
	  
				//// attributes
				CFrameBuffer* frameBuffer;

				int x;
				int y;
				int width;

				CCButtons buttons;

				static CFont*  fontBouquetChannelName;
				static CFont*  fontEventDescription;     
				static CFont*  fontEventShortDescription;
				static CFont*  fontButtons;

				std::string currentChannelName;
		};

		////
		class MenuTargetAddReminder : public CWidgetTarget
		{
			public:
				MenuTargetAddReminder( EpgPlus* _epgPlus);

				int exec(CWidgetTarget* parent, const std::string& actionKey);

			private:
				EpgPlus * epgPlus;

		};

		////
		class MenuTargetAddRecordTimer : public CWidgetTarget
		{
			public:
				MenuTargetAddRecordTimer( EpgPlus* _epgPlus);

				int exec(CWidgetTarget* parent , const std::string& actionKey);

			private:
				EpgPlus * epgPlus;

		};

		////
		class MenuTargetRefreshEpg : public CWidgetTarget
		{
			public:
				MenuTargetRefreshEpg( EpgPlus* _epgPlus);

				int exec(CWidgetTarget* parent, const std::string& actionKey);

			private:
				EpgPlus * epgPlus;

		};

		//
		typedef time_t DurationSetting;

		friend class EpgPlus::ChannelEntry;
		friend class EpgPlus::ChannelEventEntry;

	////
	public:
		//// construction / destruction
		EpgPlus();
		~EpgPlus();

		//// methods
		void init();
		void free();

		int exec(CChannelList* channelList, int selectedChannelIndex); 

	private:
		//// methods
		static std::string getTimeString ( const time_t& time , const std::string& format);
		TCChannelEventEntries::const_iterator getSelectedEvent() const;
		void createChannelEntries( int selectedChannelEntryIndex);
		void paint();
		void paintChannelEntry( int position);
		void hide();

		//// properties
		CFrameBuffer*   frameBuffer;

		TChannelEntries displayedChannelEntries;

		Header*         header;
		TimeLine*       timeLine;

		CChannelList*   channelList;

		Footer*         footer;

		ChannelEntry*   selectedChannelEntry;
		time_t          selectedTime;

		int             channelListStartIndex;
		int             maxNumberOfDisplayableEntries; // maximal number of displayable entrys

		time_t          startTime;
		time_t          firstStartTime;
		static time_t   duration;

		int             entryHeight;

		int             headerX;
		int             headerY;
		int             headerWidth;

		int             usableScreenWidth;
		int             usableScreenHeight;
		int             usableScreenX;
		int             usableScreenY;

		int             timeLineX;
		int             timeLineY;
		int             timeLineWidth;

		int             channelsTableX;
		int             channelsTableY;
		static int      channelsTableWidth;
		int             channelsTableHeight;

		int             eventsTableX;
		int             eventsTableY;
		int             eventsTableWidth;
		int             eventsTableHeight;

		int             sliderX;
		int             sliderY;
		static int      sliderWidth;
		int             sliderHeight;
		static int      sliderBackColor;
		static int      sliderKnobColor;

		int             footerX;
		int             footerY;
		int             footerWidth;

		int             horGap1X;
		int             horGap1Y;
		int             horGap1Width;
		int             horGap2X;
		int             horGap2Y;
		int             horGap2Width;
		int             verGap1X;
		int             verGap1Y;
		int             verGap1Height;
		int             verGap2X;
		int             verGap2Y;
		int             verGap2Height;

		static int      horGap1Height;
		static int      horGap2Height;
		static int      verGap1Width;
		static int      verGap2Width;

		static int      horGap1Color;
		static int      horGap2Color;
		static int      verGap1Color;
		static int      verGap2Color;

		bool            refreshAll;
		bool            refreshFooterButtons;

		uint32_t 	sec_timer_id;
		
		////
		MenuTargetRefreshEpg *refreshEpg;
		MenuTargetAddRecordTimer *addRecordTimer;
		MenuTargetAddReminder *addReminder;
};

class CEPGplusHandler : public CWidgetTarget
{
	public:
		CEPGplusHandler(){};
		virtual ~CEPGplusHandler(){};
		
		int exec(CWidgetTarget* parent,  const std::string &actionKey);
};

#endif

