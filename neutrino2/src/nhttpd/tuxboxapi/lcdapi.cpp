/*
	nhttpd  -  DBoxII-Project

	Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'
	Copyright (C) 2005 SnowHead

	$Id: lcdapi.cpp 0492025 mohousch Exp $

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

// c++
#include <cstdlib>
#include <cstring>

// tuxbox
#include <driver/lcdd.h>
#include <global.h>
#include <neutrino2.h>
#include <system/settings.h>

#include <driver/lcd/lcddisplay.h>

#include <fcntl.h>
#include <unistd.h>

// nhttpd
#include "lcdapi.h"


//
void CLCDAPI::clear(void)
{
	lcdd->Clear();
}

void CLCDAPI::lockDisplay(int plock)
{
	lcdd->Lock();
}

bool CLCDAPI::showPng(char *filename)
{
	return lcdd->ShowPng(filename);
}

bool CLCDAPI::shotPng(char *filename)
{
	return lcdd->DumpPng(filename);
}

////
CLCDAPI::CLCDAPI()
{
	lcdd = CLCD::getInstance();
}

