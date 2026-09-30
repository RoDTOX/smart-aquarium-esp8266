# Changelog - Smart Aquarium Controller (ESP8266)

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [3.6.5] - 2026-09-30

### 1-Minute WiFi Outage Confirmation Debounce & Transient Log Pruning
* **1-Minute WiFi Outage Confirmation Window:**
  * Prevented LittleFS flash log spam caused by transient 2-second WiFi signal drops on ESP8266.
  * Disconnections under 60 seconds are considered signal glitches and suppressed from LittleFS `/log.txt`.
  * Drops exceeding 60 seconds are confirmed as real outages, logging `[WIFI] Connection Lost (offline > 1 min)`.
* **Proactive Reconnection & Sleep Mode Fix:**
  * Added `WiFi.setSleepMode(WIFI_NONE_SLEEP)` to eliminate periodic ~5-minute WPA2 GTK rekey disconnects caused by default modem RF sleep.
  * Enabled `WiFi.setAutoReconnect(true)` in network initialization.
  * While confirmed offline (> 60s), the controller actively invokes `WiFi.reconnect()` every 30s to prevent stack stall.
  * On reconnection after a confirmed outage, logs total duration: `[WIFI] Connection Restored after X outage`.
* **On-Demand SoftAP & Dedicated Pure Station Mode:**
  * Eliminated 24/7 background broadcasting of SoftAP `BioBox-Aquarium`. ESP8266 now boots into pure `WIFI_STA` mode, dedicating 100% of the single 2.4GHz radio to the home router.
  * **Automatic Boot Fallback:** If STA connection fails to establish within 30s of boot, SoftAP starts automatically at 192.168.4.1 for recovery.
  * **Web Dashboard Control:** Added `[ Enable AP ]` button in Wi-Fi modal with password authorization (`POST /api/wifi action=enable_ap`), auto-stopping after 10 minutes.
  * **Hardware Long-Press Reset:** Holding GPIO4 LOW for $\ge 5$ seconds forces emergency SoftAP activation.
  * SoftAP shuts down automatically once connected to STA.
* **Historical Transient Log Pruning & Clear API:**
  * Added `cleanTransientWifiLogs()` on boot to automatically purge historical 2s/3s WiFi log spam from LittleFS `/log.txt`.
  * Added `POST /api/history` with `clear=1` support for manual log clearing.

## [3.6.4] - 2026-09-29

### Airstone Bubble Origin, Full Now-Line, Timeline Label Pulse Blips, 24h Labels & Compact Program Editor
* **Airstone Bubble Origin Fixed:**
  * Fixed SVG CSS transform scaling offset (`transform-box: fill-box; transform-origin: center;`) that previously made air bubbles emerge from the driftwood. Bubbles now rise strictly out of the bottom-right airstone unit (`x=845..885`, `y=290`).
* **Vertical Now-Line Across All 4 Channels:**
  * Extended real-time vertical now-line across all 4 relay channels in both Viu and Clar layouts.
* **Timeline Label Pulse Blips:**
  * Integrated rhythmic pulse animations (`.pulse-on`, `.pulse-off`) on the indicator dots next to channel names in both Viu and Clar timelines.
* **Full 24-Hour Labels Across All Themes:**
  * Added individual hour labels `00`..`23` to all themes: Viu (Sky ribbon), Clar (Heatmap header), Consolă (Radial dial circumference), and Clasic (Slot track headers).
* **Compact Channel Program Editor:**
  * In Consolă and Clar, pulse run/rest textboxes are hidden when Continuous mode is active.
  * Constrained preset dropdown width, allowing select dropdown, "Save as...", Rename, and Delete buttons to fit compactly on a single row.

## [3.6.3] - 2026-09-29

### Airstone & CO2 Realignment, Instant Draft Bar Dismissal & Sequential Startup Sync
* **Airstone & CO2 Biotope Realignment:**
  * Placed airstone with dense aeration bubbles on the right side at substrate; placed glass CO2 bubble counter and ceramic diffuser micro-mist on the left side.
* **Instant Draft Bar Dismissal:**
  * Instant status bar dismissal upon HTTP 200 OK without delay; added request guard against double-clicks.
* **Sequential Boot & Auto-Heal Schedule Sync:**
  * Replaced parallel startup calls with sequential `boot()` sequence and exponential retry loop to prevent socket drops on ESP8266.

## [3.6.2] - 2026-09-29

### Pulse LED Indicators, 60m Limit, Hierarchy Alignment, Clasic Layout & Tank Overhaul
* **Rhythmic Pulse LED Indicators:**
  * Added distinct blinking indicators across ALL 4 themes (Consolă, Clar, Viu, Clasic) via unified `getLedClass(st)`.
  * **Pulse ON:** Solid green for 1s followed by 2 crisp grey blips.
  * **Pulse OFF (Rest / Failsafe pause):** Solid grey for 1s followed by 2 crisp green blips.
* **Intermittent Cycle Limit (ON + OFF <= 60 min):**
  * Enforced `pulse_on + pulse_off <= 3600` seconds in UI with toast feedback and automatic value adjustment.
  * Backend clamping in `src/main.cpp` `handleScheduleSet` for both batch and legacy endpoints.
* **Universal Layout Hierarchy:**
  * Reordered sections across all layouts so Schedule / Program Editor is placed above Equipment Controls:
    * **Consolă:** `Channel Program` placed above `Channels & Rockers`.
    * **Clar:** `Daily Schedule Heatmap` placed above `Device Tiles & Sensors`.
    * **Viu:** `Daily Schedule Ribbon Timeline` placed above `Equipment Controls`.
    * **Clasic:** `24h Schedule & Real-Time Status` placed above `Relays Manual Controls`.
* **Title Redundancy Removed:**
  * Cleaned up duplicate project title from Consolă's status strip (`con-strip`), leaving the main application header as the single source of truth and distributing the 6 telemetry metric cells evenly.
* **Restored Classic Dashboard Layout (`🏛️ Clasic`):**
  * Added 4th theme option in header switcher: `[🐠 Viu | 📟 Consolă | 🏡 Clar | 🏛️ Clasic]`.
  * Restored retro-modern glass cards with 24-slot horizontal sliders, slot toggling, pulse duration inputs, and preset bar, wired into the unified reactive `Aq` state engine.
* **Acvariu Viu Scene Enhancements:**
  * Replaced generic fish with authentic tank fauna swimming strictly head-first in both directions: Mickey Mouse Platy (orange with 3 tail dots), Male Guppy (wavy rainbow fan-tail), Siamese Algae Eater (SAE with black lateral line), Guppy Fry school, and Red Cherry Shrimp.
  * Textured porous airstone with airline tubing producing a rich, dense aeration bubble column when Air Pump is active.
  * Clear glass CO2 bubble counter with discrete rising bubbles when CO2 is active, plus ceramic diffuser with micro-mist.
  * Lush multi-layered biotope plants: tall emerald Vallisneria ribbons, copper-tipped Ludwigia repens, Anubias on driftwood, and floating Frogbit roots.

## [3.6.0] - 2026-09-29

### Unified Multi-Theme Engine: Acvariu Viu, Consolă, and Clar with Dynamic Live Switching
* **Integrated 3 distinct layouts into a single dashboard:**
  * **Acvariu Viu (Live Biotope Simulation):** Animated SVG aquarium tank with realistic daylight rays, moonlight glow, swimming and wiggling fish, swaying aquatic plants, airstone bubble column, CO2 micro-diffuser fizz, sinking food flakes during feeding, and glowing frosted glass equipment cards.
  * **Consolă (Technical Cockpit Instrument):** Dense instrument panel with top telemetry strip, 24h SVG radial dial with 4 concentric channels, 3-position rockers (AUTO / ON / OFF), 24h light sensor telemetry sparkline, and LittleFS event log.
  * **Clar (Modern Smart Home):** Clean Scandinavian smart-home aesthetics with 2x2 device tiles, 24-column schedule heatmap matrix with now needle, activity log with category filters, and system health status.
* **Live Style & Mode Switchers in App Header:**
  * One-click switching between `🐠 Viu`, `📟 Consolă`, and `🏡 Clar` saved persistently in `localStorage['aquarium_ui_style']`.
  * Cycling theme mode (`Auto` / `Light` / `Dark`) with real-time day/night sync following Relay 1 schedule.
  * Direct access pill link to Galaxy-A6 Smart Services Hub.
* **Single Real-Device `Aq` Adapter:**
  * Unified all 3 layouts behind a single reactive API adapter polling `/api/status`, `/api/schedule`, `/api/presets`, and `/api/history`.
  * Batch schedule drafting with bottom status bar (`Discard` and `Apply to device`), roving tabindex for accessible keyboard navigation (arrow keys + Space/Enter).
  * Full modal dialog suite: Custom Peripheral Names, WiFi setup, Preset management (Create, Rename, Delete with confirmation), and OTA update trigger.
  * Zero-injection escaping on all device strings (`Lamp "big" <b>x</b>` and custom names).
* **Efficiency & Size Budget:**
  * Compacted unified dashboard gzipped payload to 28,930 bytes (~28.2 KB), well within the 40 KB flash budget. Flash utilization: 41.7%, RAM: 49.3%.

## [3.5.0] - 2026-09-29

### UI/UX Review Fixes: Schedule Drafts, Dark Mode Contrast, Robust JSON & Gzipped Dashboard
* **Schedule editing is now a local draft:**
  * Tapping hour blocks or changing mode/pulse timing no longer writes to flash on every tap.
  * A draft bar shows unsaved changes with **Discard** (real undo) and **Apply to device**.
  * All 4 channels are committed in one request (`POST /api/schedule` batch form): one flash write and one `Schedule saved` log entry, so outage events are no longer pushed out of the 15-entry log.
  * Switching presets with unsaved changes asks for confirmation; leaving the page warns too.
* **Dark mode contrast:** relay cards, GPIO tags and status banners were hard-coded light colours (white text on light grey). They now have proper dark variants.
* **Colour semantics:** manual override has its own violet banner (✋) instead of red for "Forced ON"; "Restored" events are green instead of red; the schedule legend uses neutral swatches because each channel has its own colour.
* **Robustness & security:**
  * Added `jsonEscape()` for names, presets, history, SSID and status text. A `"` in a name no longer breaks the JSON and freezes the dashboard.
  * The dashboard renders all device-provided text with `textContent` / HTML escaping (no HTML injection through names).
  * Custom names are always NUL-terminated.
* **Performance:**
  * Dashboard moved to `web/index.html` and served gzipped (~136 KB -> ~27 KB) via `tools/build_web.py`; firmware flash usage dropped ~108 KB.
  * Removed the Google Fonts dependency so the page works offline and in AP mode.
  * Status polling never overlaps, pauses in background tabs, and updates relay cards in place instead of rebuilding them every 2 s.
  * Event history is refetched only when `history_rev` changes.
  * The device shows "Device unreachable" after 2 failed polls.
* **Accessibility & polish:**
  * Keyboard support: hour slots (arrow keys + Enter/Space), channel labels, the Nexus tile, Escape and backdrop click close dialogs, and visible focus rings.
  * `prefers-reduced-motion` disables the decorative animations; toasts are announced via `aria-live`.
  * Larger hour slots on touch screens; mobile header/legend wrap cleanly; all 7 overview tiles fit on one desktop row.
  * Styled confirm dialog replaces native `confirm()`; toast timer no longer cuts consecutive messages short.
  * Power sensor shows `Mains OK` / `Power lost`; aux inputs show a neutral dot for LOW; uptime shows days.
  * `Auto` theme now follows the main light schedule (dark while the light is scheduled off).
  * Consistent default channel names; Nexus addresses grouped in one `NEXUS` config object (probe every 30 s).

## [3.4.1] - 2026-09-29

### Visual & Aesthetic Overhaul: Anatomical Swimming Physics, Deep Abyss Dark Mode & Multi-Stream Bubbles
* **Fish Swimming Physics & Anatomical Orientation:**
  * Fixed horizontal scale inversion across all CSS animation keyframes (`scaleX(-1)` swimming right, `scaleX(1)` swimming left).
  * Every inhabitant (Mickey Mouse platy, male guppy with undulating fan tail, fry school, Siamese Algae Eaters, red cherry shrimp) swims strictly head-first in both directions.
* **Purged Diver Figurine:**
  * Completely removed the decorative diver figurine markup and animations from the UI.
* **Deep Abyss Dark Mode Polish (No More Washed-Out Grays):**
  * Eliminated all chalky/milky light gray backgrounds in Dark Mode (`.stat-block`, `header`, `.timeline-relay-label`, `.timeline-card`, `.slot-off`, `.sys-badge`, inputs, and selects).
  * Applied sleek translucent deep sapphire/slate glass with luminous cyan borders (`rgba(56, 189, 248, 0.25)`), vivid labels, and crisp typography.
* **Dynamic Multi-Stream Bubble Aquarium Engine:**
  * Expanded bubbles layer to 26 multi-sized rising bubbles across 3 streams (airstone diffuser on left, filter outflow on right, ambient tank center).
  * 4 size categories: micro fizz (3-5px), small (7-9px), medium (12-16px), and 3D glass globes (20-26px with specular sheen).
  * Implemented natural sinusoidal hydrodynamic wobble keyframes (`riseWobble1`, `riseWobble2`, `riseFastMicro`) with staggered negative delays for continuous aquatic life.
* **Nexus Gateway Reverse Proxy & Live Monitoring Polish:**
  * Added reverse proxy routing in Nginx for TeslaMate static assets (`/assets/`, `/images/`) and real-time LiveView WebSockets (`/live/websocket` with HTTP 101 upgrade).
  * Resolved Grafana watchdog false restart loop by updating health check for subpath URL routing (`/grafana/api/health`) and persistent `tmux` session supervision.
  * Verified live status indicators across all 3 services (Aquarium, TeslaMate, Grafana) on the centralized Smart Services Portal (`http://galaxy-a6/` / `http://nexus/`).


## [3.4.0] - 2026-09-29

### Deep Abyss Dark Mode, Real Tank Biotope, Outage Duration Tracking & Nexus Central Gateway
* **Deep Abyss Theme Engine & Day/Night Auto-Sync:**
  * Implemented an ultra-modern Dark Mode palette (`--bg-base: #060e18`, sapphire cards `#0b192c`, glowing cyan borders `#38bdf8`, neon emerald indicators).
  * Added a header toggle button cycling `☀️ Light` &rarr; `🌙 Dark` &rarr; `🌓 Auto` with persistence in `localStorage`.
  * `Auto` mode dynamically synchronizes the visual theme with the aquarium's physical Day/Night cycle.
* **Real Planted Tank Biotope Animation:**
  * Modeled strictly on real tank photos (56L Aquatlantis planted aquarium):
    * Dark volcanic aquasoil gravel bed with layered stones.
    * Floating surface plants with hanging roots (Phyllanthus / duckweed / Salvinia).
    * Swaying lush green stem plants (Hygrophila, Bacopa, Staurogyne).
    * Live inhabitants: Mickey Mouse platy, wavy fan-tail male guppy, school of 4 fry, 2 Siamese Algae Eaters (SAE) with black lateral stripe, grazing red cherry shrimp, and zebra snail (Neritina).
* **Aviation Beacon Double-Blip Status Indicators:**
  * Replaced subtle blips with a crisp 1.2-second aviation-style beacon cadence:
    * **Solid Gray (`#94a3b8`):** Channel inactive / off hours.
    * **Solid Green (`#10b981`):** Continuous ON conducting state.
    * **Intermittent Running:** Base green (720ms) + 2 rapid white blips (180ms blip 1, 120ms gap, 180ms blip 2).
    * **Intermittent Pause:** Base white (720ms) + 2 rapid green blips (180ms blip 1, 120ms gap, 180ms blip 2).
* **Streamlined Chapter 1 Schedule Editor UX:**
  * **Channel Deselection:** Tapping an active channel track deselects it (`selectedRelayNum = 0`), returning to a clean overview and showing a gentle helper prompt.
  * **Compact mm:ss Pulse Inputs:** Replaced bulky separate second inputs with inline formatted `mm:ss` controls (`⏱️ ON: [ 05:00 ]` and `⏸️ OFF: [ 25:00 ]`).
  * **Schedule Undo / Revert (`[ ↺ Restore ]`):** Dynamically appears when changes are pending, allowing 1-click reverting of unsaved modifications without reloading.
  * **Mobile-Optimized Icon Toolbar:** Condensed preset management into sleek icon buttons: `💾` Save, `↺` Restore, `✏️` Rename, `➕` New, and `🗑️` Delete.
  * **Modal Confirmation for Preset Deletion:** Added a dedicated modal dialog preventing accidental preset deletion.
  * **Safety Cleanups:** Removed risky `[ ⚡ All 24h ]` and `[ ⚪ Clear All ]` buttons that corrupted schedules.
* **Streamlined Chapter 2 Manual Controls:**
  * Renamed to **`2. Manual Control`** with a compact segmented toolbar featuring `[ 🍽️ Feed Mode ]` and `[ ⚡ Auto All ]`.
* **Outage Duration & Fault Type Tracking:**
  * Firmware accurately calculates outage elapsed duration for both WiFi disconnects and AC power outages.
  * Dedicated event logging: `[PWR] AC Grid Restored after X outage` (styled bold red in UI) and `[WIFI] Connection Restored after Y outage` (styled amber in UI).
* **Nexus Gateway & Unified Home Portal:**
  * Transformed Samsung home server into **`nexus`** with an Nginx reverse proxy listening on port 80 (via Magisk root `iptables` redirection).
  * Direct port-free URLs: `http://nexus/aquarium`, `http://nexus/teslamate`, `http://nexus/grafana`.
  * Built the **Nexus Home Portal** on `http://nexus/` featuring 3 glassmorphic cards with live telemetry and online/offline status chips.
* **Firmware Branding:**
  * Added footer: `🐠 Aquatlantis Smart Aquarium Controller • Designed & Implemented by Bogdan S. • Firmware v3.4.0`.

---



### Tailscale Remote Status Indicator & Cross-Platform TeslaMate Ecosystem Link
* **Aquarium Dashboard Tailscale Status Card:**
  * Added a dedicated **`TAILSCALE REMOTE`** diagnostic card to the top System Overview.
  * Displays a live dynamic LED indicator and status text:
    * 🟢 **`Connected (Remote)`**: When viewing the dashboard directly over Tailscale (`100.83.135.74:8080`).
    * 🟢 **`100.83.135.74:8080 Ready`**: When viewing locally, confirming that Cinderella's proxy bridge is alive and reachable.
    * ⚪ **`Local Only`**: When Cinderella's proxy bridge is offline or unreachable.
  * Tapping the card opens the direct Tailscale remote dashboard link in a new browser tab.
* **CORS Headers (`Access-Control-Allow-Origin: *`):**
  * Added CORS headers to `/api/status`, enabling external dashboard widgets, TeslaMate, and cross-origin clients to consume telemetry without browser security blocks.
* **TeslaMate (Cinderella) Ecosystem Navbar Shortcut:**
  * Integrated a permanent **`🐠 Acvariu`** entry into the TeslaMate top navigation bar on server Cinderella.
  * Features an 8px live LED heartbeat checking port 8080 every 10s: turns vibrant green when the aquarium is online and gray if sleeping or offline.
  * 1-click direct launcher dynamically routes through Tailscale (`100.83.135.74:8080`) or local LAN (`192.168.1.28:8080`) based on user connection context.

---

## [3.3.3] - 2026-09-28

### Professional State Visuals, Dynamic Pulse Blip & Preset Renaming
* **Chapter 2 Triangle Running Status Icons:**
  * Replaced the generic green circle with a dynamic triangle running symbol (`▶️`) across all active relay status banners (`▶️ On • Until ...`, `▶️ Running • Pause in ...`).
  * Combined with `⏸️` for pause intervals and `⚪` for inactive/off channels for instant cognitive clarity.
* **Chapter 1 Unconfigured Channel (Relay 4) Disconnected / OFF View:**
  * When a channel has no operating hours scheduled (`active_hours == 0`), the track is styled with `.track-empty` (muted grayed-out slot borders and lowered opacity).
  * The sticky track label displays a crisp `[OFF]` tag and the track grid features a centered `⚪ All 24h OFF • Click any hour to schedule` notice with pass-through click events.
* **Smart Standby Dot Animation (100ms Green Blip in Pulse Pause):**
  * When a relay is within an active pulse schedule hour but currently resting/in pause (`isRelayInPulsePause()`), the channel dot displays a 100ms periodic green radar blip (`blip-green` keyframes every 1.8s) over a neutral gray background.
  * Dot stays solid constant green (`dot-active`) when conducting and solid gray (`dot-idle`) when fully inactive.
* **Preset Renaming Workflow (`[ ✏️ Rename ]`):**
  * Replaced the unused Factory Reset button on the Presets toolbar with an intuitive **`[ ✏️ Rename ]`** button.
  * Opens `#rename-preset-modal` allowing in-place name edits for active custom presets; persists to LittleFS via `POST /api/presets?rename=<id>&name=<new_name>` and logs the rename event to Event History.
* **Schedule Dirty Indicator & Quick-Fill Helpers:**
  * Modifying schedule hours or pulse settings displays `💾 Save Schedule *` with a gentle amber glow, clearly signaling unsaved preset modifications.
  * Added **`[ ⚡ All 24h ]`** and **`[ ⚪ Clear All ]`** 1-tap quick configuration buttons to the Channel Editing header.

---

## [3.3.2] - 2026-09-28

### Streamlined UI: Unified Presets Toolbar, Direct Track Selection & Chapter Restructuring
* **Chapter Restructuring & Priority Focus:**
  * **Chapter 1: 1. 24h Schedule & Real-Time Status:** Promoted schedule timeline and presets to Chapter 1, unifying schedule configuration with live physical state indicators.
  * **Chapter 2: 2. Manual Overrides & Maintenance:** Dedicated strictly to emergency manual overrides, Feed Mode (10m), Auto All, and actuator toggle cards.
  * **Chapter 3: 3. Digital Inputs & Sensors:** Unchanged (GPIO 0/4/2/15 digital inputs and A0 light telemetry).
  * **Chapter 4 & 5 Swap:** Chapter 4 is now **4. Event History** and Chapter 5 is **5. System Administration** (I/O naming, WiFi, OTA).
* **Live Physical Relay State Dots on Timeline:**
  * Added 8px status indicator dots (`.timeline-relay-dot`) directly inside the sticky labels on the 24h timeline.
  * 🟢 **Green (Conducting):** Shows when relay is actively energized / conducting (including active pulse intervals).
  * ⚫ **Slate Gray (Idle):** Shows when relay is resting or inactive.
* **Direct Timeline Track Selection (No Tab Clutter):**
  * Eliminated the 4 separate relay tab buttons (`[ 1. LUMINA ] [ 2. CO2 ] ...`).
  * Clicking any track row on the timeline directly selects that channel, highlights the track with `.timeline-track-row.selected`, and updates the channel mode and pulse configuration below.
* **Unified Presets Toolbar & Single Save Button:**
  * Replaced the confusing multi-button paradigm (*"Save Channel Settings"*, *"Save Preset"*, *"Overwrite Preset"*) with a single, clear **`[ 💾 Save Schedule ]`** button.
  * Clicking **`[ 💾 Save Schedule ]`** always commits changes to the currently loaded preset:
    * If on a custom user preset, prompts to overwrite the preset in-place on LittleFS.
    * If starting from a factory preset or unnamed schedule, prompts to name and save as a new custom preset.
  * Added **`[ ➕ New Preset ]`** button to create and name a new custom preset at any time.
  * Selecting any preset in the dropdown immediately loads and applies it across all 4 channels automatically.
* **Eliminated "Custom Schedule (Modified)" Confusion:**
  * Modifying schedule hours or pulse settings no longer resets the active preset or injects a temporary entry into the dropdown.
  * The controller backend (`updateRelayProfile`) and UI stay locked to the currently active preset, guaranteeing that clicking **`[ 💾 Save Schedule ]`** always directly overwrites the active loaded preset in-place.

---

## [3.3.1] - 2026-09-28

### Mobile Touch Timeline Scheduler, Dashboard Hierarchy Consolidation & SVG Power Icon
* **Mobile Touch-Scrollable 24h Interactive Timeline Scheduler (Option 2):**
  * Transformed the 4-track multi-relay timeline into the primary interactive schedule editor, eliminating the redundant separate 24-button grid.
  * Added smooth horizontal swipe scrolling (`overflow-x: auto; -webkit-overflow-scrolling: touch`) with `min-width: 820px`, providing comfortable 28x28px touch targets on mobile displays.
  * Implemented sticky equipment name labels (`position: sticky; left: 0`) and ruler spacer with backdrop blur, ensuring channel labels stay permanently visible during horizontal scrolling.
  * Direct tap-to-toggle: tapping any hour slot directly toggles that hour ON/OFF with immediate silent auto-commit (`saveActiveRelayScheduleSilently()`) to ESP8266 RAM/storage.
  * Automatic horizontal scroll centering on page load: viewport smoothly centers around the current local NTP hour.
* **Dashboard Chapter Hierarchy Consolidation:**
  * Clean, logical chapter numbering:
    * **Top Overview Card:** Diagnostic overview (System Mode, NTP Clock, Uptime, WiFi, RSSI, IP) without chapter number.
    * **1. Peripheral Status & Control:** Compact actuator cards with 1-tap Auto and Power badges.
    * **2. 24h Schedule & Presets:** Unified 4-channel timeline scheduler + mode/pulse duration controls + Presets toolbar.
    * **3. Digital Inputs & Sensors:** Logic inputs and ambient light telemetry.
    * **4. System Administration:** Feed mode, custom peripheral naming modal, WiFi configuration.
    * **5. Event History:** Timestamped operational log.
* **Cross-Platform SVG Power Button Icon:**
  * Replaced Unicode glyph `⏻` (U+23FB) with an inline crisp SVG vector icon, resolving the `[X]` missing character box on Android/Chrome mobile devices.
* **Clean Peripheral Names (User Choice):**
  * Removed hardcoded emoji prefixes (`🐟`, `🦐`, `🫧`, `🐚`) from relay tabs and labels; user has 100% control over equipment naming in "Customize I/O Names" (with emoji support).
  * Removed maritime helm icon (`☸️`) from peripheral card headers.
* **Aquatic Animation Directional Swimming Fix:**
  * Re-engineered fish animation keyframes with strict directional orientation: `scaleX(-1)` when swimming right, `scaleX(1)` when swimming left, ensuring fish ALWAYS swim head-first.
  * Added boundary wall turns and mid-water direction changes across 6 distinct aquatic creatures (tropical fish, goldfish, blowfish, shrimp, dolphin, sea turtle).

---

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
