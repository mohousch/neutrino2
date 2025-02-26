//
//	Neutrino-GUI  -   DBoxII-Project
//
///	$id: nfs.h 26022025 mohousch $
//	
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

#ifndef __neutrino_nfs_gui__
#define __neutrino_nfs_gui__

#include <system/settings.h>
#include <system/fsmounter.h>
#include <gui/widget/widget.h>


class CNFSMountGui : public CMenuTarget
{
	private:
		int menu();
		int menuEntry(int nr);

		char m_entry[NETWORK_NFS_NR_OF_ENTRIES][200];
		char ISO_8859_1_entry[NETWORK_NFS_NR_OF_ENTRIES][200];

		CFSMounter::FS_Support m_nfs_sup;
		CFSMounter::FS_Support m_cifs_sup;
		CFSMounter::FS_Support m_lufs_sup;
		CFSMounter::FS_Support m_smbfs_sup;

	public:
		CNFSMountGui();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CNFSUmountGui : public CMenuTarget
{
	private:
		int menu();

	public:
		CNFSUmountGui(){};
		virtual ~CNFSUmountGui(){};
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CNFSSmallMenu : public CMenuTarget
{
	public:
		CNFSSmallMenu(){};
		virtual ~CNFSSmallMenu(){};
		
		int exec( CMenuTarget* parent, const std::string & actionKey );
		void hide();
};

const char * mntRes2Str(CFSMounter::MountRes res);
const char * mntRes2Str(CFSMounter::UMountRes res);

#endif

