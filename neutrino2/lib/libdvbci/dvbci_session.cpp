/* DVB CI Transport Connection */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#ifdef __sh__
#include <stdio.h>
//#endif

#include "dvbci_session.h"
#include "dvbci_resmgr.h"
#include "dvbci_appmgr.h"
#include "dvbci_camgr.h"
#include "dvbci_datetimemgr.h"
#include "dvbci_mmi.h"

#include <neutrinoMessages.h>
#include <driver/rcinput.h>
#include <system/debug.h>


extern CRCInput *g_RCInput;

eDVBCISession* eDVBCISession::sessions[SLMS];

int eDVBCISession::buildLengthField(unsigned char *pkt, int len)
{
	if (len < 127)
	{
		*pkt++=len;
		return 1;
	} 
	else if (len < 256)
	{
		*pkt++=0x81;
		*pkt++=len;
		return 2;
	} 
	else if (len < 65535)
	{
		*pkt++=0x82;
		*pkt++=len>>8;
		*pkt++=len;
		return 3;
	} 
	else
	{
		dprintf(DEBUG_DEBUG, "too big length\n");
		exit(0);
	}
}

int eDVBCISession::parseLengthField(const unsigned char *pkt, int &len)
{
	len = 0;

	if (!(*pkt&0x80)) 
	{
		len = *pkt;

		return 1;
	}

	for (int i = 0; i < (pkt[0]&0x7F); ++i)
	{
		len <<= 8;
		len |= pkt[i + 1];
	}

	return (pkt[0] & 0x7F) + 1;
}

void eDVBCISession::sendAPDU(const unsigned char *tag, const void *data, int len)
{
	unsigned char pkt[len + 3 + 4];
	int l;

	memcpy(pkt, tag, 3);
	l = buildLengthField(pkt + 3, len);
	if (data)
		memcpy(pkt + 3 + l, data, len);

	sendSPDU(0x90, 0, 0, pkt, len + 3 + l);
}

void eDVBCISession::sendSPDU(unsigned char tag, const void *data, int len, const void *apdu, int alen)
{
	sendSPDU(slot, tag, data, len, session_nb, apdu, alen);
}

void eDVBCISession::sendSPDU(tSlot *slot, unsigned char tag, const void *data, int len, unsigned short session_nb, const void *apdu,int alen)
{
	unsigned char pkt[4096];
	unsigned char *ptr=pkt;

	*ptr++=tag;
	ptr+=buildLengthField(ptr, len+2);
	if (data)
		memcpy(ptr, data, len);
	ptr += len;
	*ptr++ = session_nb>>8;
	*ptr++ = session_nb;

	if (apdu)
		memcpy(ptr, apdu, alen);

	ptr += alen;
	//slot->send(pkt, ptr - pkt);
        sendData(slot, pkt, ptr - pkt);
}

void eDVBCISession::sendOpenSessionResponse(tSlot *slot, unsigned char session_status, const unsigned char *resource_identifier, unsigned short session_nb)
{
	char pkt[6];
	pkt[0] = session_status;
	
	printf("sendOpenSessionResponse\n");
	
	memcpy(pkt + 1, resource_identifier, 4);
	sendSPDU(slot, 0x92, pkt, 5, session_nb);
}

void eDVBCISession::recvCreateSessionResponse(const unsigned char *data)
{
	status = data[0];
	state = stateStarted;
	action = 1;
}

void eDVBCISession::recvCloseSessionRequest(const unsigned char *data)
{
	state = stateInDeletion;
	action = 1;
}

void eDVBCISession::deleteSessions(const tSlot *slot)
{
	for (unsigned short session_nb = 0; session_nb < SLMS; ++session_nb)
	{
		if (sessions[session_nb] && sessions[session_nb]->slot == slot)
			sessions[session_nb] = 0;
	}
}

eDVBCISession* eDVBCISession::createSession(tSlot *slot, const unsigned char *resource_identifier, unsigned char &status)
{
	unsigned long tag;
	unsigned short session_nb;

	for (session_nb=1; session_nb < SLMS; ++session_nb)
		if (!sessions[session_nb-1])
			break;
		
	if (session_nb == SLMS)
	{
		status=0xF3;

		return NULL;
	}

	tag = resource_identifier[0] << 24;
	tag|= resource_identifier[1] << 16;
	tag|= resource_identifier[2] << 8;
	tag|= resource_identifier[3];

	switch (tag)
	{
		case 0x00010041:
			sessions[session_nb - 1] = new eDVBCIResourceManagerSession;
			dprintf(DEBUG_DEBUG, "RESOURCE MANAGER\n");
			break;
		case 0x00020041:
			sessions[session_nb - 1] = new eDVBCIApplicationManagerSession(slot);
			dprintf(DEBUG_DEBUG, "APPLICATION MANAGER\n");
			break;
		case 0x00030041:
			sessions[session_nb - 1] = new eDVBCICAManagerSession(slot);
			dprintf(DEBUG_DEBUG, "CA MANAGER\n");
			break;
		case 0x00240041:
			sessions[session_nb - 1] = new eDVBCIDateTimeSession(slot);
			dprintf(DEBUG_DEBUG, "DATE-TIME\n");
			break;
		case 0x00400041:
			sessions[session_nb - 1] = new eDVBCIMMISession(slot);
			dprintf(DEBUG_DEBUG, "MMI - create session\n");
			break;
		case 0x00100041:
	//		session = new eDVBCIAuthSession;
			dprintf(DEBUG_DEBUG, "AuthSession\n");
	//		break;
		case 0x00200041:
		default:
			dprintf(DEBUG_DEBUG, "unknown resource type %02x %02x %02x %02x\n", resource_identifier[0], resource_identifier[1], resource_identifier[2],resource_identifier[3]);
			sessions[session_nb - 1] = 0;
			status = 0xF0;
	}

	if (!sessions[session_nb - 1])
	{
		dprintf(DEBUG_DEBUG, "unknown session.. expect crash\n");
		return NULL;
	}

	dprintf(DEBUG_DEBUG, "new session nb %d %p\n", session_nb, sessions[session_nb - 1]);
	sessions[session_nb - 1]->session_nb = session_nb;

	if (sessions[session_nb - 1])
	{
		sessions[session_nb - 1]->slot = slot;
		status = 0;
	}
	sessions[session_nb - 1]->state = stateInCreation;

        return sessions[session_nb - 1];   
}

void eDVBCISession::handleClose()
{
	unsigned char data[1]={0x00};

	sendSPDU(0x96, data, 1, 0, 0);
}

int eDVBCISession::pollAll()
{
	for (int session_nb = 1; session_nb < SLMS; ++session_nb)
        {
		if (sessions[session_nb-1])
		{
			int r;

			if (sessions[session_nb-1]->state == stateInDeletion)
			{
				sessions[session_nb-1]->handleClose();
				sessions[session_nb-1] = 0;
				r=1;
			} else
				r=sessions[session_nb - 1]->poll();

			if (r)
			{
				dprintf(DEBUG_DEBUG, "%s <\n", __func__);
				return 1;
			}
		}
	}

	return 0;
}

void eDVBCISession::receiveData(tSlot *slot, const unsigned char *ptr, size_t len)
{
	const unsigned char *pkt = (const unsigned char*)ptr;
	unsigned char tag = *pkt++;
	int llen, hlen;
	
	llen = parseLengthField(pkt, hlen);
	pkt += llen;
	
	eDVBCISession* session = NULL;
	
	if(tag == 0x91)
	{
		unsigned char status;

		session = createSession(slot, pkt, status);
		sendOpenSessionResponse(slot, status, pkt, session ? session->session_nb : 0);
		
		if (session)
		{
			session->state=stateStarted;
			session->action=1;
		}
	}
	else
	{
		unsigned session_nb;

		session_nb=pkt[hlen-2]<<8;
		session_nb|=pkt[hlen-1]&0xFF;
		
		if ((!session_nb) || (session_nb >= SLMS))
		{
			dprintf(DEBUG_DEBUG, "PROTOCOL: illegal session number %x\n", session_nb);
			return;
		}
		
		session=sessions[session_nb-1];
		if (!session)
		{
			dprintf(DEBUG_DEBUG, "PROTOCOL: data on closed session %x\n", session_nb);
			return;
		}

		switch (tag)
		{
			case 0x90:
				break;
			case 0x94:
				session->recvCreateSessionResponse(pkt);
				break;
			case 0x95:
				dprintf(DEBUG_DEBUG, "recvCloseSessionRequest");
				session->recvCloseSessionRequest(pkt);
				break;
			default:
				dprintf(DEBUG_DEBUG, "INTERNAL: nyi, tag %02x.\n", tag);
				return;
		}
	}

	hlen += llen + 1; // lengthfield and tag

	pkt = ((const unsigned char*)ptr) + hlen;
	len -= hlen;

	if (session)
	{
		while (len > 0)
		{
			int alen;
			const unsigned char *tag=pkt;
			pkt += 3; // tag
			len -= 3;
			hlen = parseLengthField(pkt, alen);
			pkt += hlen;
			len -= hlen;

			//if (eDVBCIModule::getInstance()->workarounds_active & eDVBCIModule::workaroundMagicAPDULength)
			{
				if (((len-alen) > 0) && ((len - alen) < 3))
				{
					dprintf(DEBUG_DEBUG, "WORKAROUND: applying work around MagicAPDULength\n");
					alen=len;
				}
			}

			if (session->receivedAPDU(tag, pkt, alen))
				session->action = 1;
			pkt+=alen;
			len-=alen;
		}
	}

	if (len)
		printf("PROTOCOL: warning, TL-Data has invalid length\n");
}

eDVBCISession::~eDVBCISession()
{
//	printf("destroy %p", this);
}

