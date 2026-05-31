# Context Proiect: Automatizare Acvariu „Aquatlantis Advance LED”

> [!NOTE]
> Acest document reprezintă cerințele inițiale (PRD) de la începutul proiectului.
> Sistemul a fost extins și optimizat ulterior în versiunea 2.0 (vezi [Ghidul Utilizatorului](docs/user_guide.md) și [README.md](README.md)):
> 1. **Relay 4** a fost alocat pentru **Iluminat Ambiental** de 12V DC (și nu mai este liber/auxiliar).
> 2. **Pompa de Aer (Relay 3)** rulează în regim **Continuu pe timpul nopții** în modul normal (pentru a reduce zgomotul și uzura fizică a releelor mecanice). Modul **Pulsatoriu** (1 min ON / 2 min OFF) este rezervat **strict modului de siguranță (Power Loss / Failsafe)** când alimentarea este comutată pe baterie (UPS).

Acesta este un prompt de inițializare pentru un sistem embedded de automatizare a unui acvariu de 56L, bazat pe o placă de dezvoltare cu relee integrate și cip ESP8266 (ESP-12F). Scopul este controlul riguros al iluminatului, monitorizarea rețelei electrice, controlul sincronizat al CO2-ului și oxigenării, plus implementarea unui sistem UPS DIY pe joasă tensiune (Low Voltage Failsafe) pentru supraviețuirea sistemului în caz de pană de curent.

Suntem în faza de proiectare a arhitecturii hardware, definirea diagramelor (electrice, logice/temporale, UI) și scrierea firmware-ului. Nu se folosesc slide-uri; totul trebuie generat în Markdown curat, diagrame textuale (platUML/ASCII) sau cod modular direct aplicabil.

---

## 1. Specificații Hardware și Componente

1. **Unitate Centrală (Creier):** Placă LC-Relay-ESP12-4R-MV echipată cu modul WiFi ESP-12F (ESP8266MOD). Placa are convertor AC-DC integrat (neutilizat pentru periferice externe) și terminale de intrare DC care suportă 7-30V.
2. **Actuator CO2:** Electrovalvă dedicată de acvariu model AV06-1F, alimentată la 12V DC, tip Normal Închis (NC). (Fără servomotoare sau reglaje mecanice complexe).
3. **Iluminat Principal:** Lampă LED integrată în capacul acvariului, alimentată la 12V DC.
4. **Oxigenare:** Pompă de aer de 3W, alimentată la 5V DC.
5. **Sistem Recirculare și Încălzire:** Pompă Biobox (220V AC) și Încălzitor cu termostat propriu (220V AC). Ambele sunt conectate permanent la rețea, ocolind releele de control din motive de siguranță.
6. **Sincronizare Timp:** Fără modul RTC fizic (DS3231 eliminat). Sincronizarea timpului se face exclusiv prin WiFi folosind protocolul NTP (Network Time Protocol), conexiunea fiind stabilă (zona Otopeni).

---

## 2. Arhitectură de Alimentare și Managementul Energiei (Common Rail UPS)

Pentru siguranță în mediu umed și independență față de rețeaua de 220V în zona de control, sistemul folosește o topologie de tip „Common Rail” pe joasă tensiune:
* **Sursă Principală:** Alimentator extern de 12V DC (minim 3A - 5A) conectat la rețeaua de 220V prin intermediul uneia dintre cele 3 prize ocupate în total de sistem (Priza 1: Încălzitor, Priza 2: Recirculare, Priza 3: Sursă 12V).
* **Backup (UPS):** Un pachet de baterii Li-Ion de 12V (configurație 3S, nominal 11.1V, încărcare max 12.6V) echipat cu BMS de protecție integrat.
* **Izolare și Protecție:** O diodă (1N4007 sau Schottky de putere) este montată în serie pe firul de PLUS (+) al sursei principale de 12V pentru a preveni descărcarea bateriei înapoi în alimentatorul de perete sau pe linia de monitorizare în caz de pană de curent. Bateria de backup este legată în paralel după diodă, direct pe alimentarea plăcii ESP.
* **Linia de Detecție (Power Sense):** Un divizor rezistiv (rezistențe de 10kΩ și 2.2kΩ/3.3kΩ) este conectat direct pe linia de 12V **înainte** de dioda de protecție. Acesta coboară tensiunea sub 3.3V pentru a fi citită în siguranță de un pin GPIO, permițând microcontrolerului să detecteze instantaneu căderea rețelei de 220V.
* **Magistrala de 5V (Rail Secundar):** Un convertor mic DC-DC Step-Down (Buck) este conectat la magistrala principală de 12V (după diodă) și reglat la ieșire fix pe 5V pentru a alimenta pompa de aer de 3W și o lumină ambientală secundară.

---

## 3. Mapare Pini și Alocare Relee (Pinout Modul ESP12F-X4)

Conform topologiei PCB-ului utilizat, releele sunt controlate implicit prin jumperi fizici conectați la pinii GPIO ai ESP-12F. Alocarea stabilită este:

* **Relay 1 (GPIO16):** Controlează linia de 12V DC pentru **Iluminatul Principal**. (Notă de debug: GPIO16 dă un impuls HIGH scurt la boot, comportament acceptat momentan).
* **Relay 2 (GPIO14):** Controlează linia de 12V DC pentru **Electrovalva CO2 (NC)**.
* **Relay 3 (GPIO12):** Controlează linia de 5V DC (venită din Buck Converter) pentru **Pompa de Aer (3W)**.
* **Relay 4 (GPIO13):** Liber / Rezervat pentru extensii viitoare.
* **GPIO0:** Alocat ca intrare digitală pentru **Power Sense Line** (citirea stării tensiunii de rețea prin divizorul rezistiv). De asemenea, este folosit pentru punerea cipului în mod Flash la boot.
* **GPIO4 și GPIO2:** Pini complet liberi expuși pe header, rezervați pentru extinderi sau debug UART secundar.
* **GND Comun:** Toate componentele (Sursă, Baterie, ESP, Buck Converter, Electrovalvă) partajează aceeași masă (Ground) pentru referința corectă a semnalelor.

---

## 4. Matrice de Logică și State Machine

Sistemul trebuie să ruleze o mașină de stări bazată pe citirea timpului (NTP) și monitorizarea pinului de `Power Sense`:

### Starea 1: MOD NORMAL - ZI (Power Sense = HIGH, Timp = Interval Zi)
* Iluminat Principal (Relay 1) = **ON**
* Electrovalvă CO2 (Relay 2) = **ON** (Deschisă, gazul circulă)
* Pompă de Aer (Relay 3) = **OFF**

### Starea 2: MOD NORMAL - NOAPTE (Power Sense = HIGH, Timp = Interval Noapte)
* Iluminat Principal (Relay 1) = **OFF**
* Electrovalvă CO2 (Relay 2) = **OFF** (Închisă automat / Failsafe)
* Pompă de Aer (Relay 3) = **PULSE MODE** -> Funcționare intermitentă pentru economie de energie și uzură redusă: **1 minut ON / 2 minute OFF** continuu.

### Starea 3: MOD AVARIE / PANĂ CURENT (Power Sense = LOW, Indiferent de Timp)
* Instalează instantaneu regimul de salvare a bateriei.
* Iluminat Principal (Relay 1) = **OFF** (Forțat)
* Electrovalvă CO2 (Relay 2) = **OFF** (Forțat, valva NC se închide mecanic din lipsă de tensiune, eliminând riscul de asfixiere a faunei).
* Pompă de Aer (Relay 3) = **PULSE MODE** -> Rulează pe profilul de avarie (**1 minut ON / 2 minute OFF**) alimentată din bateria de backup prin convertorul Buck, asigurând supraviețuirea peștilor pe durata penei de curent.

---

## 5. Cerințe Software, Interfață Web și Programare

* **Interfață Web Grafică (UI):** Firmware-ul va găzdui un server web minimalist (utilizând memoria internă a ESP8266, stocare prin LittleFS/SPIFFS sau direct string-uri HTML/CSS/JS în cod). Interfața trebuie să permită:
    1. Afișarea statusului curent al sistemului (Mod de funcționare, Timp NTP, Stare Rețea/Baterie).
    2. Butoane de tip *Override Manual* pentru activarea/dezactivarea individuală a fiecărui releu.
    3. O secțiune simplă de configurare (Setare interval orar Zi/Noapte, modificare timpi de Pulse Mode).
* **Programare (Flashing):** Încărcarea codului se face prin interfața UART (pinii TX, RX, GND de pe placă) utilizând o placă **Arduino Uno** ca punte USB-to-TTL (cu procesorul ATmega328P pus la somn prin legarea pinului RESET la GND). Pinul **GPIO0** trebuie tras la GND în momentul pornirii pentru a activa modul de scriere (Download Mode).

---

## 6. Obiective Curente și Livrabile Solicitate în Workspace

Deoarece ne mutăm activitatea în noul depozit și mediu de dezvoltare, avem nevoie de următoarele livrabile structurate curat, ușor de corectat și modularizat:

1. **Diagrama Electrică (Format Text/plantUML sau ASCII):** Schița completă a conexiunilor pentru "Common Rail", amplasarea diodei, a divizorului pe GPIO0, conexiunea bateriei și a releelor.
2. **Diagrama Logică / Temporală (plantUML):** Reprezentarea tranzițiilor de stare (Zi -> Noapte -> Pană de curent) și detalierea logicii de Pulse Mode pentru pompa de aer.
3. **Plan de Design UI (Wireframe în Markdown):** Structura paginii web gazduite de ESP (Elemente HTML, stilizare CSS minimalistă integrată).
4. **Firmware v1.0 (Cod Sursă C++ pentru Arduino IDE / PlatformIO):** Cod curat, modularizat, cu comentarii tehnice riguroase, implementând conexiunea WiFi, sincronizarea NTP, citirea pinului de Power Sense și controlul stărilor de releu conform matricei stabilite.

##  7. Documentatie externa

https://devices.esphome.io/devices/esp-12f-relay-x4/
https://templates.blakadder.com/assets/ESP12F_Relay_X4.pdf
https://ayatec.eu/introducing-the-esp12f-x4-relay-module/
https://templates.blakadder.com/ESP12F_Relay_X4.html
https://unicontrol.ayatec.eu/userguide/?page=web_interface
https://github.com/AthenasArch/ESP12F_Relay_X4
