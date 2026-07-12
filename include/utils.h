#pragma once
// ============================================================================
// utils.h — Small shared helper functions (formatting, watchdog helpers)
// ============================================================================

#include <Arduino.h>

namespace Utils {
    // Human-readable byte count, e.g. "128.4 KB"
    String formatBytes(uint32_t bytes);

    // Initializes the ESP32 Task Watchdog Timer with the given timeout (s).
    void initWatchdog(uint32_t timeoutSeconds);

    // Feeds ("resets") the watchdog — must be called regularly from loop().
    void feedWatchdog();
}
