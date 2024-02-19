/*
 * $Id: zapittypes.h 20.09.2023 mohousch Exp $
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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
 *
 */

#ifndef __zapittypes_h__
#define __zapittypes_h__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <inttypes.h>
#include <string>

#include <map>

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>


#if DVB_API_VERSION_MINOR < 5
#ifndef SYS_DVBC_ANNEX_A
#define SYS_DVBC_ANNEX_A	SYS_DVBC_ANNEX_AC
#endif

#ifndef SYS_DTMB
#define SYS_DTMB		SYS_DMBTH
#endif
#endif

typedef uint16_t t_service_id;
typedef uint16_t t_original_network_id;
typedef uint16_t t_transport_stream_id;
typedef int16_t t_satellite_position;
typedef uint16_t freq_id_t;
typedef uint64_t t_channel_id;
typedef uint64_t transponder_id_t;
typedef uint16_t t_bouquet_id;		// used in eventlist.h

//// channel
extern "C" {
#include <libmd5sum/md5.h>
}
#include <string.h>


// channel_id
static inline t_channel_id create_channel_id(t_service_id service_id, t_original_network_id original_network_id, t_transport_stream_id transport_stream_id, t_satellite_position satellitePosition = 0, freq_id_t freq = 0, const char *url = NULL) 
{
	if (url) 
	{
		t_channel_id cid;
		unsigned char md5[16];
		md5_buffer(url, strlen(url), md5);
		memcpy(&cid, md5, sizeof(cid));

		return cid | 0xFFFFFFFF00000000;
	}

	return ((uint64_t)(satellitePosition + freq*4) << 48) | ((uint64_t) transport_stream_id << 32) | ((uint64_t)original_network_id << 16) | (uint64_t)service_id;
}
#define CREATE_CHANNEL_ID create_channel_id(service_id, original_network_id, transport_stream_id, satellitePosition, freq)

static inline bool IS_WEBTV(t_channel_id cid)
{
	return (cid & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000;
}

#define GET_TRANSPORT_STREAM_ID_FROM_CHANNEL_ID(channel_id) ((t_transport_stream_id)((channel_id) >> 32))
#define GET_ORIGINAL_NETWORK_ID_FROM_CHANNEL_ID(channel_id) ((t_original_network_id)((channel_id) >> 16))
#define GET_SERVICE_ID_FROM_CHANNEL_ID(channel_id) ((t_service_id)(channel_id))

//// transponder_id
#define CREATE_TRANSPONDER_ID(freq, satellitePosition, original_network_id, transport_stream_id) ( ((uint64_t)freq << 48) |  ((uint64_t) ( satellitePosition >= 0 ? satellitePosition : (uint64_t)(0xF000+ abs(satellitePosition))) << 32) | ((uint64_t)transport_stream_id << 16) | (uint64_t)original_network_id)

#define GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(transponder_id) ((t_original_network_id)(transponder_id ))
#define GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(transponder_id) ((t_transport_stream_id)(transponder_id >> 16))
#define GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(transponder_id)((t_satellite_position )(transponder_id >> 32) & 0xFFFF)
#define GET_FREQ_FROM_TRANSPONDER_ID(transponder_id) ((freq_id_t)(transponder_id >> 48))

#define SAME_TRANSPONDER(id1, id2) ((id1 >> 16) == (id2 >> 16))

////
enum ChannelVideoType {
	CHANNEL_VIDEO_MPEG2 	= 0,
	CHANNEL_VIDEO_MPEG4 	= 1,	
	CHANNEL_VIDEO_HEVC 	= 2,
	CHANNEL_VIDEO_CAVS	= 3
};

//// dvbsi++
enum SiDescriptorTag {
	/* 0x00 - 0x3F: ITU-T Rec. H.222.0 | ISO/IEC 13818-1 */
	VIDEO_STREAM_DESCRIPTOR				= 0x02,
	AUDIO_STREAM_DESCRIPTOR				= 0x03,
	HIERARCHY_DESCRIPTOR				= 0x04,
	REGISTRATION_DESCRIPTOR				= 0x05,
	DATA_STREAM_ALIGNMENT_DESCRIPTOR		= 0x06,
	TARGET_BACKGROUND_GRID_DESCRIPTOR		= 0x07,
	VIDEO_WINDOW_DESCRIPTOR				= 0x08,
	CA_DESCRIPTOR					= 0x09,
	ISO_639_LANGUAGE_DESCRIPTOR			= 0x0A,
	SYSTEM_CLOCK_DESCRIPTOR				= 0x0B,
	MULTIPLEX_BUFFER_UTILIZATION_DESCRIPTOR		= 0x0C,
	COPYRIGHT_DESCRIPTOR				= 0x0D,
	MAXIMUM_BITRATE_DESCRIPTOR			= 0x0E,
	PRIVATE_DATA_INDICATOR_DESCRIPTOR		= 0x0F,
	SMOOTHING_BUFFER_DESCRIPTOR			= 0x10,
	STD_DESCRIPTOR					= 0x11,
	IBP_DESCRIPTOR					= 0x12,
	CAROUSEL_IDENTIFIER_DESCRIPTOR			= 0x13,
	/* 0x40 - 0x7F: ETSI EN 300 468 V1.9.1 (2009-03) */
	NETWORK_NAME_DESCRIPTOR				= 0x40,
	SERVICE_LIST_DESCRIPTOR				= 0x41,
	STUFFING_DESCRIPTOR				= 0x42,
	SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR		= 0x43,
	CABLE_DELIVERY_SYSTEM_DESCRIPTOR		= 0x44,
	VBI_DATA_DESCRIPTOR				= 0x45,
	VBI_TELETEXT_DESCRIPTOR				= 0x46,
	BOUQUET_NAME_DESCRIPTOR				= 0x47,
	SERVICE_DESCRIPTOR				= 0x48,
	COUNTRY_AVAILABILITY_DESCRIPTOR			= 0x49,
	LINKAGE_DESCRIPTOR				= 0x4A,
	NVOD_REFERENCE_DESCRIPTOR			= 0x4B,
	TIME_SHIFTED_SERVICE_DESCRIPTOR			= 0x4C,
	SHORT_EVENT_DESCRIPTOR				= 0x4D,
	EXTENDED_EVENT_DESCRIPTOR			= 0x4E,
	TIME_SHIFTED_EVENT_DESCRIPTOR			= 0x4F,
	COMPONENT_DESCRIPTOR				= 0x50,
	MOSAIC_DESCRIPTOR				= 0x51,
	STREAM_IDENTIFIER_DESCRIPTOR			= 0x52,
	CA_IDENTIFIER_DESCRIPTOR			= 0x53,
	CONTENT_DESCRIPTOR				= 0x54,
	PARENTAL_RATING_DESCRIPTOR			= 0x55,
	TELETEXT_DESCRIPTOR				= 0x56,
	TELEPHONE_DESCRIPTOR				= 0x57,
	LOCAL_TIME_OFFSET_DESCRIPTOR			= 0x58,
	SUBTITLING_DESCRIPTOR				= 0x59,
	TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR		= 0x5A,
	MULTILINGUAL_NETWORK_NAME_DESCRIPTOR		= 0x5B,
	MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR		= 0x5C,
	MULTILINGUAL_SERVICE_NAME_DESCRIPTOR		= 0x5D,
	MULTILINGUAL_COMPONENT_DESCRIPTOR		= 0x5E,
	PRIVATE_DATA_SPECIFIER_DESCRIPTOR		= 0x5F,
	SERVICE_MOVE_DESCRIPTOR				= 0x60,
	SHORT_SMOOTHING_BUFFER_DESCRIPTOR		= 0x61,
	FREQUENCY_LIST_DESCRIPTOR			= 0x62,
	PARTIAL_TRANSPORT_STREAM_DESCRIPTOR		= 0x63,
	DATA_BROADCAST_DESCRIPTOR			= 0x64,
	SCRAMBLING_DESCRIPTOR				= 0x65,
	DATA_BROADCAST_ID_DESCRIPTOR			= 0x66,
	TRANSPORT_STREAM_DESCRIPTOR			= 0x67,
	DSNG_DESCRIPTOR					= 0x68,
	PDC_DESCRIPTOR					= 0x69,
	AC3_DESCRIPTOR					= 0x6A,
	ANCILLARY_DATA_DESCRIPTOR			= 0x6B,
	CELL_LIST_DESCRIPTOR				= 0x6C,
	CELL_FREQUENCY_LINK_DESCRIPTOR			= 0x6D,
	ANNOUNCEMENT_SUPPORT_DESCRIPTOR			= 0x6E,
	APPLICATION_SIGNALLING_DESCRIPTOR		= 0x6F,
	ADAPTATION_FIELD_DATA_DESCRIPTOR		= 0x70,
	SERVICE_IDENTIFIER_DESCRIPTOR			= 0x71,
	SERVICE_AVAILABILITY_DESCRIPTOR			= 0x72,
	DEFAULT_AUTHORITY_DESCRIPTOR			= 0x73,	/* TS 102 323 */
	RELATED_CONTENT_DESCRIPTOR			= 0x74,	/* TS 102 323 */
	TVA_ID_DESCRIPTOR				= 0x75,	/* TS 102 323 */
	CONTENT_IDENTIFIER_DESCRIPTOR			= 0x76,	/* TS 102 323 */
	TIME_SLICE_FEC_IDENTIFIER_DESCRIPTOR		= 0x77,	/* EN 301 192 */
	ECM_REPETITION_RATE_DESCRIPTOR			= 0x78,	/* EN 301 192 */
	S2_SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR		= 0x79,
	ENHANCED_AC3_DESCRIPTOR				= 0x7A,
	DTS_DESCRIPTOR					= 0x7B,
	AAC_DESCRIPTOR					= 0x7C,
	XAIT_LOCATION_DESCRIPTOR			= 0x7D,	/* TS 102 590 aka A107.MHP 1.2 Spec */
	FTA_CONTENT_MANAGEMENT_DESCRIPTOR		= 0x7E,
	EXTENSION_DESCRIPTOR				= 0x7F,
	/* 0x80 - 0xFE: User defined */
	LOGICAL_CHANNEL_DESCRIPTOR			= 0x83,	/* IEC 62216-1 */
	HD_SIMULCAST_LOGICAL_CHANNEL_DESCRIPTOR		= 0x88,	/* DIGITALEUROPE (former EICTA) extension to IEC 62216-1 */
	/* 0xFF: Forbidden */
	FORBIDDEN_DESCRIPTOR				= 0xFF
};

// service types
typedef enum {
	ST_RESERVED				= 0x00,
	ST_DIGITAL_TELEVISION_SERVICE		= 0x01,
	ST_DIGITAL_RADIO_SOUND_SERVICE		= 0x02,
	ST_TELETEXT_SERVICE			= 0x03,
	ST_NVOD_REFERENCE_SERVICE		= 0x04,
	ST_NVOD_TIME_SHIFTED_SERVICE		= 0x05,
	ST_MOSAIC_SERVICE			= 0x06,
	//
	ST_AVC_DIGITAL_RADIO_SOUND_SERVICE	= 0x0A,
	ST_AVC_MOSAIC_SERVICE			= 0x0B,
	//
	ST_DATA_BROADCAST_SERVICE		= 0x0C,
	ST_COMMON_INTERFACE_RESERVED		= 0x0D,
	ST_RCS_MAP				= 0x0E,
	ST_RCS_FLS				= 0x0F,
	ST_DVB_MHP_SERVICE			= 0x10,
	ST_MPEG2_HD_DIGITAL_TV_SERVICE 		= 0x11,
	// 0x12 to 0x15: reserved for future use 
	ST_AVC_SD_DIGITAL_TV_SERVICE 		= 0x16,
	ST_AVC_SD_NVOD_TIME_SHIFTED_SERVICE 	= 0x17,
	ST_AVC_SD_NVOD_REFERENCE_SERVICE 	= 0x18,
	ST_AVC_HD_DIGITAL_TV_SERVICE 		= 0x19,
	ST_AVC_HD_NVOD_TIME_SHIFTED_SERVICE 	= 0x1A,
	ST_AVC_HD_NVOD_REFERENCE_SERVICE 	= 0x1B,
	//0x1c - 0x7F: reserved for future use
	// 3DTV
	ST_3DTV1_TELEVISION_SERVICE 		= 0x1C,
	ST_3DTV2_TELEVISION_SERVICE 		= 0x1D,
	ST_3DTV3_TELEVISION_SERVICE 		= 0x1E,
	//
	ST_MULTIFEED				= 0x69,
	
	// 0x80 - 0xFE: user defined
	// 0xFF: reserved for future use
} service_type_t;

//// diseqc types
typedef enum {
	NO_DISEQC,
	MINI_DISEQC,
	SMATV_REMOTE_TUNING,
	DISEQC_1_0,
	DISEQC_1_1,
	DISEQC_1_2,
	DISEQC_ADVANCED,
	DISEQC_UNICABLE,
	DISEQC_UNICABLE2
} diseqc_t;

// dvb transmission types
typedef enum {
	UNDEFINED 	= 0x0000,
	DVB_S 		= 0x0001,
	DVB_S2		= 0x0002,
	DVB_S2X		= 0x0004,
	DVB_C 		= 0x0008,
	DVB_T 		= 0x0010,
	DVB_T2		= 0x0020,
	DVB_DTMB	= 0x0040,
	DVB_A 		= 0x0080
} delivery_system_t;

//
typedef struct 
{
	uint32_t		frequency;
	fe_modulation_t		modulation;
	uint8_t			polarization;

	fe_spectral_inversion_t	inversion;
	fe_code_rate_t		fec_inner;
	fe_transmit_mode_t	transmission_mode;
	fe_bandwidth_t		bandwidth;
	fe_guard_interval_t	guard_interval;
	fe_hierarchy_t		hierarchy_information;
	uint32_t		symbol_rate;
	fe_code_rate_t		code_rate_HP;
	fe_code_rate_t		code_rate_LP;
	
	uint32_t		delsys;
	unsigned int		plp_id;
} FrontendParameters;

//// transponder
struct transponder
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	FrontendParameters feparams;
	bool updated;
	uint64_t TP_id;
	uint8_t diseqc;
	
	//
	transponder()
	{
		memset(&feparams, 0, sizeof(FrontendParameters));
		TP_id	= 0;
		transport_stream_id = 0;
		original_network_id = 0;
		diseqc = 255;
		updated = false;
	}

	//
	transponder(const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, const FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams            = p_feparams;
		updated = 0;
		TP_id = 0;
		diseqc = 255;
	}

	//
	transponder(t_transport_stream_id p_transport_stream_id, FrontendParameters p_feparams, t_original_network_id p_original_network_id)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		original_network_id = p_original_network_id;
		updated = 0;
		TP_id = 0;
		diseqc = 255;
	}
};

typedef std::map<transponder_id_t, transponder> transponder_list_t;
typedef std::map <transponder_id_t, transponder>::iterator stiterator;
typedef std::map<transponder_id_t, bool> sdt_tp_t; 			// used in zapit.cpp sdtthread

////
typedef struct Zapit_config {
	bool makeRemainingChannelsBouquet;
	int scanSDT;
	bool saveLastChannel;
	int lastchannelmode;
	t_channel_id startchanneltv_id;
	t_channel_id startchannelradio_id;
	uint32_t startchanneltv_nr;
	uint32_t startchannelradio_nr;
} t_zapit_config;

// fetype
typedef enum {
	FE_SINGLE,
	FE_LOOP,
	FE_NOTCONNECTED
} fe_mode_t;

//// sat config
typedef struct sat_config {
	t_satellite_position position;
	int diseqc;
	int commited;
	int uncommited;
	int motor_position;
	int diseqc_order;
	int lnbOffsetLow;
	int lnbOffsetHigh;
	int lnbSwitch;
	int use_in_scan;
	int use_usals;
	std::string name;
	int have_channels;
    	uint32_t system;
} sat_config_t;

typedef enum diseqc_cmd_order {
	UNCOMMITED_FIRST,
	COMMITED_FIRST
} diseqc_cmd_order_t;

typedef std::map<t_satellite_position, sat_config_t> satellite_map_t;
typedef std::map<t_satellite_position, sat_config_t>::iterator sat_iterator_t;

typedef std::map <int, std::string> scan_list_t;
typedef std::map <int, std::string>::iterator scan_list_iterator_t;

#endif /* __zapittypes_h__ */

