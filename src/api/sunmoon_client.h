#ifndef SUNMOON_CLIENT_H
#define SUNMOON_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../data/weather_data.h"
#include "../config.h"

class SunMoonClient {
private:
    // Last fetch date (persistent across deep sleep via RTC_DATA_ATTR in .cpp)
    static char lastFetchDate[12];  // "YYYY-MM-DD"

    // Parse ISO8601 timestamp to Unix epoch (reuse pattern from MeteoClient)
    time_t parseISO8601(const char* timeStr);

    // Check if data needs update (date changed)
    bool needsUpdate();

    // Build URL parameters for API calls (with optional day offset: 0=today, 1=tomorrow)
    void buildURLParams(char* params, size_t maxLen, int dayOffset = 0);

    // Fetch sun data for a specific day (0=today, 1=tomorrow)
    bool fetchSunDataForDay(int dayOffset, unsigned long& sunriseTime, unsigned long& sunsetTime,
                            float& sunriseAz, float& sunsetAz);

public:
    SunMoonClient();

    // Get sun data (sunrise/sunset) from met.no API
    bool getSunData(SunData& data);

    // Get moon data (moonrise/moonset/phase) from met.no API
    bool getMoonData(MoonData& data);
};

#endif  // SUNMOON_CLIENT_H
