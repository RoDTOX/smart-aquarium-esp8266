# Plan de Design UI: Aquatlantis Dashboard

Acest document descrie arhitectura interfeței web minimale găzduite direct pe ESP8266 pentru controlul acvariului BioBox 56L.

## 1. Concepte Vizuale și Sistemul de Culori (Aestetica Premium)
Pentru a asigura o primă impresie premium și lizibilitate în mediu întunecat (lângă acvariu noaptea), interfața folosește o temă dark modernă bazată pe următoarele variabile CSS (coloristică din paleta Slate/Sky/Emerald):

*   **Fundal Principal (`--bg-base`):** `#0b0f19` (un bleumarin foarte închis, aproape negru).
*   **Fundal Carduri (`--bg-surface` / `--bg-card`):** `#151d30` și `#1e2942` (nuanțe de albastru închis pentru a oferi profunzime tridimensională).
*   **Culoare Accent (`--accent-primary`):** `#38bdf8` (Cyan luminos) cu un efect de strălucire (`box-shadow` cu opacitate).
*   **Stări Indicatori:**
    *   *OK / PORNIT (`--state-ok`):* `#10b981` (Smarald cu aură verde).
    *   *Atenție / Puls (`--state-warn`):* `#f59e0b` (Chihlimbar).
    *   *Eroare / Oprit (`--state-danger`):* `#ef4444` (Roșu stins).

---

## 2. Wireframe / Structura Paginii
Pagina este organizată într-o grilă responsivă care se adaptează automat de la telefon (o singură coloană) la tabletă/desktop (două sau mai multe coloane).

```
+--------------------------------------------------------------+
| [Icon] Aquatlantis                WiFi Status: [Connected (O)] |
| Smart Aquarium Controller | BioBox 56L                       |
+--------------------------------------------------------------+
|                                                              |
|  +-------------------------+    +-------------------------+  |
|  | Stare Sistem            |    | Control Periferice      |  |
|  |-------------------------|    |-------------------------|  |
|  | Mod: Normal - Zi  (O)   |    | Iluminat Principal [ON ]|  |
|  | Rețea 220V: Activ (O)   |    | [Forțează] [Auto/Man]   |  |
|  | Timp: 16:40:34          |    |                         |  |
|  | Uptime: 2h 15m          |    | Electrovalvă CO2   [ON ]|  |
|  | RSSI: -65 dBm           |    | [Forțează] [Auto/Man]   |  |
|  | IP: 192.168.1.150       |    |                         |  |
|  +-------------------------+    | Pompă de Aer       [OFF]|  |
|                                 | [Forțează] [Auto/Man]   |  |
|  +-------------------------+    |                         |  |
|  | Configurare Programe    |    | Releu 4 / Aux      [OFF]|  |
|  |-------------------------|    | [Forțează] [Auto/Man]   |  |
|  | Zi Start: [ 08 ]        |    |                         |  |
|  | Zi End:   [ 20 ]        |    | [Revenire la Auto (Toate)]|
|  | Pompă ON: [ 60 ] sec     |    +-------------------------+  |
|  | Pompă OFF:[120 ] sec     |                                 |
|  | [Salvează Setări]       |    +-------------------------+  |
|  +-------------------------+    | Configurare WiFi        |  |
|                                 |-------------------------|  |
|                                 | SSID:   [My_SSID]       |  |
|                                 | Parolă: [•••••••]       |  |
|                                 | [Conectează Dispozitiv] |  |
|                                 +-------------------------+  |
+--------------------------------------------------------------+
```

---

## 3. Integrare REST API și JSON
Interfața comunică exclusiv prin apeluri asincrone `fetch` către endpoints-urile web serverului ESP8266:

### `GET /api/status`
Returnează starea completă de telemetrie și funcționare în format JSON:
```json
{
  "mode": "MOD NORMAL - ZI",
  "mode_id": 0,
  "inputs": {
    "gpio0": true,
    "gpio4": true,
    "gpio2": true,
    "gpio15": false
  },
  "time": "2026-05-30 01:25:00",
  "wifi_ssid": "Your_WiFi_SSID",
  "wifi_rssi": -54,
  "wifi_status": "Connected",
  "ip": "192.168.1.32",
  "uptime": 720,
  "relays": [
    {"num": 1, "name": "Iluminat Principal", "state": true, "override": false, "override_state": false},
    {"num": 2, "name": "Electrovalva CO2", "state": true, "override": false, "override_state": false},
    {"num": 3, "name": "Pompa de Aer", "state": false, "override": false, "override_state": false},
    {"num": 4, "name": "Iluminat Ambiental", "state": false, "override": false, "override_state": false}
  ]
}
```

### `GET /api/override`
Configurează forțarea manuală a stărilor releelor.
Parametri:
*   `clear=1`: Resetează toate releele înapoi pe controlul automat (Orar).
*   `relay=[1-4]&override=[0|1]&state=[0|1]`: Setează starea manuală (`state=1` pentru ON, `state=0` pentru OFF) sau dezactivează forțarea manuală (`override=0`).

### `GET /api/schedule` sau `POST /api/schedule`
Citește sau actualizează orarele individuale ale releelor.
*   **Citire (`GET /api/schedule` fără parametri)**: Returnează un array JSON cu setările active:
    ```json
    [
      {"num": 1, "active_hours": 2088960, "behavior": 0, "pulse_on": 60, "pulse_off": 120},
      {"num": 2, "active_hours": 489472, "behavior": 0, "pulse_on": 60, "pulse_off": 120},
      {"num": 3, "active_hours": 14682111, "behavior": 0, "pulse_on": 60, "pulse_off": 120},
      {"num": 4, "active_hours": 14680064, "behavior": 0, "pulse_on": 60, "pulse_off": 120}
    ]
    ```
*   **Actualizare (`POST /api/schedule` sau `GET /api/schedule?relay=...`)**:
    Parametri necesari:
    *   `relay=[1-4]`
    *   `active_hours=[valoare zecimală bitmap 24 biți]` (ex: `2088960` reprezintă `0x1FE000` pentru orele 13-20)
    *   `behavior=[0|1]` (0 = Continuu, 1 = Pulsatoriu)
    *   `pulse_on=[secunde]`
    *   `pulse_off=[secunde]`

### `GET /api/history`
Returnează ultimele loguri înregistrate în memoria flash în format JSON:
```json
[
  {"time": "2026-05-30 01:25:00", "msg": "Sistem pornit. Relee inițializate."},
  {"time": "2026-05-30 01:27:04", "msg": "Releul 1: Forțat MANUAL pe PORNIT"}
]
```

### `POST /api/presets`
Aplică un preset global sau execută o resetare completă de configurare.
Parametri:
*   `apply=[1-3]`: Încarcă presetul dorit (1 = Standard, 2 = Control Alge, 3 = Doar Aerare).
*   `reset=1`: Șterge fișierul LittleFS `/schedules.cfg` și încarcă valorile implicite din cod.

### `GET /api/wifi`
Modifică credențialele rețelei locale WiFi de acasă.
Parametri:
*   `ssid=[SSID]`, `pass=[Parolă]`: Pornește reconectarea la noul router.

---

## 4. Micro-interacțiuni Frontend
1.  **Polling Automat:** Pagina execută `fetch('/api/status')` o dată la 2 secunde pentru a actualiza ceasul NTP, uptime-ul, semnalul WiFi și stările releelor în timp real fără reîncărcare.
2.  **Toast Notification:** La trimiterea unei comenzi, un element plutitor în josul paginii afișează mesaje rapide precum „Releu 1 forțat manual” sau „Setări salvate!”.
3.  **Dynamic Input Blocker:** Dacă utilizatorul editează o valoare în formular, actualizarea automată din polling este suspendată temporar pe input-ul activ (prin verificarea `document.activeElement`) pentru a preveni rescrierea datelor introduse.
4.  **Glow-uri Stări:** Releele pornite au un indicator cu pulsație luminoasă verzuie, oferind feedback vizual instânt.
