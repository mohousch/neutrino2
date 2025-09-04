//
//	$Id: lcdd.cpp 04092025 mohousch Exp $
//
//	LCD-Daemon  -   DBoxII-Project
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2008 Novell, Inc. Author: Stefan Seyfried
//		  (C) 2009 Stefan Seyfried
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/lcdd.h>

#include <global.h>
#include <neutrino2.h>

#include <system/settings.h>
#include <system/debug.h>
#include <system/helpers.h>
#include <system/channellogo.h>
#include <system/weather.h>

#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <daemonc/remotecontrol.h>


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
			dprintf(DEBUG_NORMAL, "CLCD::openDevice: failed to open vfd\n");
			
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

#ifdef ENABLE_GRAPHLCD
int CLCD::GetConfigSize()
{
	//
	if (GLCD::Config.Load(kDefaultConfigFile) == false)
	{
		ng_err("CLCD::GetConfigSize Error loading config file!\n");
		return 0;
	}
	
	// driver config
	if ((GLCD::Config.driverConfigs.size() < 1))
	{
		ng_err("CLCD::GetConfigSize No driver config found!\n");
		return 0;
	}
	
	return (int) GLCD::Config.driverConfigs.size();
}

std::string CLCD::GetConfigName(int driver)
{
	if ((driver < 0) || (driver > GetConfigSize() - 1))
		driver = 0;
		
	//
	if (GLCD::Config.Load(kDefaultConfigFile) == false)
	{
		ng_err("CLCD::GetConfigName Error loading config file!\n");
		return " ";
	}
	
	// driver config
	if ((GLCD::Config.driverConfigs.size() < 1))
	{
		ng_err("CLCD::GetConfigName No driver config found!\n");
		return " ";
	}
		
	return GLCD::Config.driverConfigs[driver].name;
}

void CLCD::reinitGLCD()
{
	if (display)
		display->initGLCD();
		
	if (!has_lcd)
	{
		
	}
}

void CLCD::setGLCDBrightness(int brightness)
{
	if (display) display->setGLCDBrightness(brightness);
}
#endif

////
CLCD::CLCD()
{
	muted = false;
	percentOver = 0;
	volume = 0;
	timeout_cnt = 0;
	icon_dolby = false;	
	has_lcd = false;
	has_nglcd = false;
	clearClock = 0;
	fd = -1;
	lcd_width = 220;
	lcd_height = 176;
#if defined (BOXMODEL_VUDUO4K) || defined (BOXMODEL_VUULTIMO4K)
        lcd_width = 480;
        lcd_height = 320;
#elif defined (BOXMODEL_VUUNO4KSE)
        lcd_width = 400;
        lcd_height = 240;
#elif defined (BOXMODEL_DM7080) || defined (BOXMODEL_DM8000HD)
        lcd_width = 128;
        lcd_height = 64;
#elif defined (BOXMODEL_DM800SE) || defined (BOXMODEL_DM900) || defined (BOXMODEL_DM920)
        lcd_width = 96;
        lcd_height = 64;
#endif
	servicename = "";
	servicenumber = 0;
	epg_title = "";
	movie_big = "";
	movie_small = "";
	menutitle = "";
	movie_playmode = PLAY_MODE_STOP;
	showclock = false;
	////
	m_progressHeaderGlobal = "";
	m_progressHeaderLocal = "";
	m_progressGlobal = 0;
	m_progressLocal = 0;
	////
	logo_w = 120;
	logo_h = 60;
	logo_x = 0;
	logo_y = 12;
	w_icon_w = 35;
	w_icon_h = 35;

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	display = NULL;	
#endif
}

CLCD::~CLCD()
{
#if defined (ENABLE_4DIGITS) || defined (ENABLE_VFD)
	if (fd)
		::close(fd);
		
	fd = -1;
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (display)
	{
		delete display;
		display = NULL;
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
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
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
		ng_err("CLCD::init failed!\n");
		has_lcd = false;
		return;
	}
	
	dprintf(DEBUG_NORMAL, "CLCD::init: succeeded\n");

	// create time thread
	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 )
	{
		ng_err("CLCD::init pthread_create(TimeThread)");
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
	if (!display)
		display = new CLCDDisplay();
	
	// check if we have display
#ifdef USE_OPENGL
	if (display->init("/dev/null"))
#else
	if (display->init("/dev/fb1"))
#endif
	{
		has_lcd = true;
		
		lcd_width = display->xres;
		lcd_height = display->yres;
	
		dprintf(DEBUG_NORMAL, "CLCD::lcdInit: %d %d\n", lcd_width, lcd_height);
	}
#endif

#ifdef ENABLE_GRAPHLCD
	if (!display)
		display = new CLCDDisplay();
	
	if (g_settings.glcd_enable)
	{	
		if (display->initGLCD())
		{
			has_lcd = true;
			
			lcd_width = display->xres;
			lcd_height = display->yres;
		
			dprintf(DEBUG_NORMAL, "CLCD::lcdInit %d %d\n", lcd_width, lcd_height);
		}
	}
#endif
	
	// init buffer / fonts
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (!has_lcd)
		return false;
	
	// init raw_buffer	
	display->initBuffer();
		
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

	fonts.menu        = fontRenderer->getFont(fontname,  style_name , (lcd_height > 64)? 24 : 12);
	fonts.time        = fontRenderer->getFont(fontname2, style_name2, (lcd_height > 64)? 22 : 14);
	fonts.channelname = fontRenderer->getFont(fontname3, style_name3, (lcd_height > 64)? 30 : 15);
	fonts.menutitle   = fonts.channelname;
	fonts.timestandby = fontRenderer->getFont(fontname,  style_name , (lcd_height > 64)? 50 : 20);

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
		
		int nbpp, nchans;
		::getSize(element[i].name.c_str(), &element[i].width, &element[i].height, &nbpp, &nchans);
	}
#endif

	//
	setLED(g_settings.lcd_led, 0);

	return has_lcd;
}

void CLCD::displayUpdate()
{
	struct stat buf;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (stat("/tmp/lcd.locked", &buf) == -1)
	{
		display->update();
	}
#endif
}

void CLCD::setlcdparameter(int dimm, const int contrast, const int power, const int inverse, const int bias)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_INFO, "CLCD::setlcdparameter: brightness: %d contrast: %d power: %d inverse: %d\n", dimm, contrast, power, inverse);
	
#ifndef ENABLE_GRAPHLCD	
	if (power == 0)
		dimm = 0;
#endif

#ifdef ENABLE_4DIGITS
	std::string value = toString(255 / 15 * dimm);
	
	if (access("/proc/stb/lcd/oled_brightness", F_OK) == 0)
		proc_put("/proc/stb/lcd/oled_brightness", value.c_str(), value.length());
	else if (access("/proc/stb/fp/oled_brightness", F_OK) == 0)
		proc_put("/proc/stb/fp/oled_brightness", value.c_str(), value.length());	
#elif defined (ENABLE_VFD)
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
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	// dimm
	display->setLCDBrightness(dimm);
	
	// contrast
	display->setLCDContrast(contrast);
	
	//reverse
	if (inverse)
		display->setInverted(LCD_PIXEL_WHITE);
	else		
		display->setInverted(LCD_PIXEL_BACKGROUND);
#endif

#ifdef ENABLE_GRAPHLCD
	if (mode == MODE_STANDBY)
		display->setGLCDBrightness(g_settings.glcd_brightness_standby);
	else
		display->setGLCDBrightness(g_settings.glcd_brightness);
#endif
}

void CLCD::setlcdparameter(void)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_DEBUG, "CLCD::setlcdparameter:\n");

	last_toggle_state_power = g_settings.lcd_power;
	int dim_time = atoi(g_settings.lcd_setting_dim_time);
	int dim_brightness = g_settings.lcd_setting_dim_brightness;
	bool timeouted = (dim_time > 0) && (timeout_cnt == 0);
	int brightness = 0;
	int power = 0;

	if (timeouted)
		brightness = dim_brightness;
	else
		brightness = g_settings.lcd_brightness;

	if (last_toggle_state_power && (!timeouted || dim_brightness > 0))
		power = 1;

	if (mode == MODE_STANDBY)
		brightness = g_settings.lcd_standbybrightness;
		
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

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
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
		
	dprintf(DEBUG_NORMAL, "CLCD::showTextScreen: big:%s small:%s showmode:0x%x wakeup:%s centered:%s\n", big.empty()? "null" : big.c_str(), small.empty()? "null" : small.c_str(), showmode, perform_wakeup? "true" : "false", centered? "centered" : "not centered");
	
	//
	bool big_utf8 = false;
	bool small_utf8 = false;
	std::string cname[2];
	std::string event[4];
	int namelines = 0, eventlines = 0, maxnamelines = 2;
	int maxeventlines = 4 - namelines;
	int y = 0;
	int x = 0;
		
#if defined (ENABLE_VFD)
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
#endif
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
	
	// clear screen under banner
	if (mode == MODE_PIC)
		display->draw_fill_rect(-1, element[ELEMENT_BANNER].height - 1, lcd_width, lcd_height, LCD_PIXEL_BACKGROUND);
	else
		display->draw_fill_rect(-1, element[ELEMENT_BANNER].height - 1, lcd_width, lcd_height - fonts.time->getHeight(), LCD_PIXEL_BACKGROUND);

	//	
	if ( (g_settings.lcd_picon || g_settings.lcd_weather) && mode == MODE_TVRADIO )
		maxnamelines = 1;
		
	if (lcd_height <= 64)
		maxnamelines = 1;

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

	// calculate maxeventlines
	maxeventlines = 4 - namelines;
	maxeventlines = ((lcd_height - element[ELEMENT_BANNER].height - fonts.time->getHeight()) / fonts.menu->getHeight()) - namelines;
	
	if ( (g_settings.lcd_picon || g_settings.lcd_weather) && mode == MODE_TVRADIO )
		maxeventlines = 1;
		
	if (lcd_height <= 64)
		maxnamelines = 1;

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
				// DrDish TV appends a 0x0a to the EPG title. We could strip it in sectionsd...
				//   ...instead, strip all control characters at the end of the text for now
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
	y = element[ELEMENT_BANNER].height / 2;
	x = 1;
	
	// namelines
	for (int i = 0; i < namelines; i++) 
	{
		y += fonts.channelname->getHeight();
		
		if (centered)
		{
			int w = fonts.channelname->getRenderWidth(cname[i].c_str(), big_utf8);

			x = (lcd_width - w) / 2;
		}
		
		fonts.channelname->RenderString(x, y, lcd_width + 10, cname[i].c_str(), LCD_PIXEL_YELLOW, 0, big_utf8);
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
			
			fonts.menu->RenderString(x, y, lcd_width + 10, event[i].c_str(), LCD_PIXEL_WHITE, 0, small_utf8);
		}
	}
#endif

	if (perform_wakeup)
		wake_up();

	displayUpdate();
}

void CLCD::showText(const char *str)
{
	if (!has_lcd)
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::showText: %s\n", str? str : "null");
		 
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
#endif
}

void CLCD::showServicename(const std::string &name, const bool perform_wakeup, int pos)
{
	if (!has_lcd)
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::showServicename: name:%s\n", name.empty()? "null" : name.c_str());

	int showmode = g_settings.lcd_epgmode;

	if (!name.empty())
		servicename = name;
		
	servicenumber = pos;

	if (mode != MODE_TVRADIO)
		return;

#if defined (ENABLE_VFD)
	if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
	{
		showText((char *)servicename.c_str() );
	}
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
	{
		// servicename
		showTextScreen(servicename, epg_title, showmode, perform_wakeup, g_settings.lcd_epgalign);
		
		// servicelogo
		logo_w = 120;
		logo_h = 60;
		
		logo_x = (lcd_width - logo_w)/2; if (logo_x < 0) logo_x = 0; // FIXME:
		logo_y = lcd_height - fonts.time->getHeight() - logo_h - 2; if (logo_y < 0) logo_y = 0; // FIXME:
		
		if (g_settings.lcd_weather)
			logo_x = lcd_width - logo_w - 1;
			
		if (logo_x < 0) logo_x = 0; // FIXME:
		
		if (g_settings.lcd_picon && lcd_height > 64)
		{
			std::string logo = DATADIR "/lcd/picon_default.png";
			
			t_channel_id logoid = CZapit::getInstance()->getChannelLogoID(CZapit::getInstance()->getCurrentChannelID());
		
			if (CChannellogo::getInstance()->checkLogo(logoid))
				logo = CChannellogo::getInstance()->getLogoName(logoid);
				
			int l_w, l_h, l_bpp, l_chans;
			
			::getSize(logo.c_str(), &l_w, &l_h, &l_bpp, &l_chans);
			
			if (l_h < logo_h)
				logo_h = l_h;
				
			if (l_w < logo_w)
				logo_w = l_w;
				
			// recalculate logo_x / logo_y
			logo_x = (lcd_width - logo_w)/2; if (logo_x < 0) logo_x = 0; // FIXME:
			logo_y = lcd_height - fonts.time->getHeight() - logo_h - 2; if (logo_y < 0) logo_y = 0; // FIXME:
			
			if (g_settings.lcd_weather)
				logo_x = lcd_width - logo_w - 1; if (logo_x < 0) logo_x = 0; // FIXME:

			if (logoid != 0)
			{
				display->showPNGImage(logo.c_str(), logo_x, logo_y, logo_w, logo_h);
			}
		}
		
		// weather
		if (g_settings.lcd_weather && lcd_height > 64)
		{
			showWeather();
		}
	}
#endif
	
	if (perform_wakeup)
		wake_up();
}

void CLCD::setEPGTitle(const std::string title)
{
	if (!has_lcd)
		return;

	dprintf(DEBUG_NORMAL, "CLCD::setEPGTitle: %s\n", title.c_str());
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	epg_title.clear();
	
	epg_title = title;
#endif
}

void CLCD::showMovieInfo(const PLAYMODES playmode, const std::string big, const std::string small, const bool centered)
{
	dprintf(DEBUG_NORMAL, "CLCD::showMovieInfo: playmode:%d big:%s small:%s centered:%s\n", playmode, big.empty()? "null" : big.c_str(), small.empty()? "null" : small.c_str(), centered? "centered" : "not centered");
	
	int showmode = g_settings.lcd_epgmode;

#if defined (ENABLE_VFD)
	showText((char *)big.c_str());
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
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
#endif
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
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
			if (g_settings.lcd_standby_clock == STANDBYCLOCK_DIGITAL)
			{
				// refresh
				display->draw_fill_rect(- 1, -1, lcd_width - 1, lcd_height - 1, LCD_PIXEL_BLACK);

				// time
				fonts.timestandby->RenderString((lcd_width - 1 - fonts.timestandby->getRenderWidth(timestr))/2, lcd_height/2, fonts.timestandby->getRenderWidth("00:00:00"), timestr, LCD_PIXEL_WHITE);
				
				// date
				strftime((char*) &datestr, 20, "%d.%m.%Y", t);
				
				fonts.menu->RenderString((lcd_width - 1 - fonts.menu->getRenderWidth(datestr))/2, lcd_height - fonts.menu->getHeight() - 1, fonts.menu->getRenderWidth("00:00:0000:0", true), datestr, LCD_PIXEL_WHITE);
			}
			else if (g_settings.lcd_standby_clock == STANDBYCLOCK_ANALOG)
			{
				display->showPNGImage(element[ELEMENT_ACLOCK].name.c_str(), 0, 0, lcd_width, lcd_height);
				display->show_analog_clock(t->tm_hour, t->tm_min, t->tm_sec, lcd_width/2, lcd_height/2, 3, 2, 1);
			}
		}
		else
		{
			if (g_settings.lcd_mode == CLCD::MODE_CHANNEL_INFO)
			{
				// refresh
				display->draw_fill_rect(lcd_width - 1 - fonts.time->getRenderWidth("00:00", true), lcd_height - fonts.time->getHeight()/2, lcd_width, lcd_height, LCD_PIXEL_BACKGROUND);

				// time
				fonts.time->RenderString(lcd_width - fonts.time->getRenderWidth("00:00", true), lcd_height - 1, fonts.menu->getRenderWidth("00:00:0", true), timestr, LCD_PIXEL_WHITE);
			}
			else if (g_settings.lcd_mode == CLCD::MODE_TIME)
			{
				if (g_settings.lcd_standby_clock == STANDBYCLOCK_DIGITAL)
				{
					// refresh
					display->draw_fill_rect(- 1, -1, lcd_width - 1, lcd_height - 1, LCD_PIXEL_BLACK);

					// time
					fonts.timestandby->RenderString((lcd_width - 1 - fonts.timestandby->getRenderWidth(timestr))/2, lcd_height/2, fonts.timestandby->getRenderWidth("00:00:00"), timestr, LCD_PIXEL_WHITE);
					
					// date
					strftime((char*) &datestr, 20, "%d.%m.%Y", t);
					
					fonts.menu->RenderString((lcd_width - 1 - fonts.menu->getRenderWidth(datestr))/2, lcd_height - fonts.menu->getHeight() - 1, fonts.menu->getRenderWidth("00:00:0000:0", true), datestr, LCD_PIXEL_WHITE);
				}
				else if (g_settings.lcd_standby_clock == STANDBYCLOCK_ANALOG)
				{
					display->showPNGImage(element[ELEMENT_ACLOCK].name.c_str(), 0, 0, lcd_width, lcd_height);
					display->show_analog_clock(t->tm_hour, t->tm_min, t->tm_sec, lcd_width/2, lcd_height/2, 3, 2, 1);
				}
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
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	uint32_t * curr_screen = new uint32_t[display->raw_buffer_size];

	// Saving the whole screen is not really nice since the clock is updated
	// every second. Restoring the screen can cause a short travel to the past ;)
	display->dump_screen(&curr_screen);
	
	//
	display->draw_fill_rect (-1, 10, lcd_width, lcd_height - 12 - 2, LCD_PIXEL_BACKGROUND);
	display->show_png_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height - element[ELEMENT_SPEAKER].height);
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
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
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
					
				if ((g_RemoteControl->current_PIDs.otherPIDs.selected_apid < count) &&
					    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.otherPIDs.selected_apid].is_ac3))
					icon_dolby = true;
				else
					icon_dolby = false;
			}
		}

		// icon
		if ((muted) || (volume == 0))
			display->show_png_element(&(element[ELEMENT_MUTE]), 0, lcd_height - element[ELEMENT_MUTE].height);
		else
		{
			if (icon_dolby)
				display->show_png_element(&(element[ELEMENT_DOLBY]), 0, lcd_height - element[ELEMENT_DOLBY].height);
			else
				display->show_png_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height -element[ELEMENT_SPEAKER].height);
		}

		//strichline
		if ((muted) || (volume == 0))
		{
			display->draw_line(left, top, left + width, top + height - 1, LCD_PIXEL_WHITE);
		}
		else
		{
			int dp = vol*(width + 1)/100;
			display->draw_fill_rect(left - 1, top - 1, left + dp, top + height, LCD_PIXEL_WHITE);
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
			display->draw_fill_rect (-1, top-2         , 10, lcd_height-3  , LCD_PIXEL_BACKGROUND);
			display->draw_rectangle ( 1, top+2         ,  3, lcd_height-3-3, LCD_PIXEL_WHITE, LCD_PIXEL_BACKGROUND);
			display->draw_line      ( 3, top+2         ,  6, top-1         , LCD_PIXEL_WHITE);
			display->draw_line      ( 3, lcd_height-3-3,  6, lcd_height-3  , LCD_PIXEL_WHITE);
			display->draw_line      ( 6, top+1         ,  6, lcd_height-3-2, LCD_PIXEL_WHITE);
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
		
	if (mode != m)
		return;
		
	int left, top, width, height = 6;
	bool draw = true;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
	
	percentOver = perc;
	
	if ( (mode == MODE_TVRADIO || mode == MODE_MOVIE || mode == MODE_AUDIO) && (g_settings.lcd_statusline == STATUSLINE_PLAYTIME) )
	{
		left = 12 + 2; 
		top = lcd_height - height - 1 - 2; 
		width = lcd_width - left - 4 - fonts.time->getRenderWidth("00:00");

		// refresh
		display->draw_rectangle(left - 2, top - 2, left + width + 2, top + height + 1, LCD_PIXEL_WHITE, LCD_PIXEL_BACKGROUND);

		if (perc == (unsigned char) -1)
		{
			display->draw_line(left, top, left + width, top + height - 1, LCD_PIXEL_WHITE);
		}
		else
		{
			int dp;
			if (perc == (unsigned char) - 2)
				dp = width + 1;
			else
				dp = perc * (width + 1) / 100;
					
			display->draw_fill_rect(left - 1, top - 1, left + dp, top + height, LCD_PIXEL_PERCENT);

			if (perc == (unsigned char) - 2)
			{
				// draw a "+" to show that the event is overdue
				display->draw_line(left + width - 2, top + 1, left + width - 2, top + height - 2, LCD_PIXEL_BACKGROUND);
				display->draw_line(left + width - 1, top + (height/2), left + width - 3, top + (height/2), LCD_PIXEL_BACKGROUND);
			}
		}

		if (perform_update)
			displayUpdate();
	}
#endif
}

void CLCD::showMenuText(const int position, const char * text, const int selected, const bool utf_encoded)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::showMenuText: position:%d text:%s highlight:%d\n", position, text? text : "null", selected);
	
	if (mode != MODE_MENU_UTF8)
		return;
		
#if defined (ENABLE_VFD)						
	showText(text); // UTF-8
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	// second line
	// refresh
	display->draw_fill_rect(-1, element[ELEMENT_BANNER].height/2 + fonts.menutitle->getHeight() + 2 - 1, lcd_width, element[ELEMENT_BANNER].height/2 + fonts.menutitle->getHeight() + 2 + fonts.menu->getHeight() + 10, LCD_PIXEL_BACKGROUND);
	
	// render text
	fonts.menu->RenderString(0, element[ELEMENT_BANNER].height/2 + fonts.menutitle->getHeight() + 2 + fonts.menu->getHeight(), lcd_width + 10, text, LCD_PIXEL_WHITE, 0, utf_encoded);
#endif

	wake_up();
	displayUpdate();
}

void CLCD::showAudioTrack(const std::string &artist, const std::string &title, const std::string &album, int pos)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::showAudioTrack: artist:%s title:%s album:%s pos:%d\n", artist.empty()? "null" : artist.c_str(), title.empty()? "null" : title.c_str(), album.empty()? "null" : album.c_str(), pos);
	
	if (mode != MODE_AUDIO) 
		return;

#if defined (ENABLE_VFD)
	showText((char *)title.c_str());
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	// refresh
	display->draw_fill_rect(-1, element[ELEMENT_BANNER].height, lcd_width, element[ELEMENT_BANNER].height - 12, LCD_PIXEL_BACKGROUND);
	
	// title
	fonts.channelname->RenderString((lcd_width - fonts.channelname->getRenderWidth(title.c_str(), isUTF8(title)))/2, element[ELEMENT_BANNER].height/2 + fonts.channelname->getHeight(), lcd_width + 10, title.c_str(), LCD_PIXEL_YELLOW, 0, isUTF8(title));
	
	// artist
	fonts.menu->RenderString((lcd_width - fonts.menu->getRenderWidth(artist.c_str(), isUTF8(artist)))/2, element[ELEMENT_BANNER].height/2 + fonts.channelname->getHeight() + fonts.menu->getHeight(), lcd_width + 10, artist.c_str(),  LCD_PIXEL_WHITE, 0, isUTF8(artist));
	
	// album
	if (lcd_height > 64)
		fonts.menu->RenderString((lcd_width - fonts.menu->getRenderWidth(album.c_str(), isUTF8(album)))/2, element[ELEMENT_BANNER].height/2 + fonts.channelname->getHeight() + 2*fonts.menu->getHeight() + 2*2, lcd_width + 10, album.c_str(),  LCD_PIXEL_WHITE, 0, isUTF8(album));
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
#endif

#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	// refresh
	display->draw_fill_rect (-1, lcd_width - 12, 12, 12, LCD_PIXEL_BACKGROUND);
	
	switch(m)
	{
		case PLAY_MODE_PLAY:
			display->show_png_element(&(element[ELEMENT_PLAY]), 0, lcd_height - element[ELEMENT_PROG].height);
			break;
			
		case PLAY_MODE_STOP:
			display->draw_fill_rect (-1, lcd_height - 12, 12, 12, LCD_PIXEL_WHITE);
			break;
			
		case PLAY_MODE_PAUSE:
			display->show_png_element(&(element[ELEMENT_PAUSE]), 0, lcd_height - element[ELEMENT_PROG].height);
			break;
			
		case PLAY_MODE_FF:
			{
				int x = 2, y = lcd_height - 12;
				display->draw_line(x, y, x, y + 4, LCD_PIXEL_WHITE);
				display->draw_line(x + 1, y + 1, x + 1, y + 3, LCD_PIXEL_WHITE);
				display->draw_line(x + 2, y + 2, x + 2, y + 2, LCD_PIXEL_WHITE);
				display->draw_line(x + 3, y, x + 3, y + 4, LCD_PIXEL_WHITE);
				display->draw_line(x + 4, y + 1, x + 4, y + 3, LCD_PIXEL_WHITE);
				display->draw_line(x + 5, y + 2, x + 5, y + 2, LCD_PIXEL_WHITE);
			}
			break;
			
		case PLAY_MODE_REV:
			{
				int x = 2, y = lcd_height - 12;
				display->draw_line(x, y + 2, x, y + 2, LCD_PIXEL_WHITE);
				display->draw_line(x + 1, y + 1, x + 1, y + 3, LCD_PIXEL_WHITE);
				display->draw_line(x + 2, y, x + 2, y + 4, LCD_PIXEL_WHITE);
				display->draw_line(x + 3, y + 2, x + 3, y + 2, LCD_PIXEL_WHITE);
				display->draw_line(x + 4, y + 1, x + 4, y + 3, LCD_PIXEL_WHITE);
				display->draw_line(x + 5, y, x + 5, y + 4, LCD_PIXEL_WHITE);
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
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	display->show_png_element(&(element[ELEMENT_BANNER]), 0, 0, lcd_width, 12);
	
	if (element[ELEMENT_BANNER].width < lcd_width)
		display->draw_fill_rect(element[ELEMENT_BANNER].width - 1, -1, lcd_width, element[ELEMENT_BANNER].height - 1, LCD_PIXEL_WHITE);
#endif
}

void CLCD::setMode(const MODES m, const char * const title)
{
	if(!has_lcd) 
		return;
		
	dprintf(DEBUG_NORMAL, "CLCD::setMode: %d (%s)\n", m, (title !=NULL)? title : "NULL");
		
	mode = m;
	menutitle = title;
	
	setlcdparameter();

#if defined (ENABLE_4DIGITS)
	switch (m) 
	{
		case MODE_TVRADIO:
			showclock = false;
			if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
				showServicename(servicename, true, servicenumber);
			else if (g_settings.lcd_mode == MODE_TIME)
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
			if (g_settings.lcd_mode == MODE_CHANNEL_INFO)	
				showServicename(servicename, true, servicenumber);
			else if (g_settings.lcd_mode == MODE_TIME)
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
#endif
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	switch (m)
	{
	case MODE_TVRADIO:
	case MODE_MOVIE:
	{
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		// clear lcd
		display->clear_screen();
		
		// banner
		drawBanner();
		// statusline
		switch (g_settings.lcd_statusline)
		{
			case CLCD::STATUSLINE_PLAYTIME:
				display->show_png_element(&(element[ELEMENT_PROG]), 0, lcd_height - element[ELEMENT_PROG].height);
				showPercentOver(percentOver, false, mode);
				break;
				
			case CLCD::STATUSLINE_VOLUME:
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
		}
		
		// time
		showclock = true;
		showTime();
		break;
	}
		
	case MODE_AUDIO:
	{
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		display->clear_screen();
		drawBanner();
		display->show_png_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height - element[ELEMENT_SPEAKER].height - 1);
		showPlayMode(PLAY_MODE_STOP);
		showVolume(volume, false);
		}
		
		showclock = true;
		showTime();
		break;
	}
	
	case MODE_PIC:
	{
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		display->clear_screen();
		drawBanner();
		showclock = false;
		fonts.menutitle->RenderString(0, element[ELEMENT_BANNER].height/2 + fonts.menutitle->getHeight(), lcd_width, title, LCD_PIXEL_YELLOW, 0, true); // UTF-8
		}
		
		displayUpdate();
		break;
	}
	
	case MODE_SCART:
	{
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		display->clear_screen();
		drawBanner();
		display->show_png_element(&(element[ELEMENT_SCART]), (lcd_width-element[ELEMENT_SCART].width)/2, 12);
		display->show_png_element(&(element[ELEMENT_SPEAKER]), 0, lcd_height-element[ELEMENT_SPEAKER].height-1);

		showVolume(volume, false);
		}
		
		showclock = true;
		showTime();
		break;
	}
		
	case MODE_MENU_UTF8:
	{
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		showclock = false;
		display->clear_screen(); // clear lcd
		drawBanner();
		fonts.menutitle->RenderString(0, element[ELEMENT_BANNER].height/2 + fonts.menutitle->getHeight(), lcd_width, title, LCD_PIXEL_YELLOW, 0, true); // UTF-8
		displayUpdate();
		}
		break;
	}
		
	case MODE_SHUTDOWN:
	{
		showclock = false;
		
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		display->clear_screen();
		drawBanner();
		display->show_png_element(&(element[ELEMENT_POWER]), (lcd_width - element[ELEMENT_POWER].width)/2, (lcd_height - element[ELEMENT_POWER].height)/2);
		displayUpdate();
		}
		break;
	}
		
	case MODE_STANDBY:
	{
		// clear lcd
		display->clear_screen();
		showclock = true;
		showTime();
		break;
	}
		
	case MODE_PROGRESSBAR:
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		showclock = false;
		display->clear_screen();
		drawBanner();
		showProgressBar();
		}
		break;
		
	case MODE_PROGRESSBAR2:
		if (g_settings.lcd_mode == MODE_CHANNEL_INFO)
		{
		showclock = false;
		display->clear_screen();
		drawBanner();
		showProgressBar2();
		}
		break;
	}
#endif
	
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
		
	dprintf(DEBUG_NORMAL, "CLCD::setLED: %d\n", value);

	g_settings.lcd_led = value;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
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
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD)
	display->clear_screen();
	
	const char *LCDMINITV[] = 
	{
		"NORMAL",
		"MINITV",
		"OSD",
		"OSD / MINITV"
	};
	
	dprintf(DEBUG_NORMAL, "CLCD::setMiniTV: %s\n", LCDMINITV[value]);

	proc_put("/proc/stb/lcd/mode", value);
#endif
}

void CLCD::resume()
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	display->resume();
#endif
}

void CLCD::pause()
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
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
#endif
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
//	if (mode == MODE_SHUTDOWN)
	{
		display->clear_screen(); // clear lcd
		displayUpdate();
	}
#endif
}

bool CLCD::ShowPng(char *filename)
{
	if(!has_lcd) 
		return false;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	return display->showPNGImage(filename, 0, 0, lcd_width, lcd_height);
#endif
}

bool CLCD::DumpPng(char *filename)
{
	if(!has_lcd) 
		return false;
		
	printf("CLCD::DumpPng\n");
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	return display->dump_png(filename);
#endif
}

// showProgressBar
void CLCD::showProgressBar(int global, const char * const text)
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	int prog_w = lcd_width - 10;
	int prog_h = 12; //20;
	int prog_x = 5;
	int prog_y = (lcd_height - prog_h) / 2;
	
	if(text != NULL)
		m_progressHeaderGlobal = text;
		
	if(global >= 0)
	{
		if(global > 100)
			m_progressGlobal = 100;
		else
			m_progressGlobal = global;
	}

	if (mode == MODE_PROGRESSBAR)
	{
		// Clear Display
		display->draw_fill_rect (0, 12, lcd_width, lcd_height, LCD_PIXEL_BACKGROUND);
	
		// paint progress header 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(), true);
		
		if(width > lcd_width)
			width = lcd_width;
			
		int start_pos = (lcd_width - width)/2;
		
		// show global local only if lcd_height is heigher than 64
		if (lcd_height > 64)
			fonts.menu->RenderString(start_pos, 12 + fonts.menu->getHeight() + 2, width + 10, m_progressHeaderGlobal.c_str(), LCD_PIXEL_WHITE, 0, true);
	
		// paint global bar 
		int marker_length = (prog_w*m_progressGlobal)/100;
		
		display->draw_fill_rect(prog_x, prog_y, prog_x + prog_w, prog_y + prog_h, LCD_PIXEL_PERCENT);
		
		display->draw_fill_rect(prog_x + 1 + marker_length, prog_y + 1, prog_x + prog_w - 1, prog_y + prog_h - 1, LCD_PIXEL_BACKGROUND);
		
		displayUpdate();
	}
#endif
}

// showProgressBar2
void CLCD::showProgressBar2(int local,const char * const text_local, int global, const char * const text_global)
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	int prog_w = lcd_width - 10;
	int prog_h = 12; //20;
	int prog_x = 5;
	int prog_y1 = lcd_height/2 - prog_h - 5;
	int prog_y2 = lcd_height/2 + prog_h + 5;
	
	//
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

	if (mode == MODE_PROGRESSBAR2)
	{
		// Clear Display
		display->draw_fill_rect (0, 12, lcd_width, lcd_height, LCD_PIXEL_BACKGROUND);
	
		// paint  global caption 
		int width = fonts.menu->getRenderWidth(m_progressHeaderGlobal.c_str(), true);
		
		if(width > lcd_width)
			width = lcd_width;
			
		int start_pos = (lcd_width - width) /2;
		
		// show global local only if lcd_height is heigher than 64
		if (lcd_height > 64)
			fonts.menu->RenderString(start_pos, 12 + 20, width + 10, m_progressHeaderGlobal.c_str(), LCD_PIXEL_WHITE, 0, true);
	
		// paint global bar 
		int marker_length = (prog_w * m_progressGlobal)/100;
		
		display->draw_fill_rect(prog_x, prog_y1, prog_x + prog_w,   prog_y1 + prog_h, LCD_PIXEL_PERCENT);
		
		display->draw_fill_rect(prog_x + 1 + marker_length, prog_y1 + 1, prog_x + prog_w - 1, prog_y1 + prog_h - 1, LCD_PIXEL_BACKGROUND);
	
		
		// paint  local caption 
		width = fonts.menu->getRenderWidth(m_progressHeaderLocal.c_str(), true);
		
		if(width > lcd_width)
			width = lcd_width;
			
		start_pos = (lcd_width - width) /2;
		
		// show global local only if lcd_height is heigher than 64
		if (lcd_height > 64)
			fonts.menu->RenderString(start_pos, prog_y2 + prog_h + fonts.menu->getHeight()/2, width + 10, m_progressHeaderLocal.c_str(), LCD_PIXEL_WHITE, 0, true);
		
		// paint local bar 
		marker_length = (prog_w * m_progressLocal)/100;
		
		display->draw_fill_rect(prog_x, prog_y2, prog_x + prog_w, prog_y2 + prog_h, LCD_PIXEL_BLUE);
		
		display->draw_fill_rect(prog_x + 1 + marker_length, prog_y2 + 1, prog_x + prog_w - 1, prog_y2 + prog_h - 1, LCD_PIXEL_BACKGROUND);
		
		displayUpdate();
	}
#endif
}

// weather
void CLCD::showWeather()
{
	if(!has_lcd) 
		return;
		
#if defined (ENABLE_LCD) || defined (ENABLE_TFTLCD) || defined (ENABLE_GRAPHLCD)
	if (g_settings.lcd_mode != MODE_CHANNEL_INFO)
		return;
		
	int ctx, cix, y;

	// left
	int w_w = lcd_width - 2 - 60;
	int w_h = 60;
	int w_x = 1;
	int w_y = lcd_height - fonts.time->getHeight() - w_h - 2;
	w_icon_w = 35;
	w_icon_h = 35;

	std::string current_wcity = "";
	std::string current_wtemp = "";
	std::string current_wicon = DATADIR "/icons/unknown.png";
	
	CWeather::getInstance()->checkUpdate();

	current_wcity = CWeather::getInstance()->getCity();
	current_wtemp = CWeather::getInstance()->getCurrentTemperature();
	current_wicon = CWeather::getInstance()->getCurrentIcon();
	
	dprintf(DEBUG_NORMAL, "CLCD::showWeather %s %s %s\n", current_wcity.c_str(), current_wtemp.c_str(), current_wicon.c_str());

	// current icon
	if (current_wicon != "")
	{
		display->showPNGImage(current_wicon.c_str(), w_x, w_y + fonts.time->getHeight() + 2, w_icon_w, w_icon_h);
	}
	
	// current temp
	if (current_wtemp != "")
	{
		current_wtemp += "°";
			
		fonts.time->RenderString(w_x + 2 + w_icon_w, w_y + fonts.time->getHeight() + fonts.time->getHeight() / 2 + 2, w_w - w_icon_w - 2, current_wtemp.c_str(), LCD_PIXEL_WHITE, 0, true);
	}
#endif
}

