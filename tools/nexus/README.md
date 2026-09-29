# Nexus Gateway (Samsung Galaxy A6 / Termux Debian Server)

Unified reverse proxy, central dashboard portal, and DNS service integrating:
- **🐠 Aquatlantis Smart Aquarium Controller:** `http://nexus/aquarium/` (ESP8266 on `192.168.1.32`)
- **🚗 TeslaMate Telemetry:** `http://nexus/teslamate/` (Elixir/Phoenix on port 4000)
- **📊 Grafana Dashboards:** `http://nexus/grafana/` (Port 3000)
- **⚡ Smart Services Portal:** `http://nexus/` (Single-page glassmorphic dashboard with live health checks)

## Architecture & Files
- `nexus_portal.html`: Central dashboard portal with live polling of all 3 services.
- `nexus_nginx.conf`: Nginx reverse proxy configuration supporting subpaths, assets, and WebSocket upgrades (`/live/websocket` for LiveView).
- `start_nexus.sh`: Gateway launch script configuring iptables port 80 &rarr; 8088 redirection and services.
- `nexus_dns_responder.py`: Local DNS/multicast helper for resolving `nexus` across the LAN.
- `patch_watchdog.py`: Watchdog health check patch preventing false Grafana kills under `/grafana/` subpath.
