//=============================================================================
// YHTTPD
// Main Class
//-----------------------------------------------------------------------------
// Cyhttpd
// Main Function and Class for Handling the Webserver-Application
// - Handles Command Line Input
// - Reads and Handles "ReadConfig" (inclusive Hooking)
// - Creates Webserver and start them listening
//=============================================================================
#ifndef __yhttpd_h__
#define __yhttpd_h__
// system
#include <signal.h>
#include <stdlib.h>

// yhttpd
#include <yconfig.h>
#include <yhttpd_core/ytypes_globals.h>
#include <yhttpd_core/ywebserver.h>

//
class Cyhttpd 
{
	private:
		CWebserver *webserver; 			// Aggregation of Webserver (now: only one)
		
		//
		pthread_t thrWebServer;
		static void * webServerThread(void *data);
		Cyhttpd();

	public:
		bool flag_threading_off; 		// switch of Connection Threading
		static CStringList ConfigList; 		// Vars & Values from ReadConfig

		// constructor & destructor
		~Cyhttpd();

		// Main Programm calls
		void run(); 				// Init Hooks, ReadConfig, Start Webserver
		bool Configure();
		void stop_webserver(); 			// Remove Hooks, Stop Webserver
		static void version(FILE *dest);	// Show Webserver Version

		// Hooks
		void hooks_attach(); 			// Add a Hook-Class to HookList
		void hooks_detach(); 			// Remove a Hook-Class from HookList
		void ReadConfig(void); 			// Read the config file for the webserver
		void ReadLanguage(void); 		// Read Language Files
		
		//
		static Cyhttpd *getInstance()
		{
			static Cyhttpd* instance = NULL;
	
			if(!instance)
				instance = new Cyhttpd;

			return instance;
		};
		void Start(void);
		void Stop(void);
};

#endif // __yhttpd_h__
