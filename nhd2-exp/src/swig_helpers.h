/*
 * $Id: swig_helpers.h 07.02.2019 mohousch Exp $
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

#ifndef __SWIG_HELPERS_H__
#define __SWIG_HELPERS_H__

#include <driver/framebuffer.h>
#include <driver/rcinput.h>
#include <driver/fontrenderer.h>
#include <system/settings.h>


class CSwigHelpers
{
	private:

	public:
		CSwigHelpers(){};

		// fontrenderer
		void RenderString(int font_type, int x, int y, const int width, const char * text, const uint8_t color, const int boxheight = 0, bool utf8_encoded = true, const bool useBackground = false);

		int getRenderWidth(int font_type, const char *text, bool utf8_encoded = true);
		int getHeight(int font_type);

		// CRCInput
		neutrino_msg_t getRCCode(uint64_t timeout);
		void addTimer(uint64_t Interval, bool oneshot = true, bool correct_time = true );
		void killTimer(uint32_t id);
		neutrino_msg_data_t getRCData(uint64_t timeout);
};

#endif


