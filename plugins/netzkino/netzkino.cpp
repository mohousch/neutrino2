 /*
  $Id: netzkino.cpp 2016.02.13 mohousch Exp $

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

#include <netzkino.h>

extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);

#define NEUTRINO_ICON_NETZKINO			PLUGINDIR "/netzkino/netzkino.png"
#define NEUTRINO_ICON_NETZKINO_SMALL		PLUGINDIR "/netzkino/netzkino_small.png"
  
//
CNKMovies::CNKMovies(int mode, int id, std::string title)
{
	dprintf(DEBUG_NORMAL, "CNKMovies: mode:%d id:%d title:%s\n", mode, id, title.c_str());

	//
	mainWidget = NULL;
	leftWidget = NULL;
	rightWidget = NULL;
	headersWidget = NULL;
	footersWidget = NULL;

	item = NULL;

	//
	selected = 0;
	left_selected = 0;
	right_selected = 0;

	catMode = mode;
	catID = id;
	caption = title;
}

CNKMovies::~CNKMovies()
{
	dprintf(DEBUG_NORMAL, "~CNKMovies:\n");

	m_vMovieInfo.clear();
	nkparser.Cleanup();
	nksearch.clear();
}

void CNKMovies::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

void CNKMovies::loadNKTitles(int mode, std::string search, int id)
{
	dprintf(DEBUG_NORMAL, "CNKMovies::loadNKTitles: (mode:%d) search:%s (id:%d)\n", mode, search.c_str(), id);

	CHintBox loadBox(_("Netzkino"), _("Scan for Categories"));
	loadBox.paint();

	nkparser.Cleanup();

	//
	if (nkparser.ParseFeed((cNKFeedParser::nk_feed_mode_t)mode, search, id)) 
	{
		nkparser.DownloadThumbnails();
	} 
	else 
	{
		//FIXME show error
		MessageBox(_("Error"), _("Failed to load Netzkino Movies"), CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);

		loadBox.hide();
	}
	
	m_vMovieInfo.clear();
	nk_video_list_t &ylist = nkparser.GetVideoList();
	
	for (unsigned int count = 0; count < ylist.size(); count++) 
	{
		MI_MOVIE_INFO movieInfo;
		m_movieInfo.clearMovieInfo(&movieInfo); // refresh structure
		
		movieInfo.epgTitle = ylist[count].title;
		movieInfo.epgInfo2 = ylist[count].description;
		movieInfo.tfile = ylist[count].tfile;
		movieInfo.ytdate = ylist[count].published;
		movieInfo.ytid = ylist[count].id;
		movieInfo.file.Name = ylist[count].url;
		
		m_vMovieInfo.push_back(movieInfo);
	}

	loadBox.hide();
}

#define HEAD_BUTTONS_COUNT	2
const struct button_label NKHeadButtons[HEAD_BUTTONS_COUNT] = 
{
	{ NEUTRINO_ICON_REC, " " }, 
	{ NEUTRINO_ICON_BUTTON_HELP, " " }
};

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Neu by Netzkino") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("HD Kino") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("next focus") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Highlight") }
};

void CNKMovies::showMenu()
{
	dprintf(DEBUG_NORMAL, "CNKMovies::showMenu: mode:%d id:%d title:%s\n", catMode, catID, caption.c_str());

	//
	std::string title;
	
	title = "NetzKino";
	title += ": ";
	title += caption;

	////
	mainWidget = new CWidget(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	mainWidget->enableSaveScreen();
	mainWidget->setSelected(selected);

	// headwidget
	headBox.iWidth = frameBuffer->getScreenWidth();
	headBox.iHeight = 40;
	headBox.iX = frameBuffer->getScreenX();
	headBox.iY = frameBuffer->getScreenY();

	headersWidget = new CCHeaders(headBox.iX, headBox.iY, headBox.iWidth, headBox.iHeight, title.c_str(), NEUTRINO_ICON_NETZKINO_SMALL);

	headersWidget->setButtons(NKHeadButtons, HEAD_BUTTONS_COUNT);
	headersWidget->enablePaintDate();
	headersWidget->setGradient(NOGRADIENT);

	// foot
	footBox.iWidth = frameBuffer->getScreenWidth();
	footBox.iHeight = 40;
	footBox.iX = frameBuffer->getScreenX();
	footBox.iY = frameBuffer->getScreenY() + frameBuffer->getScreenHeight() - footBox.iHeight;

	footersWidget = new CCFooters(footBox.iX, footBox.iY, footBox.iWidth, footBox.iHeight);

	footersWidget->setButtons(FootButtons, FOOT_BUTTONS_COUNT);
	footersWidget->setGradient(NOGRADIENT);

	// leftwidget
	leftBox.iWidth = 280;
	leftBox.iHeight = mainWidget->getWindowsPos().iHeight - headBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	leftBox.iX = mainWidget->getWindowsPos().iX;
	leftBox.iY = mainWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE;

	leftWidget = new ClistBox(&leftBox);

	leftWidget->setSelected(left_selected);
	//leftWidget->enableShrinkMenu();
	leftWidget->setFocus();

	leftWidget->addItem(new CMenuForwarder("Suche", true, nksearch.c_str(), this, "search"));
	leftWidget->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	if(cats.empty())
		cats = nkparser.GetCategoryList();

	// categories
	for (unsigned i = 0; i < cats.size(); i++)
	{
		leftWidget->addItem(new CMenuForwarder(cats[i].title.c_str(), true, NULL, new CNKMovies(cNKFeedParser::CATEGORY, cats[i].id, cats[i].title), "", CRCInput::RC_nokey, NEUTRINO_ICON_NETZKINO_SMALL));
	}

	leftWidget->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	leftWidget->addItem(new CMenuSeparator());
	leftWidget->addItem(new CMenuSeparator());
	leftWidget->addItem(new CMenuSeparator());
	leftWidget->addItem(new CMenuSeparator());
	leftWidget->addItem(new CMenuSeparator());
	leftWidget->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	leftWidget->addItem(new CMenuForwarder("Beenden", true, NULL, this, "exit", CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_POWER));

	// rightwidget
	rightBox.iWidth = mainWidget->getWindowsPos().iWidth - INTER_FRAME_SPACE - leftBox.iWidth;
	rightBox.iHeight = mainWidget->getWindowsPos().iHeight - headBox.iHeight - 2*INTER_FRAME_SPACE - footBox.iHeight;
	rightBox.iX = mainWidget->getWindowsPos().iX + leftBox.iWidth + INTER_FRAME_SPACE;
	rightBox.iY = mainWidget->getWindowsPos().iY + headBox.iHeight + INTER_FRAME_SPACE;

	//
	rightWidget = new ClistBox(&rightBox);
	rightWidget->setWidgetType(ClistBox::TYPE_FRAME);
	rightWidget->setItemsPerPage(5,2);
	rightWidget->setSelected(right_selected);

	for (unsigned int i = 0; i < m_vMovieInfo.size(); i++)
	{
		item = new CMenuForwarder(m_vMovieInfo[i].epgTitle.c_str(), true, NULL, this, "play", CRCInput::RC_nokey, NULL, file_exists(m_vMovieInfo[i].tfile.c_str())? m_vMovieInfo[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->setHint(m_vMovieInfo[i].epgInfo2.c_str());

		rightWidget->addItem(item);
	}

	mainWidget->addCCItem(headersWidget);
	mainWidget->addCCItem(leftWidget);
	mainWidget->addCCItem(rightWidget);
	mainWidget->addCCItem(footersWidget);

	mainWidget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	mainWidget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	mainWidget->addKey(CRCInput::RC_info, this, CRCInput::getSpecialKeyName(CRCInput::RC_info));
	mainWidget->addKey(CRCInput::RC_record, this, CRCInput::getSpecialKeyName(CRCInput::RC_record));
	mainWidget->addKey(CRCInput::RC_blue, this, CRCInput::getSpecialKeyName(CRCInput::RC_blue));
	mainWidget->addKey(CRCInput::RC_home, this, "exit");

	mainWidget->exec(NULL, "");

	if (mainWidget)
	{
		delete mainWidget;
		mainWidget = NULL;
	}
}

void CNKMovies::showMovieInfo(MI_MOVIE_INFO& movie)
{
	m_movieInfo.showMovieInfo(movie);
}

void CNKMovies::recordMovie(MI_MOVIE_INFO& movie)
{
	CHTTPTool httpTool;
	httpTool.setTitle("NetzKino: downloading...");

	std::string target = g_settings.network_nfs_recordingdir;
	target += "/";
	target += movie.epgTitle.c_str();
	target += "." + getFileExt(movie.file.Name);

	if (httpTool.downloadFile(movie.file.Name.c_str(), target.c_str(), 100))
	{
		// write .xml
		movie.file.Name = target;
		m_movieInfo.saveMovieInfo(movie);
		
		// write thumbnail
		string tfile = g_settings.network_nfs_recordingdir;
		tfile += "/";
		tfile += movie.epgTitle.c_str();
		tfile += ".jpg";
		
		CFileHelpers::getInstance()->copyFile(movie.tfile.c_str(), tfile.c_str());
	}
	else
	{
		HintBox("Netzkino", "Movie download failed");
	}
}

int CNKMovies::exec(CWidgetTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CNKMovies::exec: actionKey:%s\n", actionKey.c_str());

	if(parent) 
		parent->hide();

	if(actionKey == "play")
	{
		right_selected = rightWidget->getSelected();

		CMovieInfoWidget movieInfoWidget;
		movieInfoWidget.setMovie(m_vMovieInfo[right_selected]);
		
		movieInfoWidget.exec(NULL, "");

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_info")
	{
		right_selected = rightWidget->getSelected();
		showMovieInfo(m_vMovieInfo[right_selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_record")
	{
		right_selected = rightWidget->getSelected();

		recordMovie(m_vMovieInfo[right_selected]);

		return RETURN_REPAINT;
	}
	else if(actionKey == "search")
	{
		nksearch.clear();

		CStringInputSMS stringInput(_("Search"), nksearch.c_str());
		int ret = stringInput.exec(NULL, "");

		printf("ret:%d nksearch:%s\n", ret, nksearch.c_str());

		if(!stringInput.getExitPressed()) //FIXME:
		{
			loadNKTitles(cNKFeedParser::SEARCH, nksearch, 0);

			rightWidget->clearItems();
			showMenu();

			return RETURN_EXIT_ALL;
		}
		else
			return RETURN_REPAINT;
	}
	else if(actionKey == "nextPage")
	{
		return RETURN_REPAINT;
	}
	else if(actionKey == "prevPage")
	{
		return RETURN_REPAINT;
	}
	else if(actionKey == "RC_red")
	{
		loadNKTitles(cNKFeedParser::CATEGORY, "Neu bei Netzkino", 81);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_green")
	{
		loadNKTitles(cNKFeedParser::CATEGORY, "HD Kino", 61);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "RC_blue")
	{
		loadNKTitles(cNKFeedParser::CATEGORY, "Highlights", 8);
		showMenu();

		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "exit")
	{
		return RETURN_EXIT_ALL;
	}

	loadNKTitles(catMode, caption, catID);
	showMenu();
	
	return RETURN_EXIT;
}

// plugin API
void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CNKMovies * NKHandler = new CNKMovies(cNKFeedParser::CATEGORY, 9681, "Highlights");
	
	NKHandler->exec(NULL, "");
	
	delete NKHandler;
	NKHandler = NULL;	
}


