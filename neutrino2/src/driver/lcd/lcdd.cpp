/*
	$Id: lcdd.cpp 30052024 mohousch Exp $

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

#include <driver/lcd/lcdd.h>

#include <global.h>
#include <neutrino2.h>

#include <system/settings.h>
#include <system/debug.h>
#include <system/helpers.h>

#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <daemonc/remotecontrol.h>

#ifdef ENABLE_GRAPHLCD
#include <driver/lcd/nglcd.h>
#endif


extern CRemoteControl * g_RemoteControl;

#if defined (__sh__)
#if defined (PLATFORM_SPARK7162)
static struct aotom_ioctl_data aotom_data;
#endif

#if defined (PLATFORM_KATHREIN) || defined (PLATFORM_SPARK7162)
static bool usb_icon = false;
static bool timer_icon = false;
static bool hdd_icon = false;
#endif

bool blocked = false;

void CLCD::openDevice()
{ 
        if (!blocked)
	{
		fd = open("/dev/vfd", O_RDWR);
		if(fd < 0)
		{
			printf("failed to open vfd\n");
			fd = open("/dev/fplarge", O_RDWR);
			if (fd < 0)
			    printf("failed to open fplarge\n");
		}
		else
			blocked = true;
	}
}

void CLCD::closeDevice()
{ 
	if (fd)
	{
		close(fd);
		blocked = false;
	}
	
	fd = -1;
}
#endif

////
CLCD::CLCD()
{
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
	muted = false;
	percentOver = 0;
	volume = 0;
	timeout_cnt = 0;
	icon_dolby = false;	
	has_lcd = false;
	clearClock = 0;
	fd = -1;
	lcd_width = 132;
	lcd_height = 64;
	servicename = "";
	servicenumber = 0;
	epg_title = "";
	movie_big = "";
	movie_small = "";
	menutitle = "";
	movie_playmode = PLAY_MODE_STOP;
	showclock = false;

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	display = NULL;	
#endif

#ifdef ENABLE_GRAPHLCD
	nglcd = NULL;
	nglcdshowclock= false;
	nglcdclearClock = 0;
#endif
}

CLCD::~CLCD()
{
#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	if (fd)
		::close(fd);
		
	fd = -1;
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	if (display)
	{
		delete display;
		display = NULL;
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (nglcd)
	{
		delete nglcd;
		nglcd = NULL;
	}
#endif
}

CLCD* CLCD::getInstance()
{
	static CLCD *lcdd = NULL;
	
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
		
#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
		g_settings.lcd_setting_dim_brightness > 0 ? setBrightness(g_settings.lcd_brightness) : setPower(1);
#elif defined (ENABLE_LCD)
		setlcdparameter();
#endif

#ifdef ENABLE_GRAPHLCD
		setlcdparameter();
#endif
	}
#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	else
		setPower(1);
#endif
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
	// init lcd 
	if (!lcdInit(fontfile, fontname, fontfile2, fontname2, fontfile3, fontname3 ))
	{
		dprintf(DEBUG_NORMAL, "CLCD::init: failed!\n");
		has_lcd = false;
		return;
	}
	
	dprintf(DEBUG_NORMAL, "CLCD::init: succeeded\n");

	// create time thread
	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		perror("CLCD::init: pthread_create(TimeThread)");
		return ;
	}
}

// FIXME: add more e.g weather icons and so on
enum elements {
	ELEMENT_BANNER 		= 0,
	ELEMENT_PROG 		= 1,
	ELEMENT_SPEAKER  	= 2,
	ELEMENT_SCART  		= 3,
	ELEMENT_POWER  		= 4,
	ELEMENT_MUTE 		= 5,
	ELEMENT_DOLBY 		= 6,
	ELEMENT_LOCK 		= 7,
	ELEMENT_HD		= 8,
	ELEMENT_TV		= 9,
	ELEMENT_RADIO		= 10,
	ELEMENT_PLAY		= 11,
	ELEMENT_PAUSE		= 12,
	ELEMENT_ACLOCK		= 13,
	ELEMENT_PICON		= 14	
};

const char * const element_name[LCD_NUMBER_OF_ELEMENTS] = {
	"lcdbanner",
	"lcdprog",
	"lcdvol",
	"lcdscart",
	"lcdpower",
	"lcdmute",
	"lcddolby",
	"lcdlock",
	"lcdhd",
	"lcdtv",
	"lcdradio",
	"lcdplay",
	"lcdpause",
	"a_clock",
	"picon_default"
};

bool CLCD::lcdInit(const char * fontfile, const char * fontname, const char * fontfile2, const char * fontname2, const char * fontfile3, const char * fontname3)
{
	// 4digits
#ifdef ENABLE_4DIGITS
#ifdef USE_OPENGL
	fd = open("/dev/null", O_RDWR);
#else
	fd = open("/dev/dbox/fp", O_RDWR);
#endif
		
	if(fd < 0)
	{
		// probe /dev/vfd
		fd = open("/dev/vfd", O_RDWR);
		
		if(fd < 0)
		{
			// probe /dev/display
			fd = open("/dev/display", O_RDWR);
			
			if(fd < 0)
			{
				fd = open("/dev/mcu", O_RDWR);

				if(fd < 0)
				{
					// probe /proc/vfd (e.g gigablue)
					fd = open("/proc/vfd", O_RDWR);
				
					if(fd < 0)
					{
						// probe /dev/dbox/oled0
						fd = open("/dev/dbox/oled0", O_RDWR);
		
						if(fd < 0) 
						{
							// probe /dev/oled0
							fd = open("/dev/oled0", O_RDWR);
						
							if(fd < 0)
							{
								fd = open("/dev/dbox/lcd0", O_RDWR);
							}
						}
					}
				}
			}
		}
	}
	
	if (fd >= 0) has_lcd = true;
	
	// set led color
#if defined (PLATFORM_GIGABLUE)
	setPower(g_settings.lcd_power);  //0:off, 1:blue, 2:red, 3:purple
#endif	// 4digits
#elif defined (ENABLE_VFD)
#if defined (__sh__)
	has_lcd = true;
#else
#ifdef USE_OPENGL
	fd = open("/dev/null", O_RDWR);
#else
	fd = open("/dev/dbox/fp", O_RDWR);
#endif
		
	if(fd < 0)
	{
		// probe /dev/vfd
		fd = open("/dev/vfd", O_RDWR);
		
		if(fd < 0)
		{
			// probe /dev/display
			fd = open("/dev/display", O_RDWR);
			
			if(fd < 0)
			{
				fd = open("/dev/mcu", O_RDWR);

				if(fd < 0)
				{
					// probe /proc/vfd (e.g gigablue)
					fd = open("/proc/vfd", O_RDWR);
				
					if(fd < 0)
					{
						// probe /dev/dbox/oled0
						fd = open("/dev/dbox/oled0", O_RDWR);
		
						if(fd < 0) 
						{
							// probe /dev/oled0
							fd = open("/dev/oled0", O_RDWR);
						
							if(fd < 0)
							{
								fd = open("/dev/dbox/lcd0", O_RDWR);
							}
						}
					}
				}
			}
		}
	}
	
	if (fd >= 0) has_lcd = true;
#endif // vfd
#elif defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	display = new CLCDDisplay();
	
	// check if we have display
#ifdef USE_OPENGL
	if (display->init("/dev/null"))
#else
	if (display->init("/dev/fb1"))
#endif
		has_lcd = true;
		
	lcd_width = display->xres;
	lcd_height = display->yres;
	
	// init fonts
	fontRenderer = new LcdFontRenderClass(display);
	
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

	fonts.menu        = fontRenderer->getFont(fontname,  style_name , 24);
	fonts.time        = fontRenderer->getFont(fontname2, style_name2, 20);
	fonts.channelname = fontRenderer->getFont(fontname3, style_name3, 30);
	fonts.menutitle   = fonts.channelname;
	fonts.timestandby = fontRenderer->getFont(fontname,  style_name , 50);
	
	has_lcd = true;

 	// init lcd_element struct
	for (int i = 0; i < LCD_NUMBER_OF_ELEMENTS; i++)
	{
		element[i].width = 0;
		element[i].height = 0;
		element[i].buffer = NULL;

		std::string file = DATADIR "/lcd/";
		file += element_name[i];
		file += ".png";
		
		element[i].name = file.c_str();
			
		display->load_png_element(file.c_str(), &(element[i]));
	}
#endif

	//nglcd
#ifdef ENABLE_GRAPHLCD
	nglcd = new nGLCD();
	
	if (nglcd->init())
		has_lcd = true;
#endif

	// set mode tv/radio
	setMode(MODE_TVRADIO);
	setLED(g_settings.lcd_led, 0);

	return has_lcd;
}

void CLCD::displayUpdate()
{
	struct stat buf;
	
#ifdef ENABLE_LCD
	if (stat("/tmp/lcd.locked", &buf) == -1)
	{
		display->update();
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (stat("/tmp/lcd.locked", &buf) == -1)
	{
		if (g_settings.glcd_enable) nglcd->update();
	}
#endif
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse, const int bias)
{
	if(!has_lcd) 
		return;
		
	if (power == 0)
		dimm = 0;
		
#if defined (ENABLE_VFD)
#ifdef __sh__
	if(dimm < 0)
		dimm = 0;
	else if(dimm > 15)
		dimm = 15;
	
        struct vfd_ioctl_data data;
	data.start_address = dimm;
	
	if(dimm < 1)
		dimm = 1;
	
	openDevice();
	
	if( ioctl(fd, VFDBRIGHTNESS, &data) < 0)  
		perror("VFDBRIGHTNESS");
	
	closeDevice();
#endif	
#elif defined (ENABLE_LCD)
	// dimm
	display->setLCDBrightness(dimm);
	
	// contrast
	display->setLCDContrast(contrast);
	
	//reverse
	if (inverse)
		display->setInverted(CLCDDisplay::PIXEL_ON);
	else		
		display->setInverted(CLCDDisplay::PIXEL_OFF);
#endif

#ifdef ENABLE_GRAPHLCD
	if (g_settings.glcd_enable) nglcd->SetBrightness(dimm);
#endif
}

void CLCD::setlcdparameter(void)
{
	if(!has_lcd) 
		return;

	last_toggle_state_power = g_settings.lcd_power;
	int dim_time = atoi(g_settings.lcd_setting_dim_time);
	int dim_brightness = g_settings.lcd_setting_dim_brightness;
	bool timeouted = (dim_time > 0) && (timeout_cnt == 0);
	int brightness = 0;
	int power = 0;

	if (timeouted)
		brightness = dim_brightness;
	else
#ifdef ENABLE_GRAPHLCD
		brightness = g_settings.glcd_brightness;
#else
		brightness = g_settings.lcd_brightness;
#endif

	if (last_toggle_state_power && (!timeouted || dim_brightness > 0))
		power = 1;

	if (mode == MODE_STANDBY)
#ifdef ENABLE_GRAPHLCD
		brightness = g_settings.glcd_brightness_standby;
#else
		brightness = g_settings.lcd_standbybrightness;
#endif
		
	setlcdparameter(brightness,
			g_settings.lcd_contrast,
			power,
			g_settings.lcd_inverse,
			0);
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

#ifdef ENABLE_LCD
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
#endif

	return tmp;
}

void CLCD::showTextScreen(const std::string &big, const std::string &small, const int showmode, const bool perform_wakeup, const bool centered)
{
	if(!has_lcd) 
		return;
		
	printf("CLCD::showTextScreen: big:%s small:%s showmode:0x%x wakeup:%s centered:%s\n", big.empty()? "null" : big.c_str(), small.empty()? "null" : small.c_str(), showmode, perform_wakeup? "true" : "false", centered? "centered" : "not centered");
		
#if defined (ENABLE_4DIGITS)
	int len = big.length();
	
	if(len == 0)
		return;

	// replace chars
	replace_all(big, "\x0d", "");
    	replace_all(big, "\n\n", "\\N");
	replace_all(big, "\n", "");
    	replace_all(big, "\\N", "\n");
    	replace_all(big, "ö", "oe");
    	replace_all(big, "ä", "ae");
    	replace_all(big, "ü", "ue");
	replace_all(big, "Ö", "Oe");
    	replace_all(big, "Ä", "Ae");
    	replace_all(big, "Ü", "Ue");
    	replace_all(big, "ß", "ss");
    	
    	if( write(fd, big.c_str(), len > 5? 5 : len ) < 0)
		perror("write to vfd failed");
#elif defined (ENABLE_VFD)
#ifdef __sh__
	int len = big.length();
	
	if(len == 0)
		return;

	// replace chars
	replace_all(big, "\x0d", "");
    	replace_all(big, "\n\n", "\\N");
	replace_all(big, "\n", "");
    	replace_all(big, "\\N", "\n");
    	replace_all(big, "ö", "oe");
    	replace_all(big, "ä", "ae");
    	replace_all(big, "ü", "ue");
	replace_all(big, "Ö", "Oe");
    	replace_all(big, "Ä", "Ae");
    	replace_all(big, "Ü", "Ue");
    	replace_all(big, "ß", "ss");
    	
    	openDevice();
	
	if(write(fd , big.c_str(), len > 16? 16 : len ) < 0)
		perror("write to vfd failed");
	
	closeDevice();
#else
	int len = big.length();
	
	if(len == 0)
		return;

	// replace chars
	replace_all(big, "\x0d", "");
    	replace_all(big, "\n\n", "\\N");
	replace_all(big, "\n", "");
    	replace_all(big, "\\N", "\n");
    	replace_all(big, "ö", "oe");
    	replace_all(big, "ä", "ae");
    	replace_all(big, "ü", "ue");
	replace_all(big, "Ö", "Oe");
    	replace_all(big, "Ä", "Ae");
    	replace_all(big, "Ü", "Ue");
    	replace_all(big, "ß", "ss");
    	
	if( write(fd, big.c_str(), len > 12? 12 : len ) < 0)
		perror("write to vfd failed");
#endif	
#elif defined (ENABLE_LCD)
	// clear screen under banner
	display->draw_fill_rect(-1, element[ELEMENT_BANNER].height + 2 - 1, lcd_width, lcd_height - fonts.time->getHeight(), CLCDDisplay::PIXEL_OFF);

	//
	bool big_utf8 = false;
	bool small_utf8 = false;
	std::string cname[2];
	std::string event[4];
	int namelines = 0, eventlines = 0, maxnamelines = 2;
	
//	if (showmode == EPGMODE_CHANNEL_TITLE_LOGO)
//		maxnamelines = 1;

	if ((showmode & CLCD::EPGMODE_CHANNEL) && !big.empty())
	{
		bool dumb = false;
		big_utf8 = isUTF8(big);
		
		while (true)
		{
			namelines = 0;
			std::string title = big;
			
			do { 
				// first try "intelligent" splitting
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

	int maxeventlines = 4 - namelines;
	
//	if (showmode == EPGMODE_CHANNEL_TITLE_LOGO)
//		maxeventlines = 1;

	if ((showmode & CLCD::EPGMODE_TITLE) && !small.empty())
	{
		bool dumb = false;
		small_utf8 = isUTF8(small);

		while (true)
		{
			eventlines = 0;
			std::string title = small;
			do { 
				// first try "intelligent" splitting
				event[eventlines] = splitString(title, lcd_width, fonts.menu, dumb, small_utf8);
				title = removeLeadingSpaces(title.substr(event[eventlines].length()));
				/* DrDish TV appends a 0x0a to the EPG title. We could strip it in sectionsd...
				   ...instead, strip all control characters at the end of the text for now */
				if (event[eventlines].length() > 0 && event[eventlines].at(event[eventlines].length() - 1) < ' ')
					event[eventlines].erase(event[eventlines].length() - 1);
				eventlines++;
			} while (title.length() > 0 && eventlines < maxeventlines);
			
			if (title.length() == 0)
				break;
				
			dumb = !dumb;	// retry with dumb splitting;
			if (!dumb)	// second retry -> get out;
				break;
		}
	}

	//	
	int y = element[ELEMENT_BANNER].height;
	int x = 1;
	
	// namelines
	for (int i = 0; i < namelines; i++) 
	{
		y += fonts.channelname->getHeight();
		
		if (centered)
		{
			int w = fonts.channelname->getRenderWidth(cname[i].c_str(), big_utf8);
			x = (lcd_width - w) / 2;
		}
		
		fonts.channelname->RenderString(x, y, lcd_width + 10, cname[i].c_str(), CLCDDisplay::PIXEL_ON, 0, big_utf8);
	}

	//
	if (eventlines > 0)
	{
		for (int i = 0; i < eventlines; i++) 
		{
			y += fonts.menu->getHeight();
			
			if (centered)
			{
				int w = fonts.menu->getRenderWidth(event[i].c_str(), small_utf8);
				x = (lcd_width - w) / 2;
			}
			
			fonts.menu->RenderString(x, y, lcd_width + 10, event[i].c_str(), CLCDDisplay::PIXEL_ON, 0, small_utf8);
		}
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (g_settings.glcd_enable) nglcd->drawText(0, 0, 0, big.length(), big);
#endif

	if (perform_wakeup)
		wake_up();

	displayUpdate();
}

void CLCD::showText(const char *str)
{
	if (!has_lcd)
		return;
		
	printf("CLCD::showText: %s\n", str? str : "null");
		 
#if defined (ENABLE_4DIGITS)
	int len = strlen(str);
	
	if(len == 0)
		return;
	
	// replace
	std::string text = str;

	// replace chars
	replace_all(text, "\x0d", "");
    	replace_all(text, "\n\n", "\\N");
	replace_all(text, "\n", "");
    	replace_all(text, "\\N", "\n");
    	replace_all(text, "ö", "oe");
    	replace_all(text, "ä", "ae");
    	replace_all(text, "ü", "ue");
	replace_all(text, "Ö", "Oe");
    	replace_all(text, "Ä", "Ae");
    	replace_all(text, "Ü", "Ue");
    	replace_all(text, "ß", "ss");
    	
    	if( write(fd, text.c_str(), len > 5? 5 : len ) < 0)
		perror("write to vfd failed");
#elif defined (ENABLE_VFD)
#ifdef __sh__
	int len = strlen(str);
	
	if(len == 0)
		return;
	
	// replace
	std::string text = str;

	// replace chars
	replace_all(text, "\x0d", "");
    	replace_all(text, "\n\n", "\\N");
	replace_all(text, "\n", "");
    	replace_all(text, "\\N", "\n");
    	replace_all(text, "ö", "oe");
    	replace_all(text, "ä", "ae");
    	replace_all(text, "ü", "ue");
	replace_all(text, "Ö", "Oe");
    	replace_all(text, "Ä", "Ae");
    	replace_all(text, "Ü", "Ue");
    	replace_all(text, "ß", "ss");
    	
    	openDevice();
	
	if(write(fd , text.c_str(), len > 16? 16 : len ) < 0)
		perror("write to vfd failed");
	
	closeDevice();
#else
	int len = strlen(str);
	
	if(len == 0)
		return;
	
	// replace
	std::string text = str;

	// replace chars
	replace_all(text, "\x0d", "");
    	replace_all(text, "\n\n", "\\N");
	replace_all(text, "\n", "");
    	replace_all(text, "\\N", "\n");
    	replace_all(text, "ö", "oe");
    	replace_all(text, "ä", "ae");
    	replace_all(text, "ü", "ue");
	replace_all(text, "Ö", "Oe");
    	replace_all(text, "Ä", "Ae");
    	replace_all(text, "Ü", "Ue");
    	replace_all(text, "ß", "ss");
    	
	if( write(fd, text.c_str(), len > 12? 12 : len ) < 0)
		perror("write to vfd failed");
#endif
#elif defined (ENABLE_LCD)
	showTextScreen(std::string(str), "", EPGMODE_CHANNEL, true, true); // always centered
#endif

#ifdef ENABLE_GRAPHLCD
	if (g_settings.glcd_enable) nglcd->drawText(0, 0, 0, strlen(str), std::string(str));
#endif
}

void CLCD::showServicename(const std::string &name, const bool perform_wakeup, int pos)
{
	if (!has_lcd)
		return;
		
	printf("CLCD::showServicename: name:%s\n", name.empty()? "null" : name.c_str());

	int showmode = g_settings.lcd_epgmode;

	if (!name.empty())
		servicename = name;
		
	servicenumber = pos;

	if (mode != MODE_TVRADIO)
		return;

#ifdef ENABLE_4DIGITS
	if (g_settings.lcd_epgmode == EPGMODE_CHANNELNUMBER)
	{
		char tmp[5];
							
		sprintf(tmp, "%04d", servicenumber); // FIXME:
							
		showText(tmp); // UTF-8
	}
#elif defined (ENABLE_VFD)
	if (g_settings.lcd_epgmode == EPGMODE_CHANNELNUMBER)
	{
		showText((char *)servicename.c_str() );
	}
#elif defined (ENABLE_LCD)
	showTextScreen(servicename, epg_title, showmode, perform_wakeup, g_settings.lcd_epgalign);
	
	// logo
	if (showmode == 7)
		display->load_screen_element(&(element[ELEMENT_PICON]), (lcd_width - element[ELEMENT_PICON].width)/2, lcd_height - fonts.menu->getHeight() - element[ELEMENT_PICON].height);
#endif

#ifdef ENABLE_GRAPHLCD
	if (g_settings.glcd_enable) nglcd->drawText(0, 0, 0, name.length(), servicename);
#endif
	
	if (perform_wakeup)
		wake_up();
}

void CLCD::showEPGTitle(const std::string title)
{
	if (!has_lcd)
		return;

	printf("CLCD::showEPGTitle: %s\n", title.c_str());
	
#if defined (ENABLE_LCD)
	if (title == epg_title)
	{
		return;
	}
	
	epg_title.clear();
	epg_title = title;

	showTextScreen("", epg_title, g_settings.lcd_epgmode, false, g_settings.lcd_epgalign);
#endif
}

void CLCD::showMovieInfo(const PLAYMODES playmode, const std::string big, const std::string small, const bool centered)
{
	printf("CLCD::showMovieInfo: playmode:%d big:%s small:%s centered:%s\n", playmode, big.empty()? "null" : big.c_str(), small.empty()? "null" : small.c_str(), centered? "centered" : "not centered");
	
	int showmode = g_settings.lcd_epgmode;

#if defined (ENABLE_VFD)
	showText((char *)big.c_str());
#elif defined (ENABLE_LCD)
	movie_playmode = playmode;
	movie_big = big;
	movie_small = small;

	if (mode != MODE_MOVIE)
		return;

	showPlayMode(movie_playmode);
	showTextScreen(movie_big, movie_small, showmode, true, centered);
#endif
}

void CLCD::setMovieAudio(const bool is_ac3)
{
	if (mode != MODE_MOVIE)
		return;
		
	movie_is_ac3 = is_ac3;
}

void CLCD::showTime(bool force)
{
	if(!has_lcd) 
		return;

#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	if (showclock) 
	{
		char timestr[21];
		struct timeb tm;
		struct tm * t;
		static int hour = 0, minute = 0;

		ftime(&tm);
		t = localtime(&tm.time);

		if(force || ((hour != t->tm_hour) || (minute != t->tm_min))) 
		{
			hour = t->tm_hour;
			minute = t->tm_min;
				
#if defined (PLATFORM_KATHREIN)	// time and date at kathrein because 16 character vfd
			strftime(timestr, 20, "%H:%M - %d.%m.%y", t);
#elif !defined(PLATFORM_SPARK7162) && !defined (PLATFORM_KATHREIN) // no time at spark7162 because clock integrated
 			strftime(timestr, 20, "%H:%M", t);
#endif				
			showText(timestr);
		}
	}

	if (CNeutrinoApp::getInstance()->recordingstatus) 
	{
		if(clearClock) 
		{
			clearClock = 0;
		} 
		else 
		{
			clearClock = 1;
		}
	} 
	else if(clearClock) 
	{ 
		// in case icon ON after record stopped
		clearClock = 0;
	}	
#elif defined (ENABLE_LCD)
	if (showclock)
	{
		char timestr[21];
		char datestr[21];
		struct timeval tm;
		struct tm * t;
		int i_w = 120;
		int i_h = 12;
		int i_bpp = 0;

		gettimeofday(&tm, NULL);
		t = localtime(&tm.tv_sec);
		
		strftime((char*) &timestr, 20, "%H:%M", t);
		
		//
		if (CNeutrinoApp::getInstance()->recordingstatus)
		{
			if (clearClock == 1)
			{
				strcpy(timestr,"  :  ");
				clearClock = 0;
			}
			else
			{
				clearClock = 1;
			}
		}
		else if(clearClock) 
		{ 
			// in case icon ON after record stopped
			clearClock = 0;
		}	

		if (mode == MODE_STANDBY)
		{
			display->clear_screen(); // clear whole lcd

			// refresh
			display->draw_fill_rect((lcd_width - 1 - fonts.timestandby->getRenderWidth(timestr))/2, (lcd_height - fonts.timestandby->getHeight())/2, fonts.time->getRenderWidth("00:00:00") + 1, lcd_height, CLCDDisplay::PIXEL_OFF);

			// time
			fonts.timestandby->RenderString((lcd_width - 1 - fonts.timestandby->getRenderWidth(timestr))/2, lcd_height/2, fonts.timestandby->getRenderWidth("00:00:00"), timestr, CLCDDisplay::PIXEL_ON);
			
			// date
			strftime((char*) &datestr, 20, "%d.%m.%Y", t);
			
			fonts.menu->RenderString((lcd_width - 1 - fonts.menu->getRenderWidth(datestr))/2, lcd_height - fonts.menu->getHeight() - 1, fonts.menu->getRenderWidth("00:00:0000:0", true), datestr, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			// refresh
			display->draw_fill_rect(lcd_width - 1 - fonts.time->getRenderWidth("00:00", true), lcd_height - fonts.time->getHeight() - 1, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);

			// time
			fonts.time->RenderString(lcd_width - fonts.time->getRenderWidth("00:00", true), lcd_height - 1, fonts.menu->getRenderWidth("00:00:0", true), timestr, CLCDDisplay::PIXEL_ON);
		}
		
		displayUpdate();
	}
	
	////test
	printf("CLCD::showTime: showclock %d clearClock %d\n", showclock, clearClock);
#endif

#ifdef ENABLE_GRAPHLCD
	if (nglcdshowclock)
	{
		char timestr[21];
		struct timeval tm;
		struct tm * t;

		gettimeofday(&tm, NULL);
		t = localtime(&tm.tv_sec);
		
		strftime((char*) &timestr, 20, "%H:%M", t);
		
		//
		if (CNeutrinoApp::getInstance()->recordingstatus)
		{
			if (nglcdclearClock == 1)
			{
				strcpy(timestr,"  :  ");
				nglcdclearClock = 0;
			}
			else
			{
				nglcdclearClock = 1;
			}
		}
		else if(nglcdclearClock) 
		{ 
			// in case icon ON after record stopped
			nglcdclearClock = 0;
		}	

		if (mode == MODE_STANDBY)
		{
			std::string a_clock = DATADIR "/lcd/a_clock.png";
			
			if (file_exists(a_clock.c_str()))
			{
				int lcd_a_clock_width = 0, lcd_a_clock_height = 0, lcd_a_clock_bpp = 0;
				getSize(a_clock.c_str(), &lcd_a_clock_width, &lcd_a_clock_height, &lcd_a_clock_bpp);
			
				if (g_settings.glcd_enable) nglcd->LcdAnalogClock(lcd_a_clock_width / 2, lcd_a_clock_height / 2, 200);
			}
		}
		
		displayUpdate();
	}
#endif
}

void CLCD::showRCLock(int duration)
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
	uint8_t * curr_screen = new uint8_t[display->raw_buffer_size];

	// Saving the whole screen is not really nice since the clock is updated
	// every second. Restoring the screen can cause a short travel to the past ;)
	display->dump_screen(&curr_screen);
	
	//
	display->draw_fill_rect (-1, 10, lcd_width, lcd_height - 12 - 2, CLCDDisplay::PIXEL_OFF);
	display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height - element[ELEMENT_SPEAKER].height);
	wake_up();
	displayUpdate();
	sleep(duration);
	
	//
	display->load_screen(&curr_screen);
	wake_up();
	displayUpdate();
	delete [] curr_screen;
#endif
}

void CLCD::showVolume(const char vol, const bool perform_update)
{
	if(!has_lcd) 
		return;
	
	volume = vol;
	
#ifdef ENABLE_LCD
	if ( (mode == MODE_TVRADIO || mode == MODE_MOVIE || mode == MODE_SCART || mode == MODE_AUDIO) && (g_settings.lcd_statusline == STATUSLINE_VOLUME) )
	{
		unsigned int height =  6;
		unsigned int left   = 12 + 2;
		unsigned int top    = lcd_height - height - 1 - 2;
		unsigned int width  = lcd_width - left - 4 - fonts.time->getRenderWidth("00:00") - 1;
		
		//
		if ((g_RemoteControl != NULL && mode == MODE_TVRADIO) || mode == MODE_MOVIE)
		{
			if (mode == MODE_MOVIE)
				icon_dolby = movie_is_ac3;
			else
			{
				uint count = g_RemoteControl->current_PIDs.APIDs.size();
					
				if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < count) &&
					    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
					icon_dolby = true;
				else
					icon_dolby = false;
			}
		}

		// icon
		if ((muted) || (volume == 0))
			display->load_screen_element(&(element[ELEMENT_MUTE]), 0, lcd_height-element[ELEMENT_MUTE].height);
		else
		{
			if (icon_dolby)
				display->load_screen_element(&(element[ELEMENT_DOLBY]), 0, lcd_height-element[ELEMENT_DOLBY].height);
			else
				display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].height);
		}

		//strichline
		if ((muted) || (volume == 0))
		{
			display->draw_line(left, top, left + width, top + height - 1, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp = vol*(width + 1)/100;
			display->draw_fill_rect(left - 1, top - 1, left + dp, top + height, CLCDDisplay::PIXEL_ON);
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
			display->draw_fill_rect (-1, top-2         , 10, lcd_height-3  , CLCDDisplay::PIXEL_OFF);
			display->draw_rectangle ( 1, top+2         ,  3, lcd_height-3-3, CLCDDisplay::PIXEL_ON, CLCDDisplay::PIXEL_OFF);
			display->draw_line      ( 3, top+2         ,  6, top-1         , CLCDDisplay::PIXEL_ON);
			display->draw_line      ( 3, lcd_height-3-3,  6, lcd_height-3  , CLCDDisplay::PIXEL_ON);
			display->draw_line      ( 6, top+1         ,  6, lcd_height-3-2, CLCDDisplay::PIXEL_ON);
		}

		if (perform_update)
			displayUpdate();
	}
#endif

	wake_up();
}

void CLCD::showPercentOver(const unsigned char perc, const bool perform_update, const MODES m)
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD)
	if (mode != m)
		return;

	int left, top, width, height = 6;
	bool draw = true;
	percentOver = perc;
	
	if ( (mode == MODE_TVRADIO || mode == MODE_MOVIE || mode == MODE_AUDIO) && (g_settings.lcd_statusline == STATUSLINE_PLAYTIME) )
	{
		left = 12 + 2; 
		top = lcd_height - height - 1 - 2; 
		width = lcd_width - left - 4 - fonts.time->getRenderWidth("00:00");

		// refresh
		display->draw_rectangle(left - 2, top - 2, left + width + 2, top + height + 1, CLCDDisplay::PIXEL_ON, CLCDDisplay::PIXEL_OFF);

		if (perc == (unsigned char) -1)
		{
			display->draw_line(left, top, left + width, top + height - 1, CLCDDisplay::PIXEL_ON);
		}
		else
		{
			int dp;
			if (perc == (unsigned char) - 2)
				dp = width + 1;
			else
				dp = perc * (width + 1) / 100;
					
			display->draw_fill_rect(left - 1, top - 1, left + dp, top + height, CLCDDisplay::PIXEL_ON);

			if (perc == (unsigned char) - 2)
			{
				// draw a "+" to show that the event is overdue
				display->draw_line(left + width - 2, top + 1, left + width - 2, top + height - 2, CLCDDisplay::PIXEL_OFF);
				display->draw_line(left + width - 1, top + (height/2), left + width - 3, top + (height/2), CLCDDisplay::PIXEL_OFF);
			}
		}

		if (perform_update)
			displayUpdate();
	}
#endif
}

void CLCD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if(!has_lcd) 
		return;
		
	printf("CLCD::showMenuText: position:%d text:%s highlight:%d\n", position, text? text : "null", highlight);
	
	if (mode != MODE_MENU_UTF8)
		return;
	
#if defined (ENABLE_VFD)						
	showText(text); // UTF-8
#elif defined (ENABLE_LCD)
	// refresh
	display->draw_fill_rect(-1, element[ELEMENT_BANNER].height + 2 + fonts.menutitle->getHeight() + 2 - 1, lcd_width, element[ELEMENT_BANNER].height + 2 + fonts.menutitle->getHeight() + 2 + fonts.menu->getHeight() + 10, CLCDDisplay::PIXEL_OFF);
	
	// render text
	fonts.menu->RenderString(0, element[ELEMENT_BANNER].height + 2 + fonts.menutitle->getHeight() + 2 + fonts.menu->getHeight(), lcd_width + 10, text, CLCDDisplay::PIXEL_ON, highlight, utf_encoded);
#endif

#ifdef ENABLE_GRAPHLCD	
	if (g_settings.glcd_enable) nglcd->drawText(0, 0, 0, strlen(text), std::string(text));
#endif

	wake_up();
	displayUpdate();
}

void CLCD::showAudioTrack(const std::string &artist, const std::string &title, const std::string &album, int pos)
{
	if(!has_lcd) 
		return;
		
	printf("CLCD::showAudioTrack: artist:%s title:%s album:%s pos:%d\n", artist.empty()? "null" : artist.c_str(), title.empty()? "null" : title.c_str(), album.empty()? "null" : album.c_str(), pos);
	
	if (mode != MODE_AUDIO) 
		return;

#if defined (ENABLE_VFD)
	showText((char *)title.c_str());
#elif defined (ENABLE_LCD)
	// refresh
	display->draw_fill_rect(-1, element[ELEMENT_BANNER].height + 2 - 1, lcd_width, element[ELEMENT_BANNER].height + 2 + fonts.channelname->getHeight() + 2 + fonts.menu->getHeight() + 2 + fonts.menu->getHeight(), CLCDDisplay::PIXEL_OFF);
	
	// title
	fonts.channelname->RenderString((lcd_width - fonts.channelname->getRenderWidth(title.c_str(), isUTF8(title)))/2, element[ELEMENT_BANNER].height + 2 + fonts.channelname->getHeight(), lcd_width + 10, title.c_str(), CLCDDisplay::PIXEL_ON, 0, isUTF8(title));
	
	// artist
	fonts.menu->RenderString((lcd_width - fonts.menu->getRenderWidth(artist.c_str(), isUTF8(artist)))/2, element[ELEMENT_BANNER].height + 2 + fonts.channelname->getHeight() + 2 + fonts.menu->getHeight(), lcd_width + 10, artist.c_str(),  CLCDDisplay::PIXEL_ON, 0, isUTF8(artist));
	
	// album
	fonts.menu->RenderString((lcd_width - fonts.menu->getRenderWidth(album.c_str(), isUTF8(album)))/2, element[ELEMENT_BANNER].height + 2 + fonts.channelname->getHeight() + 2 + 2*fonts.menu->getHeight() + 2*2, lcd_width + 10, album.c_str(),  CLCDDisplay::PIXEL_ON, 0, isUTF8(album));
#endif

	wake_up();
	displayUpdate();
}

void CLCD::showPlayMode(PLAYMODES m)
{
	if(!has_lcd) 
		return;

#if defined (ENABLE_VFD)
#ifdef __sh__
	switch(m) 
	{
		case PLAY_MODE_PLAY:
			ShowIcon(VFD_ICON_PLAY, true);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case PLAY_MODE_STOP:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case PLAY_MODE_PAUSE:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, true);
			break;
			
		case PLAY_MODE_FF:
			break;
			
		case PLAY_MODE_REV:
			break;
	}
#endif	
#elif defined (ENABLE_LCD)
	// refresh
	display->draw_fill_rect (-1, lcd_width - 12, 12, 12, CLCDDisplay::PIXEL_OFF);
	
	switch(m)
	{
		case PLAY_MODE_PLAY:
			{
				display->load_screen_element(&(element[ELEMENT_PLAY]), 0, lcd_height - element[ELEMENT_PROG].height);
				break;
			}
			
		case PLAY_MODE_STOP:
			display->draw_fill_rect (-1, lcd_height - 12, 12, 12, CLCDDisplay::PIXEL_ON);
			break;
			
		case PLAY_MODE_PAUSE:
			display->load_screen_element(&(element[ELEMENT_PAUSE]), 0, lcd_height - element[ELEMENT_PROG].height);
			break;
			
		case PLAY_MODE_FF:
			{
				int x = 2, y = lcd_height - 12;
				display->draw_line(x   ,y   , x  , y+4, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+2 ,y+2 , x+2, y+2, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+3 ,y   , x+3, y+4, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+5 ,y+2 , x+5, y+2, CLCDDisplay::PIXEL_ON);
			}
			break;
			
		case PLAY_MODE_REV:
			{
				int x = 2, y = lcd_height - 12;
				display->draw_line(x   ,y+2 , x  , y+2, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+1 ,y+1 , x+1, y+3, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+2 ,y   , x+2, y+4, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+3 ,y+2 , x+3, y+2, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+4 ,y+1 , x+4, y+3, CLCDDisplay::PIXEL_ON);
				display->draw_line(x+5 ,y   , x+5, y+4, CLCDDisplay::PIXEL_ON);
			}
			break;
	}
#endif

	wake_up();
	displayUpdate();
}

void CLCD::drawBanner()
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD)	
	display->load_screen_element(&(element[ELEMENT_BANNER]), 0, 0, lcd_width, element->height);
	
	if (element[ELEMENT_BANNER].width < lcd_width)
		display->draw_fill_rect(element[ELEMENT_BANNER].width - 1, -1, lcd_width, element[ELEMENT_BANNER].height - 1, CLCDDisplay::PIXEL_ON);
#endif
}

void CLCD::setMode(const MODES m, const char * const title)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::setMode: %d\n", m);
		
	mode = m;
	menutitle = title;
	
	setlcdparameter();

#if defined (ENABLE_4DIGITS)
	switch (m) 
	{
		case MODE_TVRADIO:
			showclock = false;
			if (g_settings.lcd_epgmode == EPGMODE_CHANNELNUMBER)
				showServicename(servicename, true, servicenumber);
			else if (g_settings.lcd_epgmode == EPGMODE_TIME)
			{
				showclock = true;
				showTime(true);
			}
			break;

		case MODE_AUDIO:
			showclock = true;
			showTime(true);
			break;
			
		case MODE_SCART:
			showclock = true;
			showTime(true);
			break;
			
		case MODE_MENU_UTF8:
			showclock = true;
			showTime(true);
			break;

		case MODE_SHUTDOWN:
			showclock = false;
			
			//
			showText((char *) "BYE");
			
			break;

		case MODE_STANDBY:				
			showclock = true;
			showTime(true);
			break;
		
		case MODE_PIC:	  
			showclock = true;
			showTime(true);
			break;
			
		case MODE_MOVIE:  		
			showclock = true;
			showTime(true);
			break;
	}

#elif defined (ENABLE_VFD)
#ifdef __sh__		
	switch (m) 
	{
		case MODE_TVRADIO:
			showclock = false;
			if (g_settings.lcd_epgmode == EPGMODE_CHANNELNUMBER)	
				showServicename(servicename, true, servicenumber);
			else if (g_settings.lcd_epgmode == EPGMODE_TIME)
			{
				showclock = true;
				showTime(true);
			}
			
#if !defined(PLATFORM_SPARK7162)			
			ShowIcon(VFD_ICON_MP3, false);	        // NOTE: @dbo  //ICON_MP3 and ICON_DOLBY switched in infoviewer 
#endif			
	
#if defined (PLATFORM_KATHREIN)
			ShowIcon(VFD_ICON_USB, usb_icon);	
			ShowIcon(VFD_ICON_HDD, hdd_icon);	
#elif defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_USB, usb_icon);	
			ShowDiskLevel();
			ShowIcon(VFD_ICON_STANDBY, false);	
#endif
			break;

		case MODE_AUDIO:
		{
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_AC3, false);			
#endif		  
			ShowIcon(VFD_ICON_MP3, true);			
			ShowIcon(VFD_ICON_TV, false);								
			ShowIcon(VFD_ICON_LOCK, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			showPlayMode(PLAY_MODE_STOP);
			showclock = false;
			break;
		}

		case MODE_SCART:	  
			ShowIcon(VFD_ICON_TV, false);	
			showclock = true;
			showTime(true);
			break;

		case MODE_MENU_UTF8:
			ShowIcon(VFD_ICON_TV, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			showclock = false;
			break;

		case MODE_SHUTDOWN:
			ClearIcons();
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_CLOCK, timer_icon);	
#endif			
			showclock = false;
			
			//
			showText((char *) "BYE");
			
			break;

		case MODE_STANDBY:
			ShowIcon(VFD_ICON_TV, false);
			ClearIcons();
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_STANDBY, true);	
#endif				
			showclock = true;
			showTime(true);
			break;
		
		case MODE_PIC:	  
			ShowIcon(VFD_ICON_TV, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			showclock = true;
			showTime(true);
			break;
			
		case MODE_MOVIE:  
			ShowIcon(VFD_ICON_TV, false);			
			showclock = false;
			break;
	}

#endif // vfd	
#elif defined (ENABLE_LCD)
	switch (m)
	{
	case MODE_TVRADIO:
	case MODE_MOVIE:
	{
		// clear lcd
		display->clear_screen();
		
		// statusline
		switch (g_settings.lcd_statusline)
		{
			case CLCD::STATUSLINE_PLAYTIME:
				drawBanner();
				display->load_screen_element(&(element[ELEMENT_PROG]), 0, lcd_height -element[ELEMENT_PROG].height);
				showPercentOver(percentOver, false, mode);
				break;
				
			case CLCD::STATUSLINE_VOLUME:
				drawBanner();
				showVolume(volume, false);
				break;
			default:
				break;
		}
		
		// servicename / title / epg
		if (mode == MODE_TVRADIO)
			showServicename(servicename, true, servicenumber);
		else // MODE_MOVIE
		{
			showMovieInfo(movie_playmode, movie_big, movie_small, g_settings.lcd_epgalign);
			setMovieAudio(movie_is_ac3);
		}
		
		// time
		showclock = true;
		showTime();
		break;
	}
		
	case MODE_AUDIO:
	{
		display->clear_screen();
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height - element[ELEMENT_SPEAKER].height - 1);
		showPlayMode(PLAY_MODE_STOP);
		showVolume(volume, false);
		showclock = true;
		showTime();
		break;
	}
	
	case MODE_PIC:
	{
		display->clear_screen();
		drawBanner();
		showclock = false;
		fonts.menutitle->RenderString(0, element[ELEMENT_BANNER].height + 2 + fonts.menutitle->getHeight(), lcd_width, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		displayUpdate();
		break;
	}
	
	case MODE_SCART:
	{
		display->clear_screen();
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].width)/2, 12);
		display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].height-1);

		showVolume(volume, false);
		showclock = true;
		showTime();
		break;
	}
		
	case MODE_MENU_UTF8:
	{
		showclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		fonts.menutitle->RenderString(0, element[ELEMENT_BANNER].height + 2 + fonts.menutitle->getHeight(), lcd_width, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		displayUpdate();
		break;
	}
		
	case MODE_SHUTDOWN:
	{
		showclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_POWER]), (lcd_width - element[ELEMENT_POWER].width)/2, (lcd_height - element[ELEMENT_POWER].height)/2);
		displayUpdate();
		break;
	}
		
	case MODE_STANDBY:
	{
		showclock = true;
		showTime();
		break;
	}

	////???
	case MODE_FILEBROWSER:
		showclock = true;
		display->clear_screen();
		showFilelist();
		break;
		
	case MODE_PROGRESSBAR:
		showclock = false;
		display->clear_screen();
		drawBanner();
		showProgressBar();
		break;
		
	case MODE_PROGRESSBAR2:
		showclock = false;
		display->clear_screen();
		drawBanner();
		showProgressBar2();
		break;
		
	case MODE_INFOBOX:
		showclock = false;
		showInfoBox();
		break;
	}
#endif

#ifdef ENABLE_GRAPHLCD
	switch (m)
	{
	case MODE_TVRADIO:
	case MODE_MOVIE:
		if (g_settings.glcd_enable) nglcd->clear();
		
		// servicename / title / epg
		if (mode == MODE_TVRADIO)
			showServicename(servicename, true, servicenumber);
		else // MODE_MOVIE
		{
			showMovieInfo(movie_playmode, movie_big, movie_small, g_settings.lcd_epgalign);
			setMovieAudio(movie_is_ac3);
		}
		
		// time
		nglcdshowclock = true;
		showTime();
		break;
	
	#if 0	
	case MODE_AUDIO:
	{
		display->clear_screen(); // clear lcd
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].height-1);
		showPlayMode(PLAY_MODE_STOP);
		showVolume(volume, false);
		nglcdshowclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
	}
	
	case MODE_SCART:
		display->clear_screen(); // clear lcd
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].width)/2, 12);
		display->load_screen_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].height-1);

		showVolume(volume, false);
		nglcdshowclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		break;
		
	case MODE_MENU_UTF8:
		nglcdshowclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		fonts.menutitle->RenderString(0, 28, lcd_width + 20, title, CLCDDisplay::PIXEL_ON, 0, true); // UTF-8
		displayUpdate();
		break;
		
	case MODE_SHUTDOWN:
		nglcdshowclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		display->load_screen_element(&(element[ELEMENT_POWER]), (lcd_width-element[ELEMENT_POWER].width)/2, 12);
		displayUpdate();
		break;
	#endif
		
	case MODE_STANDBY:
		nglcdshowclock = true;
		showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
		                 /* "showTime()" clears the whole lcd in MODE_STANDBY                         */
		break;
	#if 0
	////???
	case MODE_FILEBROWSER:
		nglcdshowclock = true;
		display->clear_screen(); // clear lcd
		showFilelist();
		break;
		
	case MODE_PROGRESSBAR:
		nglcdshowclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		showProgressBar();
		break;
		
	case MODE_PROGRESSBAR2:
		nglcdshowclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		showProgressBar2();
		break;
		
	case MODE_INFOBOX:
		nglcdshowclock = false;
		showInfoBox();
		break;
	#endif
	}
#endif
	
	////test
	printf("CLCD::setMode: mode %d showclock %d\n", mode, showclock);
	
	wake_up();
}


void CLCD::setBrightness(int bright)
{
	g_settings.lcd_brightness = bright;
	setlcdparameter();
}

int CLCD::getBrightness()
{
	return g_settings.lcd_brightness;
}

void CLCD::setBrightnessStandby(int bright)
{
	g_settings.lcd_standbybrightness = bright;
	setlcdparameter();
}

int CLCD::getBrightnessStandby()
{
	return g_settings.lcd_standbybrightness;
}

void CLCD::setContrast(int contrast)
{
	g_settings.lcd_contrast = contrast;
	setlcdparameter();
}

int CLCD::getContrast()
{
	return g_settings.lcd_contrast;
}

void CLCD::setPower(int power)
{
	dprintf(DEBUG_NORMAL, "CLCD::setPower\n");
	
	if (!has_lcd)
		return;
		
	g_settings.lcd_power = power;

#if defined (ENABLE_VFD)	
#if defined (__sh__)
	struct vfd_ioctl_data data;
	data.start_address = power;
	
	openDevice();
	
	if( ioctl(fd, VFDPWRLED, &data) < 0)  
		perror("VFDPWRLED");
	
	closeDevice();
#endif
#endif
}

int CLCD::getPower()
{
	return g_settings.lcd_power;
}

void CLCD::togglePower(void)
{
	last_toggle_state_power = 1 - last_toggle_state_power;
	
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_standbybrightness : g_settings.lcd_brightness,
			g_settings.lcd_contrast,
			last_toggle_state_power,
			g_settings.lcd_inverse,
			0);
}

void CLCD::setInverse(int inverse)
{
	g_settings.lcd_inverse = inverse;
	
	setlcdparameter();
}

int CLCD::getInverse()
{
	return g_settings.lcd_inverse;
}

void CLCD::setMuted(bool mu)
{
	muted = mu;
	showVolume(volume);
}

void CLCD::setLED(int value, int option)
{
	if (!has_lcd)
		return;
		
	printf("CLCD::setLED: %d\n", value);

	g_settings.lcd_led = value;
	
#if defined (ENABLE_LCD)
	display->setLED(value, option);
#endif

#if defined (PLATFORM_GIGABLUE)
	const char *LED[] = 
	{
		"LEDCOLOR_OFF",
		"LEDCOLOR_BLUE",
		"LEDCOLOR_RED",
		"LEDCOLOR_PURPLE"
	};
	
	dprintf(DEBUG_NORMAL, "CLCD::setLED: %s\n", LED[value]);
	  
	FILE * f;
	if((f = fopen("/proc/stb/fp/led0_pattern", "w")) == NULL) 
		return;
	
	fprintf(f, "%d", value);
	
	fclose(f);
#endif
}

void CLCD::setMiniTV(int value)
{
	if (!has_lcd)
		return;
		
	g_settings.lcd_minitv = value;
	
#if defined (ENABLE_LCD)
	const char *LCDMINITV[] = 
	{
		"NORMAL",
		"MINITV",
		"OSD",
		"OSD / MINITV"
	};
	
	dprintf(DEBUG_NORMAL, "CLCD::setMiniTV: %s\n", LCDMINITV[value]);
/* 
	FILE * f;
	if((f = fopen("/proc/stb/lcd/mode", "w")) == NULL) 
		return;
	
	fprintf(f, "%d", val);
	
	fclose(f);
*/
	proc_put("/proc/stb/lcd/mode", value);
#endif
}

void CLCD::resume()
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
	display->resume();
#endif
}

void CLCD::pause()
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
	display->pause();
#endif
}

void CLCD::ShowIcon(vfd_icon icon, bool show)
{
	if (!has_lcd)
		return;

#if defined (ENABLE_VFD)
#if defined (__sh__)
#if defined (PLATFORM_KATHREIN) || defined(PLATFORM_SPARK7162)
	switch (icon)
	{
		case VFD_ICON_USB:
			usb_icon = show;
			break;
		case VFD_ICON_CLOCK:
			timer_icon = show;
			break;
#if defined (PLATFORM_KATHREIN)
		case VFD_ICON_HDD:
			hdd_icon = show;
			break;
#endif
		default:
			break;
	}
#endif // kathrein || spark7162

	openDevice();
	
#if defined(PLATFORM_SPARK7162)
	aotom_data.u.icon.icon_nr = icon;
	aotom_data.u.icon.on = show ? 1 : 0;
	
	if (ioctl(fd, VFDICONDISPLAYONOFF, &aotom_data) <0)
		perror("VFDICONDISPLAYONOFF");	
#else
#if defined (PLATFORM_KATHREIN)
	if (icon == 17)				/* returning because not existing icon at ufs910 */
	{
		closeDevice();	
		return;
	}
#endif	
	struct vfd_ioctl_data data;

	data.data[0] = (icon - 1) & 0x1F;
	data.data[4] = show ? 1 : 0;
	
	if( ioctl(fd, VFDICONDISPLAYONOFF, &data) < 0)
		perror("VFDICONDISPLAYONOFF");
#endif	
	closeDevice();
#endif // sh
#endif // vfd
}

////
void CLCD::ShowDiskLevel()
{
#ifdef ENABLE_VFD
#ifdef __sh__
#if defined(PLATFORM_SPARK7162)
	int hdd_icons[9] = {24, 23, 21, 20, 19, 18, 17, 16, 22};
	int percent, digits, i, j;
	uint64_t t, u;
	
	if (get_fs_usage(g_settings.network_nfs_recordingdir, t, u))
	{
		ShowIcon(SPARK_HDD, true);
		percent = (u * 1000ULL) / t + 60; 
		digits = percent / 125;
		if (percent > 1050)
			digits = 9;
		
		if (digits > 0)
		{
			for (i = 0; i < digits; i++)
				ShowIcon((vfd_icon)hdd_icons[i], true);
						
			for (j = i; j < 9; j++)
				ShowIcon((vfd_icon)hdd_icons[j], false);
		}
	}
	else
	{
		ShowIcon(SPARK_HDD, false);

	}
#endif
#endif
#endif
}

void CLCD::ClearIcons()
{
	if(!has_lcd) 
		return;

#ifdef ENABLE_VFD
#ifdef __sh__	
#if defined(PLATFORM_SPARK7162)	 
	openDevice();
	aotom_data.u.icon.icon_nr = SPARK_ICON_ALL;
	aotom_data.u.icon.on = 0;
	if (ioctl(fd, VFDICONDISPLAYONOFF, &aotom_data) <0)
		perror("VFDICONDISPLAYONOFF");
	closeDevice();
#else
	int i;
	struct vfd_ioctl_data data;
	
	openDevice();
	
	for(i = 0; i <= 15; i++)
	{
		data.data[0] = i;
		data.data[4] = 0;
		
		if( ioctl(fd, VFDICONDISPLAYONOFF, &data) < 0)
			perror("VFDICONDISPLAYONOFF");
	}
	
	closeDevice();
#endif
#endif
#endif
}

void CLCD::Lock()
{
	if(!has_lcd) 
		return;
	
	creat("/tmp/lcd.locked", 0);
}

void CLCD::Unlock()
{
	if(!has_lcd) 
		return;
	
	unlink("/tmp/lcd.locked");
}

void CLCD::Clear()
{
	if(!has_lcd) 
		return;

#if defined (ENABLE_4DIGITS)
	showText("     "); // 5 empty digits
#elif defined (ENABLE_VFD)
#if defined (__sh__)
	struct vfd_ioctl_data data;
	
#if defined (PLATFORM_KATHREIN)	/* using this otherwise VFD of ufs910 is black and Neutrino has a segfault */
	data.start_address = 0x01;
	data.length = 0x0;
	openDevice();	
	if (ioctl(fd, VFDDISPLAYCLR, &data) <0)
		perror("VFDDISPLAYCLR");
	closeDevice();
#else
	data.start_address = 0;
	openDevice();	
	if( ioctl(fd, VFDDISPLAYWRITEONOFF, &data) < 0)
		perror("VFDDISPLAYCLR");
	closeDevice();
#endif
#else
	showText("            "); // 12 empty digits
#endif // sh	
#elif defined (ENABLE_LCD)
	if (mode == MODE_SHUTDOWN)
	{
		display->clear_screen(); // clear lcd
		displayUpdate();
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (g_settings.glcd_enable) nglcd->clear();
#endif
}

bool CLCD::ShowPng(char *filename)
{
	if(!has_lcd) 
		return false;
	
#ifdef ENABLE_LCD
	return display->load_png(filename);
#endif
}

bool CLCD::DumpPng(char *filename)
{
	if(!has_lcd) 
		return false;
	
#ifdef ENABLE_LCD
	return display->dump_png(filename);
#endif
}

// showInfoBox
#define EPG_INFO_FONT_HEIGHT 	9
#define EPG_INFO_SHADOW_WIDTH 	1
#define EPG_INFO_LINE_WIDTH 	1
#define EPG_INFO_BORDER_WIDTH 	2

#define EPG_INFO_WINDOW_POS 	4
#define EPG_INFO_LINE_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH
#define EPG_INFO_BORDER_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH
#define EPG_INFO_TEXT_POS 	EPG_INFO_WINDOW_POS + EPG_INFO_SHADOW_WIDTH + EPG_INFO_LINE_WIDTH + EPG_INFO_BORDER_WIDTH

#define EPG_INFO_TEXT_WIDTH 	lcd_width - (2*EPG_INFO_WINDOW_POS)

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

#ifdef ENABLE_LCD
	if( mode == MODE_INFOBOX && !m_infoBoxText.empty())
	{		
		// paint empty box
		display->draw_fill_rect (EPG_INFO_WINDOW_POS, EPG_INFO_WINDOW_POS, 	lcd_width-EPG_INFO_WINDOW_POS+1, 	  lcd_height-EPG_INFO_WINDOW_POS+1,    CLCDDisplay::PIXEL_OFF);
		display->draw_fill_rect (EPG_INFO_LINE_POS, 	 EPG_INFO_LINE_POS, 	lcd_width-EPG_INFO_LINE_POS-1, 	  lcd_height-EPG_INFO_LINE_POS-1, 	 CLCDDisplay::PIXEL_ON);
		display->draw_fill_rect (EPG_INFO_BORDER_POS, EPG_INFO_BORDER_POS, 	lcd_width-EPG_INFO_BORDER_POS-3,  lcd_height-EPG_INFO_BORDER_POS-3, CLCDDisplay::PIXEL_OFF);

		// paint title
		if(!m_infoBoxTitle.empty())
		{
			int width = fonts.menu->getRenderWidth(m_infoBoxTitle.c_str(),true);
			
			if(width > lcd_width - 20)
				width = lcd_width - 20;
			int start_pos = (lcd_width - width) /2;

			display->draw_fill_rect (start_pos, EPG_INFO_WINDOW_POS-4, 	start_pos+width+5, 	  EPG_INFO_WINDOW_POS+10,    CLCDDisplay::PIXEL_OFF);
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
#endif
}

//showFilelist
#define BAR_POS_X 		114
#define BAR_POS_Y 		10
#define BAR_POS_WIDTH 	  	6
#define BAR_POS_HEIGTH 	 	40

void CLCD::showFilelist(int flist_pos,CFileList* flist,const char * const mainDir)
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
	if(flist != NULL)
		m_fileList = flist;
	if(flist_pos != -1)
		m_fileListPos = flist_pos;
	if(mainDir != NULL)
		m_fileListHeader = mainDir;
		
	if (mode == MODE_FILEBROWSER && m_fileList != NULL && m_fileList->size() > 0)
	{    
		dprintf(DEBUG_NORMAL, "CLCD::showFilelist: FileList:OK\n");
		int size = m_fileList->size();
		
		display->draw_fill_rect(-1, -1, lcd_width, lcd_height-12, CLCDDisplay::PIXEL_OFF); // clear lcd
		
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
		
		display->draw_fill_rect (BAR_POS_X,   BAR_POS_Y,   BAR_POS_X+BAR_POS_WIDTH,   BAR_POS_Y+BAR_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display->draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+BAR_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		display->draw_fill_rect (BAR_POS_X+1, BAR_POS_Y+1+marker_offset, BAR_POS_X+BAR_POS_WIDTH-1, BAR_POS_Y+1+marker_offset+marker_length, CLCDDisplay::PIXEL_ON);
	
		displayUpdate();
	}
#endif
}

//showProgressBar
#define PROG_GLOB_POS_X 	10
#define PROG_GLOB_POS_Y 	30
#define PROG_GLOB_POS_WIDTH 	100
#define PROG_GLOB_POS_HEIGTH 	20
void CLCD::showProgressBar(int global, const char * const text,int show_escape,int timer)
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
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
		//printf("[lcdd] prog:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		
		// Clear Display
		display->draw_fill_rect (0, 12, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);
	
		// paint progress header 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, 12+12, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display->draw_fill_rect (PROG_GLOB_POS_X,   				 PROG_GLOB_POS_Y,   PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH,   PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display->draw_fill_rect (PROG_GLOB_POS_X+1+marker_length, PROG_GLOB_POS_Y+1, PROG_GLOB_POS_X+PROG_GLOB_POS_WIDTH-1, PROG_GLOB_POS_Y+PROG_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(lcd_width-40, lcd_height, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		
		displayUpdate();
	}
#endif
}

// showProgressBar2
#define PROG2_GLOB_POS_X 	10
#define PROG2_GLOB_POS_Y 	37
#define PROG2_GLOB_POS_WIDTH 	100
#define PROG2_GLOB_POS_HEIGTH 	10

#define PROG2_LOCAL_POS_X 	10
#define PROG2_LOCAL_POS_Y 	24
#define PROG2_LOCAL_POS_WIDTH 	PROG2_GLOB_POS_WIDTH
#define PROG2_LOCAL_POS_HEIGTH 	PROG2_GLOB_POS_HEIGTH

void CLCD::showProgressBar2(int local,const char * const text_local ,int global ,const char * const text_global ,int show_escape )
{
	if(!has_lcd) 
		return;
	
#ifdef ENABLE_LCD
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
		//printf("[lcdd] prog2:%s,%d,%d\n",m_progressHeaderGlobal.c_str(),m_progressGlobal,m_progressShowEscape);
		
		// Clear Display
		display->draw_fill_rect (0, 12, lcd_width, lcd_height, CLCDDisplay::PIXEL_OFF);
	
		// paint  global caption 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(),true);
		if(width > 100)
			width = 100;
		int start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, PROG2_GLOB_POS_Y+20, width+10, m_progressHeaderGlobal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
	
		// paint global bar 
		int marker_length = (PROG2_GLOB_POS_WIDTH * m_progressGlobal)/100;
		
		display->draw_fill_rect (PROG2_GLOB_POS_X,   				PROG2_GLOB_POS_Y,   PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH,   PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display->draw_fill_rect (PROG2_GLOB_POS_X+1+marker_length, PROG2_GLOB_POS_Y+1, PROG2_GLOB_POS_X+PROG2_GLOB_POS_WIDTH-1, PROG2_GLOB_POS_Y+PROG2_GLOB_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
	
		
		// paint  local caption 
		width = fonts.menu->getRenderWidth(m_progressHeaderLocal.c_str(),true);
		if(width > 100)
			width = 100;
		start_pos = (lcd_width - width) /2;
		fonts.menu->RenderString(start_pos, PROG2_LOCAL_POS_Y -3, width+10, m_progressHeaderLocal.c_str(), CLCDDisplay::PIXEL_ON,0,true);
		// paint local bar 
		marker_length = (PROG2_LOCAL_POS_WIDTH * m_progressLocal)/100;
		
		display->draw_fill_rect (PROG2_LOCAL_POS_X,   				PROG2_LOCAL_POS_Y,   PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH,   PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH,   CLCDDisplay::PIXEL_ON);
		display->draw_fill_rect (PROG2_LOCAL_POS_X+1+marker_length,   PROG2_LOCAL_POS_Y+1, PROG2_LOCAL_POS_X+PROG2_LOCAL_POS_WIDTH-1, PROG2_LOCAL_POS_Y+PROG2_LOCAL_POS_HEIGTH-1, CLCDDisplay::PIXEL_OFF);
		
		// paint foot 
		if(m_progressShowEscape  == true)
		{
			fonts.menu->RenderString(lcd_width-40, lcd_height, 40, "Home", CLCDDisplay::PIXEL_ON);
		}
		
		displayUpdate();
	}
#endif
}

