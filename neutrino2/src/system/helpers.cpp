/*
	Neutrino-HD

	License: GPL

	(C) 2012-2013 the neutrino-hd developers
	(C) 2012,2013 Stefan Seyfried

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

#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <global.h>

#include <system/debug.h>
#include <system/settings.h>
#include <system/helpers.h>
#include <system/set_threadname.h>

// zapit includes
#include <zapit/bouquets.h>

#include <wordexp.h>

#include <cstring>
#include <fstream>
#include <string>
#include <iostream>


using namespace std;

extern tallchans allchans;	// defined in bouquets.h

off_t file_size(const char *filename)
{
	struct stat stat_buf;
	if(::stat(filename, &stat_buf) == 0)
	{
		return stat_buf.st_size;
	} 
	else
	{
		return 0;
	}
}

bool file_exists(const char *filename)
{
	struct stat stat_buf;
	if(::stat(filename, &stat_buf) == 0)
	{
		return true;
	} 
	else
	{
		return false;
	}
}

void  wakeup_hdd(const char *hdd_dir)
{
	if(!check_dir(hdd_dir))
	{
		std::string wakeup_file = hdd_dir;
		wakeup_file += "/.wakeup";
		remove(wakeup_file.c_str());
		creat(wakeup_file.c_str(),S_IREAD|S_IWRITE);
		sync();
	}
}

//use for script with full path
int my_system(const char * cmd)
{
	if (!file_exists(cmd))
		return -1;
	
	dprintf(DEBUG_NORMAL, "my_system: execute:%s\n", cmd);

	return my_system(1, cmd);
}

int my_system(int argc, const char *arg, ...)
{
	int i = 0, ret = 0, childExit = 0;
#define ARGV_MAX 64
	/* static right now but could be made dynamic if necessary */
	int argv_max = ARGV_MAX;
	const char *argv[ARGV_MAX];
	va_list args;
	argv[0] = arg;
	va_start(args, arg);

	while(++i < argc)
	{
		if (i == argv_max)
		{
			fprintf(stderr, "my_system: too many arguments!\n");
			return -1;
		}
		argv[i] = va_arg(args, const char *);
	}
	argv[i] = NULL;

	pid_t parent_pid = getpgrp();
	pid_t pid;
	int maxfd = getdtablesize();// sysconf(_SC_OPEN_MAX);
	switch (pid = vfork())
	{
		case -1: /* can't vfork */
			perror("vfork");
			ret = -errno;
			break;
		case 0: /* child process */
			for(i = 3; i < maxfd; i++)
				close(i);
#if 0
			if (setsid() == -1)
				perror("my_system setsid");
#endif
			setpgid(getpid(), parent_pid);
			if (execvp(argv[0], (char * const *)argv))
			{
				ret = -errno;
				if (errno != ENOENT) /* don't complain if argv[0] only does not exist */
					fprintf(stderr, "ERROR: my_system \"%s\": %m\n", argv[0]);
			}
			_exit(ret); // terminate c h i l d proces s only
		default: /* parent returns to calling process */
			waitpid(pid, &childExit, 0);
			if (WEXITSTATUS(childExit) != 0)
				ret = (signed char)WEXITSTATUS(childExit);
			break;
	}
	va_end(args);
	return ret;
}

FILE* my_popen( pid_t& pid, const char *cmdstring, const char *type)
{
	int     pfd[2] ={-1,-1};
	FILE    *fp = NULL;

	/* only allow "r" or "w" */
	if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
		errno = EINVAL;     /* required by POSIX */
		return(NULL);
	}

	if (pipe(pfd) < 0)
		return(NULL);   /* errno set by pipe() */

	if ((pid = vfork()) < 0) {
		return(NULL);   /* errno set by vfork() */
	} else if (pid == 0) {                           /* child */
		if (*type == 'r') {
			close(pfd[0]);
			if (pfd[1] != STDOUT_FILENO) {
				dup2(pfd[1], STDOUT_FILENO);
				close(pfd[1]);
			}
		} else {
			close(pfd[1]);
			if (pfd[0] != STDIN_FILENO) {
				dup2(pfd[0], STDIN_FILENO);
				close(pfd[0]);
			}
		}
		int maxfd = getdtablesize();
		for(int i = 3; i < maxfd; i++)
			close(i);

		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
		exit(0);
	 }

	/* parent continues... */
	if (*type == 'r') {
		close(pfd[1]);
		if ((fp = fdopen(pfd[0], type)) == NULL)
			return(NULL);
		} else {
			close(pfd[0]);
		if ((fp = fdopen(pfd[1], type)) == NULL)
			return(NULL);
	}
	return(fp);
}

int safe_mkdir(const char * path)
{
	struct statfs s;
	size_t l = strlen(path);
	char d[l + 3];
	strncpy(d, path, l);

	// skip trailing slashes
	while (l > 0 && d[l - 1] == '/')
		l--;
	// find last slash
	while (l > 0 && d[l - 1] != '/')
		l--;
	if (!l)
		return -1;
	// append a single dot
	d[l++] = '.';
	d[l] = 0;

	if(statfs(d, &s) || (s.f_type == 0x72b6 /* JFFS2 */) || (s.f_type == 0x5941ff53 /* YAFFS2 */))
		return -1;
	return mkdir(path, 0755);
}

/* function used to check is this dir writable, i.e. not flash, for record etc */
int check_dir(const char * dir, bool allow_tmp)
{
	/* default to return, if statfs fail */
	int ret = -1;
	struct statfs s;
	if (::statfs(dir, &s) == 0) {
		switch (s.f_type)	/* f_type is long */
		{
			case 0xEF53L:		/*EXT2 & EXT3*/
			case 0x6969L:		/*NFS*/
			case 0xFF534D42L:	/*CIFS*/
			case 0x517BL:		/*SMB*/
			case 0x52654973L:	/*REISERFS*/
			case 0x65735546L:	/*fuse for ntfs*/
			case 0x58465342L:	/*xfs*/
			case 0x4d44L:		/*msdos*/
			case 0x0187:		/* AUTOFS_SUPER_MAGIC */
			case 0x3153464aL:	/*jfs*/
			case 0x4006L:		/*fat*/
#if 0
			case 0x72b6L:		/*jffs2*/
#endif
				ret = 0;//ok
				break; 
			case 0x858458f6L: 	/*ramfs*/
			case 0x1021994: 	/*TMPFS_MAGIC*/
				if(allow_tmp)
					ret = 0;//ok
				break;
			default:
				fprintf(stderr, "%s: Unknown File system type 0x%x\n", dir, (int)s.f_type);
				break; // error
		}
	}
	return ret;
}

bool get_fs_usage(const char * dir, uint64_t &btotal, uint64_t &bused, long *bsize/*=NULL*/)
{
	btotal = bused = 0;
	struct statfs s;

	if (::statfs(dir, &s) == 0 && s.f_blocks) 
	{
		btotal = s.f_blocks;
		bused = s.f_blocks - s.f_bfree;
		if (bsize != NULL)
			*bsize = s.f_bsize;
		//printf("fs (%s): total %llu used %llu\n", dir, btotal, bused);
		return true;
	}

	return false;
}

bool get_mem_usage(unsigned long &kbtotal, unsigned long &kbfree)
{
	unsigned long cached = 0, buffers = 0;
	kbtotal = kbfree = 0;

	FILE * f = fopen("/proc/meminfo", "r");
	if (!f)
		return false;

	char buffer[256];
	while (fgets(buffer, 255, f)) 
	{
		if (!strncmp(buffer, "Mem", 3)) 
		{
			if (!strncmp(buffer+3, "Total", 5))
				kbtotal = strtoul(buffer+9, NULL, 10);
			else if (!strncmp(buffer+3, "Free", 4))
				kbfree = strtoul(buffer+8, NULL, 10);
		}
		else if (!strncmp(buffer, "Buffers", 7)) 
		{
			buffers = strtoul(buffer+8, NULL, 10);
		}
		else if (!strncmp(buffer, "Cached", 6)) 
		{
			cached = strtoul(buffer+7, NULL, 10);
			break;
		}
	}
	fclose(f);
	kbfree = kbfree + cached + buffers;
	printf("mem: total %ld cached %ld free %ld\n", kbtotal, cached, kbfree);
	return true;
}

std::string _getPathName(std::string &path, std::string sep)
{
	size_t pos = path.find_last_of(sep);
	if (pos == std::string::npos)
		return path;

	return path.substr(0, pos);
}

std::string _getBaseName(std::string &path, std::string sep)
{
	size_t pos = path.find_last_of(sep);
	if (pos == std::string::npos)
		return path;
	if (path.length() == pos +1)
		return "";

	return path.substr(pos+1);
}

std::string getPathName(std::string &path)
{
	return _getPathName(path, "/");
}

std::string getBaseName(std::string &path)
{
	return _getBaseName(path, "/");
}

std::string getFileName(std::string &file)
{
	return _getPathName(file, ".");
}

std::string getFileExt(std::string &file)
{
	return _getBaseName(file, ".");
}


std::string getNowTimeStr(const char* format)
{
	char tmpStr[256];
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	std::string tmpString;
	
	if ( strcmp(format, "%A %d.%m.%Y") == 0 )
	{
		strftime(tmpStr, sizeof(tmpStr), "%A", localtime(&tv.tv_sec));
		tmpString = _(tmpStr);
		
		strftime(tmpStr, sizeof(tmpStr), " %d.%m.%Y", localtime(&tv.tv_sec));
		tmpString += _(tmpStr);
	}
	else if ( strcmp(format, "%A %d.%m.%Y %H:%M") == 0 )
	{
		strftime(tmpStr, sizeof(tmpStr), "%A", localtime(&tv.tv_sec));
		tmpString = _(tmpStr);
		
		strftime(tmpStr, sizeof(tmpStr), " %d.%m.%Y %H:%M", localtime(&tv.tv_sec));
		tmpString += _(tmpStr);
	}
	else if ( strcmp(format, "%A %d.%m.%Y %H:%M:%S") == 0 )
	{
		strftime(tmpStr, sizeof(tmpStr), "%A", localtime(&tv.tv_sec));
		tmpString = _(tmpStr);
		
		strftime(tmpStr, sizeof(tmpStr), " %d.%m.%Y %H:%M:%S", localtime(&tv.tv_sec));
		tmpString += _(tmpStr);
	}
	else
	{       
		strftime(tmpStr, sizeof(tmpStr), format, localtime(&tv.tv_sec));
		tmpString = _(tmpStr);
	}
	
	return tmpString;
}

std::string trim(std::string &str, const std::string &trimChars /*= " \n\r\t"*/)
{
	std::string result = str.erase(str.find_last_not_of(trimChars) + 1);
	return result.erase(0, result.find_first_not_of(trimChars));
}

std::string replace_all(const std::string &in, const std::string &entity, const std::string &symbol)
{
	std::string out = in;
	std::string::size_type loc = 0;
	while (( loc = out.find(entity, loc)) != std::string::npos )
	out.replace(loc, entity.length(), symbol);
	
	return out;
}

void strReplace(std::string & orig, const char *fstr, const std::string rstr)
{
	//replace all occurrence of fstr with rstr and, and returns a reference to itself
	size_t index = 0;
	size_t fstrlen = strlen(fstr);
	size_t rstrlen = rstr.size();

	while ((index = orig.find(fstr, index)) != std::string::npos) 
	{
		orig.replace(index, fstrlen, rstr);
		index += rstrlen;
	}
}

std::string& htmlEntityDecode(std::string& text, bool removeTags)
{
	struct decode_table_s {
		const char* utf8Code;
		const char* htmlCode;
		const unsigned int htmlCodeLen;
	};

	decode_table_s dt[] = {
		{ " ",  		"&nbsp;",	6 },
		{ "&",  		"&amp;",	5 },
		{ "&",  		";amp;",	5 },
		{ "<",  		"&lt;",	4 },
		{ ">",  		"&gt;",	4 },
		{ "\"", 		"&quot;",	6 },
		{ "'",  		"&apos;",	6 },
		{ "€",  		"&euro;",	6 },
		{ "\xe2\x80\xa6",	"&hellip;",	8 },
		{ NULL,		NULL,	0 }
	};

	char t[text.length() + 1];
	char *u = t;
	const char *p = text.c_str();
	while (*p) 
	{
		if (removeTags && *p == '<') 
		{
			while (*p && *p != '>')
				p++;
			if (*p)
				p++;
			continue;
		}

		if (p[0] == '&') 
		{
			if (p[1] == '#') 
			{
				p += 2;
				const char *format;
				if (p[0] == 'x' || p[0] == 'X') 
				{
					format = "%x";
					p++;
				} 
				else
					format = "%d";

				unsigned int r = 0;
				if (1 == sscanf(p, format, &r)) 
				{
					if (r < 0x80) 
					{
						u[0] = r & 0x7f;
						u += 1;
					} 
					else if (r < 0x800) 
					{
						u[1] = 0x80 & (r & 0x3f);
						r >>= 6;
						u[0] = 0xC0 & r;
						u += 2;
					} 
					else if (r < 0x10000) 
					{
						u[2] = 0x80 | (0x3f & r);
						r >>= 6;
						u[1] = 0x80 | (0x3f & r);
						r >>= 6;
						u[0] = 0xE0 | r;
						u += 3;
					} 
					else if (r < 0x110000) 
					{
						u[3] = 0x80 | (0x3f & r);
						r >>= 6;
						u[2] = 0x80 | (0x3f & r);
						r >>= 6;
						u[1] = 0x80 | (0x3f & r);
						r >>= 6;
						u[0] = 0xF0 | r;
						u += 4;
					}
				}
				while(*p && *p != ';')
					p++;
				if (*p)
					p++;
				continue;
			}

			decode_table_s *d = dt;
			while (d->utf8Code && strncmp(p, d->htmlCode, d->htmlCodeLen))
				d++;
			if (d->utf8Code) 
			{
				p += d->htmlCodeLen;
				const char *v = d->utf8Code;
				while (*v)
					*u++ = *v++;
				continue;
			}
		}

		*u++ = *p++;
	}
	
	*u = 0;
	text = std::string(t);
	
	return text;
}	

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}

#if __cplusplus < 201103L
std::string to_string(int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(long long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(unsigned long long i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

std::string to_string(float i)
{
	std::stringstream s;
	s << i;
	return s.str();
}
#endif

std::string to_hexstring(unsigned long long i)
{
	std::stringstream s;
	s.setf ( std::ios::hex, std::ios::basefield );
	s << i;
	return s.str();
}

std::string changeFileNameExt(std::string &filename, const char *ext)
{
	int ext_pos = 0;
	ext_pos = filename.rfind('.');
	if( ext_pos > 0)
	{
		std::string extension;
		extension = filename.substr(ext_pos + 1, filename.length() - ext_pos);
		extension = "." + extension;
		strReplace(filename, extension.c_str(), ext);
	}
	
	return filename;
}

//
std::string Lang2I18N(std::string lang)
{
	std::string ret = "C";
	
	if (lang == "de")
		ret = "de_DE";
	else if (lang == "en")
		ret = "C";
	else if (lang == "nl")
		ret = "nl_NL";
	else if (lang == "sk")
		ret = "sk_SK";
	else if (lang == "bs")
		ret = "bs_BS";
	else if (lang == "cs")
		ret = "cs_CS";
	else if (lang == "fr")
		ret = "fr_FR";
	else if (lang == "it")
		ret = "it_IT";
	else if (lang == "pl")
		ret = "pl_PL";
	else if (lang == "pt")
		ret = "pt_PT";
	else if (lang == "ru")
		ret = "ru_RU";
	else if (lang == "sv")
		ret = "sv_SV";
	else if (lang == "ar")
		ret = "ar_AE";

	return ret;
}

//
std::string locale2lang(std::string lang)
{
	std::string ret = "english";
	
	if (lang == "de") 
		ret = _("German");
	else if (lang == "en")
		ret = _("English");
	else if (lang == "nl")
		ret = _("Dutch");
	else if (lang == "sk")
		ret = _("Slovak");
	else if (lang == "bs")
		ret = _("Bosnian");
	else if (lang == "cs")
		ret = _("Czech");
	else if (lang == "fr")
		ret = _("French");
	else if (lang == "it")
		ret = _("italian");
	else if (lang == "pl")
		ret = _("Polish");
	else if (lang == "pt")
		ret = _("Portugues");
	else if (lang == "ru")
		ret = _("Russian");
	else if (lang == "sv")
		ret = _("Svenska");
	else if (lang == "ar")
		ret = _("Arabic");

	return ret;
}

void splitString(std::string &str, std::string delim, std::vector<std::string> &strlist, int start)
{
	strlist.clear();
	int end = 0;
	
	while ((end = str.find(delim, start)) != std::string::npos) 
	{
		strlist.push_back(str.substr(start, end - start));
		start = end + delim.size();
	}

	strlist.push_back(str.substr(start));
}

void splitString(std::string &str, std::string delim, std::map<std::string,std::string> &strmap, int start)
{
	int end = 0;
	if ((end = str.find(delim, start)) != std::string::npos) 
	{
		strmap[str.substr(start, end - start)] = str.substr(end - start + delim.size());
	}
}

std::string removeExtension(std::string& s)
{
	int ext_pos = 0;
	ext_pos = s.rfind('.');
	
	if( ext_pos > 0)
	{
		std::string extension;
		extension = s.substr(ext_pos + 1, s.length() - ext_pos);

		s = s.substr(0, s.length() - (extension.length() + 1));
	}

	return s;
}

// curl
static void *myrealloc(void *ptr, size_t size)
{
	if(ptr)
		return realloc(ptr, size);
	else
		return malloc(size);
}

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) 
	{
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data)
{
        std::string* pStr = (std::string*) data;
        pStr->append((char*) ptr, nmemb);
	
        return size*nmemb;
}

bool getUrl(std::string& url, std::string& answer, std::string userAgent, unsigned int timeout)
{
	dprintf(DEBUG_INFO, "getUrl: url:%s\n", url.c_str());

	CURL * curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &CurlWriteToString);
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&answer);
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, (long)1);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, userAgent.c_str());
	
	if(strcmp(g_settings.softupdate_proxyserver, "") != 0)
	{
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, g_settings.softupdate_proxyserver);
		
		if(strcmp(g_settings.softupdate_proxyusername, "") != 0)
		{
			char tmp[200];
			strcpy(tmp, g_settings.softupdate_proxyusername);
			strcat(tmp, ":");
			strcat(tmp, g_settings.softupdate_proxypassword);
			curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, tmp);
		}
	}

	char cerror[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, cerror);
	
	CURLcode httpres = curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle);

	if (httpres != 0 || answer.empty()) 
	{
		dprintf(DEBUG_NORMAL, "getUrl: error: %s\n", cerror);
		return false;
	}
	
	return true;
}

//
std::string getUrlAnswer(std::string url, std::string userAgent, unsigned int timeout)
{
	dprintf(DEBUG_INFO, "getUrl: url:%s\n", url.c_str());

	std::string answer;
	answer.clear();

	CURL * curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &CurlWriteToString);
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&answer);
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, (long)1);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, userAgent.c_str());
	
	if(strcmp(g_settings.softupdate_proxyserver, "") != 0)
	{
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, g_settings.softupdate_proxyserver);
		
		if(strcmp(g_settings.softupdate_proxyusername, "") != 0)
		{
			char tmp[200];
			strcpy(tmp, g_settings.softupdate_proxyusername);
			strcat(tmp, ":");
			strcat(tmp, g_settings.softupdate_proxypassword);
			curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, tmp);
		}
	}

	char cerror[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, cerror);
	
	CURLcode httpres = curl_easy_perform(curl_handle);

	curl_easy_cleanup(curl_handle);

	if (httpres != 0 || answer.empty()) 
	{
		dprintf(DEBUG_NORMAL, "getUrl: error: %s\n", cerror);
	}
	
	return answer;
}

bool downloadUrl(std::string url, std::string file, std::string userAgent, unsigned int timeout)
{
	dprintf(DEBUG_INFO, "downloadUrl: url:%s file:%s userAgent:%s timeout:%d\n", url.c_str(), file.c_str(), userAgent.c_str(), timeout);

	CURL * curl_handle = curl_easy_init();

	FILE * fp = fopen(file.c_str(), "wb");
	if (fp == NULL) 
	{
		perror(file.c_str());
		return false;
	}
	
	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(curl_handle, CURLOPT_FILE, fp);
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, (long)timeout);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, (long)timeout);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, (long)1);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	
	curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");//
	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, "");//
	
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, userAgent.c_str());
	
	curl_easy_setopt(curl_handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);//

	if(strcmp(g_settings.softupdate_proxyserver, "") != 0)
	{
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, g_settings.softupdate_proxyserver);
		
		if(strcmp(g_settings.softupdate_proxyusername, "") != 0)
		{
			char tmp[200];
			strcpy(tmp, g_settings.softupdate_proxyusername);
			strcat(tmp, ":");
			strcat(tmp, g_settings.softupdate_proxypassword);
			curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, tmp);
		}
	}
	
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, (long)20);

	char cerror[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, cerror);

	CURLcode httpres = curl_easy_perform(curl_handle);

	double dsize;
	curl_easy_getinfo(curl_handle, CURLINFO_SIZE_DOWNLOAD, &dsize);
	curl_easy_cleanup(curl_handle);
	fclose(fp);

	if (httpres != 0) 
	{
		dprintf(DEBUG_NORMAL, "curl error: %s\n", cerror);
		unlink(file.c_str());
		return false;
	}

	return true;
}

std::string decodeUrl(std::string url)
{
	CURL * curl_handle = curl_easy_init();

	char * str = curl_easy_unescape(curl_handle, url.c_str(), 0, NULL);

	curl_easy_cleanup(curl_handle);

	if (str)
		url = str;
	curl_free(str);
	return url;
}

std::string encodeUrl(std::string txt)
{
	CURL * curl_handle = curl_easy_init();

	char * str = curl_easy_escape(curl_handle, txt.c_str(), txt.length());

	curl_easy_cleanup(curl_handle);

	if (str)
		txt = str;
	curl_free(str);

	return txt;
}

//
int _select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int retval;
	fd_set rset, wset, xset;
	struct timeval interval;
	timerclear(&interval);

	// make a backup of all fd_set's and timeval struct
	if (readfds) rset = *readfds;
	if (writefds) wset = *writefds;
	if (exceptfds) xset = *exceptfds;
	if (timeout) interval = *timeout;

	while (1)
	{
		retval = select(maxfd, readfds, writefds, exceptfds, timeout);

		if (retval < 0)
		{
			// restore the backup before we continue
			if (readfds) *readfds = rset;
			if (writefds) *writefds = wset;
			if (exceptfds) *exceptfds = xset;
			if (timeout) *timeout = interval;
			if (errno == EINTR) continue;
			perror("select");
			break;
		}
		break;
	}
	return retval;
}

ssize_t _writeall(int fd, const void *buf, size_t count)
{
	ssize_t retval;
	char *ptr = (char*)buf;
	ssize_t handledcount = 0;
	
	if (fd < 0) 
		return -1;
		
	while (handledcount < count)
	{
		retval = write(fd, &ptr[handledcount], count - handledcount);

		if (retval == 0) 
			return -1;
			
		if (retval < 0)
		{
			if (errno == EINTR) 
				continue;
				
			perror("write");
			return retval;
		}
		handledcount += retval;
	}
	
	return handledcount;
}

ssize_t _read(int fd, void *buf, size_t count)
{
	ssize_t retval;
	char *ptr = (char*)buf;
	ssize_t handledcount = 0;
	
	if (fd < 0) 
		return -1;
		
	while (handledcount < count)
	{
		retval = read(fd, &ptr[handledcount], count - handledcount);
		
		if (retval < 0)
		{
			if (errno == EINTR) 
				continue;
				
			perror("read");
			return retval;
		}
		handledcount += retval;
		break; /* one read only */
	}
	
	return handledcount;
}

// FielHelpers
CFileHelpers::CFileHelpers()
{
	doCopyFlag = true;
}

CFileHelpers::~CFileHelpers()
{
}

CFileHelpers* CFileHelpers::getInstance()
{
	static CFileHelpers* FileHelpers = NULL;
	if(!FileHelpers)
		FileHelpers = new CFileHelpers();
	return FileHelpers;
}

bool CFileHelpers::copyFile(const char *Src, const char *Dst, mode_t mode)
{
	dprintf(DEBUG_INFO, "CFileHelpers::copyFile: %s to %s\n", Src, Dst);
	
	int FileBufSize = 0xFFFF;
	int fd1, fd2;

	doCopyFlag = true;
	unlink(Dst);

	if ((fd1 = open(Src, O_RDONLY)) < 0)
		return false;
	
	if ((fd2 = open(Dst, O_WRONLY | O_CREAT, mode)) < 0) 
	{
		close(fd1);
		return false;
	}

	char *FileBuf		= new char[FileBufSize];
	uint32_t block;
	off64_t fsizeSrc64 = lseek64(fd1, 0, SEEK_END);
	lseek64(fd1, 0, SEEK_SET);
	
	if (fsizeSrc64 > 0x7FFFFFF0) 
	{ // > 2GB
		off64_t fsize64 = fsizeSrc64;
		block = FileBufSize;
		
		while(fsize64 > 0) 
		{
			if(fsize64 < (off64_t)FileBufSize)
				block = (uint32_t)fsize64;
			read(fd1, FileBuf, block);
			write(fd2, FileBuf, block);
			fsize64 -= block;
			if (!doCopyFlag)
				break;
		}
		
		if (doCopyFlag) 
		{
			lseek64(fd2, 0, SEEK_SET);
			off64_t fsizeDst64 = lseek64(fd2, 0, SEEK_END);
			if (fsizeSrc64 != fsizeDst64){
				close(fd1);
				close(fd2);
				delete [] FileBuf;
				return false;
			}
		}
	}
	else 
	{ // < 2GB
		off_t fsizeSrc = lseek(fd1, 0, SEEK_END);
		lseek(fd1, 0, SEEK_SET);
		off_t fsize = fsizeSrc;
		block = FileBufSize;

		while(fsize > 0) 
		{
			if(fsize < (off_t)FileBufSize)
				block = (uint32_t)fsize;
			read(fd1, FileBuf, block);
			write(fd2, FileBuf, block);
			fsize -= block;
			if (!doCopyFlag)
				break;
		}
		
		if (doCopyFlag) 
		{
			lseek(fd2, 0, SEEK_SET);
			off_t fsizeDst = lseek(fd2, 0, SEEK_END);
			if (fsizeSrc != fsizeDst)
			{
				close(fd1);
				close(fd2);
				delete [] FileBuf;
				return false;
			}
		}
	}
	close(fd1);
	close(fd2);
	delete [] FileBuf;

	if (!doCopyFlag) 
	{
		sync();
		unlink(Dst);
		return false;
	}

	return true;
}

bool CFileHelpers::copyDir(const char *Src, const char *Dst, bool backupMode)
{
	DIR *Directory;
	struct dirent *CurrentFile;
	static struct stat FileInfo;
	std::string srcPath;
	std::string dstPath;
	char buf[PATH_MAX];

	//open directory
	if ((Directory = opendir(Src)) == NULL)
		return false;
	if (lstat(Src, &FileInfo) == -1) 
	{
		closedir(Directory);
		return false;
	}
	// create directory
	// is symlink
	if (S_ISLNK(FileInfo.st_mode)) 
	{
		int len = readlink(Src, buf, sizeof(buf)-1);
		if (len != -1) 
		{
			buf[len] = '\0';
			symlink(buf, Dst);
		}
	}
	else 
	{
		// directory
		if (createDir(Dst, FileInfo.st_mode & 0x0FFF) == false) 
		{
			if (errno != EEXIST) 
			{
				closedir(Directory);
				return false;
			}
		}
	}

	// read directory
	while ((CurrentFile = readdir(Directory)) != NULL) 
	{
		// ignore '.' and '..'
		if (strcmp(CurrentFile->d_name, ".") && strcmp(CurrentFile->d_name, "..")) 
		{
			srcPath = std::string(Src) + "/" + std::string(CurrentFile->d_name);
			if (lstat(srcPath.c_str(), &FileInfo) == -1) 
			{
				closedir(Directory);
				return false;
			}
			dstPath = std::string(Dst) + "/" + std::string(CurrentFile->d_name);
			// is symlink
			if (S_ISLNK(FileInfo.st_mode)) 
			{
				int len = readlink(srcPath.c_str(), buf, sizeof(buf)-1);
				if (len != -1 && len < (int) sizeof(buf)) 
				{
					buf[len] = '\0';
					symlink(buf, dstPath.c_str());
				}
			}
			// is directory
			else if (S_ISDIR(FileInfo.st_mode)) 
			{
				copyDir(srcPath.c_str(), dstPath.c_str());
			}
			// is file
			else if (S_ISREG(FileInfo.st_mode)) 
			{
				std::string save = "";
				(void)backupMode; /* squelch unused parameter warning */
#if ENABLE_EXTUPDATE
				if (backupMode && (CExtUpdate::getInstance()->isBlacklistEntry(srcPath)))
					save = ".save";
#endif
				copyFile(srcPath.c_str(), (dstPath + save).c_str(), FileInfo.st_mode & 0x0FFF);
			}
		}
	}
	closedir(Directory);
	return true;
}

bool CFileHelpers::createDir(const char *Dir, mode_t mode)
{
	char dirPath[strlen(Dir) + 1];
	DIR *dir;

	if ((dir = opendir(Dir)) != NULL) 
	{
		closedir(dir);
		errno = EEXIST;
		return false;
	}

	int ret = -1;
	while (ret == -1) 
	{
		strcpy(dirPath, Dir);
		ret = mkdir(dirPath, mode);

		if ((errno == ENOENT) && (ret == -1)) 
		{
			char * pos = strrchr(dirPath,'/');
			if (pos != NULL) {
				pos[0] = '\0';
				createDir(dirPath, mode);
			}
		}
		else
			return !ret || (errno == EEXIST);
	}
	errno = 0;
	return true;
}

bool CFileHelpers::removeDir(const char *Dir)
{
	dprintf(DEBUG_INFO, "CFileHelpers::removeDir:\n");
	
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];

	dir = opendir(Dir);
	if (dir == NULL) 
	{
		printf("Error opendir()\n");
		return false;
	}
	
	while ((entry = readdir(dir)) != NULL) 
	{
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) 
		{
			snprintf(path, (size_t) PATH_MAX, "%s/%s", Dir, entry->d_name);
			if (entry->d_type == DT_DIR)
				removeDir(path);
			else
				unlink(path);
		}
	}
	closedir(dir);
	rmdir(Dir);

	errno = 0;
	return true;
}

bool CFileHelpers::readDir(const std::string& dirname, CFileList* flist, CFileFilter * fileFilter, bool skipDirs)
{
	struct stat64 statbuf;
	dirent64 **namelist;
	int n;
	std::string dir = dirname + "/";

	dprintf(DEBUG_INFO, "CFileHelpers::readDir %s\n", dir.c_str());

	n = scandir64(dir.c_str(), &namelist, 0, alphasort64);
	if (n < 0)
	{
		perror(("CFileHelpers scandir: " + dir).c_str());
		return false;
	}
	
	for(int i = 0; i < n; i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name, ".") != 0)
		{
			// name
			file.Name = dir + namelist[i]->d_name;

			// stat
			if(stat64((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;

				// skip not wanted
				if(skipDirs)
				{
					// skip dirs
					if(S_ISDIR(file.Mode))
					{
						continue;
					}

					// skip not matched filter
					if(fileFilter != NULL )
					{
						if(!fileFilter->matchFilter(file.Name))
						{
							continue;
						}
					}
				}
				
				flist->push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return true;
}

//
CFileList CFileHelpers::readDir(const std::string& dirname, CFileFilter * fileFilter, bool skipDirs)
{
	CFileList flist;

	struct stat64 statbuf;
	dirent64 **namelist;
	int n;
	std::string dir = dirname + "/";

	dprintf(DEBUG_INFO, "CFileHelpers::readDir %s\n", dir.c_str());

	n = scandir64(dir.c_str(), &namelist, 0, alphasort64);
	if (n < 0)
	{
		perror(("CFileHelpers scandir: " + dir).c_str());
		//return false;
	}
	
	for(int i = 0; i < n; i++)
	{
		CFile file;
		if(strcmp(namelist[i]->d_name, ".") != 0)
		{
			// name
			file.Name = dir + namelist[i]->d_name;

			// stat
			if(stat64((file.Name).c_str(),&statbuf) != 0)
				perror("stat error");
			else
			{
				file.Mode = statbuf.st_mode;
				file.Size = statbuf.st_size;
				file.Time = statbuf.st_mtime;

				// skip not wanted
				if(skipDirs)
				{
					// skip dirs
					if(S_ISDIR(file.Mode))
					{
						continue;
					}

					// skip not matched filter
					if(fileFilter != NULL )
					{
						if(!fileFilter->matchFilter(file.Name))
						{
							continue;
						}
					}
				}
				
				flist.push_back(file);
			}
		}
		free(namelist[i]);
	}

	free(namelist);

	return flist;
}

void CFileHelpers::addRecursiveDir(CFileList * re_filelist, std::string rpath, CFileFilter * fileFilter)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int n;
	int dir_count = 0;

	dprintf(DEBUG_INFO, "CFileHelpers::addRecursiveDir %s\n", rpath.c_str());

	if (dir_count > 10)
	{
		return; // do not go deeper than 10 directories
	}

	bool bCancel = false;

	g_RCInput->getMsg_us(&msg, &data, 1);
	if (msg == RC_home)
	{
		// home key cancel scan
		bCancel = true;
	}
	else if (msg != RC_timeout)
	{
		// other event, save to low priority queue
		g_RCInput->postMsg( msg, data, false );
	}
	
	if(bCancel)
		return;

	dir_count++;

	CFileList tmplist;
	if(!readDir(rpath, &tmplist, NULL, false)) //dont skip dirs 
	{
		perror(("CFileHelpers::Recursive scandir: " + rpath).c_str());
	}
	else
	{
		n = tmplist.size();
		
		for(int i = 0; i < n; i++)
		{
			std::string basename = tmplist[i].Name.substr(tmplist[i].Name.rfind('/') + 1);
			
			if( basename != ".." )
			{
				if(fileFilter != NULL && (!S_ISDIR(tmplist[i].Mode)))
				{
					if(!fileFilter->matchFilter(tmplist[i].Name))
					{
						continue;
					}
				}

				if(!S_ISDIR(tmplist[i].Mode))
					re_filelist->push_back(tmplist[i]);
				else
					addRecursiveDir(re_filelist, tmplist[i].Name, fileFilter);
			}
		}
	}

	dir_count--;
}

std::string CFileHelpers::loadFile(CFile& file, int buffer_size)
{
	std::string buf;
	buf.clear();

	char buffer[buffer_size];

	dprintf(DEBUG_INFO, "CFileHelpers::laodFile: %s\n", file.getFileName().c_str());

	// open file
	int fd = open(file.Name.c_str(), O_RDONLY);
	if (fd == -1)		// cannot open file, return!!!!! 
	{
		dprintf(DEBUG_NORMAL, "CFileHelpers::laodFile: cannot open (%s)\r\n", file.getFileName().c_str());
		return std::string(" ");
	}
	
	// read file content to buffer 
	int bytes = read(fd, buffer, buffer_size - 1);
	if (bytes <= 0)		// cannot read file into buffer, return!!!! 
	{
		dprintf(DEBUG_NORMAL, "CFileHelpers::laodFile: cannot read (%s)\r\n", file.getFileName().c_str());
		return std::string(" ");
	}

	close(fd);
	buffer[bytes] = 0;	// terminate string

	buf = buffer;
	
	return buf;
}

bool CFileHelpers::saveFile(const CFile & file, const char *text, const int text_size)
{
	dprintf(DEBUG_INFO, "CFileHelpers::saveFile: %s\n", file.getName().c_str());

	bool result = false;
	int fd;

	if ((fd = open(file.Name.c_str(), O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0) 
	{
		int nr;
		nr = write(fd, text, text_size);
		//fdatasync(fd);
		close(fd);
		result = true;
	} 
	else 
	{
		dprintf(DEBUG_NORMAL, "CFileHelpers::saveFile: cannot open\r\n");
	}

	return (result);
}

//
bool eEnv::initialized = false;

void eEnv::initialize()
{
	static const struct {
		std::string name;
		std::string value;
	} cfgenv[] = {
		{ "prefix", "@prefix@" },
		{ "exec_prefix", "@exec_prefix@" },
		{ "bindir", "@bindir@" },
		{ "sbindir", "@sbindir@" },
		{ "libexecdir", "@libexecdir@" },
		{ "datarootdir", "@datarootdir@" },
		{ "datadir", "@datadir@" },
		{ "sysconfdir", "@sysconfdir@" },
		{ "sharedstatedir", "@sharedstatedir@" },
		{ "localstatedir", "@localstatedir@" },
		{ "libdir", "@libdir@" },
		{ "localedir", "@localedir@" },
	};
	size_t i;

	// 1st pass, as generated by configure.
	// Variables set by the user will not be overwritten.
	for (i = 0; i < (sizeof(cfgenv) / sizeof(*cfgenv)); i++) 
	{
		setenv(cfgenv[i].name.c_str(), cfgenv[i].value.c_str(), 0);
	}

	// 2nd pass: Resolve directories.
	for (i = 0; i < (sizeof(cfgenv) / sizeof(*cfgenv)); i++) 
	{
		std::string dest;
		eEnv::resolveVar(dest, "${" + cfgenv[i].name + "}");
		setenv(cfgenv[i].name.c_str(), dest.c_str(), 1);
	}
}

int eEnv::resolveVar(std::string &dest, const char *src)
{
	size_t i = 0;
	int ret;
	wordexp_t p;

	ret = wordexp(src, &p, WRDE_NOCMD | WRDE_UNDEF);

	if (ret != 0) 
	{
		switch (ret) 
		{
			case WRDE_BADCHAR:
				//eDebug("[eEnv] %s: bad character", __func__);
				break;
			case WRDE_BADVAL:
				//eDebug("[eEnv] %s: bad value", __func__);
				break;
			case WRDE_CMDSUB:
				//eDebug("[eEnv] %s: invalid command substitution", __func__);
				break;
			case WRDE_NOSPACE:
				//eDebug("[eEnv] %s: out of memory", __func__);
				break;
			case WRDE_SYNTAX:
				//eDebug("[eEnv] %s: syntax error", __func__);
				break;
			default:
				//eDebug("[eEnv] %s: unknown error", __func__);
				break;
		}

		return -1;
	}

	while (i < p.we_wordc) 
	{
		if (strchr(p.we_wordv[i], '$')) 
		{
			ret = eEnv::resolveVar(dest, p.we_wordv[i]);
			if (ret < 0)
				break;
		} 
		else 
		{
			dest.append(p.we_wordv[i]);
		}

		if (++i < p.we_wordc)
			dest.append(" ");
	}

	wordfree(&p);

	return ret;
}

int eEnv::resolveVar(std::string &dest, const std::string &src)
{
	return eEnv::resolveVar(dest, src.c_str());
}

std::string eEnv::resolve(const std::string &src)
{
	std::string dest;

	if (!initialized) 
	{
		eEnv::initialize();
		initialized = true;
	}

	eEnv::resolveVar(dest, src);

	return dest;
}

// cTimeMs
cTimeMs::cTimeMs(int Ms)
{
	Set(Ms);
}

uint64_t cTimeMs::Now(void)
{
	struct timeval t;
  	if (gettimeofday(&t, NULL) == 0)
     		return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  	return 0;
}

void cTimeMs::Set(int Ms)
{
  	begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  	return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  	return Now() - begin;
}

//
std::string readFile(std::string file)
{
	std::string ret_s;
	std::ifstream tmpData(file.c_str(), std::ifstream::binary);
	
	if (tmpData.is_open()) 
	{
		tmpData.seekg(0, tmpData.end);
		int length = tmpData.tellg();
		if (length > 0xffff) 
		{ 
			/* longer than 64k? better read in chunks! */
			cerr << __func__ << ": file " << file << " too big (" << length << " bytes)" << endl;
			return "";
		}
		
		tmpData.seekg(0, tmpData.beg);
		char* buffer = new char[length + 1];
		if (! buffer) 
		{
			cerr << __func__ << ": allocating " << (length + 1) << " bytes for buffer failed" << endl;
			return "";
		}
		tmpData.read(buffer, length);
		tmpData.close();
		buffer[length] = '\0';
		ret_s = (string)buffer;
		delete [] buffer;
	}
	else {
		cerr << "Error read " << file << endl;
		return "";
	}

	return ret_s;
}

//
std::string randomString(unsigned int length)
{
	std::string random = "";
	const char alphanum[] =
	    "0123456789"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz";
	unsigned int i;
	for(i = 0; i < length; ++i)
		random += alphanum[rand() % (sizeof(alphanum) - 1)];
	return random;
}

//
std::string randomFile(std::string suffix, std::string directory, unsigned int length)
{
	mkdir(directory.c_str(), 0755);
	return directory + "/" + randomString(length) + "." + suffix;
}

// proc utils
int proc_put(const char *path, const char *value, const int len)
{
	int ret, ret2;
	int pfd = open(path, O_WRONLY);
	if (pfd < 0)
		return pfd;
	ret = write(pfd, value, len);
	ret2 = close(pfd);
	if (ret2 < 0)
		return ret2;
	return ret;
}

int proc_put(const char *path, const char *value)
{
	return proc_put(path, value, strlen(value));
}

int proc_put(const char *path, std::string value)
{
	return proc_put(path, value.c_str());
}

int proc_put(const char *path, int value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, unsigned int value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, long value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, unsigned long value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, long long value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, unsigned long long value)
{
	return proc_put(path, to_string(value).c_str());
}

int proc_put(const char *path, bool state)
{
	return proc_put(path, state ? "1" : "0", 1);
}

int proc_get(const char *path, char *value, const int len)
{
	int ret, ret2;
	int pfd = open(path, O_RDONLY);
	if (pfd < 0)
		return pfd;
	ret = read(pfd, value, len);
	value[len-1] = '\0'; /* make sure string is terminated */
	if (ret >= 0)
	{
		while (ret > 0 && isspace(value[ret-1]))
			ret--;		/* remove trailing whitespace */
		value[ret] = '\0';	/* terminate, even if ret = 0 */
	}
	ret2 = close(pfd);
	if (ret2 < 0)
		return ret2;
	return ret;
}

unsigned int proc_get_hex(const char *path)
{
	unsigned int n, ret = 0;
	char buf[16];
	n = proc_get(path, buf, 16);
	if (n > 0)
		sscanf(buf, "%x", &ret);
	return ret;
}

// CChannellogo
CChannellogo* CChannellogo::getInstance()
{
	static CChannellogo* Channellogo = NULL;
	if(!Channellogo)
		Channellogo = new CChannellogo();
	return Channellogo;
}

// check for logo
bool CChannellogo::checkLogo(t_channel_id logo_id)
{	
        std::string logo_name;
	bool logo_ok = false;
	
	// first png, then jpg, then gif
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += to_hexstring(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			dprintf(DEBUG_DEBUG, "CChannellogo::checkLogo: found logo: %s\n", logo_name.c_str());
			break;
		}
	}
	
	return logo_ok;
}

void CChannellogo::getLogoSize(t_channel_id logo_id, int * width, int * height, int * bpp)
{
	std::string logo_name;
	bool logo_ok = false;
	
	// check for logo/convert channelid to logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += to_hexstring(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}
	
	if(logo_ok)
	{
		// get logo real size
		CFrameBuffer::getInstance()->getSize(logo_name.c_str(), width, height, bpp);
		
		dprintf(DEBUG_DEBUG, "CChannellogo::getLogoSize: logo: %s (%dx%d) %dbpp\n", logo_name.c_str(), *width, *height, *bpp);
	}
}

// display logo
bool CChannellogo::displayLogo(t_channel_id logo_id, int posx, int posy, int width, int height, bool upscale, bool center_x, bool center_y)
{	
        std::string logo_name;
	bool ret = false;
	bool logo_ok = false;
	
	int logo_w = width;
	int logo_h = height;
	int logo_bpp = 0;
	
	
	// check for logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	for (int i = 0; i < 3; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += to_hexstring(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}
	
	if(logo_ok)
	{
		// get logo real size
		CFrameBuffer::getInstance()->getSize(logo_name, &logo_w, &logo_h, &logo_bpp);
	
		// scale only PNG logos
		if( logo_name.find(".png") == (logo_name.length() - 4) )
		{
			// upscale
			if(upscale)
			{	
				//rescale logo image
				float aspect = (float)(logo_w) / (float)(logo_h);
					
				if (((float)(logo_w) / (float)width) > ((float)(logo_h) / (float)height)) 
				{
					logo_w = width;
					logo_h = (int)(width / aspect);
				}
				else
				{
					logo_h = height;
					logo_w = (int)(height * aspect);
				}
			}
			
			ret = CFrameBuffer::getInstance()->displayImage(logo_name, center_x?posx + (width - logo_w)/2 : posx, center_y?posy + (height - logo_h)/2 : posy, logo_w, logo_h);
		}
		else
		{
			ret = CFrameBuffer::getInstance()->displayImage(logo_name, posx, posy, width, height);
		}
        }

	return ret;
}

std::string CChannellogo::getLogoName(t_channel_id logo_id)
{
	std::string logo_name = "";
	bool logo_ok = false;
	
	// check for logo/convert channelid to logo
	std::string strLogoExt[3] = { ".png", ".jpg", ".svg" };
	
	// check for logo
	for (int i = 0; i < 2; i++)
	{
		logo_name = g_settings.logos_dir;
		logo_name += "/";
		logo_name += to_hexstring(logo_id & 0xFFFFFFFFFFFFULL);
		logo_name += strLogoExt[i].c_str();

		if(!access(logo_name.c_str(), F_OK)) 
		{
			logo_ok = true;
			break;
		}
	}

	return logo_name;
}

bool CChannellogo::loadWebTVlogos()
{
	dprintf(DEBUG_NORMAL, "CChannellogo::loadWebTVlogos:\n");
	
	if (logo_running)
		return false;

	logo_running = true;
	
	return (OpenThreads::Thread::start() == 0);
}

void CChannellogo::run()
{
	set_threadname(__func__);
	
	for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
	{
		if (IS_WEBTV(it->second.getChannelID()))
		{
			// download logos
			std::string logo_name;
			logo_name = g_settings.logos_dir;
			logo_name += "/";
			logo_name += to_hexstring(it->second.getLogoID() & 0xFFFFFFFFFFFFULL);
			logo_name += ".png";
								
			if(access(logo_name.c_str(), F_OK)) 
			{
				downloadUrl(it->second.getLogoUrl(), logo_name, "", 30);
			}
		}
	}
	
	OpenThreads::Thread::join();
	logo_running = false;
}

//
void scaleImage(const std::string &tname, int *p_w, int *p_h, int dest_w, int dest_h)
{
	int nbpp = 0;

	if(!access(tname.c_str(), F_OK) )
	{
		CFrameBuffer::getInstance()->getSize(tname, p_w, p_h, &nbpp);

		// scale
		if(*p_w <= dest_w && *p_h <= dest_h)
		{
			// do not thing
		}
		else
		{
			float aspect = (float)(*p_w) / (float)(*p_h);
					
			if (((float)(*p_w) / (float)dest_w) > ((float)(*p_h) / (float)dest_h)) 
			{
				*p_w = dest_w;
				*p_h = (int)(dest_w / aspect);
			}
			else
			{
				*p_h = dest_h;
				*p_w = (int)(dest_h * aspect);
			}
		}
	}
	else
	{
		*p_w = 0;
		*p_h = 0;
	}
}

//
std::string ReadMarkerValue(std::string strLine, const char* strMarkerName)
{
	if (strLine.find(strMarkerName) != std::string::npos)
	{
		strLine = strLine.substr(strLine.find(strMarkerName));
		strLine = strLine.substr(strLine.find_first_of('"') + 1);
		strLine = strLine.substr(0,strLine.find_first_of('"'));
		return strLine;
	}

	return std::string("");
}

