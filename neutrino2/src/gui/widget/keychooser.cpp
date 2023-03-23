/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: keychooser.cpp 2013/10/12 mohousch Exp $

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino2.h>

#include <driver/color.h>

#include <system/debug.h>

#include <gui/widget/keychooser.h>


class CKeyValue : public CMenuSeparator
{
	std::string the_text;
	public:
		int         keyvalue;

		CKeyValue() : CMenuSeparator(STRING, _("Current key"))
		{
		};

		virtual const char * getString(void)
		{
			the_text  = _("Current key");
			the_text += ": ";
			the_text += CRCInput::getKeyName(keyvalue);
			return the_text.c_str();
		};
};

CKeyChooser::CKeyChooser(int* const Key, const char* const Title, const std::string& Icon)
: CMenuWidget(Title, Icon)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	title = Title? Title : " ";
	key = Key;
	keyChooser = new CKeyChooserItem(_("Setup New Key"), key);
	keyDeleter = new CKeyChooserItemNoKey(key);
	
	/*
	if (CNeutrinoApp::getInstance()->widget_exists("keychooser"))
	{
		widget = CNeutrinoApp::getInstance()->getWidget("keychooser");	
		menu = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
		
		//
		if (menu->hasHead())
		{
			menu->setTitle(title.c_str(), Icon.c_str());
		}
	}
	else
	{
		menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		//menu->setMenuPosition(MENU_POSITION_CENTER);
		menu->setWidgetMode(MODE_SETUP);
		menu->enableShrinkMenu();
		
		//	
		menu->enablePaintHead();
		menu->setTitle(title.c_str(), Icon.c_str());
			
		//
		menu->enablePaintFoot();		
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
		menu->setFootButtons(&btn);
			
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "keychooser";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->enableSaveScreen();
		widget->addWidgetItem(menu);
	}
		
	menu->clear();

	//
	menu->addItem(new CKeyValue());
	menu->addItem(new CMenuSeparator(LINE));
	menu->addItem(new ClistBoxItem(_("back")));
	menu->addItem(new CMenuSeparator(LINE));
	menu->addItem(new ClistBoxItem(_("Setup new key"), true, NULL, keyChooser));
	menu->addItem(new ClistBoxItem(_("No key"), true, NULL, keyDeleter));
	
	//
	menu->enableShrinkMenu();
	*/
	
	enableShrinkMenu();
	
	addItem(new CKeyValue());
	addItem(new CMenuSeparator(LINE));
	addItem(new ClistBoxItem(_("back")));
	addItem(new CMenuSeparator(LINE));
	addItem(new ClistBoxItem(_("Setup new key"), true, NULL, keyChooser));
	addItem(new ClistBoxItem(_("No key"), true, NULL, keyDeleter));
}

CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}

void CKeyChooser::paint()
{
	//(((CKeyValue *)(menu->items[0]))->keyvalue) = *key;
	(((CKeyValue *)(items[0]))->keyvalue) = *key;

	//widget->paint();
	CMenuWidget::paint();
}

/*
int CKeyChooser::exec(CMenuTarget* parent, const std::string& actionKey)
{
	return widget->exec(parent, actionKey);
}
*/

//
CKeyChooserItem::CKeyChooserItem(const char * const Name, int * Key)
{
	name = Name;
	key = Key;
}

int CKeyChooserItem::exec(CMenuTarget* parent, const std::string &)
{
	dprintf(DEBUG_NORMAL, "CKeyChooserItem::exec\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	uint64_t timeoutEnd;

	int res = RETURN_REPAINT;

	if (parent)
		parent->hide();

	paint();
	CFrameBuffer::getInstance()->blit();

	g_RCInput->clearRCMsg();

	timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] == 0 ? 0xFFFF : g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);

 get_Message:
	g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
	
	if (msg != RC_timeout)
	{
		if ((msg > 0) && (msg <= RC_MaxRC))
			*key = msg;
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			res = RETURN_EXIT_ALL;
		else
			goto get_Message;
	}

	hide();
	
	return res;
}

void CKeyChooserItem::hide()
{
	dprintf(DEBUG_INFO, "CKeyChooserItem::hide\n");
	
	m_cBoxWindow.hide();
	CFrameBuffer::getInstance()->blit();
}

void CKeyChooserItem::paint()
{
	dprintf(DEBUG_INFO, "CKeyChooserItem::paint\n");

	int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	m_cBox.iWidth = 600;
	m_cBox.iHeight = hheight +  2*mheight;
	m_cBox.iX = frameBuffer->getScreenX() + ((frameBuffer->getScreenWidth() - m_cBox.iWidth) >> 1);
	m_cBox.iY = frameBuffer->getScreenY() + ((frameBuffer->getScreenHeight() - m_cBox.iHeight) >> 1);

	//box
	m_cBoxWindow.setPosition(&m_cBox);
	m_cBoxWindow.enableSaveScreen();
	m_cBoxWindow.setColor(COL_MENUCONTENT_PLUS_0);
	m_cBoxWindow.paint();

	//head 
	m_cTitle.iWidth = m_cBox.iWidth;
	m_cTitle.iHeight = hheight;
	m_cTitle.iX = m_cBox.iX;
	m_cTitle.iY = m_cBox.iY;

	CHeaders headers(&m_cTitle, name.c_str(), NEUTRINO_ICON_KEYBINDING);
	headers.paint();

	//paint msg...
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_cBox.iX + BORDER_LEFT, m_cBox.iY + hheight + mheight, m_cBox.iWidth, _("Please press the new key"), COL_MENUCONTENT, 0, true); // UTF-8

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_cBox.iX + BORDER_LEFT, m_cBox.iY + hheight + mheight* 2, m_cBox.iWidth, _("Wait a few seconds for abort"), COL_MENUCONTENT, 0, true); // UTF-8
}

