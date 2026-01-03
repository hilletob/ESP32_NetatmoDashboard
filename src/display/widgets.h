#ifndef WIDGETS_H
#define WIDGETS_H

#include "TFT_eSPI.h"
#include "../data/weather_data.h"

// Weather icon renderer
void drawWeatherIcon(TFT_eSprite& display, int x, int y, const char* iconName, int size = 40);

// Widget drawing functions
void drawIndoorTempWidget(TFT_eSprite& display, const IndoorData& data);
void drawIndoorHumidWidget(TFT_eSprite& display, const IndoorData& data);
void drawIndoorPressureWidget(TFT_eSprite& display, const IndoorData& data);
void drawIndoorCO2Widget(TFT_eSprite& display, const IndoorData& data);

void drawOutdoorTempWidget(TFT_eSprite& display, const OutdoorData& data);
void drawOutdoorHumidWidget(TFT_eSprite& display, const OutdoorData& data);

// Sun widgets (separate sunrise/sunset)
void drawSunriseWidget(TFT_eSprite& display, const SunData& data);
void drawSunsetWidget(TFT_eSprite& display, const SunData& data);

void drawForecast3hWidget(TFT_eSprite& display, const ForecastPoint& forecast);
void drawForecast6hWidget(TFT_eSprite& display, const ForecastPoint& forecast);

// New 3-day forecast column
void draw3DayForecastColumn(TFT_eSprite& display, const ForecastData& forecast);

void drawStatusBar(TFT_eSprite& display);

// Complete dashboard renderer
void drawDashboard(TFT_eSprite& display, const DashboardData& data);

#endif  // WIDGETS_H
