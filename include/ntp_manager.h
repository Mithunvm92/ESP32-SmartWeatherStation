#pragma once
// ============================================================================
// ntp_manager.h — NTP time synchronization and formatted time accessors
// ============================================================================

#include <Arduino.h>
#include <time.h>

class NtpManager {
public:
    void begin();          // configure NTP servers / timezone
    bool sync();           // trigger a (non-blocking-ish, bounded) sync attempt
    bool isSynced() const { return synced_; }

    // Formatted accessors — safe to call even before first sync (return "--").
    String timeString() const;   // HH:MM:SS (12h or 24h per config)
    String dateString() const;   // DD Mon YYYY
    String dayName() const;      // Monday, Tuesday...
    String monthName() const;
    int    year() const;
    time_t epoch() const { return synced_ ? time(nullptr) : 0; }

private:
    bool synced_ = false;
    uint32_t lastSyncAttemptMs_ = 0;
};
