/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __flashtool__
#define __flashtool__

#include <string>
#include <vector>

#include <gui/widget/progresswindow.h>


class CFlashTool
{
	private:
	
		CProgressWindow *statusViewer;
		std::string mtdDevice;
		std::string ErrorMessage;

		bool erase(int globalProgressEnd = -1);

	public:
		CFlashTool();
		~CFlashTool();

		const std::string &getErrorMessage(void) const;

		void setMTDDevice( const std::string & mtddevice );
		void setStatusViewer(CProgressWindow *statusview );

		bool program( const std::string & filename, int globalProgressEndErase = -1, int globalProgressEndFlash = -1 );
		bool readFromMTD( const std::string & filename, int globalProgressEnd=-1 );

		//bool check_cramfs( const std::string & filename );
		bool check_md5( const std::string & filename, const std::string & smd5);

		void reboot();
};

class CFlashVersionInfo
{
	private:
		char date[11];
		char time[6];
		char releaseCycle[20];
		
	public:
		char snapshot;
		
		CFlashVersionInfo(const std::string & versionString);
		
		const char * getDate(void) const;
		const char * getTime(void) const;
		const char * getReleaseCycle(void) const;
		const char * getType(void) const;
};

class CMTDInfo
{
	private:
		struct SMTDPartition
		{
			int size;
			int erasesize;
			std::string name;
			std::string filename;
		};

		std::vector<SMTDPartition *> mtdData;
		
		void getPartitionInfo();

		CMTDInfo();
		~CMTDInfo();

	public: 
		static CMTDInfo * getInstance();
	
		int getMTDCount();

		//mtdinfos abfragen (nach mtdnummer)
		std::string getMTDName(const int pos );
		std::string getMTDFileName(const int pos );
		int getMTDSize(const int pos );
		int getMTDEraseSize(const int pos );

		//mtdinfos abfragen (nach mtd-filename)
		std::string getMTDName(const std::string & filename);
		int getMTDSize( const std::string & filename );
		int getMTDEraseSize( const std::string & filename );

		int findMTDNumber(const std::string & filename);
		std::string findMTDsystem(const std::string & filename);
};

#endif
