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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <gui/streaminfo.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/widget/icons.h>
#include <daemonc/remotecontrol.h>

#include <video_cs.h>
#include <audio_cs.h>
#include <dmx_cs.h>

/*zapit includes*/
#include <zapit/frontend_c.h>
#include <zapit/satconfig.h>

#include <system/debug.h>


//// globals
extern cVideo * videoDecoder;
extern cAudio * audioDecoder;
extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern CRemoteControl *g_RemoteControl;	/* neutrino.cpp */
extern CZapit::SatelliteList satList;
extern CFrontend * live_fe;
extern t_channel_id live_channel_id; 			//defined in zapit.cpp

////
CStreamInfo::CStreamInfo()
{
	frameBuffer = CFrameBuffer::getInstance ();
	
	//
	widget = NULL;
	head = NULL;
	sec_timer_id = 0;

	//
	font_head = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_info = SNeutrinoSettings::FONT_TYPE_MENU;
	font_small = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;

	hheight = g_Font[font_head]->getHeight();
	iheight = g_Font[font_info]->getHeight();
	sheight = g_Font[font_small]->getHeight();
  
  	//
	width =  frameBuffer->getScreenWidth();
	height = frameBuffer->getScreenHeight();
	x = frameBuffer->getScreenX();
	y = frameBuffer->getScreenY();

	sigBox_pos = 0;
	paint_mode = 0;

	b_total = 0;
	bit_s = 0;
	abit_s = 0;

	signal.max_sig = 0;
	signal.max_snr = 0;
	signal.max_ber = 0;

	signal.min_sig = 0;
	signal.min_snr = 0;
	signal.min_ber = 0;

	rate.short_average = 0;
	rate.max_short_average = 0;
	rate.min_short_average = 0;
	
	 ts_dmx = NULL;
}

CStreamInfo::~CStreamInfo()
{
	if (head)
	{
		delete head;
		head = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CStreamInfo::exec(CMenuTarget * parent, const std::string&)
{
	dprintf(DEBUG_NORMAL, "CStreamInfo::exec\n");

	if (parent)
		parent->hide();
		
	//
	paint(paint_mode);
	CFrameBuffer::getInstance()->blit();
	
	// loop
	doSignalStrengthLoop();

	hide();
	
	return CMenuTarget::RETURN_REPAINT;
}

void CStreamInfo::doSignalStrengthLoop()
{	
#define RED_BAR 40
#define YELLOW_BAR 70
#define GREEN_BAR 100
#define BAR_WIDTH 150 
#define BAR_HEIGHT 12 

	sigscale = new CProgressBar(x + 10 - 1, yypos + (g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight()/2), BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);
	snrscale = new CProgressBar(x + 10 - 1, yypos + 2*(g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight()/2) + 5, BAR_WIDTH, BAR_HEIGHT, RED_BAR, GREEN_BAR, YELLOW_BAR);

	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	uint64_t maxb, minb, lastb, tmp_rate;
	int cnt = 0, i=0;
	uint16_t ssig, ssnr;
	uint32_t  ber;
	char tmp_str[150];
	int offset_tmp = 0;
	
	int offset = g_Font[font_info]->getRenderWidth(_("Bitrate"));
	int sw = g_Font[font_info]->getRenderWidth ("99999.999");
	int mm = g_Font[font_info]->getRenderWidth ("Max");//max min lenght
	maxb = minb = lastb = 0;
	
	//channel
	//CChannelList *channelList = CNeutrinoApp::getInstance ()->channelList;
	//int curnum = channelList->getActiveChannelNumber();
	//CZapitChannel * channel = channelList->getChannel(curnum);
	//CZapit::CCurrentServiceInfo si = CZapit::getInstance()->getCurrentServiceInfo();
	
	ts_setup();
	
	//
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);
	
	bool loop = true;
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd_MS(100);

	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);

		if(live_fe != NULL)
		{
			ssig = live_fe->getSignalStrength();
			ssnr = live_fe->getSignalNoiseRatio();
			ber = live_fe->getBitErrorRate();
		}

		signal.sig = ssig & 0xFFFF;
		signal.snr = ssnr & 0xFFFF;
		signal.ber = ber;

		int ret = update_rate();

		if (paint_mode == 0) 
		{
			char currate[150];
			if (cnt < 12)
				cnt++;
				
			int dheight = g_Font[font_info]->getHeight ();
			int dx1 = x + 10;
			
			//
			if (ret && (lastb != bit_s)) 
			{
				lastb = bit_s;
			  
				if (maxb < bit_s)
					rate.max_short_average = maxb = bit_s;
				if ((cnt > 10) && ((minb == 0) || (minb > bit_s)))
					rate.min_short_average = minb = bit_s;
				
				for(i = 0; i < 3; i++)
				{
					switch (i) 
					{
						case 0:
						tmp_rate = bit_s;
						sprintf(tmp_str, "%s", _("Bitrate"));
						offset_tmp = 0;
						break;
						case 1:
						tmp_rate = minb;
						sprintf(tmp_str, "%s", "Min");
						offset_tmp = offset+5;
						break;
						case 2:
						tmp_rate = maxb;
						sprintf(tmp_str, "%s", "Max");
						offset_tmp = offset+5+mm;
						break;
					}
					
					// Bitrate
					//g_Font[font_info]->RenderString (dx1+offset_tmp+((sw)*i), yypos+(dheight*4), offset, tmp_str, COL_MENUCONTENTDARK, 0, true);
					
					sprintf(currate, "%5llu.%03llu", tmp_rate / 1000ULL, tmp_rate % 1000ULL);
					frameBuffer->paintBoxRel (dx1+offset+5+((sw+mm)*i), yypos+(dheight*3), sw, dheight, COL_MENUCONTENTDARK_PLUS_0);
					
					// bitrate values max/min
					//g_Font[font_info]->RenderString (dx1+offset+10+((sw+mm)*i), yypos+(dheight*4), sw - 10, currate, COL_MENUCONTENTDARK);
				}
			}
			
			if(snrscale && sigscale)
				showSNR();
		}
		
		rate.short_average = abit_s;
		
		if (signal.max_ber < signal.ber) 
		{
			signal.max_ber = signal.ber;
		}
		
		if (signal.max_sig < signal.sig) 
		{
			signal.max_sig = signal.sig;
		}
		
		if (signal.max_snr < signal.snr) 
		{
			signal.max_snr = signal.snr;
		}
		
		if ((signal.min_ber == 0) || (signal.min_ber > signal.ber)) 
		{
			signal.min_ber = signal.ber;
		}
		
		if ((signal.min_sig == 0) || (signal.min_sig > signal.sig)) 
		{
			signal.min_sig = signal.sig;
		}
		
		if ((signal.min_snr == 0) || (signal.min_snr > signal.snr)) 
		{
			signal.min_snr = signal.snr;
		}

		paint_signal_fe(rate, signal);
		signal.old_sig = signal.sig;
		signal.old_snr = signal.snr;
		signal.old_ber = signal.ber;
		
		if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			widget->refresh();
		} 
		// switch paint mode
		//FIXME picture info
		else if (msg == CRCInput::RC_red || msg == CRCInput::RC_blue || msg == CRCInput::RC_green || msg == CRCInput::RC_yellow) 
		{
			if(!IS_WEBTV(live_channel_id))
			{
				hide ();
				if(sigscale)
					sigscale->reset();
				if(snrscale)
					snrscale->reset();
				
				paint_mode = ++paint_mode % 2;
				paint(paint_mode);
				
				continue;
			}
			else
				loop = false;
		}
		// -- any key --> abort
		else if (msg <= CRCInput::RC_MaxRC) 
		{
			loop = false;
		}
		// -- push other events
		else if (msg > CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout) 
		{
			CNeutrinoApp::getInstance ()->handleMsg (msg, data);
		}
		
		frameBuffer->blit();	
	}

	if(sigscale)
	{
		delete sigscale;
		sigscale = NULL;
	}

	if(snrscale)
	{
		delete snrscale;
		snrscale = NULL;
	}

	ts_close();
	
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;
}

void CStreamInfo::hide ()
{
	widget->hide();
	
	if (head)
	{
		delete head;
		head = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CStreamInfo::paint_signal_fe_box(int _x, int _y, int w, int h)
{
	 if(!IS_WEBTV(live_channel_id))
	 {
		int y1, y2;
		int xd = w/4;

		g_Font[font_small]->RenderString(_x, _y+iheight+15, width-10, _("Receipt signal"), COL_MENUCONTENTDARK, 0, true);

		sigBox_x = _x;
		sigBox_y = _y+iheight+15;
		sigBox_w = w;
		sigBox_h = h-iheight*3;

		frameBuffer->paintBoxRel(sigBox_x, sigBox_y, sigBox_w + 2, sigBox_h, COL_BLACK_PLUS_0);

		y1 = _y + h + iheight + iheight + iheight - 8;
		y2 = _y + h - sheight + 8;
		
		frameBuffer->paintBoxRel(_x+xd*0, y2 - 12, 16, 2, COL_RED_PLUS_0); //red
		g_Font[font_small]->RenderString(_x+20+xd*0, y2, 50, "BER", COL_MENUCONTENTDARK, 0, true);

		frameBuffer->paintBoxRel(_x+xd*1,y2- 12,16,2,COL_BLUE_PLUS_0); //blue
		g_Font[font_small]->RenderString(_x+20+xd*1, y2, 50, "SNR", COL_MENUCONTENTDARK, 0, true);

		frameBuffer->paintBoxRel(_x+8+xd*2,y2- 12,16,2, COL_GREEN_PLUS_0); //green
		g_Font[font_small]->RenderString(_x+28+xd*2, y2, 50, "SIG", COL_MENUCONTENTDARK, 0, true);
		
		frameBuffer->paintBoxRel(_x+xd*3,y2- 12,16,2,COL_YELLOW_PLUS_0); // near yellow
		g_Font[font_small]->RenderString(_x+20+xd*3, y2, 50, "Bitrate", COL_MENUCONTENTDARK, 0, true);
		
		sig_text_y = y1 - iheight;
		sig_text_ber_x =  _x +      xd * 0;
		sig_text_snr_x =  _x +  5 + xd * 1;
		sig_text_sig_x =  _x +  5 + xd * 2;
		sig_text_rate_x = _x + 10 + xd * 3;
			
		int maxmin_x; // x-position of min and max
		if (paint_mode == 0) 
		{
			maxmin_x = sig_text_ber_x-40;
		}
		else 
		{
			maxmin_x = _x + 40 + xd * 3 + 45;
		}
		g_Font[font_small]->RenderString(maxmin_x, y1 - sheight - sheight - sheight, 50, "max", COL_MENUCONTENTDARK, 0, true);
		g_Font[font_small]->RenderString(maxmin_x, y1 - sheight, 50, "min", COL_MENUCONTENTDARK, 0, true);


		sigBox_pos = 0;

		signal.old_sig = 1;
		signal.old_snr = 1;
		signal.old_ber = 1;
	}
}

void CStreamInfo::paint_signal_fe(struct bitrate br, struct feSignal s)
{
	if(!live_fe || (IS_WEBTV(live_channel_id)))
		return;
	
	int   x_now = sigBox_pos;
	int   yt = sig_text_y;
	int   yd;
	static int old_x=0,old_y=0;
	sigBox_pos = (++sigBox_pos) % sigBox_w;

	frameBuffer->paintVLine(sigBox_x + sigBox_pos, sigBox_y, sigBox_y + sigBox_h, COL_WHITE_PLUS_0);
	frameBuffer->paintVLine(sigBox_x + x_now, sigBox_y, sigBox_y + sigBox_h + 1, COL_BLACK_PLUS_0);

	long value = (long) (br.short_average / 1000ULL);

	SignalRenderStr(value,     sig_text_rate_x, yt - sheight);
	SignalRenderStr(br.max_short_average/ 1000ULL, sig_text_rate_x, yt - sheight - sheight);
	SignalRenderStr(br.min_short_average/ 1000ULL, sig_text_rate_x, yt);
	
	if ( g_RemoteControl->current_PIDs.PIDs.vpid > 0 )
	{
		yd = y_signal_fe (value, 15000, sigBox_h);// Video + Audio
	} 
	else 
	{
		yd = y_signal_fe (value, 512, sigBox_h); // Audio only
	}
	
	if ((old_x == 0 && old_y == 0) || sigBox_pos == 1) 
	{
		old_x = sigBox_x+x_now;
		old_y = sigBox_y+sigBox_h-yd;
	} 
	else 
	{
		frameBuffer->paintLine(old_x, old_y, sigBox_x+x_now, sigBox_y+sigBox_h-yd, COL_YELLOW_PLUS_0); //yellow
		old_x = sigBox_x+x_now;
		old_y = sigBox_y+sigBox_h-yd;
	}
	
	if (s.ber != s.old_ber) 
	{
		SignalRenderStr(s.ber,     sig_text_ber_x, yt - sheight);
		SignalRenderStr(s.max_ber, sig_text_ber_x, yt - sheight - sheight);
		SignalRenderStr(s.min_ber, sig_text_ber_x, yt);
	}
	yd = y_signal_fe(s.ber, 4000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x + x_now, sigBox_y + sigBox_h - yd, COL_RED_PLUS_0); //red


	if (s.sig != s.old_sig) 
	{
		SignalRenderStr(s.sig,     sig_text_sig_x, yt - sheight);
		SignalRenderStr(s.max_sig, sig_text_sig_x, yt - sheight - sheight);
		SignalRenderStr(s.min_sig, sig_text_sig_x, yt);
	}
	yd = y_signal_fe (s.sig, 65000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now, sigBox_y+sigBox_h-yd, COL_GREEN_PLUS_0); //green

	if (s.snr != s.old_snr) 
	{
		SignalRenderStr(s.snr,     sig_text_snr_x, yt - sheight);
		SignalRenderStr(s.max_snr, sig_text_snr_x, yt - sheight - sheight);
		SignalRenderStr(s.min_snr, sig_text_snr_x, yt);
	}
	
	yd = y_signal_fe (s.snr, 65000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now, sigBox_y+sigBox_h-yd, COL_BLUE_PLUS_0); //blue
}

// -- calc y from max_range and max_y
int CStreamInfo::y_signal_fe (unsigned long value, unsigned long max_value, int max_y)
{
	long l;

	if (!max_value)
		max_value = 1;

	l = ((long) max_y * (long) value) / (long) max_value;
	if (l > max_y)
		l = max_y;

	return (int) l;
}

void CStreamInfo::SignalRenderStr(unsigned int value, int _x, int _y)
{
	char str[30];

	frameBuffer->paintBoxRel(_x, _y - sheight + 5, 60, sheight - 1, COL_MENUCONTENTDARK_PLUS_0);
	sprintf(str,"%6u",value);
	g_Font[font_small]->RenderString(_x, _y + 5, 60, str, COL_MENUCONTENTDARK, 0, true);
}

void CStreamInfo::paint(int /*mode*/)
{
	//
	if (head)
	{
		delete head;
		head = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	widget = CNeutrinoApp::getInstance()->getWidget("streaminfo");
	
	//
	if (widget)
	{
		x = widget->getWindowsPos().iX;
		y = widget->getWindowsPos().iY;
		width = widget->getWindowsPos().iWidth;
		height = widget->getWindowsPos().iHeight;
		
		head = (CHeaders*)widget->getCCItem(CComponent::CC_HEAD);
	}
	else
	{
		widget = new CWidget(x, y, width, height);
		
		widget->name = "imageinfo";
		widget->paintMainFrame(true);
		
		// head
		head = new CHeaders(x, y, width, 40, _("Stream information"), NEUTRINO_ICON_INFO);
		head->enablePaintDate();
		head->setFormat("%d.%m.%Y %H:%M:%S");
		head->setLine(true, true);
		
		if (paint_mode == 0) 
			widget->addCCItem(head);

	}
	
	//
	if (paint_mode != 0) 
		widget->clearCCItems();
		
	widget->paint();
	
	const char * head_string;
	int ypos = y;
	int xpos = x + 10;
	hheight = head? head->getWindowsPos().iHeight : g_Font[font_head]->getHeight();

	if (paint_mode == 0) 
	{
		// -- tech Infos, PIG, small signal graph
		head_string = _("Stream information");

		CVFD::getInstance ()->setMode (CVFD::MODE_MENU_UTF8, head_string);

		ypos += hheight + 20;

		// Info Output
		paint_techinfo(xpos, ypos);

		paint_signal_fe_box(width - width/3 - 10, (y + 10 + height/3 + hheight), width/3, height/3 + hheight);
	} 
	else 
	{
		// -- paint large signal graph
		paint_signal_fe_box(x, y, width, height - 100);
	}
	
}

void CStreamInfo::paint_techinfo(int xpos, int ypos)
{
	char buf[100];
	int xres, yres, framerate;
	int aspectRatio;

	// paint labels
	int spaceoffset = 0,i = 0;
	int array[4] = {g_Font[font_info]->getRenderWidth(_("Resolution")),
		      g_Font[font_info]->getRenderWidth(_("Aspect Ratio")),
		      g_Font[font_info]->getRenderWidth(_("Framerate")),
		      g_Font[font_info]->getRenderWidth(_("Audiotype"))}; 
		      
	for(i = 0 ; i < 4; i++)
	{
		if(spaceoffset < array[i])
			spaceoffset = array[i];
	}
	spaceoffset += 10;

	//Pic Infos
	videoDecoder->getPictureInfo(xres, yres, framerate);

	// resolution
	ypos += iheight;
	sprintf ((char *) buf, "%s:", _("Resolution"));
	g_Font[font_info]->RenderString (xpos, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8
	
	sprintf ((char *) buf, "%dx%d", xres, yres);
	g_Font[font_info]->RenderString (xpos + spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	// aspectratio
	aspectRatio = videoDecoder->getAspectRatio();

	ypos += iheight;
	sprintf ((char *) buf, "%s:", _("Aspect Ratio"));
	g_Font[font_info]->RenderString (xpos, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	switch (aspectRatio) 
	{
		case 0:
			sprintf((char*) buf, "4:3");
			break;
		
		case 1:
			sprintf((char*) buf, "16:9");
			break;
		case 2:
			sprintf((char*) buf, "14:9");
			break;
	
		case 3:
			sprintf((char*) buf, "20:9");
			break;
		default:
			sprintf((char*)buf, _("unknown"));
			break;
	}

	g_Font[font_info]->RenderString (xpos + spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	// framerate
	ypos += iheight;
	sprintf ((char *) buf, "%s:", _("Framerate"));
	g_Font[font_info]->RenderString (xpos, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	switch (framerate) 
	{
		case 25:
			sprintf((char *) buf, "25fps");
		break;
		case 50:
			sprintf((char *) buf, "50fps");
		break;
		default:
			sprintf((char*)buf, _("unknown"));
	}
	g_Font[font_info]->RenderString (xpos+spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	// audiotype
#if 0
	ypos += iheight;
	int type, layer, freq, mode, bitrate;
	audioDecoder->getAudioInfo(type, layer, freq, bitrate, mode);

	const char *layernames[4] = { "res", "III", "II", "I" };
	const char *sampfreqnames[4] = { "44,1k", "48k", "32k", "res" };
	const char *modenames[4] = { "stereo", "joint_st", "dual_ch", "single_ch" };

	sprintf ((char *) buf, "%s: %s (%s/%s) %s", _("Audiotype"), modenames[stereo], sampfreqnames[sampfreq], layernames[layer], copy ? "c" : "");
  }

	const char *mpegmodes[4] = { "stereo", "joint_st", "dual_ch", "single_ch" };
	const char *ddmodes[8] = { "CH1/CH2", "C", "L/R", "L/C/R", "L/R/S", "L/C/R/S", "L/R/SL/SR", "L/C/R/SL/SR" };

	sprintf ((char *) buf, "%s:", _("Audiotype"));
	g_Font[font_info]->RenderString (xpos, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8

	if(type == 0) 
	{
		sprintf ((char *) buf, "MPEG %s (%d)", mpegmodes[mode], freq);
	} 
	else 
	{
		sprintf ((char *) buf, "DD %s (%d)", ddmodes[mode], freq);
	}
	g_Font[font_info]->RenderString (xpos+spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8
#endif

	// satellite
	ypos += iheight;
	sprintf ((char *) buf, "%s:", _("Satellite"));//swiped locale
	g_Font[font_info]->RenderString(xpos, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8

	t_satellite_position satellitePosition = CNeutrinoApp::getInstance ()->channelList->getActiveSatellitePosition ();
	sat_iterator_t sit = satellitePositions.find(satellitePosition);

	if(IS_WEBTV(live_channel_id))
	{
		g_Font[font_info]->RenderString (xpos + spaceoffset, ypos, width*2/3 - 10, "WebTV", COL_MENUCONTENTDARK, 0, true);	// UTF-8
	}
	else
	{
		if(sit != satellitePositions.end()) 
		{
			sprintf ((char *) buf, "%s", sit->second.name.c_str());
			g_Font[font_info]->RenderString (xpos + spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8
		}
	}

	// channel
	CChannelList *channelList = CNeutrinoApp::getInstance()->channelList;

	ypos += iheight;
	sprintf ((char *) buf, "%s:", _("Channel"));//swiped locale
	g_Font[font_info]->RenderString(xpos, ypos, width*2/3-10, buf , COL_MENUCONTENTDARK, 0, true); // UTF-8
	sprintf((char*) buf, "%s" ,channelList->getActiveChannelName().c_str());
	g_Font[font_info]->RenderString (xpos + spaceoffset, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true);	// UTF-8
 
	  if(!IS_WEBTV(live_channel_id))
	  {
		//tsfrequenz
		CZapit::CCurrentServiceInfo si = CZapit::getInstance()->getCurrentServiceInfo();

		ypos += iheight;
		char * f = NULL, *s = NULL, *m = NULL;
		
		if(live_fe != NULL)
		{
			if( live_fe->getInfo()->type == FE_QPSK) 
			{
				live_fe->getDelSys((fe_code_rate_t)si.fec, dvbs_get_modulation((fe_code_rate_t)si.fec), f, s, m);
				sprintf ((char *) buf,"%d.%d (%c) %d %s %s %s", si.tsfrequency / 1000, si.tsfrequency % 1000, si.polarisation ? 'V' : 'H', si.rate / 1000,f,m,s/*=="DVB-S2"?"S2":"S1"*/);
				g_Font[font_info]->RenderString(xpos, ypos, width*2/3-10, "Tp. Freq.:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
				g_Font[font_info]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8	
			}
		}
		
		// paint labels
		spaceoffset = g_Font[font_small]->getRenderWidth("VTXTpid:") + 5;

		// onid
		ypos += sheight;
		sprintf((char*) buf, "0x%04x (%i)", si.onid, si.onid);
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "ONid:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8

		// sid
		ypos += sheight;
		sprintf((char*) buf, "0x%04x (%i)", si.sid, si.sid);
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "Sid:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8

		// tsid
		ypos += sheight;
		sprintf((char*) buf, "0x%04x (%i)", si.tsid, si.tsid);
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "TSid:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8
		
		// pmtpid
		ypos += sheight;
		sprintf((char*) buf, "0x%04x (%i)", si.pmtpid, si.pmtpid);
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "PMTpid:", COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8 

		// vpid
		ypos += sheight;
		if ( g_RemoteControl->current_PIDs.PIDs.vpid > 0 )
		{
			sprintf((char*) buf, "0x%04x (%i)", g_RemoteControl->current_PIDs.PIDs.vpid, g_RemoteControl->current_PIDs.PIDs.vpid );
		} 
		else 
		{
			sprintf((char*) buf, "%s", _("not available"));
		}
		
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "Vpid:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8

		// apid
		ypos += sheight;
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "Apid(s):" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		if (g_RemoteControl->current_PIDs.APIDs.empty())
		{
			sprintf((char*) buf, "%s", _("not available"));
		} 
		else 
		{
			unsigned int i1 = 0, sw = spaceoffset;
			for (i1 = 0; (i1 < g_RemoteControl->current_PIDs.APIDs.size()) && (i1 < 10); i1++)
			{
				sprintf((char*) buf, "0x%04x (%i)", g_RemoteControl->current_PIDs.APIDs[i1].pid, g_RemoteControl->current_PIDs.APIDs[i1].pid );
				if (i1 == g_RemoteControl->current_PIDs.PIDs.selected_apid)
				{
					g_Font[font_small]->RenderString(xpos + sw, ypos, width*2/3 - 10, buf, COL_MENUHEAD, 0, true); // UTF-8
				}
				else
				{
					g_Font[font_small]->RenderString(xpos + sw, ypos, width*2/3 - 10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8
				}
				sw = g_Font[font_small]->getRenderWidth(buf) + sw + 10;
				if (((i1 + 1)%3 == 0) &&(g_RemoteControl->current_PIDs.APIDs.size() - 1 > i1))
				{ 
					// if we have lots of apids, put "intermediate" line with pids
					ypos += sheight;
					sw = spaceoffset;
				}
			}
		}

		// vtxtpid
		ypos += sheight;
		if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid == 0 )
			sprintf((char*) buf, "%s", _("not available"));
		else
			sprintf((char*) buf, "0x%04x (%i)", g_RemoteControl->current_PIDs.PIDs.vtxtpid, g_RemoteControl->current_PIDs.PIDs.vtxtpid );
		g_Font[font_small]->RenderString(xpos, ypos, width*2/3-10, "VTXTpid:" , COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[font_small]->RenderString(xpos + spaceoffset, ypos, width*2/3-10, buf, COL_MENUCONTENTDARK, 0, true); // UTF-8

		yypos = ypos;
	}
}

// some definition
#define TS_LEN			188
#define TS_BUF_SIZE		(TS_LEN * 2048)	/* fix dmx buffer size */

static unsigned long timeval_to_ms (const struct timeval *tv)
{
	return (tv->tv_sec * 1000) + ((tv->tv_usec + 500) / 1000);
}

long delta_time_ms (struct timeval *tv, struct timeval *last_tv)
{
	return timeval_to_ms (tv) - timeval_to_ms (last_tv);
}

uint64_t b_total;

int CStreamInfo::ts_setup()
{
	if(!IS_WEBTV(live_channel_id))
	{
		unsigned short vpid, apid = 0;

		// vpid
		vpid = g_RemoteControl->current_PIDs.PIDs.vpid;
		
		// apids
		if(g_RemoteControl->current_PIDs.APIDs.size() > 0)
			apid = g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid;

		if(vpid == 0 && apid == 0)
			return -1;

		ts_dmx = new cDemux();
		
		// open demux
#if defined (PLATFORM_COOLSTREAM)
		ts_dmx->Open(DMX_TP_CHANNEL);
#else	
		ts_dmx->Open( DMX_TP_CHANNEL, 3*3008*62, live_fe );
#endif	
		
		if(vpid > 0) 
		{
			ts_dmx->pesFilter(vpid);
			if(apid > 0)
				ts_dmx->addPid(apid);
		} 
		else
			ts_dmx->pesFilter(apid);
		
		// start filter
		ts_dmx->Start();

		gettimeofday (&first_tv, NULL);
		last_tv.tv_sec = first_tv.tv_sec;
		last_tv.tv_usec = first_tv.tv_usec;
		b_total = 0;
	}
	
	return 0;
}

int CStreamInfo::update_rate()
{
	int ret = 0;
	
	if(!IS_WEBTV(live_channel_id))
	{
		unsigned char buf[TS_BUF_SIZE];
		long b;
		
		int b_len, b_start;
		int timeout = 100;

		if(!ts_dmx)
			return 0;

		b_len = 0;
		b_start = 0;

		b_len = ts_dmx->Read(buf, sizeof (buf), timeout);

		b = b_len;
		if (b <= 0)
			return 0;

		gettimeofday (&tv, NULL);

		b_total += b;

		long d_tim_ms;

		d_tim_ms = delta_time_ms (&tv, &last_tv);
		if (d_tim_ms <= 0)
			d_tim_ms = 1;			//  ignore usecs 

		bit_s = (((uint64_t) b * 8000ULL) + ((uint64_t) d_tim_ms / 2ULL))
			/ (uint64_t) d_tim_ms;

		d_tim_ms = delta_time_ms (&tv, &first_tv);
		if (d_tim_ms <= 0)
			d_tim_ms = 1;			//  ignore usecs 

		abit_s = ((b_total * 8000ULL) + ((uint64_t) d_tim_ms / 2ULL))
			/ (uint64_t) d_tim_ms;

		last_tv.tv_sec = tv.tv_sec;
		last_tv.tv_usec = tv.tv_usec;
		ret = 1;
	}
	
	return ret;
}

int CStreamInfo::ts_close()
{
	if(!IS_WEBTV(live_channel_id))
	{
		if(ts_dmx)
			delete ts_dmx;
		
		ts_dmx = NULL;
	}
	
	return 0;
}

void CStreamInfo::showSNR()
{
	 if(!IS_WEBTV(live_channel_id))
	 {
		char percent[10];
		int barwidth = 150;
		int sig, snr;
		int posx, posy;
		int sw;

		snr = (signal.snr & 0xFFFF) * 100 / 65535;
		sig = (signal.sig & 0xFFFF) * 100 / 65535;

		int mheight = g_Font[font_info]->getHeight();
		if(sigscale->getPercent() != sig) 
		{
		  	posy = yypos + (mheight/2);
			posx = x + 10;
			sprintf(percent, "%d%% SIG", sig);
			sw = g_Font[font_info]->getRenderWidth (percent);

			sigscale->paint(sig);

			posx = posx + barwidth + 3;
			
			// FIXME:
			frameBuffer->paintBoxRel(posx, posy -1, sw, mheight-8, COL_MENUCONTENTDARK_PLUS_0);
			g_Font[font_info]->RenderString (posx+2, posy + mheight-5, sw, percent, COL_MENUCONTENTDARK);
		}

		if(snrscale->getPercent() != snr) 
		{
		  	posy = yypos + mheight + 4;
			posx = x + 10;
			sprintf(percent, "%d%% SNR", snr);
			sw = g_Font[font_info]->getRenderWidth (percent);

			snrscale->paint(snr);

			// FIXME:
			posx = posx + barwidth + 3;
			frameBuffer->paintBoxRel(posx, posy - 1, sw, mheight-8, COL_MENUCONTENTDARK_PLUS_0, 0, true);
			g_Font[font_info]->RenderString (posx + 2, posy + mheight-5, sw, percent, COL_MENUCONTENTDARK, 0, true);
		}
	}
}

