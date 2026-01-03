#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <Arduino.h>

// Trend indicators
enum class Trend {
    STABLE,
    UP,
    DOWN,
    UNKNOWN
};

// Indoor climate data (from main Netatmo device)
struct IndoorData {
    float temperature;          // °C
    uint8_t humidity;           // %
    uint16_t co2;               // ppm
    uint8_t noise;              // dB
    uint16_t pressure;          // mbar
    Trend temperatureTrend;
    Trend pressureTrend;
    Trend co2Trend;
    float minTemp;              // Daily minimum
    float maxTemp;              // Daily maximum
    unsigned long dateMinTemp;  // Unix timestamp
    unsigned long dateMaxTemp;  // Unix timestamp
    bool valid;                 // Data validity flag

    IndoorData() : temperature(0), humidity(0), co2(0), noise(0), pressure(0),
                   temperatureTrend(Trend::UNKNOWN), pressureTrend(Trend::UNKNOWN),
                   co2Trend(Trend::UNKNOWN), minTemp(0), maxTemp(0),
                   dateMinTemp(0), dateMaxTemp(0), valid(false) {}
};

// Outdoor climate data (from outdoor Netatmo module)
struct OutdoorData {
    float temperature;          // °C
    uint8_t humidity;           // %
    Trend temperatureTrend;
    float minTemp;              // Daily minimum
    float maxTemp;              // Daily maximum
    unsigned long dateMinTemp;  // Unix timestamp
    unsigned long dateMaxTemp;  // Unix timestamp
    bool valid;                 // Data validity flag

    OutdoorData() : temperature(0), humidity(0), temperatureTrend(Trend::UNKNOWN),
                    minTemp(0), maxTemp(0), dateMinTemp(0), dateMaxTemp(0), valid(false) {}
};

// Wind data (from wind Netatmo module)
struct WindData {
    uint16_t strength;          // km/h
    uint16_t angle;             // degrees (0-360)
    uint16_t gustStrength;      // km/h
    uint16_t gustAngle;         // degrees
    uint16_t maxWindStrength;   // Daily max km/h
    unsigned long dateMaxWind;  // Unix timestamp
    bool valid;                 // Data validity flag

    WindData() : strength(0), angle(0), gustStrength(0), gustAngle(0),
                 maxWindStrength(0), dateMaxWind(0), valid(false) {}
};

// Rain data (from rain Netatmo module)
struct RainData {
    float current;              // mm
    float sum1h;                // mm (last hour)
    float sum24h;               // mm (last 24 hours)
    bool valid;                 // Data validity flag

    RainData() : current(0), sum1h(0), sum24h(0), valid(false) {}
};

// Complete Netatmo weather data
struct WeatherData {
    IndoorData indoor;
    OutdoorData outdoor;
    WindData wind;
    RainData rain;
    unsigned long timestamp;    // Unix timestamp of last measurement
    String stationName;

    WeatherData() : timestamp(0), stationName("") {}
};

// Forecast data point (from MeteoSwiss/Open-Meteo)
struct ForecastPoint {
    unsigned long timestamp;    // Unix timestamp
    float temperature;          // °C
    uint8_t weatherCode;        // WMO weather code
    float precipitation;        // mm
    uint8_t humidity;           // %
    uint16_t windSpeed;         // km/h
    uint16_t windDirection;     // degrees
    bool valid;                 // Data validity flag

    ForecastPoint() : timestamp(0), temperature(0), weatherCode(0),
                     precipitation(0), humidity(0), windSpeed(0),
                     windDirection(0), valid(false) {}
};

// Day-time forecast point (3 times per day: morning, noon, evening)
struct DayTimeForecast {
    uint8_t hour;              // 6, 12, or 18 (local time)
    int8_t temperature;        // °C
    uint8_t symbolCode;        // Symbol code enum index (0=sunny, 1=partly_cloudy, 2=cloudy, 3=rain, 4=snow)
    uint8_t precipitationMm;   // mm (0-255)

    DayTimeForecast() : hour(0), temperature(0), symbolCode(0), precipitationMm(0) {}
};

// Legacy alias for backward compatibility
typedef DayTimeForecast HourlyForecast;

// Daily forecast summary with day-time breakdown
struct DailyForecast {
    unsigned long date;        // Unix timestamp (midnight UTC)
    int8_t tempMin;            // °C
    int8_t tempMax;            // °C
    uint8_t symbolCode;        // Dominant weather symbol for the day
    uint8_t precipSum;         // Total precipitation 24h (mm)
    uint8_t windSpeedMax;      // Max wind speed km/h
    uint16_t windDirection;    // Wind direction (degrees)
    DayTimeForecast times[3];  // 3 times per day: 06:00, 12:00, 18:00
    bool valid;                // Data validity flag

    DailyForecast() : date(0), tempMin(0), tempMax(0), symbolCode(0),
                     precipSum(0), windSpeedMax(0), windDirection(0), valid(false) {}
};

// Extended forecast data (met.no API)
struct ForecastData {
    ForecastPoint current;      // Current conditions (for backward compatibility)
    ForecastPoint forecast3h;   // +3 hours (deprecated, kept for compatibility)
    ForecastPoint forecast6h;   // +6 hours (deprecated, kept for compatibility)
    DailyForecast days[3];      // 3-day forecast from met.no

    ForecastData() {}
};

// Sun data (sunrise/sunset from met.no Sunrise API)
struct SunData {
    unsigned long sunriseTime;      // Unix timestamp
    unsigned long sunsetTime;       // Unix timestamp
    unsigned long solarNoonTime;    // Unix timestamp (optional)
    float sunriseAzimuth;           // 0-360 degrees
    float sunsetAzimuth;            // 0-360 degrees
    bool valid;                     // Data validity flag

    SunData() : sunriseTime(0), sunsetTime(0), solarNoonTime(0),
                sunriseAzimuth(0), sunsetAzimuth(0), valid(false) {}
};

// Moon data (moonrise/moonset/phase from met.no Sunrise API)
struct MoonData {
    unsigned long moonriseTime;     // Unix timestamp
    unsigned long moonsetTime;      // Unix timestamp
    float moonPhase;                // 0.0-1.0 (0=new, 0.25=first quarter, 0.5=full, 0.75=last quarter)
    bool valid;                     // Data validity flag

    MoonData() : moonriseTime(0), moonsetTime(0), moonPhase(0), valid(false) {}
};

// Complete dashboard data (combination of Netatmo + MeteoSwiss + Sun/Moon)
struct DashboardData {
    WeatherData weather;
    ForecastData forecast;
    SunData sunData;            // Sun data (sunrise/sunset)
    MoonData moonData;          // Moon data (moonrise/moonset/phase)
    uint32_t batteryVoltage;    // mV
    uint8_t batteryPercent;     // %
    unsigned long updateTime;   // Unix timestamp of dashboard update

    DashboardData() : batteryVoltage(0), batteryPercent(0), updateTime(0) {}
};

// Helper function to convert trend string to enum
inline Trend stringToTrend(const char* trendStr) {
    if (strcmp(trendStr, "up") == 0) return Trend::UP;
    if (strcmp(trendStr, "down") == 0) return Trend::DOWN;
    if (strcmp(trendStr, "stable") == 0) return Trend::STABLE;
    return Trend::UNKNOWN;
}

// Helper function to convert trend enum to symbol
inline const char* trendToSymbol(Trend trend) {
    switch (trend) {
        case Trend::UP: return "↑";
        case Trend::DOWN: return "↓";
        case Trend::STABLE: return "→";
        default: return "?";
    }
}

// Helper function to get CO2 quality level (German)
inline const char* getCO2Quality(uint16_t co2) {
    if (co2 > 1500) return "De Vater isch am Lufte";
    if (co2 < 800) return "Ausgezeichnet";
    if (co2 < 1000) return "Gut";
    if (co2 < 1500) return "Mittel";
    if (co2 < 2000) return "Schlecht";
    return "Sehr schlecht";
}

// Helper function to get humidity comfort level (German)
inline const char* getHumidityComfort(uint8_t humidity) {
    if (humidity < 30) return "Zu trocken";
    if (humidity <= 60) return "Optimal";
    if (humidity <= 70) return "Feucht";
    return "Zu feucht";
}

// Helper function to convert symbol code enum to icon name
inline const char* getIconFromCode(uint8_t symbolCode) {
    switch (symbolCode) {
        case 0: return "sunny";
        case 1: return "partly_cloudy";
        case 2: return "cloudy";
        case 3: return "rain";
        case 4: return "snow";
        default: return "cloudy";
    }
}

#endif  // WEATHER_DATA_H
