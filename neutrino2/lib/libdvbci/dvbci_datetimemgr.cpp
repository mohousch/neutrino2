/* DVB CI DateTime Manager */
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <time.h>

#include "dvbci_datetimemgr.h"

eDVBCIDateTimeSession::eDVBCIDateTimeSession(eDVBCISlot *tslot)
{
	slot = tslot;
	slot->hasDateTime = true;
	slot->pollConnection = true;
}

eDVBCIDateTimeSession::~eDVBCIDateTimeSession()
{
	slot->hasDateTime = false;
}

int eDVBCIDateTimeSession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	printf("[CI DT] SESSION(%d)/DATETIME %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
	for (int i = 0; i < len; i++)
		printf("%02x ", ((const unsigned char *)data)[i]);
	printf("\n");

	if ((tag[0] == 0x9f) && (tag[1] == 0x84))
	{
		switch (tag[2])
		{
			case 0x40:
				state = stateSendDateTime;
				return 1;
				break;
			default:
				printf("[CI DT] unknown APDU tag 9F 84 %02x\n", tag[2]);
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
			sendDateTime();
			return 0;
		case stateFinal:
			printf("stateFinal und action! kann doch garnicht sein ;)\n");
		// fall through
		default:
			return 0;
	}
}

void eDVBCIDateTimeSession::sendDateTime()
{
	struct tm tm_gmt;
	struct tm tm_loc;
	unsigned char tag[3] = {0x9f, 0x84, 0x41}; // date_time_response
	unsigned char msg[7] = {0, 0, 0, 0, 0, 0, 0};
	printf("[CI DT] -> %s\n", __FUNCTION__);
	time_t t = time(NULL);
	if (gmtime_r(&t, &tm_gmt) && localtime_r(&t, &tm_loc))
	{
		int Y = tm_gmt.tm_year;
		int M = tm_gmt.tm_mon + 1;
		int D = tm_gmt.tm_mday;
		int L = (M == 1 || M == 2) ? 1 : 0;
		int MJD = 14956 + D + (int)((Y - L) * 365.25) + (int)((M + 1 + L * 12) * 30.6001);

#define DEC2BCD(d) (((d / 10) << 4) + (d % 10))

		msg[0] = htons(MJD) >> 8;
		msg[1] = htons(MJD) & 0xff;
		msg[2] = DEC2BCD(tm_gmt.tm_hour);
		msg[3] = DEC2BCD(tm_gmt.tm_min);
		msg[4] = DEC2BCD(tm_gmt.tm_sec);
		msg[5] = htons(tm_loc.tm_gmtoff / 60) >> 8;
		msg[6] = htons(tm_loc.tm_gmtoff / 60) & 0xff;
	}
	sendAPDU(tag, msg, 7);
}
