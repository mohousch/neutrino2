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

#include <linux/dvb/frontend.h>

#include <eventserver.h>

#include "ci.h"
#include "descriptors.h"
#include "sdt.h"
#include "client/zapittypes.h"
#include <xmlinterface.h>

#include <map>
#define zapped_chan_is_nvod 0x80


void parseTransponders(_xmlNodePtr node, t_satellite_position satellitePosition, delivery_system_t system );
void parseChannels(_xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, t_satellite_position satellitePosition, freq_id_t freq, uint8_t polarisation );
void FindTransponder(_xmlNodePtr root);
int loadTransponders();
int loadServices(bool only_current);
void saveServices(bool tocopy=false);
void saveMotorPositions();
int loadMotorPositions(void);

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
typedef std::map<transponder_id_t, bool> sdt_tp_t;                    // used in zapit.cpp

extern transponder_list_t scantransponders;         // defined in zapit.cpp
extern transponder_list_t transponders;             // defined in zapit.cpp

#endif /* __getservices_h__ */
