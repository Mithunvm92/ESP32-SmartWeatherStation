#include "wifi_manager_ext.h"
#include <WiFiManager.h>
#include "config.h"
#include "constants.h"
#include "logger.h"

static const char* TAG = "WiFi";

bool WiFiManagerExt::begin() {
    Logger::info(TAG, "WiFi Manager starting...");
    
    // Try hardcoded WiFi first
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.println("");
    Serial.println("Connecting to: " + String(WIFI_SSID));
    
    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi Connected!");
        Serial.println("IP: " + WiFi.localIP().toString());
        wasConnected_ = true;
        reconnectAttempts_ = 0;
        return true;
    }
    
    // Hardcoded WiFi failed - use WiFiManager portal
    Serial.println("");
    Serial.println("Hardcoded WiFi failed!");
    Serial.println("Starting WiFiManager portal...");
    
    WiFi.mode(WIFI_OFF);
    delay(200);
    
    WiFiManager wm;
    wm.setDebugOutput(true);
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
    wm.setCaptivePortalEnable(true);
    wm.setWiFiAutoReconnect(true);
    
    wm.setAPCallback([](WiFiManager* wmPtr) {
        Serial.println("");
        Serial.println("========================================");
        Serial.println("SETUP PORTAL OPEN!");
        Serial.println("========================================");
        Serial.println("SSID: " + String(wmPtr->getConfigPortalSSID()));
        Serial.println("IP: 192.168.4.1");
        Serial.println("Connect phone and open browser!");
        Serial.println("========================================");
    });
    
    bool res = wm.autoConnect(WIFI_AP_NAME);
    
    if (!res) {
        Serial.println("Portal timeout - rebooting...");
        delay(3000);
        ESP.restart();
        return false;
    }
    
    Serial.println("WiFi Connected via portal!");
    Serial.println("IP: " + WiFi.localIP().toString());
    wasConnected_ = true;
    reconnectAttempts_ = 0;
    return true;
}

void WiFiManagerExt::checkConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        wasConnected_ = true;
        reconnectAttempts_ = 0;
        return;
    }
    
    wasConnected_ = false;
    reconnectAttempts_++;
    
    if (reconnectAttempts_ < Retry::WIFI_MAX_RETRIES) {
        WiFi.reconnect();
        return;
    }
    
    // Max retries reached - try portal
    Serial.println("Max retries reached, restarting portal...");
    ESP.restart();
}

void WiFiManagerExt::resetSettings() {
    WiFiManager wm;
    wm.resetSettings();
    delay(500);
    ESP.restart();
}

bool WiFiManagerExt::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

int8_t WiFiManagerExt::rssi() const {
    return isConnected() ? WiFi.RSSI() : -100;
}

uint8_t WiFiManagerExt::signalQualityPercent() const {
    int8_t r = rssi();
    if (r <= -100) return 0;
    if (r >= -50) return 100;
    return static_cast<uint8_t>(2 * (r + 100));
}

uint8_t WiFiManagerExt::signalBars() const {
    uint8_t q = signalQualityPercent();
    if (q >= 80) return 4;
    if (q >= 60) return 3;
    if (q >= 35) return 2;
    if (q >= 10) return 1;
    return 0;
}

String WiFiManagerExt::ssid() const {
    return isConnected() ? WiFi.SSID() : String("--");
}

String WiFiManagerExt::ip() const {
    return isConnected() ? WiFi.localIP().toString() : String("0.0.0.0");
}

String WiFiManagerExt::gateway() const {
    return isConnected() ? WiFi.gatewayIP().toString() : String("0.0.0.0");
}

String WiFiManagerExt::dns() const {
    return isConnected() ? WiFi.dnsIP().toString() : String("0.0.0.0");
}

String WiFiManagerExt::macAddress() const {
    return WiFi.macAddress();
}
