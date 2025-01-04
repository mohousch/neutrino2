//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: neutrino2_lua.cpp 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#include <config.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <interfaces/lua/neutrino2_lua.h>

#include <system/helpers.h>
#include <system/debug.h>


extern "C" int luaopen_neutrino2(lua_State *L); // declare the wrapped module

static int lua_gettext(lua_State *L)
{
	lua_pushstring(L, gettext(luaL_checkstring(L, 1)));
	return (1);
}

static int lua_dgettext(lua_State *L)
{
	lua_pushstring(L, dgettext(luaL_checkstring(L, 1), luaL_checkstring(L, 2) ));
	return (1);
}

neutrinoLua::neutrinoLua()
{
	lua = luaL_newstate();

	// call lua basic libs
	luaL_openlibs(lua);

	// call wrapped module
	luaopen_neutrino2(lua);
	
	//
	lua_register(lua, "_", lua_gettext);
	lua_register(lua, "dgettext", lua_dgettext);
}

neutrinoLua::~neutrinoLua()
{
	if (lua != NULL)
	{
		lua_close(lua);
		lua = NULL;
	}
}

int neutrinoLua::execFile(const char *fileName)
{
	// load script 
	int ret = luaL_loadfile(lua, fileName);

	if (ret) 
	{
		bool isString = lua_isstring(lua, -1);
		const char *null = "NULL";

		dprintf(DEBUG_NORMAL, "neutrinoLua::execFile: can't load file: %s\n", isString ? lua_tostring(lua, -1) : null);	

		return ret;
	}

	// run script
	ret = lua_pcall(lua, 0, LUA_MULTRET, 0);

	if (ret)
	{
		bool isString = lua_isstring(lua, -1);
		const char *null = "NULL";

		dprintf(DEBUG_NORMAL, "neutrinoLua::execFile: error in script: %s\n", isString ? lua_tostring(lua, -1) : null);

		
		return ret;
	}
	
	return ret;
}

