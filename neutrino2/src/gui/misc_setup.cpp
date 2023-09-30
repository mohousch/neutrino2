/*
	Neutrino-GUI  -   DBoxII-Project

	$id: misc_setup.cpp 23.09.2023 mohousch $
	
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

#include <global.h>
#include <neutrino2.h>

#include <stdio.h> 
#include <sys/stat.h>
#include <dirent.h>
#include <sys/vfs.h>

#include <xmlinterface.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/stringinput.h>

#include <gui/filebrowser.h>
#include <gui/misc_setup.h>

#include <gui/zapit_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>

// configfiles
#include <timerd/timermanager.h>
#include <nhttpd/yconfig.h>

#include <audio_cs.h>
#include <video_cs.h>


//// globals
CMenuOptionStringChooser * tzSelect;
//
extern cVideo *videoDecoder;
extern cAudio *audioDecoder;
extern CFrontend * live_fe;
//
extern Zapit_config zapitCfg;			//defined in neutrino.cpp
extern t_channel_id live_channel_id;

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("no") },
	{ 1, _("yes") }
};

// option off1 on0
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, _("off") },
        { 0, _("on") }
};

#define MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT 2
const keyval MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS[MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT] =
{
	{ 0, _("ISO-8859-1") },
	{ 1, _("UTF-8") }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 2
const keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, _("off") },
	{ 1, _("on") }
};

#define DEBUG_LEVEL_OPTIONS_COUNT	3
const keyval DEBUG_LEVEL_OPTIONS[DEBUG_LEVEL_OPTIONS_COUNT] =
{
	{ DEBUG_NORMAL, _("Normal") },
	{ DEBUG_INFO, _("Info") },
	{ DEBUG_DEBUG, _("Debug") }
};

//// general settings
int CGeneralSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CGeneralSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

//
void CGeneralSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CGeneralSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsGeneral = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("miscsetup");
	
	if (widget)
	{
		miscSettingsGeneral = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "miscsetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		miscSettingsGeneral = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		miscSettingsGeneral->setWidgetMode(ClistBox::MODE_SETUP);
		miscSettingsGeneral->enableShrinkMenu();
		
		//
		miscSettingsGeneral->enablePaintHead();
		miscSettingsGeneral->setTitle(_("Misc settings"), NEUTRINO_ICON_MISC);
		miscSettingsGeneral->setHeadLine(true, true);

		//
		miscSettingsGeneral->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsGeneral->setFootButtons(&btn);
		miscSettingsGeneral->setFootLine(true, true);
		
		//
		widget->addWidgetItem(miscSettingsGeneral);
	}
	
	// intros
	miscSettingsGeneral->addItem(new CMenuForwarder(_("back")));
	miscSettingsGeneral->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsGeneral->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// rc delay
	CMenuOptionChooser * m1 = new CMenuOptionChooser(_("Delayed shutdown"), &g_settings.shutdown_real_rcdelay, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);

	CMiscNotifier * miscNotifier = new CMiscNotifier( m1 );

	// shutdown real
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("Enable standby"), &g_settings.shutdown_real, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, miscNotifier ));

	// delayed shutdown
	miscSettingsGeneral->addItem(m1);

	// delay counter
	CStringInput * miscSettings_shutdown_count = new CStringInput(_("switch off after"), g_settings.shutdown_count, 3, _("time (in minutes) to switch from standby"), _("to deep standby (0 = off)."), "0123456789 ");
	miscSettingsGeneral->addItem(new CMenuForwarder(_("switch off after"), true, g_settings.shutdown_count, miscSettings_shutdown_count));

	// start to standby
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("Startup to standby"), &g_settings.power_standby, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// timezone
	xmlDocPtr parser;

	parser = parseXmlFile("/etc/timezone.xml");
	if (parser != NULL) 
	{	
		tzSelect = new CMenuOptionStringChooser(_("Time Zone"), g_settings.timezone, true, new CTZChangeNotifier(), CRCInput::RC_nokey, "", true);

		xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
		bool found = false;

		while (search) 
		{
			if (!strcmp(xmlGetName(search), "zone")) 
			{
				std::string name = xmlGetAttribute(search, (char *) "name");
				std::string zone = xmlGetAttribute(search, (char *) "zone");
				
				//printf("Timezone: %s -> %s\n", name.c_str(), zone.c_str());
				
				tzSelect->addOption(name.c_str());
				found = true;
			}
			search = search->xmlNextNode;
		}

		if(found)
			miscSettingsGeneral->addItem(tzSelect);
		else 
		{
			delete tzSelect;
			tzSelect = NULL;
		}	
		xmlFreeDoc(parser);
	}
	
	// radio text	
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("Radio Text"), &g_settings.radiotext_enable, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this ));
	
	// debug level
	miscSettingsGeneral->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("Debug Level"), &g_settings.debug_level, DEBUG_LEVEL_OPTIONS, DEBUG_LEVEL_OPTIONS_COUNT, true, this ));

	// online key
	miscSettingsGeneral->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	std::string ytkey = g_settings.ytkey;
	
	CStringInputSMS* keyInput = new CStringInputSMS(_("youtube ID Key:"), ytkey.c_str());
	miscSettingsGeneral->addItem(new CMenuForwarder(_("YT:"), true, ytkey.c_str(), keyInput));

	// tmdb
	std::string tmdbkey = g_settings.tmdbkey;

	CStringInputSMS* tmdbkeyInput = new CStringInputSMS(_("TMDB Key:"), tmdbkey.c_str());
	miscSettingsGeneral->addItem(new CMenuForwarder(_("TMDB:"), true, tmdbkey.c_str(), tmdbkeyInput));
	
	// prefer tmdb preview
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("TMDB Preview"), &g_settings.enable_tmdb_preview, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// prefer tmdb infos
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("TMDB Infos"), &g_settings.enable_tmdb_infos, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// reset factory setup
	miscSettingsGeneral->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	CDataResetNotifier * resetNotifier = new CDataResetNotifier();
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Reset settings to defaults"), true, NULL, resetNotifier, "factory"));
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Settings backup"),  true, NULL, resetNotifier, "backup"));
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Settings restore"), true, NULL, resetNotifier, "restore"));
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete miscSettingsGeneral;
	miscSettingsGeneral = NULL;
	delete widget;
	widget = NULL;
}

//
bool CGeneralSettings::changeNotify(const std::string& OptionName, void */*data*/)
{
	dprintf(DEBUG_NORMAL, "CGeneralSettings::changeNotify:\n");
	
	if(OptionName == _("Radio Text")) 
	{
		bool usedBackground = CFrameBuffer::getInstance()->getuseBackground();
		
		if (g_settings.radiotext_enable) 
		{
			//
			if (g_Radiotext == NULL)
				g_Radiotext = new CRadioText;

			if (g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
			{
				if(live_fe)
					g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
			}
		} 
		else 
		{
			delete g_Radiotext;
			g_Radiotext = NULL;
		}
		
		return true;
	}
	else if (OptionName == _("Debug Level"))
	{
		CNeutrinoApp::getInstance()->setDebugLevel(g_settings.debug_level);
	}

	return false;
}


// TZ notifier
bool CTZChangeNotifier::changeNotify(const std::string&, void * Data)
{
	bool found = false;
	std::string name, zone;
	
	dprintf(DEBUG_NORMAL, "CTZChangeNotifier::changeNotify: %s\n", (char *) Data);

        xmlDocPtr parser = parseXmlFile("/etc/timezone.xml");
	
        if (parser != NULL) 
	{
                xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
                while (search) 
		{
                        if (!strcmp(xmlGetName(search), "zone")) 
			{
                                name = xmlGetAttribute(search, (char *) "name");
                                zone = xmlGetAttribute(search, (char *) "zone");

				if(!strcmp(g_settings.timezone, name.c_str())) 
				{
					found = true;
					break;
				}
                        }
                        search = search->xmlNextNode;
                }
                xmlFreeDoc(parser);
        }

	if(found) 
	{
		dprintf(DEBUG_NORMAL, "CTZChangeNotifier::changeNotify: Timezone: %s -> %s\n", name.c_str(), zone.c_str());
		
		std::string cmd = "ln -sf /usr/share/zoneinfo/" + zone + " /etc/localtime";
		
		dprintf(DEBUG_NORMAL, "exec %s\n", cmd.c_str());
		
		system(cmd.c_str());
		
		tzset();
	}

	return true;
}

//
int CDataResetNotifier::exec(CMenuTarget *parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CDataResetNotifier::exec: %s\n", actionKey.c_str());
	
	if (parent)
		parent->hide();
		
	//
	CFileBrowser fileBrowser;
	CFileFilter fileFilter;

	if( actionKey == "factory") 
	{
		int result = MessageBox(_("Reset settings to defaults"), _("Are you sure ?"), mbrNo, mbYes | mbNo);
		if(result != mbrYes) 
			return true;
		
		// neutrino settings
		unlink(NEUTRINO_SETTINGS_FILE);
		
		// timerd settings
		unlink(TIMERD_CONFIGFILE);
		
		// nhttpd settings
		unlink(HTTPD_CONFIGFILE );
		unlink(YWEB_CONFIGFILE);
		
		// load default settings
		CNeutrinoApp::getInstance()->loadSetup(NEUTRINO_SETTINGS_FILE);
		
		// create default settings to stop wizard
		CNeutrinoApp::getInstance()->saveSetup(NEUTRINO_SETTINGS_FILE);
		
		// default skin
		CNeutrinoApp::getInstance()->loadSkin(g_settings.preferred_skin);
		
		CFrameBuffer::getInstance()->paintBackground();

		CFrameBuffer::getInstance()->blit();
		
		// video mode
		if(videoDecoder)
		{
			videoDecoder->SetVideoSystem(g_settings.video_Mode);

			//aspect-ratio
			videoDecoder->setAspectRatio(g_settings.video_Ratio, g_settings.video_Format);
#if defined (PLATFORM_COOLSTREAM)
			videoDecoder->SetVideoMode((analog_mode_t) g_settings.analog_mode);
#else			
			videoDecoder->SetAnalogMode( g_settings.analog_mode); 
#endif

#if !defined (PLATFORM_COOLSTREAM)	
			videoDecoder->SetSpaceColour(g_settings.hdmi_color_space);
#endif
		}

		// audio mode
		CZapit::getInstance()->setAudioMode(g_settings.audio_AnalogMode);

		if(audioDecoder)
			audioDecoder->SetHdmiDD(g_settings.hdmi_dd );

		//
		g_settings.timing_menu = DEFAULT_TIMING_MENU;
		g_settings.timing_channellist = DEFAULT_TIMING_CHANNELLIST;
		g_settings.timing_epg = DEFAULT_TIMING_EPG;
		g_settings.timing_infobar = DEFAULT_TIMING_INFOBAR;
		g_settings.timing_filebrowser = DEFAULT_TIMING_FILEBROWSER;
		g_settings.timing_numericzap = DEFAULT_TIMING_NUMERICZAP;
		
		CNeutrinoApp::getInstance()->exec(NULL, "restart");

		return true;
	}
	else if(actionKey == "backup") 
	{
		fileBrowser.Dir_Mode = true;
		
		if (fileBrowser.exec("/media") == true) 
		{
			char  fname[256];
			struct statfs s;
			int ret = ::statfs(fileBrowser.getSelectedFile()->Name.c_str(), &s);

			if(ret == 0 && s.f_type != 0x72b6L/*jffs2*/ && s.f_type != 0x5941ff53L /*yaffs2*/)
			{ 
				const char backup_sh[] = "backup.sh";

				sprintf(fname, "%s %s", backup_sh, fileBrowser.getSelectedFile()->Name.c_str());
				
				dprintf(DEBUG_NORMAL, "CDataResetNotifier::exec: executing %s\n", fname);
				
				system(fname);
			} 
			else
				MessageBox(_("Error"), _("Backup failed"), mbrBack, mbBack, NEUTRINO_ICON_ERROR);
		}

		return true;
	}
	else if(actionKey == "restore") 
	{
		fileFilter.addFilter("tar");
		fileBrowser.Filter = &fileFilter;
		
		if (fileBrowser.exec("/media") == true) 
		{
			int result = MessageBox(_("Settings restore"), _("Do you want to restore the previous settings?"), mbrNo, mbYes | mbNo);
			if(result == mbrYes) 
			{
				char  fname[256];
				
				const char restore_sh[] = "restore.sh";
				
				sprintf(fname, "%s %s", restore_sh, fileBrowser.getSelectedFile()->Name.c_str());
				
				dprintf(DEBUG_NORMAL, "CDataResetNotifier::exec: executing %s\n", fname);
				
				system(fname);
			}
			
			
		}

		return true;
	}

	return false;
}


//// channellist settings
int CChannelListSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CChannelListSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

//
void CChannelListSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CChannelListSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsChannelList = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("channelssetup");
	
	if (widget)
	{
		miscSettingsChannelList = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "channelssetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		miscSettingsChannelList = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		miscSettingsChannelList->setWidgetMode(ClistBox::MODE_SETUP);
		miscSettingsChannelList->enableShrinkMenu();
		
		//
		miscSettingsChannelList->enablePaintHead();
		miscSettingsChannelList->setTitle(_("Channellist settings"), NEUTRINO_ICON_CHANNELLIST);
		miscSettingsChannelList->setHeadLine(true, true);

		//
		miscSettingsChannelList->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsChannelList->setFootButtons(&btn);
		miscSettingsChannelList->setFootLine(true, true);
		
		//
		widget->addWidgetItem(miscSettingsChannelList);
	}
	
	// intros
	miscSettingsChannelList->addItem(new CMenuForwarder(_("back")));
	miscSettingsChannelList->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	miscSettingsChannelList->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsChannelList->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// HD list
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Create list of HD channels"), &g_settings.make_hd_list, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
	
	// virtual zap
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Virtual zap"), &g_settings.virtual_zap_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// zap cycle
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Zap cycle"), &g_settings.zap_cycle, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	//
	CZapit::getInstance()->getZapitConfig(&zapitCfg);
	
	// other
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Make Remaining Channels list"), (int *)&zapitCfg.makeRemainingChannelsBouquet, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));

	// scanSDT
	miscSettingsChannelList->addItem( new CMenuOptionChooser(_("Scan SDT for updates"), (int *)&zapitCfg.scanSDT, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true, this) );
	
	//
	miscSettingsChannelList->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// timescale
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Timescale"), &g_settings.channellist_timescale, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// channellist ca
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("HD / Crypt Icons"), &g_settings.channellist_ca, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete miscSettingsChannelList;
	miscSettingsChannelList = NULL;
	delete widget;
	widget = NULL;
}

//
bool CChannelListSettings::changeNotify(const std::string& OptionName, void */*data*/)
{
	dprintf(DEBUG_NORMAL, "CChannelListSettings::changeNotify:\n");
	
	if(OptionName == _("Create list of HD channels")) // HD bouquet
	{
		CNeutrinoApp::getInstance()->channelsInit();
		CNeutrinoApp::getInstance()->channelList->adjustToChannelID(live_channel_id);
		
		return true;
	}
	else if(OptionName == _("Make Remaining Channels list")) // rmains channel
	{
		CZapit::getInstance()->setZapitConfig(&zapitCfg);
		
		CZapit::getInstance()->reinitChannels();
		
		return true;
	}
	else if(OptionName == _("Scan SDT for updates"))
	{
		CZapit::getInstance()->setZapitConfig(&zapitCfg);
	}
	
	return false;
}


//// epg settings
int CEPGSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CEPGSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "epgdir") 
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if ( b.exec(g_settings.epg_dir.c_str())) 
		{
			const char * newdir = b.getSelectedFile()->Name.c_str();
			if(check_dir(newdir))
				printf("CNeutrinoApp::exec: Wrong/unsupported epg dir %s\n", newdir);
			else
			{
				g_settings.epg_dir = b.getSelectedFile()->Name;
				CNeutrinoApp::getInstance()->sendSectionsdConfig();
			}
		}

		hide();
		showMenu();
		
		return CMenuTarget::RETURN_EXIT_ALL;
	}
	
	showMenu();
	
	return ret;
}

#define EPG_SERVERBOX_TYPE_OPTION_COUNT 3
const keyval EPG_SERVERBOX_TYPE_OPTIONS[EPG_SERVERBOX_TYPE_OPTION_COUNT] =
{
	{ DVB_C, _("cable") },
	{ DVB_S, _("sat") },
	{ DVB_T, _("terrestrial") }
};

#define EPG_SERVERBOX_GUI_OPTION_COUNT 3
const keyval EPG_SERVERBOX_GUI_OPTIONS[EPG_SERVERBOX_GUI_OPTION_COUNT] =
{
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_NHD2, "neutrino2" },
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_NMP, "neutrinoMP" },
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2, "enigma2" }
};

void CEPGSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CEPGSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsEPG = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("epgsetup");
	
	if (widget)
	{
		miscSettingsEPG = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "epgsetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		miscSettingsEPG = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		miscSettingsEPG->setWidgetMode(ClistBox::MODE_SETUP);
		miscSettingsEPG->enableShrinkMenu();
		
		//
		miscSettingsEPG->enablePaintHead();
		miscSettingsEPG->setTitle(_("EPG settings"), NEUTRINO_ICON_BUTTON_EPG);
		miscSettingsEPG->setHeadLine(true, true);

		//
		miscSettingsEPG->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsEPG->setFootButtons(&btn);
		miscSettingsEPG->setFootLine(true, true);
		
		//
		widget->addWidgetItem(miscSettingsEPG);
	}
	
	// intros
	miscSettingsEPG->addItem(new CMenuForwarder(_("back")));
	miscSettingsEPG->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	miscSettingsEPG->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsEPG->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	//
	CEPGConfigNotifier* epgConfigNotifier = new CEPGConfigNotifier;
	CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	
	// save / restore epg on boot
	miscSettingsEPG->addItem(new CMenuOptionChooser(_("Save/Restore epg on reboot"), &g_settings.epg_save, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, epgConfigNotifier));

	// epg cache
        CStringInput * miscSettings_epg_cache = new CStringInput(_("EPG-Cache (Days)"), g_settings.epg_cache.c_str(), 2, _("How long will EPG-Data in the future cached?"), _("Set in days."), "0123456789 ", sectionsdConfigNotifier);
        miscSettingsEPG->addItem(new CMenuForwarder(_("EPG-Cache (Days)"), true, g_settings.epg_cache.c_str(), miscSettings_epg_cache));

	// extended epg cache
        CStringInput * miscSettings_epg_cache_e = new CStringInput(_("EPG Long Description (hours)"), g_settings.epg_extendedcache.c_str(), 3, _("How long into the future will EPG extended descriptions"), _("be Cached? (Set in hours)"), "0123456789 ", sectionsdConfigNotifier);
        miscSettingsEPG->addItem(new CMenuForwarder(_("EPG Long Description (hours)"), true, g_settings.epg_extendedcache.c_str(), miscSettings_epg_cache_e));

	// old events
        CStringInput * miscSettings_epg_old_events = new CStringInput(_("EPG remove after (std.)"), g_settings.epg_old_events.c_str(), 2, _("How long will EPG-Data be stored after they timed out?"), _("Set in hours"), "0123456789 ", sectionsdConfigNotifier);
        miscSettingsEPG->addItem(new CMenuForwarder(_("EPG remove after (std.)"), true, g_settings.epg_old_events.c_str(), miscSettings_epg_old_events));

	// max epg events
        CStringInput * miscSettings_epg_max_events = new CStringInput(_("Max. Events"), g_settings.epg_max_events.c_str(), 5, _("How many events should be stored?"), _("normaly 50000, 0 to disable limit"), "0123456789 ", sectionsdConfigNotifier);
        miscSettingsEPG->addItem(new CMenuForwarder(_("Max. Events"), true, g_settings.epg_max_events.c_str(), miscSettings_epg_max_events));

	// epg save dir
        miscSettingsEPG->addItem(new CMenuForwarder(_("EPG save path"), true, g_settings.epg_dir.c_str(), this, "epgdir"));
	
	// epglang
	miscSettingsEPG->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, _("Preferred EPG language")));
	
	CMenuOptionStringChooser * epglangSelect[3];
	CEPGlangSelectNotifier * EPGlangNotifier = new CEPGlangSelectNotifier();
	
	for(int i = 0; i < 3; i++) 
	{
		epglangSelect[i] = new CMenuOptionStringChooser(_("Preferred EPG language"), g_settings.pref_epgs[i], true, EPGlangNotifier, CRCInput::RC_nokey, "", true);
		std::map<std::string, std::string>::const_iterator it;
		
		epglangSelect[i]->addOption("");
		for(it = iso639rev.begin(); it != iso639rev.end(); it++) 
			epglangSelect[i]->addOption(it->first.c_str());
	}
	
	// epglang
	for(int i = 0; i < 3; i++) 
		miscSettingsEPG->addItem(epglangSelect[i]);
		
	// xmltv 
	miscSettingsEPG->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	CXMLTVConfigNotifier* xmltvConfigNotifier = new CXMLTVConfigNotifier;
	miscSettingsEPG->addItem(new CMenuOptionChooser(_("Use XMLTV EPG"), &g_settings.epg_xmltv, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, xmltvConfigNotifier));


	// localtv epg
	miscSettingsEPG->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// server box ip
	CIPInput * epg_IP = new CIPInput(_("Server Box IP"), g_settings.epg_serverbox_ip);
	CMenuForwarder* o1 = new CMenuForwarder(_("Server Box IP"), g_settings.epg_enable_localtv_epg, g_settings.epg_serverbox_ip.c_str(), epg_IP);

	// server gui (neutrino/neutrinohd/enigma2)
	CMenuOptionChooser* o2 = new CMenuOptionChooser(_("Server Box GUI"), &g_settings.epg_serverbox_gui, EPG_SERVERBOX_GUI_OPTIONS, EPG_SERVERBOX_GUI_OPTION_COUNT, g_settings.epg_enable_localtv_epg);

	// server box type (sat/cable/terrestrial)
	CMenuOptionChooser* o3 = new CMenuOptionChooser(_("Server Box type"), &g_settings.epg_serverbox_type, EPG_SERVERBOX_TYPE_OPTIONS, EPG_SERVERBOX_TYPE_OPTION_COUNT, g_settings.epg_enable_localtv_epg);

	// online EPG on/off
	COnlineEPGNotifier* onlineEPGNotifier = new COnlineEPGNotifier(o1, o2, o3);

	miscSettingsEPG->addItem(new CMenuOptionChooser(_("LocalTV EPG"), &g_settings.epg_enable_localtv_epg, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, onlineEPGNotifier));

	miscSettingsEPG->addItem(o1);
	miscSettingsEPG->addItem(o2);
	miscSettingsEPG->addItem(o3);
	
	// xmltv
	
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete miscSettingsEPG;
	miscSettingsEPG = NULL;
	delete widget;
	widget = NULL;

	delete epg_IP;
	epg_IP = NULL;
	delete onlineEPGNotifier;
	onlineEPGNotifier = NULL;
}

// onlineepg notifier
COnlineEPGNotifier::COnlineEPGNotifier(CMenuForwarder* m1, CMenuOptionChooser* m2, CMenuOptionChooser* m3)
{
	item1 = m1;
	item2 = m2;
	item3 = m3;
}

bool COnlineEPGNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_NORMAL, "COnlineEPGNotifier::changeNotify\n");

	{
		item1->setActive(g_settings.epg_enable_localtv_epg);
		item2->setActive(g_settings.epg_enable_localtv_epg);
		item3->setActive(g_settings.epg_enable_localtv_epg);
	}
	
	if (g_settings.epg_enable_localtv_epg)
		CSectionsd::getInstance()->readSIfromLocalTV(live_channel_id);
	
        return true;
}

// epg language select notifier
bool CEPGlangSelectNotifier::changeNotify(const std::string&, void *)
{
	std::vector<std::string> v_languages;
	bool found = false;
	std::map<std::string, std::string>::const_iterator it;

	//prefered audio languages
	for(int i = 0; i < 3; i++) 
	{
		if(strlen(g_settings.pref_epgs[i])) 
		{
			dprintf(DEBUG_NORMAL, "EPG: setLanguages: %d: %s\n", i, g_settings.pref_epgs[i]);
			
			std::string temp(g_settings.pref_epgs[i]);
			
			for(it = iso639.begin(); it != iso639.end(); it++) 
			{
				if(temp == it->second) 
				{
					v_languages.push_back(it->first);
					
					dprintf(DEBUG_NORMAL, "EPG: setLanguages: adding %s\n", it->first.c_str());
					
					found = true;
				}
			}
		}
	}
	
	if(found)
		CSectionsd::getInstance()->setLanguages(v_languages);
	
	return true;
}

// epg config notifier
bool CEPGConfigNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_NORMAL, "CEPGConfigNotifier::changeNotify\n");

        if (g_settings.epg_save)
		CNeutrinoApp::getInstance()->readEPG();
	
        return true;
}

// xmltv config notifier
bool CXMLTVConfigNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_NORMAL, "CXMLTVConfigNotifier::changeNotify\n");

        if (g_settings.epg_xmltv)
        {
        	for (unsigned long i = 0; i < g_settings.xmltv.size(); i++)
        	{
			CSectionsd::getInstance()->readSIfromXMLTV(g_settings.xmltv[i].c_str());
		}
	}
	
        return true;
}

//// filebrowser settings
int CFileBrowserSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CFileBrowserSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CFileBrowserSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CFileBrowserSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsFileBrowser = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("filebrowsersetup");
	
	if (widget)
	{
		miscSettingsFileBrowser = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "filebrowsersetup";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		miscSettingsFileBrowser = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);

		miscSettingsFileBrowser->setWidgetMode(ClistBox::MODE_SETUP);
		miscSettingsFileBrowser->enableShrinkMenu();
		
		//
		miscSettingsFileBrowser->enablePaintHead();
		miscSettingsFileBrowser->setTitle(_("Filebrowser settings"), NEUTRINO_ICON_FILEBROWSER);
		miscSettingsFileBrowser->setHeadLine(true, true);

		//
		miscSettingsFileBrowser->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsFileBrowser->setFootButtons(&btn);
		miscSettingsFileBrowser->setFootLine(true, true);
		
		//
		widget->addWidgetItem(miscSettingsFileBrowser);
	}
	
	// intros
	miscSettingsFileBrowser->addItem(new CMenuForwarder(_("back")));
	miscSettingsFileBrowser->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	
	// save settings
	miscSettingsFileBrowser->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsFileBrowser->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	// UTF 
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("File system"), &g_settings.filesystem_is_utf8, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT, true));

	// show rights
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("Show file rights"), &g_settings.filebrowser_showrights, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true));

	// deny dir
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("Absolute start directory"), &g_settings.filebrowser_denydirectoryleave, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true));
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete miscSettingsFileBrowser;
	miscSettingsFileBrowser = NULL;
	delete widget;
	widget = NULL;
}

// misc notifier
CMiscNotifier::CMiscNotifier( CMenuItem* i1)
{
   	toDisable[0] = i1;
}

bool CMiscNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_NORMAL, "CMiscNotifier::changeNotify\n");

   	toDisable[0]->setActive(!g_settings.shutdown_real);

   	return true;
}

