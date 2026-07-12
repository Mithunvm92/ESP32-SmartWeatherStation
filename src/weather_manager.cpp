#include "weather_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "constants.h"
#include "logger.h"

static const char* TAG = "Weather";

void WeatherManager::begin() {
    strlcpy(data_.city, OWM_CITY_NAME, sizeof(data_.city));
    Logger::info(TAG, "Weather manager initialized.");
}

bool WeatherManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning(TAG, "No WiFi — using cached weather.");
        data_.stale = true;
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&units=%s&appid=%s",
             OWM_LAT, OWM_LON, OWM_UNITS, OWM_API_KEY);

    for (uint8_t attempt = 1; attempt <= Retry::HTTP_MAX_ATTEMPTS; ++attempt) {
        WiFiClientSecure client;
        client.setInsecure(); // OpenWeather cert not pinned; acceptable for a hobby station
        HTTPClient http;
        http.setTimeout(Retry::HTTP_TIMEOUT_MS);

        const uint32_t startMs = millis();

        if (!http.begin(client, url)) {
            Logger::error(TAG, "HTTP begin() failed.");
            continue;
        }

        int httpCode = http.GET();
        data_.lastLatencyMs = millis() - startMs;

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            http.end();

            if (parseResponse(payload)) {
                data_.valid = true;
                data_.stale = false;
                data_.lastUpdatedEpoch = time(nullptr);
                Logger::info(TAG, String("Weather updated. Temp=") + data_.temperature + "C");
                return true;
            }
            Logger::error(TAG, "Failed to parse weather JSON.");
        } else {
            Logger::warning(TAG, String("Weather API HTTP error: ") + httpCode +
                                  " (attempt " + attempt + "/" + Retry::HTTP_MAX_ATTEMPTS + ")");
            http.end();
        }

        if (attempt < Retry::HTTP_MAX_ATTEMPTS) {
            delay(Retry::HTTP_RETRY_DELAY_MS);
        }
    }

    // All attempts failed — keep last-known-good data, mark as stale.
    data_.stale = true;
    return false;
}

bool WeatherManager::parseResponse(const String& json) {
    // Current-weather payload is small (~1.2 KB); a fixed static buffer avoids
    // heap fragmentation.
    StaticJsonDocument<1536> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Logger::error(TAG, String("JSON parse error: ") + err.c_str());
        return false;
    }

    if (!doc.containsKey("main") || !doc.containsKey("weather")) {
        Logger::error(TAG, "Weather JSON missing required fields.");
        return false;
    }

    strlcpy(data_.city, OWM_CITY_NAME, sizeof(data_.city));

    data_.temperature   = doc["main"]["temp"]        | 0.0f;
    data_.feelsLike      = doc["main"]["feels_like"]   | 0.0f;
    data_.humidity       = doc["main"]["humidity"]     | 0;
    data_.pressure       = doc["main"]["pressure"]     | 0;
    data_.windSpeed       = doc["wind"]["speed"]        | 0.0f;
    data_.windDeg         = doc["wind"]["deg"]          | 0;
    data_.visibilityKm    = (doc["visibility"] | 0) / 1000.0f;
    data_.cloudPercent    = doc["clouds"]["all"]        | 0;
    data_.rainProbability = 0; // "current weather" endpoint has no pop; forecast endpoint does

    JsonObject w0 = doc["weather"][0];
    const char* desc = w0["main"] | "Unknown";
    const char* icon = w0["icon"] | "01d";
    strlcpy(data_.description, desc, sizeof(data_.description));
    strlcpy(data_.iconCode, icon, sizeof(data_.iconCode));

    data_.sunrise = doc["sys"]["sunrise"] | 0;
    data_.sunset  = doc["sys"]["sunset"]  | 0;

    return true;
}
