/*
	LCD-Daemon  -   DBoxII-Project
	
	$Id: vfd.h 2013/10/12 mohousch Exp $

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

#ifndef __vfd__
#define __vfd__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <string>

#if defined (PLATFORM_COOLSTREAM)
#include <cs_vfd.h>
#endif


#if defined(PLATFORM_SPARK7162)
/* SAB Triple VFD Icons								*/
/* taken from aotom_main.h, with these numbers the Icons on Spark7162		*/
/* will work right ( hopefully all Models )					*/
typedef enum {
//----------------------------------11G-------------------------------------
	VFD_SPARK_PLAY_FASTBACKWARD = 1,		//11*16+1,
	VFD_SPARK_PLAY_HEAD = 2,
	VFD_SPARK_PLAY_LOG = 3,
	VFD_SPARK_PLAY_TAIL = 4,
	VFD_SPARK_PLAY_FASTFORWARD = 5,
	VFD_SPARK_PLAY_PAUSE = 6,
	VFD_SPARK_REC1 = 7,
	VFD_SPARK_MUTE = 8,
	VFD_SPARK_CYCLE = 9,
	VFD_SPARK_DUBI = 10,
	VFD_SPARK_CA = 11,
	VFD_SPARK_CI= 12,
	VFD_SPARK_USB = 13,
	VFD_SPARK_DOUBLESCREEN = 14,
	VFD_SPARK_REC2 = 15,
//----------------------------------12G-------------------------------------
	VFD_SPARK_HDD_A8 = 16,			//12*16+1,
	VFD_SPARK_HDD_A7 = 17,
	VFD_SPARK_HDD_A6 = 18,
	VFD_SPARK_HDD_A5 = 19,
	VFD_SPARK_HDD_A4 = 20,
	VFD_SPARK_HDD_A3 = 21,
	VFD_SPARK_HDD_FULL = 22,
	VFD_SPARK_HDD_A2 = 23,
	VFD_SPARK_HDD_A1 = 24,
	VFD_SPARK_MP3 = 25,
	VFD_SPARK_AC3 = 26,
	VFD_SPARK_TVMODE_LOG = 27,
	VFD_SPARK_AUDIO = 28,
	VFD_SPARK_ALERT = 29,
	VFD_SPARK_HDD_A9 = 30,	
//----------------------------------13G-------------------------------------
	VFD_SPARK_CLOCK_PM = 31,			//13*16+1,
	VFD_SPARK_CLOCK_AM = 32,
	VFD_SPARK_CLOCK = 33,
	VFD_SPARK_TIME_SECOND = 34,
	VFD_SPARK_DOT2 = 35,
	VFD_SPARK_STANDBY = 36,
	VFD_SPARK_TER = 37,
	VFD_SPARK_DISK_S3 = 38,
	VFD_SPARK_DISK_S2 = 39,
	VFD_SPARK_DISK_S1 = 40,
	VFD_SPARK_DISK_S0 = 41,
	VFD_SPARK_SAT = 42,
	VFD_SPARK_TIMESHIFT = 43,
	VFD_SPARK_DOT1 = 44,
	VFD_SPARK_CAB = 45,	
//----------------------------------end-------------------------------------
	SPARK_ICON_ALL = 46, 		/* with this number the aotom driver set all icons on (1) or off (0) */
	VFD_ICON_MAX
} spark7162_vfd_icon;

// neutrino common translate for spark7162
typedef enum
{
	VFD_ICON_MUTE		= VFD_SPARK_MUTE,
	VFD_ICON_DOLBY		= VFD_SPARK_DUBI,
	VFD_ICON_POWER		= VFD_SPARK_STANDBY,
	VFD_ICON_TIMESHIFT	= VFD_SPARK_REC1,
	VFD_ICON_TV		= VFD_SPARK_TVMODE_LOG,
	VFD_ICON_RADIO		= VFD_SPARK_AUDIO,
	VFD_ICON_HD		= VFD_SPARK_DOUBLESCREEN,
	VFD_ICON_1080P		= VFD_SPARK_TIME_SECOND,
	VFD_ICON_1080I		= VFD_SPARK_TIME_SECOND,
	VFD_ICON_720P		= VFD_SPARK_TIME_SECOND,
	VFD_ICON_480P		= VFD_SPARK_TIME_SECOND,
	VFD_ICON_480I		= VFD_SPARK_TIME_SECOND,
	VFD_ICON_USB		= VFD_SPARK_USB,
	VFD_ICON_MP3		= VFD_SPARK_MP3,
	VFD_ICON_PLAY		= VFD_SPARK_PLAY_LOG,
	VFD_ICON_PAUSE		= VFD_SPARK_PLAY_PAUSE,
	VFD_ICON_LOCK 		= VFD_SPARK_CA,
	VFD_ICON_AC3 		= VFD_SPARK_AC3,
	SPARK_HDD		= VFD_SPARK_HDD_A9,
	SPARK_HDD_A8		= VFD_SPARK_HDD_A8,
	SPARK_HDD_A7		= VFD_SPARK_HDD_A7,
	SPARK_HDD_A6		= VFD_SPARK_HDD_A6,
	SPARK_HDD_A5		= VFD_SPARK_HDD_A5,
	SPARK_HDD_A4		= VFD_SPARK_HDD_A4,
	SPARK_HDD_A3		= VFD_SPARK_HDD_A3,
	SPARK_HDD_FULL		= VFD_SPARK_HDD_FULL,
	SPARK_HDD_A2		= VFD_SPARK_HDD_A2,
	SPARK_HDD_A1		= VFD_SPARK_HDD_A1,
	VFD_ICON_CLOCK		= VFD_SPARK_CLOCK,
	VFD_ICON_STANDBY	= VFD_SPARK_CYCLE	
} vfd_icon;
#endif //spark7162

#if defined(PLATFORM_KATHREIN)
typedef enum
{
	vfd_910_usb = 1,
	vfd_910_hd = 2,
	vfd_910_hdd = 3,
	vfd_910_lock = 4,
	vfd_910_bt = 5,
	vfd_910_mp3 = 6,
	vfd_910_music = 7,
	vfd_910_dd = 8,
	vfd_910_mail = 9,
	vfd_910_mute = 10,
	vfd_910_play = 11,
	vfd_910_pause = 12,
	vfd_910_ff = 13,
	vfd_910_fr = 14,
	vfd_910_rec = 15,
	vfd_910_clock = 16
} ufs910_vfd_icon;

// neutrino common translate for ufs910  / ufs913
typedef enum
{
	VFD_ICON_MUTE		= vfd_910_mute,		// UFS910 hasn't all icons, therefore a value wich doesn't change anything
	VFD_ICON_DOLBY		= vfd_910_dd,
	VFD_ICON_POWER		= 17,
	VFD_ICON_TIMESHIFT	= vfd_910_rec,
	VFD_ICON_TV		= 17,
	VFD_ICON_RADIO		= vfd_910_music,
	VFD_ICON_HD		= vfd_910_hd,
	VFD_ICON_1080P		= 17,
	VFD_ICON_1080I		= 17,
	VFD_ICON_720P		= 17,
	VFD_ICON_480P		= 17,
	VFD_ICON_480I		= 17,
	VFD_ICON_USB		= vfd_910_usb,
	VFD_ICON_MP3		= vfd_910_mp3,
	VFD_ICON_PLAY		= vfd_910_play,
	VFD_ICON_PAUSE		= vfd_910_pause,
	VFD_ICON_LOCK 		= vfd_910_lock,
	VFD_ICON_HDD 		= vfd_910_hdd,	
	VFD_ICON_CLOCK		= vfd_910_clock
} vfd_icon;
#endif //ufs910, ufs913

#if !defined(PLATFORM_KATHREIN) && !defined(PLATFORM_SPARK7162) && !defined (PLATFORM_COOLSTREAM)
// duckbox
// token from micom
enum {
	ICON_MIN,             // 0x00
	ICON_STANDBY,
	ICON_SAT,
	ICON_REC,
	ICON_TIMESHIFT,
	ICON_TIMER,           // 0x05
	ICON_HD,
	ICON_USB,
	ICON_SCRAMBLED,
	ICON_DOLBY,
	ICON_MUTE,            // 0x0a
	ICON_TUNER1,
	ICON_TUNER2,
	ICON_MP3,
	ICON_REPEAT,
	ICON_Play,            // 0x0f
	ICON_TER,            
	ICON_FILE,
	ICON_480i,
	ICON_480p,
	ICON_576i,            // 0x14
	ICON_576p,
	ICON_720p,
	ICON_1080i,
	ICON_1080p,
	ICON_Play_1,          // 0x19 
	ICON_RADIO,   
	ICON_TV,      
	ICON_PAUSE,   
	ICON_MAX
};

// neutrino common
typedef enum
{
	VFD_ICON_MUTE		= ICON_MUTE,
	VFD_ICON_DOLBY		= ICON_DOLBY,
	VFD_ICON_POWER		= ICON_STANDBY,
	VFD_ICON_TIMESHIFT	= ICON_REC,
	VFD_ICON_TV		= ICON_TV,
	VFD_ICON_RADIO		= ICON_TV,
	VFD_ICON_HD		= ICON_HD,
	VFD_ICON_1080P		= ICON_1080p,
	VFD_ICON_1080I		= ICON_1080i,
	VFD_ICON_720P		= ICON_720p,
	VFD_ICON_480P		= ICON_480p,
	VFD_ICON_480I		= ICON_480i,
	VFD_ICON_USB		= ICON_USB,
	VFD_ICON_MP3		= ICON_MP3,
	VFD_ICON_PLAY		= ICON_Play,
	VFD_ICON_PAUSE		= ICON_PAUSE,
	VFD_ICON_LOCK 		= ICON_SCRAMBLED
} vfd_icon;
#endif  //common

#if defined (__sh__)
#define VFDBRIGHTNESS         0xc0425a03
#define VFDPWRLED             0xc0425a04 /* added by zeroone, also used in fp_control/global.h ; set PowerLed Brightness on HDBOX*/
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDCLEARICONS	      0xc0425af6
#define VFDSETRF              0xc0425af7
#define VFDSETFAN             0xc0425af8
#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc
#define VFDREBOOT	      0xc0425afd
#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff

#define VFDGETWAKEUPTIME      0xc0425b00
#define VFDGETVERSION         0xc0425b01
#define VFDSETDISPLAYTIME     0xc0425b02
#define VFDSETTIMEMODE        0xc0425b03
#define VFDDISPLAYCLR	      0xc0425b00

#if defined(PLATFORM_SPARK7162)
/* structs are needed for sending icons etc. to aotom 				*/
struct set_mode_s {
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct set_brightness_s {
	int level;
};

struct set_icon_s {
	int icon_nr;
	int on;
};

struct set_led_s {
	int led_nr;
	int on;
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s {
	char time[5];
};

struct set_time_s {
	char time[5];
};

struct aotom_ioctl_data {
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};
#elif defined (PLATFORM_SPARK)
/* ugly: this is copied from frontcontroller utility, but the driver
 * does not seem to provide userspace headers... :-( */


/* this setups the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s {
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct set_brightness_s {
	int level;
};

struct set_icon_s {
	int icon_nr;
	int on;
};

struct set_led_s {
	int led_nr;
	int on;
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s {
	char time[5];
};

struct set_time_s {
	char time[5];
};

struct aotom_ioctl_data {
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};

/* a strange way of defining ioctls... but anyway... */
#define VFDGETTIME      0xc0425afa
#define VFDSETTIME      0xc0425afb
#define VFDSTANDBY      0xc0425afc
#define VFDSETLED       0xc0425afe
#define VFDDISPLAYCHARS 0xc0425a00
#define VFDDISPLAYCLR   0xc0425b00
#define VFDSETMODE      0xc0425aff
#endif //Spark7162


struct vfd_ioctl_data {
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};
#endif


class CVFD
{
	public:

		enum MODES
		{
			MODE_TVRADIO,
			MODE_SCART,
			MODE_SHUTDOWN,
			MODE_STANDBY,
			MODE_MENU_UTF8,
			MODE_AUDIO,
			MODE_MOVIE,
			MODE_PIC
		};

		enum AUDIOMODES
		{
			AUDIO_MODE_PLAY,
			AUDIO_MODE_STOP,
			AUDIO_MODE_FF,
			AUDIO_MODE_PAUSE,
			AUDIO_MODE_REV
		};

		//0:off, 1:blue, 2:red, 3:purple (gigablue power button)
		enum LEDS
		{
			LED_OFF,
			LED_BLUE,
			LED_RED,
			LED_PURPLE
		};
		
		enum EPGMODE
		{
			EPGMODE_CHANNELNUMBER,
			EPGMODE_TIME
		};

	private:
		MODES			mode;

		std::string		servicename;
		int			serviceNum;
		char			volume;
		unsigned char		percentOver;
		bool			muted;
		bool			showclock;
		pthread_t		thrTime;
		int                     last_toggle_state_power;
		bool			clearClock;
		unsigned int            timeout_cnt;
		int fd;
		unsigned char brightness;
		char text[256];

		void wake_up();
		void count_down();

		CVFD();

		static void * TimeThread(void*);
		void setlcdparameter(int dimm, int power);

	public:

		~CVFD();
		bool has_lcd;	
		bool is4digits;	
		
		void setlcdparameter(void);

		static CVFD * getInstance();
		void init();
		void setMode(const MODES m, const char * const title = "");
		void showServicename(const std::string& name, const bool perform_wakeup = true, int pos = 0); // UTF-8
		void showTime(bool force = false);
		/** blocks for duration seconds */
		void showRCLock(int duration = 2);
		
		void showMenuText(const int position, const char * text, const int highlight = -1, const bool utf_encoded = false);
		void showAudioTrack(const std::string& artist, const std::string& title, const std::string& album, int pos);
		void showAudioPlayMode(AUDIOMODES m = AUDIO_MODE_PLAY);
	
		void setBrightness(int);
		int getBrightness();

		void setBrightnessStandby(int);
		int getBrightnessStandby();

		void setPower(int);
		int getPower();
		void togglePower(void);

		void setMuted(bool);

		void resume();
		void pause();
		void Lock();
		void Unlock();
		void Clear();
		void ClearIcons();
#if defined(PLATFORM_SPARK7162)
		void ShowDiskLevel();
#endif		
		void ShowIcon(vfd_icon icon, bool show);		
		void ShowText(const char * str);
		
		void setFan(bool enable);
		void setFPTime(void);
		
		// venton
		void vfd_symbol_network(int net);
		void vfd_symbol_circle(int cir);
		
#if defined (__sh__)
		void openDevice();
		void closeDevice();
#endif
};

#endif

