#pragma once
// ============================================================================
// display_manager.h — SSD1306 rendering: page rotation, disaster priority
// mode, OTA progress screen, and boot/status screens.
// ============================================================================

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "constants.h"
#include "weather_manager.h"
#include "forecast_manager.h"
#include "disaster_manager.h"
#include "ntp_manager.h"
#include "wifi_manager_ext.h"
#include "system_manager.h"

class DisplayManager {
public:
    bool begin();   // returns false if OLED init fails (system continues headless)

    // Call frequently (scheduler-driven) — cheap, only redraws if needed.
    void render(const WeatherManager& weather,
                const ForecastManager& forecast,
                const DisasterManager& disaster,
                const NtpManager& ntp,
                const WiFiManagerExt& wifi,
                const SystemManager& sys);

    // Advances to next page (called by scheduler every PAGE_ROTATE_MS).
    void nextPage();

    // Simple status/boot screens used before all subsystems are ready.
    void showBootScreen(const String& line1, const String& line2 = "");
    void showMessage(const String& title, const String& line1, const String& line2 = "");

    // OTA progress screen.
    void showOtaProgress(unsigned int percent);
    void showOtaStart();

    bool available() const { return oledOk_; }

private:
    Adafruit_SSD1306 display_{OLED_WIDTH, OLED_HEIGHT, &Wire, -1};
    bool oledOk_ = false;
    Page currentPage_ = Page::DASHBOARD;
    bool flashState_ = false;
    uint32_t lastFlashMs_ = 0;

    void drawDashboard(const WeatherManager& weather, const NtpManager& ntp, const WiFiManagerExt& wifi);
    void drawWeatherDetails(const WeatherManager& weather);
    void drawForecast(const ForecastManager& forecast);
    void drawClock(const NtpManager& ntp);
    void drawDisasterStatus(const DisasterManager& disaster);
    void drawWifiInfo(const WiFiManagerExt& wifi);
    void drawSystemInfo(const SystemManager& sys, const WeatherManager& weather);
    void drawRedAlert(const DisasterManager& disaster);

    void drawHeader(const char* title);
    void drawWifiGlyph(int16_t x, int16_t y, uint8_t bars);
    void drawWeatherIconForCode(const char* code, int16_t x, int16_t y);
    void drawCentered(const String& text, int16_t y, uint8_t textSize = 1);
};
