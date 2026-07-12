#pragma once
// ============================================================================
// config.h — User-editable configuration
// ============================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------------
// WiFi / Captive portal
// ---------------------------------------------------------------------------
// Hardcoded WiFi credentials - ESP will try these first
static constexpr char WIFI_SSID[]         = "YOUR_WIFI_SSID";      // Your WiFi name
static constexpr char WIFI_PASSWORD[]      = "YOUR_WIFI_PASSWORD";  // Your WiFi password

// Setup portal (when WiFi fails)
static constexpr char WIFI_AP_NAME[]      = "WeatherStation-Setup";
static constexpr uint32_t WIFI_PORTAL_TIMEOUT_S = 180;        // captive portal timeout

// ---------------------------------------------------------------------------
// OpenWeather API
// ---------------------------------------------------------------------------
static constexpr char OWM_API_KEY[]   = "YOUR_OPENWEATHER_API_KEY";
static constexpr char OWM_CITY_NAME[] = "Bengaluru";
static constexpr char OWM_COUNTRY[]   = "IN";
static constexpr double OWM_LAT       = 12.9716;
static constexpr double OWM_LON       = 77.5946;
static constexpr char OWM_UNITS[]     = "metric";

// ---------------------------------------------------------------------------
// GDACS (Global Disaster Alert)
// ---------------------------------------------------------------------------
static constexpr char GDACS_API_URL[] =
    "https://www.gdacs.org/gdacsapi/api/events/geteventlist/SEARCH";

// ---------------------------------------------------------------------------
// Time / NTP
// ---------------------------------------------------------------------------
static constexpr char NTP_SERVER_1[] = "pool.ntp.org";
static constexpr char NTP_SERVER_2[] = "time.google.com";
static constexpr long  TZ_OFFSET_SEC       = 5 * 3600 + 30 * 60;
static constexpr int   TZ_DST_OFFSET_SEC   = 0;
static constexpr bool  TIME_FORMAT_24H     = true;

// ---------------------------------------------------------------------------
// OTA
// ---------------------------------------------------------------------------
static constexpr char OTA_HOSTNAME[] = "weatherstation";
static constexpr char OTA_PASSWORD[] = "ota-secret";

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------
static constexpr uint8_t OLED_ADDRESS = 0x3C;
static constexpr uint8_t OLED_SDA_PIN = 8;   // ESP32-C3 Super Mini
static constexpr uint8_t OLED_SCL_PIN = 10;  // ESP32-C3 Super Mini
static constexpr uint16_t OLED_WIDTH  = 128;
static constexpr uint16_t OLED_HEIGHT = 64;
