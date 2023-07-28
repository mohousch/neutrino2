/* DVB CI DateTime Manager */
#include <stdio.h>

#include "dvbci_datetimemgr.h"
#include <system/debug.h>


eDVBCIDateTimeSession::eDVBCIDateTimeSession(tSlot *tslot)
{
	slot = tslot;
	slot->hasDateTime = true;
	slot->pollConnection = true;
}

eDVBCIDateTimeSession::~eDVBCIDateTimeSession()
{
	slot->hasDateTime = false;
}

int eDVBCIDateTimeSession::receivedAPDU(const unsigned char *tag,const void *data, int len)
{
	if ((tag[0] == 0x9f) && (tag[1] == 0x84))
	{
		switch (tag[2])
		{
			case 0x40:
				state = stateSendDateTime;

				return 1;
				break;
			default:
				dprintf(DEBUG_DEBUG, "unknown APDU tag 9F 84 %02x\n", tag[2]);
				break;
		}
	}

	return 0;
}

int eDVBCIDateTimeSession::doAction()
{
	switch (state)
	{
		case stateStarted:
			return 0;
		case stateSendDateTime:
		{
			unsigned char tag[3] = {0x9f, 0x84, 0x41}; // date_time_response
			unsigned char msg[7] = {0, 0, 0, 0, 0, 0, 0};
			sendAPDU(tag, msg, 7);

			return 0;
		}
		case stateFinal:
			dprintf(DEBUG_DEBUG, "stateFinal und action! kann doch garnicht sein ;)\n");
		default:
			return 0;
	}
}

