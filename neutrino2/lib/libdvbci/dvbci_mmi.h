#ifndef __dvbci_dvbci_mmi_h
#define __dvbci_dvbci_mmi_h

#include "dvbci_session.h"

class eDVBCIMMISession: public eDVBCISession
{
	enum {
		stateDisplayReply=statePrivate, stateFakeOK, stateIdle
	};
	
	int receivedAPDU(const unsigned char *tag, const void *data, int len);
	int doAction();
	tSlot *slot;
public:
	eDVBCIMMISession(tSlot *tslot);
	~eDVBCIMMISession();
	int stopMMI();
	int answerText(int answer);
	int answerEnq(char *answer, int len);
	int cancelEnq();
};

#endif
