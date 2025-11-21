//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: neutrinoMessages.h 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

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

struct NeutrinoMessages 
{
	enum
	{
		// infoviewer
		SHOW_EPG				= CRCInput::RC_Messages + 1,
		SHOW_INFOBAR				= CRCInput::RC_Messages + 2,
		
		// record
		SCART_ON				= CRCInput::RC_Messages + 3,
		SCART_OFF				= CRCInput::RC_Messages + 4,
		
		// nhttpd
		STANDBY_ON				= CRCInput::RC_Messages + 5,
		STANDBY_OFF				= CRCInput::RC_Messages + 6,
		
		// nhttpd / SHTDCNT / timerd
		SHUTDOWN				= CRCInput::RC_Messages + 7,
		
		// timerd
		ANNOUNCE_SHUTDOWN			= CRCInput::RC_Messages + 8,
		ANNOUNCE_ZAPTO				= CRCInput::RC_Messages + 9,
		ZAPTO					= CRCInput::RC_Messages + 10,
		ANNOUNCE_RECORD				= CRCInput::RC_Messages + 11,
		RECORD_START				= CRCInput::RC_Messages + 12,
		RECORD_STOP				= CRCInput::RC_Messages + 13,
		ANNOUNCE_SLEEPTIMER			= CRCInput::RC_Messages + 14,
		SLEEPTIMER				= CRCInput::RC_Messages + 15,
		
		// nhttpd / record
		CHANGEMODE				= CRCInput::RC_Messages + 16,
		
		// timerd
		REMIND					= CRCInput::RC_Messages + 17,
		
		// nhttpd
		LOCK_RC                 		= CRCInput::RC_Messages + 18,
		UNLOCK_RC				= CRCInput::RC_Messages + 19,
		REBOOT					= CRCInput::RC_Messages + 20,
		RESTART					= CRCInput::RC_Messages + 21,
		
		// zapit
		EVT_BOUQUETSCHANGED			= CRCInput::RC_Messages + 22,
		EVT_SERVICESCHANGED			= CRCInput::RC_Messages + 23,
		EVT_SCAN_COMPLETE			= CRCInput::RC_Messages + 24,
		EVT_SCAN_NUM_TRANSPONDERS		= CRCInput::RC_Messages + 25,
		EVT_SCAN_NUM_CHANNELS			= CRCInput::RC_Messages + 26,
		
		// triggered from userland / rcinput
		EVT_TIMER				= CRCInput::RC_Messages + 27,
		
		// remotecontrol
		EVT_PROGRAMLOCKSTATUS			= CRCInput::RC_Messages + 28,
		
		// zapit
		EVT_RECORDMODE				= CRCInput::RC_Messages + 29,
		EVT_ZAP_CA_ID				= CRCInput::RC_Messages + 30,
		EVT_SCAN_FAILED				= CRCInput::RC_Messages + 31,
		EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS= CRCInput::RC_Messages + 32,
 		EVT_SCAN_REPORT_FREQUENCY		= CRCInput::RC_Messages + 33,
 		EVT_SCAN_FOUND_RADIO_CHAN		= CRCInput::RC_Messages + 34,
 		EVT_SCAN_FOUND_DATA_CHAN		= CRCInput::RC_Messages + 35,
 		EVT_SCAN_FOUND_TV_CHAN			= CRCInput::RC_Messages + 36,
 		EVT_SCAN_REPORT_FREQUENCYP		= CRCInput::RC_Messages + 37,
 		EVT_ZAP_MOTOR				= CRCInput::RC_Messages + 38,
		EVT_SERVICES_UPD			= CRCInput::RC_Messages + 39,
		
		// sectionsd
		EVT_SI_FINISHED				= CRCInput::RC_Messages + 40,
		
		// zapit
		EVT_PMT_CHANGED				= CRCInput::RC_Messages + 41,
		
		// streamts
		EVT_STREAM_START			= CRCInput::RC_Messages + 42,
		EVT_STREAM_STOP				= CRCInput::RC_Messages + 43,
		
		// radiotext
		EVT_SHOW_RADIOTEXT			= CRCInput::RC_Messages + 44,

		// cicam
		EVT_CI_INSERTED				= CRCInput::RC_Messages + 45,
		EVT_CI_REMOVED				= CRCInput::RC_Messages + 46,
		EVT_CI_INIT_OK				= CRCInput::RC_Messages + 47,
		EVT_CI_MMI_MENU				= CRCInput::RC_Messages + 48,
		EVT_CI_MMI_LIST				= CRCInput::RC_Messages + 49,
		EVT_CI_MMI_TEXT				= CRCInput::RC_Messages + 50,
		EVT_CI_MMI_REQUEST_INPUT		= CRCInput::RC_Messages + 51,
		EVT_CI_MMI_CLOSE			= CRCInput::RC_Messages + 52,

		// infoviewer
		EVT_CURRENTEPG				= CRCInput::RC_Messages + 53,
		EVT_NEXTEPG				= CRCInput::RC_Messages + 54,
		
		// nhttpd
		EVT_POPUP				= CRCInput::RC_Messages + 55,
		EVT_EXTMSG				= CRCInput::RC_Messages + 56,
		EVT_START_PLUGIN			= CRCInput::RC_Messages + 57,

		// sectionsd
		EVT_CURRENTNEXT_EPG			= CRCInput::RC_Messages + 58,
		EVT_TIMESET				= CRCInput::RC_Messages + 59,

		// infoviewer
		EVT_NOEPG_YET				= CRCInput::RC_Messages + 60,

		// timerd
		EVT_NEXTPROGRAM				= CRCInput::RC_Messages + 61,

		// zapit
		EVT_SCAN_PROVIDER			= CRCInput::RC_Messages + 62,
		EVT_SCAN_SATELLITE			= CRCInput::RC_Messages + 63,
		EVT_SCAN_SERVICENAME			= CRCInput::RC_Messages + 64,
		EVT_ZAP_COMPLETE			= CRCInput::RC_Messages + 65,
		EVT_ZAP_FAILED				= CRCInput::RC_Messages + 66,
		EVT_ZAP_ISNVOD				= CRCInput::RC_Messages + 67,
		EVT_ZAP_SUB_COMPLETE			= CRCInput::RC_Messages + 68,
		EVT_ZAP_SUB_FAILED			= CRCInput::RC_Messages + 69,

		// remotecontrol
		EVT_ZAP_GOT_SUBSERVICES			= CRCInput::RC_Messages + 70,
		EVT_ZAP_GOTAPIDS			= CRCInput::RC_Messages + 71,
		EVT_ZAP_GOTPIDS				= CRCInput::RC_Messages + 72
	};
};

#endif

