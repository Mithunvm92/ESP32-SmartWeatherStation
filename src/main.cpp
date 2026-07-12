// ============================================================================
// main.cpp — ESP32 Smart Weather & Disaster Station Pro
// ============================================================================

#include <Arduino.h>

#include "config.h"
#include "constants.h"
#include "logger.h"
#include "wifi_manager_ext.h"

// ---------------------------------------------------------------------------
// Global subsystem instances
// ---------------------------------------------------------------------------
WiFiManagerExt g_wifi;

// ---------------------------------------------------------------------------
// Serial Commands Task
// ---------------------------------------------------------------------------
static void taskSerialCommands() {
    if (!Serial.available()) return;
    
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("resetwifi")) {
        Logger::warning("Main", "Serial command: resetwifi - clearing WiFi credentials...");
        delay(200);
        g_wifi.resetSettings();
    } 
    else if (cmd.equalsIgnoreCase("status")) {
        Logger::info("Main", "Serial command: status");
        g_wifi.printStatus();
        Logger::info("Main", String("Free heap: ") + ESP.getFreeHeap() + " bytes");
        Logger::info("Main", String("Uptime: ") + millis() + " ms");
    }
    else if (cmd.equalsIgnoreCase("reboot")) {
        Logger::warning("Main", "Serial command: reboot");
        Logger::info("Main", "Rebooting...");
        delay(500);
        ESP.restart();
    }
    else if (cmd.equalsIgnoreCase("heap")) {
        Logger::info("Main", "Serial command: heap");
        Logger::info("Main", String("Free heap: ") + ESP.getFreeHeap() + " bytes");
        Logger::info("Main", String("Heap size: ") + ESP.getHeapSize() + " bytes");
        Logger::info("Main", String("Min free heap: ") + ESP.getMinFreeHeap() + " bytes");
    }
    else {
        Logger::info("Main", String("Unknown command: '") + cmd + "'");
        Logger::info("Main", "Available: resetwifi, status, reboot, heap");
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    
    // Immediate output before any initialization
    Serial.println("");
    Serial.println("========================================");
    Serial.println("ESP32 Smart Weather Station - BOOTING");
    Serial.println("========================================");
    Serial.println("");

    Logger::setMinLevel(LogLevel::DEBUG);

    Logger::info("Main", "System starting...");
    Logger::info("Main", String("Free heap: ") + ESP.getFreeHeap());

    // WiFi (blocking during initial connect or portal)
    Logger::info("Main", "Calling WiFi begin...");
    Serial.println("[Main] About to call g_wifi.begin()");
    
    bool wifiResult = g_wifi.begin();
    
    Serial.println("[Main] WiFi begin returned: " + String(wifiResult));
    Serial.println("[Main] WiFi IP: " + g_wifi.ip());
    Logger::info("Main", String("WiFi connected! IP: ") + g_wifi.ip());

    Logger::info("Main", "Setup complete!");
    Serial.println("");
    Serial.println("========================================");
    Serial.println("Setup complete - entering loop");
    Serial.println("========================================");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
    taskSerialCommands();
    delay(100);
    yield();
}
