/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: neutrino2.h 19.11.2022 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
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


#ifndef __neutrino__
#define __neutrino__

#include <string>

#include <global.h>

#include <configfile.h>

#include <neutrinoMessages.h>

#include <driver/framebuffer.h>

#include <system/setting_helpers.h>
#include <system/configure_network.h>

#include <timerd/timerdtypes.h>

#include <gui/timerlist.h>
#include <gui/channellist.h>          		/* CChannelList */
#include <gui/rc_lock.h>
#include <gui/scan_setup.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

#include <daemonc/remotecontrol.h>    		/* st_rmsg      */

#include <gui/bouquetlist.h>

// zapit
#include <zapit/zapit.h>


//
typedef struct neutrino_font_descr
{
	const char * name;
	const char * filename;
	int          size_offset;
} neutrino_font_descr_struct;

class CNeutrinoApp : public CMenuTarget
{
 	private:
		CFrameBuffer * frameBuffer;

		enum
		{
			mode_unknown = -1,
			mode_tv = 1,		// tv mode
			mode_radio = 2,		// radio mode
			mode_scart = 3,		// scart mode
			mode_standby = 4,	// standby mode
			mode_audio = 5,		// audioplayer mode
			mode_pic = 6,		// pictureviewer mode
			mode_ts = 7,		// movieplayer mode
			mode_mask = 0xFF,	//
			norezap = 0x100		//
		};

		CConfigFile configfile;

		// font
		neutrino_font_descr_struct font;

		// modes
		int mode;
		int lastMode;
		int chmode;	// to custom channel/bouquet mode needed by channel_select widget.
		
		CTimerd::RecordingInfo *nextRecordingInfo;

		struct timeval standby_pressed_at;

		CZapit::responseGetLastChannel firstchannel;

		bool skipShutdownTimer;

		CNVODChangeExec *NVODChanger;
		CRCLock *rcLock;
		
		//
		fb_pixel_t * mute_pixbuf;
		fb_pixel_t * vol_pixbuf;

		void firstChannel();
		void tvMode( bool rezap = true );
		void radioMode( bool rezap = true );
		void standbyMode( bool bOnOff );
		void scartMode( bool bOnOff );
		void realRun(void);
		void initZapper();
		void setupFrameBuffer();
		
		CNeutrinoApp();

	public:
		~CNeutrinoApp();
		static CNeutrinoApp* getInstance();
		
		//
		void setDebugLevel( int level );
		
		//
		int exec(CMenuTarget* parent, const std::string& actionKey);
		
		//
		void saveSetup(const char * fname);
		int loadSetup(const char * fname);
		void setupFonts(const char* font_file);
		
		// skin
		void loadSkin(std::string skinName);
		void readSkinConfig(const char* const filename);
		void saveSkinConfig(const char* const filename);
		
		//
		CWidget *getWidget(const char *const widgetname, const char *const skinfilename = NULL, const bool data = false);
		
		//
		void parseClistBox(xmlNodePtr node, CWidget* widget);
		void parseCWindow(xmlNodePtr node, CWidget* widget);
		void parseCHead(xmlNodePtr node, CWidget* widget);
		void parseCFoot(xmlNodePtr node, CWidget* widget);
		void parseCTextBox(xmlNodePtr node, CWidget* widget);
		void parseCFrameBox(xmlNodePtr node, CWidget* widget){};
		void parseCCLabel(xmlNodePtr node, CWidget* widget);
		void parseCCImage(xmlNodePtr node, CWidget* widget);
		void parseCCTime(xmlNodePtr node, CWidget* widget);
		void parseCCButtons(xmlNodePtr node, CWidget* widget);
		void parseCCHline(xmlNodePtr node, CWidget* widget);
		void parseCCVline(xmlNodePtr node, CWidget* widget);
		void parseCCPig(xmlNodePtr node, CWidget* widget);
		void parseKey(xmlNodePtr node, CWidget* widget);
		//
		CMenuTarget* convertTarget(const std::string& name);
		uint32_t convertColor(const char* const color);
		uint8_t convertFontColor(const char* const color);
		int convertCorner(const char * const corner);
		int convertRadius(const char* const radius);
		int convertGradient(const char* const gradient);
		neutrino_msg_t convertKey(const char* const key);
		int convertMenuPosition(const char * const position);
		int convertClistBoxMode(const char * const mode);
		int convertClistBoxType(const char * const type);
		int convertCMenuItemID(const char * const id);
		int convertBool(const char* const value);
		int convertBorder(const char * const border);
		int convertFontSize(const char * const size);
		
		//
		void mainMenu(void);
		bool showUserMenu(int button);
		void selectNVOD();
                bool getNVODMenu(ClistBox* menu);

		void audioMute( int newValue, bool isEvent= false );
		void setVolume(const neutrino_msg_t key, const bool bDoPaint = true);

		// channellist
		CChannelList* TVchannelList;
		CChannelList* RADIOchannelList;

		CChannelList* channelList;

		//
		void channelsInit(bool bOnly = false);
		
		CColorSetupNotifier *colorSetupNotifier;

		//
		void readEPG();
		void saveEpg();

		//
		int run(int argc, char **argv);
		int handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);

		//
		int getMode() { return mode; };
		int getLastMode() { return lastMode; };
		int getChMode(){return chmode;};	// to custom channel/bouquet mode needed by channel_select widget.
		
		//
		bool doGuiRecord(char * preselectedDir, bool addTimer = false);
		void startNextRecording();
		int startAutoRecord(bool addTimer);
		void stopAutoRecord();
		
		//
		int recordingstatus;
		int timeshiftstatus;
		int recording_id;
				
		//
		void sendSectionsdConfig(void);

		//
		int getChannelMode(void) { return g_settings.channel_mode; };
		void setChannelMode(int newmode, int nMode);
		
		//
		void quickZap(int msg);
		void showInfo(const CZapitChannel *channel);
		
		//
		void stopSubtitles();
		void startSubtitles(bool show = true);
		void selectSubtitles();

		//
		void lockPlayBack(void);
		void unlockPlayBack(void);
		
		// 0 - restart 
		// 1 - shutdown
		// 2 - reboot
		enum {
			RESTART = 0,
			SHUTDOWN,
			REBOOT
		};
		
		void exitRun(int retcode = SHUTDOWN, bool save = true);
};

#endif
