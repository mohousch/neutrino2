/*
  Neutrino-GUI  -   DBoxII-Project
  
  $Id: audioplayer.h 2018/07/10 mohousch Exp $

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

#ifndef __audioplayergui__
#define __audioplayergui__

#include <driver/framebuffer.h>
#include <driver/audiofile.h>
#include <driver/audioplay.h>

#include <gui/filebrowser.h>
#include <gui/widget/widget.h>

#include <string>
#include <set>
#include <map>
#include <cstdlib>
#include <ctime>


class CAudioPlayerGui : public CMenuTarget
{
	public:
		enum State
		{
			PLAY = 0,
			STOP,
			PAUSE,
			FF,
			REV
		};

		enum
		{
			REPEAT_NONE = 0,
			REPEAT_TRACK,
			REPEAT_ALL
		};

	private:
		CFrameBuffer* m_frameBuffer;
		CBox cFrameBox;

		//
		int m_current;
		bool exit_pressed;
		bool update_t;

		//
		State          m_state;
		time_t         m_time_total;
		time_t         m_time_played;
		std::string    m_metainfo;

		int repeatMode;

		//
		CAudioPlayList m_playlist;

		int            m_LastMode;
		bool           m_inetmode;
		bool updateMeta;
		bool updateLcd;
		bool updateScreen;
	
		//
		void Init(void);

		// gui
		void hide();
		void paintInfo(CAudiofile& File);
		void paintFanArt(CAudiofile& File);

		// lcd
		void paintLCD();

		//
		void get_id3(CAudiofile* audiofile);
		void get_mp3info(CAudiofile* audiofile);

		//
		int getNext();
		void GetMetaData(CAudiofile& File);
		void updateMetaData();
		void updateTimes(const bool force = false, bool paint = true);
		CCCounter* timeCounter;
		
		/**
		* Appends the file information to the given string.
		* @param fileInfo a string where the file information will be appended
		* @param file the file to return the information for
		*/
		void getFileInfoToDisplay(std::string& fileInfo, CAudiofile& file);

		/**
		* Saves the current playlist into a .m3u playlist file.
		*/
		void savePlaylist();

		/**
		* Converts an absolute filename to a relative one
		* as seen from a file in fromDir.
		* Example:
		* absFilename: /mnt/audio/A/abc.mp3
		* fromDir: /mnt/audio/B
		* => ../A/abc.mp3 will be returned 
		* @param fromDir the directory from where we want to
		* access the file
		* @param absFilename the file we want to access in a
		* relative way from fromDir (given as an absolute path)
		* @return the location of absFilename as seen from fromDir
		* (relative path)
		*/
		std::string absPath2Rel(const std::string& fromDir, const std::string& absFilename);
	
		/** 
		* Asks the user if the file filename should be overwritten or not
		* @param filename the name of the file
		* @return true if file should be overwritten, false otherwise
		*/
		bool askToOverwriteFile(const std::string& filename);

		//
		void play(unsigned int pos);
		void stop();
		void pause();
		void ff(unsigned int seconds = 0);
		void rev(unsigned int seconds = 0);
		bool playNext(bool allow_rotate = false);
		bool playPrev(bool allow_rotate = false);
		bool shufflePlaylist(void);
		void playFile();
		
		//
		ClistBox* alist;
		CMenuItem* item;
		uint32_t sec_timer_id = 0;
		void showPlaylist();
	
	public:
		CAudioPlayerGui();
		~CAudioPlayerGui();

		int exec(CMenuTarget *parent, const std::string &actionKey);

		//
		void addToPlaylist(CAudiofile& file);
		void addToPlaylist(const CFile& file);
		void addToPlaylist(const char* fileName);
		void removeFromPlaylist(long pos);
		void clearPlaylist(void);

		//
		void setInetMode(void){m_inetmode = true;};
		void setCurrent(int pos = 0){m_current = pos;};
		void showHelp();

		//
		bool getExitPressed(){return exit_pressed;};
};

#endif
