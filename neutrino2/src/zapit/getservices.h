/*
 * $Id: getservices.h 16.11.2020 mohousch Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __getservices_h__
#define __getservices_h__

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <ctype.h>

// system 
#include <string>
#include <inttypes.h>
#include <cstdlib>

#include <linux/dvb/frontend.h>

#include <zapit/ci.h>
#include <zapit/descriptors.h>
#include <zapit/sdt.h>
#include <zapit/zapittypes.h>


#include <libxmltree/xmlinterface.h>


#define zapped_chan_is_nvod 0x80

// transponder
struct transponder
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	struct dvb_frontend_parameters feparams;
	unsigned char polarization;
	bool updated;
    	delivery_system_t system;

	transponder(t_transport_stream_id p_transport_stream_id, struct dvb_frontend_parameters p_feparams, delivery_system_t _system = DVB_S)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		original_network_id = 0;
		updated = 0;
        	system = _system;
	}

	transponder(const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, const struct dvb_frontend_parameters p_feparams, const uint8_t p_polarization = 0, delivery_system_t _system = DVB_S)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams            = p_feparams;
		polarization        = p_polarization;
		updated = 0;
		system = _system;
	}

	transponder(t_transport_stream_id p_transport_stream_id, struct dvb_frontend_parameters p_feparams, unsigned short p_polarization, t_original_network_id p_original_network_id, delivery_system_t _system = DVB_S)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = p_polarization;
		original_network_id = p_original_network_id;
		updated = 0;
        	system = _system;
	}
};

typedef std::map<transponder_id_t, transponder> transponder_list_t;
typedef std::map <transponder_id_t, transponder>::iterator stiterator;  // used in scan.cpp
typedef std::map<transponder_id_t, bool> sdt_tp_t; 			// used in zapit.cpp

//
class CServices
{
	public:
		int newtpid;
		int tcnt;
		int scnt;
		uint32_t fake_tid;
		uint32_t fake_nid;

	private:
		CServices(){tcnt = 0; scnt = 0;};
		~CServices(){};
		
	public:
		static CServices *getInstance()
		{
			static CServices * services = NULL;

			if(!services) 
			{
				services = new CServices();
			} 

			return services;
		};
		
		//
		void parseTransponders(_xmlNodePtr node, t_satellite_position satellitePosition, delivery_system_t system );
		void parseChannels(_xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, uint8_t polarisation );
		void findTransponder(_xmlNodePtr root);
		void parseSatTransponders(fe_type_t frontendType, _xmlNodePtr search, t_satellite_position satellitePosition);
		int loadMotorPositions(void);
		void saveMotorPositions();
		void init_sat(t_satellite_position position);
		int loadTransponders();
		int loadServices(bool only_current);
		void saveServices(bool tocopy=false);
};

#endif /* __getservices_h__ */

