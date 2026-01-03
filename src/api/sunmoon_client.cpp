#include "sunmoon_client.h"
#include "http_utils.h"
#include <HTTPClient.h>
#include <time.h>

// Last fetch date (persistent across deep sleep)
RTC_DATA_ATTR char SunMoonClient::lastFetchDate[12] = "";

SunMoonClient::SunMoonClient() {
}

// Parse ISO8601 timestamp with timezone to Unix epoch
// Example: "2022-12-18T09:16+01:00"
time_t SunMoonClient::parseISO8601(const char* timeStr) {
    if (!timeStr) return 0;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    int tzHour = 0, tzMin = 0;
    char tzSign = '+';

    // Parse: "2022-12-18T09:16+01:00" or "2022-12-18T09:16:34+01:00"
    int parsed = sscanf(timeStr, "%d-%d-%dT%d:%d:%d%c%d:%d",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min, &tm.tm_sec,
                       &tzSign, &tzHour, &tzMin);

    // Try without seconds if full parse failed
    if (parsed < 6) {
        parsed = sscanf(timeStr, "%d-%d-%dT%d:%d%c%d:%d",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min,
                       &tzSign, &tzHour, &tzMin);
        tm.tm_sec = 0;
    }

    if (parsed < 5) {
        ESP_LOGW("sunmoon", "Failed to parse timestamp: %s", timeStr);
        return 0;
    }

    tm.tm_year -= 1900;  // Years since 1900
    tm.tm_mon -= 1;      // Months since January
    tm.tm_isdst = -1;    // Let mktime() determine DST

    // CRITICAL: Don't change timezone during parsing!
    // mktime() will use the current timezone (CET/CEST) to create the timestamp
    // Then we adjust for the difference between API timezone and local timezone
    time_t localTime = mktime(&tm);

    // Calculate offset difference
    // API time is in +tzHour:tzMin timezone
    // We need to convert to local timezone
    // Example: API says "09:57+01:00" and we're in CET (+01:00)
    // No adjustment needed if offsets match
    // But mktime() already gave us the right timestamp!

    // Actually, we need to interpret the time AS IF it's in the API's timezone
    // The time "09:57+01:00" means 09:57 in that timezone
    // mktime() interpreted it in our local timezone
    // We need to adjust by the difference

    int apiOffsetSeconds = (tzHour * 3600 + tzMin * 60);
    if (tzSign == '-') apiOffsetSeconds = -apiOffsetSeconds;

    // Get our local timezone offset
    time_t now = time(nullptr);
    struct tm* localTm = localtime(&now);
    int localOffsetSeconds = (localTm->tm_isdst ? 7200 : 3600);  // CET=3600, CEST=7200

    // Adjust: subtract API offset, add local offset
    // This converts from "time interpreted in local TZ" to "time in API TZ"
    time_t utc = localTime - localOffsetSeconds + apiOffsetSeconds;

    return utc;
}

// Check if data needs update (date changed)
bool SunMoonClient::needsUpdate() {
    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);

    char today[12];
    snprintf(today, sizeof(today), "%04d-%02d-%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    return strcmp(today, lastFetchDate) != 0;
}

// Build URL parameters for API calls (with optional day offset: 0=today, 1=tomorrow)
void SunMoonClient::buildURLParams(char* params, size_t maxLen, int dayOffset) {
    time_t now = time(nullptr);

    // Add day offset if needed
    if (dayOffset > 0) {
        now += (dayOffset * 86400);  // Add days in seconds
    }

    struct tm* tm = localtime(&now);

    // Build date string (YYYY-MM-DD)
    char dateStr[12];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    // Build timezone offset (+HH:MM)
    int offsetHours = (tm->tm_isdst ? 2 : 1);  // CEST = +02:00, CET = +01:00
    char offset[10];  // "%2B01:00" = 8 chars + null terminator
    snprintf(offset, sizeof(offset), "%%2B%02d:00", offsetHours);  // URL-encoded '+'

    ESP_LOGI("sunmoon", "Date: %s (offset=%d days), DST: %d, Offset: +%02d:00",
             dateStr, dayOffset, tm->tm_isdst, offsetHours);

    // Build full parameter string
    snprintf(params, maxLen, "?lat=%.5f&lon=%.5f&date=%s&offset=%s",
             LOCATION_LAT, LOCATION_LON, dateStr, offset);

    // Update last fetch date only for today (dayOffset == 0)
    if (dayOffset == 0) {
        strncpy(lastFetchDate, dateStr, sizeof(lastFetchDate) - 1);
    }
}

// Fetch sun data for a specific day (0=today, 1=tomorrow)
bool SunMoonClient::fetchSunDataForDay(int dayOffset, unsigned long& sunriseTime, unsigned long& sunsetTime,
                                        float& sunriseAz, float& sunsetAz) {
    // Build URL
    char params[128];
    buildURLParams(params, sizeof(params), dayOffset);

    char url[256];
    snprintf(url, sizeof(url), "https://api.met.no/weatherapi/sunrise/3.0/sun%s", params);

    // Fetch data
    JsonDocument doc;
    if (!HTTPUtils::httpGetJSON(url, doc)) {
        ESP_LOGE("sunmoon", "Failed to fetch sun data for day offset %d", dayOffset);
        return false;
    }

    // Parse response
    JsonObject properties = doc["properties"];
    if (!properties) {
        ESP_LOGE("sunmoon", "Invalid sun response: no properties");
        return false;
    }

    // Parse sunrise
    JsonObject sunrise = properties["sunrise"];
    if (sunrise) {
        const char* timeStr = sunrise["time"];
        sunriseTime = parseISO8601(timeStr);
        sunriseAz = sunrise["azimuth"] | 0.0f;
    }

    // Parse sunset
    JsonObject sunset = properties["sunset"];
    if (sunset) {
        const char* timeStr = sunset["time"];
        sunsetTime = parseISO8601(timeStr);
        sunsetAz = sunset["azimuth"] | 0.0f;
    }

    return (sunriseTime > 0 && sunsetTime > 0);
}

// Get sun data (sunrise/sunset) from met.no API
// Shows NEXT sunrise and NEXT sunset (may be tomorrow if already passed today)
bool SunMoonClient::getSunData(SunData& data) {
    // Only fetch if date changed
    if (!needsUpdate()) {
        ESP_LOGI("sunmoon", "Sun data still valid for today (already in cache)");
        return true;  // Cached data is still valid, don't update
    }

    ESP_LOGI("sunmoon", "Fetching sun data from met.no API");

    time_t now = time(nullptr);

    // Fetch today's data
    unsigned long todaySunrise = 0, todaySunset = 0;
    float todaySunriseAz = 0.0f, todaySunsetAz = 0.0f;

    if (!fetchSunDataForDay(0, todaySunrise, todaySunset, todaySunriseAz, todaySunsetAz)) {
        ESP_LOGE("sunmoon", "Failed to fetch today's sun data");
        return false;
    }

    // Log today's times
    struct tm* tm_sunrise = localtime((time_t*)&todaySunrise);
    struct tm* tm_sunset = localtime((time_t*)&todaySunset);
    ESP_LOGI("sunmoon", "Today: sunrise=%02d:%02d, sunset=%02d:%02d",
             tm_sunrise->tm_hour, tm_sunrise->tm_min,
             tm_sunset->tm_hour, tm_sunset->tm_min);

    // Check if we need tomorrow's data
    bool needTomorrowSunrise = (now >= todaySunrise);
    bool needTomorrowSunset = (now >= todaySunset);

    // Default to today's data
    data.sunriseTime = todaySunrise;
    data.sunriseAzimuth = todaySunriseAz;
    data.sunsetTime = todaySunset;
    data.sunsetAzimuth = todaySunsetAz;

    // Fetch tomorrow's data if needed
    if (needTomorrowSunrise || needTomorrowSunset) {
        unsigned long tomorrowSunrise = 0, tomorrowSunset = 0;
        float tomorrowSunriseAz = 0.0f, tomorrowSunsetAz = 0.0f;

        if (fetchSunDataForDay(1, tomorrowSunrise, tomorrowSunset, tomorrowSunriseAz, tomorrowSunsetAz)) {
            struct tm* tm_tomorrow_sunrise = localtime((time_t*)&tomorrowSunrise);
            struct tm* tm_tomorrow_sunset = localtime((time_t*)&tomorrowSunset);
            ESP_LOGI("sunmoon", "Tomorrow: sunrise=%02d:%02d, sunset=%02d:%02d",
                     tm_tomorrow_sunrise->tm_hour, tm_tomorrow_sunrise->tm_min,
                     tm_tomorrow_sunset->tm_hour, tm_tomorrow_sunset->tm_min);

            // Use tomorrow's sunrise if today's has passed
            if (needTomorrowSunrise) {
                data.sunriseTime = tomorrowSunrise;
                data.sunriseAzimuth = tomorrowSunriseAz;
                ESP_LOGI("sunmoon", "Using tomorrow's sunrise (today's has passed)");
            }

            // Use tomorrow's sunset if today's has passed
            if (needTomorrowSunset) {
                data.sunsetTime = tomorrowSunset;
                data.sunsetAzimuth = tomorrowSunsetAz;
                ESP_LOGI("sunmoon", "Using tomorrow's sunset (today's has passed)");
            }
        } else {
            ESP_LOGW("sunmoon", "Failed to fetch tomorrow's data, using today's");
        }
    }

    // Validate
    data.valid = (data.sunriseTime > 0 && data.sunsetTime > 0);

    if (data.valid) {
        struct tm* final_sunrise = localtime((time_t*)&data.sunriseTime);
        struct tm* final_sunset = localtime((time_t*)&data.sunsetTime);
        ESP_LOGI("sunmoon", "Final data: sunrise=%02d:%02d, sunset=%02d:%02d",
                 final_sunrise->tm_hour, final_sunrise->tm_min,
                 final_sunset->tm_hour, final_sunset->tm_min);
    } else {
        ESP_LOGW("sunmoon", "Sun data incomplete");
    }

    return data.valid;
}

// Get moon data (moonrise/moonset/phase) from met.no API
bool SunMoonClient::getMoonData(MoonData& data) {
    // Only fetch if date changed
    if (!needsUpdate()) {
        ESP_LOGI("sunmoon", "Moon data still valid for today");
        return data.valid;  // Return cached data validity
    }

    ESP_LOGI("sunmoon", "Fetching moon data from met.no API");

    // Build URL
    char params[128];
    buildURLParams(params, sizeof(params));

    char url[256];
    snprintf(url, sizeof(url), "https://api.met.no/weatherapi/sunrise/3.0/moon%s", params);

    // Fetch data
    JsonDocument doc;
    if (!HTTPUtils::httpGetJSON(url, doc)) {
        ESP_LOGE("sunmoon", "Failed to fetch moon data");
        return false;
    }

    // Parse response
    JsonObject properties = doc["properties"];
    if (!properties) {
        ESP_LOGE("sunmoon", "Invalid moon response: no properties");
        return false;
    }

    // Parse moonrise
    JsonObject moonrise = properties["moonrise"];
    if (moonrise) {
        const char* timeStr = moonrise["time"];
        data.moonriseTime = parseISO8601(timeStr);
    }

    // Parse moonset
    JsonObject moonset = properties["moonset"];
    if (moonset) {
        const char* timeStr = moonset["time"];
        data.moonsetTime = parseISO8601(timeStr);
    }

    // Parse moon phase
    data.moonPhase = properties["moonphase"] | 0.0f;

    // Validate (moon times can be null during certain days, but phase should exist)
    data.valid = (data.moonPhase >= 0.0f && data.moonPhase <= 1.0f);

    if (data.valid) {
        ESP_LOGI("sunmoon", "Moon data: rise=%lu, set=%lu, phase=%.2f",
                 data.moonriseTime, data.moonsetTime, data.moonPhase);
    } else {
        ESP_LOGW("sunmoon", "Moon data incomplete");
    }

    return data.valid;
}
