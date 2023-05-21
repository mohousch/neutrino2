/*
	Copyright (C) 2015 TangoCash

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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>

#include <set>
#include <string>

#include <system/helpers.h>
#include <system/debug.h>

#include <jsoncpp/include/json/json.h>

#include <global.h>

#include <system/settings.h>
#include <system/helpers.h>
#include <system/tmdbparser.h>


CTmdb::CTmdb()
{
	thumbnail_dir = "/tmp/tmdbparser";

	key = "507930c8d6d400c85eae3a7e7b3f6c78";

	lang = g_settings.language;

	fileHelper.createDir(thumbnail_dir.c_str(), 0755);
}

CTmdb::~CTmdb()
{
	fileHelper.removeDir(thumbnail_dir.c_str());
}

// movie infos with name
bool CTmdb::getMovieInfo(std::string text)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getMovieInfo: %s\n", text.c_str());
	
	if (text.empty())
		return false;

	minfo.clear();

	std::string url = "http://api.themoviedb.org/3/";

	url += "search/multi?api_key=" + key + "&language=" + lang + "&query=" + encodeUrl(text);

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieInfo: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieInfo: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}

	Json::Value results = root.get("results", "");

	dprintf(DEBUG_NORMAL, "CTmdb::getMovieInfo: results: %d\n", results.size());

	if((int)results.size() <= 0)
		return false;

	tmdbinfo tmp;

	tmp.id = results[0].get("id", 0).asInt();
	tmp.media_type = results[0].get("media_type", "").asString();
	tmp.overview = results[0].get("overview", "").asString();
	tmp.poster_path = results[0].get("poster_path", "").asString();
	tmp.vote_average = results[0].get("vote_average", 0.0).asFloat();
	tmp.vote_count = results[0].get("vote_count", 0).asInt();
	tmp.popularity = results[0].get("popularity", 0.0).asFloat();
	tmp.original_language = results[0].get("original_language", "").asString();

	if (tmp.media_type == "tv") 
	{
		tmp.title = results[0].get("name", "").asString();
		tmp.original_title = results[0].get("original_name", "").asString();
		tmp.release_date = results.get("first_air_date", "").asString();
	}
	else if(tmp.media_type == "movie")
	{
		tmp.title = results[0].get("title", "").asString();
		tmp.original_title = results[0].get("original_title", "").asString();
		tmp.release_date = results[0].get("release_date", "").asString();
	}

	// credits infos
	url = "http://api.themoviedb.org/3/" + tmp.media_type + "/" + toString(tmp.id) + "?api_key=" + key + "&language=" + lang + "&append_to_response=credits";

	answer.clear();
	if (!::getUrl(url, answer, "", 90))
		return false;

	parsedSuccess = reader.parse(answer, root);
	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieInfo: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieInfo: %s\n", reader.getFormattedErrorMessages().c_str());

		return false;
	}

	results = root["genres"];
	tmp.genres = results[0].get("name", "").asString();
	for (unsigned int i = 1; i < results.size(); i++) 
	{
		tmp.genres += ", " + results[i].get("name", "").asString();
	}

	results = root["credits"]["cast"];
	for (unsigned int i = 0; i < results.size() && i < 10; i++) 
	{
		tmp.cast +=  "  " + results[i].get("character", "").asString() + " (" + results[i].get("name", "").asString() + ")\n";
	}

	if(tmp.media_type == "movie")
	{
		tmp.runtime = root.get("runtime", 0).asInt();
	}
	else if(tmp.media_type == "tv")
	{
		tmp.episodes = root.get("number_of_episodes", 0).asInt();
		tmp.seasons = root.get("number_of_seasons", 0).asInt();

		results = root["episode_run_time"];
		tmp.runtimes = results[0].asString();
		for (unsigned int i = 1; i < results.size(); i++) 
		{
			tmp.runtimes +=  + ", " + results[i].asString();
		}
	}

	minfo.push_back(tmp); 

	if(!minfo.empty())
		return true;

	return false;
}

bool CTmdb::getBigCover(std::string cover, std::string tname)
{ 
	dprintf(DEBUG_NORMAL, "CTmdb::getBigCover: %s\n", tname.c_str());

	bool ret = false;

	if (!cover.empty())
	{
		bool found = false;
		found = ::downloadUrl("http://image.tmdb.org/t/p/w342" + cover, tname);

		ret |= found;
	}

	return ret;
}

bool CTmdb::getSmallCover(std::string cover, std::string tname)
{ 
	dprintf(DEBUG_NORMAL, "CTmdb::getSmallCover: %s\n", tname.c_str());

	bool ret = false;

	if (!cover.empty())
	{
		bool found = false;

		found = ::downloadUrl("http://image.tmdb.org/t/p/w185" + cover, tname);

		ret |= found;
	}

	return ret;
}

std::string CTmdb::createInfoText()
{
	dprintf(DEBUG_NORMAL, "CTmdb::createInfoText\n");

	std::string infoText;

	infoText = "Vote: " + toString(minfo[0].vote_average) + "/10 Votecount: " + toString(minfo[0].vote_count);
	infoText += "\n\n";
	infoText += minfo[0].overview;
	infoText += "\n";

	if (minfo[0].media_type == "tv")
	{
		infoText += (std::string)_("Length") + ": " + minfo[0].runtimes;
	}
	else
	{
		infoText += (std::string)_("Length") + ": " + toString(minfo[0].runtime);
	}

	infoText += "\n";

	infoText += (std::string)_("Genre") + ": " + minfo[0].genres;
	infoText += "\n";
	infoText += (std::string)_("Original title") + " : " + minfo[0].original_title;
	infoText += "\n";
	infoText += (std::string)_("Year of production") + " : " + minfo[0].release_date.substr(0,4);
	infoText += "\n";

	if (minfo[0].media_type == "tv")
	{
		infoText += "Seasons/Episodes: " + toString(minfo[0].seasons) + "/" + toString(minfo[0].episodes);
		infoText += "\n";
	}

	if (!minfo[0].cast.empty())
		infoText += (std::string)_("Actors") + ":\n" + minfo[0].cast;

	return infoText;
}

////
// movie/tv list
bool CTmdb::getMovieTVList(std::string mtype, std::string list, int page)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getMovieTVList: %s: %s page:%d\n", mtype.c_str(), list.c_str(), page);

	std::string url	= "http://api.themoviedb.org/3/";

	url += mtype + "/" + encodeUrl(list) + "?api_key=" + key + "&language=" + lang + "&page=" + toString(page);

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieTVList: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieTVList: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}

	movieList.clear();

	Json::Value results = root.get("results", "");

	if (results.type() != Json::arrayValue)
		return false;

	for(unsigned int i = 0; i < results.size(); ++i)
	{
		tmdbinfo tmp;

		tmp.vote_count = results[i].get("vote_count", 0).asInt();
		tmp.id = results[i].get("id", 0).asInt();
		tmp.video = results[i].get("video", false).asBool();
		tmp.vote_average = results[i].get("vote_average", 0.0).asFloat();

		if(mtype == "tv")
		{
			tmp.title = results[i].get("name", "").asString();
			tmp.release_date = results[i].get("first_air_date", "").asString();
		}
		else
		{
			tmp.title = results[i].get("title", "").asString();
			tmp.release_date = results[i].get("release_date", "").asString();
		}

		tmp.popularity = results[i].get("popularity", 0.0).asFloat();
		tmp.poster_path = results[i].get("poster_path", "").asString();
		tmp.original_language = results[i].get("original_language", "").asString();
		tmp.original_title = results[i].get("original_title", "").asString();
		tmp.overview = results[i].get("overview", "").asString();
		tmp.runtime = 0;

		if (!tmp.title.empty())
			movieList.push_back(tmp);
	} 

	if(!movieList.empty())
		return true;

	return false;
}

// genre list
bool CTmdb::getGenreList(std::string mtype)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getGenreList:\n");

	std::string url	= "http://api.themoviedb.org/3/genre/";

	url += mtype + "/list?api_key=" + key + "&language=" + lang;

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getGenreList: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getGenreList: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}

	genreList.clear();

	Json::Value genres = root.get("genres", "");

	if (genres.type() != Json::arrayValue)
		return false;

	for(unsigned int i = 0; i < genres.size(); ++i)
	{
		tmdbinfo tmp;

		tmp.id = genres[i].get("id", 0).asInt();
		tmp.title = genres[i].get("name", "").asString();

		if (!tmp.title.empty())
			genreList.push_back(tmp);
	} 

	if(!genreList.empty())
		return true;

	return false;
}

// genre movie list
bool CTmdb::getGenreMovieList(const int id)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getGenreMovieList:\n");

	std::string url	= "http://api.themoviedb.org/3/list/";

	url += toString(id) + "?api_key=" + key + "&language=" + lang;

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getGenreMovieList: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getGenreMovieList: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}

	genreMovieList.clear();

	Json::Value results = root.get("items", "");

	if (results.type() != Json::arrayValue)
		return false;

	for(unsigned int i = 0; i < results.size(); ++i)
	{
		tmdbinfo tmp;

		tmp.vote_count = results[i].get("vote_count", 0).asInt();
		tmp.id = results[i].get("id", 0).asInt();
		tmp.media_type = results[i].get("media_type", "").asString();
		tmp.video = results[i].get("video", false).asBool();
		tmp.vote_average = results[i].get("vote_average", 0.0).asFloat();
		tmp.popularity = results[i].get("popularity", 0.0).asFloat();
		tmp.poster_path = results[i].get("poster_path", "").asString();
		tmp.original_language = results[i].get("original_language", "").asString();
		tmp.original_title = results[i].get("original_title", "").asString();
		tmp.overview = results[i].get("overview", "").asString();

		if(tmp.media_type == "tv")
		{
			tmp.title = results[i].get("name", "").asString();
			tmp.release_date = results[i].get("first_air_date", "").asString();
		}
		else
		{
			tmp.title = results[i].get("title", "").asString();
			tmp.release_date = results[i].get("release_date", "").asString();
		}

		if (!tmp.title.empty())
			genreMovieList.push_back(tmp);
	} 

	if(!genreMovieList.empty())
		return true;

	return false;
}

// season list
bool CTmdb::getSeasonsList(int id)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getSeasonsList: %d\n", id);

	seasonList.clear();

	std::string url = "http://api.themoviedb.org/3/tv/" + toString(id) + "?api_key=" + key + "&language=" + lang + "&append_to_response=credits";

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;

	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getSeasonsList: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getSeasonsList: %s\n", reader.getFormattedErrorMessages().c_str());

		return false;
	}


	Json::Value seasons = root.get("seasons", "");

	if (seasons.type() != Json::arrayValue)
		return false;

	for (unsigned int count = 0; count < seasons.size(); count++)
	{
		tmdbinfo tmp;

		tmp.title = seasons[count].get("name", "").asString();
		tmp.release_date = seasons[count].get("air_date", "").asString();
		tmp.episodes = seasons[count].get("episode_count", 0).asInt();
		tmp.overview = seasons[count].get("overview", "").asString();
		tmp.id = seasons[count].get("id", 0).asInt();
		tmp.poster_path = seasons[count].get("poster_path", "").asString();

		seasonList.push_back(tmp);
	}

	if(!seasonList.empty())
		return true;

	return false;
}

// episodes list
bool CTmdb::getEpisodesList(int id, int nr)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getEpisodesList: %d\n", id);

	episodeList.clear();

	std::string url = "http://api.themoviedb.org/3/tv/" + toString(id) + "/season/" + toString(nr) + "?api_key=" + key + "&language=" + lang + "&append_to_response=credits";

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;

	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getEpisodesList: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getEpisodesList: %s\n", reader.getFormattedErrorMessages().c_str());

		return false;
	}


	Json::Value seasons = root.get("episodes", "");

	if (seasons.type() != Json::arrayValue)
		return false;

	for (unsigned int count = 0; count < seasons.size(); count++)
	{
		tmdbinfo tmp;

		tmp.title = seasons[count].get("name", "").asString();
		tmp.release_date = seasons[count].get("air_date", "").asString();
		tmp.overview = seasons[count].get("overview", "").asString();
		tmp.id = seasons[count].get("id", 0).asInt();
		tmp.poster_path = seasons[count].get("still_path", "").asString();

		episodeList.push_back(tmp);
	}

	if(!episodeList.empty())
		return true;

	return false;
}

// video info with id | s_nr
bool CTmdb::getVideoInfo(std::string mtype, int id, int s_nr)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getVideoInfo: %s (id:%d) (ep_nr:%d)\n", mtype.c_str(), id, s_nr);

	videoInfo.clear();

	std::string url;

	if(mtype == "tv")
		url = "http://api.themoviedb.org/3/tv/" + toString(id) + "/season/" + toString(s_nr) + "/videos?api_key=" + key + "&language=" + lang;
	else
		url = "http://api.themoviedb.org/3/movie/" + toString(id) + "/videos?api_key=" +
 key + "&language=" + lang + "&append_to_response=credits";

	dprintf(DEBUG_NORMAL, "CTmdb::getVideoInfo: %s\n", url.c_str());

	std::string answer;

	Json::Value root;
	Json::Reader reader;

	if (!::getUrl(url, answer, "", 90))
		return false;

	bool parsedSuccess = reader.parse(answer, root);
	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getVideoInfo: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getVideoInfo: %s\n", reader.getFormattedErrorMessages().c_str());

		return false;
	}

	Json::Value results = root.get("results", "");

	if (results.type() != Json::arrayValue)
		return false;

	tmdbinfo tmp;

	tmp.vid = results[0].get("id", "").asString();
	tmp.vkey = results[0].get("key", "").asString();
	tmp.vname = results[0].get("name", "").asString();
	tmp.vtype = results[0].get("type", "").asString();

	videoInfo.push_back(tmp);

	if(!videoInfo.empty())
		return true;

	return false;
}

// movie/tv info with id
bool CTmdb::getMovieTVInfo(std::string mtype, int id)
{
	dprintf(DEBUG_NORMAL, "cTmdb::getMovieTVInfo: %d\n", id);

	movieInfo.clear();

	std::string url = "http://api.themoviedb.org/3/" + mtype + "/" + toString(id) + "?api_key=" + key + "&language=" + lang + "&append_to_response=credits";

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;

	bool parsedSuccess = reader.parse(answer, root);
	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieTVInfo: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::getMovieTVInfo: %s\n", reader.getFormattedErrorMessages().c_str());

		return false;
	}

	Json::Value elements = root.get("genres", "");

	if (elements.type() != Json::arrayValue)
		return false;

	tmdbinfo tmp;

	tmp.overview = root.get("overview", "").asString();
	tmp.poster_path = root.get("poster_path", "").asString();
	tmp.original_title = root.get("original_title", "").asString();
	tmp.release_date = root.get("release_date", "").asString();
	tmp.vote_average = root.get("vote_average", 0.0).asFloat();
	tmp.vote_count = root.get("vote_count", 0).asInt();
	tmp.runtime = root.get("runtime", 0).asInt();

	if (mtype == "tv") 
	{
		tmp.original_title = root.get("original_name", "").asString();
		tmp.episodes = root.get("number_of_episodes", 0).asInt();
		tmp.seasons = root.get("number_of_seasons", 0).asInt();
		tmp.release_date = root.get("first_air_date", "").asString();

		elements = root["episode_run_time"];
		tmp.runtimes = elements[0].asString();

		for (unsigned int i = 1; i < elements.size(); i++) 
		{
			tmp.runtimes +=  + ", " + elements[i].asString();
		}
	}

	elements = root["genres"];
	tmp.genres = elements[0].get("name", "").asString();

	for (unsigned int i = 1; i < elements.size(); i++) 
	{
		tmp.genres += ", " + elements[i].get("name", "").asString();
	}

	elements = root["credits"]["cast"];
	for (unsigned int i = 0; i < elements.size() && i < 10; i++) 
	{
		tmp.cast +=  "  " + elements[i].get("character", "").asString() + " (" + elements[i].get("name", "").asString() + ")\n";
	}

	movieInfo.push_back(tmp);

	if(!movieInfo.empty())
		return true;

	return false;
}

bool CTmdb::searchMovieInfo(std::string text)
{
	dprintf(DEBUG_NORMAL, "cTmdb::searchMovieInfo: %s\n", text.c_str());
	
	if (text.empty())
		return false;

	minfo.clear();

	std::string url = "http://api.themoviedb.org/3/";

	url += "search/multi?api_key=" + key + "&language=" + lang + "&query=" + encodeUrl(text);

	std::string answer;

	if (!::getUrl(url, answer, "", 90))
		return false;

	Json::Value root;
	Json::Reader reader;
	bool parsedSuccess = reader.parse(answer, root);

	if (!parsedSuccess) 
	{
		dprintf(DEBUG_NORMAL, "CTmdb::searchMovieInfo: Failed to parse JSON\n");
		dprintf(DEBUG_NORMAL, "CTmdb::searchMovieInfo: %s\n", reader.getFormattedErrorMessages().c_str());
		return false;
	}

	Json::Value results = root.get("results", "");

	dprintf(DEBUG_NORMAL, "CTmdb::searchMovieInfo: results: %d\n", results.size());

	for (unsigned int i = 0; i < results.size(); i++) 
	{
		tmdbinfo tmp;

		tmp.id = results[i].get("id", 0).asInt();
		tmp.media_type = results[i].get("media_type", "").asString();
		tmp.overview = results[i].get("overview", "").asString();
		tmp.poster_path = results[i].get("poster_path", "").asString();
		tmp.vote_average = results[i].get("vote_average", 0.0).asFloat();
		tmp.vote_count = results[i].get("vote_count", 0).asInt();
		tmp.popularity = results[i].get("popularity", 0.0).asFloat();
		tmp.original_language = results[i].get("original_language", "").asString();
		tmp.runtime = 0;

		if (tmp.media_type == "tv") 
		{
			tmp.title = results[i].get("name", "").asString();
			tmp.original_title = results[i].get("original_name", "").asString();
			tmp.release_date = results[i].get("first_air_date", "").asString();
		}
		else if(tmp.media_type == "movie")
		{
			tmp.title = results[i].get("title", "").asString();
			tmp.original_title = results[i].get("original_title", "").asString();
			tmp.release_date = results[i].get("release_date", "").asString();
		}

		minfo.push_back(tmp);
	} 

	if(!minfo.empty())
		return true;

	return false;
}

