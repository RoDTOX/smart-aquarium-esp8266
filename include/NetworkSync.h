#ifndef NETWORK_SYNC_H
#define NETWORK_SYNC_H

#include <Arduino.h>

// Initialize Wifi and NTP settings
void initNetwork();

// Non-blocking background updater for WiFi reconnection
void updateNetwork();

// Check if NTP has successfully synced time
bool isTimeSynced();

// Fetch time parts
int getCurrentHour();

// Fetch human readable timestamp
String getFormattedTime();

// Network status queries
String getWiFiSSID();
int32_t getWiFiRSSI();
String getNetworkStatusString();
String getIPAddress();

// Set new credentials and attempt reconnection
void setWiFiCredentials(const String& ssid, const String& pass);

#endif // NETWORK_SYNC_H
