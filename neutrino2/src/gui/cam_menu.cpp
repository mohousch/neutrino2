/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2010, 2012 Stefan Seyfried
	Copyright (C) 2011 CoolStream International Ltd

	License: GPLv2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation;

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cam_menu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mount.h>

#include <global.h>
#include <neutrino2.h>
#include "widget/icons.h"
#include "widget/stringinput.h"

#include "widget/progresswindow.h"

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>

#include <sectionsd/edvbstring.h>

#include <zapit/zapit.h>
#include <sectionsd/abstime.h>


#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define CI_CLOCK_OPTION_COUNT 3
static const keyval CI_CLOCK_OPTIONS[CI_CLOCK_OPTION_COUNT] = 
{
	{  6, _("CI clock normal") },
	{  7, _("CI clock high") },
	{ 12, _("CI clock high extra") }
};

#define CI_DELAY_OPTION_COUNT 5
static const keyval CI_DELAY_OPTIONS[CI_DELAY_OPTION_COUNT] = 
{
	{  16, "16"  },
	{  32, "32"  },
	{  64, "64"  },
	{ 128, "128" },
	{ 256, "256" }
};

void CCAMMenuHandler::init(void)
{
	hintBox = NULL;
	ca = cCA::GetInstance();
	close_timer = 0;
	in_menu = false;
	menu_type = menu_slot = -1;
}

int CCAMMenuHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	std::string::size_type loc;
	int slot;

	printf("CCAMMenuHandler::exec: actionkey %s\n", actionkey.c_str());
	
        if (parent)
                parent->hide();

	if ((loc = actionkey.find("ca_ci_reset", 0)) != std::string::npos) 
	{
		slot = actionkey.at(11) - '0';

		if(ca && ca->ModulePresent(CA_SLOT_TYPE_CI, slot))
			ca->ModuleReset(CA_SLOT_TYPE_CI, slot);
			
		return RETURN_EXIT;
	} else if ((loc = actionkey.find("ca_ci", 0)) != std::string::npos) 
	{
		slot = actionkey.at(5) - '0';
		printf("CCAMMenuHandler::exec: actionkey %s for slot %d\n", actionkey.c_str(), slot);
		
		return doMenu(slot, CA_SLOT_TYPE_CI);
	} else if ((loc = actionkey.find("ca_sc_reset", 0)) != std::string::npos) 
	{
		slot = actionkey.at(11) - '0';

		if(ca && ca->ModulePresent(CA_SLOT_TYPE_SMARTCARD, slot))
			ca->ModuleReset(CA_SLOT_TYPE_SMARTCARD, slot);
			
		return RETURN_EXIT;
	} else if ((loc = actionkey.find("ca_sc", 0)) != std::string::npos) 
	{
		slot = actionkey.at(5) - '0';
		printf("CCAMMenuHandler::exec: actionkey %s for slot %d\n", actionkey.c_str(), slot);
		return doMenu(slot, CA_SLOT_TYPE_SMARTCARD);
	}

	if(!parent)
		return 0;

	return doMainMenu();
}

int CCAMMenuHandler::doMainMenu()
{
	int ret, cnt;
	char name1[255] = {0};
	char str1[255]={0};

	CMenuWidget* cammenu = new CMenuWidget(_("CI-CAM"), NEUTRINO_ICON_SETTINGS);
	cammenu->setWidgetMode(MODE_SETUP);

	int CiSlots = ca ? ca->GetNumberCISlots() : 0;
	
	// intros
	cammenu->addItem(new CMenuForwarder(_("back")));
	cammenu->addItem( new CMenuSeparator(LINE) );
		
	// save settings
	cammenu->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	cammenu->addItem(new CMenuSeparator(LINE));
	
	if(CiSlots) 
	{
		cammenu->addItem(new CMenuOptionChooser(_("CI Delay"), &g_settings.ci_delay, CI_DELAY_OPTIONS, CI_DELAY_OPTION_COUNT, true, this));

		cammenu->addItem(new CMenuOptionChooser(_("Reset CI (Standby)"), &g_settings.ci_standby_reset, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
		cammenu->addItem(new CMenuOptionChooser(_("Check live Slot"), &g_settings.ci_check_live, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));

		cammenu->addItem(new CMenuSeparator(LINE));
	}

	CMenuWidget * tempMenu;
	int i = 0;

	cnt = 0;
	printf("CCAMMenuHandler::doMainMenu CI slots: %d\n", CiSlots);
	
	while (i < CiSlots && i < 2) 
	{
		if (ca->ModulePresent(CA_SLOT_TYPE_CI, i)) 
		{
			ca->ModuleName(CA_SLOT_TYPE_CI, i, name1);
			
			printf("CCAMMenuHandler::doMainMenu cam%d name %s\n", i, name1);
			
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "ca_ci%d", i);

			cammenu->addItem(new CMenuForwarder(name1, true, NULL, this, tmp));
			snprintf(tmp, sizeof(tmp), "ca_ci_reset%d", i);
			cammenu->addItem(new CMenuForwarder(_("CI Reset"), true, NULL, this, tmp));
			memset(name1, 0, sizeof(name1));

			cammenu->addItem(new CMenuOptionChooser(_("CI clock"), &g_settings.ci_clock[i], CI_CLOCK_OPTIONS, CI_CLOCK_OPTION_COUNT, true, this));

			cammenu->addItem(new CMenuOptionChooser(_("CI rpr"), &g_settings.ci_rpr[i], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));

			cammenu->addItem(new CMenuOptionChooser(_("CI ignore message"), &g_settings.ci_ignore_messages[i], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
			cammenu->addItem(new CMenuOptionChooser(_("CI save pincode"), &g_settings.ci_save_pincode[i], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
		} 
		else 
		{
			snprintf(str1, sizeof(str1), "%s %d", _("CI empty"), i+1);
			tempMenu = new CMenuWidget(str1, NEUTRINO_ICON_SETTINGS);
			cammenu->addItem(new CMenuForwarder(str1, false, NULL, tempMenu));
			memset(str1,0,sizeof(str1));
		}
		if (i < (CiSlots - 1))
			cammenu->addItem(new CMenuSeparator(LINE));
		i++;
	}

	i = 0;
	int ScNum = ca ? ca->GetNumberSmartCardSlots() : 0;
	printf("CCAMMenuHandler::doMainMenu sc slots: %d\n", ScNum);

	if(ScNum && CiSlots)
		cammenu->addItem(new CMenuSeparator(LINE));

	while (i < ScNum && i < 2) 
	{
		if (ca->ModulePresent(CA_SLOT_TYPE_SMARTCARD, i)) 
		{
			ca->ModuleName(CA_SLOT_TYPE_SMARTCARD, i, name1);
			printf("CCAMMenuHandler::doMainMenu cam%d name %s\n", i, name1);
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "ca_sc%d", i);

			cammenu->addItem(new CMenuForwarder(name1, true, NULL, this, tmp));
			memset(name1,0,sizeof(name1));
		} 
		else 
		{
			snprintf(str1, sizeof(str1), "%s %d", _("CI empty"), i);
			tempMenu = new CMenuWidget(str1, NEUTRINO_ICON_SETTINGS);
			cammenu->addItem(new CMenuForwarder(str1, false, NULL, tempMenu));
			memset(str1,0,sizeof(str1));
		}
		if (i < (ScNum - 1))
			cammenu->addItem(new CMenuSeparator(LINE));
		i++;
	}
	
	in_menu = true;
	ret = cammenu->exec(NULL, "");
	delete cammenu;
	in_menu = false;
	
	return ret;
}

#define CI_MSG_TIME 5
int CCAMMenuHandler::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	//printf("CCAMMenuHandler::handleMsg: msg 0x%x data 0x%x\n", msg, data);
	
	int msgret;
	handleCamMsg(msg, data, msgret);
	
	return msgret;
}

void CCAMMenuHandler::showHintBox(const char *const Caption, const char * const Text, uint32_t timeout)
{
	hideHintBox();
	
	hintBox = new CHintBox(Caption, Text);
	
	hintBox->paint();
	
	if(timeout > 0) 
	{
		sleep(timeout);
		hideHintBox();
	}
}

void CCAMMenuHandler::hideHintBox(void)
{
	if(hintBox != NULL) 
	{
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
	}
}

int CCAMMenuHandler::handleCamMsg(const neutrino_msg_t msg, neutrino_msg_data_t data, int &msgret, bool from_menu)
{
	char str[269];
	char cnt[5];
	int i;
	MMI_MENU_LIST_INFO Menu;
	MMI_ENQUIRY_INFO MmiEnquiry;
	MMI_MENU_LIST_INFO *pMenu = &Menu;
	MMI_ENQUIRY_INFO *pMmiEnquiry = &MmiEnquiry;
	CA_MESSAGE Msg, *rMsg;

	//printf("CCAMMenuHandler::handleCamMsg: msg %x data %x from %s\n", msg, data, from_menu ? "menu" : "neutrino");
	
	msgret = messages_return::unhandled;

	if ((msg == NeutrinoMessages::EVT_TIMER) && (data == close_timer)) 
	{
		printf("CCAMMenuHandler::handleCamMsg: EVT_TIMER close_timer %d\n", close_timer);
		g_RCInput->killTimer(close_timer);
		msgret = messages_return::cancel_info;
	}

	if (msg != NeutrinoMessages::EVT_CA_MESSAGE)
		return 1;

	msgret = messages_return::handled;

	rMsg = (CA_MESSAGE *)data;
	if (!rMsg)
		return -1;

	Msg = *rMsg;
	delete rMsg;

	uint32_t MsgId		= Msg.MsgId;
	CA_SLOT_TYPE SlotType	= Msg.SlotType;
	int curslot		= Msg.Slot;

	printf("CCAMMenuHandler::handleCamMsg: CA msg %x from %s\n", MsgId, from_menu ? "menu" : "neutrino");

	if (g_settings.ci_ignore_messages[curslot] && !from_menu && MsgId != CA_MESSAGE_MSG_MMI_REQ_INPUT
	&& MsgId != CA_MESSAGE_MSG_MMI_CLOSE && MsgId != CA_MESSAGE_MSG_INIT_OK
	&& MsgId != CA_MESSAGE_MSG_INSERTED && MsgId != CA_MESSAGE_MSG_REMOVED)
		return 1;

	hideHintBox();

	if (SlotType != CA_SLOT_TYPE_SMARTCARD && SlotType != CA_SLOT_TYPE_CI)
		return -1;

	if(MsgId == CA_MESSAGE_MSG_INSERTED) 
	{
		snprintf(str, sizeof(str), "%s %d", SlotType == CA_SLOT_TYPE_CI ? _("CI inserted") : _("Cam inserted"), (int)curslot + 1);
		
		printf("CCAMMenuHandler::handleCamMsg: %s\n", str);
		HintBox(_("Info"), str);
		if (in_menu)
			msgret = messages_return::cancel_all;
	} 
	else if (MsgId == CA_MESSAGE_MSG_REMOVED) 
	{
		snprintf(str, sizeof(str), "%s %d", SlotType == CA_SLOT_TYPE_CI ? _("CI removed") : _("CAM removed"), (int)curslot + 1);

		printf("CCAMMenuHandler::handleCamMsg: %s\n", str);
		HintBox(_("Info"), str);

		if (in_menu)
			msgret = messages_return::cancel_all;
	} 
	else if(MsgId == CA_MESSAGE_MSG_INIT_OK) 
	{
		char name[255] = "Unknown";
		if (ca)
			ca->ModuleName(SlotType, curslot, name);

		snprintf(str, sizeof(str), "%s %d: %s", SlotType == CA_SLOT_TYPE_CI ? _("CI Init OK") : _("CAM Init OK"), (int)curslot + 1, name);
		printf("CCAMMenuHandler::handleCamMsg: %s\n", str);
		//CCamManager::getInstance()->Start(CZapit::getInstance()->GetCurrentChannelID(), CCamManager::PLAY, true);
		HintBox(_("Info"), str);
	} 
	else if(MsgId == CA_MESSAGE_MSG_INIT_FAILED) 
	{
		char name[255] = "Unknown";
		if (ca)
			ca->ModuleName(SlotType, curslot, name);

		snprintf(str, sizeof(str), "%s %d: %s", SlotType == CA_SLOT_TYPE_CI ? _("CI Init failed") : _("CAM Init failed"), (int)curslot + 1, name);

		printf("CCAMMenuHandler::handleCamMsg: %s\n", str);
		HintBox(_("Info"), str);
	} 
	else if(MsgId == CA_MESSAGE_MSG_MMI_MENU || MsgId == CA_MESSAGE_MSG_MMI_LIST) 
	{
		bool sublevel = false;

		if(MsgId != CA_MESSAGE_MSG_MMI_MENU)
			sublevel = true;

		if (!(Msg.Flags & CA_MESSAGE_HAS_PARAM1_DATA))
			return -1;

		memmove(pMenu, (MMI_MENU_LIST_INFO*)Msg.Msg.Data[0], sizeof(MMI_MENU_LIST_INFO));
		free((void *)Msg.Msg.Data[0]);

		printf("CCAMMenuHandler::handleCamMsg: slot %d menu ready, title %s choices %d\n", curslot, convertDVBUTF8(pMenu->title, strlen(pMenu->title), 0).c_str(), pMenu->choice_nb);

		int menuret = RETURN_REPAINT;
		int selected = -1;
		
		if(pMenu->choice_nb && from_menu) 
		{
			CMenuWidget* menu = new CMenuWidget(convertDVBUTF8(pMenu->title, strlen(pMenu->title), 0).c_str(), NEUTRINO_ICON_SETTINGS);
			menu->enableSaveScreen();

			//CMenuSelectorTarget * selector = new CMenuSelectorTarget(&selected);
			int slen = strlen(pMenu->subtitle);
			
			if(slen) 
			{
				char * sptr = pMenu->subtitle;
				char * tptr = sptr;
				int bpos = 0;
				
				for(int li = 0; li < slen; li++) 
				{
					if((tptr[li] == 0x8A) || ((bpos > 38) && (tptr[li] == 0x20)) ) 
					{
						bpos = 0;
						tptr[li] = 0;
						printf("CCAMMenuHandler::handleCamMsg: subtitle: %s\n", sptr);
						menu->addItem(new CMenuForwarder(convertDVBUTF8(sptr, strlen(sptr), 0).c_str(), false));
						sptr = &tptr[li+1];
					}
					bpos++;
				}
				
				if(strlen(sptr)) 
				{
					printf("CCAMMenuHandler::handleCamMsg: subtitle: %s\n", sptr);
					menu->addItem(new CMenuForwarder(convertDVBUTF8(sptr, strlen(sptr), 0).c_str(), false));
				}
			}
			
			for(i = 0; (i < pMenu->choice_nb) && (i < MAX_MMI_ITEMS); i++) 
			{
				snprintf(cnt, sizeof(cnt), "%d", i);
				if(sublevel)
					menu->addItem(new CMenuForwarder(convertDVBUTF8(pMenu->choice_item[i], strlen(pMenu->choice_item[i]), 0).c_str(), true, NULL, this, cnt));
				else
					menu->addItem(new CMenuForwarder(convertDVBUTF8(pMenu->choice_item[i], strlen(pMenu->choice_item[i]), 0).c_str(), true, NULL, this, cnt));
			}
			
			slen = strlen(pMenu->bottom);
			if(slen) 
			{
				printf("CCAMMenuHandler::handleCamMsg: bottom: %s\n", pMenu->bottom);
				menu->addItem(new CMenuForwarder(convertDVBUTF8(pMenu->bottom, slen, 0).c_str(), false));
			}

			selected = menu->getSelected();
			menuret = menu->exec(NULL, "");
			delete menu;
		} 
		else 
		{
			char lstr[255];
			int slen = 0;

			if(strlen(pMenu->title))
				slen += snprintf(&lstr[slen], 255-slen, "%s\n", pMenu->title);
			if(strlen(pMenu->subtitle))
				slen += snprintf(&lstr[slen], 255-slen, "%s\n", pMenu->subtitle);

			for(i = 0; (i < pMenu->choice_nb) && (i < MAX_MMI_ITEMS); i++)
				slen += snprintf(&lstr[slen], 255-slen, "%s\n", pMenu->choice_item[i]);

			if(strlen(pMenu->bottom))
				slen += snprintf(&lstr[slen], 255-slen, "%s\n", pMenu->bottom);

			HintBox(_("Info"), convertDVBUTF8(lstr, slen, 0).c_str());
			return 0;
		}

		if(sublevel)
			return menuret == RETURN_EXIT_ALL ? 3 : 0;

		if(selected >= 0) 
		{
			printf("CCAMMenuHandler::handleCamMsg: selected %d:%s sublevel %s\n", selected, pMenu->choice_item[i], sublevel ? "yes" : "no");
			ca->MenuAnswer(SlotType, curslot, selected+1);
			timeoutEnd = CRCInput::calcTimeoutEnd(10);
			return 1;
		} 
		else 
		{
			return menuret == RETURN_EXIT_ALL ? 3 : 2;
		}
	}
	else if(MsgId == CA_MESSAGE_MSG_MMI_REQ_INPUT) 
	{
		if (!(Msg.Flags & CA_MESSAGE_HAS_PARAM1_DATA))
			return -1;

		memmove(pMmiEnquiry, (MMI_ENQUIRY_INFO *)Msg.Msg.Data[0], sizeof(MMI_ENQUIRY_INFO));
		free((void *)Msg.Msg.Data[0]);
		printf("CCAMMenuHandler::handleCamMsg: slot %d input request, text %s\n", curslot, convertDVBUTF8(pMmiEnquiry->enquiryText, strlen(pMmiEnquiry->enquiryText), 0).c_str());

		std::string ENQAnswer;
#if 1
		if (/* !from_menu && */ g_settings.ci_save_pincode[curslot] && pMmiEnquiry->blind != 0 && (int) g_settings.ci_pincode[curslot].length() == pMmiEnquiry->answerlen) 
		{
			static int acount = 0;
			static time_t last_ask = 0;

			ENQAnswer = g_settings.ci_pincode[curslot];
			printf("CCAMMenuHandler::handleCamMsg: using saved answer [%s] (#%d, time diff %d)\n", ENQAnswer.c_str(), acount, (int) (time_monotonic() - last_ask));
			if ((time_monotonic() - last_ask) < 10) 
			{
				acount++;
				if (acount > 4)
					g_settings.ci_pincode[curslot].clear();
			} else 
			{
				last_ask = time_monotonic();
				acount = 0;
			}
		} 
		else 
#endif
		{
			//CEnquiryInput *Inquiry = new CEnquiryInput((char *)convertDVBUTF8(pMmiEnquiry->enquiryText, strlen(pMmiEnquiry->enquiryText), 0).c_str(), &ENQAnswer, pMmiEnquiry->answerlen, pMmiEnquiry->blind != 0, _(" "));
			//Inquiry->exec(NULL, "");
			//delete Inquiry;
			//g_settings.ci_pincode[curslot] = ENQAnswer;
		}

		printf("CCAMMenuHandler::handleCamMsg: input=[%s]\n", ENQAnswer.c_str());

		if((int) ENQAnswer.length() != pMmiEnquiry->answerlen) 
		{
			printf("CCAMMenuHandler::handleCamMsg: wrong input len\n");
			ca->InputAnswer(SlotType, curslot, (unsigned char *)ENQAnswer.c_str(), 0);
			return 1; //FIXME
		} 
		else 
		{
			ca->InputAnswer(SlotType, curslot, (unsigned char *)ENQAnswer.c_str(), pMmiEnquiry->answerlen);
			return 1;
		}
	}
	else if(MsgId == CA_MESSAGE_MSG_MMI_CLOSE) 
	{
		int timeout = 0;
		if (Msg.Flags & CA_MESSAGE_HAS_PARAM1_INT)
			timeout = Msg.Msg.Param[0];
			
		printf("CCAMMenuHandler::handleCamMsg: close request slot: %d (timeout %d)\n", curslot, timeout);
		
		//ca->MenuClose(SlotType, curslot);
		if (timeout)
			close_timer = g_RCInput->addTimer(timeout*1000*1000, true);
		else
			msgret = messages_return::cancel_info;
		return 0;
	}
	else if(MsgId == CA_MESSAGE_MSG_MMI_TEXT) 
	{
		printf("CCAMMenuHandler::handleCamMsg: text\n");
	}
	else if(MsgId == CA_MESSAGE_MSG_CHANNEL_CHANGE) 
	{
		if (!(Msg.Flags & CA_MESSAGE_HAS_PARAM1_LONG))
			return -1;
/*
		t_channel_id chid = Msg.Msg.ParamLong[0];
		
		printf("CCAMMenuHandler::handleCamMsg: CA_MESSAGE_MSG_CHANNEL_CHANGE: %" PRIx64 "\n", chid);
		
		CZapitChannel * channel = CServiceManager::getInstance()->FindChannel48(chid);
		
		if (!channel) 
		{
			printf("CCAMMenuHandler::handleCamMsg: channel %" PRIx64 "not found\n", chid);
			return -1;
		}
		CNeutrinoApp::getInstance()->zapTo(channel->getChannelID());
*/
	} 
	
	return 1;
}

int CCAMMenuHandler::doMenu(int slot, CA_SLOT_TYPE slotType)
{
	int res = RETURN_REPAINT;
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	bool doexit = false;

	menu_slot = slot;
	menu_type = slotType;
	
	while(!doexit) 
	{
		printf("CCAMMenuHandler::doMenu: enter menu for slot %d\n", slot);
		timeoutEnd = CRCInput::calcTimeoutEnd(10);

		ca->MenuEnter(slotType, slot);
		
		while(true) 
		{
			showHintBox(_("Info"), slotType == CA_SLOT_TYPE_CI ? _("CI waiting") : _("CAM waiting"));

			g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);
			printf("CCAMMenuHandler::doMenu: msg %lx data %lx\n", msg, data);
			if (msg == RC_timeout) 
			{
				printf("CCAMMenuHandler::doMenu: menu timeout\n");
				hideHintBox();
				HintBox(_("Info"), slotType == CA_SLOT_TYPE_CI ? _("CI Timeout") : _("CAM Timeout"), 450, 3);
				ca->MenuClose(slotType, slot);
				return RETURN_REPAINT;
			}
			/* -1 = not our event, 0 = back to top menu, 1 = continue loop, 2 = quit , 3 = quit all*/
			int msgret;
			int ret = handleCamMsg(msg, data, msgret, true);
			printf("CCAMMenuHandler::doMenu: handleCamMsg ret: %d\n", ret);
			
			if((msgret & messages_return::unhandled) && (msg > RC_Events)) 
			{
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & (messages_return::cancel_all | messages_return::cancel_info))
				{
					doexit = true;
					res = RETURN_EXIT_ALL;
					break;
				}
			}
			
			if (ret == 1) 
			{
				/* workaround: dont cycle here on timers */
				if (msg != NeutrinoMessages::EVT_TIMER)
					timeoutEnd = CRCInput::calcTimeoutEnd(10);
				continue;
			} 
			else if (ret == 2) 
			{
				doexit = true;
				break;
			} 
			else if (ret == 3) 
			{
				res = RETURN_EXIT_ALL;
				doexit = true;
				break;
			} 
			else 
			{ // ret == 0
				break;
			}
		}
	}
	ca->MenuClose(slotType, slot);
	hideHintBox();
	menu_type = menu_slot = -1;
	printf("CCAMMenuHandler::doMenu: return\n");
	return res;
}

bool CCAMMenuHandler::changeNotify(const std::string& OptionName, void * Data)
{
	if (OptionName == _("CI Delay")) 
	{
		printf("CCAMMenuHandler::changeNotify: ci_delay %d\n", g_settings.ci_delay);
		ca->SetCIDelay(g_settings.ci_delay);
		
		return true;
	}
	else if (OptionName == _("CI rpr")) 
	{
		for (unsigned int i = 0; i < ca->GetNumberCISlots(); i++) 
		{
			printf("CCAMMenuHandler::changeNotify: ci_rpr[%d] %d\n", i, g_settings.ci_rpr[i]);
			ca->SetCIRelevantPidsRouting(g_settings.ci_rpr[i], i);
		}
		
		return true;
	}
	else if (OptionName == _("CI Clock")) 
	{
		for (unsigned int i = 0; i < ca->GetNumberCISlots(); i++) 
		{
			printf("CCAMMenuHandler::changeNotify: ci_clock[%d] %d\n", i, g_settings.ci_clock[i]);
			ca->SetTSClock(g_settings.ci_clock[i] * 1000000, i);
		}
		
		return true;
	}
	else if (OptionName == _("CI save pincode")) 
	{
		int enabled = *(int *) Data;
		if (!enabled) {
			for (unsigned int i = 0; i < ca->GetNumberCISlots(); i++) 
			{
				printf("CCAMMenuHandler::changeNotify: clear saved pincode[%d]\n", i);
				g_settings.ci_pincode[i].clear();
			}
		}
	}
	else if (OptionName == _("CI check live slot")) 
	{
		ca->setCheckLiveSlot(g_settings.ci_check_live);
	}
	else if (OptionName == _("CI Tuner")) 
	{
		printf("CCAMMenuHandler::changeNotify: bind CI to tuner %d\n", g_settings.ci_tuner);
		//CCamManager::getInstance()->SetCITuner(g_settings.ci_tuner);
	}
	
	return false;
}

