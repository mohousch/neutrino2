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

//#include <system/settings.h>
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
	time_t timestamp;
	int weekday; // 0=Sunday, 1=Monday, ...
	std::string icon;
	float temperatureMin;
	float temperatureMax;
	time_t sunriseTime;
	time_t sunsetTime;
	float windSpeed;
	int windBearing;
} forecast_data;

class CWeather
{
	private:
		std::string coords;
		std::string city;
		std::string timezone;
		current_data current;
		std::vector<forecast_data> v_forecast;
		std::string key;
		time_t last_time;

	public:
		static CWeather *getInstance();
		CWeather();
		~CWeather();
		bool checkUpdate(bool forceUpdate = false);
		void setCoords(std::string new_coords, std::string new_city = "Unknown");

		bool GetWeatherDetails();

		// globals
		std::string getCity()
		{
			return city;
		};

		// current conditions
#if 0
		std::string getCurrentTimestamp()
		{
			return std::to_string((int)(current.timestamp));
		};

		time_t getCurrentTimestamp()
		{
			return current.timestamp;
		};

		std::string getCurrentTemperature()
		{
			return std::to_string((int)(current.temperature + 0.5));
		};

		std::string getCurrentHumidity()
		{
			return std::to_string((int)(current.humidity * 100.0));
		};

		std::string getCurrentPressure()
		{
			return std::to_string(current.pressure);
		};

		std::string getCurrentWindSpeed()
		{
			return std::to_string(current.windSpeed);
		};

		std::string getCurrentWindBearing()
		{
			return std::to_string(current.windBearing);
		};

		std::string getCurrentIcon()
		{
			return ICONSDIR"/weather/" + current.icon;
		};

		// forecast conditions
		int getForecastSize()
		{
			return (int)v_forecast.size();
		};

		int getForecastWeekday(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return v_forecast[i].weekday;
		};

		std::string getForecastTemperatureMin(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return std::to_string((int)(v_forecast[i].temperatureMin + 0.5));
		};

		std::string getForecastTemperatureMax(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return std::to_string((int)(v_forecast[i].temperatureMax + 0.5));
		};

		std::string getForecastWindSpeed(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return std::to_string(v_forecast[i].windSpeed);
		};

		std::string getForecastWindBearing(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return std::to_string(v_forecast[i].windBearing);
		};

		std::string getForecastIcon(int i = 0)
		{
			if (i > (int)v_forecast.size())
				i = (int)v_forecast.size();
			return ICONSDIR"/weather/" + v_forecast[i].icon;
		};
#endif
};

#endif
