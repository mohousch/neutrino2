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

#ifndef __WEATHER__
#define __WEATHER__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <time.h>
#include <vector>

#include <system/helpers.h>


struct current_data
{
	time_t timestamp;
	std::string icon;
	float temperature;
	float humidity;
	float pressure;
	float windSpeed;
	int windBearing;

	current_data():
		timestamp(0),
		icon("unknown.png"),
		temperature(0),
		humidity(0),
		pressure(0),
		windSpeed(0),
		windBearing(0)
	{}
};

typedef struct
{
	std::string city;
	float lat;
	float lon;
} geoLocation;

class CWeather
{
	private:
		geoLocation myLocation;
		current_data current;
		std::string key;
		time_t last_time;

	public:
		static CWeather *getInstance();
		CWeather();
		~CWeather();
		
		bool checkUpdate(bool forceUpdate = false);
		bool GetWeatherDetails();
		bool getMyGeoLocation();

		////
		std::string getCity(){return myLocation.city;};
		std::string getCurrentTimestamp(){return toString((int)(current.timestamp));};
		std::string getCurrentTemperature(){return toString((int)(current.temperature + 0.5));};
		std::string getCurrentHumidity(){return toString((int)(current.humidity * 100.0));};
		std::string getCurrentPressure(){return toString(current.pressure);};
		std::string getCurrentWindSpeed(){return toString(current.windSpeed);};
		std::string getCurrentWindBearing(){return toString(current.windBearing);};
		std::string getCurrentIcon(){return DATADIR "/icons/" + current.icon;};
};

#endif

