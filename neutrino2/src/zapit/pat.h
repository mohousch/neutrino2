/*
 * $Id: pat.h,v 1.19 2013/08/18 11:23:30 mohousch Exp $
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

#ifndef __zapit_pat_h__
#define __zapit_pat_h__

#include <zapit/channel.h>
#include <zapit/frontend_c.h>


class CPat
{
	private:
		CPat(){};
		~CPat(){};
		
	public:
		//
		static CPat *getInstance()
		{
			static CPat * pat = NULL;

			if(!pat) 
			{
				pat = new CPat();
			} 

			return pat;
		};
		
		//
		int parsePAT(CZapitChannel * const channel, CFrontend * fe);
};

#endif /* __zapit_pat_h__ */

