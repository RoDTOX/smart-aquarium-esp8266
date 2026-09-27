# System Patterns & Architecture

## Architecture Overview
- **Hardware Layer:** LC-Relay-ESP12-4R-MV (ESP8266 ESP-12F) with 4 optocoupler-isolated relays, 4 digital input headers (GPIO 0, 4, 2, 15), and 1 ADC channel (A0).
- **Core Firmware Loop:** Non-blocking state machine in `main.cpp` calling submodules (`updateNetwork()`, `updateRelays()`).
- **Persistence Patterns:**
  - LittleFS binary structs: `/schedules.cfg` (Relay profiles), `/names.cfg` (Custom I/O names), `/user_presets.cfg` (User presets), `/active_preset.cfg` (Currently applied preset index).
  - LittleFS rolling logger: `/log.txt` auto-rotates at 2.5 KB to prevent flash degradation.
- **Network Discovery Stack:**
  - `ESP8266NetBIOS`: Windows native resolution (`http://acvariu/`).
  - `ESP8266LLMNR`: Windows 10/11 local link resolution.
  - `ESP8266SSDP`: UPnP device broadcasting (*Aquatlantis Smart Aquarium* in Windows Explorer).
  - `ESP8266mDNS`: Bonjour resolution (`http://acvariu.local/`).
- **UI Architecture:**
  - Self-hosted single-page HTML5/CSS3 application stored in flash memory (`DASHBOARD_HTML`).
  - RESTful polling every 2s via `/api/status`, `/api/schedule`, `/api/presets`, `/api/history`.
  - Variant C Compact Relay Cards: Header contains equipment name, GPIO tag, and dual interactive action badges (`[ AUTO ]` & `[ ⏻ ]`).
