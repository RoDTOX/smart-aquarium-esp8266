# Tech Context

## Toolchain & Dependencies
- **Build System:** PlatformIO Core (CLI `pio run`).
- **Framework:** Arduino framework for ESP8266 (`framework-arduinoespressif8266 @ 3.30102.0`).
- **Target Board:** `esp12e` (ESP8266 80MHz, 80KB RAM, 4MB Flash).
- **Filesystem:** LittleFS (SPIFFS replacement with wear-leveling).
- **Deployment:** Wireless OTA via HTTP POST to `/update` (`curl.exe -i -u admin:<pass> -F "firmware=@firmware.bin" http://<ip>/update`).
- **Secrets Management:** `include/secrets.h` is strictly gitignored.
