/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: dvbsub_select.cpp 2011/11/11 11:23:30 mohousch Exp $

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


#include <global.h>
#include <neutrino2.h>
#include <gui/widget/icons.h>

#include <gui/dvbsub_select.h>

#include <system/debug.h>


// dvb
extern int dvbsub_getpid();				// defined in libdvbsub
//extern int dvbsub_getpid(int *pid, int *running);				// defined in libdvbsub
// tuxtxt
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);

int CDVBSubSelectMenuHandler::exec(CMenuTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CDVBSubSelectMenuHandler::exec:\n");

	int res = RETURN_EXIT_ALL;

	if (parent) 
		parent->hide();

	doMenu();

	return res;
}

int CDVBSubSelectMenuHandler::doMenu()
{
	int res = RETURN_EXIT_ALL;
	
	dprintf(DEBUG_NORMAL, "CDVBSubSelectMenuHandler::doMenu:\n");

	//
	CWidget* widget = NULL;
	ClistBox* DVBSubSelector = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("subselect");
	
	if (widget)
	{
		DVBSubSelector = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{			
		DVBSubSelector = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		DVBSubSelector->setWidgetMode(MODE_SETUP);
		DVBSubSelector->enableShrinkMenu();
						
		DVBSubSelector->enablePaintHead();
		DVBSubSelector->setTitle(_("Subtitle Select"), NEUTRINO_ICON_SUBT);

		DVBSubSelector->enablePaintFoot();
							
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
							
		DVBSubSelector->setFootButtons(&btn);
						
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "subselect";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(DVBSubSelector);
	}
	
	DVBSubSelector->clearItems();
	
	CSubtitleChangeExec SubtitleChanger;
	unsigned int count;

	//dvb/tuxtxt subs
	CChannelList * channelList = CNeutrinoApp::getInstance ()->channelList;
	int curnum = channelList->getActiveChannelNumber();
	CZapitChannel * cc = channelList->getChannel(curnum);

	bool sep_added = false;
	if(cc) 
	{
		for (int i = 0 ; i < (int)cc->getSubtitleCount() ; ++i) 
		{
			CZapitAbsSub* s = cc->getChannelSub(i);
			
			// dvbsub
			if (s->thisSubType == CZapitAbsSub::DVB) 
			{
				CZapitDVBSub * sd = reinterpret_cast<CZapitDVBSub*>(s);
				printf("[CDVBSubSelectMenuHandler] adding DVB subtitle %s pid 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId);
				if(!sep_added) 
				{
					sep_added = true;
				}
				char spid[10];
				//int pid = sd->pId;
				snprintf(spid,sizeof(spid), "DVB:%d", sd->pId);
				char item[64];
	
				snprintf(item,sizeof(item), "DVB: %s", sd->ISO639_language_code.c_str());
				DVBSubSelector->addItem(new ClistBoxItem(item, sd->pId != dvbsub_getpid() /*dvbsub_getpid(&pid, NULL)*/, NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
			
			//txt subs
			if (s->thisSubType == CZapitAbsSub::TTX) 
			{
				CZapitTTXSub* sd = reinterpret_cast<CZapitTTXSub*>(s);
				printf("[CDVBSubSelectMenuHandler] adding TTX subtitle %s pid 0x%x mag 0x%X page 0x%x\n", sd->ISO639_language_code.c_str(), sd->pId, sd->teletext_magazine_number, sd->teletext_page_number);
				if(!sep_added) 
				{
					sep_added = true;
				}
				char spid[64];
				int page = ((sd->teletext_magazine_number & 0xFF) << 8) | sd->teletext_page_number;
				int pid = sd->pId;
				snprintf(spid,sizeof(spid), "TTX:%d:%03X:%s", sd->pId, page, sd->ISO639_language_code.c_str()); 
				char item[64];
				
				snprintf(item, sizeof(item), "TTX: %s", sd->ISO639_language_code.c_str());
				DVBSubSelector->addItem(new ClistBoxItem(item,  !tuxtx_subtitle_running(&pid, &page, NULL), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
		}
		
		if(sep_added) 
		{
			DVBSubSelector->addItem(new CMenuSeparator(LINE));
			DVBSubSelector->addItem(new ClistBoxItem(_("Stop subtitles"), true, NULL, &SubtitleChanger, "off", RC_red, NEUTRINO_ICON_BUTTON_RED ));
		}
		else
			DVBSubSelector->addItem(new ClistBoxItem(_("Subtitles not found"), false));
	}

	res = widget->exec(NULL, "");

#ifdef TESTING
	delete DVBSubSelector;
	delete widget;
#endif
	
	return res;
}

