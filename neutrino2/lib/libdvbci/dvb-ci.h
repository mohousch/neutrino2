/*
	$Id: dvb-ci.h 25.11.2023 mohousch Exp $
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

#ifndef __DVBCI_H
#define __DVBCI_H

#include <asm/types.h>

#include <pthread.h>

#include <queue>

#include <list>

#include <zapit/cam.h>


#define MAX_MMI_ITEMS 20
#define MAX_MMI_TEXT_LEN 255
#define MAX_MMI_CHOICE_TEXT_LEN 255

#define TUNER_A		0
#define TUNER_B		1
#define TUNER_C		2
#define TUNER_D		3

#define MAX_SLOTS	2
#define DVBADAPTER_MAX	2

typedef struct
{
        int slot;
        int choice_nb;
        char title[MAX_MMI_TEXT_LEN];
        char subtitle[MAX_MMI_TEXT_LEN];
        char bottom[MAX_MMI_TEXT_LEN];
        char choice_item[MAX_MMI_ITEMS][MAX_MMI_CHOICE_TEXT_LEN];
} MMI_MENU_LIST_INFO;

typedef struct
{
        int slot;
        int blind;
        int answerlen;
        char enguiryText[MAX_MMI_TEXT_LEN];

} MMI_ENGUIRY_INFO;

// constants taken from dvb-apps 
#define T_SB                0x80	// sb                           primitive   h<--m
#define T_RCV               0x81	// receive                      primitive   h-->m
#define T_CREATE_T_C        0x82	// create transport connection  primitive   h-->m
#define T_C_T_C_REPLY       0x83	// ctc reply                    primitive   h<--m
#define T_DELETE_T_C        0x84	// delete tc                    primitive   h<->m
#define T_D_T_C_REPLY       0x85	// dtc reply                    primitive   h<->m
#define T_REQUEST_T_C       0x86	// request transport connection primitive   h<--m
#define T_NEW_T_C           0x87	// new tc / reply to t_request  primitive   h-->m
#define T_T_C_ERROR         0x77	// error creating tc            primitive   h-->m
#define T_DATA_LAST         0xA0	// convey data from higher      constructed h<->m
#define T_DATA_MORE         0xA1	// convey data from higher      constructed h<->m

typedef enum {
	eDataTimeout, 
	eDataError, 
	eDataReady, 
	eDataWrite, 
	eDataStatusChanged
} eData;

typedef enum {
	eStatusNone, 
	eStatusWait
} eStatus;

struct queueData
{
	__u8 prio;
	unsigned char *data;
	unsigned int len;
	queueData( unsigned char *_data, unsigned int _len, __u8 _prio = 0 )
		:prio(_prio), data(_data), len(_len)
	{

	}
	bool operator < ( const struct queueData &a ) const
	{
		return prio < a.prio;
	}
};

class eDVBCIMMISession;
class eDVBCIApplicationManagerSession;
class eDVBCICAManagerSession;

typedef struct
{
        pthread_t   slot_thread;
	int          slot;
	int          fd;
	int          connection_id;
        eStatus     status;  

        int          receivedLen;
	unsigned char * receivedData;
	
	void*        pClass;
        
	bool        pollConnection;
	bool        camIsReady;
	
	eDVBCIMMISession* mmiSession;
	eDVBCIApplicationManagerSession* appSession;
	eDVBCICAManagerSession* camgrSession;
	
	bool	    hasAppManager;
	bool        hasMMIManager;
	bool        hasCAManager;
	bool        hasDateTime;
	
	bool        mmiOpened;
	char        name[512];
        bool        init;
 
	std::priority_queue<queueData> sendqueue;

        CCaPmt      *caPmt;
	int source;
} tSlot;

eData sendData(tSlot *slot, unsigned char* data, int len);

class cDvbCi 
{
	private:
		int slots;
		int ci_num;

	        std::list<tSlot*> slot_data;
                pthread_t slot_thread;
                
		//
                void setSource(int slot, int source);
                bool checkQueueSize(tSlot* slot);
                void process_tpdu(tSlot* slot, unsigned char tpdu_tag, __u8* data, int asn_data_length, int con_id);
 
 	public:
		////
		cDvbCi(int Slots);
		~cDvbCi();
		static cDvbCi * getInstance();
		
		//
		void slot_pollthread(void *c);

		//
		bool CamPresent(int slot);
		bool GetName(int slot, char * name);
                void CI_MenuAnswer(unsigned char bSlotIndex,unsigned char choice);
                void CI_Answer(unsigned char bSlotIndex,unsigned char *pBuffer,unsigned char nLength);
                void CI_CloseMMI(unsigned char bSlotIndex);
                void CI_EnterMenu(unsigned char bSlotIndex);
                void reset(int slot);
                bool SendCaPMT(CCaPmt *caPmt, int source = TUNER_A);
                int getCINum(void){return ci_num;};
                void SetTSClock(uint32_t Speed, int slot);
};

#endif //__DVBCI_H

