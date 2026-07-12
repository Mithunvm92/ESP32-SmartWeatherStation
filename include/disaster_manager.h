#pragma once
// ============================================================================
// disaster_manager.h — GDACS disaster alert fetch, parse, and cache
//
// GDACS (Global Disaster Alert and Coordination System) publishes a JSON
// event-list API. We poll it and surface the highest-severity active alert.
// ============================================================================

#include <Arduino.h>

enum class AlertLevel : uint8_t {
    NONE = 0,
    GREEN,
    ORANGE,
    RED
};

enum class DisasterType : uint8_t {
    UNKNOWN = 0,
    EARTHQUAKE,
    FLOOD,
    CYCLONE,
    VOLCANO,
    TSUNAMI,
    WILDFIRE,
    DROUGHT
};

struct DisasterEvent {
    bool          valid       = false;
    AlertLevel    level        = AlertLevel::NONE;
    DisasterType  type          = DisasterType::UNKNOWN;
    char          name[48]      = "--";
    char          country[32]   = "--";
    float         magnitude     = 0.0f;   // for earthquakes
    uint32_t      lastUpdatedEpoch = 0;
};

struct DisasterData {
    bool           valid = false;
    bool           stale = true;
    DisasterEvent  worst;                 // highest-severity active event
    uint8_t        activeCount = 0;
    uint32_t       lastCheckedEpoch = 0;
};

class DisasterManager {
public:
    void begin();
    bool update();
    const DisasterData& data() const { return data_; }
    bool hasRedAlert() const { return data_.valid && data_.worst.level == AlertLevel::RED; }

    static const char* levelToString(AlertLevel level);
    static const char* typeToString(DisasterType type);

private:
    DisasterData data_;
    bool parseResponse(const String& json);
    static DisasterType parseType(const char* code);
    static AlertLevel   parseLevel(const char* code);
};
