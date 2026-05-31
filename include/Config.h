#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif


// ==========================================
// 1. WiFi & Access Point Settings
// ==========================================
// Enter your local WiFi details here:
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID "Your_WiFi_SSID"
#endif

#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

// Fallback Access Point name if connection fails
#ifndef AP_SSID
#define AP_SSID "BioBox-Aquarium"
#endif

#ifndef AP_PASS
#define AP_PASS "12345678"
#endif

// OTA Update Credentials
#ifndef OTA_USER
#define OTA_USER "admin"
#endif

#ifndef OTA_PASS
#define OTA_PASS "admin123"
#endif


// ==========================================
// 2. Hardware Pin Mappings
// ==========================================
// Relay outputs (configured by jumpers on LC-Relay-ESP12-4R-MV)
#define PIN_RELAY_1 16 // Main Lighting (12V DC)
#define PIN_RELAY_2 14 // CO2 Solenoid (12V DC, Normal Closed)
#define PIN_RELAY_3 12 // Air Pump (5V DC)
#define PIN_RELAY_4 13 // Ambient Lighting (12V DC)

// Digital input pins (exposed on headers J7 and J8)
#define PIN_INPUT_GPIO0       0   // GPIO0
#define PIN_INPUT_GPIO4       4   // GPIO4
#define PIN_INPUT_GPIO2       2   // GPIO2
#define PIN_INPUT_GPIO15      15  // GPIO15

// Analog input pin for Light Sensor LDR
#define PIN_INPUT_LIGHT_A0    A0  // ADC0

// Status LED (Onboard Blue LED, active LOW)
#define PIN_STATUS_LED        5   // GPIO5

// Relay active state (HIGH or LOW)
// If relays trigger on low voltage, change this to LOW.
#define RELAY_ACTIVE_LEVEL HIGH

// ==========================================
// 3. NTP & Timezone Configuration
// ==========================================
#define NTP_SERVER_PRIMARY "ro.pool.ntp.org"
#define NTP_SERVER_BACKUP "pool.ntp.org"

// POSIX Timezone representation for Europe/Bucharest (with DST rules)
// EET is UTC+2, EEST (DST) is UTC+3.
// Starts last Sunday of March (M3.5.0) at 3:00 AM, ends last Sunday of October
// (M10.5.0) at 4:00 AM.
#define TIMEZONE_POSIX "EET-2EEST,M3.5.0/3,M10.5.0/4"

// ==========================================
// 4. Default Operational Schedules & Timers
// ==========================================
// 24-hour scheduler defaults (bitmaps where bit 0 = hour 00:00, bit 23 = hour 23:00)
// Relay 1 (Main Lighting): Active 13:00 - 21:00 (Hours 13 to 20 inclusive) -> 0x1FE000
#define DEFAULT_SCHED_RELAY_1    0x1FE000
// Relay 2 (CO2 Solenoid): Active 11:00 - 19:00 with Siesta 15:00 - 16:00 -> 0x77800
#define DEFAULT_SCHED_RELAY_2    0x77800
// Relay 3 (Air Pump): Active 21:00 - 11:00 (Hours 21-23 and 0-10 inclusive) -> 0xE007FF
#define DEFAULT_SCHED_RELAY_3    0xE007FF
// Relay 4 (Ambient Light): Active 07:00 - 13:00 and 21:00 - 00:00 -> 0xE01F80
#define DEFAULT_SCHED_RELAY_4    0xE01F80

// Pulse Mode default values
#define DEFAULT_PULSE_ON_SEC     60   // 1 minute in seconds
#define DEFAULT_PULSE_OFF_SEC    120  // 2 minutes in seconds

// ==========================================
// 5. EEPROM Layout Configs
// ==========================================
#define EEPROM_SIZE              512
#define EEPROM_MAGIC_NUMBER      0xCAFE2026 // To detect valid saved configuration

#endif // CONFIG_H
