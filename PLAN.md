# Development Plan & Backlog: Smart Aquarium Controller

This document tracks all development tasks (TODO and DONE), prioritized by system importance and impact.

> [!IMPORTANT]
> **Security & Git:** This file is versioned in Git. Never include credentials, WiFi passwords, or sensitive keys here!

---

## 🚀 Current System Status

* **Current Firmware:** `v3.3.0`
* **Hardware Status:** Fully assembled, stable, and operating in production (BioBox 56L aquarium).
* **Access Endpoints:** `http://acvariu/` (Windows), `http://acvariu.local/` (Apple/Android), or `http://192.168.1.32/`.

---

## ✅ Done (Completed)

| Date & Time | Priority | Category | Feature / Completed Task | System Impact |
|---|---|---|---|---|
| **2026-09-28 11:54** | **P1 - Critical** | **OTA / Deploy** | **Firmware v3.3.0 build & OTA flash verification** | Firmware v3.3.0 live on ESP8266; aquatic theme, timeline, and preset overwrite verified. |
| **2026-09-28 11:50** | **P1 - Critical** | **Presets / UX** | **Direct Preset Overwrite & Silent Schedule Auto-Commit (Section 3)** | Enabled in-place preset overwriting (`overwriteUserPreset()`), smart modal warning, and auto-sync of modified hours. |
| **2026-09-28 11:45** | **P1 - Critical** | **UI / Visualization** | **24-Hour Multi-Relay Visual Infographic Timeline (Section 1)** | Added 0-24h horizontal timeline with diagonal hatched stripes for pulse mode and live real-time "NOW" cursor. |
| **2026-09-28 11:40** | **P2 - Medium** | **Design / Theme** | **Cheerful Aquatic Theme Overhaul (UI / UX)** | Turquoise-azure ocean gradient, volumetric sunbeams, animated bubbles, seabed decor, and frosted glass cards. |
| **2026-09-27 23:53** | **P2 - Medium** | **UI / Controls** | **Section 3 Relay tab buttons & unsaved modification race condition fix** | Replaced dropdown with 4 dedicated illuminated buttons; resolved status polling overwriting modified state. |
| **2026-09-27 23:45** | **P1 - Critical** | **Hardware** | **Flyback diode (1N4007) installed on 12V CO2 solenoid coil** | Eliminates inductive back-EMF spikes and protects ESP8266 power rail from brownouts. |
| **2026-09-27 23:25** | **P1 - Critical** | **Bugfix / Web UI** | **Fix activePresetId reference error & restore dynamic relay/preset rendering** | Resolved undeclared JS variable preventing dropdown presets and custom relay names from rendering. |
| **2026-09-27 22:55** | **P1 - Critical** | **OTA / Deploy** | **Firmware v3.2.0 build & OTA flash verification** | Firmware v3.2.0 deployed live on ESP8266; Variant C controls & English UI active. |
| **2026-09-27 22:50** | **P1 - Critical** | **UI / Controls** | **Variant C peripheral controls & dual action badges (Section 2)** | Eliminated 3rd row; dual action badges `[ AUTO ]` & `[ ⏻ ]` with dynamic colors & 1-tap override. |
| **2026-09-27 22:45** | **P2 - Medium** | **Presets / UX** | **Active preset retention & custom schedule indicator (Section 3)** | Dropdown retains active preset; displays `⚙️ Custom Schedule (Modified)` when hours are customized. |
| **2026-09-27 22:40** | **P2 - Medium** | **UI / Status** | **Tasteful system mode emojis (Section 1: System Status)** | Clean indicators: `☀️ Day (Normal)`, `🌙 Night (Normal)`, and `⚡ Power Loss (UPS)`. |
| **2026-09-27 22:35** | **P2 - Medium** | **Localization** | **Comprehensive English localization across codebase & documentation** | 100% of firmware code, UI elements, API responses, logs, and docs translated to English. |
| **2026-09-27 22:25** | **P1 - Critical** | **OTA / Deploy** | **Flash firmware v3.1.1 completed and verified over network (OTA)** | Firmware v3.1.1 live on ESP8266; compact layout active. |
| **2026-09-27 22:22** | **P1 - Critical** | **UI / UX** | **Eliminate visual redundancy & compact peripheral cards (Section 2)** | Removed duplicate `OFF` badge, integrated `GPIO` tag, single concise status pill. |
| **2026-09-27 22:20** | **P2 - Medium** | **UI / Clarity** | **Refactor schedule configuration logic (Section 3)** | Replaced ambiguous "Active behavior" with "Mode during checked hours (Continuous / Intermittent)". |
| **2026-09-27 22:18** | **P2 - Medium** | **Header / Layout** | **Header polish & relocated WiFi indicator to Section 1** | Clean header without disconnected badge; clock in strict `HH:MM:SS` format. |
| **2026-09-27 22:15** | **P2 - Medium** | **Audit / Logging** | **System event log text polish (Section 6)** | Concise, professional system messages without repetitive phrasing. |
| **2026-09-27 22:05** | **P1 - Critical** | **OTA / Deploy** | **Flash firmware v3.1 completed and verified over network (OTA)** | Firmware v3.1 live on ESP8266; validated response at `http://192.168.1.32/api/status`. |
| **2026-09-27 22:00** | **P1 - Critical** | **Safety / UX** | **Live countdown timers under relays (Section 2: Peripheral Controls)** | Displays remaining pulse time (`🟢 Running • Pause in 45s`, `⏸️ Resumes in 1m 20s`) or next scheduled trigger. |
| **2026-09-27 21:55** | **P2 - Medium** | **Audit / OTA** | **Automated OTA flash audit & post-flash WiFi reconnection logging** | Logs upload initiation, file name, byte size, and automatic WiFi reconnection on reboot. |
| **2026-09-27 21:50** | **P2 - Medium** | **Mobile / UX** | **Resolve mobile dropdown flickering (Section 3)** | Prevents re-rendering `<select>` elements while the native mobile picker is open. |
| **2026-09-27 21:45** | **P2 - Medium** | **Mobile / Logging** | **Remove year from logs & enable responsive mobile wrapping (Section 6)** | Compact `[DD.MM HH:MM:SS]` logs with word-wrap and smooth horizontal/vertical scrolling. |
| **2026-09-27 21:40** | **P3 - Layout** | **UI / Structure** | **Remove ambient light sensor from System Status (Section 1)** | Dedicated strictly to Section 4 (Digital Inputs & Sensors) for clean dashboard hierarchy. |
| **2026-09-27 21:35** | **P3 - Visual** | **Design / Typography** | **Modern typography & styling (Google Fonts Outfit + JetBrains Mono)** | Glassmorphism design with deep midnight blue tones, translucent borders, and crisp legibility. |
| **2026-09-27 21:10** | **P1 - Critical** | **OTA / Deploy** | **Flash firmware v3.0 completed successfully over network (OTA)** | Firmware v3.0 operational with modular architecture on aquarium controller. |
| **2026-09-27 21:05** | **P1 - Critical** | **Documentation** | **Full documentation overhaul (`README.md`, `docs/user_guide.md`, `docs/ui_design.md`)** | Documentation aligned with v3.0 architecture and workflows. |
| **2026-09-27 21:00** | **P1 - Critical** | **Network / UX** | **Multi-protocol Windows discovery (NetBIOS, LLMNR, SSDP / UPnP)** | Enables direct Windows access (`http://acvariu/` and Windows Explorer auto-discovery). |
| **2026-09-27 20:55** | **P1 - Critical** | **Backend / UI** | **Dynamic relay and sensor renaming (Custom I/O Names)** | LittleFS persistence in `/names.cfg`. User can customize hardware labels at any time. |
| **2026-09-27 20:50** | **P1 - Critical** | **Backend / UI** | **Custom named presets with web interface saving** | Save active schedule under custom name to `/user_presets.cfg`, instant 1-click activation. |
| **2026-09-27 20:45** | **P2 - Medium** | **UI / UX** | **Separate Minute & Second fields for Pulse Mode** | Intuitive pulse configuration replacing raw seconds input. |
| **2026-09-27 20:40** | **P2 - Medium** | **Feature** | **Smart Feeding Mode (10-minute automated pause)** | Temporarily halts aeration/pumping with on-screen countdown and auto-resumption. |
| **2026-09-27 20:35** | **P2 - Medium** | **UI / Layout** | **Modular web dashboard restructuring** | Logical hierarchy: System Status -> Peripheral Controls -> Schedules -> I/O -> Admin -> Logs. |
| **2026-09-27 20:30** | **P2 - Medium** | **Logging / UX** | **Single-line event log & explicit operation confirmations** | Streamlined log `[DD.MM HH:MM:SS]` with clear visual confirmation toast on every user action. |
| **2026-09-27 20:25** | **P3 - Optimization** | **Performance** | **Eliminate CDN dependency & introduce live LDR sensor progress bar** | 100% offline local network capability with instant asset loading. |
| **2026-05-30 18:00** | **P1 - Critical** | **Backend** | **Power Loss Failsafe Mode (UPS Failsafe on GPIO0)** | Auto-shedding of high loads and pulsed air pump to preserve battery runtime. |
| **2026-05-30 17:30** | **P1 - Critical** | **Backend** | **NTP time synchronization with Romania timezone (EET/EEST & DST)** | Accurate scheduling without external hardware RTC module. |
| **2026-05-30 16:00** | **P2 - Medium** | **Storage** | **Schedule persistence on LittleFS (`/schedules.cfg`)** | Non-volatile flash persistence across reboots. |

---

## 💡 Backlog & Future Tasks (TODO)

Planned enhancements prioritized by system safety, reliability, and user impact.

### Priority P1 (High Impact / Aquarium Safety)

* [ ] **Submersible Digital Temperature Sensor (DS18B20) on GPIO4:**
  * *Description:* Water temperature is the most critical vital parameter for fish and plants. GPIO4 is currently exposed on header with internal pull-up capability. Connecting a waterproof DS18B20 probe will provide continuous live temperature telemetry.
  * *Safety Alarms:* Visual alarm and notification if water drops below 23°C (failed heater) or exceeds 28°C (summer overheating).
  * *Estimated Effort:* Low (`OneWire` and `DallasTemperature` libraries).

---

### Priority P2 (Advanced Features & UX Refinements)

* [ ] **Low Water Level Float Switch (GPIO2 or GPIO15):**
  * *Description:* Detect water evaporation in BioBox pump chamber.
  * *Action:* Visual alert if water level is dangerously low, preventing dry-run pump damage.
* [ ] **Automated CO2 Siesta Ramp (Planted Tank Preset):**
  * *Description:* Preset optimized for algae inhibition (BBA control) with 1.5h mid-day CO2 siesta.
* [ ] **Configurable Feeding Mode Duration:**
  * *Description:* Select duration (5 min, 10 min, 15 min) directly before triggering feeding mode.

---

### Priority P3 (Integrations & Visual Themes)

* [ ] **MQTT / Home Assistant Integration (Auto-Discovery):**
  * *Description:* Publish sensor states and relay controls over MQTT for Home Assistant dashboard integration.
* [ ] **Accent Color Theme Selector (Cyan / Emerald / Amethyst / Amber):**
  * *Description:* Allow user to choose UI accent color palette according to personal preference.
* [ ] **Downloadable Configuration Backup & Restore (JSON):**
  * *Description:* Export schedules, custom names, and presets to `aquarium_backup.json` with restore capability.

---

## 📌 Development Conventions

1. When a new idea is proposed, add it to the **Backlog (TODO)** table with its designated priority (`P1`, `P2`, `P3`).
2. When a feature is implemented and verified, move it to the **Done** table with the exact completion timestamp `[YYYY-MM-DD HH:MM]`.
3. Major milestones and breaking changes are documented in `CHANGELOG.md` upon each release.
