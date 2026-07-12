#pragma once
// ============================================================================
// wifi_manager_ext.h — WiFi connection lifecycle, captive portal, reconnect
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>

// Serial command handler for WiFiManager commands
void handleWiFiSerialCommand(const String& cmd);

class WiFiManagerExt {
public:
    // Starts WiFiManager captive portal if no saved credentials, otherwise
    // connects using saved credentials. Blocking only during initial boot
    // (acceptable — this happens once before the main loop starts).
    bool begin();

    // Non-blocking health check + auto-reconnect. Call periodically from
    // the scheduler.
    void checkConnection();

    // Wipes saved WiFi credentials and restarts into captive portal mode.
    void resetSettings();

    // Print current WiFi status to Serial
    void printStatus() const;

    bool isConnected() const;
    int8_t rssi() const;
    uint8_t signalQualityPercent() const;
    uint8_t signalBars() const;   // 0-4 bars

    String ssid() const;
    String ip() const;
    String gateway() const;
    String dns() const;
    String macAddress() const;
    uint32_t connectedSinceEpoch() const { return connectedSinceEpoch_; }

    // Portal state tracking
    bool isPortalActive() const { return portalActive_; }

private:
    bool wasConnected_ = false;
    uint8_t reconnectAttempts_ = 0;
    uint32_t connectedSinceEpoch_ = 0;
    bool portalActive_ = false;
    
    // Internal methods
    void resetWiFiState();
    void logCurrentWiFiState();
    void launchConfigPortal();
};
