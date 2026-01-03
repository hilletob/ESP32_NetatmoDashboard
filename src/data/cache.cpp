#include "cache.h"

bool DataCache::init() {
    if (!LittleFS.begin(true)) {  // Format if mount fails
        ESP_LOGE("cache", "LittleFS mount failed");
        return false;
    }

    ESP_LOGI("cache", "LittleFS initialized");
    ESP_LOGI("cache", "Total: %u bytes, Used: %u bytes",
            LittleFS.totalBytes(), LittleFS.usedBytes());

    return true;
}

bool DataCache::save(const DashboardData& data) {
    ESP_LOGI("cache", "Saving dashboard data to cache");

    JsonDocument doc;

    // Metadata
    doc["cacheTime"] = millis();
    doc["updateTime"] = data.updateTime;

    // Indoor data
    JsonObject indoor = doc["indoor"].to<JsonObject>();
    indoor["temp"] = data.weather.indoor.temperature;
    indoor["humidity"] = data.weather.indoor.humidity;
    indoor["co2"] = data.weather.indoor.co2;
    indoor["noise"] = data.weather.indoor.noise;
    indoor["pressure"] = data.weather.indoor.pressure;
    indoor["tempTrend"] = (int)data.weather.indoor.temperatureTrend;
    indoor["pressTrend"] = (int)data.weather.indoor.pressureTrend;
    indoor["co2Trend"] = (int)data.weather.indoor.co2Trend;
    indoor["minTemp"] = data.weather.indoor.minTemp;
    indoor["maxTemp"] = data.weather.indoor.maxTemp;
    indoor["valid"] = data.weather.indoor.valid;

    // Outdoor data
    JsonObject outdoor = doc["outdoor"].to<JsonObject>();
    outdoor["temp"] = data.weather.outdoor.temperature;
    outdoor["humidity"] = data.weather.outdoor.humidity;
    outdoor["tempTrend"] = (int)data.weather.outdoor.temperatureTrend;
    outdoor["minTemp"] = data.weather.outdoor.minTemp;
    outdoor["maxTemp"] = data.weather.outdoor.maxTemp;
    outdoor["valid"] = data.weather.outdoor.valid;

    // Forecast current
    JsonObject current = doc["current"].to<JsonObject>();
    current["temp"] = data.forecast.current.temperature;
    current["code"] = data.forecast.current.weatherCode;
    current["humidity"] = data.forecast.current.humidity;
    current["valid"] = data.forecast.current.valid;

    // Forecast 3h
    JsonObject f3h = doc["forecast3h"].to<JsonObject>();
    f3h["temp"] = data.forecast.forecast3h.temperature;
    f3h["code"] = data.forecast.forecast3h.weatherCode;
    f3h["precip"] = data.forecast.forecast3h.precipitation;
    f3h["valid"] = data.forecast.forecast3h.valid;

    // Forecast 6h
    JsonObject f6h = doc["forecast6h"].to<JsonObject>();
    f6h["temp"] = data.forecast.forecast6h.temperature;
    f6h["code"] = data.forecast.forecast6h.weatherCode;
    f6h["precip"] = data.forecast.forecast6h.precipitation;
    f6h["valid"] = data.forecast.forecast6h.valid;

    // 3-day forecast (met.no)
    JsonArray days = doc["days"].to<JsonArray>();
    for (int i = 0; i < 3; i++) {
        JsonObject day = days.add<JsonObject>();
        day["date"] = data.forecast.days[i].date;
        day["tempMin"] = data.forecast.days[i].tempMin;
        day["tempMax"] = data.forecast.days[i].tempMax;
        day["symbol"] = data.forecast.days[i].symbolCode;
        day["precipSum"] = data.forecast.days[i].precipSum;
        day["windMax"] = data.forecast.days[i].windSpeedMax;
        day["windDir"] = data.forecast.days[i].windDirection;
        day["valid"] = data.forecast.days[i].valid;

        // Day-time forecast (3 times: 06:00, 12:00, 18:00)
        JsonArray times = day["times"].to<JsonArray>();
        for (int t = 0; t < 3; t++) {
            JsonObject time = times.add<JsonObject>();
            time["h"] = data.forecast.days[i].times[t].hour;
            time["t"] = data.forecast.days[i].times[t].temperature;
            time["s"] = data.forecast.days[i].times[t].symbolCode;
            time["p"] = data.forecast.days[i].times[t].precipitationMm;
        }
    }

    // Battery
    doc["batteryMv"] = data.batteryVoltage;
    doc["batteryPct"] = data.batteryPercent;

    // Sun data
    JsonObject sun = doc["sunData"].to<JsonObject>();
    sun["sunrise"] = data.sunData.sunriseTime;
    sun["sunset"] = data.sunData.sunsetTime;
    sun["solarNoon"] = data.sunData.solarNoonTime;
    sun["sunriseAz"] = data.sunData.sunriseAzimuth;
    sun["sunsetAz"] = data.sunData.sunsetAzimuth;
    sun["valid"] = data.sunData.valid;

    // Moon data
    JsonObject moon = doc["moonData"].to<JsonObject>();
    moon["moonrise"] = data.moonData.moonriseTime;
    moon["moonset"] = data.moonData.moonsetTime;
    moon["phase"] = data.moonData.moonPhase;
    moon["valid"] = data.moonData.valid;

    // Write to file
    File file = LittleFS.open(CACHE_FILE, "w");
    if (!file) {
        ESP_LOGE("cache", "Failed to open cache file for writing");
        return false;
    }

    size_t written = serializeJson(doc, file);
    file.close();

    if (written == 0) {
        ESP_LOGE("cache", "Failed to write cache file");
        return false;
    }

    ESP_LOGI("cache", "Cache saved: %u bytes", written);
    return true;
}

bool DataCache::load(DashboardData& data) {
    if (!LittleFS.exists(CACHE_FILE)) {
        ESP_LOGW("cache", "Cache file does not exist");
        return false;
    }

    File file = LittleFS.open(CACHE_FILE, "r");
    if (!file) {
        ESP_LOGE("cache", "Failed to open cache file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        ESP_LOGE("cache", "Failed to parse cache file: %s", error.c_str());
        return false;
    }

    // Check cache age
    unsigned long cacheTime = doc["cacheTime"] | 0;
    unsigned long ageMs = millis() - cacheTime;
    unsigned long ageSec = ageMs / 1000;

    ESP_LOGI("cache", "Cache age: %lu seconds", ageSec);

    if (ageSec > CACHE_MAX_AGE_SEC) {
        ESP_LOGW("cache", "Cache is too old (> %d seconds)", CACHE_MAX_AGE_SEC);
        return false;
    }

    // Load data
    data.updateTime = doc["updateTime"] | 0;

    // Indoor data
    JsonObject indoor = doc["indoor"];
    data.weather.indoor.temperature = indoor["temp"] | 0.0f;
    data.weather.indoor.humidity = indoor["humidity"] | 0;
    data.weather.indoor.co2 = indoor["co2"] | 0;
    data.weather.indoor.noise = indoor["noise"] | 0;
    data.weather.indoor.pressure = indoor["pressure"] | 0;
    data.weather.indoor.temperatureTrend = (Trend)(indoor["tempTrend"] | 0);
    data.weather.indoor.pressureTrend = (Trend)(indoor["pressTrend"] | 0);
    data.weather.indoor.co2Trend = (Trend)(indoor["co2Trend"] | 0);
    data.weather.indoor.minTemp = indoor["minTemp"] | 0.0f;
    data.weather.indoor.maxTemp = indoor["maxTemp"] | 0.0f;
    data.weather.indoor.valid = indoor["valid"] | false;

    // Outdoor data
    JsonObject outdoor = doc["outdoor"];
    data.weather.outdoor.temperature = outdoor["temp"] | 0.0f;
    data.weather.outdoor.humidity = outdoor["humidity"] | 0;
    data.weather.outdoor.temperatureTrend = (Trend)(outdoor["tempTrend"] | 0);
    data.weather.outdoor.minTemp = outdoor["minTemp"] | 0.0f;
    data.weather.outdoor.maxTemp = outdoor["maxTemp"] | 0.0f;
    data.weather.outdoor.valid = outdoor["valid"] | false;

    // Forecast current
    JsonObject current = doc["current"];
    data.forecast.current.temperature = current["temp"] | 0.0f;
    data.forecast.current.weatherCode = current["code"] | 0;
    data.forecast.current.humidity = current["humidity"] | 0;
    data.forecast.current.valid = current["valid"] | false;

    // Forecast 3h
    JsonObject f3h = doc["forecast3h"];
    data.forecast.forecast3h.temperature = f3h["temp"] | 0.0f;
    data.forecast.forecast3h.weatherCode = f3h["code"] | 0;
    data.forecast.forecast3h.precipitation = f3h["precip"] | 0.0f;
    data.forecast.forecast3h.valid = f3h["valid"] | false;

    // Forecast 6h
    JsonObject f6h = doc["forecast6h"];
    data.forecast.forecast6h.temperature = f6h["temp"] | 0.0f;
    data.forecast.forecast6h.weatherCode = f6h["code"] | 0;
    data.forecast.forecast6h.precipitation = f6h["precip"] | 0.0f;
    data.forecast.forecast6h.valid = f6h["valid"] | false;

    // 3-day forecast (met.no)
    JsonArray days = doc["days"];
    if (days) {
        for (int i = 0; i < 3 && i < days.size(); i++) {
            JsonObject day = days[i];
            data.forecast.days[i].date = day["date"] | 0;
            data.forecast.days[i].tempMin = day["tempMin"] | 0;
            data.forecast.days[i].tempMax = day["tempMax"] | 0;
            data.forecast.days[i].symbolCode = day["symbol"] | 0;
            data.forecast.days[i].precipSum = day["precipSum"] | 0;
            data.forecast.days[i].windSpeedMax = day["windMax"] | 0;
            data.forecast.days[i].windDirection = day["windDir"] | 0;
            data.forecast.days[i].valid = day["valid"] | false;

            // Day-time forecast (3 times: 06:00, 12:00, 18:00)
            JsonArray times = day["times"];
            if (times) {
                for (int t = 0; t < 3 && t < times.size(); t++) {
                    JsonObject time = times[t];
                    data.forecast.days[i].times[t].hour = time["h"] | 0;
                    data.forecast.days[i].times[t].temperature = time["t"] | 0;
                    data.forecast.days[i].times[t].symbolCode = time["s"] | 0;
                    data.forecast.days[i].times[t].precipitationMm = time["p"] | 0;
                }
            }
        }
    }

    // Battery
    data.batteryVoltage = doc["batteryMv"] | 0;
    data.batteryPercent = doc["batteryPct"] | 0;

    // Sun data
    JsonObject sun = doc["sunData"];
    data.sunData.sunriseTime = sun["sunrise"] | 0;
    data.sunData.sunsetTime = sun["sunset"] | 0;
    data.sunData.solarNoonTime = sun["solarNoon"] | 0;
    data.sunData.sunriseAzimuth = sun["sunriseAz"] | 0.0f;
    data.sunData.sunsetAzimuth = sun["sunsetAz"] | 0.0f;
    data.sunData.valid = sun["valid"] | false;

    // Moon data
    JsonObject moon = doc["moonData"];
    data.moonData.moonriseTime = moon["moonrise"] | 0;
    data.moonData.moonsetTime = moon["moonset"] | 0;
    data.moonData.moonPhase = moon["phase"] | 0.0f;
    data.moonData.valid = moon["valid"] | false;

    ESP_LOGI("cache", "Cache loaded successfully");
    return true;
}

bool DataCache::isValid() {
    if (!LittleFS.exists(CACHE_FILE)) {
        return false;
    }

    return (getAgeSeconds() <= CACHE_MAX_AGE_SEC);
}

unsigned long DataCache::getAgeSeconds() {
    if (!LittleFS.exists(CACHE_FILE)) {
        return UINT32_MAX;  // Very old
    }

    File file = LittleFS.open(CACHE_FILE, "r");
    if (!file) {
        return UINT32_MAX;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return UINT32_MAX;
    }

    unsigned long cacheTime = doc["cacheTime"] | 0;
    unsigned long ageMs = millis() - cacheTime;
    return ageMs / 1000;
}

void DataCache::clear() {
    if (LittleFS.exists(CACHE_FILE)) {
        LittleFS.remove(CACHE_FILE);
        ESP_LOGI("cache", "Cache cleared");
    }
}
