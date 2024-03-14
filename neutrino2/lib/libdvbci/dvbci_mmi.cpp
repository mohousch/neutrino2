/* DVB CI MMI */
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

#include "dvbci_mmi.h"

#include <neutrinoMessages.h>
#include <driver/rcinput.h>


extern CRCInput *g_RCInput;

eDVBCIMMISession::eDVBCIMMISession(tSlot *tslot)
{
	slot = tslot;

	slot->hasMMIManager = true;
	slot->mmiSession = this;
}

eDVBCIMMISession::~eDVBCIMMISession()
{
        if (g_RCInput)
           	g_RCInput->postMsg(NeutrinoMessages::EVT_CI_MMI_CLOSE, 0);
	
	slot->hasMMIManager = false;
	slot->mmiSession = NULL;
	slot->mmiOpened = false;
}

int eDVBCIMMISession::receivedAPDU(const unsigned char *tag, const void *data, int len)
{
	if ((tag[0] == 0x9f) && (tag[1] == 0x88))
        {
	
                /* from e2 mmi_ui.cpp */ 
                switch (tag[2])
		{
			case 0x00: /* close */
                        	if (g_RCInput)
                          		g_RCInput->postMsg(NeutrinoMessages::EVT_CI_MMI_CLOSE, 0);
		    	break;

		    	case 0x01: /* display control */
		       		state = stateDisplayReply;
		       		return 1;
		    		break;

		    	case 0x07: /* menu enq */
                    	{
		                MMI_ENGUIRY_INFO* enquiry = (MMI_ENGUIRY_INFO*) malloc(sizeof(MMI_ENGUIRY_INFO));
		                memset(enquiry, 0, sizeof(MMI_ENGUIRY_INFO));

				unsigned char *d = (unsigned char*)data;
				unsigned char *max = ((unsigned char*)d) + len;
		
				int textlen = len - 2;
		
				if ((d + 2) > max)
				    	break;
				
				int blind = *d++ & 1;
				int alen = *d++;
			
				printf("%d bytes text\n", textlen);
				
				if ((d + textlen) > max)
				   	break;
		
				char str[textlen + 1];
				
				memcpy(str, ((char*)d), textlen);
				str[textlen] = '\0';
			       
				enquiry->slot = slot->slot;
				enquiry->blind = blind;
				enquiry->answerlen = alen;
				strcpy(enquiry->enguiryText, str);

		                if (g_RCInput)
		                  	g_RCInput->postMsg(NeutrinoMessages::EVT_CI_MMI_REQUEST_INPUT, (neutrino_msg_data_t) enquiry);

			        slot->mmiOpened = true;
                    	}
		    	break;

		    	case 0x09: /* menu last */
		    	case 0x0c: /* list last */
		    	{
		               	MMI_MENU_LIST_INFO* listInfo = (MMI_MENU_LIST_INFO*) malloc(sizeof(MMI_MENU_LIST_INFO));
		               	memset(listInfo, 0, sizeof(MMI_MENU_LIST_INFO));

		               	listInfo->slot = slot->slot;
		               	listInfo->choice_nb = 0;

			       	unsigned char *d = (unsigned char*)data;
			      	unsigned char *max = ((unsigned char*)d) + len;
			       	int pos = 0;

				/*
		               	if (tag[2] == 0x09)
		                   	printf("menu_last\n");
		               	else
		                   	printf("list_last\n");
				*/

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
				       
				       printf("text tag: %02x %02x %02x\n", d[0], d[1], d[2]);
				       d += 3;
				       d += eDVBCISession::parseLengthField(d, textlen);
				       
				       printf("%d bytes text", textlen);

				       if ((d + textlen) > max)
					       break;
				       
				       char str[textlen + 1];
				       memcpy(str, ((char*)d), textlen);
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
						   
						   printf("%d. %s\n",listInfo->choice_nb, listInfo->choice_item[listInfo->choice_nb - 1]);
		                       }
			       	}

		               	if (g_RCInput)
			       	{
					if (tag[2] == 0x09)
				               g_RCInput->postMsg(NeutrinoMessages::EVT_CI_MMI_MENU, (neutrino_msg_data_t) listInfo);
					else
				               g_RCInput->postMsg(NeutrinoMessages::EVT_CI_MMI_LIST, (neutrino_msg_data_t) listInfo);
			       	}
		    	}
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
			state=stateIdle;
			//state=stateFakeOK;
			//return 1;
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
	unsigned char tag[]={0x9f, 0x88, 0x00};
	unsigned char data[]={0x00};
	sendAPDU(tag, data, 1);
	
	slot->mmiOpened = false;

	return 0;
}

int eDVBCIMMISession::answerText(int answer)
{
	unsigned char tag[] = {0x9f, 0x88, 0x0B};
	unsigned char data[] = {0x00};
	data[0] = answer & 0xff;
	sendAPDU(tag, data, 1);
	
	return 0;
}

int eDVBCIMMISession::answerEnq(char *answer, int len)
{
	unsigned char data[len+1];
	data[0] = 0x01; // answer ok
	memcpy(data+1, answer, len);

	unsigned char tag[]={0x9f, 0x88, 0x08};
	sendAPDU(tag, data, len+1);

	return 0;
}

int eDVBCIMMISession::cancelEnq()
{
	unsigned char tag[]={0x9f, 0x88, 0x08};
	unsigned char data[]={0x00}; // canceled
	sendAPDU(tag, data, 1);
	
	slot->mmiOpened = false;
	
	return 0;
}

