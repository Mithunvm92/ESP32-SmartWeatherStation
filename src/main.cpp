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
    delay(300);
    Logger::setMinLevel(LogLevel::INFO);

    Logger::info("Main", "========================================");
    Logger::info("Main", "ESP32 Smart Weather Station");
    Logger::info("Main", "========================================");

    // WiFi (blocking during initial connect or portal)
    Logger::info("Main", "Initializing WiFi...");
    g_wifi.begin();
    Logger::info("Main", String("WiFi connected. IP: ") + g_wifi.ip());

    Logger::info("Main", "Setup complete!");
    Logger::info("Main", "Commands: resetwifi, status, reboot, heap");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
    taskSerialCommands();
    delay(100);
    yield();
}
