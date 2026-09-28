# User & Configuration Guide: Aquatlantis Smart Aquarium Controller (v3.2)

This guide details the operation, configuration, and administration of the ESP8266-based aquarium controller (LC-Relay-ESP12-4R-MV 4-relay board).

---

## 1. Local Network Connection & Discovery (IP & Hostnames)

The ESP-12F board runs in hybrid **AP + STA** (Access Point + Station) mode. It connects to your home WiFi network and provides multiple automatic discovery protocols:

### Web Dashboard Access Methods

| Device | Recommended URL | Protocol Used | Notes |
|---|---|---|---|
| **Windows 10 / 11** | **`http://acvariu/`** | **NetBIOS / LLMNR** | Works natively in Edge, Chrome, and Firefox without `.local`. |
| **Windows Explorer** | **This PC -> Network** | **SSDP / UPnP** | Discovered under *Other Devices* as *"Aquatlantis Smart Aquarium"*. Double-click opens your browser. |
| **iPhone / iPad (iOS)** | **`http://acvariu.local/`** | **mDNS (Bonjour)** | Works natively in Safari and Chrome. |
| **Android** | **`http://acvariu.local/`** or **Direct IP** | **mDNS / IP** | On some Android browsers mDNS resolution can be restricted; use direct local IP if needed. |
| **Any Device** | **`http://192.168.1.32/`** | **Direct HTTP (IP)** | Replace with the actual IP assigned by your router. |

### Why `acvariu.local` Previously Failed on Windows
1. **Public vs. Private Network Profile:** When Windows WiFi connection is set to "Public Network", the Windows firewall blocks UDP port 5353 (mDNS). Switching the connection to **Private Network** in Windows Settings unblocks mDNS.
2. **Multi-Protocol Solution:** Firmware includes native responders for **NetBIOS (`http://acvariu/`)**, **LLMNR**, and **SSDP (UPnP)**. On Windows, you no longer need `.local` — navigate directly to `http://acvariu/` or open it from Windows File Explorer's Network section.

### Finding the Device IP if Changed
1. **Windows Explorer:** Open *File Explorer* -> click *Network* on the left -> under *Other Devices*, find *Aquatlantis Smart Aquarium*.
2. **Ping Command (PowerShell / Command Prompt):**
   ```powershell
   ping acvariu
   # or
   ping acvariu.local
   ```
   The response reveals the current IP (e.g. `Reply from 192.168.1.32`).
3. **Static DHCP Lease (Recommended):** In your router admin page (e.g. `192.168.1.1`), under *DHCP Static Leases / Reserved IP*, map the ESP8266 MAC address to a fixed IP (such as `192.168.1.32`).

### Fallback Access Point (AP) Mode
If your router is offline or WiFi credentials have changed:
1. The board broadcasts its own fallback WiFi network: **`BioBox-Aquarium`**.
2. Connect with your phone using the password defined in `secrets.h` (`AP_PASS`, default: `12345678`).
3. Open your browser and navigate to: **`http://192.168.4.1/`**.
4. Go to **5. System Administration** -> **WiFi Setup**, enter your new SSID and password, then click **Connect Device**.

---

## 2. Custom Relay & Sensor Names (Custom I/O Names)

Hardware names are fully customizable and persist across reboots:
1. In the dashboard, navigate to **5. System Administration**.
2. Click **"✏️ Rename Relays & Sensors"**.
3. In the modal dialog, configure custom names:
   * **Relays 1 - 4:** e.g., *"WRGB Plant Light"*, *"JBL CO2 Solenoid"*, *"Eheim Air Pump"*, *"Blue Night Light"*.
   * **Digital Inputs (GPIO 0, 4, 2, 15):** e.g., *"Mains Power Sensor"*, *"Low Water Float Switch"*.
   * **Analog Sensor (A0):** e.g., *"Room LDR Light Sensor"*.
4. Click **"Save Names"**. Configuration is stored on LittleFS (`/names.cfg`) and survives reboots and firmware updates.

---

## 3. Schedule Presets & 24h Operation Timeline

In addition to the 3 built-in factory presets, you can configure, inspect, and save custom schedules:

### 24-Hour Visual Infographic Timeline (Section 1)
* Directly under **System Status**, an interactive horizontal timeline maps out 00:00 – 24:00 operations across all 4 channels:
  * **Continuous Channels:** Solid, vibrant gradient bars indicate active run hours.
  * **Intermittent / Pulse Channels:** Prominent diagonal hatched stripes (`repeating-linear-gradient`) indicate pulsed ON/OFF cycling.
  * **Real-Time "NOW" Indicator:** A vertical red tracking cursor with a live numeric pin displays current local time.
  * **Direct Interaction:** Clicking any relay label switches the active schedule tab; clicking any hourly segment toggles that hour in the editor.

### Saving a New Preset
1. Configure active hours and pulse parameters across all 4 relays in **3. Schedule Configuration (24h)**.
2. In the *Schedule Presets* section, click **"Save Preset"**.
3. Enter a descriptive name (e.g., *"Summer Schedule"*, *"Algae Treatment"*).
4. The preset is persisted to LittleFS (`/user_presets.cfg`) and appears in the dropdown.

### Overwriting an Existing Preset
* **1-Click Toolbar Overwrite:** When an existing custom preset (e.g. *"DOI"*) is selected from the dropdown, a dedicated **`[ 🔄 Overwrite "DOI" ]`** button appears in the toolbar. Clicking it automatically performs a silent commit of your current unsaved modifications and overwrites the preset in-place on LittleFS.
* **Modal Smart Overwrite:** If you click **"Save Preset"** and input or keep an existing preset name, the modal displays an overwrite advisory (*"⚠️ Preset exists. Clicking will overwrite..."*) and changes the action button to **`[ 🔄 Overwrite "<Name>" ]`**.

### Applying and Deleting Presets
* **Apply:** Select the preset from the dropdown and click **"Apply"**. All 4 relay schedules update simultaneously.
* **Delete:** When a custom preset is selected, the **"Delete"** button appears. (Factory presets 1–3 are write-protected).
* **Reset to Defaults:** Restores all relay schedules to recommended factory settings for Aquatlantis BioBox.

---

## 4. Smart Feeding Mode

During feeding or water maintenance, aeration and strong currents can disperse flake food or draw it into surface skimmers.
* In **2. Peripheral Status & Control**, click **"🫧 Feed Mode (10m)"**.
* **Action:** Immediately pauses Relay 3 (Air Pump / Filtration) for 10 minutes.
* **Visual Alert:** A countdown banner appears at the top (*"Feed Mode Active • 09:45 remaining"*).
* **Auto-Resume:** When the timer expires, the pump automatically resumes its scheduled profile.
* You can cancel Feeding Mode at any time by clicking **"Cancel"** in the banner.

---

## 5. Intermittent Mode (Pulse Mode) with Minutes & Seconds

To pulse a peripheral (e.g. aeration during power outages or fine CO2 dosing):
1. Select the target relay in the schedule selector.
2. Under *Mode during active hours*, select **"Pulse / Intermittent (repeating ON / OFF cycles)"**.
3. Configure the duration pairs:
   * **Pulse ON:** `[ Min ] m [ Sec ] s` (active run time)
   * **Pause OFF:** `[ Min ] m [ Sec ] s` (idle rest time)
4. Click **"Save Relay Schedule"**.

---

## 6. Single-Line System Event Log

* Compact single-line timeline: `[DD.MM HH:MM:SS] Message`.
* Timestamps omit year for maximum legibility on mobile screens.
* All user actions (schedule save, manual override, preset applied, renaming) generate explicit confirmation logs and UI toasts.
* `/log.txt` automatically rotates at 2.5 KB to protect flash memory endurance.

---

## 7. Real-Time Light Sensor Telemetry

* Replaced external CDN dependencies with a self-hosted, animated progress meter (0% - 100%).
* 100% offline capability — dashboard loads instantly on local LAN without internet access.
* **Smart Lamp Diagnostic:** If Relay 1 (Main Light) is ON for >30 seconds but the sensor reads under 15%, the system logs an automatic diagnostic warning to inspect the fixture's power supply.

---

## 8. Over-The-Air Wireless Firmware Updates (OTA)

Update firmware wirelessly through any browser:
1. In PlatformIO, compile the firmware (`pio run`). Binary is located at `.pio/build/esp12e/firmware.bin`.
2. Open **`http://acvariu/update`** (or `http://acvariu.local/update`).
3. Authenticate with credentials defined in `Config.h` (default: `admin` / `admin123`).
4. Select `firmware.bin` and click **Flash Firmware**. The board writes the image and reboots automatically in ~12 seconds.
