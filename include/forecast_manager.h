#pragma once
// ============================================================================
// forecast_manager.h — OpenWeather 5 day / 3 hour forecast fetch and cache
// ============================================================================

#include <Arduino.h>

static constexpr uint8_t FORECAST_SLOTS = 5;   // next 5 x 3-hour slots (~15 hrs)
static constexpr uint8_t FORECAST_DAYS  = 5;   // 5-day daily high/low summary

struct ForecastSlot {
    char   time[6]   = "--:--";  // HH:MM
    float  temp        = 0.0f;
    char   icon[8]     = "01d";
    int    pop          = 0;      // probability of precipitation (%)
};

struct DailyForecast {
    char  dayLabel[4] = "---";   // Mon, Tue...
    float high          = 0.0f;
    float low            = 0.0f;
    char  icon           [8] = "01d";
};

struct ForecastData {
    bool valid = false;
    bool stale = true;
    ForecastSlot   hourly[FORECAST_SLOTS];
    DailyForecast  daily[FORECAST_DAYS];
    uint32_t lastUpdatedEpoch = 0;
};

class ForecastManager {
public:
    void begin();
    bool update();
    const ForecastData& data() const { return data_; }

private:
    ForecastData data_;
    bool parseResponse(const String& json);
};
