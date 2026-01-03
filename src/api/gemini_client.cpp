#include "gemini_client.h"
#include "http_utils.h"
#include <time.h>

GeminiClient::GeminiClient() {
}

String GeminiClient::buildPrompt(const WeatherData& weather, unsigned long timestamp) {
    // Format timestamp as dd.mm.yyyy hh:mm
    time_t t = timestamp;
    struct tm* tm = localtime(&t);
    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02d.%02d.%04d %02d:%02d",
             tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
             tm->tm_hour, tm->tm_min);

    String  prompt = "Aktuelles Datum / Zeit: ";
    prompt += timeStr;
    prompt += "\n\n";
    prompt += " Ort: ";
    prompt += LOCATION_NAME;
    prompt += "\n\n";

    prompt += "Du bist ein sympathischer Wetter- und Raumklima-Assistent fuer ein Dashboard.\n";
    prompt += "Deine Aufgabe ist es, aus strukturierten Wetterdaten eine kurze, leicht witzige Aussage mit einer plausiblen Vermutung zu formulieren.\n\n";

    prompt += "Ton & Stil:\n";
    prompt += "- Freundlich, trocken-humorig, nie albern\n";
    prompt += "- Vermutungen immer vorsichtig formulieren (z. B. \"wohl\", \"duerfte\", \"scheint\")\n";
    prompt += "- Der Text muss auch in einem professionellen Umfeld passen\n\n";

    prompt += "Regeln:\n";
    prompt += "- Genau 1 Satz, maximal 160 Zeichen\n";
    prompt += "- Die Aussage muss eine Beobachtung + eine Vermutung enthalten\n";
    prompt += "- Die Vermutung muss direkt aus den Trends oder Kombinationen der Werte abgeleitet sein\n";
    prompt += "- Moegliche Ableitungen sind z. B.:\n";
    prompt += "  - Steigende Temperatur + steigendes CO2 -> mehr Personen\n";
    prompt += "  - Fallendes CO2 -> Lueften oder weniger Personen\n";
    prompt += "  - Kalte Aussentemperatur -> alle bleiben lieber drinnen\n";
    prompt += "  - Hohe Luftfeuchtigkeit -> schwere oder feuchte Luft\n";
    prompt += "- Der Unix Timestamp muss zuerst in lokale Uhrzeit (Europe/Zurich) umgerechnet werden\n";
    prompt += "- Der Timestamp darf fuer zeitbezogene Vermutungen genutzt werden (Morgen, Mittag, Abend)\n";
    prompt += "- Der Timestamp darf fuer Jahreszeitliche Vermutungen genutzt werden (Winter, Frühling, Sommer, Herbst)\n";
    prompt += "- Der Timestamp darf fuer Vermutungen bezüglich Feiertagen genutzt werden\n";

    // 50% chance to include the "Vater am Lüften" hint (only if CO2 > 1500 ppm)
    //if (weather.indoor.valid && weather.indoor.co2 > 1500 && random(2) == 0) {
    //    prompt += "- Die Luftqualität ist schlecht und je nach Trend hat Vater soeben gelüftet oder wird dies demnächst vornehmen\n";
    //}

    // 50% chance to include the "Ofen" hint (only if temp < 20°C)
    //if (weather.indoor.valid && weather.indoor.temperature < 20.0 && random(2) == 0) {
    //    prompt += "- Falls die Innentemperatur unter 20°C ist, erwähne dass der Ofen wohl noch etwas eingefeuert werden muss\n";
    //}

    if (random(50) == 0) {
        prompt += "- Du bist eine Katze und machst Miau Miau\n";
    }

    prompt += "- Keine Zahlen wiederholen, keine Aufzaehlungen\n";
    prompt += "- Keine Empfehlungen oder Anweisungen\n\n";



    // Weather data
    prompt += "Wetterdaten:\n";

    if (weather.indoor.valid) {
        // Helper lambda to convert trend to German
        auto trendToGerman = [](Trend t) -> const char* {
            switch (t) {
                case Trend::UP: return "steigend";
                case Trend::DOWN: return "fallend";
                case Trend::STABLE: return "stabil";
                default: return "";
            }
        };

        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                "Innen: %.1fC %s, %d%% Luftfeuchtigkeit, %d ppm CO2 %s, %d mbar Luftdruck %s\n",
                weather.indoor.temperature, trendToGerman(weather.indoor.temperatureTrend),
                weather.indoor.humidity,
                weather.indoor.co2, trendToGerman(weather.indoor.co2Trend),
                weather.indoor.pressure, trendToGerman(weather.indoor.pressureTrend));
        prompt += buffer;
    }

    if (weather.outdoor.valid) {
        auto trendToGerman = [](Trend t) -> const char* {
            switch (t) {
                case Trend::UP: return "steigend";
                case Trend::DOWN: return "fallend";
                case Trend::STABLE: return "stabil";
                default: return "";
            }
        };

        char buffer[128];
        snprintf(buffer, sizeof(buffer),
                "Aussen: %.1fC %s, %d%% Luftfeuchtigkeit\n",
                weather.outdoor.temperature, trendToGerman(weather.outdoor.temperatureTrend),
                weather.outdoor.humidity);
        prompt += buffer;
    }


    return prompt;
}

const char* GeminiClient::getSeason(unsigned long timestamp) {
    time_t t = timestamp;
    struct tm* tm = localtime(&t);
    int month = tm->tm_mon + 1;  // 1-12

    if (month >= 3 && month <= 5) return "Fruehling";
    if (month >= 6 && month <= 8) return "Sommer";
    if (month >= 9 && month <= 11) return "Herbst";
    return "Winter";
}

const char* GeminiClient::getTimeOfDay(unsigned long timestamp) {
    time_t t = timestamp;
    struct tm* tm = localtime(&t);
    int hour = tm->tm_hour;

    if (hour >= 5 && hour < 12) return "Morgen";
    if (hour >= 12 && hour < 18) return "Nachmittag";
    if (hour >= 18 && hour < 22) return "Abend";
    return "Nacht";
}

const char* GeminiClient::getHoliday(unsigned long timestamp) {
    time_t t = timestamp;
    struct tm* tm = localtime(&t);
    int month = tm->tm_mon + 1;
    int day = tm->tm_mday;

    // Fixed holidays
    if (month == 1 && day == 1) return "Neujahr";
    if (month == 8 && day == 1) return "Bundesfeier";
    if (month == 12 && day == 24) return "Heiligabend";
    if (month == 12 && day == 25) return "Weihnachten";
    if (month == 12 && day == 26) return "Stephanstag";
    if (month == 12 && day == 31) return "Silvester";

    // Easter calculation (simplified - could use full algorithm)
    // For now, just check typical Easter dates in April
    if (month == 4 && (day >= 1 && day <= 20)) {
        // Easter Sunday typically falls here
        if (day == tm->tm_wday + 14) return "Ostern";  // Rough approximation
    }

    return "";  // No holiday
}

String GeminiClient::generateCommentary(const WeatherData& weather, unsigned long timestamp) {
    ESP_LOGI("gemini", "Generating weather commentary");

    // Check if API key is configured
    if (strlen(GEMINI_API_KEY) == 0 || strcmp(GEMINI_API_KEY, "your-gemini-api-key-here") == 0) {
        ESP_LOGE("gemini", "GEMINI_API_KEY not configured");
        return "";
    }

    // Build prompt
    String prompt = buildPrompt(weather, timestamp);
    ESP_LOGD("gemini", "Prompt: %s", prompt.c_str());

    // Build JSON request body
    // Gemini API format: {"contents":[{"parts":[{"text":"prompt"}]}]}
    JsonDocument requestDoc;
    JsonArray contents = requestDoc["contents"].to<JsonArray>();
    JsonObject content = contents.add<JsonObject>();
    JsonArray parts = content["parts"].to<JsonArray>();
    JsonObject part = parts.add<JsonObject>();
    part["text"] = prompt;

    String requestBody;
    serializeJson(requestDoc, requestBody);

    ESP_LOGD("gemini", "Request size: %d bytes", requestBody.length());

    // Make API request
    JsonDocument responseDoc;
    if (!HTTPUtils::httpPostJSON(GEMINI_API_URL, requestBody.c_str(), responseDoc, GEMINI_API_KEY)) {
        ESP_LOGE("gemini", "API request failed");
        return "";
    }

    // Parse response
    // Response format: {"candidates":[{"content":{"parts":[{"text":"response"}]}}]}
    JsonArray candidates = responseDoc["candidates"];
    if (!candidates || candidates.size() == 0) {
        ESP_LOGE("gemini", "No candidates in response");
        return "";
    }

    JsonObject candidate = candidates[0];
    JsonObject contentObj = candidate["content"];
    if (!contentObj) {
        ESP_LOGE("gemini", "No content in candidate");
        return "";
    }

    JsonArray partsArray = contentObj["parts"];
    if (!partsArray || partsArray.size() == 0) {
        ESP_LOGE("gemini", "No parts in content");
        return "";
    }

    JsonObject partObj = partsArray[0];
    const char* text = partObj["text"];
    if (!text) {
        ESP_LOGE("gemini", "No text in part");
        return "";
    }

    String commentary = String(text);
    commentary.trim();  // Remove leading/trailing whitespace

    // Replace umlauts for ASCII-only FreeFonts
    commentary.replace("ä", "ae");
    commentary.replace("ö", "oe");
    commentary.replace("ü", "ue");
    commentary.replace("Ä", "Ae");
    commentary.replace("Ö", "Oe");
    commentary.replace("Ü", "Ue");
    commentary.replace("ß", "ss");

    ESP_LOGI("gemini", "Generated: %s", commentary.c_str());
    return commentary;
}
