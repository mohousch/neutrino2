/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mount.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>
#include "gui/widget/stringinput.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/progresswindow.h"

#include "system/setting_helpers.h"
#include "system/settings.h"
#include "system/debug.h"

#include <gui/cam_menu.h>

#include <sectionsd/edvbstring.h>
#include <libdvbci/dvb-ci.h>


extern cDvbCi * ci;

void CCAMMenuHandler::init(void)
{
	hintBox = NULL;
}

int CCAMMenuHandler::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CCAMMenuHandler::exec: actionkey %s\n", actionKey.c_str());

	int ret = RETURN_REPAINT;

        if (parent)
                parent->hide();

	if(actionKey == "cam1") 
	{
		return doMenu(0);
	}	
	else if(actionKey == "cam2") 
	{
		return doMenu(1);
	}	
	if(actionKey == "reset1") 
	{
		ci->reset(0);
		return RETURN_EXIT_ALL;
	}	
	if(actionKey == "reset2") 
	{
		ci->reset(1);
		return RETURN_EXIT_ALL;
	}	

	doMainMenu();

	return ret;
}

void CCAMMenuHandler::doMainMenu()
{
	dprintf(DEBUG_NORMAL, "CCAMMenuHandler::doMainMenu\n");

	char name[255];
	char str[255];
	
	//
	CWidget* widget = NULL;
	ClistBox* cammenu = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("cammenu");
	
	if (widget)
	{
		cammenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		cammenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		cammenu->setWidgetMode(MODE_SETUP);
		cammenu->enableShrinkMenu();
		
		cammenu->enablePaintHead();
		cammenu->setTitle(_("CI Cam settings"), NEUTRINO_ICON_CI);

		cammenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		cammenu->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "cammenu";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(cammenu);
	}
	
	cammenu->clearItems();
	
	// intros
	cammenu->addItem(new ClistBoxItem(_("back")));

	//
	CWidget* tempMenuWidget = NULL;
	ClistBox* tempMenu = NULL;
	
	for (int i = 0; i < ci->ci_num; i++)
	{
		if(ci->CamPresent(i)) 
		{
			ci->GetName(i, name);
			
			dprintf(DEBUG_NORMAL, "CCAMMenuHandler::doMenu cam %d name %s\n", i, name);
			
			char CAM[255];
			sprintf(CAM, "cam%d", i + 1);

			cammenu->addItem(new ClistBoxItem(name, true, NULL, this, CAM, RC_nokey));

			char RESET[32];
			sprintf(RESET, "reset%d", i + 1);

			cammenu->addItem(new CMenuSeparator(LINE));
			cammenu->addItem(new ClistBoxItem(_("CI cam reset"), true, NULL, this, RESET, RC_nokey));
		} 
		else 
		{
			sprintf(str, "%s %d", _("No CAM in slot"), i + 1);
			
			/*
			if (CNeutrinoApp::getInstance()->widget_exists("tempcam2"))
			{
				tempMenuWidget = CNeutrinoApp::getInstance()->getWidget("tempcam2");
				tempMenu = (ClistBox*)tempMenuWidget->getWidgetItem(WIDGETITEM_LISTBOX);
				
				if (tempMenu->hasHead())
				{
					tempMenu->setTitle(str, NEUTRINO_ICON_CI);
				}
			}
			else
			*/
			{
				tempMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

				tempMenu->setWidgetMode(MODE_SETUP);
				tempMenu->enableShrinkMenu();
				
				//
				tempMenu->enablePaintHead();
				tempMenu->setTitle(str, NEUTRINO_ICON_CI);
				
				//
				tempMenu->enablePaintFoot();		
				const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
				tempMenu->setFootButtons(&btn);
				
				//
				tempMenuWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
				tempMenuWidget->setMenuPosition(MENU_POSITION_CENTER);
				tempMenuWidget->enableSaveScreen();
				tempMenuWidget->addWidgetItem(tempMenu);
			}
			
			//
			tempMenu->clear();

			//
			cammenu->addItem(new CMenuSeparator(LINE));
			cammenu->addItem(new ClistBoxItem(str, false, NULL, tempMenuWidget));
		}
	}	

	//
	widget->exec(NULL, "");
}

#define CI_MSG_TIME 5
int CCAMMenuHandler::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int ret = messages_return::handled;

	int camret = handleCamMsg(msg, data);
	
	if(camret < 0) 
	{
		ret = messages_return::unhandled;
	}
	
	return ret;
}

int CCAMMenuHandler::handleCamMsg(const neutrino_msg_t msg, neutrino_msg_data_t data, bool from_menu)
{
	int ret = 0;
	char str[255];
	int i;
	MMI_MENU_LIST_INFO Menu;
	MMI_ENGUIRY_INFO MmiEnquiry;
	int curslot;
	MMI_MENU_LIST_INFO * pMenu = &Menu;
	MMI_ENGUIRY_INFO * pMmiEnquiry = &MmiEnquiry;

	if(msg == NeutrinoMessages::EVT_CI_INSERTED) 
	{
		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;
		}
		
		sprintf(str, "%s %d", _("CAM inserted in slot"), (int) data + 1);

		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleMsg: %s\n", str);
		
		hintBox = new CHintBox(_("Information"), str);

		if(hintBox != NULL) 
			hintBox->paint();

		sleep(CI_MSG_TIME);

		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;
		}

	} 
	else if (msg == NeutrinoMessages::EVT_CI_REMOVED) 
	{
		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;
		}

		sprintf(str, "%s %d", _("CAM removed from slot"), (int) data + 1);

		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleMsg: %s\n", str);
		
		hintBox = new CHintBox(_("Information"), str);

		if(hintBox != NULL) 
			hintBox->paint();

		sleep(CI_MSG_TIME);

		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;	
		}
	} 
	else if(msg == NeutrinoMessages::EVT_CI_INIT_OK) 
	{
		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;
		}
		char name[255] = "Unknown";
		
		ci->GetName((int) data, name);
		sprintf(str, "%s %d: %s", _("CAM init complete"), (int) data+1, name);

		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleMsg: %s\n", str);
		
		hintBox = new CHintBox(_("Information"), str);

		if(hintBox != NULL) 
			hintBox->paint();

		sleep(CI_MSG_TIME);

		if(hintBox != NULL) 
		{
			hintBox->hide();
			delete hintBox;
			hintBox = NULL;
		}
	}
	else if(msg == NeutrinoMessages::EVT_CI_MMI_MENU || msg == NeutrinoMessages::EVT_CI_MMI_LIST) 
	{
		bool sublevel = false;
		if(msg != NeutrinoMessages::EVT_CI_MMI_MENU)
			sublevel = true;
		
		memcpy(pMenu, (MMI_MENU_LIST_INFO*) data, sizeof(MMI_MENU_LIST_INFO));
		free((void *)data);

		curslot = pMenu->slot;

		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: slot %d menu ready, title %s choices %d\n", curslot, convertDVBUTF8(pMenu->title, strlen(pMenu->title), 0).c_str(), pMenu->choice_nb);

		if(hintBox) 
			hintBox->hide();

		int selected = -1;
		
		if(pMenu->choice_nb) 
		{
			//
			CWidget* menuWidget = NULL;
			ClistBox* menu = NULL;
			
			/*
			if (CNeutrinoApp::getInstance()->widget_exists("tempcam"))
			{
				menuWidget = CNeutrinoApp::getInstance()->getWidget("tempcam");
				menu = (ClistBox*)menuWidget->getWidgetItem(WIDGETITEM_LISTBOX);
				
				if (menu->hasHead())
					menu->setTitle(convertDVBUTF8(pMenu->title, strlen(pMenu->title), 0).c_str(), NEUTRINO_ICON_CI);
			}
			else
			*/
			{
				menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

				menu->setWidgetMode(MODE_SETUP);
				menu->enableShrinkMenu();
				
				//
				menu->enablePaintHead();
				menu->setTitle(convertDVBUTF8(pMenu->title, strlen(pMenu->title), 0).c_str(), NEUTRINO_ICON_CI);
				
				//
				menu->enablePaintFoot();		
				const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
				menu->setFootButtons(&btn);
				
				//
				menuWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
				menuWidget->setMenuPosition(MENU_POSITION_CENTER);
				menuWidget->enableSaveScreen();
				menuWidget->addWidgetItem(menu);
			}
			
			menu->clear();

			int slen = strlen(pMenu->subtitle);
			
			if(slen) 
			{
				char * sptr = pMenu->subtitle;
				char * tptr = sptr;
				int bpos = 0;
				for(int i1 = 0; i1 < slen; i1++) 
				{
					if((tptr[i1] == 0x8A) || ((bpos >= 38) && (tptr[i1] == 0x20)) ) 
					{
						bpos = 0;
						tptr[i1] = 0;
						
						dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: subtitle: %s\n", sptr);
						
						menu->addItem(new ClistBoxItem(convertDVBUTF8(sptr, strlen(sptr), 0).c_str(), false));
						sptr = &tptr[i1+1];
					}
					bpos++;
				}
				
				if(strlen(sptr)) 
				{
					dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: subtitle: %s\n", sptr);
					
					menu->addItem(new ClistBoxItem(convertDVBUTF8(sptr, strlen(sptr), 0).c_str(), false));
				}
			}
			
			for(i = 0; i < pMenu->choice_nb; i++) 
			{
				if(sublevel)
					menu->addItem(new ClistBoxItem(convertDVBUTF8(pMenu->choice_item[i], strlen(pMenu->choice_item[i]), 0).c_str()));
				else
					menu->addItem(new ClistBoxItem(convertDVBUTF8(pMenu->choice_item[i], strlen(pMenu->choice_item[i]), 0).c_str(), true, NULL, NULL, NULL, CRCInput::convertDigitToKey(i+1)));
			}
			slen = strlen(pMenu->bottom);
			
			if(slen) 
			{
				dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: bottom: %s\n", pMenu->bottom);
				
				menu->addItem(new ClistBoxItem(convertDVBUTF8(pMenu->bottom, slen, 0).c_str(), false));
			}

			menuWidget->exec(NULL, "");
			selected = menu->getSelected();
		} 
		else 
		{
			char _str[255];
			snprintf(_str, 255, "%s\n%s\n%s", pMenu->title, pMenu->subtitle, pMenu->bottom);
			
			if(hintBox)
			{
				delete hintBox;
				hintBox = NULL;
			}

			hintBox = new CHintBox(_("Information"), _(convertDVBUTF8(_str, strlen(_str), 0).c_str()));
			if(hintBox != NULL) 
				hintBox->paint();

			sleep(4);//FIXME
			
			if(!from_menu) 
			{
				if(hintBox != NULL) 
				{
					delete hintBox;
					hintBox = NULL;
				}
			}
			return 1;
		}

		if(sublevel)
			return 0;

		if(selected >= 0) 
		{
			dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: selected %d:%s sublevel %s\n", selected, pMenu->choice_item[i], sublevel ? "yes" : "no");

			ci->CI_MenuAnswer(curslot, selected + 1);

			timeoutEnd = CRCInput::calcTimeoutEnd(10);
			return 1;
		} 
		else 
		{
			return 2;
		}
	}
	else if(msg == NeutrinoMessages::EVT_CI_MMI_REQUEST_INPUT) 
	{
		memcpy(pMmiEnquiry, (MMI_ENGUIRY_INFO*) data, sizeof(MMI_ENGUIRY_INFO));
		free((void *)data);
		curslot = pMmiEnquiry->slot;
		
		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: slot %d input request, text %s\n", curslot, convertDVBUTF8(pMmiEnquiry->enguiryText, strlen(pMmiEnquiry->enguiryText), 0).c_str());
		
		if(hintBox)
			hintBox->hide();

		char cPIN[pMmiEnquiry->answerlen+1];
		cPIN[0] = 0;

		CPINInput *PINInput = new CPINInput((char *) convertDVBUTF8(pMmiEnquiry->enguiryText, strlen(pMmiEnquiry->enguiryText), 0).c_str(), cPIN, 4);
		PINInput->exec(NULL, "");
		delete PINInput;

		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: input=[%s]\n", cPIN);

		if((int) strlen(cPIN) != pMmiEnquiry->answerlen) 
		{
			dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: wrong input len\n");

			ci->CI_Answer(curslot, (unsigned char *) cPIN, 0);

			return 0;
		} 
		else 
		{
			ci->CI_Answer(curslot, (unsigned char *) cPIN, pMmiEnquiry->answerlen);

			return 1;
		}
	}
	else if(msg == NeutrinoMessages::EVT_CI_MMI_CLOSE) 
	{
		curslot = (int) data;
		
		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: close request slot: %d\n", curslot);

		ci->CI_CloseMMI(curslot);

		return 0;
	}
	else if(msg == NeutrinoMessages::EVT_CI_MMI_TEXT) 
	{
		curslot = (int) data;
		
		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::handleCamMsg: text\n");
	} 
	else
		ret = -1;

	return ret;
}

int CCAMMenuHandler::doMenu(int slot)
{
	int res = RETURN_REPAINT;
	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	bool doexit = false;

	while(!doexit) 
	{
		dprintf(DEBUG_NORMAL, "CCAMMenuHandler::doMenu: slot %d\n", slot);

		timeoutEnd = CRCInput::calcTimeoutEnd(10);

		ci->CI_EnterMenu(slot);

		while(true) 
		{
			if(hintBox)
			{
				delete hintBox;
				hintBox = NULL;
			}
			
			hintBox = new CHintBox(_("Information"), _("Waiting for CI answer"));

			if(hintBox != NULL) 
				hintBox->paint();

			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
			
			if (msg == RC_timeout) 
			{
				if(hintBox)
				{
					delete hintBox;
					hintBox = NULL;
				}

				hintBox = new CHintBox(_("Information"), _("Timeout waiting CI menu ready"));
				
				if(hintBox != NULL) 
					hintBox->paint();

				dprintf(DEBUG_NORMAL, "CCAMMenuHandler::doMenu: menu timeout\n");
				
				sleep(5);

				if(hintBox != NULL) 
				{
					delete hintBox;
					hintBox = NULL;
				}

				ci->CI_CloseMMI(slot);

				return RETURN_REPAINT;
			} 
			
			/* -1 = not our event, 0 = back to top menu, 1 = continue loop, 2 = quit */
			int ret = handleCamMsg(msg, data, true);
			if(ret < 0 && (msg > RC_Messages)) 
			{
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & ( messages_return::cancel_all | messages_return::cancel_info ) )
				{
					doexit = true;
					res = RETURN_EXIT_ALL;
				}
			} 
			else if (ret == 1) 
			{
				timeoutEnd = CRCInput::calcTimeoutEnd(10);
				continue;
			} 
			else if (ret == 2) 
			{
				doexit = true;
				break;
			} 
			else 
			{
				break;
			}
		}
	}

	ci->CI_CloseMMI(slot);

	if(hintBox) 
	{
		delete hintBox;
		hintBox = NULL;
	}
	
	dprintf(DEBUG_NORMAL, "CCAMMenuHandler::doMenu: return\n");
	
	return res; 
}


