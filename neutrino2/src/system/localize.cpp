/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <system/localize.h>
#include <system/debug.h>
#include <system/helpers.h>

#include <cstring>
#include <fstream>
#include <string>

#include <iostream>
#include <map>


//
static const char * iso639filename = DATADIR "/iso-codes/iso-639.tab";

std::map<std::string, std::string> iso639;
std::map<std::string, std::string> iso639rev;

void initialize_iso639_map(void)
{
	std::string s, t, u, v;
	std::ifstream in(iso639filename);
	
	if (in.is_open())
	{
		while (in.peek() == '#')
			getline(in, s);
		while (in >> s >> t >> u >> std::ws)
		{
			getline(in, v);
			iso639[s] = v;
			if (s != t) {
				iso639[t] = v;
			}
			iso639rev[v] = s;
		}
	}
	else
		std::cout << "Loading " << iso639filename << " failed." << std::endl;
}

const char * getISO639Description(const char * const iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));

	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}

//
void CLocaleManager::loadLocale(const char* const locale)
{
	dprintf(DEBUG_NORMAL, "CLocaleManager::loadLocale: %s\n", locale);
	
	// set lang
	setenv("LANG", locale, 1);
	setenv("LANGUAGE", locale, 1);	
	setlocale(LC_MESSAGES, locale);
}

void CLocaleManager::registerPlugin(const char *const plugin, const char * const localedir)
{
	dprintf(DEBUG_NORMAL, "CLocaleManager::registerPlugin: %s\n", plugin);
	
	bindtextdomain(plugin, /*LOCALEDIR*/localedir);
}

