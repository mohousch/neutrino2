/*
	Neutrino-GUI  -   DBoxII-Project

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


#ifndef __streaminfo__
#define __streaminfo__

#include <gui/widget/widget.h>

#include <driver/gdi/framebuffer.h>

#include <gui/widget/component.h>

#include <dmx_cs.h>


class CStreamInfo : public CWidgetTarget
{
	private:

		CFrameBuffer	*frameBuffer;
		
		//
		CWidget *widget;
		CCHeaders *head;
		uint32_t sec_timer_id;
		
		//
		int x;
		int y;
		int width;
		int height;
		int hheight, iheight, sheight; 	// head/info/small font height

		int yypos;
		int  paint_mode;

		int  font_head;
		int  font_info;
		int  font_small;

		int   sigBox_x;
		int   sigBox_y;
		int   sigBox_w;
		int   sigBox_h;
		int   sigBox_pos;
		int   sig_text_y;
		int   sig_text_ber_x;
		int   sig_text_sig_x;
		int   sig_text_snr_x;
		int   sig_text_rate_x;

		struct feSignal 
		{
			unsigned long	ber, old_ber, max_ber, min_ber;
			unsigned long	sig, old_sig, max_sig, min_sig;
			unsigned long	snr, old_snr, max_snr, min_snr;
		} signal;
		
		struct bitrate 
		{
			unsigned int short_average, max_short_average, min_short_average;
		} rate;

		void doSignalStrengthLoop();

		int dvrfd, dmxfd;
		struct timeval tv, last_tv, first_tv;
		unsigned long long bit_s;
		unsigned long long abit_s;
		unsigned long long b_total;

		int update_rate();
		int ts_setup();
		int ts_close();

		void paint(int mode);
		void paint_techinfo(int x, int y);
		void paint_signal_fe_box(int x, int y, int w, int h);
		void paint_signal_fe(struct bitrate rate, struct feSignal s);
		int  y_signal_fe(unsigned long value, unsigned long max_range, int max_y);
		void SignalRenderStr (unsigned int value, int x, int y);
		CCProgressBar *sigscale;
		CCProgressBar *snrscale;
		void showSNR ();
		
		cDemux * ts_dmx;

	public:

		CStreamInfo();
		~CStreamInfo();

		void hide();
		int exec(CWidgetTarget* parent, const std::string & actionKey);

};

#endif

