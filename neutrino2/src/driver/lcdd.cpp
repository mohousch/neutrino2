/*
	$Id: lcdd.cpp 2013/10/12 mohousch Exp $

	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2008 Novell, Inc. Author: Stefan Seyfried
		  (C) 2009 Stefan Seyfried

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

#include <driver/lcdd.h>

#include <global.h>
#include <neutrino2.h>
#include <system/settings.h>
#include <system/debug.h>

#include <liblcddisplay/lcddisplay.h>
#include <gui/widget/icons.h>

#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <daemonc/remotecontrol.h>


extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CLCD::CLCD()
	: configfile('\t')
{
#ifdef LCD_UPDATE
	m_fileList = NULL;
	m_fileListPos = 0;
	m_fileListHeader = "";
	m_infoBoxText = "";
	m_infoBoxAutoNewline = 0;
	m_progressShowEscape = 0;
	m_progressHeaderGlobal = "";
	m_progressHeaderLocal = "";
	m_progressGlobal = 0;
	m_progressLocal = 0;
#endif // LCD_UPDATE
	muted = false;
	percentOver = 0;
	volume = 0;
	timeout_cnt = 0;
	icon_dolby = false;	
	has_lcd = true;
	is4digits = false;
	clearClock = 0;
}

CLCD::~CLCD()
{
#if 0
	for (int i = 0; i < LCD_NUMBER_OF_BACKGROUNDS; i++)
	{
		delete [] (background[i]);
	}
#endif
}

CLCD* CLCD::getInstance()
{
	static CLCD* lcdd = NULL;
	if(lcdd == NULL)
	{
		lcdd = new CLCD();
	}
	
	return lcdd;
}

void CLCD::count_down() 
{
	if (timeout_cnt > 0) 
	{
		timeout_cnt--;
		if (timeout_cnt == 0) 
		{
			setlcdparameter();
		}
	} 
}

void CLCD::wake_up() 
{
	if (atoi(g_settings.lcd_setting_dim_time) > 0) 
	{
		timeout_cnt = atoi(g_settings.lcd_setting_dim_time);
		setlcdparameter();
	}
}

void* CLCD::TimeThread(void *)
{
	while(1)
	{
		sleep(1);
		struct stat buf;
		if (stat("/tmp/lcd.locked", &buf) == -1) 
		{
			CLCD::getInstance()->showTime();
			CLCD::getInstance()->count_down();
		} 
		else
			CLCD::getInstance()->wake_up();
	}
	
	return NULL;
}

void CLCD::init(const char * fontfile, const char * fontname, const char * fontfile2, const char * fontname2, const char * fontfile3, const char * fontname3)
{
	if (!lcdInit(fontfile, fontname, fontfile2, fontname2, fontfile3, fontname3 ))
	{
		dprintf(DEBUG_NORMAL, "CLCD::init: LCD-Init failed!\n");
		has_lcd = false;
	}

	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("CLCD::init: pthread_create(TimeThread)");
		return ;
	}
}

enum elements {
	ELEMENT_BANNER = 0,
	ELEMENT_MOVIESTRIPE = 1,
	ELEMENT_SPEAKER  = 2,
	ELEMENT_SCART  = 3,
	ELEMENT_POWER  = 4,
	ELEMENT_MUTE = 5,
	ELEMENT_DOLBY = 6,
};

const char * const element_name[LCD_NUMBER_OF_ELEMENTS] = {
	"lcdbannr",
	"lcdprog",
	"lcdvol",
	"lcdscart",
	"lcdpower",
	"lcdmute",
	"lcddolby"
};

#define NUMBER_OF_PATHS 2
const char * const background_path[NUMBER_OF_PATHS] = {
	LCDDIR_VAR ,
	DATADIR "/lcdd/icons/"
};

bool CLCD::lcdInit(const char * fontfile, const char * fontname, const char * fontfile2, const char * fontname2, const char * fontfile3, const char * fontname3)
{
	fontRenderer = new LcdFontRenderClass(&display);
	const char * style_name = fontRenderer->AddFont(fontfile);
	const char * style_name2;
	const char * style_name3;

	if (fontfile2 != NULL)
		style_name2 = fontRenderer->AddFont(fontfile2);
	else
	{
		style_name2 = style_name;
		fontname2   = fontname;
	}

	if (fontfile3 != NULL)
		style_name3 = fontRenderer->AddFont(fontfile3);
	else
	{
		style_name3 = style_name;
		fontname3   = fontname;
	}
	fontRenderer->InitFontCache();

	fonts.menu        = fontRenderer->getFont(fontname,  style_name , 12);
	fonts.time        = fontRenderer->getFont(fontname2, style_name2, 14);
	fonts.channelname = fontRenderer->getFont(fontname3, style_name3, 15);
	fonts.menutitle   = fonts.channelname;

	setAutoDimm(g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM]);

	if (!display.isAvailable())
	{
		dprintf(DEBUG_NORMAL, "CLCD::lcdInit: exit...(no lcd-support)\n");
		return false;
	}
 
	for (int i = 0; i < LCD_NUMBER_OF_ELEMENTS; i++)
	{
		bool bgfound = false;
		//(element[i]) = new unsigned char[display.element_buffer_size];
		//memset((element[i]), 0, display.element_buffer_size);
		element[i].header.width = 0;
		element[i].header.height = 0;
		element[i].buffer_size = 0;
		element[i].buffer = NULL;

		for (int j = 0; j < NUMBER_OF_PATHS; j++)
		{
			std::string file = background_path[j];
			file += element_name[i];
			file += ".png";
			bgfound = display.load_png_element(file.c_str(), &(element[i]));
			if (bgfound)
				break;
		}
		//if (!bgfound)
		//	printf("[neutrino/lcd] no valid %s element.\n", element_name[i]);
	}	

	setMode(MODE_TVRADIO);

	return true;
}

void CLCD::displayUpdate()
{
	struct stat buf;
	if (stat("/tmp/lcd.locked", &buf) == -1)
	{
		display.update();
		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_DUMP_PNG])
			display.dump_png("/tmp/lcdd.png");
	}
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse, const int bias)
{
	if(!has_lcd) 
		return;
	
	if (!display.isAvailable())
		return;
	int fd;
	if (power == 0)
		dimm = 0;
	
	// dimm
	display.setLCDBrightness(dimm);
	
	// contrast
	display.setLCDContrast(contrast);
	
	// power ???
	
	//reverse
	if (inverse)
		display.setInverted(CLCDDisplay::PIXEL_ON);
	else		
		display.setInverted(CLCDDisplay::PIXEL_OFF);
}

void CLCD::setlcdparameter(void)
{
	if(!has_lcd) 
		return;
	
	last_toggle_state_power = g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
	int dim_time = atoi(g_settings.lcd_setting_dim_time);
	int dim_brightness = g_settings.lcd_setting_dim_brightness;
	bool timeouted = (dim_time > 0) && (timeout_cnt == 0);
	int brightness, power = 0;

	if (timeouted)
		brightness = dim_brightness;
	else
		brightness = g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];

	if (last_toggle_state_power && (!timeouted || dim_brightness > 0))
		power = 1;

	if (mode == MODE_STANDBY)
		brightness = g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS];

	setlcdparameter(brightness,
			g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST],
			power,
			g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE],
			0 /*g_settings.lcd_setting[SNeutrinoSettings::LCD_BIAS]*/);
}

static std::string removeLeadingSpaces(const std::string & text)
{
	int pos = text.find_first_not_of(" ");

	if (pos != -1)
		return text.substr(pos);

	return text;
}

static std::string splitString(const std::string & text, const int maxwidth, LcdFont *font, bool dumb, bool utf8)
{
	int pos;
	std::string tmp = removeLeadingSpaces(text);

	if (font->getRenderWidth(tmp.c_str(), utf8) > maxwidth)
	{
		do
		{
			if (dumb)
				tmp = tmp.substr(0, tmp.length()-1);
			else
			{
				pos = tmp.find_last_of("[ .-]+"); // TODO characters might be UTF-encoded!
				if (pos != -1)
					tmp = tmp.substr(0, pos);
				else // does not fit -> fall back to dumb split
					tmp = tmp.substr(0, tmp.length()-1);
			}
		} while (font->getRenderWidth(tmp.c_str(), utf8) > maxwidth);
	}

	return tmp;
}

/* display "big" and "small" text.
   TODO: right now, "big" is hardcoded as utf-8, small is not (for EPG)
 */
void CLCD::showTextScreen(const std::string & big, const std::string & small, const int showmode, const bool perform_wakeup, const bool centered)
{
	if(!has_lcd) 
		return;
	
	unsigned int lcd_width  = display.xres;
	unsigned int lcd_height = display.yres;

	/* the "showmode" variable is a bit map:
		0x01	show "big" string
		0x02	show "small" string
		0x04	show separator line if big and small are present / shown
		0x08	show only one line of "big" string
	 */

	/* draw_fill_rect is braindead: it actually fills _inside_ the described rectangle,
	   so that you have to give it one pixel additionally in every direction ;-(
	   this is where the "-1 to 120" intead of "0 to 119" comes from */
	display.draw_fill_rect(-1, 8+2, lcd_width, lcd_height-8-2-2, CLCDDisplay::PIXEL_OFF);

	bool big_utf8 = false;
	bool small_utf8 = false;
	std::string cname[2];
	std::string event[4];
	int namelines = 0, eventlines = 0, maxnamelines = 2;
	if (showmode & 8)
		maxnamelines = 1;

	if ((showmode & 1) && !big.empty())
	{
		bool dumb = false;
		big_utf8 = isUTF8(big);
		while (true)
		{
			namelines = 0;
			std::string title = big;
			do { // first try "intelligent" splitting
				cname[namelines] = splitString(title, lcd_width, fonts.channelname, dumb, big_utf8);
				title = removeLeadingSpaces(title.substr(cname[namelines].length()));
				namelines++;
			} while (title.length() > 0 && namelines < maxnamelines);
			if (title.length() == 0)
				break;
			dumb = !dumb;	// retry with dumb splitting;
			if (!dumb)	// second retry -> get out;
				break;
		}
	}

	// one nameline => 2 eventlines, 2 namelines => 1 eventline
	int maxeventlines = 4 - (namelines * 3 + 1) / 2;
	
	if (lcd_height < 64)
		maxeventlines = 2 - (namelines * 1 + 1) / 2;

	if ((showmode & 2) && !small.empty())
	{
		bool dumb = false;
		small_utf8 = isUTF8(small);
		while (true)
		{
			eventlines = 0;
			std::string title = small;
			do { // first try "intelligent" splitting
				event[eventlines] = splitString(title, lcd_width, fonts.menu, dumb, small_utf8);
				title = removeLeadingSpaces(title.substr(event[eventlines].length()));
				/* DrDish TV appends a 0x0a to the EPG title. We could strip it in sectionsd...
				   ...instead, strip all control characters at the end of the text for now */
				if (event[eventlines].length() > 0 && event[eventlines].at(event[eventlines].length() - 1) < ' ')
					event[eventlines].erase(event[eventlines].length() - 1);
				eventlines++;
			} while (title.length() >0 && eventlines < maxeventlines);
			if (title.length() == 0)
				break;
			dumb = !dumb;	// retry with dumb splitting;
			if (!dumb)	// second retry -> get out;
				break;
		}
	}

	/* this values were determined empirically */
	//int y = 8 + (41 - namelines*14 - eventlines*10)/2;
	//
	int t_h = 41;
	if (lcd_height < 64)
		t_h = t_h*lcd_height/64;
	int y = 8 + (t_h - namelines*14 - eventlines*10)/2;
	//
	int x = 1;
	for (int i = 0; i < namelines; i++) {
		y += 14;
		if (centered)
		{
			int w = fonts.channelname->getRenderWidth(cname[i].c_str(), big_utf8);
			x = (lcd_width - w) / 2;
		}
		fonts.channelname->RenderString(x, y, lcd_width + 10, cname[i].c_str(), CLCDDisplay::PIXEL_ON, 0, big_utf8);
	}
	y++;
	
	if (eventlines > 0 && namelines > 0 && showmode & 4)
	{
		y++;
		display.draw_line(0, y, lcd_width - 1, y, CLCDDisplay::PIXEL_ON);
	}
	
	if (eventlines > 0)
	{
		for (int i = 0; i < eventlines; i++) {
			y += 10;
			if (centered)
			{
				int w = fonts.menu->getRenderWidth(event[i].c_str(), small_utf8);
				x = (lcd_width - w) / 2;
			}
			fonts.menu->RenderString(x, y, lcd_width + 10, event[i].c_str(), CLCDDisplay::PIXEL_ON, 0, small_utf8);
		}
	}

	if (perform_wakeup)
		wake_up();

	displayUpdate();
}

void CLCD::showServicename(const std::string name, const bool perform_wakeup, int pos)
{
	/*
	   1 = show servicename
	   2 = show epg title
	   4 = draw separator line between name and EPG
	 */
	int showmode = g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE];

	//printf("CLCD::showServicename '%s' epg: '%s'\n", name.c_str(), epg_title.c_str());

	if (!name.empty())
		servicename = name;

	if (mode != MODE_TVRADIO)
		return;

	showTextScreen(servicename, epg_title, showmode, perform_wakeup, true);
	
	return;
}

void CLCD::setEPGTitle(const std::string title)
{
	if (title == epg_title)
	{
		//fprintf(stderr,"CLCD::setEPGTitle: not changed\n");
		return;
	}
	epg_title = title;
	showServicename("", false);
}

void CLCD::setMovieInfo(const AUDIOMODES playmode, const std::string big, const std::string small, const bool centered)
{
	int showmode = g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE];
	
	showmode |= 3; // take only the separator line from the config

	movie_playmode = playmode;
	movie_big = big;
	movie_small = small;
	movie_centered = centered;

	if (mode != MODE_MOVIE)
		return;

	showAudioPlayMode(movie_playmode);
	showTextScreen(movie_big, movie_small, showmode, true, movie_centered);
}

void CLCD::setMovieAudio(const bool is_ac3)
{
	movie_is_ac3 = is_ac3;

	if (mode != MODE_MOVIE)
		return;

	showPercentOver(percentOver, true, MODE_MOVIE);
}

void CLCD::showTime()
{
	if(!has_lcd) 
		return;
	
	if (showclock)
	{
		char timestr[21];
		struct timeval tm;
		struct tm * t;

		gettimeofday(&tm, NULL);
		t = localtime(&tm.tv_sec);
		
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;

		if (mode == MODE_STANDBY)
		{
			display.clear_screen(); // clear lcd
			//ShowNewClock(&display, t->tm_hour, t->tm_min, t->tm_sec, t->tm_wday, t->tm_mday, t->tm_mon, CNeutrinoApp::getInstance()->recordingstatus);
		}
		else
		{
			if (CNeutrinoApp::getInstance ()->recordingstatus && clearClock == 1)
			{
				strcpy(timestr,"  :  ");
				clearClock = 0;
			}
			else
			{
				strftime((char*) &timestr, 20, "%H:%M", t);
				clearClock = 1;
			}

			display.draw_fill_rect (lcd_width-50-1, lcd_height-12, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);

			fonts.time->RenderString(lcd_width - 4 - fonts.time->getRenderWidth(timestr), lcd_height-1, 50, timestr, CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}

void CLCD::showRCLock(int duration)
{
	if(!has_lcd) 
		return;
	
	std::string icon = DATADIR "/lcdd/icons/rclock.raw";
	raw_display_t curr_screen = new unsigned char[display.raw_buffer_size];

	// Saving the whole screen is not really nice since the clock is updated
	// every second. Restoring the screen can cause a short travel to the past ;)
	display.dump_screen(&curr_screen);
	display.draw_fill_rect (-1, 10, display.xres, display.yres-12-2, CLCDDisplay::PIXEL_OFF);
	display.paintIcon(icon,44,25,false);
	wake_up();
	displayUpdate();
	sleep(duration);
	display.load_screen(&curr_screen);
	wake_up();
	displayUpdate();
	delete [] curr_screen;
}

void CLCD::showVolume(const char vol, const bool perform_update)
{
	if(!has_lcd) 
		return;
	
	volume = vol;
	if (
	    ((mode == MODE_TVRADIO) && (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])) ||
	    ((mode == MODE_MOVIE) && (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])) ||
	    (mode == MODE_SCART) ||
	    (mode == MODE_AUDIO)
	    )
	{
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;
		unsigned int height =  6;
		unsigned int left   = 12+2;
		unsigned int top    = lcd_height - height - 1 - 2;
		unsigned int width  = lcd_width - left - 4 - 50;

		if ((muted) || (volume==0))
			display.load_screen_element(&(element[ELEMENT_MUTE]), 0, lcd_height-element[ELEMENT_MUTE].header.height);
		else
		{
			if (icon_dolby)
				display.load_screen_element(&(element[ELEMENT_DOLBY]), 0, lcd_height-element[ELEMENT_DOLBY].header.height);
			else
				display.load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].header.height);
		}

		//strichlin
		if ((muted) || (volume==0))
		{
			display.draw_line (left, top, left+width, top+height-1, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp = vol*(width+1)/100;
			display.draw_fill_rect (left-1, top-1, left+dp, top+height, CLCDDisplay::PIXEL_ON);
		}
		
		if(mode == MODE_AUDIO)
		{
			// 51 = top-1-2
			// 52 = top-1-1

			// 54 = top-1+1
			// 55 = top-1+2
			// 58 = lcd_height-3-3
			// 59 = lcd_height-3-2
			// 61 = lcd_height-3
			display.draw_fill_rect (-1, top-2         , 10, lcd_height-3  , CLCDDisplay::PIXEL_OFF);
			display.draw_rectangle ( 1, top+2         ,  3, lcd_height-3-3, CLCDDisplay::PIXEL_ON, CLCDDisplay::PIXEL_OFF);
			display.draw_line      ( 3, top+2         ,  6, top-1         , CLCDDisplay::PIXEL_ON);
			display.draw_line      ( 3, lcd_height-3-3,  6, lcd_height-3  , CLCDDisplay::PIXEL_ON);
			display.draw_line      ( 6, top+1         ,  6, lcd_height-3-2, CLCDDisplay::PIXEL_ON);
		}

		if (perform_update)
		  displayUpdate();
	}
	wake_up();
}

void CLCD::showPercentOver(const unsigned char perc, const bool perform_update, const MODES m)
{
	if(!has_lcd) 
		return;
	
	if (mode != m)
		return;

	int left, top, width, height = 6;
	bool draw = true;
	percentOver = perc;
	
	if (mode == MODE_TVRADIO || mode == MODE_MOVIE)
	{
		unsigned int lcd_width = display.xres;
		unsigned int lcd_height = display.yres;

		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 0)
		{
			left = 12+2; top = lcd_height - height - 1 - 2; width = lcd_width - left - 4 - 50;
		}
		else if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 2)
		{
			left = 12+2; top = 1+2; width = lcd_width - left - 4;
		}
		else if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] == 3)
		{
			left = 12+2; top = 1+2; width = lcd_width - left - 4 - 20;

			if ((g_RemoteControl != NULL && mode == MODE_TVRADIO) || mode == MODE_MOVIE)
			{
				bool is_ac3;
				if (mode == MODE_MOVIE)
					is_ac3 = movie_is_ac3;
				else
				{
					uint count = g_RemoteControl->current_PIDs.APIDs.size();
					if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < count) &&
					    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
						is_ac3 = true;
					else
						is_ac3 = false;
				}

				const char * icon;
				if (is_ac3)
					icon = DATADIR "/lcdd/icons/dd.raw";
				else
					icon = DATADIR "/lcdd/icons/stereo.raw";

				display.paintIcon(icon, left+width+5, 1, false);
			}
		}
		else
			draw = false;

		if (draw)
		{
			display.draw_rectangle (left-2, top-2, left+width+2, top+height+1, CLCDDisplay::PIXEL_ON, CLCDDisplay::PIXEL_OFF);

			if (perc == (unsigned char) -1)
			{
				display.draw_line (left, top, left+width, top+height-1, CLCDDisplay::PIXEL_ON);
			}
			else
			{
				int dp;
				if (perc == (unsigned char) -2)
					dp = width+1;
				else
					dp = perc * (width + 1) / 100;
				display.draw_fill_rect (left-1, top-1, left+dp, top+height, CLCDDisplay::PIXEL_ON);

				if (perc == (unsigned char) -2)
				{
					// draw a "+" to show that the event is overdue
					display.draw_line(left+width-2, top+1, left+width-2, top+height-2, CLCDDisplay::PIXEL_OFF);
					display.draw_line(left+width-1, top+(height/2), left+width-3, top+(height/2), CLCDDisplay::PIXEL_OFF);
				}
			}
		}

		if (perform_update)
			displayUpdate();
	}
}

void CLCD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if(!has_lcd) 
		return;
	
	/* hack, to not have to patch too much in movieplayer.cpp */
	if (mode == MODE_MOVIE) 
	{
		size_t p;
		AUDIOMODES m = movie_playmode;
		std::string mytext = text;
		if (mytext.find("> ") == 0) 
		{
			mytext = mytext.substr(2);
			m = AUDIO_MODE_PLAY;
		} 
		else if (mytext.find("|| ") == 0) 
		{
			mytext = mytext.substr(3);
			m = AUDIO_MODE_PAUSE;
		} 
		else if ((p = mytext.find("s||> ")) < 3) 
		{
			mytext = mytext.substr(p + 5);
			m = AUDIO_MODE_PLAY;
		} 
		else if ((p = mytext.find("x>> ")) < 3) 
		{
			mytext = mytext.substr(p + 4);
			m = AUDIO_MODE_FF;
		} 
		else if ((p = mytext.find("x<< ")) < 3) 
		{
			mytext = mytext.substr(p + 4);
			m = AUDIO_MODE_REV;
		}
		setMovieInfo(m, "", mytext, false);
		return;
	}

	if (mode != MODE_MENU_UTF8)
		return;

	// reload specified line
	unsigned int lcd_width  = display.xres;
	unsigned int lcd_height = display.yres;
	
	display.draw_fill_rect(-1, 35+14*position, lcd_width, 35+14+14*position, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0,35+11+14*position, lcd_width + 20, text, CLCDDisplay::PIXEL_INV, highlight, utf_encoded);

	wake_up();
	displayUpdate();
}

void CLCD::showAudioTrack(const std::string & artist, const std::string & title, const std::string & album, int pos)
{
	if(!has_lcd) 
		return;
	
	if (mode != MODE_AUDIO) 
		return;
	
	unsigned int lcd_width  = display.xres;
	unsigned int lcd_height = display.yres;

	// reload specified line
	display.draw_fill_rect (-1, 10, lcd_width, 24, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1, 20, lcd_width, 37, CLCDDisplay::PIXEL_OFF);
	display.draw_fill_rect (-1, 33, lcd_width, 50, CLCDDisplay::PIXEL_OFF);
	fonts.menu->RenderString(0, 22, lcd_width + 5, artist.c_str(), CLCDDisplay::PIXEL_ON, 0, isUTF8(artist));
	fonts.menu->RenderString(0, 35, lcd_width + 5, album.c_str(),  CLCDDisplay::PIXEL_ON, 0, isUTF8(album));
	fonts.menu->RenderString(0, 48, lcd_width + 5, title.c_str(),  CLCDDisplay::PIXEL_ON, 0, isUTF8(title));

	wake_up();
	displayUpdate();

}

void CLCD::showAudioPlayMode(AUDIOMODES m)
{
	if(!has_lcd) 
		return;
	
	display.draw_fill_rect (-1,51,10,62, CLCDDisplay::PIXEL_OFF);
	switch(m)
	{
		case AUDIO_MODE_PLAY:
			{
				int x=3,y=53;
				display.draw_line(x  ,y  ,x  ,y+8, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1,y+1,x+1,y+7, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2,y+2,x+2,y+6, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3,y+3,x+3,y+5, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4,y+4,x+4,y+4, CLCDDisplay::PIXEL_ON);
				break;
			}
		case AUDIO_MODE_STOP:
			display.draw_fill_rect (1, 53, 8 ,61, CLCDDisplay::PIXEL_ON);
			break;
		case AUDIO_MODE_PAUSE:
			display.draw_line(1,54,1,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(2,54,2,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(6,54,6,60, CLCDDisplay::PIXEL_ON);
			display.draw_line(7,54,7,60, CLCDDisplay::PIXEL_ON);
			break;
		case AUDIO_MODE_FF:
			{
				int x=2,y=55;
				display.draw_line(x   ,y   , x  , y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2 ,y+2 , x+2, y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3 ,y   , x+3, y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+5 ,y+2 , x+5, y+2, CLCDDisplay::PIXEL_ON);
			}
			break;
		case AUDIO_MODE_REV:
			{
				int x=2,y=55;
				display.draw_line(x   ,y+2 , x  , y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+2 ,y   , x+2, y+4, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+3 ,y+2 , x+3, y+2, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display.draw_line(x+5 ,y   , x+5, y+4, CLCDDisplay::PIXEL_ON);
			}
			break;
	}
	wake_up();
	displayUpdate();
}

void CLCD::showAudioProgress(const char perc, bool isMuted)
{
	if(!has_lcd) 
		return;
	
	if (mode == MODE_AUDIO)
	{
		display.draw_fill_rect (11,53,73,61, CLCDDisplay::PIXEL_OFF);
		int dp = int( perc/100.0*61.0+12.0);
		display.draw_fill_rect (11,54,dp,60, CLCDDisplay::PIXEL_ON);
		if(isMuted)
		{
			if(dp > 12)
			{
				display.draw_line(12, 56, dp-1, 56, CLCDDisplay::PIXEL_OFF);
				display.draw_line(12, 58, dp-1, 58, CLCDDisplay::PIXEL_OFF);
			}
			else
				display.draw_line (12,55,72,59, CLCDDisplay::PIXEL_ON);
		}
		displayUpdate();
	}
}

void CLCD::drawBanner()
{
	if(!has_lcd) 
		return;
	
	unsigned int lcd_width  = display.xres;
	display.load_screen_element(&(element[ELEMENT_BANNER]), 0, 0);
	
	if (element[ELEMENT_BANNER].header.width < lcd_width)
		display.draw_fill_rect (element[ELEMENT_BANNER].header.width-1, -1, lcd_width, element[ELEMENT_BANNER].header.height-1, CLCDDisplay::PIXEL_ON);
}

void CLCD::setMode(const MODES m, const char * const title)
{
	if(!has_lcd) 
		return;
	
	unsigned int lcd_width  = display.xres;
	unsigned int lcd_height = display.yres;

	mode = m;
	menutitle = title;
	setlcdparameter();

	switch (m)
	{
	case MODE_TVRADIO:
	case MODE_MOVIE:
		display.clear_screen(); // clear lcd
		
		switch (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME])
		{
		case 0:
			drawBanner();
			display.load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].header.width)/2, 12);
			display.load_screen_element(&(element[ELEMENT_MOVIESTRIPE]), 0, lcd_height-element[ELEMENT_MOVIESTRIPE].header.height);
			showPercentOver(percentOver, false, mode);
			break;
		case 1:
			drawBanner();
			display.load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].header.width)/2, 12);
			showVolume(volume, false);
			break;
		case 2:
			display.load_screen_element(&(element[ELEMENT_MOVIESTRIPE]), 0, 0);
			display.load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].header.width)/2, 12);
			showVolume(volume, false);
			showPercentOver(percentOver, false, mode);
			break;
		case 3:
			display.load_screen_element(&(element[ELEMENT_MOVIESTRIPE]), 0, 0);
			display.load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].header.width)/2, 12);
			showVolume(volume, false);
			showPercentOver(percentOver, false, mode);
			break;
		default:
			break;
		}
		if (mode == MODE_TVRADIO)
			showServicename(servicename);
		else
		{
			setMovieInfo(movie_playmode, movie_big, movie_small, movie_centered);
			setMovieAudio(movie_is_ac3);
		}
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	case MODE_AUDIO:
	{
		display.clear_screen(); // clear lcd
		drawBanner();
		display.load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].header.height-1);
		showAudioPlayMode(AUDIO_MODE_STOP);
		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	}
	case MODE_SCART:
		display.clear_screen(); // clear lcd
		drawBanner();
		display.load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].header.width)/2, 12);
		display.load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].header.height-1);

		showVolume(volume, false);
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	case MODE_MENU_UTF8:
		showclock = false;
		display.clear_screen(); // clear lcd
		drawBanner();
		fonts.menutitle->RenderString(0, 28, display.xres + 20, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		displayUpdate();
		break;
	case MODE_SHUTDOWN:
		showclock = false;
		display.clear_screen(); // clear lcd
		drawBanner();
		display.load_screen_element(&(element[ELEMENT_POWER]), (lcd_width-element[ELEMENT_POWER].header.width)/2, 12);
		displayUpdate();
		break;
	case MODE_STANDBY:
		showclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		                 /* "showTime()" clears the whole lcd in MODE_STANDBY                         */
		break;
#ifdef LCD_UPDATE
	case MODE_FILEBROWSER:
		showclock = true;
		display.clear_screen(); // clear lcd
		showFilelist();
		break;
	case MODE_PROGRESSBAR:
		showclock = false;
		display.clear_screen(); // clear lcd
		drawBanner();
		showProgressBar();
		break;
	case MODE_PROGRESSBAR2:
		showclock = false;
		display.clear_screen(); // clear lcd
		drawBanner();
		showProgressBar2();
		break;
	case MODE_INFOBOX:
		showclock = false;
		showInfoBox();
		break;
#endif // LCD_UPDATE
	}
	
	wake_up();
}


void CLCD::setBrightness(int bright)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS] = bright;
	setlcdparameter();
}

int CLCD::getBrightness()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS];
}

void CLCD::setBrightnessStandby(int bright)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] = bright;
	setlcdparameter();
}

int CLCD::getBrightnessStandby()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS];
}

void CLCD::setContrast(int contrast)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST] = contrast;
	setlcdparameter();
}

int CLCD::getContrast()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST];
}

void CLCD::setPower(int power)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER] = power;
	setlcdparameter();
}

int CLCD::getPower()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER];
}

void CLCD::togglePower(void)
{
	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_setting[SNeutrinoSettings::LCD_STANDBY_BRIGHTNESS] : g_settings.lcd_setting[SNeutrinoSettings::LCD_BRIGHTNESS],
			g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST],
			last_toggle_state_power,
			g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE],
			0 /*g_settings.lcd_setting[SNeutrinoSettings::LCD_BIAS]*/);
}

void CLCD::setInverse(int inverse)
{
	g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE] = inverse;
	setlcdparameter();
}

int CLCD::getInverse()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE];
}

void CLCD::setAutoDimm(int /*autodimm*/)
{
}

int CLCD::getAutoDimm()
{
	return g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM];
}

void CLCD::setMuted(bool mu)
{
	muted = mu;
	showVolume(volume);
}

void CLCD::resume()
{
	if(!has_lcd) 
		return;
	
	display.resume();
}

void CLCD::pause()
{
	if(!has_lcd) 
		return;
	
	display.pause();
}

void CLCD::ShowIcon(vfd_icon icon, bool show)
{
	switch (icon)
	{
		case VFD_ICON_MUTE:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_MUTE", show);
			break;
		case VFD_ICON_DOLBY:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_DOLBY", show);
			icon_dolby = show;
			break;
		case VFD_ICON_POWER:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_POWER", show);
			break;
		case VFD_ICON_TIMESHIFT:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_TIMESHIFT", show);
			break;
		case VFD_ICON_TV:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_TV", show);
			break;
		case VFD_ICON_RADIO:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_RADIO", show);
			break;
		case VFD_ICON_HD:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_HD", show);
			break;
		case VFD_ICON_1080P:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_1080P", show);
			break;
		case VFD_ICON_1080I:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_1080I", show);
			break;
		case VFD_ICON_720P:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_720P", show);
			break;
		case VFD_ICON_480P:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_480P", show);
			break;
		case VFD_ICON_480I:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_480I", show);
			break;
		case VFD_ICON_USB:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_USB", show);
			break;
		case VFD_ICON_MP3:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_MP3", show);
			break;
		case VFD_ICON_PLAY:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_PLAY", show);
			break;
		case VFD_ICON_PAUSE:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_PAUSE", show);
			break;
		case VFD_ICON_LOCK:
			fprintf(stderr, "CLCD::ShowIcon(%s, %d)\n", "VFD_ICON_LOCK", show);
		default: 
			fprintf(stderr, "CLCD::ShowIcon(%d, %d)\n", icon, show);
	}	
}

void CLCD::Lock()
{
/*
	if(!has_lcd) return;
	creat("/tmp/vfd.locked", 0);
*/
}

void CVFD::Unlock()
{
/*
	if(!has_lcd) return;
	unlink("/tmp/vfd.locked");
*/
}

void CLCD::Clear()
{
	if(!has_lcd) 
		return;
	
	if (mode == MODE_SHUTDOWN)
	{
		display.clear_screen(); // clear lcd
		displayUpdate();
	}

	return;
}

bool CLCD::ShowPng(char *filename)
{
	if(!has_lcd) 
		return false;
	
	return display.load_png(filename);
}

bool CLCD::DumpPng(char *filename)
{
	if(!has_lcd) 
		return false;
	
	return display.dump_png(filename);
}

#ifdef LCD_UPDATE
// showInfoBox
#define EPG_INFO_FONT_HEIGHT 9
#define EPG_INFO_SHADOW_WIDTH 1
#define EPG_INFO_LINE_WIDTH 1
#define EPG_INFO_BORDER_WIDTH 2

#define EPG_INFO_WINDOW_POS 4
#define EPG_INFO_LINE_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH
#define EPG_INFO_BORDER_POS EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH
#define EPG_INFO_TEXT_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH + EPG_INFO_BORDER_WIDTH

#define EPG_INFO_TEXT_WIDTH lcd_width - (2*EPG_INFO_WINDOW_POS)

// timer 0: OFF, timer>0 time to show in seconds,  timer>=999 endless
void CLCD::showInfoBox(const char * const title, const char * const text ,int autoNewline,int timer)
{
	if(!has_lcd) 
		return;
	
	//printf("[lcdd] Info: \n");
	if(text != NULL)
		m_infoBoxText = text;
	if(title != NULL)
		m_infoBoxTitle = title;
	if(timer != -1)
		m_infoBoxTimer = timer;
	if(autoNewline != -1)
		m_infoBoxAutoNewline = autoNewline;

	//printf("[lcdd] Info: %s,%s,%d,%d\n",m_infoBoxTitle.c_str(),m_infoBoxText.c_str(),m_infoBoxAutoNewline,m_infoBoxTimer);
	if( mode == MODE_INFOBOX &&
	    !m_infoBoxText.empty())
	{
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;
		
		// paint empty box
		display.draw_fill_rect (EPG_INFO_WINDOW_POS, EPG_INFO_WINDOW_POS, 	lcd_width-EPG_INFO_WINDOW_POS+1, 	  lcd_height-EPG_INFO_WINDOW_POS+1,    CLCDDisplay::PIXEL_OFF);
		display.draw_fill_rect (EPG_INFO_LINE_POS, 	 EPG_INFO_LINE_POS, 	lcd_width-EPG_INFO_LINE_POS-1, 	  lcd_height-EPG_INFO_LINE_POS-1, 	 CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (EPG_INFO_BORDER_POS, EPG_INFO_BORDER_POS, 	lcd_width-EPG_INFO_BORDER_POS-3,  lcd_height-EPG_INFO_BORDER_POS-3, CLCDDisplay::PIXEL_OFF);

		// paint title
		if(!m_infoBoxTitle.empty())
		{
			int width = fonts.menu->getRenderWidth(m_infoBoxTitle.c_str(),true);
			
			if(width > lcd_width - 20)
				width = lcd_width - 20;
			int start_pos = (lcd_width - width) /2;

			display.draw_fill_rect (start_pos, EPG_INFO_WINDOW_POS-4, 	start_pos+width+5, 	  EPG_INFO_WINDOW_POS+10,    CLCDDisplay::PIXEL_OFF);
			fonts.menu->RenderString(start_pos+4,EPG_INFO_WINDOW_POS+5, width+5, m_infoBoxTitle.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		}

		// paint info 
		std::string text_line;
		int line;
		int pos = 0;
		int length = m_infoBoxText.size();
		for(line = 0; line < 5; line++)
		{
			text_line.clear();
			while ( m_infoBoxText[pos] != '\n' &&
					((fonts.menu->getRenderWidth(text_line.c_str(), true) < EPG_INFO_TEXT_WIDTH-10) || !m_infoBoxAutoNewline )&& 
					(pos < length)) // UTF-8
			{
				if ( m_infoBoxText[pos] >= ' ' && m_infoBoxText[pos] <= '~' )  // any char between ASCII(32) and ASCII (126)
					text_line += m_infoBoxText[pos];
				pos++;
			} 
			//printf("[lcdd] line %d:'%s'\r\n",line,text_line.c_str());
			fonts.menu->RenderString(EPG_INFO_TEXT_POS+1,EPG_INFO_TEXT_POS+(line*EPG_INFO_FONT_HEIGHT)+EPG_INFO_FONT_HEIGHT+3, EPG_INFO_TEXT_WIDTH, text_line.c_str(), CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
			if ( m_infoBoxText[pos] == '\n' )
				pos++; // remove new line
		}
		displayUpdate();
	}
}

//showFilelist
#define BAR_POS_X 		114
#define BAR_POS_Y 		 10
#define BAR_POS_WIDTH 	  6
#define BAR_POS_HEIGTH 	 40

void CLCD::showFilelist(int flist_pos,CFileList* flist,const char * const mainDir)
{
	if(!has_lcd) 
		return;
	
	//printf("[lcdd] FileList\n");
	if(flist != NULL)
		m_fileList = flist;
	if(flist_pos != -1)
		m_fileListPos = flist_pos;
	if(mainDir != NULL)
		m_fileListHeader = mainDir;
		
	if (mode == MODE_FILEBROWSER && 
	    m_fileList != NULL &&
	    m_fileList->size() > 0)
	{    
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;

		dprintf(DEBUG_NORMAL, "CLCD::showFilelist: FileList:OK\n");
		int size = m_fileList->size();
		
		display.draw_fill_rect(-1, -1, lcd_width, lcd_height-12, CLCDDisplay::PIXEL_OFF); // clear lcd
		
		if(m_fileListPos > size)
			m_fileListPos = size-1;
		
		int width = fonts.menu->getRenderWidth(m_fileListHeader.c_str(), true); 
		
		if(width > lcd_width - 10)
			width = lcd_width - 10;
		fonts.menu->RenderString((lcd_width - width) / 2, 11, width+5, m_fileListHeader.c_str(), CLCDDisplay::PIXEL_ON);
		
		//printf("list%d,%d\r\n",m_fileListPos,(*m_fileList)[m_fileListPos].Marked);
		std::string text;
		int marked;
		
		if(m_fileListPos >  0)
		{
			if ( (*m_fileList)[m_fileListPos-1].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos-1].getFileName();
			fonts.menu->RenderString(1, 12+12, BAR_POS_X+5, text.c_str(), marked);
		}
		
		if(m_fileListPos <  size)
		{
			if ((*m_fileList)[m_fileListPos-0].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos-0].getFileName();
			fonts.time->RenderString(1, 12+12+14, BAR_POS_X+5, text.c_str(), marked);
		}
		
		if(m_fileListPos <  size-1)
		{
			if ((*m_fileList)[m_fileListPos+1].Marked == false )
			{
				text ="";
				marked = CLCDDisplay::PIXEL_ON;
			}
			else
			{
				text ="*";
				marked = CLCDDisplay::PIXEL_INV;
			}
			text += (*m_fileList)[m_fileListPos+1].getFileName();
			fonts.menu->RenderString(1, 12+12+14+12, BAR_POS_X+5, text.c_str(), marked);
		}
		
		// paint marker
		int pages = (((size-1)/3 )+1);
		int marker_length = (BAR_POS_HEIGTH-2) / pages;
		if(marker_length <4)
			marker_length=4;// not smaller than 4 pixel
		int marker_offset = ((BAR_POS_HEIGTH-2-marker_length) * m_fileListPos) /size  ;
		//printf("%d,%d,%d\r\n",pages,marker_length,marker_offset);
		
		display.draw_fill_rect (BAR_POS_X,   BAR_POS_Y,   BAR_POS_X+BAR_POS_WIDTH,   BAR_POS_Y+BAR_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+BAR_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		display.draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1+marker_offset, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+1+marker_offset+marker_length, CLCDDisplay::PIXEL_ON);
	
		displayUpdate();
	}
}

//showProgressBar
#define PROG_GLOB_POS_X 10
#define PROG_GLOB_POS_Y 30
#define PROG_GLOB_POS_WIDTH 100
#define PROG_GLOB_POS_HEIGTH 20
void CLCD::showProgressBar(int global, const char * const text,int show_escape,int timer)
{
	if(!has_lcd) 
		return;
	
	if(text != NULL)
		m_progressHeaderGlobal = text;
		
	if(timer != -1)
		m_infoBoxTimer = timer;
		
	if(global >= 0)
	{
		if(global > 100)
			m_progressGlobal =100;
		else
			m_progressGlobal = global;
	}
	
	if(show_escape != -1)
		m_progressShowEscape = show_escape;

	if (mode == MODE_PROGRESSBAR)
	{
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;

		//printf("[lcdd] prog:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		// Clear Display
		display.draw_fill_rect (0, 12, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);
	
		// paint progress header 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, 12+12, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display.draw_fill_rect (PROG_GLOB_POS_X,   				 PROG_GLOB_POS_Y,   PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH,   PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG_GLOB_POS_X+1+marker_length, PROG_GLOB_POS_Y+1, PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH-1, PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(lcd_width-40, lcd_height, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		
		displayUpdate();
	}
}

// showProgressBar2
#define PROG2_GLOB_POS_X 10
#define PROG2_GLOB_POS_Y 37
#define PROG2_GLOB_POS_WIDTH 100
#define PROG2_GLOB_POS_HEIGTH 10

#define PROG2_LOCAL_POS_X 10
#define PROG2_LOCAL_POS_Y 24
#define PROG2_LOCAL_POS_WIDTH PROG2_GLOB_POS_WIDTH
#define PROG2_LOCAL_POS_HEIGTH PROG2_GLOB_POS_HEIGTH

void CLCD::showProgressBar2(int local,const char * const text_local ,int global ,const char * const text_global ,int show_escape )
{
	if(!has_lcd) 
		return;
	
	//printf("[lcdd] prog2\n");
	if(text_local != NULL)
		m_progressHeaderLocal = text_local;
		
	if(text_global != NULL)
		m_progressHeaderGlobal = text_global;
		
	if(global >= 0)
	{
		if(global > 100)
			m_progressGlobal =100;
		else
			m_progressGlobal = global;
	}
	
	if(local >= 0)
	{
		if(local > 100)
			m_progressLocal =100;
		else
			m_progressLocal = local;
	}
	
	if(show_escape != -1)
		m_progressShowEscape = show_escape;

	if (mode == MODE_PROGRESSBAR2)
	{
		unsigned int lcd_width  = display.xres;
		unsigned int lcd_height = display.yres;

		//printf("[lcdd] prog2:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		// Clear Display
		display.draw_fill_rect (0, 12, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);
	
		// paint  global caption 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, PROG2_GLOB_POS_Y+20, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG2_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display.draw_fill_rect (PROG2_GLOB_POS_X,   				PROG2_GLOB_POS_Y,   PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH,   PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG2_GLOB_POS_X+1+marker_length, PROG2_GLOB_POS_Y+1, PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH-1, PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		
		// paint  local caption 
		width = fonts.menu->getRenderWidth(m_progressHeaderLocal.c_str(),true);
		if(width > 100)
			width = 100;
		start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, PROG2_LOCAL_POS_Y -3, width+10, m_progressHeaderLocal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
		// paint local bar 
		marker_length = (PROG2_LOCAL_POS_WIDTH * m_progressLocal)/100;
		
		display.draw_fill_rect (PROG2_LOCAL_POS_X,   				PROG2_LOCAL_POS_Y,   PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH,   PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display.draw_fill_rect (PROG2_LOCAL_POS_X+1+marker_length,   PROG2_LOCAL_POS_Y+1, PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH-1, PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(lcd_width-40, lcd_height, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		
		displayUpdate();
	}
}
#endif // LCD_UPDATE


