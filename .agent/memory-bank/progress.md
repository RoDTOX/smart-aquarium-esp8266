# Progress Tracking

## Overall Status
Production operational (`v3.6.5`). Multi-theme dashboard engine (Acvariu Viu, Consolă, Clar, Clasic) active, verified over WiFi OTA.

## What Works
- **1-Minute WiFi Outage Confirmation (v3.6.5):** Confirmare de 60s înainte de a declara drop WiFi; spamul de drop-uri tranzitorii de 2s a fost complet eliminat din LittleFS `/log.txt`. Logurile istorice de 2s au fost curățate automat la boot. Auto-reconnect periodic la 30s când e offline.
- **Multi-Theme Engine (v3.6.4):** Live instant switching between 4 distinct layouts (`🐠 Viu`, `📟 Consolă`, `🏡 Clar`, `🏛️ Clasic`) and 3 modes (`Auto`, `Light`, `Dark`).
- **Aeration Bubbles from Airstone:** Buble mari și dese ieșind strict din piatra de aerare din colțul dreapta jos (eliminat offset-ul de scalare SVG ce le plasa la crenguță).
- **Full-Height Vertical Now-Line:** Bara verticală de oră curentă acoperă toate cele 4 relee atât pe tema Viu cât și pe Clar.
- **Timeline Label Pulse Blips:** Indicator ritmic verde/gri integrat direct pe punctele din dreptul canalelor în timeline.
- **24-Hour Labels Everywhere:** Toate orele `00`..`23` afișate pe Viu (Sky), Clar (Matrix), Clasic (Slots) și Consolă (Cadran radial).
- **Compact Channel Program Editor:** Câmpurile de puls ascunse când canalul e pe Continuu; dropdown preset compact cu butoane integrate pe același rând.
- **Batch Schedule Draft:** Local edits with draft bar, real discard, and atomic single-write batch commit to flash.
- **Roving Tabindex Accessibility:** Full arrow-key navigation on 24h timeline slots, dial arcs, and ribbon hours.
- **Zero-Injection Escaping:** Safe text rendering of peripheral names and status descriptions.
- Dynamic custom relay and sensor renaming with LittleFS persistence (`/names.cfg`).
- Custom user-named presets with LittleFS persistence (`/user_presets.cfg`), with In-UI Create, Rename, and Delete.
- Active preset persistence (`/active_preset.cfg`) and modified schedule detection.
- Windows multi-protocol discovery (NetBIOS, LLMNR, SSDP / UPnP) and mobile mDNS.
- Smart Feeding Mode (10-minute automated aeration pause with countdown).
- UPS power loss failsafe mode on GPIO 0.
- Wireless OTA flashing portal at `/update` with event logging hooks.
- 100% offline self-hosted dashboard compressed with gzip (~33.9 KB payload <= 40 KB).

## Known Issues
None. All reported synchronization, responsiveness, and visual layout issues resolved and live-verified on hardware.
