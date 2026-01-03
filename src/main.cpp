#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "TFT_eSPI.h"
#include "config.h"

// API clients
#include "api/netatmo_client.h"
#include "api/meteo_client.h"
#include "api/gemini_client.h"

// Display
#include "display/widgets.h"
#include "display/fonts.h"

// Data
#include "data/weather_data.h"
#include "data/cache.h"

// Power management
#include "power/sleep_manager.h"
#include "power/battery.h"

// Global objects
#ifdef EPAPER_ENABLE
EPaper display;
#else
TFT_eSprite display = TFT_eSprite(&tft);
#endif

NetatmoClient netatmoClient;
MeteoClient meteoClient;
GeminiClient geminiClient;

// Function prototypes
bool connectWiFi();
void disconnectWiFi();
bool syncTime();
bool fetchWeatherData(DashboardData& data);
void updateDisplay(const DashboardData& data);
void scheduleNextWake(unsigned long netatmoLastUpdate);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);  // Wait for serial to stabilize

    ESP_LOGI("main", "=== ESP32 Weather Dashboard Starting ===");
    ESP_LOGI("main", "Build date: %s %s", __DATE__, __TIME__);

    // Initialize sleep manager (disables watchdog)
    SleepManager::init();
    SleepManager::incrementWakeCount();

    // Battery monitoring disabled (ESP32-C3 has no built-in battery ADC)
    // Battery::init();

    // Initialize LittleFS for caching
    if (!DataCache::init()) {
        ESP_LOGE("main", "Failed to initialize cache");
    }

    // Configure timezone for Switzerland (must be done on every boot as it doesn't persist)
    // POSIX TZ string: CET-1CEST,M3.5.0,M10.5.0/3
    // - CET-1: Central European Time, UTC+1
    // - CEST: Central European Summer Time
    // - M3.5.0: DST starts last Sunday in March at 02:00
    // - M10.5.0/3: DST ends last Sunday in October at 03:00
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    ESP_LOGI("main", "Timezone configured: CET/CEST");

    // Show splash screen only on first boot
    if (SleepManager::getWakeCount() == 1) {
        ESP_LOGI("main", "First boot - showing splash screen");
        display.begin();
        display.setRotation(0);  // Landscape orientation
        display.fillScreen(TFT_WHITE);

        display.setTextColor(TFT_BLACK, TFT_WHITE);
        display.setTextDatum(MC_DATUM);  // Middle-center for centered text

        // Title - Large and bold
        display.setFreeFont(FSSB24);  // FreeSans Bold 24pt
        display.drawString("Wetter Dashboard", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 60);

        // Subtitle - Medium
        display.setFreeFont(FSS18);  // FreeSans 18pt
        display.drawString(LOCATION_NAME, SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 10);

        // Status - Small
        display.setFreeFont(FSS12);  // FreeSans 12pt
        display.drawString("Wettergoetter werden konsultiert...", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 40);

        // Info - Tiny
        display.setFreeFont(FSS9);  // FreeSans 9pt
        display.drawString("(Erstmalige Initialisierung)", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 80);

        display.update();

        delay(1000);
    } else {
        ESP_LOGI("main", "Wake #%d - fetching data in background", SleepManager::getWakeCount());
        // Still need to initialize display hardware (but don't show splash)
        display.begin();
        display.setRotation(0);
    }

    // Prepare dashboard data
    DashboardData dashboardData;
    bool dataAvailable = false;

    // Connect WiFi
    if (connectWiFi()) {
        // Sync time if needed
        if (SleepManager::shouldSyncTime()) {
            if (syncTime()) {
                ESP_LOGI("main", "Time synchronized successfully");
            } else {
                ESP_LOGW("main", "Time sync failed, using RTC time");
            }
        }

        // Fetch fresh weather data
        if (fetchWeatherData(dashboardData)) {
            ESP_LOGI("main", "Weather data fetched successfully");

            // Save to cache for offline use
            DataCache::save(dashboardData);
            dataAvailable = true;
            SleepManager::setLastUpdateSuccess(true);
        } else {
            ESP_LOGE("main", "Failed to fetch weather data");
            SleepManager::setLastUpdateSuccess(false);
        }
    } else {
        ESP_LOGE("main", "WiFi connection failed");
    }

    // CRITICAL: Disconnect WiFi before display operations
    disconnectWiFi();

    // If fresh data not available, try to load from cache
    if (!dataAvailable) {
        ESP_LOGI("main", "Attempting to load cached data");
        if (DataCache::load(dashboardData)) {
            ESP_LOGI("main", "Loaded data from cache (age: %lu sec)",
                    DataCache::getAgeSeconds());
            dataAvailable = true;
        }
    }

    // Battery reading disabled (ESP32-C3 has no built-in battery ADC)
    // Battery::read(dashboardData.batteryVoltage, dashboardData.batteryPercent);
    dashboardData.updateTime = SleepManager::getEpoch();

    // Update display
    if (dataAvailable) {
        updateDisplay(dashboardData);
    } else {
        // Show error screen
        ESP_LOGE("main", "No data available (fresh or cached)");
        display.begin();  // Re-init required
        display.setRotation(0);
        display.fillScreen(TFT_WHITE);
        display.setTextColor(TFT_BLACK, TFT_WHITE);
        display.setTextDatum(MC_DATUM);

        // Error message with FreeFonts
        display.setFreeFont(FSSB24);  // Large bold
        display.drawString("FEHLER", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 50);

        display.setFreeFont(FSS18);  // Medium
        display.drawString("Keine Daten verfügbar", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 10);

        display.setFreeFont(FSS12);  // Small
        display.drawString("WiFi und API prüfen", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 50);

        display.update();
    }

    // Schedule next wake and enter deep sleep (pass Netatmo timestamp for smart scheduling)
    scheduleNextWake(dashboardData.weather.timestamp);
}

void loop() {
    // Should never reach here (deep sleep in setup)
    ESP_LOGE("main", "ERROR: Reached loop() - this should not happen");
    delay(60000);
}

bool connectWiFi() {
    ESP_LOGI("wifi", "Connecting to WiFi: %s", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_TIMEOUT_MS) {
            ESP_LOGE("wifi", "Connection timeout");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    ESP_LOGI("wifi", "Connected! IP: %s", WiFi.localIP().toString().c_str());
    ESP_LOGI("wifi", "RSSI: %d dBm", WiFi.RSSI());

    return true;
}

void disconnectWiFi() {
    ESP_LOGI("wifi", "Disconnecting WiFi");
    WiFi.disconnect(true, true);  // Disconnect and power off radio
    WiFi.mode(WIFI_OFF);
    delay(500);  // Wait for full shutdown
}

bool syncTime() {
    ESP_LOGI("time", "Syncing time via NTP");

    // Configure NTP servers (timezone is already set in setup())
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);

    // CRITICAL: Re-apply timezone after configTime() as it resets timezone to UTC
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // Wait for time sync (max 15 seconds)
    int retries = 30;
    while (retries > 0) {
        time_t now = time(nullptr);
        if (now > 1700000000) {  // Year > 2023
            struct tm* timeinfo = gmtime(&now);
            ESP_LOGI("time", "Time synced: %04d-%02d-%02d %02d:%02d:%02d UTC",
                    timeinfo->tm_year + 1900,
                    timeinfo->tm_mon + 1,
                    timeinfo->tm_mday,
                    timeinfo->tm_hour,
                    timeinfo->tm_min,
                    timeinfo->tm_sec);

            SleepManager::setEpoch(now);
            return true;
        }

        delay(500);
        retries--;
    }

    ESP_LOGE("time", "NTP sync timeout");
    return false;
}

bool fetchWeatherData(DashboardData& data) {
    ESP_LOGI("main", "Fetching weather data from APIs");

    bool success = true;

    // Fetch Netatmo data
    if (!netatmoClient.getWeatherData(data.weather)) {
        ESP_LOGE("main", "Failed to fetch Netatmo data");
        success = false;
    }

    // Fetch MeteoSwiss forecast
    if (!meteoClient.getForecast(data.forecast)) {
        ESP_LOGE("main", "Failed to fetch forecast data");
        success = false;
    }

    // Generate Gemini AI commentary (only if we have weather data)
    if (data.weather.indoor.valid || data.weather.outdoor.valid) {
        unsigned long timestamp = data.weather.timestamp > 0 ?
            data.weather.timestamp : time(nullptr);

        data.aiCommentary = geminiClient.generateCommentary(data.weather, timestamp);

        if (data.aiCommentary.length() == 0) {
            ESP_LOGW("main", "Gemini commentary failed - widget will be empty");
        } else {
            ESP_LOGI("main", "Gemini: %s", data.aiCommentary.c_str());
        }
    }

    // Even if one API fails, we can still show partial data
    return (data.weather.indoor.valid || data.weather.outdoor.valid || data.forecast.current.valid);
}

void updateDisplay(const DashboardData& data) {
    ESP_LOGI("display", "Updating ePaper display");

    // CRITICAL: Re-initialize display before update (Seeed_GFX bug workaround)
    display.begin();
    display.setRotation(0);

    // Draw dashboard
    drawDashboard(display, data);

    // Update display (takes 15-20 seconds)
    ESP_LOGI("display", "Refreshing screen (this takes 15-20 seconds)...");
    unsigned long startTime = millis();
    display.update();
    unsigned long duration = millis() - startTime;
    ESP_LOGI("display", "Screen updated in %lu ms", duration);

    // Put display to sleep
    display.sleep();
}

void scheduleNextWake(unsigned long netatmoLastUpdate) {
    // Calculate next wake time based on Netatmo update cycle
    // Netatmo updates every 10 minutes, we wake 11 minutes after last update

    time_t now = SleepManager::getEpoch();
    unsigned long sleepSeconds = UPDATE_INTERVAL_MIN * 60;  // Fallback: 11 minutes from now
    bool usedNetatmoTimestamp = false;

    // Try to use Netatmo's last update timestamp for smart scheduling
    if (netatmoLastUpdate > 0) {
        // Calculate when we should wake up (11 minutes after Netatmo's last update)
        time_t nextWake = netatmoLastUpdate + (UPDATE_INTERVAL_MIN * 60);

        // Only use this if it's in the future
        if (nextWake > now) {
            sleepSeconds = nextWake - now;
            usedNetatmoTimestamp = true;
            ESP_LOGI("sleep", "Last Netatmo update: %lu (UTC timestamp)", netatmoLastUpdate);
            ESP_LOGI("sleep", "Current time: %lu (UTC)", now);
            ESP_LOGI("sleep", "Next wake scheduled for: %lu (11 min after update)", nextWake);
        } else {
            ESP_LOGW("sleep", "Calculated wake time is in the past (next=%lu, now=%lu)", nextWake, now);
            ESP_LOGI("sleep", "Using fallback: 11 minutes from now");
        }
    } else {
        ESP_LOGW("sleep", "No Netatmo timestamp available");
        ESP_LOGI("sleep", "Using fallback: 11 minutes from now");
    }

    // Clamp sleep duration to safe limits
    if (sleepSeconds < MINIMUM_SLEEP_SEC) {
        ESP_LOGW("sleep", "Sleep time too short (%lu sec), clamping to %d sec",
                sleepSeconds, MINIMUM_SLEEP_SEC);
        sleepSeconds = MINIMUM_SLEEP_SEC;
    }
    if (sleepSeconds > MAXIMUM_SLEEP_SEC) {
        ESP_LOGW("sleep", "Sleep time too long (%lu sec), clamping to %d sec",
                sleepSeconds, MAXIMUM_SLEEP_SEC);
        sleepSeconds = MAXIMUM_SLEEP_SEC;
    }

    ESP_LOGI("main", "=== Update cycle complete ===");
    ESP_LOGI("main", "Sleeping for %lu seconds (%lu minutes)",
            sleepSeconds, sleepSeconds / 60);
    ESP_LOGI("main", "Free heap: %u bytes, Min free: %u bytes",
            ESP.getFreeHeap(), ESP.getMinFreeHeap());

    // Enter deep sleep
    SleepManager::deepSleep(sleepSeconds);
}
