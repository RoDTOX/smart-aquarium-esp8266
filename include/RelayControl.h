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

// Represents customizable names for all I/O pins
struct CustomIONames {
    char relays[4][32];        // Relay 1..4 custom friendly names
    char digitalInputs[4][32]; // GPIO0, GPIO4, GPIO2, GPIO15 friendly names
    char analogInput[32];      // A0 (Light Sensor) friendly name
};

// Represents a user-saved schedule preset
#define MAX_USER_PRESETS 8
struct UserPreset {
    bool active;
    char name[32];
    RelayProfile profiles[4];
};

// Initialize the relay output pins and load LittleFS settings
void initRelays();

// Fetch status of a relay (1-indexed: 1 to 4)
RelayState getRelayState(int relayNum);
String getRelayStatusDescription(int relayNum);

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

// Custom I/O Names API
CustomIONames getCustomIONames();
void setCustomIONames(const CustomIONames& names);
String getRelayCustomName(int relayNum);
String getIONamesJSON();

// User Presets API
int saveUserPreset(const String& name);
bool overwriteUserPreset(int id);
bool applyUserPreset(int id);
bool deleteUserPreset(int id);
String getPresetsJSON();
int getActivePresetId();
void setActivePresetId(int id);

// Smart Feeding Mode API (Temporarily suspends aeration/filtration so food settles)
void setFeedMode(bool active, uint32_t durationSec = 600);
bool isFeedModeActive();
uint32_t getFeedModeRemainingSec();

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
