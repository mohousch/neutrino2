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

#include <zapit/channel.h>

#include <libxmltree/xmlinterface.h>


////
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

//
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

