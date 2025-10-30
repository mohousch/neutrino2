//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: rcinput.cpp 30102025 mohousch Exp $
//
//	Homepage: http://dbox.cyberphoria.org/
//
//	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#include <driver/rcinput.h>

#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef ENABLE_LIRC
#include <linux/lirc.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>


const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {
	"/dev/input/event0", 
	"/dev/input/event1", 
	"/dev/input/event2", 
	"/dev/input/event3"
};

typedef struct input_event t_input_event;

#ifdef ENABLE_LIRC
__u64 lastScanCode = 0;
__u32 lastKeyCode = 0;
uint64_t FirstTime = 0;	
char keyName[30] = "";
int count;
#endif

bool CRCInput::loadRCConfig(const char * const fileName)
{
	printf("CRCInput::loadRCConfig:\n");
	
	if(!configfile.loadConfig(fileName))
		printf("CRCInput::loadRCConfig: %s not found, using default\n", fileName);

	key_0 = configfile.getInt32("key_0", KEY_0);
	key_1 = configfile.getInt32("key_1", KEY_1);
	key_2 = configfile.getInt32("key_2", KEY_2);
	key_3 = configfile.getInt32("key_3", KEY_3);
	key_4 = configfile.getInt32("key_4", KEY_4);
	key_5 = configfile.getInt32("key_5", KEY_5);
	key_6 = configfile.getInt32("key_6", KEY_6);
	key_7 = configfile.getInt32("key_7", KEY_7);
	key_8 = configfile.getInt32("key_8", KEY_8);
	key_9 = configfile.getInt32("key_9", KEY_9);

#ifdef USE_OPENGL			
	key_up = configfile.getInt32("key_up", 0x67);
	key_left = configfile.getInt32("key_left", 0x69);
	key_right = configfile.getInt32("key_right", 0x6a);
	key_down = configfile.getInt32("key_down", 0x6c);
#else
	key_up = configfile.getInt32("key_up", KEY_UP);
	key_left = configfile.getInt32("key_left", KEY_LEFT);
	key_right = configfile.getInt32("key_right", KEY_RIGHT);
	key_down = configfile.getInt32("key_down", KEY_DOWN);
#endif
			
	key_spkr = configfile.getInt32("key_spkr", KEY_MUTE);
	
#ifdef USE_OPENGL
	key_minus = configfile.getInt32("key_minus", 0x72);
	key_plus = configfile.getInt32("key_plus", 0x73);
#else			
	key_minus = configfile.getInt32("key_minus", KEY_VOLUMEDOWN);
	key_plus = configfile.getInt32("key_plus", KEY_VOLUMEUP);
#endif	

	key_standby = configfile.getInt32("key_standby", KEY_POWER);
			
#if defined (__sh__)
	key_home = configfile.getInt32("key_home", KEY_HOME);
#else
	key_home = configfile.getInt32("key_home", 0xAE);
#endif			

#if defined (PLATFORM_DGS)
	key_setup = configfile.getInt32("key_setup", 0x8B);
#else
	key_setup = configfile.getInt32("key_setup", KEY_MENU);
#endif				
			
	key_page_up = configfile.getInt32("key_page_up", 0x192);
	key_page_down = configfile.getInt32("key_page_down", 0x193);	
			
	key_ok = configfile.getInt32("key_ok", KEY_OK);
			
	key_red = configfile.getInt32("key_red", KEY_RED);
	key_green = configfile.getInt32("key_green", KEY_GREEN);
	key_yellow = configfile.getInt32("key_yellow", KEY_YELLOW);
	key_blue	= configfile.getInt32("key_blue", KEY_BLUE);

#if defined (PLATFORM_DGS)
	key_audio = configfile.getInt32("key_audio", 0x188);
	key_video = configfile.getInt32("key_video", 0xE2);		
	key_text = configfile.getInt32("key_text", 0x184);
#else
	key_audio = configfile.getInt32("key_audio", KEY_AUDIO);
	key_video = configfile.getInt32("key_video", KEY_VIDEO);		
	key_text = configfile.getInt32("key_text", KEY_TEXT);
#endif

#if defined (PLATFORM_DGS)
	key_info = configfile.getInt32("key_info", 0x166);
#elif defined (PLATFORM_UFC960)
	key_info = configfile.getInt32("key_info", 0x8A);	
#else
	key_info = configfile.getInt32("key_info", KEY_INFO);
#endif			
			
#if defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)
	key_epg = configfile.getInt32("key_epg", 0x16D);
#else			
	key_epg = configfile.getInt32("key_epg", KEY_EPG);
#endif			

#if defined (PLATFORM_DGS)
	key_recall = configfile.getInt32("key_recall", 0x19C);
#else
	key_recall = configfile.getInt32("key_recall", KEY_BACK);
#endif			

	key_favorites = configfile.getInt32("key_favorites", KEY_FAVORITES);
	key_sat = configfile.getInt32("key_sat", KEY_SAT);
	key_help = configfile.getInt32("key_help", KEY_HELP);
	key_timer = configfile.getInt32("key_timer", 0x16A);
			
	key_record = configfile.getInt32("key_record", KEY_RECORD);
#if defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)	
	key_play = configfile.getInt32("key_play", 0xA4);
	key_pause = configfile.getInt32("key_pause", 0x16A);
#else
#ifdef USE_OPENGL
	key_play = configfile.getInt32("key_play", 0xcf);
#else
	key_play = configfile.getInt32("key_play", KEY_PLAY);
#endif
	key_pause = configfile.getInt32("key_pause", KEY_PAUSE);
#endif	
	key_forward = configfile.getInt32("key_forward", KEY_FASTFORWARD);
	key_rewind = configfile.getInt32("key_rewind", KEY_REWIND);
	key_stop = configfile.getInt32("key_stop", KEY_STOP);
	key_timeshift = configfile.getInt32("key_timeshift", KEY_TIME);
			
#if defined (__sh__)
	key_mode = configfile.getInt32("key_mode", KEY_MODE);
#else
	key_mode = configfile.getInt32("key_mode", 0x179);
#endif			

#if defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)
	key_next = configfile.getInt32("key_next", 0x197);
	key_prev = configfile.getInt32("key_prev", 0x19C);
#else
	key_next = configfile.getInt32("key_next", KEY_NEXT /*0xFFFFFFF0*/);
	key_prev = configfile.getInt32("key_prev", KEY_PREVIOUS /*0xFFFFFFF1*/);
#endif			

	//
	key_music = configfile.getInt32("key_music", 0x3F );
	key_picture = configfile.getInt32("key_picture", 0x169 );
	key_pvr = configfile.getInt32("key_pvr", KEY_PVR);
	key_media = configfile.getInt32("key_media", KEY_MEDIA);
			
	key_repeat = configfile.getInt32("key_repeat", 0x81);
	key_slow = configfile.getInt32("key_slow", 0x199 );
			
	key_dvbsub = configfile.getInt32("key_dvbsub", KEY_DVBSUB);

	key_pip = configfile.getInt32("key_pip", KEY_PIP);
	key_pippos = configfile.getInt32("key_pippos", 0x175);
	key_pipswap = configfile.getInt32("key_pipswap", 0X9E);
	key_pipsubch = configfile.getInt32("key_pipsubch", 0x188);

	key_net = configfile.getInt32("key_net", KEY_NET);
	
#if defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)
	key_bookmark = configfile.getInt32("key_bookmark", 0x169);
#else
	key_bookmark = configfile.getInt32("key_bookmark", 0x9C);
#endif	

#if defined (PLATFORM_DGS)
	key_multifeed = configfile.getInt32("key_multifeed", 0x42);
#elif defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)
	key_multifeed = configfile.getInt32("key_multifeed", 0x172);
#else
	key_multifeed = configfile.getInt32("key_multifeed", 0x165);
#endif
	
	//			
	key_f1 = configfile.getInt32("key_f1", 0x3B);
	key_f2 = configfile.getInt32("key_f2", 0x3C);
	key_f3 = configfile.getInt32("key_f3", 0x3D);
	key_f4 = configfile.getInt32("key_f4", 0x3E);
	
	//		
	key_vfdup = configfile.getInt32("key_vfdup", VFD_UP);
	key_vfddown = configfile.getInt32("key_vfddown", VFD_DOWN);
	key_vfdright = configfile.getInt32("key_vfdright", VFD_RIGHT);
	key_vfdleft = configfile.getInt32("key_vfdleft", VFD_LEFT);
	key_vfdpower = configfile.getInt32("key_vfdpower", VFD_POWER);
	key_vfdmenu = configfile.getInt32("key_vfdmenu", VFD_MENU);
	key_vfdexit = configfile.getInt32("key_vfdexit", VFD_EXIT);
	key_vfdok = configfile.getInt32("key_vfdok", VFD_OK);
	
	return true;
}

bool CRCInput::saveRCConfig(const char * const fileName)
{
	printf("CRCInput::saveRCConfig:\n");
	
	configfile.setInt32("key_0", key_0);
	configfile.setInt32("key_1", key_1);
	configfile.setInt32("key_2", key_2);
	configfile.setInt32("key_3", key_3);
	configfile.setInt32("key_4", key_4);
	configfile.setInt32("key_5", key_5);
	configfile.setInt32("key_6", key_6);
	configfile.setInt32("key_7", key_7);
	configfile.setInt32("key_8", key_8);
	configfile.setInt32("key_9", key_9);
			
	configfile.setInt32("key_up", key_up);
	configfile.setInt32("key_left", key_left);
	configfile.setInt32("key_right", key_right);
	configfile.setInt32("key_down", key_down);
			
	configfile.setInt32("key_spkr", key_spkr);
			
	configfile.setInt32("key_minus", key_minus);
	configfile.setInt32("key_plus", key_plus);		

	configfile.setInt32("key_standby", key_standby);
			
	configfile.setInt32("key_home", key_home);
			
	configfile.setInt32("key_setup", key_setup);
				
	configfile.setInt32("key_page_up", key_page_up);
	configfile.setInt32("key_page_down", key_page_down);
					
	configfile.setInt32("key_ok", key_ok);
			
	configfile.setInt32("key_red", key_red);
	configfile.setInt32("key_green", key_green);
	configfile.setInt32("key_yellow", key_yellow);
	configfile.setInt32("key_blue", key_blue);

	configfile.setInt32("key_audio", key_audio);
	configfile.setInt32("key_video", key_video);		
	configfile.setInt32("key_text", key_text);

	configfile.setInt32("key_info", key_info);
				
	configfile.setInt32("key_epg", key_epg);
			
	configfile.setInt32("key_recall", key_recall);	

	configfile.setInt32("key_favorites", key_favorites);
	configfile.setInt32("key_sat", key_sat);
	configfile.setInt32("key_help", key_help);
	configfile.setInt32("key_timer", key_timer);
			
	configfile.setInt32("key_record", key_record);
	configfile.setInt32("key_play", key_play);
	configfile.setInt32("key_pause", key_pause);
	configfile.setInt32("key_forward", key_forward);
	configfile.setInt32("key_rewind", key_rewind);
	configfile.setInt32("key_stop", key_stop);
	configfile.setInt32("key_timeshift", key_timeshift);
					
	configfile.setInt32("key_mode", key_mode);
			
	configfile.setInt32("key_next", key_next);
	configfile.setInt32("key_prev", key_prev);
			
	configfile.setInt32("key_music", key_music);
	configfile.setInt32("key_picture", key_picture);
	configfile.setInt32("key_pvr", key_pvr);
	configfile.setInt32("key_media", key_media);
			
	configfile.setInt32("key_repeat", key_repeat);
	configfile.setInt32("key_slow", key_slow);
			
	configfile.setInt32("key_dvbsub", key_dvbsub);

	configfile.setInt32("key_pip", key_pip);
	configfile.setInt32("key_pippos", key_pippos);
	configfile.setInt32("key_pipswap", key_pipswap);
	configfile.setInt32("key_pipsubch", key_pipsubch);

	configfile.setInt32("key_net", key_net);
			
	configfile.setInt32("key_bookmark", key_bookmark);

	configfile.setInt32("key_multifeed", key_multifeed);
			
	configfile.setInt32("key_f1", key_f1);
	configfile.setInt32("key_f2", key_f2);
	configfile.setInt32("key_f3", key_f3);
	configfile.setInt32("key_f4", key_f4);
					
	configfile.setInt32("key_vfdup", key_vfdup);
	configfile.setInt32("key_vfddown", key_vfddown);
	configfile.setInt32("key_vfdright", key_vfdright);
	configfile.setInt32("key_vfdleft", key_vfdleft);
	configfile.setInt32("key_vfdpower", key_vfdpower);
	configfile.setInt32("key_vfdmenu", key_vfdmenu);
	configfile.setInt32("key_vfdexit", key_vfdexit);
	configfile.setInt32("key_vfdok", key_vfdok);
	
	if(configfile.getModifiedFlag())
	{
		configfile.saveConfig(fileName);
	}
	
	return true;
}

CRCInput::CRCInput() : configfile('\t')
{
	timerid = 1;

	// pipe_high
	if (pipe(fd_pipe_high_priority) < 0)
	{
		ng_err("CRCInput::CRCInput: fd_pipe_high_priority failed\n");
		exit(-1);
	}

	fcntl(fd_pipe_high_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_high_priority[1], F_SETFL, O_NONBLOCK );

	// pipe_low
	if (pipe(fd_pipe_low_priority) < 0)
	{
		ng_err("CRCInput::CRCInput: fd_pipe_low_priority failed\n");
		exit(-1);
	}

	fcntl(fd_pipe_low_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_low_priority[1], F_SETFL, O_NONBLOCK );

	// event devices
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		fd_rc[i] = -1;
	}
	
	//
	fd_lirc = -1;
	
	repeat_block = repeat_block_generic = 0;
	
	//load rcconfig
	if( !loadRCConfig(NEUTRINO_RCCONFIG_FILE) )
		printf("CRCInput::CRCInput: Loading of rc config file failed. Using defaults.\n");
		
	// lirc
#ifdef ENABLE_LIRC
#if HAVE_KERNEL_LIRC
	unsigned mode = LIRC_MODE_SCANCODE;
	
	fd_lirc = ::open("/dev/lirc0", O_RDONLY, 0);
	
	if(fd_lirc < 0)
	{
		ng_err("CRCInput::CRCInput: /dev/lirc0 failed\n");
	}

	if (::ioctl(fd_lirc, LIRC_SET_REC_MODE, &mode) < 0)
	{
		ng_err("CRCInput::CRCInput /dev/lirc0 failed\n");
	}
#else
	struct sockaddr_un  vAddr;
	
	vAddr.sun_family = AF_UNIX;
	
	if (access("/var/run/lirc/lircd", F_OK) == 0)
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	else
	{
		strcpy(vAddr.sun_path, "/dev/lircd");
	}
	
	fd_lirc = socket(AF_UNIX, SOCK_STREAM, 0);
	
	if(fd_lirc < 0)
	{
		ng_err("CRCInput::CRCInput lircd socket failed\n");
		fd_lirc = -1;
	}
	
	if (connect(fd_lirc, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		ng_err("CRCInput::CRCInput lircd connect failed\n");
		::close(fd_lirc);
		fd_lirc = -1;
	}
#endif
#endif

	//
	open();
	
	rc_last_key =  KEY_MAX;
}

void CRCInput::open()
{
	close();

	// 
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if ((fd_rc[i] = ::open(RC_EVENT_DEVICE[i], O_RDONLY)) == -1)
			perror(RC_EVENT_DEVICE[i]);
		else
		{
			fcntl(fd_rc[i], F_SETFL, O_NONBLOCK);
		}
				
		dprintf(DEBUG_INFO, "CRCInput::open: %s fd %d\n", RC_EVENT_DEVICE[i], fd_rc[i]);		
	}
	
	calculateMaxFd();
}

void CRCInput::close()
{
	// fd_rc
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++) 
	{
		if (fd_rc[i] != -1) 
		{
			::close(fd_rc[i]);
			fd_rc[i] = -1;
		}
	}

	calculateMaxFd();
}

void CRCInput::calculateMaxFd()
{
	fd_max = fd_lirc;

	//
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		if (fd_rc[i] > fd_max)
			fd_max = fd_rc[i];
	
	if(fd_pipe_high_priority[0] > fd_max)
		fd_max = fd_pipe_high_priority[0];
	
	if(fd_pipe_low_priority[0] > fd_max)
		fd_max = fd_pipe_low_priority[0];
}

CRCInput::~CRCInput()
{
	//
	close();

	//
	if(fd_pipe_high_priority[0])
		::close(fd_pipe_high_priority[0]);
	
	if(fd_pipe_high_priority[1])
		::close(fd_pipe_high_priority[1]);

	//
	if(fd_pipe_low_priority[0])
		::close(fd_pipe_low_priority[0]);
	
	if(fd_pipe_low_priority[1])
		::close(fd_pipe_low_priority[1]);
		
	//
#ifdef ENABLE_LIRC
	::close(fd_lirc);
#endif
}

void CRCInput::stopInput()
{
	close();
}

void CRCInput::restartInput()
{
	close();
	open();
}

uint32_t CRCInput::addTimer(uint64_t Interval, bool oneshot, bool correct_time )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	uint64_t timeNow = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	timer _newtimer;
	
	if (!oneshot)
		_newtimer.interval = Interval;
	else
		_newtimer.interval = 0;

	_newtimer.id = timerid++;
	
	if ( correct_time )
		_newtimer.times_out = timeNow + Interval;
	else
		_newtimer.times_out = Interval;

	_newtimer.correct_time = correct_time;

	std::vector<timer>::iterator e;
	for ( e = timers.begin(); e != timers.end(); ++e )
	{
		if ( e->times_out > _newtimer.times_out )
			break;
	}

	timers.insert(e, _newtimer);

	return _newtimer.id;
}

uint32_t CRCInput::addTimer(struct timeval Timeout)
{
	uint64_t timesout = (uint64_t) Timeout.tv_usec + (uint64_t)((uint64_t) Timeout.tv_sec * (uint64_t) 1000000);

	return addTimer( timesout, true, false );
}

uint32_t CRCInput::addTimer(const time_t *Timeout)
{
	return addTimer( (uint64_t)*Timeout*(uint64_t)1000000, true, false );
}

void CRCInput::killTimer(uint32_t id)
{
	dprintf(DEBUG_DEBUG, "CRCInput::killTimer: killing timer %d\n", id);
	
	std::vector<timer>::iterator e;
	
	for ( e = timers.begin(); e != timers.end(); ++e )
	{
		if ( e->id == id )
		{
			timers.erase(e);
			break;
		}
	}
}

uint32_t CRCInput::checkTimers()
{
	struct timeval tv;
	uint32_t _id = 0;

	gettimeofday( &tv, NULL );
	uint64_t timeNow = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	std::vector<timer>::iterator e;
	
	for ( e = timers.begin(); e != timers.end(); ++e )
	{
		if ( e->times_out < timeNow + 2000 )
		{
			_id = e->id;
			
			if ( e->interval != 0 )
			{
				timer _newtimer;
				_newtimer.id = e->id;
				_newtimer.interval = e->interval;
				_newtimer.correct_time = e->correct_time;
				
				if ( _newtimer.correct_time )
					_newtimer.times_out = timeNow + e->interval;
				else
					_newtimer.times_out = e->times_out + e->interval;

		            	timers.erase(e);
		            	
				for ( e = timers.begin(); e != timers.end(); ++e )
				{
					if ( e->times_out > _newtimer.times_out )
						break;
				}

				timers.insert(e, _newtimer);
			}
			else
				timers.erase(e);

			break;
		}
	}

	return _id;
}

int64_t CRCInput::calcTimeoutEnd(const int timeout_in_seconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec + (uint64_t)timeout_in_seconds) * (uint64_t) 1000000;
}

int64_t CRCInput::calcTimeoutEnd_MS(const int timeout_in_milliseconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uint64_t timeNow = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	return ( timeNow + timeout_in_milliseconds * 1000 );
}

void CRCInput::getMsgAbsoluteTimeout(neutrino_msg_t * msg, neutrino_msg_data_t * data, uint64_t *TimeoutEnd, bool bAllowRepeatLR)
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	uint64_t timeNow = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	uint64_t diff;

	if ( *TimeoutEnd < timeNow + 100 )
		diff = 100;  // Minimum Differenz...
	else
		diff = ( *TimeoutEnd - timeNow );
	
	getMsg_us( msg, data, diff, bAllowRepeatLR );
}

void CRCInput::getMsg(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR)
{
	getMsg_us(msg, data, (uint64_t) Timeout * 1000 * 1000, bAllowRepeatLR);
}

void CRCInput::getMsg_ms(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR)
{
	getMsg_us(msg, data, (uint64_t) Timeout * 1000, bAllowRepeatLR);
}

void CRCInput::getMsg_us(neutrino_msg_t * msg, neutrino_msg_data_t * data, uint64_t Timeout, bool bAllowRepeatLR)
{
	static uint64_t last_keypress = 0ULL;
	uint64_t getKeyBegin;
	
	static __u16 rc_last_repeat_key = KEY_MAX;

	struct timeval tv, tvselect;
	uint64_t InitialTimeout = Timeout;
	int64_t targetTimeout;

	int timer_id;
	fd_set rfds;
	t_input_event ev;	

	*data = 0;

	// repeat till timeout
	gettimeofday( &tv, NULL );
	getKeyBegin = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	while(1) 
	{
		timer_id = 0;

		// check timers
		if ( timers.size()> 0 )
		{
			gettimeofday( &tv, NULL );
			uint64_t t_n = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);
			
			if ( timers[0].times_out < t_n )
			{
				timer_id = checkTimers();
				
				*msg = NeutrinoMessages::EVT_TIMER;
				*data = timer_id;
				
				return;
			}
			else
			{
				targetTimeout = timers[0].times_out - t_n;
				
				if ( (uint64_t) targetTimeout> Timeout)
					targetTimeout = Timeout;
				else
					timer_id = timers[0].id;
			}
		}
		else
			targetTimeout = Timeout;

		tvselect.tv_sec = targetTimeout/1000000;
		tvselect.tv_usec = targetTimeout%1000000;

		FD_ZERO(&rfds);
		
		// lirc
#ifdef ENABLE_LIRC
		if (fd_lirc)
			FD_SET(fd_lirc, &rfds);
#endif
		// event devices
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if (fd_rc[i] != -1)
				FD_SET(fd_rc[i], &rfds);
		}
		
		//
		FD_SET(fd_pipe_high_priority[0], &rfds);
		FD_SET(fd_pipe_low_priority[0], &rfds);

		int status = select(fd_max + 1, &rfds, NULL, NULL, &tvselect);

		if ( status == -1 )
		{
			ng_err("CRCInput::getMsg_us select failed\n");
			
			// in case of an error return timeout...?!
			*msg = RC_timeout;
			*data = 0;
			
			return;
		}
		else if ( status == 0 ) // Timeout!
		{
			if ( timer_id != 0 )
			{
				timer_id = checkTimers();
				
				if ( timer_id != 0 )
				{
					*msg = NeutrinoMessages::EVT_TIMER;
					*data = timer_id;
					
					return;
				}
				else
					continue;
			}
			else
			{
				*msg = RC_timeout;
				*data = 0;
				
				return;
			}
		}
		
		// fd_pipe_high
		if(FD_ISSET(fd_pipe_high_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_high_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us:got event from high-pri pipe msg=(0x%llx) data:(0x%llx) <\n", *msg, *data );

			return;
		}

		// fd_rc
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++) 
		{
			if ((fd_rc[i] != -1) && (FD_ISSET(fd_rc[i], &rfds))) 
			{
				int ret;

				ret = read(fd_rc[i], &ev, sizeof(t_input_event));

				if(ret != sizeof(t_input_event)) 
				{
					continue;
				}
								
				dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us:got event from device type: 0x%X key: 0x%X value %d, translate: 0x%X -%s <\n", ev.type, ev.code, ev.value, translate(ev.code, i), getKeyName(translate(ev.code, i)).c_str() );
				
				if (ev.type != EV_KEY)
					continue;

				uint32_t trkey = translate(ev.code, i);

				if (trkey == RC_nokey) 
					continue;
				
				if (ev.value) 
				{
					uint64_t now_pressed;
					bool keyok = true;

					tv = ev.time;
					now_pressed = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);
					
					//
					if (ev.code == rc_last_key)
					{
						if((trkey == RC_up) || (trkey == RC_down   ) ||
							(trkey == RC_plus   ) || (trkey == RC_minus  ) ||
							(trkey == RC_page_down   ) || (trkey == RC_page_up  ) ||
							((bAllowRepeatLR) && ((trkey == RC_left ) ||
								(trkey == RC_right))))
						{
							if (rc_last_repeat_key != ev.code) 
							{
								if ((now_pressed > last_keypress + repeat_block) ||
										(now_pressed < last_keypress)) 
									rc_last_repeat_key = ev.code;
								else
									keyok = false;
							}
						}
						else
							keyok = false;						
					}
					else
						rc_last_repeat_key = KEY_MAX;

					rc_last_key = ev.code;

					if (keyok) 
					{
						if ((now_pressed > last_keypress + repeat_block_generic) || (now_pressed < last_keypress)) 
						{
							last_keypress = now_pressed;

							*msg = trkey;
							*data = 0; // <- button pressed

							return;
						}						
					} //if keyok
				} // if (ev.value)
				else 
				{
					// clear rc_last_key on keyup event
					rc_last_key = KEY_MAX;
					
					if (trkey == RC_standby) 
					{
						*msg = RC_standby;
						*data = 1; // <- button released
						
						return;
					}
				}
			}// if FDSET
		} // for NUMBER_OF_EVENT_DEVICES

		// fd_lirc
#ifdef ENABLE_LIRC
#if HAVE_KERNEL_LIRC
		if (fd_lirc != -1 && (FD_ISSET(fd_lirc, &rfds)))
		{
			ssize_t ret;
			lirc_scancode lircdata;
			
			ret = read(fd_lirc, &lircdata, sizeof(lircdata));
			
			if (ret != sizeof(lircdata))
			{
				continue;
			}
			
			if (lircdata.keycode == 0)
			{
				// skip keys coming in too fast
           			if ( (lircdata.scancode == lastScanCode) && ((lircdata.timestamp - FirstTime) / 1000000) < repeat_block/1000)
              				continue;
              			
              			//
              			dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us: got event from LIRC: timestamp:%lld flags:%d proto:%d keycode: 0x%x scancode:%llx (%s) timestampdiff:%d repeat_block:%d <\n", lircdata.timestamp, lircdata.flags, lircdata.rc_proto, lircdata.keycode, lircdata.scancode, getSpecialKeyName(translate(lircdata.scancode, 0)), (lircdata.timestamp - FirstTime) / 1000000, repeat_block/1000);
              		
              			FirstTime = lircdata.timestamp;
              			lastScanCode = lircdata.scancode;
              			
              			*data = 0;
				*msg = translate(lastScanCode, 0);
              		}
              		else
              		{
              			// skip keys coming in too fast
           			if ( (lircdata.scancode == lastScanCode) && ((lircdata.timestamp - FirstTime) / 1000000) < repeat_block/1000)
              				continue;
              			
              			dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us: got event from LIRC: timestamp:%lld flags:%d proto:%d keycode: 0x%x scancode:%llx (%s) timestampdiff:%d repeat_block:%d <\n", lircdata.timestamp, lircdata.flags, lircdata.rc_proto, lircdata.keycode, lircdata.scancode, getSpecialKeyName(translate(lircdata.keycode, 0)), (lircdata.timestamp - FirstTime) / 1000000, repeat_block);
              		
              			FirstTime = lircdata.timestamp;
              			lastKeyCode = lircdata.keycode;
              			lastScanCode = lircdata.scancode;
              			
              			*data = 0;
				*msg = translate(lastKeyCode, 0);
              		}
			
			return;
		}
#else
		if (fd_lirc != -1 && (FD_ISSET(fd_lirc, &rfds)))
		{
			ssize_t ret;
			char vBuffer[128];
			
			memset(vBuffer, 0, 128);
			
			ret = ::read(fd_lirc, vBuffer, 128);
			
			if (sscanf(vBuffer, "%*x %x %29s", &count, keyName) != 2)  // '29' in '%29s' is LIRC_KEY_BUF-1!
			{
				continue;
			}
			
			if (count == 0)
			{
				dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us: got event from LIRC:keyName:%s <\n", keyName);
				
				// translate keyName to RC_key
				*msg = translateKey(keyName);
			}
			
			return;
		}
#endif
#endif

		// pipe low prio
		if(FD_ISSET(fd_pipe_low_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_low_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			dprintf(DEBUG_NORMAL, ANSI_RED"CRCInput::getMsg_us: got event from low-pri pipe msg=(0x%llx) data:(0x%llx) <\n", *msg, *data );

			return;
		}

		if ( InitialTimeout == 0 )
		{
			*msg = RC_timeout;
			*data = 0;
			
			return;
		}
		else
		{
			// recalculate timeout
			gettimeofday( &tv, NULL );
			int64_t getKeyNow = (int64_t) tv.tv_usec + (int64_t)((int64_t) tv.tv_sec * (int64_t) 1000000);
			int64_t diff = (getKeyNow - getKeyBegin);
			
			if( Timeout <= (uint64_t) diff )
			{
				*msg = RC_timeout;
				*data = 0;
				return;
			}
			else
				Timeout -= diff;
		}
	}
}

void CRCInput::setRepeat(unsigned int delay,unsigned int period)
{
	dprintf(DEBUG_NORMAL, "CRCInput::setRepeat: %d %d\n", delay, period);
	
	repeat_block = delay * 1000ULL;
	repeat_block_generic = period * 1000ULL;

	struct input_event ie;
	
	// delay
	ie.type = EV_REP;
	ie.code = REP_DELAY;
	ie.value = delay;
	
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			write(fd_rc[i], &ie, sizeof(ie));
		}
	}

	// period
	ie.code = REP_PERIOD;
	ie.value = period;
	
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			write(fd_rc[i], &ie, sizeof(ie));
		}
	}
}

void CRCInput::postMsg(const neutrino_msg_t msg, const neutrino_msg_data_t data, const bool Priority)
{
	dprintf(DEBUG_NORMAL, ANSI_RED "CRCInput::postMsg: msg:(0x%llx) data:(0x%llx) >\n", msg, data);

	struct event buf;
	
	buf.msg  = msg;
	buf.data = data;

	if (Priority)
		write(fd_pipe_high_priority[1], &buf, sizeof(buf));
	else
		write(fd_pipe_low_priority[1], &buf, sizeof(buf));
}

void CRCInput::clearRCMsg()
{
	t_input_event ev;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			while (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
				;
		}
	}
	
	rc_last_key =  KEY_MAX;
}

// isNumeric - test if key is 0..9
bool CRCInput::isNumeric(const neutrino_msg_t key)
{
	return ((key == RC_0) || ((key >= RC_1) && (key <= RC_9)));
}

// getNumericValue - return numeric value of the key or -1
int CRCInput::getNumericValue(const neutrino_msg_t key)
{
	return ((key == RC_0) ? (int)0 : (((key >= RC_1) && (key <= RC_9)) ? (int)(key - RC_1 + 1) : (int)-1));
}

// convertDigitToKey - return key representing digit or RC_nokey
static const unsigned int digit_to_key[10] = 
{
	CRCInput::RC_0, 
	CRCInput::RC_1, 
	CRCInput::RC_2, 
	CRCInput::RC_3, 
	CRCInput::RC_4, 
	CRCInput::RC_5, 
	CRCInput::RC_6, 
	CRCInput::RC_7, 
	CRCInput::RC_8, 
	CRCInput::RC_9
};

unsigned long CRCInput::convertDigitToKey(const unsigned int digit)
{
	return (digit < 10) ? digit_to_key[digit] : RC_nokey;
}

// getUnicodeValue - return unicode value of the key or -1
#define UNICODE_VALUE_SIZE 58
static const int unicode_value[UNICODE_VALUE_SIZE] = {-1 , -1 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', -1 , -1 ,
						      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', -1 , -1 , 'A', 'S',
						      'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', -1 /* FIXME */, -1 /* FIXME */, -1 , '\\', 'Z', 'X', 'C', 'V',
						      'B', 'N', 'M', ',', '.', '/', -1, -1, -1, ' '};

int CRCInput::getUnicodeValue(const neutrino_msg_t key)
{
	if (key < UNICODE_VALUE_SIZE)
		return unicode_value[key];
	else
		return -1;
}

// transforms the rc-key to const char
const char * CRCInput::getSpecialKeyName(const unsigned long key)
{
	switch(key)
	{
		case RC_standby:
			return "RC_standby";
				
		case RC_home:
			return "RC_home";

		case RC_setup:
			return "RC_setup";

		case RC_red:
			return "RC_red";
				
		case RC_green:
			return "RC_green";
				
		case RC_yellow:
			return "RC_yellow";
				
		case RC_blue:
			return "RC_blue";
				
		case RC_page_up:
			return "RC_page_up";
				
		case RC_page_down:
			return "RC_page_down";
				
		case RC_up:
			return "RC_up";
				
		case RC_down:
			return "RC_down";
				
		case RC_left:
			return "RC_left";
				
		case RC_right:
			return "RC_right";
				
		case RC_ok:
			return "RC_ok";
				
		case RC_plus:
			return "RC_plus";
				
		case RC_minus:
			return "RC_minus";
				
		case RC_spkr:
			return "RC_spkr";
				
		case RC_info:
			return "RC_info";

		case RC_audio:
			return "RC_audio";
				
		case RC_video:
			return "RC_video";
		
		case RC_picture:
			return "RC_picture";
				
		case RC_music:
			return "RC_music";
			
		case RC_pvr:
			return "RC_pvr";
			
		case RC_media:
			return "RC_media";			

		case RC_text:
			return "RC_text";

		case RC_epg:
			return "RC_epg";
				
		case RC_recall:
			return "RC_recall";
				
		case RC_favorites:
			return "RC_favorites";
				
		case RC_sat:
			return "RC_sat";
			
		case RC_help:
			return "RC_help";
				
		case RC_play:
			return "RC_play";
				
		case RC_stop:
			return "RC_stop";
				
		case RC_forward:
			return "RC_forward";
				
		case RC_rewind:
			return "RC_rewind";
				
		case RC_timeshift:
			return "RC_timeshift";

		case RC_mode:
			return "RC_mode";
				
		case RC_record:
			return "RC_record";
				
		case RC_pause:
			return "RC_pause";
				
		case RC_loop:
			return "RC_loop";
				
		case RC_slow:
			return "RC_slow";			

		case RC_next:
			return "RC_next";
				
		case RC_prev:
			return "RC_prev";

		case RC_dvbsub:
			return "RC_dvbsub";
				
		case RC_pip:
			return "RC_pip";
				
		case RC_pippos:
			return "RC_pippos";
				
		case RC_pipswap:
			return "RC_pipswap";
				
		case RC_pipsubch:
			return "RC_pipsubch";
				
		case RC_bookmark:
			return "RC_bookmark";
				
		case RC_net:
			return "RC_net";

		case RC_multifeed:
			return "RC_multifeed";
			
		case RC_f1:
			return "RC_f1";
					
		case RC_f2:
			return "RC_f2";
			
		case RC_f3:
			return "RC_f3";
			
		case RC_f4:
			return "RC_f4";			
				
		//
		case RC_vfdup:
			return "RC_vfdup";
				
		case RC_vfddown:
			return "RC_vfddown";
				
		case RC_vfdright:
			return "RC_vfdright";
				
		case RC_vfdleft:
			return "RC_vfdleft";
				
		case RC_vfdpower:
			return "RC_vfdpower";
				
		case RC_vfdmenu:
			return "RC_vfdmenu";
				
		case RC_vfdexit:
			return "RC_vfdexit";
				
		case RC_vfdok:
			return "RC_vfdok";

		case RC_timeout:
			return "RC_timeout";

		case RC_nokey:
			return "RC_none";
			
		default:
			return "RC_unknown";
	}
}

std::string CRCInput::getKeyName(const unsigned long key)
{
	int unicode_value = getUnicodeValue(key);
	
	if (unicode_value == -1)
		return getSpecialKeyName(key);
	else
	{
		char tmp[2];
		tmp[0] = unicode_value;
		tmp[1] = 0;
		
		return std::string(tmp);
	}
}

// transforms the rc-key to generic - internal use only!
int CRCInput::translate(uint64_t code, int num)
{
	// common
	if (code == key_standby) return RC_standby;
	else if (code == key_spkr) return RC_spkr;
			
	else if (code == key_mode) return RC_mode;
	else if (code == key_net) return RC_net;
					
	else if (code == key_page_up) return RC_page_up;
	else if (code == key_page_down) return RC_page_down;
	else if (code == key_plus) return RC_plus;
	else if (code == key_minus) return RC_minus;
	
	else if (code == key_setup) return RC_setup;
	else if (code == key_left) return RC_left;
	else if (code == key_up) return RC_up;
	else if (code == key_ok) return RC_ok;
	else if (code == key_down) return RC_down;
	else if (code == key_right) return RC_right;
	else if (code == key_home) return RC_home;
			
	// special
	else if (code == key_recall) return RC_recall;
	else if (code == key_info) return RC_info;
	else if (code == key_bookmark) return RC_bookmark;
	else if (code == key_help) return RC_help;
	else if (code == key_timer) return RC_timer;
	
	// colored
	else if (code == key_red) return RC_red;
	else if (code == key_green) return RC_green;
	else if (code == key_yellow) return RC_yellow;
	else if (code == key_blue) return RC_blue;
	
	// alphanumeric
	else if (code == key_1) return RC_1;
	else if (code == key_2) return RC_2;
	else if (code == key_3) return RC_3;
	else if (code == key_4) return RC_4;
	else if (code == key_5) return RC_5;
	else if (code == key_6) return RC_6;
	else if (code == key_7) return RC_7;
	else if (code == key_8) return RC_8;
	else if (code == key_9) return RC_9;
	else if (code == key_0) return RC_0;
	
	// media
	else if (code == key_video) return RC_video;
	else if (code == key_music) return RC_music;
	else if (code == key_picture) return RC_picture;
	else if (code == key_pvr) return RC_pvr;
	else if (code == key_media) return RC_media;		
	else if (code == key_record) return RC_record;
	else if (code == key_rewind) return RC_rewind;
	else if (code == key_play) return RC_play;
	else if (code == key_forward) return RC_forward;
	else if (code == key_repeat) return RC_loop;
	else if (code == key_slow) return RC_slow;
	else if (code == key_stop) return RC_stop;
	else if (code == key_pause) return RC_pause;
	else if (code == key_audio) return RC_audio;
	else if (code == key_next) return RC_next;
	else if (code == key_prev) return RC_prev;
	else if (code == key_timeshift) return RC_timeshift;
	
	// dvb 
	else if (code == key_sat) return RC_sat;
	else if (code == key_favorites) return RC_favorites;
	else if (code == key_multifeed) return RC_multifeed;
	else if (code == key_dvbsub) return RC_dvbsub;
	else if (code == key_text) return RC_text;
	else if (code == key_epg) return RC_epg;
	else if (code == key_pip) return RC_pip;
	else if (code == key_pippos) return RC_pippos;
	else if (code == key_pipswap) return RC_pipswap;
	else if (code == key_pipsubch) return RC_pipsubch;
	
	// functions
	else if (code == key_f1) return RC_f1;
	else if (code == key_f2) return RC_f2;
	else if (code == key_f3) return RC_f3;
	else if (code == key_f4) return RC_f4;
	
	// frontpanel
	else if (code == key_vfdpower) return RC_standby;
	else if (code == key_vfdmenu) return RC_setup;
	else if (code == key_vfdexit) return RC_home;
	else if (code == key_vfdok) return RC_ok;
	else if (code == key_vfdleft) return RC_left;
	else if (code == key_vfdright) return RC_right;
	else if (code == key_vfddown) return RC_down;
	else if (code == key_vfdup) return RC_up;	
	else return RC_nokey;
}

#ifdef ENABLE_LIRC
uint32_t CRCInput::translateKey(const char *name)
{
// FIXME:
	if (!strcmp(name, "KEY_OK")) return RC_ok;
	else if (!strcmp(name, "KEY_EXIT")) return RC_home;
	else if (!strcmp(name, "KEY_DOWN")) return RC_down;
	else if (!strcmp(name, "KEY_UP")) return RC_up;
	else if (!strcmp(name, "KEY_RIGHT")) return RC_right;
	else if (!strcmp(name, "KEY_LEFT")) return RC_left;
	else if (!strcmp(name, "KEY_RED")) return RC_red;
	else if (!strcmp(name, "KEY_GREEN")) return RC_green;
	else if (!strcmp(name, "KEY_YELLOW")) return RC_yellow;
	else if (!strcmp(name, "KEY_BLUE")) return RC_blue;
	else if (!strcmp(name, "KEY_INFO")) return RC_info;
	else if (!strcmp(name, "KEY_EPG")) return RC_epg;
	else if (!strcmp(name, "KEY_MODE")) return RC_setup; //FIXME:
	else if (!strcmp(name, "KEY_RECORD")) return RC_record;
	else if (!strcmp(name, "KEY_PLAY")) return RC_play;
	else if (!strcmp(name, "KEY_STOP")) return RC_stop;
	else if (!strcmp(name, "KEY_PAUSE")) return RC_pause;
	else if (!strcmp(name, "KEY_CHANNELDOWN")) return RC_page_down;
	else if (!strcmp(name, "KEY_CHANNELUP")) return RC_page_up;
	else if (!strcmp(name, "KEY_VOLUMEUP")) return RC_plus;
	else if (!strcmp(name, "KEY_VOLUMEDOWN")) return RC_minus;
	else if (!strcmp(name, "KEY_TEXT")) return RC_text;
	else if (!strcmp(name, "KEY_REWIND")) return RC_rewind;
	else if (!strcmp(name, "KEY_FORWARD")) return RC_forward;
	else if (!strcmp(name, "KEY_SHUFFLE")) return RC_loop;
	else if (!strcmp(name, "KEY_NUMERIC_0")) return RC_0;
	else if (!strcmp(name, "KEY_NUMERIC_1")) return RC_1;
	else if (!strcmp(name, "KEY_NUMERIC_2")) return RC_2;
	else if (!strcmp(name, "KEY_NUMERIC_3")) return RC_3;
	else if (!strcmp(name, "KEY_NUMERIC_4")) return RC_4;
	else if (!strcmp(name, "KEY_NUMERIC_5")) return RC_5;
	else if (!strcmp(name, "KEY_NUMERIC_6")) return RC_6;
	else if (!strcmp(name, "KEY_NUMERIC_7")) return RC_7;
	else if (!strcmp(name, "KEY_NUMERIC_8")) return RC_8;
	else if (!strcmp(name, "KEY_NUMERIC_9")) return RC_9;
	else if (!strcmp(name, "KEY_MUTE")) return RC_spkr;
	else if (!strcmp(name, "KEY_POWER")) return RC_standby;
	else return RC_nokey;
}
#endif

////
#define SMSKEY_TIMEOUT 2000

SMSKeyInput::SMSKeyInput()
{
	resetOldKey();
	m_timeout = SMSKEY_TIMEOUT;
}

unsigned char SMSKeyInput::handleMsg(const neutrino_msg_t msg)
{
	timeval keyTime;
	gettimeofday(&keyTime, NULL);
	bool timeoutNotReached = (keyTime.tv_sec*1000 + keyTime.tv_usec/1000 <= m_oldKeyTime.tv_sec*1000 + m_oldKeyTime.tv_usec/1000 + m_timeout);

	unsigned char key = 0;
	
	if(msg == CRCInput::RC_1)
	{
		key = '1';
	}
	
	if(msg == CRCInput::RC_2)
	{
		if(m_oldKey == 'a' && timeoutNotReached)
			key = 'b';
		else if(m_oldKey == 'b' && timeoutNotReached)
			key = 'c';
		else if(m_oldKey == 'c' && timeoutNotReached)
			key = '2';
		else
			key = 'a';
	}
	else if(msg == CRCInput::RC_3)
	{
		if(m_oldKey == 'd' && timeoutNotReached)
			key = 'e';
		else if(m_oldKey == 'e' && timeoutNotReached)
			key = 'f';
		else if(m_oldKey == 'f' && timeoutNotReached)
			key = '3';
		else
			key = 'd';
	}
	else if(msg == CRCInput::RC_4)
	{
		if(m_oldKey == 'g' && timeoutNotReached)
			key = 'h';
		else if(m_oldKey == 'h' && timeoutNotReached)
			key = 'i';
		else if(m_oldKey == 'i' && timeoutNotReached)
			key = '4';
		else
			key = 'g';
	}
	else if(msg == CRCInput::RC_5)
	{
		if(m_oldKey == 'j' && timeoutNotReached)
			key = 'k';
		else if(m_oldKey == 'k' && timeoutNotReached)
			key = 'l';
		else if(m_oldKey == 'l' && timeoutNotReached)
			key = '5';
		else
			key = 'j';
	}
	else if(msg == CRCInput::RC_6)
	{
		if(m_oldKey == 'm' && timeoutNotReached)
			key = 'n';
		else if(m_oldKey == 'n' && timeoutNotReached)
			key = 'o';
		else if(m_oldKey == 'o' && timeoutNotReached)
			key = '6';
		else
			key = 'm';
	}
	else if(msg == CRCInput::RC_7)
	{
		if(m_oldKey == 'p' && timeoutNotReached)
			key = 'q';
		else if(m_oldKey == 'q' && timeoutNotReached)
			key = 'r';
		else if(m_oldKey == 'r' && timeoutNotReached)
			key = 's';
		else if(m_oldKey == 's' && timeoutNotReached)
			key = 's';
		else
			key = 'p';
	}
	else if(msg == CRCInput::RC_8)
	{
		if(m_oldKey == 't' && timeoutNotReached)
			key = 'u';
		else if(m_oldKey == 'u' && timeoutNotReached)
			key = 'v';
		else if(m_oldKey == 'v' && timeoutNotReached)
			key = '8';
		else
			key = 't';
	}
	else if(msg == CRCInput::RC_9)
	{
		if(m_oldKey == 'w' && timeoutNotReached)
			key = 'x';
		else if(m_oldKey == 'x' &&timeoutNotReached)
			key = 'y';
		else if(m_oldKey == 'y' &&timeoutNotReached)
			key = 'z';
		else if(m_oldKey == 'z' && timeoutNotReached)
			key = '9';
		else
			key = 'w';
	}
	else if(msg == CRCInput::RC_0)
	{
		key = '0';
	}
	
	m_oldKeyTime = keyTime;
	m_oldKey = key;
	
	return key;
}

void SMSKeyInput::resetOldKey()
{
	m_oldKeyTime.tv_sec = 0;
	m_oldKeyTime.tv_usec = 0;
	m_oldKey = 0;
}

unsigned char SMSKeyInput::getOldKey() const
{
	return m_oldKey;
}

const timeval* SMSKeyInput::getOldKeyTime() const
{
 	return &m_oldKeyTime;
}

time_t SMSKeyInput::getOldKeyTimeSec() const
{
	return m_oldKeyTime.tv_sec;
}


int SMSKeyInput::getTimeout() const
{
	return m_timeout;
}

void SMSKeyInput::setTimeout(int timeout)
{
	m_timeout = timeout;
}

