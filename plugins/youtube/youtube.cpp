/* 
  $Id: youtube.cpp 2018/07/11 mohousch Exp $

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

#include <youtube.h>

extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

#define NEUTRINO_ICON_YT				PLUGINDIR "/youtube/youtube.png"
#define NEUTRINO_ICON_YT_SMALL			PLUGINDIR "/youtube/youtube_small.png"

YTB_SETTINGS m_settings;

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};
 
CYTBrowser::CYTBrowser(int mode): configfile ('\t')
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	moviesMenu = NULL;
	item = NULL;

	init();

	ytmode = mode;
}

CYTBrowser::~CYTBrowser()
{
	dprintf(DEBUG_NORMAL, "CYTBrowser: del\n");
	
	m_vMovieInfo.clear();

	ytparser.Cleanup();

	saveSettings(&m_settings);
	ytsearch.clear();
}

void CYTBrowser::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

void CYTBrowser::init(void)
{
	dprintf(DEBUG_NORMAL, "CYTBrowser::init\n");
	
	loadSettings(&m_settings);
}

bool CYTBrowser::loadSettings(YTB_SETTINGS *settings)
{
	bool result = true;
	
	dprintf(DEBUG_NORMAL, "CYTBrowser::loadSettings\r\n");
	
	if(configfile.loadConfig(YTBROWSER_SETTINGS_FILE))
	{
		settings->ytorderby = configfile.getInt32("ytorderby", cYTFeedParser::ORDERBY_PUBLISHED);
		settings->ytregion = configfile.getString("ytregion", "default");
		settings->ytautoplay = configfile.getInt32("ytautoplay", 0);
	}
	else
	{
		dprintf(DEBUG_NORMAL, "CYTBrowser::loadSettings failed\r\n"); 
		configfile.clear();
		result = false;
	}
	
	return (result);
}

bool CYTBrowser::saveSettings(YTB_SETTINGS *settings)
{
	bool result = true;
	dprintf(DEBUG_NORMAL, "CYTBrowser::saveSettings\r\n");

	configfile.setInt32("ytorderby", settings->ytorderby);
	configfile.setString("ytregion", settings->ytregion);
	configfile.setInt32("ytautoplay", settings->ytautoplay);
 
 	if (configfile.getModifiedFlag())
		configfile.saveConfig(YTBROWSER_SETTINGS_FILE);
	
	return (result);
}

//
#define YT_HEAD_BUTTONS_COUNT	2
const struct button_label YTHeadButtons[YT_HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " }
};

#define YT_FOOT_BUTTONS_COUNT  4
const struct button_label YTFootButtons[YT_FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("next results") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("prev results") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, " " },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("most popular")}
};

void CYTBrowser::showMenu()
{
	dprintf(DEBUG_NORMAL, "CYTBrowser::showMenu:\n");

	//
	std::string title;
	title = _("Youtube Player");
	title += " : ";
		
	title += getFeedLocale();

	//
	moviesMenu = new CMenuWidget();
	
	std::string itemTitle;

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		itemTitle = m_vMovieInfo[i].epgTitle + " (" + to_string(m_vMovieInfo[i].length) + " Min)";

		item = new ClistBoxItem(itemTitle.c_str(), true, NULL, this, "play", RC_nokey, NULL,  file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->setHint(m_vMovieInfo[i].epgInfo2.c_str());
 
		moviesMenu->addItem(item);
	}

	//
	moviesMenu->setWidgetMode(MODE_LISTBOX);
	moviesMenu->setWidgetType(TYPE_FRAME);
	moviesMenu->setItemsPerPage(3, 2);

	//
	moviesMenu->setTitle(title.c_str(), NEUTRINO_ICON_YT_SMALL);
	moviesMenu->enablePaintDate();
	moviesMenu->setHeadButtons(YTHeadButtons, YT_HEAD_BUTTONS_COUNT);
	
	//
	moviesMenu->setFootButtons(YTFootButtons, YT_FOOT_BUTTONS_COUNT);
	
	//
	moviesMenu->enablePaintItemInfo();

	moviesMenu->addKey(RC_info, this, CRCInput::getSpecialKeyName(RC_info));
	moviesMenu->addKey(RC_setup, this, CRCInput::getSpecialKeyName(RC_setup));
	moviesMenu->addKey(RC_red, this, CRCInput::getSpecialKeyName(RC_red));
	moviesMenu->addKey(RC_green, this, CRCInput::getSpecialKeyName(RC_green));
	moviesMenu->addKey(RC_blue, this, CRCInput::getSpecialKeyName(RC_blue));
	moviesMenu->addKey(RC_record, this, CRCInput::getSpecialKeyName(RC_record));

	moviesMenu->exec(NULL, "");
	
	delete moviesMenu;
	moviesMenu = NULL;
}

void CYTBrowser::playMovie(void)
{
	if (m_vMovieInfo.empty())
		return;
		
	if(m_settings.ytautoplay)
	{
		// add selected video
		tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[moviesMenu->getSelected()]);

		// get related videos
		loadYTTitles(cYTFeedParser::RELATED, "", m_vMovieInfo[moviesMenu->getSelected()].ytid, false);

		for(int i = 0; i < m_vMovieInfo.size(); i++)
		{
			if (&m_vMovieInfo[i].file != NULL) 
				tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[i]);
		}

		tmpMoviePlayerGui.exec(NULL, "");
	}
	else
	{
		if (&m_vMovieInfo[moviesMenu->getSelected()].file != NULL) 
		{
			tmpMoviePlayerGui.addToPlaylist(m_vMovieInfo[moviesMenu->getSelected()]);
			tmpMoviePlayerGui.exec(NULL, "");
		}
	}
}

void CYTBrowser::showMovieInfo(void)
{
	if (m_vMovieInfo.empty())
		return;
		
	m_movieInfo.showMovieInfo(m_vMovieInfo[moviesMenu->getSelected()]);
}

void CYTBrowser::recordMovie(void)
{
	if (m_vMovieInfo.empty())
		return;
		
	::start_file_recording(m_vMovieInfo[moviesMenu->getSelected()].epgTitle.c_str(), m_vMovieInfo[moviesMenu->getSelected()].epgInfo2.c_str(), m_vMovieInfo[moviesMenu->getSelected()].file.Name.c_str());
}

void CYTBrowser::loadYTTitles(int mode, std::string search, std::string id, bool show_hint)
{
	dprintf(DEBUG_NORMAL, "CYTBrowser::loadYTTitles: parsed %d old mode %d new mode %d region %s\n", ytparser.Parsed(), ytparser.GetFeedMode(), ytmode, m_settings.ytregion.c_str());

	CHintBox loadBox(_("Youtube Player"), _("Scan for Movies ..."));

	if(show_hint)
		loadBox.paint();

	//
	ytparser.Cleanup();
	
	if (m_settings.ytregion == "default")
		ytparser.SetRegion("");
	else
		ytparser.SetRegion(m_settings.ytregion);

REPEAT:
	if (!ytparser.Parsed() || (ytparser.GetFeedMode() != mode)) 
	{
	
		if (ytparser.ParseFeed((cYTFeedParser::yt_feed_mode_t)mode, search, id, (cYTFeedParser::yt_feed_orderby_t)m_settings.ytorderby))
		{
			ytparser.DownloadThumbnails();
		} 
		else 
		{
			//FIXME show error
			MessageBox(_("Error"), _("Failed to load youtube feed"), mbrCancel, mbCancel, NEUTRINO_ICON_ERROR);
			
			//return;
			if(mode == cYTFeedParser::PREV)
			{
				mode = ytparser.GetFeedMode();
				goto REPEAT;
			}
		}
	}
	
	m_vMovieInfo.clear();
	yt_video_list_t &ylist = ytparser.GetVideoList();
	
	for (unsigned i = 0; i < ylist.size(); i++) 
	{
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
		
		movieInfo.epgChannel = ylist[i].author;
		movieInfo.epgTitle = ylist[i].title;
		movieInfo.epgInfo1 = ylist[i].category;
		movieInfo.epgInfo2 = ylist[i].description;
		movieInfo.length = ylist[i].duration/60 ;
		movieInfo.tfile = ylist[i].tfile;
		movieInfo.ytdate = ylist[i].published;
		movieInfo.ytid = ylist[i].id;
		movieInfo.file.Name = ylist[i].GetUrl();
		
		m_vMovieInfo.push_back(movieInfo);
	}

	if(show_hint)
		loadBox.hide();
}

const keyval YT_FEED_OPTIONS[] =
{
       { cYTFeedParser::MOST_POPULAR, _("Most popular today") },
};

#define YT_FEED_OPTION_COUNT (sizeof(YT_FEED_OPTIONS)/sizeof(keyval))

const keyval YT_ORDERBY_OPTIONS[] =
{
        { cYTFeedParser::ORDERBY_PUBLISHED, _("publishing date") },
        { cYTFeedParser::ORDERBY_RELEVANCE, _("relevance") },
        { cYTFeedParser::ORDERBY_VIEWCOUNT, _("view count") },
        { cYTFeedParser::ORDERBY_RATING, _("rating") },
};

#define YT_ORDERBY_OPTION_COUNT (sizeof(YT_ORDERBY_OPTIONS)/sizeof(keyval))

std::string CYTBrowser::getFeedLocale(void)
{
	std::string ret = "Most popular today";

	if (ytmode == cYTFeedParser::RELATED)
		ret = "Related videos";

	if (ytmode == cYTFeedParser::SEARCH)
		ret = "Search keyword";

	for (unsigned i = 0; i < YT_FEED_OPTION_COUNT; i++) 
	{
		if (ytmode == YT_FEED_OPTIONS[i].key)
			ret = YT_FEED_OPTIONS[i].valname;
	}
	
	return ret;
}

int CYTBrowser::showCategoriesMenu(void)
{
	dprintf(DEBUG_NORMAL, "CYTBrowser::showCategoriesMenu:");

	int res = -1;

	CMenuWidget mainMenu(_("Youtube Player"), NEUTRINO_ICON_YT_SMALL);

	mainMenu.enableSaveScreen();
	mainMenu.setWidgetMode(MODE_MENU);
	mainMenu.enableShrinkMenu();

	mainMenu.addItem(new ClistBoxItem(_("Most popular today"), true, NULL, new CYTBrowser(cYTFeedParser::MOST_POPULAR), NULL));

	mainMenu.addItem(new CMenuSeparator(LINE));
	
	// search
	mainMenu.addItem(new ClistBoxItem(_("Search keyword"), true, ytsearch.c_str(), this, "search", RC_red, NEUTRINO_ICON_BUTTON_RED));
	
	// ytorder
	mainMenu.addItem(new CMenuOptionChooser(_("Order by"), &m_settings.ytorderby, YT_ORDERBY_OPTIONS, YT_ORDERBY_OPTION_COUNT, true, NULL, RC_green, NEUTRINO_ICON_BUTTON_GREEN, true));

	mainMenu.addItem(new CMenuSeparator(LINE));

	char rstr[20];
	sprintf(rstr, "%s", m_settings.ytregion.c_str());
	CMenuOptionStringChooser * region = new CMenuOptionStringChooser(_("Region"), rstr, true, NULL, RC_blue, NEUTRINO_ICON_BUTTON_BLUE, true);
	region->addOption("default");
	region->addOption("DE");
	region->addOption("PL");
	region->addOption("RU");
	region->addOption("NL");
	region->addOption("CZ");
	region->addOption("FR");
	region->addOption("HU");
	region->addOption("US");
	mainMenu.addItem(region);

	mainMenu.addItem(new CMenuSeparator(LINE));

	// autoplay
	mainMenu.addItem(new CMenuOptionChooser(_("Auto Play"), &m_settings.ytautoplay, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	mainMenu.exec(NULL, "");
	
	if(rstr != m_settings.ytregion) 
	{
		m_settings.ytregion = rstr;
	}

	res = mainMenu.getSelected();

	return res;
}

int CYTBrowser::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CYTBrowser::exec: actionKey:%s\n", actionKey.c_str());

	if(parent) 
		parent->hide();

	if(actionKey == "play")
	{
		playMovie();

		if(m_settings.ytautoplay)
		{
			showMenu();
			return RETURN_EXIT_ALL;
		}
		else
			return RETURN_REPAINT;
	}
	else if(actionKey == "RC_info")
	{
		showMovieInfo();

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_setup")
	{
		if (m_vMovieInfo.empty())
			return RETURN_REPAINT;
		
		int res = showCategoriesMenu();

		if( res >= 0 && res <= 6)
		{
			showMenu();
			return RETURN_EXIT_ALL;
		}
		else
			return RETURN_REPAINT;
	}
	else if(actionKey == "RC_blue")
	{
		ytvid = m_vMovieInfo[moviesMenu->getSelected()].ytid;
		ytmode = cYTFeedParser::RELATED;

		loadYTTitles(ytmode, ytsearch, ytvid);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_red")
	{
		ytvid = m_vMovieInfo[moviesMenu->getSelected()].ytid;
		ytmode = cYTFeedParser::NEXT;

		loadYTTitles(ytmode, ytsearch, ytvid);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		ytvid = m_vMovieInfo[moviesMenu->getSelected()].ytid;
		ytmode = cYTFeedParser::PREV;

		loadYTTitles(ytmode, ytsearch, ytvid);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_record")
	{
		recordMovie();
		return RETURN_REPAINT;
	}
	else if(actionKey == "search")
	{
		ytmode = cYTFeedParser::SEARCH;

		CStringInputSMS stringInput(_("Search"), ytsearch.c_str());
		int ret = stringInput.exec(NULL, "");

		if(!stringInput.getExitPressed() /*&& !ytsearch.empty()*/) //FIXME:
		{
			loadYTTitles(ytmode, ytsearch, ytvid);
			showMenu();

			return RETURN_EXIT_ALL;
		}
		else
			return RETURN_REPAINT;
	}

	loadYTTitles(ytmode, ytsearch, ytvid);
	showMenu();
	
	return RETURN_EXIT;
}

//
void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CYTBrowser* YTHandler = new CYTBrowser(cYTFeedParser::MOST_POPULAR);
	
	YTHandler->exec(NULL, "");
	
	delete YTHandler;
	YTHandler = NULL;		
}


