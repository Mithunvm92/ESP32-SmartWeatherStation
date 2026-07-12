# ESP32 Smart Weather & Disaster Station

ESP32-C3 based weather station with WiFiManager for easy setup.

## Features

- **WiFiManager Integration**: Auto-connect to saved networks, fallback to captive portal
- **Setup Portal**: SSID `WeatherStation-Setup`, Password `weather123`
- **Serial Commands**: `resetwifi`, `status`, `reboot`, `heap`

## WiFiManager Behavior

| Scenario | Result |
|----------|--------|
| No saved credentials | Immediately broadcasts `WeatherStation-Setup` |
| Saved WiFi works | Connects and continues |
| Saved WiFi fails | Retries 5 times, then launches portal (no reboot) |

## Serial Commands

- `resetwifi` - Clear credentials and restart into portal
- `status` - Show WiFi status, heap, uptime
- `reboot` - Restart device
- `heap` - Show memory info

## Serial Output Example

```
[0000000000][INFO][WiFi] ****************************************
[0000000000][INFO][WiFi] *       WiFiManagerExt::begin()        *
[0000000000][INFO][WiFi] ****************************************
[0000000000][INFO][WiFi] ========================================
[0000000000][INFO][WiFi]        SETUP PORTAL ACTIVE         
[0000000000][INFO][WiFi] ========================================
[0000000000][INFO][WiFi] SSID: WeatherStation-Setup
[0000000000][INFO][WiFi] Password: weather123
[0000000000][INFO][WiFi] IP: 192.168.4.1
[0000000000][INFO][WiFi] Connect from phone now!
[0000000000][INFO][WiFi] ========================================
```

## Build

```bash
pio run
pio run --target upload
```

## Hardware

- ESP32-C3 Super Mini
- SSD1306 OLED (128x64)
