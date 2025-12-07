//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$id: timerlist.cpp 26112025 mohousch $
//	
//	Copyright (C) 2001 Steffen Hehn 'McClean'
//	and some other guys
//	Homepage: http://dbox.cyberphoria.org/
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <global.h>
#include <neutrino2.h>

#include <gui/timerlist.h>
#include <gui/pluginlist.h>
#include <gui/plugins.h>

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>

#include <driver/gdi/fontrenderer.h>
#include <driver/gdi/color.h>

#include <driver/rcinput.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/channellist.h>
#include <gui/filebrowser.h>

#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <system/settings.h>
#include <system/fsmounter.h>
#include <system/debug.h>

//
#include <zapit/zapit.h>
#include <zapit/channel.h>
#include <zapit/bouquets.h>

#include <gui/channel_select.h>


////
extern char recDir[255];			// defined in neutrino.cpp

class CTimerListNewNotifier : public CChangeObserver
{
	private:
		CMenuItem* m1;
		CMenuItem* m2;
		CMenuItem* m3;
		CMenuItem* m4;
		CMenuItem* m5;
		CMenuItem* m6;
		char* display;
		int* iType;
		time_t* stopTime;
	public:
		CTimerListNewNotifier( int *Type, time_t *time, CMenuItem *a1, CMenuItem *a2, CMenuItem *a3, CMenuItem *a4, CMenuItem *a5, CMenuItem *a6, char *d)
		{
			m1 = a1;
			m2 = a2;
			m3 = a3;
			m4 = a4;
			m5 = a5;
			m6 = a6;
			display = d;
			iType = Type;
			stopTime = time;
		}
		
		bool changeNotify(const std::string& /*OptionName*/, void *)
		{
			CTimerd::CTimerEventTypes type = (CTimerd::CTimerEventTypes) *iType;
			
			if(type == CTimerd::TIMER_RECORD)
			{
				*stopTime = (time(NULL)/60)*60;
				struct tm *tmTime2 = localtime(stopTime);
				sprintf( display, "%02d.%02d.%04d %02d:%02d", tmTime2->tm_mday, tmTime2->tm_mon + 1,
							tmTime2->tm_year + 1900,
							tmTime2->tm_hour, tmTime2->tm_min);

				m1->setHidden(false);
				m6->setHidden((recDir != NULL)? false : true);
			}
			else
			{
				*stopTime = 0;
				strcpy(display,"                ");
				m1->setHidden(true);
				m6->setHidden(true);
			}
			
			if(type == CTimerd::TIMER_RECORD ||
				type == CTimerd::TIMER_ZAPTO ||
				type == CTimerd::TIMER_NEXTPROGRAM)
			{
				m2->setHidden(false);
			}
			else
			{
				m2->setHidden(true);
			}
			
			if(type == CTimerd::TIMER_STANDBY)
				m3->setHidden(false);
			else
				m3->setHidden(true);
			
			if(type == CTimerd::TIMER_REMIND)
				m4->setHidden(false);
			else
				m4->setHidden(true);
			
			if(type == CTimerd::TIMER_EXEC_PLUGIN)
				m5->setHidden(false);
			else
				m5->setHidden(true);
			
			return true;
		}
};

class CTimerListRepeatNotifier : public CChangeObserver
{
	private:
		CMenuForwarder *m1;
		CMenuForwarder *m2;

		int* iRepeat;

	public:
		CTimerListRepeatNotifier( int *repeat, CMenuForwarder *a1, CMenuForwarder *a2)
		{
			m1 = a1;
			m2 = a2;
			iRepeat = repeat;
		}

		bool changeNotify(const std::string& /*OptionName*/, void *)
		{
			if(*iRepeat >= (int)CTimerd::TIMERREPEAT_WEEKDAYS)
				m1->setHidden(false);
			else
				m1->setHidden(true);
			
			if (*iRepeat != (int)CTimerd::TIMERREPEAT_ONCE)
				m2->setHidden(false);
			else
				m2->setHidden(true);
			
			return true;
		}
};

class CTimerListApidNotifier : public CChangeObserver
{
	private:
		int *o_dflt;
		int *o_std;
		int *o_alt;
		int *o_ac3;
		CMenuItem *m_dflt;
		CMenuItem *m_std;
		CMenuItem *m_alt;
		CMenuItem *m_ac3;
		
	public:
		CTimerListApidNotifier( int *o1, int *o2, int *o3, int *o4)
		{
			o_dflt = o1;
			o_std = o2;
			o_alt = o3;
			o_ac3 = o4;
		}

		void setItems(CMenuItem *m1, CMenuItem *m2, CMenuItem *m3, CMenuItem *m4)
		{
			m_dflt = m1;
			m_std = m2;
			m_alt = m3;
			m_ac3 = m4;
		}

		bool changeNotify(const std::string& OptionName, void *)
		{
			if(OptionName == _("record default audio streams"))
			{
				if(*o_dflt == 0)
				{
					m_std->setActive(true);
					m_alt->setActive(true);
					m_ac3->setActive(true);
				}
				else
				{
					m_std->setActive(false);
					m_alt->setActive(false);
					m_ac3->setActive(false);
					*o_std = 0;
					*o_alt = 0;
					*o_ac3 = 0;
				}
			}
			else
			{
				if(*o_std || *o_alt || *o_ac3)
					*o_dflt = 0;
			}

			return true;
		}
};

////
const struct button_label TimerListButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , _("Delete"), 0 },
	{ NEUTRINO_ICON_BUTTON_GREEN , _("New timer"), 0 },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Reload"), 0 },
	{ NEUTRINO_ICON_BUTTON_OKAY, _("Modify"), 0 }
};

CTimerList::CTimerList()
{
	dprintf(DEBUG_DEBUG, "CTimerList::CTimerList:\n");

	frameBuffer = CFrameBuffer::getInstance();

	selected = 0;
	skipEventID = 0;

	//
	timerlistWidget = NULL;
	listBox = NULL;
	item = NULL;
	m6 = NULL;
	m7 = NULL;
	m10 = NULL;
	
	sec_timer_id = 0;

	plugin_chooser = NULL;
	strcpy(timerNew.pluginName, "");
	strncpy(timerNew.recordingDir, g_settings.network_nfs_recordingdir, sizeof(timerNew.recordingDir));
	strcpy(timerNew.message, "");

	// box	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() / 20 * 17;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() / 20 * 18;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
	
	valueString.clear();	
}

CTimerList::~CTimerList()
{
	dprintf(DEBUG_DEBUG, "CTimerList::del\n");

	timerlist.clear();

	if (plugin_chooser)
	{
		delete plugin_chooser;
		plugin_chooser = NULL;
	}
	
	if (timerlistWidget)
	{
		delete timerlistWidget;
		timerlistWidget = NULL;
	}
	
	this->clearValueString();
}

int CTimerList::exec(CTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CTimerList::exec: actionKey:%s\n", actionKey.c_str());

	if(parent)
		parent->hide();

	std::string chanName;
	
	CSelectChannelWidget * CSelectChannelWidgetHandler = NULL;
	
	selected = listBox? listBox->getSelected() : 0;
	
	if((actionKey == "RC_red") && !(timerlist.empty()))
	{
		CTimerd::getInstance()->removeTimerEvent(timerlist[selected].eventID);
		skipEventID = timerlist[selected].eventID;
		
		showMainMenu();
		
		return RETURN_EXIT;
	}
	else if(actionKey == "RC_green")
	{
		hide();
			
		showNewTimerMenu();
		
		showMainMenu();
		
		return RETURN_EXIT;
	}
	else if(actionKey == "RC_yellow")
	{
		showMainMenu();
		
		return RETURN_EXIT;
	}
	else if(actionKey == "RC_ok"  && !(timerlist.empty()))
	{
		showModifyTimerMenu();
		
		showMainMenu();
		
		return RETURN_EXIT;
	}
	else if(actionKey == "tv")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "tv");
		
		timerNew_chan_id = CSelectChannelWidgetHandler->getChannelID();
		timerNew_channel_name = CZapit::getInstance()->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		
		this->setValueString(timerNew_channel_name.c_str());
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		return CTarget::RETURN_REPAINT;
	}
	else if(actionKey == "radio")
	{
		CSelectChannelWidgetHandler = new CSelectChannelWidget();
		CSelectChannelWidgetHandler->exec(NULL, "radio");
		
		timerNew_chan_id = CSelectChannelWidgetHandler->getChannelID();
		timerNew_channel_name = CZapit::getInstance()->getChannelName(CSelectChannelWidgetHandler->getChannelID());
		
		this->setValueString(timerNew_channel_name.c_str());
		
		delete CSelectChannelWidgetHandler;
		CSelectChannelWidgetHandler = NULL;
		
		return CTarget::RETURN_REPAINT;
	}
	else if(actionKey == "recording_dir")
	{
		CFileBrowser b;
		b.Dir_Mode = true;
		
		if (b.exec(g_settings.network_nfs_recordingdir))
			strncpy(timerNew.recordingDir, b.getSelectedFile()->Name.c_str(), sizeof(timerNew.recordingDir) - 1);

		this->setValueString(b.getSelectedFile()->Name.c_str());

		return CTarget::RETURN_REPAINT;
	}
	else if (actionKey == "plugin_chooser")
	{
		plugin_chooser = new CPluginChooser(timerNew.pluginName);
		
		plugin_chooser->exec(NULL, "");
		
		this->setValueString(timerNew.pluginName);
		
		return CTarget::RETURN_REPAINT;
	}
	else if (actionKey == "modifytimer")
	{
		timerlist[selected].announceTime = timerlist[selected].alarmTime - 60;
		
		if(timerlist[selected].eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
			CTimerd::getInstance()->getWeekdaysFromStr(&timerlist[selected].eventRepeat, m_weekdaysStr.c_str());
		
		if(timerlist[selected].eventType == CTimerd::TIMER_RECORD)
		{
			timerlist[selected].announceTime -= 120; // 2 more mins for rec timer
			if (timer_apids_dflt)
				timerlist[selected].apids = TIMERD_APIDS_CONF;
			else
				timerlist[selected].apids = (timer_apids_std * TIMERD_APIDS_STD) | (timer_apids_ac3 * TIMERD_APIDS_AC3) | (timer_apids_alt * TIMERD_APIDS_ALT);
				
			CTimerd::getInstance()->modifyTimerAPid(timerlist[selected].eventID,timerlist[selected].apids);
			CTimerd::getInstance()->modifyRecordTimerEvent(timerlist[selected].eventID, timerlist[selected].announceTime, timerlist[selected].alarmTime, timerlist[selected].stopTime, timerlist[selected].eventRepeat, timerlist[selected].repeatCount,timerlist[selected].recordingDir);
		} 
		else
		{
			CTimerd::getInstance()->modifyTimerEvent(timerlist[selected].eventID, timerlist[selected].announceTime, timerlist[selected].alarmTime, timerlist[selected].stopTime, timerlist[selected].eventRepeat, timerlist[selected].repeatCount);
		}
		
		return CTarget::RETURN_EXIT;
	}
	else if (actionKey == "newtimer")
	{
		timerNew.announceTime = timerNew.alarmTime - 60;
		CTimerd::EventInfo eventinfo;
		CTimerd::EventInfo recinfo;
		
		eventinfo.epgID = 0;
		eventinfo.epg_starttime = 0;
		eventinfo.channel_id = timerNew.channel_id;
		eventinfo.apids = TIMERD_APIDS_CONF;
		eventinfo.recordingSafety = false;
		timerNew.standby_on = (timerNew_standby_on == 1);
		void *data = NULL;
		
		if(timerNew.eventType == CTimerd::TIMER_STANDBY)
			data = &(timerNew.standby_on);
		else if(timerNew.eventType == CTimerd::TIMER_NEXTPROGRAM ||
			timerNew.eventType == CTimerd::TIMER_ZAPTO ||
			timerNew.eventType == CTimerd::TIMER_RECORD)
		{
			if(timerNew_channel_name.empty())
				return CTarget::RETURN_REPAINT;
			else
				timerNew.channel_id = timerNew_chan_id;
			
			if (timerNew.eventType == CTimerd::TIMER_RECORD)
			{
				recinfo.epgID = 0;
				recinfo.epg_starttime = 0;
				recinfo.channel_id = timerNew.channel_id;
				recinfo.apids = TIMERD_APIDS_CONF;
				recinfo.recordingSafety = false;

				timerNew.announceTime -= 120; // 2 more mins for rec timer

				strncpy(recinfo.recordingDir, timerNew.recordingDir, sizeof(recinfo.recordingDir));
				data = &recinfo;
			} 
			else
			{
				eventinfo.epgID = 0;
				eventinfo.epg_starttime = 0;
				eventinfo.channel_id = timerNew.channel_id;
		
				data = &eventinfo;
			}
		}
		else if(timerNew.eventType == CTimerd::TIMER_REMIND)
		{
			data = timerNew.message;
		}
		else if (timerNew.eventType == CTimerd::TIMER_EXEC_PLUGIN)
		{	
			data = timerNew.pluginName;
		}
		
		if(timerNew.eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
			CTimerd::getInstance()->getWeekdaysFromStr(&timerNew.eventRepeat, m_weekdaysStr.c_str());

		if (CTimerd::getInstance()->addTimerEvent(timerNew.eventType, data, timerNew.announceTime, timerNew.alarmTime,  timerNew.stopTime, timerNew.eventRepeat, timerNew.repeatCount, false) == -1)
		{
			bool forceAdd = askUserOnTimerConflict(timerNew.announceTime, timerNew.stopTime);

			if (forceAdd)
			{
				CTimerd::getInstance()->addTimerEvent(timerNew.eventType, data, timerNew.announceTime, timerNew.alarmTime, timerNew.stopTime, timerNew.eventRepeat, timerNew.repeatCount, true);
			}
		}
		
		return CTarget::RETURN_EXIT;
	}

	int ret = showMainMenu();
	this->clearValueString();

	return ret;
}

void CTimerList::updateEvents(void)
{
	timerlist.clear();
	
	// get timerlist
	CTimerd::getInstance()->getTimerList(timerlist);
	
	// remove last deleted event from List
	CTimerd::TimerList::iterator timer = timerlist.begin();
	
	for(; timer != timerlist.end(); timer++)
	{
		if(timer->eventID == skipEventID)
		{
			timerlist.erase(timer);
			break;
		}
	}
	
	// sort
	sort(timerlist.begin(), timerlist.end());
}

int CTimerList::showMainMenu()
{
	dprintf(DEBUG_NORMAL, "CTimerList::showMainMenu\n");

	int res = CTarget::RETURN_REPAINT;
	
	//
	updateEvents();
	
	//
	setLCDMode(_("Timerlist"));
	
	timerlistWidget = CNeutrinoApp::getInstance()->getWidget("timerlist");
	
	if (timerlistWidget)
	{
		listBox = (ClistBox*)timerlistWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		timerlistWidget = new CWidget(&cFrameBox);
		timerlistWidget->name = "timerlist";
		
		//
		listBox = new ClistBox(&cFrameBox);
		
		// head
		listBox->enablePaintHead();
		listBox->setTitle(_("Timerlist"), NEUTRINO_ICON_TIMER);
		listBox->enablePaintDate();

		// foot
		listBox->enablePaintFoot();
		listBox->setFootButtons(TimerListButtons, 4);
		
		//
		timerlistWidget->addCCItem(listBox);
	}
	
	listBox->clearItems();

	for (unsigned int count = 0; count < timerlist.size(); count++)
	{
		std::string alarm("");

		CTimerd::timerEvent &timer = timerlist[count];
		char zAlarmTime[25] = {0};
		struct tm *alarmTime = localtime(&(timer.alarmTime));
		strftime(zAlarmTime, 20, "%d.%m. %H:%M", alarmTime);
		char zStopTime[25] = {0};
		struct tm *stopTime = localtime(&(timer.stopTime));
		strftime(zStopTime,20, "%d.%m. %H:%M", stopTime);

		alarm = zAlarmTime;

		if(timer.stopTime != 0)
		{
			alarm += " ";
			alarm += zStopTime;
		}

		// event repeat
		alarm += " ";
		alarm += convertTimerRepeat2String(timer.eventRepeat);

		if (timer.eventRepeat != CTimerd::TIMERREPEAT_ONCE)
		{
			char srepeatcount[25] = {0};
	
			if (timer.repeatCount != 0)
			// Unicode 8734 (hex: 221E) not available in all fonts
			// sprintf(srepeatcount,"∞");
				sprintf(srepeatcount, "00");
			else
				sprintf(srepeatcount, "%ux", timer.repeatCount);

			//alarm += " ";
			//alarm += srepeatcount;
			//FIXME: dont need to show repeatscount
		}

		//
		std::string zAddData("");
		switch(timer.eventType)
		{
			case CTimerd::TIMER_NEXTPROGRAM :
			case CTimerd::TIMER_ZAPTO :
			case CTimerd::TIMER_RECORD :
				{
					zAddData = convertChannelId2String(timer.channel_id); // UTF-8
					if(timer.apids != TIMERD_APIDS_CONF)
					{
						std::string sep = "";
						zAddData += " (";
						if(timer.apids & TIMERD_APIDS_STD)
						{
							zAddData += "STD";
							sep = "/";
						}
						if(timer.apids & TIMERD_APIDS_ALT)
						{
							zAddData += sep;
							zAddData += "ALT";
							sep = "/";
						}
						if(timer.apids & TIMERD_APIDS_AC3)
						{
							zAddData += sep;
							zAddData += "AC3";
							sep = "/";
						}
						zAddData += ')';
					}

					if(timer.epgID != 0)
					{
						CEPGData epgdata;
						
						if (CSectionsd::getInstance()->getEPGid(timer.epgID, timer.epg_starttime, &epgdata))
						{
							zAddData += " : ";
							zAddData += epgdata.title;
						}
						else if(strlen(timer.epgTitle)!=0)
						{
							zAddData += " : ";
							zAddData += timer.epgTitle;
						}
					}
					else if(strlen(timer.epgTitle)!=0)
					{
						zAddData += " : ";
						zAddData += timer.epgTitle;
					}
				}
				break;
			case CTimerd::TIMER_STANDBY:
				{
					zAddData = timer.standby_on ? _("Enter standby") : _("Leave standby");
					break;
				}
			case CTimerd::TIMER_REMIND :
				{
					zAddData = timer.message; // must be UTF-8 encoded !
				}
				break;
			case CTimerd::TIMER_EXEC_PLUGIN :
			{
				zAddData = timer.pluginName;
			}
			break;
			default:{}
		}

		item = new CMenuForwarder(alarm.c_str());
		item->set2lines(true);
		item->setOption(zAddData.c_str());
		item->setOptionInfo(convertTimerType2String(timer.eventType));

		listBox->addItem(item);
	}
	
	timerlistWidget->addKey(CRCInput::RC_red, this, CRCInput::getSpecialKeyName(CRCInput::RC_red));
	timerlistWidget->addKey(CRCInput::RC_green, this, CRCInput::getSpecialKeyName(CRCInput::RC_green));
	timerlistWidget->addKey(CRCInput::RC_yellow, this, CRCInput::getSpecialKeyName(CRCInput::RC_yellow));
	timerlistWidget->addKey(CRCInput::RC_ok, this, CRCInput::getSpecialKeyName(CRCInput::RC_ok));

	res = timerlistWidget->exec(this, "");
	
	if (timerlistWidget)
	{
		delete timerlistWidget;
		timerlistWidget = NULL;
	}
	
	//
	resetLCDMode();

	return(res);
}

void CTimerList::hide()
{
	if (timerlistWidget) 
		timerlistWidget->hide();
			
	//CFrameBuffer::getInstance()->paintBackground();
}

const char * CTimerList::convertTimerType2String(const CTimerd::CTimerEventTypes type) // UTF-8
{
	switch(type)
	{
		case CTimerd::TIMER_SHUTDOWN    : return _("Shutdown") ;
		case CTimerd::TIMER_NEXTPROGRAM : return _("Next program");
		case CTimerd::TIMER_ZAPTO       : return _("Zap to");
		case CTimerd::TIMER_STANDBY     : return _("Standby");
		case CTimerd::TIMER_RECORD      : return _("Record");
		case CTimerd::TIMER_REMIND      : return _("Remind");
		case CTimerd::TIMER_SLEEPTIMER  : return _("Sleeptimer");
		case CTimerd::TIMER_EXEC_PLUGIN : return _("Execute plugin");
		default                         : return _("Unknown");
	}
}

std::string CTimerList::convertTimerRepeat2String(const CTimerd::CTimerEventRepeat rep) // UTF-8
{
	switch(rep)
	{
		case CTimerd::TIMERREPEAT_ONCE               : return _("once");
		case CTimerd::TIMERREPEAT_DAILY              : return _("daily");
		case CTimerd::TIMERREPEAT_WEEKLY             : return _("weekly");
		case CTimerd::TIMERREPEAT_BIWEEKLY           : return _("biweekly");
		case CTimerd::TIMERREPEAT_FOURWEEKLY         : return _("fourweekly");
		case CTimerd::TIMERREPEAT_MONTHLY            : return _("monthly");
		case CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION : return _("see timer");
		default:
			if(rep >=CTimerd::TIMERREPEAT_WEEKDAYS)
			{
				int weekdays = (((int)rep) >> 9);
				std::string weekdayStr = "";
				if(weekdays & 1)
					weekdayStr+= _("Monday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Tuesday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Wednesday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Thursday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Friday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Saturday");
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= _("Sunday");
				return weekdayStr;
			}
			else
				return _("Unknown");
	}
}

std::string CTimerList::convertChannelId2String(const t_channel_id id) // UTF-8
{
	
	std::string name;

	name = CZapit::getInstance()->getChannelName(id); // UTF-8

	if (name.empty())
		name = _("Unknown");

	return name;
}

#define TIMERLIST_REPEAT_OPTION_COUNT 7
const keyval TIMERLIST_REPEAT_OPTIONS[TIMERLIST_REPEAT_OPTION_COUNT] =
{
	{ CTimerd::TIMERREPEAT_ONCE       , _("once")       },
	{ CTimerd::TIMERREPEAT_DAILY      , _("daily")      },
	{ CTimerd::TIMERREPEAT_WEEKLY     , _("weekly")     },
	{ CTimerd::TIMERREPEAT_BIWEEKLY   , _("biweekly")   },
	{ CTimerd::TIMERREPEAT_FOURWEEKLY , _("fourweekly") },
	{ CTimerd::TIMERREPEAT_MONTHLY    , _("monthly")    },
	{ CTimerd::TIMERREPEAT_WEEKDAYS   , _("on weekdays")  }
};

#define TIMERLIST_STANDBY_OPTION_COUNT 2
const keyval TIMERLIST_STANDBY_OPTIONS[TIMERLIST_STANDBY_OPTION_COUNT] =
{
	{ 0 , _("Leave standby") },
	{ 1 , _("Enter standby")  }
};

#define TIMERLIST_TYPE_OPTION_COUNT 7

const keyval TIMERLIST_TYPE_OPTIONS[TIMERLIST_TYPE_OPTION_COUNT] =
{
	{ CTimerd::TIMER_SHUTDOWN   , _("Shutdown")   },
	{ CTimerd::TIMER_ZAPTO      , _("Zap to")       },
	{ CTimerd::TIMER_STANDBY    , _("Standby")     },
	{ CTimerd::TIMER_RECORD     , _("Record")      },
	{ CTimerd::TIMER_SLEEPTIMER , _("Sleeptimer")  },
	{ CTimerd::TIMER_REMIND     , _("Remind")      },
	{ CTimerd::TIMER_EXEC_PLUGIN, _("Execute plugin")  }
};

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, _("No")  },
	{ 1, _("Yes") }
};

int CTimerList::showModifyTimerMenu()
{
	int res = CTarget::RETURN_REPAINT;

	CTimerd::timerEvent *timer = &timerlist[selected];

	//
	ClistBox* timerSettings = NULL;
	CWidget* widget = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("modifytimer");
	
	if (widget)
	{
		timerSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "modifytimer";
		
		//
		timerSettings = new ClistBox(&box);

		timerSettings->setWidgetMode(ClistBox::MODE_SETUP);
		
		//
		timerSettings->enablePaintHead();
		timerSettings->setTitle(_("Modify timer"), NEUTRINO_ICON_TIMER);
		
		//
		timerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		timerSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(timerSettings);
	}
	
	//
	setLCDMode(_("Modify timer"));
	
	timerSettings->clearItems();
	
	// intros
	timerSettings->addItem(new CMenuForwarder(_("back"), true));
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	//
	timerSettings->addItem(new CMenuForwarder(_("Save Timer"), true, NULL, this, "modifytimer", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	char type[80];
	strcpy(type, convertTimerType2String(timer->eventType)); // UTF
	CMenuForwarder *m0 = new CMenuForwarder(_("Timer typ"), false, type);
	timerSettings->addItem( m0);

	// alarm time start
	CDateInput timerSettings_alarmTime(_("Alarm time"), &timer->alarmTime , _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	CMenuForwarder *m1 = new CMenuForwarder(_("Alarm time"), true, (char *)timerSettings_alarmTime.getValue(), &timerSettings_alarmTime );
	timerSettings->addItem( m1);

	// alarm time stop
	CDateInput timerSettings_stopTime(_("Stop time"), &timer->stopTime , _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	if(timer->stopTime != 0)
	{
		CMenuForwarder *m2 = new CMenuForwarder(_("Stop time"), true, (char *)timerSettings_stopTime.getValue(), &timerSettings_stopTime );
		timerSettings->addItem( m2);
	}

	// week days
	CTimerd::getInstance()->setWeekdaysToStr(timer->eventRepeat, (char *)m_weekdaysStr.c_str());
	timer->eventRepeat = (CTimerd::CTimerEventRepeat)(((int)timer->eventRepeat) & 0x1FF);
	CStringInput timerSettings_weekdays(_("on weekdays"), (char *)m_weekdaysStr.c_str(), 7, _("Mo Tu We Th Fr Sa Su"), _("'X'=timer '-' no timer"), "-X");
	CMenuForwarder *m4 = new CMenuForwarder(_("on weekdays"), true, m_weekdaysStr.c_str(), &timerSettings_weekdays );
	CIntInput timerSettings_repeatCount(_("repeats"), (int&)timer->repeatCount,3, _("amount of timer repeats"), _("0 for unlimited repeats"));

	// repeats
	CMenuForwarder *m5 = new CMenuForwarder(_("repeats"), true, (char *)timerSettings_repeatCount.getValue(), &timerSettings_repeatCount);

	// repeat
	CTimerListRepeatNotifier notifier((int *)&timer->eventRepeat, m4, m5);
	CMenuOptionChooser * m3 = new CMenuOptionChooser(_("Repeat"), (int *)&timer->eventRepeat, TIMERLIST_REPEAT_OPTIONS, TIMERLIST_REPEAT_OPTION_COUNT, true, &notifier);

	// recdir
	strncpy(timer->recordingDir, g_settings.network_nfs_recordingdir, sizeof(timer->recordingDir));
	m7 = new CMenuForwarder(_("Recording directory"), true, timer->recordingDir, this, "recording_dir");

	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	timerSettings->addItem(m3);
	timerSettings->addItem(m4);
	timerSettings->addItem(m5);
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	if (timer->eventType == CTimerd::TIMER_RECORD)
	{
		timerSettings->addItem(m7);
	}
	
	//
	CWidget* timerSettings_apidsWidget = NULL;
	ClistBox* timerSettings_apids = NULL;
	
	//
	CBox box;
	box.iWidth = MENU_WIDTH;
	box.iHeight = MENU_HEIGHT;
	box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
	box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
	timerSettings_apidsWidget = new CWidget(&box);
	timerSettings_apidsWidget->name = "apidstimerlist";
	
	//			
	timerSettings_apids = new ClistBox(&box);

	timerSettings_apids->setWidgetMode(ClistBox::MODE_SETUP);
	
	//				
	timerSettings_apids->enablePaintHead();
	timerSettings_apids->setTitle(_("Audio PIDs"), NEUTRINO_ICON_TIMER);
	
	//
	timerSettings_apids->enablePaintFoot();
						
	const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
						
	timerSettings_apids->setFootButtons(&btn);
					
	//
	timerSettings_apidsWidget->addCCItem(timerSettings_apids);

	CTimerListApidNotifier apid_notifier(&timer_apids_dflt, &timer_apids_std, &timer_apids_ac3, &timer_apids_alt);
	timer_apids_dflt = (timer->apids == 0) ? 1 : 0 ;
	timer_apids_std = (timer->apids & TIMERD_APIDS_STD) ? 1 : 0 ;
	timer_apids_ac3 = (timer->apids & TIMERD_APIDS_AC3) ? 1 : 0 ;
	timer_apids_alt = (timer->apids & TIMERD_APIDS_ALT) ? 1 : 0 ;

	timerSettings_apids->addItem(new CMenuForwarder(_("back"), true, NULL, NULL, NULL, CRCInput::RC_nokey, NEUTRINO_ICON_BUTTON_LEFT));
	timerSettings_apids->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	CMenuOptionChooser* ma1 = new CMenuOptionChooser(_("Record default audio streams"), &timer_apids_dflt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids->addItem(ma1);
	CMenuOptionChooser* ma2 = new CMenuOptionChooser(_("Record standard stream"), &timer_apids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids->addItem(ma2);
	CMenuOptionChooser* ma3 = new CMenuOptionChooser(_("Record alternative streams"), &timer_apids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids->addItem(ma3);
	CMenuOptionChooser* ma4 = new CMenuOptionChooser(_("Record AC3 streams"), &timer_apids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids->addItem(ma4);
	apid_notifier.setItems(ma1,ma2,ma3,ma4);

	if(timer->eventType ==  CTimerd::TIMER_RECORD)
	{  
		timerSettings->addItem(new CMenuForwarder(_("Audio PIDs"), true, NULL, timerSettings_apidsWidget));
	}

	res = widget->exec(this, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        resetLCDMode();
	
	return res;
}

int CTimerList::showNewTimerMenu()
{
	int res = CTarget::RETURN_REPAINT;
	
	// Defaults
	timerNew.eventType = CTimerd::TIMER_RECORD ;
	timerNew.eventRepeat = CTimerd::TIMERREPEAT_ONCE ;
	timerNew.repeatCount = 0;
	timerNew.alarmTime = (time(NULL)/60)*60;
	timerNew.stopTime = (time(NULL)/60)*60;
	timerNew.channel_id = 0;
	strcpy(timerNew.message, "");
	timerNew_standby_on = false;
	strncpy(timerNew.recordingDir, g_settings.network_nfs_recordingdir, sizeof(timerNew.recordingDir));

	//
	ClistBox* timerSettings = NULL;
	CWidget* widget = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("newtimer");
	
	if (widget)
	{
		timerSettings = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		CBox box;
		box.iWidth = MENU_WIDTH;
		box.iHeight = MENU_HEIGHT;
		box.iX = CFrameBuffer::getInstance()->getScreenX() + (CFrameBuffer::getInstance()->getScreenWidth() - box.iWidth) / 2;
		box.iY = CFrameBuffer::getInstance()->getScreenY() + (CFrameBuffer::getInstance()->getScreenHeight() - box.iHeight) / 2;
		
		widget = new CWidget(&box);
		widget->name = "newtimer";
		
		//
		timerSettings = new ClistBox(&box);

		timerSettings->setWidgetMode(ClistBox::MODE_SETUP);
		
		//
		timerSettings->enablePaintHead();
		timerSettings->setTitle(_("New timer"), NEUTRINO_ICON_TIMER);
		
		//
		timerSettings->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };
			
		timerSettings->setFootButtons(&btn);
		
		//
		widget->addCCItem(timerSettings);
	}
	
	//
	setLCDMode(_("New timer"));
	
	timerSettings->clearItems();
	
	// intros
	timerSettings->addItem(new CMenuForwarder(_("back"), true));
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	
	//
	timerSettings->addItem(new CMenuForwarder(_("Save timer"), true, NULL, this, "newtimer", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));

	// alarm time
	CDateInput timerSettings_alarmTime(_("Alarm time"), &(timerNew.alarmTime) , _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	CMenuForwarder *m1 = new CMenuForwarder(_("Alarm time"), true, (char *)timerSettings_alarmTime.getValue(), &timerSettings_alarmTime );
	m1->setHidden(false);

	// stop time
	CDateInput timerSettings_stopTime(_("Stop time"), &(timerNew.stopTime) , _("Use 0..9, or use Up/Down,"), _("OK saves, HOME! aborts"));
	CMenuForwarder *m2 = new CMenuForwarder(_("Stop time"), true, (char *)timerSettings_stopTime.getValue(), &timerSettings_stopTime );
	m2->setHidden(false);

	// weeks
	CStringInput timerSettings_weekdays(_("on weekdays"), (char *)m_weekdaysStr.c_str(), 7, _("Mo Tu We Th Fr Sa Su"), _("'X'=timer '-' no timer"), "-X");
	CMenuForwarder *m4 = new CMenuForwarder(_("on weekdays"), true, m_weekdaysStr.c_str(), &timerSettings_weekdays);
	m4->setHidden(true);

	// repeat count
	CIntInput timerSettings_repeatCount(_("repeats"), (int&)timerNew.repeatCount, 3, _("amount of timer repeats"), _("0 for unlimited repeats"));
	CMenuForwarder *m5 = new CMenuForwarder(_("repeats"), true, (char *)timerSettings_repeatCount.getValue(), &timerSettings_repeatCount);
	m5->setHidden(true);

	CTimerListRepeatNotifier notifier((int *)&timerNew.eventRepeat, m4, m5);
	
	// repeat
	CMenuOptionChooser* m3 = new CMenuOptionChooser(_("Repeat"), (int *)&timerNew.eventRepeat, TIMERLIST_REPEAT_OPTIONS, TIMERLIST_REPEAT_OPTION_COUNT, true, &notifier, CRCInput::RC_nokey, "", true);
	
	// channel
	m6 = new CMenuForwarder(_("Channel"), true, timerNew_channel_name.c_str(), this, CNeutrinoApp::getInstance()->getMode() == CNeutrinoApp::mode_tv? "tv" : "radio");

	// recording dir
	m7 = new CMenuForwarder(_("Recording directory"), true, timerNew.recordingDir, this, "recording_dir");

	// sb mode
	CMenuOptionChooser* m8 = new CMenuOptionChooser(_("SB mode"), &timerNew_standby_on, TIMERLIST_STANDBY_OPTIONS, TIMERLIST_STANDBY_OPTION_COUNT, /*false*/true);
	m8->setHidden(true);

	// message
	CStringInputSMS timerSettings_msg(_("Message"), timerNew.message);
	CMenuForwarder *m9 = new CMenuForwarder(_("Message"), true, timerNew.message, &timerSettings_msg );
	m9->setHidden(true);

	// plugin
	m10 = new CMenuForwarder(_("Plugin"), true, timerNew.pluginName, this, "plugin_chooser");
	m10->setHidden(true);

	CTimerListNewNotifier notifier2((int *)&timerNew.eventType,
					&timerNew.stopTime, m2, m6, m8, m9, m10, m7,
					(char *)timerSettings_stopTime.getValue());
					
	CMenuOptionChooser *m0 = new CMenuOptionChooser(_("Timer typ"), (int *)&timerNew.eventType, TIMERLIST_TYPE_OPTIONS, TIMERLIST_TYPE_OPTION_COUNT, true, &notifier2);

	timerSettings->addItem( m0);
	timerSettings->addItem( m1);
	timerSettings->addItem( m2);
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	timerSettings->addItem( m3);
	timerSettings->addItem( m4);
	timerSettings->addItem( m5);
	timerSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	timerSettings->addItem( m6);
	timerSettings->addItem( m7);
	timerSettings->addItem( m8);
	timerSettings->addItem( m9);
	timerSettings->addItem( m10);

	//
	res = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
        resetLCDMode();

	return res;
}

bool askUserOnTimerConflict(time_t announceTime, time_t stopTime)
{
	CTimerd::TimerList overlappingTimers = CTimerd::getInstance()->getOverlappingTimers(announceTime,stopTime);

	std::string timerbuf = _("Timer conflict. Create the timer anyway?");
	timerbuf += "\n";
	
	for (CTimerd::TimerList::iterator it = overlappingTimers.begin(); it != overlappingTimers.end(); it++)
	{
		timerbuf += CTimerList::convertTimerType2String(it->eventType);
		timerbuf += " (";
		timerbuf += CTimerList::convertChannelId2String(it->channel_id); // UTF-8
		if(it->epgID != 0)
		{
			CEPGData epgdata;

			if (CSectionsd::getInstance()->getEPGid(it->epgID, it->epg_starttime, &epgdata))
			{
				timerbuf += ":";
				timerbuf += epgdata.title;
			}
			else if(strlen(it->epgTitle)!=0)
			{
				timerbuf += ":";
				timerbuf += it->epgTitle;
			}
		}
		timerbuf += ")";

		timerbuf += ":\n";
		char at[25] = {0};
		struct tm *annTime = localtime(&(it->announceTime));
		strftime(at,20,"%d.%m. %H:%M",annTime);
		timerbuf += at;
		timerbuf += " - ";

		char st[25] = {0};
		struct tm *sTime = localtime(&(it->stopTime));
		strftime(st,20,"%d.%m. %H:%M",sTime);
		timerbuf += st;
		timerbuf += "\n";
	}

	return (MessageBox(_("Information"), timerbuf.c_str(), CMessageBox::mbrNo, CMessageBox::mbNo | CMessageBox::mbYes) == CMessageBox::mbrYes);
}

