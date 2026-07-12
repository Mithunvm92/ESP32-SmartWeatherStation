// ============================================================================
// main.cpp — ESP32 Smart Weather & Disaster Station Pro
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include "config.h"
#include "constants.h"
#include "logger.h"
#include "wifi_manager_ext.h"

// ---------------------------------------------------------------------------
// Display Setup
// ---------------------------------------------------------------------------
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define I2C_SDA       8   // ESP32-C3 Super Mini
#define I2C_SCL       10  // ESP32-C3 Super Mini

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------------------------------------------------------------------
// Global subsystem instances
// ---------------------------------------------------------------------------
WiFiManagerExt g_wifi;

// ---------------------------------------------------------------------------
// Display Functions
// ---------------------------------------------------------------------------
void displayBootScreen(const char* line1, const char* line2 = "", const char* line3 = "") {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 Weather Station");
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    
    int y = 18;
    if (line1) { display.setCursor(0, y); display.println(line1); y += 10; }
    if (line2) { display.setCursor(0, y); display.println(line2); y += 10; }
    if (line3) { display.setCursor(0, y); display.println(line3); }
    
    display.display();
}

void displayWiFiStatus() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("=== WiFi STATUS ===");
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    
    if (WiFi.status() == WL_CONNECTED) {
        display.setCursor(0, 14);
        display.println("Status: CONNECTED");
        display.print("IP: ");
        display.println(WiFi.localIP().toString());
        display.print("SSID: ");
        String ssid = WiFi.SSID();
        if (ssid.length() > 15) ssid = ssid.substring(0, 15);
        display.println(ssid);
        display.print("RSSI: ");
        display.print(WiFi.RSSI());
        display.println(" dBm");
    } else {
        display.setCursor(0, 14);
        display.println("Status: DISCONNECTED");
    }
    
    display.display();
}

// ---------------------------------------------------------------------------
// Serial Commands Task
// ---------------------------------------------------------------------------
static void taskSerialCommands() {
    if (!Serial.available()) return;
    
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("resetwifi")) {
        Logger::warning("Main", "Serial command: resetwifi");
        displayBootScreen("WiFi Reset", "Restarting...");
        delay(500);
        g_wifi.resetSettings();
    } 
    else if (cmd.equalsIgnoreCase("status")) {
        Logger::info("Main", "Serial command: status");
        g_wifi.printStatus();
        displayWiFiStatus();
    }
    else if (cmd.equalsIgnoreCase("reboot")) {
        Logger::warning("Main", "Serial command: reboot");
        displayBootScreen("Rebooting...");
        delay(500);
        ESP.restart();
    }
    else if (cmd.equalsIgnoreCase("heap")) {
        Logger::info("Main", "Serial command: heap");
        Logger::info("Main", String("Free heap: ") + ESP.getFreeHeap() + " bytes");
    }
    else {
        Logger::info("Main", String("Unknown: ") + cmd);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Unknown: " + cmd);
        display.println("Cmds: resetwifi");
        display.println("       status");
        display.println("       reboot");
        display.display();
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("");
    Serial.println("========================================");
    Serial.println("ESP32 Smart Weather Station");
    Serial.println("========================================");

    // Initialize I2C for OLED
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[Display] SSD1306 allocation failed!");
        for (;;); // Don't proceed, loop forever
    }
    
    Serial.println("[Display] SSD1306 initialized");
    displayBootScreen("Starting...", "Weather Station");
    delay(1000);

    // WiFi
    Serial.println("[WiFi] Connecting...");
    displayBootScreen("Connecting", "to WiFi...");
    
    bool wifiResult = g_wifi.begin();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected!");
        displayBootScreen("WiFi Connected!", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WiFi] Failed!");
        displayBootScreen("WiFi Failed!", "Check credentials");
    }
    
    delay(2000);

    Serial.println("Setup complete!");
    Serial.println("Commands: resetwifi, status, reboot, heap");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
    taskSerialCommands();
    displayWiFiStatus();
    delay(2000);
    yield();
}
