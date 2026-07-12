#include "ntp_manager.h"
#include "config.h"
#include "logger.h"

static const char* TAG = "NTP";

void NtpManager::begin() {
    configTime(TZ_OFFSET_SEC, TZ_DST_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
    Logger::info(TAG, "NTP configured, awaiting first sync...");
}

bool NtpManager::sync() {
    lastSyncAttemptMs_ = millis();

    struct tm timeinfo;
    // getLocalTime has an internal short timeout; safe to call from scheduler.
    if (!getLocalTime(&timeinfo, 1500)) {
        Logger::warning(TAG, "NTP sync failed / not yet available.");
        return false;
    }

    if (!synced_) {
        Logger::info(TAG, "NTP synced successfully.");
    }
    synced_ = true;
    return true;
}

String NtpManager::timeString() const {
    if (!synced_) return "--:--:--";
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return "--:--:--";

    char buf[16];
    if (TIME_FORMAT_24H) {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        int hour12 = timeinfo.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (timeinfo.tm_hour >= 12) ? "PM" : "AM";
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);
    }
    return String(buf);
}

String NtpManager::dateString() const {
    if (!synced_) return "--/--/----";
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return "--/--/----";

    char buf[24];
    strftime(buf, sizeof(buf), "%d %b %Y", &timeinfo);
    return String(buf);
}

String NtpManager::dayName() const {
    if (!synced_) return "---";
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return "---";
    char buf[16];
    strftime(buf, sizeof(buf), "%A", &timeinfo);
    return String(buf);
}

String NtpManager::monthName() const {
    if (!synced_) return "---";
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return "---";
    char buf[16];
    strftime(buf, sizeof(buf), "%B", &timeinfo);
    return String(buf);
}

int NtpManager::year() const {
    if (!synced_) return 0;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) return 0;
    return timeinfo.tm_year + 1900;
}
