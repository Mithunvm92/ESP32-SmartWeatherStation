#pragma once
// ============================================================================
// weather_manager.h — OpenWeather "current weather" fetch, parse, and cache
// ============================================================================

#include <Arduino.h>
#include <time.h>

struct WeatherData {
    bool     valid            = false;   // has ever been successfully populated
    bool     stale             = true;    // true if last fetch failed (showing cache)

    char     city[32]          = "--";
    float    temperature       = 0.0f;
    float    feelsLike         = 0.0f;
    int      humidity          = 0;
    int      pressure          = 0;
    float    windSpeed         = 0.0f;
    int      windDeg           = 0;
    float    visibilityKm      = 0.0f;
    int      cloudPercent      = 0;
    int      rainProbability   = 0;      // from "pop" if available, else 0
    char     description[32]   = "--";
    char     iconCode[8]       = "01d";
    time_t   sunrise           = 0;
    time_t   sunset            = 0;
    uint32_t lastUpdatedEpoch  = 0;
    uint32_t lastLatencyMs     = 0;
};

class WeatherManager {
public:
    void begin();
    bool update();               // blocking-bounded HTTP fetch (called from scheduler)
    const WeatherData& data() const { return data_; }

private:
    WeatherData data_;
    bool parseResponse(const String& json);
};
