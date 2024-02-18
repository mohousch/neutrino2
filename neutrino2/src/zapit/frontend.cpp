/*
 * $Id: frontend.cpp 23.09.2023 mohousch Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <cmath>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <fstream>

#include <system/debug.h>
#include <system/helpers.h>

#include <zapit/frontend_c.h>


//// globals
#define SOUTH		0
#define NORTH		1
#define EAST		0
#define WEST		1
#define USALS


#define diff(x,y)	(max(x,y) - min(x,y))
#define min(x,y)	((x < y) ? x : y)
#define max(x,y)	((x > y) ? x : y)

#define diff(x,y)	(max(x,y) - min(x,y))

#define TIMER_INIT()					\
	static unsigned int tmin = 2000, tmax = 0;	\
	struct timeval tv, tv2;				\
	unsigned int timer_msec = 0;

#define TIMER_START()					\
	gettimeofday(&tv, NULL);

#define TIMER_STOP(label)				\
	gettimeofday(&tv2, NULL);			\
	timer_msec = ((tv2.tv_sec-tv.tv_sec) * 1000) +	\
		     ((tv2.tv_usec-tv.tv_usec) / 1000); \
	if(tmin > timer_msec) tmin = timer_msec;	\
	if(tmax < timer_msec) tmax = timer_msec;	\
	 printf("%s: %u msec (min %u max %u)\n",	\
		 label, timer_msec, tmin, tmax);
		 
//S2API
#define MAXFRONTENDCMDS 16
dtv_property Frontend[MAXFRONTENDCMDS];
dtv_properties CmdSeq;

#define SETCMD(c, d) { Frontend[CmdSeq.num].cmd = (c);\
                       Frontend[CmdSeq.num].u.data = (d);\
                       if (CmdSeq.num++ > MAXFRONTENDCMDS) {\
                          return;\
                          }\
                     }
                     
#define MAX_DELSYS 	8
////
extern satellite_map_t satellitePositions;	// defined in zapit.cpp
extern transponder_list_t transponders;		// defined in zapit.cpp

////
CFrontend::CFrontend(int num, int adap)
{
	fd = -1;
	
	// dvb adapter index
	feadapter = adap;
	
	// frontend index
	fenumber = num;
	
	//
	mode = (fe_mode_t)FE_SINGLE;
	locked = false;
	standby = true;
	slave = false;
	tuned = false;

	memset(&curfe, 0, sizeof(curfe));
	
	// initialize curfe values
	curfe.fec_inner = FEC_3_4;
	curfe.fec_inner = FEC_3_4;
	curfe.modulation = QAM_64;
	
	//
	diseqcType = NO_DISEQC;
	diseqcRepeats = 0;
	// unicable
	uni_lnb = 0;
	uni_scr = -1;
	uni_qrg = 0;
	uncommitedInput = 255;
	diseqc = 255;
	
	// currentTP
	currentTransponder.feparams.polarization = 1;
	currentTransponder.feparams.frequency = 0;
	currentTransponder.TP_id = 0;
	currentTransponder.diseqc = 255;
	
	// to allow Open() switch it off
	currentVoltage = SEC_VOLTAGE_OFF; //SEC_VOLTAGE_13;
	currentToneMode = SEC_TONE_ON;
	
	//
	deliverySystemMask = UNDEFINED;
	forcedDelSys = UNDEFINED;
	fe_can_multistream = false;
	hybrid = false;
	powered = false;
}

CFrontend::~CFrontend(void)
{
	if(fd >= 0) 
		Close();
}

bool CFrontend::Open()
{
	dprintf(DEBUG_INFO, "CFrontend::Open:\n");
	
	if(!standby)
		return false;

	char filename[256];

	sprintf(filename, "/dev/dvb/adapter%d/frontend%d", feadapter, fenumber);
	
	if (fd < 0) 
	{
		// open frontend
		if ( (fd = open(filename, O_RDWR | O_NONBLOCK) ) < 0)
		{
			//dprintf(DEBUG_INFO, "CFrontend::Open:cannot open %s\n", filename);
			perror("CFrontend::Open:");
			return false;
		}
		
		// get frontend info
		getFEInfo();
		dprintf(DEBUG_NORMAL, "CFrontend::Open %s %s (type: %d)\n", filename, info.name, info.type);
	}
	
	currentTransponder.TP_id = 0;
	
	standby = false;
	
	return true;
}

//
void CFrontend::getFEInfo(void)
{
	dprintf(DEBUG_INFO, "CFrontend::getFEInfo:\n");
	
	if(::ioctl(fd, FE_GET_INFO, &info) < 0)
		perror("FE_GET_INFO");
}

//
void CFrontend::getFEDelSysMask(void)
{
	dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask:\n");
	
	bool legacy = true;

	deliverySystemMask = UNDEFINED;

#if HAVE_DVB_API_VERSION >= 5
#ifdef DTV_ENUM_DELSYS
	struct dtv_property p[1];
	memset(p, 0, sizeof(p));
	p[0].cmd = DTV_ENUM_DELSYS;
	struct dtv_properties CmdSeq;
	CmdSeq.num = 1;
	CmdSeq.props = p;
	
	int ret = ::ioctl(fd, FE_GET_PROPERTY, &CmdSeq);
	
	if (ret >= 0) 
	{
		for (uint32_t i = 0; i < p[0].u.buffer.len; i++) 
		{
			if (i >= MAX_DELSYS) 
			{
				dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) ERROR: too many delivery systems\n", feadapter, fenumber);
				break;
			}

			switch (p[0].u.buffer.data[i]) 
			{
				case SYS_DVBC_ANNEX_A:
				case SYS_DVBC_ANNEX_B:
				case SYS_DVBC_ANNEX_C:
					deliverySystemMask |= DVB_C;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-C (delivery_system: %x)\n", feadapter, fenumber, DVB_C);
					break;
					
				case SYS_DVBT:
					deliverySystemMask |= DVB_T;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-T (delivery_system: %x)\n", feadapter, fenumber, DVB_T);
					break;
					
				case SYS_DVBT2:
					deliverySystemMask |= DVB_T2;
					fe_can_multistream = info.caps & FE_CAN_MULTISTREAM;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-T2 (delivery_system: %x / Multistream: %s)\n", feadapter, fenumber, DVB_T2, fe_can_multistream ? "yes" :"no");
					break;
					
				case SYS_DVBS:
					deliverySystemMask |= DVB_S;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-S (delivery_system: %x)\n", feadapter, fenumber, DVB_S);
					break;
					
				case SYS_DVBS2:
#ifdef SYS_DVBS2X
				case SYS_DVBS2X:
#endif
					deliverySystemMask |= DVB_S2;
					fe_can_multistream = info.caps & FE_CAN_MULTISTREAM;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-S2 (delivery_system: %x / Multistream: %s)\n", feadapter, fenumber, DVB_S2, fe_can_multistream ? "yes" :"no");
#if !defined (__sh__)
					if (fe_can_multistream)
					{
						deliverySystemMask |= DVB_S2X;
						dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DVB-S2X (delivery_system: %x / Multistream: %s)\n", feadapter, fenumber, DVB_S2X, fe_can_multistream ? "yes" :"no");
					}
#endif
					break;
				
#ifdef SYS_DTMB	
				case SYS_DTMB:
					deliverySystemMask |= DVB_DTMB;
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) add delivery system DTMB (delivery_system: %x)\n", feadapter, fenumber, DVB_DTMB);
					break;
#endif
				
				default:
					dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: (fe%d:%d) ERROR: delivery system unknown (delivery_system: %d)\n", feadapter, fenumber, p[0].u.buffer.data[i]);
					continue;
			}

		}
		
		legacy = false;
	} 
	else 
	{
		dprintf(DEBUG_INFO, "CFrontend::getFEDelSysMask: can't query delivery systems on frontend (%d:%d) - falling back to legacy mode\n", feadapter, fenumber);
	}
#endif
#endif

	if (legacy) 
	{
		// Legacy mode (DVB-API < 5.5):
		switch (info.type) 
		{
			case FE_QPSK:
				deliverySystemMask |= DVB_S;
				deliverySystemMask |= DVB_S2;
				
				if (info.caps & FE_CAN_MULTISTREAM)
					deliverySystemMask |= DVB_S2X;
				break;
			case FE_OFDM:
				deliverySystemMask |= DVB_T;
#ifdef SYS_DVBT2
				if (info.caps & FE_CAN_2G_MODULATION)
					deliverySystemMask |= DVB_T2;
#endif
				break;
			case FE_QAM:
				deliverySystemMask |= DVB_C;
				break;
			default:
				printf("CFrontend::getFEDelSysMask: ERROR: unknown frontend type %d on frontend (%d:%d)", info.type, feadapter, fenumber);
		}
	}
	
	//
	if ( (deliverySystemMask & DVB_C && deliverySystemMask & DVB_T) || (deliverySystemMask & DVB_C && deliverySystemMask & DVB_T2) || (deliverySystemMask & DVB_C) && (deliverySystemMask & DVB_T) && (deliverySystemMask & DVB_T2) )
	{
		hybrid = true;
		dprintf(DEBUG_NORMAL, "CFrontend::getFEDelSysMask: fe(%d:%d) %s (delsys:0x%x) isHybrid:%s\n", feadapter, fenumber, info.name, deliverySystemMask, hybrid? "true" : "false");
	}
}

//
void CFrontend::Init(void)
{
	dprintf(DEBUG_INFO, "CFrontend::Init:\n");
		
	secSetVoltage(SEC_VOLTAGE_13, 15);
	secSetTone(SEC_TONE_OFF, 15);
	setDiseqcType(diseqcType);
}

void CFrontend::setMasterSlave(bool _slave)
{
	dprintf(DEBUG_INFO, "CFrontend::setMasterSlave:\n");
	
	if(slave == _slave)
		return;

	if(_slave) 
	{
		secSetVoltage(SEC_VOLTAGE_OFF, 0);
		secSetTone(SEC_TONE_OFF, 15);
	}
	
	slave = _slave;
}

void CFrontend::Close()
{
	dprintf(DEBUG_INFO, "CFrontend::Close:\n");
	
	if(standby)
		return;
	
	if ( !slave && diseqcType > MINI_DISEQC )
		sendDiseqcStandby();
	
	secSetVoltage(SEC_VOLTAGE_OFF, 0);
	secSetTone(SEC_TONE_OFF, 15);

	tuned = false;
	standby = true;
	
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
	
	dprintf(DEBUG_NORMAL, "CFrontend::Close fe(%d:%d) %s\n", feadapter, fenumber, info.name);
}

void CFrontend::reset(void)
{
	dprintf(DEBUG_INFO, "CFrontend::reset:\n");
	
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
	
	usleep(150000);

	char filename[128];

	sprintf(filename, "/dev/dvb/adapter%d/frontend%d", feadapter, fenumber);
	
	dprintf(DEBUG_INFO, "cFrontend(%d,%d) reset %s\n", feadapter, fenumber, filename);

	if ((fd = ::open(filename, O_RDWR | O_NONBLOCK | O_SYNC)) < 0)
		perror(filename);
	
	usleep(150000);
}

void CFrontend::setMasterSlave()
{
	dprintf(DEBUG_INFO, "CFrontend::setMasterSlave:\n");
	
	secSetVoltage(SEC_VOLTAGE_OFF, 0);
	secSetTone(SEC_TONE_OFF, 15);
}

fe_code_rate_t CFrontend::getCFEC()
{
	if (info.type == FE_QPSK) 
	{
		 return curfe.fec_inner;
	} 
	else 
	{
		return curfe.fec_inner;
	}
}

/* satellites.xml
* FEC auto => fec_inner=0
* FEC 1/2 => fec_inner=1
* FEC 2/3 => fec_inner=2
* FEC 3/4 => fec_inner=3
* FEC 5/6 => fec_inner=4
* FEC 7/8 => fec_inner=5
* FEC 8/9 => fec_inner=6
* FEC 3/5 => fec_inner=7
* FEC 4/5 => fec_inner=8
* FEC 9/10 => fec_inner=9 
*/
// only for dvb-s/dvb-s2
typedef enum dvb_fec 
{
	fAuto,
	f1_2,
	f2_3,
	f3_4,
	f5_6,
	f7_8,
	f8_9,
	f3_5,
	f4_5,
	f9_10,
	fNone = 15
} dvb_fec_t;

fe_code_rate_t CFrontend::getCodeRate(const uint8_t fec_inner, int system)
{
	dvb_fec_t f = (dvb_fec_t) fec_inner;
    fe_code_rate_t fec = FEC_NONE;

	if (system == 0) 
	{
		switch (f) 
		{
			case fNone:
				fec = FEC_NONE;
                break;
			case f1_2:
				fec = FEC_1_2;
                break;
			case f2_3:
				fec = FEC_2_3;
                break;
			case f3_4:
				fec = FEC_3_4;
                break;
			case f5_6:
				fec = FEC_5_6;
                break;
			case f7_8:
				fec = FEC_7_8;
                break;
			default:
				dprintf(DEBUG_INFO, "CFrontend::getCodeRate: no valid fec for DVB-S set FEC_AUTO\n");
			case fAuto:
				fec = FEC_AUTO;
                break;
		}
	} 
	else if(system == 1)
	{
		switch (f) 
		{
			case f1_2:
				fec = FEC_S2_QPSK_1_2;	//1+9
                break;
			case f2_3:
				fec = FEC_S2_QPSK_2_3; //2+9
                break;
			case f3_4:
				fec = FEC_S2_QPSK_3_4; //3+9
                break;
			case f3_5:
				fec = FEC_S2_QPSK_3_5; //4+9
                break;
			case f4_5:
				fec = FEC_S2_QPSK_4_5; //5+9
                break;
			case f5_6:
				fec = FEC_S2_QPSK_5_6; //6+9
                break;
			case f7_8:
				fec = FEC_S2_QPSK_7_8; //7+9
                break;
			case f8_9:
				fec = FEC_S2_QPSK_8_9; //8+9
                break;
			case f9_10:
				fec = FEC_S2_QPSK_9_10; //9+9
                break;
			default:
				dprintf(DEBUG_INFO, "CFrontend::getCodeRate: no valid fec for DVB-S2 set...\n");
			case fAuto:
				fec = FEC_AUTO;
                break;
		}
	}
	
	return fec;
}

fe_modulation_t CFrontend::getModulation(const uint8_t modulation)
{
	switch (modulation) 
	{
		case 0x00:
			return QPSK;
		case 0x01:
			return QAM_16;
		case 0x02:
			return QAM_32;
		case 0x03:
			return QAM_64;
		case 0x04:
			return QAM_128;
		case 0x05:
			return QAM_256;
		default:
			return QAM_AUTO;
	}
}

uint32_t CFrontend::getFrequency(void) const
{
	return curfe.frequency;
}

uint8_t CFrontend::getPolarization(void) const
{
	return currentTransponder.feparams.polarization;
}

uint32_t CFrontend::getRate()
{
	uint32_t rate = 0;

	if (info.type == FE_QPSK) 
	{
		rate = curfe.symbol_rate;
	} 
	else if(info.type == FE_QAM)
	{
		rate = curfe.symbol_rate;
	}

	return rate;
}

fe_status_t CFrontend::getStatus(void) const
{ 
	fe_status_t status;
	
	if( ::ioctl(fd, FE_READ_STATUS, &status) < 0)
		perror("FE_READ_STATUS");

	return (fe_status_t) (status & FE_HAS_LOCK);
}

FrontendParameters CFrontend::getFrontend(void) const
{
	FrontendParameters feparams;
	
	return feparams;
	
}

uint32_t CFrontend::getBitErrorRate(void) const
{
	uint32_t ber = 0;

	if(::ioctl(fd, FE_READ_BER, &ber) < 0)
	      perror("FE_READ_BER");

	return ber;
}

uint16_t CFrontend::getSignalStrength(void) const
{
	uint16_t strength = 0;

	if(::ioctl(fd, FE_READ_SIGNAL_STRENGTH, &strength) < 0)
	      perror("FE_READ_SIGNAL_STRENGHT");

	return strength;
}

uint16_t CFrontend::getSignalNoiseRatio(void) const
{
	uint16_t snr = 0;

	if(::ioctl(fd, FE_READ_SNR, &snr) < 0)
		perror("FE_READ_SNR");

	return snr;
}

uint32_t CFrontend::getUncorrectedBlocks(void) const
{
	uint32_t blocks = 0;

	if(::ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &blocks) < 0)
		perror("FE_READ_UNCORRECTED_BLOCKS");

	return blocks;
}

void CFrontend::getDelSys(int f, int m, char *&fec, char *&sys, char *&mod)
{
	// sys/mod
	if (info.type == FE_QPSK) 
	{
		if (f < FEC_S2_QPSK_1_2) 
		{
			sys = (char *)"DVB-S";
			mod = (char *)"QPSK";
		}		
		else if (f < FEC_S2_8PSK_1_2) 
		{
			sys = (char *)"DVB-S2";
			mod = (char *)"QPSK";
		}		
		else 
		{
			sys = (char *)"DVB-S2";
			mod = (char *)"8PSK";
		}
	} 
	else if (info.type == FE_QAM || info.type == FE_OFDM || info.type == FE_ATSC) 
	{
		if ( info.type == FE_QAM )
			sys = (char *)"DVB-C";
		else if (info.type == FE_OFDM ) 
			sys = (char *)"DVB-T";
		else if (info.type == FE_ATSC)
        		sys = (char *)"DVB_A";

		switch (m) 
		{
			case QAM_16:
				mod = (char *)"QAM_16";
				break;
			case QAM_32:
				mod = (char *)"QAM_32";
				break;
			case QAM_64:
				mod = (char *)"QAM_64";
				break;
			case QAM_128:
				mod = (char *)"QAM_128";
				break;
			case QAM_256:
				mod = (char *)"QAM_256";
				break;
			case QAM_AUTO:
			default:
				mod = (char *)"QAM_AUTO";
				break;
		}
	}

	// fec
	switch (f) 
	{
		case FEC_NONE:
			fec = (char *)"FEC_NONE";
			break;
				
		case FEC_1_2:		  
		case FEC_S2_QPSK_1_2:		  
		case FEC_S2_8PSK_1_2:		  
			fec = (char *)"1/2";
			break;

		case FEC_2_3:		  
		case FEC_S2_QPSK_2_3:		  
		case FEC_S2_8PSK_2_3:		  
			fec = (char *)"2/3";
			break;

		case FEC_3_4:		  
		case FEC_S2_QPSK_3_4:		  
		case FEC_S2_8PSK_3_4:		  
			fec = (char *)"3/4";
			break;

		case FEC_S2_QPSK_3_5:		  
		case FEC_S2_8PSK_3_5:
		  
			fec = (char *)"3/5";
			break;			

		case FEC_4_5:		  
		case FEC_S2_QPSK_4_5:		  
		case FEC_S2_8PSK_4_5:		  
			fec = (char *)"4/5";
			break;

		case FEC_5_6:		  
		case FEC_S2_QPSK_5_6:		  
		case FEC_S2_8PSK_5_6:	  
			fec = (char *)"5/6";
			break;

		case FEC_6_7:
			fec = (char *)"6/7";
			break;

		case FEC_7_8:		  
		case FEC_S2_QPSK_7_8:		  
		case FEC_S2_8PSK_7_8:		  
			fec = (char *)"7/8";
			break;

		case FEC_8_9:		  
		case FEC_S2_QPSK_8_9:		  
		case FEC_S2_8PSK_8_9:		  
			fec = (char *)"8/9";
			break;
			
		case FEC_S2_QPSK_9_10:		  
		case FEC_S2_8PSK_9_10:		  
			fec = (char *)"9/10";
			break;			
			
		default:
			dprintf(DEBUG_INFO, "CFrontend::getDelSys: fe(%d:%d) getDelSys: unknown FEC: %d !!!\n", feadapter, fenumber, f);
		
		case FEC_AUTO:
			fec = (char *)"AUTO";
			break;
	}
}

//
uint32_t CFrontend::getFEBandwidth(fe_bandwidth_t bandwidth)
{
	uint32_t bandwidth_hz = 0;	// BANDWIDTH_AUTO

	switch (bandwidth) 
	{
		case BANDWIDTH_8_MHZ:
		default:
			bandwidth_hz  = 8000000;
			break;
			
		case BANDWIDTH_7_MHZ:
			bandwidth_hz  = 7000000;
			break;
			
		case BANDWIDTH_6_MHZ:
			bandwidth_hz  = 6000000;
			break;

		case BANDWIDTH_5_MHZ:
			bandwidth_hz  = 5000000;
			break;
			
		case BANDWIDTH_1_712_MHZ:
			bandwidth_hz  = 1712000;
			break;
			
		case BANDWIDTH_10_MHZ:
			bandwidth_hz  = 10000000;
			break;

		case BANDWIDTH_AUTO:
			bandwidth_hz  = 0;
	}

	return bandwidth_hz;
}

//
#define TIME_STEP 		200
#define TIMEOUT_MAX_MS		9000

struct dvb_frontend_event CFrontend::getEvent(void)
{
	struct dvb_frontend_event event;
	struct pollfd pfd;
	static unsigned int timedout = 0;

	TIMER_INIT();
	
	int msec = TIME_STEP;

	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;
	
	memset(&event, 0, sizeof(struct dvb_frontend_event));
	
	dprintf(DEBUG_INFO, "cFrontend::getEvent: fe(%d:%d) max timeout: %d\n", feadapter, fenumber, TIMEOUT_MAX_MS);
	
	TIMER_START();

	while ((int) timer_msec < TIMEOUT_MAX_MS) 
	{
		int ret = poll(&pfd, 1, TIMEOUT_MAX_MS);

		if (ret < 0) 
		{
			perror("CFrontend::getEvent poll");
			continue;
		}

		if (ret == 0) 
		{
			TIMER_STOP("CFrontend::getEvent: poll timeout, time");
			msec += TIME_STEP;
			continue;
		}		

		if (pfd.revents & (POLLIN | POLLPRI)) 
		{
			TIMER_STOP("CFrontend::getEvent: poll has event after");

			memset(&event, 0, sizeof(struct dvb_frontend_event));

			if( ::ioctl(fd, FE_GET_EVENT, &event) < 0 )
			{
				perror("CFrontend::getEvent ioctl");
				dprintf(DEBUG_INFO, "FD=%d RET=%d errno=%d\n", fd, ret, errno);
				continue;
			}

			if (event.status & FE_HAS_LOCK) 
			{
				dprintf(DEBUG_NORMAL, "cFrontend::getEvent fe(%d:%d) FE_HAS_LOCK: freq %lu\n", feadapter, fenumber, (long unsigned int)event.parameters.frequency);
				tuned = true;
				break;
			} 
			else if (event.status & FE_TIMEDOUT) 
			{
				if(timedout < timer_msec)
					timedout = timer_msec;
				
				dprintf(DEBUG_INFO, "cFrontend::getEvent fe(%d:%d) FE_TIMEDOUT\n", feadapter, fenumber);
			} 
			else 
			{
				if (event.status & FE_HAS_SIGNAL)
					printf("cFrontend::getEvent fe(%d:%d) FE_HAS_SIGNAL\n", feadapter, fenumber);
				if (event.status & FE_HAS_CARRIER)
					printf("cFrontend::getEvent fe(%d:%d) FE_HAS_CARRIER\n", feadapter, fenumber);
				if (event.status & FE_HAS_VITERBI)
					printf("cFrontend::getEvent fe(%d:%d) FE_HAS_VITERBI\n", feadapter, fenumber);
				if (event.status & FE_HAS_SYNC)
					printf("cFrontend::getEvent fe(%d:%d) FE_HAS_SYNC\n", feadapter, fenumber);
				if (event.status & FE_REINIT)
					printf("cFrontend::getEvent fe(%d:%d) FE_REINIT\n", feadapter, fenumber);
			}
		} 
		else if (pfd.revents & POLLHUP) 
		{
			TIMER_STOP("CFrontend::getEvent: poll hup after");
			reset();
		}
	}

	return event;
}

// set frontend
/// S2API ///
#if HAVE_DVB_API_VERSION >= 5
void CFrontend::setFrontend(const FrontendParameters *feparams, bool /*nowait*/)
{
	dprintf(DEBUG_NORMAL, "CFontend::setFrontend: fe(%d:%d) delsys:0x%x (feparams.delsys:0x%x)\n", feadapter, fenumber, deliverySystemMask, feparams->delsys);
	
	fe_delivery_system delsys = SYS_DVBS;
	fe_modulation_t modulation = QPSK;
	fe_code_rate_t fec_inner = FEC_AUTO; 
	fe_rolloff_t rolloff = ROLLOFF_35;
	int fec;
	fe_pilot_t pilot = PILOT_OFF;

	// decode the needed settings for DVB_S/S2
	if (feparams->delsys == DVB_S || feparams->delsys == DVB_S2 || feparams->delsys == DVB_S2X)
	{
		fec_inner = feparams->fec_inner;
		//delsys = dvbs_get_delsys(fec_inner);
		delsys = getFEDeliverySystem(feparams->delsys);
		modulation = dvbs_get_modulation(fec_inner);
		rolloff = dvbs_get_rolloff(delsys);
		
		//
		switch ((int)fec_inner) 
		{
			case FEC_1_2:			  
			case FEC_S2_QPSK_1_2:			  
			case FEC_S2_8PSK_1_2:			  
				fec = FEC_1_2;
				break;

			case FEC_2_3:			  
			case FEC_S2_QPSK_2_3:			  
			case FEC_S2_8PSK_2_3:			  
				fec = FEC_2_3;

				if (modulation == PSK_8)
					pilot = PILOT_ON;
				break;

			case FEC_3_4:			  
			case FEC_S2_QPSK_3_4:			  
			case FEC_S2_8PSK_3_4:			  
				fec = FEC_3_4;

				if (modulation == PSK_8)
					pilot = PILOT_ON;
				break;

			case FEC_S2_QPSK_3_5:			  
			case FEC_S2_8PSK_3_5:			  
				fec = FEC_3_5;

				if (modulation == PSK_8)
					pilot = PILOT_ON;
				break;				

			case FEC_4_5:			  
			case FEC_S2_QPSK_4_5:			  
			case FEC_S2_8PSK_4_5:			  
				fec = FEC_4_5;
				break;

			case FEC_5_6:			  
			case FEC_S2_QPSK_5_6:			  
			case FEC_S2_8PSK_5_6:		  
				fec = FEC_5_6;

				if (modulation == PSK_8)
					pilot = PILOT_ON;
				break;

			case FEC_6_7:
				fec = FEC_6_7;
				break;

			case FEC_7_8:			  
			case FEC_S2_QPSK_7_8:			  
			case FEC_S2_8PSK_7_8:			  
				fec = FEC_7_8;
				break;

			case FEC_8_9:			  
			case FEC_S2_QPSK_8_9:			  
			case FEC_S2_8PSK_8_9:			  
				fec = FEC_8_9;
				break;
				
			case FEC_S2_QPSK_9_10:			  
			case FEC_S2_8PSK_9_10:			  
				fec = FEC_9_10;
				break;
				
			default:
				dprintf(DEBUG_NORMAL, "CFrontend::setFrontend: fe(%d:%d) DEMOD: unknown FEC: %d\n", feadapter, fenumber, fec_inner);
			
			case FEC_AUTO:			  
			case FEC_S2_AUTO:			  
				fec = FEC_AUTO;
				break;
		}
	}

	//
	memset(&Frontend, 0, sizeof(Frontend));
  	memset(&CmdSeq, 0, sizeof(CmdSeq));

  	CmdSeq.props = Frontend;

	// clear
  	SETCMD(DTV_CLEAR, 0);

	if (::ioctl(fd, FE_SET_PROPERTY, &CmdSeq) < 0) 
	{
		perror("FE_SET_PROPERTY");
     	}

  	CmdSeq.num = 0;

	// check frontend status
	struct dvb_frontend_event ev;

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;

	int ret = poll(&pfd, 1, 100);

	if (ret > 0) 
	{
		if (::ioctl(fd, FE_GET_EVENT, &ev) < 0)
			tuned = false;
	}

	//
	if (feparams->delsys == DVB_S || feparams->delsys == DVB_S2)
	{
		// common for DVB-S and DVB-S2
		SETCMD(DTV_DELIVERY_SYSTEM, delsys);
		
		if (diseqcType == DISEQC_UNICABLE)
		{
			SETCMD(DTV_FREQUENCY, sendEN50494TuningCommand(feparams->frequency, currentToneMode == SEC_TONE_ON, currentVoltage == SEC_VOLTAGE_18, !!uni_lnb));
		}
		else if (diseqcType == DISEQC_UNICABLE2)
		{
			SETCMD(DTV_FREQUENCY, sendEN50607TuningCommand(feparams->frequency, currentToneMode == SEC_TONE_ON, currentVoltage == SEC_VOLTAGE_18, uni_lnb));
		}
		else
		{
			SETCMD(DTV_FREQUENCY, feparams->frequency);
		}
		
		SETCMD(DTV_MODULATION, modulation);
		SETCMD(DTV_SYMBOL_RATE, feparams->symbol_rate);
		SETCMD(DTV_INNER_FEC, fec );
		SETCMD(DTV_INVERSION, feparams->inversion);
	
		//DVB-S2
		if (getFEDeliverySystem(feparams->delsys) == SYS_DVBS2) 
		{
			SETCMD(DTV_ROLLOFF, rolloff);
			SETCMD(DTV_PILOT, pilot);		
		}
	}
	else if (feparams->delsys == DVB_C)
	{
		SETCMD(DTV_DELIVERY_SYSTEM, getFEDeliverySystem(feparams->delsys));
     		SETCMD(DTV_FREQUENCY, feparams->frequency);
     		SETCMD(DTV_INVERSION, feparams->inversion);
     		SETCMD(DTV_SYMBOL_RATE, feparams->symbol_rate);
		SETCMD(DTV_INNER_FEC, feparams->fec_inner);
     		SETCMD(DTV_MODULATION, feparams->modulation);
	}
	else if (feparams->delsys == DVB_T || feparams->delsys == DVB_T2 || feparams->delsys == DVB_DTMB)
	{
     		SETCMD(DTV_DELIVERY_SYSTEM, getFEDeliverySystem(feparams->delsys));
     		SETCMD(DTV_FREQUENCY, feparams->frequency);
     		SETCMD(DTV_CODE_RATE_LP, feparams->code_rate_LP);
     		SETCMD(DTV_CODE_RATE_HP, feparams->code_rate_HP);
     		SETCMD(DTV_MODULATION, feparams->modulation);
     		SETCMD(DTV_TRANSMISSION_MODE, feparams->transmission_mode);
     		SETCMD(DTV_GUARD_INTERVAL, feparams->guard_interval);
     		SETCMD(DTV_HIERARCHY, feparams->hierarchy_information);
     		SETCMD(DTV_BANDWIDTH_HZ, getFEBandwidth(feparams->bandwidth));
     		SETCMD(DTV_INVERSION, feparams->inversion);
     		
     		//
     		if (feparams->delsys == DVB_T2)
     		{
#if defined DTV_STREAM_ID
			SETCMD(DTV_STREAM_ID, feparams->plp_id);
#elif defined DTV_DVBT2_PLP_ID
			SETCMD(DTV_DVBT2_PLP_ID, feparams->plp_id);
#endif
		}
	}
	else if(feparams->delsys == DVB_A)
	{
		SETCMD(DTV_DELIVERY_SYSTEM, getFEDeliverySystem(feparams->delsys));
     		SETCMD(DTV_FREQUENCY, feparams->frequency);
     		SETCMD(DTV_MODULATION, feparams->modulation);
     		SETCMD(DTV_INVERSION, feparams->inversion);
	}
	else
	{
		dprintf(DEBUG_INFO, "CFrontend::setFrontend: Unknow Frontend Type\n");
	}

	//
	SETCMD(DTV_TUNE, 0);

	// tune
  	if (::ioctl(fd, FE_SET_PROPERTY, &CmdSeq) < 0) 
	{
		perror("FE_SET_PROPERTY");
     	}
}
#else //api3
void CFrontend::setFrontend(const FrontendParameters * feparams, bool nowait)
{
	dprintf(DEBUG_NORMAL, "CFontend::setFrontend: fe(%d:%d)\n", feadapter, fenumber);
		
	fe_modulation_t modulation = QAM_16;
	fe_code_rate_t fec_inner = FEC_3_4;
	
	switch (info.type) 
	{
		case FE_QPSK:
			fec_inner = feparams->fec_inner;
			modulation = dvbs_get_modulation(fec_inner);
			break;

		case FE_QAM:      
			fec_inner = feparams->fec_inner;
			modulation = feparams->modulation;
			break;

		case FE_OFDM:
			modulation = feparams->modulation;
			fec_inner = feparams->code_rate_HP;
			break;
			
		case FE_ATSC:
		default:
			dprintf(DEBUG_NORMAL, "CFrontend::setFrontend: unknown frontend type, exiting\n");
			break;
	}
	
	// getDelSys
	//char *f, *s, *m;
	//getDelSys(fec_inner, modulation, f, s, m);
	//dprintf(DEBUG_INFO, "cFrontend::setFrontend fe(%d:%d) DEMOD: FEC %s system %s modulation %s\n", feadapter, fenumber, f, s, m);

	// clear event queue
	struct dvb_frontend_event event;
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;

	int ret = poll(&pfd, 1, 100);
	
	memset(&event, 0, sizeof(struct dvb_frontend_event));
	
	if(ret > 0)
	{	
		if(::ioctl(fd, FE_GET_EVENT, &event) < 0)
			perror("FE_GET_EVENT");
		
		dprintf(DEBUG_NORMAL, "CFrontend::setFrontend: fe(%d:%d) CLEAR DEMOD: event status %d\n", feadapter, fenumber, event.status);
	}

	// set frontend
	if ((::ioctl(fd, FE_SET_FRONTEND, feparams)) < 0) 
		perror("FE_SET_FRONTEND");
}
#endif

void CFrontend::secSetTone(const fe_sec_tone_mode_t toneMode, const uint32_t ms)
{
	if ( slave || info.type != FE_QPSK)
		return;

	if (currentToneMode == toneMode)
		return;
	
	// unicable
	if (diseqcType > DISEQC_ADVANCED)
	{
		/* this is too ugly for words. the "currentToneMode" is the only place
		   where the global "highband" state is saved. So we need to fake it for
		   unicable and still set the tone on... */
		currentToneMode = toneMode; /* need to know polarization for unicable */
		::ioctl(fd, FE_SET_TONE, SEC_TONE_OFF); /* tone must be off except for the time
							  of sending commands */
		return;
	}

	dprintf(DEBUG_INFO, "CFrontend::secSetTone: fe(%d:%d) tone %s\n", feadapter, fenumber, toneMode == SEC_TONE_ON ? "on" : "off");

	if (::ioctl(fd, FE_SET_TONE, toneMode) == 0) 
	{
		currentToneMode = toneMode;
		usleep(1000 * ms);
	}
}

void CFrontend::secSetVoltage(const fe_sec_voltage_t voltage, const uint32_t ms)
{
	dprintf(DEBUG_NORMAL, "CFrontend::secSetVoltage: fe(%d:%d) voltage %s\n", feadapter, fenumber, voltage == SEC_VOLTAGE_OFF ? "OFF" : voltage == SEC_VOLTAGE_13 ? "13" : "18");
	
	if ( slave || info.type != FE_QPSK)
		return;
	
	if (currentVoltage == voltage)
		return;
	
	// unicable
	if (diseqcType > DISEQC_ADVANCED)
	{
		/* see my comment in secSetTone... */
		currentVoltage = voltage; /* need to know polarization for unicable */
		::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13); /* voltage must not be 18V */
		return;
	}

	//dprintf(DEBUG_INFO, "CFrontend::secSetVoltage: fe(%d:%d) voltage %s\n", feadapter, fenumber, voltage == SEC_VOLTAGE_OFF ? "OFF" : voltage == SEC_VOLTAGE_13 ? "13" : "18");

	if (::ioctl(fd, FE_SET_VOLTAGE, voltage) == 0) 
	{
		currentVoltage = voltage;
		usleep(1000 * ms);	// FIXME : is needed ?
	}
}

void CFrontend::secResetOverload(void)
{
}

void CFrontend::sendDiseqcCommand(const struct dvb_diseqc_master_cmd *cmd, const uint32_t ms)
{
	if ( slave || info.type != FE_QPSK) 
		return;

	printf("[fe%d] Diseqc cmd: ", fenumber);
	for (int i = 0; i < cmd->msg_len; i++)
		printf("0x%X ", cmd->msg[i]);
	printf("\n");

	if (::ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, cmd) == 0)
		usleep(1000 * ms);
}

uint32_t CFrontend::getDiseqcReply(const int /*timeout_ms*/) const
{
	return 0;
}

void CFrontend::sendToneBurst(const fe_sec_mini_cmd_t burst, const uint32_t ms)
{
	if ( slave || info.type != FE_QPSK) 
		return;
	
	if (::ioctl(fd, FE_DISEQC_SEND_BURST, burst) == 0)
		usleep(1000 * ms);
}

void CFrontend::setDiseqcType(const diseqc_t newDiseqcType)
{
	if (info.type != FE_QPSK)
		return;
	
	switch (newDiseqcType) 
	{
		case NO_DISEQC:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: NO_DISEQC\n");
			break;
			
		case MINI_DISEQC:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: MINI_DISEQC\n");
			break;
			
		case SMATV_REMOTE_TUNING:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: SMATV_REMOTE_TUNING\n");
			break;
			
		case DISEQC_1_0:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_1_0\n");
			break;
			
		case DISEQC_1_1:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_1_1\n");
			break;
			
		case DISEQC_1_2:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_1_2\n");
			break;
			
		case DISEQC_ADVANCED:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_ADVANCED\n");
			break;

		case DISEQC_UNICABLE:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_UNICABLE\n");
			break;

		case DISEQC_UNICABLE2:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: DISEQC_UNICABLE2\n");
			break;
			
		default:
			dprintf(DEBUG_INFO, "CFrontend::setDiseqcType: Invalid DiSEqC type\n");
			return;
	}
	
	if (diseqcType != newDiseqcType) 
	{
		diseqcType = newDiseqcType;
		sendDiseqcPowerOn();
		sendDiseqcReset();
	}
}

void CFrontend::setLnbOffsets(int32_t _lnbOffsetLow, int32_t _lnbOffsetHigh, int32_t _lnbSwitch)
{
	lnbOffsetLow = _lnbOffsetLow * 1000;
	lnbOffsetHigh = _lnbOffsetHigh * 1000;
	lnbSwitch = _lnbSwitch * 1000;

	dprintf(DEBUG_INFO, "CFrontend::setLnbOffsets: fe(%d:%d) setLnbOffsets %d/%d/%d\n", feadapter, fenumber, lnbOffsetLow, lnbOffsetHigh, lnbSwitch);
}

void CFrontend::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t command, uint8_t num_parameters, uint8_t parameter1, uint8_t parameter2, int repeat)
{
	if (info.type != FE_QPSK) 
		return;
	
	struct dvb_diseqc_master_cmd cmd;
	int i;

	dprintf(DEBUG_INFO, "CFrontend::sendMotorCommand: fe(%d:%d) sendMotorCommand: cmdtype   = %x, address = %x, cmd   = %x\n", feadapter, fenumber, cmdtype, address, command);
	
	dprintf(DEBUG_INFO, "CFrontend::sendMotorCommand fe(%d:%d) sendMotorCommand: num_parms = %d, parm1   = %x, parm2 = %x\n", feadapter, fenumber, num_parameters, parameter1, parameter2);

	cmd.msg[0] = cmdtype;		//command type
	cmd.msg[1] = address;		//address
	cmd.msg[2] = command;		//command
	cmd.msg[3] = parameter1;
	cmd.msg[4] = parameter2;
	cmd.msg_len = 3 + num_parameters;
	
	secSetVoltage(SEC_VOLTAGE_13, 15);
	secSetTone(SEC_TONE_OFF, 25);

	for(i = 0; i <= repeat; i++)
		sendDiseqcCommand(&cmd, 50);

	dprintf(DEBUG_INFO, "CFrontend::sendMotorCommand: fe(%d:%d) motor command sent.\n", feadapter, fenumber);
	
}

void CFrontend::positionMotor(uint8_t motorPosition)
{
	if (info.type != FE_QPSK) 
		return;
	
	struct dvb_diseqc_master_cmd cmd = 
	{
		{0xE0, 0x31, 0x6B, 0x00, 0x00, 0x00}, 4
	};

	if (motorPosition != 0) 
	{
		secSetVoltage(SEC_VOLTAGE_13, 15);
		secSetTone(SEC_TONE_OFF, 15);
		cmd.msg[3] = motorPosition;
		sendDiseqcCommand(&cmd, 15);
		
		dprintf(DEBUG_INFO, "CFrontend::sendMotorCommand fe(%d:%d) motor positioning command sent.\n", feadapter, fenumber);
	}
}

bool CFrontend::setInput(CZapitChannel * channel, bool nvod)
{
	transponder_list_t::iterator tpI;
	transponder_id_t ct = channel->getTransponderId();

	// if we have the same TP return false
	if (tuned && (ct == currentTransponder.TP_id))
		return false;

	// new TP
	// if nvod
	if (nvod) 
	{
		for (tpI = transponders.begin(); tpI != transponders.end(); tpI++) 
		{
			if ((ct & 0xFFFFFFFFULL) == (tpI->first & 0xFFFFFFFFULL))
				break;
		}
	} 
	else
		tpI = transponders.find(channel->getTransponderId());

	if (tpI == transponders.end()) 
	{
		dprintf(DEBUG_INFO, "CFrontend::setInput: Transponder 0x%llx for channel 0x%llx not found\n", ct, channel->getChannelID());
		return false;
	}

	currentTransponder.TP_id = tpI->first;

	setInput(channel->getSatellitePosition(), tpI->second.feparams.frequency, tpI->second.feparams.polarization);

	return true;
}

void CFrontend::setInput(t_satellite_position satellitePosition, uint32_t frequency, uint8_t polarization)
{
	sat_iterator_t sit = satellitePositions.find(satellitePosition);

	if (info.type == FE_QPSK)
	{
		dprintf(DEBUG_NORMAL, "CFrontend::setInput: fe(%d:%d) (delsys:0x&x) SatellitePosition %d -> %d\n", feadapter, fenumber, deliverySystemMask, currentSatellitePosition, satellitePosition);
	}
	else
		dprintf(DEBUG_NORMAL, "CFrontend::setInput: fe(%d:%d)(delsys:0x%x)\n", feadapter, fenumber, deliverySystemMask);

	// unicable uses diseqc parameter for input selection
	uni_lnb = sit->second.diseqc;

	// set lnb offset
	if(info.type == FE_QPSK)
		setLnbOffsets(sit->second.lnbOffsetLow, sit->second.lnbOffsetHigh, sit->second.lnbSwitch);

	if (diseqcType == DISEQC_UNICABLE || diseqcType == DISEQC_UNICABLE2)
		return;

	// set diseqc
	if (diseqcType != DISEQC_ADVANCED) 
	{
		setDiseqc(sit->second.diseqc, polarization, frequency);
		return;
	}

	// set simple or committed diseqc
	if (sit->second.diseqc_order == COMMITED_FIRST) 
	{
		if (setDiseqcSimple(sit->second.commited, polarization, frequency))
			uncommitedInput = 255;
		
		sendUncommittedSwitchesCommand(sit->second.uncommited);
	} 
	else 
	{
		if (sendUncommittedSwitchesCommand(sit->second.uncommited))
			diseqc = -1;
		
		setDiseqcSimple(sit->second.commited, polarization, frequency);
	}
}

//
bool CFrontend::tuneChannel(CZapitChannel * channel, bool nvod)
{
	dprintf(DEBUG_NORMAL, "CFrontend::tuneChannel: fe(%d:%d) delsys:0x%x tpid 0x%llx (channel_tp: 0x%llx nvod:%s)\n", feadapter, fenumber, deliverySystemMask, currentTransponder.TP_id, channel->getTransponderId(), nvod? "true" : "false");

	//
	transponder_list_t::iterator transponder = transponders.find(channel->getTransponderId());

	if (transponder == transponders.end())
		return false;

	return tuneFrequency(&transponder->second.feparams, false);
}

//
int CFrontend::tuneFrequency(FrontendParameters * feparams, bool nowait)
{
	dprintf(DEBUG_NORMAL, "CFrontend::tuneFrequency: fe(%d:%d) delsys:0x%x (params.delsys:0x%x)\n", feadapter, fenumber, deliverySystemMask, feparams->delsys);
	
	transponder TP;

	// save current feparams
	memcpy(&curfe, feparams, sizeof(FrontendParameters));
	
	// save feparams im TP struct
	memcpy(&TP.feparams, feparams, sizeof(FrontendParameters));

	//FIXME:pol für Sat TEST
	if(info.type == FE_QPSK)
		TP.feparams.polarization = feparams->polarization;

	return setParameters(&TP, nowait);
}

//
int CFrontend::setParameters(transponder * TP, bool nowait)
{
	int freq_offset = 0;
	
#if HAVE_DVB_API_VERSION >= 5
	if (TP->feparams.delsys == DVB_S || TP->feparams.delsys == DVB_S2)
#else
	if (info.type == FE_QPSK)
#endif
	{
		bool high_band;

		if ((int)TP->feparams.frequency < lnbSwitch) 
		{
			high_band = false;
			freq_offset = lnbOffsetLow;
		} 
		else 
		{
			high_band = true;
			freq_offset = lnbOffsetHigh;
		}

		// calculate freq for sat
		TP->feparams.frequency = abs((int)(TP->feparams.frequency - freq_offset));
		
		// set Sec for Sat
		setSec(TP->diseqc, TP->feparams.polarization, high_band);

		dprintf(DEBUG_NORMAL, "CFrontend::setParameters: fe(%d:%d) freq= %d (offset= %d) fec= %d\n", feadapter, fenumber, TP->feparams.frequency, freq_offset, TP->feparams.fec_inner);
	}

	//
#if HAVE_DVB_API_VERSION >= 5
	if (TP->feparams.delsys == DVB_C)
#else
	if (info.type == FE_QAM)
#endif
	{
		// freq convert to hz cable freq are in cables.xml in khz
		if (TP->feparams.frequency < 1000*1000)
			TP->feparams.frequency = TP->feparams.frequency * 1000;
		
		dprintf(DEBUG_NORMAL, "cFrontend::setParameters: fe(%d,%d) freq= %d fec= %d mod= %d inv= %d\n", feadapter, fenumber, TP->feparams.frequency, TP->feparams.fec_inner, TP->feparams.modulation, TP->feparams.inversion);
	}

	//
#if HAVE_DVB_API_VERSION >= 5
	if (TP->feparams.delsys == DVB_T || TP->feparams.delsys == DVB_T2)
#else
	if (info.type == FE_OFDM) 
#endif
	{
		if (TP->feparams.frequency < 1000*1000)
			TP->feparams.frequency = TP->feparams.frequency * 1000;
			
		// setSecVoltage
		secSetVoltage(powered ? SEC_VOLTAGE_13 : SEC_VOLTAGE_OFF, 100);
			
		dprintf(DEBUG_NORMAL, "cFrontend::setParameters: fe(%d:%d) freq= %d band=%d HP=%d LP=%d const=%d trans=%d guard=%d hierarchy=%d inv=%d plp_id=%d\n", feadapter, fenumber, TP->feparams.frequency, TP->feparams.bandwidth, TP->feparams.code_rate_HP, TP->feparams.code_rate_LP, TP->feparams.modulation, TP->feparams.transmission_mode, TP->feparams.guard_interval, TP->feparams.hierarchy_information, TP->feparams.inversion, (TP->feparams.delsys = DVB_T2)?TP->feparams.plp_id : 0);
	}
	
	do {
		tuned = false;

		setFrontend(&TP->feparams, nowait);
		
		getEvent();
	} while (0);

	dprintf(DEBUG_NORMAL, "CFrontend::setParameters: fe(%d:%d) %s\n", feadapter, fenumber, tuned? "tuned" : "tune failed");

#if HAVE_DVB_API_VERSION >= 5
	if (forcedDelSys == DVB_S || forcedDelSys == DVB_S2)
#else
	if (info.type == FE_QPSK)
#endif
		TP->feparams.frequency += freq_offset;

	return tuned;
}

/* frequency is the IF-frequency (950-2100), what a stupid spec...
   high_band, horizontal, bank are actually bool (0/1)
   bank specifies the "switch bank" (as in Mini-DiSEqC A/B) */
uint32_t CFrontend::sendEN50494TuningCommand(const uint32_t frequency, const int high_band, const int horizontal, const int bank)
{
	uint32_t bpf = uni_qrg;

	struct dvb_diseqc_master_cmd cmd = {
		{0xe0, 0x10, 0x5a, 0x00, 0x00, 0x00}, 5
	};
	unsigned int t = (frequency / 1000 + bpf + 2) / 4 - 350;
	if (t < 1024 && uni_scr >= 0 && uni_scr < 8)
	{
		uint32_t ret = (t + 350) * 4000 - frequency;
		dprintf(DEBUG_NORMAL, "[unicable] 18V=%d TONE=%d, freq=%d qrg=%d scr=%d bank=%d ret=%d\n", currentVoltage == SEC_VOLTAGE_18, currentToneMode == SEC_TONE_ON, frequency, bpf, uni_scr, bank, ret);
		
		if ( !slave && info.type == FE_QPSK) 
		{
			cmd.msg[3] = (t >> 8)			|	/* highest 3 bits of t */
					(uni_scr << 5)		|	/* adress */
					(bank << 4)		|	/* input 0/1 */
					(horizontal << 3)	|	/* horizontal == 0x08 */
					(high_band) << 2;		/* high_band  == 0x04 */

			cmd.msg[4] = t & 0xFF;
			::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
			usleep(15 * 1000);		/* en50494 says: >4ms and < 22 ms */
			sendDiseqcCommand(&cmd, 50);	/* en50494 says: >2ms and < 60 ms */
			::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
		}
		
		return (t + 350) * 4000 - frequency;
	}
	dprintf(DEBUG_NORMAL, "ooops. t > 1024? (%d) or uni_scr out of range? (%d)", t, uni_scr);
	
	return 0;
}

uint32_t CFrontend::sendEN50607TuningCommand(const uint32_t frequency, const int high_band, const int horizontal, const int bank)
{
	uint32_t bpf = uni_qrg;
	struct dvb_diseqc_master_cmd cmd = { {0x70, 0x00, 0x00, 0x00, 0x00, 0x00}, 4 };

	unsigned int t = frequency / 1000 - 100;
	if (t < 0x800 && uni_scr >= 0 && uni_scr < 32)
	{
		uint32_t ret = bpf * 1000;
		dprintf(DEBUG_NORMAL, "[unicable-JESS] 18V=%d TONE=%d, freq=%d qrg=%d scr=%d bank=%d ret=%d\n", currentVoltage == SEC_VOLTAGE_18, currentToneMode == SEC_TONE_ON, frequency, bpf, uni_scr, bank, ret);
		if (!slave && info.type == FE_QPSK)
		{
			cmd.msg[1] = ((uni_scr & 0x1F) << 3)		|	/* user band adress ( 0 to 31) */
			/* max. possible tuning word = 0x7FF */
				((t >> 8) & 0x07);				/* highest 3 bits of t (MSB) */
			cmd.msg[2] = t & 0xFF;					/* tuning word (LSB) */
			cmd.msg[3] = (0 << 4)				|	/* no uncommited switch */
			/* I really don't know if the combines of option and position bits are right here,
			because I can'test it, assuming here 4 sat positions */
				((bank & 0x03) << 2)			|	/* input 0/1/2/3 */
				(horizontal << 1)			|	/* horizontal == 0x02 */
				high_band;					/* high_band  == 0x01 */
			::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
			usleep(15 * 1000);					/* en50494 says: >4ms and < 22 ms */
			sendDiseqcCommand(&cmd, 50);				/* en50494 says: >2ms and < 60 ms */
			::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
		}
		return ret;
	}
	dprintf(DEBUG_NORMAL, "ooops. t > 2047? (%d) or uni_scr out of range? (%d)", t, uni_scr);
	
	return 0;
}


bool CFrontend::sendUncommittedSwitchesCommand(int input)
{
	struct dvb_diseqc_master_cmd cmd = {
		{0xe0, 0x10, 0x39, 0x00, 0x00, 0x00}, 4
	};

	dprintf(DEBUG_INFO, "CFrontend::sendUncommittedSwitchesCommand fe(%d:%d) uncommitted  %d -> %d\n", feadapter, fenumber, uncommitedInput, input);
	
	if ((input < 0) || (uncommitedInput == input))
		return false;

	uncommitedInput = input;
	cmd.msg[3] = 0xF0 + input;

	secSetTone(SEC_TONE_OFF, 20);
	sendDiseqcCommand(&cmd, 125);
	return true;
}

/* off = low band, on - hi band , vertical 13v, horizontal 18v */
bool CFrontend::setDiseqcSimple(int sat_no, const uint8_t pol, const uint32_t frequency)
{
	//for monoblock
	fe_sec_voltage_t v = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	//fe_sec_mini_cmd_t b = (sat_no & 1) ? SEC_MINI_B : SEC_MINI_A;
	bool high_band = ((int)frequency >= lnbSwitch);

	struct dvb_diseqc_master_cmd cmd = {
		{0xe0, 0x10, 0x38, 0x00, 0x00, 0x00}, 4
	};

	dprintf(DEBUG_INFO, "CFrontend::setDiseqcSimple: fe(%d:%d) diseqc input  %d -> %d\n", feadapter, fenumber, diseqc, sat_no);
	
	currentTransponder.diseqc = sat_no;
	
	if( slave)
		return true;

	if ((sat_no >= 0) && (diseqc != sat_no)) 
	{
		diseqc = sat_no;
		dprintf(DEBUG_INFO, "CFrontend::setDiseqcSimple: fe(%d:%d) diseqc no. %d\n", feadapter, fenumber, sat_no);

		cmd.msg[3] = 0xf0 | (((sat_no * 4) & 0x0f) | (high_band ? 1 : 0) | ((pol & 1) ? 0 : 2));

		//for monoblock - needed ??
		secSetVoltage(v, 15);
		//secSetVoltage(SEC_VOLTAGE_13, 15);//FIXME for test
		secSetTone(SEC_TONE_OFF, 20);

		sendDiseqcCommand(&cmd, 100);
		return true;
	}
	
	return false;
}

void CFrontend::setDiseqc(int sat_no, const uint8_t pol, const uint32_t frequency)
{
	uint8_t loop;
	bool high_band = ((int)frequency >= lnbSwitch);
	struct dvb_diseqc_master_cmd cmd = { {0xE0, 0x10, 0x38, 0xF0, 0x00, 0x00}, 4 };
	
	//fe_sec_voltage_t polarity = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	//fe_sec_tone_mode_t tone = high_band ? SEC_TONE_ON : SEC_TONE_OFF;//seems needed?
	
	fe_sec_mini_cmd_t b = (sat_no & 1) ? SEC_MINI_B : SEC_MINI_A;
	int delay = 0;
	
	//test
	if (info.type != FE_QPSK) 
		return;

	if( slave)
		return;

	//secSetVoltage(polarity, 15);	/* first of all set the "polarization" */
	//secSetTone(tone, 1);		/* set the "band" */

	//secSetVoltage(SEC_VOLTAGE_13, 15);//FIXME for test
	secSetTone(SEC_TONE_OFF, 20);

	for (loop = 0; loop <= diseqcRepeats; loop++) 
	{
		//usleep(50*1000);                  /* sleep at least 50 milli seconds */

		if (diseqcType == MINI_DISEQC)
			sendToneBurst(b, 1);

		delay = 0;
		if (diseqcType == DISEQC_1_1) 
		{	/* setup the uncommited switch first */
			
			delay = 60;	// delay for 1.0 after 1.1 command
			cmd.msg[2] = 0x39;	/* port group = uncommited switches */
#if 1
			/* for 16 inputs */
			cmd.msg[3] = 0xF0 | ((sat_no / 4) & 0x03);
			sendDiseqcCommand(&cmd, 100);	/* send the command to setup second uncommited switch and wait 100 ms !!! */
#else
			/* for 64 inputs */
			uint8_t cascade_input[16] = { 0xF0, 0xF4, 0xF8, 0xFC, 0xF1, 0xF5, 0xF9, 0xFD, 0xF2, 0xF6, 0xFA,
				0xFE, 0xF3, 0xF7, 0xFB, 0xFF
			};
			cmd.msg[3] = cascade_input[(sat_no / 4) & 0xFF];
			sendDiseqcCommand(&cmd, 100);	/* send the command to setup first uncommited switch and wait 100 ms !!! */
			cmd.msg[3] &= 0xCF;
			sendDiseqcCommand(&cmd, 100);	/* send the command to setup second uncommited switch and wait 100 ms !!! */
#endif
		}
		if (diseqcType >= DISEQC_1_0) 
		{	/* DISEQC 1.0 */
			
			usleep(delay * 1000);
			//cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] = 0x38;	/* port group = commited switches */
			cmd.msg[3] = 0xF0 | ((sat_no % 4) << 2) | ((pol & 1) ? 0 : 2) | (high_band ? 1 : 0);
			sendDiseqcCommand(&cmd, 100);	/* send the command to setup second commited switch */
		}
		cmd.msg[0] = 0xE1;
		usleep(50 * 1000);	/* sleep at least 50 milli seconds */
	}

	usleep(25 * 1000);

	if (diseqcType == SMATV_REMOTE_TUNING)
		sendDiseqcSmatvRemoteTuningCommand(frequency);

#if 0	// setSec do this, when tune called
	if (high_band)
		secSetTone(SEC_TONE_ON, 20);
#endif
	//secSetTone(tone, 20);
	currentTransponder.diseqc = sat_no;
}

void CFrontend::setSec(const uint8_t /*sat_no*/, const uint8_t pol, const bool high_band)
{
	if (info.type != FE_QPSK) 
		return;
	
	fe_sec_voltage_t v = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t t = high_band ? SEC_TONE_ON : SEC_TONE_OFF;

	secSetVoltage(v, 15);
	secSetTone(t, 15);
	currentTransponder.feparams.polarization = pol & 1;
}

void CFrontend::sendDiseqcPowerOn(void)
{
	if (info.type != FE_QPSK) 
		return;
	
	// FIXME power on can take a while. Should be wait
	// more time here ? 15 ms enough for some switches ?
#if 1
	dprintf(DEBUG_INFO, "CFrontend::sendDiseqcPowerOn: fe(%d:%d) diseqc power on\n", feadapter, fenumber);
	
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x03);
#else
	struct dvb_diseqc_master_cmd cmd = {
		{0xE0, 0x10, 0x03, 0x00, 0x00, 0x00}, 3
	};
	sendDiseqcCommand(&cmd, 100);
#endif
}

void CFrontend::sendDiseqcReset(void)
{
	if (info.type != FE_QPSK) 
		return;
	
	dprintf(DEBUG_INFO, "CFrontend::sendDiseqcReset: fe(%d:%d) diseqc reset\n", feadapter, fenumber);
	
	if (diseqcType < DISEQC_UNICABLE)
	{
		/* Reset && Clear Reset */
		sendDiseqcZeroByteCommand(0xe0, 0x10, 0x00);
		sendDiseqcZeroByteCommand(0xe0, 0x10, 0x01);
	}
	else
	{
		// Send reset to 'all' equipment
		sendDiseqcZeroByteCommand(0xe0, 0x00, 0x00);
	}
}

void CFrontend::sendDiseqcStandby(void)
{
	if (info.type != FE_QPSK) 
		return;
	
	dprintf(DEBUG_INFO, "CFrontend::sendDiseqcStandby: fe(%d:%d) diseqc standby\n", feadapter, fenumber);

	if (diseqcType > DISEQC_ADVANCED)
	{
		/* use ODU_Power_OFF command for unicable or jess here
		to set the used UB frequency of the frontend to standby */
		struct dvb_diseqc_master_cmd cmd = {{0}, 6};
		printf("[fe%d] standby scr: %d\n", fenumber, uni_scr);
		if (diseqcType == DISEQC_UNICABLE)
		{
			cmd.msg[0] = 0xe0;
			cmd.msg[1] = 0x10;
			cmd.msg[2] = 0x5a;
			cmd.msg[3] = ((uni_scr & 0x07) << 5);
			cmd.msg_len = 5;
		}
		if (diseqcType == DISEQC_UNICABLE2)
		{
			cmd.msg[0] = 0x70;
			cmd.msg[1] = ((uni_scr & 0x1F) << 3);
			cmd.msg_len = 4;
		}
		::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
		usleep(15 * 1000);
		sendDiseqcCommand(&cmd, 100);
		::ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
		return;
	}
	
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x02);
}

void CFrontend::sendDiseqcZeroByteCommand(uint8_t framing_byte, uint8_t address, uint8_t command)
{
	if (info.type != FE_QPSK) 
		return;
	
	struct dvb_diseqc_master_cmd diseqc_cmd = {
		{framing_byte, address, command, 0x00, 0x00, 0x00}, 3
	};

	sendDiseqcCommand(&diseqc_cmd, 40);
}

void CFrontend::sendDiseqcSmatvRemoteTuningCommand(const uint32_t frequency)
{
	/* [0] from master, no reply, 1st transmission
	 * [1] intelligent slave interface for multi-master bus
	 * [2] write channel frequency
	 * [3] frequency
	 * [4] frequency
	 * [5] frequency */
	
	if (info.type != FE_QPSK) 
		return;

	struct dvb_diseqc_master_cmd cmd = {
		{0xe0, 0x71, 0x58, 0x00, 0x00, 0x00}, 6
	};

	cmd.msg[3] = (((frequency / 10000000) << 4) & 0xF0) | ((frequency / 1000000) & 0x0F);
	cmd.msg[4] = (((frequency / 100000) << 4) & 0xF0) | ((frequency / 10000) & 0x0F);
	cmd.msg[5] = (((frequency / 1000) << 4) & 0xF0) | ((frequency / 100) & 0x0F);

	sendDiseqcCommand(&cmd, 15);
}

int CFrontend::driveToSatellitePosition(t_satellite_position satellitePosition, bool from_scan)
{
	int waitForMotor = 0;
	int new_position = 0, old_position = 0;
	bool use_usals = 0;

	if(diseqcType == DISEQC_ADVANCED) //FIXME testing
	{
		dprintf(DEBUG_INFO, "CFrontend::driveToSatellitePosition fe(%d:%d) SatellitePosition %d -> %d\n", feadapter, fenumber, currentSatellitePosition, satellitePosition);
		
		bool moved = false;

		sat_iterator_t sit = satellitePositions.find(satellitePosition);
		if (sit == satellitePositions.end()) 
		{
			dprintf(DEBUG_INFO, "CFrontend::driveToSatellitePosition: fe(%d:%d) satellite position %d not found!\n", feadapter, fenumber, satellitePosition);
			
			return 0;
		} 
		else 
		{
			new_position = sit->second.motor_position;
			use_usals = sit->second.use_usals;
		}
		
		sit = satellitePositions.find(currentSatellitePosition);
		if (sit != satellitePositions.end())
			old_position = sit->second.motor_position;

		dprintf(DEBUG_INFO, "CFrontend::driveToSatellitePosition fe(%d:%d) motorPosition %d -> %d usals %s\n", feadapter, fenumber, old_position, new_position, use_usals ? "on" : "off");

		if (currentSatellitePosition == satellitePosition)
			return 0;

		if (use_usals) 
		{
			gotoXX(satellitePosition);
			moved = true;
		} 
		else 
		{
			if (new_position > 0) 
			{
				positionMotor(new_position);
				moved = true;
			}
		}

		if (from_scan || (new_position > 0 && old_position > 0)) 
		{
			waitForMotor = motorRotationSpeed ? 2 + abs(satellitePosition - currentSatellitePosition) / motorRotationSpeed : 0;
		}

		if (moved) 
		{
			waitForMotor = motorRotationSpeed ? 2 + abs(satellitePosition - currentSatellitePosition) / motorRotationSpeed : 0;
			currentSatellitePosition = satellitePosition;
		}
	}

	return waitForMotor;
}

static const int gotoXTable[10] = { 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };

double factorial_div(double value, int x)
{
	if (!x)
		return 1;
	else {
		while (x > 1) {
			value = value / x--;
		}
	}
	return value;
}

double powerd(double x, int y)
{
	int i = 0;
	double ans = 1.0;

	if (!y)
		return 1.000;
	else 
	{
		while (i < y) 
		{
			i++;
			ans = ans * x;
		}
	}
	return ans;
}

double SIN(double x)
{
	int i = 0;
	int j = 1;
	int sign = 1;
	double y1 = 0.0;
	double diff = 1000.0;

	if (x < 0.0) 
	{
		x = -1 * x;
		sign = -1;
	}

	while (x > 360.0 * M_PI / 180) 
	{
		x = x - 360 * M_PI / 180;
	}

	if (x > (270.0 * M_PI / 180)) 
	{
		sign = sign * -1;
		x = 360.0 * M_PI / 180 - x;
	} 
	else if (x > (180.0 * M_PI / 180)) 
	{
		sign = sign * -1;
		x = x - 180.0 * M_PI / 180;
	} 
	else if (x > (90.0 * M_PI / 180)) 
	{
		x = 180.0 * M_PI / 180 - x;
	}

	while (powerd(diff, 2) > 1.0E-16) 
	{
		i++;
		diff = j * factorial_div(powerd(x, (2 * i - 1)), (2 * i - 1));
		y1 = y1 + diff;
		j = -1 * j;
	}
	
	return (sign * y1);
}

double COS(double x)
{
	return SIN(90 * M_PI / 180 - x);
}

double ATAN(double x)
{
	int i = 0;		/* counter for terms in binomial series */
	int j = 1;		/* sign of nth term in series */
	int k = 0;
	int sign = 1;		/* sign of the input x */
	double y = 0.0;		/* the output */
	double deltay = 1.0;	/* the value of the next term in the series */
	double addangle = 0.0;	/* used if arctan > 22.5 degrees */

	if (x < 0.0) 
	{
		x = -1 * x;
		sign = -1;
	}

	while (x > 0.3249196962) 
	{
		k++;
		x = (x - 0.3249196962) / (1 + x * 0.3249196962);
	}
	addangle = k * 18.0 * M_PI / 180;

	while (powerd(deltay, 2) > 1.0E-16) 
	{
		i++;
		deltay = j * powerd(x, (2 * i - 1)) / (2 * i - 1);
		y = y + deltay;
		j = -1 * j;
	}
	
	return (sign * (y + addangle));
}

double ASIN(double x)
{
	return 2 * ATAN(x / (1 + std::sqrt(1.0 - x * x)));
}

double Radians(double number)
{
	return number * M_PI / 180;
}

double Deg(double number)
{
	return number * 180 / M_PI;
}

double Rev(double number)
{
	return number - std::floor(number / 360.0) * 360;
}

double calcElevation(double SatLon, double SiteLat, double SiteLon, int Height_over_ocean = 0)
{
	const double a0 = 0.58804392, a1 = -0.17941557, a2 = 0.29906946E-1, a3 = -0.25187400E-2, a4 = 0.82622101E-4;
	const double f = 1.00 / 298.257;	// Earth flattning factor
	const double r_sat = 42164.57;	// Distance from earth centre to satellite
	const double r_eq = 6378.14;	// Earth radius
	double sinRadSiteLat = SIN(Radians(SiteLat)), cosRadSiteLat = COS(Radians(SiteLat));
	double Rstation = r_eq / (std::sqrt(1.00 - f * (2.00 - f) * sinRadSiteLat * sinRadSiteLat));
	double Ra = (Rstation + Height_over_ocean) * cosRadSiteLat;
	double Rz = Rstation * (1.00 - f) * (1.00 - f) * sinRadSiteLat;
	double alfa_rx = r_sat * COS(Radians(SatLon - SiteLon)) - Ra;
	double alfa_ry = r_sat * SIN(Radians(SatLon - SiteLon));
	double alfa_rz = -Rz, alfa_r_north = -alfa_rx * sinRadSiteLat + alfa_rz * cosRadSiteLat;
	double alfa_r_zenith = alfa_rx * cosRadSiteLat + alfa_rz * sinRadSiteLat;
	double El_geometric = Deg(ATAN(alfa_r_zenith / std::sqrt(alfa_r_north * alfa_r_north + alfa_ry * alfa_ry)));
	double x = std::fabs(El_geometric + 0.589);
	double refraction = std::fabs(a0 + a1 * x + a2 * x * x + a3 * x * x * x + a4 * x * x * x * x);
	double El_observed = 0.00;

	if (El_geometric > 10.2)
		El_observed = El_geometric + 0.01617 * (COS(Radians(std::fabs(El_geometric))) / SIN(Radians(std::fabs(El_geometric))));
	else 
	{
		El_observed = El_geometric + refraction;
	}

	if (alfa_r_zenith < -3000)
		El_observed = -99;

	return El_observed;
}

double calcAzimuth(double SatLon, double SiteLat, double SiteLon, int Height_over_ocean = 0)
{
	const double f = 1.00 / 298.257;	// Earth flattning factor
	const double r_sat = 42164.57;	// Distance from earth centre to satellite
	const double r_eq = 6378.14;	// Earth radius

	double sinRadSiteLat = SIN(Radians(SiteLat)), cosRadSiteLat = COS(Radians(SiteLat));
	double Rstation = r_eq / (std::sqrt(1 - f * (2 - f) * sinRadSiteLat * sinRadSiteLat));
	double Ra = (Rstation + Height_over_ocean) * cosRadSiteLat;
	double Rz = Rstation * (1 - f) * (1 - f) * sinRadSiteLat;
	double alfa_rx = r_sat * COS(Radians(SatLon - SiteLon)) - Ra;
	double alfa_ry = r_sat * SIN(Radians(SatLon - SiteLon));
	double alfa_rz = -Rz;
	double alfa_r_north = -alfa_rx * sinRadSiteLat + alfa_rz * cosRadSiteLat;
	double Azimuth = 0.00;

	if (alfa_r_north < 0)
		Azimuth = 180 + Deg(ATAN(alfa_ry / alfa_r_north));
	else
		Azimuth = Rev(360 + Deg(ATAN(alfa_ry / alfa_r_north)));

	return Azimuth;
}

double calcDeclination(double SiteLat, double Azimuth, double Elevation)
{
	return Deg(ASIN(SIN(Radians(Elevation)) * SIN(Radians(SiteLat)) + COS(Radians(Elevation)) * COS(Radians(SiteLat)) + COS(Radians(Azimuth))));
}

double calcSatHourangle(double Azimuth, double Elevation, double Declination, double Lat)
{
	double a = -COS(Radians(Elevation)) * SIN(Radians(Azimuth));
	double b = SIN(Radians(Elevation)) * COS(Radians(Lat)) - COS(Radians(Elevation)) * SIN(Radians(Lat)) * COS(Radians(Azimuth));

	// Works for all azimuths (northern & sourhern hemisphere)
	double returnvalue = 180 + Deg(ATAN(a / b));

	(void)Declination;

	if (Azimuth > 270) 
	{
		returnvalue = ((returnvalue - 180) + 360);
		if (returnvalue > 360)
			returnvalue = 360 - (returnvalue - 360);
	}

	if (Azimuth < 90)
		returnvalue = (180 - returnvalue);

	return returnvalue;
}

void CFrontend::gotoXX(t_satellite_position pos)
{
	if (info.type != FE_QPSK) 
		return;
	
	int RotorCmd;
	int satDir = pos < 0 ? WEST : EAST;
	double SatLon = abs(pos) / 10.00;
	double SiteLat = gotoXXLatitude;
	double SiteLon = gotoXXLongitude;

	if (gotoXXLaDirection == SOUTH)
		SiteLat = -SiteLat;
	if (gotoXXLoDirection == WEST)
		SiteLon = 360 - SiteLon;
	if (satDir == WEST)
		SatLon = 360 - SatLon;
	
	dprintf(DEBUG_INFO, "CFrontend::gotoXX: fe(%d:%d) siteLatitude = %f, siteLongitude = %f, %f degrees\n", feadapter, fenumber, SiteLat, SiteLon, SatLon);

	double azimuth = calcAzimuth(SatLon, SiteLat, SiteLon);
	double elevation = calcElevation(SatLon, SiteLat, SiteLon);
	double declination = calcDeclination(SiteLat, azimuth, elevation);
	double satHourAngle = calcSatHourangle(azimuth, elevation, declination, SiteLat);
	
	dprintf(DEBUG_INFO, "CFrontend::gotoXX: fe(%d:%d) azimuth=%f, elevation=%f, declination=%f, PolarmountHourAngle=%f\n", feadapter, fenumber, azimuth, elevation, declination, satHourAngle);
	
	if (SiteLat >= 0) 
	{
		int tmp = (int)round(fabs(180 - satHourAngle) * 10.0);
		RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
		if (satHourAngle < 180)	// the east
			RotorCmd |= 0xE000;
		else
			RotorCmd |= 0xD000;
	} 
	else 
	{
		if (satHourAngle < 180) 
		{
			int tmp = (int)round(fabs(satHourAngle) * 10.0);
			RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
			RotorCmd |= 0xD000;
		} 
		else 
		{
			int tmp = (int)round(fabs(360 - satHourAngle) * 10.0);
			RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
			RotorCmd |= 0xE000;
		}
	}

	dprintf(DEBUG_INFO, "CFrontend::gotoXX: fe(%d:%d) RotorCmd = %04x\n", feadapter, fenumber, RotorCmd);
	
	sendMotorCommand(0xE0, 0x31, 0x6E, 2, ((RotorCmd & 0xFF00) / 0x100), RotorCmd & 0xFF, repeatUsals);
}

//
uint32_t CFrontend::getDeliverySystem()
{
	uint32_t system = DVB_S;
	
#if HAVE_DVB_API_VERSION >= 5
	system = deliverySystemMask;
#else
	switch ( info.type ) 
	{
		case FE_QAM:
			system = DVB_C;
			break;

		case FE_QPSK:
			system = DVB_S;
			break;

		case FE_OFDM:
			system = DVB_T;
			break;
			
		case FE_ATSC:
            		system = DVB_A;
			break;
	}
#endif
	
	return system;
}

//
fe_delivery_system_t CFrontend::getFEDeliverySystem(uint32_t sys)
{
	fe_delivery_system_t delsys;

	switch (sys) 
	{
		case DVB_S:
			delsys = SYS_DVBS;
			break;
			
		case DVB_S2:
		case DVB_S2X:
			delsys = SYS_DVBS2;
			break;
			
		case DVB_T:
			delsys = SYS_DVBT;
			break;

		case DVB_T2:
			delsys = SYS_DVBT2;
			break;

		case DVB_C:
#ifdef SYS_DVBC_ANNEX_A
			delsys = SYS_DVBC_ANNEX_A;
#else
			delsys = SYS_DVBC_ANNEX_AC;
#endif
			break;

		case DVB_DTMB:
			delsys = SYS_DTMB;
			break;

		default:
			delsys = SYS_UNDEFINED;
			break;
	}

	return delsys;
}

//
bool CFrontend::changeDelSys(uint32_t delsys)
{
	dprintf(DEBUG_NORMAL, "CFrontend::changeDelSys: fe(%d:%d) to delsys:0x%x\n", feadapter, fenumber, delsys);
	
	if ( !(deliverySystemMask & delsys) )
		return true;
	
#if 0 //HAVE_DVB_API_VERSION >= 5
	struct dtv_property p[2];
	memset(p, 0, sizeof(p));
	struct dtv_properties cmdseq;
	cmdseq.props = p;
	cmdseq.num = 2;
	p[0].cmd = DTV_CLEAR;
	p[1].cmd = DTV_DELIVERY_SYSTEM;
	p[1].u.data = SYS_UNDEFINED;
	
	//
	switch (delsys)
	{
		case DVB_S:
		case DVB_S2:
		case DVB_S2X:
			p[1].u.data = SYS_DVBS;
			break;
			
		case DVB_T:
			p[1].u.data = SYS_DVBT;
			break;
			
		case DVB_T2:
			p[1].u.data = SYS_DVBT2;
			break;
		
#ifdef SYS_DTMB	
		case DVB_DTMB:
			p[1].u.data = SYS_DTMB;
			break;
#endif
			
		case DVB_C:
			secSetVoltage(SEC_VOLTAGE_OFF, 0);
#ifdef SYS_DVBC_ANNEX_A
			p[1].u.data = SYS_DVBC_ANNEX_A;
#else
			p[1].u.data = SYS_DVBC_ANNEX_AC;
#endif
			break;
			
		case DVB_A:
			p[1].u.data = SYS_ATSC;
			break;
		
		default:
			break;
	}
	
	if (::ioctl(fd, FE_SET_PROPERTY, &cmdseq) < 0)
	{
		perror("FE_SET_PROPERTY");
		return false;
	}
#else
	std::string dest = "/proc/stb/frontend/";
	dest += fenumber;
	dest += "/mode";
	
	switch (delsys)
	{
		case DVB_S:
		case DVB_S2:
		case DVB_S2X:
			proc_put(dest.c_str(), "2");
			break;
			
		case DVB_T:
		case DVB_T2:
		case DVB_DTMB:
			proc_put(dest.c_str(), "1");
			break;
			
		case DVB_C:
			secSetVoltage(SEC_VOLTAGE_OFF, 0);
			proc_put(dest.c_str(), "0");
			break;
			
		case DVB_A:
			proc_put(dest.c_str(), "3");
			break;
		
		default:
			break;
	}
#endif

	forcedDelSys = delsys;
	
	return true;
}

