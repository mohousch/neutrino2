/*
 * $Id: cam.cpp,v 1.34 2013/08/18 11:23:30 mohousch Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>,
 *             thegoodguy         <thegoodguy@berlios.de>
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

/* zapit */
#include <cam.h>
#include <settings.h> /* CAMD_UDS_NAME         */
#include <messagetools.h>   /* get_length_field_size */

#include <string.h>


unsigned char CCam::getVersion(void) const
{
	return 0x9F;
}

const char *CCam::getSocketName(void) const
{
	return CAMD_UDS_NAME;
}

bool CCam::sendMessage(const char * const data, const size_t length, bool update)
{
	/* send_data return false without trying, if no opened connection */
	if(update) 
	{
		if(!send_data(data, length)) 
		{
			if (!open_connection())
				return false;
			return send_data(data, length);
		}
		
		return true;
	}

	close_connection();

	if(!length) 
		return false;
	
	if (!open_connection())
		return false;

	return send_data(data, length);
}

bool CCam::setCaPmt(CZapitChannel * thischannel, CCaPmt * const caPmt, int demux, int camask, bool update)
{
	if (!caPmt)
		return true;

	printf("CCam::setCaPmt: demux_index:(%d) camask:(%d) update:(%s)\n", demux, camask, update ? "yes" : "no" );
	
	unsigned int size = caPmt->CamgetLength();
	unsigned char buffer[3 + get_length_field_size(size) + size];
	size_t pos = caPmt->CamwriteToBuffer(thischannel, buffer, demux, camask);

	return sendMessage((char *)buffer, pos, update);
}
