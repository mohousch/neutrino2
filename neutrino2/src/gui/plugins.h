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

#ifndef __plugins__
#define __plugins__

#include <string>
#include <vector>

#include <gui/widget/icons.h>

#include <driver/framebuffer.h>

#include <system/localize.h>
#include <system/helpers.h>


typedef void (*PluginExec)(void);
typedef void (*PluginInit)(void);
typedef void (*PluginDel)(void);

typedef enum plugin_type
{
	PLUGIN_TYPE_DISABLED 	= 0,
	PLUGIN_TYPE_SCRIPT   	= 1,
	PLUGIN_TYPE_LEGACY 	= 2,
	PLUGIN_TYPE_PYTHON   	= 3,
	PLUGIN_TYPE_LUA         = 4
}
plugin_type_t;

typedef enum integration_type
{
	INTEGRATION_TYPE_DISABLED	= 0,
	INTEGRATION_TYPE_MAIN		= 1,
	INTEGRATION_TYPE_MULTIMEDIA	= 2,
	INTEGRATION_TYPE_SETTING	= 3,
	INTEGRATION_TYPE_SERVICE	= 4,
	INTEGRATION_TYPE_POWER		= 5,
	INTEGRATION_TYPE_USER		= 6
}
integration_type_t;

class CPlugins
{
	public:
		typedef enum p_type
		{
			P_TYPE_DISABLED = 0x1,
			P_TYPE_SCRIPT   = 0x2,
			P_TYPE_LEGACY   = 0x4,
			P_TYPE_PYTHON   = 0x8,
			P_TYPE_LUA      = 0x10
		}
		p_type_t;

		//
		typedef enum i_type
		{
			I_TYPE_DISABLED	= 0x1,
			I_TYPE_MAIN		= 0x2,
			I_TYPE_MULTIMEDIA	= 0x4,
			I_TYPE_SETTING		= 0x8,
			I_TYPE_SERVICE		= 0x10,
			I_TYPE_POWER		= 0x20,
			I_TYPE_USER		= 0x40
		}
		i_type_t;
		
		struct plugin
		{
			std::string filename;
			std::string cfgfile;
			std::string pluginfile;
			std::string name;
			std::string description;
			std::string version;
			CPlugins::p_type_t type;
			CPlugins::i_type_t integration;
			std::string icon;
			bool hide;
			
			bool operator< (const plugin& a) const
			{
				return this->filename < a.filename ;
			}
		};

	private:

		CFrameBuffer	*frameBuffer;

		int number_of_plugins;
		std::vector<plugin> plugin_list;
		std::string plugin_dir;

		bool parseCfg(plugin *plugin_data);
	
	public:

		~CPlugins();
		
		//
		void setPluginDir(const std::string& dir) { plugin_dir = dir; }
		void addPlugin(const char *dir);
		void loadPlugins();
		
		//
		int find_plugin(const std::string & filename);
		bool pluginfile_exists(const std::string & filename);
		bool plugin_exists(const std::string & filename);

		// get methods
		inline int getNumberOfPlugins(void) const { return plugin_list.size(); }
		inline const char* getName(const int number) const { return plugin_list[number].name.c_str(); }
		inline const char* getPluginFile(const int number) const { return plugin_list[number].pluginfile.c_str(); }
		inline const char* getFileName(const int number) const { return plugin_list[number].filename.c_str(); }
		inline const std::string& getDescription(const int number) const { return plugin_list[number].description; }
		inline const std::string& getVersion(const int number) const { return plugin_list[number].version; }
		inline int getType(const int number) const { return plugin_list[number].type; }
		inline int getIntegration(const int number) const { return plugin_list[number].integration; }
		inline const char* getIcon(const int number) const { return plugin_list[number].icon.c_str();}
		inline bool isHidden(const int number) {return plugin_list[number].hide;};

		CPlugins::p_type_t getPluginType(int type);
		CPlugins::i_type_t getPluginIntegration(int integration);

		inline bool isHidden(const int number) const { return plugin_list[number].hide; }

		//
		void startPlugin(int number);
		void start_plugin_by_name(const std::string & filename);
		void startScriptPlugin(int number);
		void startPlugin(const char * const filename);
		bool hasPlugin(CPlugins::p_type_t type);
		
		//
		void removePlugin(int number);
};

#endif
