# Active Context

## Current Focus
Unified Multi-Theme Engine Integration (v3.6.0) completed, verified, and flashed to ESP8266 (`192.168.1.32`).

## Recent Changes (v3.6.0)
- Built unified multi-theme single-page dashboard in `web/index.html` hosting all three layouts:
  1. **Acvariu Viu:** Dynamic animated SVG aquarium tank (swimming fish, swaying plants, bubbles, food flakes, daylight/moonlight effects) with equipment cards.
  2. **Consolă:** Cockpit technical instrument panel with top status strip, 24h radial dial instrument, 3-position rockers, and 24h telemetry sparkline.
  3. **Clar:** Modern Scandinavian smart-home aesthetics with 2x2 device tiles, 24-column schedule heatmap matrix, and filtered activity log.
- Live Layout & Mode Switchers in header:
  - Layout switch (`🐠 Viu`, `📟 Consolă`, `🏡 Clar`) stored in `localStorage['aquarium_ui_style']`.
  - Theme mode cycle (`Auto`, `Light`, `Dark`) stored in `localStorage['aquarium_theme']` syncing with Relay 1 schedule.
  - Remote link pill to Galaxy-A6 Smart Services Hub.
- Unified reactive `Aq` adapter connected to live ESP8266 REST API with batch schedule drafting, roving tabindex keyboard accessibility, modal dialog suite (Names, WiFi, Presets Save/Rename/Delete), and zero-injection escaping.
- Compacted build: Gzip payload is 28,930 bytes (~28.2 KB, budget <= 40 KB). Flash: 41.7%, RAM: 49.3%.
- Flashed via OTA to `192.168.1.32` and verified live over local IP and Galaxy-A6 Nginx proxy.

## Recent Changes (v3.6.1 - Schedule & Dial Synchronization Fixes)
- Fixed schedule draft desynchronization bug in `web/index.html`: `fetchSchedules(forceDraftSync)` now computes `currentlyDirty` before updating `state.saved`, preventing `state.draft` from staying locked at all zeros on initial load and after preset application.
- Fixed preset loading workflow: `api.loadPreset()` now calls `fetchSchedules(true)` to force draft synchronization directly from the device upon applying a preset.
- Fixed radial dial SVG patterns: Defined static `<defs>` inside `<svg id="dial">` in the HTML markup, avoiding browser detach bugs when dynamically rewriting SVG innerHTML. Set explicit `fill` attributes on all segments.
- Fixed duplicate status description text in Consolă channel list (`renderChannels`), displaying clean, concise status without repeated "On · On" or "Off · Off".
- Added initial `fetchHistory()` invocation on page load to populate the activity timeline immediately.
- Recompiled and OTA flashed firmware to ESP8266 (`192.168.1.32`). Gzip payload: 29,032 bytes ($\le 40\text{ KB}$). Verified live.

## Recent Changes (v3.6.2 - Pulse LED Blip, 60m Limit, Hierarchy Reorder, Clasic Theme, Lush Tank)
- **LED Blip Indicator for Intermittent Relays:** Implemented rhythmic blip indicator across ALL 4 themes (Consolă, Clar, Viu, Clasic) using `getLedClass(st)` and CSS `@keyframes led-pulse-on` (1s green + 2 grey blips) and `led-pulse-off` (1s grey + 2 green blips).
- **Intermittent Cycle Limit (ON + OFF <= 60 min):** Enforced in UI (`api.setPulse` with toast alert) and in backend (`src/main.cpp` `handleScheduleSet` batch and single endpoints).
- **Universal Layout Hierarchy (Schedule Editor above Controls):**
  - Consolă: `Channel Program` placed ABOVE `Channels & Rockers`.
  - Clar: `Daily Schedule Heatmap` placed ABOVE `Device Tiles & Sensors`.
  - Viu: `Daily Schedule Ribbon Timeline` placed ABOVE `Equipment Controls`.
  - Clasic: `24h Schedule & Real-Time Status` placed ABOVE `Relays Manual Controls`.
- **Title Redundancy Removed:** Removed duplicate `AQUATLANTIS BIOBOX 56L` brand box from Consolă status strip (`con-strip`), leaving the header bar as single title and cleanly balancing the 6 telemetry metric cells.
- **Restored Classic Dashboard (`🏛️ Clasic`):** Integrated 4th layout option in header switcher, restoring retro-modern glass cards with 24-slot horizontal sliders, slot toggling, pulse duration inputs, and preset bar.
- **Acvariu Viu Scene Enhancements:** Authentic fish swimming head-first (Mickey Mouse Platy, Male Guppy wavy fan-tail, Siamese Algae Eater, Guppy Fry school, Red Cherry Shrimp), textured porous airstone with dense sparkling bubble column, glass CO2 bubble counter with discrete rising bubbles, ceramic diffuser with micro-mist, and lush multi-species plants (Vallisneria, Ludwigia repens, Anubias, Frogbit).
## Recent Changes (v3.6.3 - Bubble Inversion, Instant Draft Bar Dismissal, Sequential Boot)
- **Airstone & CO2 Bubble Realignment in Acvariu Viu:**
  - Moved `#airstone-unit` and `.air-col` (large, dense, turbulent bubbles $r=4\dots13\text{px}$) to the **RIGHT** side ($x=845\dots885$, $y=290$) at the gravel bed.
  - Placed `#bubble-counter` and `#co2-diffuser` on the **LEFT** side ($x=26$ on glass wall and $x=130$ on substrate), producing discrete indicator ticks inside the counter chamber and delicate, sparse micro-mist ($r=1.1\dots1.8\text{px}$) rising gently into the water column.
- **Instant Draft Bar Dismissal & Double-Click Guard:**
  - Updated `commitSchedules()`: the moment the server responds with HTTP 200 OK, `state.saved = clone(state.draft); emit();` is executed immediately, setting `isDirty() = false` and hiding the draft bar without waiting for a secondary background schedule re-fetch.
  - Added `isApplyingSchedule` flag and disabled state with "Applying..." label on the apply button to prevent duplicate or trailing requests.
- **Sequential Boot & Auto-Heal Initial Schedule Synchronization:**
  - Resolved the empty timeline / active preset bug on initial load caused by lwIP socket drops when 4 concurrent requests hit single-threaded ESP8266WebServer.
  - Replaced parallel startup calls with sequential `async function boot()`: `await fetchStatus()`, `await fetchSchedules(true)`, `await fetchPresets()`, `await fetchHistory()`.
  - Added a 3-attempt exponential retry loop in `fetchSchedules(forceDraftSync, retries=3)`.
  - Added auto-heal check in 2-second background poll `fetchStatus()`: automatically syncs schedules if `!schedulesLoaded`.
## Recent Changes (v3.6.4 - Bubble Coordinate Origin Fix, Full Now-Line, Timeline Label Pulse Blip, 24h Labels, Compact Program Editor)
- **Air Bubbles Rising Strictly from Airstone (Fixed Crenguță / SVG Scale Offset):**
  - Identified root cause of bubbles appearing at the driftwood / "crenguță": SVG CSS `scale(0.6)` without element-relative transform-box scaled coordinates relative to SVG canvas origin `(0, 0)`, shifting $x=870$ to $x=522$.
  - Fixed by adding `transform-box: fill-box; transform-origin: center;` and keeping translation strictly vertical with slight horizontal wobble in `@keyframes airBubbleRise`.
  - Every bubble now emerges directly out of the bottom right airstone (`x=845..885`, `y=290`) and rises straight up.
- **Vertical Time Indicator (Now-Line) Extended Across ALL 4 Relays:**
  - Extended the vertical time indicator in both **Viu** and **Clar** layouts across all 4 relay channels (Lumina, CO2, AER, N/A) without stopping after the first lane.
  - In Viu, connected through Sky band down to lane 4; in Clar, connects across all 4 slot rows.
- **LED Blip Indicator on Timeline Row Labels:**
  - Added `.lane-label i.pulse-on`, `.lane-label i.pulse-off`, `.clar-row-label i.pulse-on`, `.clar-row-label i.pulse-off` to the 1.8s rhythmic pulse CSS animations.
  - Timeline labels now pulse rhythmically (green running blip or grey resting blip) directly beside the channel names on both Viu and Clar timelines.
- **Labels on ALL 24 Hours Across All Themes:**
  - **Viu (Sky):** Displays all 24 hours `00`..`23`.
  - **Clar (Matrix):** Displays all 24 hours `00`..`23`.
  - **Consolă (Radial Dial):** Displays all 24 hours around the circumference.
  - **Clasic (Track headers):** Displays all 24 hours aligned 1-to-1 with the 24 slots.
- **Channel Program Editor Refinements:**
  - In Consolă and Clar, pulse run/rest textboxes are completely hidden when continuous mode is active.
  - Preset dropdown width constrained (`max-width: 180px`), allowing dropdown, "Save as...", Rename, and Delete buttons to fit compactly on a single row.
- **Build & Flash:** Binary compiled cleanly (`pio run` SUCCESS, gzip: 33,974 B $\le 40\text{ KB}$, Flash: 42.2%, RAM: 49.3%). Flashed via OTA to `192.168.1.32` and verified live.

## Immediate Next Steps (Backlog)
1. P1: Add submersible waterproof digital temperature probe (DS18B20) on GPIO4.
2. P1: Solder flyback diode (1N4007) across 12V CO2 solenoid coil to suppress inductive spikes.
3. P2: Add water evaporation float switch on GPIO2 or GPIO15.
