// ============================================================================
// main.cpp — ESP32 Smart Weather & Disaster Station Pro
//
// Orchestrates all subsystems via a non-blocking millis() scheduler.
// No delay() is used anywhere in the runtime hot path.
// ============================================================================

#include <Arduino.h>

#include "config.h"
#include "constants.h"
#include "version.h"
#include "logger.h"
#include "scheduler.h"
#include "utils.h"

#include "wifi_manager_ext.h"
#include "ntp_manager.h"
#include "weather_manager.h"
#include "forecast_manager.h"
#include "disaster_manager.h"
#include "ota_manager.h"
#include "system_manager.h"
#include "display_manager.h"

// ---------------------------------------------------------------------------
// Global subsystem instances
// ---------------------------------------------------------------------------
static Scheduler        g_scheduler;
static WiFiManagerExt   g_wifi;
static NtpManager       g_ntp;
static WeatherManager   g_weather;
static ForecastManager  g_forecast;
static DisasterManager  g_disaster;
static OtaManager       g_ota;
static SystemManager    g_system;
static DisplayManager   g_display;

// Task IDs (kept so we can pause/resume e.g. during OTA)
static int8_t g_taskDisplayRefresh = -1;
static int8_t g_taskPageRotate     = -1;

// ---------------------------------------------------------------------------
// Task callbacks
// ---------------------------------------------------------------------------
static void taskClockTick() {
    g_ntp.sync(); // cheap no-op if already synced recently; getLocalTime is fast
}

static void taskWeatherUpdate() {
    Logger::info("Main", "Running scheduled weather update...");
    g_weather.update();
}

static void taskForecastUpdate() {
    Logger::info("Main", "Running scheduled forecast update...");
    g_forecast.update();
}

static void taskDisasterUpdate() {
    Logger::info("Main", "Running scheduled GDACS disaster check...");
    g_disaster.update();
}

static void taskWifiCheck() {
    g_wifi.checkConnection();
}

static void taskDisplayRefresh() {
    if (g_ota.isUpdating()) return; // don't fight the OTA progress screen
    g_display.render(g_weather, g_forecast, g_disaster, g_ntp, g_wifi, g_system);
}

static void taskPageRotate() {
    // Disaster priority mode locks the display; skip rotation while a red
    // alert is active. DisplayManager::render() handles the override, but
    // we also avoid silently advancing the "current page" pointer so the
    // user returns to the same page once the alert clears.
    if (g_disaster.hasRedAlert()) return;
    g_display.nextPage();
}

static void taskWatchdogFeed() {
    Utils::feedWatchdog();
}

static void taskSerialCommands() {
    if (!Serial.available()) return;
    // Accept CR, LF, or CRLF line endings — PlatformIO's monitor default
    // varies by OS/terminal setting, so don't rely on '\n' alone.
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // strips any stray \r and whitespace too

    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("resetwifi")) {
        Logger::warning("Main", "Serial command received: resetwifi -> clearing saved WiFi credentials...");
        g_display.showMessage("WiFi Reset", "Clearing saved", "credentials...");
        delay(200); // let the log line and OLED message actually flush before reboot
        g_wifi.resetSettings(); // clears NVS creds and calls ESP.restart()
    } else {
        Logger::info("Main", String("Unknown serial command: '") + cmd + "'. Try: resetwifi");
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200); // brief allowance for USB-CDC enumeration only, not a runtime delay
    Logger::setMinLevel(LogLevel::INFO);

    Logger::info("Main", String(FW_NAME) + " v" + FW_VERSION_STRING);
    Logger::info("Main", String("Build: ") + FW_BUILD_DATE + " " + FW_BUILD_TIME);

    g_system.begin();

    bool oled = g_display.begin();
    if (oled) {
        g_display.showBootScreen("Booting...", "Connecting WiFi");
    }
    if (!oled) {
        Logger::warning("Main", "Running headless — OLED not detected.");
    }

    // --- WiFi (blocking only during first-time captive portal / initial connect) ---
    g_wifi.begin();
    if (oled) g_display.showBootScreen("WiFi Connected", g_wifi.ip());

    // --- Time ---
    g_ntp.begin();
    if (oled) g_display.showBootScreen("Syncing time...");
    for (uint8_t i = 0; i < 10 && !g_ntp.sync(); ++i) {
        delay(300); // bounded, one-time boot wait — not used in the main loop
    }

    // --- Managers ---
    g_weather.begin();
    g_forecast.begin();
    g_disaster.begin();

    // --- OTA ---
    g_ota.begin(
        [oled]() { if (oled) g_display.showOtaStart(); },
        [oled](unsigned int pct) { if (oled) g_display.showOtaProgress(pct); },
        [oled]() { if (oled) g_display.showMessage("OTA Update", "Complete!", "Rebooting..."); }
    );

    // --- Watchdog ---
    Utils::initWatchdog(WDT_TIMEOUT_S);

    // --- Initial data fetch (so the display never shows blank/empty data) ---
    if (oled) g_display.showBootScreen("Fetching weather...");
    g_weather.update();
    if (oled) g_display.showBootScreen("Fetching forecast...");
    g_forecast.update();
    if (oled) g_display.showBootScreen("Checking disasters...");
    g_disaster.update();

    // --- Register scheduled tasks ---
    g_scheduler.addTask("clock",     Interval::CLOCK_TICK_MS,      taskClockTick);
    g_scheduler.addTask("weather",   Interval::WEATHER_MS,         taskWeatherUpdate);
    g_scheduler.addTask("forecast",  Interval::FORECAST_MS,        taskForecastUpdate);
    g_scheduler.addTask("disaster",  Interval::DISASTER_MS,        taskDisasterUpdate);
    g_scheduler.addTask("wifiCheck", Interval::WIFI_CHECK_MS,      taskWifiCheck);
    g_scheduler.addTask("wdtFeed",   Interval::WATCHDOG_FEED_MS,   taskWatchdogFeed, true);
    g_scheduler.addTask("serialCmd", 200UL,                        taskSerialCommands, true);

    g_taskDisplayRefresh = g_scheduler.addTask("dispRefresh", Interval::DISPLAY_REFRESH_MS, taskDisplayRefresh, true);
    g_taskPageRotate     = g_scheduler.addTask("pageRotate",  Interval::PAGE_ROTATE_MS,     taskPageRotate);

    Logger::info("Main", "Setup complete. Entering main loop.");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop() {
    g_ota.handle();          // near-zero cost when idle; must be called often
    g_scheduler.run();       // dispatches any due tasks — never blocks
    // Intentionally no delay() here: OTA and other latency-sensitive
    // operations need loop() to spin as fast as possible. The scheduler
    // itself throttles how often expensive work actually runs.
}
