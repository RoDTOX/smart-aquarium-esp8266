# UI Design Plan & REST API: Aquatlantis Dashboard (v3.2)

This document describes the web interface architecture and REST API endpoints hosted on the ESP8266 controller for the Aquatlantis BioBox 56L aquarium.

## 1. Visual Concepts & Color Palette

To provide a sleek, non-fatiguing experience next to an illuminated aquarium at night, the UI uses a dark glassmorphism aesthetic based on CSS variables:

*   **Base Background (`--bg-base`):** `#080d1a` (deep midnight blue).
*   **Card Background (`--bg-surface` / `--bg-card`):** `#111827` and `#1a233a` with subtle translucency.
*   **Accent Color (`--accent-primary`):** `#38bdf8` (vibrant cyan).
*   **Status Indicators:**
    *   *OK / Active (`--state-ok`):* `#10b981` (emerald green with pulse indicator).
    *   *Warning / Pulse Rest (`--state-warn`):* `#f59e0b` (amber).
    *   *Danger / Manual Forced Off (`--state-danger`):* `#ef4444` (crimson red).
*   **Zero External CDN Dependencies:** All styling, typography, and SVG/Unicode icons are self-hosted directly from flash memory for 100% offline local network reliability.

---

## 2. Layout Structure & Navigation Flow

The dashboard is structured into a prioritized modular layout:

```text
+--------------------------------------------------------------+
| [Header] Aquatlantis Smart Aquarium                          |
+--------------------------------------------------------------+
| [Banner] 🫧 Feed Mode Active (09:45 remaining)      [Cancel] |
+--------------------------------------------------------------+
| 1. SYSTEM STATUS                                             |
|  - Operating Mode: ☀️ Day (Normal) / 🌙 Night / ⚡ Power Loss |
|  - NTP Clock, Uptime, WiFi Network, RSSI, IP, Hostname       |
+--------------------------------------------------------------+
| 2. PERIPHERAL STATUS & CONTROL (Variant C Compact)           |
|  - Relay 1 [Main Light] [GPIO 16]         [AUTO]  [⏻]        |
|    └─ Pill: ⚪ Off • Starts at 13:00                         |
|  - Relay 2 [CO2 Solenoid] [GPIO 14]       [AUTO]  [⏻]        |
|    └─ Pill: ⚪ Off • Starts at 11:00                         |
|  - Relay 3 [Air Pump] [GPIO 12]           [AUTO]  [⏻]        |
|    └─ Pill: 🟢 Running • Pause in 9m 54s                     |
|  - Relay 4 [Ambient Light] [GPIO 13]      [AUTO]  [⏻]        |
|    └─ Pill: 🟢 On • Until 0:00                               |
|  - Quick Actions: [🫧 Feed Mode (10m)]  [Auto (All)]          |
+--------------------------------------------------------------+
| 3. 24H SCHEDULE CONFIGURATION & PRESETS                      |
|  - Relay Selector (1-4) with dynamic name                    |
|  - Visual 24-hour hour grid (00:00 - 23:00)                  |
|  - Operating Mode: Continuous / Intermittent (Pulse)         |
|  - Pulse Duration: Run Time [Min][Sec] / Rest Time [Min][Sec]|
|  - [💾 Save Relay Schedule]                                  |
|  - Presets: [Dropdown] [Apply] [Save New] [Delete]           |
+--------------------------------------------------------------+
| 4. DIGITAL INPUTS & AUXILIARY SENSORS (I/O)                  |
|  - GPIO 0 (Power Sensor), GPIO 4, GPIO 2, GPIO 15            |
|  - Light Sensor A0: Live dynamic progress meter              |
+--------------------------------------------------------------+
| 5. SYSTEM ADMINISTRATION                                     |
|  - [✏️ Rename Relays & Sensors]                              |
|  - [📶 WiFi Setup]                                           |
|  - [⬆️ OTA Firmware Flash]                                   |
+--------------------------------------------------------------+
| 6. SYSTEM LOG (COMPACT TIMELINE)                             |
|  - [DD.MM HH:MM:SS] Single-line verified system events       |
|  - Explicit user operation confirmations                     |
+--------------------------------------------------------------+
```

---

## 3. REST API Specification (JSON)

### `GET /api/status`
Returns complete telemetry, custom hardware names, active preset, and feed mode status:
```json
{
  "mode": "🌙 Night (Normal)",
  "mode_id": 1,
  "feed_mode": false,
  "feed_remaining": 0,
  "inputs": {
    "gpio0": true,
    "gpio4": true,
    "gpio2": true,
    "gpio15": false
  },
  "time": "2026-09-27 23:23:27",
  "wifi_ssid": "Home_WiFi",
  "wifi_rssi": -63,
  "wifi_status": "Connected",
  "ip": "192.168.1.32",
  "uptime": 86400,
  "light_percent": 8,
  "active_preset": 1,
  "relays": [
    {"num": 1, "name": "Main Light", "state": false, "override": false, "override_state": false, "status_desc": "⚪ Off • Starts at 13:00"},
    {"num": 2, "name": "CO2 Solenoid", "state": false, "override": false, "override_state": false, "status_desc": "⚪ Off • Starts at 11:00"},
    {"num": 3, "name": "Air Pump", "state": true, "override": false, "override_state": false, "status_desc": "🟢 Running • Pause in 9m 54s"},
    {"num": 4, "name": "Ambient Light", "state": true, "override": false, "override_state": false, "status_desc": "🟢 On • Until 0:00"}
  ],
  "names": {
    "relays": ["Main Light", "CO2 Solenoid", "Air Pump", "Ambient Light"],
    "inputs": ["Power Sensor (GPIO0)", "GPIO 4 (Free)", "GPIO 2 (Free)", "GPIO 15 (Free)"],
    "analog": "Light Sensor (A0)"
  }
}
```

### `POST /api/names`
Updates custom peripheral names and saves them to LittleFS (`/names.cfg`).
Parameters (`application/x-www-form-urlencoded`):
* `r1, r2, r3, r4`: Names for relays 1..4 (max 31 chars).
* `in0, in4, in2, in15`: Names for digital inputs (max 31 chars).
* `a0`: Name for analog sensor input.

### `GET /api/presets`
Returns all presets (built-in factory presets + user-defined presets):
```json
[
  {"id": 1, "name": "Preset 1: Standard Aquatlantis (Factory)", "builtin": true},
  {"id": 2, "name": "Preset 2: Algae Control (Factory)", "builtin": true},
  {"id": 3, "name": "Preset 3: Maintenance / Lights Off (Factory)", "builtin": true},
  {"id": 100, "name": "Summer Schedule", "builtin": false}
]
```

### `POST /api/presets`
Presets management:
* `apply=[id]`: Applies specified preset (1..3 or 100..107).
* `save=1&name=[name]`: Saves current 4-relay schedule under specified custom name.
* `delete=[id]`: Deletes custom preset (id >= 100).
* `reset=1`: Resets schedules to factory defaults.

### `POST /api/feed`
Controls Feeding Mode:
* `action=start&duration=600`: Pauses air pump for 10 minutes (600s).
* `action=stop`: Immediately cancels feeding pause and resumes schedule.

### `GET /api/schedule` & `POST /api/schedule`
* `GET`: Returns the array of 4 relay profiles.
* `POST`: Saves schedule for specified relay:
  * `relay=[1-4]`
  * `active_hours=[bitmap 24h]`
  * `behavior=[0|1]` (0 = continuous, 1 = pulse)
  * `pulse_on=[seconds]`
  * `pulse_off=[seconds]`

### `POST /api/override`
Manual override configuration:
* `clear=1`: Resets all relays to automatic schedule.
* `relay=[1-4]&override=[0|1]&state=[0|1]`: Sets manual forced state.

### `GET /api/history`
Returns event logs in compact single-line format (`DD.MM HH:MM:SS`):
```json
[
  {"time": "27.09 23:20:15", "msg": "System started • Relays initialized"},
  {"time": "27.09 23:22:40", "msg": "Schedule saved: Main Light [Continuous]"}
]
```

### `GET /description.xml`
SSDP / UPnP device schema used by Windows File Explorer for automatic network discovery.
