/*
 * $Id: bouquets.h 23.09.2023 mohousch Exp $
 *
 * BouquetManager for zapit - d-box2 linux project
 *
 * (C) 2002 by Simplex    <simplex@berlios.de>,
 *             rasc       <rasc@berlios.de>,
 *             thegoodguy <thegoodguy@berlios.de>
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

#ifndef __bouquets_h__
#define __bouquets_h__

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <ctype.h>

#include <inttypes.h>

#include "zapit.h"
#include <zapit/channel.h>

#include <libxmltree/xmlinterface.h>


class CZapitBouquet
{
	public:
		std::string Name;
		bool bHidden;
		bool bLocked;
		bool bUser;
		bool bFav;
		bool bWebTV;

		ZapitChannelList radioChannels;
		ZapitChannelList tvChannels;

		//
		inline CZapitBouquet(const std::string name) { Name = name; bHidden = false; bLocked = false; bUser = false; bFav = false; bWebTV = false;}
		
		CZapitChannel * getChannelByChannelID(const t_channel_id channel_id, const unsigned char serviceType = ST_RESERVED);
		void addService(CZapitChannel * newChannel);
		void removeService(CZapitChannel * oldChannel);
		void removeService(const t_channel_id channel_id, unsigned char serviceType = ST_RESERVED) 			 			{ 
			removeService(getChannelByChannelID(channel_id, serviceType)); 
		}
		void moveService (const unsigned int oldPosition, const unsigned int newPosition, const unsigned char serviceType);
		void sortBouquet(void);
};

typedef std::vector<CZapitBouquet *> BouquetList;

class CBouquetManager
{
	public:
		BouquetList Bouquets;
		
	private:
		CZapitBouquet* remainChannels;

		void makeRemainingChannelsBouquet(void);
		void parseBouquetsXml(const char* fname, bool ub = false);
		void writeBouquetHeader(FILE * bouq_fd, uint32_t i, const char * bouquetName);
		void writeBouquetFooter(FILE * bouq_fd);
		void writeBouquetChannels(FILE * bouq_fd, uint32_t i);
		void makeBouquetfromCurrentservices (const xmlNodePtr root);

	public:
		CBouquetManager() 
		{ 
			remainChannels = NULL; 
			//Bouquets = NULL; 
		};
		
		class ChannelIterator
		{
			private:
				CBouquetManager * Owner;
				CZapit::channelsMode mode;
				unsigned int b;
				int c;
				friend class CZapit;
				
				ZapitChannelList *getBouquet() 
				{ 
					ZapitChannelList * ret = &(Owner->Bouquets[b]->tvChannels);
					
					if (mode == CZapit::MODE_TV)
						ret = &(Owner->Bouquets[b]->tvChannels);
					else if (mode == CZapit::MODE_RADIO)
						ret = &(Owner->Bouquets[b]->radioChannels);
						
					return ret;
				};
				
			public:
				ChannelIterator(CBouquetManager *owner, const CZapit::channelsMode Mode = CZapit::MODE_TV);
				ChannelIterator operator ++(int);
				CZapitChannel* operator *();
				ChannelIterator FindChannelNr(const unsigned int channel);
				int getLowestChannelNumberWithChannelID(const t_channel_id channel_id);
				int getNrofFirstChannelofBouquet(const unsigned int bouquet_nr);
				bool EndOfChannels() { return (c == -2); };
		};

		ChannelIterator tvChannelsBegin() { return ChannelIterator(this, CZapit::MODE_TV); };
		ChannelIterator radioChannelsBegin() { return ChannelIterator(this, CZapit::MODE_RADIO); };

		void saveBouquets(void);
		void saveUBouquets(void);
		void loadBouquets(bool loadCurrentBouquet = false);
		CZapitBouquet* addBouquet(const std::string& name, bool ub = false, bool webtvb = false);
		CZapitBouquet* addBouquetIfNotExist(const std::string& name, bool ub = false, bool webtvb = false);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CZapitBouquet* bouquet);
		int  existsBouquet(char const * const name);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet(unsigned int bq_id, const t_channel_id channel_id);
		void renameBouquet(const unsigned int bouquet, const char * const newName); // UTF-8 encoded
		void setBouquetLock(const unsigned int bouquet, const bool lock);
		void setBouquetHidden(const unsigned int bouquet, const bool hidden);

		//
		void clearAll();

		//
		CZapitChannel* findChannelByChannelID(const t_channel_id channel_id);
		CZapitChannel* findChannelByName(std::string name, const t_service_id sid);

		// webtv
		void parseWebTVBouquet(std::string filename);
		void loadWebTVBouquets(const std::string& dirname);
		
		//
		void sortBouquets(void);
};

/*
 * Struct for channel comparison by channel names 
 *
 * TODO:
 * Channel names are not US-ASCII, but UTF-8 encoded.
 * Hence we need a compare function that considers the whole unicode charset.
 * For instance all countless variants of the letter a have to be regarded as the same letter.
 */ 
struct CmpChannelByChName: public std::binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
	static bool comparetolower(const char a, const char b)
	{
		return tolower(a) < tolower(b);
	};

	bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
	{
		return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
	};
};

struct CmpBouquetByChName: public std::binary_function <const CZapitBouquet * const, const CZapitBouquet * const, bool>
{
	static bool comparetolower(const char a, const char b)
	{
		return tolower(a) < tolower(b);
	};

	bool operator() (const CZapitBouquet * const c1, const CZapitBouquet * const c2)
	{
		return std::lexicographical_compare(c1->Name.begin(), c1->Name.end(), c2->Name.begin(), c2->Name.end(), comparetolower);
	};
};

#endif /* __bouquets_h__ */


