# Active Context

## Current Focus
Idle — task 'v3.2.0 Modernization & English Localization' completed 2026-09-27.

## Recent Changes (v3.2.0)
- Implemented Variant C compact two-row relay cards with dual interactive action badges (`[ AUTO ]` and `[ ⏻ ]`).
- Added LittleFS persistence for `activePresetId` (`/active_preset.cfg`), displaying modified status when schedule hours are customized.
- Tasteful mode badges: `☀️ Day (Normal)`, `🌙 Night (Normal)`, and `⚡ Power Loss (UPS)`.
- Fixed frontend JS variable initialization for `activePresetId` and null checks.
- 100% English translation across firmware C++ code, UI dashboard, OTA updater, system event logs, and documentation.

## Immediate Next Steps (Backlog)
1. P1: Add submersible waterproof digital temperature probe (DS18B20) on GPIO4.
2. P1: Solder flyback diode (1N4007) across 12V CO2 solenoid coil to suppress inductive spikes.
3. P2: Add water evaporation float switch on GPIO2 or GPIO15.
