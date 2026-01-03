#include "meteo_client.h"
#include "http_utils.h"
#include <HTTPClient.h>
#include <time.h>
#include "../power/sleep_manager.h"

// HTTP Caching (persistent across deep sleep)
RTC_DATA_ATTR char MeteoClient::lastModified[32] = "";
RTC_DATA_ATTR unsigned long MeteoClient::expiresTimestamp = 0;

MeteoClient::MeteoClient() {
}

// Parse ISO8601 timestamp to Unix epoch with UTC → local time conversion
time_t MeteoClient::parseISO8601(const char* timeStr) {
    if (!timeStr) return 0;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    // Parse: "2025-12-26T13:00:00Z"
    int parsed = sscanf(timeStr, "%d-%d-%dT%d:%d:%d",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    if (parsed < 5) {
        ESP_LOGW("meteo", "Failed to parse timestamp: %s", timeStr);
        return 0;
    }

    tm.tm_year -= 1900;  // Years since 1900
    tm.tm_mon -= 1;      // Months since January

    // Parse as UTC
    setenv("TZ", "UTC", 1);
    tzset();
    time_t utc = mktime(&tm);

    // Convert to local time (CET/CEST)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    return utc;
}

// Parse HTTP date header (RFC 2822) to Unix epoch
time_t MeteoClient::parseHTTPDate(const char* dateStr) {
    if (!dateStr || strlen(dateStr) == 0) return 0;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    // Parse: "Thu, 26 Dec 2025 14:00:00 GMT"
    // Note: strptime may not be available on all ESP32 platforms
    // Using manual parsing as fallback
    char month[4];
    int day, year, hour, min, sec;

    int parsed = sscanf(dateStr, "%*s %d %3s %d %d:%d:%d",
                       &day, month, &year, &hour, &min, &sec);

    if (parsed != 6) {
        ESP_LOGW("meteo", "Failed to parse HTTP date: %s", dateStr);
        return 0;
    }

    tm.tm_mday = day;
    tm.tm_year = year - 1900;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    // Parse month
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            tm.tm_mon = i;
            break;
        }
    }

    // Parse as GMT/UTC
    setenv("TZ", "UTC", 1);
    tzset();
    time_t timestamp = mktime(&tm);

    // Restore local timezone
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    return timestamp;
}

// Parse met.no symbol code to enum index
uint8_t MeteoClient::parseSymbolCode(const char* symbol) {
    if (!symbol) return 2;  // Default: cloudy

    // met.no symbol codes: clearsky, fair, partlycloudy, cloudy, rain, snow, etc.
    // Map to our enum: 0=sunny, 1=partly_cloudy, 2=cloudy, 3=rain, 4=snow

    if (strstr(symbol, "clearsky")) return 0;
    if (strstr(symbol, "fair")) return 1;
    if (strstr(symbol, "partlycloudy")) return 1;
    if (strstr(symbol, "cloudy")) return 2;
    if (strstr(symbol, "lightrain")) return 3;
    if (strstr(symbol, "rain")) return 3;
    if (strstr(symbol, "sleet")) return 3;  // Rain + snow mix
    if (strstr(symbol, "snow")) return 4;
    if (strstr(symbol, "fog")) return 2;    // Fallback to cloudy

    return 2;  // Default: cloudy
}

// Legacy function kept for compatibility
void MeteoClient::parseForecastPoint(JsonObject hourly, int index, ForecastPoint& point) {
    // Deprecated: kept for backward compatibility only
    point.valid = false;
}

// Main forecast fetch function
bool MeteoClient::getForecast(ForecastData& data) {
    ESP_LOGI("meteo", "Fetching forecast from met.no");

    // 1. Check HTTP cache expiration
    time_t now = SleepManager::getEpoch();
    if (now < expiresTimestamp && expiresTimestamp > 0) {
        ESP_LOGI("meteo", "Cache still valid (expires in %ld sec)", expiresTimestamp - now);
        // Use cached data - return true to skip API call
        // Note: Actual cached data is loaded from LittleFS cache by main.cpp
        return true;
    }

    // 2. Build URL (max 4 decimal places for optimal caching)
    String url = METEO_FORECAST_URL;
    url += "?lat=";
    url += String(LOCATION_LAT, 4);  // 46.8042
    url += "&lon=";
    url += String(LOCATION_LON, 4);  // 9.8565

    ESP_LOGI("meteo", "URL: %s", url.c_str());

    // 3. HTTP Request with caching headers
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.begin(url);
    http.addHeader("User-Agent", HTTP_USER_AGENT);

    // Add If-Modified-Since header if we have a cached timestamp
    if (strlen(lastModified) > 0) {
        http.addHeader("If-Modified-Since", lastModified);
        ESP_LOGD("meteo", "If-Modified-Since: %s", lastModified);
    }

    int httpCode = http.GET();

    // 4. Handle HTTP status codes
    if (httpCode == 304) {
        ESP_LOGI("meteo", "304 Not Modified - using cached data");
        http.end();
        return true;  // Data hasn't changed
    }

    if (httpCode == 429) {
        ESP_LOGE("meteo", "429 Throttling! Too many requests");
        http.end();
        return false;
    }

    if (httpCode == 403) {
        ESP_LOGE("meteo", "403 Forbidden! Check User-Agent header");
        http.end();
        return false;
    }

    if (httpCode != 200) {
        ESP_LOGE("meteo", "HTTP error: %d", httpCode);
        http.end();
        return false;
    }

    // 5. Save HTTP caching headers
    if (http.hasHeader("Expires")) {
        String expires = http.header("Expires");
        expiresTimestamp = parseHTTPDate(expires.c_str());
        ESP_LOGI("meteo", "Expires: %s (timestamp: %lu)", expires.c_str(), expiresTimestamp);
    }

    if (http.hasHeader("Last-Modified")) {
        String lastMod = http.header("Last-Modified");
        strncpy(lastModified, lastMod.c_str(), sizeof(lastModified) - 1);
        lastModified[sizeof(lastModified) - 1] = '\0';
        ESP_LOGI("meteo", "Last-Modified: %s", lastModified);
    }

    // 6. Parse JSON (GeoJSON format)
    WiFiClient* stream = http.getStreamPtr();
    JsonDocument doc;  // ArduinoJson v7 auto-sizing

    ESP_LOGI("meteo", "Free heap before JSON: %u", ESP.getFreeHeap());

    DeserializationError error = deserializeJson(doc, *stream);
    http.end();

    if (error) {
        ESP_LOGE("meteo", "JSON parse error: %s", error.c_str());
        return false;
    }

    ESP_LOGI("meteo", "Free heap after JSON: %u", ESP.getFreeHeap());

    // 7. Extract timeseries array
    JsonArray timeseries = doc["properties"]["timeseries"];
    if (!timeseries) {
        ESP_LOGE("meteo", "No timeseries in response");
        return false;
    }

    ESP_LOGI("meteo", "Timeseries entries: %d", timeseries.size());

    // 8. Get current time for day calculation
    time_t currentTime = now;
    struct tm* currentTm = localtime(&currentTime);

    // Calculate midnight of today (local time)
    struct tm todayMidnight = *currentTm;
    todayMidnight.tm_hour = 0;
    todayMidnight.tm_min = 0;
    todayMidnight.tm_sec = 0;
    time_t todayStart = mktime(&todayMidnight);

    // 9. Aggregate 3 days of forecast data
    for (int day = 0; day < 3; day++) {
        time_t dayStart = todayStart + (day * 86400);  // Midnight of day
        time_t dayEnd = dayStart + 86400;              // Midnight of next day

        DailyForecast& dailyForecast = data.days[day];
        dailyForecast.date = dayStart;
        dailyForecast.tempMin = 127;   // Start with max values
        dailyForecast.tempMax = -128;
        dailyForecast.precipSum = 0;
        dailyForecast.windSpeedMax = 0;
        dailyForecast.windDirection = 0;
        dailyForecast.symbolCode = 2;  // Default: cloudy
        int hourlyIdx = 0;
        int symbolCount[5] = {0};  // Count occurrences of each symbol type

        // Iterate through timeseries
        for (JsonObject ts : timeseries) {
            const char* timeStr = ts["time"];
            time_t entryTime = parseISO8601(timeStr);

            // Check if this entry belongs to current day
            if (entryTime < dayStart || entryTime >= dayEnd) continue;

            // Extract instant data
            JsonObject instant = ts["data"]["instant"]["details"];
            if (!instant) continue;

            float temp = instant["air_temperature"] | 0.0f;
            float windMs = instant["wind_speed"] | 0.0f;
            uint16_t windDir = instant["wind_from_direction"] | 0;

            // Update min/max temperature
            dailyForecast.tempMin = min(dailyForecast.tempMin, (int8_t)temp);
            dailyForecast.tempMax = max(dailyForecast.tempMax, (int8_t)temp);

            // Update max wind speed (convert m/s to km/h)
            uint8_t windKmh = (uint8_t)(windMs * 3.6);
            if (windKmh > dailyForecast.windSpeedMax) {
                dailyForecast.windSpeedMax = windKmh;
                dailyForecast.windDirection = windDir;
            }

            // Extract precipitation from next_1_hours (if available)
            JsonObject next1h = ts["data"]["next_1_hours"];
            if (next1h) {
                float precip = next1h["details"]["precipitation_amount"] | 0.0f;
                dailyForecast.precipSum += (uint8_t)(precip * 10);  // Store as mm*10

                // Extract symbol code
                const char* symbol = next1h["summary"]["symbol_code"];
                if (symbol) {
                    uint8_t code = parseSymbolCode(symbol);
                    symbolCount[code]++;
                }
            }

            // Collect day-time data points (3 times: 06:00, 12:00, 18:00)
            // IMPORTANT: Map hours to fixed array indices to keep positions consistent
            struct tm* entryTm = localtime(&entryTime);
            int timeSlotIdx = -1;
            if (entryTm->tm_hour == 6) timeSlotIdx = 0;
            else if (entryTm->tm_hour == 12) timeSlotIdx = 1;
            else if (entryTm->tm_hour == 18) timeSlotIdx = 2;

            if (timeSlotIdx >= 0) {
                DayTimeForecast& dtf = dailyForecast.times[timeSlotIdx];
                dtf.hour = entryTm->tm_hour;
                dtf.temperature = (int8_t)temp;

                // Get symbol from next_1_hours
                if (next1h) {
                    const char* symbol = next1h["summary"]["symbol_code"];
                    dtf.symbolCode = parseSymbolCode(symbol);

                    float precip = next1h["details"]["precipitation_amount"] | 0.0f;
                    dtf.precipitationMm = (uint8_t)(precip * 10);  // Store as mm*10
                } else {
                    dtf.symbolCode = 2;  // cloudy
                    dtf.precipitationMm = 0;
                }

                hourlyIdx++;  // Still count for logging
            }
        }

        // Determine dominant symbol for the day (most frequent)
        uint8_t maxCount = 0;
        uint8_t dominantSymbol = 2;  // Default: cloudy
        for (int i = 0; i < 5; i++) {
            if (symbolCount[i] > maxCount) {
                maxCount = symbolCount[i];
                dominantSymbol = i;
            }
        }
        dailyForecast.symbolCode = dominantSymbol;

        // Validate data
        dailyForecast.valid = (dailyForecast.tempMin < 127 && dailyForecast.tempMax > -128);

        if (dailyForecast.valid) {
            ESP_LOGI("meteo", "Day %d: %d/%d°C, precip: %dmm, wind: %dkm/h, symbol: %d, hours: %d",
                    day, dailyForecast.tempMin, dailyForecast.tempMax,
                    dailyForecast.precipSum / 10, dailyForecast.windSpeedMax,
                    dailyForecast.symbolCode, hourlyIdx);
        } else {
            ESP_LOGW("meteo", "Day %d: No valid data", day);
        }
    }

    // 10. Fill current forecast (for backward compatibility)
    if (data.days[0].valid && data.days[0].times[0].temperature != 0) {
        data.current.temperature = data.days[0].times[0].temperature;
        data.current.weatherCode = data.days[0].symbolCode;
        data.current.valid = true;
    }

    ESP_LOGI("meteo", "Forecast parsed successfully");
    ESP_LOGI("meteo", "Free heap: %u, min: %u", ESP.getFreeHeap(), ESP.getMinFreeHeap());

    return true;
}

// Legacy WMO weather code functions (kept for compatibility)
const char* MeteoClient::getWeatherDescription(uint8_t code) {
    if (code == 0 || code == 1) return "Clear";
    if (code == 2) return "Partly Cloudy";
    if (code == 3) return "Cloudy";
    if (code >= 45 && code <= 48) return "Fog";
    if (code >= 51 && code <= 57) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Rain Showers";
    if (code >= 85 && code <= 86) return "Snow Showers";
    if (code >= 95) return "Thunderstorm";
    return "Unknown";
}

const char* MeteoClient::getWeatherIcon(uint8_t code) {
    if (code == 0 || code == 1) return "sunny";
    if (code == 2) return "partly_cloudy";
    if (code == 3) return "cloudy";
    if (code >= 61 && code <= 67) return "rain";
    if (code >= 71 && code <= 77) return "snow";
    return "cloudy";
}
