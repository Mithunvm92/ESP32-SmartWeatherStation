// Minimal display test - just for ESP32-C3
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_SDA 8
#define I2C_SCL 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Display Test Starting...");

    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("I2C started");

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed!");
        while (1);
    }

    Serial.println("SSD1306 found!");

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("HELLO!");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("ESP32-C3 Test");
    display.display();

    Serial.println("Display updated!");
}

void loop() {
    delay(100);
}
