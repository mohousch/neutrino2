/* DVB CI MMI */
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

#include "dvbci_mmi.h"
#include "ca_ci.h"


eDVBCIMMISession::eDVBCIMMISession(eDVBCISlot *tslot)
{
	slot = tslot;
	slot->hasMMIManager = true;
	slot->mmiSession = this;
}

eDVBCIMMISession::~eDVBCIMMISession()
{
	/* Send a message to Neutrino cam_menu handler */
	CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
	memset(pMsg, 0, sizeof(CA_MESSAGE));

	pMsg->MsgId = CA_MESSAGE_MSG_MMI_CLOSE;
	pMsg->SlotType = CA_SLOT_TYPE_CI;
	pMsg->Slot = slot->slot;
	cCA::GetInstance()->SendMessage(pMsg);

	slot->hasMMIManager = false;
	slot->mmiSession = NULL;
	slot->mmiOpened = false;
}

int eDVBCIMMISession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	printf("[CI MMI] SESSION(%d)/MMI %02x %02x %02x: ", session_nb, tag[0], tag[1], tag[2]);
	for (int i = 0; i < len; i++)
		printf("%02x ", ((const unsigned char *)data)[i]);
	printf("\n");

	if ((tag[0] == 0x9f) && (tag[1] == 0x88))
	{
		/* from e2 mmi_ui.cpp */
		switch (tag[2])
		{
			case 0x00: /* close */
			{
				/* Send a message to Neutrino cam_menu handler */
				CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
				memset(pMsg, 0, sizeof(CA_MESSAGE));

				pMsg->MsgId = CA_MESSAGE_MSG_MMI_CLOSE;
				pMsg->SlotType = CA_SLOT_TYPE_CI;
				pMsg->Slot = slot->slot;
				stopMMI();
				cCA::GetInstance()->SendMessage(pMsg);
			}
			break;
			case 0x01: /* display control */
				state = stateDisplayReply;
				return 1;
				break;
			case 0x07: /* menu enq */
			{
				MMI_ENQUIRY_INFO *enquiry = (MMI_ENQUIRY_INFO *) malloc(sizeof(MMI_ENQUIRY_INFO));
				memset(enquiry, 0, sizeof(MMI_ENQUIRY_INFO));
				unsigned char *d = (unsigned char *)data;
				unsigned char *max = ((unsigned char *)d) + len;

				int textlen = len - 2;
				if ((d + 2) > max)
					break;

				int blind = *d++ & 1;
				int alen = *d++;
				printf("%d bytes text\n", textlen);
				if ((d + textlen) > max)
					break;

				char str[textlen + 1];
				memcpy(str, ((char *)d), textlen);
				str[textlen] = '\0';
				printf("enq-text: %s", str);
				enquiry->slot = slot->slot;
				enquiry->blind = blind;
				enquiry->answerlen = alen;
				strcpy(enquiry->enquiryText, str);

				/* Send a message to Neutrino cam_menu handler */
				CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
				memset(pMsg, 0, sizeof(CA_MESSAGE));

				pMsg->MsgId = CA_MESSAGE_MSG_MMI_REQ_INPUT;
				pMsg->SlotType = CA_SLOT_TYPE_CI;
				pMsg->Slot = slot->slot;
				pMsg->Flags = CA_MESSAGE_HAS_PARAM1_DATA;
				pMsg->Msg.Data[0] = (uint8_t *)enquiry;
				cCA::GetInstance()->SendMessage(pMsg);

				slot->mmiOpened = true;
			}
			break;
			case 0x09: /* menu last */
			case 0x0c: /* list last */
			{
				MMI_MENU_LIST_INFO *listInfo = (MMI_MENU_LIST_INFO *) malloc(sizeof(MMI_MENU_LIST_INFO));
				memset(listInfo, 0, sizeof(MMI_MENU_LIST_INFO));

				listInfo->slot = slot->slot;
				listInfo->choice_nb = 0;

				unsigned char *d = (unsigned char *)data;
				unsigned char *max = ((unsigned char *)d) + len;
				int pos = 0;

				if (tag[2] == 0x09)
					printf("menu_last\n");
				else
					printf("list_last\n");

				if (d > max)
					break;

				int n = *d++;

				if (n == 0xFF)
					n = 0;
				else
					n++;

				for (int i = 0; i < (n + 3); ++i)
				{
					int textlen;
					if ((d + 3) > max)
						break;

					//printf("text tag: %02x %02x %02x\n", d[0], d[1], d[2]);
					d += 3;
					d += eDVBCISession::parseLengthField(d, textlen);

					printf("%d bytes text > ", textlen);
					if ((d + textlen) > max)
						break;

					char str[textlen + 1];
					memcpy(str, ((char *)d), textlen);
					str[textlen] = '\0';

					int type = pos++;

					if (type == 0) /* title */
						strcpy(listInfo->title, str);
					else if (type == 1) /* subtitle */
						strcpy(listInfo->subtitle, str);
					else if (type == 2) /* bottom */
						strcpy(listInfo->bottom, str);
					else /* text */
					{
						strcpy(listInfo->choice_item[listInfo->choice_nb], str);
						listInfo->choice_nb++;
						printf("%d. ", listInfo->choice_nb);
						//printf("%s\n", listInfo->choice_item[listInfo->choice_nb - 1]);
					}
					while (textlen--)
						printf("%c", *d++);
					printf("\n");
				}

				if (tag[2] == 0x09)
				{
					/* Send a message to Neutrino cam_menu handler */
					CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
					memset(pMsg, 0, sizeof(CA_MESSAGE));

					pMsg->MsgId = CA_MESSAGE_MSG_MMI_MENU;
					pMsg->SlotType = CA_SLOT_TYPE_CI;
					pMsg->Slot = slot->slot;
					pMsg->Flags = CA_MESSAGE_HAS_PARAM1_DATA;
					pMsg->Msg.Data[0] = (uint8_t *)listInfo;
					cCA::GetInstance()->SendMessage(pMsg);
				}
				else
				{
					/* Send a message to Neutrino cam_menu handler */
					CA_MESSAGE *pMsg = (CA_MESSAGE *) malloc(sizeof(CA_MESSAGE));
					memset(pMsg, 0, sizeof(CA_MESSAGE));

					pMsg->MsgId = CA_MESSAGE_MSG_MMI_LIST;
					pMsg->SlotType = CA_SLOT_TYPE_CI;
					pMsg->Slot = slot->slot;
					pMsg->Flags = CA_MESSAGE_HAS_PARAM1_DATA;
					pMsg->Msg.Data[0] = (uint8_t *)listInfo;
					cCA::GetInstance()->SendMessage(pMsg);
				}
			}
			break;
			default:
				break;
		}
	}
	return 0;
}

int eDVBCIMMISession::doAction()
{
	switch (state)
	{
		case stateStarted:
			state = stateIdle;
			break;
		case stateDisplayReply:
		{
			unsigned char tag[] = {0x9f, 0x88, 0x02};
			unsigned char data[] = {0x01, 0x01};
			sendAPDU(tag, data, 2);
			state = stateIdle;
			break;
		}
		case stateFakeOK:
		{
			unsigned char tag[] = {0x9f, 0x88, 0x0b};
			unsigned char data[] = {5};
			sendAPDU(tag, data, 1);
			state = stateIdle;
			break;
		}
		case stateIdle:
			break;
		default:
			break;
	}
	return 0;
}

int eDVBCIMMISession::stopMMI()
{
	printf("[CI MMI] eDVBCIMMISession::stopMMI()\n");

	unsigned char tag[] = {0x9f, 0x88, 0x00};
	unsigned char data[] = {0x00};
	sendAPDU(tag, data, 1);

	slot->mmiOpened = false;
	return 0;
}

int eDVBCIMMISession::answerText(int answer)
{
	printf("[CI MMI] eDVBCIMMISession::answerText(%d)\n", answer);

	unsigned char tag[] = {0x9f, 0x88, 0x0B};
	unsigned char data[] = {0x00};
	data[0] = answer & 0xff;
	sendAPDU(tag, data, 1);

	return 0;
}

int eDVBCIMMISession::answerEnq(char *answer, int len)
{
	printf("[CI MMI] eDVBCIMMISession::answerEnq(%d bytes)\n", len);

	unsigned char data[len + 1];
	data[0] = 0x01; // answer ok
	memcpy(data + 1, answer, len);

	unsigned char tag[] = {0x9f, 0x88, 0x08};
	sendAPDU(tag, data, len + 1);

	return 0;
}

int eDVBCIMMISession::cancelEnq()
{
	printf("[CI MMI] eDVBCIMMISession::cancelEnq()\n");

	unsigned char tag[] = {0x9f, 0x88, 0x08};
	unsigned char data[] = {0x00}; // canceled
	sendAPDU(tag, data, 1);
	slot->mmiOpened = false;

	return 0;
}

