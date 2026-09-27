#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266LLMNR.h>
#include <ESP8266SSDP.h>
#include <LittleFS.h>
#include "Config.h"
#include "NetworkSync.h"
#include "RelayControl.h"

// Initialize the web server on port 80
ESP8266WebServer server(80);

// Current system-wide operational mode
SystemMode currentSystemMode = MODE_DAY;

// Debounced power state for PIN_INPUT_GPIO0 (Power Sense)
static int debouncedPowerState = HIGH;

// Expanded Dashboard HTML in FLASH memory (PROGMEM)
const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aquatlantis | Smart Aquarium</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600;700&family=Outfit:wght@400;500;600;700;800&display=swap');
        
        :root {
            --bg-base: #080d1a;
            --bg-surface: rgba(21, 30, 50, 0.75);
            --bg-card: rgba(28, 40, 68, 0.7);
            --text-primary: #f8fafc;
            --text-secondary: #94a3b8;
            --accent-primary: #38bdf8;
            --accent-primary-glow: rgba(56, 189, 248, 0.35);
            --state-ok: #10b981;
            --state-ok-glow: rgba(16, 185, 129, 0.35);
            --state-warn: #f59e0b;
            --state-warn-glow: rgba(245, 158, 11, 0.35);
            --state-danger: #ef4444;
            --state-danger-glow: rgba(239, 68, 68, 0.35);
            --border-color: rgba(255, 255, 255, 0.08);
            --border-glow: rgba(56, 189, 248, 0.15);
        }
        
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Outfit', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: radial-gradient(circle at 50% 0%, #172554 0%, var(--bg-base) 100%);
            color: var(--text-primary);
            padding: 20px 14px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }
        
        .container {
            max-width: 900px;
            width: 100%;
            display: flex;
            flex-direction: column;
            gap: 18px;
        }
        
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: linear-gradient(135deg, var(--bg-surface), var(--bg-card));
            padding: 16px 20px;
            border-radius: 14px;
            border: 1px solid var(--border-color);
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        
        h1 {
            font-size: 22px;
            font-weight: 800;
            background: linear-gradient(to right, #38bdf8, #818cf8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .sys-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 13px;
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
            flex-shrink: 0;
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
        
        .card {
            background: linear-gradient(135deg, var(--bg-surface), var(--bg-card));
            border-radius: 14px;
            border: 1px solid var(--border-color);
            padding: 18px;
            box-shadow: 0 4px 18px rgba(0,0,0,0.25);
            display: flex;
            flex-direction: column;
            gap: 14px;
            transition: box-shadow 0.2s;
        }
        
        .card:hover {
            box-shadow: 0 6px 22px rgba(56, 189, 248, 0.07);
        }
        
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 10px;
        }
        
        .card-title {
            font-size: 16px;
            font-weight: 700;
            color: var(--accent-primary);
            display: flex;
            align-items: center;
            gap: 8px;
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
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }

        /* Feed Mode Banner */
        .feed-banner {
            display: none;
            background: linear-gradient(135deg, rgba(245, 158, 11, 0.15), rgba(245, 158, 11, 0.05));
            border: 1px solid rgba(245, 158, 11, 0.4);
            border-radius: 12px;
            padding: 12px 16px;
            align-items: center;
            justify-content: space-between;
            gap: 12px;
            animation: pulse-orange 2.5s infinite;
        }
        .feed-banner.show {
            display: flex;
        }
        .feed-text {
            font-size: 13px;
            font-weight: 600;
            color: #fde68a;
        }
        
        /* Relays List */
        .relay-item {
            background: var(--bg-card);
            border-radius: 10px;
            padding: 12px 14px;
            border: 1px solid var(--border-color);
            display: flex;
            flex-direction: column;
            gap: 8px;
        }
        
        .relay-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .relay-name {
            font-weight: 700;
            font-size: 15px;
            color: var(--text-primary);
        }
        
        .relay-gpio-tag {
            font-family: 'JetBrains Mono', Consolas, monospace;
            font-size: 11px;
            color: var(--text-secondary);
            background: rgba(255, 255, 255, 0.05);
            padding: 2px 6px;
            border-radius: 4px;
            font-weight: 500;
        }
        
        .relay-status-banner {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 8px 12px;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 600;
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid var(--border-color);
            line-height: 1.4;
            letter-spacing: 0.2px;
        }
        .relay-status-banner.status-active {
            background: rgba(16, 185, 129, 0.1);
            border-color: rgba(16, 185, 129, 0.3);
            color: #34d399;
        }
        .relay-status-banner.status-paused {
            background: rgba(245, 158, 11, 0.12);
            border-color: rgba(245, 158, 11, 0.35);
            color: #fbbf24;
            animation: pulse-orange 3s infinite;
        }
        .relay-status-banner.status-idle {
            background: rgba(148, 163, 184, 0.06);
            border-color: rgba(148, 163, 184, 0.18);
            color: var(--text-secondary);
        }
        .relay-status-banner.status-warn {
            background: rgba(239, 68, 68, 0.12);
            border-color: rgba(239, 68, 68, 0.35);
            color: #f87171;
        }
        
        .relay-action-badges {
            display: flex;
            align-items: center;
            gap: 6px;
        }

        .btn-badge {
            font-family: inherit;
            font-size: 11px;
            font-weight: 700;
            padding: 5px 10px;
            border-radius: 6px;
            cursor: pointer;
            transition: all 0.2s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 4px;
            border: 1px solid transparent;
            user-select: none;
            line-height: 1;
        }

        .btn-badge:hover {
            transform: translateY(-1px);
        }

        .btn-badge:active {
            transform: translateY(1px);
        }

        /* AUTO badge button */
        .btn-badge-auto.active {
            background: rgba(16, 185, 129, 0.18);
            color: #34d399;
            border-color: rgba(16, 185, 129, 0.45);
            box-shadow: 0 0 10px rgba(16, 185, 129, 0.25);
        }

        .btn-badge-auto.inactive {
            background: rgba(255, 255, 255, 0.04);
            color: var(--text-secondary);
            border-color: var(--border-color);
            opacity: 0.65;
        }

        .btn-badge-auto.inactive:hover {
            opacity: 1;
            background: rgba(255, 255, 255, 0.08);
            color: var(--text-primary);
        }

        /* POWER badge button */
        .btn-badge-power.neutral {
            background: rgba(255, 255, 255, 0.04);
            color: var(--text-secondary);
            border-color: var(--border-color);
            opacity: 0.65;
        }

        .btn-badge-power.neutral:hover {
            opacity: 1;
            background: rgba(255, 255, 255, 0.08);
            color: var(--text-primary);
        }

        .btn-badge-power.forced-on {
            background: linear-gradient(135deg, #10b981, #059669);
            color: #ffffff;
            border-color: #34d399;
            box-shadow: 0 0 12px rgba(16, 185, 129, 0.45);
        }

        .btn-badge-power.forced-off {
            background: linear-gradient(135deg, #ef4444, #dc2626);
            color: #ffffff;
            border-color: #f87171;
            box-shadow: 0 0 12px rgba(239, 68, 68, 0.45);
        }
        
        .btn {
            background: linear-gradient(135deg, #38bdf8, #2563eb);
            color: white;
            border: none;
            padding: 8px 14px;
            border-radius: 8px;
            font-family: inherit;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            font-size: 13px;
        }
        
        .btn:hover {
            opacity: 0.92;
            box-shadow: 0 0 10px rgba(56, 189, 248, 0.35);
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
            background: rgba(255,255,255,0.14);
            box-shadow: none;
        }
        
        .btn-danger {
            background: linear-gradient(135deg, #ef4444, #b91c1c);
        }
        
        .btn-warning {
            background: linear-gradient(135deg, #f59e0b, #d97706);
            color: #111827;
        }
        
        .btn-sm {
            padding: 6px 12px;
            font-size: 12px;
            border-radius: 6px;
        }
        
        .badge {
            font-size: 11px;
            padding: 3px 8px;
            border-radius: 4px;
            font-weight: 700;
            text-transform: uppercase;
        }
        
        .badge-auto { background: rgba(56, 189, 248, 0.15); color: var(--accent-primary); border: 1px solid rgba(56, 189, 248, 0.3); }
        .badge-manual { background: rgba(245, 158, 11, 0.15); color: var(--state-warn); border: 1px solid rgba(245, 158, 11, 0.3); }

        /* 24-hour visual grid */
        .hour-grid {
            display: grid;
            grid-template-columns: repeat(12, 1fr);
            gap: 6px;
            margin-top: 8px;
        }
        
        @media(max-width: 550px) {
            .hour-grid {
                grid-template-columns: repeat(6, 1fr);
            }
        }
        
        .hour-cell {
            background-color: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 8px 4px;
            border-radius: 6px;
            font-size: 11px;
            font-weight: 700;
            text-align: center;
            cursor: pointer;
            transition: all 0.15s;
            user-select: none;
        }
        
        .hour-cell:hover {
            border-color: var(--accent-primary);
        }
        
        .hour-cell.active {
            background-color: var(--accent-primary);
            color: #0b0f19;
            box-shadow: 0 0 8px var(--accent-primary-glow);
            border-color: var(--accent-primary);
        }
        
        /* Form controls */
        .form-group {
            display: flex;
            flex-direction: column;
            gap: 5px;
        }
        
        label {
            font-size: 11px;
            color: var(--text-secondary);
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        input[type="number"], input[type="text"], input[type="password"], select {
            background-color: var(--bg-card);
            border: 1px solid var(--border-color);
            padding: 9px 12px;
            border-radius: 8px;
            color: white;
            font-family: inherit;
            font-size: 13px;
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
        
        .pulse-input-pair {
            display: flex;
            align-items: center;
            gap: 6px;
        }
        .pulse-input-pair input {
            width: 60px;
            text-align: center;
        }
        .pulse-input-pair span {
            font-size: 12px;
            color: var(--text-secondary);
        }

        /* Timeline Container & Items */
        .timeline-container {
            max-height: 250px;
            overflow-y: auto;
            overflow-x: auto;
            display: flex;
            flex-direction: column;
            gap: 6px;
            padding-right: 4px;
            -webkit-overflow-scrolling: touch;
        }
        
        .timeline-container::-webkit-scrollbar {
            width: 6px;
            height: 6px;
        }
        .timeline-container::-webkit-scrollbar-thumb {
            background: var(--border-color);
            border-radius: 4px;
        }
        
        .timeline-item {
            display: flex;
            align-items: flex-start;
            gap: 8px;
            padding: 8px 10px;
            background: rgba(255, 255, 255, 0.02);
            border-left: 3px solid var(--accent-primary);
            border-radius: 6px;
            font-size: 12px;
            line-height: 1.4;
            min-width: 0;
        }
        
        .timeline-time {
            font-family: 'JetBrains Mono', Consolas, monospace;
            font-size: 11px;
            color: var(--accent-primary);
            background: rgba(56, 189, 248, 0.12);
            padding: 2px 6px;
            border-radius: 4px;
            flex-shrink: 0;
            font-weight: 600;
            white-space: nowrap;
        }
        
        .timeline-msg {
            color: var(--text-primary);
            word-break: break-word;
            flex-grow: 1;
            min-width: 0;
        }
        
        /* Modals */
        .modal {
            display: none;
            position: fixed;
            top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(11, 15, 25, 0.85);
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
            border-radius: 14px;
            border: 1px solid var(--border-color);
            padding: 22px;
            max-width: 440px;
            width: 100%;
            box-shadow: 0 10px 30px rgba(0,0,0,0.6);
            display: flex;
            flex-direction: column;
            gap: 14px;
            max-height: 90vh;
            overflow-y: auto;
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 8px;
        }
        .modal-header h2 {
            font-size: 17px;
            font-weight: 700;
            color: var(--accent-primary);
        }
        .close-btn {
            font-size: 24px;
            color: var(--text-secondary);
            cursor: pointer;
            line-height: 1;
        }
        .close-btn:hover {
            color: var(--state-danger);
        }
        
        .toast {
            position: fixed;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%) translateY(100px);
            background-color: #1e2942;
            border: 1px solid var(--accent-primary);
            padding: 10px 20px;
            border-radius: 8px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.5);
            transition: transform 0.3s ease;
            z-index: 3000;
            font-weight: 600;
            font-size: 13px;
        }
        .toast.show {
            transform: translateX(-50%) translateY(0);
        }
        
        footer {
            text-align: center;
            font-size: 12px;
            color: var(--text-secondary);
            padding: 12px 0 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <header>
            <div>
                <h1>Aquatlantis</h1>
                <p style="font-size: 12px; color: var(--text-secondary)">Smart Controller • BioBox 56L</p>
            </div>
        </header>

        <!-- Feed Mode Alert Banner -->
        <div class="feed-banner" id="feed-banner">
            <div class="feed-text">
                🫧 <strong>Feed Mode Active:</strong> Air pump temporarily paused (<span id="feed-timer">10:00</span> remaining).
            </div>
            <button class="btn btn-sm btn-secondary" onclick="stopFeedMode()">Cancel</button>
        </div>

        <!-- 1. SYSTEM STATUS -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">1. System Status</div>
                <div class="sys-badge" id="wifi-badge">
                    <span class="status-dot dot-green" id="wifi-dot"></span>
                    <span id="wifi-status-text">Connected</span>
                </div>
            </div>
            <div class="compact-grid">
                <div class="stat-block">
                    <span class="stat-label">System Mode</span>
                    <span class="stat-val" id="mode-val">Day (Normal)</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label">NTP Clock</span>
                    <span class="stat-val" id="time-val">00:00:00</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label">System Uptime</span>
                    <span class="stat-val" id="uptime-val">0s</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label">WiFi Network</span>
                    <span class="stat-val" id="ssid-val">-</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label">WiFi Signal</span>
                    <span class="stat-val" id="rssi-val">-65 dBm</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label">IP Address</span>
                    <span class="stat-val" id="ip-val">192.168.1.32</span>
                </div>
            </div>
        </div>

        <!-- 2. PERIPHERAL STATUS & CONTROL -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">2. Peripheral Status & Control</div>
                <div style="display: flex; gap: 8px;">
                    <button class="btn btn-sm btn-warning" onclick="startFeedMode()" title="Pause air pump for 10 minutes to allow fish feeding">🫧 Feed Mode (10m)</button>
                    <button class="btn btn-secondary btn-sm" onclick="clearOverrides()">Auto (All)</button>
                </div>
            </div>
            <div id="relays-container" style="display: flex; flex-direction: column; gap: 10px;">
                <!-- Dynamically populated from status -->
            </div>
        </div>

        <!-- 3. SCHEDULE CONFIGURATION & PRESETS -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">3. Schedule Configuration (24h)</div>
                <select id="relay-select" style="width: auto; padding: 6px 10px;" onchange="onRelaySelected(this.value)">
                    <option value="1">Relay 1</option>
                    <option value="2">Relay 2</option>
                    <option value="3">Relay 3</option>
                    <option value="4">Relay 4</option>
                </select>
            </div>
            
            <div style="display: flex; flex-direction: column; gap: 6px;">
                <label>Active Operating Hours (00 - 23):</label>
                <div class="hour-grid" id="hour-grid">
                    <!-- 24 cells -->
                </div>
            </div>

            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 14px; margin-top: 6px;">
                <div class="form-group">
                    <label for="relay-behavior">Mode during active hours</label>
                    <select id="relay-behavior" onchange="onBehaviorChanged(this.value)">
                        <option value="0">Continuous (active throughout checked hours)</option>
                        <option value="1">Pulse / Intermittent (repeating ON / OFF cycles)</option>
                    </select>
                </div>
                
                <div id="pulse-settings-row" style="display: none; grid-template-columns: 1fr 1fr; gap: 10px;">
                    <div class="form-group">
                        <label>Pulse ON (Running Time)</label>
                        <div class="pulse-input-pair">
                            <input type="number" id="pulse-on-min" min="0" max="60" value="1">
                            <span>m</span>
                            <input type="number" id="pulse-on-sec" min="0" max="59" value="0">
                            <span>s</span>
                        </div>
                    </div>
                    <div class="form-group">
                        <label>Pause OFF (Rest Time)</label>
                        <div class="pulse-input-pair">
                            <input type="number" id="pulse-off-min" min="0" max="120" value="2">
                            <span>m</span>
                            <input type="number" id="pulse-off-sec" min="0" max="59" value="0">
                            <span>s</span>
                        </div>
                    </div>
                </div>
            </div>

            <button class="btn" style="margin-top: 4px;" onclick="saveRelaySchedule()">💾 Save Relay Schedule</button>

            <!-- Presets Management -->
            <div style="border-top: 1px solid var(--border-color); padding-top: 14px; margin-top: 10px; display: flex; flex-direction: column; gap: 10px;">
                <label>Schedule Presets</label>
                <div style="display: flex; gap: 8px; flex-wrap: wrap; align-items: center;">
                    <select id="preset-select" style="flex: 1; min-width: 200px;" onchange="onPresetSelected(this.value)">
                        <!-- Populated dynamically via API -->
                    </select>
                    <button class="btn btn-sm" onclick="applyPreset()">Apply</button>
                    <button class="btn btn-sm btn-secondary" onclick="openSavePresetModal()">Save Preset</button>
                    <button class="btn btn-sm btn-danger" id="btn-delete-preset" style="display: none;" onclick="deletePreset()">Delete</button>
                    <button class="btn btn-sm btn-secondary" style="color: #f87171;" onclick="resetToDefaults()">Factory Reset</button>
                </div>
            </div>
        </div>

        <!-- 4. DIGITAL INPUTS & SENSORS (I/O) -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">4. Digital Inputs & Sensors</div>
            </div>
            <div class="compact-grid">
                <div class="stat-block">
                    <span class="stat-label" id="lbl-gpio0">GPIO 0 (Power Sensor)</span>
                    <span class="stat-val" id="input-gpio0"><span class="status-dot dot-orange"></span> ...</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label" id="lbl-gpio4">GPIO 4 (Aux/Free)</span>
                    <span class="stat-val" id="input-gpio4"><span class="status-dot dot-orange"></span> ...</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label" id="lbl-gpio2">GPIO 2 (Aux/Free)</span>
                    <span class="stat-val" id="input-gpio2"><span class="status-dot dot-orange"></span> ...</span>
                </div>
                <div class="stat-block">
                    <span class="stat-label" id="lbl-gpio15">GPIO 15 (Aux/Free)</span>
                    <span class="stat-val" id="input-gpio15"><span class="status-dot dot-orange"></span> ...</span>
                </div>
            </div>
            
            <!-- Analog Light Sensor Live Bar -->
            <div style="margin-top: 8px; display: flex; flex-direction: column; gap: 6px;">
                <div style="display: flex; justify-content: space-between; font-size: 13px;">
                    <span id="lbl-analog">Light Sensor (A0)</span>
                    <span id="light-percent-label" style="font-weight: 700; color: var(--accent-primary)">0%</span>
                </div>
                <div style="background: rgba(255,255,255,0.06); border-radius: 8px; height: 12px; overflow: hidden;">
                    <div id="light-meter-bar" style="height: 100%; width: 0%; background: linear-gradient(90deg, #0284c7, #38bdf8, #fbbf24); border-radius: 8px; transition: width 0.4s ease;"></div>
                </div>
            </div>
        </div>

        <!-- 5. SYSTEM ADMINISTRATION -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">5. System Administration</div>
            </div>
            <div style="display: flex; gap: 10px; flex-wrap: wrap;">
                <button class="btn btn-secondary" onclick="openNamesModal()">✏️ Customize I/O Names</button>
                <button class="btn btn-secondary" onclick="openWifiModal()">📶 WiFi Setup</button>
                <button class="btn" onclick="window.open('/update', '_blank')">⬆️ Firmware Update (OTA)</button>
            </div>
        </div>

        <!-- 6. EVENT HISTORY (TIMELINE) -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">6. Event History</div>
                <button class="btn btn-secondary btn-sm" onclick="fetchHistory()">Refresh</button>
            </div>
            <div class="timeline-container" id="timeline-container">
                <p style="font-size: 13px; color: var(--text-secondary)">Loading event history...</p>
            </div>
        </div>

        <!-- Modal Customize I/O Names -->
        <div id="names-modal" class="modal">
            <div class="modal-content">
                <div class="modal-header">
                    <h2>Customize Peripheral Names</h2>
                    <span class="close-btn" onclick="closeNamesModal()">&times;</span>
                </div>
                <div class="form-group">
                    <label>Relay 1 Name (GPIO 16)</label>
                    <input type="text" id="name-r1" maxlength="31">
                </div>
                <div class="form-group">
                    <label>Relay 2 Name (GPIO 14)</label>
                    <input type="text" id="name-r2" maxlength="31">
                </div>
                <div class="form-group">
                    <label>Relay 3 Name (GPIO 12)</label>
                    <input type="text" id="name-r3" maxlength="31">
                </div>
                <div class="form-group">
                    <label>Relay 4 Name (GPIO 13)</label>
                    <input type="text" id="name-r4" maxlength="31">
                </div>
                <div class="form-group">
                    <label>GPIO 0 Input Name</label>
                    <input type="text" id="name-in0" maxlength="31">
                </div>
                <div class="form-group">
                    <label>GPIO 4 Input Name</label>
                    <input type="text" id="name-in4" maxlength="31">
                </div>
                <div class="form-group">
                    <label>GPIO 2 Input Name</label>
                    <input type="text" id="name-in2" maxlength="31">
                </div>
                <div class="form-group">
                    <label>GPIO 15 Input Name</label>
                    <input type="text" id="name-in15" maxlength="31">
                </div>
                <div class="form-group">
                    <label>Analog Sensor (A0) Name</label>
                    <input type="text" id="name-a0" maxlength="31">
                </div>
                <button class="btn" onclick="saveCustomNames()">💾 Save Names</button>
            </div>
        </div>

        <!-- Modal Save New Preset -->
        <div id="save-preset-modal" class="modal">
            <div class="modal-content" style="max-width: 360px;">
                <div class="modal-header">
                    <h2>Save New Preset</h2>
                    <span class="close-btn" onclick="closeSavePresetModal()">&times;</span>
                </div>
                <div class="form-group">
                    <label for="new-preset-name">Preset Name</label>
                    <input type="text" id="new-preset-name" placeholder="e.g. Summer Schedule, Maintenance..." maxlength="31">
                </div>
                <button class="btn" onclick="confirmSavePreset()">Save Preset</button>
            </div>
        </div>

        <!-- Modal WiFi Setup -->
        <div id="wifi-modal" class="modal">
            <div class="modal-content" style="max-width: 380px;">
                <div class="modal-header">
                    <h2>WiFi Setup</h2>
                    <span class="close-btn" onclick="closeWifiModal()">&times;</span>
                </div>
                <div class="form-group">
                    <label for="wifi-ssid">Network SSID</label>
                    <input type="text" id="wifi-ssid" placeholder="Local WiFi Network Name">
                </div>
                <div class="form-group">
                    <label for="wifi-pass">Password</label>
                    <input type="password" id="wifi-pass" placeholder="••••••••">
                </div>
                <button class="btn" onclick="saveWifiAndClose()">Connect Device</button>
            </div>
        </div>

        <footer>
            Aquatlantis Smart Aquarium Controller • ESP8266 ESP-12F
        </footer>
    </div>

    <div class="toast" id="toast">Notification</div>

    <script>
        let schedules = [];
        let presetsList = [];
        let currentNames = null;
        let activePresetId = 1;

        function showToast(msg) {
            const t = document.getElementById('toast');
            if (!t) return;
            t.innerText = msg;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 3500);
        }

        function formatUptime(sec) {
            const h = Math.floor(sec / 3600);
            const m = Math.floor((sec % 3600) / 60);
            const s = sec % 60;
            return `${h}h ${m}m ${s}s`;
        }

        function markScheduleModified() {
            if (activePresetId !== 0) {
                activePresetId = 0;
                fetchPresets();
            }
        }

        function initHourGrid() {
            const grid = document.getElementById('hour-grid');
            if (!grid) return;
            grid.innerHTML = '';
            for (let i = 0; i < 24; i++) {
                const cell = document.createElement('div');
                cell.className = 'hour-cell';
                cell.innerText = i.toString().padStart(2, '0');
                cell.dataset.hour = i;
                cell.onclick = () => {
                    cell.classList.toggle('active');
                    markScheduleModified();
                };
                grid.appendChild(cell);
            }
        }

        function onRelaySelected(relayNum) {
            const sched = schedules.find(s => s.num == relayNum);
            if (!sched) return;

            document.getElementById('relay-behavior').value = sched.behavior;
            onBehaviorChanged(sched.behavior);

            // Separate min and sec
            document.getElementById('pulse-on-min').value = Math.floor(sched.pulse_on / 60);
            document.getElementById('pulse-on-sec').value = sched.pulse_on % 60;
            document.getElementById('pulse-off-min').value = Math.floor(sched.pulse_off / 60);
            document.getElementById('pulse-off-sec').value = sched.pulse_off % 60;

            const cells = document.querySelectorAll('.hour-cell');
            cells.forEach(cell => {
                const hr = parseInt(cell.dataset.hour);
                const isActive = (sched.active_hours & (1 << hr)) !== 0;
                if (isActive) cell.classList.add('active');
                else cell.classList.remove('active');
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
                    container.innerHTML = '<p style="font-size: 13px; color: var(--text-secondary)">No event logs recorded yet.</p>';
                    return;
                }
                
                data.reverse().forEach(event => {
                    let t = event.time || '';
                    // Clean legacy dates (e.g. "2026-09-27 19:11:08" -> "27.09 19:11:08")
                    if (/^\d{4}[-/]\d{2}[-/]\d{2}/.test(t)) {
                        const parts = t.split(' ');
                        const dp = parts[0].split(/[-/]/);
                        t = `${dp[2]}.${dp[1]}${parts[1] ? ' ' + parts[1] : ''}`;
                    }
                    const item = document.createElement('div');
                    item.className = 'timeline-item';
                    item.innerHTML = `
                        <span class="timeline-time">${t}</span>
                        <span class="timeline-msg">${event.msg}</span>
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
                const currentRelay = document.getElementById('relay-select').value;
                onRelaySelected(currentRelay);
            } catch (err) {
                console.error("Error fetching schedules:", err);
            }
        }

        async function fetchPresets() {
            try {
                const res = await fetch('/api/presets');
                presetsList = await res.json();
                const sel = document.getElementById('preset-select');
                if (!sel) return;
                sel.innerHTML = '';
                
                // If active schedule is modified/custom (id == 0), include the custom entry
                if (activePresetId === 0) {
                    const optCustom = document.createElement('option');
                    optCustom.value = '0';
                    optCustom.innerText = '⚙️ Custom Schedule (Modified)';
                    sel.appendChild(optCustom);
                }

                presetsList.forEach(p => {
                    const opt = document.createElement('option');
                    opt.value = p.id;
                    opt.innerText = p.name;
                    sel.appendChild(opt);
                });

                if (activePresetId !== undefined && activePresetId !== null) {
                    sel.value = activePresetId;
                }
                onPresetSelected(sel.value);
            } catch (err) {
                console.error("Error fetching presets:", err);
            }
        }

        function onPresetSelected(val) {
            const p = presetsList.find(x => x.id == val);
            const btnDel = document.getElementById('btn-delete-preset');
            if (btnDel) {
                if (p && !p.builtin) {
                    btnDel.style.display = 'inline-block';
                } else {
                    btnDel.style.display = 'none';
                }
            }
        }

        async function fetchStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                document.getElementById('mode-val').innerText = data.mode;
                let timeStr = data.time || '';
                if (timeStr.indexOf(' ') !== -1) {
                    timeStr = timeStr.substring(timeStr.indexOf(' ') + 1);
                }
                document.getElementById('time-val').innerText = timeStr;
                document.getElementById('uptime-val').innerText = formatUptime(data.uptime);
                document.getElementById('ssid-val').innerText = data.wifi_ssid;
                document.getElementById('rssi-val').innerText = data.wifi_rssi + ' dBm';
                document.getElementById('ip-val').innerText = data.ip;
                
                // Update light meter in Section 4
                const lightVal = document.getElementById('light-val');
                if (lightVal) lightVal.innerText = data.light_percent + '%';
                const lightBar = document.getElementById('light-meter-bar');
                if (lightBar) lightBar.style.width = data.light_percent + '%';
                const lightPctLbl = document.getElementById('light-percent-label');
                if (lightPctLbl) lightPctLbl.innerText = data.light_percent + '%';
                
                // Track active preset
                if (data.active_preset !== undefined && data.active_preset !== activePresetId) {
                    activePresetId = data.active_preset;
                    fetchPresets();
                }

                // Feed mode banner
                const feedBanner = document.getElementById('feed-banner');
                if (data.feed_mode) {
                    feedBanner.classList.add('show');
                    const rem = data.feed_remaining || 0;
                    const m = Math.floor(rem / 60);
                    const s = rem % 60;
                    document.getElementById('feed-timer').innerText = `${m}:${s < 10 ? '0' : ''}${s}`;
                } else {
                    feedBanner.classList.remove('show');
                }

                // Digital Inputs
                const updateInputDot = (id, state) => {
                    const el = document.getElementById(id);
                    if (state) el.innerHTML = '<span class="status-dot dot-green"></span> HIGH (3.3V)';
                    else el.innerHTML = '<span class="status-dot dot-red"></span> LOW (GND)';
                };
                updateInputDot('input-gpio0', data.inputs.gpio0);
                updateInputDot('input-gpio4', data.inputs.gpio4);
                updateInputDot('input-gpio2', data.inputs.gpio2);
                updateInputDot('input-gpio15', data.inputs.gpio15);

                // Update input custom labels
                if (data.names) {
                    currentNames = data.names;
                    if (data.names.inputs) {
                        document.getElementById('lbl-gpio0').innerText = data.names.inputs[0];
                        document.getElementById('lbl-gpio4').innerText = data.names.inputs[1];
                        document.getElementById('lbl-gpio2').innerText = data.names.inputs[2];
                        document.getElementById('lbl-gpio15').innerText = data.names.inputs[3];
                    }
                    if (data.names.analog) {
                        const lblA0 = document.getElementById('lbl-analog');
                        if (lblA0) lblA0.innerText = data.names.analog;
                    }
                    // Update relay selector options guarded against mobile picker flickering
                    const sel = document.getElementById('relay-select');
                    if (sel && document.activeElement !== sel && data.names.relays) {
                        for (let i = 0; i < 4; i++) {
                            if (sel.options[i] && data.names.relays[i]) {
                                const expectedText = `Relay ${i + 1}: ${data.names.relays[i]}`;
                                if (sel.options[i].text !== expectedText) {
                                    sel.options[i].text = expectedText;
                                }
                            }
                        }
                    }
                }

                // WiFi status badge
                const wifiDot = document.getElementById('wifi-dot');
                const wifiText = document.getElementById('wifi-status-text');
                wifiText.innerText = data.wifi_status;
                wifiDot.className = 'status-dot';
                if (data.wifi_status === 'Connected') wifiDot.classList.add('dot-green');
                else wifiDot.classList.add('dot-red');

                // Relays card rendering (Variant C: Two rows, compact interactive badges)
                const container = document.getElementById('relays-container');
                container.innerHTML = '';
                
                data.relays.forEach(relay => {
                    const item = document.createElement('div');
                    item.className = 'relay-item';
                    const gpioPin = relay.num === 1 ? 16 : relay.num === 2 ? 14 : relay.num === 3 ? 12 : 13;
                    
                    let bannerClass = 'status-idle';
                    const desc = relay.status_desc || (relay.state ? 'Running' : 'Inactive');
                    if (relay.override) {
                        bannerClass = 'status-warn';
                    } else if (relay.state) {
                        bannerClass = 'status-active';
                    } else if (desc.includes('Pause') || desc.includes('pause') || desc.includes('Feed')) {
                        bannerClass = 'status-paused';
                    }
                    
                    const isAuto = !relay.override;
                    const autoClass = isAuto ? 'btn-badge-auto active' : 'btn-badge-auto inactive';
                    
                    let powerClass = 'btn-badge-power neutral';
                    let powerLabel = '⏻';
                    if (relay.override) {
                        if (relay.state) {
                            powerClass = 'btn-badge-power forced-on';
                            powerLabel = '⏻ ON';
                        } else {
                            powerClass = 'btn-badge-power forced-off';
                            powerLabel = '⏻ OFF';
                        }
                    }
                    
                    item.innerHTML = `
                        <div class="relay-header">
                            <div style="display: flex; align-items: center; gap: 8px;">
                                <span class="relay-name">${relay.name}</span>
                                <span class="relay-gpio-tag">GPIO ${gpioPin}</span>
                            </div>
                            <div class="relay-action-badges">
                                <button class="btn-badge ${autoClass}" onclick="setRelayAuto(${relay.num})" title="Switch to Automatic Schedule">AUTO</button>
                                <button class="btn-badge ${powerClass}" onclick="toggleRelayPower(${relay.num}, ${relay.state ? 1 : 0}, ${relay.override ? 1 : 0})" title="Toggle Force ON / OFF">${powerLabel}</button>
                            </div>
                        </div>
                        <div class="relay-status-banner ${bannerClass}">
                            ${desc}
                        </div>
                    `;
                    container.appendChild(item);
                });
            } catch (err) {
                console.error("Error fetching status:", err);
            }
        }

        async function setRelayAuto(num) {
            try {
                const res = await fetch('/api/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${num}&override=0`
                });
                if (res.ok) {
                    showToast(`Relay ${num} returned to Auto schedule`);
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function toggleRelayPower(num, currentState, isOverride) {
            try {
                const targetState = isOverride ? (currentState ? 0 : 1) : (currentState ? 0 : 1);
                const res = await fetch('/api/override', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `relay=${num}&override=1&state=${targetState}`
                });
                if (res.ok) {
                    showToast(`Relay ${num} forced ${targetState ? 'ON' : 'OFF'}`);
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
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
                    showToast("All peripherals returned to Auto schedule.");
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function saveRelaySchedule() {
            const relayNum = document.getElementById('relay-select').value;
            const behavior = document.getElementById('relay-behavior').value;
            
            const onMin = parseInt(document.getElementById('pulse-on-min').value) || 0;
            const onSecPart = parseInt(document.getElementById('pulse-on-sec').value) || 0;
            const pulseOn = Math.max(1, onMin * 60 + onSecPart);

            const offMin = parseInt(document.getElementById('pulse-off-min').value) || 0;
            const offSecPart = parseInt(document.getElementById('pulse-off-sec').value) || 0;
            const pulseOff = Math.max(1, offMin * 60 + offSecPart);

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
                    showToast(`Relay ${relayNum} schedule saved and confirmed!`);
                    await fetchSchedules();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Error saving relay schedule!");
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function applyPreset() {
            const pId = document.getElementById('preset-select').value;
            if (pId == 0) return;
            if (!confirm(`Are you sure you want to apply this preset? It will overwrite current relay schedules.`)) return;
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `apply=${pId}`
                });
                if (res.ok) {
                    showToast("Preset applied successfully!");
                    await fetchSchedules();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Error applying preset!");
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        function openSavePresetModal() {
            document.getElementById('save-preset-modal').classList.add('show');
            document.getElementById('new-preset-name').value = '';
            document.getElementById('new-preset-name').focus();
        }
        function closeSavePresetModal() {
            document.getElementById('save-preset-modal').classList.remove('show');
        }

        async function confirmSavePreset() {
            const name = document.getElementById('new-preset-name').value.trim();
            if (!name) {
                showToast("Please enter a name for the preset!");
                return;
            }
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `save=1&name=${encodeURIComponent(name)}`
                });
                if (res.ok) {
                    showToast(`Preset "${name}" saved!`);
                    closeSavePresetModal();
                    await fetchPresets();
                    await fetchHistory();
                } else {
                    showToast("Error saving preset!");
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function deletePreset() {
            const pId = document.getElementById('preset-select').value;
            if (!confirm("Are you sure you want to delete this custom preset?")) return;
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `delete=${pId}`
                });
                if (res.ok) {
                    showToast("Preset deleted!");
                    await fetchPresets();
                    await fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function resetToDefaults() {
            if (!confirm("Are you sure you want to reset all relays to factory defaults?")) return;
            try {
                const res = await fetch('/api/presets', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'reset=1'
                });
                if (res.ok) {
                    showToast("Relay schedules reset to factory defaults!");
                    await fetchSchedules();
                    await fetchStatus();
                    await fetchHistory();
                }
            } catch (err) {
                showToast("Communication error!");
            }
        }

        async function startFeedMode() {
            try {
                const res = await fetch('/api/feed', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'action=start&duration=600'
                });
                if (res.ok) {
                    showToast("Feed Mode started for 10 minutes!");
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Error!");
            }
        }

        async function stopFeedMode() {
            try {
                const res = await fetch('/api/feed', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: 'action=stop'
                });
                if (res.ok) {
                    showToast("Feed Mode cancelled. Resumed schedule.");
                    fetchStatus();
                    fetchHistory();
                }
            } catch (err) {
                showToast("Error!");
            }
        }

        function openNamesModal() {
            document.getElementById('names-modal').classList.add('show');
            if (currentNames) {
                if (currentNames.relays) {
                    document.getElementById('name-r1').value = currentNames.relays[0] || '';
                    document.getElementById('name-r2').value = currentNames.relays[1] || '';
                    document.getElementById('name-r3').value = currentNames.relays[2] || '';
                    document.getElementById('name-r4').value = currentNames.relays[3] || '';
                }
                if (currentNames.inputs) {
                    document.getElementById('name-in0').value = currentNames.inputs[0] || '';
                    document.getElementById('name-in4').value = currentNames.inputs[1] || '';
                    document.getElementById('name-in2').value = currentNames.inputs[2] || '';
                    document.getElementById('name-in15').value = currentNames.inputs[3] || '';
                }
                if (currentNames.analog) {
                    document.getElementById('name-a0').value = currentNames.analog || '';
                }
            }
        }

        function closeNamesModal() {
            document.getElementById('names-modal').classList.remove('show');
        }

        async function saveCustomNames() {
            const body = new URLSearchParams({
                r1: document.getElementById('name-r1').value.trim(),
                r2: document.getElementById('name-r2').value.trim(),
                r3: document.getElementById('name-r3').value.trim(),
                r4: document.getElementById('name-r4').value.trim(),
                in0: document.getElementById('name-in0').value.trim(),
                in4: document.getElementById('name-in4').value.trim(),
                in2: document.getElementById('name-in2').value.trim(),
                in15: document.getElementById('name-in15').value.trim(),
                a0: document.getElementById('name-a0').value.trim()
            });

            try {
                const res = await fetch('/api/names', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: body.toString()
                });
                if (res.ok) {
                    showToast("Peripheral names saved!");
                    closeNamesModal();
                    await fetchStatus();
                    await fetchHistory();
                } else {
                    showToast("Error saving names!");
                }
            } catch (err) {
                showToast("Communication error!");
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
            const ssid = document.getElementById('wifi-ssid').value.trim();
            const pass = document.getElementById('wifi-pass').value;
            if (!ssid) {
                showToast("SSID cannot be empty!");
                return;
            }
            try {
                showToast("Sending WiFi credentials. Reconnecting...");
                await fetch('/api/wifi', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
                });
                closeWifiModal();
            } catch (err) {
                showToast("Error!");
            }
        }

        // Init UI
        initHourGrid();
        fetchStatus();
        fetchSchedules();
        fetchPresets();
        fetchHistory();

        // Polling every 2s
        setInterval(() => {
            fetchStatus();
            fetchHistory();
        }, 2000);
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
    r += "\"feed_mode\":" + String(isFeedModeActive() ? "true" : "false") + ",";
    r += "\"feed_remaining\":" + String(getFeedModeRemainingSec()) + ",";
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
    r += "\"active_preset\":" + String(getActivePresetId()) + ",";
    
    // Relay items
    r += "\"relays\":[";
    for (int i = 1; i <= 4; i++) {
        RelayState s = getRelayState(i);
        r += "{";
        r += "\"num\":" + String(i) + ",";
        r += "\"name\":\"" + getRelayCustomName(i) + "\",";
        r += "\"state\":" + String(s.physicalState ? "true" : "false") + ",";
        r += "\"override\":" + String(s.manualOverride ? "true" : "false") + ",";
        r += "\"override_state\":" + String(s.manualState ? "true" : "false") + ",";
        r += "\"status_desc\":\"" + getRelayStatusDescription(i) + "\"";
        r += "}";
        if (i < 4) r += ",";
    }
    r += "],";
    
    // Custom names
    r += "\"names\":" + getIONamesJSON();
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

// REST API endpoint: Presets management
void handlePresets() {
    if (server.hasArg("apply")) {
        int presetNum = server.arg("apply").toInt();
        if (presetNum >= 1 && presetNum <= 3) {
            applyPreset(presetNum);
            server.send(200, "application/json", "{\"success\":true}");
            return;
        } else if (presetNum >= 100) {
            bool ok = applyUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true}");
                return;
            }
        }
    } else if (server.hasArg("save") && server.hasArg("name")) {
        String name = server.arg("name");
        name.trim();
        if (name.length() > 0) {
            int id = saveUserPreset(name);
            server.send(200, "application/json", "{\"success\":true,\"id\":" + String(id) + "}");
            return;
        }
    } else if (server.hasArg("delete")) {
        int presetNum = server.arg("delete").toInt();
        if (presetNum >= 100) {
            bool ok = deleteUserPreset(presetNum);
            if (ok) {
                server.send(200, "application/json", "{\"success\":true}");
                return;
            }
        }
    } else if (server.hasArg("reset") && server.arg("reset") == "1") {
        resetSettingsToDefault();
        server.send(200, "application/json", "{\"success\":true}");
        return;
    }
    server.send(400, "text/plain", "Bad Request");
}

// REST API endpoint: Custom IO names management
void handleNamesSet() {
    CustomIONames names = getCustomIONames();
    
    if (server.hasArg("r1")) strncpy(names.relays[0], server.arg("r1").c_str(), 31);
    if (server.hasArg("r2")) strncpy(names.relays[1], server.arg("r2").c_str(), 31);
    if (server.hasArg("r3")) strncpy(names.relays[2], server.arg("r3").c_str(), 31);
    if (server.hasArg("r4")) strncpy(names.relays[3], server.arg("r4").c_str(), 31);
    
    if (server.hasArg("in0")) strncpy(names.digitalInputs[0], server.arg("in0").c_str(), 31);
    if (server.hasArg("in4")) strncpy(names.digitalInputs[1], server.arg("in4").c_str(), 31);
    if (server.hasArg("in2")) strncpy(names.digitalInputs[2], server.arg("in2").c_str(), 31);
    if (server.hasArg("in15")) strncpy(names.digitalInputs[3], server.arg("in15").c_str(), 31);
    
    if (server.hasArg("a0")) strncpy(names.analogInput, server.arg("a0").c_str(), 31);
    
    setCustomIONames(names);
    server.send(200, "application/json", "{\"success\":true}");
}

// REST API endpoint: Feed mode control
void handleFeed() {
    if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "start") {
            uint32_t dur = server.hasArg("duration") ? server.arg("duration").toInt() : 600;
            setFeedMode(true, dur);
            server.send(200, "application/json", "{\"success\":true,\"active\":true}");
            return;
        } else if (action == "stop") {
            setFeedMode(false);
            server.send(200, "application/json", "{\"success\":true,\"active\":false}");
            return;
        }
    }
    server.send(400, "text/plain", "Bad Request");
}

void setup() {
    // Start Serial debug port
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n========================================");
    Serial.println("Aquatlantis Smart Aquarium Controller v3.0");
    Serial.println("========================================");
    
    // Setup inputs
    pinMode(PIN_INPUT_GPIO0, INPUT);
    pinMode(PIN_INPUT_GPIO4, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO2, INPUT_PULLUP);
    pinMode(PIN_INPUT_GPIO15, INPUT);
    Serial.println("[Init] GPIO pins configured.");
    
    // Initialize Submodules (loads settings, names, presets, and logs boot event)
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
    server.on("/api/presets", HTTP_GET, []() {
        server.send(200, "application/json", getPresetsJSON());
    });
    server.on("/api/presets", HTTP_POST, handlePresets);
    server.on("/api/names", HTTP_GET, []() {
        server.send(200, "application/json", getIONamesJSON());
    });
    server.on("/api/names", HTTP_POST, handleNamesSet);
    server.on("/api/feed", HTTP_POST, handleFeed);
    
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not Found");
    });
    
    // Start Web Server
    server.begin();
    Serial.println("[Init] HTTP server started on port 80.");
    
    // Start mDNS responder (acvariu.local)
    if (MDNS.begin("acvariu")) {
        Serial.println("[mDNS] Started successfully. Access via http://acvariu.local/");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("[mDNS] Error starting responder.");
    }

    // Start NetBIOS responder (Windows http://acvariu/ native resolution)
    NBNS.begin("ACVARIU");
    Serial.println("[NetBIOS] Started. Access via http://acvariu/ on Windows.");

    // Start LLMNR responder (Link-Local Multicast Name Resolution for Windows 10/11)
    LLMNR.begin("acvariu");
    Serial.println("[LLMNR] Started. Resolves http://acvariu/ on Windows 10/11.");

    // Start SSDP responder (Shows up in Windows Explorer -> Network)
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("Aquatlantis Smart Aquarium");
    SSDP.setSerialNumber("ESP8266-AQUARIUM-01");
    SSDP.setURL("/");
    SSDP.setModelName("ESP-12F Aquarium Controller");
    SSDP.setManufacturer("Aquatlantis");
    SSDP.begin();
    server.on("/description.xml", HTTP_GET, [](){
        SSDP.schema(server.client());
    });
    Serial.println("[SSDP] Windows UPnP Network Discovery registered.");
    
    // Setup Custom OTA Update with event logging and LittleFS post-flash detection
    server.on("/update", HTTP_GET, []() {
        if (!server.authenticate(OTA_USER, OTA_PASS)) {
            return server.requestAuthentication();
        }
        String html = "<!DOCTYPE html><html><head><title>Aquatlantis OTA Update</title><meta name='viewport' content='width=device-width, initial-scale=1'></head>"
                      "<body style='font-family:sans-serif;background:#080d1a;color:#fff;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;'>"
                      "<div style='background:#151d30;padding:26px;border-radius:12px;border:1px solid rgba(255,255,255,0.1);max-width:400px;width:100%;text-align:center;'>"
                      "<h2 style='margin-top:0;color:#38bdf8;'>Aquatlantis OTA Update</h2>"
                      "<p style='color:#94a3b8;font-size:14px;'>Upload compiled firmware.bin binary:</p>"
                      "<form method='POST' action='/update' enctype='multipart/form-data' style='display:flex;flex-direction:column;gap:16px;margin-top:20px;'>"
                      "<input type='file' name='firmware' accept='.bin' style='color:#fff;background:rgba(255,255,255,0.05);padding:10px;border-radius:6px;border:1px solid rgba(255,255,255,0.1);'>"
                      "<input type='submit' value='Flash Firmware' style='background:#38bdf8;color:#080d1a;border:none;padding:12px;border-radius:6px;font-weight:700;cursor:pointer;'>"
                      "</form>"
                      "<p style='margin-top:20px;'><a href='/' style='color:#38bdf8;text-decoration:none;'>&larr; Back to Dashboard</a></p>"
                      "</div></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, []() {
        if (!server.authenticate(OTA_USER, OTA_PASS)) {
            return server.requestAuthentication();
        }
        if (Update.hasError()) {
            server.send(200, "text/html", "Update error: " + String(Update.getError()));
            logSystemEvent("OTA update failed (Error: " + String(Update.getError()) + ")");
        } else {
            server.client().setNoDelay(true);
            server.send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='12;URL=/'><title>Success</title></head><body style='font-family:sans-serif;background:#080d1a;color:#10b981;text-align:center;padding:50px;'><h2>OTA Update Successful!</h2><p style='color:#fff;'>Device is rebooting. Redirecting in 12 seconds...</p></body></html>");
            delay(150);
            server.client().stop();
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            if (!server.authenticate(OTA_USER, OTA_PASS)) return;
            WiFiUDP::stopAll();
            Serial.printf("[OTA] Flash started: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, U_FLASH)) {
                Update.printError(Serial);
                logSystemEvent("OTA update failed: Insufficient space");
            } else {
                logSystemEvent("OTA update initiated (" + upload.filename + ")");
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Flash successful: %u bytes\n", upload.totalSize);
                float kb = (float)upload.totalSize / 1024.0f;
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f KB", kb);
                logSystemEvent("OTA update completed (" + String(buf) + ") • Rebooting");
                
                File f = LittleFS.open("/flash_done.flag", "w");
                if (f) {
                    f.print(upload.filename + " [" + String(buf) + "]");
                    f.close();
                }
            } else {
                Update.printError(Serial);
                logSystemEvent("OTA update failed during finalization");
            }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            Update.end();
            logSystemEvent("OTA update aborted by user");
        }
    });
    Serial.println("[Init] OTA web updater registered on /update with logging hooks.");
}

void loop() {
    // 1. Maintain background tasks (NTP sync checks, connection updates, status LED blink)
    updateNetwork();
    
    // Detect reconnection after OTA flash
    static bool flashReconnectedLogged = false;
    if (!flashReconnectedLogged && WiFi.status() == WL_CONNECTED && isTimeSynced()) {
        flashReconnectedLogged = true;
        if (LittleFS.exists("/flash_done.flag")) {
            File f = LittleFS.open("/flash_done.flag", "r");
            String flashInfo = "firmware.bin";
            if (f) {
                flashInfo = f.readString();
                flashInfo.trim();
                f.close();
            }
            LittleFS.remove("/flash_done.flag");
            logSystemEvent("New firmware active • Connected to WiFi (" + getIPAddress() + ")");
        }
    }
    
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

    // 4. Smart Diagnostic: Detect Main Light (Relay 1) physical failure
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
                logSystemEvent("Warning: " + getRelayCustomName(1) + " is ON, but sensor reads under 15%");
                lampDefectLogged = true;
            }
        }
    } else {
        lampTurnedOnTime = 0;
        lampDefectLogged = false;
    }
    
    // 5. Web requests handler
    server.handleClient();
    
    // 6. Periodic serial logger (every 5 seconds) to aid deployment diagnostics
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
            
            Serial.printf("      Relay %d [%s]: %s | Mode: %s | Behavior: %s\n",
                          i,
                          getRelayCustomName(i).c_str(),
                          s.physicalState ? "ON" : "OFF",
                          modeStr.c_str(),
                          behaviorStr.c_str());
        }
        Serial.println("-----------------------------------------------------------------");
    }
    
    // Tiny delay to yield to the ESP8266 background processes (prevent watchdog timeouts)
    delay(5);
}
