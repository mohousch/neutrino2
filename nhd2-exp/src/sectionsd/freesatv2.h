#ifndef FREESAT_H
#define FREESAT_H

#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h> 
#include <string> 

#include <config.h>


#define START   '\0' 
#define STOP    '\0' 
#define ESCAPE  '\1' 

#define TABLE1_FILENAME CONFIGDIR "/freesat.t1"
#define TABLE2_FILENAME CONFIGDIR "/freesat.t2"

class huffTableEntry 
{
	friend class freesatHuffmanDecoder;
	private: 
		uint32_t value; 
		uint16_t bits; 
		char next;
		huffTableEntry * nextEntry;
	public: 
		huffTableEntry(unsigned int Value, short Bits, char Next, huffTableEntry * NextEntry) : value(Value), bits(Bits), next(Next), nextEntry(NextEntry)
		{ }
};

class freesatHuffmanDecoder 
{
	private:
		huffTableEntry	*m_tables[2][256];
		bool 		loadFile(int tableid, const char *filename);
		void 		loadTables();
		bool		m_tablesLoaded;
	public:
		freesatHuffmanDecoder();
		~freesatHuffmanDecoder();
		std::string decode(const unsigned char *src, size_t size);
};

#endif

