#include <LittleFS.h>
#include <vector>
#include "RelayControl.h"
#include "Config.h"

// Forward declaration of NetworkSync helpers to avoid circular includes
extern String getFormattedTime();
extern String getFormattedLogTime();
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
        out.println("[" + getFormattedTime() + "] Log automatically rotated (LittleFS).");
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
    String t = getFormattedLogTime();
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
        setActivePresetId(1);
        saveSettingsToEEPROM();
        logSystemEvent("Preset applied: Standard Aquatlantis");
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
        setActivePresetId(2);
        saveSettingsToEEPROM();
        logSystemEvent("Preset applied: Algae Control");
    } else if (presetNum == 3) {
        // Maintenance setup:
        // Light & CO2: OFF permanent (0x0)
        // Air Pump: ON permanent (24h continuous -> 0xFFFFFF)
        // Ambient Light: OFF permanent (0x0)
        profiles[0] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[1] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[2] = { 0xFFFFFF,  BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        profiles[3] = { 0x00000,   BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
        setActivePresetId(3);
        saveSettingsToEEPROM();
        logSystemEvent("Preset applied: Maintenance / No Lights");
    }
}

// Static Custom I/O Names
static CustomIONames customNames;

void saveCustomIONames(const CustomIONames& names) {
    customNames = names;
    File f = LittleFS.open("/names.cfg", "w");
    if (f) {
        uint32_t magic = 0x4E414D45;
        f.write((uint8_t*)&magic, sizeof(magic));
        f.write((uint8_t*)&customNames, sizeof(customNames));
        f.close();
        Serial.println("[LittleFS] Saved custom I/O names.");
    }
}

void loadCustomIONames() {
    if (LittleFS.exists("/names.cfg")) {
        File f = LittleFS.open("/names.cfg", "r");
        if (f) {
            uint32_t magic = 0;
            f.read((uint8_t*)&magic, sizeof(magic));
            if (magic == 0x4E414D45) {
                f.read((uint8_t*)&customNames, sizeof(customNames));
                f.close();
                Serial.println("[LittleFS] Loaded custom I/O names.");
                return;
            }
            f.close();
        }
    }
    // Default names
    strncpy(customNames.relays[0], "Main Lighting", 31); customNames.relays[0][31] = '\0';
    strncpy(customNames.relays[1], "CO2 Solenoid", 31);   customNames.relays[1][31] = '\0';
    strncpy(customNames.relays[2], "Air Pump", 31);       customNames.relays[2][31] = '\0';
    strncpy(customNames.relays[3], "Ambient Lighting", 31); customNames.relays[3][31] = '\0';
    
    strncpy(customNames.digitalInputs[0], "Power Sensor (GPIO0)", 31); customNames.digitalInputs[0][31] = '\0';
    strncpy(customNames.digitalInputs[1], "GPIO 4 (Aux/Free)", 31);             customNames.digitalInputs[1][31] = '\0';
    strncpy(customNames.digitalInputs[2], "GPIO 2 (Aux/Free)", 31);             customNames.digitalInputs[2][31] = '\0';
    strncpy(customNames.digitalInputs[3], "GPIO 15 (Aux/Free)", 31);            customNames.digitalInputs[3][31] = '\0';
    
    strncpy(customNames.analogInput, "Light Sensor (A0)", 31); customNames.analogInput[31] = '\0';
    
    saveCustomIONames(customNames);
}

CustomIONames getCustomIONames() {
    return customNames;
}

void setCustomIONames(const CustomIONames& names) {
    saveCustomIONames(names);
    logSystemEvent("Peripheral names updated");
}

String getRelayCustomName(int relayNum) {
    if (relayNum >= 1 && relayNum <= 4) {
        return String(customNames.relays[relayNum - 1]);
    }
    return "Relay " + String(relayNum);
}

String getIONamesJSON() {
    String r = "{";
    r += "\"relays\":[";
    for (int i = 0; i < 4; i++) {
        r += "\"" + String(customNames.relays[i]) + "\"";
        if (i < 3) r += ",";
    }
    r += "],\"inputs\":[";
    for (int i = 0; i < 4; i++) {
        r += "\"" + String(customNames.digitalInputs[i]) + "\"";
        if (i < 3) r += ",";
    }
    r += "],\"analog\":\"" + String(customNames.analogInput) + "\"}";
    return r;
}

// User Presets
static UserPreset userPresets[MAX_USER_PRESETS];
static int activePresetId = 1;

int getActivePresetId() {
    return activePresetId;
}

void setActivePresetId(int id) {
    activePresetId = id;
    File f = LittleFS.open("/active_preset.cfg", "w");
    if (f) {
        f.write((uint8_t*)&activePresetId, sizeof(activePresetId));
        f.close();
    }
}

void loadUserPresets() {
    for (int i = 0; i < MAX_USER_PRESETS; i++) {
        userPresets[i].active = false;
        memset(userPresets[i].name, 0, sizeof(userPresets[i].name));
    }
    if (LittleFS.exists("/user_presets.cfg")) {
        File f = LittleFS.open("/user_presets.cfg", "r");
        if (f) {
            uint32_t magic = 0;
            f.read((uint8_t*)&magic, sizeof(magic));
            if (magic == 0x50524553) {
                f.read((uint8_t*)userPresets, sizeof(userPresets));
                f.close();
                Serial.println("[LittleFS] Loaded user presets.");
            } else {
                f.close();
            }
        }
    }
    if (LittleFS.exists("/active_preset.cfg")) {
        File f = LittleFS.open("/active_preset.cfg", "r");
        if (f) {
            f.read((uint8_t*)&activePresetId, sizeof(activePresetId));
            f.close();
        }
    }
}

void saveUserPresetsToFS() {
    File f = LittleFS.open("/user_presets.cfg", "w");
    if (f) {
        uint32_t magic = 0x50524553;
        f.write((uint8_t*)&magic, sizeof(magic));
        f.write((uint8_t*)userPresets, sizeof(userPresets));
        f.close();
        Serial.println("[LittleFS] Saved user presets.");
    }
}

int saveUserPreset(const String& name) {
    int targetSlot = -1;
    for (int i = 0; i < MAX_USER_PRESETS; i++) {
        if (userPresets[i].active && String(userPresets[i].name).equalsIgnoreCase(name)) {
            targetSlot = i;
            break;
        }
    }
    if (targetSlot == -1) {
        for (int i = 0; i < MAX_USER_PRESETS; i++) {
            if (!userPresets[i].active) {
                targetSlot = i;
                break;
            }
        }
    }
    if (targetSlot == -1) {
        targetSlot = 0; // Overwrite oldest if full
    }
    
    userPresets[targetSlot].active = true;
    strncpy(userPresets[targetSlot].name, name.c_str(), 31);
    userPresets[targetSlot].name[31] = '\0';
    for (int i = 0; i < 4; i++) {
        userPresets[targetSlot].profiles[i] = profiles[i];
    }
    saveUserPresetsToFS();
    setActivePresetId(100 + targetSlot);
    logSystemEvent("Preset saved: \"" + name + "\"");
    return 100 + targetSlot;
}

bool applyUserPreset(int id) {
    int idx = id - 100;
    if (idx < 0 || idx >= MAX_USER_PRESETS || !userPresets[idx].active) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        profiles[i] = userPresets[idx].profiles[i];
    }
    saveSettingsToEEPROM();
    setActivePresetId(id);
    logSystemEvent("Preset applied: \"" + String(userPresets[idx].name) + "\"");
    return true;
}

bool deleteUserPreset(int id) {
    int idx = id - 100;
    if (idx < 0 || idx >= MAX_USER_PRESETS || !userPresets[idx].active) {
        return false;
    }
    String deletedName = String(userPresets[idx].name);
    userPresets[idx].active = false;
    memset(userPresets[idx].name, 0, sizeof(userPresets[idx].name));
    saveUserPresetsToFS();
    if (activePresetId == id) {
        setActivePresetId(0);
    }
    logSystemEvent("Preset deleted: \"" + deletedName + "\"");
    return true;
}

String getPresetsJSON() {
    String r = "[";
    r += "{\"id\":1,\"name\":\"Preset 1: Standard Aquatlantis (Factory)\",\"builtin\":true},";
    r += "{\"id\":2,\"name\":\"Preset 2: Algae Control (Factory)\",\"builtin\":true},";
    r += "{\"id\":3,\"name\":\"Preset 3: Maintenance / Lights Off (Factory)\",\"builtin\":true}";
    
    for (int i = 0; i < MAX_USER_PRESETS; i++) {
        if (userPresets[i].active) {
            r += ",{\"id\":" + String(100 + i) + ",\"name\":\"" + String(userPresets[i].name) + "\",\"builtin\":false}";
        }
    }
    r += "]";
    return r;
}

// Feed Mode state
static bool feedModeActive = false;
static unsigned long feedModeStartTime = 0;
static uint32_t feedModeDurationMs = 600000;

void setFeedMode(bool active, uint32_t durationSec) {
    feedModeActive = active;
    if (active) {
        feedModeStartTime = millis();
        feedModeDurationMs = durationSec * 1000UL;
        logSystemEvent("Feed mode started (" + String(durationSec / 60) + " min) • Air pump paused");
    } else {
        logSystemEvent("Feed mode stopped • Schedule resumed");
    }
}

bool isFeedModeActive() {
    if (feedModeActive && (millis() - feedModeStartTime >= feedModeDurationMs)) {
        feedModeActive = false;
        logSystemEvent("Feed mode completed • Schedule resumed");
    }
    return feedModeActive;
}

uint32_t getFeedModeRemainingSec() {
    if (!feedModeActive) return 0;
    unsigned long elapsed = millis() - feedModeStartTime;
    if (elapsed >= feedModeDurationMs) {
        return 0;
    }
    return (feedModeDurationMs - elapsed) / 1000UL;
}

void resetSettingsToDefault() {
    profiles[0] = { DEFAULT_SCHED_RELAY_1, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[1] = { DEFAULT_SCHED_RELAY_2, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[2] = { DEFAULT_SCHED_RELAY_3, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    profiles[3] = { DEFAULT_SCHED_RELAY_4, BEHAVIOR_CONTINUOUS, DEFAULT_PULSE_ON_SEC, DEFAULT_PULSE_OFF_SEC };
    setActivePresetId(1);
    saveSettingsToEEPROM();
    logSystemEvent("Settings reset to factory defaults");
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
    loadCustomIONames();
    loadUserPresets();
    logSystemEvent("System started • Relays initialized");
}

RelayState getRelayState(int relayNum) {
    if (relayNum < 1 || relayNum > 4) {
        return {false, false, false};
    }
    return relays[relayNum - 1];
}

String getRelayStatusDescription(int relayNum) {
    if (relayNum < 1 || relayNum > 4) return "";
    int idx = relayNum - 1;
    RelayState s = relays[idx];
    RelayProfile p = profiles[idx];
    
    // 1. Manual Override takes precedence
    if (s.manualOverride) {
        return s.manualState ? "⚡ Forced Manual (ON)" : "⚡ Forced Manual (OFF)";
    }
    
    // 2. Power loss mode
    if (lastExecutedMode == MODE_POWER_LOSS) {
        if (relayNum == 3) {
            uint32_t interval = relayPulseState[2] ? 60000 : 120000;
            unsigned long elapsed = millis() - relayLastToggle[2];
            uint32_t rem = (elapsed < interval) ? ((interval - elapsed) / 1000UL) : 0;
            return relayPulseState[2] ? ("⚡ Outage Failsafe: Pulse ON (" + String(rem) + "s)") : ("⚡ Outage Failsafe: Pause (" + String(rem) + "s)");
        }
        return "⚡ Safety shutdown (Power outage)";
    }
    
    // 3. Feeding mode (applies to Air Pump / Relay 3)
    if (isFeedModeActive() && relayNum == 3) {
        uint32_t rem = getFeedModeRemainingSec();
        uint32_t m = rem / 60;
        uint32_t sec = rem % 60;
        return "🫧 Feed Mode • Paused for " + String(m) + "m " + (sec < 10 ? "0" : "") + String(sec) + "s";
    }
    
    // 4. Automatic Scheduler evaluation
    int currentHour = getCurrentHour();
    int hr = (currentHour >= 0 && currentHour <= 23) ? currentHour : 12;
    bool isHourActive = (p.activeHours & (1UL << hr)) != 0;
    
    if (isHourActive) {
        if (p.behavior == BEHAVIOR_CONTINUOUS) {
            int nextOffHour = -1;
            for (int h = 1; h <= 24; h++) {
                int checkH = (hr + h) % 24;
                if ((p.activeHours & (1UL << checkH)) == 0) {
                    nextOffHour = checkH;
                    break;
                }
            }
            if (nextOffHour != -1) {
                return "🟢 On • Until " + String(nextOffHour) + ":00";
            } else {
                return "🟢 Running 24/7";
            }
        } else {
            // Pulse Mode
            unsigned long elapsed = millis() - relayLastToggle[idx];
            uint32_t targetMs = relayPulseState[idx] ? (p.pulseOnSec * 1000UL) : (p.pulseOffSec * 1000UL);
            uint32_t remainingSec = (elapsed < targetMs) ? ((targetMs - elapsed) / 1000UL) : 0;
            
            String remStr;
            if (remainingSec >= 60) {
                remStr = String(remainingSec / 60) + "m " + (remainingSec % 60 < 10 ? "0" : "") + String(remainingSec % 60) + "s";
            } else {
                remStr = String(remainingSec) + "s";
            }
            
            if (relayPulseState[idx]) {
                return "🟢 Running • Pause in " + remStr;
            } else {
                return "⏸️ In Pause • Resumes in " + remStr;
            }
        }
    } else {
        int nextOnHour = -1;
        for (int h = 1; h <= 24; h++) {
            int checkH = (hr + h) % 24;
            if ((p.activeHours & (1UL << checkH)) != 0) {
                nextOnHour = checkH;
                break;
            }
        }
        if (nextOnHour != -1) {
            return "⚪ Off • Starts at " + String(nextOnHour) + ":00";
        } else {
            return "⚪ Off • No schedule active";
        }
    }
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
    
    String rName = getRelayCustomName(relayNum);
    if (overrideActive) {
        if (!oldOverride || oldState != targetState) {
            logSystemEvent("Manual command: " + rName + " -> " + (targetState ? "ON" : "OFF"));
        }
    } else {
        if (oldOverride) {
            logSystemEvent(rName + " -> Reverted to Auto mode");
        }
    }
}

void clearManualOverrides() {
    bool anyChanged = false;
    for (int i = 0; i < 4; i++) {
        if (relays[i].manualOverride) {
            relays[i].manualOverride = false;
            anyChanged = true;
        }
    }
    if (anyChanged) {
        logSystemEvent("All peripherals reverted to Auto mode");
    }
}

void updateRelayProfile(int relayNum, uint32_t activeHours, uint8_t behavior, uint32_t pulseOnSec, uint32_t pulseOffSec) {
    if (relayNum < 1 || relayNum > 4) return;
    
    profiles[relayNum - 1].activeHours = activeHours;
    profiles[relayNum - 1].behavior = behavior;
    profiles[relayNum - 1].pulseOnSec = pulseOnSec;
    profiles[relayNum - 1].pulseOffSec = pulseOffSec;
    
    setActivePresetId(0); // Custom schedule
    saveSettingsToEEPROM();
    String rName = getRelayCustomName(relayNum);
    String behStr;
    if (behavior == 1) {
        String onStr = (pulseOnSec >= 60 && pulseOnSec % 60 == 0) ? String(pulseOnSec / 60) + "m" : String(pulseOnSec) + "s";
        String offStr = (pulseOffSec >= 60 && pulseOffSec % 60 == 0) ? String(pulseOffSec / 60) + "m" : String(pulseOffSec) + "s";
        behStr = "Pulse: " + onStr + " ON / " + offStr + " OFF";
    } else {
        behStr = "Continuous";
    }
    logSystemEvent("Schedule saved: " + rName + " [" + behStr + "]");
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
            
            // Strip year from legacy logs (e.g. 2026-09-27 19:11:08 -> 27.09 19:11:08)
            if (timestamp.length() >= 19 && timestamp.charAt(4) == '-' && timestamp.charAt(7) == '-') {
                String mm = timestamp.substring(5, 7);
                String dd = timestamp.substring(8, 10);
                String timePart = timestamp.substring(11);
                timestamp = dd + "." + mm + " " + timePart;
            }
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
        case MODE_DAY: return "☀️ Day (Normal)";
        case MODE_NIGHT: return "🌙 Night (Normal)";
        case MODE_POWER_LOSS: return "⚡ Power Loss (UPS)";
        default: return "Unknown";
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
    
    // Ambient Lighting (Relay 4) Energy Saving:
    // If ambient light is scheduled to be ON, but natural room light is already bright (e.g. > 50%),
    // turn it OFF to save energy.
    if (targets[3] && getLightLevelPercent() > 50) {
        targets[3] = false;
    }

    // Smart Feeding Mode: If active, force Air Pump (Relay 3) OFF so food doesn't disperse
    if (isFeedModeActive()) {
        targets[2] = false;
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
        Serial.printf("[SYSTEM] System operating mode changed to: %s\n", getModeString(currentMode).c_str());
        lastExecutedMode = currentMode;
    }
}

// Telemetry API implementations
int getLightLevelPercent() {
    int val = analogRead(PIN_INPUT_LIGHT_A0);
    int percent = map(val, 0, 1023, 0, 100);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

// RAM Telemetry circular buffer (last 96 readings)
static int telemetryLightPercent[96] = {0};
static String telemetryTime[96];
static int telemetryCount = 0;
static int telemetryIndex = 0;

void addTelemetryReading(int percent, const String& timeStr) {
    telemetryLightPercent[telemetryIndex] = percent;
    telemetryTime[telemetryIndex] = timeStr;
    telemetryIndex = (telemetryIndex + 1) % 96;
    if (telemetryCount < 96) {
        telemetryCount++;
    }
}

String getTelemetryJSON() {
    String r = "[";
    int start = 0;
    if (telemetryCount == 96) {
        start = telemetryIndex;
    }
    for (int i = 0; i < telemetryCount; i++) {
        int idx = (start + i) % 96;
        r += "{";
        r += "\"time\":\"" + telemetryTime[idx] + "\",";
        r += "\"light\":" + String(telemetryLightPercent[idx]);
        r += "}";
        if (i < telemetryCount - 1) r += ",";
    }
    r += "]";
    return r;
}
