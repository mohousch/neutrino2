/*
 * $Header: /cvs/tuxbox/apps/misc/libs/libxmltree/xmlinterface.cpp,v 1.3 2009/02/18 17:51:55 seife Exp $
 *
 * xmlinterface for zapit - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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


 * those files (xmlinterface.cpp and xmlinterface.h) lived at three different places
   in the tuxbox-cvs before, so look there for history information:
   - apps/dvb/zapit/include/zapit/xmlinterface.h
   - apps/dvb/zapit/src/xmlinterface.cpp
   - apps/tuxbox/neutrino/daemons/sectionsd/xmlinterface.cpp
   - apps/tuxbox/neutrino/src/system/xmlinterface.cpp
   - apps/tuxbox/neutrino/src/system/xmlinterface.h
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "xmlinterface.h"
#include <xmltok.h>


unsigned long xmlGetNumericAttribute(const xmlNodePtr node, const char *name, const int base)
{
	char *ptr = xmlGetAttribute(node, name);

	if (!ptr)
		return 0;

	return strtoul(ptr, 0, base);
}

long xmlGetSignedNumericAttribute(const xmlNodePtr node, const char *name, const int base)
{
	char *ptr = xmlGetAttribute(node, name);

	if (!ptr)
		return 0;

	return strtol(ptr, 0, base);
}

xmlNodePtr xmlGetNextOccurence(xmlNodePtr cur, const char * s)
{
	while ((cur != NULL) && (strcmp(xmlGetName(cur), s) != 0))
		cur = cur->xmlNextNode;
		
	return cur;
}

//
std::string Unicode_Character_to_UTF8(const int character)
{
	char buf[XML_UTF8_ENCODE_MAX];
	int length = XmlUtf8Encode(character, buf);
	
	return std::string(buf, length);
}

xmlDocPtr parseXml(const char * data)
{
	XMLTreeParser* tree_parser;

	tree_parser = new XMLTreeParser(NULL);

	if (!tree_parser->Parse(data, strlen(data), true))
	{
		delete tree_parser;
		return NULL;
	}

	if (!tree_parser->RootNode())
	{
		delete tree_parser;
		return NULL;
	}
	
	return tree_parser;
}

xmlDocPtr parseXmlFile(const char * filename, bool warning_by_nonexistence)
{
	char buffer[2048];
	XMLTreeParser* tree_parser;
	size_t done;
	size_t length;
	FILE* xml_file;

	xml_file = fopen(filename, "r");

	if (xml_file == NULL)
	{
	        if (warning_by_nonexistence)
			perror(filename);
		return NULL;
	}

	tree_parser = new XMLTreeParser(NULL);

	do
	{
		length = fread(buffer, 1, sizeof(buffer), xml_file);
		done = length < sizeof(buffer);

		if (!tree_parser->Parse(buffer, length, done))
		{
			fprintf(stderr, "%s: Error parsing \"%s\": %s at line %d\n",
				__FUNCTION__,
				filename,
				tree_parser->ErrorString(tree_parser->GetErrorCode()),
				tree_parser->GetCurrentLineNumber());

			fclose(xml_file);
			delete tree_parser;
			return NULL;
		}
	}
	while (!done);

	fclose(xml_file);

	if (!tree_parser->RootNode())
	{
		delete tree_parser;
		return NULL;
	}
	return tree_parser;
}

