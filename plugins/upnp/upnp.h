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

#ifndef __upnpplayergui__
#define __upnpplayergui__

#include <string>
#include <sstream>
#include <upnpclient.h>

#include <driver/gfx/framebuffer.h>

#include <driver/audiofile.h>
#include <driver/pictureviewer.h>
#include <gui/filebrowser.h>

#include <system/helpers.h>


struct UPnPResource
{
	std::string	url;
	std::string	protocol;
	std::string	size;
	std::string	duration;
};

struct UPnPEntry
{
	std::string	id;
	bool		isdir;
	std::string	title;
	std::string	artist;
	std::string	album;
	std::string	children;
	std::vector<UPnPResource> resources;
	int		preferred;
};

class CUpnpBrowserGui : public CMenuTarget
{
	private:
		std::vector<CUPnPDevice> m_devices;
		std::vector<UPnPEntry>* entries;
		CUPnPSocket* m_socket;

		// gui
		typedef enum {
			UPNP_GUI_DEVICE = 0,
			UPNP_GUI_ENTRY,
			UPNP_GUI_SUBENTRY
		}UPNP_GUI;

		CFrameBuffer * m_frameBuffer;

		CBox cFrameBox;
		ClistBox *listBox;
		CMenuItem *item;
		uint32_t sec_timer_id;

		UPNP_GUI gui;

		unsigned int selected;
		std::string thumbnail_dir;
		CFileHelpers fileHelper;

		//
		CAudioPlayerGui tmpAudioPlayerGui;
		CPictureViewerGui tmpPictureViewerGui;
		CMoviePlayerGui tmpMoviePlayerGui;

		//
		std::vector<UPnPEntry>* decodeResult(std::string);
		void splitProtocol(std::string &protocol, std::string &prot, std::string &network, std::string &mime, std::string &additional);

		bool loadItem(std::string, int _selected);
		void loadDevices(bool hint = true);

		void showMenuDevice();
		void showMenuEntry();

	public:
		CUpnpBrowserGui(UPNP_GUI g = UPNP_GUI_DEVICE);
		~CUpnpBrowserGui();
		int exec(CMenuTarget* parent, const std::string& actionKey);
		void hide();
};

#endif
