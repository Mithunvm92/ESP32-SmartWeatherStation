#include "disaster_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "constants.h"
#include "logger.h"

static const char* TAG = "Disaster";

void DisasterManager::begin() {
    Logger::info(TAG, "Disaster manager initialized.");
}

DisasterType DisasterManager::parseType(const char* code) {
    if (!code) return DisasterType::UNKNOWN;
    if (strcmp(code, "EQ") == 0) return DisasterType::EARTHQUAKE;
    if (strcmp(code, "FL") == 0) return DisasterType::FLOOD;
    if (strcmp(code, "TC") == 0) return DisasterType::CYCLONE;
    if (strcmp(code, "VO") == 0) return DisasterType::VOLCANO;
    if (strcmp(code, "TS") == 0) return DisasterType::TSUNAMI;
    if (strcmp(code, "WF") == 0) return DisasterType::WILDFIRE;
    if (strcmp(code, "DR") == 0) return DisasterType::DROUGHT;
    return DisasterType::UNKNOWN;
}

AlertLevel DisasterManager::parseLevel(const char* code) {
    if (!code) return AlertLevel::NONE;
    if (strcasecmp(code, "Red") == 0)    return AlertLevel::RED;
    if (strcasecmp(code, "Orange") == 0) return AlertLevel::ORANGE;
    if (strcasecmp(code, "Green") == 0)  return AlertLevel::GREEN;
    return AlertLevel::NONE;
}

const char* DisasterManager::levelToString(AlertLevel level) {
    switch (level) {
        case AlertLevel::RED:    return "RED";
        case AlertLevel::ORANGE: return "ORANGE";
        case AlertLevel::GREEN:  return "GREEN";
        default:                 return "NONE";
    }
}

const char* DisasterManager::typeToString(DisasterType type) {
    switch (type) {
        case DisasterType::EARTHQUAKE: return "Earthquake";
        case DisasterType::FLOOD:      return "Flood";
        case DisasterType::CYCLONE:    return "Cyclone";
        case DisasterType::VOLCANO:    return "Volcano";
        case DisasterType::TSUNAMI:    return "Tsunami";
        case DisasterType::WILDFIRE:   return "Wildfire";
        case DisasterType::DROUGHT:    return "Drought";
        default:                       return "Unknown";
    }
}

bool DisasterManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning(TAG, "No WiFi — using cached disaster status.");
        data_.stale = true;
        return false;
    }

    for (uint8_t attempt = 1; attempt <= Retry::HTTP_MAX_ATTEMPTS; ++attempt) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.setTimeout(Retry::HTTP_TIMEOUT_MS);

        if (!http.begin(client, GDACS_API_URL)) {
            Logger::error(TAG, "HTTP begin() failed.");
            continue;
        }

        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            http.end();

            if (parseResponse(payload)) {
                data_.valid = true;
                data_.stale = false;
                data_.lastCheckedEpoch = time(nullptr);
                Logger::info(TAG, String("GDACS updated. Active=") + data_.activeCount +
                                   " Worst=" + levelToString(data_.worst.level));
                return true;
            }
            Logger::warning(TAG, "GDACS response parsed with no usable events.");
            data_.stale = false; // empty list is a valid "all clear" state
            data_.lastCheckedEpoch = time(nullptr);
            return true;
        } else {
            Logger::warning(TAG, String("GDACS API HTTP error: ") + httpCode +
                                  " (attempt " + attempt + "/" + Retry::HTTP_MAX_ATTEMPTS + ")");
            http.end();
        }

        if (attempt < Retry::HTTP_MAX_ATTEMPTS) {
            delay(Retry::HTTP_RETRY_DELAY_MS);
        }
    }

    data_.stale = true;
    return false;
}

bool DisasterManager::parseResponse(const String& json) {
    // GDACS event list can contain many events; we only need a handful of
    // fields per event, filtered to keep RAM usage bounded.
    StaticJsonDocument<384> filter;
    JsonArray filterArr = filter.to<JsonArray>();
    JsonObject filterItem = filterArr.createNestedObject();
    filterItem["eventtype"] = true;
    filterItem["alertlevel"] = true;
    filterItem["country"] = true;
    filterItem["name"] = true;
    filterItem["severitydata"]["severity"] = true;

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, json, DeserializationOption::Filter(filter));
    if (err) {
        Logger::error(TAG, String("GDACS JSON parse error: ") + err.c_str());
        return false;
    }

    JsonArray events = doc.as<JsonArray>();
    if (events.isNull()) {
        // Some GDACS responses wrap the list in an object; handle gracefully.
        data_.activeCount = 0;
        data_.worst = DisasterEvent{};
        return false;
    }

    AlertLevel worstLevel = AlertLevel::NONE;
    DisasterEvent worstEvent;
    uint8_t active = 0;

    for (JsonObject ev : events) {
        const char* alertCode = ev["alertlevel"] | "Green";
        AlertLevel level = parseLevel(alertCode);
        if (level == AlertLevel::NONE) continue;

        active++;

        if (level > worstLevel) {
            worstLevel = level;
            worstEvent.valid = true;
            worstEvent.level = level;
            worstEvent.type = parseType(ev["eventtype"] | "");
            strlcpy(worstEvent.name, ev["name"] | "Unnamed Event", sizeof(worstEvent.name));
            strlcpy(worstEvent.country, ev["country"] | "Unknown", sizeof(worstEvent.country));
            worstEvent.magnitude = ev["severitydata"]["severity"] | 0.0f;
            worstEvent.lastUpdatedEpoch = time(nullptr);
        }
    }

    data_.activeCount = active;
    data_.worst = worstEvent;
    return active > 0;
}
