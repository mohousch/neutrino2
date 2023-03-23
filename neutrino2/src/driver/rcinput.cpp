/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: rcinput.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
                      2003 thegoodguy

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/rcinput.h>
#include <driver/stream2file.h>

#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>

#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
#include <termio.h>
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

#include <unistd.h>
#include <fcntl.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <eventserver.h>

#include <global.h>
#include <neutrino2.h>

#include <timerd/timerd.h>

#include <system/debug.h>


#define ENABLE_REPEAT_CHECK

const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {
	"/dev/input/event0", 
	"/dev/input/event1", 
	"/dev/input/event2", 
	"/dev/input/event3"
};

typedef struct input_event t_input_event;

#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
static struct termio orig_termio;
static bool saved_orig_termio = false;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

bool CRCInput::loadRCConfig(const char * const fileName)
{
	printf("CRCInput::loadRCConfig:\n");
	
	/* if rc.conf not exists load default */
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
			
	key_up = configfile.getInt32("key_up", KEY_UP);
	key_left = configfile.getInt32("key_left", KEY_LEFT);
	key_right = configfile.getInt32("key_right", KEY_RIGHT);
	key_down = configfile.getInt32("key_down", KEY_DOWN);
			
	key_spkr = configfile.getInt32("key_spkr", KEY_MUTE);
				
	key_minus = configfile.getInt32("key_minus", KEY_VOLUMEDOWN);
	key_plus = configfile.getInt32("key_plus", KEY_VOLUMEUP);	

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
			
	key_record = configfile.getInt32("key_record", KEY_RECORD);
#if defined (PLATFORM_GIGABLUE) || defined(PLATFORM_VUPLUS) || defined(PLATFORM_ODIN)	
	key_play = configfile.getInt32("key_play", 0xA4);
	key_pause = configfile.getInt32("key_pause", 0x16A);
#else
	key_play = configfile.getInt32("key_play", KEY_PLAY);
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
	key_next = configfile.getInt32("key_next", 0xFFFFFFF0);
	key_prev = configfile.getInt32("key_prev", 0xFFFFFFF1);
#endif			

	/* added from cuberevo3000hd so fix it please */
	key_music = configfile.getInt32("key_music", 0x3F );
	key_picture = configfile.getInt32("key_picture", 0x169 );
			
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
				
	key_f1 = configfile.getInt32("key_f1", 0x3B);
	key_f2 = configfile.getInt32("key_f2", 0x3C);
	key_f3 = configfile.getInt32("key_f3", 0x3D);
	key_f4 = configfile.getInt32("key_f4", 0x3E);
	
	//key_aspect = configfile.getInt32("key_aspect", 0x40);	
			
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
			
	/* added from cuberevo3000hd so fix it please */
	configfile.setInt32("key_music", key_music);
	configfile.setInt32("key_picture", key_picture);
			
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
	
	//configfile.setInt32("key_aspect", key_aspect);
					
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
		/* save neu configuration */
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
		perror("fd_pipe_high_priority");
		exit(-1);
	}

	fcntl(fd_pipe_high_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_high_priority[1], F_SETFL, O_NONBLOCK );

	// pipe_low
	if (pipe(fd_pipe_low_priority) < 0)
	{
		perror("fd_pipe_low_priority");
		exit(-1);
	}

	fcntl(fd_pipe_low_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_low_priority[1], F_SETFL, O_NONBLOCK );
	
	// pipe_event
	/*
	if (pipe(fd_pipe_event) < 0)
	{
		perror("fd_pipe_event");
		exit(-1);
	}

	fcntl(fd_pipe_event[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_event[1], F_SETFL, O_NONBLOCK );
	*/

	// open event-library
	fd_event = 0;

	// prepare neutrino server socket
	struct sockaddr_un servaddr;
	int    clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, NEUTRINO_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(NEUTRINO_UDS_NAME);

	if ((fd_event = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("CRCInput::CRCInput socket\n");
	}

	if ( bind(fd_event, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("CRCInput::CRCInput bind failed...\n");
		exit(-1);
	}

	if (listen(fd_event, 15) !=0)
	{
		perror("CRCInput::CRCInput listen failed...\n");
		exit(-1);
	}

	// open rc
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		fd_rc[i] = -1;
	}
	
	repeat_block = repeat_block_generic = 0;
	
	//load rcconfig
	if( !loadRCConfig(NEUTRINO_RCCONFIG_FILE) )
		printf("CRCInput::CRCInput: Loading of rc config file failed. Using defaults.\n");
	
	open();
	
	rc_last_key =  KEY_MAX;
}

void CRCInput::open()
{
	close();

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

#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	fd_keyb = STDIN_FILENO;
#else
	fd_keyb = 0;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
	 
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	::fcntl(fd_keyb, F_SETFL, O_NONBLOCK);

	struct termio new_termio;

	::ioctl(STDIN_FILENO, TCGETA, &orig_termio);

	saved_orig_termio      = true;

	new_termio             = orig_termio;
	new_termio.c_lflag    &= ~ICANON;
	//new_termio.c_lflag    &= ~(ICANON|ECHO);
	new_termio.c_cc[VMIN ] = 1;
	new_termio.c_cc[VTIME] = 0;

	::ioctl(STDIN_FILENO, TCSETA, &new_termio);
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
	
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
	
	// fd_kb
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	if (saved_orig_termio)
	{
		::ioctl(STDIN_FILENO, TCSETA, &orig_termio);
				
		dprintf(DEBUG_DEBUG, "CRCInput::close:Original terminal settings restored.\n");	
	}
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

	calculateMaxFd();
}

void CRCInput::calculateMaxFd()
{
	fd_max = fd_event;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		if (fd_rc[i] > fd_max)
			fd_max = fd_rc[i];
	
	if(fd_pipe_high_priority[0] > fd_max)
		fd_max = fd_pipe_high_priority[0];
	
	if(fd_pipe_low_priority[0] > fd_max)
		fd_max = fd_pipe_low_priority[0];
		
	/*
	if(fd_pipe_event[0] > fd_max)
		fd_max = fd_pipe_event[0];
	*/
}

CRCInput::~CRCInput()
{
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

	/*
	if(fd_pipe_event[0])
		::close(fd_pipe_event[0]);
	
	if(fd_pipe_event[1])
		::close(fd_pipe_event[1]);
	*/
		
	//
	if(fd_event)
		::close(fd_event);
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

int CRCInput::addTimer(uint64_t Interval, bool oneshot, bool correct_time )
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
		_newtimer.times_out = timeNow+ Interval;
	else
		_newtimer.times_out = Interval;

	_newtimer.correct_time = correct_time;

	std::vector<timer>::iterator e;
	for ( e = timers.begin(); e != timers.end(); ++e )
		if ( e->times_out > _newtimer.times_out )
			break;

	timers.insert(e, _newtimer);

	return _newtimer.id;
}

int CRCInput::addTimer(struct timeval Timeout)
{
	uint64_t timesout = (uint64_t) Timeout.tv_usec + (uint64_t)((uint64_t) Timeout.tv_sec * (uint64_t) 1000000);

	return addTimer( timesout, true, false );
}

int CRCInput::addTimer(const time_t *Timeout)
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

int CRCInput::checkTimers()
{
	struct timeval tv;
	int _id = 0;

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
		            	
				for ( e= timers.begin(); e!= timers.end(); ++e )
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

	if ( *msg == NeutrinoMessages::EVT_TIMESET )
	{
		// recalculate timeout....
		*TimeoutEnd = *TimeoutEnd + *(int64_t*) *data;
	}
}

void CRCInput::getMsg(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR)
{
	getMsg_us(msg, data, (uint64_t) Timeout * 100 * 1000, bAllowRepeatLR);
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

	// wiederholung reinmachen - dass wirklich die ganze zeit bis timeout gewartet wird!
	gettimeofday( &tv, NULL );
	getKeyBegin = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);

	while(1) 
	{
		timer_id = 0;

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
					targetTimeout= Timeout;
				else
					timer_id = timers[0].id;
			}
		}
		else
			targetTimeout = Timeout;

		tvselect.tv_sec = targetTimeout/1000000;
		tvselect.tv_usec = targetTimeout%1000000;

		FD_ZERO(&rfds);
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if (fd_rc[i] != -1)
				FD_SET(fd_rc[i], &rfds);
		}
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (true)
#else
		if (fd_keyb > 0)
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
			FD_SET(fd_keyb, &rfds);

		FD_SET(fd_event, &rfds);
		FD_SET(fd_pipe_high_priority[0], &rfds);
		FD_SET(fd_pipe_low_priority[0], &rfds);
		//
		//FD_SET(fd_pipe_event[0], &rfds);

		int status = select(fd_max + 1, &rfds, NULL, NULL, &tvselect);

		if ( status == -1 )
		{
			perror("[neutrino - getMsg_us]: select returned ");
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

			//dprintf(DEBUG_NORMAL, "\n\033[1;32mCRCInput::getMsg_us:got event from high-pri pipe msg=(0x%x) data:(0x%x) <\033[0m\n", *msg, *data );

			return;
		}

#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (FD_ISSET(fd_keyb, &rfds))
		{
			int trkey;
			char key = 0;
			
			read(fd_keyb, &key, sizeof(key));

			switch(key)
			{
				case 27: // <- Esc
					trkey = KEY_HOME;
					break;
				case 10: // <- Return
				case 'o':
					trkey = KEY_OK;
					break;
				case 'p':
					trkey = KEY_POWER;
					break;
				case 's':
					trkey = KEY_SETUP;
					break;
				case 'h':
					trkey = KEY_HELP;
					break;
				case 'i':
					trkey = KEY_UP;
					break;
				case 'm':
					trkey = KEY_DOWN;
					break;
				case 'j':
					trkey = KEY_LEFT;
					break;
				case 'k':
					trkey = KEY_RIGHT;
					break;
				case 'r':
					trkey = KEY_RED;
					break;
				case 'g':
					trkey = KEY_GREEN;
					break;
				case 'y':
					trkey = KEY_YELLOW;
					break;
				case 'b':
					trkey = KEY_BLUE;
					break;
				case '0':
					trkey = RC_0;
					break;
				case '1':
					trkey = RC_1;
					break;
				case '2':
					trkey = RC_2;
					break;
				case '3':
					trkey = RC_3;
					break;
				case '4':
					trkey = RC_4;
					break;
				case '5':
					trkey = RC_5;
					break;
				case '6':
					trkey = RC_6;
					break;
				case '7':
					trkey = RC_7;
					break;
				case '8':
					trkey = RC_8;
					break;
				case '9':
					trkey = RC_9;
					break;
				case '+':
					trkey = RC_plus;
					break;
				case '-':
					trkey = RC_minus;
					break;
				case 'a':
					trkey = KEY_A;
					break;
				case 'u':
					trkey = KEY_U;
					break;
				case '/':
					trkey = KEY_SLASH;
					break;
				case '\\':
					trkey = KEY_BACKSLASH;
					break;
				default:
					trkey = RC_nokey;
			}
			
			if (trkey != RC_nokey)
			{
				*msg = trkey;
				*data = 0; /* <- button pressed */
				return;
			}
		}
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

		/*/
		if(FD_ISSET(fd_pipe_event[0], &rfds))
		{
			struct pevent buf;

			read(fd_pipe_event[0], &buf, sizeof(buf));
			
			unsigned char* p;
			p = new unsigned char[ sizeof(buf) + 1];

			*msg  = buf.msg;
			*data = (const neutrino_msg_data_t) p;

			dprintf(DEBUG_NORMAL, "\033[1;32mCRCInput::getMsg_us:got event from event pipe msg=(0x%x) data:(0x%x) <\033[0m\n", *msg, *data );

			return;
		}
		*/

		// fd_eventclient
		if(FD_ISSET(fd_event, &rfds)) 
		{
			socklen_t          clilen;
			struct sockaddr_in cliaddr;
			clilen = sizeof(cliaddr);
			int fd_eventclient = accept(fd_event, (struct sockaddr *) &cliaddr, &clilen);

			*msg = RC_nokey;
			
			CEventServer::eventHead emsg;
			int read_bytes = recv(fd_eventclient, &emsg, sizeof(emsg), MSG_WAITALL);

			if ( read_bytes == sizeof(emsg) ) 
			{
				bool dont_delete_p = false;

				unsigned char* p;
				p = new unsigned char[ emsg.dataSize + 1 ];

				if ( p != NULL )
				{
					read_bytes = recv(fd_eventclient, p, emsg.dataSize, MSG_WAITALL);
					
					//dprintf(DEBUG_NORMAL, "\n\033[1;32mCRCInput::getMsg_us:got event from fd_event msg=(0x%x) data:(0x%x) <\033[0m\n", emsg.eventID, *(unsigned*) p);

					if (emsg.initiatorID == CEventServer::INITID_NEUTRINO)
					{					  
						//dprintf(DEBUG_INFO, "CRCInput::getMsg_us: event - from NEUTRINO %x %x\n", emsg.eventID, *(unsigned*) p);					
						
						if ((emsg.eventID == NeutrinoMessages::EVT_RECORDING_ENDED) && (read_bytes == sizeof(stream2file_status2_t)))
						{
							*msg  = NeutrinoMessages::EVT_RECORDING_ENDED;
							*data = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
						
						//
						switch(emsg.eventID)
						{
							// zapit
							case NeutrinoMessages::EVT_RECORDMODE:
								*msg  = NeutrinoMessages::EVT_RECORDMODE;
								*data = *(bool*)p;
								break;
								
							//case NeutrinoMessages::EVT_RECORDMODE_DEACTIVATED:
							//	*msg  = NeutrinoMessages::EVT_RECORDMODE;
							//	*data = false;
							//	break;
								
							case NeutrinoMessages::EVT_ZAP_COMPLETE:
								*msg = NeutrinoMessages::EVT_ZAP_COMPLETE;
								break;
								
							case NeutrinoMessages::EVT_ZAP_FAILED:
								*msg = NeutrinoMessages::EVT_ZAP_FAILED;
								break;
								
							case NeutrinoMessages::EVT_ZAP_SUB_FAILED:
								*msg = NeutrinoMessages::EVT_ZAP_SUB_FAILED;
								break;
								
							case NeutrinoMessages::EVT_ZAP_ISNVOD:
								*msg = NeutrinoMessages::EVT_ZAP_ISNVOD;
								break;
								
							case NeutrinoMessages::EVT_ZAP_SUB_COMPLETE:
								*msg = NeutrinoMessages::EVT_ZAP_SUB_COMPLETE;
								break;
								
							case NeutrinoMessages::EVT_SCAN_COMPLETE:
								*msg  = NeutrinoMessages::EVT_SCAN_COMPLETE;
								*data = 0;
								break;
								
							case NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS:
								*msg  = NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
								*msg  = NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY:
								*msg = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN:
								*msg = NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN;
								break;
								
							case NeutrinoMessages::EVT_SCAN_SERVICENAME:
								*msg = NeutrinoMessages::EVT_SCAN_SERVICENAME;
								break;
							case NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN:
								*msg  = NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN:
								*msg  = NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN:
								*msg  = NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP:
								*msg  = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_NUM_CHANNELS:
								*msg = NeutrinoMessages::EVT_SCAN_NUM_CHANNELS;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_PROVIDER:
								*msg = NeutrinoMessages::EVT_SCAN_PROVIDER;
								break;
								
							case NeutrinoMessages::EVT_SCAN_SATELLITE:
								*msg = NeutrinoMessages::EVT_SCAN_SATELLITE;
								break;
							case NeutrinoMessages::EVT_BOUQUETSCHANGED:
								*msg  = NeutrinoMessages::EVT_BOUQUETSCHANGED;
								*data = 0;
								break;
								
							case NeutrinoMessages::EVT_SERVICESCHANGED:
								*msg  = NeutrinoMessages::EVT_SERVICESCHANGED;
								*data = 0;
								break;
								
							case NeutrinoMessages::EVT_ZAP_CA_CLEAR:
								*msg  = NeutrinoMessages::EVT_ZAP_CA_CLEAR;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_ZAP_CA_LOCK:
								*msg  = NeutrinoMessages::EVT_ZAP_CA_LOCK;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_ZAP_CA_FTA:
								*msg  = NeutrinoMessages::EVT_ZAP_CA_FTA;
								*data = *(unsigned*) p;
								break;
							
							case NeutrinoMessages::EVT_ZAP_CA_ID :
								*msg = NeutrinoMessages::EVT_ZAP_CA_ID;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SCAN_FAILED:
								*msg  = NeutrinoMessages::EVT_SCAN_FAILED;
								*data = 0;
								break;
								
							case NeutrinoMessages::EVT_ZAP_MOTOR:
								*msg  = NeutrinoMessages::EVT_ZAP_MOTOR;
								*data = *(unsigned*) p;
								break;
								
							case NeutrinoMessages::EVT_SERVICES_UPD:
								*msg          = NeutrinoMessages::EVT_SERVICES_UPD;
								*data         = 0;
								break;
								
							case NeutrinoMessages::EVT_PMT_CHANGED:
								*msg          = NeutrinoMessages::EVT_PMT_CHANGED;
								*data = (neutrino_msg_data_t) p;
								break;
							
							// sectionsd	
							case NeutrinoMessages::EVT_TIMESET:
								{
                                    					if ((int64_t)last_keypress > *(int64_t*)p)
										last_keypress += *(int64_t *)p;

								    	*msg = NeutrinoMessages::EVT_TIMESET;
								    	*data = (neutrino_msg_data_t) p;
								   	 dont_delete_p = true;
								}
								break;
								
							case NeutrinoMessages::EVT_CURRENTNEXT_EPG:
								*msg = NeutrinoMessages::EVT_CURRENTNEXT_EPG;
								*data = (neutrino_msg_data_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::EVT_SI_FINISHED:
								*msg = NeutrinoMessages::EVT_SI_FINISHED;
								*data = 0;
								break;
								
							// httpd	
							case NeutrinoMessages::REBOOT :
								*msg = NeutrinoMessages::REBOOT;
								*data = 0;
								break;
								
							case NeutrinoMessages::RESTART :
								*msg = NeutrinoMessages::RESTART;
								*data = 0;
								break;
								
							case NeutrinoMessages::EVT_POPUP :
								*msg = NeutrinoMessages::EVT_POPUP;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::EVT_EXTMSG :
								*msg = NeutrinoMessages::EVT_EXTMSG;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::CHANGEMODE :	// Change
								*msg = NeutrinoMessages::CHANGEMODE;
								*data = *(size_t*) p;
								break;
								
							case NeutrinoMessages::STANDBY_TOGGLE :
								*msg = NeutrinoMessages::STANDBY_TOGGLE;
								*data = 0;
								break;
								
							case NeutrinoMessages::LOCK_RC :
								*msg = NeutrinoMessages::LOCK_RC;
								*data = 0;
								break;
								
							case NeutrinoMessages::UNLOCK_RC :
								*msg = NeutrinoMessages::UNLOCK_RC;
								*data = 0;
								break;
								
							// timerd
							case NeutrinoMessages::ANNOUNCE_RECORD :
								*msg = NeutrinoMessages::ANNOUNCE_RECORD;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::ANNOUNCE_ZAPTO :
								*msg = NeutrinoMessages::ANNOUNCE_ZAPTO;
								*data = 0;
								break;
								
							case NeutrinoMessages::ANNOUNCE_SHUTDOWN :
								*msg = NeutrinoMessages::ANNOUNCE_SHUTDOWN;
								*data = 0;
								break;
								
							case NeutrinoMessages::ANNOUNCE_SLEEPTIMER :
								*msg = NeutrinoMessages::ANNOUNCE_SLEEPTIMER;
								*data = 0;
								break;
								
							case NeutrinoMessages::SLEEPTIMER :
								*msg = NeutrinoMessages::SLEEPTIMER;
								*data = 0;
								break;
								
							case NeutrinoMessages::RECORD_START :
								*msg = NeutrinoMessages::RECORD_START;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::RECORD_STOP :
								*msg = NeutrinoMessages::RECORD_STOP;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::ZAPTO :
								*msg = NeutrinoMessages::ZAPTO;
								*data = (size_t)  p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::EVT_NEXTPROGRAM :
								*msg = NeutrinoMessages::EVT_NEXTPROGRAM;
								*data = (size_t)  p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::SHUTDOWN :
								*msg = NeutrinoMessages::SHUTDOWN;
								*data = 0;
								break;
								
							case NeutrinoMessages::STANDBY_ON :
								*msg = NeutrinoMessages::STANDBY_ON;
								*data = 0;
								break;
								
							case NeutrinoMessages::STANDBY_OFF :
								*msg = NeutrinoMessages::STANDBY_OFF;
								*data = 0;
								break;
								
							case NeutrinoMessages::REMIND :
								*msg = NeutrinoMessages::REMIND;
								*data = (size_t) p;
								dont_delete_p = true;
								break;
								
							case NeutrinoMessages::EVT_START_PLUGIN :
								*msg = NeutrinoMessages::EVT_START_PLUGIN;
								*data = (size_t) p;
								dont_delete_p = true;
								break;

								
							//
							default:
								printf("CRCInput::getMsg_us: event INITID_NEUZTRINO - unknown eventID 0x%x\n",  emsg.eventID );
						}
						
						if (((*msg) >= RC_WithData) && ((*msg) < RC_WithData + 0x10000000))
						{
							*data = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
					}
					else if (emsg.initiatorID == CEventServer::INITID_GENERIC_INPUT_EVENT_PROVIDER)
					{					  
						//dprintf(DEBUG_INFO, "CRCInput::getMsg_us: event - from GENERIC_INPUT_EVENT_PROVIDER %x %x\n", emsg.eventID, *(unsigned*) p);						
						
						if (read_bytes == sizeof(int))
						{
							*msg  = *(int *)p;
							*data = emsg.eventID;
						}
					}
					else
						printf("CRCInput::getMsg_us: event - unknown initiatorID 0x%x\n",  emsg.initiatorID);
					
					if ( !dont_delete_p )
					{
						delete[] p;
						p = NULL;
					}
				}
			}
			else
			{
				printf("CRCInput::getMsg_us: event - read failed!\n");
			}

			::close(fd_eventclient);

			if ( *msg != RC_nokey )
			{
				// raus hier :)
				//printf("[neutrino] event 0x%x\n", *msg);
				return;
			}
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
					//dprintf(DEBUG_INFO, "CRCInput::getMsg_us: read event %d != %ld\n", ret, sizeof(t_input_event) );	
					continue;
				}
								
				//dprintf(DEBUG_NORMAL, "\n\033[1;32mCRCInput::getMsg_us:got event from device type: 0x%X key: 0x%X value %d, translate: 0x%X -%s<\033[0m\n", ev.type, ev.code, ev.value, translate(ev.code, i), getKeyName(translate(ev.code, i)).c_str() );

				uint32_t trkey = translate(ev.code, i);

				if (trkey == RC_nokey) 
					continue;
				
				if (ev.value) 
				{
					uint64_t now_pressed;
					bool keyok = true;

					tv = ev.time;
					now_pressed = (uint64_t) tv.tv_usec + (uint64_t)((uint64_t) tv.tv_sec * (uint64_t) 1000000);
					if (ev.code == rc_last_key) 
					{
						/* only allow selected keys to be repeated */
						/* (why?)                                  */
						if((trkey == RC_up) || (trkey == RC_down   ) ||
							(trkey == RC_plus   ) || (trkey == RC_minus  ) ||
							(trkey == RC_page_down   ) || (trkey == RC_page_up  ) ||
							//(trkey == RC_standby) ||
							((bAllowRepeatLR) && ((trkey == RC_left ) ||
								(trkey == RC_right))))
						{
#ifdef ENABLE_REPEAT_CHECK
							if (rc_last_repeat_key != ev.code) 
							{
								if ((now_pressed > last_keypress + repeat_block) ||
										/* accept all keys after time discontinuity: */
										(now_pressed < last_keypress)) 
									rc_last_repeat_key = ev.code;
								else
									keyok = false;
							}
#endif
						}
						else
							keyok = false;
					}
					else
						rc_last_repeat_key = KEY_MAX;

					rc_last_key = ev.code;

					if (keyok) 
					{
#ifdef ENABLE_REPEAT_CHECK
						if ((now_pressed > last_keypress + repeat_block_generic) ||
								/* accept all keys after time discontinuity: */
								(now_pressed < last_keypress)) 
#endif
						{
							last_keypress = now_pressed;

							*msg = trkey;
							*data = 0; /* <- button pressed */

							return;
						}
					} /*if keyok */
				} /* if (ev.value) */
				else 
				{
					// clear rc_last_key on keyup event
					rc_last_key = KEY_MAX;
					if (trkey == RC_standby) 
					{
						*msg = RC_standby;
						*data = 1; /* <- button released */
						return;
					}
				}
			}/* if FDSET */
		} /* for NUMBER_OF_EVENT_DEVICES */

		// pipe low prio
		if(FD_ISSET(fd_pipe_low_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_low_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			//dprintf(DEBUG_NORMAL, "\n\033[1;32mCRCInput::getMsg_us:got event from low-pri pipe msg=(0x%x) data:(0x%x) <\033[0m\n", *msg, *data );

			return;
		}

		if ( InitialTimeout == 0 )
		{
			//nicht warten wenn kein key da ist
			*msg = RC_timeout;
			*data = 0;
			return;
		}
		else
		{
			//timeout neu kalkulieren
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
	dprintf(DEBUG_INFO, "CRCInput::postMsg %x %x %d >\n", msg, data, Priority );

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
static const unsigned int digit_to_key[10] = {RC_0, RC_1, RC_2, RC_3, RC_4, RC_5, RC_6, RC_7, RC_8, RC_9};

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
			
		//case RC_aspect:
		//	return "RC_aspect";			
				
		/* VFD Tasten the generic values are from cuberevo so fix it */
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
			printf("CRCInput::getSpecialKeyName: unknown key: 0x%x\n", key);
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
int CRCInput::translate(unsigned int code, int num)
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
	
	//else if (code == key_aspect) return RC_aspect;
	
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

/*
void CRCInput::sendEvent(const neutrino_msg_t event, void *data, const unsigned int datalen)
{
	dprintf(DEBUG_NORMAL, "CRCInput::sendEvent >\n");
		
	//
	struct pevent buf;
	
	buf.msg  = event;
	buf.data = data;

	write(fd_pipe_event[1], &buf, sizeof(buf));
}
*/

//
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
	
	if(msg == RC_1)
	{
		key = '1';
	}
	
	if(msg == RC_2)
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
	else if(msg == RC_3)
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
	else if(msg == RC_4)
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
	else if(msg == RC_5)
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
	else if(msg == RC_6)
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
	else if(msg == RC_7)
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
	else if(msg == RC_8)
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
	else if(msg == RC_9)
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
	else if(msg == RC_0)
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


