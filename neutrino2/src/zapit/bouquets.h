/*
 * $Id: bouquets.h 16.11.2020 thegoodguy Exp $
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
#include "zapit/channel.h"

#include <libxmltree/xmlinterface.h>


using namespace std;

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

typedef vector<CZapitBouquet *> BouquetList;

class CBouquetManager
{
	private:
		CZapitBouquet* remainChannels;

		void makeRemainingChannelsBouquet(void);
		void parseBouquetsXml(const char* fname, bool ub = false);
		void writeBouquetHeader(FILE * bouq_fd, uint32_t i, const char * bouquetName);
		void writeBouquetFooter(FILE * bouq_fd);
		void writeBouquetChannels(FILE * bouq_fd, uint32_t i);
		void makeBouquetfromCurrentservices (const _xmlNodePtr root);

	public:
		CBouquetManager() { remainChannels = NULL; };
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
					if (mode == CZapit::MODE_TV)
						return &(Owner->Bouquets[b]->tvChannels);
					else if (mode == CZapit::MODE_RADIO)
						return &(Owner->Bouquets[b]->radioChannels);
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

		BouquetList Bouquets;

		void saveBouquets(void);
		void saveUBouquets(void);
		void saveBouquets(const CZapit::bouquetMode bouquetMode, const char * const providerName);
		void loadBouquets(bool loadCurrentBouquet = false);

		CZapitBouquet* addBouquet(const std::string& name, bool ub = false, bool webtvb = false);
		CZapitBouquet* addBouquetIfNotExist(const std::string& name, bool ub = false, bool webtvb = false);
		void deleteBouquet(const unsigned int id);
		void deleteBouquet(const CZapitBouquet* bouquet);
		int  existsBouquet(char const * const name);
		void moveBouquet(const unsigned int oldId, const unsigned int newId);
		bool existsChannelInBouquet(unsigned int bq_id, const t_channel_id channel_id);

		void clearAll();

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
struct CmpChannelByChName: public binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
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

struct CmpBouquetByChName: public binary_function <const CZapitBouquet * const, const CZapitBouquet * const, bool>
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


