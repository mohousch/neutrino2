//
// $Id: SIlanguage.cpp,v 1.4 2006/04/21 20:40:13 houdini Exp $
//
// Class for filtering preferred language
//
//    Copyright (C) 2001 arzka (dbox2@oh3mqu.pp.hyper.fi)
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
// $Log: SIlanguage.cpp,v $
// Revision 1.4  2006/04/21 20:40:13  houdini
// improvments when not using the MultiLanguageEPG feature
//
// Revision 1.3  2006/04/13 19:10:54  mws
// bugfix returned wrong value for error while saving;
// preincrement iterators;
//
// Revision 1.2  2006/04/12 21:23:58  Arzka
// Optimization.
// Removed unnecessary copying of std:map and
// removed few avoidable std::string creation
//
// Revision 1.1  2006/03/26 20:13:49  Arzka
// Added support for selecting EPG language
//

#include "SIlanguage.hpp"

#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include <fstream>

#include <system/debug.h>


std::vector<std::string> SIlanguage::languages;
pthread_mutex_t SIlanguage::languages_lock = PTHREAD_MUTEX_INITIALIZER;

/*
ALL = Show all available languages
FIRST_FIRST = Show first found language from list. If not found show first of available language
FIRST_ALL = Show first found language from list. If not found show all available languages.
ALL_FIRST = Show all wanted languages if possible. If not found show first of av ailable language
ALL_ALL = Show all wanted languages if possible. If not found show all available languages.
*/

CSectionsd::SIlanguageMode_t SIlanguage::mode = CSectionsd::FIRST_ALL;

void SIlanguage::filter(const std::map<std::string, std::string>& s, int max, std::string& retval)
{
	pthread_mutex_lock(&languages_lock);
	// languages cannot get iterated through
	// if another thread is updating it simultaneously
	int count = max;

	if (mode != CSectionsd::ALL) 
	{
		for (std::vector<std::string>::const_iterator it = languages.begin(); it != languages.end() ; ++it) 
		{
			std::map<std::string,std::string>::const_iterator text;
			if ((text = s.find(*it)) != s.end()) 
			{
				if (count != max) 
				{
					retval.append(" \n");
				}
				retval.append(text->second);
				if (--count == 0) 
					break;
				if (mode == CSectionsd::FIRST_FIRST || mode == CSectionsd::FIRST_ALL) 
				{
					break;
				}
			}
		}
	}

	if (retval.length() == 0) 
	{
		// return all available languages
		if (s.begin() != s.end()) 
		{
			for (std::map<std::string, std::string>::const_iterator it = s.begin(); it != s.end() ; ++it) 
			{
				if (it != s.begin()) 
				{
					retval.append(" \n");
				}
				retval.append(it->second);
				if (--max == 0) 
					break;
				if (mode == CSectionsd::FIRST_FIRST || mode == CSectionsd::ALL_FIRST) 
				{
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&languages_lock);
}


bool SIlanguage::loadLanguages()
{
	dprintf(DEBUG_NORMAL, "SIlanguage::loadLanguages:\n");
	
	pthread_mutex_lock(&languages_lock);
	std::ifstream file(LANGUAGEFILE);
	std::string word;
	CSectionsd::SIlanguageMode_t tmpMode = CSectionsd::FIRST_ALL;
	std::vector<std::string> tmpLang;

	if (!(file >> word)) 
		goto error;
	if (word == "FIRST_FIRST") 
	{
		tmpMode = CSectionsd::FIRST_FIRST;
	} 
	else if (word == "FIRST_ALL") 
	{
		tmpMode = CSectionsd::FIRST_ALL;
	} 
	else if (word == "ALL_FIRST") 
	{
		tmpMode = CSectionsd::ALL_FIRST;
	} 
	else if (word == "ALL_ALL") 
	{
		tmpMode = CSectionsd::ALL_ALL;
	} 
	else if (word == "OFF") 
	{
		tmpMode = CSectionsd::LANGUAGE_MODE_OFF;
	}

	while (!file.eof()) 
	{
		if ((file >> word)) 
		{
			tmpLang.push_back(word);
		}
	}

	if (tmpLang.empty()) 
		tmpLang.push_back("OFF");
	languages = tmpLang;
	mode = tmpMode;
	pthread_mutex_unlock(&languages_lock);
	
	return true;

error:
	tmpLang.push_back("OFF");
	languages = tmpLang;
	mode = tmpMode;
	pthread_mutex_unlock(&languages_lock);
	
	return false;
}

bool SIlanguage::saveLanguages()
{
	dprintf(DEBUG_NORMAL, "SIlanguage::saveLanguages:\n");
	
	pthread_mutex_lock(&languages_lock);
	std::ofstream file(LANGUAGEFILE);
	
	switch(mode) 
	{
		case CSectionsd::ALL:
			file << "ALL\n";
			break;
		case CSectionsd::FIRST_FIRST:
			file << "FIRST_FIRST\n";
			break;
		case CSectionsd::FIRST_ALL:
			file << "FIRST_ALL\n";
			break;
		case CSectionsd::ALL_FIRST:
			file << "ALL_FIRST\n";
			break;
		case CSectionsd::ALL_ALL:
			file << "ALL_ALL\n";
			break;
		case CSectionsd::LANGUAGE_MODE_OFF:
			file << "OFF\n";
			break;
	}

	for (std::vector<std::string>::iterator it = languages.begin(); it != languages.end() ; ++it) 
	{
		file << *it;
		file << "\n";
		if (file.fail()) 
			goto error;
	}

	file.close();
	if (file.fail()) 
		goto error;

	pthread_mutex_unlock(&languages_lock);
	return true;

error:
	pthread_mutex_unlock(&languages_lock);
	return false;
}

void SIlanguage::setLanguages(const std::vector<std::string>& newLanguages)
{
	dprintf(DEBUG_NORMAL, "SIlanguage::setLanguages:\n");
	
	pthread_mutex_lock(&languages_lock);
	languages = newLanguages;
	pthread_mutex_unlock(&languages_lock);
}

std::vector<std::string> SIlanguage::getLanguages()
{
	dprintf(DEBUG_NORMAL, "SIlanguage::getLanguages:\n");
	
	pthread_mutex_lock(&languages_lock);
	std::vector<std::string> retval(languages);  // Store it before unlock
	pthread_mutex_unlock(&languages_lock);
	return retval;
}

void SIlanguage::setMode(CSectionsd::SIlanguageMode_t newMode)
{
	mode = newMode;
}

CSectionsd::SIlanguageMode_t SIlanguage::getMode()
{
	return mode;
}

