# Project Brief: Smart Aquarium Controller (ESP8266)

## Purpose & Scope
Production-grade IoT automation firmware for the Aquatlantis BioBox 56L freshwater planted aquarium. Runs on the LC-Relay-ESP12-4R-MV 4-relay board powered by an ESP8266 (ESP-12F).

## Core Capabilities
- **Peripherals Control:** 4 relays managing Main Lighting (GPIO 16), CO2 Solenoid Valve (GPIO 14), Air Pump (GPIO 12), and Ambient Lighting (GPIO 13).
- **Timekeeping:** NTP synchronization with automatic EET/EEST (Romania) timezone rules and DST transitions.
- **Fail-Safe Operation:** GPIO 0 mains power monitoring switches the air pump to pulsed power-saving mode (1m ON / 2m OFF) and shuts off high-load equipment during AC outages.
- **User Ergonomics:** Dynamic relay/sensor renaming, custom user-defined presets, 10-minute smart feeding pause, multi-protocol local discovery (NetBIOS, LLMNR, SSDP, mDNS), and 100% offline self-contained dashboard.
