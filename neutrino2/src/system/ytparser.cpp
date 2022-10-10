/*
        Copyright (C) 2013 CoolStream International Ltd

        License: GPLv2

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation;

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <set>
#include <map>
#include <vector>
#include <bitset>
#include <string>
#include <fstream>

#include <jsoncpp/include/json/json.h>

#include <system/ytparser.h>

#include <system/debug.h>
#include <system/helpers.h>
#include <system/settings.h>
#include <global.h>

//
bool parseJsonFromString(std::string& jData, Json::Value *root, std::string *errMsg)
{
	Json::CharReaderBuilder builder;
	Json::CharReader* reader(builder.newCharReader());
	std::string errs = "";
	const char* jData_c = jData.c_str();

	bool ret = reader->parse(jData_c, jData_c + strlen(jData_c), root, &errs);
	if (!ret || (!errs.empty())) {
		ret = false;
		if (errMsg != NULL)
			*errMsg = errs;
	}
	delete reader;
	return ret;
}

bool parseJsonFromFile(std::string& jFile, Json::Value *root, std::string *errMsg)
{
	std::string jData = readFile(jFile);
	bool ret = parseJsonFromString(jData, root, errMsg);
	jData.clear();
	return ret;
}

//
std::string cYTVideoInfo::GetUrl(int fmt)
{
	yt_urlmap_iterator_t it;
	
	if (fmt) 
	{
		if ((it = formats.find(fmt)) != formats.end())
			return it->second.url;
		return "";
	}
	
	if ((it = formats.find(37)) != formats.end())
		return it->second.url;
	if ((it = formats.find(22)) != formats.end())
		return it->second.url;
	if ((it = formats.find(18)) != formats.end())
		return it->second.url;
	
	return "";
}

cYTFeedParser::cYTFeedParser()
{
	thumbnail_dir = "/tmp/ytparser";
	parsed = false;
	feedmode = -1;
	tquality = "mqdefault";
	max_results = 25;

	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

cYTFeedParser::~cYTFeedParser()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

bool cYTFeedParser::parseFeedJSON(std::string &answer)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeedJSON\n");
	
	Json::Value root;
	Json::Reader reader;

	std::ostringstream ss;
	std::ifstream fh(curfeedfile.c_str(), std::ifstream::in);
	ss << fh.rdbuf();
	std::string filedata = ss.str();

	bool parsedSuccess = reader.parse(filedata, root, false);

	if(!parsedSuccess)
	{
		parsedSuccess = reader.parse(answer, root, false);
	}
	
	if(!parsedSuccess)
	{
		printf("Failed to parse JSON\n");
		printf("%s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}
	
	next.clear();
	prev.clear();
	
	next = root.get("nextPageToken", "").asString();
	prev = root.get("prevPageToken", "").asString();
  
	cYTVideoInfo vinfo;
	vinfo.description.clear();
	
	Json::Value elements = root["items"];
	
	for(unsigned int i = 0; i < elements.size(); ++i)
	{
		if(elements[i]["id"].type() == Json::objectValue) 
		{
			vinfo.id = elements[i]["id"].get("videoId", "").asString();
		}
		else if(elements[i]["id"].type() == Json::stringValue) 
		{
			vinfo.id = elements[i].get("id", "").asString();
		}
		vinfo.title = elements[i]["snippet"].get("title", "").asString();
		vinfo.description = elements[i]["snippet"].get("description", "").asString();
		vinfo.published = elements[i]["snippet"].get("publishedAt", "").asString().substr(0, 10);
		std::string thumbnail = elements[i]["snippet"]["thumbnails"]["default"].get("url", "").asString();
		// save thumbnail "default", if "high" not found
		vinfo.thumbnail = elements[i]["snippet"]["thumbnails"]["high"].get("url", thumbnail).asString();
		vinfo.author = elements[i]["snippet"].get("channelTitle", "unkown").asString();
		vinfo.category = "";
		
		
		// duration/url
		if (!vinfo.id.empty()) 
		{
			// duration
			parseFeedDetailsJSON(vinfo);
		
			// url/fill videos list
			if(ParseVideoInfo(vinfo))
				videos.push_back(vinfo);
		}
	}
	
	parsed = !videos.empty();
	
	return parsed;
}

bool cYTFeedParser::parseFeedDetailsJSON(cYTVideoInfo &vinfo)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeedDetailsJSON:\n");
	
	key = g_settings.ytkey;
	
	vinfo.duration = 0;
	// See at https://developers.google.com/youtube/v3/docs/videos
	std::string url = "https://www.googleapis.com/youtube/v3/videos?id=" + vinfo.id + "&part=contentDetails&key=" + key;

	std::string answer;
	
	if (!::getUrl(url, answer))
		return false;
  
	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root, false);
	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeedDetailsJSON: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeedDetailsJSON: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}
  
	Json::Value elements = root["items"];
	std::string duration = elements[0]["contentDetails"].get("duration", "").asString();
	
	if (duration.find("PT") != std::string::npos) 
	{
		int h = 0, m = 0, s = 0;
		if (duration.find("H") != std::string::npos) 
		{
			sscanf(duration.c_str(), "PT%dH%dM%dS", &h, &m, &s);
		}
		else if (duration.find("M") != std::string::npos) 
		{
			sscanf(duration.c_str(), "PT%dM%dS", &m, &s);
		}
		else if (duration.find("S") != std::string::npos) 
		{
			sscanf(duration.c_str(), "PT%dS", &s);
		}
		//printf(">>>>> duration: %s, h: %d, m: %d, s: %d\n", duration.c_str(), h, m, s);
		vinfo.duration = h*3600 + m*60 + s;
	}
	
	return true;
}

bool cYTFeedParser::supportedFormat(int fmt)
{
	if((fmt == 37) || (fmt == 22) || (fmt == 18))
		return true;
	
	return false;
}

////
#if 1
bool cYTFeedParser::decodeVideoInfo(std::string &answer, cYTVideoInfo &vinfo)
{
	bool ret = false;

	//FIXME check expire
	std::vector<std::string> ulist;

	// Extract player_response
	std::string::size_type player_resp_start = answer.find("player_response=");
	if (player_resp_start == std::string::npos) {
		printf("player_response not found\n");
		return false;
	}

	player_resp_start = answer.find("=", player_resp_start) + 1;
	std::string::size_type player_resp_end = answer.find("&", player_resp_start);
	if (player_resp_end == std::string::npos) {
		player_resp_end = answer.length();
	}
	std::string player_response = answer.substr(player_resp_start, player_resp_end - player_resp_start);
	::decodeUrl(player_response);

	// Load player_response as json
	Json::Value root;
	if (!parseJsonFromString(player_response, &root, NULL)) {
		printf("Decoding player_response failed\n");
		return false;
	}

	const Json::Value streamingData = root["streamingData"];
	if (!streamingData) {
		printf("streamingData element not present\n");
		return false;
	}

	const Json::Value formats = streamingData["formats"];
	if (!formats) {
		printf("formats element not present\n");
		return false;
	}

	for (Json::Value::iterator it = formats.begin(); it != formats.end(); ++it) 
	{
		const Json::Value format = *it;

		int id = format["itag"].asInt();
		std::string quality = format["quality"].asString();
		std::string url;

		if (!format["url"].empty()) {
			url = format["url"].asString();
		} else if (!format["cipher"].empty()) {
			// FIXME add support for cipher (is it still used or was it replaced by signatureCipher?)
			printf("cipher unsupported: %s\n", format["cipher"].asCString());
			continue;
		} else if (!format["signatureCipher"].empty()) {
			// FIXME add support for signatureCipher
			printf("signatureCipher unsupported: %s\n", format["signatureCipher"].asCString());
			continue;
		} else {
			printf("Unable to find url\n");
			continue;
		}
#ifdef DEBUG_PARSER
		printf("%d: %s - %s\n", id, quality.c_str(), url.c_str());
#endif

		cYTVideoUrl yurl;
		if (supportedFormat(id) && !url.empty()) {
			yurl.quality = quality;
			yurl.url = url;
			yurl.type = format["mimeType"].asString();
			vinfo.formats.insert(yt_urlmap_pair_t(id, yurl));
			ret = true;
		}
	}

	return ret;
}
#else
bool cYTFeedParser::decodeVideoInfo(std::string &answer, cYTVideoInfo &vinfo)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::decodeVideoInfo\n");
	
	bool ret = false;
	
	answer = ::decodeUrl(answer);

	if(answer.find("token=") == std::string::npos)
		return ret;
	
	vinfo.formats.clear();

	//FIXME check expire
	std::vector<std::string> ulist;
	unsigned fmt = answer.find("url_encoded_fmt_stream_map");
	
	if (fmt != std::string::npos) 
	{
		fmt = answer.find("=", fmt);
		::splitString(answer, ",", ulist, fmt + 1);
		
		for (unsigned i = 0; i < ulist.size(); i++) 
		{
			std::map<std::string,std::string> smap;
			std::vector<std::string> uparams;
			::splitString(ulist[i], "&", uparams);
			
			if (uparams.size() < 3)
				continue;
			
			for (unsigned j = 0; j < uparams.size(); j++) 
			{
				uparams[j] = ::decodeUrl(uparams[j]);

				::splitString(uparams[j], "=", smap);
			}

			// url
			cYTVideoUrl yurl;
			yurl.url = smap["url"];
			
			// sig
			std::string::size_type ptr = smap["url"].find("signature=");
			if (ptr != std::string::npos)
			{
				ptr = smap["url"].find("=", ptr);
				smap["url"].erase(0, ptr + 1);

				if((ptr = smap["url"].find("&")) != std::string::npos)
					yurl.sig = smap["url"].substr(0, ptr);
			}
			
			// quality/type
			int id = atoi(smap["itag"].c_str());
			if (supportedFormat(id) && !yurl.url.empty() && !yurl.sig.empty()) 
			{
				printf("true3\n");
				yurl.quality = smap["quality"];
				yurl.type = smap["type"];
				
				vinfo.formats.insert(yt_urlmap_pair_t(id, yurl));
				ret = true;
			}
		}
	}
	
	return ret;
}
#endif

bool cYTFeedParser::ParseVideoInfo(cYTVideoInfo &vinfo)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::ParseVideoInfo\n");
	
	bool ret = false;

	std::vector<std::string> estr;
	estr.push_back("&el=embedded");
	estr.push_back("&el=vevo");
	estr.push_back("&el=detailpage");

	for (unsigned i = 0; i < estr.size(); i++) 
	{
		std::string vurl = "https://www.youtube.com/get_video_info?";
		vurl += "video_id=";
		vurl += vinfo.id;
		vurl += estr[i];
		vurl += "&eurl=https://youtube.googleapis.com/v/";
		//vurl += ::encodeUrl("&eurl=https://youtube.com/youtubei/v1/player?key=AIzaSyAO_FJ2SlqU8Q4STEHLGCilw_Y9_11qcW8/");
		vurl += vinfo.id;
		//vurl += "&ps=default";
		vurl += "&gl=US";
		vurl += "&hl=en";
        vurl += "&asv=3";
        vurl += "&sts=1588";
		//vurl += "&html5=1";
		//vurl += "&c=TVHTML5&cver=6.20180913";
		
		std::string answer;
		if (!::getUrl(vurl, answer))
			continue;
		
		ret = decodeVideoInfo(answer, vinfo);
		
		if (ret)
			break;
	}
	
	return ret;
}

bool cYTFeedParser::ParseFeed(std::string &url)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeed(2)\n");
	
	videos.clear();

	std::string answer;

	curfeedfile = thumbnail_dir;
	curfeedfile += "/";
	curfeedfile += curfeed;
	curfeedfile += ".xml";

	if (!::getUrl(url, answer))
		return false;

	return parseFeedJSON(answer);
}

bool cYTFeedParser::ParseFeed(yt_feed_mode_t mode, std::string search, std::string vid, yt_feed_orderby_t orderby)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::parseFeed(1) mode:%d search:%s vid:%s order:%d\n", mode, search.c_str(), vid.c_str(), orderby);
	
	key = g_settings.ytkey;
	std::string answer;
	std::string url = "https://www.googleapis.com/youtube/v3/search?";
	bool append_res = true;
	
	if (mode < FEED_LAST) 
	{
		switch(mode) 
		{
			case MOST_POPULAR:
			default:
				curfeed = "&chart=mostPopular";
				break;
				
			case MOST_POPULAR_ALL_TIME:
				curfeed = "&chart=mostPopular";
				break;
		}
		
		url = "https://www.googleapis.com/youtube/v3/videos?part=snippet";
		
		if (!region.empty()) 
		{
			url += "&regionCode=";
			url += region;
		}
		url += curfeed;
	}
	else if (mode == NEXT) 
	{
		if (next.empty())
			return false;
		
		url = nextprevurl;
		url += "&pageToken=";
		url += next;
		append_res = false;
	}
	else if (mode == PREV) 
	{
		if (prev.empty())
			return false;
		
		url = nextprevurl;
		url += "&pageToken=";
		url += prev;
		append_res = false;
	}
	else if (mode == RELATED) 
	{
		if (vid.empty())
			return false;

		url = "https://www.googleapis.com/youtube/v3/search?part=snippet&relatedToVideoId=";
		url += vid;
		url += "&type=video&key=";
		url += key;
		append_res = false;
	}
	else if (mode == SEARCH) 
	{
		//if (search.empty())
		//	return false;
		
		search = encodeUrl(search);
	
		url = "https://www.googleapis.com/youtube/v3/search?q=";
		url += search;
		url += "&part=snippet";

		const char *orderby_values[] = { "date", "relevance", "viewCount", "rating", "title", "videoCount"};
		url += "&order=" + std::string(orderby_values[orderby & 3]);
	}
	else if(mode == SEARCH_BY_ID)
	{
		if(vid.empty())
			return false;

		videos.clear();
		cYTVideoInfo vinfo;

		vinfo.id = vid;

		if (!vinfo.id.empty()) 
		{
			// duration
			//parseFeedDetailsJSON(vinfo);
		
			// url/fill videos list
			if(ParseVideoInfo(vinfo))
			{
				videos.push_back(vinfo);
				return true;
			}
			else
				return false;
		}

		return false;
	}

	feedmode = mode;
	if (append_res) 
	{
		url += "&maxResults=";
		char res[10];
		sprintf(res, "%d", max_results);
		url += res;
		url += "&key=" + key;
		nextprevurl = url;
	}

	return ParseFeed(url);
}

bool cYTFeedParser::DownloadThumbnails()
{
	bool ret = false;
	
	for (unsigned i = 0; i < videos.size(); i++) 
	{
		if (!videos[i].thumbnail.empty()) 
		{
			std::string fname = thumbnail_dir;
			fname += "/";
			fname += videos[i].id;
			fname += ".jpg";
			bool found = !access(fname.c_str(), F_OK);
			if (!found)
				found = ::downloadUrl(videos[i].thumbnail, fname);
			if (found)
				videos[i].tfile = fname;
			ret |= found;
		}
	}
	
	return ret;
}

void cYTFeedParser::Cleanup(bool delete_thumbnails)
{
	dprintf(DEBUG_NORMAL, "cYTFeedParser::Cleanup: %d videos\n", (int)videos.size());
	
	if (delete_thumbnails) 
	{
		for (unsigned i = 0; i < videos.size(); i++) 
		{
			unlink(videos[i].tfile.c_str());
		}
	}
	unlink(curfeedfile.c_str());
	videos.clear();
	parsed = false;
	feedmode = -1;
}
