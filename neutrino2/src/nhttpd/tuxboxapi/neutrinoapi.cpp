//=============================================================================
// NHTTPD
// NeutrionAPI
//
// Aggregates: NeutrinoYParser, NeutrinoControlAPI
// Defines Interfaces to:CControldClient, CSectionsd, CZapit,
//			CTimerdClient,CLCDAPI
// Place for common used Neutrino-functions used by NeutrinoYParser, NeutrinoControlAPI
//=============================================================================

// C
#include <cstdlib>
#include <cstring>

// C++
#include <string>
#include <fstream>
#include <map>
#include <sstream>

#include <unistd.h>

// tuxbox
#include <neutrinoMessages.h>
#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/color.h>

#include <gui/widget/icons.h>

#include <daemonc/remotecontrol.h>

/* dvbapi*/
#include <video_cs.h>
#include <audio_cs.h>
#include <dmx_cs.h>

/*zapit includes*/
#include <zapit/frontend_c.h>
#include <zapit/satconfig.h>
#include <zapit/zapit.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>


extern tallchans allchans;
extern CBouquetManager *g_bouquetManager;
extern CFrontend * frontend;
extern cVideo * videoDecoder;
extern cAudio * audioDecoder;

extern CRemoteControl *g_RemoteControl;	// neutrino.cpp
extern CZapit::SatelliteList satList;

// yhttpd
#include "ylogging.h"

// nhttpd
#include "neutrinoapi.h"

#ifdef ENABLE_LCDAPI
#include "lcdapi.h"
#endif

// opengl liveplayback
#if defined (USE_PLAYBACK)
void stopOpenGLplayback();
#endif

//
// No Class Helpers
//

#ifndef initialize_iso639_map
bool _initialize_iso639_map(void)
{
	std::string s, t, u, v;

	std::ifstream in("/share/iso-codes/iso-639.tab");

	if(!in.is_open())
		std::ifstream in("/usr/local/share/iso-codes/iso-639.tab");
	
	if (in.is_open())
	{
		while (in.peek() == '#')
			getline(in, s);
		while (in >> s >> t >> u >> std::ws)
		{
			getline(in, v);
			iso639[s] = v;
			if (s != t)
				iso639[t] = v;
		}
		in.close();
		return true;
	}
 	else
		return false;
}
#endif

const char * _getISO639Description(const char * const iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));
	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}

//
// Initialization of static variables
//
std::string CNeutrinoAPI::Dbox_Hersteller[4]	= {"none", "Nokia", "Philips", "Sagem"};
std::string CNeutrinoAPI::videooutput_names[5]	= {"CVBS", "RGB with CVBS", "S-Video", "YUV with VBS", "YUV with CVBS"};
#if defined (__sh__)
std::string CNeutrinoAPI::videoformat_names[2]	= {"4:3", "16:9"};
#else
std::string CNeutrinoAPI::videoformat_names[5]	= {"automatic", "4:3", "14:9", "16:9", "20:9"};
#endif
std::string CNeutrinoAPI::audiotype_names[5] 	= {"none", "single channel","dual channel","joint stereo","stereo"};
std::string CNeutrinoAPI::mpegmodes[] 		= { "stereo", "joint_st", "dual_ch", "single_ch" };
std::string CNeutrinoAPI::ddmodes[] 		= { "CH1/CH2", "C", "L/R", "L/C/R", "L/R/S", "L/C/R/S", "L/R/SL/SR", "L/C/R/SL/SR" };

//
// Constructor & Destructor
//
CNeutrinoAPI::CNeutrinoAPI()
{
	NeutrinoYParser = new CNeutrinoYParser(this);
	ControlAPI = new CControlAPI(this);
	
#ifdef ENABLE_LCDAPI
	LcdAPI = new CLCDAPI();
#endif	

	UpdateBouquets();
}

CNeutrinoAPI::~CNeutrinoAPI(void)
{
#ifdef ENABLE_LCDAPI
	if (LcdAPI)
		delete LcdAPI;
#endif

	if (NeutrinoYParser)
		delete NeutrinoYParser;
		
	if (ControlAPI)
		delete ControlAPI;
}

void CNeutrinoAPI::UpdateBouquets(void)
{
#if 0 //FIXME
	BouquetList.clear();
	CZapit::getInstance()->getBouquets(BouquetList, true, true);
	for (unsigned int i = 1; i <= BouquetList.size(); i++)
		UpdateBouquet(i);

	UpdateChannelList();
#endif
}

void CNeutrinoAPI::ZapTo(const char * const target)
{
	t_channel_id channel_id;

	sscanf(target,
		"%llx",
		&channel_id);

	ZapToChannelId(channel_id);
}

void CNeutrinoAPI::ZapToChannelId(t_channel_id channel_id)
{
	// standby modus
	if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_standby)
	{
		CZapit::getInstance()->setStandby(false);
		
// opengl liveplayback
#if defined (USE_PLAYBACK)
		stopOpenGLplayback();
#endif		
	
		if (channel_id != 0) 
		{
			if (CZapit::getInstance()->zapTo_record(channel_id) != CZapit::ZAP_INVALID_PARAM)
				CSectionsd::getInstance()->setServiceChanged(channel_id, false);
		}

		// stop playback im standby
		CZapit::getInstance()->stopPlayBack();
	}
	else
	{
		if ( channel_id == CZapit::getInstance()->getCurrentServiceID() )
			return;
		
// opengl liveplayback
#if defined (USE_PLAYBACK)
		stopOpenGLplayback();
#endif		

		if (CZapit::getInstance()->zapTo_serviceID(channel_id) != CZapit::ZAP_INVALID_PARAM)
			CSectionsd::getInstance()->setServiceChanged(channel_id, false);
	}
}

void CNeutrinoAPI::ZapToSubService(const char * const target)
{
	t_channel_id channel_id;

	sscanf(target,
		"%llx",
		&channel_id);
		
// opengl liveplayback
#if defined (USE_PLAYBACK)
	stopOpenGLplayback();
#endif		

	if (CZapit::getInstance()->zapTo_subServiceID(channel_id) != CZapit::ZAP_INVALID_PARAM)
		CSectionsd::getInstance()->setServiceChanged(channel_id, false);
}

t_channel_id CNeutrinoAPI::ChannelNameToChannelId(std::string search_channel_name)
{
//FIXME depending on mode missing
	//int mode = CZapit::getInstance()->getMode();
	t_channel_id channel_id = (t_channel_id)-1;
	CStringArray channel_names = ySplitStringVector(search_channel_name, ",");
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++) 
	{
		std::string channel_name = it->second.getName();
		for(unsigned int j=0;j<channel_names.size();j++)
		{
			if(channel_names[j].length() == channel_name.length() &&
				equal(channel_names[j].begin(), channel_names[j].end(),
				channel_name.begin(), nocase_compare)) //case insensitive  compare
			{
				channel_id = it->second.channel_id;
				break;
			}
		}
	}
	return channel_id;
}

//
// Get functions
//
bool CNeutrinoAPI::GetStreamInfo(int bitInfo[10])
{
	char *key, *tmpptr, buf[100];
	long value;
	int pos = 0;

	memset(bitInfo, 0, sizeof(bitInfo));

	FILE *fd = fopen("/proc/bus/bitstream", "rt");

	if (fd == NULL)
	{
		dprintf("error while opening proc-bitstream\n" );
		return false;
	}

	fgets(buf, 35, fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,35,fd)!=NULL)
		{
			buf[strlen(buf)-1] = 0;
			tmpptr = buf;
			key = strsep(&tmpptr,":");
			value = strtoul(tmpptr,NULL,0);
			bitInfo[pos] = value;
			pos++;
		}
	}
	fclose(fd);

	return true;
}

bool CNeutrinoAPI::GetChannelEvents(void)
{
	CSectionsd::getInstance()->getChannelEvents(eList);
	CChannelEventList::iterator eventIterator;

	ChannelListEvents.clear();

	if (eList.begin() == eList.end())
		return false;

	for (eventIterator = eList.begin(); eventIterator != eList.end(); eventIterator++)
		ChannelListEvents[(*eventIterator).get_channel_id()] = &(*eventIterator);

	return true;
}

std::string CNeutrinoAPI::GetServiceName(t_channel_id channel_id)
{
	tallchans_iterator it = allchans.find(channel_id);
	if (it != allchans.end())
		return it->second.getName();
	else
		return "";
}

CZapit::BouquetChannelList *CNeutrinoAPI::GetBouquet(unsigned int, int)
{
	//FIXME
	printf("CNeutrinoAPI::GetChannelList still used !\n");
	return NULL;
}

CZapit::BouquetChannelList *CNeutrinoAPI::GetChannelList(int)
{
	//FIXME
	printf("CNeutrinoAPI::GetChannelList still used !\n");
	return NULL;
}

void CNeutrinoAPI::UpdateBouquet(unsigned int)
{
	//FIXME
}

void CNeutrinoAPI::UpdateChannelList(void)
{
	//FIXME
}

std::string CNeutrinoAPI::timerEventType2Str(CTimerd::CTimerEventTypes type)
{
	std::string result;
	switch (type) 
	{
		case CTimerd::TIMER_SHUTDOWN:
			result = "Shutdown";
			break;
		case CTimerd::TIMER_NEXTPROGRAM:
			result = "Next program";
			break;
		case CTimerd::TIMER_ZAPTO:
			result = "Zap to";
			break;
		case CTimerd::TIMER_STANDBY:
			result = "Standby";
			break;
		case CTimerd::TIMER_RECORD:
			result = "Record";
			break;
		case CTimerd::TIMER_REMIND:
			result = "Reminder";
			break;
		case CTimerd::TIMER_EXEC_PLUGIN:
			result = "Execute plugin";
			break;
		case CTimerd::TIMER_SLEEPTIMER:
			result = "Sleeptimer";
			break;
		default:
			result = "Unknown";
			break;
	}
	return result;
}

std::string CNeutrinoAPI::timerEventRepeat2Str(CTimerd::CTimerEventRepeat rep)
{
	std::string result;
	switch (rep) 
	{
		case CTimerd::TIMERREPEAT_ONCE:
			result = "once";
			break;
		case CTimerd::TIMERREPEAT_DAILY:
			result = "daily";
			break;
		case CTimerd::TIMERREPEAT_WEEKLY:
			result = "weekly";
			break;
		case CTimerd::TIMERREPEAT_BIWEEKLY:
			result = "2-weekly";
			break;
		case CTimerd::TIMERREPEAT_FOURWEEKLY:
			result = "4-weekly";
			break;
		case CTimerd::TIMERREPEAT_MONTHLY:
			result = "monthly";
			break;
		case CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION:
			result = "event";
			break;
		case CTimerd::TIMERREPEAT_WEEKDAYS:
			result = "weekdays";
			break;
		default:
			if (rep > CTimerd::TIMERREPEAT_WEEKDAYS)
			{
				if (rep & 0x0200)
					result += "Mo ";
				if (rep & 0x0400)
					result += "Tu ";
				if (rep & 0x0800)
					result += "We ";
				if (rep & 0x1000)
					result += "Th ";
				if (rep & 0x2000)
					result += "Fr ";
				if (rep & 0x4000)
					result += "Sa ";
				if (rep & 0x8000)
					result += "Su ";
			}
			else
				result = "Unknown";
	}
	
	return result;
}

std::string CNeutrinoAPI::getVideoAspectRatioAsString(void) 
{
	int aspectRatio = videoDecoder->getAspectRatio();
	
	if (aspectRatio >= 0 && aspectRatio <= 4)
		return videoformat_names[aspectRatio];
	else
		return "unknown";
}

int CNeutrinoAPI::setVideoAspectRatioAsString(std::string newRatioString) 
{
	int newRatioInt = -1;
	for(int i = 0; i < (int)sizeof(videoformat_names); i++)
	{
		if( videoformat_names[i] == newRatioString)
		{
			newRatioInt = i;
			break;
		}
	}
	
	if(newRatioInt != -1)		
		videoDecoder->setAspectRatio(newRatioInt, -1);

	return newRatioInt;
}

std::string CNeutrinoAPI::getVideoResolutionAsString(void) 
{
	int xres, yres, framerate;
	videoDecoder->getPictureInfo(xres, yres, framerate);
	std::stringstream out;
	out << xres << "x" << yres;
	
	return out.str();
}

std::string CNeutrinoAPI::getVideoFramerateAsString(void) 
{
	int xres, yres, framerate;
	std::string sframerate="unknown";
	videoDecoder->getPictureInfo(xres, yres, framerate);

	switch(framerate)
	{
#if !defined (__sh__)
		case 2:
			sframerate="25fps";break;
		case 5:
			sframerate="50fps";break;
#else
		case 25:
			sframerate="25fps";break;
		case 50:
			sframerate="50fps";break;
#endif
	}

	return sframerate;
}

std::string CNeutrinoAPI::getAudioInfoAsString(void) 
{
#if 0
	int type, layer, freq, mode, lbitrate;
	audioDecoder->getAudioInfo(type, layer, freq, lbitrate, mode);
	std::stringstream out;

	if(type == 0)
		out << "MPEG " << mpegmodes[mode] << " (" << freq <<")";
	else
		out << "DD " << ddmodes[mode] << " (" << freq <<")";

	return out.str();
#endif	
}

std::string CNeutrinoAPI::getCryptInfoAsString(void) 
{
	extern int pmt_caids[11];
	unsigned short i,j;
	std::stringstream out;
	
	std::string casys[11]={"Irdeto:", "Betacrypt:", "Seca:","Viaccess:","Nagra:","Conax: ","Cryptoworks:","Videoguard:","EBU:","XCrypt:","PowerVU:"};

	for(j = 0; j < 11; j++)
	{
		if(pmt_caids[j])
		{
			out << casys[j] << hex << pmt_caids[j]<< "\n";
		}
	}

	return out.str();
}

std::string CNeutrinoAPI::getLogoFile(std::string _logoURL, t_channel_id channelId) 
{
	std::string channelIdAsString = string_printf("%llx", channelId & 0xFFFFFFFFFFFFULL);
	std::string channelName = GetServiceName(channelId);
	//replace(channelName, " ", "_");
	_logoURL += "/";
	
	if(access((_logoURL + channelIdAsString + ".jpg").c_str(), 4) == 0)
		return _logoURL + channelIdAsString + ".jpg";
	else if (access((_logoURL + channelIdAsString + ".png").c_str(), 4) == 0)
		return _logoURL + channelIdAsString + ".png";
	else if (access((_logoURL + channelName + ".jpg").c_str(), 4) == 0)
		return _logoURL + channelName + ".jpg";
	else if (access((_logoURL + channelName + ".png").c_str(), 4) == 0)
		return _logoURL + channelName + ".png";
	else
		return "";
}

