/*
 * $Id: scan.h,v 1.14 2013/08/18 11:23:30 mohousch Exp $
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

#ifndef __scan_h__
#define __scan_h__

#include <linux/dvb/frontend.h>

#include <inttypes.h>

#include <map>
#include <string>


class CScan
{
	private:
		CScan(){};
		~CScan(){};
		
	public:
		//
		static CScan *getInstance()
		{
			static CScan * scan = NULL;

			if(!scan) 
			{
				scan = new CScan();
			} 

			return scan;
		};
		
		//
		bool tuneFrequency(FrontendParameters *feparams, uint8_t polarization, t_satellite_position satellitePosition, int feindex);
		int addToScan(transponder_id_t TsidOnid, FrontendParameters *feparams, uint8_t polarity, bool fromnit = 0, int feindex = 0);
		int getSDTS(t_satellite_position satellitePosition, int feindex);
		int scanTransponder(_xmlNodePtr transponder, uint8_t diseqc_pos, t_satellite_position satellitePosition, bool /*satfeed*/, int feindex);
		void scanProvider(_xmlNodePtr search, t_satellite_position satellitePosition, uint8_t diseqc_pos, bool satfeed, int feindex);
};

#endif /* __scan_h__ */

