/* 
  $Id: mediaportal.cpp 2015/13/22 mohousch Exp $

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

#include <plugin.h>


extern "C" void plugin_exec(void);
extern "C" void plugin_init(void);
extern "C" void plugin_del(void);


class CMediaPortal : public CTarget
{
	private:
		CFrameBuffer* frameBuffer;
		CWidget* widget;
		ClistBox* mediaPortal;
		CMenuItem* item;
		
		int selected;

		void showMenu(void);
	
	public:
		CMediaPortal();
		~CMediaPortal();
		
		int exec(CTarget* parent, const std::string& actionKey);
};

CMediaPortal::CMediaPortal()
{
	dprintf(DEBUG_NORMAL, "$Id: CMediaPortal, 2016.02.10 mohousch Exp $\n");
	
	frameBuffer = CFrameBuffer::getInstance();

	widget = NULL;
	mediaPortal = NULL;
	item = NULL;
	
	selected = 0;
}

CMediaPortal::~CMediaPortal()
{
	dprintf(DEBUG_NORMAL, "CMediaPortal: del\n");
}

int CMediaPortal::exec(CTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CMediaPortal::exec: actionKey:%s\n", actionKey.c_str());

	if(parent) 
		parent->hide();
	
	if (!actionKey.empty())
	{
		g_PluginList->startPlugin(actionKey.c_str());
		
		return RETURN_REPAINT;
	}

	showMenu();
	
	return RETURN_REPAINT;
}

void CMediaPortal::showMenu(void)
{
	const struct button_label HButtons = { NEUTRINO_ICON_BUTTON_MUTE_SMALL, "" };
	
	//
	widget = new CWidget(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());
	
	//
	mediaPortal = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

	mediaPortal->setWidgetMode(ClistBox::MODE_LISTBOX);
	mediaPortal->setWidgetLayout(ClistBox::LAYOUT_FRAME);
	
	mediaPortal->paintMainFrame(false);
//	mediaPortal->enableSaveScreen();

	// youtube
	if (g_PluginList->plugin_exists("youtube"))
	{
		item = new CMenuForwarder("You Tube", true, NULL, this, "youtube", CRCInput::RC_nokey, NULL, PLUGINDIR "/youtube/youtube.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("youtube")).c_str());
		mediaPortal->addItem(item);
	}

	// netzkino
	if (g_PluginList->plugin_exists("netzkino"))
	{
		item = new CMenuForwarder("NetzKino", true, NULL, this, "netzkino", CRCInput::RC_nokey, NULL, PLUGINDIR "/netzkino/netzkino.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("netzkino")).c_str());
		mediaPortal->addItem(item);
	}

	// icecast
	if (g_PluginList->plugin_exists("icecast"))
	{
		item = new CMenuForwarder("Ice Cast", true, NULL, this, "icecast", CRCInput::RC_nokey, NULL, PLUGINDIR "/icecast/icecast.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("icecast")).c_str());
		mediaPortal->addItem(item);
	}

	// internetradio
	if (g_PluginList->plugin_exists("internetradio"))
	{
		item = new CMenuForwarder("Internet Radio", true, NULL, this, "internetradio", CRCInput::RC_nokey, NULL,  PLUGINDIR "/internetradio/internetradio.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("internetradio")).c_str());
		mediaPortal->addItem(item);
	}

	// nFilm
	if (g_PluginList->plugin_exists("nfilm"))
	{
		item = new CMenuForwarder("Movie Trailer", true, NULL, this, "nfilm", CRCInput::RC_nokey, NULL, PLUGINDIR "/nfilm/nfilm.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("nfilm")).c_str());
		mediaPortal->addItem(item);
	}

	// nTVShows
	if (g_PluginList->plugin_exists("ntvshows"))
	{
		item = new CMenuForwarder("Serien Trailer", true, NULL, this, "ntvshows", CRCInput::RC_nokey, NULL, PLUGINDIR "/ntvshows/ntvshows.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("ntvshows")).c_str());
		mediaPortal->addItem(item);
	}
	
	// arte concert
	if (g_PluginList->plugin_exists("arte_concert"))
	{
		item = new CMenuForwarder("Arte Concert", true, NULL, this, "arte_concert", CRCInput::RC_nokey, NULL, PLUGINDIR "/arte_concert/arte_concert_hint.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("arte_concert")).c_str());
		mediaPortal->addItem(item);
	}
	
	// media_one
	if (g_PluginList->plugin_exists("media_one"))
	{
		item = new CMenuForwarder("Media One", true, NULL, this, "media_one", CRCInput::RC_nokey, NULL, PLUGINDIR "/media_one/media_one.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("media_one")).c_str());
		mediaPortal->addItem(item);
	}
	
	// mtv
	if (g_PluginList->plugin_exists("mtv"))
	{
		item = new CMenuForwarder("MTV", true, NULL, this, "mtv", CRCInput::RC_nokey, NULL, PLUGINDIR "/mtv/mtv_hint.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("mtv")).c_str());
		mediaPortal->addItem(item);
	}
	
	// netzkino_hd
	if (g_PluginList->plugin_exists("netzkino_hd"))
	{
		item = new CMenuForwarder("Netzkino HD", true, NULL, this, "netzkino_hd", CRCInput::RC_nokey, NULL, PLUGINDIR "/netzkino_hd/netzkino.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("netzkino_hd")).c_str());
		mediaPortal->addItem(item);
	}
	
	// plutotv
	if (g_PluginList->plugin_exists("plutotv"))
	{
		item = new CMenuForwarder("Pluto TV VOD", true, NULL, this, "plutotv", CRCInput::RC_nokey, NULL, PLUGINDIR "/plutotv/plutotv.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("plutotv")).c_str());
		mediaPortal->addItem(item);
	}
	
	widget->addCCItem(mediaPortal);

	widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

//plugin API
void plugin_init(void)
{
}

void plugin_del(void)
{
}

void plugin_exec(void)
{
	CMediaPortal * mpHandler = new CMediaPortal();
	
	mpHandler->exec(NULL, "");
	
	delete mpHandler;
	mpHandler = NULL;
}

