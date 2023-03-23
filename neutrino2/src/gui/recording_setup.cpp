/*
	Neutrino-GUI  -   DBoxII-Project

	$id: recording_setup.cpp 2016.01.02 21:43:30 mohousch $
	
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

#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>

#include <gui/filebrowser.h>
#include <gui/recording_setup.h>

#include <system/debug.h>
#include <system/setting_helpers.h>
#include <system/helpers.h>

#include <timerd/timerd.h>


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

// recording settings
extern char recDir[255];			// defined in neutrino.cpp
extern char timeshiftDir[255];			// defined in neutrino.cpp
extern bool autoshift;				// defined in neutrino.cpp
extern int startAutoRecord(bool addTimer);	// defined in neutrino.cpp
extern void stopAutoRecord();			// defined in neutrino.cpp

CRecordingSettings::CRecordingSettings()
{
}

CRecordingSettings::~CRecordingSettings()
{
}

int CRecordingSettings::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CRecordingSettings::exec: actionKey: %s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	if(actionKey == "savesettings")
	{
		CNeutrinoApp::getInstance()->exec(NULL, "savesettings");
		
		return ret;
	}
	else if(actionKey == "recording")
	{
		CHintBox * hintBox = new CHintBox(_("Information"), _("Save settings now")); // UTF-8
		hintBox->paint();
		
		CNeutrinoApp::getInstance()->setupRecordingDevice();
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
		
		return ret;
	}
	else if(actionKey == "recordingdir")
	{
		if(parent)
			parent->hide();
		
		CFileBrowser b;
		b.Dir_Mode = true;

		if (b.exec(g_settings.network_nfs_recordingdir)) 
		{
			const char * newdir = b.getSelectedFile()->Name.c_str();
			dprintf(DEBUG_NORMAL, "CRecordingSettings::exec: New recordingdir: selected %s\n", newdir);

			if(check_dir(newdir))
				printf("CRecordingSettings::exec: Wrong/unsupported recording dir %s\n", newdir);
			else
			{
				strncpy(g_settings.network_nfs_recordingdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_recordingdir)-1 );
				
				dprintf(DEBUG_NORMAL, "CRecordingSettings::exec: New recordingdir: %s\n", g_settings.network_nfs_recordingdir);
				
				sprintf(timeshiftDir, "%s/.timeshift", g_settings.network_nfs_recordingdir);
					
				safe_mkdir(timeshiftDir);
				dprintf(DEBUG_NORMAL, "CRecordingSettings::exec: New timeshift dir: %s\n", timeshiftDir);
			}
		}

		getString() = g_settings.network_nfs_recordingdir;
		
		return ret;
	}
	
	showMenu();
	
	return ret;
}

bool CRecordingSettings::changeNotify(const std::string& OptionName, void */*data*/)
{
	dprintf(DEBUG_NORMAL, "CRecordingSettings::changeNotify:\n");
	
	if(OptionName == _("Permanent timeshift")) 
	{	  
		if(g_settings.auto_timeshift)
			startAutoRecord(true);
		else
		{
			if(autoshift) 
			{
				stopAutoRecord();
				
				CNeutrinoApp::getInstance()->recordingstatus = 0;
				CNeutrinoApp::getInstance()->timeshiftstatus = 0;
			}
		}
	
		return true;
	}

	return false;
}

void CRecordingSettings::showMenu()
{
	dprintf(DEBUG_NORMAL, "CRecordingSettings::showMenu:\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* recordingSettings = NULL; 
	
	widget = CNeutrinoApp::getInstance()->getWidget("recordingsetup");
	
	if (widget)
	{
		recordingSettings = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		recordingSettings = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		recordingSettings->setWidgetMode(MODE_SETUP);
		recordingSettings->enableShrinkMenu();
		
		recordingSettings->enablePaintHead();
		recordingSettings->setTitle(_("Recording settings"), NEUTRINO_ICON_RECORDING);

		recordingSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		recordingSettings->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "recordingsetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(recordingSettings);
	}
	
	recordingSettings->clearItems();
	
	//
	int rec_pre = 0;
	int rec_post = 0;
	
	CTimerd::getInstance()->getRecordingSafety(rec_pre, rec_post);

	sprintf(g_settings.record_safety_time_before, "%02d", rec_pre/60);
	sprintf(g_settings.record_safety_time_after, "%02d", rec_post/60);

	CRecordingSafetyNotifier *RecordingSafetyNotifier = new CRecordingSafetyNotifier;

	//safety time befor
	CStringInput * timerBefore = new CStringInput(_("Record start time correction"), g_settings.record_safety_time_before, 2, _("Correction time in min. (00=off). This time"), _("will be deducted of every record timer."),"0123456789 ", RecordingSafetyNotifier);
	ClistBoxItem *fTimerBefore = new ClistBoxItem(_("Record start time correction"), true, g_settings.record_safety_time_before, timerBefore);

	//safety time after
	CStringInput * timerAfter = new CStringInput(_("Record stop time correction"), g_settings.record_safety_time_after, 2, _("Correction time in min. (00=off). This time"), _("will added to stop time of every record timer."),"0123456789 ", RecordingSafetyNotifier);
	ClistBoxItem *fTimerAfter = new ClistBoxItem(_("Record stop time correction"), true, g_settings.record_safety_time_after, timerAfter);

	//audiopids
	g_settings.recording_audio_pids_std = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_STD ) ? 1 : 0 ;
	g_settings.recording_audio_pids_alt = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_ALT ) ? 1 : 0 ;
	g_settings.recording_audio_pids_ac3 = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_AC3 ) ? 1 : 0 ;
	
	CRecAPIDSettingsNotifier * an = new CRecAPIDSettingsNotifier;

	//default
	CMenuOptionChooser* aoj1 = new CMenuOptionChooser(_("default audio streams"), &g_settings.recording_audio_pids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);

	//alt
	CMenuOptionChooser* aoj2 = new CMenuOptionChooser(_("Record alternative streams"), &g_settings.recording_audio_pids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);

	//ac3
	CMenuOptionChooser* aoj3 = new CMenuOptionChooser(_("Record AC3 streams"), &g_settings.recording_audio_pids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);

	//epg in name format
	CMenuOptionChooser* oj11 = new CMenuOptionChooser(_("Filenames (with EPG data)"), &g_settings.recording_epg_for_filename, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	// save in channeldir
	CMenuOptionChooser* oj13 = new CMenuOptionChooser(_("Save in channel dir"), &g_settings.recording_save_in_channeldir, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	//RecDir
	ClistBoxItem* fRecDir = new ClistBoxItem(_("Recording directory"), true, g_settings.network_nfs_recordingdir, this, "recordingdir");

	// intros
	recordingSettings->addItem(new ClistBoxItem(_("back")));
	recordingSettings->addItem( new CMenuSeparator(LINE) );
	
	// save settings
	recordingSettings->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, CNeutrinoApp::getInstance(), "savesettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	recordingSettings->addItem(new ClistBoxItem(_("Activate changes"), true, NULL, this, "recording"));

	recordingSettings->addItem(new CMenuSeparator(LINE | STRING, _("Timer settings")));
	recordingSettings->addItem(fTimerBefore);
	recordingSettings->addItem(fTimerAfter);

	//apids
	recordingSettings->addItem(new CMenuSeparator(LINE | STRING, _("default audio streams")));
	recordingSettings->addItem(aoj1);
	recordingSettings->addItem(aoj2);
	recordingSettings->addItem(aoj3);

	//
	recordingSettings->addItem(new CMenuSeparator(LINE));
	//epg in name format
	recordingSettings->addItem(oj11);
	
	// save in channeldir
	recordingSettings->addItem(oj13);

	recordingSettings->addItem(new CMenuSeparator(LINE));

	//recdir
	recordingSettings->addItem(fRecDir);
	
	// timeshift
	recordingSettings->addItem(new CMenuSeparator(LINE | STRING, _("Timeshift")));
	
	// record time
	recordingSettings->addItem(new CMenuOptionNumberChooser(_("Fast/timeshift record time(hours)"), &g_settings.record_hours, true, 1, 24, NULL) );

	// timeshift
	if (recDir != NULL)
	{
		// permanent timeshift
		recordingSettings->addItem(new CMenuOptionChooser(_("Permanent timeshift"), &g_settings.auto_timeshift, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, this));
	}
	
	//
	widget->exec(NULL, "");
}

// recording safety notifier
bool CRecordingSafetyNotifier::changeNotify(const std::string&, void *)
{
	CTimerd::getInstance()->setRecordingSafety(atoi(g_settings.record_safety_time_before)*60, atoi(g_settings.record_safety_time_after)*60);

   	return true;
}

// rec apids notifier
bool CRecAPIDSettingsNotifier::changeNotify(const std::string&, void *)
{
	g_settings.recording_audio_pids_default = ( (g_settings.recording_audio_pids_std ? TIMERD_APIDS_STD : 0) | (g_settings.recording_audio_pids_alt ? TIMERD_APIDS_ALT : 0) | (g_settings.recording_audio_pids_ac3 ? TIMERD_APIDS_AC3 : 0));

	return true;
}



