#ifndef __dvbci_dvbci_tc_h
#define __dvbci_dvbci_tc_h

#include "dvb-ci.h"

#define SLMS	256

class eDVBCISession
{
	static eDVBCISession* sessions[SLMS];
	static eDVBCISession* createSession(tSlot *slot, const unsigned char *resource_identifier, unsigned char &status);
	static void sendSPDU(tSlot *slot, unsigned char tag,const void *data, int len, unsigned short session_nb, const void *apdu=0,int alen=0);
	static void sendOpenSessionResponse(tSlot *slot,unsigned char session_status, const unsigned char *resource_identifier,unsigned short session_nb);
	void recvCreateSessionResponse(const unsigned char *data);
	void recvCloseSessionRequest(const unsigned char *data);
protected:
	int state;
	int status;
	int action;
	tSlot *slot;		//base only
	unsigned short session_nb;
	virtual int receivedAPDU(const unsigned char *tag, const void *data, int len) = 0;
	void sendAPDU(const unsigned char *tag, const void *data=0,int len=0);
	virtual int doAction()=0;
	void handleClose();
public:
	virtual ~eDVBCISession();

	static void deleteSessions(const tSlot *slot);
	void sendSPDU(unsigned char tag, const void *data, int len,const void *apdu=0, int alen=0);

	int poll() { if (action) { action=doAction(); return 1; } return 0; }
	enum { stateInCreation, stateBusy, stateInDeletion, stateStarted, statePrivate};
	
	static int parseLengthField(const unsigned char *pkt, int &len);
	static int buildLengthField(unsigned char *pkt, int len);

	static void receiveData(tSlot *slot, const unsigned char *ptr, size_t len);
	
	int getState() { return state; }
	int getStatus() { return status; }
	
	static int pollAll();
	
};

#endif
