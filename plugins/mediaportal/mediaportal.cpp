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


class CMediaPortal : public CMenuTarget
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
		
		int exec(CMenuTarget* parent, const std::string& actionKey);
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

int CMediaPortal::exec(CMenuTarget * parent, const std::string & actionKey)
{
	dprintf(DEBUG_NORMAL, "CMediaPortal::exec: actionKey:%s\n", actionKey.c_str());

	int returnval = RETURN_REPAINT;

	if(parent) 
		parent->hide();

	if(actionKey == "youtube")
	{
		g_PluginList->startPlugin("youtube");

		return RETURN_REPAINT;
	}
	else if(actionKey == "netzkino")
	{
		g_PluginList->startPlugin("netzkino");

		return RETURN_REPAINT;
	}
	else if(actionKey == "icecast")
	{
		g_PluginList->startPlugin("icecast");

		return RETURN_REPAINT;
	}
	else if(actionKey == "internetradio")
	{
		g_PluginList->startPlugin("internetradio");

		return RETURN_REPAINT;
	}
	else if(actionKey == "ard")
	{
		g_PluginList->startPlugin("ard");

		return RETURN_REPAINT;
	}
	else if(actionKey == "nfilm")
	{
		g_PluginList->startPlugin("nfilm");

		return RETURN_REPAINT;
	}
	else if(actionKey == "ntvshows")
	{
		g_PluginList->startPlugin("ntvshows");

		return RETURN_REPAINT;
	}
	else if(actionKey == "arte_concert")
	{
		g_PluginList->startPlugin("arte_concert");

		return RETURN_REPAINT;
	}
	else if(actionKey == "media_one")
	{
		g_PluginList->startPlugin("media_one");

		return RETURN_REPAINT;
	}
	else if(actionKey == "mtv")
	{
		g_PluginList->startPlugin("mtv");

		return RETURN_REPAINT;
	}
	else if(actionKey == "netzkino_hd")
	{
		g_PluginList->startPlugin("netzkino_hd");

		return RETURN_REPAINT;
	}
	else if(actionKey == "plutotv")
	{
		g_PluginList->startPlugin("plutotv");

		return RETURN_REPAINT;
	}
	/*
	else if (actionKey == "delete")
	{
		selected = mediaPortal->getSelected();

		// remove selected plugin
		g_PluginList->removePlugin(selected);

		// relaod plugins
		g_PluginList->loadPlugins();

		if(selected > (int)g_PluginList->getNumberOfPlugins() - 1)
			selected = (int)g_PluginList->getNumberOfPlugins() - 1;

		showMenu();
		return RETURN_EXIT_ALL;
	}
	*/

	showMenu();
	
	return returnval;
}

void CMediaPortal::showMenu(void)
{
	const struct button_label HButtons = { NEUTRINO_ICON_BUTTON_MUTE_SMALL, "" };
	
	widget = new CWidget();
	mediaPortal = new ClistBox(frameBuffer->getScreenX(), frameBuffer->getScreenY(), frameBuffer->getScreenWidth(), frameBuffer->getScreenHeight());

	mediaPortal->setWidgetMode(MODE_LISTBOX);
	mediaPortal->setWidgetType(WIDGET_TYPE_FRAME);
	
	//
	mediaPortal->enablePaintHead();
	mediaPortal->setTitle(_("Media Portal"), PLUGINDIR "/mediaportal/mp.png");
	mediaPortal->enablePaintDate();
	//mediaPortal->setHeadButtons(&HButtons);
	
	// 
	mediaPortal->enablePaintItemInfo();

	// youtube
	if (g_PluginList->plugin_exists("youtube"))
	{
		item = new ClistBoxItem("You Tube", true, NULL, this, "youtube", RC_nokey, NULL, PLUGINDIR "/youtube/youtube.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("youtube")).c_str());
		mediaPortal->addItem(item);
	}

	// netzkino
	if (g_PluginList->plugin_exists("netzkino"))
	{
		item = new ClistBoxItem("NetzKino", true, NULL, this, "netzkino", RC_nokey, NULL, PLUGINDIR "/netzkino/netzkino.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("netzkino")).c_str());
		mediaPortal->addItem(item);
	}

	// icecast
	if (g_PluginList->plugin_exists("icecast"))
	{
		item = new ClistBoxItem("Ice Cast", true, NULL, this, "icecast", RC_nokey, NULL, PLUGINDIR "/icecast/icecast.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("icecast")).c_str());
		mediaPortal->addItem(item);
	}

	// internetradio
	if (g_PluginList->plugin_exists("internetradio"))
	{
		item = new ClistBoxItem("Internet Radio", true, NULL, this, "internetradio", RC_nokey, NULL,  PLUGINDIR "/internetradio/internetradio.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("internetradio")).c_str());
		mediaPortal->addItem(item);
	}

	// nFilm
	if (g_PluginList->plugin_exists("nfilm"))
	{
		item = new ClistBoxItem("Movie Trailer", true, NULL, this, "nfilm", RC_nokey, NULL, PLUGINDIR "/nfilm/nfilm.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("nfilm")).c_str());
		mediaPortal->addItem(item);
	}

	// nTVShows
	if (g_PluginList->plugin_exists("ntvshows"))
	{
		item = new ClistBoxItem("Serien Trailer", true, NULL, this, "ntvshows", RC_nokey, NULL, PLUGINDIR "/ntvshows/ntvshows.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("ntvshows")).c_str());
		mediaPortal->addItem(item);
	}
	
	// arte concert
	if (g_PluginList->plugin_exists("arte_concert"))
	{
		item = new ClistBoxItem("Arte Concert", true, NULL, this, "arte_concert", RC_nokey, NULL, PLUGINDIR "/arte_concert/arte_concert_hint.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("arte_concert")).c_str());
		mediaPortal->addItem(item);
	}
	
	// media_one
	if (g_PluginList->plugin_exists("media_one"))
	{
		item = new ClistBoxItem("Media One", true, NULL, this, "media_one", RC_nokey, NULL, PLUGINDIR "/media_one/media_one.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("media_one")).c_str());
		mediaPortal->addItem(item);
	}
	
	// mtv
	if (g_PluginList->plugin_exists("mtv"))
	{
		item = new ClistBoxItem("MTV", true, NULL, this, "mtv", RC_nokey, NULL, PLUGINDIR "/mtv/mtv_hint.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("mtv")).c_str());
		mediaPortal->addItem(item);
	}
	
	// netzkino_hd
	if (g_PluginList->plugin_exists("netzkino_hd"))
	{
		item = new ClistBoxItem("Netzkino HD", true, NULL, this, "netzkino_hd", RC_nokey, NULL, PLUGINDIR "/netzkino_hd/netzkino.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("netzkino_hd")).c_str());
		mediaPortal->addItem(item);
	}
	
	// plutotv
	if (g_PluginList->plugin_exists("plutotv"))
	{
		item = new ClistBoxItem("Pluto TV VOD", true, NULL, this, "plutotv", RC_nokey, NULL, PLUGINDIR "/plutotv/plutotv.png");
		item->setHint(g_PluginList->getDescription(g_PluginList->find_plugin("plutotv")).c_str());
		mediaPortal->addItem(item);
	}
	
	widget->addItem(mediaPortal);
	//widget->addKey(RC_spkr, this, "delete");

	widget->exec(NULL, "");
	
	delete mediaPortal;
	mediaPortal = NULL;
	
	delete widget;
	widget = NULL;
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


