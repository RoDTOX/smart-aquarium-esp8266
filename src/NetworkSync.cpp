#include <time.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "NetworkSync.h"

static bool time_synced = false;

void initNetwork() {
    Serial.println("Initializing NetworkSync...");
    
    // Configure Onboard Status LED (Active LOW)
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // Start with LED OFF
    
    // Set Hostname for the device
    WiFi.hostname("Aquatlantis-Aquarium");
    
    // Start STA + AP mode so the user can always configure the device locally
    WiFi.mode(WIFI_AP_STA);
    
    // Start access point with configured credentials
    bool ap_ok = WiFi.softAP(AP_SSID, AP_PASS);
    if (ap_ok) {
        Serial.print("SoftAP Started. IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("Failed to start SoftAP.");
    }
    
    // Start station connection
    // If the SSID is not the default placeholder, initialize connection.
    if (strcmp(DEFAULT_WIFI_SSID, "Your_WiFi_SSID") != 0 && strlen(DEFAULT_WIFI_SSID) > 0) {
        Serial.printf("Connecting to configured SSID: %s...\n", DEFAULT_WIFI_SSID);
        WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);
    } else {
        Serial.println("No default SSID configured. Attempting to connect with saved credentials...");
        WiFi.begin(); // ESP8266 SDK automatically loads last saved credentials from flash
    }
    
    // Configure NTP using Espressif's built-in SDK function
    // configTime maps NTP packets directly to the system clock (time.h API)
    Serial.printf("Configuring NTP with timezone: %s\n", TIMEZONE_POSIX);
    configTime(TIMEZONE_POSIX, NTP_SERVER_PRIMARY, NTP_SERVER_BACKUP);
}

void updateNetwork() {
    static wl_status_t last_status = WL_IDLE_STATUS;
    wl_status_t current_status = WiFi.status();
    
    if (current_status != last_status) {
        last_status = current_status;
        Serial.printf("[WiFi] Status changed to: %s\n", getNetworkStatusString().c_str());
        if (current_status == WL_CONNECTED) {
            Serial.print("[WiFi] Connected successfully. Local IP: ");
            Serial.println(WiFi.localIP());
        }
    }

    // Status LED Blinking Logic (Active LOW)
    unsigned long now = millis();
    static unsigned long lastLEDToggle = 0;
    static bool ledState = false; // false = LED OFF (HIGH), true = LED ON (LOW)

    unsigned long blinkInterval = 1000;
    bool isPulsePattern = false;

    if (current_status != WL_CONNECTED) {
        // Fast blink (100ms ON, 100ms OFF) - trying to connect to WiFi
        blinkInterval = 100;
    } else if (!isTimeSynced()) {
        // Medium blink (500ms ON, 500ms OFF) - connected to WiFi, waiting for NTP sync
        blinkInterval = 500;
    } else {
        // Heartbeat pattern - 100ms pulse every 3000ms
        isPulsePattern = true;
    }

    if (isPulsePattern) {
        // Heartbeat pulse: ON for 100ms, OFF for 2900ms
        unsigned long cycleTime = now % 3000;
        if (cycleTime < 100) {
            digitalWrite(PIN_STATUS_LED, LOW);  // LED ON (Active LOW)
        } else {
            digitalWrite(PIN_STATUS_LED, HIGH); // LED OFF
        }
    } else {
        // Symmetric blink for connecting states
        if (now - lastLEDToggle >= blinkInterval) {
            lastLEDToggle = now;
            ledState = !ledState;
            digitalWrite(PIN_STATUS_LED, ledState ? LOW : HIGH);
        }
    }
}

bool isTimeSynced() {
    time_t now = time(nullptr);
    // Year 2026 is ~1767129600 epoch seconds. We check if time is past 1000000 seconds.
    if (now > 1000000LL) {
        time_synced = true;
    } else {
        time_synced = false;
    }
    return time_synced;
}

int getCurrentHour() {
    if (!isTimeSynced()) return -1;
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_hour;
}



String getFormattedTime() {
    if (!isTimeSynced()) {
        return "NTP Not Synced";
    }
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buf);
}

String getWiFiSSID() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return "Disconnected";
}

int32_t getWiFiRSSI() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return -99;
}

String getNetworkStatusString() {
    switch (WiFi.status()) {
        case WL_NO_SHIELD: return "No Shield";
        case WL_IDLE_STATUS: return "Idle";
        case WL_NO_SSID_AVAIL: return "No SSID Available";
        case WL_SCAN_COMPLETED: return "Scan Completed";
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connect Failed";
        case WL_CONNECTION_LOST: return "Connection Lost";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown";
    }
}

String getIPAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

void setWiFiCredentials(const String& ssid, const String& pass) {
    Serial.printf("[WiFi] Changing credentials. SSID: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
}
