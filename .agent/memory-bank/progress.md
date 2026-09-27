# Progress Tracking

## Overall Status
Production operational (`v3.2.0`). System mounted on Aquatlantis BioBox 56L aquarium, verified over WiFi OTA.

## What Works
- Dynamic custom relay and sensor renaming with LittleFS persistence (`/names.cfg`).
- Custom user-named presets with LittleFS persistence (`/user_presets.cfg`).
- Active preset persistence (`/active_preset.cfg`) and modified schedule detection.
- Windows multi-protocol discovery (NetBIOS, LLMNR, SSDP / UPnP) and mobile mDNS.
- Smart Feeding Mode (10-minute automated aeration pause with countdown).
- Variant C two-row compact peripheral controls with 1-tap override and live countdown timers.
- Single-line compact event log (`DD.MM HH:MM:SS`) with automatic flash protection rotation.
- UPS power loss failsafe mode on GPIO 0.
- Wireless OTA flashing portal at `/update` with event logging hooks.
- 100% offline self-hosted dashboard.

## Known Issues
None. All reported UI glitches, mobile dropdown flickering, and variable reference errors have been resolved.
