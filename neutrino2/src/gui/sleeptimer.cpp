/*      
        Neutrino-GUI  -   DBoxII-Project

        Copyright (C) 2001 Steffen Hehn 'McClean'
        Homepage: http://dbox.cyberphoria.org/

        License: GPL

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <global.h>

#include <gui/sleeptimer.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>

#include <timerd/timerd.h>

#include <daemonc/remotecontrol.h>

#include <system/debug.h>


extern t_channel_id live_channel_id; 			//defined in zapit.cpp

int CSleepTimerWidget::exec(CMenuTarget* parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CSleepTimerWidget::exec\n");

	int    res = CMenuTarget::RETURN_REPAINT;
	int    shutdown_min = 0;
	char   value[16];
	CStringInput* inbox = NULL;

	if (parent)
		parent->hide();
   
	shutdown_min = CTimerd::getInstance()->getSleepTimerRemaining();  // remaining shutdown time?
	sprintf(value,"%03d", shutdown_min);
	CSectionsd::CurrentNextInfo info_CurrentNext;
	CSectionsd::getInstance()->getCurrentNextServiceKey(live_channel_id & 0xFFFFFFFFFFFFULL, info_CurrentNext);
	
  	if ( info_CurrentNext.flags & CSectionsd::epgflags::has_current) 
	{
  		time_t jetzt = time(NULL);
  		int current_epg_time_duration_rest = (info_CurrentNext.current_time.duration + 150 - (jetzt - info_CurrentNext.current_time.starttime ))/60 ;
  		
  		if(shutdown_min == 0 && current_epg_time_duration_rest > 0 && current_epg_time_duration_rest < 1000)
  		{
  			sprintf(value,"%03d", current_epg_time_duration_rest);
  		}
  	}

	inbox = new CStringInput(_("Sleeptimer"), value, 3, _("Shutdown time in min. (000=off)"), _("The STB will shutdown after this time."), "0123456789 ");
	inbox->exec (NULL, "");
	inbox->hide ();

	delete inbox;

	int new_val = atoi(value);
		
	if(shutdown_min != new_val) 
	{
		shutdown_min = new_val;
		
		dprintf(DEBUG_NORMAL, "CSleepTimerWidget::exec: sleeptimer min: %d\n", shutdown_min);
		
		if (shutdown_min == 0)	// if set to zero remove existing sleeptimer 
		{
			if(CTimerd::getInstance()->getSleeptimerID() > 0) 
			{
				CTimerd::getInstance()->removeTimerEvent(CTimerd::getInstance()->getSleeptimerID());
			}
		}
		else	// set the sleeptimer to actual time + shutdown mins and announce 1 min before
			CTimerd::getInstance()->setSleeptimer(time(NULL) + ((shutdown_min - 1) * 60), time(NULL) + shutdown_min * 60, 0);
	}
	
	return res;
}

