#include <LittleFS.h>
#include <vector>
#include "RelayControl.h"
#include "Config.h"

// Forward declaration of NetworkSync helpers to avoid circular includes
extern String getFormattedTime();
extern int getCurrentHour();

// Volatile states of the relays (physical output, override flags)
static RelayState relays[4] = {
    {false, false, false},
    {false, false, false},
    {false, false, false},
    {false, false, false}
};

// Persistent profiles loaded/saved in EEPROM
static RelayProfile profiles[4];

// Pulse control state variables for all 4 relays
static bool relayPulseState[4] = {false, false, false, false};
static unsigned long relayLastToggle[4] = {0, 0, 0, 0};
static bool wasHourActive[4] = {false, false, false, false};
static SystemMode lastExecutedMode = MODE_DAY;

// Helper to write physically to GPIO
static void writePhysicalRelay(int relayNum, bool active) {
    int pin = -1;
    switch (relayNum) {
        case 1: pin = PIN_RELAY_1; break;
        case 2: pin = PIN_RELAY_2; break;
        case 3: pin = PIN_RELAY_3; break;
        case 4: pin = PIN_RELAY_4; break;
        default: return;
    }
    
    // Check if the state changed to prevent writing to GPIO repeatedly
    if (relays[relayNum - 1].physicalState != active) {
        relays[relayNum - 1].physicalState = active;
        digitalWrite(pin, active ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
    }
}

// Rotates the log file when it grows beyond 2.5KB (keeps only the last 15 entries)
// to prevent write cycles amplification and avoid infinite rewrite loop bugs.
static void limitLogSize() {
    if (!LittleFS.exists("/log.txt")) return;
    
    File f = LittleFS.open("/log.txt", "r");
    if (!f) return;
    
    size_t size = f.size();
    f.close();
    
    // If the log is small (under ~35 lines), do not rotate it
    if (size < 2500) {
        return;
    }
    
    // Reopen for reading all entries
    f = LittleFS.open("/log.txt", "r");
    if (!f) return;
    
    std::vector<String> lines;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            lines.push_back(line);
        }
    }
    f.close();
    
    // Rewrite keeping only the last 15 logs
    File out = LittleFS.open("/log.txt", "w");
    if (out) {
        int start = (lines.size() > 15) ? (lines.size() - 15) : 0;
        out.println("[" + getFormattedTime() + "] Log rotit automat pentru protejare Flash.");
        for (size_t i = start; i < lines.size(); i++) {
            // Only keep normal lines to prevent any infinite loops on corrupt entries
            if (lines[i].length() < 120) {
                out.println(lines[i]);
            }
        }
        out.close();
        Serial.println("[LittleFS Log] Rotated successfully to reduce flash write cycles.");
    }
}

void logSystemEvent(const String& msg) {
    String t = getFormattedTime();
    String line = "[" + t + "] " + msg;
    
    Serial.printf("[SYSTEM LOG] %s\n", line.c_str());
    
    // Open log file in append mode
    File f = LittleFS.open("/log.txt", "a");
    if (f) {
        f.println(line);
        f.close();
    } else {
        Serial.println("[ERROR] Failed to open /log.txt for writing.");
    }
    
    // Perform log size checks
    limitLogSize();
}

void loadSettingsFromEEPROM() {
    if (LittleFS.exists("/schedules.cfg")) {
        File f = LittleFS.open("/schedules.cfg", "r");
        if (f) {
            uint32_t magic = 0;
            f.read((uint8_t*)&magic, sizeof(magic));
            if (magic == EEPROM_MAGIC_NUMBER) {
                f.read((uint8_t*)profiles, sizeof(profiles));
                Serial.println("[LittleFS Config] Loaded configurations successfully.");
                f.close();
                return;
            }
            f.close();
        }
    }
    
    Serial.println("[LittleFS Config] Config file not found or invalid. Initializing defaults...");
    profiles[0] = { DEFAULT_SCHED_RELAY_1, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[1] = { DEFAULT_SCHED_RELAY_2, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[2] = { DEFAULT_SCHED_RELAY_3, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[3] = { DEFAULT_SCHED_RELAY_4, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    
    saveSettingsToEEPROM();
}

void saveSettingsToEEPROM() {
    File f = LittleFS.open("/schedules.cfg", "w");
    if (f) {
        uint32_t magic = EEPROM_MAGIC_NUMBER;
        f.write((uint8_t*)&magic, sizeof(magic));
        f.write((uint8_t*)profiles, sizeof(profiles));
        f.close();
        Serial.println("[LittleFS Config] Saved successfully.");
    } else {
        Serial.println("[LittleFS Config] Error: Failed to open /schedules.cfg for writing.");
    }
}

void applyPreset(int presetNum) {
    if (presetNum == 1) {
        profiles[0] = { DEFAULT_SCHED_RELAY_1, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[1] = { DEFAULT_SCHED_RELAY_2, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[2] = { DEFAULT_SCHED_RELAY_3, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[3] = { DEFAULT_SCHED_RELAY_4, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        saveSettingsToEEPROM();
        logSystemEvent("Preset 1 (Standard Aquatlantis) aplicat.");
    } else if (presetNum == 2) {
        // Algae Control setup:
        // Light: 14:00 - 20:00 (6 hours -> 0x7E000)
        // CO2: 12:00 - 18:00 with Siesta 15:00 - 16:00 (5 hours -> 0x37000)
        // Air Pump: 20:00 - 12:00 (Hours 20-23 and 0-11 -> 0xF00FFF) Continuous at night
        // Ambient Light: 20:00 - 23:00 (Hours 20-22 -> 0x700000)
        profiles[0] = { 0x7E000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[1] = { 0x37000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[2] = { 0xF00FFF,  BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, 180 };
        profiles[3] = { 0x700000,  BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        saveSettingsToEEPROM();
        logSystemEvent("Preset 2 (Control Alge) aplicat.");
    } else if (presetNum == 3) {
        // Maintenance setup:
        // Light & CO2: OFF permanent (0x0)
        // Air Pump: ON permanent (24h continuous -> 0xFFFFFF)
        // Ambient Light: OFF permanent (0x0)
        profiles[0] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[1] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[2] = { 0xFFFFFF,  BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[3] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        saveSettingsToEEPROM();
        logSystemEvent("Preset 3 (Mentenanță/Fără Lumini) aplicat.");
    }
}

void resetSettingsToDefault() {
    profiles[0] = { DEFAULT_SCHED_RELAY_1, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[1] = { DEFAULT_SCHED_RELAY_2, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[2] = { DEFAULT_SCHED_RELAY_3, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[3] = { DEFAULT_SCHED_RELAY_4, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    saveSettingsToEEPROM();
    logSystemEvent("Setările au fost resetate la valorile implicite.");
}

void initRelays() {
    pinMode(PIN_RELAY_1, OUTPUT);
    pinMode(PIN_RELAY_2, OUTPUT);
    pinMode(PIN_RELAY_3, OUTPUT);
    pinMode(PIN_RELAY_4, OUTPUT);
    
    // Force off initially
    digitalWrite(PIN_RELAY_1, !RELAY_ACTIVE_LEVEL);
    digitalWrite(PIN_RELAY_2, !RELAY_ACTIVE_LEVEL);
    digitalWrite(PIN_RELAY_3, !RELAY_ACTIVE_LEVEL);
    digitalWrite(PIN_RELAY_4, !RELAY_ACTIVE_LEVEL);
    
    // Mount LittleFS flash filesystem
    if (LittleFS.begin()) {
        Serial.println("[LittleFS] Mounted successfully.");
    } else {
        Serial.println("[LittleFS] Error mounting filesystem.");
    }
    
    loadSettingsFromEEPROM();
    logSystemEvent("Sistem pornit. Relee inițializate.");
}

RelayState getRelayState(int relayNum) {
    if (relayNum < 1 || relayNum > 4) {
        return {false, false, false};
    }
    return relays[relayNum - 1];
}

RelayProfile getRelayProfile(int relayNum) {
    if (relayNum < 1 || relayNum > 4) {
        return {0, 0, 0, 0};
    }
    return profiles[relayNum - 1];
}

void setRelayManualOverride(int relayNum, bool overrideActive, bool targetState) {
    if (relayNum < 1 || relayNum > 4) return;
    
    bool oldOverride = relays[relayNum - 1].manualOverride;
    bool oldState = relays[relayNum - 1].manualState;
    
    relays[relayNum - 1].manualOverride = overrideActive;
    relays[relayNum - 1].manualState = targetState;
    
    if (overrideActive) {
        if (!oldOverride || oldState != targetState) {
            logSystemEvent("Releul " + String(relayNum) + ": Forțat MANUAL pe " + String(targetState ? "PORNIT" : "OPRIT"));
        }
    } else {
        if (oldOverride) {
            logSystemEvent("Releul " + String(relayNum) + ": Redat controlului automat (Orar)");
        }
    }
}

void clearManualOverrides() {
    for (int i = 0; i < 4; i++) {
        if (relays[i].manualOverride) {
            relays[i].manualOverride = false;
            logSystemEvent("Releul " + String(i + 1) + ": Redat controlului automat (Orar)");
        }
    }
}

void updateRelayProfile(int relayNum, uint32_t activeHours, uint8_t behavior, uint32_t pulseOnSec, uint32_t pulseOffSec) {
    if (relayNum < 1 || relayNum > 4) return;
    
    profiles[relayNum - 1].activeHours = activeHours;
    profiles[relayNum - 1].behavior = behavior;
    profiles[relayNum - 1].pulseOnSec = pulseOnSec;
    profiles[relayNum - 1].pulseOffSec = pulseOffSec;
    
    saveSettingsToEEPROM();
    logSystemEvent("Releul " + String(relayNum) + ": Program actualizat în memorie.");
}

String getHistoryJSON() {
    if (!LittleFS.exists("/log.txt")) {
        return "[]";
    }
    
    File f = LittleFS.open("/log.txt", "r");
    if (!f) return "[]";
    
    String r = "[";
    bool first = true;
    
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        
        int closeBracket = line.indexOf(']');
        String timestamp = "N/A";
        String msg = line;
        
        if (line.startsWith("[") && closeBracket > 0) {
            timestamp = line.substring(1, closeBracket);
            msg = line.substring(closeBracket + 1);
            msg.trim();
        }
        
        if (!first) r += ",";
        first = false;
        
        // Escape quotes to prevent invalid JSON
        msg.replace("\"", "\\\"");
        
        r += "{";
        r += "\"time\":\"" + timestamp + "\",";
        r += "\"msg\":\"" + msg + "\"";
        r += "}";
    }
    f.close();
    r += "]";
    return r;
}

String getSchedulesJSON() {
    String r = "[";
    for (int i = 0; i < 4; i++) {
        r += "{";
        r += "\"num\":" + String(i + 1) + ",";
        r += "\"active_hours\":" + String(profiles[i].activeHours) + ",";
        r += "\"behavior\":" + String(profiles[i].behavior) + ",";
        r += "\"pulse_on\":" + String(profiles[i].pulseOnSec) + ",";
        r += "\"pulse_off\":" + String(profiles[i].pulseOffSec);
        r += "}";
        if (i < 3) r += ",";
    }
    r += "]";
    return r;
}

String getModeString(SystemMode mode) {
    switch (mode) {
        case MODE_DAY: return "MOD NORMAL - ZI";
        case MODE_NIGHT: return "MOD NORMAL - NOAPTE";
        case MODE_POWER_LOSS: return "MOD AVARIE - PANĂ CURENT";
        default: return "UNKNOWN";
    }
}

void updateRelays(SystemMode currentMode) {
    unsigned long now = millis();
    
    // 1. Calculate target automatic states
    bool targets[4] = {false, false, false, false};
    
    if (currentMode == MODE_POWER_LOSS) {
        // Power Loss Failsafe: Turn OFF lights and CO2 to save battery.
        // Run Air Pump (Relay 3) on battery-saving Pulse Mode (1m ON / 2m OFF)
        targets[0] = false; // Light OFF
        targets[1] = false; // CO2 OFF
        targets[3] = false; // Ambient Light OFF
        
        // Handle Air Pump pulsing
        if (lastExecutedMode != MODE_POWER_LOSS) {
            // Force start ON when entering power loss mode
            relayPulseState[2] = true;
            relayLastToggle[2] = now;
        }
        uint32_t interval = relayPulseState[2] ? 60000 : 120000;
        if (now - relayLastToggle[2] >= interval) {
            relayPulseState[2] = !relayPulseState[2];
            relayLastToggle[2] = now;
        }
        targets[2] = relayPulseState[2];
    } else {
        // Normal scheduling evaluation
        int currentHour = getCurrentHour();
        // Fallback to Hour 12 (Daytime) if NTP has not synchronized yet
        int hr = (currentHour >= 0 && currentHour <= 23) ? currentHour : 12;
        
        for (int i = 0; i < 4; i++) {
            // Check if the current hour is active in the scheduler bitmap
            bool isHourActive = (profiles[i].activeHours & (1UL << hr)) != 0;
            
            if (isHourActive) {
                if (profiles[i].behavior == BEHAVIOR_CONTINUOUS) {
                    targets[i] = true;
                } else {
                    // Pulse Mode behavior
                    // Restart cycle ON if transitioning from inactive to active
                    if (!wasHourActive[i]) {
                        relayPulseState[i] = true;
                        relayLastToggle[i] = now;
                    }
                    
                    uint32_t interval = relayPulseState[i] ? (profiles[i].pulseOnSec * 1000) : (profiles[i].pulseOffSec * 1000);
                    if (now - relayLastToggle[i] >= interval) {
                        relayPulseState[i] = !relayPulseState[i];
                        relayLastToggle[i] = now;
                    }
                    targets[i] = relayPulseState[i];
                }
            } else {
                targets[i] = false;
            }
            
            wasHourActive[i] = isHourActive;
        }
    }
    
    // 2. Drive physical pins (apply manual overrides if active, unless in power loss mode)
    for (int i = 0; i < 4; i++) {
        if (currentMode == MODE_POWER_LOSS) {
            // In power loss mode, force pins OFF, except the calculated targets for the air pump
            if (i == 2) {
                writePhysicalRelay(3, targets[2]);
            } else {
                writePhysicalRelay(i + 1, false);
            }
        } else {
            if (relays[i].manualOverride) {
                writePhysicalRelay(i + 1, relays[i].manualState);
            } else {
                writePhysicalRelay(i + 1, targets[i]);
            }
        }
    }
    
    // Log system-wide mode transitions to Serial only to protect flash wear.
    // We update this at the end so that transitions can be detected during the calculations above.
    if (currentMode != lastExecutedMode) {
        Serial.printf("[SYSTEM] Mod operare sistem schimbat în: %s\n", getModeString(currentMode).c_str());
        lastExecutedMode = currentMode;
    }
}
