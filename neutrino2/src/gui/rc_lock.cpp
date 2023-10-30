/*
	Neutrino-GUI  -   DBoxII-Project

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

#include <gui/rc_lock.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>

#include <system/debug.h>


const std::string CRCLock::NO_USER_INPUT = "noUserInput";

int CRCLock::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "CRCLock::exec\n");

	if (parent)
		parent->hide();

	bool no_input = (actionKey == NO_USER_INPUT);

	if (MessageBox(_("Lock Remote Control"), _("Your box remote control will be locked.\n To unlock it, press <RED> \n and <MENU> on your remote control."), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel, NEUTRINO_ICON_INFO, 450, no_input ? 5 : -1, no_input) == CMessageBox::mbrCancel)
		return CMenuTarget::RETURN_EXIT_ALL;

	// -- Lockup Box	
	lockBox();

	MessageBox(_("Lock Remote Control"), _("Remote control reactivated."), CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO, 450, no_input ? 5 : -1);

	return  CMenuTarget::RETURN_EXIT_ALL;
}

void CRCLock::lockBox()
{
	dprintf(DEBUG_DEBUG, "CRCLock::lockBox\n");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	uint64_t timeoutEnd;

	// -- Loop until release key pressed
	// -- Key sequence:  <RED> <DBOX> within 5 secs
	while  (1) 
	{
		timeoutEnd = CRCInput::calcTimeoutEnd(9999999);
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if (msg == NeutrinoMessages::UNLOCK_RC)
			break;

		if (msg == CRCInput::RC_red)  
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(5);
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if (msg == CRCInput::RC_setup)
				break;
		}

		if (msg == CRCInput::RC_timeout) 
			continue;

		// -- Zwen told me: Eating only RC events would be nice
		// -- so be it...

		if (msg > CRCInput::RC_MaxRC) 
		{
			CNeutrinoApp::getInstance()->handleMsg(msg, data); 
		} 
		else 
		{
			CVFD::getInstance()->showRCLock();
			// Since showRCLock blocks 2secs for each key we eat all
			// messages created during this time. Hopefully this ok...
			g_RCInput->clearRCMsg();
		}
	}

	return;
}

