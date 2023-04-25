/* DVB CI CA Manager */
#include <stdio.h>
#include <stdint.h>
#include <algorithm>

#include "dvbci_camgr.h"

eDVBCICAManagerSession::eDVBCICAManagerSession(eDVBCISlot *tslot)
{
	slot = tslot;
}

eDVBCICAManagerSession::~eDVBCICAManagerSession()
{
	slot->hasCAManager = false;
	slot->camgrSession = NULL;
}

int eDVBCICAManagerSession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	printf("[CI CA] SESSION(%d)/CA %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
	for (int i = 0; i < len; i++)
		printf("%02x ", ((const unsigned char *)data)[i]);
	printf("\n");

	if ((tag[0] == 0x9f) && (tag[1] == 0x80))
	{
		switch (tag[2])
		{
			case 0x31:
			{
				printf("[CI CA] ca info:\n");
				for (int i = 0; i < len; i += 2)
				{
					printf("%04x ", (((const unsigned char *)data)[i] << 8) | (((const unsigned char *)data)[i + 1]));
					caids.push_back((((const unsigned char *)data)[i] << 8) | (((const unsigned char *)data)[i + 1]));
				}
				if (!caids.empty())
				{
					for (u32 i = 0; i < caids.size(); i++)
					{
						if (caids[i] == 0x1830)
						{
							caids.push_back(0x186A);
							printf("%04x ", 0x186A);
							caids.push_back(0x186D);
							printf("%04x ", 0x186D);
							break;
						}

						if (caids[i] == 0x0648)
						{
							caids.push_back(0x0650);
							printf("%04x ", 0x0650);
							break;
						}
					}
				}
				std::sort(caids.begin(), caids.end());
				printf("\n");

				slot->pollConnection = false;
				slot->hasCAManager = true;
				slot->camgrSession = this;
			}
			break;
			default:
				printf("[CI CA] unknown APDU tag 9F 80 %02x\n", tag[2]);
				break;
		}
	}
	return 0;
}

int eDVBCICAManagerSession::doAction()
{
	switch (state)
	{
		case stateStarted:
		{
			const unsigned char tag[3] = {0x9F, 0x80, 0x30}; // ca info enq
			sendAPDU(tag);
			state = stateFinal;
			return 0;
		}
		case stateFinal:
			printf("[CI CA] stateFinal and action should not happen\n");
		// fall through
		default:
			return 0;
	}
}

int eDVBCICAManagerSession::sendCAPMT(unsigned char *data, int len)
{
	const unsigned char tag[3] = {0x9F, 0x80, 0x32}; // ca_pmt

	sendAPDU(tag, data, len);

	return 0;
}

