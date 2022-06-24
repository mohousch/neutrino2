/* DVB CI Application Manager */
#include <stdio.h>
#include <string.h>

#include "dvbci_appmgr.h"
#include <system/debug.h>


eDVBCIApplicationManagerSession::eDVBCIApplicationManagerSession(tSlot *tslot)
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

int eDVBCIApplicationManagerSession::receivedAPDU(const unsigned char *tag,const void *data, int len)
{
	if ((tag[0]==0x9f) && (tag[1]==0x80))
	{
		switch (tag[2])
		{
		case 0x21:
		{
			int dl;
			dprintf(DEBUG_DEBUG, "application info:\n");
			dprintf(DEBUG_DEBUG, "  len: %d\n", len);
			dprintf(DEBUG_DEBUG, "  application_type: %d\n", ((unsigned char*)data)[0]);
			dprintf(DEBUG_DEBUG, "  application_manufacturer: %02x %02x\n", ((unsigned char*)data)[2], ((unsigned char*)data)[1]);
			dprintf(DEBUG_DEBUG, "  manufacturer_code: %02x %02x\n", ((unsigned char*)data)[4],((unsigned char*)data)[3]);
			dprintf(DEBUG_DEBUG, "  menu string: \n");

			dl=((unsigned char*)data)[5];
			if ((dl + 6) > len)
			{
				dprintf(DEBUG_DEBUG, "warning, invalid length (%d vs %d)\n", dl+6, len);
				dl=len-6;
			}
			char str[dl + 1];
			memcpy(str, ((char*)data) + 6, dl);
			str[dl] = '\0';

			strcpy(slot->name, str);

			dprintf(DEBUG_DEBUG, "set name %s on slot %d, %p\n", slot->name, slot->slot, slot);
			break;
		}
		default:
			dprintf(DEBUG_DEBUG, "unknown APDU tag 9F 80 %02x\n", tag[2]);
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
			const unsigned char tag[3]={0x9F, 0x80, 0x20}; // application manager info e    sendAPDU(tag);
		    
			sendAPDU(tag);
			state=stateFinal;

			return 1;
		}

		case stateFinal:
			printf("in final state.");
	    
	    		wantmenu = 0;
	    
			if (wantmenu)
			{
				printf("wantmenu: sending Tenter_menu\n");
				const unsigned char tag[3]={0x9F, 0x80, 0x22};  // Tenter_menu
				sendAPDU(tag);
				wantmenu=0;

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
	dprintf(DEBUG_DEBUG, "in appmanager -> startmmi()\n");
	const unsigned char tag[3]={0x9F, 0x80, 0x22};  // Tenter_menu
	sendAPDU(tag);

	slot->mmiOpened = true;

	return 0;
}

