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

#define UPDATE_CYCLE 15 // minutes

CWeather *weather = NULL;

CWeather *CWeather::getInstance()
{
	if (!weather)
		weather = new CWeather();
	return weather;
}

CWeather::CWeather()
{
	key = g_settings.weather_api_key;
	v_forecast.clear();
	last_time = 0;
	coords = "";
	city = "";
}

CWeather::~CWeather()
{
	v_forecast.clear();
}

void CWeather::setCoords(std::string new_coords, std::string new_city)
{
	if (coords.compare(new_coords))
	{
		coords = new_coords;
		city = new_city;
		checkUpdate(true);
	}
}

bool CWeather::checkUpdate(bool forceUpdate)
{
	time_t current_time = time(NULL);
	if (forceUpdate || (difftime(current_time, last_time) > (UPDATE_CYCLE * 60)))
		return GetWeatherDetails();
	else
		return false;
}

bool CWeather::GetWeatherDetails()
{
	dprintf(DEBUG_NORMAL, "[CWeather]: %s\n", __func__);

	last_time = time(NULL);

	std::string lat = coords.substr(0,coords.find_first_of(','));
	std::string lon = coords.substr(coords.find_first_of(',')+1);

	std::string data = "https://api.openweathermap.org/data/2.5/onecall?lat=" + lat + "&lon=" + lon + "&units=metric&lang=de&exclude=minutely,hourly,flags,alerts&appid=" + key;

	std::string answer;
	std::string formattedErrors;

	double found = 0;

	v_forecast.clear();

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
		printf("Failed to parse JSON\n");
		printf("%s\n", formattedErrors.c_str());
		return false;
	}

	found = DataValues["current"].get("dt", 0).asDouble();

	printf("[CWeather]: results found: %lf\n", found);

	if (found > 0)
	{
		timezone = DataValues["timezone"].asString();
		current.timestamp = DataValues["current"].get("dt", 0).asDouble();
		current.temperature = DataValues["current"].get("temp", "").asFloat();
		current.pressure = DataValues["current"].get("pressure", "").asFloat();
		current.humidity = DataValues["current"].get("humidity", "").asFloat();
		current.windSpeed = DataValues["current"].get("wind_speed", "").asFloat();
		current.windBearing = DataValues["current"].get("wind_deg", "").asDouble();
		current.icon = DataValues["current"]["weather"][0].get("icon", "").asString();
		if (current.icon.empty())
			current.icon = "unknown.png";
		else
			current.icon = current.icon + ".png";
		printf("[CWeather]: temp in %s (%s): %.1f - %s\n", city.c_str(), timezone.c_str(), current.temperature, current.icon.c_str());

		forecast_data daily_data;
		Json::Value elements = DataValues["daily"];
		for (unsigned int i = 0; i < elements.size(); i++)
		{
			daily_data.timestamp = elements[i].get("dt", 0).asDouble();
			daily_data.weekday = (int)(localtime(&daily_data.timestamp)->tm_wday);
			daily_data.icon = elements[i]["weather"][0].get("icon", "").asString();
			if (daily_data.icon.empty())
				daily_data.icon = "unknown.png";
			else
				daily_data.icon = daily_data.icon + ".png";
			daily_data.temperatureMin = elements[i]["temp"].get("min", "").asFloat();
			daily_data.temperatureMax = elements[i]["temp"].get("max", "").asFloat();
			daily_data.sunriseTime = elements[i].get("sunrise", 0).asDouble();
			daily_data.sunsetTime = elements[i].get("sunset", 0).asDouble();
			daily_data.windSpeed = elements[i].get("wind_speed", 0).asFloat();
			daily_data.windBearing = elements[i].get("wind_deg", 0).asDouble();

			struct tm *timeinfo;
			timeinfo = localtime(&daily_data.timestamp);

			dprintf(DEBUG_NORMAL, "[CWeather]: temp %d.%d.%d: min %.1f - max %.1f -> %s\n", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, daily_data.temperatureMin, daily_data.temperatureMax, daily_data.icon.c_str());

			v_forecast.push_back(daily_data);
		}

		return true;
	}

	return false;
}

