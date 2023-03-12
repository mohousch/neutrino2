
#ifndef __system_helpers__
#define __system_helpers__

/*
	Neutrino-HD

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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <sstream>

#include <curl/curl.h>
#include <curl/easy.h>

#include <vector>
#include <string>
#include <map>

#include <driver/file.h>
#include <driver/rcinput.h>
#include <driver/framebuffer.h>

#include <gui/widget/icons.h>

#include <system/settings.h>

// zapit types
#include <zapit/zapittypes.h>
#include <zapit/getservices.h>

#include <OpenThreads/Thread>


int my_system(const char * cmd);
int my_system(int argc, const char *arg, ...); /* argc is number of arguments including command */

FILE* my_popen( pid_t& pid, const char *cmdstring, const char *type);

int safe_mkdir(const char * path);
inline int safe_mkdir(std::string path) { return safe_mkdir(path.c_str()); }
off_t file_size(const char *filename);
bool file_exists(const char *filename);
void wakeup_hdd(const char *hdd_dir);
int check_dir(const char * dir, bool allow_tmp = false);
bool get_fs_usage(const char * dir, uint64_t &btotal, uint64_t &bused, long *bsize=NULL);
bool get_mem_usage(unsigned long &total, unsigned long &free);

//
std::string getPathName(std::string &path);
std::string getBaseName(std::string &path);
std::string getFileName(std::string &file);
std::string getFileExt(std::string &file);
std::string getNowTimeStr(const char* format);
std::string trim(std::string &str, const std::string &trimChars = " \n\r\t");
std::string replace_all(const std::string &in, const std::string &entity, const std::string &symbol);

//
unsigned long long getcurrenttime();
void strReplace(std::string & orig, const char *fstr, const std::string rstr);
std::string& htmlEntityDecode(std::string& text, bool removeTags = false);

#if __cplusplus < 201103L
std::string to_string(int);
std::string to_string(unsigned int);
std::string to_string(long);
std::string to_string(unsigned long);
std::string to_string(long long);
std::string to_string(unsigned long long);
std::string to_string(float);
#else
/* hack... */
#define to_string(x) std::to_string(x)
#endif

std::string to_hexstring(unsigned long long);

//std::string Lang2ISO639_1(std::string& lang);
std::string Lang2I18N(std::string lang);
std::string locale2lang(std::string lang);

inline int atoi(std::string &s) { return atoi(s.c_str()); }
inline int atoi(const std::string &s) { return atoi(s.c_str()); }
inline int access(std::string &s, int mode) { return access(s.c_str(), mode); }
inline int access(const std::string &s, int mode) { return access(s.c_str(), mode); }

inline void cstrncpy(char *dest, const char * const src, size_t n) { n--; strncpy(dest, src, n); dest[n] = 0; }
inline void cstrncpy(char *dest, const std::string &src, size_t n) { n--; strncpy(dest, src.c_str(), n); dest[n] = 0; }

std::string changeFileNameExt(std::string &filename, const char *ext);

void splitString(std::string &str, std::string delim, std::vector<std::string> &strlist, int start = 0);
void splitString(std::string &str, std::string delim, std::map<std::string,std::string> &strmap, int start = 0);

//
std::string removeExtension(std::string& s);

// curl
struct MemoryStruct {
	char *memory;
	size_t size;
};

size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data);
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);

std::string encodeUrl(std::string txt);
std::string decodeUrl(std::string url);

bool getUrl(std::string &url, std::string &answer, std::string userAgent = "", unsigned int timeout = 10);
bool downloadUrl(std::string url, std::string file, std::string userAgent = "", unsigned int timeout = 10);
std::string getUrlAnswer(std::string url, std::string userAgent = "", unsigned int timeout = 10);

//
int _select(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
ssize_t _writeall(int fd, const void *buf, size_t count);
ssize_t _read(int fd, void *buf, size_t count);

//
std::string randomString(unsigned int length = 10);
std::string randomFile(std::string suffix = "tmp", std::string directory = "/tmp", unsigned int length = 10);

//
class RandomNumber
{
	public:
		RandomNumber()
		{
			srand(time(0));
		}

		int operator()(int n)
		{
			return ((long long)n * rand() / RAND_MAX);
		}
};

//
class CFileHelpers
{
	public:
		CFileHelpers();
		~CFileHelpers();
		static CFileHelpers* getInstance();
		bool doCopyFlag;

		bool copyDir(const char *Src, const char *Dst, bool backupMode = false);
		bool createDir(const char *Dir, mode_t mode = 0755);
		bool removeDir(const char *Dir);
		bool readDir(const std::string& dirname, CFileList* flist, CFileFilter* fileFilter = NULL, bool skipDirs = true);
		CFileList readDir(const std::string& dirname, CFileFilter* fileFilter = NULL, bool skipDirs = true);
		void addRecursiveDir(CFileList * re_filelist, std::string rpath, CFileFilter * fileFilter = NULL);

		//
		bool copyFile(const char *Src, const char *Dst, mode_t mode = 00664);
		std::string loadFile(CFile & file, int buffer_size);
		bool saveFile(const CFile& file, const char *text, const int text_size);
};

//
class eEnv {
	private:
		static bool initialized;
		static void initialize();
		static int resolveVar(std::string &dest, const char *src);
		static int resolveVar(std::string &dest, const std::string &src);
	public:
		static std::string resolve(const std::string &path);
};

class cTimeMs {
	private:
		uint64_t begin;

	public:
		cTimeMs(int Ms = 0);
		static uint64_t Now(void);
		void Set(int Ms = 0);
		bool TimedOut(void);
		uint64_t Elapsed(void);
};

//
std::string readFile(std::string file);

// proc utils
int proc_put(const char *path, const char *value, const int len);
int proc_put(const char *path, const char *value);
int proc_put(const char *path, std::string value);
int proc_put(const char *path, int value);
int proc_put(const char *path, unsigned int value);
int proc_put(const char *path, long value);
int proc_put(const char *path, unsigned long value);
int proc_put(const char *path, long long value);
int proc_put(const char *path, unsigned long long value);
int proc_put(const char *path, bool state);
int proc_get(const char *path, char *value, const int len);
unsigned int proc_get_hex(const char *path);

// channel logo helper class
class CChannellogo : public OpenThreads::Thread
{
	public:
		CChannellogo(){logo_running = false;};
		~CChannellogo(){};
		
		static CChannellogo* getInstance();
		
		bool displayLogo(t_channel_id logo_id, int posx, int posy, int width, int height, bool upscale = false, bool center_x = true, bool center_y = true);
		bool checkLogo(t_channel_id logo_id);
		void getLogoSize(t_channel_id logo_id, int * width, int * height, int * bpp);
		std::string getLogoName(t_channel_id logo_id);
		
		// webtv
		void run();
		bool loadWebTVlogos();
		bool logo_running;
};

//
void scaleImage(const std::string &tname, int *p_w, int *p_h, int dest_w = PIC_W, int dest_h = PIC_H);

//
std::string ReadMarkerValue(std::string strLine, const char* strMarkerName);

#endif
