/*
	Experimental OPKG-Manager - Neutrino-GUI

	Based upon Neutrino-GUI 
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Implementation: 
	Copyright (C) 2012 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

        License: GPL

        This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.

		
	NOTE for ignorant distributors:
	It's not allowed to distribute any compiled parts of this code, if you don't accept the terms of GPL.
	Please read it and understand it right!
	This means for you: Hold it, if not, leave it! You could face legal action! 
	Otherwise ask the copyright owners, anything else would be theft!
*/

#ifndef __OPKG_MANAGER__
#define __OPKG_MANAGER__

#include <gui/widget/menue.h>

#include <string>


#define OM_MAX_LINE_LENGTH 512

typedef enum 
{
	OM_LIST,
	OM_LIST_INSTALLED,
	OM_LIST_UPGRADEABLE,
	OM_UPDATE,
	OM_UPGRADE,
	
	OM_MAX
} pkg_info_t;

typedef struct opkg_cmd_t
{
	const pkg_info_t info;
	const char * cmdstr;

} opkg_cmd_struct_t;

class COPKGManager : public CMenuTarget
{
	private:
		std::string filename;
		
		std::vector<std::string>* vp_pkg_menu;
		std::vector<std::string> v_pkg_list;
		std::vector<std::string> v_pkg_installed;
		std::vector<std::string> v_pkg_upgradable;
		
		bool execCmd(const char* cmdstr);
		bool getPkgData(const int pkg_content_id);
		std::string getBlankPkgName(const std::string& line);
		bool showPkgMenu(const int pkg_content_id);
		int showMenu();

	public:	

		COPKGManager();
		~COPKGManager();
		
		int exec(CMenuTarget* parent, const std::string & actionKey);
		static bool hasOpkgSupport();
};

#endif
