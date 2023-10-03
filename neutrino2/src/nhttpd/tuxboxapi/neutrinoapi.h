#ifndef __nhttpd_neutrinoapi_h__
#define __nhttpd_neutrinoapi_h__

// c++
#include <map>
#include <string>

// tuxbox
#include <eventserver.h>

#include <sectionsd/sectionsd.h>
#include <timerd/timerd.h>
#include <zapit/zapit.h>

// nhttpd
#include "helper.h"
#include "neutrinoyparser.h"
#include "controlapi.h"

#ifdef ENABLE_LCDAPI
#include "lcdapi.h"
#endif

//-------------------------------------------------------------------------
// No Class Helpers
const char * _getISO639Description(const char * const iso);
bool _initialize_iso639_map(void);

//-------------------------------------------------------------------------
class CNeutrinoAPI
{
#ifdef ENABLE_LCDAPI
	CLCDAPI                 *LcdAPI;
#endif	
	// complete channellists
	CZapit::BouquetChannelList RadioChannelList,TVChannelList;
	// events of actual channel
	std::map<unsigned, CChannelEvent *> ChannelListEvents;
	// List of available tv bouquets
	std::map<int, CZapit::BouquetChannelList> TVBouquetsList;
	// List of available radio bouquets
	std::map<int, CZapit::BouquetChannelList> RadioBouquetsList;

	//bool standby_mode;

	// some constants
	static std::string Dbox_Hersteller[4];
	static std::string videooutput_names[5];
#ifdef __sh__
	static std::string videoformat_names[2];
#else
	static std::string videoformat_names[5];
#endif	
	static std::string audiotype_names[5];
	static std::string mpegmodes[];
	static std::string ddmodes[];

	// get functions to collect data
	bool GetChannelEvents(void);
	bool GetStreamInfo(int bitinfo[10]);
	std::string GetServiceName(t_channel_id channel_id);
	CZapit::BouquetChannelList *GetBouquet(unsigned int BouquetNr, int Mode);
	CZapit::BouquetChannelList *GetChannelList(int Mode);

	// support functions
	void ZapTo          (const char * const target);
	void ZapToSubService(const char * const target);
	void ZapToChannelId (t_channel_id channel_id);
	t_channel_id ChannelNameToChannelId(std::string search_channel_name);

	void UpdateBouquet(unsigned int BouquetNr);
	void UpdateChannelList(void);

	//
	std::string timerEventType2Str(CTimerd::CTimerEventTypes type);
	std::string timerEventRepeat2Str(CTimerd::CTimerEventRepeat rep);
	std::string getVideoAspectRatioAsString(void);
	int setVideoAspectRatioAsString(std::string newRatioString);
	std::string getVideoResolutionAsString(void);
	std::string getVideoFramerateAsString(void);
	std::string getAudioInfoAsString(void);
	std::string getCryptInfoAsString(void);
	std::string getLogoFile(std::string _logoURL, t_channel_id channelId);
public:
	CNeutrinoAPI();
	~CNeutrinoAPI(void);

	CChannelEventList	eList;
	CNeutrinoYParser	*NeutrinoYParser;
	CControlAPI		*ControlAPI;

	friend class CNeutrinoYParser; // Backreference
	friend class CControlAPI;
};

#endif /*__nhttpd_neutrinoapi_h__*/
