/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrinoMessages.h 2013/10/12 mohousch Exp $

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
		/* infoviewer */
		SHOW_EPG				 = CRCInput::RC_Messages + 1,
		SHOW_INFOBAR				 = CRCInput::RC_Messages + 2,
		
		/* vcrcontrol */
		VCR_ON					 = CRCInput::RC_Messages + 3,
		VCR_OFF					 = CRCInput::RC_Messages + 4,
		
		/* nhttpd */
		STANDBY_ON				 = CRCInput::RC_Messages + 5,
		STANDBY_OFF				 = CRCInput::RC_Messages + 6,
		
		/* nhttpd / SHTDCNT / timerd */
		SHUTDOWN				 = CRCInput::RC_Messages + 8,
		
		/* timerd */
		ANNOUNCE_SHUTDOWN			 = CRCInput::RC_Messages + 9,
		ANNOUNCE_ZAPTO				 = CRCInput::RC_Messages + 10,
		ZAPTO					 = CRCInput::RC_Messages + 11,
		ANNOUNCE_RECORD				 = CRCInput::RC_Messages + 12,
		RECORD_START				 = CRCInput::RC_Messages + 13,
		RECORD_STOP				 = CRCInput::RC_Messages + 14,
		ANNOUNCE_SLEEPTIMER			 = CRCInput::RC_Messages + 15,
		SLEEPTIMER				 = CRCInput::RC_Messages + 16,
		
		/* nhttpd / vcrcontrol */
		CHANGEMODE				 = CRCInput::RC_Messages + 17,
		
		/* timerd */
		REMIND					 = CRCInput::RC_Messages + 18,
		
		/* nhttpd */
		LOCK_RC                 		 = CRCInput::RC_Messages + 19,
		UNLOCK_RC               		 = CRCInput::RC_Messages + 20,
		REBOOT					 = CRCInput::RC_Messages + 21,
		RESTART					 = CRCInput::RC_Messages + 22,
		
		/* zapit */ 
		EVT_BOUQUETSCHANGED                      = CRCInput::RC_Events + 6,
		EVT_SERVICESCHANGED                      = CRCInput::RC_Events + 7,
		EVT_SCAN_COMPLETE                        = CRCInput::RC_Events + 16,
		EVT_SCAN_NUM_TRANSPONDERS                = CRCInput::RC_Events + 17,
		EVT_SCAN_NUM_CHANNELS                    = CRCInput::RC_Events + 18,
		
		/* triggered from userland / rcinput */
		EVT_TIMER                                = CRCInput::RC_Events + 20,
		
		/* remotecontrol */
		EVT_PROGRAMLOCKSTATUS                    = CRCInput::RC_Events + 22,
		
		/* zapit */
		EVT_RECORDMODE				 = CRCInput::RC_Events + 24,
		EVT_ZAP_CA_ID				 = CRCInput::RC_Events + 25,
		EVT_SCAN_FAILED                          = CRCInput::RC_Events + 29,
		EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS = CRCInput::RC_Events + 30,
 		EVT_SCAN_REPORT_FREQUENCY                = CRCInput::RC_Events + 31,
 		EVT_SCAN_FOUND_RADIO_CHAN                = CRCInput::RC_Events + 32,
 		EVT_SCAN_FOUND_DATA_CHAN                 = CRCInput::RC_Events + 33,
 		EVT_SCAN_FOUND_TV_CHAN                   = CRCInput::RC_Events + 34,
 		EVT_SCAN_REPORT_FREQUENCYP               = CRCInput::RC_Events + 36,
 		EVT_ZAP_MOTOR                            = CRCInput::RC_Events + 37,
		EVT_SERVICES_UPD                         = CRCInput::RC_Events + 38,
		
		/* sectionsd */
		EVT_SI_FINISHED                          = CRCInput::RC_Events + 39,
		
		/* zapit */
		EVT_PMT_CHANGED				 = CRCInput::RC_Events + 40,
		
		/* hdmi cec */
		EVT_HDMI_CEC_VIEW_ON                     = CRCInput::RC_Events + 41,
		EVT_HDMI_CEC_STANDBY                     = CRCInput::RC_Events + 42,

		/* cicam */
		EVT_CI_INSERTED				 = CRCInput::RC_Events + 60, /* data = slot num */
		EVT_CI_REMOVED				 = CRCInput::RC_Events + 61, /* data = slot num */
		EVT_CI_INIT_OK				 = CRCInput::RC_Events + 62, /* data = slot num */
		EVT_CI_MMI_MENU				 = CRCInput::RC_Events + 63,
		EVT_CI_MMI_LIST				 = CRCInput::RC_Events + 64,
		EVT_CI_MMI_TEXT				 = CRCInput::RC_Events + 65,
		EVT_CI_MMI_REQUEST_INPUT		 = CRCInput::RC_Events + 66,
		EVT_CI_MMI_CLOSE			 = CRCInput::RC_Events + 67,

		/* infoviewer */
		EVT_CURRENTEPG                           = CRCInput::RC_WithData +  1,
		EVT_NEXTEPG                              = CRCInput::RC_WithData +  2,
		
		/* nhttpd */
		EVT_POPUP                                = CRCInput::RC_WithData +  3,
		EVT_EXTMSG                               = CRCInput::RC_WithData +  4,
		EVT_START_PLUGIN                         = CRCInput::RC_WithData +  5,

		/* sectionsd */
		EVT_CURRENTNEXT_EPG                      = CRCInput::RC_WithData +  6, /* data: (t_channel_id *) */
		EVT_TIMESET                              = CRCInput::RC_WithData +  7, /* data: (long long *)    */

		/* infoviewer */
		EVT_NOEPG_YET                            = CRCInput::RC_WithData +  8, /* data: (t_channel_id *) */

		/* timerd */
		EVT_NEXTPROGRAM                          = CRCInput::RC_WithData +  9, /* data: (t_channel_id *) */

		/* zapit */
 		EVT_SCAN_FOUND_A_CHAN                    = CRCInput::RC_WithData + 10,
		EVT_SCAN_PROVIDER                        = CRCInput::RC_WithData + 11,
		EVT_SCAN_SATELLITE                       = CRCInput::RC_WithData + 12,
		EVT_SCAN_SERVICENAME                     = CRCInput::RC_WithData + 13,
		EVT_ZAP_COMPLETE                         = CRCInput::RC_WithData + 14, /* data: (t_channel_id *) */
		EVT_ZAP_FAILED                           = CRCInput::RC_WithData + 15, /* data: (t_channel_id *) */
		EVT_ZAP_ISNVOD                           = CRCInput::RC_WithData + 16, /* data: (t_channel_id *) */
		EVT_ZAP_SUB_COMPLETE                     = CRCInput::RC_WithData + 17, /* data: (t_channel_id *) */
		EVT_ZAP_SUB_FAILED                       = CRCInput::RC_WithData + 18, /* data: (t_channel_id *) */

		/* remotecontrol */
		EVT_ZAP_GOT_SUBSERVICES                  = CRCInput::RC_WithData + 19, /* data: (t_channel_id *) */
		EVT_ZAP_GOTAPIDS                         = CRCInput::RC_WithData + 20, /* data: (t_channel_id *) */
		EVT_ZAP_GOTPIDS                          = CRCInput::RC_WithData + 21  /* data: (t_channel_id *) */
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
