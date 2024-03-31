 /*
 	Neutrino-GUI  -   DBoxII-Project
 	
	$id: movieinfo.cpp 01.04.2024 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include <unistd.h>

#include <gui/widget/infobox.h>

#include <system/debug.h>
#include <system/helpers.h>
#include <system/tmdbparser.h>
#include <system/settings.h>

#include <gui/movieplayer.h>
#include <gui/movieinfo.h>


//// CMovieInfoWidget
CMovieInfoWidget::CMovieInfoWidget()
{
	m_movieInfo.clearMovieInfo(&movieFile);
}

CMovieInfoWidget::~CMovieInfoWidget()
{
	m_movieInfo.clearMovieInfo(&movieFile);
}

void CMovieInfoWidget::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

void CMovieInfoWidget::setMovie(MI_MOVIE_INFO& file)
{
	m_movieInfo.clearMovieInfo(&movieFile);

	movieFile = file;
}

void CMovieInfoWidget::setMovie(const CFile& file, std::string title, std::string info1, std::string info2, std::string tfile, std::string duration, std::string rating)
{
	m_movieInfo.clearMovieInfo(&movieFile);

	movieFile.file.Name = file.Name;
	movieFile.epgTitle = title;
	movieFile.epgInfo1 = info1;
	movieFile.epgInfo2 = info2;
	movieFile.tfile = tfile;
	movieFile.length = atol(duration.c_str());
	movieFile.vote_average = (float)atol(rating.c_str());
}

void CMovieInfoWidget::setMovie(const char* fileName, std::string title, std::string info1, std::string info2, std::string tfile, std::string duration, std::string rating)
{
	m_movieInfo.clearMovieInfo(&movieFile);

	movieFile.file.Name = fileName;
	movieFile.epgTitle = title;
	movieFile.epgInfo1 = info1;
	movieFile.epgInfo2 = info2;
	movieFile.tfile = tfile;
	movieFile.length = atol(duration.c_str());
	movieFile.vote_average = (float)atol(rating.c_str());
	
	printf("setMovie:%d\n", movieFile.length);
}

void CMovieInfoWidget::funArt()
{
	// mainBox
	CBox box;
	box.iX = CFrameBuffer::getInstance()->getScreenX();
	box.iY = CFrameBuffer::getInstance()->getScreenY();
	box.iWidth = CFrameBuffer::getInstance()->getScreenWidth();
	box.iHeight = CFrameBuffer::getInstance()->getScreenHeight();

	// titleBox
	CBox titleBox;
	titleBox.iX = box.iX + 10;
	titleBox.iY = box.iY + 10;
	titleBox.iWidth = box.iWidth;
	titleBox.iHeight = 40;

	// starBox
	CBox starBox;
	starBox.iX = box.iX +10;
	starBox.iY = box.iY + titleBox.iHeight + 10;
	starBox.iWidth = 25;
	starBox.iHeight = 25;

	// playBox
	CBox playBox;
	playBox.iWidth = 300;
	playBox.iHeight = 60;
	playBox.iX = box.iX + 10;
	playBox.iY = box.iY + box.iHeight - 10 - playBox.iHeight;
	
	// infoBox
	CBox infoBox;
	infoBox.iWidth = 300;
	infoBox.iHeight = 60;
	infoBox.iX = box.iX + 10 + playBox.iWidth + 10;
	infoBox.iY = box.iY + box.iHeight - 10 - infoBox.iHeight;

	// textBox
	CBox textBox;
	textBox.iWidth = box.iWidth/2 - 20;
	textBox.iHeight = box.iHeight - playBox.iHeight - starBox.iHeight - titleBox.iHeight - 3*10 - 100;
	textBox.iX = box.iX + 10;
	textBox.iY = starBox.iY + 10 + 60;
	
	// artBox
	CBox artBox;
	artBox.iWidth = box.iWidth/2 - 20;
	artBox.iHeight = box.iHeight - 20;
	artBox.iX = box.iX + box.iWidth/2 + 10;
	artBox.iY = box.iY + 10;

	CFrameBox * testFrameBox = new CFrameBox(&box);
	CWidget * widget = new CWidget(&box);

	// artFrame
	CFrameItem * artFrame = new CFrameItem();
	artFrame->setMode(FRAME_PICTURE);
	artFrame->setPosition(&artBox);
	artFrame->setIconName(movieFile.tfile.c_str());
	artFrame->setActive(false);

	testFrameBox->addFrame(artFrame);

	// title
	CFrameItem *titleFrame = new CFrameItem();
	titleFrame->setMode(FRAME_LABEL);
	int t_w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getRenderWidth(movieFile.epgTitle);
	if (t_w > box.iWidth)
		t_w = box.iWidth;
	titleFrame->setPosition(titleBox.iX, titleBox.iY, t_w, titleBox.iHeight);
	titleFrame->paintMainFrame(false);
	titleFrame->setTitle(movieFile.epgTitle.c_str());
	titleFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	titleFrame->setActive(false);

	testFrameBox->addFrame(titleFrame);

	// vote
	for (int i = 0; i < 5; i++)
	{
		CFrameItem *starOffFrame = new CFrameItem();
		starOffFrame->setMode(FRAME_ICON);
		starOffFrame->setPosition(starBox.iX + i*25, starBox.iY, starBox.iWidth, starBox.iHeight);
		starOffFrame->setIconName(NEUTRINO_ICON_STAR_OFF);
		starOffFrame->paintMainFrame(false);
		starOffFrame->setActive(false);

		testFrameBox->addFrame(starOffFrame);
	}

	int average = movieFile.vote_average/2 + 1;

	for (int i = 0; i < average; i++)
	{
		CFrameItem *starOnFrame = new CFrameItem();
		starOnFrame->setMode(FRAME_ICON);
		starOnFrame->setPosition(starBox.iX + i*25, starBox.iY, starBox.iWidth, starBox.iHeight);
		starOnFrame->setIconName(NEUTRINO_ICON_STAR_ON);
		starOnFrame->paintMainFrame(false);
		starOnFrame->setActive(false);

		testFrameBox->addFrame(starOnFrame);
	}
	
	// duration
	std::string l_buffer = "";
	if (movieFile.length != 0)
	{
		l_buffer = toString(movieFile.length);
		l_buffer += " Min";
	}
	
	if (movieFile.productionDate)
	{
		l_buffer += " " + toString(movieFile.productionDate);
	}
	
	if (!movieFile.genres.empty())
	{
		l_buffer += " (" + movieFile.genres + ")";
	}
	
	CFrameItem *lengthFrame = new CFrameItem();
	lengthFrame->setMode(FRAME_LABEL);
		
	int l_w = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getRenderWidth(l_buffer);
			
	lengthFrame->setPosition(titleBox.iX, starBox.iY + starBox.iHeight + 10, l_w, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight());
	lengthFrame->paintMainFrame(false);
		
	lengthFrame->setTitle(l_buffer.c_str());
	lengthFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	lengthFrame->setActive(false);

	testFrameBox->addFrame(lengthFrame);

	// text
	CFrameItem *textFrame = new CFrameItem();
	textFrame->setMode(FRAME_TEXT);
	textFrame->setPosition(&textBox);
	std::string buffer;
	buffer = movieFile.epgInfo1;
	buffer += "\n";
	buffer += movieFile.epgInfo2;

	textFrame->setTitle(buffer.c_str());
	textFrame->paintMainFrame(false);
	textFrame->setActive(false);
	
	testFrameBox->addFrame(textFrame);

	// play
	CFrameItem *playFrame = new CFrameItem();
	playFrame->setPosition(&playBox);
	playFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	playFrame->setTitle("Movie abspielen");
	playFrame->setIconName(NEUTRINO_ICON_PLAY);
	playFrame->setActionKey(this, "playMovie");
	playFrame->setBorderMode();

	testFrameBox->addFrame(playFrame);

	// infoFrame
	CFrameItem * infoFrame = new CFrameItem();
	infoFrame->setPosition(&infoBox);
	infoFrame->setCaptionFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
	infoFrame->setTitle("Movie Details");
	infoFrame->setIconName(NEUTRINO_ICON_INFO);
	infoFrame->setActionKey(this, "MovieInfo");
	infoFrame->setBorderMode();

	testFrameBox->addFrame(infoFrame, true);

	widget->addCCItem(testFrameBox);
	
	//
	widget->setTimeOut(g_settings.timing_filebrowser);
	widget->exec(NULL, "");

	delete widget;
	widget = NULL;
}

int CMovieInfoWidget::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMovieInfoWidget::exec: actionKey:%s\n", actionKey.c_str());
	
	if(parent)
		hide();
	
	if(actionKey == "playMovie")
	{
		CMoviePlayerGui tmpMoviePlayerGui;
		tmpMoviePlayerGui.addToPlaylist(movieFile);

		tmpMoviePlayerGui.exec(NULL, "");

		return CMenuTarget::RETURN_REPAINT;
	}
	else if(actionKey == "MovieInfo")
	{
		m_movieInfo.showMovieInfo(movieFile);

		return CMenuTarget::RETURN_REPAINT;
	}

	funArt();
	
	return CMenuTarget::RETURN_EXIT;
}

