/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: infoviewer.h 2013/09/03 10:45:30 mohousch Exp $

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


#ifndef __infoview__
#define __infoview__

#include <string>

#include <sectionsd/sectionsd.h>

#include <driver/rcinput.h>

#include <driver/gfx/framebuffer.h>
#include <driver/gfx/fontrenderer.h>

#include <system/settings.h>

#include <gui/widget/widget.h>
#include <gui/widget/widget_helpers.h>


class CInfoViewer
{
	public:
		enum
		{
			NO_AC3,
			AC3_AVAILABLE,
			AC3_ACTIVE
		};
		
		bool chanready;
		bool is_visible;		
		
	private:
		void Init(void);
		void initFrames(void);

		CFrameBuffer *frameBuffer;
		
		//
		bool gotTime;
		
		//
		CCTime* timer;
		CCIcon* recIcon;
		CCLabel* currentLabel;
		CCLabel* currentStartTime;
		CCLabel* currentPlayTime;
		CCLabel* nextLabel;
		CCLabel* nextStartTime;
		CCLabel* nextPlayTime;
		
		//
		bool recordModeActive;
		bool CA_Status;
		bool showButtonBar;
		
		//
		int BoxEndX;
		int BoxEndY;
		int BoxStartX;
		int BoxStartY;
		int BoxHeight;
		int BoxWidth;

		// channel number
		int ChanNumberX;
		int ChanNumberY;
		
		// channel name
		int ChanNameX;
		int ChanNameY;
		int ChanNameWidth;
		int ChanNameHeight;
		
		// channel info
		int ChanInfoX;
		int ChanInfoY;
		int ChanInfoHeight;
		
		// sat info
		int satNameWidth;
		int satNameHeight;
		int freqStartX;
		int freqWidth;
		
		// buttonbar
		int buttonBarStartX;
		int buttonBarStartY;
		int buttonBarHeight;
		
		// icons
		int icon_w_subt, icon_h_subt;
		int icon_w_vtxt, icon_h_vtxt;
		int icon_w_aspect, icon_h_aspect;
		int icon_w_dd, icon_h_dd;
		int icon_w_sd, icon_h_sd;
		int icon_w_hd, icon_h_hd;
		int icon_w_reso, icon_h_reso;
		int icon_w_ca, icon_h_ca;
		int icon_w_rt, icon_h_rt;
		int icon_w_rec, icon_h_rec;

		// colored icons
		int icon_red_w, icon_red_h;
		int icon_green_w, icon_green_h;
		int icon_yellow_w, icon_yellow_h;
		int icon_blue_w, icon_blue_h;
		
		int icn_red_posx;
		int icon_green_posx;
		int icon_yellow_posx;
		int icon_blue_posx;
		
		// tuner
		int TunerNumWidth;
		int TunerNumHeight;
		
		// channel logo
		int pic_w;
		int pic_h;
		int logo_w; 
		int logo_h;
		int logo_bpp;
		int pic_x;
		int pic_y;
		
		// date
		int dateWidth;
		int dateHeight;
		
		// dimensions of radiotext window		
		int             rt_dx;
		int             rt_dy;
		int             rt_x;
		int             rt_y;
		int             rt_h;
		int             rt_w;	

		int	asize;
		bool show_dot;
		bool new_chan;

		CSectionsd::CurrentNextInfo info_CurrentNext;
		t_channel_id   channel_id;

		int timescale_posx;
		int timescale_posy;
		unsigned int runningPercent;

		char aspectRatio;
		uint32_t sec_timer_id;
		bool virtual_zap_mode;
		
		CChannelEventList evtlist;
		CChannelEventList::iterator eli;

		CCProgressBar *snrscale, *sigscale, *timescale;
		
		//
		void showEPGData(bool calledFromEvent = false);
		void paintTime(int posx, int posy, unsigned int timeFont);
		void showButton_Audio();
		void showButton_SubServices();
		void showIcon_16_9();		
		void showIcon_RadioText(bool rt_available) const;		
		void showIcon_CA_Status() const;
		void showIcon_VTXT()      const;
		void showRecordIcon(bool show);
		void paintRecordIcon(int posx, int posy);
		void showIcon_SubT() const;
		void showIcon_Resolution() const;
		void showFailure();
		void showMotorMoving(int duration);
		void showLcdPercentOver();
		void showSNR();
		void Set_CA_Status(int Status);
		void getCurrentNextEPG(t_channel_id ChannelID, bool newChan = false, int EPGPos = 0);

 	public:
		CInfoViewer();
		~CInfoViewer();

		////
		void showTitle(const int _ChanNum, const std::string& _ChannelName, const t_satellite_position _satellitePosition, const t_channel_id _new_channel_id = 0, const bool _calledFromNumZap = false, int _epgpos = 0);
		void killTitle();
		////
		int handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);
		void clearVirtualZapMode() {virtual_zap_mode = false;}	// used in channellist.cpp
		////
		void showRadiotext(); 	// needed in radiotext
		void killRadiotext(); 	// needed in radiotext
		void showSubchan(); 	// needed in CNVODChangeExec
		////
		void getEPG(const t_channel_id for_channel_id, CSectionsd::CurrentNextInfo &info); // needed by CSleepTimerWidget
		CSectionsd::CurrentNextInfo getCurrentNextInfo() { return info_CurrentNext; }
};

#endif

