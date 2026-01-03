#include "widgets.h"
#include "layout.h"
#include "fonts.h"
#include "icons.h"
#include "../config.h"
#include <time.h>

// Helper to draw a card border
void drawCard(TFT_eSprite& display, int x, int y) {
    display.drawRect(x, y, CARD_WIDTH, CARD_HEIGHT, TFT_BLACK);
}

// Helper to format timestamp as time (HH:MM)
void formatTime(unsigned long timestamp, char* buffer, size_t bufferSize) {
    if (timestamp == 0) {
        snprintf(buffer, bufferSize, "--:--");
        return;
    }
    time_t t = timestamp;
    struct tm* tm = localtime(&t);
    snprintf(buffer, bufferSize, "%02d:%02d", tm->tm_hour, tm->tm_min);
}

// Helper to draw temperature with degree symbol (drawn as circle)
void drawTemperature(TFT_eSprite& display, float temp, int16_t x, int16_t y,
                     const GFXfont* valueFont, const GFXfont* unitFont, uint8_t datum) {
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f", temp);

    display.setTextDatum(datum);
    display.setFreeFont(valueFont);

    // Draw temperature value
    display.drawString(tempStr, x, y);

    // Measure text width to position degree symbol
    int16_t textWidth = display.textWidth(tempStr);

    // Draw degree symbol as small circle (positioned at top-right of number)
    int16_t degX = x + textWidth + 6;  // 6px spacing
    int16_t degY = y + 3;              // Offset from top
    display.drawCircle(degX, degY, 3, TFT_BLACK);
    display.drawCircle(degX, degY, 2, TFT_BLACK);  // Thicker circle

    // Draw "C" after degree symbol
    display.setFreeFont(unitFont);
    display.drawString("C", degX + 7, y);
}

// Helper to draw trend arrow using geometric shapes
void drawTrendArrow(TFT_eSprite& display, int x, int y, Trend trend) {
    int cx = x + 12;  // Center X
    int cy = y + 12;  // Center Y

    switch (trend) {
        case Trend::UP:
            // Diagonal arrow pointing up-right (↗)
            // Shaft
            display.drawLine(cx - 6, cy + 6, cx + 6, cy - 6, TFT_BLACK);
            display.drawLine(cx - 5, cy + 6, cx + 6, cy - 5, TFT_BLACK);
            display.drawLine(cx - 6, cy + 5, cx + 5, cy - 6, TFT_BLACK);
            // Arrowhead
            display.fillTriangle(cx + 6, cy - 6,
                                cx + 6, cy - 2,
                                cx + 2, cy - 6, TFT_BLACK);
            break;

        case Trend::DOWN:
            // Diagonal arrow pointing down-right (↘)
            // Shaft
            display.drawLine(cx - 6, cy - 6, cx + 6, cy + 6, TFT_BLACK);
            display.drawLine(cx - 5, cy - 6, cx + 6, cy + 5, TFT_BLACK);
            display.drawLine(cx - 6, cy - 5, cx + 5, cy + 6, TFT_BLACK);
            // Arrowhead
            display.fillTriangle(cx + 6, cy + 6,
                                cx + 6, cy + 2,
                                cx + 2, cy + 6, TFT_BLACK);
            break;

        case Trend::STABLE:
            // Horizontal arrow pointing right (→)
            // Shaft
            display.drawLine(cx - 8, cy, cx + 6, cy, TFT_BLACK);
            display.drawLine(cx - 8, cy - 1, cx + 6, cy - 1, TFT_BLACK);
            display.drawLine(cx - 8, cy + 1, cx + 6, cy + 1, TFT_BLACK);
            // Arrowhead
            display.fillTriangle(cx + 6, cy,
                                cx + 2, cy - 4,
                                cx + 2, cy + 4, TFT_BLACK);
            break;

        default:
            break;
    }
}

void drawWeatherIcon(TFT_eSprite& display, int x, int y, const char* iconName, int size) {
    // Center point for the icon
    int cx = x + size / 2;
    int cy = y + size / 2;

    if (strcmp(iconName, "sunny") == 0) {
        // Sun icon with rays
        int sunRadius = size / 6;
        int rayLength = size / 5;

        // Draw sun circle (filled)
        display.fillCircle(cx, cy, sunRadius, TFT_BLACK);

        // Draw 8 rays around the sun
        for (int i = 0; i < 8; i++) {
            float angle = i * PI / 4;
            int innerX = cx + cos(angle) * (sunRadius + 2);
            int innerY = cy + sin(angle) * (sunRadius + 2);
            int outerX = cx + cos(angle) * (sunRadius + rayLength);
            int outerY = cy + sin(angle) * (sunRadius + rayLength);

            // Draw thick rays
            display.drawLine(innerX, innerY, outerX, outerY, TFT_BLACK);
            display.drawLine(innerX + 1, innerY, outerX + 1, outerY, TFT_BLACK);
            display.drawLine(innerX, innerY + 1, outerX, outerY + 1, TFT_BLACK);
        }
    }
    else if (strcmp(iconName, "partly_cloudy") == 0) {
        // Sun behind cloud
        int sunRadius = size / 8;
        int sunX = cx - size / 6;
        int sunY = cy - size / 6;

        // Draw partial sun (upper left)
        display.drawCircle(sunX, sunY, sunRadius, TFT_BLACK);
        display.drawCircle(sunX, sunY, sunRadius - 1, TFT_BLACK);

        // Draw 4 sun rays (only visible ones)
        for (int i = 0; i < 4; i++) {
            float angle = (i * PI / 2) + PI;  // Left and top rays
            int innerX = sunX + cos(angle) * (sunRadius + 1);
            int innerY = sunY + sin(angle) * (sunRadius + 1);
            int outerX = sunX + cos(angle) * (sunRadius + size / 8);
            int outerY = sunY + sin(angle) * (sunRadius + size / 8);
            display.drawLine(innerX, innerY, outerX, outerY, TFT_BLACK);
        }

        // Draw cloud (overlapping sun)
        int cloudY = cy + size / 10;
        // Cloud bumps (3 circles)
        display.fillCircle(cx - size / 8, cloudY, size / 8, TFT_BLACK);
        display.fillCircle(cx, cloudY - size / 12, size / 7, TFT_BLACK);
        display.fillCircle(cx + size / 8, cloudY, size / 8, TFT_BLACK);
        // Cloud base
        display.fillRect(cx - size / 6, cloudY, size / 3, size / 8, TFT_BLACK);
    }
    else if (strcmp(iconName, "cloudy") == 0) {
        // Cloud icon - three bumps
        int cloudY = cy;

        // Draw three overlapping circles for cloud bumps
        display.fillCircle(cx - size / 6, cloudY, size / 7, TFT_BLACK);
        display.fillCircle(cx, cloudY - size / 10, size / 6, TFT_BLACK);
        display.fillCircle(cx + size / 6, cloudY, size / 7, TFT_BLACK);

        // Cloud base rectangle
        display.fillRect(cx - size / 5, cloudY, size / 2.5, size / 7, TFT_BLACK);
    }
    else if (strcmp(iconName, "rain") == 0) {
        // Cloud with rain drops
        int cloudY = cy - size / 8;

        // Draw cloud
        display.fillCircle(cx - size / 8, cloudY, size / 9, TFT_BLACK);
        display.fillCircle(cx, cloudY - size / 12, size / 8, TFT_BLACK);
        display.fillCircle(cx + size / 8, cloudY, size / 9, TFT_BLACK);
        display.fillRect(cx - size / 7, cloudY, size / 3.5, size / 9, TFT_BLACK);

        // Draw rain drops (5 drops)
        int dropY = cy + size / 6;
        for (int i = 0; i < 5; i++) {
            int dropX = cx - size / 5 + (i * size / 10);
            // Draw thick rain lines
            display.drawLine(dropX, dropY, dropX, dropY + size / 8, TFT_BLACK);
            display.drawLine(dropX + 1, dropY, dropX + 1, dropY + size / 8, TFT_BLACK);
        }
    }
    else if (strcmp(iconName, "snow") == 0) {
        // Cloud with snowflakes
        int cloudY = cy - size / 8;

        // Draw cloud
        display.fillCircle(cx - size / 8, cloudY, size / 9, TFT_BLACK);
        display.fillCircle(cx, cloudY - size / 12, size / 8, TFT_BLACK);
        display.fillCircle(cx + size / 8, cloudY, size / 9, TFT_BLACK);
        display.fillRect(cx - size / 7, cloudY, size / 3.5, size / 9, TFT_BLACK);

        // Draw snowflakes (asterisks)
        int flakeY = cy + size / 6;
        int flakeSize = size / 12;

        for (int i = 0; i < 3; i++) {
            int flakeX = cx - size / 6 + (i * size / 6);
            // Vertical line
            display.drawLine(flakeX, flakeY - flakeSize, flakeX, flakeY + flakeSize, TFT_BLACK);
            // Horizontal line
            display.drawLine(flakeX - flakeSize, flakeY, flakeX + flakeSize, flakeY, TFT_BLACK);
            // Diagonal lines
            display.drawLine(flakeX - flakeSize/1.5, flakeY - flakeSize/1.5,
                           flakeX + flakeSize/1.5, flakeY + flakeSize/1.5, TFT_BLACK);
            display.drawLine(flakeX - flakeSize/1.5, flakeY + flakeSize/1.5,
                           flakeX + flakeSize/1.5, flakeY - flakeSize/1.5, TFT_BLACK);
        }
    }
}

void drawHeader(TFT_eSprite& display, const char* location, unsigned long timestamp) {
    // Draw header background
    display.fillRect(0, HEADER_Y, SCREEN_WIDTH, HEADER_HEIGHT, TFT_WHITE);
    display.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, TFT_BLACK);

    // Location on left
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS12);
    display.drawString(location, MARGIN, HEADER_Y + 8);

    // Date/time on right (same size as location)
    if (timestamp > 0) {
        time_t t = timestamp;
        struct tm* tm = localtime(&t);
        char dateStr[32];
        snprintf(dateStr, sizeof(dateStr), "%02d.%02d.%04d %02d:%02d",
                tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                tm->tm_hour, tm->tm_min);
        display.setTextDatum(TR_DATUM);
        display.setFreeFont(FSS12);  // Match location font size
        display.drawString(dateStr, SCREEN_WIDTH - MARGIN, HEADER_Y + 8);
    }
}

void drawIndoorTempWidget(TFT_eSprite& display, const IndoorData& data) {
    if (!data.valid) return;

    display.drawRect(INDOOR_TEMP_X, INDOOR_TEMP_Y, CARD_WIDTH, TEMP_CARD_HEIGHT, TFT_BLACK);

    display.setTextColor(TFT_BLACK, TFT_WHITE);

    // Label
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Innen", INDOOR_TEMP_X + CARD_PADDING, INDOOR_TEMP_Y + CARD_LABEL_Y);

    // Large temperature value with degree symbol
    drawTemperature(display, data.temperature,
                    INDOOR_TEMP_X + CARD_PADDING,
                    INDOOR_TEMP_Y + CARD_VALUE_Y,
                    FSSB18, FSS12, TL_DATUM);

    // Trend arrow
    drawTrendArrow(display, INDOOR_TEMP_X + CARD_TREND_X_OFFSET, INDOOR_TEMP_Y + CARD_TREND_Y, data.temperatureTrend);

    // Min/max detail with custom degree symbols (two lines)
    if (data.minTemp != 0 || data.maxTemp != 0) {
        display.setTextDatum(TL_DATUM);
        display.setFreeFont(FSS9);
        int y = INDOOR_TEMP_Y + CARD_DETAIL_Y;
        int pipeX = INDOOR_TEMP_X + 120;  // Fixed position for pipe alignment

        // Line 1: min
        int x = INDOOR_TEMP_X + CARD_PADDING;
        display.setFreeFont(FSS9);
        display.drawString("min  ", x, y);
        x += display.textWidth("min  ");

        // Temperature value in bold
        display.setFreeFont(FSSB9);
        char tempStr[10];
        snprintf(tempStr, sizeof(tempStr), "%.1f", data.minTemp);
        display.drawString(tempStr, x, y);
        x += display.textWidth(tempStr) + 5;

        // Degree symbol
        display.drawCircle(x, y + 3, 2, TFT_BLACK);
        display.drawCircle(x, y + 3, 3, TFT_BLACK);
        x += 5;

        display.setFreeFont(FSS9);
        display.drawString("C  ", x, y);
        x += display.textWidth("C  ");

        // Min time
        char timeStr[16];
        formatTime(data.dateMinTemp, timeStr, sizeof(timeStr));
        display.drawString(timeStr, x, y);
        display.drawString(" Uhr", x + display.textWidth("00:00"), y);

        // Line 2: max
        y += 18;  // Move to next line
        x = INDOOR_TEMP_X + CARD_PADDING;
        display.setFreeFont(FSS9);
        display.drawString("max  ", x, y);
        x += display.textWidth("max  ");

        // Temperature value in bold
        display.setFreeFont(FSSB9);
        snprintf(tempStr, sizeof(tempStr), "%.1f", data.maxTemp);
        display.drawString(tempStr, x, y);
        x += display.textWidth(tempStr) + 5;

        // Degree symbol
        display.drawCircle(x, y + 3, 2, TFT_BLACK);
        display.drawCircle(x, y + 3, 3, TFT_BLACK);
        x += 5;

        display.setFreeFont(FSS9);
        display.drawString("C  ", x, y);
        x += display.textWidth("C  ");

        // Max time
        formatTime(data.dateMaxTemp, timeStr, sizeof(timeStr));
        display.drawString(timeStr, x, y);
        display.drawString(" Uhr", x + display.textWidth("00:00"), y);
    }
}

void drawOutdoorTempWidget(TFT_eSprite& display, const OutdoorData& data) {
    display.drawRect(OUTDOOR_TEMP_X, OUTDOOR_TEMP_Y, CARD_WIDTH, TEMP_CARD_HEIGHT, TFT_BLACK);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Aussen", OUTDOOR_TEMP_X + CARD_PADDING, OUTDOOR_TEMP_Y + CARD_LABEL_Y);

    if (!data.valid) {
        display.setFreeFont(FSSB18);
        display.drawString("n/a", OUTDOOR_TEMP_X + CARD_PADDING, OUTDOOR_TEMP_Y + CARD_VALUE_Y);
        return;
    }

    drawTemperature(display, data.temperature,
                    OUTDOOR_TEMP_X + CARD_PADDING,
                    OUTDOOR_TEMP_Y + CARD_VALUE_Y,
                    FSSB18, FSS12, TL_DATUM);

    drawTrendArrow(display, OUTDOOR_TEMP_X + CARD_TREND_X_OFFSET, OUTDOOR_TEMP_Y + CARD_TREND_Y, data.temperatureTrend);

    // Min/max detail with custom degree symbols (two lines)
    if (data.minTemp != 0 || data.maxTemp != 0) {
        display.setTextDatum(TL_DATUM);
        display.setFreeFont(FSS9);
        int y = OUTDOOR_TEMP_Y + CARD_DETAIL_Y;
        int pipeX = OUTDOOR_TEMP_X + 120;  // Fixed position for pipe alignment

        // Line 1: min
        int x = OUTDOOR_TEMP_X + CARD_PADDING;
        display.setFreeFont(FSS9);
        display.drawString("min  ", x, y);
        x += display.textWidth("min  ");

        // Temperature value in bold
        display.setFreeFont(FSSB9);
        char tempStr[10];
        snprintf(tempStr, sizeof(tempStr), "%.1f", data.minTemp);
        display.drawString(tempStr, x, y);
        x += display.textWidth(tempStr) + 5;

        // Degree symbol
        display.drawCircle(x, y + 3, 2, TFT_BLACK);
        display.drawCircle(x, y + 3, 3, TFT_BLACK);
        x += 5;

        display.setFreeFont(FSS9);
        display.drawString("C  ", x, y);
        x += display.textWidth("C  ");

        // Min time
        char timeStr[16];
        formatTime(data.dateMinTemp, timeStr, sizeof(timeStr));
        display.drawString(timeStr, x, y);
        display.drawString(" Uhr", x + display.textWidth("00:00"), y);

        // Line 2: max
        y += 18;  // Move to next line
        x = OUTDOOR_TEMP_X + CARD_PADDING;
        display.setFreeFont(FSS9);
        display.drawString("max  ", x, y);
        x += display.textWidth("max  ");

        // Temperature value in bold
        display.setFreeFont(FSSB9);
        snprintf(tempStr, sizeof(tempStr), "%.1f", data.maxTemp);
        display.drawString(tempStr, x, y);
        x += display.textWidth(tempStr) + 5;

        // Degree symbol
        display.drawCircle(x, y + 3, 2, TFT_BLACK);
        display.drawCircle(x, y + 3, 3, TFT_BLACK);
        x += 5;

        display.setFreeFont(FSS9);
        display.drawString("C  ", x, y);
        x += display.textWidth("C  ");

        // Max time
        formatTime(data.dateMaxTemp, timeStr, sizeof(timeStr));
        display.drawString(timeStr, x, y);
        display.drawString(" Uhr", x + display.textWidth("00:00"), y);
    }
}

void drawIndoorHumidWidget(TFT_eSprite& display, const IndoorData& data) {
    if (!data.valid) return;

    drawCard(display, INDOOR_HUMID_X, INDOOR_HUMID_Y);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Innen", INDOOR_HUMID_X + CARD_PADDING, INDOOR_HUMID_Y + CARD_LABEL_Y);

    // Draw humidity value (bold) and unit (non-bold) separately
    char humidStr[16];
    snprintf(humidStr, sizeof(humidStr), "%d", data.humidity);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    int valueX = INDOOR_HUMID_X + CARD_PADDING;
    int valueY = INDOOR_HUMID_Y + CARD_VALUE_Y;
    display.drawString(humidStr, valueX, valueY);

    // Draw unit in non-bold (measure width before font switch)
    int valueWidth = display.textWidth(humidStr);
    display.setFreeFont(FSS18);  // Use FSS18 to match size of FSSB18
    int unitX = valueX + valueWidth + 4;
    display.drawString("%", unitX, valueY);

    // Climate status
    const char* status = getHumidityComfort(data.humidity);
    char detailStr[32];
    snprintf(detailStr, sizeof(detailStr), "Raumklima: %s", status);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(detailStr, INDOOR_HUMID_X + CARD_PADDING, INDOOR_HUMID_Y + CARD_DETAIL_Y);
}

void drawOutdoorHumidWidget(TFT_eSprite& display, const OutdoorData& data) {
    drawCard(display, OUTDOOR_HUMID_X, OUTDOOR_HUMID_Y);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Aussen", OUTDOOR_HUMID_X + CARD_PADDING, OUTDOOR_HUMID_Y + CARD_LABEL_Y);

    if (!data.valid) {
        display.setFreeFont(FSSB18);
        display.drawString("n/a", OUTDOOR_HUMID_X + CARD_PADDING, OUTDOOR_HUMID_Y + CARD_VALUE_Y);
        return;
    }

    // Draw humidity value (bold) and unit (non-bold) separately
    char humidStr[16];
    snprintf(humidStr, sizeof(humidStr), "%d", data.humidity);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    int valueX = OUTDOOR_HUMID_X + CARD_PADDING;
    int valueY = OUTDOOR_HUMID_Y + CARD_VALUE_Y;
    display.drawString(humidStr, valueX, valueY);

    // Draw unit in non-bold (measure width before font switch)
    int valueWidth = display.textWidth(humidStr);
    display.setFreeFont(FSS18);  // Use FSS18 to match size of FSSB18
    int unitX = valueX + valueWidth + 4;
    display.drawString("%", unitX, valueY);

    // Dew point calculation (Magnus formula)
    float a = 17.27;
    float b = 237.7;
    float alpha = ((a * data.temperature) / (b + data.temperature)) + log(data.humidity / 100.0);
    float dewPoint = (b * alpha) / (a - alpha);

    // Draw dew point with custom degree symbol (value bold, unit non-bold)
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    int dewX = OUTDOOR_HUMID_X + CARD_PADDING;
    int dewY = OUTDOOR_HUMID_Y + CARD_DETAIL_Y;

    // Draw "Taupunkt: " in non-bold
    display.drawString("Taupunkt: ", dewX, dewY);
    dewX += display.textWidth("Taupunkt: ");

    // Draw temperature value in bold
    char dewTempStr[10];
    snprintf(dewTempStr, sizeof(dewTempStr), "%.1f", dewPoint);
    display.setFreeFont(FSSB9);
    display.drawString(dewTempStr, dewX, dewY);
    dewX += display.textWidth(dewTempStr) + 5;

    // Draw degree symbol (circle)
    display.drawCircle(dewX, dewY + 3, 2, TFT_BLACK);
    display.drawCircle(dewX, dewY + 3, 3, TFT_BLACK);
    dewX += 5;

    // Draw "C" in non-bold
    display.setFreeFont(FSS9);
    display.drawString("C", dewX, dewY);
}

void drawAirQualityWidget(TFT_eSprite& display, const IndoorData& data) {
    if (!data.valid) return;

    display.drawRect(AIR_QUALITY_X, AIR_QUALITY_Y, CARD_WIDTH, AIR_QUALITY_CARD_HEIGHT, TFT_BLACK);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Luftqualitaet innen", AIR_QUALITY_X + CARD_PADDING, AIR_QUALITY_Y + CARD_LABEL_Y);

    // Draw CO2 value (bold) and unit (non-bold) separately
    char co2Str[16];
    snprintf(co2Str, sizeof(co2Str), "%d", data.co2);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    int valueX = AIR_QUALITY_X + CARD_PADDING;
    int valueY = AIR_QUALITY_Y + CARD_VALUE_Y;
    display.drawString(co2Str, valueX, valueY);

    // Draw unit in non-bold (measure width before font switch)
    int valueWidth = display.textWidth(co2Str);
    display.setFreeFont(FSS18);  // Use FSS18 to match size of FSSB18
    int unitX = valueX + valueWidth + 4;
    display.drawString("ppm", unitX, valueY);

    drawTrendArrow(display, AIR_QUALITY_X + CARD_TREND_X_OFFSET, AIR_QUALITY_Y + CARD_TREND_Y, data.co2Trend);
}

void drawPressureWidget(TFT_eSprite& display, const IndoorData& data) {
    if (!data.valid) return;

    display.drawRect(PRESSURE_X, PRESSURE_Y, CARD_WIDTH, PRESSURE_CARD_HEIGHT, TFT_BLACK);

    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Luftdruck aussen", PRESSURE_X + CARD_PADDING, PRESSURE_Y + CARD_LABEL_Y);

    // Draw pressure value (bold) and unit (non-bold) separately
    char pressStr[16];
    snprintf(pressStr, sizeof(pressStr), "%d", data.pressure);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    int valueX = PRESSURE_X + CARD_PADDING;
    int valueY = PRESSURE_Y + CARD_VALUE_Y;
    display.drawString(pressStr, valueX, valueY);

    // Draw unit in non-bold (measure width before font switch)
    int valueWidth = display.textWidth(pressStr);
    display.setFreeFont(FSS18);  // Use FSS18 to match size of FSSB18
    int unitX = valueX + valueWidth + 4;
    display.drawString("hPa", unitX, valueY);

    drawTrendArrow(display, PRESSURE_X + CARD_TREND_X_OFFSET, PRESSURE_Y + CARD_TREND_Y, data.pressureTrend);
}

// Legacy forecast widgets (deprecated - now using 3-day forecast column)
// Kept for backward compatibility but not used in 3-column layout
void drawForecast3hWidget(TFT_eSprite& display, const ForecastPoint& forecast) {
    // No longer used in 3-column layout
}

void drawForecast6hWidget(TFT_eSprite& display, const ForecastPoint& forecast) {
    // No longer used in 3-column layout
}

// Helper: Draw moon phase visual
void drawMoonPhase(TFT_eSprite& display, int x, int y, float phase, int size) {
    // Moon phase: 0.0-1.0
    // 0.0 = new moon (dark)
    // 0.25 = first quarter (right half lit)
    // 0.5 = full moon (fully lit)
    // 0.75 = last quarter (left half lit)
    // 1.0 = new moon again

    int radius = size / 2;
    int cx = x + radius;
    int cy = y + radius;

    // Draw outer circle (moon outline)
    display.drawCircle(cx, cy, radius, TFT_BLACK);
    display.drawCircle(cx, cy, radius - 1, TFT_BLACK);  // Thicker outline

    // Fill moon based on phase
    if (phase < 0.01) {
        // New moon - fill entire circle (dark)
        display.fillCircle(cx, cy, radius - 2, TFT_BLACK);
    } else if (phase > 0.99) {
        // New moon (cycle complete) - fill entire circle (dark)
        display.fillCircle(cx, cy, radius - 2, TFT_BLACK);
    } else if (phase > 0.48 && phase < 0.52) {
        // Full moon - leave empty (white)
        // Circle outline already drawn
    } else if (phase < 0.5) {
        // Waxing (0.0 to 0.5) - fill from right side
        // Draw filled semi-circle on right, gradually revealing left
        float fillRatio = phase * 2.0;  // 0.0 to 1.0
        int fillWidth = radius * 2 * (1.0 - fillRatio);

        // Fill right side gradually
        for (int dy = -radius + 2; dy < radius - 2; dy++) {
            int lineWidth = sqrt(radius * radius - dy * dy);
            int startX = cx - (int)(lineWidth * (1.0 - fillRatio));
            display.drawFastHLine(startX, cy + dy, cx + lineWidth - startX, TFT_BLACK);
        }
    } else {
        // Waning (0.5 to 1.0) - fill from left side
        // Gradually fill left side
        float fillRatio = (phase - 0.5) * 2.0;  // 0.0 to 1.0
        int fillWidth = radius * 2 * fillRatio;

        // Fill left side gradually
        for (int dy = -radius + 2; dy < radius - 2; dy++) {
            int lineWidth = sqrt(radius * radius - dy * dy);
            int endX = cx + (int)(lineWidth * (1.0 - fillRatio));
            display.drawFastHLine(cx - lineWidth, cy + dy, endX - (cx - lineWidth), TFT_BLACK);
        }
    }
}

// Sun Widget: Combined sunrise/sunset in one widget (256×85px)
// Sunrise Widget (256×82px)
void drawSunriseWidget(TFT_eSprite& display, const SunData& data) {
    // Draw card border
    display.drawRect(SUN_X, SUN_Y, CARD_WIDTH, SUN_CARD_HEIGHT, TFT_BLACK);

    if (!data.valid || data.sunriseTime == 0) {
        // Label without date if invalid
        display.setTextColor(TFT_BLACK, TFT_WHITE);
        display.setTextDatum(TL_DATUM);
        display.setFreeFont(FSS9);
        display.drawString("Sonnenaufgang", SUN_X + CARD_PADDING, SUN_Y + CARD_LABEL_Y);

        display.setFreeFont(FSSB18);
        display.drawString("n/a", SUN_X + CARD_PADDING, SUN_Y + CARD_VALUE_Y);
        return;
    }

    // Extract time and date
    struct tm sunriseTm_buf;
    struct tm* sunriseTm = localtime((time_t*)&data.sunriseTime);
    if (sunriseTm) sunriseTm_buf = *sunriseTm;

    // Label "Sonnenaufgang dd.mm.yyyy"
    char labelStr[32];
    snprintf(labelStr, sizeof(labelStr), "Sonnenaufgang %02d.%02d.%04d",
             sunriseTm_buf.tm_mday, sunriseTm_buf.tm_mon + 1, sunriseTm_buf.tm_year + 1900);
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(labelStr, SUN_X + CARD_PADDING, SUN_Y + CARD_LABEL_Y);

    // Vertical up arrow for sunrise (similar to trend arrows)
    int arrowX = SUN_X + CARD_PADDING + 12;
    int arrowY = SUN_Y + 35;
    int cx = arrowX;
    int cy = arrowY;
    // Shaft (vertical line, thicker)
    display.drawLine(cx, cy + 6, cx, cy - 6, TFT_BLACK);
    display.drawLine(cx - 1, cy + 6, cx - 1, cy - 6, TFT_BLACK);
    display.drawLine(cx + 1, cy + 6, cx + 1, cy - 6, TFT_BLACK);
    // Arrowhead (pointing up)
    display.fillTriangle(cx, cy - 6,
                        cx - 4, cy - 2,
                        cx + 4, cy - 2, TFT_BLACK);

    // Sunrise time (bold, large, left-aligned)
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", sunriseTm_buf.tm_hour, sunriseTm_buf.tm_min);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    display.drawString(timeStr, arrowX + 20, SUN_Y + 25);

    // Azimuth (small)
    char azStr[16];
    snprintf(azStr, sizeof(azStr), "Azimut: %.0f", data.sunriseAzimuth);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(azStr, SUN_X + CARD_PADDING, SUN_Y + 62);
}

// Sunset Widget (256×82px)
void drawSunsetWidget(TFT_eSprite& display, const SunData& data) {
    // Draw card border
    display.drawRect(MOON_X, MOON_Y, CARD_WIDTH, MOON_CARD_HEIGHT, TFT_BLACK);

    if (!data.valid || data.sunsetTime == 0) {
        // Label without date if invalid
        display.setTextColor(TFT_BLACK, TFT_WHITE);
        display.setTextDatum(TL_DATUM);
        display.setFreeFont(FSS9);
        display.drawString("Sonnenuntergang", MOON_X + CARD_PADDING, MOON_Y + CARD_LABEL_Y);

        display.setFreeFont(FSSB18);
        display.drawString("n/a", MOON_X + CARD_PADDING, MOON_Y + CARD_VALUE_Y);
        return;
    }

    // Extract time and date
    struct tm sunsetTm_buf;
    struct tm* sunsetTm = localtime((time_t*)&data.sunsetTime);
    if (sunsetTm) sunsetTm_buf = *sunsetTm;

    // Label "Sonnenuntergang dd.mm.yyyy"
    char labelStr[32];
    snprintf(labelStr, sizeof(labelStr), "Sonnenuntergang %02d.%02d.%04d",
             sunsetTm_buf.tm_mday, sunsetTm_buf.tm_mon + 1, sunsetTm_buf.tm_year + 1900);
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(labelStr, MOON_X + CARD_PADDING, MOON_Y + CARD_LABEL_Y);

    // Vertical down arrow for sunset (similar to trend arrows)
    int arrowX = MOON_X + CARD_PADDING + 12;
    int arrowY = MOON_Y + 35;
    int cx = arrowX;
    int cy = arrowY;
    // Shaft (vertical line, thicker)
    display.drawLine(cx, cy - 6, cx, cy + 6, TFT_BLACK);
    display.drawLine(cx - 1, cy - 6, cx - 1, cy + 6, TFT_BLACK);
    display.drawLine(cx + 1, cy - 6, cx + 1, cy + 6, TFT_BLACK);
    // Arrowhead (pointing down)
    display.fillTriangle(cx, cy + 6,
                        cx - 4, cy + 2,
                        cx + 4, cy + 2, TFT_BLACK);

    // Sunset time (bold, large, left-aligned)
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", sunsetTm_buf.tm_hour, sunsetTm_buf.tm_min);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSSB18);
    display.drawString(timeStr, arrowX + 20, MOON_Y + 25);

    // Azimuth (small)
    char azStr[16];
    snprintf(azStr, sizeof(azStr), "Azimut: %.0f", data.sunsetAzimuth);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(azStr, MOON_X + CARD_PADDING, MOON_Y + 62);
}

// Moon Widget: Visual phase + moonrise/moonset (256×82px)
void drawMoonWidget(TFT_eSprite& display, const MoonData& data) {
    // Draw card border
    display.drawRect(MOON_X, MOON_Y, CARD_WIDTH, MOON_CARD_HEIGHT, TFT_BLACK);

    // Label "MOND"
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("MOND", MOON_X + CARD_PADDING, MOON_Y + CARD_LABEL_Y);

    if (!data.valid) {
        display.setFreeFont(FSSB18);
        display.drawString("n/a", MOON_X + CARD_PADDING, MOON_Y + CARD_VALUE_Y);
        return;
    }

    // Moon phase percentage (top-right)
    char phaseStr[16];
    snprintf(phaseStr, sizeof(phaseStr), "Phase: %d%%", (int)(data.moonPhase * 100));
    display.setTextDatum(TR_DATUM);
    display.setFreeFont(FSS9);
    display.drawString(phaseStr, MOON_X + CARD_WIDTH - CARD_PADDING, MOON_Y + CARD_LABEL_Y);

    // Moon phase visual (40×40px on left)
    int moonIconX = MOON_X + CARD_PADDING;
    int moonIconY = MOON_Y + 28;
    drawMoonPhase(display, moonIconX, moonIconY, data.moonPhase, 40);

    // Moonrise time (center-left with ↑ arrow)
    int riseX = MOON_X + 60;
    int riseY = MOON_Y + 32;

    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Aufgang", riseX, riseY);

    struct tm moonriseTm_buf, moonsetTm_buf;  // Buffers for localtime() copies
    if (data.moonriseTime > 0) {
        struct tm* moonriseTm = localtime((time_t*)&data.moonriseTime);
        if (moonriseTm) moonriseTm_buf = *moonriseTm;  // Copy to local buffer
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", moonriseTm_buf.tm_hour, moonriseTm_buf.tm_min);
        display.setFreeFont(FSSB12);
        display.drawString(timeStr, riseX, riseY + 16);

        // Up arrow
        display.drawString("^", riseX - 12, riseY + 16);
    } else {
        display.setFreeFont(FSSB12);
        display.drawString("--:--", riseX, riseY + 16);
    }

    // Moonset time (center-right with ↓ arrow)
    int setX = MOON_X + 160;
    int setY = MOON_Y + 32;

    display.setTextDatum(TL_DATUM);
    display.setFreeFont(FSS9);
    display.drawString("Untergang", setX, setY);

    if (data.moonsetTime > 0) {
        struct tm* moonsetTm = localtime((time_t*)&data.moonsetTime);
        if (moonsetTm) moonsetTm_buf = *moonsetTm;  // Copy to local buffer
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", moonsetTm_buf.tm_hour, moonsetTm_buf.tm_min);
        display.setFreeFont(FSSB12);
        display.drawString(timeStr, setX, setY + 16);

        // Down arrow
        display.drawString("v", setX - 12, setY + 16);
    } else {
        display.setFreeFont(FSSB12);
        display.drawString("--:--", setX, setY + 16);
    }
}

// Helper: Draw 3-time grid (06h, 12h, 18h)
void drawDayTimeGrid(TFT_eSprite& display, const DayTimeForecast times[3],
                     int x, int y, int width) {
    int cellWidth = width / 3;

    for (int i = 0; i < 3; i++) {
        int cellX = x + (i * cellWidth);
        const DayTimeForecast& dt = times[i];
        bool hasData = (dt.hour != 0);  // hour == 0 means no data

        display.setFreeFont(FSS9);
        display.setTextDatum(TC_DATUM);

        if (hasData) {
            // Weather icon (24×24px)
            const char* icon = getIconFromCode(dt.symbolCode);
            drawWeatherIcon(display, cellX + cellWidth/2 - 12, y, icon, 24);

            // Temperature with custom degree symbol (matching other widgets)
            char tempStr[6];
            snprintf(tempStr, sizeof(tempStr), "%d", dt.temperature);
            int tempTextY = y + 27;

            // Draw temperature number in bold
            display.setFreeFont(FSSB9);
            int tempWidth = display.textWidth(tempStr);
            int startX = cellX + cellWidth/2 - (tempWidth + 12) / 2;  // Center: text + spacing + degree + C
            display.setTextDatum(TL_DATUM);
            display.drawString(tempStr, startX, tempTextY);

            // Draw degree symbol (two circles) with spacing
            int degX = startX + tempWidth + 4;  // Increased spacing from 2 to 4
            display.drawCircle(degX, tempTextY + 3, 2, TFT_BLACK);
            display.drawCircle(degX, tempTextY + 3, 3, TFT_BLACK);

            // Draw "C" in non-bold
            display.setFreeFont(FSS9);
            display.drawString("C", degX + 5, tempTextY);

            // Precipitation with unit (value bold, unit non-bold)
            char precipStr[10];
            snprintf(precipStr, sizeof(precipStr), "%.1f", dt.precipitationMm / 10.0);

            // Calculate centered position for value + space + unit
            display.setFreeFont(FSSB9);
            int precipValueWidth = display.textWidth(precipStr);
            display.setFreeFont(FSS9);
            int spaceWidth = display.textWidth(" ");
            int precipUnitWidth = display.textWidth("mm");
            int totalWidth = precipValueWidth + spaceWidth + precipUnitWidth;
            int precipStartX = cellX + cellWidth/2 - totalWidth/2;

            // Draw value in bold
            display.setFreeFont(FSSB9);
            display.setTextDatum(TL_DATUM);
            display.drawString(precipStr, precipStartX, y + 42);

            // Draw unit in non-bold with space
            display.setFreeFont(FSS9);
            display.drawString(" mm", precipStartX + precipValueWidth, y + 42);
        } else {
            // Show placeholder for missing data
            display.drawString("-", cellX + cellWidth/2, y + 15);
        }
    }
}

// Helper: Draw single daily forecast section
void drawDailyForecastSection(TFT_eSprite& display, const DailyForecast& day,
                               int x, int y, const char* dayLabel, bool isToday) {
    display.setTextColor(TFT_BLACK, TFT_WHITE);
    display.setTextDatum(TL_DATUM);

    // Day header (use FSS9 to match other widget labels like "Innen")
    display.setFreeFont(FSS9);
    display.drawString(dayLabel, x + CARD_PADDING, y + CARD_LABEL_Y);

    if (!day.valid) {
        display.setFreeFont(FSS9);
        display.drawString("n/a", x + CARD_PADDING, y + CARD_VALUE_Y);
        return;
    }

    // Icon size (24×24 for all sections - consistent sizing)
    int iconSize = 24;
    int iconY = y + CARD_VALUE_Y;  // Align with other cards (28px from top)

    // Daily weather icon
    const char* iconName = getIconFromCode(day.symbolCode);
    drawWeatherIcon(display, x + CARD_PADDING, iconY, iconName, iconSize);

    // Temperature range with custom degree symbols (values bold, units non-bold)
    display.setTextDatum(TL_DATUM);
    int tempX = x + CARD_PADDING + iconSize + 8;
    int tempY = iconY + 5;

    // Draw min temperature in bold
    char tempStr[10];
    snprintf(tempStr, sizeof(tempStr), "%d", day.tempMin);
    display.setFreeFont(FSSB12);
    display.drawString(tempStr, tempX, tempY);
    tempX += display.textWidth(tempStr) + 5;

    // Draw degree symbol for min temp
    display.drawCircle(tempX, tempY + 3, 2, TFT_BLACK);
    display.drawCircle(tempX, tempY + 3, 3, TFT_BLACK);
    tempX += 5;

    // Draw "C/" in non-bold
    display.setFreeFont(FSS12);
    display.drawString("C/", tempX, tempY);
    tempX += display.textWidth("C/");

    // Draw max temperature in bold
    snprintf(tempStr, sizeof(tempStr), "%d", day.tempMax);
    display.setFreeFont(FSSB12);
    display.drawString(tempStr, tempX, tempY);
    tempX += display.textWidth(tempStr) + 5;

    // Draw degree symbol for max temp
    display.drawCircle(tempX, tempY + 3, 2, TFT_BLACK);
    display.drawCircle(tempX, tempY + 3, 3, TFT_BLACK);
    tempX += 5;

    // Draw "C" in non-bold
    display.setFreeFont(FSS12);
    display.drawString("C", tempX, tempY);

    // Precipitation (if > 0) - value bold, unit non-bold
    if (day.precipSum > 0) {
        char precipStr[16];
        snprintf(precipStr, sizeof(precipStr), "%.1f", day.precipSum / 10.0);

        // Calculate position for right-aligned text
        display.setFreeFont(FSSB9);
        int valueWidth = display.textWidth(precipStr);
        display.setFreeFont(FSS9);
        int spaceWidth = display.textWidth(" ");
        int unitWidth = display.textWidth("mm");
        int totalWidth = valueWidth + spaceWidth + unitWidth;
        int precipX = x + FORECAST_COL_WIDTH - 6 - totalWidth;

        // Draw value in bold
        display.setFreeFont(FSSB9);
        display.setTextDatum(TL_DATUM);
        display.drawString(precipStr, precipX, iconY + 5);

        // Draw unit in non-bold with space
        display.setFreeFont(FSS9);
        display.drawString(" mm", precipX + valueWidth, iconY + 5);
    }

    // 3-time grid (06h, 12h, 18h) - now without time labels, more compact
    int gridY = y + 60;  // Reduced spacing from daily forecast section
    drawDayTimeGrid(display, day.times, x + CARD_PADDING, gridY, FORECAST_COL_WIDTH - 12);
}

// Main 3-day forecast column
void draw3DayForecastColumn(TFT_eSprite& display, const ForecastData& forecast) {
    // Three daily sections with individual borders (8px gaps between)
    const char* dayLabels[] = {"Heute", "Morgen", "Uebermorgen"};
    const int sectionHeights[] = {FORECAST_HEUTE_HEIGHT, FORECAST_MORGEN_HEIGHT, FORECAST_UEBERMORGEN_HEIGHT};

    // Calculate dynamic Y-positions with 8px gaps (matching CARD_SPACING)
    int yPositions[] = {
        FORECAST_COL_Y,  // HEUTE starts at top
        FORECAST_COL_Y + FORECAST_HEUTE_HEIGHT + FORECAST_SECTION_GAP,  // MORGEN
        FORECAST_COL_Y + FORECAST_HEUTE_HEIGHT + FORECAST_SECTION_GAP +
            FORECAST_MORGEN_HEIGHT + FORECAST_SECTION_GAP  // UEBERMORGEN
    };

    for (int i = 0; i < 3; i++) {
        // Draw individual border for each section (like other cards)
        display.drawRect(FORECAST_COL_X, yPositions[i], FORECAST_COL_WIDTH, sectionHeights[i], TFT_BLACK);

        drawDailyForecastSection(display, forecast.days[i],
                                 FORECAST_COL_X, yPositions[i],
                                 dayLabels[i], i == 0);  // isToday = (i == 0)
    }
}

void drawStatusBar(TFT_eSprite& display) {
    // Just draw separator line at bottom
    display.drawFastHLine(0, STATUS_BAR_Y, SCREEN_WIDTH, TFT_BLACK);
}

void drawDashboard(TFT_eSprite& display, const DashboardData& data) {
    display.fillScreen(TFT_WHITE);

    // Header
    drawHeader(display, LOCATION_NAME, data.updateTime);

    // COLUMN 1: INDOOR SENSORS
    drawIndoorTempWidget(display, data.weather.indoor);
    drawIndoorHumidWidget(display, data.weather.indoor);
    drawAirQualityWidget(display, data.weather.indoor);
    drawPressureWidget(display, data.weather.indoor);

    // COLUMN 2: OUTDOOR SENSORS + SUN WIDGETS
    drawOutdoorTempWidget(display, data.weather.outdoor);
    drawOutdoorHumidWidget(display, data.weather.outdoor);
    drawSunriseWidget(display, data.sunData);
    drawSunsetWidget(display, data.sunData);

    // COLUMN 3: 3-DAY FORECAST
    draw3DayForecastColumn(display, data.forecast);

    // Status bar (separator line only)
    drawStatusBar(display);
}

// Legacy function names for compatibility
void drawIndoorCO2Widget(TFT_eSprite& display, const IndoorData& data) {
    drawAirQualityWidget(display, data);
}

void drawIndoorPressureWidget(TFT_eSprite& display, const IndoorData& data) {
    drawPressureWidget(display, data);
}
