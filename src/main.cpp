#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266LLMNR.h>
#include <ESP8266SSDP.h>
#include <LittleFS.h>
#include "Config.h"
#include "NetworkSync.h"
#include "RelayControl.h"

// Initialize the web server on port 80
ESP8266WebServer server(80);

// Current system-wide operational mode
SystemMode currentSystemMode = MODE_DAY;

// Debounced power state for PIN_INPUT_GPIO0 (Power Sense)
static int debouncedPowerState = HIGH;

// Dashboard page: generated at build time from web/index.html (gzip, PROGMEM)
// by tools/build_web.py -> include/dashboard_html_gz.h
#include "dashboard_html_gz.h"

// Serve Dashboard HTML page from Flash memory
void handleRoot() {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "text/html", (PGM_P)DASHBOARD_HTML_GZ, DASHBOARD_HTML_GZ_LEN);
}

// REST API endpoint: Returns system status and states as JSON
void handleStatus() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String r = "{";
    r += "\"mode\":\"" + jsonEscape(getModeString(currentSystemMode)) + "\",";
    r += "\"mode_id\":" + String((int)currentSystemMode) + ",";
    r += "\"feed_mode\":" + String(isFeedModeActive() ? "true" : "false") + ",";
    r += "\"feed_remaining\":" + String(getFeedModeRemainingSec()) + ",";
    r += "\"inputs\":{";
    r += "\"gpio0\":" + String(debouncedPowerState == HIGH ? "true" : "false") + ",";
    r += "\"gpio4\":" + String(digitalRead(PIN_INPUT_GPIO4) == HIGH ? "true" : "false") + ",";
    r += "\"gpio2\":" + String(digitalRead(PIN_INPUT_GPIO2) == HIGH ? "true" : "false") + ",";
    r += "\"gpio15\":" + String(digitalRead(PIN_INPUT_GPIO15) == HIGH ? "true" : "false");
    r += "},";
    r += "\"time\":\"" + jsonEscape(getFormattedTime()) + "\",";
    r += "\"wifi_ssid\":\"" + jsonEscape(getWiFiSSID()) + "\",";
    r += "\"wifi_rssi\":" + String(getWiFiRSSI()) + ",";
    r += "\"wifi_status\":\"" + jsonEscape(getNetworkStatusString()) + "\",";
    r += "\"ip\":\"" + getIPAddress() + "\",";
    r += "\"uptime\":" + String(millis() / 1000) + ",";
    r += "\"light_percent\":" + String(getLightLevelPercent()) + ",";
    r += "\"active_preset\":" + String(getActivePresetId()) + ",";
    r += "\"history_rev\":" + String(getHistoryRevision()) + ",";
    
    // Relay items
    r += "\"relays\":[";
    for (int i = 1; i <= 4; i++) {
        RelayState s = getRelayState(i);
        r += "{";
        r += "\"num\":" + String(i) + ",";
        r += "\"name\":\"" + jsonEscape(getRelayCustomName(i)) + "\",";
        r += "\"state\":" + String(s.physicalState ? "true" : "false") + ",";
        r += "\"override\":" + String(s.manualOverride ? "true" : "false") + ",";
        r += "\"override_state\":" + String(s.manualState ? "true" : "false") + ",";
        r += "\"in_pause\":" + String(isRelayInPulsePause(i) ? "true" : "false") + ",";
        r += "\"status_desc\":\"" + jsonEscape(getRelayStatusDescription(i)) + "\"";
        r += "}";
        if (i < 4) r += ",";
    }
    r += "],";
    
    // Custom names
    r += "\"names\":" + getIONamesJSON();
    r += "}";
    server.send(200, "application/json", r);
}

// REST API endpoint: Configures manual override for relays
void handleOverride() {
    if (server.hasArg("clear") && server.arg("clear") == "1") {
        clearManualOverrides();
        server.send(200, "text/plain", "Cleared all overrides");
        return;
    }
    
    if (server.hasArg("relay")) {
        int rNum = server.arg("relay").toInt();
        if (rNum >= 1 && rNum <= 4) {
            bool overrideActive = false;
            bool targetState = false;
            
            if (server.hasArg("override")) {
                overrideActive = (server.arg("override") == "1");
            }
            if (server.hasArg("state")) {
                targetState = (server.arg("state") == "1");
            }
            
            setRelayManualOverride(rNum, overrideActive, targetState);
            server.send(200, "text/plain", "Override configured");
            return;
        }
    }
    
    server.send(400, "text/plain", "Bad Request");
}

// REST API endpoint: Set/Update relay profiles (persisted to LittleFS)
// Batch form (preferred, one write): r1_hours, r1_behavior, r1_on, r1_off ... r4_*
// Legacy form: relay, active_hours, behavior, pulse_on, pulse_off
void handleScheduleSet() {
    if (server.hasArg("r1_hours")) {
        RelayProfile next[4];
        for (int i = 0; i < 4; i++) {
            String p = "r" + String(i + 1) + "_";
            if (!server.hasArg(p + "hours") || !server.hasArg(p + "behavior") ||
                !server.hasArg(p + "on") || !server.hasArg(p + "off")) {
                server.send(400, "text/plain", "Bad Request");
                return;
            }
            next[i].activeHours = strtoul(server.arg(p + "hours").c_str(), NULL, 10) & 0xFFFFFFUL;
            next[i].behavior = (uint8_t)server.arg(p + "behavior").toInt();
            next[i].pulseOnSec = (uint32_t)server.arg(p + "on").toInt();
            next[i].pulseOffSec = (uint32_t)server.arg(p + "off").toInt();
            if (next[i].behavior > 1 || next[i].pulseOnSec == 0 || next[i].pulseOffSec == 0) {
                server.send(400, "text/plain", "Bad Request");
                return;
            }
            if (next[i].behavior == 1 && (next[i].pulseOnSec + next[i].pulseOffSec > 3600)) {
                if (next[i].pulseOnSec >= 3600) { next[i].pulseOnSec = 3540; next[i].pulseOffSec = 60; }
                else { next[i].pulseOffSec = 3600 - next[i].pulseOnSec; }
            }
        }
        updateAllRelayProfiles(next);
        server.send(200, "text/plain", "OK");
        return;
    }
    if (server.hasArg("relay") && server.hasArg("active_hours") && server.hasArg("behavior") && server.hasArg("pulse_on") && server.hasArg("pulse_off")) {
        int r = server.arg("relay").toInt();
        uint32_t hours = strtoul(server.arg("active_hours").c_str(), NULL, 10);
        uint8_t behavior = server.arg("behavior").toInt();
        uint32_t on = server.arg("pulse_on").toInt();
        uint32_t off = server.arg("pulse_off").toInt();
        
        if (r >= 1 && r <= 4 && (behavior == 0 || behavior == 1) && on > 0 && off > 0) {
            if (behavior == 1 && (on + off > 3600)) {
                if (on >= 3600) { on = 3540; off = 60; }
                else { off = 3600 - on; }
            }
            updateRelayProfile(r, hours, behavior, on, off);
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Bad Request");
}

// REST API endpoint: Fetch active schedules for all relays
void handleScheduleGet() {
    server.send(200, "application/json", getSchedulesJSON());
}

// REST API endpoint: Returns rolling event log history
void handleHistory() {
    server.send(200, "application/json", getHistoryJSON());
}

// REST API endpoint: Saves new WiFi SSID and Password, then reconnects
void handleWiFi() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
        String newSSID = server.arg("ssid");
        String newPass = server.arg("pass");
        setWiFiCredentials(newSSID, newPass);
        server.send(200, "text/plain", "Reconnecting...");
        return;
    }
    server.send(400, "text/plain", "Bad Request");
}

// REST API endpoint: Presets management
void handlePresets() {
    if (server.hasArg("apply")) {
        int presetNum = server.arg("apply").toInt();
        if (presetNum >= 1 && presetNum <= 3) {
            applyPreset(presetNum);
            server.send(200, "application/json", "{\"success\":true}");
            return;
        } else if (presetNum >= 100) {
            bool ok = applyUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true}");
                return;
            }
        }
    } else if (server.hasArg("overwrite")) {
        int presetNum = server.arg("overwrite").toInt();
        if (presetNum >= 100) {
            bool ok = overwriteUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true,\"id\":" + String(presetNum) + "}");
                return;
            }
        }
    } else if (server.hasArg("save") && server.hasArg("name")) {
        String name = server.arg("name");
        name.trim();
        if (name.length() > 0) {
            int id = saveUserPreset(name);
            server.send(200, "application/json", "{\"success\":true,\"id\":" + String(id) + "}");
            return;
        }
    } else if (server.hasArg("delete")) {
        int presetNum = server.arg("delete").toInt();
        if (presetNum >= 100) {
            bool ok = deleteUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true}");
                return;
            }
        }
    } else if (server.hasArg("rename") && server.hasArg("name")) {
        int presetNum = server.arg("rename").toInt();
        String name = server.arg("name");
        name.trim();
        if (presetNum >= 100 && name.length() > 0) {
            bool ok = renameUserPreset(presetNum, name);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true,\"id\":" + String(presetNum) + "}");
                return;
            }
        }
    } else if (server.hasArg("reset") && server.arg("reset") == "1") {
        resetSettingsToDefault();
        server.send(200, "application/json", "{\"success\":true}");
        return;
    }
    server.send(400, "text/plain", "Bad Request");
}

// REST API endpoint: Custom IO names management
void handleNamesSet() {
    CustomIONames names = getCustomIONames();
    
    // Copies at most 31 chars and always NUL-terminates the 32-byte field
    auto setName = [](char* dst, const char* argName) {
        if (!server.hasArg(argName)) return;
        String v = server.arg(argName);
        v.trim();
        strncpy(dst, v.c_str(), 31);
        dst[31] = 0;
    };
    setName(names.relays[0], "r1");
    setName(names.relays[1], "r2");
    setName(names.relays[2], "r3");
    setName(names.relays[3], "r4");
    setName(names.digitalInputs[0], "in0");
    setName(names.digitalInputs[1], "in4");
    setName(names.digitalInputs[2], "in2");
    setName(names.digitalInputs[3], "in15");
    setName(names.analogInput, "a0");
    
    setCustomIONames(names);
    server.send(200, "application/json", "{\"success\":true}");
}

// REST API endpoint: Feed mode control
void handleFeed() {
    if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "start") {
            uint32_t dur = server.hasArg("duration") ? server.arg("duration").toInt() : 600;
            setFeedMode(true, dur);
            server.send(200, "application/json", "{\"success\":true,\"active\":true}");
            return;
        } else if (action == "stop") {
            setFeedMode(false);
            server.send(200, "application/json", "{\"success\":true,\"active\":false}");
            return;
        }
    }
    server.send(400, "text/plain", "Bad Request");
}

void setup() {
    // Start Serial debug port
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n========================================");
    Serial.println("Aquatlantis Smart Aquarium Controller v3.5.0");
    Serial.println("========================================");
    
    // Setup inputs
    pinMode(PIN_INPUT_GPIO0, INPUT);
    pinMode(PIN_INPUT_GPIO4, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO2, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO15, INPUT);
    Serial.println("[Init] GPIO pins configured.");
    
    // Initialize Submodules (loads settings, names, presets, and logs boot event)
    initNetwork();
    initRelays();
    
    // Web Server URL configuration
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/override", HTTP_POST, handleOverride);
    server.on("/api/schedule", HTTP_GET, handleScheduleGet);
    server.on("/api/schedule", HTTP_POST, handleScheduleSet);
    server.on("/api/history", HTTP_GET, handleHistory);
    server.on("/api/wifi", HTTP_POST, handleWiFi);
    server.on("/api/presets", HTTP_GET, []() {
        server.send(200, "application/json", getPresetsJSON());
    });
    server.on("/api/presets", HTTP_POST, handlePresets);
    server.on("/api/names", HTTP_GET, []() {
        server.send(200, "application/json", getIONamesJSON());
    });
    server.on("/api/names", HTTP_POST, handleNamesSet);
    server.on("/api/feed", HTTP_POST, handleFeed);
    
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
    
    // Start Web Server
    server.begin();
    Serial.println("[Init] HTTP server started on port 80.");
    
    // Start mDNS responder (acvariu.local)
    if (MDNS.begin("acvariu")) {
        Serial.println("[mDNS] Started successfully. Access via http://acvariu.local/");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("[mDNS] Error starting responder.");
    }

    // Start NetBIOS responder (Windows http://acvariu/ native resolution)
    NBNS.begin("ACVARIU");
    Serial.println("[NetBIOS] Started. Access via http://acvariu/ on Windows.");

    // Start LLMNR responder (Link-Local Multicast Name Resolution for Windows 10/11)
    LLMNR.begin("acvariu");
    Serial.println("[LLMNR] Started. Resolves http://acvariu/ on Windows 10/11.");

    // Start SSDP responder (Shows up in Windows Explorer -> Network)
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("Aquatlantis Smart Aquarium");
    SSDP.setSerialNumber("ESP8266-AQUARIUM-01");
    SSDP.setURL("/");
    SSDP.setModelName("ESP-12F Aquarium Controller");
    SSDP.setManufacturer("Aquatlantis");
    SSDP.begin();
    server.on("/description.xml", HTTP_GET, [](){
        SSDP.schema(server.client());
    });
    Serial.println("[SSDP] Windows UPnP Network Discovery registered.");
    
    // Setup Custom OTA Update with event logging and LittleFS post-flash detection
    server.on("/update", HTTP_GET, []() {
        if (!server.authenticate(OTA_USER, OTA_PASS)) {
            return server.requestAuthentication();
        }
        String html = "<!DOCTYPE html><html><head><title>Aquatlantis OTA Update</title><meta name='viewport' content='width=device-width, initial-scale=1'></head>"
                      "<body style='font-family:sans-serif;background:#080d1a;color:#fff;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;'>"
                      "<div style='background:#151d30;padding:26px;border-radius:12px;border:1px solid rgba(255,255,255,0.1);max-width:400px;width:100%;text-align:center;'>"
                      "<h2 style='margin-top:0;color:#38bdf8;'>Aquatlantis OTA Update</h2>"
                      "<p style='color:#94a3b8;font-size:14px;'>Upload compiled firmware.bin binary:</p>"
                      "<form method='POST' action='/update' enctype='multipart/form-data' style='display:flex;flex-direction:column;gap:16px;margin-top:20px;'>"
                      "<input type='file' name='firmware' accept='.bin' style='color:#fff;background:rgba(255,255,255,0.05);padding:10px;border-radius:6px;border:1px solid rgba(255,255,255,0.1);'>"
                      "<input type='submit' value='Flash Firmware' style='background:#38bdf8;color:#080d1a;border:none;padding:12px;border-radius:6px;font-weight:700;cursor:pointer;'>"
                      "</form>"
                      "<p style='margin-top:20px;'><a href='/' style='color:#38bdf8;text-decoration:none;'>&larr; Back to Dashboard</a></p>"
                      "</div></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, []() {
        if (!server.authenticate(OTA_USER, OTA_PASS)) {
            return server.requestAuthentication();
        }
        if (Update.hasError()) {
            server.send(200, "text/html", "Update error: " + String(Update.getError()));
            logSystemEvent("OTA update failed (Error: " + String(Update.getError()) + ")");
        } else {
            server.client().setNoDelay(true);
            server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='12;URL=/'><title>Success</title></head><body style='font-family:sans-serif;background:#080d1a;color:#10b981;text-align:center;padding:50px;'><h2>OTA Update Successful!</h2><p style='color:#fff;'>Device is rebooting. Redirecting in 12 seconds...</p></body></html>");
            delay(150);
            server.client().stop();
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            if (!server.authenticate(OTA_USER, OTA_PASS)) return;
            WiFiUDP::stopAll();
            Serial.printf("[OTA] Flash started: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, U_FLASH)) {
                Update.printError(Serial);
                logSystemEvent("OTA update failed: Insufficient space");
            } else {
                logSystemEvent("OTA update initiated (" + upload.filename + ")");
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Flash successful: %u bytes\n", upload.totalSize);
                float kb = (float)upload.totalSize / 1024.0f;
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f KB", kb);
                logSystemEvent("OTA update completed (" + String(buf) + ") • Rebooting");
                
                File f = LittleFS.open("/flash_done.flag", "w");
                if (f) {
                    f.print(upload.filename + " [" + String(buf) + "]");
                    f.close();
                }
            } else {
                Update.printError(Serial);
                logSystemEvent("OTA update failed during finalization");
            }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            Update.end();
            logSystemEvent("OTA update aborted by user");
        }
    });
    Serial.println("[Init] OTA web updater registered on /update with logging hooks.");
}

void loop() {
    // 1. Maintain background tasks (NTP sync checks, connection updates, status LED blink)
    updateNetwork();
    
    // Detect reconnection after OTA flash
    static bool flashReconnectedLogged = false;
    if (!flashReconnectedLogged && WiFi.status() == WL_CONNECTED && isTimeSynced()) {
        flashReconnectedLogged = true;
        if (LittleFS.exists("/flash_done.flag")) {
            File f = LittleFS.open("/flash_done.flag", "r");
            String flashInfo = "firmware.bin";
            if (f) {
                flashInfo = f.readString();
                flashInfo.trim();
                f.close();
            }
            LittleFS.remove("/flash_done.flag");
            logSystemEvent("New firmware active • Connected to WiFi (" + getIPAddress() + ")");
        }
    }
    
    // Maintain mDNS responder
    MDNS.update();
    
    // 1. Debounce PIN_INPUT_GPIO0 (Power Sense) to avoid false triggers or flapping
    static unsigned long lastPowerChangeTime = 0;
    static int lastPowerRawState = HIGH;
    
    int currentPowerRaw = digitalRead(PIN_INPUT_GPIO0);
    if (currentPowerRaw != lastPowerRawState) {
        lastPowerChangeTime = millis();
        lastPowerRawState = currentPowerRaw;
    }
    
    if ((millis() - lastPowerChangeTime) >= 100) { // 100ms stable debounce period
        debouncedPowerState = currentPowerRaw;
    }
    
    // Outage Duration Tracking: WiFi drops and Power Loss
    static bool wasInPowerLoss = false;
    static unsigned long powerLossStartMillis = 0;
    static bool wasWifiConnected = true;
    static unsigned long wifiDropStartMillis = 0;
    
    // Check WiFi connection status and measure disconnect duration
    bool isWifiConnected = (WiFi.status() == WL_CONNECTED);
    if (!isWifiConnected && wasWifiConnected) {
        wifiDropStartMillis = millis();
        wasWifiConnected = false;
        Serial.println(F("[WIFI] Connection lost! Tracking outage duration..."));
    } else if (isWifiConnected && !wasWifiConnected) {
        wasWifiConnected = true;
        if (wifiDropStartMillis > 0) {
            uint32_t durSec = (millis() - wifiDropStartMillis) / 1000UL;
            wifiDropStartMillis = 0;
            logSystemEvent("[WIFI] Connection Restored after " + formatDuration(durSec) + " outage");
        }
    }
    
    // 2. Execute state machine logic based on NTP synchronized time and Power Sense (debounced)
    if (debouncedPowerState == LOW) {
        currentSystemMode = MODE_POWER_LOSS;
        if (!wasInPowerLoss) {
            wasInPowerLoss = true;
            powerLossStartMillis = millis();
            logSystemEvent("[PWR] Power Outage Detected! Switched to Battery Failsafe");
        }
    } else {
        if (wasInPowerLoss) {
            wasInPowerLoss = false;
            uint32_t durSec = (powerLossStartMillis > 0) ? ((millis() - powerLossStartMillis) / 1000UL) : 0;
            powerLossStartMillis = 0;
            logSystemEvent("[PWR] AC Grid Restored after " + formatDuration(durSec) + " outage");
        }
        
        if (isTimeSynced()) {
            int currentHour = getCurrentHour();
            RelayProfile r3 = getRelayProfile(3);
            
            // For logging logic, we determine general "Day/Night" based on Relay 3 (Air Pump) active hours
            bool airPumpScheduled = (r3.activeHours & (1UL << currentHour)) != 0;
            if (airPumpScheduled) {
                currentSystemMode = MODE_NIGHT;
            } else {
                currentSystemMode = MODE_DAY;
            }
        } else {
            // Safe fallback: If WiFi is down or NTP hasn't updated yet, run in DAY mode
            currentSystemMode = MODE_DAY;
        }
    }
    
    // 3. Command relays based on calculated system state & scheduling profiles
    updateRelays(currentSystemMode);

    // 4. Smart Diagnostic: Detect Main Light (Relay 1) physical failure
    static bool lampDefectLogged = false;
    static unsigned long lampTurnedOnTime = 0;
    RelayState relay1State = getRelayState(1);
    
    if (relay1State.physicalState) {
        if (lampTurnedOnTime == 0) {
            lampTurnedOnTime = millis();
        }
        // If light has been ON for more than 30 seconds, check the sensor
        if (millis() - lampTurnedOnTime >= 30000) {
            int lightLvl = getLightLevelPercent();
            if (lightLvl < 15 && !lampDefectLogged) {
                logSystemEvent("Warning: " + getRelayCustomName(1) + " is ON, but sensor reads under 15%");
                lampDefectLogged = true;
            }
        }
    } else {
        lampTurnedOnTime = 0;
        lampDefectLogged = false;
    }
    
    // 5. Web requests handler
    server.handleClient();
    
    // 6. Periodic serial logger (every 5 seconds) to aid deployment diagnostics
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime >= 5000) {
        lastLogTime = millis();
        
        Serial.printf("\n[LOG] Time: %s | Mode: %s | WiFi: %s | IP: %s\n",
                      getFormattedTime().c_str(),
                      getModeString(currentSystemMode).c_str(),
                      getNetworkStatusString().c_str(),
                      getIPAddress().c_str());
                      
        Serial.printf("      Inputs: GPIO0:%d | GPIO4:%d | GPIO2:%d | GPIO15:%d\n",
                      digitalRead(PIN_INPUT_GPIO0),
                      digitalRead(PIN_INPUT_GPIO4),
                      digitalRead(PIN_INPUT_GPIO2),
                      digitalRead(PIN_INPUT_GPIO15));
                      
        for (int i = 1; i <= 4; i++) {
            RelayState s = getRelayState(i);
            RelayProfile p = getRelayProfile(i);
            String modeStr = s.manualOverride ? "MANUAL" : "AUTO";
            String behaviorStr = p.behavior == 1 ? "PULSE (" + String(p.pulseOnSec) + "s/" + String(p.pulseOffSec) + "s)" : "CONTINUOUS";
            
            Serial.printf("      Relay %d [%s]: %s | Mode: %s | Behavior: %s\n",
                          i,
                          getRelayCustomName(i).c_str(),
                          s.physicalState ? "ON" : "OFF",
                          modeStr.c_str(),
                          behaviorStr.c_str());
        }
        Serial.println("-----------------------------------------------------------------");
    }
    
    // Tiny delay to yield to the ESP8266 background processes (prevent watchdog timeouts)
    delay(5);
}
