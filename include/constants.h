#pragma once
// ============================================================================
// constants.h — System-wide constants, timing intervals, buffer sizes
// ============================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Scheduler intervals (milliseconds)
// ---------------------------------------------------------------------------
namespace Interval {
    static constexpr uint32_t CLOCK_TICK_MS      = 1000UL;            // 1 s
    static constexpr uint32_t WEATHER_MS         = 15UL * 60 * 1000;  // 15 min
    static constexpr uint32_t FORECAST_MS        = 30UL * 60 * 1000;  // 30 min
    static constexpr uint32_t DISASTER_MS        = 5UL * 60 * 1000;   // 5 min
    static constexpr uint32_t DISPLAY_REFRESH_MS = 100UL;             // 100 ms
    static constexpr uint32_t WIFI_CHECK_MS      = 10UL * 1000;       // 10 s
    static constexpr uint32_t NTP_RESYNC_MS      = 60UL * 60 * 1000;  // 1 hr
    static constexpr uint32_t PAGE_ROTATE_MS     = 60UL * 1000;       // 60 s
    static constexpr uint32_t WATCHDOG_FEED_MS   = 5UL * 1000;        // 5 s
    static constexpr uint32_t FLASH_BLINK_MS     = 500UL;             // 0.5 s
}

// ---------------------------------------------------------------------------
// Retry / reliability
// ---------------------------------------------------------------------------
namespace Retry {
    static constexpr uint8_t  HTTP_MAX_ATTEMPTS   = 3;
    static constexpr uint32_t HTTP_RETRY_DELAY_MS = 2000UL;
    static constexpr uint32_t HTTP_TIMEOUT_MS     = 8000UL;
    static constexpr uint8_t  WIFI_MAX_RETRIES    = 5;
}

// ---------------------------------------------------------------------------
// Buffer sizes
// ---------------------------------------------------------------------------
namespace BufSize {
    static constexpr size_t SMALL  = 32;
    static constexpr size_t MEDIUM = 64;
    static constexpr size_t LARGE  = 128;
}

// ---------------------------------------------------------------------------
// Display pages
// ---------------------------------------------------------------------------
enum class Page : uint8_t {
    DASHBOARD = 0,
    WEATHER_DETAILS,
    FORECAST,
    CLOCK,
    DISASTER,
    WIFI_INFO,
    SYSTEM_INFO,
    PAGE_COUNT   // keep last
};

// ---------------------------------------------------------------------------
// Watchdog timeout
// ---------------------------------------------------------------------------
static constexpr uint32_t WDT_TIMEOUT_S = 30;
