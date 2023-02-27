/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: epgview.cpp 15.01.2019 mohousch Exp $

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

#include <unistd.h>

#include <algorithm>
#include <gui/epgview.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/mountchooser.h>
#include <gui/timerlist.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/encoding.h>

#include <gui/filebrowser.h>
#include <gui/pictureviewer.h>

#include <system/debug.h>
#include <system/tmdbparser.h>


#define TIMESCALE_W	150
#define TIMESCALE_H	6

static CProgressBar * timescale;

int findItem(std::string strItem, std::vector<std::string> & vecItems) 
{
	for (std::vector<std::string>::size_type nCnt = 0; nCnt < vecItems.size(); nCnt++) 
	{
		std::string strThisItem = vecItems[nCnt];
		if (strItem == strThisItem) 
		{
			return nCnt;
		}
	}
	return -1;
}

// 21.07.2005 - rainerk
// Merge multiple extended event strings into one description and localize the label
// Examples:
//   Actor1-ActorX      -> Darsteller 1, 2, 3
//   Year of production -> Produktionsjahr
//   Director           -> Regisseur
//   Guests             -> Gäste
void reformatExtendedEvents(std::string strItem, std::string strLabel, bool bUseRange, CEPGData & epgdata) 
{
	std::vector<std::string> & vecDescriptions = epgdata.itemDescriptions;
	std::vector<std::string> & vecItems = epgdata.items;
	
	// Merge multiple events into 1 (Actor1-)
	if (bUseRange) 
	{
		bool bHasItems = false;
		char index[3];
		// Maximum of 10 items should suffice
		for (int nItem = 1; nItem < 11; nItem++) 
		{
			sprintf(index, "%d", nItem);
			// Look for matching items
			int nPos = findItem(std::string(strItem) + std::string(index), vecDescriptions);
			if (-1 != nPos) 
			{
				std::string item = std::string(vecItems[nPos]);
				vecDescriptions.erase(vecDescriptions.begin() + nPos);
				vecItems.erase(vecItems.begin() + nPos);
				if (false == bHasItems) 
				{
					// First item added, so create new label item
					vecDescriptions.push_back(strLabel);
					vecItems.push_back(item + ", ");
					bHasItems = true;
				} 
				else 
				{
					vecItems.back().append(item).append(", ");
				}
			}
		}
		
		// Remove superfluous ", "
		if (bHasItems) 
		{
			vecItems.back().resize(vecItems.back().length() - 2);
		}
	} 
	else 
	{	
		// Single string event (e.g. Director)
		// Look for matching items
		int nPos = findItem(strItem, vecDescriptions);
		
		if (-1 != nPos) 
		{
			vecDescriptions[nPos] = strLabel;
		}
	}
}

// foot
struct button_label FButtons[4] =
{
	{ DUMMY_ICON, " " },
	{ DUMMY_ICON,  " "},
	{ NEUTRINO_ICON_BUTTON_YELLOW, "Schedule"},
	{ DUMMY_ICON, " " }
};

CEpgData::CEpgData()
{
	frameBuffer = CFrameBuffer::getInstance();
	
	widget = NULL;
	textBox = NULL;
	headers = NULL;
	footers = NULL;
	cFollowScreeningWindow = NULL;
	audioIcon = NULL;
	
	//
	initFrames();
	
	//
	epgBuffer.clear();
	evtlist.clear();

	//
	widget = CNeutrinoApp::getInstance()->getWidget("epgview");
	
	if (widget)
	{
		textBox = (CTextBox*)widget->getWidgetItem(WIDGETITEM_TEXTBOX);
		headers = (CHeaders*)widget->getWidgetItem(WIDGETITEM_HEAD);
		footers = (CFooters*)widget->getWidgetItem(WIDGETITEM_FOOT);
		cFollowScreeningWindow = (CWindow*)widget->getWidgetItem(WIDGETITEM_WINDOW, "screening");
	}
	else
	{
		textBox = new CTextBox(&cTextBox);
		headers = new CHeaders(&cHeadBox);
		footers = new CFooters(&cFootBox);
		cFollowScreeningWindow = new CWindow(&cFollowScreeningBox);
		
		// head
		headers->enablePaintDate();
		headers->setFormat("%d.%m.%Y %H:%M:%S");
		
		// foot
		
		widget = new CWidget(&cFrameBox);
		widget->name = "epgview";
		
		widget->addWidgetItem(textBox);
		widget->addWidgetItem(headers);
		widget->addWidgetItem(footers);
		widget->addWidgetItem(cFollowScreeningWindow);
	}
	
	//
	if (cFollowScreeningWindow)
		cFollowScreeningBox = cFollowScreeningWindow->getWindowsPos();
		
	int timeScaleWidth = TIMESCALE_W;
	int timeScaleHeight = TIMESCALE_H;
	
	// recalculate timeScale width
	timeScaleWidth = cFollowScreeningBox.iWidth - 20 - 4*g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(">") - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("0000000000000") - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("0000000000");
	
	if (timeScaleWidth > TIMESCALE_W)
		timeScaleWidth = TIMESCALE_W;
		
	timescale = new CProgressBar(cFollowScreeningBox.iX + (cFollowScreeningBox.iWidth - timeScaleWidth)/2, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - timeScaleHeight)/2, timeScaleWidth, timeScaleHeight);
	
	epg_done = -1;

	//
	if (textBox) 
	{
		textBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
		textBox->setMode(SCROLL);
		textBox->enableSaveScreen();
	}
}

CEpgData::~CEpgData()
{
	epgBuffer.clear();
	evtlist.clear();
}

void CEpgData::initFrames()
{
	// mainBox	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 100;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 100;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;	

	// headBox
	cHeadBox.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getHeight() + 10;
	cHeadBox.iWidth = cFrameBox.iWidth;
	cHeadBox.iX = cFrameBox.iX;
	cHeadBox.iY = cFrameBox.iY;

	// foot
	cFootBox.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getHeight() + 10;
	cFootBox.iWidth = cFrameBox.iWidth;
	cFootBox.iX = cFrameBox.iX;
	cFootBox.iY = cFrameBox.iY + cFrameBox.iHeight - cFootBox.iHeight;

	// screening bar
	cFollowScreeningBox.iHeight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight() + 6;
	cFollowScreeningBox.iWidth = cFrameBox.iWidth;
	cFollowScreeningBox.iX = cFrameBox.iX;
	cFollowScreeningBox.iY = cFrameBox.iY + cFrameBox.iHeight - cFootBox.iHeight - cFollowScreeningBox.iHeight;

	// textBox
	cTextBox.iX = cFrameBox.iX;
	cTextBox.iY = cFrameBox.iY + cHeadBox.iHeight;
	cTextBox.iWidth = cFrameBox.iWidth;
	cTextBox.iHeight = cFrameBox.iHeight - cHeadBox.iHeight - cFootBox.iHeight - cFollowScreeningBox.iHeight;
}

#define GENRE_MOVIE_COUNT 9
const char* genre_movie[GENRE_MOVIE_COUNT] =
{
	_("movie / drama"),
	_("detective / thriller"),
	_("adventure / western/war"),
	_("science fiction / fantasy / horror"),
	_("comedy"),
	_("soap / melodrama / folkloric"),
	_("romance"),
	_("serious / classical / religious / historical movie / drama"),
	_("adult movie / drama")
};

#define GENRE_NEWS_COUNT 5
const char* genre_news[GENRE_NEWS_COUNT] =
{
	_("news"),
	_("news / weather report"),
	_("news magazine"),
	_("documentary"),
	_("discussion / interview/debate")
};

#define GENRE_SHOW_COUNT 4
const char* genre_show[GENRE_SHOW_COUNT] =
{
	_("Show / Gameshow"),
	_("game show / quiz / contest"),
	_("variety show"),
	_("talk show")
};

#define GENRE_SPORTS_COUNT 12
const char* genre_sports[GENRE_SPORTS_COUNT] =
{
	_("sports"),
	_("special events (Olympic Games,World Cup etc.)"),
	_("sports magazines"),
	_("football / soccer"),
	_("tennis / squash"),
	_("team sports (excluding football)"),
	_("athletics"),
	_("motor sports"),
	_("water sports"),
	_("winter sports"),
	_("equestrian"),
	_("martial sports"),
};

#define GENRE_CHILDRENS_PROGRAMMES_COUNT 6
const char* genre_childrens_programmes[GENRE_CHILDRENS_PROGRAMMES_COUNT] =
{
	_("children / juvenile program"),
	_("pre-school children's programmes"),
	_("entertainment programmes for 6 to 14"),
	_("entertainment programmes for 10 to 16"),
	_("informational / educational / school programmes"),
	_("cartoons / puppets"),
};

#define GENRE_MUSIC_DANCE_COUNT 7
const char* genre_music_dance[GENRE_MUSIC_DANCE_COUNT] =
{
	_("music / ballet / dance"),
	_("rock / pop"),
	_("serious music / classical music"),
	_("folk / traditional music"),
	_("jazz"),
	_("musical / opera"),
	_("ballet"),
};

#define GENRE_ARTS_COUNT 12
const char* genre_arts_dance[GENRE_ARTS_COUNT] =
{
	_("arts / culture"),
	_("performing arts"),
	_("fine arts"),
	_("religion"),
	_("popular culture / traditional arts"),
	_("literature"),
	_("film/cinema"),
	_("experimental film/video"),
	_("broadcasting / press"),
	_("new media"),
	_("arts/culture magazines"),
	_("fashion"),
};

#define GENRE_SOCIAL_POLITICAL_COUNT 4
const char* genre_social_political[GENRE_SOCIAL_POLITICAL_COUNT] =
{
	_("social & politic events / business"),
	_("magazines / reports / documentary"),
	_("economics / social advisory"),
	_("remarkable people"),
};

#define GENRE_DOCUS_MAGAZINES_COUNT 8
const char* genre_docus_magazines[GENRE_DOCUS_MAGAZINES_COUNT] =
{
	_("documentation / magazine"),
	_("nature/animals / environment"),
	_("technology/natural sciences"),
	_("medicine / physiology / psychology"),
	_("foreign countries/expeditions"),
	_("social / spiritual sciences"),
	_("further education"),
	_("languages"),
};

#define GENRE_TRAVEL_HOBBIES_COUNT 8
const char* genre_travel_hobbies[GENRE_TRAVEL_HOBBIES_COUNT] =
{
	_("travel & recreation"),
	_("tourism / travel"),
	_("tourism / travel"),
	_("handicraft"),
	_("motoring"),
	_("fitness & health"),
	_("cooking"),
	_("gardening")
};

const char genre_sub_classes[10] =
{
	GENRE_MOVIE_COUNT,
	GENRE_NEWS_COUNT,
	GENRE_SHOW_COUNT,
	GENRE_SPORTS_COUNT,
	GENRE_CHILDRENS_PROGRAMMES_COUNT,
	GENRE_MUSIC_DANCE_COUNT,
	GENRE_ARTS_COUNT,
	GENRE_SOCIAL_POLITICAL_COUNT,
	GENRE_DOCUS_MAGAZINES_COUNT,
	GENRE_TRAVEL_HOBBIES_COUNT
};

const char** genre_sub_classes_list[10] =
{
	genre_movie,
	genre_news,
	genre_show,
	genre_sports,
	genre_childrens_programmes,
	genre_music_dance,
	genre_arts_dance,
	genre_social_political,
	genre_docus_magazines,
	genre_travel_hobbies
};

bool CEpgData::hasFollowScreenings(const t_channel_id /*channel_id*/, const std::string & title) 
{
	time_t curtime = time(NULL);
	
	for (CChannelEventList::iterator e = evtlist.begin(); e != evtlist.end(); ++e )
	{
		if (e->startTime > curtime && e->eventID && e->description == title)
			return true;
	}

	return false;	
}

const char * GetGenre(const unsigned char contentClassification) // UTF-8
{
	const char* res = NULL;

	int i = (contentClassification & 0xF0);
	
	if ((i >= 0x010) && (i < 0x0B0))
	{
		i >>= 4;
		i--;

		res = genre_sub_classes_list[i][((contentClassification & 0x0F) < genre_sub_classes[i]) ? (contentClassification & 0x0F) : 0];
	}
	else
		res = _("unknown");

	return res;
}

static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

extern char recDir[255];// defined in neutrino.cpp

//
void CEpgData::showHead(const t_channel_id channel_id)
{
	std::string text1 = epgData.title;

	std::string logo;

	logo = CChannellogo::getInstance()->getLogoName(channel_id);

	//
	if (headers)
	{
		headers->clear();
		
		headers->setTitle(text1.c_str());
		headers->setIcon(logo.c_str());
	}
}

int CEpgData::show(const t_channel_id channel_id, uint64_t a_id, time_t * a_startzeit, bool doLoop )
{
	dprintf(DEBUG_NORMAL, "CEpgData::show: %llx\n", channel_id);

	int res = RETURN_REPAINT;
	static uint64_t id;
	static time_t startzeit;
	 
	if(a_startzeit)
		startzeit = *a_startzeit;
	
	id = a_id;

	// getepg data
	GetEPGData(channel_id, id, &startzeit);
	
	if (doLoop)
	{
		evtlist.clear();
		
		CSectionsd::getInstance()->getEventsServiceKey(channel_id & 0xFFFFFFFFFFFFULL, evtlist);
		
		// Houdini added for Private Premiere EPG start sorted by start date/time 2005-08-15
  		sort(evtlist.begin(), evtlist.end(), sortByDateTime);
	}

	// 21.07.2005 - rainerk
	// Only show info1 if it's not included in info2!
	std::string strEpisode = "";	// Episode title in case info1 gets stripped
	if (!epgData.info1.empty()) 
	{
		bool bHide = false;
		if (false == epgData.info2.empty()) 
		{
			// Look for the first . in info1, usually this is the title of an episode.
			std::string::size_type nPosDot = epgData.info1.find('.');
			if (std::string::npos != nPosDot) 
			{
				nPosDot += 2; // Skip dot and first blank
				
				/*
				Houdini: changed for safety reason (crashes with some events at WDR regional)
				if (nPosDot < epgData.info2.length()) {   // Make sure we don't overrun the buffer
				*/
				if (nPosDot < epgData.info2.length() && nPosDot < epgData.info1.length()) 
				{   
					// Make sure we don't overrun the buffer

					// Check if the stuff after the dot equals the beginning of info2
					if (0 == epgData.info2.find(epgData.info1.substr(nPosDot, epgData.info1.length() - nPosDot))) 
					{
						strEpisode = epgData.info1.substr(0, nPosDot) + "\n";
						bHide = true;
					}
				}
			}
			// Compare strings normally if not positively found to be equal before			
			if (false == bHide && false == (std::string::npos == epgData.info2.find(epgData.info1))) 
			{
				bHide = true;
			}
		}
		
		if (false == bHide) 
		{
			epgBuffer = epgData.info1;
			epgBuffer += "\n";
		}
	}

	//scan epg-data - sort to list
	if (epgData.info2.empty())
	{
		epgBuffer += _("No detailed informations available");
		epgBuffer += "\n";
	}
	else
	{
		epgBuffer += strEpisode;
		epgBuffer += epgData.info2;
		epgBuffer += "\n";
	}
	
	epgBuffer += "\n";

	// 21.07.2005 - rainerk
	// extended information
	if(0 != epgData.itemDescriptions.size() && 0 != epgData.items.size()) 
	{
		char line[256];
		std::vector<std::string>::iterator description;
		std::vector<std::string>::iterator item;

		epgBuffer += "\n";
		
		for (description = epgData.itemDescriptions.begin(), item = epgData.items.begin(); description != epgData.itemDescriptions.end(), item != epgData.items.end(); ++description, ++item) 
		{
			sprintf(line, "%s: %s", (*(description)).c_str(), (*(item)).c_str());

			epgBuffer += line;
			epgBuffer += "\n";
			
		}
	}

	// fsk
	if (epgData.fsk > 0)
	{
		char _tfsk[11];
		sprintf (_tfsk, "FSK: ab %d", epgData.fsk );

		epgBuffer += _tfsk;
		epgBuffer += "\n";
	}
	
	// length information
	char lengthInfo[11];
	sprintf(lengthInfo, "%d", epgData.epg_times.dauer / 60);
	//epgBuffer += _("length: ");
	epgBuffer += lengthInfo;
	epgBuffer += " ";
	epgBuffer += _("Minutes");
	epgBuffer += "\n";
	
	// genre:
	if (epgData.contentClassification.length() > 0)
	{
		epgBuffer += GetGenre(epgData.contentClassification[0]);
		epgBuffer += "\n";
	}

	// -- display more screenings on the same channel
	// -- 2002-05-03 rasc
	if (hasFollowScreenings(channel_id, epgData.title)) 
	{
		epgBuffer += _("More Screenings on this Channel");
		epgBuffer += ":";

		FollowScreenings(channel_id, epgData.title);
	}

	// head
	showHead(channel_id);
	
	std::string fromto;
	int widthl, widthr;
	fromto = epg_start;
	fromto += " - ";
	fromto += epg_end;

	if(textBox)
	{
		textBox->setText(epgBuffer.c_str());
	}

	// show Timer Event Buttons
	showTimerEventBar(true);

	//
	widget->paint();
	
	//// FIXME: use CCLabel/CCText
	//from-to
	widthl = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(fromto);
	
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("<") + 5, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), widthl, fromto, COL_MENUHEAD);
	
	//epg-date
	widthr = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(epg_date);
	
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + cFollowScreeningBox.iWidth - 10 - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(">") - 5 - widthr, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), widthr, epg_date, COL_MENUHEAD);
	
	//show Content&Component for Dolby & 16:9 //FIXME:
        CSectionsd::ComponentTagList tags;
        bool have_16_9 = false;
        tags.clear();
	
	if ( CSectionsd::getInstance()->getComponentTagsUniqueKey( epgData.eventID, tags ) )
        {
                for (unsigned int i = 0; i < tags.size(); i++)
                {
			int icon_w_aspect = 0;
			int icon_h_aspect = 0;
			int icon_w_dd = 0;
			int icon_h_dd = 0;
			
			frameBuffer->getIconSize(NEUTRINO_ICON_16_9, &icon_w_aspect, &icon_h_aspect);
			frameBuffer->getIconSize(NEUTRINO_ICON_DD, &icon_w_dd, &icon_h_dd);
			
			//FIXME:
                        if( tags[i].streamContent == 1 && (tags[i].componentType == 2 || tags[i].componentType == 3) )
                        {
                        	have_16_9 = true;
                        	
                                frameBuffer->paintIcon(NEUTRINO_ICON_16_9, cFollowScreeningBox.iX + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("<") + 5 + widthl + 5, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - icon_h_aspect)/2);
                        }
                        
                        if( tags[i].streamContent == 2 && tags[i].componentType == 5 )
                        {
                               frameBuffer->paintIcon(NEUTRINO_ICON_DD, cFollowScreeningBox.iX + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("<") + 5 + widthl + 5 + have_16_9? icon_w_aspect + 5 : 0, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - icon_h_dd)/2);
                        }
                }
        }


	//show progressbar
	if ( epg_done != -1 )
	{
		timescale->reset();
		timescale->paint(epg_done);
	}

	// get prevnext epg data
	GetPrevNextEPGData( epgData.eventID, &epgData.epg_times.startzeit );

	// prev <<<<
	if (prev_id != 0)
	{
		//cLeftFWindow.setColor(COL_MENUCONTENT_PLUS_3);
		//cLeftFWindow.paint();

		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + 10, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth("<"), "<", COL_MENUCONTENT + 3);
	}

	// next >>>>
	if (next_id != 0)
	{
		//cRightFWindow.setColor(COL_MENUCONTENT_PLUS_3);
		//cRightFWindow.paint();

		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + cFollowScreeningBox.iWidth - 10 - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(">"), cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(">"), ">", COL_MENUCONTENT + 3);
	}
	
	// blit
	frameBuffer->blit();

	if ( doLoop )
	{
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;

		// add sec timer
		sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

		bool loop = true;

		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		while(loop)
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

			if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
			{
				// head
				widget->refresh();

				//
				GetEPGData(channel_id, id, &startzeit, false);
				
				if ( epg_done!= -1 ) 
				{
					timescale->reset();
					timescale->paint(epg_done);
				}
			} 

			switch ( msg )
			{					
				case NeutrinoMessages::EVT_CURRENTNEXT_EPG:
					if (((*(t_channel_id *) data) == (channel_id /*& 0xFFFFFFFFFFFFULL*/))) 
					{
						show(channel_id);
					}
					CNeutrinoApp::getInstance()->handleMsg(msg, data);
					break;
					
				case RC_left:
					if (prev_id != 0)
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + 10, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), widthr, "<", COL_MENUCONTENT + 3);

						show(channel_id, prev_id, &prev_zeit, false);
					}
					break;

				case RC_right:
					if (next_id != 0)
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(cFollowScreeningBox.iX + cFollowScreeningBox.iWidth - 10 - widthr, cFollowScreeningBox.iY + (cFollowScreeningBox.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight(), widthr, ">", COL_MENUCONTENT + 3);

						show(channel_id, next_id, &next_zeit, false);
					}
					break;

				case RC_down:
				case RC_page_down:
					if(textBox)
						textBox->scrollPageDown(1);
					break;

				case RC_up:
				case RC_page_up:
					if(textBox)
						textBox->scrollPageUp(1);
					break;

				// 31.05.2002 dirch		record timer
				case RC_red:
					if (recDir != NULL)
					{
						if(CTimerd::getInstance()->isTimerdAvailable())
						{
							bool doRecord = true;
							strcpy(recDir, g_settings.network_nfs_recordingdir);
							
							if (doRecord)
							{
								if (CTimerd::getInstance()->addRecordTimerEvent(channel_id, epgData.epg_times.startzeit, epgData.epg_times.startzeit + epgData.epg_times.dauer, epgData.eventID, epgData.epg_times.startzeit, epgData.epg_times.startzeit - (ANNOUNCETIME + 120 ), TIMERD_APIDS_CONF, true, recDir,false) == -1)
								{
									if(askUserOnTimerConflict(epgData.epg_times.startzeit - (ANNOUNCETIME + 120), epgData.epg_times.startzeit + epgData.epg_times.dauer))
									{
										CTimerd::getInstance()->addRecordTimerEvent(channel_id, epgData.epg_times.startzeit, epgData.epg_times.startzeit + epgData.epg_times.dauer, epgData.eventID, epgData.epg_times.startzeit, epgData.epg_times.startzeit - (ANNOUNCETIME + 120 ), TIMERD_APIDS_CONF, true, recDir,true);
														 
										MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
									}
								} 
								else 
								{
									MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
								}
							}
						}
						else
							printf("timerd not available\n");
					}
					break;

				// 31.05.2002 dirch		zapto timer
				case RC_yellow:
				{
					{
						if(CTimerd::getInstance()->isTimerdAvailable())
						{
							CTimerd::getInstance()->addZaptoTimerEvent(channel_id, epgData.epg_times.startzeit, epgData.epg_times.startzeit - ANNOUNCETIME, 0, epgData.eventID, epgData.epg_times.startzeit, 0);
										
							MessageBox(_("Schedule Event"), _("The event is scheduled.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO, MENU_WIDTH, -1, false, BORDER_ALL);
						}
						else
							printf("timerd not available\n");
					}
					break;
				}
				
				case RC_info:
					//if(textBox)
					//	textBox->setBigFonts();
					break;

				case RC_ok:
				case RC_timeout:
					loop = false;
					break;
					
				case RC_favorites:
				case RC_sat:
					g_RCInput->postMsg (msg, 0);
					loop = false;
					break;

				default:
					// konfigurierbare Keys handlen...
					if (msg == (neutrino_msg_t)g_settings.key_channelList_cancel)
						loop = false;
					else
					{
						if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
						{
							loop = false;
							res = RETURN_EXIT_ALL;
						}
					}
			}

			frameBuffer->blit();	
		}
		
		hide();

		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
	
	return res;
}

void CEpgData::hide()
{
	widget->hide();

	//
	epgBuffer.clear();
}

void CEpgData::GetEPGData(const t_channel_id channel_id, uint64_t id, time_t* startzeit, bool clear)
{
	if(clear)
	{
		epgBuffer.clear();
		epgData.title.clear();
	}

	bool res;
	if ( id != 0 )
		res = CSectionsd::getInstance()->getEPGid(id, *startzeit, &epgData);
	else
		res = CSectionsd::getInstance()->getActualEPGServiceKey(channel_id & 0xFFFFFFFFFFFFULL, &epgData);

	if ( res )
	{
		// If we have items, merge and localize them (e.g. actor1, actor2, ... -> Actors)
		if (false == epgData.itemDescriptions.empty()) 
		{
			reformatExtendedEvents("Year of production", _("Year of production"), false, epgData);
			reformatExtendedEvents("Original title", _("Original title"), false, epgData);
			reformatExtendedEvents("Director", _("Director"), false, epgData);
			reformatExtendedEvents("Actor", _("Actors"), true, epgData);
			reformatExtendedEvents("Guests", _("Guests"), false, epgData);
			reformatExtendedEvents("Presenter", _("Presenter"), false, epgData);
		}
		
		struct tm *pStartZeit = localtime(&(epgData.epg_times).startzeit);
		char temp[11];
		strftime( temp, sizeof(temp), "%d.%m.%Y", pStartZeit);
		epg_date= temp;
		strftime( temp, sizeof(temp), "%H:%M", pStartZeit);
		epg_start= temp;

		long int uiEndTime((epgData.epg_times).startzeit + (epgData.epg_times).dauer);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
		strftime( temp, sizeof(temp), "%H:%M", pEndeZeit);
		epg_end = temp;

		epg_done = -1;
		
		if (( time(NULL)- (epgData.epg_times).startzeit )>= 0 )
		{
			unsigned nProcentagePassed=(unsigned)((float)(time(NULL) -(epgData.epg_times).startzeit)/(float)(epgData.epg_times).dauer*100.);
			if (nProcentagePassed <= 100)
				epg_done = nProcentagePassed;
		}
	}
}

void CEpgData::GetPrevNextEPGData(uint64_t id, time_t* startzeit)
{
        prev_id = 0;
        next_id = 0;
        unsigned int i;

        for ( i = 0; i < evtlist.size(); i++ )
        {
                if ( ( evtlist[i].eventID == id ) && ( evtlist[i].startTime == *startzeit ) )
                {
                        if ( i > 0 )
                        {
                                prev_id = evtlist[i - 1].eventID;
                                prev_zeit = evtlist[i - 1].startTime;
                        }
                        
                        if ( i < ( evtlist.size()- 1 ) )
                        {
                                next_id = evtlist[i + 1].eventID;
                                next_zeit = evtlist[i + 1].startTime;
                        }
                        break;
                }
        }
}

//
// -- get following screenings of this program title
// -- yek! a better class design would be more helpfull
// -- BAD THING: Cross channel screenings will not be shown
// --            $$$TODO
// -- 2002-05-03 rasc
//

int CEpgData::FollowScreenings(const t_channel_id /*channel_id*/, const std::string & title)
{
	CChannelEventList::iterator e;
	time_t			curtime;
	struct  tm		*tmStartZeit;
	std::string		screening_dates,screening_nodual;
	int			count;
	char			tmpstr[256];

  	count = 0;
	screening_dates = screening_nodual = "";
    	curtime = time(NULL);

	for ( e = evtlist.begin(); e != evtlist.end(); ++e )
	{
	    	if (e->startTime <= curtime) 
			continue;
		if (! e->eventID) 
			continue;
		if (e->description == title) 
		{
			count++;
			tmStartZeit = localtime(&(e->startTime));

			screening_dates = "    ";

			strftime(tmpstr, sizeof(tmpstr), "%A", tmStartZeit);
			screening_dates += _(tmpstr);

			strftime(tmpstr, sizeof(tmpstr), "  %d.%m.%Y", tmStartZeit );
			screening_dates += tmpstr;

			strftime(tmpstr, sizeof(tmpstr), "  %H:%M", tmStartZeit );
			screening_dates += tmpstr;

			if (screening_dates != screening_nodual)
			{
				screening_nodual = screening_dates;

				epgBuffer += "\n";
				epgBuffer += screening_dates;
			}
		}
	}
		
	return count;
}

// foot
void CEpgData::showTimerEventBar(bool _show)
{
	if (recDir != NULL)
	{
		FButtons[0].button = NEUTRINO_ICON_BUTTON_RED;
		FButtons[0].localename = _("record");
	}

	//
	if (footers)
	{
		footers->clear();
		footers->setButtons(FButtons, 4);
	}
}

//  -- EPG Data Viewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)

int CEPGDataHandler::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CEPGDataHandler::exec:\n");

	int res = RETURN_REPAINT;
	CChannelList* channelList;
	CEpgData* e;

	if (parent) 
		parent->hide();

	e = new CEpgData;

	channelList = CNeutrinoApp::getInstance()->channelList;

	e->show(channelList->getActiveChannel_EPGID());
	
	delete e;

	return res;
}

