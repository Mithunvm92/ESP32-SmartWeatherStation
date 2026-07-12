#pragma once
// ============================================================================
// system_manager.h — Heap/flash/CPU stats, uptime, persisted restart counter
// ============================================================================

#include <Arduino.h>

class SystemManager {
public:
    void begin();   // loads persisted restart count from NVS, increments it

    uint32_t freeHeap() const;
    uint32_t largestFreeBlock() const;
    uint32_t heapUsedPercent() const;   // approximation vs total heap at boot
    uint32_t flashSizeBytes() const;
    uint32_t cpuFreqMHz() const;
    uint32_t uptimeSeconds() const;
    uint32_t restartCount() const { return restartCount_; }

    // Returns true if heap usage is unhealthy (used for reliability checks).
    bool isHeapCritical() const;

private:
    uint32_t restartCount_ = 0;
    uint32_t totalHeapAtBoot_ = 0;
};
