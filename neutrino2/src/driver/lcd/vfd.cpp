/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: vfd.cpp 2013/10/12 mohousch Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <driver/lcd/vfd.h>

#include <global.h>
#include <neutrino2.h>
#include <system/settings.h>

#include <fcntl.h>
#include <sys/timeb.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <system/debug.h>
#include <system/helpers.h>


extern CRemoteControl *g_RemoteControl;

#if defined (__sh__)
#if defined (PLATFORM_SPARK7162)
static struct aotom_ioctl_data aotom_data;
#endif

#if defined (PLATFORM_KATHREIN) || defined (PLATFORM_SPARK7162)
static bool usb_icon = false;
static bool timer_icon = false;
static bool hdd_icon = false;
#endif

//konfetti: let us share the device with evremote and fp_control
//it does currently not support more than one user (see e.g. micom)
bool blocked = false;

void CVFD::openDevice()
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

void CVFD::closeDevice()
{ 
	if (fd)
	{
		close(fd);
		blocked = false;
	}
	
	fd = -1;
}
#endif

// default: has_lcd:true, is4digits:0, has_led:0
// constructor
CVFD::CVFD()
{
	// vfd
	has_lcd = true;

	// 4digits
	is4digits = false;
	
	// tftlcd
	istftlcd = false;
	
#if defined (ENABLE_4DIGITS)
	is4digits = true;
#endif

#if !defined (__sh__)
	// probe /dev/dbox/fp
	fd = open("/dev/dbox/fp", O_RDWR);
		
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
								dprintf(DEBUG_NORMAL, "CVFD::init no VFD/LCD detected\n");
								has_lcd = false;
							}
						}
					}
				}
			}
		}
	}
#endif	
	
	text[0] = 0;
	clearClock = 0;
	timeout_cnt = 0;
}

CVFD::~CVFD()
{ 
#if !defined (__sh__)
	if(fd > 0)
		close(fd);
	
	fd = -1;
#endif
}

CVFD * CVFD::getInstance()
{
	static CVFD * lcdd = NULL;

	if(lcdd == NULL) 
	{
		lcdd = new CVFD();
	}

	return lcdd;
}

void CVFD::count_down() 
{
	if (timeout_cnt > 0) 
	{
		timeout_cnt--;
		if (timeout_cnt == 0) 
		{
			if (g_settings.lcd_setting_dim_brightness > 0) 
			{
				// save lcd brightness, setBrightness() changes global setting
				int b = g_settings.lcd_brightness;
				setBrightness(g_settings.lcd_setting_dim_brightness);
				g_settings.lcd_brightness = b;
			} 
			else 
			{
				//setPower(0);
			}
		}
	} 
}

void CVFD::wake_up() 
{  
	if (atoi(g_settings.lcd_setting_dim_time) > 0) 
	{
		timeout_cnt = atoi(g_settings.lcd_setting_dim_time);
		g_settings.lcd_setting_dim_brightness > 0 ? setBrightness(g_settings.lcd_brightness) : setPower(1);
	}
	else
		setPower(1);	
}

void * CVFD::TimeThread(void *)
{
	while(1) 
	{
		sleep(1);
		struct stat buf;
		
                if (stat("/tmp/vfd.locked", &buf) == -1) 
		{
                        CVFD::getInstance()->showTime();
                        CVFD::getInstance()->count_down();
                } 
		else
                        CVFD::getInstance()->wake_up();
	}

	return NULL;
}

void CVFD::init()
{
	dprintf(DEBUG_NORMAL, "CVFD::init\n");
	
	brightness = -1;
	
	// set mode tv/radio
	setMode(MODE_TVRADIO);

	// time thread
	if (pthread_create (&thrTime, NULL, TimeThread, NULL) != 0 ) 
	{
		perror("CVFD::init: pthread_create(TimeThread)");
		return ;
	}
	
	
	
	// set led color
#if defined (PLATFORM_GIGABLUE)
	setPower(g_settings.lcd_power);  //0:off, 1:blue, 2:red, 3:purple
#elif defined (PLATFORM_VENTON)
	vfd_symbol_network(0);
	vfd_symbol_circle(0);
#endif
}

void CVFD::setlcdparameter(int dimm, const int power)
{
	if(!has_lcd || is4digits) 
		return;
	
	if(dimm < 0)
		dimm = 0;
	else if(dimm > 15)
		dimm = 15;

	if(!power)
		dimm = 0;

	if(brightness == dimm)
		return;

	brightness = dimm;

	dprintf(DEBUG_DEBUG, "CVFD::setlcdparameter dimm %d power %d\n", dimm, power);
	
#if defined (__sh__)
        struct vfd_ioctl_data data;
	data.start_address = dimm;
	
	if(dimm < 1)
		dimm = 1;
	brightness = dimm;
	
	openDevice();
	
	if( ioctl(fd, VFDBRIGHTNESS, &data) < 0)  
		perror("VFDBRIGHTNESS");
	
	closeDevice();
#endif		
}

void CVFD::setlcdparameter(void)
{
	if(!has_lcd || is4digits) 
		return;

	last_toggle_state_power = g_settings.lcd_power;
	
	setlcdparameter( (mode == MODE_STANDBY) ? g_settings.lcd_brightness : g_settings.lcd_brightness, last_toggle_state_power);
}

void CVFD::showServicename(const std::string& name, const bool perform_wakeup, int pos) // UTF-8
{
	dprintf(DEBUG_DEBUG, "CVFD::showServicename: %s\n", name.c_str());
	
	if(!has_lcd) 
		return;
	
	servicename = name;
	serviceNum = pos;

	if (mode != MODE_TVRADIO)
		return;

	if (is4digits)
	{
		char tmp[5];
						
		sprintf(tmp, "%04d", pos);
						
		ShowText(tmp); // UTF-8
	}
	else
	{
		ShowText((char *)name.c_str() );
	}

	wake_up();
}

void CVFD::showTime(bool force)
{
	if(!has_lcd)
		return;

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
			ShowText(timestr);
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
}

void CVFD::showRCLock(int duration)
{
}

void CVFD::showMenuText(const int position, const char * text, const int highlight, const bool utf_encoded)
{
	if(!has_lcd) 
		return;

	if (mode != MODE_MENU_UTF8)
		return;

	if (is4digits)
	{
		char tmp[5];
						
		sprintf(tmp, "%04d", position);
						
		ShowText(tmp); // UTF-8
	}
	else
	{
		ShowText((char *)text);
	}
	
	wake_up();
}

void CVFD::showAudioTrack(const std::string& artist, const std::string& title, const std::string& album, int pos)
{
	if(!has_lcd) 
		return;	

	if (mode != MODE_AUDIO) 
		return;

	dprintf(DEBUG_DEBUG, "CVFD::showAudioTrack: %s\n", title.c_str());
	
	if (is4digits)
	{
		char tmp[5];
						
		sprintf(tmp, "%04d", pos);
						
		ShowText(tmp); // UTF-8
	}
	else
	{
		ShowText((char *)title.c_str());
	}
	
	wake_up();
}

void CVFD::showAudioPlayMode(AUDIOMODES m)
{
	if(!has_lcd || is4digits) 
		return;	

	switch(m) 
	{
		case AUDIO_MODE_PLAY:
			ShowIcon(VFD_ICON_PLAY, true);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case AUDIO_MODE_STOP:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, false);
			break;
			
		case AUDIO_MODE_PAUSE:
			ShowIcon(VFD_ICON_PLAY, false);
			ShowIcon(VFD_ICON_PAUSE, true);
			break;
			
		case AUDIO_MODE_FF:
			break;
			
		case AUDIO_MODE_REV:
			break;
	}

	wake_up();
}

void CVFD::setMode(const MODES m, const char * const title)
{
	if(!has_lcd) 
		return;

	// show title
	if(!is4digits) 
	{
		if(strlen(title))
			ShowText((char *)title);
	}

	mode = m;

	setlcdparameter();

	switch (m) 
	{
		case MODE_TVRADIO:
			if (g_settings.lcd_epgmode == EPGMODE_CHANNELNUMBER)	
				showServicename(g_RemoteControl->getCurrentChannelName(), true, g_RemoteControl->getCurrentChannelNumber());
			else if (g_settings.lcd_epgmode == EPGMODE_TIME)
				showTime(true);
			
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
			showclock = true;
			break;

		case MODE_AUDIO:
		{
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_AC3, false);			
#endif		  
			ShowIcon(VFD_ICON_MP3, true);			
			ShowIcon(VFD_ICON_TV, false);			
			showAudioPlayMode(AUDIO_MODE_STOP);			
			showclock = true;			
			ShowIcon(VFD_ICON_LOCK, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			//showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
			break;
		}

		case MODE_SCART:	  
			ShowIcon(VFD_ICON_TV, false);	
			showclock = true;
			//showTime();      /* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
			break;

		case MODE_MENU_UTF8:
			ShowIcon(VFD_ICON_TV, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			showclock = true;
			break;

		case MODE_SHUTDOWN:
			//Clear();
			/* clear all symbols */
			ClearIcons();
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_CLOCK, timer_icon);	
#endif			
			showclock = false;
			
			//
			ShowText((char *) "BYE");
			
			break;

		case MODE_STANDBY:
			ShowIcon(VFD_ICON_TV, false);
			ClearIcons();
#if defined(PLATFORM_SPARK7162)
			ShowIcon(VFD_ICON_STANDBY, true);	
#endif
							
			showclock = true;
			showTime(true);      	/* "showclock = true;" implies that "showTime();" does a "displayUpdate();" */
						/* "showTime()" clears the whole lcd in MODE_STANDBY */
			break;
		
		case MODE_PIC:	  
			ShowIcon(VFD_ICON_TV, false);			
			ShowIcon(VFD_ICON_HD, false);
			ShowIcon(VFD_ICON_DOLBY, false);
			
			showclock = true;
			break;
			
		case MODE_MOVIE:  
			ShowIcon(VFD_ICON_TV, false);			
			showclock = true;
			break;
	}

	wake_up();
}

void CVFD::setBrightness(int bright)
{
	if(!has_lcd || is4digits) 
		return;

	g_settings.lcd_brightness = bright;
	
	setlcdparameter();
}

int CVFD::getBrightness()
{
	//FIXME for old neutrino.conf
	if(g_settings.lcd_brightness > DEFAULT_LCD_BRIGHTNESS)
		g_settings.lcd_brightness = DEFAULT_LCD_BRIGHTNESS;

	return g_settings.lcd_brightness;
}

void CVFD::setBrightnessStandby(int bright)
{
	if(!has_lcd || is4digits) 
		return;

	g_settings.lcd_standbybrightness = bright;
	setlcdparameter();
}

int CVFD::getBrightnessStandby()
{
	//FIXME for old neutrino.conf
	if(g_settings.lcd_standbybrightness > DEFAULT_LCD_STANDBYBRIGHTNESS )
		g_settings.lcd_standbybrightness = DEFAULT_LCD_STANDBYBRIGHTNESS;
	
	return g_settings.lcd_standbybrightness;
}

void CVFD::setPower(int power)
{
	if(!has_lcd) 
		return;

#if defined (__sh__)
	struct vfd_ioctl_data data;
	data.start_address = power;
	
	openDevice();
	
	if( ioctl(fd, VFDPWRLED, &data) < 0)  
		perror("VFDPWRLED");
	
	closeDevice();
//#endif
#else
#if defined (PLATFORM_GIGABLUE)
	const char *VFDLED[] = {
		"VFD_OFF",
		"VFD_BLUE",
		"VFD_RED",
		"VFD_PURPLE"
	};
	
	dprintf(DEBUG_NORMAL, "CVFD::setPower: %s\n", VFDLED[power]);
	  
	FILE * f;
	if((f = fopen("/proc/stb/fp/led0_pattern", "w")) == NULL) 
		return;
	
	fprintf(f, "%d", power);
	fclose(f);
#endif	
#endif
}

int CVFD::getPower()
{
	return g_settings.lcd_power;
}

void CVFD::togglePower(void)
{
	if(!has_lcd || is4digits) 
		return;

	last_toggle_state_power = 1 - last_toggle_state_power;
	setlcdparameter((mode == MODE_STANDBY) ? g_settings.lcd_standbybrightness : g_settings.lcd_brightness, last_toggle_state_power);
}

void CVFD::setMuted(bool mu)
{
	if(!has_lcd || is4digits) 
		return;
	
	muted = mu;	
}

void CVFD::resume()
{
	if(!has_lcd || is4digits) 
		return;
}

void CVFD::pause()
{
	if(!has_lcd || is4digits) 
		return;
}

void CVFD::Lock()
{
	if(!has_lcd || is4digits) 
		return;

	creat("/tmp/vfd.locked", 0);
}

void CVFD::Unlock()
{
	if(!has_lcd || is4digits) 
		return;

	unlink("/tmp/vfd.locked");
}

void CVFD::Clear()
{
	if(!has_lcd) 
		return;
	
#if defined (ENABLE_4DIGITS)
	ShowText("     "); // 5 empty digits
#elif defined (__sh__)
	struct vfd_ioctl_data data;
	
#if defined (PLATFORM_KATHREIN)		/* using this otherwise VFD of ufs910 is black and Neutrino has a segfault 		*/
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
	ShowText("            "); // 12 empty digits
#endif
}

void CVFD::ClearIcons()				/* switcht all VFD Icons off		*/
{
	if(!has_lcd || is4digits) 
		return;

#if defined (__sh__)	
#if defined(PLATFORM_SPARK7162)		/* using one command for switching off all Icons*/	 
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
}

#if defined(PLATFORM_SPARK7162)			/* only for Spark7162 STB's which Display has a HDD Level indicator */	 
void CVFD::ShowDiskLevel()
{
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
		
		//printf("HDD Fuell = %d Digits = %d\n", percent, digits);
		
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
}
#endif

void CVFD::ShowIcon(vfd_icon icon, bool show)
{
	if(!has_lcd || is4digits) 
		return;
	
	dprintf(DEBUG_DEBUG, "CVFD::ShowIcon %s %x\n", show ? "show" : "hide", (int) icon);

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
#endif

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
#endif
}

void CVFD::ShowText(const char * str)
{
	if(!has_lcd) 
		return;
	
	dprintf(DEBUG_INFO, "CVFD::ShowText: [%s]\n", str);

	int len = strlen(str);
	
	//FIXME: some vfd treiber can not handle NULL string len
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
	 
#if defined (__sh__)	 
	openDevice();
	
	if(write(fd , text.c_str(), len > 16? 16 : len ) < 0)
		perror("write to vfd failed");
	
	closeDevice();
#elif defined (PLATFORM_ODIN) || defined (PLATFORM_GIGABLUE)
	if( write(fd, text.c_str(), len > 5? 5 : len ) < 0)
		perror("write to vfd failed");
#else
	if( write(fd, text.c_str(), len > 12? 12 : len ) < 0)
		perror("write to vfd failed");
	
#endif
}

void CVFD::setFan(bool enable)
{
#if defined (__sh__)
	/*
	 openDevice();
	
	if( ioctl(fd, VFDSETFAN, enable) < 0)  
		perror("VFDSETFAN");
	
	closeDevice();
	*/
#endif	
}

void CVFD::vfd_symbol_network(int net)
{
	const char *VFDNET[] = {
		"OFF",
		"ON"
	};
	
	dprintf(DEBUG_NORMAL, "CVFD::vfd_symbol_network: %s\n", VFDNET[net]);
	
#if defined (PLATFORM_VENTON)	
	FILE *f;
	if((f = fopen("/proc/stb/lcd/symbol_network","w")) == NULL) 
	{
		printf("cannot open /proc/stb/lcd/symbol_network (%m)\n");
		return;
	}	
	fprintf(f,"%i", net);
	fclose(f);
#endif	
}

void CVFD::vfd_symbol_circle(int cir)
{
	const char *VFDCIRCLE[] = {
		"OFF",
		"ON"
	};
	
	dprintf(DEBUG_NORMAL, "CVFD::vfd_symbol_circle: %s\n", VFDCIRCLE[cir]);
	
#if defined (PLATFORM_VENTON)	
	FILE *f;
	if((f = fopen("/proc/stb/lcd/symbol_circle","w")) == NULL) 
	{
		printf("cannotopen /proc/stb/lcd/symbol_circle (%m)\n");
	
		return;
	}
	fprintf(f,"%i", cir);
	fclose(f);
#endif	
}

void CVFD::setFPTime(void)
{
	if(!has_lcd)
		return;

#if defined (__sh__)
	openDevice();
	
	char timebuf[6];
	time_t tnow = time(NULL);
	
	tm *now = localtime(&tnow);
	timebuf[0] = 1;
	timebuf[1] = now->tm_min;
	timebuf[2] = now->tm_hour;
	timebuf[3] = now->tm_mday;
	timebuf[4] = now->tm_mon+1;
	timebuf[5] = now->tm_year % 100;
	
	struct vfd_ioctl_data data;
	memset(&data, 0, sizeof(struct vfd_ioctl_data));
	memcpy(&data, timebuf, 6); 
	
	if( ioctl(fd, VFDSETTIME, &data) < 0)  
		perror("VFDPWRLED");
	
	closeDevice();
#else
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
		
	proc_put("/proc/stb/fp/rtc", t + lt->tm_gmtoff);
#endif
}

