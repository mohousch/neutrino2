/*
    Copyright (C) 2018-2021 TangoCash

    License: GPLv2

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation;

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <ctype.h>

#include <array>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <linux/input.h>

#include "linux-uapi-cec.h"
#include "hdmi_cec.h"
#include "hdmi_cec_types.h"

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define NORMAL "\x1B[0m"

#define EPOLL_WAIT_TIMEOUT (-1)
#define EPOLL_MAX_EVENTS (1)

#define CEC_FALLBACK_DEVICE "/dev/cec0"
#define CEC_HDMIDEV "/dev/hdmi_cec"
#if BOXMODEL_H7
#define RC_DEVICE  "/dev/input/event2"
#else
#define RC_DEVICE  "/dev/input/event1"
#endif

hdmi_cec *hdmi_cec::hdmi_cec_instance = NULL;

//hack to get an instance before first call
hdmi_cec *CEC = hdmi_cec::getInstance();

hdmi_cec::hdmi_cec()
{
	standby_cec_activ = autoview_cec_activ = standby = muted = running = false;
	hdmiFd = -1;
	volume = 0;
	fallback = false;
	tv_off = true;
	deviceType = CEC_LOG_ADDR_TYPE_UNREGISTERED;
	audio_destination = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM;
}

hdmi_cec::~hdmi_cec()
{
	if (hdmiFd >= 0)
	{
		close(hdmiFd);
		hdmiFd = -1;
	}
}

hdmi_cec *hdmi_cec::getInstance()
{
	if (hdmi_cec_instance == NULL)
	{
		hdmi_cec_instance = new hdmi_cec();
	}
	return hdmi_cec_instance;
}

bool hdmi_cec::SetCECMode(VIDEO_HDMI_CEC_MODE _deviceType)
{
	physicalAddress[0] = 0x10;
	physicalAddress[1] = 0x00;
	logicalAddress = 1;

	if (_deviceType == VIDEO_HDMI_CEC_MODE_OFF)
	{
		Stop();
		return false;
	}
	else
		deviceType = _deviceType;

	//hal_info(GREEN "[CEC] switch on %s\n" NORMAL, __func__);

#if PLATFORM_VUPLUS
	if (hdmiFd == -1)
	{
		hdmiFd = ::open(CEC_HDMIDEV, O_RDWR | O_NONBLOCK | O_CLOEXEC);
		if (hdmiFd >= 0)
		{
			::ioctl(hdmiFd, 0); /* flush old messages */
		}
	}
#endif

	if (hdmiFd == -1)
	{
		hdmiFd = open(CEC_FALLBACK_DEVICE, O_RDWR | O_CLOEXEC);

		if (hdmiFd >= 0)
		{
			fallback = true;
#if PLATFORM_VUPLUS
			//hal_info(RED "[CEC] fallback on %s\n" NORMAL, __func__);
#endif

			__u32 monitor = CEC_MODE_INITIATOR | CEC_MODE_FOLLOWER;
			struct cec_caps caps = {};

			if (ioctl(hdmiFd, CEC_ADAP_G_CAPS, &caps) < 0)
				printf("[CEC] %s: get caps failed\n", __func__);

			if (caps.capabilities & CEC_CAP_LOG_ADDRS)
			{
				struct cec_log_addrs laddrs = {};

				if (ioctl(hdmiFd, CEC_ADAP_S_LOG_ADDRS, &laddrs) < 0)
					printf("[CEC] %s: reset log addr failed\n", __func__);

				memset(&laddrs, 0, sizeof(laddrs));

				/*
				 * NOTE: cec_version, osd_name and deviceType should be made configurable,
				 * CEC_ADAP_S_LOG_ADDRS delayed till the desired values are available
				 * (saves us some startup speed as well, polling for a free logical address
				 * takes some time)
				 */
				laddrs.cec_version = CEC_OP_CEC_VERSION_2_0;
				//hw_caps_t *_caps = get_hwcaps();
				strcpy(laddrs.osd_name, /*_caps->boxname*/"unknown");//FIXME
				laddrs.vendor_id = CEC_VENDOR_ID_NONE;

				switch (deviceType)
				{
					case CEC_LOG_ADDR_TV:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_TV;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_TV;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_TV;
						break;
					case CEC_LOG_ADDR_RECORD_1:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_RECORD;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_RECORD;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_RECORD;
						break;
					case CEC_LOG_ADDR_TUNER_1:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_TUNER;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_TUNER;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_TUNER;
						break;
					case CEC_LOG_ADDR_PLAYBACK_1:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_PLAYBACK;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_PLAYBACK;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_PLAYBACK;
						break;
					case CEC_LOG_ADDR_AUDIOSYSTEM:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_AUDIOSYSTEM;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_AUDIOSYSTEM;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM;
						break;
					default:
						laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_UNREGISTERED;
						laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_SWITCH;
						laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_SWITCH;
						break;
				}
				laddrs.num_log_addrs++;

				if (ioctl(hdmiFd, CEC_ADAP_S_LOG_ADDRS, &laddrs) < 0)
					printf("[CEC] %s: et log addr failed\n", __func__);
			}

			if (ioctl(hdmiFd, CEC_S_MODE, &monitor) < 0)
				printf("[CEC] %s: monitor failed\n", __func__);

		}
	}

	if (hdmiFd >= 0)
	{
		GetCECAddressInfo();

		if (autoview_cec_activ)
			SetCECState(false);

		Start();
		return true;

	}
	return false;
}

void hdmi_cec::GetCECAddressInfo()
{
	if (hdmiFd >= 0)
	{
		bool hasdata = false;
		struct addressinfo addressinfo;

		if (fallback)
		{
			__u16 phys_addr;
			struct cec_log_addrs laddrs = {};

			::ioctl(hdmiFd, CEC_ADAP_G_PHYS_ADDR, &phys_addr);
			addressinfo.physical[0] = (phys_addr >> 8) & 0xff;
			addressinfo.physical[1] = phys_addr & 0xff;

			::ioctl(hdmiFd, CEC_ADAP_G_LOG_ADDRS, &laddrs);
			addressinfo.logical = laddrs.log_addr[0];

			switch (laddrs.log_addr_type[0])
			{
				case CEC_LOG_ADDR_TYPE_TV:
					addressinfo.type = CEC_LOG_ADDR_TV;
					break;
				case CEC_LOG_ADDR_TYPE_RECORD:
					addressinfo.type = CEC_LOG_ADDR_RECORD_1;
					break;
				case CEC_LOG_ADDR_TYPE_TUNER:
					addressinfo.type = CEC_LOG_ADDR_TUNER_1;
					break;
				case CEC_LOG_ADDR_TYPE_PLAYBACK:
					addressinfo.type = CEC_LOG_ADDR_PLAYBACK_1;
					break;
				case CEC_LOG_ADDR_TYPE_AUDIOSYSTEM:
					addressinfo.type = CEC_LOG_ADDR_AUDIOSYSTEM;
					break;
				case CEC_LOG_ADDR_TYPE_UNREGISTERED:
				default:
					addressinfo.type = CEC_LOG_ADDR_UNREGISTERED;
					break;
			}
			hasdata = true;
		}
		else
		{
			if (::ioctl(hdmiFd, 1, &addressinfo) >= 0)
			{
				hasdata = true;
			}
		}
		if (hasdata)
		{
			deviceType = addressinfo.type;
			logicalAddress = addressinfo.logical;
			if (memcmp(physicalAddress, addressinfo.physical, sizeof(physicalAddress)))
			{
				//hal_info(GREEN "[CEC] %s: detected physical address change: %02X%02X --> %02X%02X\n" NORMAL, __func__, physicalAddress[0], physicalAddress[1], addressinfo.physical[0], addressinfo.physical[1]);
				memcpy(physicalAddress, addressinfo.physical, sizeof(physicalAddress));
				ReportPhysicalAddress();
			}
		}
	}
}

void hdmi_cec::ReportPhysicalAddress()
{
	struct cec_message txmessage;
	txmessage.initiator = logicalAddress;
	txmessage.destination = CEC_LOG_ADDR_BROADCAST;
	txmessage.data[0] = CEC_MSG_REPORT_PHYSICAL_ADDR;
	txmessage.data[1] = physicalAddress[0];
	txmessage.data[2] = physicalAddress[1];
	txmessage.data[3] = deviceType;
	txmessage.length = 4;
	SendCECMessage(txmessage);
}

void hdmi_cec::SendCECMessage(struct cec_message &txmessage, int sleeptime)
{
	if (hdmiFd >= 0)
	{

		char str[txmessage.length * 6];
		for (int i = 0; i < txmessage.length; i++)
		{
			sprintf(str + (i * 6), "[0x%02X]", txmessage.data[i]);
		}
		//hal_info(GREEN "[CEC] send message %s to %s (0x%02X>>0x%02X) '%s' (%s)\n" NORMAL, ToString((cec_logical_address)txmessage.initiator), txmessage.destination == 0xf ? "all" : ToString((cec_logical_address)txmessage.destination), txmessage.initiator, txmessage.destination, ToString((cec_opcode)txmessage.data[0]), str);

		if (fallback)
		{
			struct cec_msg msg;
			cec_msg_init(&msg, txmessage.initiator, txmessage.destination);
			memcpy(&msg.msg[1], txmessage.data, txmessage.length);
			msg.len = txmessage.length + 1;
			ioctl(hdmiFd, CEC_TRANSMIT, &msg);
		}
		else
		{
			struct cec_message_fb message;
			message.address = txmessage.destination;
			message.length = txmessage.length;
			memcpy(&message.data, txmessage.data, txmessage.length);
			::write(hdmiFd, &message, 2 + message.length);
		}

		usleep(sleeptime * 1000);
	}
}

void hdmi_cec::SetCECAutoStandby(bool state)
{
	standby_cec_activ = state;
}

void hdmi_cec::SetCECAutoView(bool state)
{
	autoview_cec_activ = state;
}

void hdmi_cec::SetCECState(bool state)
{
	struct cec_message message;

	standby = state;

	if ((standby_cec_activ) && state)
	{
		message.initiator = logicalAddress;
		message.destination = CEC_OP_PRIM_DEVTYPE_TV;
		message.data[0] = CEC_MSG_STANDBY;
		message.length = 1;
		SendCECMessage(message);

		message.initiator = logicalAddress;
		message.destination = CEC_OP_PRIM_DEVTYPE_TV;
		message.data[0] = CEC_MSG_GIVE_DEVICE_POWER_STATUS;
		message.length = 1;
		SendCECMessage(message);
	}

	if ((autoview_cec_activ) && !state)
	{
		message.initiator = logicalAddress;
		message.destination = CEC_OP_PRIM_DEVTYPE_TV;
		message.data[0] = CEC_MSG_GET_CEC_VERSION;
		message.length = 1;
		SendCECMessage(message);

		message.initiator = logicalAddress;
		message.destination = CEC_OP_PRIM_DEVTYPE_TV;
		message.data[0] = CEC_MSG_GIVE_DEVICE_POWER_STATUS;
		message.length = 1;
		SendCECMessage(message);

#if BOXMODEL_VUPLUS_ALL
		int cnt = 0;

		while (tv_off && (cnt < 5))
		{
#endif

			message.initiator = logicalAddress;
			message.destination = CEC_OP_PRIM_DEVTYPE_TV;
			message.data[0] = CEC_MSG_IMAGE_VIEW_ON;
			message.length = 1;
			SendCECMessage(message);

			message.initiator = logicalAddress;
			message.destination = CEC_OP_PRIM_DEVTYPE_TV;
			message.data[0] = CEC_MSG_GIVE_DEVICE_POWER_STATUS;
			message.length = 1;
			SendCECMessage(message);

#if BOXMODEL_VUPLUS_ALL
			cnt++;
		}
#endif

		GetCECAddressInfo();

		message.initiator = logicalAddress;
		message.destination = CEC_LOG_ADDR_BROADCAST;
		message.data[0] = CEC_MSG_ACTIVE_SOURCE;
		message.data[1] = physicalAddress[0];
		message.data[2] = physicalAddress[1];
		message.length = 3;
		SendCECMessage(message);

		message.initiator = logicalAddress;
		message.destination = CEC_LOG_ADDR_BROADCAST;
		message.data[0] = CEC_OPCODE_SET_OSD_NAME;
		message.data[1] = 0x6e; //n
		message.data[2] = 0x65; //e
		message.data[3] = 0x75; //u
		message.data[4] = 0x74; //t
		message.data[5] = 0x72; //r
		message.data[6] = 0x69; //i
		message.data[7] = 0x6e; //n
		message.data[8] = 0x6f; //o
		message.length = 9;
		SendCECMessage(message);

		request_audio_status();
	}

}

long hdmi_cec::translateKey(unsigned char code)
{
	long key = 0;
	switch (code)
	{
		case CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL:
			key = KEY_MENU;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER0:
			key = KEY_0;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER1:
			key = KEY_1;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER2:
			key = KEY_2;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER3:
			key = KEY_3;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER4:
			key = KEY_4;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER5:
			key = KEY_5;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER6:
			key = KEY_6;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER7:
			key = KEY_7;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER8:
			key = KEY_8;
			break;
		case CEC_USER_CONTROL_CODE_NUMBER9:
			key = KEY_9;
			break;
		case CEC_USER_CONTROL_CODE_CHANNEL_UP:
			key = KEY_CHANNELUP;
			break;
		case CEC_USER_CONTROL_CODE_CHANNEL_DOWN:
			key = KEY_CHANNELDOWN;
			break;
		case CEC_USER_CONTROL_CODE_PLAY:
			key = KEY_PLAY;
			break;
		case CEC_USER_CONTROL_CODE_STOP:
			key = KEY_STOP;
			break;
		case CEC_USER_CONTROL_CODE_PAUSE:
			key = KEY_PAUSE;
			break;
		case CEC_USER_CONTROL_CODE_RECORD:
			key = KEY_RECORD;
			break;
		case CEC_USER_CONTROL_CODE_REWIND:
			key = KEY_REWIND;
			break;
		case CEC_USER_CONTROL_CODE_FAST_FORWARD:
			key = KEY_FASTFORWARD;
			break;
		case CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE:
			key = KEY_INFO;
			break;
		case CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING:
			key = KEY_PROGRAM;
			break;
		case CEC_USER_CONTROL_CODE_PLAY_FUNCTION:
			key = KEY_PLAY;
			break;
		case CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION:
			key = KEY_PLAYPAUSE;
			break;
		case CEC_USER_CONTROL_CODE_RECORD_FUNCTION:
			key = KEY_RECORD;
			break;
		case CEC_USER_CONTROL_CODE_STOP_FUNCTION:
			key = KEY_STOP;
			break;
		case CEC_USER_CONTROL_CODE_SELECT:
			key = KEY_OK;
			break;
		case CEC_USER_CONTROL_CODE_LEFT:
			key = KEY_LEFT;
			break;
		case CEC_USER_CONTROL_CODE_RIGHT:
			key = KEY_RIGHT;
			break;
		case CEC_USER_CONTROL_CODE_UP:
			key = KEY_UP;
			break;
		case CEC_USER_CONTROL_CODE_DOWN:
			key = KEY_DOWN;
			break;
		case CEC_USER_CONTROL_CODE_EXIT:
			key = KEY_EXIT;
			break;
		case CEC_USER_CONTROL_CODE_F2_RED:
			key = KEY_RED;
			break;
		case CEC_USER_CONTROL_CODE_F3_GREEN:
			key = KEY_GREEN;
			break;
		case CEC_USER_CONTROL_CODE_F4_YELLOW:
			key = KEY_YELLOW;
			break;
		case CEC_USER_CONTROL_CODE_F1_BLUE:
			key = KEY_BLUE;
			break;
		default:
			key = KEY_MENU;
			break;
	}
	return key;
}

bool hdmi_cec::Start()
{
	if (running)
		return false;

	if (hdmiFd == -1)
		return false;

	running = true;
	OpenThreads::Thread::setSchedulePriority(THREAD_PRIORITY_MIN);
	return (OpenThreads::Thread::start() == 0);
}

bool hdmi_cec::Stop()
{
	if (!running)
		return false;

	running = false;

	OpenThreads::Thread::cancel();

	if (hdmiFd >= 0)
	{
		close(hdmiFd);
		hdmiFd = -1;
	}

	return (OpenThreads::Thread::join() == 0);
}

void hdmi_cec::run()
{
	OpenThreads::Thread::setCancelModeAsynchronous();
	int n;
	int epollfd = epoll_create1(0);
	struct epoll_event event;
	event.data.fd = hdmiFd;
	event.events = EPOLLIN;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, hdmiFd, &event);

	std::array<struct epoll_event, EPOLL_MAX_EVENTS> events;

	while (running)
	{
		n = epoll_wait(epollfd, events.data(), EPOLL_MAX_EVENTS, EPOLL_WAIT_TIMEOUT);
		for (int i = 0; i < n; ++i)
		{
			if (events[i].events & EPOLLIN)
				Receive(events[i].events);
		}
	}
}

void hdmi_cec::Receive(int what)
{
	if (what & EPOLLIN)
	{

		bool hasdata = false;
		struct cec_message rxmessage;
		struct cec_message txmessage;

		if (fallback)
		{
			struct cec_msg msg;
			if (::ioctl(hdmiFd, CEC_RECEIVE, &msg) >= 0)
			{
				rxmessage.length = msg.len - 1;
				rxmessage.initiator = cec_msg_initiator(&msg);
				rxmessage.destination = cec_msg_destination(&msg);
				rxmessage.opcode = cec_msg_opcode(&msg);
				memcpy(&rxmessage.data, &msg.msg[1], rxmessage.length);
				hasdata = true;
			}
		}
		else
		{
			struct cec_message_fb rx_message;
			if (::read(hdmiFd, &rx_message, 2) == 2)
			{
				if (::read(hdmiFd, &rx_message.data, rx_message.length) == rx_message.length)
				{
					rxmessage.length = rx_message.length;
					rxmessage.initiator = rx_message.address;
					rxmessage.destination = logicalAddress;
					rxmessage.opcode = rx_message.data[0];
					memcpy(&rxmessage.data, rx_message.data, rx_message.length);
					hasdata = true;
				}
			}
		}

		if (hasdata)
		{
			bool keypressed = false;
			static unsigned char pressedkey = 0;

			char str[rxmessage.length * 6];
			for (int i = 0; i < rxmessage.length; i++)
			{
				sprintf(str + (i * 6), "[0x%02X]", rxmessage.data[i]);
			}
			//hal_info(GREEN "[CEC] received message %s to %s (0x%02X>>0x%02X) '%s' (%s)\n" NORMAL, ToString((cec_logical_address)rxmessage.initiator), rxmessage.destination == 0xf ? "all" : ToString((cec_logical_address)rxmessage.destination), rxmessage.initiator, rxmessage.destination, ToString((cec_opcode)rxmessage.opcode), str);

			switch (rxmessage.opcode)
			{
				//case CEC_OPCODE_ACTIVE_SOURCE:
				case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
				{
					txmessage.destination = CEC_LOG_ADDR_BROADCAST; //rxmessage.initiator;
					txmessage.initiator = logicalAddress; //rxmessage.destination;
					txmessage.data[0] = CEC_MSG_ACTIVE_SOURCE;
					txmessage.data[1] = physicalAddress[0];
					txmessage.data[2] = physicalAddress[1];
					txmessage.length = 3;
					if (!standby)
						SendCECMessage(txmessage);
					break;
				}
				case CEC_OPCODE_REPORT_AUDIO_STATUS:
				{
					muted = ((rxmessage.data[1] & 0x80) == 0x80);
					volume = ((rxmessage.data[1] & 0x7F) / 127.0) * 100.0;
					if (muted)
						printf("[CEC] %s volume muted\n", ToString((cec_logical_address)rxmessage.initiator));
					else
						printf("[CEC] %s volume %d \n", ToString((cec_logical_address)rxmessage.initiator), volume);
					break;
				}
				case CEC_OPCODE_DEVICE_VENDOR_ID:
				case CEC_OPCODE_VENDOR_COMMAND_WITH_ID:
				{
					uint64_t iVendorId = ((uint64_t)rxmessage.data[1] << 16) +
					    ((uint64_t)rxmessage.data[2] << 8) +
					    (uint64_t)rxmessage.data[3];
					//hal_info(GREEN "[CEC] decoded message '%s' (%s)\n" NORMAL, ToString((cec_opcode)rxmessage.opcode), ToString((cec_vendor_id)iVendorId));
					break;
				}
				case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
				{
					txmessage.destination = rxmessage.initiator;
					txmessage.initiator = rxmessage.destination;
					txmessage.data[0] = GetResponseOpcode((cec_opcode)rxmessage.opcode);
					txmessage.data[1] = standby ? CEC_POWER_STATUS_STANDBY : CEC_POWER_STATUS_ON;
					txmessage.length = 2;
					SendCECMessage(txmessage);
					break;
				}
				case CEC_OPCODE_REPORT_POWER_STATUS:
				{
					if ((rxmessage.data[1] == CEC_POWER_STATUS_ON) || (rxmessage.data[1] == CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON))
					{
						//hal_info(GREEN "[CEC] %s reporting state on (%d)\n" NORMAL, ToString((cec_logical_address)rxmessage.initiator), rxmessage.data[1]);
						if (rxmessage.initiator == CEC_OP_PRIM_DEVTYPE_TV)
							tv_off = false;
					}
					else
					{
						//hal_info(GREEN "[CEC] %s reporting state off (%d)\n" NORMAL, ToString((cec_logical_address)rxmessage.initiator), rxmessage.data[1]);
						if (rxmessage.initiator == CEC_OP_PRIM_DEVTYPE_TV)
							tv_off = true;
					}
					break;
				}
				case CEC_OPCODE_STANDBY:
				{
					if (rxmessage.initiator == CEC_OP_PRIM_DEVTYPE_TV)
						tv_off = true;
					break;
				}
				case CEC_OPCODE_USER_CONTROL_PRESSED: /* key pressed */
				{
					keypressed = true;
					pressedkey = rxmessage.data[1];
				} // fall through
				case CEC_OPCODE_USER_CONTROL_RELEASE: /* key released */
				{
					long code = translateKey(pressedkey);
					//hal_info(GREEN "[CEC] decoded key %s (%ld)\n" NORMAL, ToString((cec_user_control_code)pressedkey), code);
					handleCode(code, keypressed);
					break;
				}
			}
		}
	}
}

void hdmi_cec::handleCode(long code, bool keypressed)
{
	int evd = open(RC_DEVICE, O_RDWR);
	if (evd < 0)
	{
		//hal_info(RED "[CEC] opening " RC_DEVICE " failed" NORMAL);
		return;
	}
	if (keypressed)
	{
		if (rc_send(evd, code, CEC_KEY_PRESSED) < 0)
		{
			//hal_info(RED "[CEC] writing 'KEY_PRESSED' event failed" NORMAL);
			close(evd);
			return;
		}
		rc_sync(evd);
	}
	else
	{
		if (rc_send(evd, code, CEC_KEY_RELEASED) < 0)
		{
			//hal_info(RED "[CEC] writing 'KEY_RELEASED' event failed" NORMAL);
			close(evd);
			return;
		}
		rc_sync(evd);
	}
	close(evd);
}

int hdmi_cec::rc_send(int fd, unsigned int code, unsigned int value)
{
	struct input_event ev;

	ev.type = EV_KEY;
	ev.code = code;
	ev.value = value;
	return write(fd, &ev, sizeof(ev));
}

void hdmi_cec::rc_sync(int fd)
{
	struct input_event ev;

	gettimeofday(&ev.time, NULL);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	write(fd, &ev, sizeof(ev));
}

void hdmi_cec::send_key(unsigned char key, unsigned char destination)
{
	struct cec_message txmessage;
	txmessage.destination = destination;
	txmessage.initiator = logicalAddress;
	txmessage.data[0] = CEC_OPCODE_USER_CONTROL_PRESSED;
	txmessage.data[1] = key;
	txmessage.length = 2;
	SendCECMessage(txmessage, 1);

	txmessage.destination = destination;
	txmessage.initiator = logicalAddress;
	txmessage.data[0] = CEC_OPCODE_USER_CONTROL_RELEASE;
	txmessage.length = 1;
	SendCECMessage(txmessage, 0);
}

void hdmi_cec::request_audio_status()
{
	struct cec_message txmessage;
	txmessage.destination = audio_destination;
	txmessage.initiator = logicalAddress;
	txmessage.data[0] = CEC_OPCODE_GIVE_AUDIO_STATUS;
	txmessage.length = 1;
	SendCECMessage(txmessage, 0);
}

void hdmi_cec::vol_up()
{
	send_key(CEC_USER_CONTROL_CODE_VOLUME_UP, audio_destination);
	request_audio_status();
}
void hdmi_cec::vol_down()
{
	send_key(CEC_USER_CONTROL_CODE_VOLUME_DOWN, audio_destination);
	request_audio_status();
}
void hdmi_cec::toggle_mute()
{
	send_key(CEC_USER_CONTROL_CODE_MUTE, audio_destination);
	request_audio_status();
}

void hdmi_cec::SetAudioDestination(int audio_dest)
{
	switch (audio_dest)
	{
		case 2:
			audio_destination = CEC_OP_PRIM_DEVTYPE_TV;
			break;
		case 1:
		default:
			audio_destination = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM;
			break;
	}
}

