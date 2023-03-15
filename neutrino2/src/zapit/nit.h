/*
 * $Id: nit.h,v 1.7 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __zapit_nit_h__
#define __zapit_nit_h__


class CNit
{
	private:
		CNit(){};
		~CNit(){};
		
	public:
		//
		static CNit *getInstance()
		{
			static CNit * nit = NULL;

			if(!nit ) 
			{
				nit = new CNit();
			} 

			return nit;
		};
		
		//
		int parseNIT(t_satellite_position satellitePosition, freq_id_t freq, int feindex = 0);
};

#endif /* __zapit_nit_h__ */
