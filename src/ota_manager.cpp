#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "config.h"
#include "logger.h"

static const char* TAG = "OTA";
static bool* s_updatingFlag = nullptr;

void OtaManager::begin(std::function<void()> onStart,
                        std::function<void(unsigned int)> onProgress,
                        std::function<void()> onEnd) {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    s_updatingFlag = &updating_;

    ArduinoOTA.onStart([this, onStart]() {
        updating_ = true;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Logger::info(TAG, "OTA update starting: " + type);
        if (onStart) onStart();
    });

    ArduinoOTA.onEnd([this, onEnd]() {
        Logger::info(TAG, "OTA update complete. Rebooting...");
        updating_ = false;
        if (onEnd) onEnd();
    });

    ArduinoOTA.onProgress([onProgress](unsigned int progress, unsigned int total) {
        if (total == 0) return;
        unsigned int percent = (progress * 100) / total;
        if (onProgress) onProgress(percent);
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        updating_ = false;
        const char* reason = "Unknown";
        switch (error) {
            case OTA_AUTH_ERROR:    reason = "Auth Failed"; break;
            case OTA_BEGIN_ERROR:   reason = "Begin Failed"; break;
            case OTA_CONNECT_ERROR: reason = "Connect Failed"; break;
            case OTA_RECEIVE_ERROR: reason = "Receive Failed"; break;
            case OTA_END_ERROR:     reason = "End Failed"; break;
        }
        Logger::error(TAG, String("OTA Error: ") + reason);
    });

    ArduinoOTA.begin();
    Logger::info(TAG, String("OTA ready. Hostname: ") + OTA_HOSTNAME);
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}
