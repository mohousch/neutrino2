	/*
	$Id: audio_video_select.h 2015/07/26 10:45:30 mohousch Exp $

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

#ifndef __AUDIO_VIDEO_SELECT_h__
#define __AUDIO_VIDEO_SELECT_h__

#include <sys/types.h>

#include <string>
#include <vector>

#include <sys/stat.h>

#include <gui/widget/widget.h>
#include <gui/widget/listbox.h>

//#include <gui/movieinfo.h>


////
class CAVPIDSelectWidget : public CMenuTarget
{		
	public:
		int showAudioDialog();
		int exec(CMenuTarget * parent, const std::string & actionKey);
};

////
class CAVPIDChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

////
class CAVSubPIDChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif

