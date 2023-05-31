/*
 * $Id: satconfig.h,v 1.31 2013/08/18 11:23:30 mohousch Exp $
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

#ifndef _SAT_CONFIG_H_
#define _SAT_CONFIG_H_

typedef struct sat_config {
	t_satellite_position position;
	int diseqc;
	int commited;
	int uncommited;
	int motor_position;
	int diseqc_order;
	int lnbOffsetLow;
	int lnbOffsetHigh;
	int lnbSwitch;
	int use_in_scan;
	int use_usals;
	std::string name;
	int have_channels;
    	uint32_t system;
} sat_config_t;

typedef enum diseqc_cmd_order {
	UNCOMMITED_FIRST,
	COMMITED_FIRST
} diseqc_cmd_order_t;

typedef std::map<t_satellite_position, sat_config_t> satellite_map_t;
typedef std::map<t_satellite_position, sat_config_t>::iterator sat_iterator_t;

typedef std::map <int, std::string> scan_list_t;
typedef std::map <int, std::string>::iterator scan_list_iterator_t;

#endif		/* _SAT_CONFIG_H_ */

