# Changelog - Smart Aquarium Controller (ESP8266)

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [3.3.0] - 2026-09-28

### Direct Preset Overwrite, 24h Visual Infographic Timeline & Vibrant Aquatic Theme Overhaul
* **Direct Preset Overwrite & Silent Auto-Commit (Section 3):**
  * Added in-place preset overwriting via backend API (`POST /api/presets` with `overwrite=<id>`) and `overwriteUserPreset(int id)`.
  * Dedicated `[ 🔄 Overwrite "<Name>" ]` button in the Presets toolbar when a user preset is active or selected.
  * Smart Save Preset modal: auto-detects existing preset names in real time, displays an overwrite advisory, and switches the action button to overwrite mode.
  * Silent schedule auto-commit (`saveActiveRelayScheduleSilently()`): automatically synchronizes modified hour cells and pulse parameters to ESP8266 RAM/EEPROM prior to tab switching, saving, or overwriting, eliminating unsaved schedule state mismatches.
* **24-Hour Multi-Relay Visual Timeline (Section 1: System Status):**
  * Added an interactive 0–24h horizontal timeline in System Status displaying schedule distribution across all 4 peripherals simultaneously.
  * Distinctive visual coding: Continuous active hours rendered with vibrant gradient bars; Intermittent / Pulse mode active hours rendered with a high-contrast diagonal hatched stripe pattern (`repeating-linear-gradient`).
  * Real-time "NOW" vertical cursor and red time pin tracking NTP clock with automatic recalculation on window resize.
  * Interactive tracks: clicking any relay label switches the active scheduler tab, and clicking any hour slot toggles that hour in the editor.
* **Cheerful Aquatic Theme Overhaul (UI / UX):**
  * Replaced dark midnight theme with an inviting, vibrant aquatic aesthetic: turquoise and azure ocean gradient, volumetric sunbeams, subtle animated floating bubbles, and seabed illustrations (corals, starfish, sea shells, treasure chest).
  * High-contrast frosted glass cards (`rgba(255, 255, 255, 0.78)` with `backdrop-filter: blur(18px)`) ensure legibility in bright aquarium lighting.
  * Aquatic icons (`🐟`, `🦐`, `🫧`, `🐚`) on relay tabs and marine helm indicators (☸️) on peripheral cards.
  * Fully self-contained within ESP8266 flash (~47.4% Flash, ~48.9% RAM) with 0 external CDN dependencies.

---

## [3.2.1] - 2026-09-27

### Section 3 Relay Tabs, Modification Race Condition Fix & Schedule Verification
* **Touch-Friendly 4-Button Relay Tab Selector (Section 3):**
  * Replaced the `<select id="relay-select">` dropdown with 4 dedicated responsive tab buttons (`1. Main Light`, `2. CO2 Solenoid`, `3. Air Pump`, `4. Ambient Light`).
  * The active relay is highlighted with an illuminated cyan glow; a single tap instantly switches the 24h schedule view.
  * Button labels update dynamically whenever custom hardware names are modified.
* **Resolved Unsaved Modification Race Condition:**
  * Fixed an issue where manual schedule adjustments (toggling hour cells or modifying pulse min/sec) reverted back to the active preset after 1-2 seconds.
  * Added client-side `isScheduleDirty` state tracking to prevent background status polling (`/api/status`) from overriding the `⚙️ Custom Schedule (Modified)` state before the user clicks save.

---

## [3.2.0] - 2026-09-27

### Peripheral Controls Variant C, Preset Retention & Full English Localization
* **Compact Variant C Peripheral Controls (Section 2):**
  * Eliminated the 3rd controls row (manual toggle buttons), reducing card height by ~40% for optimal mobile usability.
  * Header layout: custom relay name and discrete hardware pin tag (`GPIO XX`) on the left, dual interactive action badges on the right:
    * `[ AUTO ]`: Active vibrant green (`#10b981`) when in automatic schedule; neutral slate gray when overridden. Clicking reverts relay to Auto mode with 1 tap.
    * `[ ⏻ ]`: Vibrant glowing green (`#10b981`) with `⏻ ON` text when forced ON; glowing crimson red (`#ef4444`) with `⏻ OFF` text when forced OFF; neutral slate gray when in Auto. Clicking toggles manual override state with 1 tap.
  * Unified single status pill directly beneath the header (`🟢 Running • Pause in 6m 55s`, `⚪ Off • Starts at 13:00`, `🟡 Manual ON`, `🔴 Manual OFF`).
* **Active Preset Retention & Custom Schedule Indication (Section 3):**
  * Persisted `activePresetId` in non-volatile flash (`/active_preset.cfg`).
  * Dropdown retains and displays the active preset across reboots and page refreshes.
  * When any schedule or pulse setting is manually modified, the selector automatically switches to `⚙️ Custom Schedule (Modified)`.
* **Tasteful System Mode Badges (Section 1):**
  * Added clean, non-intrusive status indicators: `☀️ Day (Normal)`, `🌙 Night (Normal)`, and `⚡ Power Loss (UPS)`.
  * Removed decorative orange bullet from Section 1 header for a clean, professional aesthetic.
* **Actuators vs. Telemetry Architecture Clarification:**
  * Maintained strict separation between Section 2 (Actuators / switched power loads on GPIO 16, 14, 12, 13) and Section 4 (Telemetry / auxiliary digital logic inputs and ADC).
  * Preserves clean mobile ergonomics and ensures scalability for future modular sensors (DS18B20 1-Wire temperature probe, water level float switches).
* **Frontend Robustness & Reference Fix:**
  * Declared global `activePresetId` and added null-safe element checks in `fetchPresets()` and `fetchStatus()`, ensuring reliable rendering of custom relay names and user presets.
* **Comprehensive English Localization:**
  * 100% of firmware code, comments, web UI dashboard, OTA updater, system event logs, and repository documentation translated to English.

---

## [3.1.1] - 2026-09-27

### Text Refactoring, Redundancy Elimination & Compact Layout
* **Visual Redundancy Elimination (Section 2: Peripheral Controls & Status):**
  * Removed duplicate `OFF` / `ON` text in relay corner; unified into a single status pill.
  * Discrete hardware pin tag (`GPIO 16`, `GPIO 14`, etc.) positioned alongside equipment name.
  * Concise status messages:
    * `⚪ Off • Starts at 13:00`
    * `🟢 Running • Pause in 9m 59s`
    * `⏸️ Paused • Resumes in 6m 55s`
    * `🟢 On • Until 21:00`
    * `⚪ Off • No active hours`
* **Schedule Configuration Clarification (Section 3: 24h Schedule & Presets):**
  * Replaced redundant label `ACTIVE BEHAVIOR: Continuous (Always On)` with clear mode indicator: `Mode during checked hours: Continuous (full duration) / Intermittent (pulsed ON / OFF cycles)`.
  * Clarified pulse duration fields: `Pulse ON (Run Time)` and `Pause OFF (Rest Time)`.
* **Header Polish & WiFi Indicator Relocation (Section 1: System Status):**
  * Removed disconnected `(Connected)` badge from main page header for clean look.
  * Relocated `🟢 Connected` indicator directly to Section 1 title bar.
  * NTP clock adjusted to strict `HH:MM:SS` format to prevent truncation on mobile screens.
* **Event Log Text Polish (Section 6: System Log):**
  * Rewritten all system log messages into concise, professional phrases:
    * `System booted • Controller online`
    * `OTA update started (firmware.bin)`
    * `OTA update completed (456 KB) • Rebooting`
    * `New firmware active • Connected to WiFi (192.168.1.32)`
    * `Preset applied: "First"`
    * `Schedule saved: Air Pump [Pulse: 10m ON / 10m OFF]`
    * `Manual command: Air Pump -> ON`
    * `Air Pump -> Reverted to Auto`
    * `Feeding Mode started (10 min) • Pump paused`
    * `Feeding Mode finished • Schedule resumed`

---

## [3.1.0] - 2026-09-27

### UX Improvements, Live Pulse Countdown & Mobile Reliability
* **Live Monitoring & Countdown Timers (Section 2: Peripheral Controls & Status):**
  * Added dynamic colored status banner under each relay showing exact remaining time:
    * In Pulse mode: eliminated confusion over `OFF` status by showing active pause (`⏸️ Paused • Resumes in 1m 20s`) or remaining active run time (`🟢 Running • Pause in 45s`).
    * In Continuous mode: displays active scheduled window (`🟢 On • Until 21:00`).
    * When off by schedule: displays next start time (`⚪ Off • Starts at 13:00`).
    * In Feeding Mode or manual override: prominent yellow/red indicators.
* **Full OTA Flash Audit & Reconnect Logging:**
  * Native OTA endpoint at `/update` with non-volatile flash logging hooks.
  * Logs upload start with binary filename (`OTA Flash STARTED: Writing 'firmware.bin'...`).
  * Logs successful completion with exact byte size (`OTA Flash SUCCESS: 'firmware.bin' (457.2 KB) written to Flash`).
  * On reboot after WiFi connection and NTP sync, automatically logs resumption: `WiFi connection re-established after Flash... (IP: 192.168.1.32, RSSI: -62 dBm). Firmware v3.1 active.`
* **Mobile Dropdown Flickering Fix (Section 3):**
  * Protected relay select element against re-rendering while native mobile picker is open (`document.activeElement !== sel`).
* **Mobile System Log Optimization (Section 6):**
  * Removed year from timestamps (`DD.MM HH:MM:SS`).
  * Added responsive layout with `word-break: break-word;` and smooth horizontal/vertical scrolling on narrow screens.
* **System Status Cleanup (Section 1):**
  * Removed ambient light sensor from Section 1 (consolidated in Section 4 dedicated to hardware sensors).
* **Typography & Visual Design Modernization:**
  * Imported Google Fonts: `Outfit` (modern geometric UI font) and `JetBrains Mono` (high-legibility monospaced font for clock, IP, and logs).
  * Refined dark glassmorphism design with deep midnight blue tones, translucent borders, and subtle glow effects.

---

## [3.0.0] - 2026-09-27

### Major Release & Core Feature Additions
* **Dynamic Relay & Sensor Renaming (Custom I/O Names):**
  * User can rename all 4 relays, auxiliary digital inputs (GPIO 0, 4, 2, 15), and analog sensor directly from the admin panel.
  * Names stored on LittleFS (`/names.cfg`), persisting across reboots and firmware updates.
* **Custom Named Presets:**
  * Ability to save the current 4-relay schedule as a new named preset (stored in `/user_presets.cfg`).
  * 1-click preset activation, preset deletion, and factory reset.
* **Multi-Protocol Local Network Discovery (Windows & Mobile):**
  * **NetBIOS (`NBNS`):** Direct Windows access via `http://acvariu/`.
  * **LLMNR:** Local name resolution for Windows 10 and 11.
  * **SSDP / UPnP:** Device automatically appears in *Windows File Explorer* -> *Network* as "Aquatlantis Smart Aquarium".
  * **mDNS:** Access via `http://acvariu.local/` on iOS, macOS, and Android.
* **Smart Feeding Mode:**
  * Dedicated "🫧 Feeding Mode (10m)" button in Peripheral Controls.
  * Temporarily pauses air pump / filtration for 10 minutes to prevent food dispersion.
  * Real-time countdown timer banner with automatic schedule resumption upon expiration.
* **Separate Pulse Mode Duration Fields (Minutes & Seconds):**
  * Replaced raw seconds input with 4 intuitive fields: `ON Time [Min][Sec]` and `OFF Time [Min][Sec]`.
* **Modular Web Interface Restructuring:**
  * Reordered sections by operational importance:
    1. *System Status* (Network indicators, mode, NTP clock, feeding alert).
    2. *Peripheral Controls* (Manual commands, auto force, feeding pause).
    3. *24h Schedule Configuration & Presets* (24h grid, continuous/pulse modes, preset manager).
    4. *Digital Inputs & Sensors (I/O)* (Pin logic states and live LDR progress bar).
    5. *System Administration* (Hardware renaming, WiFi setup, OTA flash).
    6. *System Log* (Compact timeline).
* **Single-Line Event Log & Explicit Confirmations:**
  * Compact single-line format: `[DD.MM HH:MM:SS] Message`.
  * Removed year from timestamp for clarity and space efficiency.
  * Explicit confirmation toasts on every action.
* **Zero-CDN Architecture (100% Offline Capability):**
  * Removed external CDN dependency (Chart.js).
  * Replaced chart with live real-time progress bar for LDR sensor (0% - 100%).
  * Web interface loads instantly on LAN even without internet connectivity.
* **Complete Documentation Overhaul:**
  * Updated `README.md`, `docs/user_guide.md`, and `docs/ui_design.md` to reflect v3.0 architecture.

---

## [2.0.0] - 2026-05-30

### Features & Stability
* **NTP Time Synchronization:** Accurate scheduling without external RTC hardware, configured with Romania timezone (EET/EEST) and automatic DST transitions.
* **LittleFS Persistence:** Relay schedules saved in binary format (`/schedules.cfg`), eliminating EEPROM wear.
* **Power Loss Failsafe Mode (UPS Mode):** Mains power monitoring on GPIO0. On AC power loss, disconnects high-load lighting and CO2, switching air pump to pulsed conservation mode (1m ON / 2m OFF).
* **Wireless Updates (OTA Web Update):** Dedicated secure portal at `/update` for serial-free firmware updates.
* **Defective Lamp Diagnostic:** Automated correlation between Relay 1 (Light) and LDR sensor (alerts if light is ON but sensor reads below 15%).
* **Log Auto-Rotation:** `/log.txt` capped at 2.5 KB to protect flash memory endurance.

---

## [1.0.0] - 2026-04-15

### Initial Release
* PlatformIO project scaffold for LC-Relay-ESP12-4R-MV board (ESP-12F).
* Pin allocation for 4 relays and 4 digital inputs.
* Basic HTTP ON/OFF control via ESP8266WebServer.
* Fallback Access Point mode (`BioBox-Aquarium`).
