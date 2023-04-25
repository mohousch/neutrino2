/* DVB CI Application Manager */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "dvbci_appmgr.h"

/* prevent possibly segfaults: read at end of this file */
#define yy_debug 0

eDVBCIApplicationManagerSession::eDVBCIApplicationManagerSession(eDVBCISlot *tslot)
{
	slot = tslot;
	slot->hasAppManager = true;
	slot->appSession = this;
}

eDVBCIApplicationManagerSession::~eDVBCIApplicationManagerSession()
{
	slot->hasAppManager = false;
	slot->appSession = NULL;
}

int eDVBCIApplicationManagerSession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	printf("[CI AM] SESSION(%d)/APP %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
	for (int i = 0; i < len; i++)
		printf("%02x ", ((const unsigned char *)data)[i]);
	printf("\n");

	if ((tag[0] == 0x9f) && (tag[1] == 0x80))
	{
		switch (tag[2])
		{
			case 0x21:
			{
				int dl;
				printf("[CI AM] application info:\n");
				printf("[CI AM]   len: %d\n", len);
				printf("[CI AM]   application_type: %d\n", ((unsigned char *)data)[0]);
				printf("[CI AM]   application_manufacturer: %02x %02x\n", ((unsigned char *)data)[2], ((unsigned char *)data)[1]);
				printf("[CI AM]   manufacturer_code: %02x %02x\n", ((unsigned char *)data)[4], ((unsigned char *)data)[3]);
				printf("  menu string: ");
				dl = ((unsigned char *)data)[5];
				if ((dl + 6) > len)
				{
					printf("[CI AM] warning, invalid length (%d vs %d)\n", dl + 6, len);
					dl = len - 6;
				}
				char str[dl + 1];
				memcpy(str, ((char *)data) + 6, dl);
				str[dl] = '\0';
				for (int i = 0; i < dl; ++i)
					printf("%c", ((unsigned char *)data)[i + 6]);
				printf("\n");

				strcpy(slot->name, str);
				if (!strcmp(slot->name, "AlphaCrypt"))
					slot->multi = true;
				printf("[CI AM] set cam name %s on slot(%d)\n", slot->name, slot->slot);
				break;
			}
			default:
				printf("[CI AM] unknown APDU tag 9F 80 %02x\n", tag[2]);
				break;
		}
	}
	return 0;
}

int eDVBCIApplicationManagerSession::doAction()
{
	switch (state)
	{
		case stateStarted:
		{
			const unsigned char tag[3] = {0x9F, 0x80, 0x20}; // application manager info e sendAPDU(tag);
			sendAPDU(tag);
			state = stateFinal;
			checkBlist();
			return 1;
		}
		case stateFinal:
			printf("[CI AM] in final state.\n");
			wantmenu = 0;
			if (wantmenu)
			{
				printf("[CI AM] wantmenu: sending Tenter_menu\n");
				const unsigned char tag[3] = {0x9F, 0x80, 0x22}; // Tenter_menu
				sendAPDU(tag);
				wantmenu = 0;
				return 0;
			}
			else
				return 0;
		default:
			return 0;
	}
}

int eDVBCIApplicationManagerSession::startMMI()
{
	printf("[CI AM] in appmanager -> startmmi()\n");
	const unsigned char tag[3] = {0x9F, 0x80, 0x22}; // Tenter_menu
	sendAPDU(tag);
	slot->mmiOpened = true;
	return 0;
}

bool eDVBCIApplicationManagerSession::readBlist()
{
	int rc, i;
	char cSid[4] = {0, 0, 0, 0};
	u16 Sid;
	FILE *fd;
	char blacklist_file[32];

	sprintf(blacklist_file, "/etc/blacklist_slot_%d", slot->slot);

	if (access(blacklist_file, F_OK) != 0)
		return false;
	fd = fopen(blacklist_file, "r");
	if (!fd)
		return false;
	else
	{
		do
		{
			for (i = 0; i < 4; i++)
			{
				rc = fgetc(fd);
				if (rc == ',' || rc == EOF) break;
				cSid[i] = (char)rc;
			}
			if (rc == EOF) goto fin;
			if (i == 4)
			{
				Sid = (u16)strtol(cSid, NULL, 16);
				slot->bsids.push_back(Sid);
			}
		}
		while (rc != EOF);
fin:
		fclose(fd);
	}
	if (slot->bsids.size())
		return true;
	else
		return false;
}

int eDVBCIApplicationManagerSession::checkBlist()
{
	if (readBlist())
	{
		/* out commented: causes sometimes segfault when reboot....don't know why :(*/
#if yy_debug
		printf("Blacked sids: %d > ", slot->bsids.size());
		for (unsigned int i = 0; i < slot->bsids.size(); i++)
			printf("%04x ", slot->bsids[i]);
		printf("\n");
#endif
	}
	return 0;
}

