//
// $Id: SIsections.cpp,v 1.61 2009/09/04 18:37:00 dbt Exp $
//
// classes for SI sections (dbox-II-project)
//
//    Homepage: http://dbox2.elxsi.de
//
//    Copyright (C) 2001 fnbrd (fnbrd@gmx.de)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h> // fuer poll()

#include <set>
#include <algorithm>
#include <string>

#include "SIutils.hpp"
#include "SIservices.hpp"
#include "SIevents.hpp"

#include "SIsections.hpp"
#include <dmxapi.h>

#include <edvbstring.h>


#define NOVA		0x3ffe
#define CANALDIGITAAL	0x3fff

//#define DEBUG

struct descr_generic_header 
{
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
} __attribute__ ((packed)) ;

struct descr_short_event_header 
{
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned language_code_hi		: 8;
	unsigned language_code_mid		: 8;
	unsigned language_code_lo		: 8;
	unsigned event_name_length		: 8;
} __attribute__ ((packed)) ;

struct descr_service_header 
{
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned service_typ			: 8;
	unsigned service_provider_name_length	: 8;
} __attribute__ ((packed)) ;

struct descr_extended_event_header 
{
	unsigned descriptor_tag			: 8;
	unsigned descriptor_length		: 8;
	unsigned descriptor_number		: 4;
	unsigned last_descriptor_number		: 4;
	unsigned iso_639_2_language_code_hi	: 8;
	unsigned iso_639_2_language_code_mid	: 8;
	unsigned iso_639_2_language_code_lo	: 8;
	unsigned length_of_items		: 8;
} __attribute__ ((packed)) ;

struct service_list_entry 
{
	unsigned service_id_hi			: 8;
	unsigned service_id_lo			: 8;
	unsigned service_type			: 8;
} __attribute__ ((packed)) ;

struct private_data_specifier 
{
	unsigned byte1				: 8;
	unsigned byte2				: 8;
	unsigned byte3				: 8;
	unsigned byte4				: 8;
} __attribute__ ((packed)) ;

inline unsigned min(unsigned a, unsigned b)
{
	return b < a ? b : a;
}

static int get_table(unsigned char hi, unsigned char mid, unsigned char lo)
{
	char lang[4];
	lang[0] = hi;
	lang[1] = mid;
	lang[2] = lo;
	lang[3] = 0;
	
	if(!strcmp(lang, "pol"))
		return 2;
	else if(!strcmp(lang, "tur"))
		return 9;
	else if(!strcmp(lang, "gre"))
		return 7;
	else if(!strcmp(lang, "rus"))
		return 5;
	else if(!strcmp(lang, "bul"))
		return 5;
	else if(!strcmp(lang, "ara"))
		return 6;
	return 0;
}

bool check_blacklisted(const t_original_network_id onid, const t_transport_stream_id tsid)
{
	if ( (onid == 0x0001) &&
			((tsid == 0x03F0) || (tsid == 0x0408) || (tsid == 0x040E) || (tsid == 0x0412) || (tsid == 0x0416) || (tsid == 0x041E) ||
			 (tsid == 0x0420) || (tsid == 0x0422) || (tsid == 0x0424) || (tsid == 0x0444) ))
		return true;
	else
		return false;
}
//-----------------------------------------------------------------------
// Da es vorkommen kann das defekte Packete empfangen werden
// sollte hier alles ueberprueft werden.
// Leider ist das noch nicht bei allen Descriptoren so.
//-----------------------------------------------------------------------
void SIsectionEIT::parseLinkageDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	if(maxlen >= sizeof(struct descr_linkage_header))
	{
		SIlinkage l((const struct descr_linkage_header *)buf);
		e.linkage_descs.insert(e.linkage_descs.end(), l);
		//printf("LinkName: %s\n", l.name.c_str());
	}
}

void SIsectionEIT::parsePDCDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	if (maxlen >= sizeof(struct descr_pdc_header))
	{
		const struct descr_pdc_header *s = (struct descr_pdc_header *)buf;
		time_t now = time(NULL);
		struct tm tm_r;
		struct tm t = *localtime_r(&now, &tm_r); // this initializes the time zone in 't'
		t.tm_isdst = -1; // makes sure mktime() will determine the correct DST setting
		int month = t.tm_mon;
		t.tm_mon = ((s->pil1 >> 3) & 0x0F) - 1;
		t.tm_mday = ((s->pil0 & 0x0F) << 1) | ((s->pil1 & 0x80) >> 7);
		t.tm_hour = ((s->pil1 & 0x07) << 2) | ((s->pil2 & 0xC0) >> 6);
		t.tm_min = s->pil2 & 0x3F;
		t.tm_sec = 0;
		if (month == 11 && t.tm_mon == 0) // current month is dec, but event is in jan
			t.tm_year++;
		else if (month == 0 && t.tm_mon == 11) // current month is jan, but event is in dec
			t.tm_year--;
		e.vps = mktime(&t);
		// fprintf(stderr, "SIsectionEIT::parsePDCDescriptor: vps: %ld %s", e.vps, ctime(&e.vps));
	}
}

void SIsectionEIT::parseComponentDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	if(maxlen>=sizeof(struct descr_component_header))
		e.components.insert(SIcomponent((const struct descr_component_header *)buf));
}

void SIsectionEIT::parseContentDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	struct descr_generic_header *cont=(struct descr_generic_header *)buf;
	if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
		return; // defekt
	const uint8_t *classification = buf + sizeof(struct descr_generic_header);
	while(classification <= buf + sizeof(struct descr_generic_header) + cont->descriptor_length - 2) 
	{
		e.contentClassification += std::string((const char *)classification, 1);
		// printf("Content: 0x%02hhx\n", *classification);
		e.userClassification += std::string((const char *)classification + 1, 1);
		// printf("User: 0x%02hhx\n", *(classification+1));
		classification+=2;
	}
}

void SIsectionEIT::parseParentalRatingDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	struct descr_generic_header *cont=(struct descr_generic_header *)buf;
	if(cont->descriptor_length+sizeof(struct descr_generic_header)>maxlen)
		return; // defekt
		
	const uint8_t *s = buf + sizeof(struct descr_generic_header);
	
	while(s<buf+sizeof(struct descr_generic_header)+cont->descriptor_length-4) 
	{
		e.ratings.insert(SIparentalRating(std::string((const char *)s, 3), *(s + 3)));
		s += 4;
	}
}

void SIsectionEIT::parseExtendedEventDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	struct descr_extended_event_header *evt=(struct descr_extended_event_header *)buf;
	if((evt->descriptor_length + sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_extended_event_header) - sizeof(descr_generic_header)))
		return; // defekt
		
	unsigned char *items = (unsigned char *)(buf + sizeof(struct descr_extended_event_header));
	int tsidonid = (e.transport_stream_id << 16) | e.original_network_id;
	int table = get_table(evt->iso_639_2_language_code_hi, evt->iso_639_2_language_code_mid, evt->iso_639_2_language_code_lo);

	char lang[] = {tolower(evt->iso_639_2_language_code_hi), tolower(evt->iso_639_2_language_code_mid), tolower(evt->iso_639_2_language_code_lo), '\0'};
	std::string language(lang);

	while(items < (unsigned char *)(buf + sizeof(struct descr_extended_event_header) + evt->length_of_items)) 
	{

		// TODO What info should be in item & itemDescription?
		// Should I make language filter those as well?  Arzka

		if(*items)
		{
			// 21.07.2005 - collect all extended events in one
			// string, delimit multiple entries with a newline
			//e.itemDescription.append(std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items)));
			e.itemDescription.append(convertDVBUTF8((const char *)(items+1), min(maxlen - ((const char *)items + 1 - (const char *)buf), *items), table, tsidonid));
			e.itemDescription.append("\n");
		}
		items += 1 + *items;
		if(*items) 
		{
			// 21.07.2005 - collect all extended events in one
			// string, delimit multiple entries with a newline
			//e.item.append(std::string((const char *)(items+0), min(maxlen-((const char *)items+1-buf), *items)));
			e.item.append(convertDVBUTF8((const char *)(items+1), min(maxlen - ((const char *)items + 1 - (const char *)buf), *items), table, tsidonid));
			e.item.append("\n");
		}
		items+=1+*items;
	}

	if(*items) 
	{
		{
			//e.appendExtendedText(language, std::string((const char *)(items+1), min(maxlen-((const char *)items+1-buf), *items)));
			e.appendExtendedText(language, convertDVBUTF8((const char *)(items + 1), min(maxlen - ((const char *)items + 1 - (const char *)buf), (*items)), table, tsidonid));
			//printf("Extended Text: %s\n", e.extendedText.c_str());
		}
	}
}

void SIsectionEIT::parseShortEventDescriptor(const uint8_t *buf, SIevent &e, unsigned maxlen)
{
	struct descr_short_event_header *evt = (struct descr_short_event_header *)buf;
	if((evt->descriptor_length+sizeof(descr_generic_header)>maxlen) || (evt->descriptor_length<sizeof(struct descr_short_event_header)-sizeof(descr_generic_header)))
		return; // defekt
		
	int tsidonid = (e.transport_stream_id << 16) | e.original_network_id;
	int table = get_table(evt->language_code_hi, evt->language_code_mid, evt->language_code_lo);

	char lang[] = {tolower(evt->language_code_hi), tolower(evt->language_code_mid), tolower(evt->language_code_lo), '\0'};
	std::string language(lang);

	buf += sizeof(struct descr_short_event_header);
	
	if(evt->event_name_length)
		e.setName(language, convertDVBUTF8((const char *)buf, evt->event_name_length, table, tsidonid));

	buf += evt->event_name_length;
	unsigned char textlength = *((unsigned char *)buf);
	
	if(textlength > 2)
		e.setText(language, convertDVBUTF8((const char *)(++buf), textlength, table, tsidonid));
}

void SIsectionEIT::parseDescriptors(const uint8_t *des, unsigned len, SIevent &e)
{
	struct descr_generic_header *desc;
	/* we pass the buffer including the eit_event header, so we have to
	   skip it here... */
	des += sizeof(struct eit_event);
	len -= sizeof(struct eit_event);
	
	while(len>=sizeof(struct descr_generic_header)) 
	{
		desc = (struct descr_generic_header *)des;
		// printf("Type: %s\n", decode_descr(desc->descriptor_tag));
		if(desc->descriptor_tag==0x4D)
			parseShortEventDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x4E)
			parseExtendedEventDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x54)
			parseContentDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x50)
			parseComponentDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x55)
			parseParentalRatingDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x4A)
			parseLinkageDescriptor((const uint8_t *)desc, e, len);
		else if(desc->descriptor_tag==0x69)
			parsePDCDescriptor((const uint8_t *)desc, e, len);
		if((unsigned)(desc->descriptor_length+2)>len)
			break;
		len-=desc->descriptor_length+2;
		des+=desc->descriptor_length+2;
	}
}

// Die infos aus dem Puffer holen
void SIsectionEIT::parse(void)
{
	const uint8_t *actPos;
	const uint8_t *bufEnd;
	struct eit_event *evt;
	unsigned short descriptors_loop_length;

	if (!buffer || parsed)
		return;

	if (bufferLength < sizeof(SI_section_EIT_header) + sizeof(struct eit_event)) 
	{
		bufferLength=0;
		return;
	}

	unsigned char table_id = header()->table_id;
	unsigned char version_number = header()->version_number;
	actPos = buffer + sizeof(SI_section_EIT_header);
	bufEnd = buffer + bufferLength;

	while (actPos < bufEnd - sizeof(struct eit_event)) 
	{
		evt = (struct eit_event *) actPos;
		SIevent e(evt);
		e.service_id = service_id();
		e.original_network_id = original_network_id();
		e.transport_stream_id = transport_stream_id();
		e.table_id = table_id;
		e.version = version_number;
		descriptors_loop_length = sizeof(struct eit_event) + ((evt->descriptors_loop_length_hi << 8) | evt->descriptors_loop_length_lo);
		parseDescriptors(actPos, min((unsigned)(bufEnd - actPos), descriptors_loop_length), e);
		evts.insert(e);
		actPos += descriptors_loop_length;
	}

	parsed = 1;
}

