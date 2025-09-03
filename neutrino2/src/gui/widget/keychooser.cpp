//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: keychooser.cpp 31012025 mohousch Exp $
//
//	Copyright (C) 2001 Steffen Hehn 'McClean' and some other guys
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

#include <global.h>
#include <neutrino2.h>

#include <driver/gdi/color.h>

#include <system/debug.h>

#include <gui/widget/keychooser.h>


class CKeyValue : public CMenuSeparator
{
	std::string the_text;
	
	public:
		long keyvalue;

		CKeyValue(long key) : CMenuSeparator(CMenuSeparator::STRING, _("Current key")){keyvalue = key;};

		virtual const char * getString(void)
		{
			the_text  = _("Current key");
			the_text += ": ";
			the_text += CRCInput::getKeyName(keyvalue);
			
			return the_text.c_str();
		};
};

////
CKeyChooser::CKeyChooser(long * const Key, const char* const Title)
{
	frameBuffer = CFrameBuffer::getInstance();
	
	//
	title = Title? Title : " ";
	key = Key;
	keyChooser = new CKeyChooserItem(_("Setup New Key"), key);
	keyDeleter = new CKeyChooserItemNoKey(key);
	widget = NULL;
	menu = NULL;
	selected = -1;
	
	//
	cFrameBox.iWidth = MENU_WIDTH;
	cFrameBox.iHeight = MENU_HEIGHT;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}

void CKeyChooser::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

int CKeyChooser::paint()
{
	int ret = RETURN_REPAINT;
	
	widget = CNeutrinoApp::getInstance()->getWidget("keychooser");	
	
	if (widget)
	{
		menu = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
		
		widget->setTitle(title.c_str(), NEUTRINO_ICON_KEYBINDING);
	}
	else
	{
		//
		widget = new CWidget(&cFrameBox);
		widget->name = "keychooser";
		widget->enableSaveScreen();
		
		//
		menu = new ClistBox(&cFrameBox);
		menu->setWidgetMode(ClistBox::MODE_MENU);
		
		//	
		menu->enablePaintHead();
		menu->setTitle(title.c_str(), NEUTRINO_ICON_KEYBINDING);
			
		//
		menu->enablePaintFoot();		
		const struct button_label btn = { NEUTRINO_ICON_INFO, " ", 0 };		
		menu->setFootButtons(&btn);
			
		//
		widget->addCCItem(menu);
	}

	menu->clear();
	
	menu->addItem(new CKeyValue(*key));
	menu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	menu->addItem(new CMenuForwarder(_("back")));
	menu->addItem(new CMenuSeparator(CMenuSeparator::LINE));
	menu->addItem(new CMenuForwarder(_("Setup new key"), true, NULL, this, "setnewkey"));
	menu->addItem(new CMenuForwarder(_("No key"), true, NULL, this, "deletekey"));


	menu->setSelected(selected);
	
	ret = widget->exec(NULL, "");
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	return ret;
}

int CKeyChooser::exec(CWidgetTarget *parent, const std::string &actionKey)
{
	int ret = RETURN_REPAINT;
	
	if (parent)
		hide();
	
	if (actionKey == "setnewkey")
	{
		keyChooser->exec(NULL, "");
		if (menu) 
		{
			selected = menu->getSelected();
		}
		paint();
		return RETURN_EXIT;
	}
	else if (actionKey == "deletekey")
	{
		keyDeleter->exec(NULL, "");
		if (menu) 
		{
			selected = menu->getSelected();
		}
		paint();
		return RETURN_EXIT;
	}
	
	ret = paint();
	
	return ret;
}

////
CKeyChooserItem::CKeyChooserItem(const char * const Name, long * Key)
{
	name = Name;
	key = Key;
	
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	m_cBox.iWidth = MENU_WIDTH;
	m_cBox.iHeight = hheight + 2*mheight;
	m_cBox.iX = CFrameBuffer::getInstance()->getScreenX() + ((CFrameBuffer::getInstance()->getScreenWidth() - m_cBox.iWidth) >> 1);
	m_cBox.iY = CFrameBuffer::getInstance()->getScreenY() + ((CFrameBuffer::getInstance()->getScreenHeight() - m_cBox.iHeight) >> 1);
}

int CKeyChooserItem::exec(CWidgetTarget *parent, const std::string &)
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

	timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_infobar == 0 ? 0xFFFF : g_settings.timing_infobar);

 get_Message:
	g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
	
	if (msg != CRCInput::RC_timeout)
	{
		if ((msg > 0) && (msg <= CRCInput::RC_MaxRC))
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

	//box
	m_cBoxWindow.setPosition(&m_cBox);
	m_cBoxWindow.setCorner(g_settings.Head_radius | g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	m_cBoxWindow.paint();

	//head 
	m_cTitle.iWidth = m_cBox.iWidth;
	m_cTitle.iHeight = hheight;
	m_cTitle.iX = m_cBox.iX;
	m_cTitle.iY = m_cBox.iY;

	CCHeaders headers(&m_cTitle, name.c_str(), NEUTRINO_ICON_KEYBINDING);
	headers.paint();

	// line 1
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_cBox.iX + BORDER_LEFT, m_cBox.iY + hheight + mheight, m_cBox.iWidth, _("Please press the new key"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8

	// line 2
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(m_cBox.iX + BORDER_LEFT, m_cBox.iY + hheight + mheight*2, m_cBox.iWidth, _("Wait a few seconds for abort"), COL_MENUCONTENT_TEXT_PLUS_0, 0, true); // UTF-8
}

