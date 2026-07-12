#include "utils.h"
#include <esp_task_wdt.h>
#include "logger.h"

static const char* TAG = "Utils";

String Utils::formatBytes(uint32_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    if (bytes < 1024UL * 1024) return String(bytes / 1024.0f, 1) + " KB";
    return String(bytes / (1024.0f * 1024.0f), 1) + " MB";
}

void Utils::initWatchdog(uint32_t timeoutSeconds) {
    // NOTE: esp_task_wdt_init() signature differs between ESP32 Arduino core
    // versions. This matches core 2.x (esp_task_wdt_init(timeout_s, panic)).
    // If you're on core 3.x, use:
    //   esp_task_wdt_config_t cfg = { .timeout_ms = timeoutSeconds * 1000,
    //                                  .idle_core_mask = 0, .trigger_panic = true };
    //   esp_task_wdt_init(&cfg);
    esp_task_wdt_init(timeoutSeconds, true);
    esp_task_wdt_add(NULL); // watch the current (loop) task
    Logger::info(TAG, String("Watchdog initialized, timeout=") + timeoutSeconds + "s");
}

void Utils::feedWatchdog() {
    esp_task_wdt_reset();
}
