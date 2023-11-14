/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: filebrowser.h 2018/08/24 mohousch Exp $

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


#ifndef __FILEBROWSER_HPP__
#define __FILEBROWSER_HPP__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/file.h>

#include <driver/gfx/framebuffer.h>
#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/color.h>

#include <driver/rcinput.h>

#include <gui/infoviewer.h>

#include <gui/widget/listbox.h>

#include <system/settings.h>

#include <string>
#include <vector>


#define FILEBROWSER_NUMBER_OF_SORT_VARIANTS 5

class CFileBrowser
{
	private:
		CFrameBuffer *frameBuffer;
		CBox cFrameBox;
		CWidget *widget;
		ClistBox *listBox;
		CMenuItem *item;
		
		uint32_t sec_timer_id;

		CFileList selected_filelist;
		CFileList filelist;
		
		std::string Path;
		
		/**
		 * @param selection select the specified entry, ignored if selection == -1
		 */
		void ChangeDir(const std::string & filename, int selection = -1);
		
		void SMSInput(const neutrino_msg_t msg);

		unsigned int selected;
		bool exit_pressed;

		std::vector<unsigned int> selections;
		
		std::string name;
		std::string base;
		
		bool bCancel;

		SMSKeyInput m_SMSKeyInput;

		void paint();
		void recursiveDelete(const char* file);
		void hide();

	protected:
		void commonInit();

	public:
		bool Multi_Select;
		bool Dirs_Selectable;
		bool Dir_Mode;
		bool use_filter;
		
		CFileFilter * Filter;

		CFileBrowser();
		CFileBrowser(const char * const _base);
		~CFileBrowser();

		bool exec(const char * const dirname);
		CFile *getSelectedFile();
		
		inline const CFileList &getSelectedFiles(void) const
		{
			return selected_filelist;
		}

		inline const std::string & getCurrentDir(void) const
		{
			return Path;
		}

		//
		bool getExitPressed(){return exit_pressed;};

		bool readDir(const std::string & dirname, CFileList* flist);
		void addRecursiveDir(CFileList * re_filelist, std::string path, bool bRootCall);
};

#endif

