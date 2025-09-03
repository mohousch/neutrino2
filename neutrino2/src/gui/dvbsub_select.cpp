//
//	Neutrino-GUI  -   DBoxII-Project
//	
//	$Id: dvb_select.cpp 21122024 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <global.h>
#include <neutrino2.h>

#include <gui/widget/icons.h>

#include <gui/dvbsub_select.h>
#include <gui/audio_select.h>

#include <system/debug.h>


// dvb
extern int dvbsub_getpid();				// defined in libdvbsub
// tuxtxt
extern int tuxtx_subtitle_running(int *pid, int *page, int *running);

int CDVBSubSelectMenuHandler::exec(CWidgetTarget * parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CDVBSubSelectMenuHandler::exec:\n");

	int res = CWidgetTarget::RETURN_EXIT_ALL;

	if (parent) 
		parent->hide();

	doMenu();

	return res;
}

int CDVBSubSelectMenuHandler::doMenu()
{
	int res = CWidgetTarget::RETURN_EXIT_ALL;
	
	dprintf(DEBUG_NORMAL, "CDVBSubSelectMenuHandler::doMenu:\n");

	//
	CWidget* widget = NULL;
	ClistBox* DVBSubSelector = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("subselect");
	
	if (widget)
	{
		DVBSubSelector = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "subselect";
		
		//			
		DVBSubSelector = new ClistBox(&box);

		DVBSubSelector->setWidgetMode(ClistBox::MODE_SETUP);
		DVBSubSelector->enableShrinkMenu();
		
		//				
		DVBSubSelector->enablePaintHead();
		DVBSubSelector->setTitle(_("Subtitle Select"), NEUTRINO_ICON_SUBT);

		//
		DVBSubSelector->enablePaintFoot();					
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };					
		DVBSubSelector->setFootButtons(&btn);
						
		//
		widget->addCCItem(DVBSubSelector);
	}
	
	CSubtitleChangeExec SubtitleChanger;
	unsigned int count;

	//dvb/tuxtxt subs
	CZapitChannel * cc = CZapit::getInstance()->getCurrentChannel();

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
				snprintf(spid, sizeof(spid), "DVB:%d", sd->pId);
				char item[64];
	
				snprintf(item, sizeof(item), "DVB: %s", sd->ISO639_language_code.c_str());
				DVBSubSelector->addItem(new CMenuForwarder(item, sd->pId != dvbsub_getpid(), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
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
				DVBSubSelector->addItem(new CMenuForwarder(item,  !tuxtx_subtitle_running(&pid, &page, NULL), NULL, &SubtitleChanger, spid, CRCInput::convertDigitToKey(++count)));
			}
		}
		
		if(sep_added) 
		{
			DVBSubSelector->addItem(new CMenuSeparator(CMenuSeparator::LINE, NULL, true));
			DVBSubSelector->addItem(new CMenuForwarder(_("Stop subtitles"), true, NULL, &SubtitleChanger, "off", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
		}
		else
			DVBSubSelector->addItem(new CMenuForwarder(_("Subtitles not found"), false));
	}

	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	return res;
}

