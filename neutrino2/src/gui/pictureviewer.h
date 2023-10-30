/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#ifndef __pictureviewergui__
#define __pictureviewergui__

#include <string>

#include <driver/framebuffer.h>
#include <driver/pictureviewer/pictureviewer.h>
#include <gui/widget/widget.h>
#include <gui/filebrowser.h>


class CPicture
{
	public:
		std::string Filename;
		std::string Name;
		std::string Type;
		time_t Date;
};

typedef std::vector<CPicture> CPicturePlayList;

//
class CPictureViewerGui : public CMenuTarget
{
	public:
		enum State
		{
			SINGLE = 0,
			SLIDESHOW
		};
		
	private:

		CFrameBuffer* frameBuffer;

		//
		CPictureViewer* g_PicViewer;

		//
		unsigned int selected;		
		State m_state;

		CPicturePlayList playlist;

		long m_time;
		int m_LastMode;

		void view(unsigned int nr);
		void show();
		void paintLCD();
		void hide();
		
	public:
		CPictureViewerGui();
		~CPictureViewerGui();
		int  exec(CMenuTarget* parent, const std::string& actionKey);

		void addToPlaylist(CPicture& file);
		void addToPlaylist(const CFile& file);
		void addToPlaylist(const char* fileName);
		void clearPlaylist(void);
		void removeFromPlaylist(long pos);

		void setState(State state = SINGLE){m_state = state;};
		void showHelp();
		int getSelected(void){return selected;};
		void setCurrent(int pos){selected = pos;};
};

#endif


