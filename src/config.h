#ifndef CONFIG_H
#define CONFIG_H

// Include local overrides if they exist
#if __has_include("config.local.h")
#include "config.local.h"
#endif

// WiFi Configuration
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef WIFI_TIMEOUT_MS
#define WIFI_TIMEOUT_MS 10000  // 10 seconds
#endif

// Netatmo API Configuration
#ifndef NETATMO_CLIENT_ID
#define NETATMO_CLIENT_ID ""
#endif
#ifndef NETATMO_CLIENT_SECRET
#define NETATMO_CLIENT_SECRET ""
#endif
#ifndef NETATMO_REFRESH_TOKEN
#define NETATMO_REFRESH_TOKEN ""
#endif

// Station Configuration (Default: Davos - Grialetschhüsli)
#ifndef NETATMO_DEVICE_ID
#define NETATMO_DEVICE_ID "70:ee:50:19:27:82"
#endif

// Location Configuration (Davos coordinates for MeteoSwiss)
#ifndef LOCATION_LAT
#define LOCATION_LAT 46.8042
#endif
#ifndef LOCATION_LON
#define LOCATION_LON 9.8565
#endif
#ifndef LOCATION_NAME
#define LOCATION_NAME "Davos"
#endif

// Update Configuration
#ifndef UPDATE_INTERVAL_MIN
#define UPDATE_INTERVAL_MIN 11  // Minutes after last Netatmo update
#endif
#ifndef MINIMUM_SLEEP_SEC
#define MINIMUM_SLEEP_SEC 60  // Minimum 1 minute between updates
#endif
#ifndef MAXIMUM_SLEEP_SEC
#define MAXIMUM_SLEEP_SEC 900  // Maximum 15 minutes between updates
#endif

// Hardware Configuration
#ifndef BATTERY_PIN
#define BATTERY_PIN A0  // GPIO1
#endif

// Display Configuration
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// NTP Configuration
#ifndef NTP_SERVER_1
#define NTP_SERVER_1 "time1.google.com"
#endif
#ifndef NTP_SERVER_2
#define NTP_SERVER_2 "time2.google.com"
#endif
#ifndef NTP_TIMEOUT_MS
#define NTP_TIMEOUT_MS 15000  // 15 seconds
#endif

// API Endpoints
#define NETATMO_TOKEN_URL "https://api.netatmo.com/oauth2/token"
#define NETATMO_WEATHER_URL "https://api.netatmo.com/api/getstationsdata"
#define METEO_FORECAST_URL "https://api.met.no/weatherapi/locationforecast/2.0/compact"

// HTTP Configuration
#ifndef HTTP_TIMEOUT_MS
#define HTTP_TIMEOUT_MS 20000  // 20 seconds (met.no can be slower)
#endif
#ifndef HTTP_USER_AGENT
// CRITICAL: met.no requires User-Agent with contact info, otherwise 403 Forbidden!
// Override this in config.local.h with your actual email
#define HTTP_USER_AGENT "ESP32-Davos-WeatherDashboard/2.0 (github.com/yourusername/esp32netatmo)"
#endif

// Cache Configuration
#define CACHE_FILE "/weather_cache.json"
#define CACHE_MAX_AGE_SEC 7200  // 2 hours

// Battery Voltage Thresholds (mV)
#define BATTERY_MIN_MV 3300
#define BATTERY_MAX_MV 4350

#endif  // CONFIG_H
