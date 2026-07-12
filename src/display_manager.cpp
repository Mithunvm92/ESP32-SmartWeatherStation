#include "display_manager.h"
#include <Wire.h>
#include "config.h"
#include "icons.h"
#include "logger.h"
#include "version.h"

static const char* TAG = "Display";

bool DisplayManager::begin() {
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Logger::error(TAG, "SSD1306 init failed — continuing headless.");
        oledOk_ = false;
        return false;
    }

    oledOk_ = true;
    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);
    display_.setRotation(0);
    display_.display();
    Logger::info(TAG, "SSD1306 initialized.");
    return true;
}

void DisplayManager::nextPage() {
    uint8_t next = (static_cast<uint8_t>(currentPage_) + 1) % static_cast<uint8_t>(Page::PAGE_COUNT);
    currentPage_ = static_cast<Page>(next);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void DisplayManager::drawCentered(const String& text, int16_t y, uint8_t textSize) {
    display_.setTextSize(textSize);
    int16_t x1, y1;
    uint16_t w, h;
    display_.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display_.setCursor(x, y);
    display_.print(text);
}

void DisplayManager::drawHeader(const char* title) {
    display_.setTextSize(1);
    display_.setCursor(2, 0);
    display_.print(title);
    display_.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);
}

void DisplayManager::drawWifiGlyph(int16_t x, int16_t y, uint8_t bars) {
    // 4 vertical bars of increasing height, filled up to `bars` (0-4)
    for (uint8_t i = 0; i < 4; ++i) {
        int16_t barH = 2 + i * 2;
        int16_t barX = x + i * 3;
        int16_t barY = y + (8 - barH);
        if (i < bars) {
            display_.fillRect(barX, barY, 2, barH, SSD1306_WHITE);
        } else {
            display_.drawRect(barX, barY, 2, barH, SSD1306_WHITE);
        }
    }
}

void DisplayManager::drawWeatherIconForCode(const char* code, int16_t x, int16_t y) {
    // OpenWeather icon codes: 01=clear,02/03/04=clouds,09/10=rain,11=storm,13=snow,50=fog
    const uint8_t* icon = ICON_CLOUD;
    if (!code || strlen(code) < 2) {
        display_.drawBitmap(x, y, icon, ICON_W, ICON_H, SSD1306_WHITE);
        return;
    }

    char prefix[3] = { code[0], code[1], '\0' };
    bool night = (code[strlen(code) - 1] == 'n');

    if (strcmp(prefix, "01") == 0) icon = night ? ICON_MOON : ICON_SUN;
    else if (strcmp(prefix, "02") == 0 || strcmp(prefix, "03") == 0 || strcmp(prefix, "04") == 0) icon = ICON_CLOUD;
    else if (strcmp(prefix, "09") == 0 || strcmp(prefix, "10") == 0) icon = ICON_RAIN;
    else if (strcmp(prefix, "11") == 0) icon = ICON_STORM;
    else if (strcmp(prefix, "13") == 0) icon = ICON_SNOW;
    else if (strcmp(prefix, "50") == 0) icon = ICON_FOG;

    display_.drawBitmap(x, y, icon, ICON_W, ICON_H, SSD1306_WHITE);
}

// ---------------------------------------------------------------------------
// Page: Dashboard
// ---------------------------------------------------------------------------
void DisplayManager::drawDashboard(const WeatherManager& weather, const NtpManager& ntp, const WiFiManagerExt& wifi) {
    const WeatherData& w = weather.data();

    display_.setTextSize(1);
    display_.setCursor(2, 0);
    display_.print(w.city);

    drawWifiGlyph(OLED_WIDTH - 20, 1, wifi.signalBars());
    display_.setCursor(OLED_WIDTH - 44, 0);
    display_.print(ntp.timeString().substring(0, 5));

    display_.drawFastHLine(0, 10, OLED_WIDTH, SSD1306_WHITE);

    drawWeatherIconForCode(w.iconCode, 4, 16);

    display_.setTextSize(2);
    char tempBuf[12];
    snprintf(tempBuf, sizeof(tempBuf), "%.1fC", w.temperature);
    display_.setCursor(48, 18);
    display_.print(tempBuf);

    display_.setTextSize(1);
    display_.setCursor(48, 38);
    display_.print(w.description);

    char humBuf[16];
    snprintf(humBuf, sizeof(humBuf), "Humidity %d%%", w.humidity);
    display_.setCursor(2, 52);
    display_.print(humBuf);

    if (w.stale) {
        display_.setCursor(OLED_WIDTH - 30, 52);
        display_.print("CACHE");
    }
}

// ---------------------------------------------------------------------------
// Page: Weather details
// ---------------------------------------------------------------------------
void DisplayManager::drawWeatherDetails(const WeatherManager& weather) {
    const WeatherData& w = weather.data();
    drawHeader("Weather Details");

    display_.setTextSize(1);
    char line[24];

    snprintf(line, sizeof(line), "Feels Like: %.1fC", w.feelsLike);
    display_.setCursor(2, 14); display_.print(line);

    snprintf(line, sizeof(line), "Pressure:   %d hPa", w.pressure);
    display_.setCursor(2, 24); display_.print(line);

    snprintf(line, sizeof(line), "Wind: %.1fm/s %ddeg", w.windSpeed, w.windDeg);
    display_.setCursor(2, 34); display_.print(line);

    snprintf(line, sizeof(line), "Visibility: %.1fkm", w.visibilityKm);
    display_.setCursor(2, 44); display_.print(line);

    snprintf(line, sizeof(line), "Cloud: %d%%  Rain: %d%%", w.cloudPercent, w.rainProbability);
    display_.setCursor(2, 54); display_.print(line);

    if (w.stale) {
        display_.setCursor(OLED_WIDTH - 30, 0);
        display_.print("OLD");
    }
}

// ---------------------------------------------------------------------------
// Page: Forecast
// ---------------------------------------------------------------------------
void DisplayManager::drawForecast(const ForecastManager& forecast) {
    const ForecastData& f = forecast.data();
    drawHeader("Forecast");

    // Hourly strip (top)
    int16_t slotW = OLED_WIDTH / FORECAST_SLOTS;
    for (uint8_t i = 0; i < FORECAST_SLOTS; ++i) {
        int16_t x = i * slotW;
        display_.setTextSize(1);
        display_.setCursor(x + 2, 12);
        display_.print(f.hourly[i].time);

        char t[8];
        snprintf(t, sizeof(t), "%.0fC", f.hourly[i].temp);
        display_.setCursor(x + 2, 30);
        display_.print(t);
    }

    display_.drawFastHLine(0, 40, OLED_WIDTH, SSD1306_WHITE);

    // Daily high/low summary (bottom, first 3 days fit nicely)
    display_.setCursor(2, 44);
    display_.print("Daily H/L:");
    for (uint8_t i = 0; i < 3 && i < FORECAST_DAYS; ++i) {
        char d[20];
        snprintf(d, sizeof(d), "%s %.0f/%.0f", f.daily[i].dayLabel, f.daily[i].high, f.daily[i].low);
        display_.setCursor(2 + i * 42, 54);
        display_.print(d);
    }

    if (f.stale) {
        display_.setCursor(OLED_WIDTH - 30, 0);
        display_.print("OLD");
    }
}

// ---------------------------------------------------------------------------
// Page: Clock
// ---------------------------------------------------------------------------
void DisplayManager::drawClock(const NtpManager& ntp) {
    drawCentered(ntp.dayName(), 8, 1);
    drawCentered(ntp.timeString(), 24, 2);
    drawCentered(ntp.dateString(), 48, 1);

    if (!ntp.isSynced()) {
        drawCentered("Syncing time...", 58, 1);
    }
}

// ---------------------------------------------------------------------------
// Page: Disaster status
// ---------------------------------------------------------------------------
void DisplayManager::drawDisasterStatus(const DisasterManager& disaster) {
    const DisasterData& d = disaster.data();
    drawHeader("Disaster Status");

    if (!d.valid || d.activeCount == 0) {
        drawCentered("No Active Alerts", 26, 1);
        drawCentered("GDACS: All Clear", 40, 1);
        return;
    }

    char line[24];
    snprintf(line, sizeof(line), "Active Events: %d", d.activeCount);
    display_.setCursor(2, 14);
    display_.print(line);

    snprintf(line, sizeof(line), "Level: %s", DisasterManager::levelToString(d.worst.level));
    display_.setCursor(2, 26);
    display_.print(line);

    snprintf(line, sizeof(line), "%s", DisasterManager::typeToString(d.worst.type));
    display_.setCursor(2, 38);
    display_.print(line);

    display_.setCursor(2, 50);
    display_.print(d.worst.country);

    if (d.stale) {
        display_.setCursor(OLED_WIDTH - 30, 0);
        display_.print("OLD");
    }
}

// ---------------------------------------------------------------------------
// Page: WiFi info
// ---------------------------------------------------------------------------
void DisplayManager::drawWifiInfo(const WiFiManagerExt& wifi) {
    drawHeader("WiFi");

    char line[32];
    display_.setCursor(2, 12);
    display_.print("SSID: ");
    display_.print(wifi.ssid());

    snprintf(line, sizeof(line), "RSSI: %d dBm", wifi.rssi());
    display_.setCursor(2, 22);
    display_.print(line);

    drawWifiGlyph(90, 12, wifi.signalBars());

    display_.setCursor(2, 32);
    display_.print("IP: ");
    display_.print(wifi.ip());

    display_.setCursor(2, 42);
    display_.print("GW: ");
    display_.print(wifi.gateway());

    display_.setCursor(2, 52);
    display_.print("MAC: ");
    display_.print(wifi.macAddress());
}

// ---------------------------------------------------------------------------
// Page: System info
// ---------------------------------------------------------------------------
void DisplayManager::drawSystemInfo(const SystemManager& sys, const WeatherManager& weather) {
    drawHeader("System");

    char line[32];
    display_.setCursor(2, 12);
    display_.print("FW v");
    display_.print(FW_VERSION_STRING);

    snprintf(line, sizeof(line), "Heap: %lu KB (%lu%%)",
             static_cast<unsigned long>(sys.freeHeap() / 1024),
             static_cast<unsigned long>(sys.heapUsedPercent()));
    display_.setCursor(2, 22);
    display_.print(line);

    snprintf(line, sizeof(line), "Flash: %lu MB  CPU: %luMHz",
             static_cast<unsigned long>(sys.flashSizeBytes() / (1024 * 1024)),
             static_cast<unsigned long>(sys.cpuFreqMHz()));
    display_.setCursor(2, 32);
    display_.print(line);

    uint32_t up = sys.uptimeSeconds();
    snprintf(line, sizeof(line), "Uptime: %luh %lum", static_cast<unsigned long>(up / 3600),
             static_cast<unsigned long>((up % 3600) / 60));
    display_.setCursor(2, 42);
    display_.print(line);

    snprintf(line, sizeof(line), "Restarts: %lu  API: %lums",
             static_cast<unsigned long>(sys.restartCount()),
             static_cast<unsigned long>(weather.data().lastLatencyMs));
    display_.setCursor(2, 52);
    display_.print(line);
}

// ---------------------------------------------------------------------------
// Red alert (disaster priority mode) — flashing, full-screen
// ---------------------------------------------------------------------------
void DisplayManager::drawRedAlert(const DisasterManager& disaster) {
    const uint32_t now = millis();
    if (now - lastFlashMs_ >= 500) {
        lastFlashMs_ = now;
        flashState_ = !flashState_;
    }

    if (flashState_) {
        display_.fillRect(0, 0, OLED_WIDTH, 12, SSD1306_WHITE);
        display_.setTextColor(SSD1306_BLACK);
    } else {
        display_.setTextColor(SSD1306_WHITE);
    }

    drawCentered("!! RED ALERT !!", 2, 1);
    display_.setTextColor(SSD1306_WHITE);

    display_.drawBitmap((OLED_WIDTH - ICON_W) / 2, 14, ICON_WARNING, ICON_W, ICON_H, SSD1306_WHITE);

    const DisasterEvent& e = disaster.data().worst;
    drawCentered(DisasterManager::typeToString(e.type), 32, 1);
    drawCentered(e.country, 42, 1);

    if (e.type == DisasterType::EARTHQUAKE && e.magnitude > 0) {
        char mag[16];
        snprintf(mag, sizeof(mag), "Mag: %.1f", e.magnitude);
        drawCentered(mag, 52, 1);
    } else {
        drawCentered(e.name, 52, 1);
    }
}

// ---------------------------------------------------------------------------
// Top-level render dispatcher
// ---------------------------------------------------------------------------
void DisplayManager::render(const WeatherManager& weather,
                             const ForecastManager& forecast,
                             const DisasterManager& disaster,
                             const NtpManager& ntp,
                             const WiFiManagerExt& wifi,
                             const SystemManager& sys) {
    if (!oledOk_) return; // headless mode — never blocks the rest of the system

    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);

    // Disaster priority mode overrides normal page rotation entirely.
    if (disaster.hasRedAlert()) {
        drawRedAlert(disaster);
    } else {
        switch (currentPage_) {
            case Page::DASHBOARD:        drawDashboard(weather, ntp, wifi); break;
            case Page::WEATHER_DETAILS:  drawWeatherDetails(weather); break;
            case Page::FORECAST:         drawForecast(forecast); break;
            case Page::CLOCK:            drawClock(ntp); break;
            case Page::DISASTER:         drawDisasterStatus(disaster); break;
            case Page::WIFI_INFO:        drawWifiInfo(wifi); break;
            case Page::SYSTEM_INFO:      drawSystemInfo(sys, weather); break;
            default: break;
        }
    }

    display_.display();
}

// ---------------------------------------------------------------------------
// Boot / status / OTA screens
// ---------------------------------------------------------------------------
void DisplayManager::showBootScreen(const String& line1, const String& line2) {
    if (!oledOk_) return;
    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);
    drawCentered(FW_NAME_SHORT, 4, 1);
    display_.drawFastHLine(0, 14, OLED_WIDTH, SSD1306_WHITE);
    drawCentered(line1, 28, 1);
    if (line2.length()) drawCentered(line2, 42, 1);
    drawCentered(String("v") + FW_VERSION_STRING, 56, 1);
    display_.display();
}

void DisplayManager::showMessage(const String& title, const String& line1, const String& line2) {
    if (!oledOk_) return;
    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);
    drawHeader(title.c_str());
    drawCentered(line1, 26, 1);
    if (line2.length()) drawCentered(line2, 40, 1);
    display_.display();
}

void DisplayManager::showOtaStart() {
    showMessage("OTA Update", "Update starting...", "Do not power off");
}

void DisplayManager::showOtaProgress(unsigned int percent) {
    if (!oledOk_) return;
    display_.clearDisplay();
    display_.setTextColor(SSD1306_WHITE);
    drawHeader("OTA Update");
    drawCentered("Do not power off", 22, 1);

    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%u%%", percent);
    drawCentered(pctBuf, 34, 2);

    // Progress bar
    int16_t barW = OLED_WIDTH - 20;
    int16_t barX = 10;
    int16_t barY = 54;
    display_.drawRect(barX, barY, barW, 8, SSD1306_WHITE);
    int16_t fillW = (barW - 2) * static_cast<int32_t>(percent) / 100;
    display_.fillRect(barX + 1, barY + 1, fillW, 6, SSD1306_WHITE);

    display_.display();
}
