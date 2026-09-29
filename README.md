# Smart Aquarium Controller: ESP8266-Based Automation for Aquatlantis BioBox (v3.0)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP--12F%20%7C%20ESP8266-blue?logo=espressif)](https://www.espressif.com/en/products/socs/esp8266)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

A modular, production-ready C++ firmware for the **LC-Relay-ESP12-4R-MV** board (powered by the **ESP-12F / ESP8266** WiFi module). Designed specifically for automating freshwater planted aquariums (such as the Aquatlantis BioBox 56L), this controller automates lighting schedules, synchronizes CO2 solenoid valves, drives nocturnal aeration, supports custom user presets, and features a low-voltage **UPS Failsafe Mode** to keep your aquatic ecosystem alive during power outages.

---

## 📖 Documentation & Guides

Explore the detailed architecture and manuals in the `docs/` directory:
*   **[End-User & Configuration Guide (docs/user_guide.md)](docs/user_guide.md)** – Network discovery instructions (Windows / iOS / Android), custom IO naming, presets management, OTA updates, and troubleshooting.
*   **[Detailed Electrical Wiring Diagram (docs/wiring_diagram.txt)](docs/wiring_diagram.txt)** – Common Rail low-voltage UPS system, Schottky diode isolation, and GPIO0 voltage divider layout.
*   **[UI Wireframe & REST API Specification (docs/ui_design.md)](docs/ui_design.md)** – Complete REST endpoints, JSON payloads, and UI architecture.
*   **[State Machine Logic (docs/logical_diagram.mermaid)](docs/logical_diagram.mermaid)** – Day, Night, and Power Loss Failsafe state transition flow.

---

## 🚀 Key Features (v3.2.0)

*   **Compact Variant C Peripheral Controls:** Dual interactive action badges (`[ AUTO ]` and `[ ⏻ ]`) with dynamic glowing status colors and instant 1-tap overrides right from each relay header, eliminating clutter and saving 40% vertical screen space on mobile.
*   **Active Preset Retention:** Selected preset persists across reboots and page refreshes; dynamically highlights `⚙️ Custom Schedule (Modified)` if hours are manually customized.
*   **Tasteful System Mode Badges:** Subtle, clean mode indicators: `☀️ Day (Normal)`, `🌙 Night (Normal)`, and `⚡ Power Loss (UPS)`.
*   **Custom I/O & Relay Naming:** Rename any relay or digital/analog sensor directly from the Web Interface. All names persist across reboots in LittleFS (`/names.cfg`).
*   **Custom User Presets:** Save your active schedule as a new named preset (e.g. *Plant Growth*, *Algae Treatment*). Switch presets with a single click or restore factory defaults anytime.
*   **Intuitive Pulse Mode (Min & Sec):** Configure intermittent pump cycles with dedicated Minute and Second fields instead of raw seconds.
*   **Smart Feeding Mode:** One-click 10-minute aeration pause with live countdown timer to prevent fish food from scattering or being pulled into skimmers.
*   **Multi-Protocol Network Discovery:**
    *   **Windows:** Resolves instantly via **NetBIOS** (`http://acvariu/`) and **LLMNR**; automatically discovered in Windows Explorer (*This PC -> Network*) via **SSDP / UPnP**.
    *   **Apple iOS / macOS / Android:** Resolves via **mDNS** at `http://acvariu.local/`.
    *   **Direct IP:** Accessible directly via local IP (e.g., `http://192.168.1.32/`).
*   **Reordered & Modular Web Interface:**
    1. **1. System Status** (Operational mode, NTP clock, uptime, WiFi, IP, active feeding pause alert)
    2. **2. Peripheral Controls & Status** (Dual action badges, live countdown timers, feeding mode trigger)
    3. **3. 24h Schedule Configuration & Presets** (Visual 24-hour hour grid, Pulse min/sec, Preset manager)
    4. **4. Digital Inputs & Auxiliary Sensors** (GPIO states + real-time Analog Light Sensor meter)
    5. **5. System Administration** (Hardware renaming, WiFi setup, OTA firmware flashing)
    6. **6. System Log** (Single-line compact log, timestamp `DD.MM HH:MM:SS` without year, verified confirmations)
*   **100% Self-Contained:** Self-sufficient dashboard running with zero CDN dependencies for reliable local-network operation.
*   **UPS Power Loss Failsafe (MODE_POWER_LOSS):** Hardware line on `GPIO0` monitors mains voltage. On power loss, high-load peripherals shut off while the Air Pump runs in battery-saving pulse mode (1 min ON / 2 min OFF).
*   **Network Time Synchronization (NTP):** Real-time clock syncing using POSIX timezone rules for Romania (EET/EEST) with automatic Daylight Saving Time (DST).
*   **Over-The-Air (OTA) Updates:** Flash new firmware wirelessly through any browser at `http://acvariu.local/update` (or `http://acvariu/update`).

---

## 🔌 Hardware GPIO Allocation (Pinout)

*   **Relay 1 (GPIO16):** Main Lighting (12V DC LED Lamp)
*   **Relay 2 (GPIO14):** CO2 Solenoid Valve (12V DC, Normal Closed)
*   **Relay 3 (GPIO12):** Air Pump (5V DC, via Buck Converter)
*   **Relay 4 (GPIO13):** Ambient Lighting (12V DC LED Strip)
*   **GPIO 0:** Mains Voltage Power Sense (Debounced, active LOW on failure)
*   **GPIO 4, 2, 15:** Available Digital Inputs (Sensors, float switches, or diagnostic jumpers)
*   **ADC0 (A0):** Analog Light Sensor (LDR)
*   **GPIO 5 (Status LED):** Onboard blue LED (Active LOW) signaling WiFi and NTP status.

---

## 🛠️ File Structure

```text
smart-aquarium-esp8266/
├── include/
│   ├── Config.h         <-- Pin mappings, WiFi credentials, default 24h schedule bitmaps
│   ├── NetworkSync.h    <-- NTP sync declarations, WiFi state routines, mDNS/LLMNR/SSDP
│   └── RelayControl.h   <-- Custom names, User presets, Feed mode, LittleFS persistence
├── src/
│   ├── NetworkSync.cpp  <-- WiFi manager, NTP state machine, status LED blink patterns
│   ├── RelayControl.cpp <-- Rolling logger, LittleFS persistence (/names.cfg, /user_presets.cfg)
│   └── main.cpp         <-- Arduino setup/loop, REST APIs, SSDP/NetBIOS, serves the dashboard
├── web/
│   └── index.html       <-- Dashboard Web UI (edit here; gzipped into flash at build time)
├── tools/
│   └── build_web.py     <-- PlatformIO pre-build script: web/index.html -> include/dashboard_html_gz.h
└── platformio.ini       <-- PlatformIO configuration file targeting ESP-12E board
```

---

## ⚡ Flashing & Development

This project is built and managed using **PlatformIO**.

### Prerequisites
1.  Connect UART pins (`TX`, `RX`, `GND`, `3.3V`/`5V`) on the LC-Relay board.
2.  Connect `GPIO0` to `GND` and power cycle to enter **UART Download Mode**.

### Build & Upload via CLI
```bash
# Build firmware
pio run

# Upload via Serial
pio run --target upload

# Serial Monitor (115200 Baud)
pio device monitor --baud 115200
```

### Wireless (OTA) Flashing
Once installed, update without cables by navigating to:
**`http://acvariu/update`** or **`http://acvariu.local/update`** and uploading `.pio/build/esp12e/firmware.bin`.
