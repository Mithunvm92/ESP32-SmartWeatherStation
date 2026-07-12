#include "wifi_manager_ext.h"
#include <WiFiManager.h>
#include <esp_wifi.h>
#include "nvs_flash.h"
#include "config.h"
#include "constants.h"
#include "logger.h"

static const char* TAG = "WiFi";

// Global pointer for callback access
static WiFiManagerExt* g_wifiInstance = nullptr;

// ============================================================================
// Serial Command Handler
// ============================================================================
void handleWiFiSerialCommand(const String& cmd) {
    if (cmd.equalsIgnoreCase("resetwifi")) {
        Logger::warning(TAG, ">>> Serial command: resetwifi");
        Logger::info(TAG, "Clearing saved WiFi credentials and restarting...");
        WiFiManager wm;
        wm.resetSettings();
        delay(500);
        ESP.restart();
    } 
    else if (cmd.equalsIgnoreCase("status")) {
        Logger::info(TAG, ">>> Serial command: status");
        if (g_wifiInstance) g_wifiInstance->printStatus();
    } 
    else if (cmd.equalsIgnoreCase("reboot")) {
        Logger::warning(TAG, ">>> Serial command: reboot");
        Logger::info(TAG, "Rebooting device...");
        delay(500);
        ESP.restart();
    } 
    else if (cmd.equalsIgnoreCase("heap")) {
        Logger::info(TAG, ">>> Serial command: heap");
        Logger::info(TAG, String("Free heap: ") + ESP.getFreeHeap() + " bytes");
        Logger::info(TAG, String("Heap size: ") + ESP.getHeapSize() + " bytes");
        Logger::info(TAG, String("Min free heap: ") + ESP.getMinFreeHeap() + " bytes");
    }
}

// ============================================================================
// WiFiManagerExt Implementation
// ============================================================================

void WiFiManagerExt::resetWiFiState() {
    Logger::debug(TAG, "========================================");
    Logger::debug(TAG, "Resetting WiFi state before WiFiManager");
    Logger::debug(TAG, "========================================");
    
    // Force station mode
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Disconnect, clear BSSID, clear WiFi config
    WiFi.disconnect(true, true);
    delay(200);
    
    // Additional ESP32-C3 specific cleanup
    wifi_config_t current_conf;
    esp_wifi_get_config(WIFI_IF_STA, &current_conf);
    esp_wifi_set_config(WIFI_IF_STA, &current_conf);
    
    Logger::debug(TAG, "WiFi state reset complete");
}

void WiFiManagerExt::logCurrentWiFiState() {
    Logger::info(TAG, "========== Current WiFi State ==========");
    
    // Get stored SSID if any
    wifi_config_t wifi_conf;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_conf);
    String savedSSID = (char*)wifi_conf.sta.ssid;
    if (savedSSID.length() == 0) {
        savedSSID = "NONE";
    }
    Logger::info(TAG, String("Saved SSID: ") + savedSSID);
    
    // Current WiFi status
    wl_status_t status = WiFi.status();
    String statusStr;
    switch(status) {
        case WL_IDLE_STATUS:     statusStr = "IDLE"; break;
        case WL_NO_SSID_AVAIL:   statusStr = "NO_SSID_AVAIL"; break;
        case WL_SCAN_COMPLETED:  statusStr = "SCAN_COMPLETED"; break;
        case WL_CONNECTED:       statusStr = "CONNECTED"; break;
        case WL_CONNECT_FAILED:  statusStr = "CONNECT_FAILED"; break;
        case WL_CONNECTION_LOST: statusStr = "CONNECTION_LOST"; break;
        case WL_DISCONNECTED:    statusStr = "DISCONNECTED"; break;
        default:                 statusStr = "UNKNOWN"; break;
    }
    Logger::info(TAG, String("WiFi Status: ") + statusStr);
    
    // Current mode
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    String modeStr = (mode == WIFI_MODE_STA) ? "STA" : 
                     (mode == WIFI_MODE_AP) ? "AP" : 
                     (mode == WIFI_MODE_APSTA) ? "AP+STA" : "NULL";
    Logger::info(TAG, String("WiFi Mode: ") + modeStr);
    
    // RSSI
    Logger::info(TAG, String("RSSI: ") + WiFi.RSSI() + " dBm");
    
    // Current IP
    Logger::info(TAG, String("Current IP: ") + WiFi.localIP().toString());
    
    Logger::info(TAG, "==========================================");
}

void WiFiManagerExt::printStatus() const {
    Logger::info(TAG, "========== WiFi Status Report ==========");
    Logger::info(TAG, String("Connected: ") + (isConnected() ? "YES" : "NO"));
    Logger::info(TAG, String("SSID: ") + ssid());
    Logger::info(TAG, String("IP: ") + ip());
    Logger::info(TAG, String("Gateway: ") + gateway());
    Logger::info(TAG, String("DNS: ") + dns());
    Logger::info(TAG, String("RSSI: ") + WiFi.RSSI() + " dBm");
    Logger::info(TAG, String("Signal: ") + signalQualityPercent() + "%");
    Logger::info(TAG, String("Bars: ") + signalBars() + "/4");
    Logger::info(TAG, String("MAC: ") + macAddress());
    Logger::info(TAG, String("Portal Active: ") + (portalActive_ ? "YES" : "NO"));
    Logger::info(TAG, "========================================");
}

bool WiFiManagerExt::begin() {
    Logger::info(TAG, "");
    Logger::info(TAG, "****************************************");
    Logger::info(TAG, "*       WiFiManagerExt::begin()        *");
    Logger::info(TAG, "****************************************");
    
    // Store instance for callbacks
    g_wifiInstance = this;
    
    // Step 1: Reset WiFi state to ensure clean start
    resetWiFiState();
    
    // Step 2: Log current state
    logCurrentWiFiState();
    
    // Step 3: Create WiFiManager instance
    WiFiManager wm;
    wm.setDebugOutput(true);
    
    // Step 4: Configure portal timeout - 0 = never timeout during debugging
    wm.setConfigPortalTimeout(0);  // NEVER TIMEOUT while debugging
    wm.setConnectTimeout(30);
    wm.setBreakAfterConfig(true);
    
    // Step 5: Configure AP callback with enhanced logging
    wm.setAPCallback([](WiFiManager* wmPtr) {
        if (g_wifiInstance) g_wifiInstance->portalActive_ = true;
        Logger::info(TAG, "");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "       SETUP PORTAL ACTIVE         ");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, String("SSID: ") + wmPtr->getConfigPortalSSID());
        Logger::info(TAG, "Password: weather123");
        Logger::info(TAG, "IP: 192.168.4.1");
        Logger::info(TAG, "Connect from phone now!");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "");
    });
    
    // Step 6: Configure save config callback
    wm.setSaveConfigCallback([]() {
        Logger::info(TAG, "*** CREDENTIALS SAVED TO NVS ***");
    });
    
    // Step 7: Try autoConnect with saved credentials first
    Logger::info(TAG, "");
    Logger::info(TAG, ">>> Trying saved credentials...");
    Logger::info(TAG, "");
    
    bool ok = wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);
    
    if (ok && WiFi.status() == WL_CONNECTED) {
        // Connected successfully
        portalActive_ = false;
        Logger::info(TAG, "");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "         CONNECTED!                ");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, String("SSID: ") + WiFi.SSID());
        Logger::info(TAG, String("IP: ") + WiFi.localIP().toString());
        Logger::info(TAG, String("Gateway: ") + WiFi.gatewayIP().toString());
        Logger::info(TAG, String("RSSI: ") + WiFi.RSSI() + " dBm");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "");
        
        wasConnected_ = true;
        reconnectAttempts_ = 0;
        connectedSinceEpoch_ = time(nullptr);
        return true;
    }
    
    // If we get here, either no saved creds or connection failed
    // Check if portal is running
    Logger::warning(TAG, "");
    Logger::warning(TAG, "====================================");
    Logger::warning(TAG, "    Opening setup portal...         ");
    Logger::warning(TAG, "====================================");
    Logger::warning(TAG, "Portal SSID: WeatherStation-Setup");
    Logger::warning(TAG, "Portal Password: weather123");
    Logger::warning(TAG, "Portal IP: 192.168.4.1");
    Logger::warning(TAG, "");
    
    // Block in portal mode - stays open until user configures
    // DO NOT return from this function until connected or portal closes
    while (true) {
        delay(100);
        yield();
        
        // Check if connected after user saves credentials
        if (WiFi.status() == WL_CONNECTED) {
            portalActive_ = false;
            Logger::info(TAG, "");
            Logger::info(TAG, "====================================");
            Logger::info(TAG, "   WiFi CONNECTED AFTER CONFIG!   ");
            Logger::info(TAG, "====================================");
            Logger::info(TAG, String("SSID: ") + WiFi.SSID());
            Logger::info(TAG, String("IP: ") + WiFi.localIP().toString());
            Logger::info(TAG, "====================================");
            
            wasConnected_ = true;
            reconnectAttempts_ = 0;
            connectedSinceEpoch_ = time(nullptr);
            return true;
        }
    }
}

void WiFiManagerExt::checkConnection() {
    const bool connected = (WiFi.status() == WL_CONNECTED);

    if (connected) {
        if (!wasConnected_) {
            Logger::info(TAG, "====================================");
            Logger::info(TAG, "       WiFi RECONNECTED!            ");
            Logger::info(TAG, "====================================");
            Logger::info(TAG, String("SSID: ") + WiFi.SSID());
            Logger::info(TAG, String("IP: ") + WiFi.localIP().toString());
            Logger::info(TAG, String("RSSI: ") + WiFi.RSSI() + " dBm");
            Logger::info(TAG, "====================================");
            connectedSinceEpoch_ = time(nullptr);
        }
        wasConnected_ = true;
        reconnectAttempts_ = 0;
        return;
    }

    // Not connected
    wasConnected_ = false;
    reconnectAttempts_++;

    Logger::warning(TAG, "");
    Logger::warning(TAG, ">>> WiFi DISCONNECTED");
    Logger::warning(TAG, String("Reconnect attempt: ") + reconnectAttempts_ + "/" + Retry::WIFI_MAX_RETRIES);
    Logger::warning(TAG, String("Current RSSI: ") + WiFi.RSSI() + " dBm");

    if (reconnectAttempts_ < Retry::WIFI_MAX_RETRIES) {
        Logger::info(TAG, ">>> Attempting reconnect...");
        WiFi.reconnect();
        return;
    }

    // Exhausted all reconnect attempts
    Logger::error(TAG, "");
    Logger::error(TAG, "====================================");
    Logger::error(TAG, "   MAX RECONNECT ATTEMPTS REACHED   ");
    Logger::error(TAG, "====================================");
    Logger::error(TAG, ">>> Launching WiFiManager portal...");
    Logger::error(TAG, ">>> DO NOT REBOOT - staying in portal");
    Logger::error(TAG, "");

    // DO NOT reboot - launch portal instead
    launchConfigPortal();
}

void WiFiManagerExt::launchConfigPortal() {
    WiFiManager wm;
    wm.setDebugOutput(true);
    
    // Never timeout in portal mode
    wm.setConfigPortalTimeout(0);
    wm.setConnectTimeout(30);
    wm.setBreakAfterConfig(true);
    
    // Enhanced AP callback
    wm.setAPCallback([](WiFiManager* wmPtr) {
        if (g_wifiInstance) g_wifiInstance->portalActive_ = true;
        Logger::info(TAG, "");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "       SETUP PORTAL ACTIVE         ");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, String("SSID: ") + wmPtr->getConfigPortalSSID());
        Logger::info(TAG, "Password: weather123");
        Logger::info(TAG, "IP: 192.168.4.1");
        Logger::info(TAG, "Connect from phone now!");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "");
    });
    
    // Save config callback
    wm.setSaveConfigCallback([]() {
        Logger::info(TAG, "*** New credentials saved to NVS ***");
    });
    
    Logger::info(TAG, ">>> Opening setup portal...");
    Logger::info(TAG, String("Portal SSID: ") + WIFI_AP_NAME);
    Logger::info(TAG, "Portal IP: 192.168.4.1");
    
    // Start portal - will block until configured or timeout (0 = never)
    bool ok = wm.startConfigPortal(WIFI_AP_NAME, WIFI_AP_PASSWORD);
    
    if (ok) {
        portalActive_ = false;
        Logger::info(TAG, "====================================");
        Logger::info(TAG, "   CREDENTIALS SAVED - CONNECTED!  ");
        Logger::info(TAG, "====================================");
        Logger::info(TAG, String("SSID: ") + WiFi.SSID());
        Logger::info(TAG, String("IP: ") + WiFi.localIP().toString());
        Logger::info(TAG, "====================================");
        wasConnected_ = true;
        reconnectAttempts_ = 0;
        connectedSinceEpoch_ = time(nullptr);
    } else {
        Logger::warning(TAG, "Portal closed without configuration");
        // Stay in portal mode - don't reboot
    }
}

void WiFiManagerExt::resetSettings() {
    Logger::warning(TAG, "");
    Logger::warning(TAG, "====================================");
    Logger::warning(TAG, "     RESETTING WiFi SETTINGS        ");
    Logger::warning(TAG, "====================================");
    
    // Step 1: Disconnect first
    WiFi.disconnect(true, true);
    delay(200);
    
    // Step 2: Reset WiFi Manager settings
    WiFiManager wm;
    wm.resetSettings();
    delay(200);
    
    // Step 3: Clear NVS WiFi namespace
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("WIFI", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_erase_all(nvs_handle);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        Logger::info(TAG, "NVS WiFi storage cleared");
    } else {
        Logger::warning(TAG, "NVS open failed: " + String(err));
    }
    
    // Step 4: Force WiFi off
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    Logger::info(TAG, "====================================");
    Logger::info(TAG, "All WiFi credentials erased!");
    Logger::info(TAG, "Restarting into setup portal...");
    Logger::info(TAG, "====================================");
    Logger::info(TAG, "");
    
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
