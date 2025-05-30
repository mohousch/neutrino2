/*
	Copyright (C) 2017, 2018, 2019, 2020 TangoCash

	“Powered by OpenWeather” https://openweathermap.org/api/one-call-api

	License: GPLv2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation;

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>

#include <set>
#include <string>

#include <system/set_threadname.h>

#include <global.h>
#include <jsoncpp/include/json/json.h>

#include <system/debug.h>
#include <system/weather.h>


#define UPDATE_CYCLE 	60 // minutes

CWeather *weather = NULL;

const char *icons[] = {
	"01d", "clear-day.png",
	"01n", "clear-night.png",
	"03d", "cloudy.png",
	"03n", "cloudy.png",
	"04d", "cloudy.png",
	"04n", "cloudy.png",
	"50d", "fog.png",
	"02d", "partly-cloudy-day.png",
	"02n", "partly-cloudy-night.png",
	"10d", "rain.png",
	"10n", "rain.png",
	"50d", "sleet.png",
	"13d", "snow.png",
	"11d", "wind.png",
	NULL
};

CWeather *CWeather::getInstance()
{
	if (!weather)
		weather = new CWeather();
		
	return weather;
}

CWeather::CWeather()
{
	key = g_settings.weather_api_key;
	last_time = 0;
}

CWeather::~CWeather()
{
}

bool CWeather::checkUpdate(bool forceUpdate)
{
	time_t current_time = time(NULL);
	
	if (forceUpdate || (difftime(current_time, last_time) > (UPDATE_CYCLE * 60)))
		return GetWeatherDetails();
	else
		return false;
}

bool CWeather::getMyGeoLocation()
{
	bool ret = false;
	
	std::string url = "http://ip-api.com/json/";
	
	std::string answer;
	std::string formattedErrors;

	Json::CharReaderBuilder builder;
	Json::CharReader * reader = builder.newCharReader();
	Json::Value DataValues;

	answer.clear();

	if (!getUrl(url, answer))
	{
		delete reader;
		return false;
	}
	
	bool parsedSuccess = reader->parse(answer.c_str(), answer.c_str() + answer.size(), &DataValues, &formattedErrors);
	delete reader;

	if (!parsedSuccess)
	{
		printf("Failed to parse JSON\n");
		printf("%s\n", formattedErrors.c_str());
		
		return false;
	}
	
	std::string status = DataValues["status"].asString();
	
	if (status != "success")
		return false;
		
	ret = true;

	myLocation.city = DataValues["city"].asString();
	myLocation.lat = DataValues["lat"].asDouble();
	myLocation.lon = DataValues["lon"].asDouble();
	
	dprintf(DEBUG_NORMAL, "CWeather::getMyGeoLocation: city: %s lat: %f lon: %f\n", myLocation.city.c_str(), myLocation.lat, myLocation.lon);
	
	return ret;
}

bool CWeather::GetWeatherDetails()
{
	last_time = time(NULL);
	
	std::string data = "https://api.openweathermap.org/data/2.5/weather?lat=" + toString(myLocation.lat) + "&lon=" + toString(myLocation.lon) + "&units=metric&appid=" + key;
	
	dprintf(DEBUG_NORMAL, "CWeather::GetWeatherDetails: url:%s\n", data.c_str());

	std::string answer;
	std::string formattedErrors;

	int found = 0;

	Json::CharReaderBuilder builder;
	Json::CharReader * reader = builder.newCharReader();
	Json::Value DataValues;

	answer.clear();

	if (!getUrl(data, answer))
	{
		delete reader;
		return false;
	}

	bool parsedSuccess = reader->parse(answer.c_str(), answer.c_str() + answer.size(), &DataValues, &formattedErrors);
	delete reader;

	if (!parsedSuccess)
	{
		ng_err("Failed to parse JSON\n");
		ng_err("%s\n", formattedErrors.c_str());
		
		return false;
	}

	found = DataValues["dt"].asInt();

	dprintf(DEBUG_NORMAL, "CWeather::GetWeatherDetails: results found: %d\n", found);

	if (found)
	{
		current.timestamp = DataValues["dt"].asInt();
		current.temperature = DataValues["main"].get("temp", "").asFloat();
		current.pressure = DataValues["main"].get("pressure", "").asFloat();
		current.humidity = DataValues["main"].get("humidity", "").asFloat();
		current.windSpeed = DataValues["wind"].get("speed", "").asFloat();
		current.windBearing = DataValues["wind"].get("deg", "").asDouble();
		std::string icon = DataValues["weather"][0].get("icon", "").asString();
		
		int i = 0;
		
		while (icons[i] != NULL)
		{
			if (strcmp(icon.c_str(), icons[i]) == 0)
			{
				current.icon = icons[i + 1];
			}
			i++;
		}
		
		if (current.icon.empty())
			current.icon = "unknown.png";
			
		dprintf(DEBUG_NORMAL, "CWeather::GetWeatherDetails: temp in %s %.1f (%s) (%d)\n", myLocation.city.c_str(), current.temperature, current.icon.c_str(), current.timestamp);

		return true;
	}

	return false;
}

