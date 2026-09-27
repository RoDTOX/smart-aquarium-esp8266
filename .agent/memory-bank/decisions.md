# Architectural Decisions Log

## 2026-09-27 — Actuators vs. Telemetry Domain Separation

**Context**: Considered whether to merge Section 2 (Peripheral Controls) and Section 4 (Digital Inputs & Auxiliary Sensors) into a single dashboard card.

**Decision**: Kept Section 2 and Section 4 strictly separate.

**Consequences**:
- Preserves clean mobile ergonomics: users can toggle high-voltage loads without scrolling past telemetry inputs.
- Preserves scalability for upcoming modular sensors (DS18B20 temperature probe on GPIO4, water float switch on GPIO2/15) without cluttering the actuator cards.

**Alternatives considered**:
- Merging all GPIOs into one unified list — rejected because it produces an unwieldy hybrid card mixing 220V/12V switched loads with 3.3V logic signals.

**Related**: [docs/ui_design.md](file:///c:/Users/BSeceleanu/.github_repos/smart-aquarium-esp8266/docs/ui_design.md), [src/main.cpp](file:///c:/Users/BSeceleanu/.github_repos/smart-aquarium-esp8266/src/main.cpp).

---

## 2026-09-27 — Variant C Dual-Action Badges for Peripheral Controls

**Context**: The initial relay card layout occupied excessive vertical screen space on mobile phones due to a third row dedicated to a manual toggle button and switch.

**Decision**: Adopted Variant C layout: moved actions to the header row using dual compact action badges (`[ AUTO ]` and `[ ⏻ ]`) with dynamic glowing status colors and instant 1-tap override.

**Consequences**:
- Reduces relay card vertical height by ~40% on mobile screens.
- Provides immediate visual feedback (glowing emerald for forced ON, glowing red for forced OFF, emerald for active AUTO).

**Related**: [CHANGELOG.md](file:///c:/Users/BSeceleanu/.github_repos/smart-aquarium-esp8266/CHANGELOG.md), [src/main.cpp](file:///c:/Users/BSeceleanu/.github_repos/smart-aquarium-esp8266/src/main.cpp).
