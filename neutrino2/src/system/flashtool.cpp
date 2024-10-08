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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libmd5sum.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/reboot.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,7)
#include <mtd/mtd-user.h>
#else
#include <mtd/mtd-user.h>
#include <linux/mtd/mtd.h>
#endif

#include <global.h>

#include <driver/encoding.h>

#include <system/debug.h>
#include <system/flashtool.h>


CFlashTool::CFlashTool()
{
	statusViewer = NULL;
	mtdDevice = 	"";
	ErrorMessage = 	"";
}

void CFlashTool::setStatusViewer( CProgressWindow *statusview )
{
	statusViewer = statusview;
}

const std::string & CFlashTool::getErrorMessage(void) const
{
	return ErrorMessage;
}

void CFlashTool::setMTDDevice( const std::string & mtddevice )
{
	mtdDevice = mtddevice;
	dprintf(DEBUG_NORMAL, "flashtool.cpp: set mtd device to %s\n", mtddevice.c_str());
}

bool CFlashTool::readFromMTD( const std::string &filename, int globalProgressEnd )
{
	int fd1, fd2;
	long filesize;
	int globalProgressBegin = 0;

	if (mtdDevice.empty())
	{
		ErrorMessage = "mtd-device not set";
		return false;
	}

	if( (fd1 = open( mtdDevice.c_str(), O_RDONLY )) < 0 )
	{
		ErrorMessage = _("can't open mtd-device");
		return false;
	}

	if( (fd2 = open( filename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP  |  S_IROTH | S_IWOTH)) < 0 )
	{
		ErrorMessage = _("can't open file");
		close(fd1);
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}
	filesize = CMTDInfo::getInstance()->getMTDSize(mtdDevice);

	char buf[1024];
	long fsize = filesize;

	while(fsize > 0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( fd1, &buf, block);
		write( fd2, &buf, block);
		fsize -= block;
		char prog = char(100-(100./filesize*fsize));

		dprintf(DEBUG_NORMAL, "\rReading %s to %s %2u %% complete.\n", mtdDevice.c_str(), filename.c_str(), prog );
		
		if(statusViewer)
		{
			if(globalProgressEnd != -1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEnd-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}
	}

	close(fd1);
	close(fd2);

	return true;
}

bool CFlashTool::program( const std::string &filename, int globalProgressEndErase, int globalProgressEndFlash )
{
	int fd1, fd2;
	long filesize;
	int globalProgressBegin = 0;

	if (mtdDevice.empty())
	{
		ErrorMessage = "mtd-device not set";
		return false;
	}

	if( (fd1 = open( filename.c_str(), O_RDONLY )) < 0 )
	{
		ErrorMessage = _("can't open file");
		return false;
	}

	filesize = lseek( fd1, 0, SEEK_END);
	lseek( fd1, 0, SEEK_SET);

	if(filesize == 0)
	{
		ErrorMessage = _("the filesize is 0 Bytes");
		return false;
	}

	// erase
	if(statusViewer)
	{
		statusViewer->showStatusMessageUTF(_("erasing flash")); // UTF-8
	}
	
	if(!erase(globalProgressEndErase))
	{
		return false;
	}

	if(statusViewer)
	{
		if(globalProgressEndErase != -1)
		{
			statusViewer->showGlobalStatus(globalProgressEndErase);
		}

		statusViewer->showStatusMessageUTF(_("programming flash")); // UTF-8
	}

	// write
	if( (fd2 = open( mtdDevice.c_str(), O_WRONLY )) < 0 )
	{
		ErrorMessage = _("can't open mtd-device");
		close(fd1);
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}

	char buf[1024];
	long fsize = filesize;

	while(fsize > 0)
	{
		long block = fsize;
		if(block>(long)sizeof(buf))
		{
			block = sizeof(buf);
		}
		read( fd1, &buf, block);
		write( fd2, &buf, block);
		fsize -= block;
		char prog = char(100-(100./filesize*fsize));

		dprintf(DEBUG_NORMAL, "\rFlashing %s to %s %2u %% complete.\n", filename.c_str(), mtdDevice.c_str(), prog );

		if(statusViewer)
		{
			if(globalProgressEndFlash != -1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEndFlash-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}
	}

	close(fd1);
	close(fd2);

	return true;
}

bool CFlashTool::erase(int globalProgressEnd)
{
	int fd;
	mtd_info_t meminfo;
	erase_info_t lerase;
	int globalProgressBegin = 0;

	if( (fd = open( mtdDevice.c_str(), O_RDWR )) < 0 )
	{
		ErrorMessage = _("can't open mtd-device");
		return false;
	}

	if( ioctl( fd, MEMGETINFO, &meminfo ) != 0 )
	{
		ErrorMessage = _("can't get mtd-info");
		return false;
	}

	if(statusViewer)
	{
		globalProgressBegin = statusViewer->getGlobalStatus();
	}

	lerase.length = meminfo.erasesize;
	for (lerase.start = 0; lerase.start < meminfo.size; lerase.start += meminfo.erasesize)
	{
		dprintf(DEBUG_NORMAL, "Erasing %s erase size %x start %x size %x\n", mtdDevice.c_str(), meminfo.erasesize, lerase.start, meminfo.size );
		dprintf(DEBUG_NORMAL, "\rErasing %u Kbyte @ %x -- %2u %% complete.", meminfo.erasesize/1024, lerase.start, lerase.start*100/meminfo.size );
		
		if(statusViewer)
		{
			int prog = int(lerase.start*100./meminfo.size);
			
			if(globalProgressEnd!=-1)
			{
				int globalProg = globalProgressBegin + int((globalProgressEnd-globalProgressBegin) * prog/100. );
				statusViewer->showGlobalStatus(globalProg);
			}
		}

		if(ioctl( fd, MEMERASE, &lerase) != 0)
		{
			ErrorMessage = _("erasure of flash failed");
			close(fd);
			return false;
		}
	}

	close(fd);

	return true;
}

#define FROMHEX(c) ((c)>='a' ? (c)-'a'+10 : ((c)>='A' ? (c)-'A'+10 : (c)-'0'))
bool CFlashTool::check_md5( const std::string & filename, const std::string & smd5)
{
	unsigned char md5[16];
	unsigned char omd5[16];

	const char * ptr = smd5.c_str();

	if(strlen(ptr) < 32)
		return false;
	
	dprintf(DEBUG_NORMAL, "CFlashTool::check_md5: check file %s md5 %s\n", filename.c_str(), ptr);

	for(int i = 0; i < 16; i++)
		omd5[i] = FROMHEX(ptr[i*2])*16 + FROMHEX(ptr[i*2+1]);

	md5_file(filename.c_str(), 1, md5);
	
	if(memcmp(md5, omd5, 16))
		return false;
	
	return true;
}

void CFlashTool::reboot()
{
	::sync();
	::reboot(RB_AUTOBOOT);
	::exit(0);
}

////
CFlashVersionInfo::CFlashVersionInfo(const std::string &versionString)
{

	for(int i = 0; i < 20; i++)
		releaseCycle[i] = versionString[i];
	
	//SBBBYYYYMMTTHHMM -- formatsting

	// recover type
	snapshot = versionString[0];

	// recover release cycle version
	releaseCycle[0] = versionString[1];
	releaseCycle[1] = '.';

	if (versionString[2] == '0')
	{
		releaseCycle[2] = versionString[3];
		releaseCycle[3] = 0;
	}
	else 
	{
		releaseCycle[2] = versionString[2];
		releaseCycle[3] = versionString[3];
		releaseCycle[4] = 0;
	}

	// recover date
	date[0] = versionString[10];
	date[1] = versionString[11];
	date[2] = '.';
	date[3] = versionString[8];
	date[4] = versionString[9];
	date[5] = '.';
	date[6] = versionString[4];
	date[7] = versionString[5];
	date[8] = versionString[6];
	date[9] = versionString[7];
	date[10] = 0;

	// recover time stamp
	time[0] = versionString[12];
	time[1] = versionString[13];
	time[2] = ':';
	time[3] = versionString[14];
	time[4] = versionString[15];
	time[5] = 0; 
}

const char * CFlashVersionInfo::getDate(void) const
{
	return date;
}

const char * CFlashVersionInfo::getTime(void) const
{
	return time;
}

const char * CFlashVersionInfo::getReleaseCycle(void) const
{
	return releaseCycle;
}

const char * CFlashVersionInfo::getType(void) const
{
	switch (snapshot)
	{
		case '0':
			return "Release";
		case '1':
			return "Snapshot";
		case '2':
			return "Beta";
		case 'A':
			return "Addon";
		case 'T':
			return "Text";
		default:
			return "Unknown";
	}
}

//// MTDInfo
CMTDInfo::CMTDInfo()
{
	getPartitionInfo();
}

CMTDInfo::~CMTDInfo()
{
	for(int x = 0; x < getMTDCount(); x++)
	{
		delete mtdData[x];
	}
	mtdData.clear();
}

CMTDInfo* CMTDInfo::getInstance()
{
	static CMTDInfo* MTDInfo = NULL;

	if(!MTDInfo)
	{
		MTDInfo = new CMTDInfo();
	}
	return MTDInfo;
}

void CMTDInfo::getPartitionInfo()
{
	FILE* fd = fopen("/proc/mtd", "r");
	if(!fd)
	{
		perror("cannot read /proc/mtd");
		return;
	}
	
	char buf[1000];
	fgets(buf,sizeof(buf),fd);
	
	while(!feof(fd))
	{
		if(fgets(buf, sizeof(buf), fd) != NULL)
		{
			char mtdname[50] = "";
			int mtdnr = 0;
			int mtdsize = 0;
			int mtderasesize = 0;
			
			sscanf(buf, "mtd%d: %x %x \"%s\"\n", &mtdnr, &mtdsize, &mtderasesize, mtdname);
			SMTDPartition *tmp = new SMTDPartition;
			tmp->size = mtdsize;
			tmp->erasesize = mtderasesize;
			std::string tmpstr = buf;
			tmp->name = tmpstr.substr( tmpstr.find('\"') + 1, tmpstr.rfind('\"') - tmpstr.find('\"') - 1);
			sprintf((char*) &buf, "/dev/mtd%d", mtdnr);
			tmp->filename = buf;
			mtdData.push_back(tmp);
		}
	}
	fclose(fd);
}

int CMTDInfo::getMTDCount()
{
	return mtdData.size();
}

std::string CMTDInfo::getMTDName(const int pos)
{
//#warning TODO: check /proc/mtd specification to determine mtdname encoding

	return FILESYSTEM_ENCODING_TO_UTF8(std::string(mtdData[pos]->name).c_str());
}

std::string CMTDInfo::getMTDFileName(const int pos)
{
	return mtdData[pos]->filename;
}

int CMTDInfo::getMTDSize(const int pos)
{
	return mtdData[pos]->size;
}

int CMTDInfo::getMTDEraseSize(const int pos)
{
	return mtdData[pos]->erasesize;
}

int CMTDInfo::findMTDNumber(const std::string &filename)
{
	for(int x = 0; x < getMTDCount(); x++)
	{
		if(filename == getMTDFileName(x))
		{
			return x;
		}
	}
	return -1;
}

std::string CMTDInfo::getMTDName(const std::string &filename)
{
	return getMTDName( findMTDNumber(filename) );
}

int CMTDInfo::getMTDSize( const std::string &filename )
{
	return getMTDSize( findMTDNumber(filename) );
}

int CMTDInfo::getMTDEraseSize( const std::string &filename )
{
	return getMTDEraseSize( findMTDNumber(filename) );
}

std::string CMTDInfo::findMTDsystem(const std::string &filename)
{
	for(int i = 0; i < getMTDCount(); i++) 
	{
		if(getMTDName(i) == filename) 
		{
			return getMTDFileName(i);
		}
	}
	
	return "";
}

////
#define OM_MAX_LINE_LENGTH 512

COPKGManager::COPKGManager()
{
	v_pkg_list.clear();
	v_pkg_installed.clear();
	v_pkg_upgradable.clear();
}

const opkg_cmd_struct_t pkg_types[OM_MAX] =
{
	{OM_LIST, 		"opkg list"},
	{OM_LIST_INSTALLED, 	"opkg list-installed"},
	{OM_LIST_UPGRADEABLE,	"opkg list-upgradable"},
	{OM_UPDATE,		"opkg update"},
	{OM_UPGRADE,		"opkg upgrade"},
};

const char *pkg_menu_names[] = {
	"List All",
	"List Installed",
	"List Upgradable",
	"Update Package List",
	"Upgrade System",
};

const char * cmd_names[] = {
	"list",
	"list-installed",
	"list-upgradable",
	"update",
	"upgrade",
};

bool COPKGManager::hasOpkgSupport()
{
	std::string deps[] = {"/bin/opkg-cl", "/bin/opkg-key", "/etc/opkg/opkg.conf", "/var/lib/opkg"};
	bool ret = true;
	
	for (uint i = 0; i < (sizeof(deps) / sizeof(deps[0])); i++)
	{
		if(access(deps[i].c_str(), R_OK) !=0)
		{
			printf("[neutrino opkg] %s not found\n", deps[i].c_str());
			ret = false;
		}
	}
	
	return ret;
}

bool COPKGManager::getPkgData(const int pkg_content_id, std::vector<std::string>* vp_pkg_menu)
{
	char cmd[100];
	FILE * output;
	char buf[OM_MAX_LINE_LENGTH];
	int in, pos;
	bool is_pkgline;
	pos = 0;
	
	switch (pkg_content_id) 
	{
		case OM_LIST: //list of pkgs
		{
			v_pkg_list.clear();
			break;
		}
		
		case OM_LIST_INSTALLED: //installed pkgs
		{
			v_pkg_installed.clear();
			break;
		}
		
		case OM_LIST_UPGRADEABLE:
		{
			v_pkg_upgradable.clear();
			break;
		}
		
		default:
			printf("unknown content id! \n\t");
			break;
	}
	
	// dump output to /tmp
	sprintf(cmd, "%s > /tmp/%s.list", pkg_types[pkg_content_id].cmdstr, cmd_names[pkg_content_id]);
	
	printf("COPKGManager: executing >> %s\n", cmd);
	
	system(cmd);
	
	// check if cmd executed
	char buffer[255];
	sprintf(buffer, "/tmp/%s.list", cmd_names[pkg_content_id]);
	
	output = fopen(buffer, "r");

	if(output != NULL)
	{
		while (true)
		{
			in = fgetc(output);
			if (in == EOF)
				break;

			buf[pos] = (char)in;
			if (pos == 0)
				is_pkgline = ((in != ' ') && (in != '\t'));
			
			// avoid buffer overflow
			if (pos + 1 > OM_MAX_LINE_LENGTH)
				in = '\n';
			else
				pos++;
			buf[pos] = 0;
			
			if (in == '\b' || in == '\n')
			{
				// start a new line
				pos = 0;
				if ((in == '\n') && is_pkgline)
				{
					//clean up string
					int ipos = -1;
					std::string line = buf;
					while( (ipos = line.find('\n')) != -1 )
						line = line.erase(ipos, 1);
									
					//add to lists
					switch (pkg_content_id) 
					{
						case OM_LIST: //list of pkgs
						{
							v_pkg_list.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						case OM_LIST_INSTALLED: //installed pkgs
						{
							v_pkg_installed.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						case OM_LIST_UPGRADEABLE:
						{
							v_pkg_upgradable.push_back(line);
							//printf("%s\n", buf);
							break;
						}
						default:
							printf("unknown output! \n\t");
							printf("%s\n", buf);
							break;
					}
				}
			}
		}

		fclose(output);
	}
	else
		return false;
	
	unlink(buffer);
	
	switch (pkg_content_id) 
	{
		case OM_LIST: //list of pkgs
		{
			vp_pkg_menu = &v_pkg_list;
			break;
		}
		
		case OM_LIST_INSTALLED: //installed pkgs
		{
			vp_pkg_menu = &v_pkg_installed;
			break;
		}
		
		case OM_LIST_UPGRADEABLE:
		{
			vp_pkg_menu = &v_pkg_upgradable;
			break;
		}
		
		default:
			printf("unknown content id! \n\t");
			break;
	}
	
	return true;
}

std::string COPKGManager::getBlankPkgName(const std::string& line)
{
	int l_pos = line.find(" ");
	std::string name = line.substr(0, l_pos);
	
	return name;
}

bool COPKGManager::execCmd(const char * cmdstr)
{
	char cmd[100];

	snprintf(cmd, sizeof(cmd),"%s", cmdstr);
	
	printf("COPKGManager: executing %s\n", cmd);
	
	if(!system(cmd))
	{
		sleep(2);
		return false;
	}

	return true;
}

bool COPKGManager::installPackage(const char *filename)
{
	bool ret = false;
	
	std::string action_name = "opkg install " + getBlankPkgName(filename);
		
	if(system(action_name.c_str()))
	{
		ret = true;
	}
	
	return ret;
}

bool COPKGManager::removePackage(const char *filename)
{
	bool ret = false;
	
	std::string action_name = "opkg remove " + getBlankPkgName(filename);
		
	if( system(action_name.c_str()))
	{
		ret = true;
	}
	
	return ret;
}
		
