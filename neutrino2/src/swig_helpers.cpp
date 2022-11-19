/*
 * $Id: swig_helpers.cpp 07.02.2019 mohousch Exp $
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <swig_helpers.h>


// fontRenderer
void CSwigHelpers::RenderString(int font_type, int x, int y, const int width, const char * text, const uint8_t color, const int boxheight, bool utf8_encoded, const bool useBackground)
{
	g_Font[font_type]->RenderString(x, y, width, text, color, boxheight, utf8_encoded, useBackground);
}

int CSwigHelpers::getRenderWidth(int font_type, const char *text, bool utf8_encoded)
{
	return g_Font[font_type]->getRenderWidth(text, utf8_encoded);
}

int CSwigHelpers::getHeight(int font_type)
{
	return g_Font[font_type]->getHeight();
}

// CRCInput
neutrino_msg_t CSwigHelpers::getRCCode(uint64_t timeout)
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	g_RCInput->getMsg_ms(&msg, &data, timeout);
	
	if (msg <= RC_MaxRC) 
		return msg;
	else 
		return -1;
}

void CSwigHelpers::addTimer(uint64_t Interval, bool oneshot, bool correct_time)
{
	g_RCInput->addTimer(Interval, oneshot, correct_time);
}

void CSwigHelpers::killTimer(uint32_t id)
{
	g_RCInput->killTimer(id);
}

neutrino_msg_data_t CSwigHelpers::getRCData(uint64_t timeout)
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	g_RCInput->getMsg_ms(&msg, &data, timeout);
	
	if (msg <= RC_MaxRC) 
		return data;
	else 
		return -1;
}


