/*
 * $Id: pmt.h,v 1.11 2013/08/18 11:23:30 mohousch Exp $
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

#ifndef __zapit_pmt_h__
#define __zapit_pmt_h__

#include "channel.h"
#include "ci.h"
#include "frontend_c.h"


int parse_pmt(CZapitChannel * const channel, CFrontend * fe);
int pmt_set_update_filter(CZapitChannel * const channel, int * fd, CFrontend * fe);
int pmt_stop_update_filter(int * fd);

#endif /* __zapit_pmt_h__ */
