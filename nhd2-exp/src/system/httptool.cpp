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

#include <curl/curl.h>
#include <curl/easy.h>

#include <global.h>

#include <system/debug.h>
#include <system/httptool.h>


CHTTPTool::CHTTPTool()
{
	statusViewer = new CProgressWindow();
	
	userAgent = "neutrino/httpdownloader";
}

void CHTTPTool::setTitle(const char * const title)
{
	captionString = title;
	
	if (statusViewer)
		statusViewer->setTitle(captionString.c_str());
}

int CHTTPTool::show_progress(void * clientp, double dltotal, double dlnow, double /*ultotal*/, double /*ulnow*/)
{
	CHTTPTool * hTool = ((CHTTPTool *)clientp);
	
	if(hTool->statusViewer)
	{
		int progress = int( dlnow*100.0/dltotal);
		
		if(hTool->iGlobalProgressEnd != -1)
		{
			int globalProg = hTool->iGlobalProgressBegin + int((hTool->iGlobalProgressEnd-hTool->iGlobalProgressBegin) * progress/100. );

			hTool->statusViewer->showGlobalStatus(globalProg);
		}
	}

	return 0;
}

bool CHTTPTool::downloadFile(const std::string &URL, const char * const downloadTarget, int globalProgressEnd)
{
	CURL *curl;
	CURLcode res;
	FILE *headerfile;

	dprintf(DEBUG_INFO, "neutrino/httpdownloader: open file %s\n", downloadTarget);

	headerfile = fopen(downloadTarget, "w");
	if (!headerfile)
		return false;

	dprintf(DEBUG_INFO, "neutrino/httpdownloader: open file ok\n");
	dprintf(DEBUG_INFO, "neutrino/httpdownloader: url is %s\n", URL.c_str());

	res = (CURLcode) 1;
	curl = curl_easy_init();
	
	if(curl)
	{
		iGlobalProgressEnd = globalProgressEnd;
		
		if(statusViewer)
		{
			statusViewer->paint();
			statusViewer->showStatusMessageUTF(URL.c_str());
	
			iGlobalProgressBegin = statusViewer->getGlobalStatus();
		}
		
		curl_easy_setopt(curl, CURLOPT_URL, URL.c_str() );
		curl_easy_setopt(curl, CURLOPT_FILE, headerfile);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, show_progress);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
		//curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1800);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

		if(strcmp(g_settings.softupdate_proxyserver, "") != 0)
		{
			//use proxyserver
			dprintf(DEBUG_INFO, "neutrino/httpdownloader: use proxyserver : %s\n", g_settings.softupdate_proxyserver);

			curl_easy_setopt(curl, CURLOPT_PROXY, g_settings.softupdate_proxyserver);

			if(strcmp(g_settings.softupdate_proxyusername, "") != 0)
			{
				char tmp[200];
				strcpy(tmp, g_settings.softupdate_proxyusername);
				strcat(tmp, ":");
				strcat(tmp, g_settings.softupdate_proxypassword);
				curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, tmp);
			}
		}

		dprintf(DEBUG_INFO, "neutrino/httpdownloader: going to download\n");

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	dprintf(DEBUG_DEBUG, "neutrino/httpdownloader: download code %d\n", res);

	if (headerfile)
	{
		fflush(headerfile);
		fclose(headerfile);
	}

	if(statusViewer)
	{
		statusViewer->hide();
	}	

	return res == 0;
}

