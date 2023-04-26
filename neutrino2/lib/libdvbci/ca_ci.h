#ifndef __CA_CI_H__
#define __CA_CI_H__

#include <config.h>

#include <stdint.h>
#include <asm/types.h>
#include <pthread.h>
#include <list>
#include <queue>
#include <vector>
#include <set>

#include "mmi.h"
#include "cs_types.h"


/* constants taken from dvb-apps */
#define T_SB                0x80    // sb                           primitive   h<--m
#define T_RCV               0x81    // receive                      primitive   h-->m
#define T_CREATE_T_C        0x82    // create transport connection  primitive   h-->m
#define T_C_T_C_REPLY       0x83    // ctc reply                    primitive   h<--m
#define T_DELETE_T_C        0x84    // delete tc                    primitive   h<->m
#define T_D_T_C_REPLY       0x85    // dtc reply                    primitive   h<->m
#define T_REQUEST_T_C       0x86    // request transport connection primitive   h<--m
#define T_NEW_T_C           0x87    // new tc / reply to t_request  primitive   h-->m
#define T_T_C_ERROR         0x77    // error creating tc            primitive   h-->m
#define T_DATA_LAST         0xA0    // convey data from higher      constructed h<->m
#define T_DATA_MORE         0xA1    // convey data from higher      constructed h<->m

/* max multi decrypt per ci-cam */
#define CI_MAX_MULTI            5

enum CA_SLOT_TYPE
{
	CA_SLOT_TYPE_SMARTCARD,
	CA_SLOT_TYPE_CI,
	CA_SLOT_TYPE_ALL
};

enum CA_MESSAGE_FLAGS
{
	CA_MESSAGE_EMPTY            = (1 << 0),
	CA_MESSAGE_HAS_PARAM1_DATA  = (1 << 1), /// Free after use!
	CA_MESSAGE_HAS_PARAM1_INT   = (1 << 2),
	CA_MESSAGE_HAS_PARAM1_PTR   = (1 << 3),
	CA_MESSAGE_HAS_PARAM2_INT   = (1 << 4),
	CA_MESSAGE_HAS_PARAM2_PTR   = (1 << 5),
	CA_MESSAGE_HAS_PARAM2_DATA  = (1 << 6),
	CA_MESSAGE_HAS_PARAM3_DATA  = (1 << 7), /// Free after use!
	CA_MESSAGE_HAS_PARAM3_INT   = (1 << 8),
	CA_MESSAGE_HAS_PARAM3_PTR   = (1 << 9),
	CA_MESSAGE_HAS_PARAM4_INT   = (1 << 10),
	CA_MESSAGE_HAS_PARAM4_PTR   = (1 << 11),
	CA_MESSAGE_HAS_PARAM4_DATA  = (1 << 12),
	CA_MESSAGE_HAS_PARAM5_INT   = (1 << 13),
	CA_MESSAGE_HAS_PARAM5_PTR   = (1 << 14),
	CA_MESSAGE_HAS_PARAM5_DATA  = (1 << 15),
	CA_MESSAGE_HAS_PARAM6_INT   = (1 << 16),
	CA_MESSAGE_HAS_PARAM6_PTR   = (1 << 17),
	CA_MESSAGE_HAS_PARAM6_DATA  = (1 << 18),
	CA_MESSAGE_HAS_PARAM1_LONG  = (1 << 19),
	CA_MESSAGE_HAS_PARAM2_LONG  = (1 << 20),
	CA_MESSAGE_HAS_PARAM3_LONG  = (1 << 21),
	CA_MESSAGE_HAS_PARAM4_LONG  = (1 << 22)
};

enum CA_MESSAGE_MSGID
{
	CA_MESSAGE_MSG_INSERTED,
	CA_MESSAGE_MSG_REMOVED,
	CA_MESSAGE_MSG_INIT_OK,
	CA_MESSAGE_MSG_INIT_FAILED,
	CA_MESSAGE_MSG_MMI_MENU,
	CA_MESSAGE_MSG_MMI_MENU_ENTER,
	CA_MESSAGE_MSG_MMI_MENU_ANSWER,
	CA_MESSAGE_MSG_MMI_LIST,
	CA_MESSAGE_MSG_MMI_TEXT,
	CA_MESSAGE_MSG_MMI_REQ_INPUT,
	CA_MESSAGE_MSG_MMI_CLOSE,
	CA_MESSAGE_MSG_INTERNAL,
	CA_MESSAGE_MSG_PMT_ARRIVED,
	CA_MESSAGE_MSG_CAPMT_ARRIVED,
	CA_MESSAGE_MSG_CAT_ARRIVED,
	CA_MESSAGE_MSG_ECM_ARRIVED,
	CA_MESSAGE_MSG_EMM_ARRIVED,
	CA_MESSAGE_MSG_CHANNEL_CHANGE,
	CA_MESSAGE_MSG_GUI_READY,
	CA_MESSAGE_MSG_EXIT
};

typedef struct CA_MESSAGE
{
	uint32_t MsgId;
	enum CA_SLOT_TYPE SlotType;
	int Slot;
	uint32_t Flags;
	union
	{
		uint8_t *Data[4];
		uint32_t Param[4];
		void *Ptr[4];
		uint64_t ParamLong[4];
	} Msg;
} CA_MESSAGE;

typedef std::set<int> ca_map_t;
typedef ca_map_t::iterator ca_map_iterator_t;

typedef std::vector<u16>            		bSIDVector;
typedef std::vector<u16>            		CaIdVector;
typedef std::vector<u16>::iterator      	CaIdVectorIterator;
typedef std::vector<u16>::const_iterator	CaIdVectorConstIterator;

#define CA_MESSAGE_SIZE     	sizeof(CA_MESSAGE)
#define CA_MESSAGE_ENTRIES  	256
#define CA_MESSAGE_ENTRIES_CI   128
#define CA_MESSAGE_ENTRIES_SC   64

#ifndef CS_CA_PDATA
#define CS_CA_PDATA     	void
#endif

typedef enum
{
	TUNER_A,
	TUNER_B,
	TUNER_C,
	TUNER_D
#if BOXMODEL_VUSOLO4K || BOXMODEL_VUDUO4K || BOXMODEL_VUDUO4KSE || BOXMODEL_VUULTIMO4K || BOXMODEL_VUUNO4KSE || BOXMODEL_VUUNO4K
	, TUNER_E
	, TUNER_F
	, TUNER_G
	, TUNER_H
	, TUNER_I
	, TUNER_J
	, TUNER_K
	, TUNER_L
	, TUNER_M
	, TUNER_N
	, TUNER_O
	, TUNER_P
#if BOXMODEL_VUULTIMO4K
	, TUNER_Q
	, TUNER_R
	, TUNER_S
	, TUNER_T
	, TUNER_U
	, TUNER_V
	, TUNER_W
	, TUNER_X
#endif
#endif
} source_t;

typedef enum
{
	eDataTimeout,
	eDataError,
	eDataReady,
	eDataWrite,
	eDataStatusChanged
} eData;

typedef enum
{
	eStatusNone,
	eStatusWait,
	eStatusReset
} eStatus;

struct queueData
{
	__u8 prio;
	unsigned char *data;
	unsigned int len;
	queueData(unsigned char *_data, unsigned int _len, __u8 _prio = 0)
		: prio(_prio), data(_data), len(_len)
	{

	}
	bool operator < (const struct queueData &a) const
	{
		return prio < a.prio;
	}
};

class eDVBCIMMISession;
class eDVBCIApplicationManagerSession;
class eDVBCICAManagerSession;
class eDVBCIContentControlManagerSession;

typedef struct
{
	pthread_t slot_thread;
	unsigned int slot;
	int fd;
	int connection_id;
	eStatus status;

	int receivedLen;
	unsigned char *receivedData;

	void *pClass;

	bool pollConnection;
	bool camIsReady;

	eDVBCIMMISession *mmiSession;
	eDVBCIApplicationManagerSession *appSession;
	eDVBCICAManagerSession *camgrSession;
	eDVBCIContentControlManagerSession *ccmgrSession;

	bool hasAppManager;
	bool hasMMIManager;
	bool hasCAManager;
	bool hasCCManager;
	bool hasDateTime;
	bool mmiOpened;
	bool init;

	bool ccmgr_ready;

	char ci_dev[32];
	char name[512];

	bool newPids;
	bool newCapmt;
	bool multi;
	bool recordUse[CI_MAX_MULTI];
	bool liveUse[CI_MAX_MULTI];
	u16 SID[CI_MAX_MULTI];
	u64 TP;
	int ci_use_count;
	u32 pmtlen;
	u8 source;
	u8 camask;
	unsigned char pmtdata[1024 * 4];

	int counter;
	CaIdVector cam_caids;
	std::priority_queue<queueData> sendqueue;

	std::vector<u16> pids;

	bSIDVector bsids;
	unsigned char lastKey[32];
	unsigned char scrambled;
	u8 lastParity;
	bool DataLast;
	bool DataRCV;
	bool SidBlackListed;
	/* private data */
	void *private_data;

} eDVBCISlot;

eData sendData(eDVBCISlot *slot, unsigned char *data, int len);

typedef std::list<eDVBCISlot *>::iterator SlotIt;

/// CA module class
class cCA
{
	private:
		/// Static instance of the CA module
		//static cCA *inst;
		/// Private constructor (singleton method)
		cCA(void);
		/// Private data for the CA module
		CS_CA_PDATA *privateData;
		/// set inputs with tuner letter in tsmux
		void setInputs();
		/// write ci info file to /tmp
		void write_ci_info(int slot, CaIdVector caids);
		/// delete ci info file
		void del_ci_info(int slot);
		/// extract audio / video pids from capmt
		void extractPids(eDVBCISlot *slot);
		/// ci module is detected
		void ci_inserted(eDVBCISlot *slot);
		/// ci module is removed
		void ci_removed(eDVBCISlot *slot);
		/// decode the tpdu tag
		void process_tpdu(eDVBCISlot *slot, unsigned char tpdu_tag, __u8 *data, int asn_data_length, int con_id);
		/// set flag of running ci record to false
		bool StopRecordCI(u64 TP, u16 SID, u8 source, u32 calen);
		/// set flag of running ci live-tv to false
		bool StopLiveCI(u64 TP, u16 SID, u8 source, u32 calen);
		/// find an unused ci slot for use with service
		SlotIt FindFreeSlot(u64 tpid, u8 source, u16 sid, ca_map_t camap, u8 scrambled);
		/// get slot iterator by slot number
		SlotIt GetSlot(unsigned int slot);
		/// send buffered capmt to ci modul
		bool SendCaPMT(eDVBCISlot *slot);
		/// send a dummy capmt to ci for deactivating
		bool SendNullPMT(eDVBCISlot *slot);
		/// set ci source
		void setSource(eDVBCISlot *slot);
		/// set demux source
		void setInputSource(eDVBCISlot *slot, bool ci);
		/// check if data in queue
		bool checkQueueSize(eDVBCISlot *slot);
		int num_slots;
		bool init;
		void SendPMT();
		pthread_mutex_t ciMutex;
		std::list<eDVBCISlot *> slot_data;
		pthread_t slot_thread;

	public:
		/// sh4 unused
		bool Init(void);
		/// init ci and start the loop
		cCA(int Slots);
		/// Returns the number of CI slots
		uint32_t GetNumberCISlots(void);
		/// Returns the number of Smartcard slots
		uint32_t GetNumberSmartCardSlots(void);
		/// Singleton
		static cCA *GetInstance(void);
		/// Send PMT to a individual or to all available modules (DEPRECATED)
		bool SendPMT(int Unit, unsigned char *Data, int Len, enum CA_SLOT_TYPE SlotType = CA_SLOT_TYPE_ALL);
		/// Sends a message to the CA thread
		bool SendMessage(const CA_MESSAGE *Msg);
		/// Sets the frequency (in Hz) of the TS stream input (only valid for CI)
		/// sh4 unused
		void SetTSClock(u32 Speed, int slot = 0);
		/// dvb wait delay for ci response
		void SetCIDelay(int Delay);
		/// relevant pids routing
		void SetCIRelevantPidsRouting(int RPR, int slot = 0);
		/// Start the CA module
		/// sh4 unused
		bool Start(void);
		/// Stops the CA module
		/// sh4 unused
		void Stop(void);
		/// Notify that the GUI is ready to receive messages
		/// (CA messages coming from a module)
		/// sh4 unused
		void Ready(bool Set);
		/// Resets a module (if possible)
		/// sh4 unused
		void ModuleReset(enum CA_SLOT_TYPE, uint32_t Slot);
		/// Checks if a module is present
		bool ModulePresent(enum CA_SLOT_TYPE, uint32_t Slot);
		/// Returns the module name in array Name
		void ModuleName(enum CA_SLOT_TYPE, uint32_t Slot, char *Name);
		/// Notify the module we want to enter menu
		void MenuEnter(enum CA_SLOT_TYPE, uint32_t Slot);
		/// Notify the module with our answer (choice nr)
		void MenuAnswer(enum CA_SLOT_TYPE, uint32_t Slot, uint32_t choice);
		/// Notify the module with our answer (binary)
		void InputAnswer(enum CA_SLOT_TYPE, uint32_t Slot, uint8_t *Data, int Len);
		/// Notify the module we closed the menu
		void MenuClose(enum CA_SLOT_TYPE, uint32_t Slot);
		/// Get the supported CAIDs
		int GetCAIDS(CaIdVector &Caids);
		/// Send a CA-PMT object and Raw unparsed PMT to the CA layer
		bool SendCAPMT(u64 /*Source*/, u8 /*DemuxSource*/, u8 /*DemuxMask*/, const unsigned char * /*CAPMT*/, u32 /*CAPMTLen*/, const unsigned char * /*RawPMT*/, u32 /*RawPMTLen*/, enum CA_SLOT_TYPE SlotType = CA_SLOT_TYPE_ALL,
		    unsigned char scrambled = 0, ca_map_t camap = std::set<int>(), int mode = 0, bool enabled = false);
		/// sh4 unused
		bool SendDateTime(void);
		/// the main loop
		void slot_pollthread(void *c);
		/// check if current channel uses any ci module
		bool checkChannelID(u64 chanID);
		/// set checking for live-tv use ci to true
		void setCheckLiveSlot(int check);
		/// as the name says
		bool CheckCerts(void);
		/// Virtual destructor
		virtual ~cCA();
};

#endif // __CA_CI_H__

