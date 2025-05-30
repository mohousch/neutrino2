//
// $Id: cam.h 20.10.2023 mohousch Exp $
//
// (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
//	thegoodguy         <thegoodguy@berlios.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef __zapit_cam_h__
#define __zapit_cam_h__

#include <vector>

#include <zapit/channel.h>
#include <basicclient.h>


//// defines
#define CAMD_UDS_NAME  			"/tmp/camd.socket"

////
class CCaDescriptor
{
	private:
		unsigned descriptor_tag		: 8;
		unsigned descriptor_length	: 8;
		unsigned CA_system_ID		: 16;
		unsigned reserved1		: 3;
		unsigned CA_PID			: 13;
		std::vector <unsigned char> private_data_byte;

	public:
		CCaDescriptor(const unsigned char * const buffer);
		unsigned writeToBuffer(unsigned char * const buffer);
		unsigned getLength(void)	{ return descriptor_length + 2; }
};

/*
 * children of this class need to delete all
 * CCaDescriptors in their destructors
 */
class CCaTable
{
	private:
		std::vector <CCaDescriptor *> ca_descriptor;

	protected:
		CCaTable(void)			{ info_length = 0; };
		~CCaTable(void);
		unsigned getLength(void)	{ return info_length + 2; }
		unsigned writeToBuffer(unsigned char * const buffer);

	public:
		unsigned reserved2		: 4;
		unsigned info_length		: 12;
		void addCaDescriptor(const unsigned char * const buffer);
};

class CEsInfo : public CCaTable
{
	protected:
		unsigned getLength(void)	{ return CCaTable::getLength() + 3; }
		unsigned writeToBuffer(unsigned char * const buffer);

	public:
		unsigned stream_type		: 8;
		unsigned reserved1		: 3;
		unsigned elementary_PID		: 13;

	friend class CCaPmt;
};

class CZapitChannel;

class CCaPmt : public CCaTable
{
	protected:
		unsigned ca_pmt_length;

	public:
		~CCaPmt(void);
		unsigned getLength(bool addPrivate = false);
		unsigned writeToBuffer(CZapitChannel * thisChannel, unsigned char * const buffer, int demux = 0, int camask = 1, bool addPrivate = false);

		unsigned ca_pmt_list_management	: 8;
		unsigned program_number		: 16;
		unsigned reserved1		: 2;
		unsigned version_number		: 5;
		unsigned current_next_indicator	: 1;

		std::vector<CEsInfo *> es_info;
};

////
class CCam : public CBasicClient
{
	private:
		virtual const char *getSocketName(void) const;

	public:
		bool sendMessage(const char * const data, const size_t length, bool update = false);
		bool setCaPmt(CZapitChannel * thischannel, CCaPmt * const caPmt, int demux = 0, int camask = 1, bool update = false);
};

#endif /* __cam_h__ */

