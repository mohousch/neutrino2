/*
	Neutrino-GUI  -   DBoxII-Project

	MP3Player by Dirch
	
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

#include <gui/pictureviewer.h>

#include <global.h>
#include <neutrino.h>

#include <daemonc/remotecontrol.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/nfs.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

// remove this
#include <gui/widget/hintbox.h>

#include <gui/widget/helpbox.h>
#include <gui/widget/stringinput.h>

#include <gui/pictureviewer_setup.h>

#include <system/settings.h>
#include <system/debug.h>

#include <algorithm>
#include <sys/stat.h>
#include <sys/time.h>


CPictureViewerGui::CPictureViewerGui()
{
	dprintf(DEBUG_INFO, "CPictureViewerGui::CPictureViewerGui()\n");
	
	frameBuffer = CFrameBuffer::getInstance();

	m_state = SINGLE;

	g_PicViewer = new CPictureViewer();

	selected = 0;
}

CPictureViewerGui::~CPictureViewerGui()
{
	playlist.clear();

	// free picviewer mem
	g_PicViewer->cleanup();
	delete g_PicViewer;
	g_PicViewer = NULL;
}

void CPictureViewerGui::hide()
{
	frameBuffer->paintBackground();
	frameBuffer->blit();
}

int CPictureViewerGui::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_NORMAL, "CPictureViewerGui::exec: actionKey:%s\n", actionKey.c_str());
	
	// 
	g_PicViewer->setScaling( (ScalingMode)g_settings.picviewer_scaling);
	g_PicViewer->setVisible(g_settings.screen_StartX, g_settings.screen_EndX, g_settings.screen_StartY, g_settings.screen_EndY);

	if(g_settings.video_Ratio == 1)
		g_PicViewer->setAspectRatio(16.0/9);
	else
		g_PicViewer->setAspectRatio(4.0/3);

	if(parent)
		parent->hide();

	// tell neutrino we're in pic_mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_pic );
	
	// remember last mode
	m_LastMode = (CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	
	// Save and Clear background
	bool usedBackground = frameBuffer->getuseBackground();
	if (usedBackground) 
	{
		frameBuffer->saveBackgroundImage();
		frameBuffer->clearFrameBuffer();
		frameBuffer->blit();
	}
	
	//
	CNeutrinoApp::getInstance()->lockPlayBack();

	show();

	// free picviewer mem
	g_PicViewer->cleanup();

	// Restore previous background	
	if (usedBackground) 
	{
		frameBuffer->restoreBackgroundImage();
		frameBuffer->useBackground(true);
	}
	
	//
	CNeutrinoApp::getInstance()->unlockPlayBack();

	// Restore last mode
	CNeutrinoApp::getInstance()->handleMsg(NeutrinoMessages::CHANGEMODE, m_LastMode);

	if(!playlist.empty())
		playlist.clear();

	// 
	return RETURN_EXIT;
}

void CPictureViewerGui::show()
{
	dprintf(DEBUG_NORMAL, "CPictureViewerGui::show\n");

	neutrino_msg_t msg;
	neutrino_msg_data_t data;

	CVFD::getInstance()->setMode(CVFD::MODE_PIC, _("Pictureviewer"));

	int timeout;

	bool loop = true;

	//		
	if (!playlist.empty())
		view(selected);

	if (m_state == SLIDESHOW)
	{
		m_time = (long)time(NULL);
	}
	
	while(loop)
	{
		if(m_state != SLIDESHOW)
		{
			timeout = 10; 
		}
		else
		{
			timeout = (int)(m_time + g_settings.picviewer_slide_time - (long)time(NULL))*10;
			if(timeout < 0 )
				timeout = 1;
			else if(timeout >= 999)
				timeout = 999;
		}

		g_RCInput->getMsg(&msg, &data, timeout);

		if( msg == RC_home)
		{
			loop = false;
		}
		else if (msg == RC_timeout)
		{
			if(m_state == SLIDESHOW)
			{
				m_time = (long)time(NULL);
				unsigned int next = selected + 1;
				if (next >= playlist.size())
					next = 0;
				
				view(next);
			}
		}
		else if (msg == RC_left)
		{
			if (!playlist.empty())
			{
				view((selected == 0) ? (playlist.size() - 1) : (selected - 1));
			}
		}
		else if (msg == RC_right)
		{
			if (!playlist.empty())
			{
				unsigned int next = selected + 1;
				if (next >= playlist.size())
					next = 0;
				view(next);
			}
		}
		else if( msg == RC_1 )
		{ 
			g_PicViewer->zoom(2.0/3);
		}
		else if( msg == RC_2 )
		{ 
			g_PicViewer->move(0, -10);
		}
		else if( msg == RC_3 )
		{ 
			g_PicViewer->zoom(1.5);
		}
		else if( msg == RC_4 )
		{ 
			g_PicViewer->move(-10, 0);
		}
		else if( msg == RC_6 )
		{ 
			if (playlist.empty())
			{
				g_PicViewer->move(10, 0);
			}
		}
		else if( msg == RC_8 )
		{ 
			if (playlist.empty())
			{
				g_PicViewer->move(0, 10);
			}
		}
		else if(msg == RC_0)
		{
			if (!playlist.empty())
				view(selected);
		}
		else if(msg == RC_setup)
		{
			CPictureViewerSettings * pictureViewerSettingsMenu = new CPictureViewerSettings();
			pictureViewerSettingsMenu->exec(NULL, "");
			delete pictureViewerSettingsMenu;
			pictureViewerSettingsMenu = NULL;					
		}
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) !=NeutrinoMessages::mode_pic)
			{
				loop = false;
				m_LastMode = data;
			}
		}
		else if(msg == NeutrinoMessages::RECORD_START ||
				  msg == NeutrinoMessages::ZAPTO ||
				  msg == NeutrinoMessages::STANDBY_ON ||
				  msg == NeutrinoMessages::SHUTDOWN ||
				  msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}

		frameBuffer->blit();	
	}

	hide();
}

void CPictureViewerGui::view(unsigned int index)
{
	selected = index;
	
#if ENABLE_LCD
	CVFD::getInstance()->showMenuText(0, playlist[index].Name.c_str());
#else	
	CVFD::getInstance()->showMenuText(selected + 1, playlist[index].Name.c_str());
#endif
	
	g_PicViewer->showImage(playlist[index].Filename);
}

void CPictureViewerGui::addToPlaylist(CPicture& file)
{	
	dprintf(DEBUG_DEBUG, "CPictureViewerGui::addToPlaylist: %s\n", file.Filename.c_str());
	
	playlist.push_back(file);
}

void CPictureViewerGui::addToPlaylist(const CFile& file)
{
	dprintf(DEBUG_DEBUG, "CPictureViewerGui::addToPlaylist: %s\n", file.Name.c_str());

	CPicture pic;
	struct stat statbuf;
				
	pic.Filename = file.Name;
	std::string tmp = file.Name.substr(file.Name.rfind('/') + 1);
	pic.Name = tmp.substr(0, tmp.rfind('.'));
	pic.Type = tmp.substr(tmp.rfind('.') + 1);
				
	if(stat(pic.Filename.c_str(), &statbuf) != 0)
		printf("stat error");
	pic.Date = statbuf.st_mtime;

	playlist.push_back(pic);
}

void CPictureViewerGui::addToPlaylist(const char* fileName)
{
	dprintf(DEBUG_DEBUG, "CPictureViewerGui::addToPlaylist: %s\n", fileName);

	CFile file;
	file.Name = fileName;

	CPicture pic;
	struct stat statbuf;
				
	pic.Filename = fileName;
	std::string tmp = file.Name.substr(file.Name.rfind('/') + 1);
	pic.Name = tmp.substr(0, tmp.rfind('.'));
	pic.Type = tmp.substr(tmp.rfind('.') + 1);
				
	if(stat(pic.Filename.c_str(), &statbuf) != 0)
		printf("stat error");
	pic.Date = statbuf.st_mtime;

	playlist.push_back(pic);
}

void CPictureViewerGui::clearPlaylist(void)
{
	dprintf(DEBUG_NORMAL, "CPictureViewerGui::clearPlaylist:\n");

	if (!playlist.empty())
	{
		playlist.clear();
		selected = 0;
	}
}

void CPictureViewerGui::removeFromPlaylist(long pos)
{
	dprintf(DEBUG_NORMAL, "CPictureViewerGui::removeFromPlayList:\n");

	playlist.erase(playlist.begin() + pos); 
}

void CPictureViewerGui::showHelp()
{
	CHelpBox helpbox;

	helpbox.addLine(_("Menu mode"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, _("Display image"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0, _("Do not scale picture"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, _("Change sort order"));
	helpbox.addPagebreak();
	helpbox.addLine(_("Diashow mode"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_LEFT, _("Previous image"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RIGHT, _("Next image"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, _("Change sort order"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HOME, _("Exit"));
	helpbox.addPagebreak();
	helpbox.addLine(_("Show mode"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_OKAY, _("Reread image"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_LEFT, _("Previous image"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_RIGHT, _("Next image"));
	helpbox.addPagebreak();
	helpbox.addLine(NEUTRINO_ICON_BUTTON_0, _("Reread image (no scaling)"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_1, _("Zoom out"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_2, _("Scroll up"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_3, _("Zoom in"));
	//helpbox.addPagebreak();
	helpbox.addLine(NEUTRINO_ICON_BUTTON_4, _("Scroll left"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_5, _("Change sort order"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_6, _("Scroll right"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_8, _("Scroll down"));
	helpbox.addLine(NEUTRINO_ICON_BUTTON_HOME, _("Exit"));
	//helpbox.addPagebreak();
	//helpbox.addLine("Version: $Revision: 2.0 $");

	hide();
	helpbox.show(_("Information"));
}


