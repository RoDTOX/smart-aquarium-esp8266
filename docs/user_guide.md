# User & Configuration Guide: Aquatlantis Smart Aquarium Controller (v3.4)

This guide details the operation, configuration, and administration of the ESP8266-based aquarium controller (LC-Relay-ESP12-4R-MV 4-relay board).

---

## 1. Local Network Connection & Discovery (IP & Hostnames)

The ESP-12F board runs in hybrid **AP + STA** (Access Point + Station) mode. It connects to your home WiFi network and provides multiple automatic discovery protocols:

### Web Dashboard Access Methods

| Device / Interface | Recommended URL | Protocol Used | Notes |
|---|---|---|---|
| **Nexus Central Gateway** | **`http://nexus/`** | **Nginx Port 80** | Central Home Portal with live cards for Aquarium, TeslaMate, and Grafana. |
| **Direct Aquarium Link** | **`http://nexus/aquarium`** | **Reverse Proxy (Port 80)** | Direct access to the aquarium controller without remembering port numbers or IP. |
| **Windows 10 / 11** | **`http://acvariu/`** | **NetBIOS / LLMNR** | Works natively in Edge, Chrome, and Firefox without `.local`. |
| **Windows Explorer** | **This PC -> Network** | **SSDP / UPnP** | Discovered under *Other Devices* as *"Aquatlantis Smart Aquarium"*. Double-click opens your browser. |
| **iPhone / iPad (iOS)** | **`http://acvariu.local/`** | **mDNS (Bonjour)** | Works natively in Safari and Chrome. |
| **Android** | **`http://acvariu.local/`** or **Direct IP** | **mDNS / IP** | On some Android browsers mDNS resolution can be restricted; use direct local IP if needed. |
| **Any Device** | **`http://192.168.1.32/`** | **Direct HTTP (IP)** | Replace with the actual IP assigned by your router. |
| **Tailscale Remote** | **`http://100.83.135.74:8080/`** | **Tailscale Bridge** | Direct secure remote tunnel via Samsung home node. |


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
2. Click **"✏️ Customize I/O Names"**.
3. In the modal dialog, configure custom names (emojis supported):
   * **Relays 1 - 4:** e.g., *"💡 WRGB Light"*, *"🫧 CO2 Solenoid"*, *"🌊 Air Pump"*, *"🌙 Night Light"*.
   * **Digital Inputs (GPIO 0, 4, 2, 15):** e.g., *"Mains Power Sensor"*, *"Low Water Float Switch"*.
   * **Analog Sensor (A0):** e.g., *"Room LDR Light Sensor"*.
4. Click **"Save Names"**. Configuration is stored on LittleFS (`/names.cfg`) and survives reboots and firmware updates.

---

## 3. Interactive 24h Timeline Scheduler & Presets (Chapter 1)

The 24h schedule configuration, live physical telemetry, and presets are unified into **Chapter 1: 1. 24h Schedule & Real-Time Status**:

### Mobile Touch-Scrollable 24-Hour Timeline Scheduler
* **Touch-Friendly Ergonomics:** An interactive horizontal timeline with smooth horizontal swipe (`min-width: 820px`) providing comfortable ~28x28px touch targets for every hour on phones.
* **Sticky Channel Names & Live Dynamic State Dots:** Equipment labels remain permanently visible on the left while swiping through hours. Each track row features an 8px circular indicator:
  * 🟢 **Solid Green (Conducting):** The relay is physically conducting / energized.
  * ⚡ **Green Blip (100ms pulse every 1.8s):** When inside an active scheduled hour but resting in pulse pause ("Standby: off now, but active schedule running").
  * ⚫ **Slate Gray (Inactive):** The relay is idle / off outside scheduled hours.
* **Unconfigured Channel Disconnected / OFF View:**
  * When a channel has no hours configured (e.g. Relay 4), the track row is styled muted gray (`.track-empty`), displays an `[OFF]` tag, and shows a centered `⚪ All 24h OFF • Click any hour to schedule` notice.
* **1-Tap Quick Fill Controls:** Under the timeline, click **`[ ⚡ All 24h ]`** to activate all 24 hours or **`[ ⚪ Clear All ]`** to clear the selected channel in a single tap.
* **Auto-Scroll to Current Hour:** On page load, the view automatically centers around the current local NTP hour.
* **Tap-to-Toggle with Draft:** Tapping any hour slot toggles that hour ON or OFF in a local draft. An amber bar then shows *"Unsaved schedule changes"* with:
  * **`✓ Apply to device`**: sends all 4 channels to the ESP8266 in one request (one flash write, one `Schedule saved` log entry).
  * **`↺ Discard`**: reverts the draft to the schedule currently running on the device.
  * Leaving the page or loading another preset while a draft is pending asks for confirmation first.
* **Keyboard:** Hour slots can be focused with Tab; the arrow keys move between hours and channels, and Enter/Space toggles the hour.
* **Direct Track Row Selection:** Tapping any track row selects that channel for configuring active mode and pulse durations below (no separate channel tab buttons).
* **Visual Coding:** Continuous active hours are rendered with solid vibrant gradient bars; Intermittent/Pulse hours feature high-contrast diagonal stripes (`repeating-linear-gradient`).
* **Real-Time "NOW" Indicator:** A vertical red tracking line with a live numeric pin displays current local time.

### Unified Preset Toolbar & Rename Workflow
* **1-Click Preset Loading:** Selecting any preset from the dropdown applies it immediately across all 4 channels (a confirmation is shown only if you have unsaved draft changes).
* **Single "💾 Save Schedule" Button:**
  * When modifications are made, the button shows **`💾 Save Schedule *`** with a subtle amber glow indicating unsaved preset changes.
  * If working on an active custom preset, clicking **"💾 Save Schedule"** asks to overwrite preset '<Name>'. Confirming applies the draft to the device and overwrites the preset in-place on LittleFS.
  * If starting from a factory preset or unnamed schedule, prompts to name and save as a new custom preset.
* **"✏️ Rename" Button:** Appears whenever a custom preset is active, opening a dialog to rename the preset in-place on LittleFS.
* **"➕ New Preset" Button:** Opens a modal dialog allowing you to name and save the current timeline settings as a brand new preset at any time.
* **"Delete" Button:** Appears whenever a custom preset is active, allowing quick deletion. (Factory presets 1–3 are write-protected).

---

## 4. Manual Overrides & Maintenance (Chapter 2)

Dedicated strictly to manual control, emergency actions, and equipment maintenance:
* **Running Status Badges with Media Icons:**
  * **`▶️ On • Until HH:MM`** / **`▶️ Running • Pause in ...`**: Clean running triangle indicator for active conducting states.
  * **`⏸️ In Pause • Resumes in ...`**: Pause indicator when resting in an active intermittent cycle.
  * **`⚪ Off • Starts at HH:MM`** / **`⚪ Off • No schedule active`**: Inactive channel indicators.
* **Smart Feeding Mode:** Click **"🫧 Feed Mode (10m)"** to pause Relay 3 (Air Pump / Filtration) for 10 minutes. A countdown banner appears at the top (*"Feed Mode Active • 09:45 remaining"*). Click **"Cancel"** at any time to resume schedule.
* **Auto (All):** Click to instantly clear all manual overrides and return all 4 relays to their automated 24h timeline schedule.
* **Interactive Relay Badges:**
  * **AUTO:** Green when following schedule; gray when manually overridden.
  * **⏻ ON / OFF:** Toggle between forced ON, forced OFF, or back to automatic schedule.

---

## 5. Digital Inputs & Sensors (Chapter 3)

* Monitors digital inputs on GPIO 0, 4, 2, and 15 (e.g. mains power detection, float switches).
* **Analog Light Sensor (A0):** Self-hosted, animated progress meter (0% - 100%) tracking ambient room lighting.

---

## 6. Event History & System Administration (Chapters 4 & 5)

* **Chapter 4: Event History:** Rolling 20-event log history (`[DD.MM HH:MM:SS] Message`) with instant **Refresh** button. `/log.txt` rotates automatically at 2.5 KB to protect flash memory endurance.
* **Chapter 5: System Administration:** Quick access to **Customize I/O Names**, **WiFi Setup**, and **Firmware Update (OTA)**.

---

## 7. Over-The-Air Wireless Firmware Updates (OTA)

Update firmware wirelessly through any browser:
1. In PlatformIO, compile the firmware (`pio run`). Binary is located at `.pio/build/esp12e/firmware.bin`.
2. Open **`http://acvariu/update`** (or `http://acvariu.local/update` / `http://nexus/aquarium/update`).
3. Authenticate with credentials defined in `secrets.h` (default: `admin` / `21051990`).
4. Select `firmware.bin` and click **Flash Firmware**. The board writes the image and reboots automatically in ~12 seconds.

---

## 8. Deep Abyss Dark Mode & Real Biotope Animation

* **Theme Switcher:** Tapping the header button cycles through `☀️ Light`, `🌙 Dark`, and `🌓 Auto`.
  * In `Auto` mode, the dashboard automatically syncs with the aquarium's physical Day/Night cycle (lighting active vs. off).
* **Real Planted Tank Biotope Animation:** Modeled directly after the real 56L Aquatlantis planted setup:
  * Volcanic gravel substrate bed and hanging duckweed/Salvinia roots.
  * Swaying stem plants (Hygrophila, Bacopa, Staurogyne).
  * Multi-stream dynamic rising bubbles (micro, small, medium, and 3D glass globes with realistic sinusoidal wobble).
  * Inhabitants with strict head-first swimming direction: Mickey Mouse platy, male guppy with undulating fan tail, fry school, Siamese Algae Eaters with horizontal black stripe, red cherry shrimp, and zebra snail.
* **Aviation Double-Blip Status Dots:** In pulse mode, relays blink with a high-visibility double-blip sequence (green/white running, white/green pause) on a 1.2-second cycle.
* **Outage Duration Tracking:** Automatically measures and logs duration when AC grid power is cut or restored (`[PWR] Power Outage Detected` in bold red, `[PWR] AC Grid Restored after X outage` in green) and when WiFi reconnects (`[WIFI] Connection Restored after Y outage` in green).

---

## 9. Nexus Central Gateway & Home Portal

* **Unified Port 80 Access:** Runs on the Samsung home server node (`192.168.1.28` / Tailscale `100.83.135.74`) via Nginx and Magisk `iptables` NAT redirection (`80 -> 8088`).
* **Home Portal (`http://nexus/` or `http://192.168.1.28/`):** Glassmorphic landing page displaying real-time Online/Offline health for:
  * 🐠 **Aquarium Controller** &rarr; `http://nexus/aquarium` (or `http://192.168.1.28/aquarium`)
  * 🚗 **TeslaMate** &rarr; `http://nexus/teslamate` (or `http://192.168.1.28/teslamate`)
  * 📊 **Grafana** &rarr; `http://nexus/grafana` (or `http://192.168.1.28/grafana`)
* **Access via Tailscale (Remote):**
  * Current Tailscale device name: **`galaxy-a6`** (`galaxy-a6.tail481af9.ts.net`).
  * Direct URLs: **`http://galaxy-a6/aquarium`** or **`http://100.83.135.74/aquarium`**.
  * To use `http://nexus/` on Tailscale, rename the device `galaxy-a6` to `nexus` in the Tailscale Admin Console.
* **Access via Local Wi-Fi (Laptop):**
  * Direct URL: **`http://192.168.1.28/aquarium`** (works immediately without configuration).
  * For `http://nexus/`: Add `192.168.1.28 nexus` to Windows `hosts` file (`C:\Windows\System32\drivers\etc\hosts`) or router DNS.

