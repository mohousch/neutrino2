//=============================================================================
// YHTTPD
// Main Program
//=============================================================================
#include <stdio.h>

// system
#include <csignal>
#include <unistd.h>
#include <cstdlib>
#include <pwd.h>
#include <grp.h>
#include <syscall.h>

// yhttpd
#include "yconfig.h"
#include "ylanguage.h"
#include "yhook.h"

#ifdef Y_CONFIG_USE_YPARSER
#include "mod_yparser.h"
static CyParser yParser;
#endif

//
// Setting yhttpd Instance
//
#include "yhttpd.h"


//
CStringList Cyhttpd::ConfigList;

#ifdef Y_CONFIG_USE_AUTHHOOK
#include "mod_auth.h"
static CmAuth *auth = NULL;
#endif

#ifdef Y_CONFIG_USE_WEBLOG
#include "mod_weblog.h"
static CmWebLog *weblog = NULL;
#endif

#ifdef Y_CONFIG_USE_SENDFILE
#include "mod_sendfile.h"
static CmodSendfile *mod_sendfile = NULL;
#endif

#ifdef Y_CONFIG_USE_CACHE
#include "mod_cache.h"
static CmodCache mod_cache; // static instance
#endif

#if defined(CONFIG_SYSTEM_TUXBOX)
#include "neutrinoapi.h"
static CNeutrinoAPI *NeutrinoAPI;
#endif

#include <system/debug.h>

//
//
//
void yhttpd_reload_config() 
{
	Cyhttpd::getInstance()->ReadConfig();
}

//
// Main Entry
//
void thread_cleanup (void *p)
{
	Cyhttpd *y = (Cyhttpd *)p;
	
	if (y) 
	{
		y->stop_webserver();
		delete y;
	}
	
	y = NULL;
}

//
// Start
//
void Cyhttpd::Start(void)
{
	dprintf(DEBUG_DEBUG, "Cyhttpd::Start:\n");
	
	// start webserver thread
	if (pthread_create(&thrWebServer, NULL, webServerThread, (void *) this) != 0 )
	{
		dprintf(DEBUG_DEBUG, "Cyhttpd::Init: create webServerThread failed\n");
	}
}

//
// Stop
//
void Cyhttpd::Stop(void)
{
	dprintf(DEBUG_DEBUG, "Cyhttpd::Stop:\n");
}

//
//
//
void * Cyhttpd::webServerThread(void *data)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
	dprintf(DEBUG_DEBUG, "Webserver %s tid %ld\n", WEBSERVERNAME, syscall(__NR_gettid));
	
	Cyhttpd *yhttpd = (Cyhttpd *)data;

	pthread_cleanup_push(thread_cleanup, yhttpd);

	yhttpd->flag_threading_off = true;

	yhttpd->hooks_attach();
	yhttpd->ReadConfig();

	if (yhttpd->Configure()) 
	{
		// Start Webserver: fork ist if not in debug mode
		dprintf(DEBUG_DEBUG, "Webserver starting...\n");
		dprintf(DEBUG_DEBUG, "Start in Debug-Mode\n"); // non forked debugging loop

		yhttpd->run();
	}

	pthread_cleanup_pop(0);

	dprintf(DEBUG_DEBUG, "Main end\n");
	return (void *) EXIT_SUCCESS;
}

//
// Class yhttpd
//
Cyhttpd::Cyhttpd() 
{
	webserver = new CWebserver();
	flag_threading_off = false;
}

//
Cyhttpd::~Cyhttpd() 
{
	if (webserver)
	{
		delete webserver;
		webserver = NULL;
	}

	CLanguage::deleteInstance();
}

//
// Change to Root
//
bool Cyhttpd::Configure() 
{
	if (!getuid()) // you must be root to do that!
	{
		// Get user and group data
#ifdef Y_CONFIG_FEATURE_HTTPD_USER
		struct passwd *pwd = NULL;
		struct group *grp = NULL;
		std::string username = ConfigList["server.user_name"];
		std::string groupname= ConfigList["server.group_name"];

		// get user data
		if(username != "")
		{
			if((pwd = getpwnam(username.c_str())) == NULL)
			{
				dperror("Dont know user to set uid\n");
				return false;
			}
		}
		// get group data
		if(groupname != "")
		{
			if((grp = getgrnam(groupname.c_str())) == NULL)
			{
				dprintf(DEBUG_DEBUG, "Can not get Group-Information. Group: %s\n", groupname.c_str());
				return false;
			}
		}
#endif
		// change root directory
#ifdef Y_CONFIG_FEATURE_CHROOT
		if(!ConfigList["server.chroot"].empty())
		{
			dprintf(DEBUG_DEBUG, "do chroot to dir:%s\n", ConfigList["server.chroot"].c_str() );
			
			// do change Root
			if(chroot(ConfigList["server.chroot"].c_str()) == -1)
			{
				dperror("Change Root failed\n");
				return false;
			}
			// Set Working Dir
			if(chdir("/") == -1)
			{
				dperror("Change Directory to Root failed\n");
				return false;
			}
		}
#endif
#ifdef Y_CONFIG_FEATURE_HTTPD_USER
		if(username != "" && pwd != NULL && grp != NULL)
		{
			dprintf(DEBUG_DEBUG, "set user and groups\n");

			// drop root privileges
			setgid(grp->gr_gid);
			setgroups(0, NULL);
			// set user group
			if(groupname != "")
			initgroups(username.c_str(), grp->gr_gid);
			// set user
			if(setuid(pwd->pw_uid) == -1)
			{
				dperror("Change User Context failed\n");
				return false;
			}
		}
#endif
	}
	
	return true;
}

//
// Main Webserver call
//
void Cyhttpd::run() 
{
	if (webserver) 
	{
		if (flag_threading_off)
			webserver->is_threading = false;
			
		webserver->run();
	} 
	else
		dprintf(DEBUG_DEBUG, "Error initializing WebServer\n");
}

//
// Show Version Text and Number
//
void Cyhttpd::version(FILE *dest) 
{
	fprintf(dest, "%s - Webserver v%s\n", HTTPD_NAME, HTTPD_VERSION);
}

//
// Stop WebServer
//
void Cyhttpd::stop_webserver() 
{
	dprintf(DEBUG_DEBUG, "stop requested......\n");
	
	if (webserver) 
	{
		webserver->stop();
		hooks_detach();
	}
}

//
// Attach hooks (use hook order carefully)
//
void Cyhttpd::hooks_attach() 
{
#ifdef Y_CONFIG_USE_AUTHHOOK
	// First Check Authentication
	auth = new CmAuth();
	CyhookHandler::attach(auth);
#endif

#ifdef Y_CONFIG_USE_TESTHOOK
	testhook = new CTesthook();
	CyhookHandler::attach(testhook);
#endif

#if defined(CONFIG_SYSTEM_TUXBOX)
	NeutrinoAPI = new CNeutrinoAPI();
	CyhookHandler::attach(NeutrinoAPI->NeutrinoYParser);

	CyhookHandler::attach(NeutrinoAPI->ControlAPI);
#else
#ifdef Y_CONFIG_USE_YPARSER
	CyhookHandler::attach(&yParser);
#endif
#endif

#ifdef Y_CONFIG_USE_CACHE
	CyhookHandler::attach(&mod_cache);
#endif

#ifdef Y_CONFIG_USE_SENDFILE
	mod_sendfile = new CmodSendfile();
	CyhookHandler::attach(mod_sendfile);
#endif
#ifdef Y_CONFIG_USE_WEBLOG
	weblog = new CmWebLog();
	CyhookHandler::attach(weblog);
#endif
}

//
// Detach hooks & Destroy
//
void Cyhttpd::hooks_detach() 
{
#ifdef Y_CONFIG_USE_AUTHHOOK
	CyhookHandler::detach(auth);
	delete auth;
#endif

#ifdef Y_CONFIG_USE_TESTHOOK
	CyhookHandler::detach(testhook);
	delete testhook;
#endif

#if defined(CONFIG_SYSTEM_TUXBOX)
	CyhookHandler::detach(NeutrinoAPI->NeutrinoYParser);
#else
#ifdef Y_CONFIG_USE_YPARSER
	CyhookHandler::detach(&yParser);
#endif
#endif

#ifdef Y_CONFIG_USE_CACHE
	CyhookHandler::detach(&mod_cache);
#endif

#ifdef Y_CONFIG_USE_SENDFILE
	CyhookHandler::detach(mod_sendfile);
#endif

#ifdef Y_CONFIG_USE_WEBLOG
	CyhookHandler::detach(weblog);
	delete weblog;
#endif
}

//
// Read Webserver Configurationfile
// Call "Hooks_ReadConfig" so Hooks can read/write own Configuration Values
//
void Cyhttpd::ReadConfig(void) 
{
	dprintf(DEBUG_DEBUG, "ReadConfig Start\n");
	
	CConfigFile *Config = new CConfigFile(',');
	bool have_config = false;
	if (access(HTTPD_CONFIGFILE, 4) == 0)
		have_config = true;
	Config->loadConfig(HTTPD_CONFIGFILE);
	
	// convert old config files
	if (have_config) 
	{
		if (Config->getInt32("configfile.version", 0) == 0) 
		{
			CConfigFile OrgConfig = *Config;
			Config->clear();

			Config->setInt32("server.log.loglevel", OrgConfig.getInt32("LogLevel", 0));
			Config->setInt32("configfile.version", CONF_VERSION);
			Config->setString("webserver.websites", "WebsiteMain");
			Config->setBool("webserver.threading", OrgConfig.getBool("THREADS", true));
			Config->setInt32("WebsiteMain.port", OrgConfig.getInt32("Port", HTTPD_STANDARD_PORT));
			Config->setString("WebsiteMain.directory", OrgConfig.getString("PrivatDocRoot", PRIVATEDOCUMENTROOT));
			if (OrgConfig.getString("PublicDocRoot", "") != "")
				Config->setString("WebsiteMain.override_directory",OrgConfig.getString("PublicDocRoot", PRIVATEDOCUMENTROOT));
			// mod_auth
			Config->setString("mod_auth.username", OrgConfig.getString("AuthUser", AUTHUSER));
			Config->setString("mod_auth.password", OrgConfig.getString("AuthPassword", AUTHPASSWORD));
			Config->setString("mod_auth.no_auth_client", OrgConfig.getString("NoAuthClient", ""));
			Config->setString("mod_auth.authenticate", OrgConfig.getString("Authenticate", "false"));

			Config->setString("mod_sendfile.mime_types", HTTPD_SENDFILE_EXT);

			Config->saveConfig(HTTPD_CONFIGFILE);

		}
		
		// Add Defaults for Version 2
		if (Config->getInt32("configfile.version") < 2) 
		{
			Config->setString("mod_sendfile.mime_types", HTTPD_SENDFILE_EXT);
			Config->setInt32("configfile.version", CONF_VERSION);
			Config->setString("mod_sendfile.sendAll", "false");
			Config->saveConfig(HTTPD_CONFIGFILE);
		}
		
		// Add Defaults for Version 4
		if (Config->getInt32("configfile.version") < 4) 
		{
			Config->setInt32("configfile.version", CONF_VERSION);
			Config->setString("Language.selected", HTTPD_DEFAULT_LANGUAGE);
			Config->setString("Language.directory", HTTPD_LANGUAGEDIR);
			if (Config->getString("WebsiteMain.hosted_directory", "") == "")
				Config->setString("WebsiteMain.hosted_directory", "/var/hosted");
			Config->saveConfig(HTTPD_CONFIGFILE);
		}
	}

	// get variables
	webserver->init(Config->getInt32("WebsiteMain.port", HTTPD_STANDARD_PORT), Config->getBool("webserver.threading", true));
	// informational use
	ConfigList["WebsiteMain.port"] = itoa(Config->getInt32("WebsiteMain.port", HTTPD_STANDARD_PORT));
	ConfigList["webserver.threading"] = Config->getString("webserver.threading", "true");
	ConfigList["configfile.version"] = Config->getInt32("configfile.version", CONF_VERSION);
	ConfigList["server.log.loglevel"] = itoa(Config->getInt32("server.log.loglevel", 0));
	ConfigList["server.no_keep-alive_ips"] = Config->getString("server.no_keep-alive_ips", "");
	webserver->conf_no_keep_alive_ips = Config->getStringVector("server.no_keep-alive_ips");

	// MainSite
	ConfigList["WebsiteMain.directory"] = Config->getString("WebsiteMain.directory", PRIVATEDOCUMENTROOT);
	ConfigList["WebsiteMain.override_directory"] = Config->getString("WebsiteMain.override_directory", PUBLICDOCUMENTROOT);
	ConfigList["WebsiteMain.hosted_directory"] = Config->getString("WebsiteMain.hosted_directory", HOSTEDDOCUMENTROOT);

	// Check location of logos
	if (Config->getString("Tuxbox.LogosURL", "") == "") 
	{
		if (access(std::string(ConfigList["WebsiteMain.directory"] + "/logos").c_str(), 4) == 0) 
		{
			Config->setString("Tuxbox.LogosURL", ConfigList["WebsiteMain.directory"] + "/logos");
			have_config = false; //save config
		}
		else if (access(std::string(ConfigList["WebsiteMain.override_directory"] ).c_str(), 4) == 0)
		{
			Config->setString("Tuxbox.LogosURL", ConfigList["WebsiteMain.override_directory"] + "/logos");
			have_config = false; //save config
		}
	}
	ConfigList["Tuxbox.LogosURL"] = Config->getString("Tuxbox.LogosURL", "");

#ifdef Y_CONFIG_USE_OPEN_SSL
	ConfigList["SSL"] = Config->getString("WebsiteMain.ssl", "false");
	ConfigList["SSL_pemfile"] = Config->getString("WebsiteMain.ssl_pemfile", SSL_PEMFILE);
	ConfigList["SSL_CA_file"] = Config->getString("WebsiteMain.ssl_ca_file", SSL_CA_FILE);

	CySocket::SSL_pemfile = ConfigList["SSL_pemfile"];
	CySocket::SSL_CA_file = ConfigList["SSL_CA_file"];
	if(ConfigList["SSL"] == "true")
	CySocket::initSSL();
#endif
	ConfigList["server.user_name"] = Config->getString("server.user_name", "");
	ConfigList["server.group_name"] = Config->getString("server.group_name", "");
	ConfigList["server.chroot"] = Config->getString("server.chroot", "");

	// language
	ConfigList["Language.directory"] = Config->getString("Language.directory", HTTPD_LANGUAGEDIR);
	ConfigList["Language.selected"] = Config->getString("Language.selected", HTTPD_DEFAULT_LANGUAGE);
	Cyhttpd::getInstance()->ReadLanguage();

	// Read App specifig settings by Hook
	CyhookHandler::Hooks_ReadConfig(Config, ConfigList);

	// Save if new defaults are set
	//if (!have_config)
		Config->saveConfig(HTTPD_CONFIGFILE);
		
	dprintf(DEBUG_DEBUG, "ReadConfig End\n");
	
	delete Config;
	Config = NULL;
}

//
// Read Webserver Configurationfile for languages
//
void Cyhttpd::ReadLanguage(void) 
{
	// Init Class vars
	CLanguage *lang = CLanguage::getInstance();
	
	dprintf(DEBUG_DEBUG, "ReadLanguage:%s\n", ConfigList["Language.selected"].c_str());
	
	lang->setLanguage(ConfigList["Language.selected"]);
}

