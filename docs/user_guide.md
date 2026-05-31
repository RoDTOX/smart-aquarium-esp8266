# Ghid de Utilizare și Configurare: Aquatlantis Smart Aquarium Controller

Acest ghid detaliază configurarea fizică și software a controlerului pentru acvariu bazat pe cipul ESP8266 (placa cu 4 relee LC-Relay-ESP12-4R-MV). Documentul este structurat astfel încât să poată fi înțeles cu ușurință de către utilizator, dar și de către un viitor asistent AI care va fi însărcinat să extindă codul.

---

## 1. Conectare și WiFi Local

Placa ESP-12F rulează în mod hibrid **AP + STA** (Access Point + Station). Aceasta înseamnă că se va conecta la routerul tău local (dacă este disponibil), dar va emite și o rețea proprie pentru configurare de urgență.

### Conexiunea normală (STA)
La prima pornire, controlerul va încerca să se conecteze la rețeaua configurată implicit în [Config.h](../include/Config.h):
*   **SSID implicit:** Definit în `secrets.h` ca `DEFAULT_WIFI_SSID`
*   **Parolă implicită:** Definită în `secrets.h` ca `DEFAULT_WIFI_PASS`

Dacă se conectează cu succes, va obține un IP local de la router (ex. `192.168.1.32`). Îl poți accesa din browserul oricărui dispozitiv din aceeași rețea prin:
*   **Adresă Web (mDNS):** **`http://acvariu.local/`**
*   **Adresă IP:** `http://192.168.1.[IP_ACVARIU]`

### Modul Access Point (AP) de siguranță
Dacă rețeaua WiFi de acasă devine indisponibilă sau datele de autentificare s-au schimbat:
1.  Placa va emite propria rețea WiFi numită: **`BioBox-Aquarium`**
2.  Parola de conectare la AP: Definită în `secrets.h` ca `AP_PASS`
3.  După ce te conectezi cu telefonul/laptopul la această rețea, accesează în browser adresa IP: **`http://192.168.4.1/`**
4.  Derulează până la secțiunea **Configurare WiFi**, introdu noul SSID și parola, apoi apasă pe **Conectează Dispozitiv**. Placa se va conecta automat la noul router.

---

## 2. Acces de la Distanță prin Tailscale

Dacă vrei să accesezi dashboard-ul acvariului din afara rețelei de acasă (ex. prin internet mobil), poți ruta traficul prin dispozitivul Linux/Android de pe rețeaua locală care are Tailscale instalat (IP Tailscale: `100.84.5.4`, IP Local: `192.168.1.X`).

Există două metode principale prin care poți realiza acest lucru:

### Metoda A: Subnet Router (Recomandată)
Această metodă expune întreaga rețea locală `192.168.1.0/24` în rețeaua ta VPN Tailscale, permițându-ți să tastezi direct IP-ul local al acvariului de oriunde.

1.  **Activează IP Forwarding** pe dispozitivul Linux (telefonul Android cu Linux/Tailscale):
    ```bash
    echo 'net.ipv4.ip_forward = 1' | sudo tee -a /etc/sysctl.conf
    sudo sysctl -p
    ```
2.  **Pornește Tailscale cu reclamă de rute:**
    ```bash
    sudo tailscale up --advertise-routes=192.168.1.0/24
    ```
3.  **Aprobă rutele în consola Tailscale:**
    *   Intră pe [Tailscale Admin Console](https://login.tailscale.com/admin/machines).
    *   Identifică dispozitivul tău Linux (`100.84.5.4`).
    *   Apasă pe cele trei puncte din dreapta -> **Edit route settings**.
    *   Bifează subrețeaua `192.168.1.0/24` și salvează.
4.  **Acces:** De acum, din orice dispozitiv conectat la contul tău Tailscale, poți accesa acvariul direct la adresa lui IP locală (de ex. `http://192.168.1.32/`).

### Metoda B: Reverse Proxy prin Nginx
Dacă nu vrei să expui toată rețeaua de acasă, poți configura un server Nginx pe telefonul Linux (`100.84.5.4`) care să asculte pe un port specific (de ex. `8080`) și să trimită pachetele către acvariu.

1.  Instalează Nginx pe telefonul Linux:
    ```bash
    sudo apt update && sudo apt install nginx -y
    ```
2.  Creează un fișier de configurare în `/etc/nginx/sites-available/aquarium`:
    ```nginx
    server {
        listen 8080;
        server_name 100.84.5.4;

        location / {
            proxy_pass http://192.168.1.32/; # Înlocuiește cu IP-ul real al acvariului
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            
            # Dezactivează buffering-ul pentru răspunsuri rapide ale butoanelor
            proxy_buffering off;
        }
    }
    ```
3.  Activează configurația și restartează Nginx:
    ```bash
    sudo ln -s /etc/nginx/sites-available/aquarium /etc/nginx/sites-enabled/
    sudo systemctl restart nginx
    ```
4.  **Acces:** Deschide browserul pe orice dispozitiv din Tailscale și accesează: **`http://100.84.5.4:8080/`**.

---

## 3. Panoul Web (Dashboard) & Logica Orarelor

Interfața web este proiectată cu un stil modern Dark-Mode Glassmorphism și este 100% responsivă (optimizată special pentru ecranele telefoanelor).

### Moduri de Control pentru Relee
Fiecare releu (Iluminat, CO2, Aer, Aux) are două moduri de funcționare:
*   **Auto:** Starea releului este determinată strict de orarul configurat (24h bitmap).
*   **Manual:** Utilizatorul poate forța starea releului ON/OFF folosind butoanele **Forțează Manual**. Starea manuală persistă până când se apasă pe butonul **Revenire la Auto** (sau "Clear Overrides").

### Logica Orarelor (Bitmap-ul de 24 de ore)
În loc să folosim structuri complexe cu ore și minute de start/stop, orarele releelor sunt stocate sub formă de **bitmaps pe 24 de biți** (fiecare bit corespunde unei ore din zi, de la bitul 0 = `00:00-00:59` la bitul 23 = `23:00-23:59`).
*   Dacă bitul aferent orei curente este `1`, releul este considerat **Activ**.
*   Dacă bitul este `0`, releul este considerat **Inactiv**.
*   **Exemplu:** Orarul implicit al luminii (13:00 - 21:00) înseamnă biții 13-20 setați pe `1` (valoare hexa: `0x1FE000`).

### Persistență pe LittleFS
Toate orarele configurate și stările modificate sunt salvate automat în sistemul de fișiere intern al cipului, în fișierul binar **`/schedules.cfg`**.
*   **Avantaj:** Salvarea nu folosește EEPROM-ul clasic volatil, ci LittleFS. Setările supraviețuiesc penelor de curent și scrierilor ulterioare de firmware (firmware flash prin USB sau OTA).

### Preseturi din Fabrică
Pentru o configurare rapidă, există 3 preseturi pre-configurate în [RelayControl.cpp](../src/RelayControl.cpp):
1.  **Preset 1: Standard** (Lumină: 13-21, CO2: 11-19 cu siestă 15-16, Pompă Aer: 21-11 continuu, Lumină Ambientală: 07-13 și 21-00).
2.  **Preset 2: Control Alge** (Lumină: 13-19, CO2: 12-18 cu siestă 15-16, Pompă Aer: 20-12 continuu, Lumină Ambientală: 20-23).
3.  **Preset 3: Doar Filtrare/Aerare** (Lumină & CO2 oprite permanent, Pompă Aer pornită 24/24 continuu).

---

## 4. Actualizări Wireless (OTA Update)

Dacă modifici codul firmware, nu este nevoie să conectezi placa fizic la calculator prin adaptorul serial de fiecare dată. Poți face upload-ul direct prin WiFi:

1.  Compilează codul în PlatformIO pentru a obține fișierul `.bin`:
    *   În VS Code, deschide panoul PlatformIO -> **Build** (sau rulează `pio run` în terminal).
    *   Fișierul compilat se va genera la locația: `.pio/build/esp12e/firmware.bin`.
2.  Accesează în browser portalul de update al acvariului: **`http://acvariu.local/update`** (sau `http://[IP]/update`).
3.  Introdu datele de autentificare de securitate (dacă sunt cerute, altfel selectează fișierul direct):
    *   Selectează fișierul `firmware.bin` local de pe disc.
    *   Apasă pe **Update** și așteaptă finalizarea procesului (placa se va reseta automat după scriere).

---

## 5. Diagnosticare și Monitorizare Serială

În cazul în care interfața web nu răspunde, poți folosi diagnosticarea fizică:

### Semnalele Onboard LED (GPIO5 - Active LOW)
LED-ul albastru de pe placa ESP-12F indică starea rețelei prin tipare de clipire:
*   **Clipire Foarte Rapidă (la 100ms):** Placa încearcă să se conecteze la routerul WiFi (STA).
*   **Clipire Medie (la 500ms):** Placa s-a conectat la WiFi, dar așteaptă sincronizarea timpului cu serverul NTP.
*   **Heartbeat scurt (100ms pornit la fiecare 3 secunde):** Funcționare normală. WiFi conectat și ora NTP sincronizată.

### Mesaje în Serial Monitor (115200 baud)
Dacă conectezi placa serial prin USB (pinii TX, RX, GND) și rulezi `pio device monitor --baud 115200`, vei vedea că la fiecare 5 secunde se printează un log complet cu starea curentă a sistemului:
```text
[TELEMETRY] Time: 2026-05-30 01:25:00 | Uptime: 720s | Mode: DAY
  WiFi Status: Connected | SSID: Your_WiFi_SSID | RSSI: -54 dBm | IP: 192.168.1.32
  Inputs: GPIO0=HIGH | GPIO4=HIGH | GPIO2=HIGH | GPIO15=LOW
  Relays Status:
    Relay 1 [Iluminat]: State=ON | Mode=AUTO | Behavior=CONTINUOUS | Sched=[8-19]
    Relay 2 [CO2 Solenoid]: State=ON | Mode=AUTO | Behavior=CONTINUOUS | Sched=[7-18]
    Relay 3 [Air Pump]: State=OFF | Mode=AUTO | Behavior=PULSE (60s/120s) | Sched=[0-7,20-23]
    Relay 4 [Aux]: State=OFF | Mode=AUTO | Behavior=CONTINUOUS | Sched=[]
```
*   **Notă specială pentru GPIO0 (Power Sense):** Dacă citește `LOW`, sistemul consideră că alimentarea de la rețea a picat, forțând instant oprirea luminii (Relay 1) și a CO2-ului (Relay 2), menținând doar pompa de aer pe regim intermitent (pulsuri) din bateria de backup.

---

## 6. Ghid de Handover pentru AI (Predare Proiect)

> [!NOTE]
> Această secțiune este redactată special pentru asistenții AI care vor deschide acest proiect în viitor. Dacă ești un AI, citește cu atenție regulile de mai jos înainte de a face orice modificare!

### Structura Codului
*   [main.cpp](../src/main.cpp): Serverul web HTTP port 80, mașina de stări principală, API-ul REST sub formă de JSON, inițializarea mDNS și a serverului OTA HTTP Update.
*   [NetworkSync.cpp](../src/NetworkSync.cpp): Conexiunea WiFi locală (STA), Access Point de fallback (`AP_SSID`/`AP_PASS`), sincronizarea timpului prin standardul C `configTime()` cu timezone-ul local al României și managementul tiparului de blink al LED-ului de status.
*   [RelayControl.cpp](../src/RelayControl.cpp): Managementul orarelor pe LittleFS, scrierea fișierului `/schedules.cfg` și scrierea istoricului de evenimente în `/log.txt` (cu funcție de auto-rotire a logului la dimensiuni mai mari de 3KB pentru a preveni umplerea flash-ului).
*   [Config.h](../include/Config.h): Configurațiile statice ale rețelelor, pinout-ul plăcii ESP8266, definirea orarelor implicite și a timpilor de pulse mode.

### Reguli Importante la Modificarea Codului
1.  **Fără utilizare de EEPROM direct:** Toată persistența datelor trebuie să treacă prin sistemul de fișiere `LittleFS` (folosind `/schedules.cfg`). Nu folosi biblioteca `<EEPROM.h>`.
2.  **Menținerea formatului JSON REST:** Pagina web face interogări asincrone la fiecare 2 secunde pe `GET /api/status`. Orice modificare adusă variabilelor globale trebuie reflectată corespunzător în JSON-ul generat în `main.cpp`.
3.  **Ordinea elementelor din UI:** Design-ul este responsive și optimizat pentru telefoane. Nu schimba ordinea cardurilor din DOM: Status -> Inputs -> Scheduler -> Relays -> WiFi -> Timeline (poziționat separat jos).
4.  **Uptime & Timp:** Folosește API-ul `time.h` standard și funcțiile din [NetworkSync.h](../include/NetworkSync.h) (`getFormattedTime()`, `getCurrentHour()`) pentru a menține logică sincronă.

---

## 7. Întrebări Frecvente (Q&A) & Depanare (Troubleshooting)

Această secțiune oferă răspunsuri și soluții pentru cele mai comune situații apărute în utilizarea cotidiană sau depanarea controlerului de acvariu.

### Q1: Am schimbat orarele implicite în cod (în `Config.h` sau presets), dar placa folosește tot vechile setări după flash-uire. De ce?
*   **Cauză:** Sistemul folosește un mecanism de persistență pe LittleFS. La prima pornire, placa a salvat orarele în fișierul binar `/schedules.cfg`. Ulterior, chiar dacă rescrii firmware-ul, placa detectează fișierul existent și îl încarcă pe acesta, ignorând valorile `DEFAULT_SCHED_*` hardcodate.
*   **Soluție:** Accesează interfața Web și apasă pe butonul **„Resetare Fabrică”** (Factory Reset) sau pe **„Aplică Preset”** (Preset 1). Această acțiune va șterge/suprascrie fișierul de configurare de pe LittleFS, forțând placa să încarce setările din cod.

### Q2: De ce nu pot accesa adresa `http://acvariu.local` de pe telefonul meu cu Android?
*   **Cauză:** Sistemul de operare Android nu are suport nativ implicit pentru protocolul mDNS (Multicast DNS) în Chrome sau alte browsere standard, spre deosebire de iOS, macOS și Windows.
*   **Soluție:** Poți accesa controlerul folosind adresa IP locală directă a plăcii (de exemplu, `http://192.168.1.32/`). Poți afla această adresă din panoul de control al routerului tău sau verificând Serial Monitor la bootare. Alternativ, poți folosi o aplicație gratuită de rețea (cum ar fi *Service Browser* sau *Fing*) pentru a scana serviciile `_http._tcp` din rețea.

### Q3: Cum funcționează modul de siguranță la pana de curent (Failsafe UPS / Power Loss)?
*   **Cauză:** Monitorizarea tensiunii de la rețea se face prin pinul `GPIO0` (`PIN_INPUT_GPIO0`), conectat la senzorul UPS-ului. Când curentul cade, pinul trece în starea `LOW`.
*   **Comportament Failsafe:** Placa intră instantaneu în starea de urgență `MODE_POWER_LOSS`:
    *   **Lumina Principală (Releul 1), CO2 (Releul 2) și Lumina Ambientală (Releul 4)** sunt forțate în starea **OPRIT (OFF)**. Oprirea electrovalvei de CO2 este critică: pe lângă faptul că plantele nu pot face fotosinteză la întuneric, solenoidul consumă în mod normal 100-300mA când este pornit. Dezactivarea sa salvează masiv bateria UPS-ului.
    *   **Pompa de Aer (Releul 3)** trece în **Modul Pulsatoriu** (1 minut pornit / 2 minute oprit). Acest ciclu intermitent asigură o oxigenare suficientă a apei pentru pești și bacteriile din filtru, în timp ce reduce consumul de energie al pompei cu 66%, prelungind considerabil durata de viață a bateriei.
    *   **Blocare Comenzi:** Orice comandă manuală din interfața Web este dezactivată până la revenirea curentului de la rețea (când `GPIO0` redevine `HIGH`).

### Q4: Cum pot schimba rețeaua WiFi sau parola dacă routerul meu s-a schimbat?
*   **Cauză:** Placa nu se mai poate conecta la vechiul router și rămâne blocată la bootare în timp ce încearcă conectarea.
*   **Soluție:** 
    1. Dacă placa nu se poate conecta la router în decurs de 30 de secunde, ea va intra automat în modul **Access Point de siguranță**.
    2. Caută pe telefon sau laptop rețeaua WiFi emisă de placă, numită **`BioBox-Aquarium`**, și conectează-te folosind parola definită în `secrets.h` (`AP_PASS`).
    3. Deschide browserul și accesează adresa **`http://192.168.4.1/`**.
    4. Navighează la secțiunea **Configurare WiFi**, introdu noul SSID și parola corespunzătoare, apoi apasă pe **Conectează Dispozitiv**. Placa se va restarta și se va conecta la noua rețea WiFi locală.

### Q5: Ora afișată pe interfața Web este incorectă sau LED-ul albastru de pe placă clipește constant la 500ms.
*   **Cauză:** Placa clipește rapid la 500ms când este conectată la rețeaua WiFi, dar nu a putut încă să sincronizeze ora prin serverul NTP (protocolul de timp prin Internet).
*   **Soluție:** Asigură-te că routerul tău este conectat la internet. Placa utilizează servere NTP românești (`ro.pool.ntp.org`). Imediat ce serviciul de internet redevine activ, sincronizarea se va face automat în fundal, LED-ul va reveni la tiparul de funcționare normală (un scurt „heartbeat” la fiecare 3 secunde), iar ora se va actualiza corespunzător cu regulile locale de fus orar (inclusiv ora de vară/iarnă).

### Q6: De ce pompa de aer nu rulează în mod pulsatoriu pe timpul nopții?
*   **Cauză:** Releele mecanice au o durată de viață limitată de numărul de cicluri de comutare. Dacă pompa ar fi controlată în mod pulsatoriu în fiecare noapte (20:00 - 08:00) o dată la 1-2 minute, releul s-ar comuta de sute de ori pe noapte. Aceasta ar duce la un zgomot constant extrem de deranjant pe timp de noapte și ar uza complet contactele fizice ale releului în mai puțin de 5 luni.
*   **Soluție:** Modul pulsatoriu este pornit **strict în starea de avarie (Power Loss)** pentru supraviețuirea acvariului pe baterie. În mod normal, pe timp de noapte, pompa de aer rulează în regim continuu. Astfel, releul comută doar de 2 ori pe zi (pornește seara, se oprește dimineața), mărindu-i durata de viață la zeci de ani.

### Q7: Există riscul ca memoria flash a plăcii ESP8266 să se uzeze rapid din cauza logării?
*   **Cauză:** Placa rulează 24/7/365, iar scrierile frecvente pe flash pot deteriora celulele de memorie (LittleFS).
*   **Soluție:** Firmware-ul a fost optimizat special împotriva scrierilor inutile:
    *   Schedules (`/schedules.cfg`) se scriu pe disc doar în momentul modificării explicite a unui orar sau a forțării manuale din UI.
    *   Istoricul de evenimente (`/log.txt`) are o funcție de rotație automată limitată la **3 KB**. Când se depășește această limită, fișierul este trunchiat și rescris de la zero.
    *   Nu se rulează scrieri automate sau periodice de telemetrie, starea senzorilor fiind menținută doar în memoria RAM. Flash-ul este protejat la maximum și poate funcționa mulți ani fără probleme de degradare.
