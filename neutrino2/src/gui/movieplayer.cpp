/*
  Neutrino-GUI  -   DBoxII-Project
  
  $Id: movieplayer.cpp 24.12.2018 mohousch Exp $

  Movieplayer (c) 2003, 2004 by gagga
  Based on code by Dirch, obi and the Metzler Bros. Thanks.

  $Id: movieplayer.cpp,v 1.97 2004/07/18 00:54:52 thegoodguy Exp $

  Homepage: http://www.giggo.de/dbox2/movieplayer.html

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

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <sys/mount.h>

#include <algorithm>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <poll.h>
#include <sys/timeb.h>

#include <gui/movieplayer.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/vcrcontrol.h>
#include <driver/color.h>
#include <driver/file.h>

#include <daemonc/remotecontrol.h>

#include <timerd/timerd.h>

#include <system/settings.h>
#include <system/helpers.h>
#include <system/tmdbparser.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/nfs.h>
#include <gui/audio_video_select.h>
#include <gui/movieplayer_setup.h>
#include <gui/filebrowser.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/infobox.h>

#include <system/debug.h>

#include <libxmltree/xmlinterface.h>

//
#include <playback_cs.h>
#include <video_cs.h>


// scripts
#define MOVIEPLAYER_START_SCRIPT 	CONFIGDIR "/movieplayer.start" 
#define MOVIEPLAYER_END_SCRIPT 	CONFIGDIR "/movieplayer.end"

// CMovieInfoViewer
#define TIMEOSD_FONT 		SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME
#define TIMEBARH 		38
#define BOXHEIGHT_MOVIEINFO	125
#define BUTTON_BAR_HEIGHT	25
#define TIMESCALE_BAR_HEIGHT	4

extern cPlayback *playback;

// movieplayer
CMoviePlayerGui::CMoviePlayerGui()
{
	dprintf(DEBUG_INFO, "CMoviePlayerGui::CMoviePlayerGui()\n");
	
	stopped = false;

	frameBuffer = CFrameBuffer::getInstance();

	selected = 0;

	// infoViewer
	visible = false;
	initFrames();
	
	//
	mplist = NULL;
	item = NULL;
	
	//
	CBox box;
	box.iWidth = frameBuffer->getScreenWidth();
	box.iHeight = frameBuffer->getScreenHeight();
	box.iX = frameBuffer->getScreenX();
	box.iY = frameBuffer->getScreenY();
	
	mplist = new ClistBox(&box);
	
	moviescale = new CProgressBar(cFrameBoxInfo.iX + BORDER_LEFT, cFrameBoxInfo.iY + 30, cFrameBoxInfo.iWidth - BORDER_LEFT - BORDER_RIGHT, TIMESCALE_BAR_HEIGHT);

	timeCounter = NULL;
	file_prozent = 0;
}

CMoviePlayerGui::~CMoviePlayerGui()
{
	playlist.clear();
	
	if (moviescale)
	{
		delete moviescale;
		moviescale = NULL;
	}
	
	if (timeCounter)
	{
		delete timeCounter ;
		timeCounter = NULL;
	}
	
	if (mplist)
	{
		delete mplist;
		mplist = NULL;
	}
}

void CMoviePlayerGui::cutNeutrino()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::cutNeutrino\n");
	
	if (stopped)
		return;
	
	// tell neutrino we are in ts mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, NeutrinoMessages::mode_ts);
	
	// save (remeber) last mode
	m_LastMode = (CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	
	//
	CNeutrinoApp::getInstance()->lockPlayBack();
	
	// start mp start-script
	puts("[movieplayer.cpp] executing " MOVIEPLAYER_START_SCRIPT ".");
	if (system(MOVIEPLAYER_START_SCRIPT) != 0)
		perror("Datei " MOVIEPLAYER_START_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " MOVIEPLAYER_START_SCRIPT " not found. Please create if needed.\n");

	stopped = true;
}

void CMoviePlayerGui::restoreNeutrino()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::restoreNeutrino\n");
	
	if (!stopped)
		return;
	
	//	
	CNeutrinoApp::getInstance()->unlockPlayBack();

	// tell neutrino that we are in the last mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, m_LastMode);
	
	//show infobar
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	
	// start end script
	puts("[movieplayer.cpp] executing " MOVIEPLAYER_END_SCRIPT ".");
	if (system(MOVIEPLAYER_END_SCRIPT) != 0)
		perror("Datei " MOVIEPLAYER_END_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " MOVIEPLAYER_END_SCRIPT " not found. Please create if needed.\n");

	stopped = false;
}

void CMoviePlayerGui::updateLcd(const std::string & lcd_filename)
{
	char tmp[20];
	std::string lcd;

	switch (playstate) 
	{
		case CMoviePlayerGui::PAUSE:
			//lcd = "|| ";
			lcd = lcd_filename;
			break;
			
		case CMoviePlayerGui::REW:
			sprintf(tmp, "%dx<< ", speed);
			lcd = tmp;
			lcd += lcd_filename;
			break;
			
		case CMoviePlayerGui::FF:
			sprintf(tmp, "%dx>> ", speed);
			lcd = tmp;
			lcd += lcd_filename;
			break;

		case CMoviePlayerGui::SLOW:
			sprintf(tmp, "%ds>> ", slow);
			lcd = tmp;
			lcd += lcd_filename;
			break;

		default:
			//lcd = "> ";
			lcd = lcd_filename;
			break;
	}
	
	CVFD::getInstance()->showMenuText(0, lcd.c_str(), -1, true);	
}

void CMoviePlayerGui::addToPlaylist(MI_MOVIE_INFO& mfile)
{	
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::addToPlaylist: %s\n", mfile.file.Name.c_str());
	
	playlist.push_back(mfile);
}

void CMoviePlayerGui::addToPlaylist(const CFile& file, std::string title, std::string info1, std::string info2, std::string tfile)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::addToPlaylist: %s\n", file.Name.c_str());

	MI_MOVIE_INFO movieInfo;
	cMovieInfo.clearMovieInfo(&movieInfo); // refresh structure

	movieInfo.file.Name = file.Name;
	movieInfo.epgTitle = title;
	movieInfo.epgInfo1 = info1;
	movieInfo.epgInfo2 = info2;
	movieInfo.tfile = tfile;

	// load movie infos (from xml file)
	cMovieInfo.loadMovieInfo(&movieInfo);
					
	// 
	playlist.push_back(movieInfo);
}

void CMoviePlayerGui::addToPlaylist(const char* fileName, std::string title, std::string info1, std::string info2, std::string tfile)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::addToPlaylist: %s\n", fileName);

	MI_MOVIE_INFO movieInfo;
	cMovieInfo.clearMovieInfo(&movieInfo); // refresh structure

	movieInfo.file.Name = fileName;
	movieInfo.epgTitle = title;
	movieInfo.epgInfo1 = info1;
	movieInfo.epgInfo2 = info2;
	movieInfo.tfile = tfile;

	// load movie infos (from xml file)
	cMovieInfo.loadMovieInfo(&movieInfo);
					
	// 
	playlist.push_back(movieInfo);
}

void CMoviePlayerGui::clearPlaylist(void)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::clearPlaylist:\n");

	if (!playlist.empty())
	{
		playlist.clear();
		selected = 0;
	}
}

void CMoviePlayerGui::removeFromPlaylist(long pos)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::removeFromPlayList:\n");

	playlist.erase(playlist.begin() + pos); 
}

void CMoviePlayerGui::startMovieInfoViewer(void)
{
	if(sec_timer_id == 0)
		sec_timer_id = g_RCInput->addTimer(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]*1000*1000);
}

void CMoviePlayerGui::killMovieInfoViewer(void)
{
	if(sec_timer_id)
	{
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;
	}
}

int CMoviePlayerGui::exec(CMenuTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::exec: actionKey:%s\n", actionKey.c_str());

	if (parent) 
		parent->hide();

	bool usedBackground = frameBuffer->getuseBackground();

	if (usedBackground) 
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->clearFrameBuffer();
		frameBuffer->blit();
	}
	
	//
	position = 0;
	file_prozent = 0;
	duration = 0;
	startposition = 0;
	
	//
	speed = 1;
	slow = 0;
	
	// global flags
	update_lcd = false;
	start_play = false;
	exit = false;
	m_loop = false;
	m_multiselect = false;
	show_bookmark = true;
	
	// for playing
	playstate = CMoviePlayerGui::STOPPED;
	is_file_player = false;
	
	//
	time_forced = false;
	
	// 
	g_numpida = 0;
	g_vpid = 0;
	g_vtype = 0;
	g_currentapid = 0;
	g_currentac3 = 0;

	sec_timer_id = 0;
	
	// cutneutrino
	cutNeutrino();
	
	//
	PlayFile();
	
	// quit
	// Restore previous background
	if (usedBackground) 
	{
		frameBuffer->restoreBackgroundImage();
		frameBuffer->useBackground(true);
		frameBuffer->paintBackground();
		frameBuffer->blit();
	}
	
	// clear playlist
	if(!playlist.empty())
		playlist.clear();

	// restore neutrino
	restoreNeutrino();
	
	//
	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	return RETURN_EXIT;
}

void CMoviePlayerGui::play(unsigned int pos)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::play:\n");
	
	if(!playlist.size())
		return;

	selected = pos;
			
	//
	if(playback->playing)
		playback->Close();

	// init player
#if defined (PLATFORM_COOLSTREAM)
	if(playlist[pos].file.getExtension() != CFile::EXTENSION_TS)
		is_file_player = true;

	playback->Open(is_file_player ? PLAYMODE_FILE : PLAYMODE_TS);
#else			
	playback->Open();
#endif			
			
	duration = 0;
	if(playlist[pos].length != 0)
		duration = playlist[pos].length * 60 * 1000; // in ms
			  
	// PlayBack Start
#if defined (PLATFORM_COOLSTREAM)			  
	if(!playback->Start((char *)playlist[pos].file.Name.c_str(), g_vpid, g_vtype, g_currentapid, g_currentac3, duration))
#else
	if(!playback->Start((char *)playlist[pos].file.Name.c_str()))
#endif
	{
		dprintf(DEBUG_NORMAL, "CMoviePlayerGui::play: Starting Playback failed!\n");
		playback->Close();
		restoreNeutrino();
	} 
	else 
	{
		// set PlayState
		playstate = CMoviePlayerGui::PLAY;

		CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
				
		// set position 
		playback->SetPosition(startposition * 1000);
				
		// get position / duration
#if defined (PLATFORM_COOLSTREAM)
		playback->GetPosition(position, duration);
#else
		playback->GetPosition(position, duration);
#endif
		
		//
		if(playlist[selected].file.getType() == CFile::FILE_AUDIO)
		{
			if (!playlist[selected].tfile.empty())
				frameBuffer->loadBackgroundPic(playlist[selected].tfile);
		}
	}
}

void CMoviePlayerGui::playNext()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::playNext:\n");
	
	if(!playlist.size())
		return;
	
	//FIXME:
	if(m_loop && playstate == CMoviePlayerGui::PLAY) // loop
	{
		//
		update_lcd = true;
		start_play = true;
	}
	else if(!playlist.empty() && selected + 1 < playlist.size() && playstate == CMoviePlayerGui::PLAY) 
	{
		selected++;

		//
		if(playlist[selected].ytid.empty())
		{
			if(!playlist[selected].audioPids.empty()) 
			{
				g_currentapid = playlist[selected].audioPids[0].epgAudioPid;
				g_currentac3 = playlist[selected].audioPids[0].atype;

				//
				currentapid = g_currentapid;
			}

			for (int i = 0; i < (int)playlist[selected].audioPids.size(); i++) 
			{
						if (playlist[selected].audioPids[i].selected) 
						{
#if defined (PLATFORM_COOLSTREAM)
							g_currentapid = playlist[selected].audioPids[i].epgAudioPid;
#else
							g_currentapid = i;
#endif							
							g_currentac3 = playlist[selected].audioPids[i].atype;

							//
#if defined (PLATFORM_COOLSTREAM)
							currentapid = g_currentapid;
							currentac3 = g_currentac3;
#else
							currentapid = 0;
#endif
						}
			}

			//
			g_vpid = playlist[selected].epgVideoPid;
			g_vtype = playlist[selected].VideoType;

			// startposition
			if (!m_multiselect)			
				startposition = showStartPosSelectionMenu(); // in sec

			if(startposition < 0)
				exit = true;
		}
				
		//
		update_lcd = true;
		start_play = true;

		//
		if(playlist[selected].file.getType() == CFile::FILE_AUDIO)
		{
			if (!playlist[selected].tfile.empty())
				frameBuffer->loadBackgroundPic(playlist[selected].tfile);
		}
	}
}

void CMoviePlayerGui::playPrev()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::playPrev:\n");
	
	if(!playlist.size())
		return;

	//FIXME:
	if(!playlist.empty() && selected > 0 && playstate == CMoviePlayerGui::PLAY) 
	{
		selected--;

		//
		if(playlist[selected].ytid.empty())
		{
			if(!playlist[selected].audioPids.empty()) 
			{
						g_currentapid = playlist[selected].audioPids[0].epgAudioPid;
						g_currentac3 = playlist[selected].audioPids[0].atype;

							//
							currentapid = g_currentapid;
			}

			for (int i = 0; i < (int)playlist[selected].audioPids.size(); i++) 
			{
				if (playlist[selected].audioPids[i].selected) 
				{
#if defined (PLATFORM_COOLSTREAM)
					g_currentapid = playlist[selected].audioPids[i].epgAudioPid;
#else
					g_currentapid = i;
#endif
					g_currentac3 = playlist[selected].audioPids[i].atype;

					//
#if defined (PLATFORM_COOLSTREAM)
					currentapid = g_currentapid;
					currentac3 = g_currentac3;
#else
					currentapid = 0;
#endif
				}
			}

			//
			g_vpid = playlist[selected].epgVideoPid;
			g_vtype = playlist[selected].VideoType;

			// startposition
			if (!m_multiselect)			
				startposition = showStartPosSelectionMenu(); // in sec

			if(startposition < 0)
				exit = true;
		}

		//
		update_lcd = true;
		start_play = true;

		//
		if(playlist[selected].file.getType() == CFile::FILE_AUDIO)
		{
			if (!playlist[selected].tfile.empty())
				frameBuffer->loadBackgroundPic(playlist[selected].tfile);
		}
	}
}

void CMoviePlayerGui::stop()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::stop\n");

	//exit play
	playstate = CMoviePlayerGui::STOPPED;
			
	if(playlist[selected].ytid == "timeshift")
	{
		// stop record if recording
		if( CNeutrinoApp::getInstance()->recordingstatus) 
		{
			if(MessageBox(_("Information"), _("You really want to to stop record ?"), mbrYes, mbYes | mbNo, NULL, 450, 30, true) == mbrYes)
			{
				CVCRControl::getInstance()->Stop();
				CTimerd::getInstance()->stopTimerEvent(CNeutrinoApp::getInstance()->recording_id);
				CVFD::getInstance()->ShowIcon(VFD_ICON_TIMESHIFT, false );

				CNeutrinoApp::getInstance()->recording_id = 0;
				CNeutrinoApp::getInstance()->recordingstatus = 0;
				CNeutrinoApp::getInstance()->timeshiftstatus = 0;
			}
		} 
	}
			
	if(m_loop)
		m_loop = false;
			
	if(m_multiselect)
		m_multiselect = false;
			
	exit = true;
}

//
void CMoviePlayerGui::PlayFile(void)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile\n");

	neutrino_msg_t msg;
	neutrino_msg_data_t data;
	
	// multi_select
	if(playlist.size() > 1)
		m_multiselect = true;
	
	//
	if(!playlist.empty())
	{	
		//
		if(playlist[selected].ytid.empty())
		{
			if(!playlist[selected].audioPids.empty()) 
			{
				g_currentapid = playlist[selected].audioPids[0].epgAudioPid;
				g_currentac3 = playlist[selected].audioPids[0].atype;

				//
				currentapid = g_currentapid;
			}

			for (int i = 0; i < (int)playlist[selected].audioPids.size(); i++) 
			{
				if (playlist[selected].audioPids[i].selected) 
				{
#if defined (PLATFORM_COOLSTREAM)
					g_currentapid = playlist[selected].audioPids[i].epgAudioPid;
#else
					g_currentapid = i;	//FIXME
#endif						
					g_currentac3 = playlist[selected].audioPids[i].atype;

#if defined (PLATFORM_COOLSTREAM)
					currentapid = g_currentapid;
					currentac3 = g_currentac3;
#else
					currentapid = 0;
#endif
				}
			}

			//
			g_vpid = playlist[selected].epgVideoPid;
			g_vtype = playlist[selected].VideoType;

			// startposition
			if (!m_multiselect)			
				startposition = showStartPosSelectionMenu(); // in sec

			// audio files
			if(playlist[selected].file.getType() == CFile::FILE_AUDIO)
			{
				if (!playlist[selected].tfile.empty())
					frameBuffer->loadBackgroundPic(playlist[selected].tfile);
			}
		}

		//
		update_lcd = true;
		start_play = true;

		// secure
		if(startposition < 0)
			exit = true;

		//
		if (!playlist[selected].ytid.empty())
			show_bookmark = false;
	}
						
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);	

	// bookmarks menu
	timeb current_time;

	int width = 280;
	int height = 65;
    	int x = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - width) / 2;
    	int y = frameBuffer->getScreenY() + frameBuffer->getScreenHeight() - height - 20;

	CBox boxposition(x, y, width, height);	// window position for the hint boxes

	// backword hintbox
	CTextBox newBackwordHintBox;
	newBackwordHintBox.setPosition(&boxposition);
	newBackwordHintBox.setText(_("New jump back\n 'blue' for endposition"));

	// forward hintbox
	CTextBox newForwardHintBox;
	newForwardHintBox.setPosition(&boxposition);
	newForwardHintBox.setText(_("New jump forward\n 'blue' for endposition"));

	//
	int jump_not_until = 0;	// any jump shall be avoided until this time (in seconds from moviestart)
	MI_BOOKMARK new_bookmark;	// used for new movie info bookmarks created from the movieplayer
	new_bookmark.pos = 0;		// clear , since this is used as flag for bookmark activity
	new_bookmark.length = 0;

	//
	CWidget* widget = NULL;
	ClistBox* bookStartMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("bookmark");
	
	if (widget)
	{
		bookStartMenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		bookStartMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		bookStartMenu->setWidgetMode(MODE_MENU);
		bookStartMenu->enableShrinkMenu();
			
		bookStartMenu->enablePaintHead();
		bookStartMenu->setTitle(_("Bookmarks"), NEUTRINO_ICON_MOVIE);

		bookStartMenu->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
		bookStartMenu->setFootButtons(&btn);
			
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "bookmark";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(bookStartMenu);
	}
	
	bookStartMenu->clearItems();

	bookStartMenu->addItem(new ClistBoxItem(_("New Bookmark")));
	bookStartMenu->addItem(new ClistBoxItem(_("Repeat")));
	bookStartMenu->addItem(new ClistBoxItem(_("Jump over")));
	bookStartMenu->addItem(new ClistBoxItem(_("Movie start:")));
	bookStartMenu->addItem(new ClistBoxItem(_("Movie end:")));
	
	////FIXME:
	//showMovieInfo();

	// play loop
	do {
		// exit
		if (exit) 
		{	  
			exit = false;
			
			dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: stop (3)\n");
			playstate = CMoviePlayerGui::STOPPED;
			break;
		}
		
		// bookmarks
		bool doBookmark = true;
		if (doBookmark)
		{	  
			if (playstate == CMoviePlayerGui::PLAY) 
			{				
#if defined (PLATFORM_COOLSTREAM)
				playback->GetPosition(position, duration);
#else
 				playback->GetPosition(position, duration);
#endif				
				// check if !jump is stale (e.g. if user jumped forward or backward)
				int play_sec = position / 1000;	// get current seconds from moviestart

				if (play_sec + 10 < jump_not_until || play_sec > jump_not_until + 10)
					jump_not_until = 0;

				// do bookmark activities only, if there is no new bookmark started
				if (new_bookmark.pos == 0)
				{
					// process bookmarks if we have any movie info
					if (&playlist[selected] != NULL)
					{
						// movieend bookmark
						if (playlist[selected].bookmarks.end != 0) 
						{
							// stop playing
							// we ARE close behind the stop position, stop playing 
							if (play_sec >= playlist[selected].bookmarks.end && play_sec <= playlist[selected].bookmarks.end + 2 && play_sec > jump_not_until)
							{
								dprintf(DEBUG_INFO, "CMoviePlayerGui::PlayFile: bookmark stop\r\n");
								playstate = CMoviePlayerGui::STOPPED;
							}
						}
						
						// jump bookmark
						int loop = true;

						for (int book_nr = 0; book_nr < MI_MOVIE_BOOK_USER_MAX && loop == true; book_nr++) 
						{
							if (playlist[selected].bookmarks.user[book_nr].pos != 0 && playlist[selected].bookmarks.user[book_nr].length != 0) 
							{
								// do jump
								if (play_sec >= playlist[selected].bookmarks.user[book_nr].pos && play_sec <= playlist[selected].bookmarks.user[book_nr].pos + 2 && play_sec > jump_not_until)	//
								{
									g_jumpseconds = playlist[selected].bookmarks.user[book_nr].length;

									// jump back
									if (playlist[selected].bookmarks.user[book_nr].length < 0) 
									{
										if (g_jumpseconds > -15)
											g_jumpseconds = -15;

										g_jumpseconds = g_jumpseconds + playlist[selected].bookmarks.user[book_nr].pos;

										//playstate = CMoviePlayerGui::JPOS;	// bookmark  is of type loop, jump backward
										playback->SetPosition(g_jumpseconds * 1000);
									} 
									// jump forward
									else if (playlist[selected].bookmarks.user[book_nr].length > 0) 
									{
										// jump at least 15 seconds
										if (g_jumpseconds < 15)
											g_jumpseconds = 15;
										g_jumpseconds = g_jumpseconds + playlist[selected].bookmarks.user[book_nr].pos;

										//
										playback->SetPosition(g_jumpseconds * 1000);
									}
									
									dprintf(DEBUG_INFO, "CMoviePlayerGui::PlayFile: do jump %d sec\r\n", g_jumpseconds);
									update_lcd = true;
									loop = false;	// do no further bookmark checks
								}
							}
						}
					}
				}
			}
		}	

		// LCD 
		if (update_lcd) 
		{
			update_lcd = false;

			updateLcd(playlist[selected].epgTitle.empty()? playlist[selected].file.getFileName() : playlist[selected].epgTitle);
		}

		// timeosd
		if (IsVisible()) 
		{
			if(duration > 100)
				file_prozent = (position / (duration / 100));
			
			moviescale->reset();	
			moviescale->paint(file_prozent, false);
			updateTime();
		}

		// start playing
		if (start_play) 
		{
			dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: Startplay at %d seconds\n", startposition);

			start_play = false;
			
			//
			play(selected);
			
			// FIXME:
			//showMovieInfo();
		}

		//get position/duration/speed
		if ( playstate >= CMoviePlayerGui::PLAY )
		{
#if defined (PLATFORM_COOLSTREAM)
			if( playback->GetPosition(position, duration) )
#else
			if( playback->GetPosition(position, duration) )
#endif			
			{
				playback->GetSpeed(speed);
								
				dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: speed %d position %d duration %d\n", speed, position, duration);					
			}
			else
			{
				// clear frameBuffer if we played audio
				if(playlist[selected].file.getType() == CFile::FILE_AUDIO)
				{
					frameBuffer->useBackground(false);
					frameBuffer->paintBackground();
				}
		
				//
				if (m_loop)
					playNext();
				else if((playlist.size() > 1) && (selected + 1 < playlist.size()))
					playNext();
				else
					stop();
			}
		}
		
		// loop msg
		g_RCInput->getMsg(&msg, &data, 10);	// 1 secs
		
		if (msg == RC_stop) 
		{
			dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: stop (1)\n");
			
			stop();
		} 
		else if (msg == RC_play) 
		{
			if (mplist && mplist->isPainted())
			{
				play(mplist->getSelected());
			}
			else
			{
				if (playstate >= CMoviePlayerGui::PLAY) 
				{
					playstate = CMoviePlayerGui::PLAY;
					update_lcd = true;
					CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
					CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);
					
					speed = 1;
					playback->SetSpeed(speed);
				}

				if (time_forced) 
				{
					time_forced = false;
					
					hide();
				}
				
				if (IsVisible())
				{ 
					hide();
				}

				// movie title
				if(playlist[selected].ytid != "timeshift")
				{
					if (IsVisible()) 
					{
						//showMovieInfo();//FIXME:
					}
					else 
					{
						//showMovieInfo();//FIXME:
					}
				}
			}
		} 
		else if ( msg == RC_pause) 
		{
			if (mplist && !mplist->isPainted())
			{
				update_lcd = true;
				
				if (playstate == CMoviePlayerGui::PAUSE) 
				{
					playstate = CMoviePlayerGui::PLAY;
					CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);
					// show play icon
					CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, true);
					speed = 1;
					playback->SetSpeed(speed);
				} 
				else 
				{
					playstate = CMoviePlayerGui::PAUSE;
					CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, true);
					CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
					speed = 0;
					playback->SetSpeed(speed);
				}

				//show MovieInfoBar
				if(playlist[selected].ytid != "timeshift")
				{
					if (IsVisible()) 
					{
						//showMovieInfo();//FIXME:
					}
					else 
					{
						//showMovieInfo();//FIXME:
					}
				}
			}
		} 
		else if (msg == RC_blue) 
		{
			if (mplist && mplist->isPainted())
			{
				mplist->hide();
				openMovieFileBrowser();
				//hide();
				showPlaylist();
			}
			else
			{
			if (IsVisible())
			{ 
				hide();
			}
			
			//			
			if(playlist[selected].ytid.empty())
			{
				int pos_sec = position / 1000;

				if (newForwardHintBox.isPainted() == true) 
				{
					// yes, let's get the end pos of the jump forward
					new_bookmark.length = pos_sec - new_bookmark.pos;
					dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: commercial length: %d\r\n", new_bookmark.length);
					if (cMovieInfo.addNewBookmark(&playlist[selected], new_bookmark) == true) 
					{
						cMovieInfo.saveMovieInfo(playlist[selected]);	// save immediately in xml file
					}
					new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
					newForwardHintBox.hide();
				} 
				else if (newBackwordHintBox.isPainted() == true) 
				{
					// yes, let's get the end pos of the jump backward
					new_bookmark.length = new_bookmark.pos - pos_sec;
					new_bookmark.pos = pos_sec;
					dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: loop length: %d\r\n", new_bookmark.length);
					if (cMovieInfo.addNewBookmark(&playlist[selected], new_bookmark) == true) 
					{
						cMovieInfo.saveMovieInfo(playlist[selected]);	// save immediately in xml file
						jump_not_until = pos_sec + 5;	// avoid jumping for this time
					}
					new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
					newBackwordHintBox.hide();
				} 
				else 
				{
					// no, nothing else to do, we open a new bookmark menu
					new_bookmark.name = "";	// use default name
					new_bookmark.pos = 0;
					new_bookmark.length = 0;

					//
					widget->exec(NULL, "none");
					int select = -1;
					select = bookStartMenu->getSelected();
					
					//
					if(select == 0) 
					{
						// new bookmark
						new_bookmark.pos = pos_sec;
						new_bookmark.length = 0;

						if (cMovieInfo.addNewBookmark(&playlist[selected], new_bookmark) == true)
							cMovieInfo.saveMovieInfo(playlist[selected]);	// save immediately in xml file
						new_bookmark.pos = 0;	// clear again, since this is used as flag for bookmark activity
					} 
					else if(select == 1) 
					{
						// jump forward bookmark
						new_bookmark.pos = pos_sec;
						dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: new bookmark 1. pos: %d\r\n", new_bookmark.pos);
						newForwardHintBox.paint();
					} 
					else if(select == 2) 
					{
						// jump backward bookmark
						new_bookmark.pos = pos_sec;
						dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: new bookmark 1. pos: %d\r\n", new_bookmark.pos);
						newBackwordHintBox.paint();
					} 
					else if(select == 3) 
					{
						// movie start bookmark
						playlist[selected].bookmarks.start = pos_sec;

						dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: New movie start pos: %d\r\n", playlist[selected].bookmarks.start);

						cMovieInfo.saveMovieInfo(playlist[selected]);	// save immediately in xml file
					} 
					else if(select == 4) 
					{
						// Moviebrowser movie end bookmark
						playlist[selected].bookmarks.end = pos_sec;

						dprintf(DEBUG_DEBUG, "CMoviePlayerGui::PlayFile: New movie end pos: %d\r\n", playlist[selected].bookmarks.start);

						cMovieInfo.saveMovieInfo(playlist[selected]);	//save immediately in xml file
					}
				}
			}
			}		
		} 
		else if ( msg == RC_audio || msg == RC_green) 
		{
			if (mplist && mplist->isPainted())
			{
				mplist->hide();
				doTMDB(playlist[mplist->getSelected()]);
				showPlaylist();
			}
			else
			{
				if (IsVisible())
				{ 
					hide();
				}
				
				CAVPIDSelectWidget * AVSelectHandler = new CAVPIDSelectWidget();
				AVSelectHandler->exec(NULL, "");
								
				delete AVSelectHandler;
				AVSelectHandler = NULL;
			}
		} 
		else if(msg == RC_yellow)
		{
			if (mplist && mplist->isPainted())
			{
				clearPlaylist();
				hide();
				showPlaylist();
			}
			else
			{
				if (IsVisible())
				{ 
					hide();
				}
				
				//show help
				showHelpTS();
			}
		}
		else if (msg == RC_info)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->hide();
				cMovieInfo.showMovieInfo(playlist[mplist->getSelected()]);
				showPlaylist();
			}
			else
			{
				if (!IsVisible()) 
					showMovieInfo();//FIXME:
				else
				{
					hide();
					cMovieInfo.showMovieInfo(playlist[selected]);
				}	
			}
		}
		else if(msg == RC_setup)
		{
			if (mplist && !mplist->isPainted())
			{
				hide();
				
				CMoviePlayerSettings* moviePlayerSettings = new CMoviePlayerSettings();

				moviePlayerSettings->exec(NULL, "");
				delete moviePlayerSettings;
				moviePlayerSettings = NULL;
			}
		} 
		else if (msg == RC_rewind) 
		{
			if (mplist && !mplist->isPainted())
			{
				// backward
				speed = (speed >= 0) ? -1 : speed - 1;
							
				if(speed < -15)
					speed = -15;			
				
				// hide icons
				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
				CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);

				playback->SetSpeed(speed);
				playstate = CMoviePlayerGui::REW;
				update_lcd = true;

				if (IsVisible()) 
				{
					hide();
				}
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		}
		else if (msg == RC_forward) 
		{	
			if (mplist && !mplist->isPainted())
			{
				// fast-forward
				speed = (speed <= 0) ? 2 : speed + 1;
							
				if(speed > 15)
					speed = 15;			
				
				// icons
				CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
				CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);

				playback->SetSpeed(speed);

				update_lcd = true;
				playstate = CMoviePlayerGui::FF;

				if (IsVisible())
				{ 
					hide();
				}

				// movie info viewer
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_1) 
		{
			if (mplist && !mplist->isPainted())
			{	
				// Jump Backwards 1 minute
				//update_lcd = true;
				playback->SetPosition(position - (60*1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_3) 
		{
			if (mplist && !mplist->isPainted())
			{	
				// Jump Forward 1 minute
				//update_lcd = true;
				playback->SetPosition(position + (60 *1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_4) 
		{	
			if (mplist && !mplist->isPainted())
			{
				// Jump Backwards 5 minutes
				playback->SetPosition(position - (5 * 60 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_6) 
		{	
			if (mplist && !mplist->isPainted())
			{
				// Jump Forward 5 minutes
				playback->SetPosition(position + (5 * 60 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_7) 
		{
			if (mplist && !mplist->isPainted())
			{	
				// Jump Backwards 10 minutes
				playback->SetPosition(position - (10 * 60 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_9) 
		{	
			if (mplist && !mplist->isPainted())
			{
				// Jump Forward 10 minutes
				playback->SetPosition(position + (10 * 60 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if ( msg == RC_2 )
		{	
			if (mplist && !mplist->isPainted())
			{
				// goto start
				playback->SetPosition(startposition * 1000);
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if ( msg == RC_loop )
		{
			if (mplist && !mplist->isPainted())
			{
				if(m_loop)
					m_loop = false;
				else
					m_loop = true;
				
				dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: Repeat Modus: [%s]\n", m_loop? "ON" : "OFF");
			}
		} 
		else if (msg == RC_5) 
		{
			if (mplist && !mplist->isPainted())
			{	
				// goto middle
				playback->SetPosition(duration/2);
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_8) 
		{
			if (mplist && !mplist->isPainted())
			{	
				// goto end
				playback->SetPosition(duration - (60 * 1000));
				
				//time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_page_up) 
		{
			if (mplist && mplist->isPainted())
			{
				mplist->scrollPageUp();
			}
			else
			{
				playback->SetPosition(position + (10 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_page_down) 
		{
			if (mplist && mplist->isPainted())
			{
				mplist->scrollPageDown();
			}
			else
			{
				playback->SetPosition(position - (10 * 1000));
				
				// time
				if (!IsVisible()) 
				{	
					time_forced = true;
					//showMovieInfo();//FIXME:
				}
			}
		} 
		else if (msg == RC_0) 
		{
			if (mplist && !mplist->isPainted())
			{
				// cancel bookmark
				if (new_bookmark.pos != 0) 
				{
					new_bookmark.pos = 0;	// stop current bookmark activity, TODO:  might bemoved to another key
					if(newBackwordHintBox.isPainted())
						newBackwordHintBox.hide();

					if(newForwardHintBox.isPainted())
						newForwardHintBox.hide();
				}
				jump_not_until = (position / 1000) + 10;
			}
		} 
#if !defined (PLATFORM_COOLSTREAM)		
		else if (msg == RC_slow) 
		{
			if (mplist && !mplist->isPainted())
			{
				if (slow > 0)
					slow = 0;
				
				slow += 2;
			
				// set slow
				playback->SetSlow(slow);
				//update_lcd = true;
				playstate = CMoviePlayerGui::SLOW;
				update_lcd = true;
			}
		}
#endif		
		else if(msg == RC_red)
		{
			if (mplist && mplist->isPainted())
			{
				// FIXME: segfault
				/*
				//mplist->scrollPageDown();
				mplist->hide();
				CMoviePlayList::iterator p = playlist.begin() + mplist->getSelected();
				playlist.erase(p);

				if (selected >= (int)playlist.size())
					selected = playlist.size() - 1;
			
				showPlaylist();
				*/
			}
			else
			{
				if (IsVisible())
					hide();
					
				cMovieInfo.showMovieInfo(playlist[selected]);
			}
		}
		else if(msg == RC_home)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->hide();
				//paintInfo(m_playlist[m_current]);
			}
			else
			{
				if (IsVisible()) 
				{
					hide();
				}
			}
		}
		else if(msg == RC_left || msg == RC_prev)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->swipLeft();
			}
			else
			{
				playPrev();
			}
		}
		else if(msg == RC_right || msg == RC_next)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->swipRight();
			}
			else
			{
				playNext();
			}
		}
		else if (msg == (neutrino_msg_t)g_settings.key_screenshot)
		{
			if (mplist && !mplist->isPainted())
			{
		 		if(MessageBox(_("Information"), _("create screenshot?"), mbrNo, mbYes | mbNo) == mbrYes) 
				{
					CVCRControl::getInstance()->Screenshot(0, (char *)playlist[selected].file.Name.c_str());
				}
			}
		}
		else if(msg == RC_ok)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->hide();
				play(mplist->getSelected());
			}
			else
			{
				hide();
				showPlaylist();
			}
		}
		else if(msg == RC_up)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->scrollLineUp();
			}
		}
		else if(msg == RC_down)
		{
			if (mplist && mplist->isPainted())
			{
				mplist->scrollLineDown();
			}
		}
		else if ((msg == NeutrinoMessages::ANNOUNCE_RECORD) || msg == NeutrinoMessages::RECORD_START || msg == NeutrinoMessages::ZAPTO || msg == NeutrinoMessages::STANDBY_ON || msg == NeutrinoMessages::SHUTDOWN || msg == NeutrinoMessages::SLEEPTIMER) 
		{	
			// Exit for Record/Zapto Timers
			exit = true;
			g_RCInput->postMsg(msg, data);
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (IsVisible())
			{ 
				hide();
			}
			
			if (mplist && mplist->isPainted())
				mplist->refresh();
		}
		else 
		{
			if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				exit = true;
			else if ( msg <= RC_MaxRC ) 
			{
				CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);
	
				update_lcd = true;
			}
		}

		if (exit) 
		{
			dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: stop (2)\n");	

			if(playlist[selected].ytid.empty() && playlist[selected].file.getType() != CFile::FILE_AUDIO)
			{
				// if we have a movie information, try to save the stop position
				ftime(&current_time);
				playlist[selected].dateOfLastPlay = current_time.time;
				current_time.time = time(NULL);
				playlist[selected].bookmarks.lastPlayStop = position / 1000;
				
				cMovieInfo.saveMovieInfo(playlist[selected]);
			}
		}

		frameBuffer->blit();
	} while (playstate >= CMoviePlayerGui::PLAY);
	
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::PlayFile: stop (4)\n");	

	if(IsVisible())
	{
		hide();
	}
	
	playback->Close();

	CVFD::getInstance()->ShowIcon(VFD_ICON_PLAY, false);
	CVFD::getInstance()->ShowIcon(VFD_ICON_PAUSE, false);
}

void CMoviePlayerGui::showHelpTS()
{
	CHelpBox helpbox;

	helpbox.addLine(NEUTRINO_ICON_BUTTON_RED, _("Movie info"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_GREEN, _("Select audio track"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_YELLOW, _("Help"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_BLUE, _("Create bookmark"));
	helpbox.addSeparator();
	helpbox.addLine(NEUTRINO_ICON_BUTTON_SETUP, _("Movieplayer settings"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HELP, _("show movie infoviewer"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, _("show playlist"));
	helpbox.addSeparator();
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0, _("skip bookmark"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_1, _("approx. 1 minute back"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_2, _("goto start") );
	helpbox.addLine(NEUTRINO_ICON_BUTTON_3, _("skip approx. 1 minute"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_4, _("approx. 5 minutes back"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, _("goto middle"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_6, _("skip approx. 5 minutes"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_7, _("approx. 10 minutes back"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_8, _("goto end"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_9, _("skip approx. 10 minutes"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_TOP, _("skip approx. 10 seconds"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_DOWN, _("approx. 10 seconds back"));

	hide();

	helpbox.show(_("Information"));
}

int CMoviePlayerGui::showStartPosSelectionMenu(void)
{
	dprintf(DEBUG_INFO, "CMoviePlayerGui::showStartPosSelectionMenu\r\n");
	
	int pos = -1;
	int result = 0;
	int menu_nr = 0;
	int position[MI_MOVIE_BOOK_USER_MAX];
	
	if(playlist[selected].bookmarks.lastPlayStop == 0 /*|| playlist[selected].bookmarks.start == 0*/)
		return(result);

	// reset all start pos
	playlist[selected].bookmarks.start = 0;
	
	char start_pos[13]; 
	snprintf(start_pos, 12,"%3d min", playlist[selected].bookmarks.start/60);
	
	char play_pos[13]; 	
	snprintf(play_pos, 12,"%3d min", playlist[selected].bookmarks.lastPlayStop/60); 
	
	char book[MI_MOVIE_BOOK_USER_MAX][20];

	//
	CWidget* widget = NULL;
	ClistBox* startPosSelectionMenu = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("startpos");
	
	if (widget)
	{
		startPosSelectionMenu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		startPosSelectionMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		//startPosSelectionMenu->setMenuPosition(MENU_POSITION_CENTER);
		startPosSelectionMenu->setWidgetMode(MODE_SETUP);
		startPosSelectionMenu->enableShrinkMenu();
			
		startPosSelectionMenu->enablePaintHead();
		startPosSelectionMenu->setTitle(_("Start movie from:"), NEUTRINO_ICON_MOVIE);

		startPosSelectionMenu->enablePaintFoot();
				
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
		startPosSelectionMenu->setFootButtons(&btn);
			
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "startpos";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(startPosSelectionMenu);
	}
	
	startPosSelectionMenu->clearItems();
	
	// bookmark start
	if(playlist[selected].bookmarks.start != 0)
	{
		startPosSelectionMenu->addItem(new ClistBoxItem(_("Movie start:"), true, start_pos));
		position[menu_nr++] = playlist[selected].bookmarks.start;
	}
	
	// bookmark laststop
	if(playlist[selected].bookmarks.lastPlayStop != 0) 
	{
		startPosSelectionMenu->addItem(new ClistBoxItem(_("Last play stop:"), true, play_pos));
		position[menu_nr++] = playlist[selected].bookmarks.lastPlayStop;
	}
	
	// movie start
	startPosSelectionMenu->addItem(new ClistBoxItem(_("Movie start"), true, NULL));

	position[menu_nr++] = 0;

	int sep_pos = menu_nr;

	for(int i = 0 ; i < MI_MOVIE_BOOK_USER_MAX && menu_nr < MI_MOVIE_BOOK_USER_MAX; i++ )
	{
		if( playlist[selected].bookmarks.user[i].pos != 0 )
		{
			if(playlist[selected].bookmarks.user[i].length >= 0)
				position[menu_nr] = playlist[selected].bookmarks.user[i].pos;
			else
				position[menu_nr] = playlist[selected].bookmarks.user[i].pos + playlist[selected].bookmarks.user[i].length;
				
			snprintf(book[i], 19,"%5d min", position[menu_nr]/60);

			dprintf(DEBUG_NORMAL, "CMoviePlayerGui::showStartPosSelectionMenu adding boomark menu N %d, position %d sec\n", menu_nr, position[menu_nr]);
			
			startPosSelectionMenu->addItem(new ClistBoxItem(playlist[selected].bookmarks.user[i].name.c_str(), true, book[i]));
			menu_nr++;
		}
	}

	widget->setTimeOut(/*SNeutrinoSettings::TIMING_INFOBAR*/10);
	widget->exec(NULL, "");
	
	// check what menu item was ok'd  and set the appropriate play offset
	result = startPosSelectionMenu->getSelected();
	
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::showStartPosSelectionMenu: result %d\n", result);

	if(result < 0)
		return -1;

	pos = position[result];
	
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::showStartPosSelectionMenu: selected bookmark %d position %d sec\n", result, pos);
	
	return(pos) ;
}

// InfoViewer
void CMoviePlayerGui::showMovieInfo()
{
	visible = true;

	show(playlist[selected].epgTitle, (playlist[selected].epgInfo1.empty())? playlist[selected].epgInfo2 : playlist[selected].epgInfo1, file_prozent, ac3state, speed, playstate, (playlist[selected].ytid.empty())? true : false, m_loop); //FIXME:

	startMovieInfoViewer();
}

void CMoviePlayerGui::initFrames()
{
	// movieinfo
	cFrameBoxInfo.iHeight = BOXHEIGHT_MOVIEINFO;
	cFrameBoxInfo.iWidth = g_settings.screen_EndX - g_settings.screen_StartX - BORDER_LEFT - BORDER_RIGHT;
	cFrameBoxInfo.iX = g_settings.screen_StartX + 10;
	cFrameBoxInfo.iY = g_settings.screen_EndY - 10 - cFrameBoxInfo.iHeight;

	//movieinfo buttonbar
	cFrameBoxButton.iWidth = cFrameBoxInfo.iWidth;
	cFrameBoxButton.iHeight = BUTTON_BAR_HEIGHT;
	cFrameBoxButton.iX = g_settings.screen_StartX + 10;
	cFrameBoxButton.iY = cFrameBoxInfo.iY + cFrameBoxInfo.iHeight - cFrameBoxButton.iHeight;
}

void CMoviePlayerGui::hide()
{
	//printf("CMovieInfoViewer::hide: x %d y %d xend %d yend %d\n", m_xstart, m_y , m_xend, m_height + 15);

	if(!visible)
		return;

	// hide infoviewer
	frameBuffer->paintBackgroundBoxRel(cFrameBoxInfo.iX - 1, cFrameBoxInfo.iY - 1, cFrameBoxInfo.iWidth + 2, cFrameBoxInfo.iHeight + 2);

	frameBuffer->blit();

	visible = false;

	killMovieInfoViewer();
}

//showMovieInfo
void CMoviePlayerGui::show(std::string Title, std::string Info, short Percent, const unsigned int ac3state, const int speed, const int playstate, bool show_bookmark, bool m_loop)
{
	dprintf(DEBUG_INFO, "CMoviePlayerGui::showMovieInfo:\n");
	
	// icons dimension
	frameBuffer->getIconSize(NEUTRINO_ICON_16_9, &icon_w_aspect, &icon_h_aspect);
	frameBuffer->getIconSize(NEUTRINO_ICON_DD, &icon_w_dd, &icon_h_dd);
	
	// colored user icons
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_red_w, &icon_red_h);
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_GREEN, &icon_green_w, &icon_green_h);
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_YELLOW, &icon_yellow_w, &icon_yellow_h);
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_BLUE, &icon_blue_w, &icon_blue_h);
	
	// border
	if (g_settings.infobar_border)
		frameBuffer->paintBoxRel(cFrameBoxInfo.iX, cFrameBoxInfo.iY, cFrameBoxInfo.iWidth, cFrameBoxInfo.iHeight, COL_MENUCONTENT_PLUS_6, g_settings.infobar_radius, g_settings.infobar_corner);
		
	// paint info box
	frameBuffer->paintBoxRel(g_settings.infobar_border? cFrameBoxInfo.iX + 2 : cFrameBoxInfo.iX, g_settings.infobar_border? cFrameBoxInfo.iY + 2 : cFrameBoxInfo.iY, g_settings.infobar_border? cFrameBoxInfo.iWidth - 4 : cFrameBoxInfo.iWidth, g_settings.infobar_border? cFrameBoxInfo.iHeight - 4 : cFrameBoxInfo.iHeight, COL_INFOBAR_PLUS_0, g_settings.infobar_radius, g_settings.infobar_corner, g_settings.infobar_gradient, g_settings.infobar_gradient_direction); 
		
	// botton bar
	if (g_settings.infobar_buttonbar)
	{
		frameBuffer->paintBoxRel(g_settings.infobar_border? cFrameBoxButton.iX + 2: cFrameBoxButton.iX, cFrameBoxButton.iY, g_settings.infobar_border? cFrameBoxButton.iWidth - 4 : cFrameBoxButton.iWidth, g_settings.infobar_border? cFrameBoxButton.iHeight - 2 : cFrameBoxButton.iHeight, COL_INFOBAR_SHADOW_PLUS_1, g_settings.infobar_radius, g_settings.infobar_radius? CORNER_BOTTOM : CORNER_NONE);
	}
	
	// button line
	if(g_settings.infobar_buttonline)
	{
		CCHline hline(cFrameBoxButton.iX + BORDER_LEFT, cFrameBoxButton.iY, cFrameBoxButton.iWidth - BORDER_LEFT - BORDER_RIGHT, cFrameBoxButton.iHeight);
		
		hline.setColor(COL_INFOBAR_SHADOW_PLUS_1);
		hline.setGradient(3);
		hline.paint();
	}
	
	// show date/time
	std::string datestr = getNowTimeStr("%d.%m.%Y %H:%M");
			
	int widthtime = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(datestr.c_str(), true); //UTF-8
	int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
			
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(cFrameBoxInfo.iX + cFrameBoxInfo.iWidth - BORDER_RIGHT - widthtime, cFrameBoxInfo.iY + 5 + height, widthtime, datestr.c_str(), COL_INFOBAR, 0, true); // UTF-8
	
	std::string title = "Movieplayer";
	int widthtitle = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(title.c_str(), true); //UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(cFrameBoxInfo.iX + BORDER_LEFT, cFrameBoxInfo.iY + 5 + height, widthtitle, _(title.c_str()), COL_INFOBAR, 0, true); // UTF-8
	
	// red (movie info)
	int icon_w, icon_h;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_RED, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, cFrameBoxButton.iX + BORDER_LEFT, cFrameBoxButton.iY + 1, 0, true, icon_w, cFrameBoxButton.iHeight - 2);

	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(cFrameBoxButton.iX + BORDER_LEFT + icon_w + ICON_OFFSET, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight(), cFrameBoxButton.iWidth/5, (char *)"Info", (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
	// green (AV select)
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_GREEN, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, cFrameBoxButton.iX + cFrameBoxButton.iWidth/5, cFrameBoxButton.iY + 1, 0, true, icon_w, cFrameBoxButton.iHeight - 2);

	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(cFrameBoxButton.iX + (cFrameBoxButton.iWidth/5) + icon_w + ICON_OFFSET, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight(), cFrameBoxButton.iWidth/5, _("Audio"), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
		
	// yellow (help)
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_YELLOW, &icon_w, &icon_h);
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, cFrameBoxButton.iX + (cFrameBoxButton.iWidth/5)*2, cFrameBoxButton.iY + 1, 0, true, icon_w, cFrameBoxButton.iHeight - 2);

	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(cFrameBoxButton.iX + (cFrameBoxButton.iWidth/5)*2 + icon_w + ICON_OFFSET, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight(), cFrameBoxButton.iWidth/5, _("Help"), (COL_INFOBAR_SHADOW * 1), 0, true); // UTF-8
	
	// blue (bookmark)
	if (show_bookmark)
	{
		frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_BLUE, &icon_w, &icon_h);
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, cFrameBoxButton.iX + (cFrameBoxButton.iWidth/5)*3, cFrameBoxButton.iY + 1, 0, true, icon_w, cFrameBoxButton.iHeight - 2);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(cFrameBoxButton.iX + (cFrameBoxButton.iWidth/5)*3 + icon_w + ICON_OFFSET, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight(), cFrameBoxButton.iWidth/5, _("Bookmarks"), (COL_INFOBAR_SHADOW + 1), 0, true); // UTF-8
	}
		
	// ac3
	int icon_w_ac3, icon_h_ac3;
	frameBuffer->getIconSize(NEUTRINO_ICON_DD, &icon_w_ac3, &icon_h_ac3);
	frameBuffer->paintIcon( (ac3state == CInfoViewer::AC3_ACTIVE)?NEUTRINO_ICON_DD : NEUTRINO_ICON_DD_GREY, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h_ac3)/2);
		
	// 4:3/16:9
	const char * aspect_icon = NEUTRINO_ICON_16_9_GREY;
				
	if(g_settings.video_Ratio == ASPECTRATIO_169)
		aspect_icon = NEUTRINO_ICON_16_9;
	
	int icon_w_asp, icon_h_asp;
	frameBuffer->getIconSize(aspect_icon, &icon_w_asp, &icon_h_asp);
	frameBuffer->paintIcon(aspect_icon, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h_asp)/2);
	
	// mp keys
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_FF_SMALL, &icon_w, &icon_h);
		
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_REW_SMALL, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp - ICON_TO_ICON_OFFSET - 5*icon_w, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h)/2);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_PLAY_SMALL, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp - ICON_TO_ICON_OFFSET - 4*icon_w, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h)/2);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_PAUSE_SMALL, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp - ICON_TO_ICON_OFFSET - 3*icon_w, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h)/2);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_STOP_SMALL, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp - ICON_TO_ICON_OFFSET - 2*icon_w, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h)/2);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_FF_SMALL, cFrameBoxButton.iX + cFrameBoxButton.iWidth - BORDER_RIGHT - icon_w_ac3 - ICON_TO_ICON_OFFSET - icon_w_asp - ICON_TO_ICON_OFFSET - icon_w, cFrameBoxButton.iY + (cFrameBoxButton.iHeight - icon_h)/2);
		
	//playstate
	const char* icon = NEUTRINO_ICON_PLAY;
		
	switch(playstate)
	{
		case CMoviePlayerGui::PAUSE: icon = NEUTRINO_ICON_PAUSE; break;
		case CMoviePlayerGui::PLAY: icon = NEUTRINO_ICON_PLAY; break;
		case CMoviePlayerGui::REW: icon = NEUTRINO_ICON_REW; break;
		case CMoviePlayerGui::FF: icon = NEUTRINO_ICON_FF; break;
		case CMoviePlayerGui::SOFTRESET: break;
		case CMoviePlayerGui::SLOW: break;
		case CMoviePlayerGui::STOPPED: break;
	}

	if(m_loop && playstate == CMoviePlayerGui::PLAY)
		icon = NEUTRINO_ICON_REPEAT_TRACK;
	
	frameBuffer->getIconSize(icon, &icon_w, &icon_h);

	//
	int icon_x = cFrameBoxButton.iX + BORDER_LEFT + ICON_OFFSET;
	int icon_y = cFrameBoxInfo.iY + 30 + TIMESCALE_BAR_HEIGHT + (cFrameBoxInfo.iHeight - 30 - TIMESCALE_BAR_HEIGHT - cFrameBoxButton.iHeight - icon_h) / 2;

	frameBuffer->paintIcon(icon, icon_x, icon_y);
		
	// paint speed
	char strSpeed[4];
	if( playstate == CMoviePlayerGui::FF || playstate == CMoviePlayerGui::REW )
	{
		sprintf(strSpeed, "%d", speed);
		
		//FIXME:??? position
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(icon_x + icon_w + ICON_OFFSET, icon_y + (icon_h - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight(), cFrameBoxInfo.iWidth/5, strSpeed, COL_INFOBAR ); // UTF-8
	}
		
	int speedWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("-8");
		
	int InfoStartX = cFrameBoxInfo.iX + ICON_OFFSET + BORDER_LEFT + icon_w + ICON_OFFSET + speedWidth + 2*ICON_OFFSET;
	int InfoWidth = cFrameBoxInfo.iWidth - BORDER_LEFT - BORDER_RIGHT -2*ICON_OFFSET - icon_w - speedWidth - 2*ICON_OFFSET - 2*BORDER_LEFT;
	
	// title
	int TitleHeight = cFrameBoxInfo.iY + 30 + TIMESCALE_BAR_HEIGHT + (cFrameBoxInfo.iHeight - (30 + TIMESCALE_BAR_HEIGHT + cFrameBoxButton.iHeight) -2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight())/2 + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();	//40???

	int t_w = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth("00:00:00 / 00:00:00");
	
	// Title	
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoStartX, TitleHeight, InfoWidth - t_w, (char *)Title.c_str(), COL_INFOBAR, 0, true);
	
	//
	int t_h = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	
	//
	timeCounter = new CCCounter(cFrameBoxInfo.iX + cFrameBoxInfo.iWidth - 5 - t_w, cFrameBoxInfo.iY + 30 + TIMESCALE_BAR_HEIGHT + (cFrameBoxInfo.iHeight - (30 + TIMESCALE_BAR_HEIGHT + cFrameBoxButton.iHeight) -2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight())/2, t_w, t_h);

	// position/duration
	time_t tDisplayTime = position/1000;
	time_t dDisplayTime = duration/1000;
	
	timeCounter->setTotalTime(dDisplayTime);
	timeCounter->setPlayTime(tDisplayTime);
	
	timeCounter->paint();
	
	// Info
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoStartX, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() + TitleHeight, InfoWidth, (char *)Info.c_str(), COL_INFOBAR, 0, true);
	
	// progressbar
	if(Percent < 0)
		Percent = 0;
	
	if(Percent > 100)
		Percent = 100;
	
	moviescale->reset();
	moviescale->paint(/*cFrameBoxInfo.iX + BORDER_LEFT, cFrameBoxInfo.iY + 30,*/ Percent);
}

void CMoviePlayerGui::updateTime()
{
	time_t tDisplayTime = position/1000;
	time_t dDisplayTime = duration/1000;
	
	timeCounter->setTotalTime(dDisplayTime);
	timeCounter->setPlayTime(tDisplayTime);
	
	timeCounter->refresh();
}

//
const struct button_label HeadButtons = { NEUTRINO_ICON_BUTTON_HELP, ""} ;

#define FOOT_BUTTONS_COUNT	4
const struct button_label FootButtons[FOOT_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Delete") },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("TMDB info") },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Delete all") },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Add") },
};

void CMoviePlayerGui::showPlaylist()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::showPlaylist:\n");
	
	mplist->clear();

	for(unsigned int i = 0; i < playlist.size(); i++)
	{
		item = new ClistBoxItem(playlist[i].epgTitle.c_str());

		item->setOption(playlist[i].epgChannel.c_str());

		item->setInfo1(playlist[i].epgInfo1.c_str());

		item->setInfo2(playlist[i].epgInfo2.c_str());

		item->setHintIcon(file_exists(playlist[i].tfile.c_str())? playlist[i].tfile.c_str() : DATADIR "/icons/nopreview.jpg");

		item->set2lines();

		std::string tmp = playlist[i].epgInfo1;
		tmp += "\n";
		tmp += playlist[i].epgInfo2;

		item->setHint(tmp.c_str());

		mplist->addItem(item);
	}

	mplist->setWidgetType(TYPE_FRAME);
	mplist->setItemsPerPage(6, 2);
	
	mplist->setSelected(selected);
	
	mplist->enablePaintHead();
	mplist->setTitle(_("Movieplayer"), NEUTRINO_ICON_MOVIE);
	mplist->enablePaintDate();
	mplist->setFormat("%d.%m.%Y %H:%M:%S");
	mplist->setHeadButtons(&HeadButtons);
	mplist->setHeadLine(true, true);
	
	mplist->enablePaintFoot();
	mplist->setFootButtons(FootButtons, FOOT_BUTTONS_COUNT);
	mplist->setFootLine(true, true);
	
	mplist->enablePaintItemInfo();
	
	mplist->paint();
}

//
void CMoviePlayerGui::openMovieFileBrowser()
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::openMovieFileBrowser:\n");
	
	CFileFilter fileFilter;
	CFileList filelist;
	
	CFileBrowser filebrowser((g_settings.filebrowser_denydirectoryleave) ? g_settings.network_nfs_recordingdir : "");

	fileFilter.clear();
	filelist.clear();

	fileFilter.addFilter("ts");
	fileFilter.addFilter("mpg");
	fileFilter.addFilter("mpeg");
	fileFilter.addFilter("divx");
	fileFilter.addFilter("avi");
	fileFilter.addFilter("mkv");
	fileFilter.addFilter("asf");
	fileFilter.addFilter("aiff");
	fileFilter.addFilter("m2p");
	fileFilter.addFilter("mpv");
	fileFilter.addFilter("m2ts");
	fileFilter.addFilter("vob");
	fileFilter.addFilter("mp4");
	fileFilter.addFilter("mov");	
	fileFilter.addFilter("flv");	
	fileFilter.addFilter("dat");
	fileFilter.addFilter("trp");
	fileFilter.addFilter("vdr");
	fileFilter.addFilter("mts");
	fileFilter.addFilter("wmv");
	fileFilter.addFilter("wav");
	fileFilter.addFilter("flac");
	fileFilter.addFilter("mp3");
	fileFilter.addFilter("wma");
	fileFilter.addFilter("ogg");

	filebrowser.Multi_Select = true;
	filebrowser.Dirs_Selectable = true;
	filebrowser.Filter = &fileFilter;

	std::string Path = g_settings.network_nfs_recordingdir;

	if (filebrowser.exec(Path.c_str()))
	{
		Path = filebrowser.getCurrentDir();

		MI_MOVIE_INFO movieInfo;
		cMovieInfo.clearMovieInfo(&movieInfo); // refresh structure

		CFileList::const_iterator files = filebrowser.getSelectedFiles().begin();
		for(; files != filebrowser.getSelectedFiles().end(); files++)
		{
			// filter them
			MI_MOVIE_INFO movieInfo;
			cMovieInfo.clearMovieInfo(&movieInfo); // refresh structure
					
			movieInfo.file.Name = files->Name;
					
			// load movie infos (from xml file)
			cMovieInfo.loadMovieInfo(&movieInfo);

			// skip duplicate
			for (unsigned long i = 0; i < playlist.size(); i++)
			{
				if(playlist[i].file.getFileName() == movieInfo.file.getFileName())
					playlist.erase(playlist.begin() + i); 
			}
					
			// 
			playlist.push_back(movieInfo);
		}
	}
}

void CMoviePlayerGui::doTMDB(MI_MOVIE_INFO& movieFile)
{
	dprintf(DEBUG_NORMAL, "CMoviePlayerGui::CMoviePlayerGui::doTMDB:\n");
	
	//				
	CTmdb * tmdb = new CTmdb();

	tmdb->clearMInfo();

	if(tmdb->getMovieInfo(movieFile.epgTitle))
	{
		std::vector<tmdbinfo>& minfo_list = tmdb->getMInfos();

		std::string buffer;

		buffer = movieFile.epgTitle;
		buffer += "\n\n";
	
		// prepare print buffer  
		buffer += "Vote: " + to_string(minfo_list[0].vote_average) + "/10 Votecount: " + to_string(minfo_list[0].vote_count);
		buffer += "\n\n";
		buffer += minfo_list[0].overview;
		buffer += "\n";

		buffer += (std::string)_("Length (Min)") + ": " + to_string(minfo_list[0].runtime);
		buffer += "\n";

		buffer += (std::string)_("Genre") + ": " + minfo_list[0].genres;
		buffer += "\n";
		buffer += (std::string)_("Original Title") + " : " + minfo_list[0].original_title;
		buffer += "\n";
		buffer += (std::string)_("Year of production") + " : " + minfo_list[0].release_date.substr(0,4);
		buffer += "\n";

		if (!minfo_list[0].cast.empty())
			buffer += (std::string)_("Actors") + ":\n" + minfo_list[0].cast;

		// thumbnail
		std::string tname = tmdb->getThumbnailDir();
		tname += "/";
		tname += movieFile.epgTitle;
		tname += ".jpg";

		tmdb->getSmallCover(minfo_list[0].poster_path, tname);
		
		// scale pic
		int p_w = 0;
		int p_h = 0;

		scaleImage(tname, &p_w, &p_h);
	
		CBox position(g_settings.screen_StartX + 50, g_settings.screen_StartY + 50, g_settings.screen_EndX - g_settings.screen_StartX - 100, g_settings.screen_EndY - g_settings.screen_StartY - 100); 
	
		CInfoBox * infoBox = new CInfoBox(&position, movieFile.epgTitle.c_str(), NEUTRINO_ICON_TMDB);

		infoBox->setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO1);
		infoBox->setMode(SCROLL);
		infoBox->setText(buffer.c_str(), tname.c_str(), p_w, p_h);
		infoBox->exec();
		delete infoBox;

		if(MessageBox(_("Information"), _("Prefer TMDB Infos"), mbrNo, mbYes | mbNo) == mbrYes) 
		{
			// tfile
			std::string tname = movieFile.file.Name;
			changeFileNameExt(tname, ".jpg");

			if(tmdb->getBigCover(minfo_list[0].poster_path, tname)) 
				movieFile.tfile = tname;

			// epgInfo1
			if(movieFile.epgInfo1.empty())
				movieFile.epgInfo1 = buffer;
			
			// productionDate	
			if (movieFile.productionDate == 0)
				movieFile.productionDate = atoi(minfo_list[0].release_date.substr(0,4));
			
			// genres	
			if(movieFile.genres.empty())
				movieFile.genres = minfo_list[0].genres;
				
			// average
			if (movieFile.vote_average == 0)
				movieFile.vote_average = minfo_list[0].vote_average;

			cMovieInfo.saveMovieInfo(movieFile);
		}  
	}
	else
	{
		MessageBox(_("Information"), _("TMDB Infos are not available"), mbrBack, mbBack, NEUTRINO_ICON_INFO);
	}

	delete tmdb;
	tmdb = NULL;
}

