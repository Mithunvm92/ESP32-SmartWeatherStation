#include "system_manager.h"
#include <Preferences.h>
#include "logger.h"

static const char* TAG = "System";
static Preferences prefs;

void SystemManager::begin() {
    totalHeapAtBoot_ = ESP.getFreeHeap();

    prefs.begin("sysinfo", false);
    restartCount_ = prefs.getUInt("restarts", 0) + 1;
    prefs.putUInt("restarts", restartCount_);
    prefs.end();

    Logger::info(TAG, String("Boot #") + restartCount_ +
                       " | Free heap: " + totalHeapAtBoot_ + " bytes");
}

uint32_t SystemManager::freeHeap() const {
    return ESP.getFreeHeap();
}

uint32_t SystemManager::largestFreeBlock() const {
    return ESP.getMaxAllocHeap();
}

uint32_t SystemManager::heapUsedPercent() const {
    if (totalHeapAtBoot_ == 0) return 0;
    uint32_t used = (totalHeapAtBoot_ > freeHeap()) ? (totalHeapAtBoot_ - freeHeap()) : 0;
    return (used * 100UL) / totalHeapAtBoot_;
}

uint32_t SystemManager::flashSizeBytes() const {
    return ESP.getFlashChipSize();
}

uint32_t SystemManager::cpuFreqMHz() const {
    return ESP.getCpuFreqMHz();
}

uint32_t SystemManager::uptimeSeconds() const {
    return static_cast<uint32_t>(millis() / 1000UL);
}

bool SystemManager::isHeapCritical() const {
    // Below 20 KB free heap on an ESP32-C3 is considered risky for TLS + JSON work.
    return freeHeap() < 20000;
}
