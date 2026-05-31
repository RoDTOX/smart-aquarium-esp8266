#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>

// Represents the system's operational states
enum SystemMode {
    MODE_DAY,
    MODE_NIGHT,
    MODE_POWER_LOSS
};

// Represents the state of a single relay
struct RelayState {
    bool physicalState;    // Actual pin output state (true = ON/Active, false = OFF/Inactive)
    bool manualOverride;   // Whether this relay is currently controlled manually
    bool manualState;      // The target manual state if overridden
};

enum RelayBehavior {
    BEHAVIOR_CONTINUOUS = 0,
    BEHAVIOR_PULSE = 1
};

// Represents the schedule and behavior profile of a single relay
struct RelayProfile {
    uint32_t activeHours;   // 24-bit bitmap where bit 0 = hour 0, bit 23 = hour 23
    uint8_t behavior;       // RelayBehavior (0 = Continuous, 1 = Pulse)
    uint32_t pulseOnSec;    // Pulse ON duration in seconds
    uint32_t pulseOffSec;   // Pulse OFF duration in seconds
};


// Initialize the relay output pins and load EEPROM settings
void initRelays();

// Fetch status of a relay (1-indexed: 1 to 4)
RelayState getRelayState(int relayNum);

// Set manual override for a specific relay
void setRelayManualOverride(int relayNum, bool overrideActive, bool targetState = false);

// Reset all relays back to automatic scheduler control
void clearManualOverrides();

// Execute relay control state machine logic (evaluates 24h bitmap and handles pulsing)
void updateRelays(SystemMode currentMode);

// Settings management (LittleFS config file persistence)
void loadSettingsFromEEPROM();
void saveSettingsToEEPROM();
void applyPreset(int presetNum);
void resetSettingsToDefault();

// Get and update individual relay profile settings
RelayProfile getRelayProfile(int relayNum);
void updateRelayProfile(int relayNum, uint32_t activeHours, uint8_t behavior, uint32_t pulseOnSec, uint32_t pulseOffSec);

// Event Logging API
void logSystemEvent(const String& msg);
String getHistoryJSON();
String getSchedulesJSON();

// Telemetry API
int getLightLevelPercent();
void addTelemetryReading(int percent, const String& timeStr);
String getTelemetryJSON();

// Helper to convert SystemMode to string
String getModeString(SystemMode mode);

#endif // RELAY_CONTROL_H
