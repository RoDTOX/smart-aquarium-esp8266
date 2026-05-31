#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "Config.h"
#include "NetworkSync.h"
#include "RelayControl.h"

// Initialize the web server on port 80
ESP8266WebServer server(80);

// Initialize the OTA update server
ESP8266HTTPUpdateServer httpUpdater;

// Current system-wide operational mode
SystemMode currentSystemMode = MODE_DAY;

// Debounced power state for PIN_INPUT_GPIO0 (Power Sense)
static int debouncedPowerState = HIGH;

// Expanded Dashboard HTML in FLASH memory (PROGMEM)
const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="ro">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aquatlantis | Smart Aquarium</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;800&display=swap" rel="stylesheet">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg-base: #0b0f19;
            --bg-surface: #151d30;
            --bg-card: #1e2942;
            --text-primary: #f8fafc;
            --text-secondary: #94a3b8;
            --accent-primary: #38bdf8;
            --accent-primary-glow: rgba(56, 189, 248, 0.3);
            --state-ok: #10b981;
            --state-ok-glow: rgba(16, 185, 129, 0.3);
            --state-warn: #f59e0b;
            --state-warn-glow: rgba(245, 158, 11, 0.3);
            --state-danger: #ef4444;
            --state-danger-glow: rgba(239, 68, 68, 0.3);
            --border-color: rgba(255, 255, 255, 0.08);
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Outfit', sans-serif;
            background: radial-gradient(circle at 50% 0%, #1e2942 0%, var(--bg-base) 100%);
            color: var(--text-primary);
            padding: 24px 16px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }
        
        .container {
            max-width: 1000px;
            width: 100%;
            display: flex;
            flex-direction: column;
            gap: 24px;
        }
        
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: linear-gradient(135deg, var(--bg-surface), var(--bg-card));
            padding: 20px 24px;
            border-radius: 16px;
            border: 1px solid var(--border-color);
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
            backdrop-filter: blur(12px);
            transition: box-shadow 0.2s;
        }
        
        header:hover {
            box-shadow: 0 6px 24px rgba(56, 189, 248, 0.08);
        }
        
        h1 {
            font-size: 24px;
            font-weight: 800;
            background: linear-gradient(to right, #38bdf8, #818cf8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .sys-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 14px;
            font-weight: 600;
            padding: 6px 12px;
            border-radius: 9999px;
            background: rgba(255,255,255,0.05);
            border: 1px solid var(--border-color);
        }
        
        .status-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            display: inline-block;
        }
        
        @keyframes pulse-green {
            0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.4); }
            70% { box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
            100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
        }
        @keyframes pulse-orange {
            0% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0.4); }
            70% { box-shadow: 0 0 0 6px rgba(245, 158, 11, 0); }
            100% { box-shadow: 0 0 0 0 rgba(245, 158, 11, 0); }
        }
        @keyframes pulse-red {
            0% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.4); }
            70% { box-shadow: 0 0 0 6px rgba(239, 68, 68, 0); }
            100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
        }
        
        .dot-green { background-color: var(--state-ok); animation: pulse-green 2s infinite; }
        .dot-orange { background-color: var(--state-warn); animation: pulse-orange 2s infinite; }
        .dot-red { background-color: var(--state-danger); animation: pulse-red 2s infinite; }
        
        .main-layout {
            display: flex;
            flex-direction: row;
            gap: 20px;
            width: 100%;
            align-items: flex-start;
        }
        
        .layout-column {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        
        .column-left {
            flex: 1.6; /* 60% width */
            min-width: 0;
        }
        
        .column-right {
            flex: 1.1; /* 40% width */
            min-width: 0;
        }
        
        @media(max-width: 850px) {
            .main-layout {
                flex-direction: column;
            }
            .column-left, .column-right {
                flex: none;
                width: 100%;
            }
        }
        
        .card {
            background: linear-gradient(135deg, var(--bg-surface), var(--bg-card));
            border-radius: 16px;
            border: 1px solid var(--border-color);
            padding: 20px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
            display: flex;
            flex-direction: column;
            gap: 16px;
            backdrop-filter: blur(12px);
            transition: box-shadow 0.2s, transform 0.2s;
        }
        
        .card:hover {
            box-shadow: 0 6px 24px rgba(56, 189, 248, 0.08);
        }
        
        .card-title {
            font-size: 18px;
            font-weight: 600;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 10px;
            color: var(--accent-primary);
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        
        .stat-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 0;
            border-bottom: 1px solid rgba(255,255,255,0.03);
        }
        
        .stat-label {
            color: var(--text-secondary);
            font-size: 14px;
        }
        
        .stat-val {
            font-weight: 600;
            font-size: 14px;
        }
        
        .relay-item {
            background: var(--bg-card);
            border-radius: 10px;
            padding: 8px 10px;
            border: 1px solid var(--border-color);
            display: flex;
            flex-direction: column;
            gap: 6px;
        }
        
        .relay-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .relay-name {
            font-weight: 600;
            font-size: 15px;
        }
        
        .relay-meta {
            display: flex;
            align-items: center;
            gap: 6px;
            font-size: 12px;
            color: var(--text-secondary);
        }
        
        .relay-controls {
            display: flex;
            gap: 8px;
            align-items: center;
            margin-top: 2px;
        }
        
        .btn {
            background: linear-gradient(135deg, #3b82f6, #1d4ed8);
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 8px;
            font-family: inherit;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            font-size: 14px;
        }
        
        .btn:hover {
            opacity: 0.9;
            box-shadow: 0 0 10px rgba(59, 130, 246, 0.4);
            transform: translateY(-1px);
        }
        
        .btn:active {
            transform: translateY(1px);
        }
        
        .btn-secondary {
            background: rgba(255,255,255,0.08);
            color: var(--text-primary);
            border: 1px solid var(--border-color);
        }
        
        .btn-secondary:hover {
            background: rgba(255,255,255,0.15);
            box-shadow: none;
            transform: translateY(-1px);
        }
        
        .btn-danger {
            background: linear-gradient(135deg, #ef4444, #b91c1c);
        }
        
        .btn-danger:hover {
            box-shadow: 0 0 10px rgba(239, 68, 68, 0.4);
            transform: translateY(-1px);
        }
        
        .btn-sm {
            padding: 6px 12px;
            font-size: 12px;
            border-radius: 6px;
        }
        
        .compact-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
            gap: 10px;
        }
        
        .stat-block {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid var(--border-color);
            padding: 8px 10px;
            border-radius: 8px;
            display: flex;
            flex-direction: column;
            gap: 2px;
            min-width: 0;
        }
        
        .stat-block .stat-label {
            font-size: 10px;
            color: var(--text-secondary);
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }
        
        .stat-block .stat-val {
            font-size: 13px;
            font-weight: 600;
            color: var(--text-primary);
        }
        
        .form-group {
            display: flex;
            flex-direction: column;
            gap: 6px;
        }
        
        label {
            font-size: 11px;
            color: var(--text-secondary);
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        input[type="number"], input[type="text"], input[type="password"], select {
            background-color: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 10px;
            border-radius: 8px;
            color: white;
            font-family: inherit;
            font-size: 14px;
            outline: none;
            transition: border-color 0.2s;
            width: 100%;
        }
        
        select option {
            background-color: var(--bg-card);
            color: white;
        }
        
        input:focus, select:focus {
            border-color: var(--accent-primary);
        }
        
        .form-row {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
        }
        
        @media(max-width: 480px) {
            .form-row {
                grid-template-columns: 1fr;
            }
        }
        
        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 48px;
            height: 24px;
        }
        
        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(255,255,255,0.15);
            transition: .3s;
            border-radius: 24px;
        }
        
        .slider:before {
            position: absolute;
            content: "";
            height: 18px;
            width: 18px;
            left: 3px;
            bottom: 3px;
            background-color: white;
            transition: .3s;
            border-radius: 50%;
        }
        
        input:checked + .slider {
            background-color: var(--state-ok);
            box-shadow: 0 0 8px var(--state-ok-glow);
        }
        
        input:checked + .slider:before {
            transform: translateX(24px);
        }
        
        .toast {
            position: fixed;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%) translateY(100px);
            background-color: var(--bg-card);
            border: 1px solid var(--accent-primary);
            padding: 12px 24px;
            border-radius: 8px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.5);
            transition: transform 0.3s ease;
            z-index: 1000;
            font-weight: 600;
        }
        
        .toast.show {
            transform: translateX(-50%) translateY(0);
        }
        
        .badge {
            font-size: 11px;
            padding: 3px 8px;
            border-radius: 4px;
            font-weight: bold;
            text-transform: uppercase;
        }
        
        .badge-auto { background: rgba(56, 189, 248, 0.15); color: var(--accent-primary); border: 1px solid rgba(56, 189, 248, 0.3); }
        .badge-manual { background: rgba(245, 158, 11, 0.15); color: var(--state-warn); border: 1px solid rgba(245, 158, 11, 0.3); }
        
        /* Timeline logger styles */
        .timeline-container {
            max-height: 250px;
            overflow-y: auto;
            display: flex;
            flex-direction: column;
            gap: 10px;
            padding-right: 4px;
        }
        
        .timeline-container::-webkit-scrollbar {
            width: 6px;
        }
        
        .timeline-container::-webkit-scrollbar-track {
            background: rgba(255, 255, 255, 0.01);
            border-radius: 4px;
        }
        
        .timeline-container::-webkit-scrollbar-thumb {
            background: var(--border-color);
            border-radius: 4px;
            transition: background 0.2s;
        }
        
        .timeline-container::-webkit-scrollbar-thumb:hover {
            background: var(--accent-primary);
        }
        
        .timeline-item {
            border-left: 2px solid var(--accent-primary);
            padding-left: 12px;
            position: relative;
        }
        
        .timeline-item::before {
            content: '';
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: var(--accent-primary);
            position: absolute;
            left: -5px;
            top: 6px;
        }
        
        .timeline-time {
            font-size: 11px;
            color: var(--text-secondary);
            font-weight: 600;
        }
        
        .timeline-msg {
            font-size: 13px;
            margin-top: 2px;
        }
        
        /* Hour Grid Scheduler styles */
        .sched-relay-selector {
            background-color: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 10px;
            border-radius: 8px;
            color: white;
            font-family: inherit;
            font-weight: 600;
            width: auto;
        }
        
        .hour-grid {
            display: grid;
            grid-template-columns: repeat(12, 1fr);
            gap: 6px;
            margin-top: 10px;
        }
        
        @media(max-width: 480px) {
            .hour-grid {
                grid-template-columns: repeat(6, 1fr);
            }
        }
        
        .hour-cell {
            background-color: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 8px 4px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
            text-align: center;
            cursor: pointer;
            transition: all 0.2s;
            user-select: none;
        }
        
        .hour-cell:hover {
            border-color: var(--accent-primary);
        }
        
        .hour-cell.active {
            background-color: var(--accent-primary);
            color: var(--bg-base);
            box-shadow: 0 0 8px var(--accent-primary-glow);
            border-color: var(--accent-primary);
        }

        /* Modal Configurare WiFi */
        .modal {
            display: none;
            position: fixed;
            top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(11, 15, 25, 0.8);
            backdrop-filter: blur(8px);
            z-index: 2000;
            justify-content: center;
            align-items: center;
            padding: 16px;
        }
        .modal.show {
            display: flex;
        }
        .modal-content {
            background: linear-gradient(135deg, var(--bg-surface), var(--bg-card));
            border-radius: 16px;
            border: 1px solid var(--border-color);
            padding: 24px;
            max-width: 400px;
            width: 100%;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            display: flex;
            flex-direction: column;
            gap: 16px;
            position: relative;
            animation: modalEnter 0.25s ease-out;
        }
        @keyframes modalEnter {
            from { transform: scale(0.95); opacity: 0; }
            to { transform: scale(1); opacity: 1; }
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 10px;
            margin-bottom: 4px;
        }
        .modal-header h2 {
            font-size: 18px;
            font-weight: 800;
            background: linear-gradient(to right, #38bdf8, #818cf8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .close-btn {
            font-size: 24px;
            color: var(--text-secondary);
            cursor: pointer;
            transition: color 0.2s;
            line-height: 1;
        }
        .close-btn:hover {
            color: var(--state-danger);
        }
        
        footer {
            text-align: center;
            font-size: 12px;
            color: var(--text-secondary);
            margin-top: auto;
            padding: 20px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <div>
                <h1>Aquatlantis</h1>
                <p style="font-size: 12px; color: var(--text-secondary)">Smart Aquarium Controller | BioBox 56L</p>
            </div>
            <div class="sys-badge" id="wifi-badge">
                <span class="status-dot dot-green" id="wifi-dot"></span>
                <span id="wifi-status-text">Conectat</span>
            </div>
        </header>

        <div class="main-layout">
            <!-- Left Column (Wider): Stare Sistem, Intrări Digitale, Planificator -->
            <div class="layout-column column-left">
                <!-- Stare Sistem -->
                <div class="card">
                    <div class="card-title">
                        Stare Sistem
                        <span class="status-dot dot-green" id="system-dot"></span>
                    </div>
                    <div class="compact-grid">
                        <div class="stat-block">
                            <span class="stat-label">Mod Operare</span>
                            <span class="stat-val" id="mode-val">MOD NORMAL - ZI</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">Ceas (NTP Sync)</span>
                            <span class="stat-val" id="time-val">00:00:00</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">Uptime</span>
                            <span class="stat-val" id="uptime-val">0s</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">WiFi SSID</span>
                            <span class="stat-val" id="ssid-val">-</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">Semnal RSSI</span>
                            <span class="stat-val" id="rssi-val">-99 dBm</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">Adresă IP</span>
                            <span class="stat-val" id="ip-val">192.168.1.32</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">Senzor Lumină</span>
                            <span class="stat-val" id="light-val">0%</span>
                        </div>
                    </div>
                </div>

                <!-- Intrări Digitale (Senzori) -->
                <div class="card">
                    <div class="card-title">Intrări Digitale (Senzori)</div>
                    <div class="compact-grid">
                        <div class="stat-block">
                            <span class="stat-label">GPIO 0 (Senzor Alimentare)</span>
                            <span class="stat-val" id="input-gpio0"><span class="status-dot dot-orange"></span> Se încarcă...</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">GPIO 4 (Liber)</span>
                            <span class="stat-val" id="input-gpio4"><span class="status-dot dot-orange"></span> Se încarcă...</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">GPIO 2 (Liber)</span>
                            <span class="stat-val" id="input-gpio2"><span class="status-dot dot-orange"></span> Se încarcă...</span>
                        </div>
                        <div class="stat-block">
                            <span class="stat-label">GPIO 15 (Liber)</span>
                            <span class="stat-val" id="input-gpio15"><span class="status-dot dot-orange"></span> Se încarcă...</span>
                        </div>
                    </div>
                </div>

                <!-- Planificator 24H (Scheduler) -->
                <div class="card">
                    <div class="card-title">
                        Configurare Orar Avansat (24h)
                        <select id="relay-select" class="sched-relay-selector" onchange="onRelaySelected(this.value)">
                            <option value="1">Releu 1: Iluminat Principal</option>
                            <option value="2">Releu 2: Electrovalvă CO2</option>
                            <option value="3">Releu 3: Pompă de Aer</option>
                            <option value="4">Releu 4: Iluminat Ambiental</option>
                        </select>
                    </div>
                    
                    <div style="display: flex; flex-direction: column; gap: 12px;">
                        <label>Alege orele active de funcționare (00-23):</label>
                        <div class="hour-grid" id="hour-grid">
                            <!-- Hour cells will be inserted here dynamically -->
                        </div>
                    </div>

                    <div class="form-row" style="margin-top: 8px;">
                        <div class="form-group">
                            <label for="relay-behavior">Comportament Activ</label>
                            <select id="relay-behavior" onchange="onBehaviorChanged(this.value)">
                                <option value="0">Continuu (Întotdeauna Pornit)</option>
                                <option value="1">Intermitent (Pulse Mode)</option>
                            </select>
                        </div>
                        <div class="form-row" id="pulse-settings-row">
                            <div class="form-group">
                                <label for="relay-pulse-on">Pompă ON (secunde)</label>
                                <input type="number" id="relay-pulse-on" min="1" max="3600" value="60">
                            </div>
                            <div class="form-group">
                                <label for="relay-pulse-off">Pompă OFF (secunde)</label>
                                <input type="number" id="relay-pulse-off" min="1" max="3600" value="120">
                            </div>
                        </div>
                    </div>
                    
                    <button class="btn" onclick="saveRelaySchedule()">Salvează Programul Releului</button>
                    
                    <!-- Preseturi & Administrare -->
                    <div style="border-top: 1px solid var(--border-color); padding-top: 16px; margin-top: 16px; display: flex; flex-direction: column; gap: 12px;">
                        <label>Preseturi & Administrare Programe</label>
                        <div style="display: flex; gap: 12px; align-items: center; flex-wrap: wrap;">
                            <select id="preset-select" style="flex: 1; min-width: 200px;">
                                <option value="1">Preset 1: Standard Aquatlantis (Lumină, CO2, Ambient, Aer Nocturn)</option>
                                <option value="2">Preset 2: Control Alge (Lumină Redusă, CO2 Redus, Aer Extins)</option>
                                <option value="3">Preset 3: Mentenanță/Tratament (Fără Lumini, Aer permanent 24h)</option>
                            </select>
                            <button class="btn btn-sm" onclick="applyPreset()">Aplică Preset</button>
                            <button class="btn btn-sm btn-danger" onclick="resetToDefaults()" style="background: linear-gradient(135deg, #ef4444, #b91c1c); border: none;">Resetare Fabrică</button>
                        </div>
                    </div>
                </div>

                <!-- Grafic Telemetrie Luminozitate -->
                <div class="card" style="margin-top: 12px;">
                    <div class="card-title">Istoric Luminozitate (24 Ore)</div>
                    <div style="position: relative; height: 220px; width: 100%;">
                        <canvas id="telemetryChart"></canvas>
                    </div>
                </div>
            </div>

            <!-- Right Column (Narrower): Control Periferice, WiFi Config -->
            <div class="layout-column column-right">
                <!-- Control Periferice -->
                <div class="card">
                    <div class="card-title">Control Periferice</div>
                    <div id="relays-container" style="display: flex; flex-direction: column; gap: 12px;">
                        <!-- Relays will be inserted here dynamically -->
                    </div>
                    <button class="btn btn-secondary btn-sm" onclick="clearOverrides()" style="margin-top: 8px;">Revenire la Auto (Toate)</button>
                </div>

                <!-- Administrare Sistem -->
                <div class="card">
                    <div class="card-title">Administrare Sistem</div>
                    <div style="display: flex; flex-direction: column; gap: 12px;">
                        <button class="btn btn-secondary" onclick="openWifiModal()">Configurare WiFi</button>
                        <button class="btn" onclick="window.open('/update', '_blank')">Update Firmware</button>
                    </div>
                </div>
            </div>
        </div>

        <!-- Istoric Evenimente (Timeline) - Full Width at bottom -->
        <div class="card" style="margin-top: 20px;">
            <div class="card-title">Istoric Evenimente (Timeline)</div>
            <div class="timeline-container" id="timeline-container">
                <p style="font-size: 13px; color: var(--text-secondary)">Se încarcă istoricul...</p>
            </div>
        </div>

        <!-- Modal pentru Configurare WiFi -->
        <div id="wifi-modal" class="modal">
            <div class="modal-content">
                <div class="modal-header">
                    <h2>Configurare WiFi</h2>
                    <span class="close-btn" onclick="closeWifiModal()">&times;</span>
                </div>
                <div class="form-group">
                    <label for="wifi-ssid">SSID Rețea</label>
                    <input type="text" id="wifi-ssid" placeholder="Nume rețea locală">
                </div>
                <div class="form-group">
                    <label for="wifi-pass">Parolă</label>
                    <input type="password" id="wifi-pass" placeholder="••••••••">
                </div>
                <button class="btn" style="margin-top: 8px;" onclick="saveWifiAndClose()">Conectează Dispozitivul</button>
            </div>
        </div>
        
        <footer>
            Aquatlantis Smart Aquarium Controller • ESP8266 ESP-12F • Otopeni, România
        </footer>
    </div>

    <div class="toast" id="toast">Setări salvate cu succes!</div>

    <script>
        // Local state of schedules loaded from API
        let schedules = [];

        function showToast(msg) {
            const t = document.getElementById('toast');
            t.innerText = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 3000);
        }

        function formatUptime(sec) {
            const h = Math.floor(sec / 3600);
            const m = Math.floor((sec % 3600) / 60);
            const s = sec % 60;
            return `${h}h ${m}m ${s}s`;
        }

        // Initialize 24 hour grid cells
        function initHourGrid() {
            const grid = document.getElementById('hour-grid');
            grid.innerHTML = '';
            for (let i = 0; i < 24; i++) {
                const cell = document.createElement('div');
                cell.className = 'hour-cell';
                cell.innerText = i.toString().padStart(2, '0');
                cell.dataset.hour = i;
                cell.onclick = () => {
                    cell.classList.toggle('active');
                };
                grid.appendChild(cell);
            }
        }

        function onRelaySelected(relayNum) {
            const sched = schedules.find(s => s.num == relayNum);
            if (!sched) return;

            // Load behavior
            document.getElementById('relay-behavior').value = sched.behavior;
            onBehaviorChanged(sched.behavior);

            // Load pulse settings
            document.getElementById('relay-pulse-on').value = sched.pulse_on;
            document.getElementById('relay-pulse-off').value = sched.pulse_off;

            // Load hours bitmap
            const cells = document.querySelectorAll('.hour-cell');
            cells.forEach(cell => {
                const hr = parseInt(cell.dataset.hour);
                const isActive = (sched.active_hours & (1 << hr)) !== 0;
                if (isActive) {
                    cell.classList.add('active');
                } else {
                    cell.classList.remove('active');
                }
            });
        }

        function onBehaviorChanged(val) {
            const row = document.getElementById('pulse-settings-row');
            if (val == "1") {
                row.style.display = 'grid';
            } else {
                row.style.display = 'none';
            }
        }

        async function fetchHistory() {
            try {
                const res = await fetch('/api/history');
                const data = await res.json();
                const container = document.getElementById('timeline-container');
                container.innerHTML = '';
                
                if (data.length === 0) {
                    container.innerHTML = '<p style="font-size: 13px; color: var(--text-secondary)">Nu există loguri încă.</p>';
                    return;
                }
                
                data.reverse().forEach(event => {
                    const item = document.createElement('div');
                    item.className = 'timeline-item';
                    item.innerHTML = `
                        <div class="timeline-time">${event.time}</div>
                        <div class="timeline-msg">${event.msg}</div>
                    `;
                    container.appendChild(item);
                });
            } catch (err) {
                console.error("Error fetching history:", err);
            }
        }

        async function fetchSchedules() {
            try {
                const res = await fetch('/api/schedule');
                schedules = await res.json();
                // Refresh currently selected relay schedule in grid
                const currentRelay = document.getElementById('relay-select').value;
                onRelaySelected(currentRelay);
            } catch (err) {
                console.error("Error fetching schedules:", err);
            }
        }

        async function fetchStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                // Update stats
                document.getElementById('mode-val').innerText = data.mode;
                document.getElementById('time-val').innerText = data.time;
                document.getElementById('uptime-val').innerText = formatUptime(data.uptime);
                document.getElementById('ssid-val').innerText = data.wifi_ssid;
                document.getElementById('rssi-val').innerText = data.wifi_rssi + ' dBm';
                document.getElementById('ip-val').innerText = data.ip;
                document.getElementById('light-val').innerText = data.light_percent + '%';
                
                // Update Digital Inputs
                const updateInputDot = (id, state) => {
                    const el = document.getElementById(id);
                    if (state) {
                        el.innerHTML = '<span class="status-dot dot-green"></span> HIGH (3.3V)';
                    } else {
                        el.innerHTML = '<span class="status-dot dot-red"></span> LOW (GND)';
                    }
                };
                updateInputDot('input-gpio0', data.inputs.gpio0);
                updateInputDot('input-gpio4', data.inputs.gpio4);
                updateInputDot('input-gpio2', data.inputs.gpio2);
                updateInputDot('input-gpio15', data.inputs.gpio15);
                
                // System operational dot
                const sysDot = document.getElementById('system-dot');
                sysDot.className = 'status-dot';
                if (data.mode_id === 0) sysDot.classList.add('dot-green'); // Day
                else if (data.mode_id === 1) sysDot.classList.add('dot-orange'); // Night
                else if (data.mode_id === 2) sysDot.classList.add('dot-red'); // Power Loss
                
                // WiFi badge
                const wifiBadge = document.getElementById('wifi-badge');
                const wifiDot = document.getElementById('wifi-dot');
                const wifiText = document.getElementById('wifi-status-text');
                wifiText.innerText = data.wifi_status;
                wifiDot.className = 'status-dot';
                if (data.wifi_status === 'Connected') {
                    wifiDot.classList.add('dot-green');
                } else {
                    wifiDot.classList.add('dot-red');
                }
                
                // Relays list
                const container = document.getElementById('relays-container');
                container.innerHTML = '';
                
                data.relays.forEach(relay => {
                    const item = document.createElement('div');
                    item.className = 'relay-item';
                    
                    const badgeClass = relay.override ? 'badge-manual' : 'badge-auto';
                    const badgeText = relay.override ? 'Manual' : 'Auto';
                    const stateDotClass = relay.state ? 'dot-green' : 'dot-red';
                    const stateText = relay.state ? 'PORNIT' : 'OPRIT';
                    
                    item.innerHTML = `
                        <div class="relay-header">
                            <div>
                                <span class="relay-name">${relay.name}</span>
                                <div class="relay-meta">Releu ${relay.num} (GPIO ${relay.num === 1 ? 16 : relay.num === 2 ? 14 : relay.num === 3 ? 12 : 13})</div>
                            </div>
                            <div style="text-align: right;">
                                <span class="badge ${badgeClass}">${badgeText}</span>
                                <div style="font-size: 11px; margin-top: 4px; display: flex; align-items: center; gap: 4px; justify-content: flex-end;">
                                    <span class="status-dot ${stateDotClass}"></span> ${stateText}
                                </div>
                            </div>
                        </div>
                        <div class="relay-controls">
                            <button class="btn btn-sm btn-secondary ${relay.override ? 'btn-danger' : ''}" onclick="toggleOverride(${relay.num}, ${relay.override ? 0 : 1})">
                                ${relay.override ? 'Eliberează Auto' : 'Forțează Manual'}
                            </button>
                            <label class="toggle-switch" style="visibility: ${relay.override ? 'visible' : 'hidden'}">
                                <input type="checkbox" ${relay.state ? 'checked' : ''} onchange="toggleState(${relay.num}, this.checked ? 1 : 0)">
                                <span class="slider"></span>
                            </label>
                        </div>
                    `;
                    container.appendChild(item);
                });
                
            } catch (err) {
                console.error("Error fetching status:", err);
            }
        }

        async function toggleOverride(num, override) {
            try {
                const res = await fetch('/api/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${num}&override=${override}`
                });
                if (res.ok) {
                    showToast(override ? `Forțare manuală activată pentru Releu ${num}` : `Releu ${num} redat controlului automat`);
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        async function toggleState(num, state) {
            try {
                const res = await fetch('/api/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${num}&override=1&state=${state}`
                });
                if (res.ok) {
                    showToast(`Releu ${num} setat pe ${state ? 'PORNIT' : 'OPRIT'}`);
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        async function clearOverrides() {
            try {
                const res = await fetch('/api/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'clear=1'
                });
                if (res.ok) {
                    showToast("Toate releele au revenit în mod automat.");
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        async function saveRelaySchedule() {
            const relayNum = document.getElementById('relay-select').value;
            const behavior = document.getElementById('relay-behavior').value;
            const pulseOn = document.getElementById('relay-pulse-on').value;
            const pulseOff = document.getElementById('relay-pulse-off').value;

            // Calculate hours bitmap
            let bitmap = 0;
            const cells = document.querySelectorAll('.hour-cell');
            cells.forEach(cell => {
                if (cell.classList.contains('active')) {
                    const hr = parseInt(cell.dataset.hour);
                    bitmap |= (1 << hr);
                }
            });

            try {
                const res = await fetch('/api/schedule', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${relayNum}&active_hours=${bitmap}&behavior=${behavior}&pulse_on=${pulseOn}&pulse_off=${pulseOff}`
                });
                if (res.ok) {
                    showToast(`Orarul Releului ${relayNum} a fost salvat!`);
                    await fetchSchedules();
                    await fetchHistory();
                } else {
                    showToast("Eroare la salvarea orarului!");
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        async function saveWifi() {
            const ssid = document.getElementById('wifi-ssid').value;
            const pass = document.getElementById('wifi-pass').value;
            
            if (!ssid) {
                showToast("SSID-ul nu poate fi gol!");
                return;
            }
            
            try {
                showToast("Se trimit credențialele. Dispozitivul va încerca reconectarea...");
                fetch('/api/wifi', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
                });
            } catch (err) {
                showToast("Eroare de trimitere!");
            }
        }

        function openWifiModal() {
            document.getElementById('wifi-modal').classList.add('show');
            const currentSSID = document.getElementById('ssid-val').innerText;
            if (currentSSID && currentSSID !== '-') {
                document.getElementById('wifi-ssid').value = currentSSID;
            }
        }

        function closeWifiModal() {
            document.getElementById('wifi-modal').classList.remove('show');
        }

        async function saveWifiAndClose() {
            const ssid = document.getElementById('wifi-ssid').value;
            if (!ssid) {
                showToast("SSID-ul nu poate fi gol!");
                return;
            }
            await saveWifi();
            closeWifiModal();
        }

        async function applyPreset() {
            const pNum = document.getElementById('preset-select').value;
            if (!confirm(`Sigur dorești să aplici Presetul ${pNum}? Aceasta va suprascrie orarul actual al releelor.`)) {
                return;
            }
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `apply=${pNum}`
                });
                if (res.ok) {
                    showToast("Preset aplicat cu succes!");
                    await fetchSchedules();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Eroare la aplicarea presetului!");
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        async function resetToDefaults() {
            if (!confirm("Sigur dorești să resetezi toate releele la setările din fabrică?")) {
                return;
            }
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'reset=1'
                });
                if (res.ok) {
                    showToast("Programe resetate la setările din fabrică!");
                    await fetchSchedules();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Eroare la resetarea programelor!");
                }
            } catch (err) {
                showToast("Eroare de comunicare!");
            }
        }

        let telemetryChartInstance = null;
        async function fetchTelemetry() {
            try {
                const res = await fetch('/api/telemetry');
                const data = await res.json();
                
                const labels = data.map(item => item.time);
                const values = data.map(item => item.light);
                
                const ctx = document.getElementById('telemetryChart').getContext('2d');
                
                if (telemetryChartInstance) {
                    telemetryChartInstance.data.labels = labels;
                    telemetryChartInstance.data.datasets[0].data = values;
                    telemetryChartInstance.update();
                } else {
                    const gradient = ctx.createLinearGradient(0, 0, 0, 200);
                    gradient.addColorStop(0, 'rgba(56, 189, 248, 0.3)');
                    gradient.addColorStop(1, 'rgba(56, 189, 248, 0.0)');
                    
                    telemetryChartInstance = new Chart(ctx, {
                        type: 'line',
                        data: {
                            labels: labels,
                            datasets: [{
                                label: 'Nivel Lumină (%)',
                                data: values,
                                borderColor: '#38bdf8',
                                borderWidth: 2,
                                backgroundColor: gradient,
                                fill: true,
                                tension: 0.3,
                                pointRadius: 2,
                                pointHoverRadius: 5,
                                pointBackgroundColor: '#38bdf8'
                            }]
                        },
                        options: {
                            responsive: true,
                            maintainAspectRatio: false,
                            plugins: {
                                legend: {
                                    display: false
                                },
                                tooltip: {
                                    backgroundColor: '#1e2942',
                                    titleFont: { family: 'Outfit', size: 12 },
                                    bodyFont: { family: 'Outfit', size: 12 },
                                    borderColor: 'rgba(255, 255, 255, 0.08)',
                                    borderWidth: 1,
                                    displayColors: false,
                                    callbacks: {
                                        label: function(context) {
                                            return `Lumină: ${context.parsed.y}%`;
                                        }
                                    }
                                }
                            },
                            scales: {
                                x: {
                                    grid: {
                                        color: 'rgba(255, 255, 255, 0.05)'
                                    },
                                    ticks: {
                                        color: '#94a3b8',
                                        font: { family: 'Outfit', size: 10 },
                                        maxTicksLimit: 8
                                    }
                                },
                                y: {
                                    min: 0,
                                    max: 100,
                                    grid: {
                                        color: 'rgba(255, 255, 255, 0.05)'
                                    },
                                    ticks: {
                                        color: '#94a3b8',
                                        font: { family: 'Outfit', size: 10 },
                                        stepSize: 20,
                                        callback: function(value) {
                                            return value + '%';
                                        }
                                    }
                                }
                            }
                        }
                    });
                }
            } catch (err) {
                console.error("Error fetching telemetry:", err);
            }
        }

        // Initialize UI components
        initHourGrid();

        // Initial Data Loads
        fetchStatus();
        fetchSchedules();
        fetchHistory();
        fetchTelemetry();

        // Polling updates (every 2 seconds)
        setInterval(() => {
            fetchStatus();
            fetchHistory();
        }, 2000);

        // Polling telemetry (every 30 seconds)
        setInterval(() => {
            fetchTelemetry();
        }, 30000);
    </script>
</body>
</html>
)rawhtml";

// Serve Dashboard HTML page from Flash memory
void handleRoot() {
    server.send_P(200, "text/html", DASHBOARD_HTML);
}

// REST API endpoint: Returns system status and states as JSON
void handleStatus() {
    String r = "{";
    r += "\"mode\":\"" + getModeString(currentSystemMode) + "\",";
    r += "\"mode_id\":" + String((int)currentSystemMode) + ",";
    r += "\"inputs\":{";
    r += "\"gpio0\":" + String(debouncedPowerState == HIGH ? "true" : "false") + ",";
    r += "\"gpio4\":" + String(digitalRead(PIN_INPUT_GPIO4) == HIGH ? "true" : "false") + ",";
    r += "\"gpio2\":" + String(digitalRead(PIN_INPUT_GPIO2) == HIGH ? "true" : "false") + ",";
    r += "\"gpio15\":" + String(digitalRead(PIN_INPUT_GPIO15) == HIGH ? "true" : "false");
    r += "},";
    r += "\"time\":\"" + getFormattedTime() + "\",";
    r += "\"wifi_ssid\":\"" + getWiFiSSID() + "\",";
    r += "\"wifi_rssi\":" + String(getWiFiRSSI()) + ",";
    r += "\"wifi_status\":\"" + getNetworkStatusString() + "\",";
    r += "\"ip\":\"" + getIPAddress() + "\",";
    r += "\"uptime\":" + String(millis() / 1000) + ",";
    r += "\"light_percent\":" + String(getLightLevelPercent()) + ",";
    r += "\"relays\":[";
    for (int i = 1; i <= 4; i++) {
        RelayState s = getRelayState(i);
        String name;
        switch(i) {
            case 1: name = "Iluminat Principal"; break;
            case 2: name = "Electrovalva CO2"; break;
            case 3: name = "Pompa de Aer"; break;
            case 4: name = "Iluminat Ambiental"; break;
            default: name = "Releu"; break;
        }
        r += "{";
        r += "\"num\":" + String(i) + ",";
        r += "\"name\":\"" + name + "\",";
        r += "\"state\":" + String(s.physicalState ? "true" : "false") + ",";
        r += "\"override\":" + String(s.manualOverride ? "true" : "false") + ",";
        r += "\"override_state\":" + String(s.manualState ? "true" : "false");
        r += "}";
        if (i < 4) r += ",";
    }
    r += "]";
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

// REST API endpoint: Set/Update a specific relay profile (saves to EEPROM)
void handleScheduleSet() {
    if (server.hasArg("relay") && server.hasArg("active_hours") && server.hasArg("behavior") && server.hasArg("pulse_on") && server.hasArg("pulse_off")) {
        int r = server.arg("relay").toInt();
        uint32_t hours = strtoul(server.arg("active_hours").c_str(), NULL, 10);
        uint8_t behavior = server.arg("behavior").toInt();
        uint32_t on = server.arg("pulse_on").toInt();
        uint32_t off = server.arg("pulse_off").toInt();
        
        if (r >= 1 && r <= 4 && (behavior == 0 || behavior == 1) && on > 0 && off > 0) {
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

// REST API endpoint: Load presets or factory reset configurations
void handlePresets() {
    if (server.hasArg("apply")) {
        int presetNum = server.arg("apply").toInt();
        if (presetNum >= 1 && presetNum <= 3) {
            applyPreset(presetNum);
            server.send(200, "text/plain", "OK");
            return;
        }
    } else if (server.hasArg("reset") && server.arg("reset") == "1") {
        resetSettingsToDefault();
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(400, "text/plain", "Bad Request");
}

void setup() {
    // Start Serial debug port
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n========================================");
    Serial.println("Aquatlantis Smart Aquarium Controller v2.0");
    Serial.println("========================================");
    
    // Setup inputs
    pinMode(PIN_INPUT_GPIO0, INPUT);
    pinMode(PIN_INPUT_GPIO4, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO2, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO15, INPUT);
    Serial.println("[Init] GPIO pins configured.");
    
    // Initialize Submodules (loads settings and logs boot event)
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
    server.on("/api/presets", HTTP_POST, handlePresets);
    server.on("/api/telemetry", HTTP_GET, []() {
        server.send(200, "application/json", getTelemetryJSON());
    });
    
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
    
    // Start Web Server
    server.begin();
    Serial.println("[Init] HTTP server started on port 80.");
    
    // Start mDNS responder
    if (MDNS.begin("acvariu")) {
        Serial.println("[mDNS] Started successfully. Access via http://acvariu.local/");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("[mDNS] Error starting responder.");
    }
    
    // Start OTA web updater
    httpUpdater.setup(&server, "/update", OTA_USER, OTA_PASS);
    Serial.println("[Init] OTA web updater registered on /update with credentials.");
}

void loop() {
    // 1. Maintain background tasks (NTP sync checks, connection updates, status LED blink)
    updateNetwork();
    
    // Maintain mDNS responder
    MDNS.update();
    
    // Debounce PIN_INPUT_GPIO0 (Power Sense) to avoid false triggers or flapping
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
    
    // 2. Execute state machine logic based on NTP synchronized time and Power Sense (debounced)
    if (debouncedPowerState == LOW) {
        currentSystemMode = MODE_POWER_LOSS;
    } else if (isTimeSynced()) {
        int currentHour = getCurrentHour();
        RelayProfile r3 = getRelayProfile(3);
        
        // For logging logic, we determine general "Day/Night" based on Relay 3 (Air Pump) active hours
        // Normally, if Air Pump is active, it means it is Night. If not active, it is Day.
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
    
    // 3. Command relays based on calculated system state & scheduling profiles
    updateRelays(currentSystemMode);
    
    // 6. Periodic telemetry history collection (every 15 minutes)
    static unsigned long lastTelemetryTime = 0;
    if (isTimeSynced() && (lastTelemetryTime == 0 || millis() - lastTelemetryTime >= 900000UL)) {
        lastTelemetryTime = millis();
        int currentPercent = getLightLevelPercent();
        // Get the current HH:MM time
        char timeBuf[10];
        time_t tNow = time(nullptr);
        struct tm* tInfo = localtime(&tNow);
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", tInfo);
        addTelemetryReading(currentPercent, String(timeBuf));
    }

    // 7. Smart Diagnostic: Detect Main Light (Relay 1) physical failure
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
                logSystemEvent("[ATENȚIE] Defecțiune lampă! Releul 1 este PORNIT, dar luminozitatea este sub 15% (" + String(lightLvl) + "%). Verifică alimentarea lămpii.");
                lampDefectLogged = true;
            }
        }
    } else {
        lampTurnedOnTime = 0;
        lampDefectLogged = false;
    }
    
    // 4. Web requests handler
    server.handleClient();
    
    // 5. Periodic serial logger (every 5 seconds) to aid deployment diagnostics
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
            
            // Format active hours bitmap to intervals
            String activeHoursStr = "";
            int start = -1;
            for (int h = 0; h < 24; h++) {
                bool active = (p.activeHours & (1UL << h)) != 0;
                if (active) {
                    if (start == -1) start = h;
                } else {
                    if (start != -1) {
                        if (activeHoursStr.length() > 0) activeHoursStr += ",";
                        if (start == h - 1) activeHoursStr += String(start);
                        else activeHoursStr += String(start) + "-" + String(h - 1);
                        start = -1;
                    }
                }
            }
            if (start != -1) {
                if (activeHoursStr.length() > 0) activeHoursStr += ",";
                if (start == 23) activeHoursStr += "23";
                else activeHoursStr += String(start) + "-23";
            }
            if (activeHoursStr.length() == 0) activeHoursStr = "None";
            
            Serial.printf("      Relay %d: %s | Mode: %s | Behavior: %s | Sched: [%s]\n",
                          i,
                          s.physicalState ? "ON" : "OFF",
                          modeStr.c_str(),
                          behaviorStr.c_str(),
                          activeHoursStr.c_str());
        }
        Serial.println("-----------------------------------------------------------------");
    }
    
    // Tiny delay to yield to the ESP8266 background processes (prevent watchdog timeouts)
    delay(5);
}
