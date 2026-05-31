# Smart Aquarium Controller: ESP8266-Based Automation for Aquatlantis BioBox

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP--12F%20%7C%20ESP8266-blue?logo=espressif)](https://www.espressif.com/en/products/socs/esp8266)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

A modular, production-ready C++ firmware for the **LC-Relay-ESP12-4R-MV** board (powered by the **ESP-12F / ESP8266** WiFi module). Designed specifically for automating freshwater planted aquariums (such as the Aquatlantis BioBox 56L), this controller automates lighting schedules, synchronizes CO2 solenoid valves, drives nocturnal aeration, and features a low-voltage **UPS Failsafe Mode** to keep your aquatic ecosystem alive during power outages.

---

## 📖 Documentation & Wiring Guides

Explore the detailed architecture and manuals in the `docs/` directory:
*   **[End-User & Configuration Guide (docs/user_guide.md)](docs/user_guide.md)** – WiFi credentials setup, Tailscale VPN remote proxy routing, OTA HTTP updates, and AI handover rules.
*   **[Detailed Electrical Wiring Diagram (docs/wiring_diagram.txt)](docs/wiring_diagram.txt)** – Common Rail low-voltage UPS system, Schottky diode isolation, and GPIO0 voltage divider layout.
*   **[UI Wireframe & REST API Specification (docs/ui_design.md)](docs/ui_design.md)** – REST endpoints, JSON payloads, and responsive glassmorphic frontend styling.
*   **[State Machine Logic (docs/logical_diagram.mermaid)](docs/logical_diagram.mermaid)** – Day, Night, and Power Loss Failsafe state transition flow.

---

## 🚀 Key Features

*   **Network Time Synchronization (NTP):** Real-time clock syncing using POSIX timezone rules for Romania (EET/EEST) with automatic Daylight Saving Time (DST) handling—no external hardware RTC chip (e.g., DS3231) required.
*   **LittleFS Storage Persistence:** All relay schedules and the rolling event history log are stored directly in the onboard flash memory via LittleFS. Configuration persists across restarts, power losses, and firmware flashes.
*   **UPS Power Loss Failsafe (MODE_POWER_LOSS):** Dedicated hardware sense line on `GPIO0` monitors mains voltage. Upon power loss, the controller automatically disables high-draw loads (Main Light, Solenoid CO2, Ambient Light) and pulses the Air Pump (1 min ON / 2 min OFF) to maintain water oxygenation while conserving battery backup.
*   **Continuous Night Aeration:** Unlike wearing out mechanical relays with 24/7 pulse mode, the air pump operates in continuous mode at night (reducing clicking noise and extending relay lifespan to decades).
*   **Digital Input Telemetry:** Real-time logic state monitoring (GPIO 0, 4, 2, 15) represented on the Web UI as color-coded indicators (Green = HIGH/3.3V, Red = LOW/GND).
*   **mDNS Local Addressability (`http://acvariu.local/`):** Access the web dashboard instantly from any local network device without looking up the IP address.
*   **Over-The-Air (OTA) Updates:** Flash new firmware updates wirelessly through any web browser by visiting `http://acvariu.local/update`.
*   **Premium Web UI (Mobile Optimized):** Responsive, dark-themed glassmorphic dashboard (HTML5/CSS3/Vanilla JS) featuring polling data updates, input edit locking, and notification toasts.
*   **Local AP Fallback:** Automatically spins up a secure Access Point (`BioBox-Aquarium`) when the main WiFi network is unreachable, allowing configuration changes at `http://192.168.4.1/`. The AP password is defined in `secrets.h`.

---

## 🔌 Hardware GPIO Allocation (Pinout)

*   **Relay 1 (GPIO16):** Main Lighting (12V DC LED Lamp)
*   **Relay 2 (GPIO14):** CO2 Solenoid Valve (12V DC, Normal Closed)
*   **Relay 3 (GPIO12):** Air Pump (5V DC, via Buck Converter)
*   **Relay 4 (GPIO13):** Ambient Lighting (12V DC LED Strip)
*   **GPIO 0, 4, 2, 15:** Digital Inputs (Mains sense, float switches, or diagnostic jumpers)
*   **GPIO 5 (Status LED):** Onboard blue LED (Active LOW) signaling WiFi status and NTP syncing states.

---

## 🛠️ File Structure

```text
smart-aquarium-esp8266/
├── include/
│   ├── Config.h         <-- Pin mappings, WiFi credentials, default 24h schedule bitmaps
│   ├── NetworkSync.h    <-- NTP sync declarations, WiFi state routines, mDNS service
│   └── RelayControl.h   <-- LittleFS persistence, profile structs, failsafe logic
├── src/
│   ├── NetworkSync.cpp  <-- WiFi manager, NTP state machine, status LED blink patterns
│   ├── RelayControl.cpp <-- Rolling logger (3KB cap), LittleFS schedules management
│   └── main.cpp         <-- Arduino setup/loop, REST APIs, HTTP OTA server, Web UI resource
└── platformio.ini       <-- PlatformIO configuration file targeting the ESP-12E board
```

---

## ⚡ Flashing & Development

This project is built and managed using the **PlatformIO** ecosystem.

### Prerequisites
1.  Locate the UART interface pins (`TX`, `RX`, `GND`, `3.3V`/`5V`) on the LC-Relay board.
2.  If using an Arduino Uno as a USB-to-TTL bridge, wire the Uno's `RESET` pin to `GND` to bypass the ATmega328P.
3.  Connect `GPIO0` to `GND` on the ESP8266.
4.  Power cycle or reset the ESP8266 to enter **UART Download Mode**.

### Build & Upload via CLI
To compile the source files and upload the binary via the auto-detected serial port, run:
```bash
# Compile and Upload
pio run --target upload

# Start Serial Monitor (115200 Baud)
pio device monitor --baud 115200
```
*Note: Once flashed, disconnect `GPIO0` from `GND` and press the physical Reset button to execute the new firmware.*

---

## 🏷️ Suggested GitHub Topics (SEO Tags)
If hosting on GitHub, add these tags to the repository settings to improve search visibility:
`esp8266`, `smart-aquarium`, `aquarium-controller`, `planted-tank`, `home-automation`, `iot-device`, `platformio-ide`, `co2-solenoid`, `littlefs`, `failsafe-system`, `ota-updates`, `glassmorphism`, `espressif`
