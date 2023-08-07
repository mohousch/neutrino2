/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrinoMessages.h 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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


#ifndef __neutrinoMessages__
#define __neutrinoMessages__

#include <driver/rcinput.h>


struct messages_return
{
	enum
	{
		none 		= 0x00,
		handled		= 0x01,
		unhandled	= 0x02,
		cancel_all	= 0x04,
		cancel_info 	= 0x08
	};
};

struct NeutrinoMessages {
	enum
	{
		SHOW_EPG				 = RC_Messages + 1,
		SHOW_INFOBAR				 = RC_Messages + 2,
		VCR_ON					 = RC_Messages + 3,
		VCR_OFF					 = RC_Messages + 4,
		STANDBY_ON				 = RC_Messages + 5,
		STANDBY_OFF				 = RC_Messages + 6,
		STANDBY_TOGGLE				 = RC_Messages + 7,
		SHUTDOWN				 = RC_Messages + 8,
		ANNOUNCE_SHUTDOWN			 = RC_Messages + 9,
		ANNOUNCE_ZAPTO				 = RC_Messages + 10,
		ZAPTO					 = RC_Messages + 11,
		ANNOUNCE_RECORD				 = RC_Messages + 12,
		RECORD_START				 = RC_Messages + 13,
		RECORD_STOP				 = RC_Messages + 14,
		ANNOUNCE_SLEEPTIMER			 = RC_Messages + 15,
		SLEEPTIMER				 = RC_Messages + 16,
		CHANGEMODE				 = RC_Messages + 17,
		REMIND					 = RC_Messages + 18,
		LOCK_RC                 		 = RC_Messages + 19,
		UNLOCK_RC               		 = RC_Messages + 20,
		REBOOT					 = RC_Messages + 21,
		RESTART					 = RC_Messages + 22,

		EVT_VOLCHANGED                           = RC_Events + 1,
		EVT_MUTECHANGED                          = RC_Events + 2,
		EVT_VCRCHANGED                           = RC_Events + 3,
		EVT_MODECHANGED                          = RC_Events + 4,
		EVT_BOUQUETSCHANGED                      = RC_Events + 6,
		EVT_SERVICESCHANGED                      = RC_Events + 7,
		EVT_SCAN_COMPLETE                        = RC_Events + 16,
		EVT_SCAN_NUM_TRANSPONDERS                = RC_Events + 17,
		EVT_SCAN_NUM_CHANNELS                    = RC_Events + 18,
		EVT_SHUTDOWN                             = RC_Events + 19,
		EVT_TIMER                                = RC_Events + 20,
		EVT_PROGRAMLOCKSTATUS                    = RC_Events + 22,
		EVT_RECORDMODE				 = RC_Events + 24,
		
		EVT_ZAP_CA_ID				 = RC_Events + 25,
		
		EVT_SCAN_FAILED                          = RC_Events + 29,
		EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS = RC_Events + 30,
 		EVT_SCAN_REPORT_FREQUENCY                = RC_Events + 31,
 		EVT_SCAN_FOUND_RADIO_CHAN                = RC_Events + 32,
 		EVT_SCAN_FOUND_DATA_CHAN                 = RC_Events + 33,
 		EVT_SCAN_FOUND_TV_CHAN                   = RC_Events + 34,
 		EVT_SCAN_REPORT_FREQUENCYP               = RC_Events + 36,
 		EVT_ZAP_MOTOR                            = RC_Events + 37,

 		/* sectionsd */
		EVT_SERVICES_UPD                         = RC_Events + 38,
		EVT_SI_FINISHED                          = RC_Events + 39,
		
		/* zapit */
		EVT_PMT_CHANGED				 = RC_Events + 40,
		
		/* hdmi cec */
		EVT_HDMI_CEC_VIEW_ON                     = RC_Events + 41,
		EVT_HDMI_CEC_STANDBY                     = RC_Events + 42,

		/* NEVER CHANGE THIS */
		EVT_CI_INSERTED				 = RC_Events + 60, /* data = slot num */
		EVT_CI_REMOVED				 = RC_Events + 61, /* data = slot num */
		EVT_CI_INIT_OK				 = RC_Events + 62, /* data = slot num */
		EVT_CI_MMI_MENU				 = RC_Events + 63,
		EVT_CI_MMI_LIST				 = RC_Events + 64,
		EVT_CI_MMI_TEXT				 = RC_Events + 65,
		EVT_CI_MMI_REQUEST_INPUT		 = RC_Events + 66,
		EVT_CI_MMI_CLOSE			 = RC_Events + 67,

		/**/
		EVT_CURRENTEPG                           = RC_WithData +  1,
		EVT_NEXTEPG                              = RC_WithData +  2,
		EVT_POPUP                                = RC_WithData +  3,
		EVT_EXTMSG                               = RC_WithData +  4,
		EVT_START_PLUGIN                         = RC_WithData +  5,

		/* sectionsd */
		EVT_CURRENTNEXT_EPG                      = RC_WithData +  6, /* data: (t_channel_id *) */
		EVT_TIMESET                              = RC_WithData +  7, /* data: (long long *)    */

		/* "sectionsd" events triggered by neutrino */
		EVT_NOEPG_YET                            = RC_WithData +  8, /* data: (t_channel_id *) */

		/* "timerd" events triggered by neutrino */
		EVT_NEXTPROGRAM                          = RC_WithData +  9, /* data: (t_channel_id *) */

		/* zapit */
 		EVT_SCAN_FOUND_A_CHAN                    = RC_WithData + 10,
		EVT_SCAN_PROVIDER                        = RC_WithData + 11,
		EVT_SCAN_SATELLITE                       = RC_WithData + 12,
		EVT_SCAN_SERVICENAME                     = RC_WithData + 13,
		EVT_ZAP_COMPLETE                         = RC_WithData + 14, /* data: (t_channel_id *) */
		EVT_ZAP_FAILED                           = RC_WithData + 15, /* data: (t_channel_id *) */
		EVT_ZAP_ISNVOD                           = RC_WithData + 16, /* data: (t_channel_id *) */
		EVT_ZAP_SUB_COMPLETE                     = RC_WithData + 17, /* data: (t_channel_id *) */
		EVT_ZAP_SUB_FAILED                       = RC_WithData + 18, /* data: (t_channel_id *) */

		/* "zapit" events triggered by neutrino */
		EVT_ZAP_GOT_SUBSERVICES                  = RC_WithData + 19, /* data: (t_channel_id *) */
		EVT_ZAP_GOTAPIDS                         = RC_WithData + 20, /* data: (t_channel_id *) */
		EVT_ZAP_GOTPIDS                          = RC_WithData + 21 /* data: (t_channel_id *) */
	};

	enum
	{
		mode_unknown = -1,
		mode_tv = 	1,
		mode_radio = 2,
		mode_scart = 3,
		mode_standby = 4,
		mode_audio = 5,
		mode_pic = 6,
		mode_ts = 7,
		mode_mask = 0xFF,
		norezap = 0x100
	};
};

#endif
