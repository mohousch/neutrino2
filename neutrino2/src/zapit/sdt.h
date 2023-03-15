/*
 * $Id: sdt.h,v 1.18 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __zapit_sdt_h__
#define __zapit_sdt_h__

#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>


class CSdt
{
	private:
		CSdt(){};
		~CSdt(){};
		
	public:
		//
		static CSdt *getInstance()
		{
			static CSdt * sdt = NULL;

			if(!sdt) 
			{
				sdt = new CSdt();
			} 

			return sdt;
		};
		
		//
		int parseSDT(t_transport_stream_id* , t_original_network_id*, t_satellite_position satellitePosition, freq_id_t freq, int feindex = 0);
		int parseCurrentSDT( const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id,t_satellite_position satellitePosition, freq_id_t freq, CFrontend * fe);
};

#endif /* __zapit_sdt_h__ */

