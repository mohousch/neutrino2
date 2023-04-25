#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/dvb/ca.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/types.h>
#include <poll.h>
#include <list>
#include <string>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#include <queue>

#include "ca_ci.h"
//#include "hal_debug.h"
//#include "hardware_caps.h"

#include "dvbci_session.h"
#include "dvbci_appmgr.h"
#include "dvbci_camgr.h"
#include "dvbci_mmi.h"
#include "dvbci_ccmgr.h"

#include <neutrinoMessages.h>
#include <driver/rcinput.h>

/* for some debug > set to 1 */
#define x_debug 1
#define y_debug 0
#define z_debug 0
#define tsb_debug 0
#define wd_debug 0

extern CRCInput *g_RCInput;

static const char *FILENAME = "[ca_ci]";
#if defined (__sh__)
const char ci_path[] = "/dev/dvb/adapter0/ci%d";
ca_slot_info_t info;
#else
const char ci_path[] = "/dev/ci%d";
static int last_source = -1;
#endif
static bool checkLiveSlot = false;
static bool CertChecked = false;
static bool Cert_OK = false;
static uint8_t NullPMT[50] = 
{0x9F, 0x80, 0x32, 0x2E, 0x03, 0x6E, 0xA7, 0x37, 0x00, 0x00, 0x1B, 0x15, 0x7D, 0x00, 0x00, 0x03, 0x15, 0x7E, 0x00, 0x00, 0x03, 0x15, 0x7F, 0x00, 0x00, 0x06, 0x15, 0x80, 0x00, 0x00, 0x06, 0x15, 0x82, 0x00, 0x00, 0x0B, 0x08, 0x7B, 0x00, 0x00, 0x05, 0x09, 0x42, 0x00, 0x00, 0x06, 0x15, 0x81, 0x00, 0x00
};

static cCA *pcCAInstance = NULL;

/* für callback */
/* nur diese Message wird vom CI aus neutrinoMessages.h benutzt */
/* für den CamMsgHandler, darum hier einfach mal definiert */
/* die Feinheiten werden ja in CA_MESSAGE verpackt */
#if defined (__sh__)
uint32_t EVT_CA_MESSAGE = 0x80000000 + 60;
#else
uintptr_t EVT_CA_MESSAGE = 0x80000000 + 60;
#endif

static hal_messenger cam_messenger = NULL;

void hal_register_messenger(hal_messenger messenger)
{
	cam_messenger = messenger;
	return;
}

cCA::cCA(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
}

cCA::~cCA()
{
	printf("%s -> %s\n", FILENAME, __func__);
}

cCA *cCA::GetInstance()
{
	if (pcCAInstance == NULL)
	{
		printf("%s -> %s\n", FILENAME, __FUNCTION__);

		//hw_caps_t *caps = get_hwcaps();
		pcCAInstance = new cCA(4);
	}
	return pcCAInstance;
}

bool cCA::checkQueueSize(eDVBCISlot *slot)
{
	return (slot->sendqueue.size() > 0);
}

/* write ci info file */
void cCA::write_ci_info(int slot, CaIdVector caids)
{
	char buf[255];
	char mname[200];
	char fname[20];
	int count, cx, cy, i;
	snprintf(fname, sizeof(fname), "/tmp/ci-slot%d", slot);
	ModuleName(CA_SLOT_TYPE_CI, slot, mname);
	FILE *fd = fopen(fname, "w");
	if (fd == NULL) return;
	snprintf(buf, sizeof(buf), "%s\n", mname);
	fputs(buf, fd);
	if (caids.size() > 40)
		count = 40;
	else
		count = caids.size();
	cx = snprintf(buf, sizeof(buf), "Anzahl Caids: %d Slot: %d > ", count, slot);
	for (i = 0; i < count; i++)
	{
		cy = snprintf(buf + cx, sizeof(buf) - cx, "%04x ", caids[i]);
		cx += cy;
	}
	snprintf(buf + cx, sizeof(buf) - cx, "\n");
	fputs(buf, fd);
	fclose(fd);
}

void cCA::del_ci_info(int slot)
{
	char fname[20];
	snprintf(fname, sizeof(fname), "/tmp/ci-slot%d", slot);
	if (access(fname, F_OK) == 0) remove(fname);
}

/* helper function to call the cpp thread loop */
void *execute_thread(void *c)
{
	eDVBCISlot *slot = (eDVBCISlot *) c;
	cCA *obj = (cCA *)slot->pClass;
	obj->slot_pollthread(c);
	return NULL;
}

/* from dvb-apps */
int asn_1_decode(uint16_t *length, unsigned char *asn_1_array,
    uint32_t asn_1_array_len)
{
	uint8_t length_field;

	if (asn_1_array_len < 1)
		return -1;
	length_field = asn_1_array[0];

	if (length_field < 0x80)
	{
		// there is only one word
		*length = length_field & 0x7f;
		return 1;
	}
	else if (length_field == 0x81)
	{
		if (asn_1_array_len < 2)
			return -1;

		*length = asn_1_array[1];
		return 2;
	}
	else if (length_field == 0x82)
	{
		if (asn_1_array_len < 3)
			return -1;

		*length = (asn_1_array[1] << 8) | asn_1_array[2];
		return 3;
	}

	return -1;
}

//wait for a while for some data und read it if some
eData waitData(int fd, unsigned char *buffer, int *len)
{
	int retval;
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLOUT | POLLPRI | POLLIN;
	retval = poll(&fds, 1, 200);
	if (retval < 0)
	{
		printf("%s data error\n", FILENAME);
		return eDataError;
	}
	else if (retval == 0)
	{
#if wd_debug
		printf("**** wd DataTimeout\n");
#endif
		return eDataTimeout;
	}
	else if (retval > 0)
	{
		if (fds.revents & POLLIN)
		{
			int n = read(fd, buffer, *len);
			if (n > 0)
			{
				*len = n;
#if wd_debug
				printf("**** wd DataReceived\n");
#endif
				return eDataReady;
			}
			*len = 0;
			printf("%s data error\n", FILENAME);
			return eDataError;
		}
		else if (fds.revents & POLLOUT)
		{
#if wd_debug
			printf("**** wd DataWrite\n");
#endif
			return eDataWrite;
		}
		else if (fds.revents & POLLPRI)
		{
#if wd_debug
			printf("**** wd StatusChanged\n");
#endif
			return eDataStatusChanged;
		}
	}
	return eDataError;
}

static bool transmitData(eDVBCISlot *slot, unsigned char *d, int len)
{
	printf("%s -> %s len(%d)\n", FILENAME, __func__, len);

#if 0 //!defined (__sh__)//BOXMODEL_VUSOLO4K || BOXMODEL_VUDUO4K || BOXMODEL_VUDUO4KSE || BOXMODEL_VUULTIMO4K || BOXMODEL_VUZERO4K
//#if y_debug
//	for (int i = 0; i < len; i++)
//		printf("%02x ", d[i]);
//	printf("\n");
//#endif
	int res = write(slot->fd, d, len);
	printf("send: %d len: %d\n", res, len);

	free(d);
	
	if (res < 0 || res != len)
	{
		printf("error writing data to fd %d, slot %d: %m\n", slot->fd, slot->slot);
		return false;
	}
#else
//#if y_debug
//	for (int i = 0; i < len; i++)
//		printf("%02x ", d[i]);
//	printf("\n");
//#endif
	slot->sendqueue.push(queueData(d, len));
#endif
	return true;
}

//send some data on an fd, for a special slot and connection_id
eData sendData(eDVBCISlot *slot, unsigned char *data, int len)
{
#if defined (__sh__)
	// only poll connection if we are not awaiting an answer
	slot->pollConnection = false;

	//send data_last and data
	if (len < 127)
	{
		unsigned char *d = (unsigned char *) malloc(len + 5);
		memcpy(d + 5, data, len);
		d[0] = slot->slot;
		d[1] = slot->connection_id;
		d[2] = T_DATA_LAST;
		d[3] = len + 1;
		d[4] = slot->connection_id;
		len += 5;
		transmitData(slot, d, len);
	}
	else if (len > 126 && len < 255)
	{
		unsigned char *d = (unsigned char *) malloc(len + 6);
		memcpy(d + 6, data, len);
		d[0] = slot->slot;
		d[1] = slot->connection_id;
		d[2] = T_DATA_LAST;
		d[3] = 0x81;
		d[4] = len + 1;
		d[5] = slot->connection_id;
		len += 6;
		transmitData(slot, d, len);
	}
	else if (len > 254)
	{
		unsigned char *d = (unsigned char *) malloc(len + 7);
		memcpy(d + 7, data, len);
		d[0] = slot->slot;
		d[1] = slot->connection_id;
		d[2] = T_DATA_LAST;
		d[3] = 0x82;
		d[4] = len >> 8;
		d[5] = len + 1;
		d[6] = slot->connection_id;
		len += 7;
		transmitData(slot, d, len);
	}
#else
	unsigned char *d = (unsigned char *) malloc(len);
	memcpy(d, data, len);
	transmitData(slot, d, len);
#endif

	return eDataReady;
}

#if defined (__sh__)
//send a transport connection create request
bool sendCreateTC(eDVBCISlot *slot)
{
	unsigned char data[5];
	data[0] = slot->slot;
	data[1] = slot->slot + 1;   /* conid */
	data[2] = T_CREATE_T_C;
	data[3] = 1;
	data[4] = slot->slot + 1    /* conid */;
	printf("Create TC: ");
	for (int i = 0; i < 5; i++)
		printf("%02x ", data[i]);
	printf("\n");
	write(slot->fd, data, 5);
	return true;
}

static bool sendDataLast(eDVBCISlot *slot)
{
	unsigned char data[5];
	slot->pollConnection = false;
	slot->DataLast = false;
	data[0] = slot->slot;
	data[1] = slot->connection_id;
	data[2] = T_DATA_LAST;
	data[3] = 1;
	data[4] = slot->connection_id;
#if tsb_debug
	printf("*** > Data Last: ");
	for (int i = 0; i < 5; i++)
		printf("%02x ", data[i]);
	printf("\n");
#endif
	write(slot->fd, data, 5);
	return true;
}

static bool sendRCV(eDVBCISlot *slot)
{
	unsigned char send_data[5];
	slot->pollConnection = false;
	slot->DataRCV = false;
	send_data[0] = slot->slot;
	send_data[1] = slot->connection_id;
	send_data[2] = T_RCV;
	send_data[3] = 1;
	send_data[4] = slot->connection_id;
#if tsb_debug
	printf("*** > T_RCV: ");
	for (int i = 0; i < 5; i++)
		printf("%02x ", send_data[i]);
	printf("\n");
#endif
	write(slot->fd, send_data, 5);
	return true;
}

void cCA::process_tpdu(eDVBCISlot *slot, unsigned char tpdu_tag, __u8 *data, int asn_data_length, int /*con_id*/)
{
	switch (tpdu_tag)
	{
		case T_C_T_C_REPLY:
#if x_debug
			printf("%s -> %s > Got CTC Replay (slot %d, con %d)\n", FILENAME, __FUNCTION__, slot->slot, slot->connection_id);
#endif
			/*answer with data last (and if we have with data)
			--> DataLast flag will be generated in next loop from received APDU*/
			break;
		case T_DELETE_T_C:
			//FIXME: close sessions etc; slot->reset ?
			//we must answer here with t_c_replay
			printf("%s %s Got \"Delete Transport Connection\" from module ->currently not handled!\n", FILENAME, __FUNCTION__);
			break;
		case T_D_T_C_REPLY:
			printf("%s %s Got \"Delete Transport Connection Replay\" from module!\n", FILENAME, __FUNCTION__);
			break;
		case T_REQUEST_T_C:
			printf("%s %s Got \"Request Transport Connection\" from Module ->currently not handled!\n", FILENAME, __FUNCTION__);
			break;
		case T_DATA_MORE:
		{
			int new_data_length = slot->receivedLen + asn_data_length;
			printf("%s %s Got \"Data More\" from Module\n", FILENAME, __FUNCTION__);
			__u8 *new_data_buffer = (__u8 *) realloc(slot->receivedData, new_data_length);
			slot->receivedData = new_data_buffer;
			memcpy(slot->receivedData + slot->receivedLen, data, asn_data_length);
			slot->receivedLen = new_data_length;
			break;
		}
		case T_DATA_LAST:
			/* single package */
			if (slot->receivedData == NULL)
			{
				printf("%s -> %s > single package\n", FILENAME, __func__);
#if z_debug
				printf("%s -> calling receiveData with data (len %d)\n", FILENAME, asn_data_length);
				for (int i = 0; i < asn_data_length; i++)
					printf("%02x ", data[i]);
				printf("\n");
#endif
				/* to avoid illegal session number: only if > 0 */
				if (asn_data_length)
				{
					eDVBCISession::receiveData(slot, data, asn_data_length);
					eDVBCISession::pollAll();
				}
			}
			else
			{
				/* chained package
				?? DBO: I never have seen one */
				int new_data_length = slot->receivedLen + asn_data_length;
				printf("%s -> chained data\n", FILENAME);
				__u8 *new_data_buffer = (__u8 *) realloc(slot->receivedData, new_data_length);
				slot->receivedData = new_data_buffer;
				memcpy(slot->receivedData + slot->receivedLen, data, asn_data_length);
				slot->receivedLen = new_data_length;
#if z_debug
				printf("%s -> calling receiveData with data (len %d)\n", FILENAME, asn_data_length);
				for (int i = 0; i < slot->receivedLen; i++)
					printf("%02x ", slot->receivedData[i]);
				printf("\n");
#endif
				eDVBCISession::receiveData(slot, slot->receivedData, slot->receivedLen);
				eDVBCISession::pollAll();

				free(slot->receivedData);
				slot->receivedData = NULL;
				slot->receivedLen = 0;
			}
			break;
		case T_SB:
		{
			if (data[0] & 0x80)
			{
				/* we now wait for an answer so dont poll */
				slot->pollConnection = false;
				/* set the RCV Flag and set DataLast Flag false */
				slot->DataRCV = true;
				slot->DataLast = false;
			}
			else
			{
				/* set DataLast Flag if it is false*/
				if (!slot->DataLast)
				{
					slot->DataLast = true;
#if tsb_debug
					printf("**** > T_SB\n");
#endif
				}
			}
			break;
		}
		default:
			printf("%s unhandled tpdu_tag 0x%0x\n", FILENAME, tpdu_tag);
	}
}
#endif

bool cCA::SendMessage(const CA_MESSAGE *msg)
{
	//hal_debug("%s\n", __func__);
	//if (cam_messenger)
#if defined (__sh__)
		//cam_messenger(EVT_CA_MESSAGE, (uint32_t) msg);
		g_RCInput->postMsg(NeutrinoMessages::EVT_CA_MESSAGE, (uint32_t) msg);
#else
		//cam_messenger(EVT_CA_MESSAGE, (uintptr_t) msg);
		g_RCInput->postMsg(NeutrinoMessages::EVT_CA_MESSAGE, (uintptr_t) msg);
#endif

#if z_debug
	printf("*******Message\n");
	printf("msg: %p\n", msg);
	printf("MSGID: %x\n", msg->MsgId);
	printf("SlotType: %x\n", msg->SlotType);
	printf("Slot: %x\n", msg->Slot);
#endif
	return true;
}

void cCA::MenuEnter(enum CA_SLOT_TYPE, uint32_t bSlotIndex)
{
	printf("%s -> %s Slot(%d)\n", FILENAME, __func__, bSlotIndex);

	std::list<eDVBCISlot *>::iterator it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
#if 0
		if ((strstr((*it)->name, "unknown module") != NULL) && ((*it)->slot == bSlotIndex))
		{
			//the module has no real name, this is the matter if something while initializing went wrong
			//so let this take as a reset action for the module so we do not need to add a reset
			//feature to the neutrino menu
			ModuleReset(SlotType, bSlotIndex);

			return;
		}
#endif
		if ((*it)->slot == bSlotIndex)
		{
			if ((*it)->hasAppManager)
				(*it)->appSession->startMMI();
			break;
		}
	}
}

void cCA::MenuAnswer(enum CA_SLOT_TYPE, uint32_t bSlotIndex, uint32_t choice)
{
	printf("%s -> %s Slot(%d) choice(%d)\n", FILENAME, __func__, bSlotIndex, choice);

	std::list<eDVBCISlot *>::iterator it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == bSlotIndex)
		{
			if ((*it)->hasMMIManager)
				(*it)->mmiSession->answerText((int) choice);
		}
	}
}

void cCA::InputAnswer(enum CA_SLOT_TYPE, uint32_t bSlotIndex, uint8_t *pBuffer, int nLength)
{
	printf("%s -> %s Slot(%d)\n", FILENAME, __func__, bSlotIndex);

	std::list<eDVBCISlot *>::iterator it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == bSlotIndex)
		{
			if ((*it)->hasMMIManager)
				(*it)->mmiSession->answerEnq((char *) pBuffer, nLength);
			break;
		}
	}
}

void cCA::MenuClose(enum CA_SLOT_TYPE, uint32_t bSlotIndex)
{
	printf("%s -> %s Slot(%d)\n", FILENAME, __func__, bSlotIndex);

	std::list<eDVBCISlot *>::iterator it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == bSlotIndex)
		{
			if ((*it)->hasMMIManager)
				(*it)->mmiSession->stopMMI();
			break;
		}
	}
}

uint32_t cCA::GetNumberCISlots(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
	return num_slots;
}

uint32_t cCA::GetNumberSmartCardSlots(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
	return 0;
}

void cCA::ModuleName(enum CA_SLOT_TYPE, uint32_t slot, char *Name)
{
	std::list<eDVBCISlot *>::iterator it;
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == slot)
		{
			strcpy(Name, (*it)->name);
			break;
		}
	}
}

bool cCA::ModulePresent(enum CA_SLOT_TYPE, uint32_t slot)
{
	std::list<eDVBCISlot *>::iterator it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == slot)
		{
			return (*it)->camIsReady;
			break;
		}
	}
	return false;
}

int cCA::GetCAIDS(CaIdVector &Caids)
{
	std::list<eDVBCISlot *>::iterator it;
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->camIsReady)
		{
			for (unsigned int i = 0; i < (*it)->cam_caids.size(); i++)
				Caids.push_back((*it)->cam_caids[i]);
		}
	}
	return 0;
}

bool cCA::StopLiveCI(u64 TP, u16 SID, u8 source, u32 calen)
{
	printf("%s -> %s\n", FILENAME, __func__);
	std::list<eDVBCISlot *>::iterator it;
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*it)->liveUse[j] && (*it)->TP == TP && (*it)->SID[j] == SID && (*it)->source == source && !calen)
			{
				(*it)->SID[j] = 0;
				(*it)->liveUse[j] = false;
				return true;
			}
		}
	}
	return false;
}

bool cCA::StopRecordCI(u64 TP, u16 SID, u8 source, u32 calen)
{
	printf("%s -> %s\n", FILENAME, __func__);
	std::list<eDVBCISlot *>::iterator it;
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*it)->recordUse[j] && (*it)->TP == TP && (*it)->SID[j] == SID && (*it)->source == source && !calen)
			{
				(*it)->SID[j] = 0;
				(*it)->recordUse[j] = false;
				return true;
			}
		}
	}
	return false;
}

SlotIt cCA::FindFreeSlot(u64 TP, u8 source, u16 SID, ca_map_t camap, u8 scrambled)
{
	printf("%s -> %s\n", FILENAME, __func__);
	std::list<eDVBCISlot *>::iterator it;
	ca_map_iterator_t caIt;
	unsigned int i;
	int count = 0;
	int loop_count = 0;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if (!scrambled)
		{
			continue;
		}

		if ((*it)->init)
			count++;
	}

	if (!count || !scrambled)
		return it;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*it)->TP == TP && (*it)->SID[j] == SID && (*it)->source == source)
			{
				(*it)->scrambled = scrambled;
				return it;
			}
		}
	}

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->multi && (*it)->TP == TP && (*it)->source == source && (*it)->ci_use_count < CI_MAX_MULTI)
		{
			(*it)->scrambled = scrambled;
			return it;
		}
	}

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		bool tmpSidBlackListed = false;
		bool recordUse_found = false;
		bool liveUse_found = false;
		int found_count = 0;
		loop_count++;

		if ((*it)->bsids.size())
		{
			for (i = 0; i < (*it)->bsids.size(); i++)
			{
				if ((*it)->bsids[i] == SID)
				{
					tmpSidBlackListed = true;
					break;
				}
			}
			if (i == (*it)->bsids.size())
			{
				(*it)->SidBlackListed = false;
			}
		}

		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*it)->recordUse[j])
				recordUse_found = true;
			if ((*it)->liveUse[j])
			{
				liveUse_found = true;
				found_count = j;
			}
		}

		if ((*it)->camIsReady && (*it)->hasCAManager && (*it)->hasAppManager && !recordUse_found)
		{

			if (tmpSidBlackListed)
			{
				if ((*it)->source == source && (!checkLiveSlot || !liveUse_found))
				{
					SendNullPMT((eDVBCISlot *)(*it));
					(*it)->SidBlackListed = true;
					for (int j = 0; j < CI_MAX_MULTI; j++)
						(*it)->SID[j] = 0;
					(*it)->TP = 0;
					(*it)->scrambled = 0;
					continue;
				}
				else
					continue;
			}

			if (!checkLiveSlot || (!liveUse_found || ((*it)->liveUse[found_count] && (*it)->TP == TP && (*it)->SID[found_count] == SID)))
			{
#if x_debug
				printf("Slot Caids: %d > ", (*it)->cam_caids.size());
				for (i = 0; i < (*it)->cam_caids.size(); i++)
					printf("%04x ", (*it)->cam_caids[i]);
				printf("\n");
#endif
				for (i = 0; i < (*it)->cam_caids.size(); i++)
				{
					caIt = camap.find((*it)->cam_caids[i]);
					if (caIt != camap.end())
					{
						printf("Found: %04x\n", *caIt);
						(*it)->scrambled = scrambled;
						return it;
					}
					else
					{
						//printf("Not Found\n");
						if (loop_count == count)
							(*it)->scrambled = 0;
					}
				}
			}
		}
	}
	return it;
}

/* erstmal den capmt wie er von Neutrino kommt in den Slot puffern */
bool cCA::SendCAPMT(u64 tpid, u8 source, u8 camask, const unsigned char *cabuf, u32 calen, const unsigned char * /*rawpmt*/, u32 /*rawlen*/, enum CA_SLOT_TYPE /*SlotType*/, unsigned char scrambled, ca_map_t cm, int mode, bool enabled)
{
	u16 SID = (u16)(tpid & 0xFFFF);
	u64 TP = tpid >> 16;
	u32 i = 0;
	bool sid_found = false;
	bool recordUse_found = false;
	printf("%s -> %s\n", FILENAME, __func__);
	if (!num_slots) return true;    /* stb's without ci-slots */
#if x_debug
	printf("TP: %llX\n", TP);
	printf("SID: %04X\n", SID);
	printf("SOURCE: %X\n", source);
	printf("CA_MASK: %X\n", camask);
	printf("CALEN: %d\n", calen);
	printf("Scrambled: %d\n", scrambled);
	printf("Mode: %d\n", mode);
	printf("Enabled: %s\n", enabled ? "START" : "STOP");
#endif
	if (scrambled && !enabled)
	{
		if (mode)
		{
			if (StopRecordCI(TP, SID, source, calen))
				printf("Record CI set free\n");
		}
		else
		{
			if (StopLiveCI(TP, SID, source, calen))
				printf("Live CI set free\n");
		}
	}

	if (calen == 0)
		return true;
	SlotIt It = FindFreeSlot(TP, source, SID, cm, scrambled);

	if (It != slot_data.end())
	{
		printf("Slot: %d\n", (*It)->slot);

		SlotIt It2 = GetSlot(!(*It)->slot);
		/* only if 2nd CI is present */
		if (It2 != slot_data.end())
		{
			if (source == (*It2)->source && (*It2)->TP)
			{
				for (int j = 0; j < CI_MAX_MULTI; j++)
				{
					if ((*It2)->recordUse[j])
						recordUse_found = true;
				}

				if (recordUse_found)
				{
					if ((*It)->hasCCManager && (*It2)->hasCCManager)
						(*It)->SidBlackListed = true;
				}
				else
				{
					SendNullPMT((eDVBCISlot *)(*It2));
					(*It2)->scrambled = 0;
					(*It2)->TP = 0;
					for (int j = 0; j < CI_MAX_MULTI; j++)
						(*It2)->SID[j] = 0;
				}
			}
		}
		/* end 2nd CI present */
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*It)->SID[j] == SID)
				sid_found = true;
		}

		if ((*It)->multi && (*It)->TP == TP && (*It)->source == source && !sid_found && (*It)->ci_use_count < CI_MAX_MULTI)
		{
			int pos = 3;

			(*It)->SID[(*It)->ci_use_count] = SID;
			(*It)->ci_use_count++;

			if (!(cabuf[pos] & 0x80))
				pos += 1;
			else
				pos += ((cabuf[pos] & 0x7F) + 1);

			(*It)->pmtlen = calen;
			for (i = 0; i < calen; i++)
				(*It)->pmtdata[i] = cabuf[i];
			(*It)->pmtdata[pos] = 0x04;     // CAPMT_ADD
			(*It)->newCapmt = true;
		}

		else if ((*It)->TP != TP || !sid_found || (*It)->source != source)
		{
			for (int j = 0; j < CI_MAX_MULTI; j++)
				(*It)->SID[j] = 0;
			(*It)->SID[0] = SID;
			(*It)->ci_use_count = 1;
			(*It)->TP = TP;
#if !defined (__sh__)
			if (!checkLiveSlot && mode && (*It)->source != source)
				setInputSource((eDVBCISlot *)(*It), false);
#endif
			(*It)->source = source;
			(*It)->pmtlen = calen;
			for (i = 0; i < calen; i++)
				(*It)->pmtdata[i] = cabuf[i];
			(*It)->newCapmt = true;
		}

#if !defined (__sh__)
		if ((*It)->newCapmt)
			extractPids((eDVBCISlot *)(*It));
#endif
		if ((*It)->scrambled && !(*It)->SidBlackListed)
		{
			for (int j = 0; j < CI_MAX_MULTI; j++)
			{
				if (enabled && (*It)->SID[j] == SID)
				{
					if (mode)
					{
						if (!checkLiveSlot)
							(*It)->liveUse[j] = false;
						(*It)->recordUse[j] = true;
					}
					else if (!(*It)->recordUse[j])
						(*It)->liveUse[j] = true;
				}
			}
		}

		if (!(*It)->newCapmt && (*It)->ccmgr_ready && (*It)->hasCCManager && (*It)->scrambled && !(*It)->SidBlackListed)
			(*It)->ccmgrSession->resendKey((eDVBCISlot *)(*It));

	}
	else
	{
#if !defined (__sh__)
		std::list<eDVBCISlot *>::iterator it;
		recordUse_found = false;
		for (it = slot_data.begin(); it != slot_data.end(); ++it)
		{
			if ((*it)->source == source)
			{
				for (int j = 0; j < CI_MAX_MULTI; j++)
				{
					if ((*it)->recordUse[j])
						recordUse_found = true;
				}
				if (!recordUse_found && (*it)->init)
				{
					setInputSource((eDVBCISlot *)(*it), false);
				}
			}
			if (!(*it)->init)
				last_source = (int)source;
		}
#endif
		printf("No free ci-slot\n");
	}
#if x_debug
	if (!cm.empty())
	{
		printf("Service Caids: ");
		for (ca_map_iterator_t it = cm.begin(); it != cm.end(); ++it)
		{
			printf("%04x ", (*it));
		}
		printf("\n");
	}
	else
	{
		printf("CaMap Empty\n");
	}
#endif
	return true;
}

#if !defined (__sh__)
void cCA::extractPids(eDVBCISlot *slot)
{
	u32 prg_info_len;
	u32 es_info_len = 0;
	u16 pid;
	u8 *data = slot->pmtdata;
	u32 len = slot->pmtlen;
	int pos = 3;

	slot->pids.clear();

	if (!(data[pos] & 0x80))
		pos += 5;
	else
		pos += ((data[pos] & 0x7F) + 5);

	prg_info_len = ((data[pos] << 8) | data[pos + 1]) & 0xFFF;
	pos += prg_info_len + 2;

	for (u32 i = pos; i < len; i += es_info_len + 5)
	{
		pid = (data[i + 1] << 8 | data[i + 2]) & 0x1FFF;
		es_info_len = ((data[i + 3] << 8) | data[i + 4]) & 0xfff;
		slot->pids.push_back(pid);
	}

	if (slot->pids.size())
	{
		slot->newPids = true;
		printf("pids: ");
		for (u32 i = 0; i < slot->pids.size(); i++)
			printf("%04x ", slot->pids[i]);
		printf("\n");
	}
}
#endif

void cCA::setSource(eDVBCISlot *slot)
{
	char buf[64];
	snprintf(buf, 64, "/proc/stb/tsmux/ci%d_input", slot->slot);
	FILE *ci = fopen(buf, "wb");

	if (ci > (void *)0)
	{
		switch (slot->source)
		{
			case TUNER_A:
				fprintf(ci, "A");
				break;
			case TUNER_B:
				fprintf(ci, "B");
				break;
			case TUNER_C:
				fprintf(ci, "C");
				break;
			case TUNER_D:
				fprintf(ci, "D");
				break;
#if BOXMODEL_VUSOLO4K || BOXMODEL_VUDUO4K || BOXMODEL_VUDUO4KSE || BOXMODEL_VUULTIMO4K
			case TUNER_E:
				fprintf(ci, "E");
				break;
			case TUNER_F:
				fprintf(ci, "F");
				break;
			case TUNER_G:
				fprintf(ci, "G");
				break;
			case TUNER_H:
				fprintf(ci, "H");
				break;
			case TUNER_I:
				fprintf(ci, "I");
				break;
			case TUNER_J:
				fprintf(ci, "J");
				break;
			case TUNER_K:
				fprintf(ci, "K");
				break;
			case TUNER_L:
				fprintf(ci, "L");
				break;
			case TUNER_M:
				fprintf(ci, "M");
				break;
			case TUNER_N:
				fprintf(ci, "N");
				break;
			case TUNER_O:
				fprintf(ci, "O");
				break;
			case TUNER_P:
				fprintf(ci, "P");
				break;
#if BOXMODEL_VUULTIMO4K
			case TUNER_Q:
				fprintf(ci, "Q");
				break;
			case TUNER_R:
				fprintf(ci, "R");
				break;
			case TUNER_S:
				fprintf(ci, "S");
				break;
			case TUNER_T:
				fprintf(ci, "T");
				break;
			case TUNER_U:
				fprintf(ci, "U");
				break;
			case TUNER_V:
				fprintf(ci, "V");
				break;
			case TUNER_W:
				fprintf(ci, "W");
				break;
			case TUNER_X:
				fprintf(ci, "X");
				break;
#endif
#endif
		}
		fclose(ci);
	}
}

#if !defined (__sh__)
static std::string getTunerLetter(int number)
{
	return std::string(1, char(65 + number));
}

void cCA::setInputs()
{
	char input[64];
	char choices[64];
	FILE *fd = 0;

#if BOXMODEL_VUULTIMO4K
	for (int number = 0; number < 24; number++) // tuner A to X, input 0 to 23
#else
#if BOXMODEL_VUSOLO4K || BOXMODEL_VUDUO4K || BOXMODEL_VUDUO4KSE || BOXMODEL_VUUNO4KSE || BOXMODEL_VUUNO4K
	for (int number = 0; number < 16; number++) // tuner A to P, input 0 to 15
#else
	for (int number = 0; number < 4; number++) // tuner A to D, input 0 to 3
#endif
#endif
	{
		snprintf(choices, 64, "/proc/stb/tsmux/input%d_choices", number);
		if (access(choices, R_OK) < 0)
		{
			printf("no choices for input%d\n", number);
			continue;
			//break;
		}
		snprintf(input, 64, "/proc/stb/tsmux/input%d", number);
		fd = fopen(input, "wb");
		if (fd)
		{
			printf("set input%d to tuner %s\n", number, getTunerLetter(number).c_str());
			fprintf(fd, "%s", getTunerLetter(number).c_str());
			fclose(fd);
		}
		else
		{
			printf("no input%d\n", number);
		}
	}
}

void cCA::setInputSource(eDVBCISlot *slot, bool ci)
{
	char buf[64];
	printf("%s set input%d to %s%d\n", FILENAME, slot->source, ci ? "ci" : "tuner", ci ? slot->slot : slot->source);
	snprintf(buf, 64, "/proc/stb/tsmux/input%d", slot->source);
	FILE *input = fopen(buf, "wb");

	if (input > (void *)0)
	{
		if (ci)
		{
			switch (slot->slot)
			{
				case 0:
					fprintf(input, "CI0");
					break;
				case 1:
					fprintf(input, "CI1");
					break;
			}
		}
		else
		{
			switch (slot->source)
			{
				case TUNER_A:
					fprintf(input, "A");
					break;
				case TUNER_B:
					fprintf(input, "B");
					break;
				case TUNER_C:
					fprintf(input, "C");
					break;
				case TUNER_D:
					fprintf(input, "D");
					break;
#if BOXMODEL_VUSOLO4K || BOXMODEL_VUDUO4K || BOXMODEL_VUDUO4KSE || BOXMODEL_VUULTIMO4K || BOXMODEL_VUUNO4KSE || BOXMODEL_VUUNO4K
				case TUNER_E:
					fprintf(input, "E");
					break;
				case TUNER_F:
					fprintf(input, "F");
					break;
				case TUNER_G:
					fprintf(input, "G");
					break;
				case TUNER_H:
					fprintf(input, "H");
					break;
				case TUNER_I:
					fprintf(input, "I");
					break;
				case TUNER_J:
					fprintf(input, "J");
					break;
				case TUNER_K:
					fprintf(input, "K");
					break;
				case TUNER_L:
					fprintf(input, "L");
					break;
				case TUNER_M:
					fprintf(input, "M");
					break;
				case TUNER_N:
					fprintf(input, "N");
					break;
				case TUNER_O:
					fprintf(input, "O");
					break;
				case TUNER_P:
					fprintf(input, "P");
					break;
#if BOXMODEL_VUULTIMO4K
				case TUNER_Q:
					fprintf(input, "Q");
					break;
				case TUNER_R:
					fprintf(input, "R");
					break;
				case TUNER_S:
					fprintf(input, "S");
					break;
				case TUNER_T:
					fprintf(input, "T");
					break;
				case TUNER_U:
					fprintf(input, "U");
					break;
				case TUNER_V:
					fprintf(input, "V");
					break;
				case TUNER_W:
					fprintf(input, "W");
					break;
				case TUNER_X:
					fprintf(input, "X");
					break;
#endif
#endif
			}
		}
		fclose(input);
	}
}
#endif

cCA::cCA(int Slots)
{
	printf("%s -> %s %d\n", FILENAME, __func__, Slots);

	num_slots = Slots;
#if !defined (__sh__)
	setInputs();
#endif

	for (int i = 0; i < Slots; i++)
	{
		eDVBCISlot *slot = (eDVBCISlot *) malloc(sizeof(eDVBCISlot));
		slot->slot = i;
		slot->fd = -1;
		slot->connection_id = 0;
		slot->status = eStatusNone;
		slot->receivedLen = 0;
		slot->receivedData = NULL;
		slot->pClass = this;
		slot->pollConnection = false;
		slot->camIsReady = false;
		slot->hasMMIManager = false;
		slot->hasCAManager = false;
		slot->hasCCManager = false;
		slot->ccmgr_ready = false;
		slot->hasDateTime = false;
		slot->hasAppManager = false;
		slot->mmiOpened = false;

		slot->newPids = false;
		slot->newCapmt = false;
		slot->multi = false;
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			slot->SID[j] = 0;
			slot->recordUse[j] = false;
			slot->liveUse[j] = false;
		}
		slot->TP = 0;
		slot->ci_use_count = 0;
		slot->pmtlen = 0;
		slot->source = TUNER_A;
		slot->camask = 0;
		memset(slot->pmtdata, 0, sizeof(slot->pmtdata));

		slot->DataLast = false;
		slot->DataRCV = false;
		slot->SidBlackListed = false;

		slot->counter = 0;
		slot->init = false;
		sprintf(slot->name, "unknown module %d", i);

		slot->private_data = NULL;

		slot_data.push_back(slot);

		sprintf(slot->ci_dev, ci_path, i);
		slot->fd = open(slot->ci_dev, O_RDWR | O_NONBLOCK | O_CLOEXEC);
		if (slot->fd < 0)
		{
			printf("failed to open %s ->%m\n", slot->ci_dev);
		}
#if defined (__sh__)
		/* now reset the slot so the poll pri can happen in the thread */
		if (ioctl(slot->fd, CA_RESET, i) < 0)
			printf("IOCTL CA_RESET failed for slot %p\n", slot);
#else
		ioctl(slot->fd, 0);
#endif
		usleep(200000);
		/* create a thread for each slot */
		if (slot->fd > 0)
		{
			if (pthread_create(&slot->slot_thread, 0, execute_thread, (void *)slot))
			{
				printf("pthread_create");
			}
		}
	}
}

void cCA::ModuleReset(enum CA_SLOT_TYPE, uint32_t slot)
{
	printf("%s -> %s\n", FILENAME, __func__);

	std::list<eDVBCISlot *>::iterator it;
	bool haveFound = false;

	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		if ((*it)->slot == slot)
		{
			haveFound = true;
			break;
		}
	}
	if (haveFound)
	{
		(*it)->status = eStatusReset;
		usleep(200000);
#if !defined (__sh__)
		last_source = (int)(*it)->source;
		setInputSource((eDVBCISlot *)(*it), false);
#endif
		if ((*it)->hasCCManager)
			(*it)->ccmgrSession->ci_ccmgr_doClose((eDVBCISlot *)(*it));
		eDVBCISession::deleteSessions((eDVBCISlot *)(*it));
		(*it)->mmiSession = NULL;
		(*it)->hasMMIManager = false;
		(*it)->hasCAManager = false;
		(*it)->hasCCManager = false;
		(*it)->ccmgr_ready = false;
		(*it)->hasDateTime = false;
		(*it)->hasAppManager = false;
		(*it)->mmiOpened = false;
		(*it)->camIsReady = false;

		(*it)->DataLast = false;
		(*it)->DataRCV = false;
		(*it)->SidBlackListed = false;
		(*it)->bsids.clear();

		(*it)->counter = 0;
		(*it)->init = false;
		(*it)->pollConnection = false;
		sprintf((*it)->name, "unknown module %d", (*it)->slot);
		(*it)->cam_caids.clear();

		(*it)->newPids = false;
		(*it)->newCapmt = false;
		(*it)->multi = false;
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			(*it)->SID[j] = 0;
			(*it)->recordUse[j] = false;
			(*it)->liveUse[j] = false;
		}
		(*it)->TP = 0;
		(*it)->ci_use_count = 0;
		(*it)->pmtlen = 0;
		(*it)->source = TUNER_A;
		(*it)->camask = 0;
		memset((*it)->pmtdata, 0, sizeof((*it)->pmtdata));

		while ((*it)->sendqueue.size())
		{
			delete [](*it)->sendqueue.top().data;
			(*it)->sendqueue.pop();
		}

#if defined (__sh__)
		if (ioctl((*it)->fd, CA_RESET, (*it)->slot) < 0)
			printf("IOCTL CA_RESET failed for slot %d\n", slot);
#else
		ioctl((*it)->fd, 0);
#endif
		usleep(200000);
		(*it)->status = eStatusNone;
	}
}

void cCA::ci_inserted(eDVBCISlot *slot)
{
	printf("1. cam (%d) status changed ->cam now present\n", slot->slot);

	slot->mmiSession = NULL;
	slot->hasMMIManager = false;
	slot->hasCAManager = false;
	slot->hasDateTime = false;
	slot->hasAppManager = false;
	slot->mmiOpened = false;
	slot->init = false;
	sprintf(slot->name, "unknown module %d", slot->slot);
#if defined (__sh__)
	slot->status = eStatusNone;
#else
	slot->status = eStatusWait;
	slot->connection_id = slot->slot + 1;
#endif
	/* Send a message to Neutrino cam_menu handler */
	CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
	memset(pMsg, 0, sizeof(CA_MESSAGE));
	pMsg->MsgId = CA_MESSAGE_MSG_INSERTED;
	pMsg->SlotType = CA_SLOT_TYPE_CI;
	pMsg->Slot = slot->slot;
	SendMessage(pMsg);

	slot->camIsReady = true;
}

void cCA::ci_removed(eDVBCISlot *slot)
{
	printf("cam (%d) status changed ->cam now _not_ present\n", slot->slot);
#if !defined (__sh__)
	last_source = (int)slot->source;
	setInputSource(slot, false);
#endif
	if (slot->hasCCManager)
		slot->ccmgrSession->ci_ccmgr_doClose(slot);
	eDVBCISession::deleteSessions(slot);
	slot->mmiSession = NULL;
	slot->hasMMIManager = false;
	slot->hasCAManager = false;
	slot->hasCCManager = false;
	slot->ccmgr_ready = false;
	slot->hasDateTime = false;
	slot->hasAppManager = false;
	slot->mmiOpened = false;
	slot->init = false;

	slot->DataLast = false;
	slot->DataRCV = false;
	slot->SidBlackListed = false;
	slot->bsids.clear();

	slot->counter = 0;
	slot->pollConnection = false;
	sprintf(slot->name, "unknown module %d", slot->slot);
	slot->status = eStatusNone;
	slot->cam_caids.clear();

	slot->newPids = false;
	slot->newCapmt = false;
	slot->multi = false;
	for (int j = 0; j < CI_MAX_MULTI; j++)
	{
		slot->SID[j] = 0;
		slot->recordUse[j] = false;
		slot->liveUse[j] = false;
	}
	slot->TP = 0;
	slot->ci_use_count = 0;
	slot->pmtlen = 0;
	slot->source = TUNER_A;
	slot->camask = 0;
	memset(slot->pmtdata, 0, sizeof(slot->pmtdata));

	/* delete ci info file */
	del_ci_info(slot->slot);
	/* Send a message to Neutrino cam_menu handler */
	CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
	memset(pMsg, 0, sizeof(CA_MESSAGE));
	pMsg->MsgId = CA_MESSAGE_MSG_REMOVED;
	pMsg->SlotType = CA_SLOT_TYPE_CI;
	pMsg->Slot = slot->slot;
	SendMessage(pMsg);

	while (slot->sendqueue.size())
	{
		delete [] slot->sendqueue.top().data;
		slot->sendqueue.pop();
	}
	slot->camIsReady = false;
	usleep(100000);
}

void cCA::slot_pollthread(void *c)
{
	unsigned char data[1024 * 4];
	eDVBCISlot *slot = (eDVBCISlot *) c;
	bool wait = false;

	while (1)
	{
#if !defined (__sh__)

		int len = 1024 * 4;
		eData status;

		switch (slot->status)
		{
			case eStatusReset:
				while (slot->status == eStatusReset)
				{
					usleep(1000);
				}
				break;
			case eStatusNone:
			{
				status = waitData(slot->fd, data, &len);
				if (status == eDataReady)
				{
					if (!slot->camIsReady)
					{
#if y_debug
// test was kommt
						if (len)
						{
							printf("1. received : > ");
							for (int i = 0; i < len; i++)
								printf("%02x ", data[i]);
							printf("\n");
						}
#endif
						ci_inserted(slot);
						//setInputSource(slot, true);
						goto FROM_FIRST;
					}
				}
				/* slow down the loop, if no CI cam present */
//				printf("***sleep\n");
				sleep(1);
				} /* case statusnone */
			break;
			case eStatusWait:
			{
				status = waitData(slot->fd, data, &len);
FROM_FIRST:
				if (status == eDataReady)
				{
					wait = false;
					slot->pollConnection = false;
					if (len)
					{
						eDVBCISession::receiveData(slot, data, len);
						eDVBCISession::pollAll();
					}
				} /*if data ready */
				else if (status == eDataWrite)
				{
					wait = true;
					/* only writing any data here while status = eDataWrite */
					if (!slot->sendqueue.empty())
					{
						const queueData &qe = slot->sendqueue.top();
						int res = write(slot->fd, qe.data, qe.len);
						if (res >= 0 && (unsigned int)res == qe.len)
						{
							delete [] qe.data;
							slot->sendqueue.pop();
						}
						else
						{
							printf("r = %d, %m\n", res);
						}
					}
				}
				else if (status == eDataStatusChanged)
				{
					if (slot->camIsReady)
					{
						ci_removed(slot);
					}
				}
			}
			break;
			default:
				printf("unknown state %d\n", slot->status);
				break;
		} /* switch(slot->status) */
#else       /* Duckbox */
		int len = 1024 * 4;
		unsigned char *d;
		eData status;

		switch (slot->status)
		{
			case eStatusReset:
				while (slot->status == eStatusReset)
				{
					usleep(1000);
				}
				break;
			case eStatusNone:
			{
				if (slot->camIsReady)
				{
					if (sendCreateTC(slot))
					{
						slot->status = eStatusWait;
					}
					else
					{
						usleep(100000);
					}
				}
				else
				{
					/* wait for pollpri */
					status = waitData(slot->fd, data, &len);
					if (status == eDataStatusChanged)
					{
						info.num = slot->slot;

						if (ioctl(slot->fd, CA_GET_SLOT_INFO, &info) < 0)
							printf("IOCTL CA_GET_SLOT_INFO failed for slot %d\n", slot->slot);

						printf("flags %d %d %d ->slot %d\n", info.flags, CA_CI_MODULE_READY, info.flags & CA_CI_MODULE_READY, slot->slot);

						if (info.flags & CA_CI_MODULE_READY)
						{
							ci_inserted(slot);
						}
					}
				}
				} /* case statusnone */
			break;
			case eStatusWait:
			{
				status = waitData(slot->fd, data, &len);
				if (status == eDataReady)
				{
					slot->pollConnection = false;
					d = data;
#if z_debug
					if ((len == 6 && d[4] == 0x80) || len > 6)
					{
						printf("slot: %d con-id: %d tpdu-tag: %02X len: %d\n", d[0], d[1], d[2], len);
						printf("received data: >");
						for (int i = 0; i < len; i++)
							printf("%02x ", data[i]);
						printf("\n");
					}
#endif
					/* taken from the dvb-apps */
					int data_length = len - 2;
					d += 2; /* remove leading slot and connection id */
					while (data_length > 0)
					{
						unsigned char tpdu_tag = d[0];
						unsigned short asn_data_length;
						int length_field_len;
						if ((length_field_len = asn_1_decode(&asn_data_length, d + 1, data_length - 1)) < 0)
						{
							printf("Received data with invalid asn from module on slot %02x\n", slot->slot);
							break;
						}

						if ((asn_data_length < 1) || (asn_data_length > (data_length - (1 + length_field_len))))
						{
							printf("Received data with invalid length from module on slot %02x\n", slot->slot);
							break;
						}
						slot->connection_id = d[1 + length_field_len];
#if z_debug
						printf("Setting connection_id from received data to %d\n", slot->connection_id);
#endif
						d += 1 + length_field_len + 1;
						data_length -= (1 + length_field_len + 1);
						asn_data_length--;
#if z_debug
						printf("****tpdu_tag: 0x%02X\n", tpdu_tag);
#endif
						process_tpdu(slot, tpdu_tag, d, asn_data_length, slot->connection_id);
						// skip over the consumed data
						d += asn_data_length;
						data_length -= asn_data_length;
					} // while (data_length)
				} /*if data ready */
				else if (status == eDataWrite)
				{
					/* only writing any data here while status = eDataWrite */
					if (!slot->sendqueue.empty())
					{
						const queueData &qe = slot->sendqueue.top();
						int res = write(slot->fd, qe.data, qe.len);
						if (res >= 0 && (unsigned int)res == qe.len)
						{
							delete [] qe.data;
							slot->sendqueue.pop();
						}
						else
						{
							printf("r = %d, %m\n", res);
						}
					}
					/* check for activate the pollConnection */
					if (!checkQueueSize(slot) && (slot->DataRCV || slot->mmiOpened || slot->counter > 5))
					{
						slot->pollConnection = true;
					}
					if (slot->counter < 6)
						slot->counter++;
					else
						slot->counter = 0;
					/* if Flag: send a DataLast */
					if (!checkQueueSize(slot) && slot->pollConnection && slot->DataLast)
					{
						sendDataLast(slot);
					}
					/* if Flag: send a RCV */
					if (!checkQueueSize(slot) && slot->pollConnection && slot->DataRCV)
					{
						sendRCV(slot);
					}
				}
				else if (status == eDataStatusChanged)
				{
					info.num = slot->slot;

					if (ioctl(slot->fd, CA_GET_SLOT_INFO, &info) < 0)
						printf("IOCTL CA_GET_SLOT_INFO failed for slot %d\n", slot->slot);

					printf("flags %d %d %d ->slot %d\n", info.flags, CA_CI_MODULE_READY, info.flags & CA_CI_MODULE_READY, slot->slot);

					if ((slot->camIsReady) && (!(info.flags & CA_CI_MODULE_READY)))
					{
						ci_removed(slot);
					}
				}
			}
			break;
			default:
				printf("unknown state %d\n", slot->status);
				break;
		} /* switch(slot->status) */
#endif      /* end Duckbox */
#if !defined (__sh__)
		if (!slot->init && slot->camIsReady && last_source > -1)
		{
			slot->source = (u8)last_source;
			setInputSource(slot, true);
			last_source = -1;
		}
#endif
		if (slot->hasCAManager && slot->hasAppManager && !slot->init)
		{
			slot->init = true;

			slot->cam_caids = slot->camgrSession->getCAIDs();

			printf("Anzahl Caids: %d Slot: %d > ", slot->cam_caids.size(), slot->slot);
			for (unsigned int i = 0; i < slot->cam_caids.size(); i++)
			{
				printf("%04x ", slot->cam_caids[i]);

			}
			printf("\n");

			/* write ci info file */
			write_ci_info(slot->slot, slot->cam_caids);

			/* Send a message to Neutrino cam_menu handler */
			CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
			memset(pMsg, 0, sizeof(CA_MESSAGE));
			pMsg->MsgId = CA_MESSAGE_MSG_INIT_OK;
			pMsg->SlotType = CA_SLOT_TYPE_CI;
			pMsg->Slot = slot->slot;
			SendMessage(pMsg);
			/* resend a capmt if we have one. this is not very proper but I cant any mechanism in
			neutrino currently. so if a cam is inserted a pmt is not resend */
			/* not necessary: the arrived capmt will be automaticly send */
			//SendCaPMT(slot);
		}
		if (slot->hasCAManager && slot->hasAppManager && slot->newCapmt)
		{
			SendCaPMT(slot);
			slot->newCapmt = false;
			if (slot->ccmgr_ready && slot->hasCCManager && slot->scrambled && !slot->SidBlackListed)
				slot->ccmgrSession->resendKey(slot);
		}
		/* slow down for hd51 to avoid high cpu load */
		if (wait && slot->init && !slot->mmiOpened)
			usleep(300000);
	}
}

bool cCA::SendCaPMT(eDVBCISlot *slot)
{
	printf("%s -> %s\n", FILENAME, __func__);
	if (slot->fd > 0)
	{
#if !defined (__sh__)
		setInputSource(slot, true);
#endif
		setSource(slot);
	}
	if ((slot->fd > 0) && (slot->camIsReady))
	{
		if (slot->hasCAManager)
		{
			printf("buffered capmt(%d): > \n", slot->pmtlen);
#if y_debug
			for (unsigned int i = 0; i < slot->pmtlen; i++)
				printf("%02X ", slot->pmtdata[i]);
			printf("\n");
#endif
			if (slot->pmtlen == 0)
				return true;
			slot->camgrSession->sendSPDU(0x90, 0, 0, slot->pmtdata, slot->pmtlen);
		}
	}
	return true;
}

bool cCA::Init(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
	return true;
}

bool cCA::SendDateTime(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
	return false;
}

bool cCA::Start(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
	return true;
}

void cCA::Stop(void)
{
	printf("%s -> %s\n", FILENAME, __func__);
}

void cCA::Ready(bool p)
{
	printf("%s -> %s param:%d\n", FILENAME, __func__, (int)p);
}

SlotIt cCA::GetSlot(unsigned int slot)
{
	std::list<eDVBCISlot *>::iterator it;
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
		if ((*it)->slot == slot && (*it)->init)
			return it;
	return it;
}

bool cCA::SendNullPMT(eDVBCISlot *slot)
{
	printf("%s > %s >**\n", FILENAME, __func__);
	
	if ((slot->fd > 0) && (slot->camIsReady) && (slot->hasCAManager))
	{
		slot->camgrSession->sendSPDU(0x90, 0, 0, NullPMT, 50);
	}
	
	return true;
}

bool cCA::CheckCerts(void)
{
	if (!CertChecked)
	{
		if (access(ROOT_CERT, F_OK) == 0 && access(ROOT_CERT, F_OK) == 0 && access(ROOT_CERT, F_OK) == 0)
			Cert_OK = true;
		CertChecked = true;
	}
	
	return Cert_OK;
}

bool cCA::checkChannelID(u64 chanID)
{
	std::list<eDVBCISlot *>::iterator it;
	u16 SID = (u16)(chanID & 0xFFFF);
	u64 TP = chanID >> 16;
	
	for (it = slot_data.begin(); it != slot_data.end(); ++it)
	{
		for (int j = 0; j < CI_MAX_MULTI; j++)
		{
			if ((*it)->TP == TP && (*it)->SID[j] == SID && !(*it)->SidBlackListed && (*it)->scrambled)
				return true;
		}
	}
	
	return false;
}

void cCA::setCheckLiveSlot(int check)
{
	if (check)
		checkLiveSlot = true;
	else
		checkLiveSlot = false;
}

void cCA::SetTSClock(u32 Speed, int slot)
{
	/* TODO:
	 * For now using the coolstream values from neutrino cam_menu
	 * where 6 (== 6000000 Hz) means : 'normal'
	 * and other values mean : 'high'
	 * also only ci0 will be changed
	 * for more than one ci slot code must be changed in neutrino cam_menu
	 * and in zapit where ci_clock is set during start.
	 * and here too.
	 * On the other hand: or ci_clock will be set here for all ci slots ????
	 */
	char buf[64];
	snprintf(buf, 64, "/proc/stb/tsmux/ci%d_tsclk", slot);
	FILE *ci = fopen(buf, "wb");
	printf("%s -> %s to: %s\n", FILENAME, __func__, Speed > 9 * 1000000 ? "extra_high" : Speed > 6 * 1000000 ? "high" : "normal");
	if (ci)
	{
		if (Speed > 9 * 1000000)
			fprintf(ci, "extra_high");
		else if (Speed > 6 * 1000000)
			fprintf(ci, "high");
		else
			fprintf(ci, "normal");
		fclose(ci);
	}
}

void cCA::SetCIDelay(int Delay)
{
	char buf[64];
	snprintf(buf, 64, "/proc/stb/tsmux/rmx_delay");
	FILE *ci = fopen(buf, "wb");
	printf("%s -> %s to: %i\n", FILENAME, __func__, Delay);
	if (ci)
	{
		fprintf(ci, "%i", Delay);
		fclose(ci);
	}
}

void cCA::SetCIRelevantPidsRouting(int RPR, int slot)
{
	char buf[64];
	snprintf(buf, 64, "/proc/stb/tsmux/ci%d_relevant_pids_routing", slot);
	FILE *ci = fopen(buf, "wb");
	printf("%s -> %s to: %i\n", FILENAME, __func__, RPR);
	if (ci)
	{
		fprintf(ci, "%s", RPR == 1 ? "yes" : "no");
		fclose(ci);
	}
}

