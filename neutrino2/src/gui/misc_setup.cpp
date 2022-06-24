/*
	Neutrino-GUI  -   DBoxII-Project

	$id: misc_setup.cpp 2016.01.02 21:55:30 mohousch $
	
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
#include <neutrino.h>

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


extern cVideo *videoDecoder;
extern cAudio *audioDecoder;
extern CFrontend * live_fe;

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("No") },
	{ 1, _("Yes") }
};

// misc settings
extern Zapit_config zapitCfg;			//defined in neutrino.cpp
void setZapitConfig(Zapit_config * Cfg);
void getZapitConfig(Zapit_config *Cfg);

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

CMenuOptionStringChooser * tzSelect;

int CMiscSettingsMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMiscSettingsMenu::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

// showmenu
void CMiscSettingsMenu::showMenu(void)
{
	dprintf(DEBUG_NORMAL, "CMiscSettingsMenu::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettings = NULL;
	
	//
	miscSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
	miscSettings->setMenuPosition(MENU_POSITION_CENTER);
	miscSettings->setWidgetMode(MODE_MENU);
	miscSettings->setWidgetType(WIDGET_TYPE_CLASSIC);
	miscSettings->enableShrinkMenu();
		
	//
	miscSettings->enablePaintHead();
	miscSettings->setTitle(_("Misc settings"), NEUTRINO_ICON_SETTINGS);
	miscSettings->enablePaintDate();
		
	//
	miscSettings->enablePaintFoot();
			
	const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
	miscSettings->setFootButtons(&btn);

	//miscSettings general
	miscSettings->addItem(new CMenuForwarder(_("General settings"), true, NULL, new CGeneralSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_GENERALSETTINGS));
		
	//channellist settings
	miscSettings->addItem(new CMenuForwarder(_("Channellist settings"), true, NULL, new CChannelListSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_CHANNELLISTSETTINGS));

	// epg settings
	miscSettings->addItem(new CMenuForwarder(_("EPG settings"), true, NULL, new CEPGSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_EPGSETTINGS));

	// filebrowser settings
	miscSettings->addItem(new CMenuForwarder(_("Filebrowser settings"), true, NULL, new CFileBrowserSettings(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_FILEBROWSERSETTINGS));
		
	// zapit setup (start channel)
	miscSettings->addItem(new CMenuForwarder(_("Last Channel settings"), true, NULL, new CZapitSetup(), NULL, RC_nokey, NULL, NEUTRINO_ICON_MENUITEM_STARTCHANNELSETTINGS));
		
	//
	if (widget == NULL) widget = new CWidget(miscSettings->getWindowsPos().iX, miscSettings->getWindowsPos().iY, miscSettings->getWindowsPos().iWidth, miscSettings->getWindowsPos().iHeight);
	widget->name = "miscsettings";
	widget->setMenuPosition(MENU_POSITION_CENTER);
		
	widget->addItem(miscSettings);
	
	//
	widget->exec(NULL, "");
}

//// general settings
extern CRemoteControl * g_RemoteControl;	// defined neutrino.cpp

CGeneralSettings::CGeneralSettings()
{
}

CGeneralSettings::~CGeneralSettings()
{
}

int CGeneralSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CGeneralSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
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
	
	if (CNeutrinoApp::getInstance()->getWidget("miscsetup"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("miscsetup");
		miscSettingsGeneral = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		miscSettingsGeneral = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		miscSettingsGeneral->setMenuPosition(MENU_POSITION_CENTER);
		miscSettingsGeneral->setWidgetMode(MODE_SETUP);
		miscSettingsGeneral->enableShrinkMenu();
		
		miscSettingsGeneral->enablePaintHead();
		miscSettingsGeneral->setTitle(_("Misc settings"), NEUTRINO_ICON_SETTINGS);

		miscSettingsGeneral->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsGeneral->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addItem(miscSettingsGeneral);
	}
	
	miscSettingsGeneral->clearItems();
	
	// intros
	miscSettingsGeneral->addItem(new CMenuForwarder(_("back")));
	miscSettingsGeneral->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsGeneral->addItem( new CMenuSeparator(LINE) );

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
	_xmlDocPtr parser;

	parser = parseXmlFile("/etc/timezone.xml");
	if (parser != NULL) 
	{	
		tzSelect = new CMenuOptionStringChooser(_("Time Zone"), g_settings.timezone, true, new CTZChangeNotifier(), RC_nokey, "", true);

		_xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
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

	// online key
	miscSettingsGeneral->addItem(new CMenuSeparator(LINE));

	std::string ytkey = g_settings.ytkey;
	
	CStringInputSMS* keyInput = new CStringInputSMS(_("youtube ID Key:"), ytkey.c_str());
	miscSettingsGeneral->addItem(new CMenuForwarder(_("YT:"), true, ytkey.c_str(), keyInput));

	std::string tmdbkey = g_settings.tmdbkey;

	CStringInputSMS* tmdbkeyInput = new CStringInputSMS(_("TMDB Key:"), tmdbkey.c_str());
	miscSettingsGeneral->addItem(new CMenuForwarder(_("TMDB:"), true, tmdbkey.c_str(), tmdbkeyInput));

	// prefer tmdb infos
	miscSettingsGeneral->addItem(new CMenuOptionChooser(_("TMDB"), &g_settings.enable_tmdb_infos, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	// reset factory setup
	miscSettingsGeneral->addItem(new CMenuSeparator(LINE));
	
	CDataResetNotifier * resetNotifier = new CDataResetNotifier();
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Reset settings to defaults"), true, NULL, resetNotifier, "factory"));
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Settings backup"),  true, NULL, resetNotifier, "backup"));
	miscSettingsGeneral->addItem(new CMenuForwarder(_("Settings restore"), true, NULL, resetNotifier, "restore"));
	
	//
	widget->exec(NULL, "");
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
				// hide radiomode background pic
				if (usedBackground) 
				{
					CFrameBuffer::getInstance()->saveBackgroundImage();
					CFrameBuffer::getInstance()->clearFrameBuffer();

					CFrameBuffer::getInstance()->blit();
				}

				if(live_fe)
					g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
			}
		} 
		else 
		{
			if (g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
			{
				// restore previous background
				if (usedBackground) 
				{
					CFrameBuffer::getInstance()->restoreBackgroundImage();
					CFrameBuffer::getInstance()->useBackground(true);
					CFrameBuffer::getInstance()->paintBackground();
					CFrameBuffer::getInstance()->blit();
				}
			
				CFrameBuffer::getInstance()->loadBackgroundPic("radiomode.jpg");
				CFrameBuffer::getInstance()->blit();
			}

			delete g_Radiotext;
			g_Radiotext = NULL;
		}
		
		return true;
	}

	return false;
}


// TZ notifier
bool CTZChangeNotifier::changeNotify(const std::string&, void * Data)
{
	bool found = false;
	std::string name, zone;
	
	dprintf(DEBUG_NORMAL, "CTZChangeNotifier::changeNotify: %s\n", (char *) Data);

        _xmlDocPtr parser = parseXmlFile("/etc/timezone.xml");
	
        if (parser != NULL) 
	{
                _xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;
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

// data reset notifier
extern Zapit_config zapitCfg;
void loadZapitSettings();
void getZapitConfig(Zapit_config *Cfg);

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
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);

		if(audioDecoder)
			audioDecoder->SetHdmiDD(g_settings.hdmi_dd );

		CNeutrinoApp::getInstance()->SetupTiming();

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
extern t_channel_id live_channel_id;

CChannelListSettings::CChannelListSettings()
{
}

CChannelListSettings::~CChannelListSettings()
{
}

int CChannelListSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CChannelListSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "logos_dir") 
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.logos_dir.c_str())) 
		{
			g_settings.logos_dir = b.getSelectedFile()->Name;

			dprintf(DEBUG_NORMAL, "CMiscSettings::exec: new logos dir %s\n", b.getSelectedFile()->Name.c_str());
		}

		getString() = g_settings.logos_dir;

		return ret;
	}
	
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
	
	if (CNeutrinoApp::getInstance()->getWidget("channelssetup"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("channelssetup");
		miscSettingsChannelList = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		miscSettingsChannelList = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		miscSettingsChannelList->setMenuPosition(MENU_POSITION_CENTER);
		miscSettingsChannelList->setWidgetMode(MODE_SETUP);
		miscSettingsChannelList->enableShrinkMenu();
		
		miscSettingsChannelList->enablePaintHead();
		miscSettingsChannelList->setTitle(_("Channellist settings"), NEUTRINO_ICON_SETTINGS);

		miscSettingsChannelList->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsChannelList->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addItem(miscSettingsChannelList);
	}
	
	miscSettingsChannelList->clearItems();
	
	// intros
	miscSettingsChannelList->addItem(new CMenuForwarder(_("back")));
	miscSettingsChannelList->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	miscSettingsChannelList->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsChannelList->addItem( new CMenuSeparator(LINE) );
	
	// HD list
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Create list of HD channels"), &g_settings.make_hd_list, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
	
	// virtual zap
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Virtual zap"), &g_settings.virtual_zap_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// zap cycle
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Zap cycle"), &g_settings.zap_cycle, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	//
	getZapitConfig(&zapitCfg);
	
	// other
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Make Remaining Channels list"), (int *)&zapitCfg.makeRemainingChannelsBouquet, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));

	// scanSDT
	miscSettingsChannelList->addItem( new CMenuOptionChooser(_("Scan SDT for updates"), (int *)&zapitCfg.scanSDT, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true, this) );
	
	//
	miscSettingsChannelList->addItem( new CMenuSeparator(LINE) );
	
	// channellist ca
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Infobar Crypticons"), &g_settings.channellist_ca, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	// logos dir
	miscSettingsChannelList->addItem( new CMenuForwarder(_("logos Dir"), true, g_settings.logos_dir.c_str(), this, "logos_dir" ) );
	
	// logos
	miscSettingsChannelList->addItem(new CMenuOptionChooser(_("Channel Logo"), &g_settings.epgplus_show_logo, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	
	//
	widget->exec(NULL, "");
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
		setZapitConfig(&zapitCfg);
		
		g_Zapit->reinitChannels();
		
		return true;
	}
	else if(OptionName == _("Scan SDT for updates"))
	{
		setZapitConfig(&zapitCfg);
		bool ret = (MessageBox(_("Information"), _("Neutrino restart"), mbrNo, mbNo | mbYes) == mbrYes);

		if(ret)
			CNeutrinoApp::getInstance()->ExitRun(CNeutrinoApp::RESTART);
	}
	
	return false;
}


//// epg settings
CEPGSettings::CEPGSettings()
{
}

CEPGSettings::~CEPGSettings()
{
}

int CEPGSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CEPGSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
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
				CNeutrinoApp::getInstance()->SendSectionsdConfig();
			}
		}

		getString() = g_settings.epg_dir;

		return ret;
	}
	
	showMenu();
	
	return ret;
}

#define EPG_SERVERBOX_TYPE_OPTION_COUNT 3
const keyval EPG_SERVERBOX_TYPE_OPTIONS[EPG_SERVERBOX_TYPE_OPTION_COUNT] =
{
	{ DVB_C, _("Cable") },
	{ DVB_S, _("Sat") },
	{ DVB_T, _("Terrestrial") }
};

#define EPG_SERVERBOX_GUI_OPTION_COUNT 3
const keyval EPG_SERVERBOX_GUI_OPTIONS[EPG_SERVERBOX_GUI_OPTION_COUNT] =
{
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_NHD2, "neutrinoHD2" },
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_NMP, "neutrinoMP" },
	{ SNeutrinoSettings::SATIP_SERVERBOX_GUI_ENIGMA2, "enigma2" }
};

void CEPGSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CEPGSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsEPG = NULL;
	
	if (CNeutrinoApp::getInstance()->getWidget("epgsetup"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("epgsetup");
		miscSettingsEPG = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		miscSettingsEPG = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		miscSettingsEPG->setMenuPosition(MENU_POSITION_CENTER);
		miscSettingsEPG->setWidgetMode(MODE_SETUP);
		miscSettingsEPG->enableShrinkMenu();
		
		miscSettingsEPG->enablePaintHead();
		miscSettingsEPG->setTitle(_("EPG settings"), NEUTRINO_ICON_SETTINGS);

		miscSettingsEPG->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsEPG->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addItem(miscSettingsEPG);
	}
	
	miscSettingsEPG->clearItems();
	
	// intros
	miscSettingsEPG->addItem(new CMenuForwarder(_("back")));
	miscSettingsEPG->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	miscSettingsEPG->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsEPG->addItem( new CMenuSeparator(LINE) );

	// read epg from xml
	CEPGConfigNotifier* epgConfigNotifier = new CEPGConfigNotifier;
	miscSettingsEPG->addItem(new CMenuOptionChooser(_("Restore EPG on boot"), &g_settings.epg_read, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, epgConfigNotifier));

	// save epg
	CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	miscSettingsEPG->addItem(new CMenuOptionChooser(_("Save/Restore epg on reboot"), &g_settings.epg_save, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

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
	miscSettingsEPG->addItem(new CMenuSeparator(LINE | STRING, _("Preferred EPG language")));
	
	CMenuOptionStringChooser * epglangSelect[3];
	CEPGlangSelectNotifier * EPGlangNotifier = new CEPGlangSelectNotifier();
	
	for(int i = 0; i < 3; i++) 
	{
		epglangSelect[i] = new CMenuOptionStringChooser(_("Preferred EPG language"), g_settings.pref_epgs[i], true, EPGlangNotifier, RC_nokey, "", true);
		std::map<std::string, std::string>::const_iterator it;
		
		epglangSelect[i]->addOption("");
		for(it = iso639rev.begin(); it != iso639rev.end(); it++) 
			epglangSelect[i]->addOption(it->first.c_str());
	}
	
	// epglang
	for(int i = 0; i < 3; i++) 
		miscSettingsEPG->addItem(epglangSelect[i]);

	// localtv epg
	miscSettingsEPG->addItem( new CMenuSeparator(LINE) );

	// server box ip
	CIPInput * epg_IP = new CIPInput(_("Server Box IP"), g_settings.epg_serverbox_ip);
	CMenuForwarder* o1 = new CMenuForwarder(_("Server Box IP"), g_settings.epg_enable_online_epg, g_settings.epg_serverbox_ip.c_str(), epg_IP);

	// server gui (neutrino/neutrinohd/enigma2)
	CMenuOptionChooser* o2 = new CMenuOptionChooser(_("Server Box GUI"), &g_settings.epg_serverbox_gui, EPG_SERVERBOX_GUI_OPTIONS, EPG_SERVERBOX_GUI_OPTION_COUNT, g_settings.epg_enable_online_epg);

	// server box type (sat/cable/terrestrial)
	CMenuOptionChooser* o3 = new CMenuOptionChooser(_("Server Box type"), &g_settings.epg_serverbox_type, EPG_SERVERBOX_TYPE_OPTIONS, EPG_SERVERBOX_TYPE_OPTION_COUNT, g_settings.epg_enable_online_epg);

	// online EPG on/off
	COnlineEPGNotifier* onlineEPGNotifier = new COnlineEPGNotifier(o1, o2, o3);

	miscSettingsEPG->addItem(new CMenuOptionChooser(_("Online EPG"), &g_settings.epg_enable_online_epg, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, onlineEPGNotifier));

	miscSettingsEPG->addItem(o1);
	miscSettingsEPG->addItem(o2);
	miscSettingsEPG->addItem(o3);
	
	// xmltv
	
	widget->exec(NULL, "");

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
		item1->setActive(g_settings.epg_enable_online_epg);
		item2->setActive(g_settings.epg_enable_online_epg);
		item3->setActive(g_settings.epg_enable_online_epg);
	}
	
        return true;
}

// epg language select notifier
void sectionsd_set_languages(const std::vector<std::string>& newLanguages);

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
		sectionsd_set_languages(v_languages);
	
	return true;
}

// epg config notifier
bool CEPGConfigNotifier::changeNotify(const std::string&, void *)
{
	dprintf(DEBUG_NORMAL, "CEPGConfigNotifier::changeNotify\n");

        if (g_settings.epg_read)
		CNeutrinoApp::getInstance()->readEPG();
	
        return true;
}

//// filebrowser settings
CFileBrowserSettings::CFileBrowserSettings()
{
}

CFileBrowserSettings::~CFileBrowserSettings()
{
}

int CFileBrowserSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CFileBrowserSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();

	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

void CFileBrowserSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CFileBrowserSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* miscSettingsFileBrowser = NULL;
	
	if (CNeutrinoApp::getInstance()->getWidget("filebrowsersetup"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("filebrowsersetup");
		miscSettingsFileBrowser = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		miscSettingsFileBrowser = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		miscSettingsFileBrowser->setMenuPosition(MENU_POSITION_CENTER);
		miscSettingsFileBrowser->setWidgetMode(MODE_SETUP);
		miscSettingsFileBrowser->enableShrinkMenu();
		
		miscSettingsFileBrowser->enablePaintHead();
		miscSettingsFileBrowser->setTitle(_("Filebrowser settings"), NEUTRINO_ICON_SETTINGS);

		miscSettingsFileBrowser->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		miscSettingsFileBrowser->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addItem(miscSettingsFileBrowser);
	}
	
	miscSettingsFileBrowser->clearItems();
	
	// intros
	miscSettingsFileBrowser->addItem(new CMenuForwarder(_("back")));
	miscSettingsFileBrowser->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	miscSettingsFileBrowser->addItem(new CMenuForwarder(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	miscSettingsFileBrowser->addItem( new CMenuSeparator(LINE) );

	// UTF 
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("File system"), &g_settings.filesystem_is_utf8, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT, true));

	// show rights
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("Show file rights"), &g_settings.filebrowser_showrights, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true));

	// deny dir
	miscSettingsFileBrowser->addItem(new CMenuOptionChooser(_("Absolute start directory"), &g_settings.filebrowser_denydirectoryleave, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true));
	
	//
	widget->exec(NULL, "");
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



