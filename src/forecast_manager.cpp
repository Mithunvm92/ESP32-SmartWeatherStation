#include "forecast_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"
#include "constants.h"
#include "logger.h"

static const char* TAG = "Forecast";

void ForecastManager::begin() {
    Logger::info(TAG, "Forecast manager initialized.");
}

bool ForecastManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning(TAG, "No WiFi — using cached forecast.");
        data_.stale = true;
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
              "https://api.openweathermap.org/data/2.5/forecast?lat=%.4f&lon=%.4f&units=%s&appid=%s",
              OWM_LAT, OWM_LON, OWM_UNITS, OWM_API_KEY);

    for (uint8_t attempt = 1; attempt <= Retry::HTTP_MAX_ATTEMPTS; ++attempt) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        http.setTimeout(Retry::HTTP_TIMEOUT_MS);

        if (!http.begin(client, url)) {
            Logger::error(TAG, "HTTP begin() failed.");
            continue;
        }

        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            // Use a streaming filter so we only keep the fields we need —
            // the full 5-day/3-hour payload can exceed 40 KB otherwise.
            StaticJsonDocument<256> filter;
            JsonObject filterList = filter.createNestedArray("list").createNestedObject();
            filterList["dt_txt"] = true;
            filterList["main"]["temp"] = true;
            filterList["main"]["temp_min"] = true;
            filterList["main"]["temp_max"] = true;
            filterList["weather"][0]["icon"] = true;
            filterList["pop"] = true;

            DynamicJsonDocument doc(6144);
            DeserializationError err = deserializeJson(doc, http.getStream(),
                                                        DeserializationOption::Filter(filter));
            http.end();

            if (err) {
                Logger::error(TAG, String("Forecast JSON parse error: ") + err.c_str());
            } else {
                JsonArray list = doc["list"];
                if (list.isNull() || list.size() == 0) {
                    Logger::error(TAG, "Forecast JSON has no 'list' entries.");
                } else {
                    // --- Hourly (next N x 3h slots) ---
                    uint8_t hCount = 0;
                    for (JsonObject item : list) {
                        if (hCount >= FORECAST_SLOTS) break;
                        const char* dtTxt = item["dt_txt"] | "0000-00-00 00:00:00";
                        // dtTxt format: "YYYY-MM-DD HH:MM:SS" -> extract HH:MM
                        char timeBuf[6] = "--:--";
                        if (strlen(dtTxt) >= 16) {
                            timeBuf[0] = dtTxt[11]; timeBuf[1] = dtTxt[12];
                            timeBuf[2] = ':';
                            timeBuf[3] = dtTxt[14]; timeBuf[4] = dtTxt[15];
                            timeBuf[5] = '\0';
                        }
                        strlcpy(data_.hourly[hCount].time, timeBuf, sizeof(data_.hourly[hCount].time));
                        data_.hourly[hCount].temp = item["main"]["temp"] | 0.0f;
                        const char* icon = item["weather"][0]["icon"] | "01d";
                        strlcpy(data_.hourly[hCount].icon, icon, sizeof(data_.hourly[hCount].icon));
                        data_.hourly[hCount].pop = static_cast<int>((item["pop"] | 0.0f) * 100.0f);
                        hCount++;
                    }

                    // --- Daily high/low summary (bucket by calendar day) ---
                    uint8_t dCount = 0;
                    char lastDate[11] = "";
                    for (JsonObject item : list) {
                        if (dCount >= FORECAST_DAYS) break;
                        const char* dtTxt = item["dt_txt"] | "0000-00-00 00:00:00";
                        char dateBuf[11];
                        strncpy(dateBuf, dtTxt, 10);
                        dateBuf[10] = '\0';

                        if (strcmp(dateBuf, lastDate) != 0) {
                            // New day bucket
                            strncpy(lastDate, dateBuf, sizeof(lastDate));
                            struct tm tmDay = {};
                            strptime(dateBuf, "%Y-%m-%d", &tmDay);
                            char dayLabel[4];
                            strftime(dayLabel, sizeof(dayLabel), "%a", &tmDay);
                            strlcpy(data_.daily[dCount].dayLabel, dayLabel, sizeof(data_.daily[dCount].dayLabel));
                            data_.daily[dCount].high = item["main"]["temp_max"] | 0.0f;
                            data_.daily[dCount].low  = item["main"]["temp_min"] | 0.0f;
                            const char* icon = item["weather"][0]["icon"] | "01d";
                            strlcpy(data_.daily[dCount].icon, icon, sizeof(data_.daily[dCount].icon));
                            dCount++;
                        } else if (dCount > 0) {
                            float tmax = item["main"]["temp_max"] | 0.0f;
                            float tmin = item["main"]["temp_min"] | 0.0f;
                            DailyForecast& d = data_.daily[dCount - 1];
                            if (tmax > d.high) d.high = tmax;
                            if (tmin < d.low)  d.low  = tmin;
                        }
                    }

                    data_.valid = true;
                    data_.stale = false;
                    data_.lastUpdatedEpoch = time(nullptr);
                    Logger::info(TAG, "Forecast updated successfully.");
                    return true;
                }
            }
        } else {
            Logger::warning(TAG, String("Forecast API HTTP error: ") + httpCode +
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
